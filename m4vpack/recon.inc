// recon.inc -- the decoder in the loop of level -9: m4vdec's reconstruction of the pictures (a
// port of FFmpeg's MPEG-4 Part 2 decoder, bit-exact with its C code), driven by the records the
// model codes instead of the bits. The model calls it as it codes a macroblock: at its start
// (mb_begin), once its kind and vectors are known (mb_inter, mb_intra: the motion compensated
// prediction is then in the picture), after each block's coefficients (block: the block
// reconstructed), and at its end (mb_end). So when a block is coded, the samples above and left of
// it are decoded, and its own prediction is there: what the contexts of level -9 look at.
//
// Compressing and decompressing run the same calls on the same records, so the pictures are the
// same on both sides whatever they are; m4vdec's are what they aim at (M4VPACK_RECON=file writes
// them in coding order, for comparing with a decoder; a run on one thread).

// FFmpeg's flags: a motion's directions, a macroblock's type (MB_TYPE_*), the encoders' bugs it
// works around (FF_BUG_*)
enum { MV_DIR_FORWARD = 1, MV_DIR_BACKWARD = 2, MV_DIRECT = 4 };
enum {
  MBT_INTRA = 0x0001, MBT_16x16 = 0x0002, MBT_16x8 = 0x0004, MBT_8x8 = 0x0008, MBT_INTERLACED = 0x0010,
  MBT_DIRECT2 = 0x0020, MBT_ACPRED = 0x0040, MBT_GMC = 0x0080, MBT_SKIP = 0x0100, MBT_FWD = 0x0200,
  MBT_BWD = 0x0400, MBT_BIDIR = MBT_FWD | MBT_BWD
};
enum {
  BUG_AMV = 32, BUG_QPEL_CHROMA = 64, BUG_STD_QPEL = 128,
  BUG_QPEL_CHROMA2 = 256, BUG_DIRECT_BLOCKSIZE = 512, BUG_EDGE = 1024, BUG_HPEL_CHROMA = 2048,
  BUG_DC_CLIP = 4096, BUG_IEDGE = 32768
};

enum { MV_16X16 = 0, MV_8X8 = 1, MV_FIELD = 2 };

constexpr int RC_Q = 4;              // the contexts' fixed point: quarters (of a sample, of a level)
// rc_pred_spec()'s td: which way a prediction's texture runs (its samples change more down the
// columns, neither, along the rows), or none
enum { RC_TD_COLS, RC_TD_NEITHER, RC_TD_ROWS, RC_TD_NONE };

constexpr int RC_PAIRS = 2 * (BLK_W - 1) * BLK_W;  // the pairs of neighbouring samples in an 8x8 block
enum { OP_PUT = 0, OP_PUT_NORND = 1, OP_AVG = 2 };

// the samples (8 bits): their largest, mid gray (a missing reference's); what the DC predictors
// start from (mid gray, times the DC scale of 8)
constexpr int PIX_MAX = UINT8_MAX, GRAY = 1 << (BYTE_BITS - 1);
constexpr int DC_RESET = 1024;
// the field times FFmpeg falls back on, where the field distances come out wrong
constexpr int FIELD_PB_TIME = 2, FIELD_PP_TIME = 4;

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
static inline int mid_pred(int a, int b, int c) {
  if (a > b) {
    if (c > b) b = c > a ? a : c;
  } else if (b > c)
    b = c > a ? c : a;
  return b;
}
// FFmpeg's ROUNDED_DIV and RSHIFT
static inline S64 rounded_div(S64 a, S64 b) { return (a >= 0 ? a + (b >> 1) : a - (b >> 1)) / b; }
static inline int rshift(int a, int b) { return a > 0 ? (a + ((1 << b) >> 1)) >> b : (a + ((1 << b) >> 1) - 1) >> b; }

// FFmpeg's MpegEncContext's layout: a row of macroblocks' entries and one more (mb_stride), a row
// of 8x8 blocks' and one more (b8_stride); the tables' padding (MB_PAD entries after the
// macroblocks, MV_PAD vectors before the 8x8 blocks'); a macroblock's field selects (ref_index)
constexpr int MB_PAD = 2, MV_PAD = 4, REF_INDEX = 4;
// the AC predictors of a block: its left column, then its top row (BLK_W each, the first unused)
constexpr int ACV = 2 * BLK_W;
// direct mode: the co-located vectors' components whose scalings are made once per VOP (-32..31)
constexpr int DIRECT_MVS = 64;
// the pictures: the current, the last and the next anchor
constexpr int NPICS = 3;

// a picture: its planes (rows of mb_w * MB_W luma and mb_w * BLK_W chroma samples) and what B-VOPs
// read of an anchor
typedef struct RPic {
  U8 *y, *u, *v;
  U8* mbskip;          // [mb_stride * mb_h + MB_PAD]
  U16* mbtype;         // [mb_stride * mb_h + MB_PAD]
  S16 (*mvbase)[2];    // motion vectors on the 8x8 grid (b8_stride), MV_PAD entries of padding first
  S16 (*mv)[2];
  U8* ref_index;       // [REF_INDEX * (mb_stride * mb_h + MB_PAD)] field selects of field macroblocks
} RPic;

typedef struct Recon {
  int on;              // this VOP is reconstructed
  // stream
  int width, height, mb_w, mb_h, mb_num, mb_stride, b8_stride, ly, lc;
  int progressive, mpeg_quant, qpel, sprite_usage, warp_points, warp_acc;
  U16 intra_matrix[BLK_COEFS], inter_matrix[BLK_COEFS];
  int divx_version, divx_build, xvid_build, lavc_build;
  int bugs, idct_xvid, h_edge, v_edge;
  int vol_seen, gop_seen;  // the Ctx's counts applied
  // VOP
  int pict_type, no_rounding, top_field_first, alternate_scan;
  int qscale, chroma_qscale, y_dc_scale, c_dc_scale, f_code, b_code, mcsel;
  int time_base, last_time_base;
  S64 time, last_non_b_time;
  U16 pp_time, pb_time, pp_field_time, pb_field_time;
  int t_frame;
  S16 direct_scale_mv[2][DIRECT_MVS];
  int sprite_offset[2][2], sprite_delta[2][2], sprite_shift[2], real_warp_points;
  const U8 *intra_scan, *intra_h_scan, *intra_v_scan;
  // macroblock
  int mb_x, mb_y, resync_mb_x, resync_mb_y, first_slice_line;
  int mb_intra, ac_pred, interlaced_dct, mv_dir, mv_type, mb_skipped;
  int mv[2][MB_LUMA][2], field_select[2][2], last_mv[2][2][2];
  int block_index[MB_BLOCKS], block_wrap[MB_BLOCKS];
  // prediction state (FFmpeg's MpegEncContext's layout)
  S16 *dc_val_base, *dc_val;
  S16 (*ac_val_base)[ACV], (*ac_val)[ACV];
  U8* mbintra_table;
  S8* qscale_table;
  S16 (*p_field_mv)[2][2];  // [mb_xy][field][xy]: the forward field vectors of the last P-VOP
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

// the tables for a picture of w x h, cleared as freshly allocated ones (m4vdec's dec_layout); 0 if
// there is no memory for them
static int rc_layout(Recon* d, int w, int h) {
  int i, pass, y_size, c_size, yc_size, mb_array, b8_array;
  size_t ysz, csz, at = 0;
  U8* m = 0;
  if (d->mem && d->width == w && d->height == h) return 1;
  free(d->mem);
  d->mem = 0;
  d->cur = d->last = d->next = 0;
  d->width = w;
  d->height = h;
  d->mb_w = (w + MB_W - 1) >> MB_SHIFT;
  d->mb_h = (h + MB_W - 1) >> MB_SHIFT;
  d->mb_num = d->mb_w * d->mb_h;
  d->mb_stride = d->mb_w + 1;
  d->b8_stride = d->mb_w * 2 + 1;  // (2 8x8 blocks in a macroblock's row)
  d->ly = d->mb_w * MB_W;
  d->lc = d->mb_w * BLK_W;
  y_size = d->b8_stride * (2 * d->mb_h + 1);
  c_size = d->mb_stride * (d->mb_h + 1);
  yc_size = y_size + 2 * c_size;
  mb_array = d->mb_stride * d->mb_h + MB_PAD;
  b8_array = d->b8_stride * d->mb_h * 2 + MV_PAD;
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
    d->p_field_mv = (S16(*)[2][2])take((size_t)mb_array * 2 * 2 * sizeof(S16));
    d->mbintra_table = (U8*)take((size_t)mb_array);
    d->qscale_table = (S8*)take((size_t)mb_array);
    d->mb_done = (U8*)take((size_t)mb_array);
    for (i = 0; i < NPICS; i++) {
      RPic* p = &d->pics[i];
      p->y = (U8*)take(ysz + 2 * csz);
      p->u = p->y + ysz;
      p->v = p->u + csz;
      p->mbskip = (U8*)take((size_t)mb_array);
      p->mbtype = (U16*)take((size_t)mb_array * sizeof(U16));
      p->mvbase = (S16(*)[2])take((size_t)b8_array * 2 * sizeof(S16));
      p->mv = p->mvbase + MV_PAD;
      p->ref_index = (U8*)take((size_t)REF_INDEX * mb_array);
    }
  }
  for (i = 0; i < yc_size; i++) d->dc_val_base[i] = DC_RESET;
  d->dc_val = d->dc_val_base + d->b8_stride + 1;
  d->ac_val = d->ac_val_base + d->b8_stride + 1;
  for (i = 0; i < MB_LUMA; i++) d->block_wrap[i] = d->b8_stride;
  d->block_wrap[BLK_CB] = d->block_wrap[BLK_CR] = d->mb_stride;
  return 1;
}

