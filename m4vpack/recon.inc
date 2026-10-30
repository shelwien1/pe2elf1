// recon.inc -- the decoder in the loop of level -9: m4vdec's reconstruction of the pictures (a
// port of FFmpeg's MPEG-4 Part 2 decoder, bit-exact with its C code), driven by the records the
// model codes instead of the bits, and by the vectors of the motion state (motion.inc), which the
// model keeps at every level. The model calls it as it codes a macroblock: at its start
// (mb_begin), once its kind and vectors are known (mb_inter, mb_intra: the motion compensated
// prediction is then in the picture), after each block's coefficients (block: the block
// reconstructed), and at its end (mb_end). So when a block is coded, the samples above and left of
// it are decoded, and its own prediction is there: what the contexts of level -9 look at
// (reconctx.inc).
//
// Compressing and decompressing run the same calls on the same records, so the pictures are the
// same on both sides whatever they are; m4vdec's are what they aim at (M4VPACK_RECON=file writes
// them in coding order, for comparing with a decoder; a run on one thread).

// the encoders' bugs FFmpeg works around (FF_BUG_*; its FF_BUG_AMV, which m4vdec never sets, the
// motion state leaves out)
enum {
  BUG_QPEL_CHROMA = 64, BUG_STD_QPEL = 128, BUG_QPEL_CHROMA2 = 256, BUG_DIRECT_BLOCKSIZE = 512, BUG_EDGE = 1024,
  BUG_HPEL_CHROMA = 2048, BUG_DC_CLIP = 4096, BUG_IEDGE = 32768
};

enum { OP_PUT = 0, OP_PUT_NORND = 1, OP_AVG = 2 };

// the samples (8 bits): their largest, mid gray (a missing reference's); what the DC predictors
// start from (mid gray, times the DC scale of 8)
constexpr int PIX_MAX = UINT8_MAX, GRAY = 1 << (BYTE_BITS - 1);
constexpr int DC_RESET = 1024;

// an intra DC as reconstructed: 11 bits, clipped (unless BUG_DC_CLIP)
constexpr int DC_MAX = 2047;
// a chroma vector from the sum of a 4MV macroblock's four (FFmpeg's ff_h263_round_chroma): the
// sum's low bits rounded by the table, the rest shifted
constexpr int CHROMA_ROUND_SHIFT = 3;

static const U8 y_dc_scale_table[QS_MAX + 1] = { 0,  8,  8,  8,  8,  10, 12, 14, 16, 17, 18, 19, 20, 21, 22, 23,
                                                 24, 25, 26, 27, 28, 29, 30, 31, 32, 34, 36, 38, 40, 42, 44, 46 };
static const U8 c_dc_scale_table[QS_MAX + 1] = { 0,  8,  8,  8,  8,  9,  9,  10, 10, 11, 11, 12, 12, 13, 13, 14,
                                                 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25 };
static constexpr U8 h263_chroma_roundtab[] = { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 };
static inline int round_chroma(int x) {
  return h263_chroma_roundtab[x & (countof(h263_chroma_roundtab) - 1)] + (x >> CHROMA_ROUND_SHIFT);
}
static inline U8 clip8(int v) { return (U8)clip(v, 0, PIX_MAX); }

// the AC predictors of a block: its left column, then its top row (BLK_W each, the first unused)
constexpr int ACV = 2 * BLK_W;
// the pictures: the current, the last and the next anchor
constexpr int NPICS = 3;

// a picture: its planes (rows of mb_w * MB_W luma and mb_w * BLK_W chroma samples)
typedef struct RPic {
  U8 *y, *u, *v;
} RPic;

typedef struct Recon {
  int on;              // this VOP is reconstructed
  const Motion* mo;    // the vectors (the model's motion state)
  // stream (the layout FFmpeg's: motion.inc)
  int width, height, mb_w, mb_h, mb_num, mb_stride, b8_stride, ly, lc;
  int progressive, mpeg_quant, qpel;
  U16 intra_matrix[BLK_COEFS], inter_matrix[BLK_COEFS];
  int divx_version, divx_build, xvid_build, lavc_build;
  int bugs, idct_xvid, h_edge, v_edge;
  // VOP
  int pict_type, no_rounding, alternate_scan;
  int qscale, chroma_qscale, y_dc_scale, c_dc_scale;
  const U8 *intra_scan, *intra_h_scan, *intra_v_scan;
  // macroblock
  int mb_x, mb_y;
  int mb_intra, ac_pred, interlaced_dct;
  int block_index[MB_BLOCKS], block_wrap[MB_BLOCKS];
  // prediction state (FFmpeg's MpegEncContext's layout)
  S16 *dc_val_base, *dc_val;
  S16 (*ac_val_base)[ACV], (*ac_val)[ACV];
  U8* mbintra_table;
  S8* qscale_table;
  U8* mb_done;
  // pictures
  RPic pics[NPICS];
  RPic *cur, *last, *next;
  U8* mem;                  // all of the above, in one block
  FILE* dump;               // M4VPACK_RECON: the pictures, in coding order
} Recon;

static void rc_set_qscale(Recon* d, int q) {
  q = clip(q, QS_MIN, QS_MAX);
  d->qscale = q;
  d->chroma_qscale = q;
  d->y_dc_scale = y_dc_scale_table[q];
  d->c_dc_scale = c_dc_scale_table[q];
}

// the tables for the motion state's picture size, cleared as freshly allocated ones (m4vdec's
// dec_layout); 0 if there is no memory for them
static int rc_layout(Recon* d) {
  const Motion* o = d->mo;
  int i, pass, y_size, c_size, yc_size, mb_array;
  size_t ysz, csz, at = 0;
  U8* m = 0;
  if (d->mem && d->width == o->width && d->height == o->height) return 1;
  free(d->mem);
  d->mem = 0;
  d->cur = d->last = d->next = 0;
  d->width = o->width;
  d->height = o->height;
  d->mb_w = o->mb_w;
  d->mb_h = o->mb_h;
  d->mb_num = o->mb_num;
  d->mb_stride = o->mb_stride;
  d->b8_stride = o->b8_stride;
  d->ly = d->mb_w * MB_W;
  d->lc = d->mb_w * BLK_W;
  y_size = d->b8_stride * (2 * d->mb_h + 1);
  c_size = d->mb_stride * (d->mb_h + 1);
  yc_size = y_size + 2 * c_size;
  mb_array = d->mb_stride * d->mb_h + MB_PAD;
  ysz = (size_t)d->ly * d->mb_h * MB_W;
  csz = (size_t)d->lc * d->mb_h * BLK_W;
  for (pass = 0; pass < 2; pass++) {  // the size, then the tables in it
    if (pass) {
      if (!(m = (U8*)malloc(at))) return 0;
      memset(m, 0, at);
      d->mem = m;
      at = 0;
    }
    // each table at the next multiple of ALIGN in the block
    constexpr size_t ALIGN = 16;
    auto take = [&](size_t k) -> void* {
      void* q = m + at;
      at = (at + k + ALIGN - 1) & ~(ALIGN - 1);
      return q;
    };
    d->ac_val_base = (S16(*)[ACV])take((size_t)yc_size * ACV * sizeof(S16));
    d->dc_val_base = (S16*)take((size_t)yc_size * sizeof(S16));
    d->mbintra_table = (U8*)take((size_t)mb_array);
    d->qscale_table = (S8*)take((size_t)mb_array);
    d->mb_done = (U8*)take((size_t)mb_array);
    for (i = 0; i < NPICS; i++) {
      RPic* p = &d->pics[i];
      p->y = (U8*)take(ysz + 2 * csz);
      p->u = p->y + ysz;
      p->v = p->u + csz;
    }
  }
  for (i = 0; i < yc_size; i++) d->dc_val_base[i] = DC_RESET;
  d->dc_val = d->dc_val_base + d->b8_stride + 1;
  d->ac_val = d->ac_val_base + d->b8_stride + 1;
  for (i = 0; i < MB_LUMA; i++) d->block_wrap[i] = d->b8_stride;
  d->block_wrap[BLK_CB] = d->block_wrap[BLK_CR] = d->mb_stride;
  return 1;
}

// the blocks' entries in the tables of the 8x8 grid (luma: the motion state's) and of the chroma
// planes' (FFmpeg's ff_init_block_index)
static void rc_set_block_index(Recon* d) {
  const int bs = d->b8_stride, ms = d->mb_stride;
  int i;
  for (i = 0; i < MB_LUMA; i++) d->block_index[i] = d->mo->block_index[i];
  d->block_index[BLK_CB] = ms * (d->mb_y + 1) + bs * d->mb_h * 2 + d->mb_x;
  d->block_index[BLK_CR] = ms * (d->mb_y + d->mb_h + 2) + bs * d->mb_h * 2 + d->mb_x;
}


// the encoders' builds with the bugs FFmpeg works around (h263dec.c's ff_mpeg4_workaround_bugs)
constexpr int DIVX5_VERSION = 500, DIVX502_VERSION = 502, DIVX_QPEL_FIXED = 1814;
constexpr unsigned XVID_QPEL_CHROMA_LAST = 1, XVID_EDGE_LAST = 12, XVID_DC_CLIP_LAST = 32;
constexpr unsigned LAVC_STD_QPEL_FIXED = 4653, LAVC_DIRECT_FIXED = 4655, LAVC_EDGE_FIXED = 4670,
                   LAVC_DC_CLIP_LAST = 4712;
// FF_BUG_IEDGE: libavcodec versions (major << 16 | minor << 8 | micro, micro from 100 on: FFmpeg's)
// between these, but those of the gap
constexpr int LAVC_MICRO_FFMPEG = 100;
constexpr int LAVC_IEDGE_FIRST = 3621476, LAVC_IEDGE_END = 3752552, LAVC_IEDGE_GAP = 3752037,
              LAVC_IEDGE_GAP_END = 3752191;

