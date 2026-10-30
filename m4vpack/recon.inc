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

// a picture: its planes (rows of mb_w * MB_W luma and mb_w * BLK_W chroma samples; none: 0)
struct RPic {
  U8 *y, *u, *v;
  const U8* plane(int n) const { return n < MB_LUMA ? y : n == BLK_CB ? u : v; }  // block n's
};

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

// the dequantisation, FFmpeg's dct_unquantize_mpeg2: intra (level x 2 qscale x weight) >>
// QM_INTRA_SHIFT, inter ((2 level + 1) x 2 qscale x weight) >> QM_INTER_SHIFT
constexpr int QM_INTRA_SHIFT = 4, QM_INTER_SHIFT = 5;

//--- motion compensation: the interpolation ---------------------------------------------------------

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

// a fixed length escape's level, as FFmpeg clips it: 12 bits
static inline int esc3_clip(int level) {
  return (unsigned)(level + ESC3_MAX) > 2 * ESC3_MAX - 1 ? (level < 0 ? -ESC3_MAX : ESC3_MAX - 1) : level;
}

// The decoder: the stream's, the VOP's and the macroblock's state as FFmpeg's MpegEncContext has it,
// the prediction's tables, the pictures; the reconstruction's steps, and the model's calls (below)
struct Recon {
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
  int is_intra, ac_pred, interlaced_dct;
  int block_index[MB_BLOCKS], block_wrap[MB_BLOCKS];
  // prediction state (FFmpeg's MpegEncContext's layout)
  S16 *dc_val_base, *dc_val;
  S16 (*ac_val_base)[ACV], (*ac_val)[ACV];
  U8* mbintra_table;
  S8* qscale_table;
  U8* mb_done;
  // pictures
  RPic pics[NPICS];
  RPic cur, last, next;     // pics[]' (none: y 0)
  U8* mem;                  // all of the above, in one block
  FILE* dump;               // M4VPACK_RECON: the pictures, in coding order

  void init() {
    const char* fn = getenv("M4VPACK_RECON");
    memset(this, 0, sizeof *this);
    if (fn && *fn) dump = fopen(fn, "wb");
  }
  void quit() {
    free(mem);
    mem = 0;
    if (dump) fclose(dump);
    dump = 0;
  }

  void set_qscale(int q) {
    q = clip(q, QS_MIN, QS_MAX);
    qscale = q;
    chroma_qscale = q;
    y_dc_scale = y_dc_scale_table[q];
    c_dc_scale = c_dc_scale_table[q];
  }

  // the tables for the motion state's picture size, cleared as freshly allocated ones (m4vdec's
  // dec_layout); 0 if there is no memory for them
  int layout() {
    const Motion& o = *mo;
    int i, pass, y_size, c_size, yc_size, mb_array;
    size_t ysz, csz, at = 0;
    U8* m = 0;
    if (mem && width == o.width && height == o.height) return 1;
    free(mem);
    mem = 0;
    cur = last = next = RPic();
    width = o.width;
    height = o.height;
    mb_w = o.mb_w;
    mb_h = o.mb_h;
    mb_num = o.mb_num;
    mb_stride = o.mb_stride;
    b8_stride = o.b8_stride;
    ly = mb_w * MB_W;
    lc = mb_w * BLK_W;
    y_size = b8_stride * (2 * mb_h + 1);
    c_size = mb_stride * (mb_h + 1);
    yc_size = y_size + 2 * c_size;
    mb_array = mb_stride * mb_h + MB_PAD;
    ysz = (size_t)ly * mb_h * MB_W;
    csz = (size_t)lc * mb_h * BLK_W;
    for (pass = 0; pass < 2; pass++) {  // the size, then the tables in it
      if (pass) {
        if (!(m = (U8*)malloc(at))) return 0;
        memset(m, 0, at);
        mem = m;
        at = 0;
      }
      // each table at the next multiple of ALIGN in the block
      constexpr size_t ALIGN = 16;
      auto take = [&](size_t k) -> void* {
        void* q = m + at;
        at = (at + k + ALIGN - 1) & ~(ALIGN - 1);
        return q;
      };
      ac_val_base = (S16(*)[ACV])take((size_t)yc_size * ACV * sizeof(S16));
      dc_val_base = (S16*)take((size_t)yc_size * sizeof(S16));
      mbintra_table = (U8*)take((size_t)mb_array);
      qscale_table = (S8*)take((size_t)mb_array);
      mb_done = (U8*)take((size_t)mb_array);
      for (i = 0; i < NPICS; i++) {
        RPic& p = pics[i];
        p.y = (U8*)take(ysz + 2 * csz);
        p.u = p.y + ysz;
        p.v = p.u + csz;
      }
    }
    for (i = 0; i < yc_size; i++) dc_val_base[i] = DC_RESET;
    dc_val = dc_val_base + b8_stride + 1;
    ac_val = ac_val_base + b8_stride + 1;
    for (i = 0; i < MB_LUMA; i++) block_wrap[i] = b8_stride;
    block_wrap[BLK_CB] = block_wrap[BLK_CR] = mb_stride;
    return 1;
  }