// the blocks' entries in the tables of the 8x8 grid (luma) and of the chroma planes' (FFmpeg's
// ff_init_block_index)
static void rc_set_block_index(Recon* d) {
  const int bs = d->b8_stride, ms = d->mb_stride;
  d->block_index[0] = bs * (d->mb_y * 2) + d->mb_x * 2;
  d->block_index[1] = d->block_index[0] + 1;
  d->block_index[2] = bs * (d->mb_y * 2 + 1) + d->mb_x * 2;
  d->block_index[3] = d->block_index[2] + 1;
  d->block_index[BLK_CB] = ms * (d->mb_y + 1) + bs * d->mb_h * 2 + d->mb_x;
  d->block_index[BLK_CR] = ms * (d->mb_y + d->mb_h + 2) + bs * d->mb_h * 2 + d->mb_x;
}


// the sprite's warping: the virtual points' precision (sixteenths of a sample: SPR_SUB), the
// warp's fixed point (FFmpeg's 16 - shift), FFmpeg's rho (3 - the warping accuracy)
constexpr int SPR_SUB = 16, SPR_FIX = 16, SPR_RHO = 3;
// a warp's corners (the VOP's), the number of them
constexpr int SPR_CORNERS = 4;

#include "rc_sprite.inc"  // rc_sprite(): GMC's warping

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
  const S16* dc_val = d->dc_val + d->block_index[n];
  const int wrap = d->block_wrap[n];
  int a = dc_val[-1], b = dc_val[-1 - wrap], c = dc_val[-wrap];
  if (d->first_slice_line && n != 3) {
    if (n != 2) b = c = DC_RESET;
    if (n != 1 && d->mb_x == d->resync_mb_x) b = a = DC_RESET;
  }
  if (d->mb_x == d->resync_mb_x && d->mb_y == d->resync_mb_y + 1) {
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
  d->last_mv[0][0][0] = d->last_mv[0][0][1] = d->last_mv[1][0][0] = d->last_mv[1][0][1] = 0;
}

//--- motion vectors --------------------------------------------------------------------------------

static S16* rc_pred_motion(Recon* d, int block, int* px, int* py) {
  static const int off[MB_LUMA] = { 2, 1, 1, -1 };  // (the above right candidate's place)
  const int wrap = d->b8_stride;
  S16 (*mot_val)[2] = d->cur->mv + d->block_index[block];
  S16 *A, *B, *C;
  A = mot_val[-1];
  if (d->first_slice_line && block < 3) {
    if (block == 0) {
      if (d->mb_x == d->resync_mb_x) {
        *px = *py = 0;
      } else if (d->mb_x + 1 == d->resync_mb_x) {
        C = mot_val[off[block] - wrap];
        if (d->mb_x == 0) {
          *px = C[0];
          *py = C[1];
        } else {
          *px = mid_pred(A[0], 0, C[0]);
          *py = mid_pred(A[1], 0, C[1]);
        }
      } else {
        *px = A[0];
        *py = A[1];
      }
    } else if (block == 1) {
      if (d->mb_x + 1 == d->resync_mb_x) {
        C = mot_val[off[block] - wrap];
        *px = mid_pred(A[0], 0, C[0]);
        *py = mid_pred(A[1], 0, C[1]);
      } else {
        *px = A[0];
        *py = A[1];
      }
    } else {
      B = mot_val[-wrap];
      C = mot_val[off[block] - wrap];
      if (d->mb_x == d->resync_mb_x) A[0] = A[1] = 0;
      *px = mid_pred(A[0], B[0], C[0]);
      *py = mid_pred(A[1], B[1], C[1]);
    }
  } else {
    B = mot_val[-wrap];
    C = mot_val[off[block] - wrap];
    *px = mid_pred(A[0], B[0], C[0]);
    *py = mid_pred(A[1], B[1], C[1]);
  }
  return *mot_val;
}

// a vector from its prediction and its difference as coded (m4vdec's decode_motion)
static int rc_motion(int pred, int diff, int f_code) { return diff ? mv_wrap(pred + diff, f_code) : pred; }

static void rc_update_motion_val(Recon* d) {
  const int mb_xy = d->mb_y * d->mb_stride + d->mb_x, wrap = d->b8_stride, xy = d->block_index[0];
  S16 (*mv)[2] = d->cur->mv;
  if (d->mv_type != MV_8X8) {
    int motion_x, motion_y;
    if (d->mb_intra) {
      motion_x = motion_y = 0;
    } else if (d->mv_type == MV_16X16) {
      motion_x = d->mv[0][0][0];
      motion_y = d->mv[0][0][1];
    } else {
      int i;
      motion_x = d->mv[0][0][0] + d->mv[0][1][0];
      motion_y = d->mv[0][0][1] + d->mv[0][1][1];
      motion_x = (motion_x >> 1) | (motion_x & 1);
      for (i = 0; i < 2; i++) {
        d->p_field_mv[mb_xy][i][0] = (S16)d->mv[0][i][0];
        d->p_field_mv[mb_xy][i][1] = (S16)d->mv[0][i][1];
      }
      U8* ri = d->cur->ref_index + REF_INDEX * mb_xy;
      ri[0] = ri[1] = (U8)d->field_select[0][0];
      ri[2] = ri[3] = (U8)d->field_select[0][1];
    }
    mv[xy][0] = mv[xy + 1][0] = mv[xy + wrap][0] = mv[xy + 1 + wrap][0] = (S16)motion_x;
    mv[xy][1] = mv[xy + 1][1] = mv[xy + wrap][1] = mv[xy + 1 + wrap][1] = (S16)motion_y;
  }
}