static void rc_workaround_bugs(Recon* d) {
  if (d->xvid_build >= 0 && d->divx_version >= 0) d->divx_version = d->divx_build = -1;
  // (as in m4vdec, a flag once set stays)
  if (d->divx_version >= DIVX5_VERSION && d->divx_build < DIVX_QPEL_FIXED) d->bugs |= BUG_QPEL_CHROMA;
  if (d->divx_version > DIVX502_VERSION && d->divx_build < DIVX_QPEL_FIXED) d->bugs |= BUG_QPEL_CHROMA2;
  if ((unsigned)d->xvid_build <= XVID_QPEL_CHROMA_LAST) d->bugs |= BUG_QPEL_CHROMA;
  if ((unsigned)d->xvid_build <= XVID_EDGE_LAST) d->bugs |= BUG_EDGE;
  if ((unsigned)d->xvid_build <= XVID_DC_CLIP_LAST) d->bugs |= BUG_DC_CLIP;
  if ((unsigned)d->lavc_build < LAVC_STD_QPEL_FIXED) d->bugs |= BUG_STD_QPEL;
  if ((unsigned)d->lavc_build < LAVC_DIRECT_FIXED) d->bugs |= BUG_DIRECT_BLOCKSIZE;
  if ((unsigned)d->lavc_build < LAVC_EDGE_FIXED) d->bugs |= BUG_EDGE;
  if ((unsigned)d->lavc_build <= LAVC_DC_CLIP_LAST) d->bugs |= BUG_DC_CLIP;
  if ((d->lavc_build & BYTE_MASK) >= LAVC_MICRO_FFMPEG) {
    if (d->lavc_build > LAVC_IEDGE_FIRST && d->lavc_build < LAVC_IEDGE_END &&
        (d->lavc_build < LAVC_IEDGE_GAP || d->lavc_build > LAVC_IEDGE_GAP_END))
      d->bugs |= BUG_IEDGE;
  }
  if (d->divx_version >= 0) d->bugs |= BUG_DIRECT_BLOCKSIZE;
  if ((unsigned)d->divx_version < (unsigned)DIVX5_VERSION) d->bugs |= BUG_EDGE;

  if (d->divx_version >= 0) d->bugs |= BUG_HPEL_CHROMA;
  d->idct_xvid = d->xvid_build >= 0;
}

//--- IDCT (FFmpeg's simple_idct, 8-bit, and the XviD IDCT) ----------------------------------------

// simple_idct's: the basis (cos(k pi/16) sqrt(2) 2^14, IDCT_W4 one less), its passes' shifts, a row of only
// a DC's (which it shifts up by IDCT_DC_SHIFT)
constexpr int IDCT_W1 = 22725, IDCT_W2 = 21407, IDCT_W3 = 19266, IDCT_W4 = 16383, IDCT_W5 = 12873,
            IDCT_W6 = 8867, IDCT_W7 = 4520;
constexpr int IDCT_ROW_SHIFT = 11, IDCT_COL_SHIFT = 20, IDCT_DC_SHIFT = 3;

static void simple_idct_row(S16* row) {
  U32 a0, a1, a2, a3, b0, b1, b2, b3;
  if (!(row[1] | row[2] | row[3] | row[4] | row[5] | row[6] | row[7])) {
    S16 t = (S16)((U32)(row[0] * (1 << IDCT_DC_SHIFT)) & UINT16_MAX);
    row[0] = row[1] = row[2] = row[3] = row[4] = row[5] = row[6] = row[7] = t;
    return;
  }
  a0 = ((U32)IDCT_W4 * row[0]) + (1 << (IDCT_ROW_SHIFT - 1));
  a1 = a0; a2 = a0; a3 = a0;
  a0 += (U32)IDCT_W2 * row[2]; a1 += (U32)IDCT_W6 * row[2]; a2 -= (U32)IDCT_W6 * row[2]; a3 -= (U32)IDCT_W2 * row[2];
  b0 = (U32)(IDCT_W1 * row[1]) + (U32)IDCT_W3 * row[3];
  b1 = (U32)(IDCT_W3 * row[1]) + (U32)-IDCT_W7 * row[3];
  b2 = (U32)(IDCT_W5 * row[1]) + (U32)-IDCT_W1 * row[3];
  b3 = (U32)(IDCT_W7 * row[1]) + (U32)-IDCT_W5 * row[3];
  if (row[4] | row[5] | row[6] | row[7]) {
    a0 += (U32)IDCT_W4 * row[4] + (U32)IDCT_W6 * row[6];
    a1 += (U32)-IDCT_W4 * row[4] - (U32)IDCT_W2 * row[6];
    a2 += (U32)-IDCT_W4 * row[4] + (U32)IDCT_W2 * row[6];
    a3 += (U32)IDCT_W4 * row[4] - (U32)IDCT_W6 * row[6];
    b0 += (U32)IDCT_W5 * row[5] + (U32)IDCT_W7 * row[7];
    b1 += (U32)-IDCT_W1 * row[5] + (U32)-IDCT_W5 * row[7];
    b2 += (U32)IDCT_W7 * row[5] + (U32)IDCT_W3 * row[7];
    b3 += (U32)IDCT_W3 * row[5] + (U32)-IDCT_W1 * row[7];
  }
  row[0] = (S16)((int)(a0 + b0) >> IDCT_ROW_SHIFT);
  row[7] = (S16)((int)(a0 - b0) >> IDCT_ROW_SHIFT);
  row[1] = (S16)((int)(a1 + b1) >> IDCT_ROW_SHIFT);
  row[6] = (S16)((int)(a1 - b1) >> IDCT_ROW_SHIFT);
  row[2] = (S16)((int)(a2 + b2) >> IDCT_ROW_SHIFT);
  row[5] = (S16)((int)(a2 - b2) >> IDCT_ROW_SHIFT);
  row[3] = (S16)((int)(a3 + b3) >> IDCT_ROW_SHIFT);
  row[4] = (S16)((int)(a3 - b3) >> IDCT_ROW_SHIFT);
}

static void simple_idct_col(const S16* col, int out[BLK_W]) {
  U32 a0, a1, a2, a3, b0, b1, b2, b3;
  a0 = (U32)IDCT_W4 * (col[BLK_W * 0] + ((1 << (IDCT_COL_SHIFT - 1)) / IDCT_W4));
  a1 = a0; a2 = a0; a3 = a0;
  a0 += (U32)IDCT_W2 * col[BLK_W * 2]; a1 += (U32)IDCT_W6 * col[BLK_W * 2];
  a2 += (U32)-IDCT_W6 * col[BLK_W * 2]; a3 += (U32)-IDCT_W2 * col[BLK_W * 2];
  b0 = (U32)IDCT_W1 * col[BLK_W * 1]; b1 = (U32)IDCT_W3 * col[BLK_W * 1]; b2 = (U32)IDCT_W5 * col[BLK_W * 1]; b3 = (U32)IDCT_W7 * col[BLK_W * 1];
  b0 += (U32)IDCT_W3 * col[BLK_W * 3]; b1 += (U32)-IDCT_W7 * col[BLK_W * 3]; b2 += (U32)-IDCT_W1 * col[BLK_W * 3]; b3 += (U32)-IDCT_W5 * col[BLK_W * 3];
  a0 += (U32)IDCT_W4 * col[BLK_W * 4]; a1 += (U32)-IDCT_W4 * col[BLK_W * 4]; a2 += (U32)-IDCT_W4 * col[BLK_W * 4]; a3 += (U32)IDCT_W4 * col[BLK_W * 4];
  b0 += (U32)IDCT_W5 * col[BLK_W * 5]; b1 += (U32)-IDCT_W1 * col[BLK_W * 5]; b2 += (U32)IDCT_W7 * col[BLK_W * 5]; b3 += (U32)IDCT_W3 * col[BLK_W * 5];
  a0 += (U32)IDCT_W6 * col[BLK_W * 6]; a1 += (U32)-IDCT_W2 * col[BLK_W * 6]; a2 += (U32)IDCT_W2 * col[BLK_W * 6]; a3 += (U32)-IDCT_W6 * col[BLK_W * 6];
  b0 += (U32)IDCT_W7 * col[BLK_W * 7]; b1 += (U32)-IDCT_W5 * col[BLK_W * 7]; b2 += (U32)IDCT_W3 * col[BLK_W * 7]; b3 += (U32)-IDCT_W1 * col[BLK_W * 7];
  out[0] = (int)(a0 + b0) >> IDCT_COL_SHIFT;
  out[1] = (int)(a1 + b1) >> IDCT_COL_SHIFT;
  out[2] = (int)(a2 + b2) >> IDCT_COL_SHIFT;
  out[3] = (int)(a3 + b3) >> IDCT_COL_SHIFT;
  out[4] = (int)(a3 - b3) >> IDCT_COL_SHIFT;
  out[5] = (int)(a2 - b2) >> IDCT_COL_SHIFT;
  out[6] = (int)(a1 - b1) >> IDCT_COL_SHIFT;
  out[7] = (int)(a0 - b0) >> IDCT_COL_SHIFT;
}

static void simple_idct_put_add(U8* dest, int stride, S16* block, int add) {
  int i, j, o[BLK_W];
  for (i = 0; i < BLK_W; i++) simple_idct_row(block + i * BLK_W);
  for (i = 0; i < BLK_W; i++) {
    simple_idct_col(block + i, o);
    for (j = 0; j < BLK_W; j++) {
      U8* p = dest + j * stride + i;
      *p = clip8(add ? *p + o[j] : o[j]);
    }
  }
}

// the XviD IDCT's: its passes' shifts, its rows' bases and rounders
constexpr int XVID_ROW_SHIFT = 11, XVID_COL_SHIFT = 6;
static const int TAB04[] = { 22725, 21407, 19266, 16384, 12873, 8867, 4520 };
static const int TAB17[] = { 31521, 29692, 26722, 22725, 17855, 12299, 6270 };
static const int TAB26[] = { 29692, 27969, 25172, 21407, 16819, 11585, 5906 };
static const int TAB35[] = { 26722, 25172, 22654, 19266, 15137, 10426, 5315 };