  // the blocks' entries in the tables of the 8x8 grid (luma: the motion state's) and of the chroma
  // planes' (FFmpeg's ff_init_block_index)
  void set_block_index() {
    const Motion& o = *mo;
    const int bs = b8_stride, ms = mb_stride;
    int i;
    for (i = 0; i < MB_LUMA; i++) block_index[i] = o.block_index[i];
    block_index[BLK_CB] = ms * (mb_y + 1) + bs * mb_h * 2 + mb_x;
    block_index[BLK_CR] = ms * (mb_y + mb_h + 2) + bs * mb_h * 2 + mb_x;
  }

  void workaround_bugs() {
    if (xvid_build >= 0 && divx_version >= 0) divx_version = divx_build = -1;
    // (as in m4vdec, a flag once set stays)
    if (divx_version >= DIVX5_VERSION && divx_build < DIVX_QPEL_FIXED) bugs |= BUG_QPEL_CHROMA;
    if (divx_version > DIVX502_VERSION && divx_build < DIVX_QPEL_FIXED) bugs |= BUG_QPEL_CHROMA2;
    if ((unsigned)xvid_build <= XVID_QPEL_CHROMA_LAST) bugs |= BUG_QPEL_CHROMA;
    if ((unsigned)xvid_build <= XVID_EDGE_LAST) bugs |= BUG_EDGE;
    if ((unsigned)xvid_build <= XVID_DC_CLIP_LAST) bugs |= BUG_DC_CLIP;
    if ((unsigned)lavc_build < LAVC_STD_QPEL_FIXED) bugs |= BUG_STD_QPEL;
    if ((unsigned)lavc_build < LAVC_DIRECT_FIXED) bugs |= BUG_DIRECT_BLOCKSIZE;
    if ((unsigned)lavc_build < LAVC_EDGE_FIXED) bugs |= BUG_EDGE;
    if ((unsigned)lavc_build <= LAVC_DC_CLIP_LAST) bugs |= BUG_DC_CLIP;
    if ((lavc_build & BYTE_MASK) >= LAVC_MICRO_FFMPEG) {
      if (lavc_build > LAVC_IEDGE_FIRST && lavc_build < LAVC_IEDGE_END &&
          (lavc_build < LAVC_IEDGE_GAP || lavc_build > LAVC_IEDGE_GAP_END))
        bugs |= BUG_IEDGE;
    }
    if (divx_version >= 0) bugs |= BUG_DIRECT_BLOCKSIZE;
    if ((unsigned)divx_version < (unsigned)DIVX5_VERSION) bugs |= BUG_EDGE;

    if (divx_version >= 0) bugs |= BUG_HPEL_CHROMA;
    idct_xvid = xvid_build >= 0;
  }

  //--- the transform: XviD's IDCT for XviD's streams, else FFmpeg's simple_idct ----------------------

  void idct_put(U8* dest, int stride, S16* blk) const {
    if (idct_xvid) {
      int i, j;
      xvid_idct(blk);
      for (i = 0; i < BLK_W; i++)
        for (j = 0; j < BLK_W; j++) dest[i * stride + j] = clip8(blk[i * BLK_W + j]);
    } else
      simple_idct_put_add(dest, stride, blk, 0);
  }

  void idct_add(U8* dest, int stride, S16* blk) const {
    if (idct_xvid) {
      int i, j;
      xvid_idct(blk);
      for (i = 0; i < BLK_W; i++)
        for (j = 0; j < BLK_W; j++) dest[i * stride + j] = clip8(dest[i * stride + j] + blk[i * BLK_W + j]);
    } else
      simple_idct_put_add(dest, stride, blk, 1);
  }

  //--- dequantisation ---------------------------------------------------------------------------

  void unquant_intra(S16* blk, int n, int qs) const {
    int i, level;
    blk[0] = (S16)(blk[0] * (n < MB_LUMA ? y_dc_scale : c_dc_scale));
    if (mpeg_quant) {  // MPEG-2 style, with mismatch control (FFmpeg's bitexact variant)
      int sum = -1, q2 = qs << 1;
      sum += blk[0];
      for (i = 1; i < BLK_COEFS; i++) {
        level = blk[i];
        if (level) {
          if (level < 0) level = -(int)((-level) * q2 * intra_matrix[i] >> QM_INTRA_SHIFT);
          else level = (int)(level * q2 * intra_matrix[i] >> QM_INTRA_SHIFT);
          blk[i] = (S16)level;
          sum += level;
        }
      }
      blk[BLK_LAST] ^= sum & 1;
    } else {
      int qmul = qs << 1, qadd = (qs - 1) | 1;
      for (i = 1; i < BLK_COEFS; i++) {
        level = blk[i];
        if (level) blk[i] = (S16)(level < 0 ? level * qmul - qadd : level * qmul + qadd);
      }
    }
  }