static int rc_get_amv(const Recon* d, int n) {
  int x, y, mb_v, sum, dx, dy, shift;
  int len = MV_VLC_MAX << (d->f_code - FCODE_MIN);
  const int a = d->warp_acc;
  if (d->bugs & BUG_AMV) len >>= d->qpel;
  if (d->real_warp_points == 1) {
    if (d->divx_version == DIVX413_VERSION && d->divx_build == DIVX413_BUILD && a >= d->qpel)
      sum = d->sprite_offset[0][n] / (1 << (a - d->qpel));
    else
      sum = rshift(d->sprite_offset[0][n] * (1 << d->qpel), a);
  } else {
    dx = d->sprite_delta[n][0];
    dy = d->sprite_delta[n][1];
    shift = d->sprite_shift[0];
    if (n) dy -= 1 << (shift + a + 1);
    else dx -= 1 << (shift + a + 1);
    mb_v = (int)(d->sprite_offset[0][n] + dx * d->mb_x * (unsigned)MB_W + dy * d->mb_y * (unsigned)MB_W);
    sum = 0;
    for (y = 0; y < MB_W; y++) {
      int v = (int)(mb_v + (unsigned)dy * y);
      for (x = 0; x < MB_W; x++) {
        sum += v >> shift;
        v += dx;
      }
    }
    sum = rshift(sum, a + 2 * MB_SHIFT - d->qpel);  // (the mean of the macroblock's samples' vectors)
  }
  if (sum < -len) sum = -len;
  else if (sum >= len) sum = len - 1;
  return sum;
}

static void rc_set_one_direct_mv(Recon* d, int mx, int my, int i) {
  const int xy = d->block_index[i];
  const U16 time_pp = d->pp_time, time_pb = d->pb_time;
  const S16* p = d->next->mv[xy];
  int p_mx = p[0], p_my = p[1];
  constexpr int H = DIRECT_MVS / 2;
  if ((unsigned)(p_mx + H) < DIRECT_MVS) {
    d->mv[0][i][0] = d->direct_scale_mv[0][p_mx + H] + mx;
    d->mv[1][i][0] = mx ? d->mv[0][i][0] - p_mx : d->direct_scale_mv[1][p_mx + H];
  } else {
    d->mv[0][i][0] = p_mx * time_pb / time_pp + mx;
    d->mv[1][i][0] = mx ? d->mv[0][i][0] - p_mx : p_mx * (time_pb - time_pp) / time_pp;
  }
  if ((unsigned)(p_my + H) < DIRECT_MVS) {
    d->mv[0][i][1] = d->direct_scale_mv[0][p_my + H] + my;
    d->mv[1][i][1] = my ? d->mv[0][i][1] - p_my : d->direct_scale_mv[1][p_my + H];
  } else {
    d->mv[0][i][1] = p_my * time_pb / time_pp + my;
    d->mv[1][i][1] = my ? d->mv[0][i][1] - p_my : p_my * (time_pb - time_pp) / time_pp;
  }
}

static int rc_set_direct_mv(Recon* d, int mx, int my) {
  const int mb_index = d->mb_x + d->mb_y * d->mb_stride;
  const int colocated = d->next->mbtype[mb_index];
  int i;
  if (colocated & MBT_8x8) {
    d->mv_type = MV_8X8;
    for (i = 0; i < MB_LUMA; i++) rc_set_one_direct_mv(d, mx, my, i);
    return MBT_DIRECT2 | MBT_8x8 | MBT_BIDIR;
  } else if (colocated & MBT_INTERLACED) {
    d->mv_type = MV_FIELD;
    for (i = 0; i < 2; i++) {
      int field_select = d->next->ref_index[REF_INDEX * mb_index + 2 * i];
      U16 time_pp, time_pb;
      const S16* pf = d->p_field_mv[mb_index][i];
      d->field_select[0][i] = field_select;
      d->field_select[1][i] = i;
      if (d->top_field_first) {
        time_pp = (U16)(d->pp_field_time - field_select + i);
        time_pb = (U16)(d->pb_field_time - field_select + i);
      } else {
        time_pp = (U16)(d->pp_field_time + field_select - i);
        time_pb = (U16)(d->pb_field_time + field_select - i);
      }
      if (!time_pp) time_pp = 1;  // (only in streams FFmpeg would not decode: kept from dividing by 0)
      d->mv[0][i][0] = pf[0] * time_pb / time_pp + mx;
      d->mv[0][i][1] = pf[1] * time_pb / time_pp + my;
      d->mv[1][i][0] = mx ? d->mv[0][i][0] - pf[0] : pf[0] * (time_pb - time_pp) / time_pp;
      d->mv[1][i][1] = my ? d->mv[0][i][1] - pf[1] : pf[1] * (time_pb - time_pp) / time_pp;
    }
    return MBT_DIRECT2 | MBT_16x8 | MBT_BIDIR | MBT_INTERLACED;
  } else {
    rc_set_one_direct_mv(d, mx, my, 0);
    for (i = 1; i < MB_LUMA; i++) {
      d->mv[0][i][0] = d->mv[0][0][0];
      d->mv[0][i][1] = d->mv[0][0][1];
      d->mv[1][i][0] = d->mv[1][0][0];
      d->mv[1][i][1] = d->mv[1][0][1];
    }
    // FFmpeg tests avctx->workaround_bugs here (the user option), not the autodetected flags, so the
    // detected FF_BUG_DIRECT_BLOCKSIZE never takes effect; m4vdec matches that
    d->mv_type = !d->qpel ? MV_16X16 : MV_8X8;
    return MBT_DIRECT2 | MBT_16x16 | MBT_BIDIR;
  }
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
  U8 buf[MC_MB * MC_MB];
  const U8* src;
  int src_x, src_y, motion_x, motion_y, ss;
  motion_x = d->sprite_offset[0][0];
  motion_y = d->sprite_offset[0][1];
  src_x = d->mb_x * MB_W + (motion_x >> (d->warp_acc + 1));
  src_y = d->mb_y * MB_W + (motion_y >> (d->warp_acc + 1));
  motion_x *= 1 << (GMC1_SUB - d->warp_acc);
  motion_y *= 1 << (GMC1_SUB - d->warp_acc);
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
  motion_x = d->sprite_offset[1][0];
  motion_y = d->sprite_offset[1][1];
  src_x = d->mb_x * BLK_W + (motion_x >> (d->warp_acc + 1));
  src_y = d->mb_y * BLK_W + (motion_y >> (d->warp_acc + 1));
  motion_x *= 1 << (GMC1_SUB - d->warp_acc);
  motion_y *= 1 << (GMC1_SUB - d->warp_acc);
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
  const int a = d->warp_acc;
  const int dl = d->ly, dc = d->lc;
  int ox, oy;
  // (FFmpeg calls the gmc dsp with the destination's stride for the source too: the same)
  ox = d->sprite_offset[0][0] + d->sprite_delta[0][0] * d->mb_x * MB_W + d->sprite_delta[0][1] * d->mb_y * MB_W;
  oy = d->sprite_offset[0][1] + d->sprite_delta[1][0] * d->mb_x * MB_W + d->sprite_delta[1][1] * d->mb_y * MB_W;
  rc_gmc_dsp(dest_y, dl, ref->y, MB_W, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1], d->sprite_delta[1][0],
             d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, d->h_edge, d->v_edge);
  rc_gmc_dsp(dest_y + BLK_W, dl, ref->y, MB_W, ox + d->sprite_delta[0][0] * BLK_W, oy + d->sprite_delta[1][0] * BLK_W,
             d->sprite_delta[0][0], d->sprite_delta[0][1], d->sprite_delta[1][0], d->sprite_delta[1][1], a + 1,
             (1 << (2 * a + 1)) - d->no_rounding, d->h_edge, d->v_edge);
  ox = d->sprite_offset[1][0] + d->sprite_delta[0][0] * d->mb_x * BLK_W + d->sprite_delta[0][1] * d->mb_y * BLK_W;
  oy = d->sprite_offset[1][1] + d->sprite_delta[1][0] * d->mb_x * BLK_W + d->sprite_delta[1][1] * d->mb_y * BLK_W;
  rc_gmc_dsp(dest_cb, dc, ref->u, BLK_W, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1], d->sprite_delta[1][0],
             d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, (d->h_edge + 1) >> 1,
             (d->v_edge + 1) >> 1);
  rc_gmc_dsp(dest_cr, dc, ref->v, BLK_W, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1], d->sprite_delta[1][0],
             d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding, (d->h_edge + 1) >> 1,
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
    const int motion_x = d->mv[dir][i][0], motion_y = d->mv[dir][i][1];
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
  int i;
  switch (d->mv_type) {
  case MV_16X16:
    if (d->mcsel) {
      if (d->real_warp_points == 1) rc_gmc1_motion(d, dest_y, dest_cb, dest_cr, ref);
      else rc_gmc_motion(d, dest_y, dest_cb, dest_cr, ref);
    } else if (d->qpel)
      rc_qpel_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, d->mv[dir][0][0], d->mv[dir][0][1], MB_W);
    else
      rc_mpeg_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, d->mv[dir][0][0], d->mv[dir][0][1], MB_W);
    break;
  case MV_8X8:
    rc_apply_8x8(d, dest_y, dest_cb, dest_cr, dir, ref, op);
    break;
  case MV_FIELD:
    for (i = 0; i < 2; i++) {
      if (d->qpel)
        rc_qpel_motion(d, dest_y, dest_cb, dest_cr, 1, i, d->field_select[dir][i], ref, op, d->mv[dir][i][0],
                       d->mv[dir][i][1], MB_W / 2);
      else
        rc_mpeg_motion(d, dest_y, dest_cb, dest_cr, 1, i, d->field_select[dir][i], ref, op, d->mv[dir][i][0],
                       d->mv[dir][i][1], MB_W / 2);

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
  memset(p->mbskip, 0, d->mb_stride * d->mb_h + MB_PAD);
  memset(p->mbtype, 0, (d->mb_stride * d->mb_h + MB_PAD) * sizeof(U16));
  memset(p->mvbase, 0, (d->b8_stride * d->mb_h * 2 + MV_PAD) * sizeof(*p->mvbase));
  return p;
}