static int xvid_idct_row(S16* in, const int* tab, int rnd) {
  const U32 c1 = tab[0], c2 = tab[1], c3 = tab[2], c4 = tab[3], c5 = tab[4], c6 = tab[5], c7 = tab[6];
  const int right = in[5] | in[6] | in[7];
  const int left = in[1] | in[2] | in[3];
  if (!(right | in[4])) {
    const int k = (int)(c4 * in[0] + rnd);
    if (left) {
      const U32 a0 = k + c2 * in[2], a1 = k + c6 * in[2], a2 = k - c6 * in[2], a3 = k - c2 * in[2];
      const int b0 = (int)(c1 * in[1] + c3 * in[3]);
      const int b1 = (int)(c3 * in[1] - c7 * in[3]);
      const int b2 = (int)(c5 * in[1] - c1 * in[3]);
      const int b3 = (int)(c7 * in[1] - c5 * in[3]);
      in[0] = (S16)((int)(a0 + b0) >> XVID_ROW_SHIFT);
      in[1] = (S16)((int)(a1 + b1) >> XVID_ROW_SHIFT);
      in[2] = (S16)((int)(a2 + b2) >> XVID_ROW_SHIFT);
      in[3] = (S16)((int)(a3 + b3) >> XVID_ROW_SHIFT);
      in[4] = (S16)((int)(a3 - b3) >> XVID_ROW_SHIFT);
      in[5] = (S16)((int)(a2 - b2) >> XVID_ROW_SHIFT);
      in[6] = (S16)((int)(a1 - b1) >> XVID_ROW_SHIFT);
      in[7] = (S16)((int)(a0 - b0) >> XVID_ROW_SHIFT);
    } else {
      const int a0 = k >> XVID_ROW_SHIFT;
      if (a0) {
        in[0] = in[1] = in[2] = in[3] = in[4] = in[5] = in[6] = in[7] = (S16)a0;
      } else
        return 0;
    }
  } else if (!(left | right)) {
    const int a0 = (int)(rnd + c4 * (in[0] + in[4])) >> XVID_ROW_SHIFT;
    const int a1 = (int)(rnd + c4 * (in[0] - in[4])) >> XVID_ROW_SHIFT;
    in[0] = in[3] = in[4] = in[7] = (S16)a0;
    in[1] = in[2] = in[5] = in[6] = (S16)a1;
  } else {
    const U32 k = c4 * in[0] + rnd;
    const U32 a0 = k + c2 * in[2] + c4 * in[4] + c6 * in[6];
    const U32 a1 = k + c6 * in[2] - c4 * in[4] - c2 * in[6];
    const U32 a2 = k - c6 * in[2] - c4 * in[4] + c2 * in[6];
    const U32 a3 = k - c2 * in[2] + c4 * in[4] - c6 * in[6];
    const U32 b0 = c1 * in[1] + c3 * in[3] + c5 * in[5] + c7 * in[7];
    const U32 b1 = c3 * in[1] - c7 * in[3] - c1 * in[5] - c5 * in[7];
    const U32 b2 = c5 * in[1] - c1 * in[3] + c7 * in[5] + c3 * in[7];
    const U32 b3 = c7 * in[1] - c5 * in[3] + c3 * in[5] - c1 * in[7];
    in[0] = (S16)((int)(a0 + b0) >> XVID_ROW_SHIFT);
    in[1] = (S16)((int)(a1 + b1) >> XVID_ROW_SHIFT);
    in[2] = (S16)((int)(a2 + b2) >> XVID_ROW_SHIFT);
    in[3] = (S16)((int)(a3 + b3) >> XVID_ROW_SHIFT);
    in[4] = (S16)((int)(a3 - b3) >> XVID_ROW_SHIFT);
    in[5] = (S16)((int)(a2 - b2) >> XVID_ROW_SHIFT);
    in[6] = (S16)((int)(a1 - b1) >> XVID_ROW_SHIFT);
    in[7] = (S16)((int)(a0 - b0) >> XVID_ROW_SHIFT);
  }
  return 1;
}

// its columns': tan(k pi/16) and sqrt(1/2), 2^16 times; their products
constexpr unsigned XVID_TAN1 = 0x32EC, XVID_TAN2 = 0x6A0A, XVID_TAN3 = 0xAB0E, XVID_SQRT2 = 0x5A82;
constexpr int XVID_MULT_SHIFT = 16;
static inline int xmult(unsigned c, int x) { return (int)((unsigned)((int)(c * (unsigned)x) >> XVID_MULT_SHIFT)); }
static inline void butterfly(int& a, int& b) {
  const int t = a + b;
  b = a - b;
  a = t;
}

static void xvid_idct_col_8(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7;
  mm4 = in[7 * BLK_W]; mm5 = in[5 * BLK_W]; mm6 = in[3 * BLK_W]; mm7 = in[1 * BLK_W];
  mm0 = xmult(XVID_TAN1, mm4) + mm7;
  mm1 = xmult(XVID_TAN1, mm7) - mm4;
  mm2 = xmult(XVID_TAN3, mm5) + mm6;
  mm3 = xmult(XVID_TAN3, mm6) - mm5;
  mm7 = mm0 + mm2;
  mm4 = mm1 - mm3;
  mm0 = mm0 - mm2;
  mm1 = mm1 + mm3;
  mm6 = mm0 + mm1;
  mm5 = mm0 - mm1;
  mm5 = 2 * xmult(XVID_SQRT2, mm5);
  mm6 = 2 * xmult(XVID_SQRT2, mm6);
  mm1 = in[2 * BLK_W];
  mm2 = in[6 * BLK_W];
  mm3 = xmult(XVID_TAN2, mm2) + mm1;
  mm2 = xmult(XVID_TAN2, mm1) - mm2;
  mm0 = in[0 * BLK_W] + in[4 * BLK_W];
  mm1 = in[0 * BLK_W] - in[4 * BLK_W];
  butterfly(mm0, mm3);
  butterfly(mm0, mm7);
  in[BLK_W * 0] = (S16)(mm0 >> XVID_COL_SHIFT);
  in[BLK_W * 7] = (S16)(mm7 >> XVID_COL_SHIFT);
  butterfly(mm3, mm4);
  in[BLK_W * 3] = (S16)(mm3 >> XVID_COL_SHIFT);
  in[BLK_W * 4] = (S16)(mm4 >> XVID_COL_SHIFT);
  butterfly(mm1, mm2);
  butterfly(mm1, mm6);
  in[BLK_W * 1] = (S16)(mm1 >> XVID_COL_SHIFT);
  in[BLK_W * 6] = (S16)(mm6 >> XVID_COL_SHIFT);
  butterfly(mm2, mm5);
  in[BLK_W * 2] = (S16)(mm2 >> XVID_COL_SHIFT);
  in[BLK_W * 5] = (S16)(mm5 >> XVID_COL_SHIFT);
}

static void xvid_idct_col_4(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7;
  mm0 = in[1 * BLK_W];
  mm2 = in[3 * BLK_W];
  mm1 = xmult(XVID_TAN1, mm0);
  mm3 = xmult(XVID_TAN3, mm2);
  mm7 = mm0 + mm2;
  mm4 = mm1 - mm3;
  mm0 = mm0 - mm2;
  mm1 = mm1 + mm3;
  mm6 = mm0 + mm1;
  mm5 = mm0 - mm1;
  mm6 = 2 * xmult(XVID_SQRT2, mm6);
  mm5 = 2 * xmult(XVID_SQRT2, mm5);
  mm0 = mm1 = in[0 * BLK_W];
  mm3 = in[2 * BLK_W];
  mm2 = xmult(XVID_TAN2, mm3);
  butterfly(mm0, mm3);
  butterfly(mm0, mm7);
  in[BLK_W * 0] = (S16)(mm0 >> XVID_COL_SHIFT);
  in[BLK_W * 7] = (S16)(mm7 >> XVID_COL_SHIFT);
  butterfly(mm3, mm4);
  in[BLK_W * 3] = (S16)(mm3 >> XVID_COL_SHIFT);
  in[BLK_W * 4] = (S16)(mm4 >> XVID_COL_SHIFT);
  butterfly(mm1, mm2);
  butterfly(mm1, mm6);
  in[BLK_W * 1] = (S16)(mm1 >> XVID_COL_SHIFT);
  in[BLK_W * 6] = (S16)(mm6 >> XVID_COL_SHIFT);
  butterfly(mm2, mm5);
  in[BLK_W * 2] = (S16)(mm2 >> XVID_COL_SHIFT);
  in[BLK_W * 5] = (S16)(mm5 >> XVID_COL_SHIFT);
}

static void xvid_idct_col_3(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7;
  mm7 = in[1 * BLK_W];
  mm4 = xmult(XVID_TAN1, mm7);
  mm6 = mm7 + mm4;
  mm5 = mm7 - mm4;
  mm6 = 2 * xmult(XVID_SQRT2, mm6);
  mm5 = 2 * xmult(XVID_SQRT2, mm5);
  mm0 = mm1 = in[0 * BLK_W];
  mm3 = in[2 * BLK_W];
  mm2 = xmult(XVID_TAN2, mm3);
  butterfly(mm0, mm3);
  butterfly(mm0, mm7);
  in[BLK_W * 0] = (S16)(mm0 >> XVID_COL_SHIFT);
  in[BLK_W * 7] = (S16)(mm7 >> XVID_COL_SHIFT);
  butterfly(mm3, mm4);
  in[BLK_W * 3] = (S16)(mm3 >> XVID_COL_SHIFT);
  in[BLK_W * 4] = (S16)(mm4 >> XVID_COL_SHIFT);
  butterfly(mm1, mm2);
  butterfly(mm1, mm6);
  in[BLK_W * 1] = (S16)(mm1 >> XVID_COL_SHIFT);
  in[BLK_W * 6] = (S16)(mm6 >> XVID_COL_SHIFT);
  butterfly(mm2, mm5);
  in[BLK_W * 2] = (S16)(mm2 >> XVID_COL_SHIFT);
  in[BLK_W * 5] = (S16)(mm5 >> XVID_COL_SHIFT);
}