  void unquant_inter_mpeg(S16* blk, int qs) const {
    int i, level, sum = -1, q2 = qs << 1;
    for (i = 0; i < BLK_COEFS; i++) {
      level = blk[i];
      if (level) {
        if (level < 0) level = -(int)((((-level) << 1) + 1) * q2 * inter_matrix[i] >> QM_INTER_SHIFT);
        else level = (int)(((level << 1) + 1) * q2 * inter_matrix[i] >> QM_INTER_SHIFT);
        blk[i] = (S16)level;
        sum += level;
      }
    }
    blk[BLK_LAST] ^= sum & 1;
  }

  //--- intra prediction ------------------------------------------------------------------------

  int pred_dc(int n, int* dir_ptr) const {
    const Motion& o = *mo;  // (the video packet's start: the motion state's)
    const S16* dv = dc_val + block_index[n];
    const int wrap = block_wrap[n];
    int a = dv[-1], b = dv[-1 - wrap], c = dv[-wrap];
    if (o.first_slice_line && n != 3) {
      if (n != 2) b = c = DC_RESET;
      if (n != 1 && mb_x == o.resync_mb_x) b = a = DC_RESET;
    }
    if (mb_x == o.resync_mb_x && mb_y == o.resync_mb_y + 1) {
      if (n == 0 || n == BLK_CB || n == BLK_CR) b = DC_RESET;
    }
    if (abs(a - b) < abs(b - c)) {
      *dir_ptr = 1;
      return c;
    }
    *dir_ptr = 0;
    return a;
  }

  int level_dc(int n, int pred, int level) {
    int scale = n < MB_LUMA ? y_dc_scale : c_dc_scale, ret;
    pred = (pred + (scale >> 1)) / scale;
    level += pred;
    ret = level;
    level *= scale;
    if (level & ~DC_MAX) {
      if (level < 0) level = 0;
      else if (!(bugs & BUG_DC_CLIP)) level = DC_MAX;
    }
    dc_val[block_index[n]] = (S16)level;
    return ret;
  }

  void pred_ac(S16* blk, int n, int dir) {
    int i;
    S16 *av = ac_val[block_index[n]], *av1 = av;
    const S8* qt = qscale_table;
    if (ac_pred) {
      if (dir == 0) {
        const int xy = mb_x - 1 + mb_y * mb_stride;
        av -= ACV;
        if (mb_x == 0 || qscale == qt[xy] || n == 1 || n == 3) {
          for (i = 1; i < BLK_W; i++) blk[i << BLK_SHIFT] += av[i];
        } else {
          for (i = 1; i < BLK_W; i++) blk[i << BLK_SHIFT] += rounded_div(av[i] * qt[xy], qscale);
        }
      } else {
        const int xy = mb_x + mb_y * mb_stride - mb_stride;
        av -= ACV * block_wrap[n];
        if (mb_y == 0 || qscale == qt[xy] || n == 2 || n == 3) {
          for (i = 1; i < BLK_W; i++) blk[i] += av[i + BLK_W];
        } else {
          for (i = 1; i < BLK_W; i++) blk[i] += rounded_div(av[i + BLK_W] * qt[xy], qscale);
        }
      }
    }
    for (i = 1; i < BLK_W; i++) av1[i] = blk[i << BLK_SHIFT];
    for (i = 1; i < BLK_W; i++) av1[BLK_W + i] = blk[i];
  }

  void clean_intra_table_entries(int xy) {
    if (mbintra_table[xy]) {
      const int wrap = b8_stride, bi = block_index[0];
      mbintra_table[xy] = 0;
      dc_val[bi] = dc_val[bi + 1] = dc_val[bi + wrap] = dc_val[bi + wrap + 1] = DC_RESET;
      dc_val[block_index[BLK_CB]] = dc_val[block_index[BLK_CR]] = DC_RESET;
      memset(ac_val[bi + 1], 0, sizeof(*ac_val));
      memset(ac_val[bi + wrap], 0, 2 * sizeof(*ac_val));
      memset(ac_val[block_index[BLK_CB]], 0, sizeof(*ac_val));
      memset(ac_val[block_index[BLK_CR]], 0, sizeof(*ac_val));
    }
  }

  void clean_buffers() {  // ff_mpeg4_clean_buffers, at the start of each video packet
    const int l_wrap = b8_stride, c_wrap = mb_stride;
    const int l_xy = (2 * mb_y - 1) * l_wrap + mb_x * 2 - 1;
    const int u_xy = 2 * mb_h * l_wrap + mb_y * c_wrap + mb_x - 1;
    const int v_xy = u_xy + c_wrap * (mb_h + 1);
    memset(ac_val + l_xy, 0, (l_wrap * 2 + 1) * sizeof(*ac_val));
    memset(ac_val + u_xy, 0, (c_wrap + 1) * sizeof(*ac_val));
    memset(ac_val + v_xy, 0, (c_wrap + 1) * sizeof(*ac_val));
  }