#include "rc_vop_begin.inc"  // rc_vop_begin(): a VOP's header

// a video packet starting at macroblock mbnum (m4vdec's decode_video_packet_header and resync)
static void rc_vp_begin(Recon* d, const VpHdr* k) {
  if (!d->on) return;
  d->mb_x = k->mbnum % d->mb_w;
  d->mb_y = k->mbnum / d->mb_w;
  if (k->quant) d->chroma_qscale = d->qscale = k->quant;
  if (k->hec && d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE && !rc_sprite(d, k->warp)) {
    d->on = 0;  // (FFmpeg would drop the packet)
    return;
  }
  rc_clean_buffers(d);
  d->first_slice_line = 1;
  d->resync_mb_x = d->mb_x;
  d->resync_mb_y = d->mb_y;
  rc_set_qscale(d, d->qscale);
}

// macroblock idx starts (decode_slice's part)
static void rc_mb_begin(Recon* d, int idx) {
  d->mb_x = idx % d->mb_w;
  d->mb_y = idx / d->mb_w;
  rc_set_block_index(d);
  if (d->resync_mb_x == d->mb_x && d->resync_mb_y + 1 == d->mb_y) d->first_slice_line = 0;
  d->mv_dir = MV_DIR_FORWARD;
  d->mv_type = MV_16X16;
  d->mb_intra = 0;
  d->ac_pred = 0;
}

#include "rc_mb_inter.inc"  // rc_mb_inter(): an inter macroblock's prediction