// the rows (each with its basis and rounder; the first XVID_ROWS3 always taken as nonzero), then the
// columns, with as many rows as are nonzero (3, 4 or all)
enum { XVID_ROWS3 = 3, XVID_ROWS4 = 4 };
static void xvid_idct(S16* in) {
  static const int* const tab[BLK_W] = { TAB04, TAB17, TAB26, TAB35, TAB04, TAB35, TAB26, TAB17 };
  static const int rnd[BLK_W] = { 65536, 3597, 2260, 1203, 0, 120, 512, 512 };
  int i, rows = 0;
  for (i = 0; i < BLK_W; i++)
    if (xvid_idct_row(in + i * BLK_W, tab[i], rnd[i]) || i < XVID_ROWS3) rows |= 1 << i;
  if (rows >> XVID_ROWS4) {
    for (i = 0; i < BLK_W; i++) xvid_idct_col_8(in + i);
  } else if (rows >> XVID_ROWS3) {
    for (i = 0; i < BLK_W; i++) xvid_idct_col_4(in + i);
  } else {
    for (i = 0; i < BLK_W; i++) xvid_idct_col_3(in + i);
  }
}

static void rc_idct_put(const Recon* d, U8* dest, int stride, S16* block) {
  if (d->idct_xvid) {
    int i, j;
    xvid_idct(block);
    for (i = 0; i < BLK_W; i++)
      for (j = 0; j < BLK_W; j++) dest[i * stride + j] = clip8(block[i * BLK_W + j]);
  } else
    simple_idct_put_add(dest, stride, block, 0);
}

static void rc_idct_add(const Recon* d, U8* dest, int stride, S16* block) {
  if (d->idct_xvid) {
    int i, j;
    xvid_idct(block);
    for (i = 0; i < BLK_W; i++)
      for (j = 0; j < BLK_W; j++) dest[i * stride + j] = clip8(dest[i * stride + j] + block[i * BLK_W + j]);
  } else
    simple_idct_put_add(dest, stride, block, 1);
}

//--- dequantisation ---------------------------------------------------------------------------------

// FFmpeg's dct_unquantize_mpeg2: intra (level x 2 qscale x weight) >> QM_INTRA_SHIFT, inter ((2
// level + 1) x 2 qscale x weight) >> QM_INTER_SHIFT
constexpr int QM_INTRA_SHIFT = 4, QM_INTER_SHIFT = 5;

static void rc_unquant_intra(const Recon* d, S16* block, int n, int qscale) {
  int i, level;
  block[0] = (S16)(block[0] * (n < MB_LUMA ? d->y_dc_scale : d->c_dc_scale));
  if (d->mpeg_quant) {  // MPEG-2 style, with mismatch control (FFmpeg's bitexact variant)
    int sum = -1, q2 = qscale << 1;
    sum += block[0];
    for (i = 1; i < BLK_COEFS; i++) {
      level = block[i];
      if (level) {
        if (level < 0) level = -(int)((-level) * q2 * d->intra_matrix[i] >> QM_INTRA_SHIFT);
        else level = (int)(level * q2 * d->intra_matrix[i] >> QM_INTRA_SHIFT);
        block[i] = (S16)level;
        sum += level;
      }
    }
    block[BLK_LAST] ^= sum & 1;
  } else {
    int qmul = qscale << 1, qadd = (qscale - 1) | 1;
    for (i = 1; i < BLK_COEFS; i++) {
      level = block[i];
      if (level) block[i] = (S16)(level < 0 ? level * qmul - qadd : level * qmul + qadd);
    }
  }
}

static void rc_unquant_inter_mpeg(const Recon* d, S16* block, int qscale) {
  int i, level, sum = -1, q2 = qscale << 1;
  for (i = 0; i < BLK_COEFS; i++) {
    level = block[i];
    if (level) {
      if (level < 0) level = -(int)((((-level) << 1) + 1) * q2 * d->inter_matrix[i] >> QM_INTER_SHIFT);
      else level = (int)(((level << 1) + 1) * q2 * d->inter_matrix[i] >> QM_INTER_SHIFT);
      block[i] = (S16)level;
      sum += level;
    }
  }
  block[BLK_LAST] ^= sum & 1;
}

//--- intra prediction --------------------------------------------------------------------------------

static int rc_pred_dc(const Recon* d, int n, int* dir_ptr) {
  const Motion* o = d->mo;  // (the video packet's start: the motion state's)
  const S16* dc_val = d->dc_val + d->block_index[n];
  const int wrap = d->block_wrap[n];
  int a = dc_val[-1], b = dc_val[-1 - wrap], c = dc_val[-wrap];
  if (o->first_slice_line && n != 3) {
    if (n != 2) b = c = DC_RESET;
    if (n != 1 && d->mb_x == o->resync_mb_x) b = a = DC_RESET;
  }
  if (d->mb_x == o->resync_mb_x && d->mb_y == o->resync_mb_y + 1) {
    if (n == 0 || n == BLK_CB || n == BLK_CR) b = DC_RESET;
  }
  if (abs(a - b) < abs(b - c)) {
    *dir_ptr = 1;
    return c;
  }
  *dir_ptr = 0;
  return a;
}

static int rc_level_dc(Recon* d, int n, int pred, int level) {
  int scale = n < MB_LUMA ? d->y_dc_scale : d->c_dc_scale, ret;
  pred = (pred + (scale >> 1)) / scale;
  level += pred;
  ret = level;
  level *= scale;
  if (level & ~DC_MAX) {
    if (level < 0) level = 0;
    else if (!(d->bugs & BUG_DC_CLIP)) level = DC_MAX;
  }
  d->dc_val[d->block_index[n]] = (S16)level;
  return ret;
}

static void rc_pred_ac(Recon* d, S16* block, int n, int dir) {
  int i;
  S16 *ac_val = d->ac_val[d->block_index[n]], *ac_val1 = ac_val;
  const S8* qt = d->qscale_table;
  if (d->ac_pred) {
    if (dir == 0) {
      const int xy = d->mb_x - 1 + d->mb_y * d->mb_stride;
      ac_val -= ACV;
      if (d->mb_x == 0 || d->qscale == qt[xy] || n == 1 || n == 3) {
        for (i = 1; i < BLK_W; i++) block[i << BLK_SHIFT] += ac_val[i];
      } else {
        for (i = 1; i < BLK_W; i++) block[i << BLK_SHIFT] += rounded_div(ac_val[i] * qt[xy], d->qscale);
      }
    } else {
      const int xy = d->mb_x + d->mb_y * d->mb_stride - d->mb_stride;
      ac_val -= ACV * d->block_wrap[n];
      if (d->mb_y == 0 || d->qscale == qt[xy] || n == 2 || n == 3) {
        for (i = 1; i < BLK_W; i++) block[i] += ac_val[i + BLK_W];
      } else {
        for (i = 1; i < BLK_W; i++) block[i] += rounded_div(ac_val[i + BLK_W] * qt[xy], d->qscale);
      }
    }
  }
  for (i = 1; i < BLK_W; i++) ac_val1[i] = block[i << BLK_SHIFT];
  for (i = 1; i < BLK_W; i++) ac_val1[BLK_W + i] = block[i];
}

static void rc_clean_intra_table_entries(Recon* d, int xy) {
  if (d->mbintra_table[xy]) {
    const int wrap = d->b8_stride, bi = d->block_index[0];
    d->mbintra_table[xy] = 0;
    d->dc_val[bi] = d->dc_val[bi + 1] = d->dc_val[bi + wrap] = d->dc_val[bi + wrap + 1] = DC_RESET;
    d->dc_val[d->block_index[BLK_CB]] = d->dc_val[d->block_index[BLK_CR]] = DC_RESET;
    memset(d->ac_val[bi + 1], 0, sizeof(*d->ac_val));
    memset(d->ac_val[bi + wrap], 0, 2 * sizeof(*d->ac_val));
    memset(d->ac_val[d->block_index[BLK_CB]], 0, sizeof(*d->ac_val));
    memset(d->ac_val[d->block_index[BLK_CR]], 0, sizeof(*d->ac_val));
  }
}

static void rc_clean_buffers(Recon* d) {  // ff_mpeg4_clean_buffers, at the start of each video packet
  const int l_wrap = d->b8_stride, c_wrap = d->mb_stride;
  const int l_xy = (2 * d->mb_y - 1) * l_wrap + d->mb_x * 2 - 1;
  const int u_xy = 2 * d->mb_h * l_wrap + d->mb_y * c_wrap + d->mb_x - 1;
  const int v_xy = u_xy + c_wrap * (d->mb_h + 1);
  memset(d->ac_val + l_xy, 0, (l_wrap * 2 + 1) * sizeof(*d->ac_val));
  memset(d->ac_val + u_xy, 0, (c_wrap + 1) * sizeof(*d->ac_val));
  memset(d->ac_val + v_xy, 0, (c_wrap + 1) * sizeof(*d->ac_val));
}

//--- motion compensation --------------------------------------------------------------------------