  //--- motion compensation ---------------------------------------------------------------------

  void gmc1_motion(U8* dest_y, U8* dest_cb, U8* dest_cr, const RPic& ref) const {
    constexpr int F = GMC1_ONE - 1, HALF = GMC1_ONE / 2, RND = 1 << (GMC1_SHIFT - 1);
    const Motion& o = *mo;
    U8 buf[MC_MB * MC_MB];
    const U8* src;
    int src_x, src_y, motion_x, motion_y, ss;
    motion_x = o.sprite_offset[0][0];
    motion_y = o.sprite_offset[0][1];
    src_x = mb_x * MB_W + (motion_x >> (o.warp_acc + 1));
    src_y = mb_y * MB_W + (motion_y >> (o.warp_acc + 1));
    motion_x *= 1 << (GMC1_SUB - o.warp_acc);
    motion_y *= 1 << (GMC1_SUB - o.warp_acc);
    src_x = clip(src_x, -MB_W, width);
    if (src_x == width) motion_x = 0;
    src_y = clip(src_y, -MB_W, height);
    if (src_y == height) motion_y = 0;
    src = rc_fetch(buf, MC_MB, ref.y, ly, src_x, src_y, 1, MC_MB, MC_MB, h_edge, v_edge, &ss);
    if ((motion_x | motion_y) & (HALF - 1)) {
      rc_gmc1_dsp(dest_y, ly, src, ss, MB_W, motion_x & F, motion_y & F, RND - no_rounding);
      rc_gmc1_dsp(dest_y + BLK_W, ly, src + BLK_W, ss, MB_W, motion_x & F, motion_y & F, RND - no_rounding);
    } else {  // (on half samples)
      int dxy = ((motion_x / HALF) & 1) | ((motion_y / HALF) & 1) << 1;
      rc_hpel(dest_y, ly, src, ss, MB_W, MB_W, dxy, no_rounding ? OP_PUT_NORND : OP_PUT);
    }
    motion_x = o.sprite_offset[1][0];
    motion_y = o.sprite_offset[1][1];
    src_x = mb_x * BLK_W + (motion_x >> (o.warp_acc + 1));
    src_y = mb_y * BLK_W + (motion_y >> (o.warp_acc + 1));
    motion_x *= 1 << (GMC1_SUB - o.warp_acc);
    motion_y *= 1 << (GMC1_SUB - o.warp_acc);
    src_x = clip(src_x, -BLK_W, width >> 1);
    if (src_x == width >> 1) motion_x = 0;
    src_y = clip(src_y, -BLK_W, height >> 1);
    if (src_y == height >> 1) motion_y = 0;
    src = rc_fetch(buf, MC_B, ref.u, lc, src_x, src_y, 1, MC_B, MC_B, h_edge >> 1, v_edge >> 1, &ss);
    rc_gmc1_dsp(dest_cb, lc, src, ss, BLK_W, motion_x & F, motion_y & F, RND - no_rounding);
    src = rc_fetch(buf, MC_B, ref.v, lc, src_x, src_y, 1, MC_B, MC_B, h_edge >> 1, v_edge >> 1, &ss);
    rc_gmc1_dsp(dest_cr, lc, src, ss, BLK_W, motion_x & F, motion_y & F, RND - no_rounding);
  }

  void gmc_motion(U8* dest_y, U8* dest_cb, U8* dest_cr, const RPic& ref) const {
    const Motion& o = *mo;
    const int a = o.warp_acc;
    const int dl = ly, dc = lc;
    int ox, oy;
    // (FFmpeg calls the gmc dsp with the destination's stride for the source too: the same)
    ox = o.sprite_offset[0][0] + o.sprite_delta[0][0] * mb_x * MB_W + o.sprite_delta[0][1] * mb_y * MB_W;
    oy = o.sprite_offset[0][1] + o.sprite_delta[1][0] * mb_x * MB_W + o.sprite_delta[1][1] * mb_y * MB_W;
    rc_gmc_dsp(dest_y, dl, ref.y, MB_W, ox, oy, o.sprite_delta[0][0], o.sprite_delta[0][1], o.sprite_delta[1][0],
               o.sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - no_rounding, h_edge, v_edge);
    rc_gmc_dsp(dest_y + BLK_W, dl, ref.y, MB_W, ox + o.sprite_delta[0][0] * BLK_W, oy + o.sprite_delta[1][0] * BLK_W,
               o.sprite_delta[0][0], o.sprite_delta[0][1], o.sprite_delta[1][0], o.sprite_delta[1][1], a + 1,
               (1 << (2 * a + 1)) - no_rounding, h_edge, v_edge);
    ox = o.sprite_offset[1][0] + o.sprite_delta[0][0] * mb_x * BLK_W + o.sprite_delta[0][1] * mb_y * BLK_W;
    oy = o.sprite_offset[1][1] + o.sprite_delta[1][0] * mb_x * BLK_W + o.sprite_delta[1][1] * mb_y * BLK_W;
    rc_gmc_dsp(dest_cb, dc, ref.u, BLK_W, ox, oy, o.sprite_delta[0][0], o.sprite_delta[0][1], o.sprite_delta[1][0],
               o.sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - no_rounding, (h_edge + 1) >> 1,
               (v_edge + 1) >> 1);
    rc_gmc_dsp(dest_cr, dc, ref.v, BLK_W, ox, oy, o.sprite_delta[0][0], o.sprite_delta[0][1], o.sprite_delta[1][0],
               o.sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - no_rounding, (h_edge + 1) >> 1,
               (v_edge + 1) >> 1);
  }