// an intra macroblock of m (ac: its AC prediction flag)
static void rc_mb_intra(Recon* d, int ac) {
  const int xy = d->mb_x + d->mb_y * d->mb_stride;
  d->mb_intra = 1;
  d->mcsel = 0;
  d->ac_pred = ac == 1;
  d->cur->mbtype[xy] = d->ac_pred ? (MBT_INTRA | MBT_ACPRED) : MBT_INTRA;
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
  if (d->pict_type != PT_B) rc_update_motion_val(d);
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

//--- sign prediction ------------------------------------------------------------------------------
// h264p's, for MPEG-4's 8x8 blocks: a block's first signs (in scan order) are the combination
// whose block continues the decoded samples above and left of it best. Its edges, the top row and
// the left column, are the prediction (an inter block's, in the picture until the block is
// reconstructed) plus the coefficients' contributions, the transform taken as linear (the DCT's
// basis, times 2^12). The cost of a combination is the sum over the edge samples of how far each
// is from what continues the two decoded rows (columns) R1, R2 beyond it, two ways: 2 R1 - R2
// (ECM's sign prediction), and R1 (MPEG-4 has no deblocking: where the samples are noisy, the
// extrapolation is too).

constexpr int RC_SPK = 8;  // the signs searched (2^RC_SPK combinations)
// the DCT's basis, x 2^RC_BBITS; the samples' scale, its square
constexpr int RC_BBITS = 12, RC_SBITS = 2 * RC_BBITS;
constexpr S64 RC_S24 = (S64)1 << RC_SBITS;
// the targets: what continues the decoded samples two ways (2 R1 - R2, R1)
enum { RC_T_EXTRAP, RC_T_NEXT, RC_SPT };
// a block's edge: its top row, then its left column
constexpr int RC_EDGE = 2 * BLK_W;
constexpr S64 RC_COST_MAX = (S64)1 << 62;    // (more than any combination's cost)
constexpr int RC_MARGIN_MAX = 1 << 20;       // rc_sign_predict()'s margin, capped
static const short rc_basis[BLK_W][BLK_W] = {  // C(f)/2 cos((2s+1) f pi/16) x 4096: [frequency][sample]
  { 1448, 1448, 1448, 1448, 1448, 1448, 1448, 1448 },
  { 2009, 1703, 1138, 400, -400, -1138, -1703, -2009 },
  { 1892, 784, -784, -1892, -1892, -784, 784, 1892 },
  { 1703, -400, -2009, -1138, 1138, 2009, 400, -1703 },
  { 1448, -1448, -1448, 1448, 1448, -1448, -1448, 1448 },
  { 1138, -2009, 400, 1703, -1703, -400, 2009, -1138 },
  { 784, -1892, 1892, -784, -784, 1892, -1892, 784 },
  { 400, -1138, 1703, -2009, 2009, -1703, 1138, -400 } };

typedef struct RcSign {
  int nc;                       // the signs searched (0: no prediction)
  S64 t[RC_SPT][RC_EDGE];       // the targets at the edge samples (0-7 the top row, 8-15 the left
  U8 av[RC_EDGE];               // column; av: decoded there), times 2^24
  S64 x[RC_EDGE];               // the edge samples with every searched sign +
  S64 dl[RC_SPK][RC_EDGE];      // a searched coefficient's change to them, from + to -
  S64 cost[RC_SPT][1 << RC_SPK];  // by combination (bit j: sign j negative)
  int fixed_mask, fixed_val;
} RcSign;

// the value the transform takes of level a (the actual level: for an intra block, the prediction
// included) at raster position r of block n
static int rc_deq(const Recon* d, int n, int intra, int r, int a) {
  int v, aa = a < 0 ? -a : a;
  if (!a) return 0;
  if (intra && !r) return a * (n < MB_LUMA ? d->y_dc_scale : d->c_dc_scale);
  if (d->mpeg_quant)
    v = intra ? (aa * 2 * d->qscale * d->intra_matrix[r]) >> QM_INTRA_SHIFT
              : ((2 * aa + 1) * 2 * d->qscale * d->inter_matrix[r]) >> QM_INTER_SHIFT;
  else
    v = aa * 2 * d->qscale + ((d->qscale - 1) | 1);
  return a < 0 ? -v : v;
}

// What an intra block n adds to its coded levels (as rc_block() will; changes nothing): its DC
// prediction (in levels) at 0, its AC prediction at the first column (*dir 0, the vertical scan)
// or row (1, the horizontal one)
static void rc_intra_pred(const Recon* d, int n, int* dir, int P[BLK_COEFS]) {
  const int pred = rc_pred_dc(d, n, dir), scale = n < MB_LUMA ? d->y_dc_scale : d->c_dc_scale;
  const S16* ac_val = d->ac_val[d->block_index[n]];
  const S8* qt = d->qscale_table;
  int i;
  memset(P, 0, BLK_COEFS * sizeof(int));
  P[0] = (pred + (scale >> 1)) / scale;
  if (!d->ac_pred) return;
  if (*dir == 0) {
    const int xy = d->mb_x - 1 + d->mb_y * d->mb_stride;
    ac_val -= ACV;
    for (i = 1; i < BLK_W; i++)
      P[i << BLK_SHIFT] = d->mb_x == 0 || d->qscale == qt[xy] || n == 1 || n == 3
                              ? ac_val[i] : rounded_div(ac_val[i] * qt[xy], d->qscale);
  } else {
    const int xy = d->mb_x + d->mb_y * d->mb_stride - d->mb_stride;
    ac_val -= ACV * d->block_wrap[n];
    for (i = 1; i < BLK_W; i++)
      P[i] = d->mb_y == 0 || d->qscale == qt[xy] || n == 2 || n == 3
                 ? ac_val[i + BLK_W] : rounded_div(ac_val[i + BLK_W] * qt[xy], d->qscale);
  }
}

// the scan of block n (an intra block with AC prediction: by its DC prediction's direction)
static const U8* rc_scan(const Recon* d, int n, int intra) {
  int dir;
  if (!intra || !d->ac_pred) return d->intra_scan;
  rc_pred_dc(d, n, &dir);
  return dir == 0 ? d->intra_v_scan : d->intra_h_scan;
}

// the contribution of the value v at raster position r to the edge samples, added to x
static void rc_edge_add(S64* x, int r, S64 v) {
  const short *bv = rc_basis[r >> BLK_SHIFT], *bh = rc_basis[r & (BLK_W - 1)];
  int e;
  if (!v) return;
  for (e = 0; e < BLK_W; e++) x[e] += v * bv[0] * bh[e];
  for (e = 0; e < BLK_W; e++) x[BLK_W + e] += v * bv[e] * bh[0];
}

// whether block n's signs can be predicted: an edge of it decoded, not the luma of a field DCT
// macroblock
static int rc_sign_can(const Recon* d, int n) {
  if (n < MB_LUMA) return !d->interlaced_dct && (d->mb_x || d->mb_y || n);
  return d->mb_x || d->mb_y;
}

// block n's top left sample, in its plane (field: the luma of a field DCT macroblock, whose blocks'
// rows are a field's: by its first row's)
static void rc_blk_xy(const Recon* d, int n, int field, int* bx, int* by) {
  if (n < MB_LUMA) {
    *bx = d->mb_x * MB_W + (n & 1) * BLK_W;
    *by = d->mb_y * MB_W + (n >> 1) * (field ? 1 : BLK_W);
  } else {
    *bx = d->mb_x * BLK_W;
    *by = d->mb_y * BLK_W;
  }
}
static const U8* rc_plane(const RPic* p, int n) { return n < MB_LUMA ? p->y : n == BLK_CB ? p->u : p->v; }

// The search for block n (intra, dcvlc and dc: an intra block's DC difference coded apart) whose k
// coefficients are at the scan positions pos[] with the levels lev[], of which the first nc signs
// are not known (lev[] their magnitudes there): each combination's cost. 0: no prediction.
static int rc_sign_init(const Recon* d, RcSign* sp, int n, int intra, int dcvlc, int dc, int k, const int* pos,
                        const int* lev, int nc) {
  const RPic* p = d->cur;
  const U8 *plane, *scan, *q;
  int P[BLK_COEFS], C[BLK_COEFS], cand[BLK_COEFS], stride, bx, by, e, j, r, mask, i, w, dir = 0;
  sp->nc = 0;
  if (nc <= 0 || !rc_sign_can(d, n)) return 0;
  plane = rc_plane(p, n);
  stride = n < MB_LUMA ? d->ly : d->lc;
  rc_blk_xy(d, n, 0, &bx, &by);
  q = plane + (ptrdiff_t)by * stride + bx;  // the block
  for (e = 0; e < BLK_W; e++) {
    sp->av[e] = by > 0;
    sp->av[BLK_W + e] = bx > 0;
    if (by > 0) {
      sp->t[RC_T_EXTRAP][e] = (S64)(2 * q[e - stride] - q[e - 2 * stride]) * RC_S24;
      sp->t[RC_T_NEXT][e] = (S64)q[e - stride] * RC_S24;
    }
    if (bx > 0) {
      sp->t[RC_T_EXTRAP][BLK_W + e] = (S64)(2 * q[e * stride - 1] - q[e * stride - 2]) * RC_S24;
      sp->t[RC_T_NEXT][BLK_W + e] = (S64)q[e * stride - 1] * RC_S24;
    }
    sp->x[e] = intra ? 0 : (S64)q[e] * RC_S24;
    sp->x[BLK_W + e] = intra ? 0 : (S64)q[e * stride] * RC_S24;
  }
  if (nc > RC_SPK) nc = RC_SPK;
  if (nc > k) nc = k;
  scan = d->intra_scan;
  if (intra) {
    rc_intra_pred(d, n, &dir, P);
    if (d->ac_pred) scan = dir == 0 ? d->intra_v_scan : d->intra_h_scan;
  } else
    memset(P, 0, sizeof P);
  memset(C, 0, sizeof C);
  memset(cand, 0, sizeof cand);
  if (intra && dcvlc) C[0] = dc;
  for (j = 0; j < k; j++) {
    r = scan[pos[j] & BLK_LAST];
    if (j < nc) cand[r] = 1 + j;
    else C[r] = lev[j];
  }
  for (r = 0; r < BLK_COEFS; r++)  // what the searched signs do not change
    if (!cand[r] && (P[r] || C[r])) rc_edge_add(sp->x, r, rc_deq(d, n, intra, r, P[r] + C[r]));
  for (j = 0; j < nc; j++) {
    S64 t[RC_EDGE];
    int a, vp;
    r = scan[pos[j] & BLK_LAST];
    a = lev[j] < 0 ? -lev[j] : lev[j];
    vp = rc_deq(d, n, intra, r, P[r] + a);
    rc_edge_add(sp->x, r, vp);
    memset(t, 0, sizeof t);
    rc_edge_add(t, r, rc_deq(d, n, intra, r, P[r] - a) - vp);
    memcpy(sp->dl[j], t, sizeof t);
  }
  {
    // (the edge samples not decoded zeroed, which then add nothing: loops of 16 without a branch)
    S64 x[RC_EDGE], tt[RC_SPT][RC_EDGE], dd[RC_SPK][RC_EDGE];
    for (e = 0; e < RC_EDGE; e++) {
      const S64 m = sp->av[e] ? -1 : 0;
      x[e] = sp->x[e] & m;
      for (w = 0; w < RC_SPT; w++) tt[w][e] = sp->t[w][e] & m;
      for (j = 0; j < nc; j++) dd[j][e] = sp->dl[j][e] & m;
    }
    for (i = 0, mask = 0; i < 1 << nc; i++) {
      if (i) {  // Gray code: one sign flips
        const S64* q = dd[j = __builtin_ctz((unsigned)i)];
        if (mask >> j & 1)
          for (e = 0; e < RC_EDGE; e++) x[e] -= q[e];
        else
          for (e = 0; e < RC_EDGE; e++) x[e] += q[e];
        mask ^= 1 << j;
      }
      for (w = 0; w < RC_SPT; w++) {
        S64 c = 0;
        for (e = 0; e < RC_EDGE; e++) {
          const S64 v = x[e] - tt[w][e];
          c += v < 0 ? -v : v;
        }
        sp->cost[w][mask] = c;
      }
    }
  }
  sp->nc = nc;
  sp->fixed_mask = sp->fixed_val = 0;
  return nc;
}

// the predicted sign of searched coefficient j (1 negative) by target w, given the ones decided,
// and by how much the other one costs more (quarter samples)
static int rc_sign_predict(const RcSign* sp, int w, int j, int* margin) {
  const S64* cost = sp->cost[w];
  S64 c0 = RC_COST_MAX, c1 = RC_COST_MAX, dd;
  int m, f;
  if (sp->fixed_mask == (1 << j) - 1)  // (the signs before j decided, as the model codes them: the rest free)
    for (f = 0; f < 1 << (sp->nc - j); f++) {
      m = sp->fixed_val | f << j;
      if (f & 1) {
        if (cost[m] < c1) c1 = cost[m];
      } else if (cost[m] < c0)
        c0 = cost[m];
    }
  else
    for (m = 0; m < 1 << sp->nc; m++) {
      if ((m & sp->fixed_mask) != sp->fixed_val) continue;
      if (m >> j & 1) {
        if (cost[m] < c1) c1 = cost[m];
      } else if (cost[m] < c0)
        c0 = cost[m];
    }
  dd = ((c0 > c1 ? c0 - c1 : c1 - c0) * RC_Q) >> RC_SBITS;
  *margin = dd > RC_MARGIN_MAX ? RC_MARGIN_MAX : (int)dd;

  return c1 < c0;
}

static void rc_sign_decide(RcSign* sp, int j, int negative) {
  sp->fixed_mask |= 1 << j;
  if (negative) sp->fixed_val |= 1 << j;
}

//--- the template -------------------------------------------------------------------------------
// The decoded samples above (two rows) and left (two columns) of the macroblock, which the
// reference is searched for: where they match, the macroblock has likely moved from.
constexpr int RC_TM_T = 2;                     // its thickness
constexpr int RC_TM_N = 2 * RC_TM_T * MB_W;    // its samples, all decoded

// a sample of plane p (w x h, stride s) at (x, y), clamped into it
static inline int rc_px(const U8* p, int s, int w, int h, int x, int y) {
  return p[(ptrdiff_t)clip(y, 0, h - 1) * s + clip(x, 0, w - 1)];
}

// the summed difference between 16 samples c and the reference's at a (b: the row below), at
// half-sample offsets (fx, fy), as rc_tm_sad() takes them (SSE2's averages round the same: of four,
// (sum + 2) >> 2)
static INLINE int rc_sad16(const U8* c, const U8* a, const U8* b, int fx, int fy) {
#if defined(__SSE2__)
  static_assert(MB_W == sizeof(__m128i), "rc_sad16(): a row of a macroblock, a 128-bit vector");
  const __m128i cv = _mm_loadu_si128((const __m128i*)c);
  __m128i rv, s;
  if (!(fx | fy))
    rv = _mm_loadu_si128((const __m128i*)a);
  else if (!fy)
    rv = _mm_avg_epu8(_mm_loadu_si128((const __m128i*)a), _mm_loadu_si128((const __m128i*)(a + 1)));
  else if (!fx)
    rv = _mm_avg_epu8(_mm_loadu_si128((const __m128i*)a), _mm_loadu_si128((const __m128i*)b));
  else {
    const __m128i z = _mm_setzero_si128(), two = _mm_set1_epi16(2);
    const __m128i p0 = _mm_loadu_si128((const __m128i*)a), p1 = _mm_loadu_si128((const __m128i*)(a + 1));
    const __m128i p2 = _mm_loadu_si128((const __m128i*)b), p3 = _mm_loadu_si128((const __m128i*)(b + 1));
    __m128i lo = _mm_add_epi16(_mm_add_epi16(_mm_unpacklo_epi8(p0, z), _mm_unpacklo_epi8(p1, z)),
                               _mm_add_epi16(_mm_unpacklo_epi8(p2, z), _mm_unpacklo_epi8(p3, z)));
    __m128i hi = _mm_add_epi16(_mm_add_epi16(_mm_unpackhi_epi8(p0, z), _mm_unpackhi_epi8(p1, z)),
                               _mm_add_epi16(_mm_unpackhi_epi8(p2, z), _mm_unpackhi_epi8(p3, z)));
    lo = _mm_srli_epi16(_mm_add_epi16(lo, two), 2);
    hi = _mm_srli_epi16(_mm_add_epi16(hi, two), 2);
    rv = _mm_packus_epi16(lo, hi);
  }
  s = _mm_sad_epu8(cv, rv);
  return _mm_cvtsi128_si32(s) + _mm_extract_epi16(s, sizeof(__m128i) / 2 / sizeof(S16));  // (+ the upper half's)
#else
  int x, t = 0;
  for (x = 0; x < MB_W; x++)
    t += abs(c[x] - (fx | fy ? (a[x] + a[x + fx] + b[x] + b[x + fx] + 2) >> 2 : a[x]));
  return t;
#endif
}

// the template's (the decoded samples above and left of the macroblock) summed difference from
// ref at a displacement of (hx, hy) half samples; *n its samples (0: none decoded)
static int rc_tm_sad(const Recon* d, const U8* ref, int hx, int hy, int* n) {
  constexpr int T = RC_TM_T;
  const U8* cur = d->cur->y;
  const int w = d->mb_w * MB_W, h = d->mb_h * MB_W, bx = d->mb_x * MB_W, by = d->mb_y * MB_W, ly = d->ly;
  const int ix = hx >> 1, iy = hy >> 1, fx = hx & 1, fy = hy & 1;
  int x, y, k = 0, s = 0;
  if (by >= T && bx >= T && bx - T + ix >= 0 && by - T + iy >= 0 && bx + MB_W + 1 + ix < w && by + MB_W + 1 + iy < h) {
    // (inside the picture: no clamping)
    const U8* r = ref + (ptrdiff_t)iy * ly + ix;
    for (y = by - T; y < by; y++) {
      const U8 *a = r + (ptrdiff_t)y * ly + bx;
      s += rc_sad16(cur + (ptrdiff_t)y * ly + bx, a, a + fy * ly, fx, fy);
    }
    for (y = by; y < by + MB_W; y++) {
      const U8 *c = cur + (ptrdiff_t)y * ly, *a = r + (ptrdiff_t)y * ly, *b = a + fy * ly;
      for (x = bx - T; x < bx; x++)
        s += abs(c[x] - (fx | fy ? (a[x] + a[x + fx] + b[x] + b[x + fx] + 2) >> 2 : a[x]));
    }
    *n = RC_TM_N;
    return s;
  }
  // (at the picture's edge: the reference's samples clamped into it)
  auto tmv = [&](int X, int Y) {
    return fx | fy ? (rc_px(ref, ly, w, h, X, Y) + rc_px(ref, ly, w, h, X + fx, Y) + rc_px(ref, ly, w, h, X, Y + fy) +
                      rc_px(ref, ly, w, h, X + fx, Y + fy) + 2) >> 2
                   : rc_px(ref, ly, w, h, X, Y);
  };
  if (by >= T)
    for (y = by - T; y < by; y++)
      for (x = bx; x < bx + MB_W; x++, k++) s += abs(cur[(ptrdiff_t)y * ly + x] - tmv(x + ix, y + iy));
  if (bx >= T)
    for (y = by; y < by + MB_W; y++)
      for (x = bx - T; x < bx; x++, k++) s += abs(cur[(ptrdiff_t)y * ly + x] - tmv(x + ix, y + iy));
  *n = k;
  return s;
}

//--- the intra estimate -------------------------------------------------------------------------
// h264p's largest context, an intra macroblock's coefficients estimated from the reference: its
// blocks as the reference has them where its neighbours' motion puts it (refined by matching the
// decoded samples above and left of it, the template, against the reference), transformed as the
// encoder transformed the macroblock. What it would have coded had the picture only moved.

typedef struct RcEst {
  int ok;        // made (an intra macroblock, a reference)
  int dx, dy;    // where, in full samples
  int miss;      // how far the template is from the reference there: its mean difference x 4 (-1 none)
  int F[MB_BLOCKS][BLK_COEFS];  // the reference's blocks transformed (the decoder's scale: a flat 128 is 1024)
} RcEst;
// its candidates (none, the left, above and above right macroblocks' vectors, the co-located
// one's), the search around the best (full samples)
constexpr int RC_EST_CANDS = 5, RC_EST_RANGE = 2;

// the 8x8 forward DCT of the samples at (x, y) of plane p (rows ystep apart), clamped into it,
// as the decoder's transform scales it
static void rc_fdct(int F[BLK_COEFS], const U8* p, int s, int w, int h, int x, int y, int ystep) {
  S64 t[BLK_W][BLK_W];
  int i, j, u, v;
  for (i = 0; i < BLK_W; i++)
    for (u = 0; u < BLK_W; u++) {
      S64 a = 0;
      for (j = 0; j < BLK_W; j++) a += (S64)rc_px(p, s, w, h, x + j, y + i * ystep) * rc_basis[u][j];
      t[i][u] = a;
    }
  for (v = 0; v < BLK_W; v++)
    for (u = 0; u < BLK_W; u++) {
      S64 a = 0;
      for (i = 0; i < BLK_W; i++) a += t[i][u] * rc_basis[v][i];
      F[v * BLK_W + u] = (int)((a + RC_S24 / 2) >> RC_SBITS);
    }
}

// the luma blocks of the estimate (field: those of a field DCT macroblock)
static void rc_est_luma(const Recon* d, RcEst* e, int field) {
  const RPic* r = d->last;
  const int w = d->mb_w * MB_W, h = d->mb_h * MB_W;
  int n, bx, by;
  for (n = 0; n < MB_LUMA; n++) {
    rc_blk_xy(d, n, field, &bx, &by);
    rc_fdct(e->F[n], r->y, d->ly, w, h, bx + e->dx, by + e->dy, field ? 2 : 1);
  }
}

// The estimate for the current (intra) macroblock: the displacement among the vectors of the left,
// above and above right macroblocks, the co-located one's in the reference and none, the one whose
// template matches best, refined by a search of RC_EST_RANGE samples around it
static void rc_intra_est(const Recon* d, RcEst* e) {
  const RPic* ref = d->last;
  const int qs = 1 + d->qpel, wrap = d->b8_stride, bi = d->block_index[0];
  int cand[RC_EST_CANDS][2], nc = 0, i, bx, by, best = -1, bn = 0, bs = 0, dx, dy;
  e->ok = 0;
  if (!ref) return;
  cand[nc][0] = cand[nc][1] = 0, nc++;
  if (d->mb_x > 0) cand[nc][0] = d->cur->mv[bi - 1][0] >> qs, cand[nc][1] = d->cur->mv[bi - 1][1] >> qs, nc++;
  if (d->mb_y > 0) {
    cand[nc][0] = d->cur->mv[bi - wrap][0] >> qs, cand[nc][1] = d->cur->mv[bi - wrap][1] >> qs, nc++;
    if (d->mb_x + 1 < d->mb_w)
      cand[nc][0] = d->cur->mv[bi + 2 - wrap][0] >> qs, cand[nc][1] = d->cur->mv[bi + 2 - wrap][1] >> qs, nc++;
  }
  cand[nc][0] = ref->mv[bi][0] >> qs, cand[nc][1] = ref->mv[bi][1] >> qs, nc++;
  bx = by = 0;
  for (i = 0; i < nc; i++) {
    int n;
    const int s = rc_tm_sad(d, ref->y, 2 * cand[i][0], 2 * cand[i][1], &n);
    if (!n) break;  // (no template: the first candidate)
    if (best < 0 || s < bs) best = i, bn = n, bs = s, bx = cand[i][0], by = cand[i][1];
  }
  if (bn) {
    const int cx = bx, cy = by;
    for (dy = -RC_EST_RANGE; dy <= RC_EST_RANGE; dy++)
      for (dx = -RC_EST_RANGE; dx <= RC_EST_RANGE; dx++) {
        int n;
        const int s = dx || dy ? rc_tm_sad(d, ref->y, 2 * (cx + dx), 2 * (cy + dy), &n) : bs;
        if (s < bs) bs = s, bx = cx + dx, by = cy + dy;
      }
  }
  e->dx = bx;
  e->dy = by;
  e->miss = bn ? RC_Q * bs / bn : -1;
  rc_est_luma(d, e, 0);
  {
    const int w = d->mb_w * BLK_W, h = d->mb_h * BLK_W, cx = d->mb_x * BLK_W + (bx >> 1), cy = d->mb_y * BLK_W + (by >> 1);
    rc_fdct(e->F[BLK_CB], ref->u, d->lc, w, h, cx, cy, 1);
    rc_fdct(e->F[BLK_CR], ref->v, d->lc, w, h, cx, cy, 1);
  }
  e->ok = 1;
}

// the estimate's level at raster position r of block n, in quarter levels (signed): the actual
// level, before the intra prediction
static int rc_est_q4(const Recon* d, const RcEst* e, int n, int r) {
  const int f = e->F[n][r], a = f < 0 ? -f : f;
  int v;
  if (!r) {
    const int sc = n < MB_LUMA ? d->y_dc_scale : d->c_dc_scale;
    return (RC_Q * f + (f < 0 ? -sc / 2 : sc / 2)) / sc;
  }
  // (the inverse of the quantisation the decoder undoes: MPEG's level x qscale x weight / 8,
  // H.263's level x 2 qscale)
  if (d->mpeg_quant) v = RC_Q * (1 << (QM_INTRA_SHIFT - 1)) * a / (d->qscale * d->intra_matrix[r]);
  else v = RC_Q * a / (2 * d->qscale);
  return f < 0 ? -v : v;
}


//--- what an inter block's prediction says -------------------------------------------------------
// Where the prediction is textured, motion compensation misses more; where its edges do not
// continue the decoded samples around the block, the residual has to make up the difference.

// a mean difference (in quarter samples, at least 0) against the quantiser: in RC_QR-ths of qscale
constexpr int RC_QR = 8;
static int rc_qratio(const Recon* d, int v4) { return v4 * RC_QR / (RC_Q * d->qscale); }

// Block n's prediction (in the picture, before its residual): tx its texture (the mean difference
// between neighbouring samples), em how far its top row and left column are from the decoded
// samples next to them (-1 no edge decoded), both rc_qratio(); full: every edge decoded so far, else only those on
// other macroblocks (as the coded block pattern is coded, before any block of the macroblock);
// field: the luma in fields (a field DCT macroblock's blocks)
static void rc_pred_feat(const Recon* d, int n, int full, int field, int* tx, int* em) {
  const U8* plane = rc_plane(d->cur, n);
  const U8* q;
  int stride, bx, by, x, y, s = 0, e = 0, ne = 0, top, left;
  rc_blk_xy(d, n, field, &bx, &by);
  if (n < MB_LUMA) {
    stride = d->ly << field;  // (from row to row of the block; by is a row of the picture)
    top = field ? 0 : (n >> 1) ? full : d->mb_y > 0;
    left = (n & 1) ? full : d->mb_x > 0;
  } else {
    stride = d->lc;
    top = d->mb_y > 0;
    left = d->mb_x > 0;
  }
  q = plane + (ptrdiff_t)by * (n < MB_LUMA ? d->ly : d->lc) + bx;
  for (y = 0; y < BLK_W; y++)
    for (x = 0; x < BLK_W; x++) {
      if (x < BLK_W - 1) s += abs(q[y * stride + x + 1] - q[y * stride + x]);
      if (y < BLK_W - 1) s += abs(q[(y + 1) * stride + x] - q[y * stride + x]);
    }
  if (top)
    for (x = 0; x < BLK_W; x++, ne++) e += abs(q[x] - q[x - stride]);
  if (left)
    for (y = 0; y < BLK_W; y++, ne++) e += abs(q[y * stride] - q[y * stride - 1]);
  *tx = rc_qratio(d, s * RC_Q / RC_PAIRS);
  *em = ne ? rc_qratio(d, e * RC_Q / ne) : -1;
}

// An inter block's coefficients as its edges ask for them: how far the prediction's top row and left
// column are from the decoded samples next to them, projected onto each coefficient's pattern there
// (least squares, a coefficient at a time), in quarter levels (signed) by raster position; 0 if no
// edge of the block is decoded
static int rc_edge_proj(const Recon* d, int n, int P4[BLK_COEFS]) {
  const U8* plane = rc_plane(d->cur, n);
  const U8* q;
  S64 dd[RC_EDGE];
  int stride, bx, by, e, r, top, left, field = n < MB_LUMA && d->interlaced_dct;
  rc_blk_xy(d, n, field, &bx, &by);
  if (n < MB_LUMA) {
    stride = d->ly << field;  // (from row to row of the block; by is a row of the picture)
    top = !field && by > 0;
  } else {
    stride = d->lc;
    top = by > 0;
  }
  left = bx > 0;
  if (!top && !left) return 0;
  q = plane + (ptrdiff_t)by * (n < MB_LUMA ? d->ly : d->lc) + bx;
  for (e = 0; e < BLK_W; e++) {
    dd[e] = top ? q[e - stride] - q[e] : 0;
    dd[BLK_W + e] = left ? q[e * stride - 1] - q[e * stride] : 0;
  }
  for (r = 0; r < BLK_COEFS; r++) {
    const short *bv = rc_basis[r >> BLK_SHIFT], *bh = rc_basis[r & (BLK_W - 1)];
    S64 num = 0, den = 0, c;
    for (e = 0; e < BLK_W; e++) {
      const S64 bt = (S64)bv[0] * bh[e], bl = (S64)bv[e] * bh[0];
      if (top) num += dd[e] * bt, den += bt * bt;
      if (left) num += dd[BLK_W + e] * bl, den += bl * bl;
    }
    c = den ? num * RC_S24 / den : 0;  // the coefficient, as the transform takes it
    P4[r] = (int)(RC_Q * c / (2 * d->qscale));  // (in quarter levels: H.263's level x 2 qscale)
  }
  return 1;
}

//--- what the reference says of skipping ----------------------------------------------------------

// The current macroblock as a P-VOP skips it (the reference's, unmoved) or a B-VOP's direct mode
// without a vector predicts it: zt how far the decoded samples above and left of it are from the
// reference's there (P: the area changed since; 0 none decoded), ze how far the prediction's edges
// are from them (-1 none); both rc_qratio(). A B-VOP's direct prediction is made in the
// picture, where the macroblock's own replaces it.
static void rc_skip_feat(Recon* d, int* zt, int* ze) {
  constexpr int T = RC_TM_T;
  const int bx = d->mb_x * MB_W, by = d->mb_y * MB_W, ly = d->ly;
  const U8 *cur = d->cur->y + (ptrdiff_t)by * ly + bx, *ref;
  int n = 0, s = 0, ne = 0, e = 0, x, y;
  *zt = *ze = -1;
  if (!d->last) return;
  if (d->pict_type == PT_B) {
    U8* dy = d->cur->y + (ptrdiff_t)by * ly + bx;
    U8* du = d->cur->u + (ptrdiff_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
    U8* dv = d->cur->v + (ptrdiff_t)d->mb_y * BLK_W * d->lc + d->mb_x * BLK_W;
    rc_set_direct_mv(d, 0, 0);
    rc_mpv_motion(d, dy, du, dv, 0, d->last, OP_PUT);
    rc_mpv_motion(d, dy, du, dv, 1, d->next, OP_AVG);
    ref = cur;  // (the prediction in place)
  } else
    ref = d->last->y + (ptrdiff_t)by * ly + bx;
  if (by > 0) {
    for (x = 0; x < MB_W; x++, ne++) e += abs(ref[x] - cur[x - ly]);
    if (d->pict_type != PT_B)
      for (y = by >= T ? -T : -1; y < 0; y++)
        for (x = 0; x < MB_W; x++, n++) s += abs(cur[y * ly + x] - d->last->y[(ptrdiff_t)(by + y) * ly + bx + x]);
  }
  if (bx > 0) {
    for (y = 0; y < MB_W; y++, ne++) e += abs(ref[y * ly] - cur[y * ly - 1]);
    if (d->pict_type != PT_B)
      for (y = 0; y < MB_W; y++)
        for (x = bx >= T ? -T : -1; x < 0; x++, n++) s += abs(cur[y * ly + x] - d->last->y[(ptrdiff_t)(by + y) * ly + bx + x]);
  }
  *zt = n ? rc_qratio(d, s * RC_Q / n) : -1;
  *ze = ne ? rc_qratio(d, e * RC_Q / ne) : -1;
}

//--- a vector by template matching ----------------------------------------------------------------

// The vector (in the stream's units: half or quarter samples) whose template matches best in ref:
// among none, (ax, ay) and (bx, by) (in the stream's units; the vector's prediction and the
// co-located one), in full samples, then RC_TM_RANGE around the best, then its half samples. 0:
// no template.
constexpr int RC_TM_CANDS = 3, RC_TM_RANGE = 2;
static int rc_tm_search(const Recon* d, const RPic* ref, int ax, int ay, int bx_, int by_, int* mx, int* my) {
  constexpr int R = 2 * RC_TM_RANGE;  // (in half samples)
  const int sh = d->qpel;  // (to half samples)
  int c[RC_TM_CANDS][2], i, n, best = 0, bcx, bcy, cx, cy, dx, dy, s;
  c[0][0] = c[0][1] = 0;
  c[1][0] = (ax >> sh) & ~1, c[1][1] = (ay >> sh) & ~1;
  c[2][0] = (bx_ >> sh) & ~1, c[2][1] = (by_ >> sh) & ~1;
  bcx = bcy = 0;
  for (i = 0; i < RC_TM_CANDS; i++) {
    s = rc_tm_sad(d, ref->y, c[i][0], c[i][1], &n);
    if (!n) return 0;
    if (!i || s < best) best = s, bcx = c[i][0], bcy = c[i][1];
  }
  cx = bcx, cy = bcy;
  for (dy = -R; dy <= R; dy += 2)
    for (dx = -R; dx <= R; dx += 2) {
      if (!dx && !dy) continue;
      s = rc_tm_sad(d, ref->y, cx + dx, cy + dy, &n);
      if (s < best) best = s, bcx = cx + dx, bcy = cy + dy;
    }
  cx = bcx, cy = bcy;
  for (dy = -1; dy <= 1; dy++)
    for (dx = -1; dx <= 1; dx++) {
      if (!dx && !dy) continue;
      s = rc_tm_sad(d, ref->y, cx + dx, cy + dy, &n);
      if (s < best) best = s, bcx = cx + dx, bcy = cy + dy;
    }
  *mx = bcx * (1 << sh);
  *my = bcy * (1 << sh);
  return 1;
}

// the transform of block n's prediction (in the picture, before its residual: as the encoder saw
// it), and which way its texture runs (*td: RC_TD_COLS its samples change more down the columns (at
// least RC_TD_RATIO times), the residual's energy then in the first column; RC_TD_ROWS along the
// rows; RC_TD_NEITHER)
constexpr int RC_TD_RATIO = 2;
static void rc_pred_spec(const Recon* d, int n, int field, int F[BLK_COEFS], int* td) {
  const RPic* p = d->cur;
  const U8* q;
  int x, y, bx, by, sh = 0, sv = 0;
  rc_blk_xy(d, n, field, &bx, &by);
  if (n < MB_LUMA) {
    rc_fdct(F, p->y, d->ly, d->mb_w * MB_W, d->mb_h * MB_W, bx, by, field ? 2 : 1);
    q = p->y + (ptrdiff_t)by * d->ly + bx;
    field = d->ly << field;
  } else {
    rc_fdct(F, rc_plane(p, n), d->lc, d->mb_w * BLK_W, d->mb_h * BLK_W, bx, by, 1);
    q = rc_plane(p, n) + (ptrdiff_t)by * d->lc + bx;
    field = d->lc;
  }
  for (y = 0; y < BLK_W; y++)
    for (x = 0; x < BLK_W; x++) {
      if (x < BLK_W - 1) sh += abs(q[y * field + x + 1] - q[y * field + x]);
      if (y < BLK_W - 1) sv += abs(q[(y + 1) * field + x] - q[y * field + x]);
    }
  *td = RC_TD_RATIO * sh < sv ? RC_TD_COLS : RC_TD_RATIO * sv < sh ? RC_TD_ROWS : RC_TD_NEITHER;
}