// the samples the interpolation of a macroblock (MC_MB) and of a block (MC_B) reads: one more each way
constexpr int MC_MB = MB_W + 1, MC_B = BLK_W + 1;
// half- and quarter-sample vectors: the fraction's bits; the positions (hpel: dxy, qpel: qp())
constexpr int HPEL_BITS = 1, QPEL_BITS = 2, QPEL_MASK = (1 << QPEL_BITS) - 1;
enum { HP_FULL, HP_H, HP_V, HP_HV };
constexpr int qp(int x, int y) { return y << QPEL_BITS | x; }  // (x, y: quarter samples, 0-3)
// MPEG-4's quarter-sample filter: its taps (from the middle out), its gain's bits, its reach
constexpr int QPEL_T0 = 20, QPEL_T1 = 6, QPEL_T2 = 3, QPEL_SHIFT = 5, QPEL_HALF = 3;
// FF_BUG_IEDGE: the row of the bottom field's emulated Cb window that is Cr's first
constexpr int IEDGE_ROW = 4;
// GMC with one point (FFmpeg's gmc1): the vectors in sixteenths of a sample (GMC1_ONE), from
// halves to sixteenths by 1 << (GMC1_SUB - the warping accuracy); its bilinear filter's bits
constexpr int GMC1_ONE = 16, GMC1_SUB = 3, GMC1_SHIFT = 8;

// a w x h block at (x, y) (rows y, y+ystep, ...) of a plane with coordinates clamped to the
// [0, ew-1] x [0, eh-1] reference area (FFmpeg's emulated_edge_mc in every case); in place when
// inside it (*stride = ps*ystep), else built in buf (stride bs)
static const U8* rc_fetch(U8* buf, int bs, const U8* plane, int ps, int x, int y, int ystep, int w, int h, int ew,
                          int eh, int* stride) {
  int i, j;
  if (x >= 0 && x + w <= ew && y >= 0 && y + (h - 1) * ystep < eh) {
    *stride = ps * ystep;
    return plane + (ptrdiff_t)y * ps + x;
  }
  for (i = 0; i < h; i++) {
    const U8* row = plane + (ptrdiff_t)clip(y + i * ystep, 0, eh - 1) * ps;
    for (j = 0; j < w; j++) buf[i * bs + j] = row[clip(x + j, 0, ew - 1)];
  }
  *stride = bs;
  return buf;
}

// FF_BUG_IEDGE (libavcodec 55.66-57.65 encoders): where field MC needs edge emulation, FFmpeg builds
// the Cr block in a scratch buffer overlapping the last row of the Cb one, so row 9 of the emulated
// Cb window (5th row of the bottom field) is Cr's first row. Returns the Cb block (stride 9).
static const U8* rc_fetch_cb_iedge(U8* buf, const U8* u, const U8* v, int lc, int x, int y, int field_select,
                                   int rows, int ew, int eh, int* stride) {
  int k, j;
  for (k = 0; k < rows; k++) {
    const U8* row = u + (ptrdiff_t)clip(2 * y + field_select + 2 * k, 0, eh - 1) * lc;
    for (j = 0; j < MC_B; j++) buf[k * MC_B + j] = row[clip(x + j, 0, ew - 1)];
  }
  if (field_select && rows > IEDGE_ROW) {
    const U8* row = v + (ptrdiff_t)clip(2 * y, 0, eh - 1) * lc;
    for (j = 0; j < MC_B; j++) buf[IEDGE_ROW * MC_B + j] = row[clip(x + j, 0, ew - 1)];
  }
  *stride = MC_B;
  return buf;
}

// half-pel interpolation: put (rounding), put_no_rnd, avg (FFmpeg's hpeldsp), of the samples
// f(s, x) gives from the row s
template <class F>
static inline void rc_hpel_loop(U8* dst, int ds, const U8* src, int ss, int w, int h, int op, F f) {
  int x, y;
  for (y = 0; y < h; y++, src += ss, dst += ds)
    if (op == OP_AVG)
      for (x = 0; x < w; x++) dst[x] = (U8)((dst[x] + f(src, x) + 1) >> 1);
    else
      for (x = 0; x < w; x++) dst[x] = (U8)f(src, x);
}
static void rc_hpel(U8* dst, int ds, const U8* src, int ss, int w, int h, int dxy, int op) {
  const int r1 = op == OP_PUT_NORND ? 0 : 1, r2 = op == OP_PUT_NORND ? 1 : 2;
  switch (dxy) {
  case HP_FULL: rc_hpel_loop(dst, ds, src, ss, w, h, op, [](const U8* s, int x) { return (int)s[x]; }); break;
  case HP_H:
    rc_hpel_loop(dst, ds, src, ss, w, h, op, [=](const U8* s, int x) { return (s[x] + s[x + 1] + r1) >> 1; });
    break;
  case HP_V:
    rc_hpel_loop(dst, ds, src, ss, w, h, op, [=](const U8* s, int x) { return (s[x] + s[x + ss] + r1) >> 1; });
    break;
  default:
    rc_hpel_loop(dst, ds, src, ss, w, h, op,
                 [=](const U8* s, int x) { return (s[x] + s[x + 1] + s[x + ss] + s[x + ss + 1] + r2) >> 2; });
    break;
  }
}

// MPEG-4 quarter-pel (FFmpeg's qpeldsp.c). n = 8 or 16. The filters mirror at the block's edge.
static inline void rc_qpel_store(U8* p, int v, int op) {
  constexpr int R = 1 << (QPEL_SHIFT - 1);
  if (op == OP_PUT) *p = clip8((v + R) >> QPEL_SHIFT);
  else if (op == OP_PUT_NORND) *p = clip8((v + R - 1) >> QPEL_SHIFT);
  else *p = (U8)((*p + clip8((v + R) >> QPEL_SHIFT) + 1) >> 1);
}

static void rc_qpel_line(U8* dst, int dstep, const U8* src, int sstep, int n, int op) {
  constexpr int H = QPEL_HALF;
  int e[MB_W + 2 * (H + 1)], i;
  for (i = 0; i <= n; i++) e[H + i] = src[i * sstep];
  for (i = 1; i <= H; i++) {  // (mirrored at the edges)
    e[H - i] = e[H + i - 1];
    e[H + n + i] = e[H + n + 1 - i];
  }
  for (i = 0; i < n; i++) {
    const int* s = e + H + i;
    int v = (s[0] + s[1]) * QPEL_T0 - (s[-1] + s[2]) * QPEL_T1 + (s[-2] + s[3]) * QPEL_T2 - (s[-3] + s[4]);
    rc_qpel_store(dst + i * dstep, v, op);
  }
}

static void rc_qpel_h(U8* dst, int ds, const U8* src, int ss, int n, int h, int op) {
  int y;
  for (y = 0; y < h; y++) rc_qpel_line(dst + y * ds, 1, src + y * ss, 1, n, op);
}

static void rc_qpel_v(U8* dst, int ds, const U8* src, int ss, int n, int op) {
  int x;
  for (x = 0; x < n; x++) rc_qpel_line(dst + x, ds, src + x, ss, n, op);
}

static void rc_avg2(U8* dst, int ds, const U8* a, int as, const U8* b, int bs, int n, int h, int op) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < n; x++) {
      int A = a[y * as + x], B = b[y * bs + x];
      U8* p = dst + y * ds + x;
      if (op == OP_PUT) *p = (U8)((A + B + 1) >> 1);
      else if (op == OP_PUT_NORND) *p = (U8)((A + B) >> 1);
      else *p = (U8)((*p + ((A + B + 1) >> 1) + 1) >> 1);
    }
}

static void rc_avg4(U8* dst, int ds, const U8* a, int as, const U8* b, int bs, const U8* c, int cs, const U8* e,
                    int es, int n, int h, int op) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < n; x++) {
      int s = a[y * as + x] + b[y * bs + x] + c[y * cs + x] + e[y * es + x];
      U8* p = dst + y * ds + x;
      if (op == OP_PUT) *p = (U8)((s + 2) >> 2);
      else if (op == OP_PUT_NORND) *p = (U8)((s + 1) >> 2);
      else *p = (U8)((*p + ((s + 2) >> 2) + 1) >> 1);
    }
}

// src: (n+1) x (n+1) samples with stride ss
static void rc_qpel_mc(U8* dst, int ds, const U8* src, int ss, int n, int dxy, int op, int std_qpel_bug) {
  U8 halfH[MC_MB * MB_W], halfV[MB_W * MB_W], halfHV[MB_W * MB_W], half[MB_W * MB_W];
  const int ri = op == OP_PUT_NORND ? OP_PUT_NORND : OP_PUT;  // intermediate rounding
  const int qx = dxy & QPEL_MASK, qy = dxy >> QPEL_BITS;      // (in quarter samples)
  if (std_qpel_bug && (qx & 1) && qy) {
    // old lavc qpel (FF_BUG_STD_QPEL): qpel*_mc{11,31,13,33,12,32}_old_c
    const U8* full = src;
    const int fx = qx == 3 ? 1 : 0;
    rc_qpel_h(halfH, n, full, ss, n, n + 1, ri);
    rc_qpel_v(halfV, n, full + fx, ss, n, ri);
    rc_qpel_v(halfHV, n, halfH, n, n, ri);
    if (qy == 2) rc_avg2(dst, ds, halfV, n, halfHV, n, n, n, op);  // mc12 / mc32
    else if (qy == 1) rc_avg4(dst, ds, full + fx, ss, halfH, n, halfV, n, halfHV, n, n, n, op);
    else rc_avg4(dst, ds, full + ss + fx, ss, halfH + n, n, halfV, n, halfHV, n, n, n, op);
    return;
  }
  switch (dxy) {
  case qp(0, 0): rc_avg2(dst, ds, src, ss, src, ss, n, n, op == OP_AVG ? OP_AVG : OP_PUT); break;
  case qp(1, 0): rc_qpel_h(half, n, src, ss, n, n, ri); rc_avg2(dst, ds, src, ss, half, n, n, n, op); break;
  case qp(2, 0): rc_qpel_h(dst, ds, src, ss, n, n, op); break;
  case qp(3, 0): rc_qpel_h(half, n, src, ss, n, n, ri); rc_avg2(dst, ds, src + 1, ss, half, n, n, n, op); break;
  case qp(0, 1): rc_qpel_v(half, n, src, ss, n, ri); rc_avg2(dst, ds, src, ss, half, n, n, n, op); break;
  case qp(0, 2): rc_qpel_v(dst, ds, src, ss, n, op); break;
  case qp(0, 3): rc_qpel_v(half, n, src, ss, n, ri); rc_avg2(dst, ds, src + ss, ss, half, n, n, n, op); break;
  case qp(1, 1): case qp(3, 1): case qp(1, 3): case qp(3, 3): {
    const int fx = qx == 3 ? 1 : 0, fy = qy == 3 ? n : 0;
    rc_qpel_h(halfH, n, src, ss, n, n + 1, ri);
    rc_avg2(halfH, n, halfH, n, src + fx, ss, n, n + 1, ri);
    rc_qpel_v(halfHV, n, halfH, n, n, ri);
    rc_avg2(dst, ds, halfH + fy, n, halfHV, n, n, n, op);
    break;
  }
  case qp(2, 1): case qp(2, 3):
    rc_qpel_h(halfH, n, src, ss, n, n + 1, ri);
    rc_qpel_v(halfHV, n, halfH, n, n, ri);
    rc_avg2(dst, ds, halfH + (qy == 3 ? n : 0), n, halfHV, n, n, n, op);
    break;
  case qp(1, 2): case qp(3, 2):
    rc_qpel_h(halfH, n, src, ss, n, n + 1, ri);
    rc_avg2(halfH, n, halfH, n, src + (qx == 3 ? 1 : 0), ss, n, n + 1, ri);
    rc_qpel_v(dst, ds, halfH, n, n, op);
    break;
  case qp(2, 2):
    rc_qpel_h(halfH, n, src, ss, n, n + 1, ri);
    rc_qpel_v(dst, ds, halfH, n, n, op);
    break;
  }
}