  // mpeg_motion_internal for H.263/MPEG-4 (a frame or one field)
  void mpeg_motion(U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field, int field_select,
                   const RPic& ref, int op, int motion_x, int motion_y, int h) const {
    U8 buf[(MC_MB + 1) * MC_MB];
    const U8* src;
    int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
    const int linesize = ly << field_based, uvlinesize = lc << field_based;
    dxy = ((motion_y & 1) << 1) | (motion_x & 1);
    src_x = mb_x * MB_W + (motion_x >> HPEL_BITS);
    src_y = (mb_y << (MB_SHIFT - field_based)) + (motion_y >> HPEL_BITS);
    if ((bugs & BUG_HPEL_CHROMA) && field_based) {
      mx = (motion_x >> 1) | (motion_x & 1);
      my = motion_y >> 1;
      uvdxy = ((my & 1) << 1) | (mx & 1);
      uvsrc_x = mb_x * BLK_W + (mx >> HPEL_BITS);
      uvsrc_y = (mb_y << (BLK_SHIFT - field_based)) + (my >> HPEL_BITS);
    } else {
      uvdxy = dxy | (motion_y & 2) | ((motion_x & 2) >> 1);
      uvsrc_x = src_x >> 1;
      uvsrc_y = src_y >> 1;
    }
    if (bottom_field) {
      dest_y += ly;
      dest_cb += lc;
      dest_cr += lc;
    }
    src = rc_fetch(buf, MC_MB, ref.y, ly, src_x, src_y * (1 << field_based) + field_select, 1 << field_based, MC_MB,
                   h + 1, h_edge, v_edge, &ss);
    rc_hpel(dest_y, linesize, src, ss, MB_W, h, dxy, op);
    if (field_based && (bugs & BUG_IEDGE) &&
        ((unsigned)src_x >= (unsigned)(h_edge - (motion_x & 1) - (MB_W - 1) > 0 ? h_edge - (motion_x & 1) - (MB_W - 1) : 0) ||
         (unsigned)src_y >=
             (unsigned)((v_edge >> 1) - (motion_y & 1) - h + 1 > 0 ? (v_edge >> 1) - (motion_y & 1) - h + 1 : 0)))
      src = rc_fetch_cb_iedge(buf, ref.u, ref.v, lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1, h_edge >> 1,
                              v_edge >> 1, &ss);
    else
      src = rc_fetch(buf, MC_B, ref.u, lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                     MC_B, (h >> 1) + 1, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cb, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
    src = rc_fetch(buf, MC_B, ref.v, lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                   MC_B, (h >> 1) + 1, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cr, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
  }

  void qpel_motion(U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field, int field_select,
                   const RPic& ref, int op, int motion_x, int motion_y, int h) const {
    U8 buf[MC_MB * MC_MB];
    const U8* src;
    int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
    const int linesize = ly << field_based, uvlinesize = lc << field_based;
    const int std_bug = (bugs & BUG_STD_QPEL) != 0;
    dxy = qp(motion_x & QPEL_MASK, motion_y & QPEL_MASK);
    src_x = mb_x * MB_W + (motion_x >> QPEL_BITS);
    src_y = mb_y * (MB_W >> field_based) + (motion_y >> QPEL_BITS);
    if (field_based) {
      mx = motion_x / 2;
      my = motion_y >> 1;
    } else if (bugs & BUG_QPEL_CHROMA2) {
      static const int rtab[] = { 0, 0, 1, 1, 0, 0, 0, 1 };
      mx = (motion_x >> 1) + rtab[motion_x & (countof(rtab) - 1)];
      my = (motion_y >> 1) + rtab[motion_y & (countof(rtab) - 1)];
    } else if (bugs & BUG_QPEL_CHROMA) {
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
    uvsrc_x = mb_x * BLK_W + mx;
    uvsrc_y = mb_y * (BLK_W >> field_based) + my;
    if (!field_based) {
      src = rc_fetch(buf, MC_MB, ref.y, ly, src_x, src_y, 1, MC_MB, MC_MB, h_edge, v_edge, &ss);
      rc_qpel_mc(dest_y, ly, src, ss, MB_W, dxy, op, std_bug);
    } else {
      if (bottom_field) {
        dest_y += ly;
        dest_cb += lc;
        dest_cr += lc;
      }
      src = rc_fetch(buf, MC_MB, ref.y, ly, src_x, src_y * 2 + field_select, 2, MC_MB, MC_B, h_edge, v_edge, &ss);
      rc_qpel_mc(dest_y, linesize, src, ss, BLK_W, dxy, op, std_bug);
      rc_qpel_mc(dest_y + BLK_W, linesize, src + BLK_W, ss, BLK_W, dxy, op, std_bug);
    }
    src = rc_fetch(buf, MC_B, ref.v, lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                   MC_B, (h >> 1) + 1, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cr, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
    if (field_based && (bugs & BUG_IEDGE) &&
        ((unsigned)src_x >= (unsigned)(h_edge - (motion_x & QPEL_MASK) - (MB_W - 1) > 0
                                           ? h_edge - (motion_x & QPEL_MASK) - (MB_W - 1) : 0) ||
         (unsigned)src_y >= (unsigned)((v_edge >> 1) - (motion_y & QPEL_MASK) - h + 1 > 0
                                           ? (v_edge >> 1) - (motion_y & QPEL_MASK) - h + 1 : 0)))
      src = rc_fetch_cb_iedge(buf, ref.u, ref.v, lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1, h_edge >> 1,
                              v_edge >> 1, &ss);
    else
      src = rc_fetch(buf, MC_B, ref.u, lc, uvsrc_x, uvsrc_y * (1 << field_based) + field_select, 1 << field_based,
                     MC_B, (h >> 1) + 1, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cb, uvlinesize, src, ss, BLK_W, h >> 1, uvdxy, op);
  }

  void chroma_4mv_motion(U8* dest_cb, U8* dest_cr, const RPic& ref, int op, int mx, int my) const {
    U8 buf[MC_B * MC_B];
    const U8* src;
    int src_x, src_y, dxy, ss;
    mx = round_chroma(mx);
    my = round_chroma(my);
    dxy = ((my & 1) << 1) | (mx & 1);
    mx >>= 1;
    my >>= 1;
    src_x = mb_x * BLK_W + mx;
    src_y = mb_y * BLK_W + my;
    src_x = clip(src_x, -BLK_W, width >> 1);
    if (src_x == (width >> 1)) dxy &= ~HP_H;
    src_y = clip(src_y, -BLK_W, height >> 1);
    if (src_y == (height >> 1)) dxy &= ~HP_V;
    src = rc_fetch(buf, MC_B, ref.u, lc, src_x, src_y, 1, MC_B, MC_B, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cb, lc, src, ss, BLK_W, BLK_W, dxy, op);
    src = rc_fetch(buf, MC_B, ref.v, lc, src_x, src_y, 1, MC_B, MC_B, h_edge >> 1, v_edge >> 1, &ss);
    rc_hpel(dest_cr, lc, src, ss, BLK_W, BLK_W, dxy, op);
  }

  void apply_8x8(U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const RPic& ref, int op) const {
    const Motion& o = *mo;
    U8 buf[MC_MB * MC_MB];
    const U8* src;
    int dxy, mx = 0, my = 0, src_x, src_y, i, ss;
    for (i = 0; i < MB_LUMA; i++) {  // (block i: i & 1 its column, i >> 1 its row)
      const int motion_x = o.mv[dir][i][0], motion_y = o.mv[dir][i][1];
      U8* dest = dest_y + ((i & 1) * BLK_W) + (i >> 1) * BLK_W * ly;
      if (qpel) {
        dxy = qp(motion_x & QPEL_MASK, motion_y & QPEL_MASK);
        src_x = mb_x * MB_W + (motion_x >> QPEL_BITS) + (i & 1) * BLK_W;
        src_y = mb_y * MB_W + (motion_y >> QPEL_BITS) + (i >> 1) * BLK_W;
        src_x = clip(src_x, -MB_W, width);
        if (src_x == width) dxy &= ~qp(QPEL_MASK, 0);
        src_y = clip(src_y, -MB_W, height);
        if (src_y == height) dxy &= ~qp(0, QPEL_MASK);
        src = rc_fetch(buf, MC_B, ref.y, ly, src_x, src_y, 1, MC_B, MC_B, h_edge, v_edge, &ss);
        rc_qpel_mc(dest, ly, src, ss, BLK_W, dxy, op, (bugs & BUG_STD_QPEL) != 0);
        mx += motion_x / 2;
        my += motion_y / 2;
      } else {
        dxy = 0;
        src_x = mb_x * MB_W + (i & 1) * BLK_W + (motion_x >> HPEL_BITS);
        src_y = mb_y * MB_W + (i >> 1) * BLK_W + (motion_y >> HPEL_BITS);
        src_x = clip(src_x, -MB_W, width);
        if (src_x != width) dxy |= motion_x & 1;
        src_y = clip(src_y, -MB_W, height);
        if (src_y != height) dxy |= (motion_y & 1) << 1;
        src = rc_fetch(buf, MC_B, ref.y, ly, src_x, src_y, 1, MC_B, MC_B, h_edge, v_edge, &ss);
        rc_hpel(dest, ly, src, ss, BLK_W, BLK_W, dxy, op);
        mx += motion_x;
        my += motion_y;
      }
    }
    chroma_4mv_motion(dest_cb, dest_cr, ref, op, mx, my);
  }

  void mpv_motion(U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const RPic& ref, int op) const {
    const Motion& o = *mo;
    int i;
    switch (o.mv_type) {
    case MV_16X16:
      if (o.mcsel) {
        if (o.real_warp_points == 1) gmc1_motion(dest_y, dest_cb, dest_cr, ref);
        else gmc_motion(dest_y, dest_cb, dest_cr, ref);
      } else if (qpel)
        qpel_motion(dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, o.mv[dir][0][0], o.mv[dir][0][1], MB_W);
      else
        mpeg_motion(dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, o.mv[dir][0][0], o.mv[dir][0][1], MB_W);
      break;
    case MV_8X8:
      apply_8x8(dest_y, dest_cb, dest_cr, dir, ref, op);
      break;
    case MV_FIELD:
      for (i = 0; i < 2; i++) {
        if (qpel)
          qpel_motion(dest_y, dest_cb, dest_cr, 1, i, o.field_select[dir][i], ref, op, o.mv[dir][i][0],
                         o.mv[dir][i][1], MB_W / 2);
        else
          mpeg_motion(dest_y, dest_cb, dest_cr, 1, i, o.field_select[dir][i], ref, op, o.mv[dir][i][0],
                         o.mv[dir][i][1], MB_W / 2);

      }
      break;
    }
  }

  //--- the model's calls ------------------------------------------------------------------------

  // the luma (plane 0) or chroma (1, 2) samples of the current picture at block n of the macroblock,
  // and their stride
  U8* block_at(int n, int* stride) const {
    if (n < MB_LUMA) {
      const int ls = ly << interlaced_dct, off = interlaced_dct ? ly : ly * BLK_W;
      *stride = ls;
      return cur.y + (size_t)mb_y * MB_W * ly + mb_x * MB_W + (n & 1) * BLK_W + (n >> 1) * off;
    }
    *stride = lc;
    return (n == BLK_CB ? cur.u : cur.v) + (size_t)mb_y * BLK_W * lc + mb_x * BLK_W;
  }

  // a free picture buffer for the next VOP (not one of the references)
  const RPic& free_pic() const {
    int i;
    for (i = 0; i < NPICS; i++)
      if (pics[i].y != last.y && pics[i].y != next.y) return pics[i];
    return pics[0];
  }

  // a gray stand-in for a reference the VOP has not, in a buffer it does not use
  const RPic& gray_pic() {
    const RPic& p = pics[pics[0].y == cur.y ? 1 : 0];
    const size_t ysz = (size_t)ly * mb_h * MB_W, csz = (size_t)lc * mb_h * BLK_W;
    memset(p.y, GRAY, ysz + 2 * csz);
    return p;
  }

#include "rc_vop_begin.inc"  // vop_begin(): a VOP's header

  // a video packet starting at macroblock mbnum (m4vdec's decode_video_packet_header and resync,
  // after the motion state's)
  void vp_begin(const VpHdr& k) {
    const Motion& o = *mo;
    if (!on) return;
    mb_x = k.mbnum % mb_w;
    mb_y = k.mbnum / mb_w;
    if (k.quant) chroma_qscale = qscale = k.quant;
    if (k.hec && pict_type == PT_S && o.sprite_usage == GMC_SPRITE && !o.sprite_ok) {
      on = 0;  // (FFmpeg would drop the packet)
      return;
    }
    clean_buffers();
    set_qscale(qscale);
  }

  // macroblock idx starts (decode_slice's part, after the motion state's)
  void mb_begin(int idx) {
    mb_x = idx % mb_w;
    mb_y = idx / mb_w;
    set_block_index();
    is_intra = 0;
    ac_pred = 0;
  }

  // An inter macroblock (or a skipped one): its prediction made in the picture, by the vectors the
  // motion state has for it (reconstruct_mb's part)
  void mb_inter() {
    const Motion& o = *mo;
    U8* dest_y = cur.y + (size_t)mb_y * MB_W * ly + mb_x * MB_W;
    U8* dest_cb = cur.u + (size_t)mb_y * BLK_W * lc + mb_x * BLK_W;
    U8* dest_cr = cur.v + (size_t)mb_y * BLK_W * lc + mb_x * BLK_W;
    int op = (!no_rounding || pict_type == PT_B) ? OP_PUT : OP_PUT_NORND;
    is_intra = 0;
    if (o.mv_dir & MV_DIR_FORWARD) {
      mpv_motion(dest_y, dest_cb, dest_cr, 0, last, op);
      op = OP_AVG;
    }
    if (o.mv_dir & MV_DIR_BACKWARD) mpv_motion(dest_y, dest_cb, dest_cr, 1, next, op);
  }

  // an intra macroblock (ac: its AC prediction flag)
  void mb_intra(int ac) {
    is_intra = 1;
    ac_pred = ac == 1;
  }

  // Block n of the macroblock, its coefficients b known: reconstructed (decode_block's dequantisation
  // and prediction, reconstruct_mb's transform)
  void block(int n, const Blk& b, int intra, int dcvlc) {
    S16 blk[BLK_COEFS];
    const U8* scan;
    int i, j, stride, dir;
    U8* dst = block_at(n, &stride);
    memset(blk, 0, sizeof blk);
    if (intra) {
      const int pred = pred_dc(n, &dir);
      i = -1;
      if (dcvlc) {
        blk[0] = (S16)level_dc(n, pred, b.dc);
        i = 0;
      }
      scan = ac_pred ? (dir == 0 ? intra_v_scan : intra_h_scan) : intra_scan;
      for (j = 0; j < b.n; j++) {
        int level = b.level[j];
        i += b.run[j] + 1;
        if (b.esc[j] == ESC_FIXED) level = esc3_clip(level);
        blk[scan[i & BLK_LAST]] = (S16)level;
      }
      if (!dcvlc) blk[0] = (S16)level_dc(n, pred, blk[0]);
      pred_ac(blk, n, dir);
      unquant_intra(blk, n, n < MB_LUMA ? qscale : chroma_qscale);
      idct_put(dst, stride, blk);
      return;
    }
    if (!b.n) return;
    {
      int qmul = 1, qadd = 0;
      if (!mpeg_quant) {
        qmul = qscale << 1;
        qadd = (qscale - 1) | 1;
      }
      scan = intra_scan;
      for (j = 0, i = -1; j < b.n; j++) {
        int level = b.level[j];
        i += b.run[j] + 1;
        level = level < 0 ? level * qmul - qadd : level * qmul + qadd;
        if (b.esc[j] == ESC_FIXED) level = esc3_clip(level);
        blk[scan[i & BLK_LAST]] = (S16)level;
      }
      if (mpeg_quant) unquant_inter_mpeg(blk, qscale);
      idct_add(dst, stride, blk);
    }
  }

  // the macroblock's end: what later macroblocks and VOPs read of it (decode_slice's part)
  void mb_end() {
    const int mb_xy = mb_y * mb_stride + mb_x;
    if (!is_intra) clean_intra_table_entries(mb_xy);
    else mbintra_table[mb_xy] = 1;
    qscale_table[mb_xy] = (S8)qscale;
    mb_done[mb_xy] = 1;
  }

  // the VOP's end: the macroblocks no packet had copied from the reference (m4vdec's concealment);
  // with M4VPACK_RECON, the picture written
  void vop_end() {
    int i, y;
    if (!on) return;
    for (i = 0; i < mb_num; i++) {
      const int mx = i % mb_w, my = i / mb_w;
      if (mb_done[my * mb_stride + mx]) continue;
      for (y = 0; y < MB_W; y++) {
        const size_t o = (size_t)(my * MB_W + y) * ly + mx * MB_W;
        if (last.y) memcpy(cur.y + o, last.y + o, MB_W);
        else memset(cur.y + o, GRAY, MB_W);
      }
      for (y = 0; y < BLK_W; y++) {
        const size_t o = (size_t)(my * BLK_W + y) * lc + mx * BLK_W;
        if (last.y) {
          memcpy(cur.u + o, last.u + o, BLK_W);
          memcpy(cur.v + o, last.v + o, BLK_W);
        } else {
          memset(cur.u + o, GRAY, BLK_W);
          memset(cur.v + o, GRAY, BLK_W);
        }
      }
    }
    if (dump) {  // the picture, cropped as m4vdec writes it
      const int cw = (width + 1) >> 1, ch = (height + 1) >> 1;
      for (y = 0; y < height; y++) fwrite(cur.y + (size_t)y * ly, 1, width, dump);
      for (y = 0; y < ch; y++) fwrite(cur.u + (size_t)y * lc, 1, cw, dump);
      for (y = 0; y < ch; y++) fwrite(cur.v + (size_t)y * lc, 1, cw, dump);
    }
    on = 0;
  }
};