static void rc_gmc1_dsp(U8* dst, int ds, const U8* src, int ss, int h, int x16, int y16, int rounder) {
  constexpr int O = GMC1_ONE;
  const int A = (O - x16) * (O - y16), B = x16 * (O - y16), C = (O - x16) * y16, D = x16 * y16;
  int i, j;
  for (i = 0; i < h; i++) {
    for (j = 0; j < BLK_W; j++)
      dst[j] = (U8)((A * src[j] + B * src[j + 1] + C * src[ss + j] + D * src[ss + j + 1] + rounder) >> GMC1_SHIFT);
    dst += ds;
    src += ss;
  }
}

static void rc_gmc_dsp(U8* dst, int stride, const U8* src, int h, int ox, int oy, int dxx, int dxy, int dyx, int dyy,
                       int shift, int r, int width, int height) {
  int y, vx, vy;
  const int s = 1 << shift;
  width--;
  height--;
  for (y = 0; y < h; y++) {
    int x;
    vx = ox;
    vy = oy;
    for (x = 0; x < BLK_W; x++) {
      int index, src_x = vx >> SPR_FIX, src_y = vy >> SPR_FIX;
      int frac_x = src_x & (s - 1), frac_y = src_y & (s - 1);
      src_x >>= shift;
      src_y >>= shift;
      if ((unsigned)src_x < (unsigned)width) {
        if ((unsigned)src_y < (unsigned)height) {
          index = src_x + src_y * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_x) + src[index + 1] * frac_x) * (s - frac_y) +
                                      (src[index + stride] * (s - frac_x) + src[index + stride + 1] * frac_x) * frac_y +
                                      r) >> (shift * 2));
        } else {
          index = src_x + clip(src_y, 0, height) * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_x) + src[index + 1] * frac_x) * s + r) >> (shift * 2));
        }
      } else {
        if ((unsigned)src_y < (unsigned)height) {
          index = clip(src_x, 0, width) + src_y * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_y) + src[index + stride] * frac_y) * s + r) >> (shift * 2));
        } else {
          index = clip(src_x, 0, width) + clip(src_y, 0, height) * stride;
          dst[y * stride + x] = src[index];
        }
      }
      vx += dxx;
      vy += dyx;
    }
    ox += dxy;
    oy += dyy;
  }
}

static void rc_gmc1_motion(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, const RPic* ref) {
  constexpr int F = GMC1_ONE - 1, HALF = GMC1_ONE / 2, RND = 1 << (GMC1_SHIFT - 1);
  const Motion* o = d->mo;
  U8 buf[MC_MB * MC_MB];
  const U8* src;
  int src_x, src_y, motion_x, motion_y, ss;
  motion_x = o->sprite_offset[0][0];
  motion_y = o->sprite_offset[0][1];
  src_x = d->mb_x * MB_W + (motion_x >> (o->warp_acc + 1));
  src_y = d->mb_y * MB_W + (motion_y >> (o->warp_acc + 1));
  motion_x *= 1 << (GMC1_SUB - o->warp_acc);
  motion_y *= 1 << (GMC1_SUB - o->warp_acc);
  src_x = clip(src_x, -MB_W, d->width);
  if (src_x == d->width) motion_x = 0;
  src_y = clip(src_y, -MB_W, d->height);
  if (src_y == d->height) motion_y = 0;
  src = rc_fetch(buf, MC_MB, ref->y, d->ly, src_x, src_y, 1, MC_MB, MC_MB, d->h_edge, d->v_edge, &ss);
  if ((motion_x | motion_y) & (HALF - 1)) {
    rc_gmc1_dsp(dest_y, d->ly, src, ss, MB_W, motion_x & F, motion_y & F, RND - d->no_rounding);
    rc_gmc1_dsp(dest_y + BLK_W, d->ly, src + BLK_W, ss, MB_W, motion_x & F, motion_y & F, RND - d->no_rounding);
  } else {  // (on half samples)
    int dxy = ((motion_x / HALF) & 1) | ((motion_y / HALF) & 1) << 1;
    rc_hpel(dest_y, d->ly, src, ss, MB_W, MB_W, dxy, d->no_rounding ? OP_PUT_NORND : OP_PUT);
  }
  motion_x = o->sprite_offset[1][0];
  motion_y = o->sprite_offset[1][1];
  src_x = d->mb_x * BLK_W + (motion_x >> (o->warp_acc + 1));
  src_y = d->mb_y * BLK_W + (motion_y >> (o->warp_acc + 1));
  motion_x *= 1 << (GMC1_SUB - o->warp_acc);
  motion_y *= 1 << (GMC1_SUB - o->warp_acc);
  src_x = clip(src_x, -BLK_W, d->width >> 1);
  if (src_x == d->width >> 1) motion_x = 0;
  src_y = clip(src_y, -BLK_W, d->height >> 1);
  if (src_y == d->height >> 1) motion_y = 0;
  src = rc_fetch(buf, MC_B, ref->u, d->lc, src_x, src_y, 1, MC_B, MC_B, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_gmc1_dsp(dest_cb, d->lc, src, ss, BLK_W, motion_x & F, motion_y & F, RND - d->no_rounding);
  src = rc_fetch(buf, MC_B, ref->v, d->lc, src_x, src_y, 1, MC_B, MC_B, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_gmc1_dsp(dest_cr, d->lc, src, ss, BLK_W, motion_x & F, motion_y & F, RND - d->no_rounding);
}

static void rc_gmc_motion(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, const RPic* ref) {
  const Motion* o = d->mo;
  const int a = o->warp_acc;
  const int dl = d->ly, dc = d->lc;
  int ox, oy;
  // (FFmpeg calls the gmc dsp with the destination's stride for the source too: the same)
  ox = o->sprite_offset[0][0] + o->sprite_delta[0][0] * d->mb_x * MB_W + o->sprite_delta[0][1] * d->mb_y * MB_W;
  oy = o->sprite_offset[0][1] + o->sprite_delta[1][0] * d->mb_x * MB_W + o->sprite_delta[1][1] * d->mb_y * MB_W;
  rc_gmc_dsp(dest_y, dl, ref->y, MB_W, ox, oy, o->sprite_delta[0][0], o->sprite_delta[0][1], o->sprite_delta[1][0],
             o->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, d->h_edge, d->v_edge);
  rc_gmc_dsp(dest_y + BLK_W, dl, ref->y, MB_W, ox + o->sprite_delta[0][0] * BLK_W, oy + o->sprite_delta[1][0] * BLK_W,
             o->sprite_delta[0][0], o->sprite_delta[0][1], o->sprite_delta[1][0], o->sprite_delta[1][1], a + 1,
             (1 << (2 * a + 1)) - d->no_rounding, d->h_edge, d->v_edge);
  ox = o->sprite_offset[1][0] + o->sprite_delta[0][0] * d->mb_x * BLK_W + o->sprite_delta[0][1] * d->mb_y * BLK_W;
  oy = o->sprite_offset[1][1] + o->sprite_delta[1][0] * d->mb_x * BLK_W + o->sprite_delta[1][1] * d->mb_y * BLK_W;
  rc_gmc_dsp(dest_cb, dc, ref->u, BLK_W, ox, oy, o->sprite_delta[0][0], o->sprite_delta[0][1], o->sprite_delta[1][0],
             o->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, (d->h_edge + 1) >> 1,
             (d->v_edge + 1) >> 1);
  rc_gmc_dsp(dest_cr, dc, ref->v, BLK_W, ox, oy, o->sprite_delta[0][0], o->sprite_delta[0][1], o->sprite_delta[1][0],
             o->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, (d->h_edge + 1) >> 1,
             (d->v_edge + 1) >> 1);
}

// mpeg_motion_internal for H.263/MPEG-4 (a frame or one field)
static void rc_mpeg_motion(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field,
                           int field_select, const RPic* ref, int op, int motion_x, int motion_y, int h) {
  U8 buf[(MC_MB + 1) * MC_MB];
  const U8* src;
  int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
  const int linesize = d->ly << field_based, uvlinesize = d->lc << field_based;
  dxy = ((motion_y & 1) << 1) | (motion_x & 1);
  src_x = d->mb_x * MB_W + (motion_x >> HPEL_BITS);
  src_y = (d->mb_y << (MB_SHIFT - field_based)) + (motion_y >> HPEL_BITS);
  if ((d->bugs & BUG_HPEL_CHROMA) && field_based) {
    mx = (motion_x >> 1) | (motion_x & 1);
    my = motion_y >> 1;
    uvdxy = ((my & 1) << 1) | (mx & 1);
    uvsrc_x = d->mb_x * BLK_W + (mx >> HPEL_BITS);
    uvsrc_y = (d->mb_y << (BLK_SHIFT - field_based)) + (my >> HPEL_BITS);
  } else {
    uvdxy = dxy | (motion_y & 2) | ((motion_x & 2) >> 1);
    uvsrc_x = src_x >> 1;
    uvsrc_y = src_y >> 1;
  }
  if (bottom_field) {
    dest_y += d->ly;
    dest_cb += d->lc;
    dest_cr += d->lc;
  }
  src = rc_fetch(buf, MC_MB, ref->y, d->ly, src_x, src_y * (1 << field_based) + field_select, 1 << field_based, MC_MB,
                 h + 1, d->h_edge, d->v_edge, &ss);
  rc_hpel(dest_y, linesize, src, ss, MB_W, h, dxy, op);
  if (field_based && (d->bugs & BUG_IEDGE) &&
      ((unsigned)src_x >= (unsigned)(d->h_edge - (motion_x & 1) - (MB_W - 1) > 0 ? d->h_edge - (motion_x & 1) - (MB_W - 1) : 0) ||
       (unsigned)src_y >=
           (unsigned)((d->v_edge >> 1) - (motion_y & 1) - h + 1 > 0 ? (d->v_edge >> 1) - (motion_y & 1) - h + 1 : 0)))
    src = rc_fetch_cb_iedge(buf, ref->u, ref->v, d->lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1, d->h_edge >> 1,
                            d->v_edge >> 1, &ss);
  else
    src = rc_fetch(buf, MC_B, ref->u, d->lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                   MC_B, (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cb, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
  src = rc_fetch(buf, MC_B, ref->v, d->lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                 MC_B, (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cr, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
}

static void rc_qpel_motion(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field,
                           int field_select, const RPic* ref, int op, int motion_x, int motion_y, int h) {
  U8 buf[MC_MB * MC_MB];
  const U8* src;
  int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
  const int linesize = d->ly << field_based, uvlinesize = d->lc << field_based;
  const int std_bug = (d->bugs & BUG_STD_QPEL) != 0;
  dxy = qp(motion_x & QPEL_MASK, motion_y & QPEL_MASK);
  src_x = d->mb_x * MB_W + (motion_x >> QPEL_BITS);
  src_y = d->mb_y * (MB_W >> field_based) + (motion_y >> QPEL_BITS);
  if (field_based) {
    mx = motion_x / 2;
    my = motion_y >> 1;
  } else if (d->bugs & BUG_QPEL_CHROMA2) {
    static const int rtab[] = { 0, 0, 1, 1, 0, 0, 0, 1 };
    mx = (motion_x >> 1) + rtab[motion_x & (countof(rtab) - 1)];
    my = (motion_y >> 1) + rtab[motion_y & (countof(rtab) - 1)];
  } else if (d->bugs & BUG_QPEL_CHROMA) {
    mx = (motion_x >> 1) | (motion_x & 1);
    my = (motion_y >> 1) | (motion_y & 1);
  } else {
    mx = motion_x / 2;
    my = motion_y / 2;
  }
  mx = (mx >> 1) | (mx & 1);
  my = (my >> 1) | (my & 1);
  uvdxy = (mx & 1) | ((my & 1) << 1);
  mx >>= 1;
  my >>= 1;
  uvsrc_x = d->mb_x * BLK_W + mx;
  uvsrc_y = d->mb_y * (BLK_W >> field_based) + my;
  if (!field_based) {
    src = rc_fetch(buf, MC_MB, ref->y, d->ly, src_x, src_y, 1, MC_MB, MC_MB, d->h_edge, d->v_edge, &ss);
    rc_qpel_mc(dest_y, d->ly, src, ss, MB_W, dxy, op, std_bug);
  } else {
    if (bottom_field) {
      dest_y += d->ly;
      dest_cb += d->lc;
      dest_cr += d->lc;
    }
    src = rc_fetch(buf, MC_MB, ref->y, d->ly, src_x, src_y * 2 + field_select, 2, MC_MB, MC_B, d->h_edge, d->v_edge, &ss);
    rc_qpel_mc(dest_y, linesize, src, ss, BLK_W, dxy, op, std_bug);
    rc_qpel_mc(dest_y + BLK_W, linesize, src + BLK_W, ss, BLK_W, dxy, op, std_bug);
  }
  src = rc_fetch(buf, MC_B, ref->v, d->lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                 MC_B, (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cr, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
  if (field_based && (d->bugs & BUG_IEDGE) &&
      ((unsigned)src_x >= (unsigned)(d->h_edge - (motion_x & QPEL_MASK) - (MB_W - 1) > 0
                                         ? d->h_edge - (motion_x & QPEL_MASK) - (MB_W - 1) : 0) ||
       (unsigned)src_y >= (unsigned)((d->v_edge >> 1) - (motion_y & QPEL_MASK) - h + 1 > 0
                                         ? (d->v_edge >> 1) - (motion_y & QPEL_MASK) - h + 1 : 0)))
    src = rc_fetch_cb_iedge(buf, ref->u, ref->v, d->lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1, d->h_edge >> 1,
                            d->v_edge >> 1, &ss);
  else
    src = rc_fetch(buf, MC_B, ref->u, d->lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                   MC_B, (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cb, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
}

static void rc_chroma_4mv_motion(Recon* d, U8* dest_cb, U8* dest_cr, const RPic* ref, int op, int mx, int my) {
  U8 buf[MC_B * MC_B];
  const U8* src;
  int src_x, src_y, dxy, ss;
  mx = round_chroma(mx);
  my = round_chroma(my);
  dxy = ((my & 1) << 1) | (mx & 1);
  mx >>= 1;
  my >>= 1;
  src_x = d->mb_x * BLK_W + mx;
  src_y = d->mb_y * BLK_W + my;
  src_x = clip(src_x, -BLK_W, d->width >> 1);
  if (src_x == (d->width >> 1)) dxy &= ~HP_H;
  src_y = clip(src_y, -BLK_W, d->height >> 1);
  if (src_y == (d->height >> 1)) dxy &= ~HP_V;
  src = rc_fetch(buf, MC_B, ref->u, d->lc, src_x, src_y, 1, MC_B, MC_B, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cb, d->lc, src, ss, BLK_W, BLK_W, dxy, op);
  src = rc_fetch(buf, MC_B, ref->v, d->lc, src_x, src_y, 1, MC_B, MC_B, d->h_edge >> 1, d->v_edge >> 1, &ss);
  rc_hpel(dest_cr, d->lc, src, ss, BLK_W, BLK_W, dxy, op);
}

static void rc_apply_8x8(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const RPic* ref, int op) {
  U8 buf[MC_MB * MC_MB];
  const U8* src;
  int dxy, mx = 0, my = 0, src_x, src_y, i, ss;
  for (i = 0; i < MB_LUMA; i++) {  // (block i: i & 1 its column, i >> 1 its row)
    const int motion_x = d->mo->mv[dir][i][0], motion_y = d->mo->mv[dir][i][1];
    U8* dest = dest_y + ((i & 1) * BLK_W) + (i >> 1) * BLK_W * d->ly;
    if (d->qpel) {
      dxy = qp(motion_x & QPEL_MASK, motion_y & QPEL_MASK);
      src_x = d->mb_x * MB_W + (motion_x >> QPEL_BITS) + (i & 1) * BLK_W;
      src_y = d->mb_y * MB_W + (motion_y >> QPEL_BITS) + (i >> 1) * BLK_W;
      src_x = clip(src_x, -MB_W, d->width);
      if (src_x == d->width) dxy &= ~qp(QPEL_MASK, 0);
      src_y = clip(src_y, -MB_W, d->height);
      if (src_y == d->height) dxy &= ~qp(0, QPEL_MASK);
      src = rc_fetch(buf, MC_B, ref->y, d->ly, src_x, src_y, 1, MC_B, MC_B, d->h_edge, d->v_edge, &ss);
      rc_qpel_mc(dest, d->ly, src, ss, BLK_W, dxy, op, (d->bugs & BUG_STD_QPEL) != 0);
      mx += motion_x / 2;
      my += motion_y / 2;
    } else {
      dxy = 0;
      src_x = d->mb_x * MB_W + (i & 1) * BLK_W + (motion_x >> HPEL_BITS);
      src_y = d->mb_y * MB_W + (i >> 1) * BLK_W + (motion_y >> HPEL_BITS);
      src_x = clip(src_x, -MB_W, d->width);
      if (src_x != d->width) dxy |= motion_x & 1;
      src_y = clip(src_y, -MB_W, d->height);
      if (src_y != d->height) dxy |= (motion_y & 1) << 1;
      src = rc_fetch(buf, MC_B, ref->y, d->ly, src_x, src_y, 1, MC_B, MC_B, d->h_edge, d->v_edge, &ss);
      rc_hpel(dest, d->ly, src, ss, BLK_W, BLK_W, dxy, op);
      mx += motion_x;
      my += motion_y;
    }
  }
  rc_chroma_4mv_motion(d, dest_cb, dest_cr, ref, op, mx, my);
}

static void rc_mpv_motion(Recon* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const RPic* ref, int op) {
  const Motion* o = d->mo;
  int i;
  switch (o->mv_type) {
  case MV_16X16:
    if (o->mcsel) {
      if (o->real_warp_points == 1) rc_gmc1_motion(d, dest_y, dest_cb, dest_cr, ref);
      else rc_gmc_motion(d, dest_y, dest_cb, dest_cr, ref);
    } else if (d->qpel)
      rc_qpel_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, o->mv[dir][0][0], o->mv[dir][0][1], MB_W);
    else
      rc_mpeg_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, o->mv[dir][0][0], o->mv[dir][0][1], MB_W);
    break;
  case MV_8X8:
    rc_apply_8x8(d, dest_y, dest_cb, dest_cr, dir, ref, op);
    break;
  case MV_FIELD:
    for (i = 0; i < 2; i++) {
      if (d->qpel)
        rc_qpel_motion(d, dest_y, dest_cb, dest_cr, 1, i, o->field_select[dir][i], ref, op, o->mv[dir][i][0],
                       o->mv[dir][i][1], MB_W / 2);
      else
        rc_mpeg_motion(d, dest_y, dest_cb, dest_cr, 1, i, o->field_select[dir][i], ref, op, o->mv[dir][i][0],
                       o->mv[dir][i][1], MB_W / 2);

    }
    break;
  }
}

//--- the model's calls ------------------------------------------------------------------------------

// the luma (plane 0) or chroma (1, 2) samples of the current picture at block n of the macroblock,
// and their stride
static U8* rc_block_at(const Recon* d, int n, int* stride) {
  const RPic* p = d->cur;
  if (n < MB_LUMA) {
    const int ls = d->ly << d->interlaced_dct, off = d->interlaced_dct ? d->ly : d->ly * BLK_W;
    *stride = ls;
    return p->y + (size_t)d->mb_y * MB_W * d->ly + d->mb_x * MB_W + (n & 1) * BLK_W + (n >> 1) * off;
  }
  *stride = d->lc;
  return (n == BLK_CB ? p->u : p->v) + (size_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
}

static void rc_init(Recon* d) {
  memset(d, 0, sizeof(*d));
  {
    const char* fn = getenv("M4VPACK_RECON");
    if (fn && *fn) d->dump = fopen(fn, "wb");
  }
}
static void rc_quit(Recon* d) {
  free(d->mem);
  d->mem = 0;
  if (d->dump) fclose(d->dump);
  d->dump = 0;
}

// a free picture buffer for the next VOP (not one of the references)
static RPic* rc_free_pic(Recon* d) {
  int i;
  for (i = 0; i < NPICS; i++)
    if (&d->pics[i] != d->last && &d->pics[i] != d->next) return &d->pics[i];
  return &d->pics[0];
}

// a gray stand-in for a reference the VOP has not, in a buffer it does not use
static RPic* rc_gray_pic(Recon* d) {
  RPic* p = &d->pics[&d->pics[0] == d->cur ? 1 : 0];
  const size_t ysz = (size_t)d->ly * d->mb_h * MB_W, csz = (size_t)d->lc * d->mb_h * BLK_W;
  memset(p->y, GRAY, ysz + 2 * csz);
  return p;
}

#include "rc_vop_begin.inc"  // rc_vop_begin(): a VOP's header

// a video packet starting at macroblock mbnum (m4vdec's decode_video_packet_header and resync,
// after the motion state's)
static void rc_vp_begin(Recon* d, const VpHdr* k) {
  if (!d->on) return;
  d->mb_x = k->mbnum % d->mb_w;
  d->mb_y = k->mbnum / d->mb_w;
  if (k->quant) d->chroma_qscale = d->qscale = k->quant;
  if (k->hec && d->pict_type == PT_S && d->mo->sprite_usage == GMC_SPRITE && !d->mo->sprite_ok) {
    d->on = 0;  // (FFmpeg would drop the packet)
    return;
  }
  rc_clean_buffers(d);
  rc_set_qscale(d, d->qscale);
}

// macroblock idx starts (decode_slice's part, after the motion state's)
static void rc_mb_begin(Recon* d, int idx) {
  d->mb_x = idx % d->mb_w;
  d->mb_y = idx / d->mb_w;
  rc_set_block_index(d);
  d->mb_intra = 0;
  d->ac_pred = 0;
}

// An inter macroblock (or a skipped one): its prediction made in the picture, by the vectors the
// motion state has for it (reconstruct_mb's part)
static void rc_mb_inter(Recon* d) {
  RPic* cur = d->cur;
  U8* dest_y = cur->y + (size_t)d->mb_y * MB_W * d->ly + d->mb_x * MB_W;
  U8* dest_cb = cur->u + (size_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
  U8* dest_cr = cur->v + (size_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
  int op = (!d->no_rounding || d->pict_type == PT_B) ? OP_PUT : OP_PUT_NORND;
  d->mb_intra = 0;
  if (d->mo->mv_dir & MV_DIR_FORWARD) {
    rc_mpv_motion(d, dest_y, dest_cb, dest_cr, 0, d->last, op);
    op = OP_AVG;
  }
  if (d->mo->mv_dir & MV_DIR_BACKWARD) rc_mpv_motion(d, dest_y, dest_cb, dest_cr, 1, d->next, op);
}

// an intra macroblock (ac: its AC prediction flag)
static void rc_mb_intra(Recon* d, int ac) {
  d->mb_intra = 1;
  d->ac_pred = ac == 1;
}

// Block n of the macroblock, its coefficients b known: reconstructed (decode_block's dequantisation
// and prediction, reconstruct_mb's transform)
// a fixed length escape's level, as FFmpeg clips it: 12 bits
static inline int esc3_clip(int level) {
  return (unsigned)(level + ESC3_MAX) > 2 * ESC3_MAX - 1 ? (level < 0 ? -ESC3_MAX : ESC3_MAX - 1) : level;
}
static void rc_block(Recon* d, int n, const Blk* b, int intra, int dcvlc) {
  S16 blk[BLK_COEFS];
  const U8* scan;
  int i, j, stride, dir;
  U8* dst = rc_block_at(d, n, &stride);
  memset(blk, 0, sizeof blk);
  if (intra) {
    const int pred = rc_pred_dc(d, n, &dir);
    i = -1;
    if (dcvlc) {
      blk[0] = (S16)rc_level_dc(d, n, pred, b->dc);
      i = 0;
    }
    scan = d->ac_pred ? (dir == 0 ? d->intra_v_scan : d->intra_h_scan) : d->intra_scan;
    for (j = 0; j < b->n; j++) {
      int level = b->level[j];
      i += b->run[j] + 1;
      if (b->esc[j] == ESC_FIXED) level = esc3_clip(level);
      blk[scan[i & BLK_LAST]] = (S16)level;
    }
    if (!dcvlc) blk[0] = (S16)rc_level_dc(d, n, pred, blk[0]);
    rc_pred_ac(d, blk, n, dir);
    rc_unquant_intra(d, blk, n, n < MB_LUMA ? d->qscale : d->chroma_qscale);
    rc_idct_put(d, dst, stride, blk);
    return;
  }
  if (!b->n) return;
  {
    int qmul = 1, qadd = 0;
    if (!d->mpeg_quant) {
      qmul = d->qscale << 1;
      qadd = (d->qscale - 1) | 1;
    }
    scan = d->intra_scan;
    for (j = 0, i = -1; j < b->n; j++) {
      int level = b->level[j];
      i += b->run[j] + 1;
      level = level < 0 ? level * qmul - qadd : level * qmul + qadd;
      if (b->esc[j] == ESC_FIXED) level = esc3_clip(level);
      blk[scan[i & BLK_LAST]] = (S16)level;
    }
    if (d->mpeg_quant) rc_unquant_inter_mpeg(d, blk, d->qscale);
    rc_idct_add(d, dst, stride, blk);
  }
}

// the macroblock's end: what later macroblocks and VOPs read of it (decode_slice's part)
static void rc_mb_end(Recon* d) {
  const int mb_xy = d->mb_y * d->mb_stride + d->mb_x;
  if (!d->mb_intra) rc_clean_intra_table_entries(d, mb_xy);
  else d->mbintra_table[mb_xy] = 1;
  d->qscale_table[mb_xy] = (S8)d->qscale;
  d->mb_done[mb_xy] = 1;
}

// the VOP's end: the macroblocks no packet had copied from the reference (m4vdec's concealment);
// with M4VPACK_RECON, the picture written
static void rc_vop_end(Recon* d) {
  int i, y;
  if (!d->on) return;
  for (i = 0; i < d->mb_num; i++) {
    const int mx = i % d->mb_w, my = i / d->mb_w;
    if (d->mb_done[my * d->mb_stride + mx]) continue;
    for (y = 0; y < MB_W; y++) {
      const size_t o = (size_t)(my * MB_W + y) * d->ly + mx * MB_W;
      if (d->last) memcpy(d->cur->y + o, d->last->y + o, MB_W);
      else memset(d->cur->y + o, GRAY, MB_W);
    }
    for (y = 0; y < BLK_W; y++) {
      const size_t o = (size_t)(my * BLK_W + y) * d->lc + mx * BLK_W;
      if (d->last) {
        memcpy(d->cur->u + o, d->last->u + o, BLK_W);
        memcpy(d->cur->v + o, d->last->v + o, BLK_W);
      } else {
        memset(d->cur->u + o, GRAY, BLK_W);
        memset(d->cur->v + o, GRAY, BLK_W);
      }
    }
  }
  if (d->dump) {  // the picture, cropped as m4vdec writes it
    const int cw = (d->width + 1) >> 1, ch = (d->height + 1) >> 1;
    for (y = 0; y < d->height; y++) fwrite(d->cur->y + (size_t)y * d->ly, 1, d->width, d->dump);
    for (y = 0; y < ch; y++) fwrite(d->cur->u + (size_t)y * d->lc, 1, cw, d->dump);
    for (y = 0; y < ch; y++) fwrite(d->cur->v + (size_t)y * d->lc, 1, cw, d->dump);
  }
  d->on = 0;
}
