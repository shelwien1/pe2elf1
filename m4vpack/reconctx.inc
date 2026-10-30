// reconctx.inc -- what the model of level -9 reads from the pictures the decoder in the loop
// (recon.inc) makes as it codes: the signs a block's continuing the samples around it prefers,
// the intra macroblocks' estimate from the reference (the template's match), what an inter block's
// prediction says (its texture, its edges, its spectrum), what the reference says of skipping, and
// the vector the template matches best. m4vpack's own, on the decoder's functions: ReconCtx, the
// decoder with these reads as its methods (after what they share: constants, tables, the transform
// of a block of samples), RcSign a block's sign search, RcEst the intra estimate.

constexpr int RC_Q = 4;  // the contexts' fixed point: quarters (of a sample, of a level)
// ReconCtx::pred_spec()'s td: which way a prediction's texture runs (its samples change more down the
// columns, neither, along the rows), or none
enum { RC_TD_COLS, RC_TD_NEITHER, RC_TD_ROWS, RC_TD_NONE };

constexpr int RC_PAIRS = 2 * (BLK_W - 1) * BLK_W;  // the pairs of neighbouring samples in an 8x8 block

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
constexpr int RC_MARGIN_MAX = 1 << 20;       // RcSign::predict()'s margin, capped
static const short rc_basis[BLK_W][BLK_W] = {  // C(f)/2 cos((2s+1) f pi/16) x 4096: [frequency][sample]
  { 1448, 1448, 1448, 1448, 1448, 1448, 1448, 1448 },
  { 2009, 1703, 1138, 400, -400, -1138, -1703, -2009 },
  { 1892, 784, -784, -1892, -1892, -784, 784, 1892 },
  { 1703, -400, -2009, -1138, 1138, 2009, 400, -1703 },
  { 1448, -1448, -1448, 1448, 1448, -1448, -1448, 1448 },
  { 1138, -2009, 400, 1703, -1703, -400, 2009, -1138 },
  { 784, -1892, 1892, -784, -784, 1892, -1892, 784 },
  { 400, -1138, 1703, -2009, 2009, -1703, 1138, -400 } };

// The sign search of a block (ReconCtx::sign_init() sets it up): the targets and the edges, then
// each combination's cost, and the signs decided as the model codes them
struct RcSign {
  int nc;                       // the signs searched (0: no prediction)
  S64 t[RC_SPT][RC_EDGE];       // the targets at the edge samples (0-7 the top row, 8-15 the left
  U8 av[RC_EDGE];               // column; av: decoded there), times 2^24
  S64 x[RC_EDGE];               // the edge samples with every searched sign +
  S64 dl[RC_SPK][RC_EDGE];      // a searched coefficient's change to them, from + to -
  S64 cost[RC_SPT][1 << RC_SPK];  // by combination (bit j: sign j negative)
  int fixed_mask, fixed_val;

  // each combination's cost, of the first nsigns signs searched (the edges as ReconCtx::sign_init()
  // leaves them)
  void search(int nsigns) {
    // (the edge samples not decoded zeroed, which then add nothing: loops of 16 without a branch)
    S64 xs[RC_EDGE], tt[RC_SPT][RC_EDGE], dd[RC_SPK][RC_EDGE];
    int e, w, j, i, mask;
    for (e = 0; e < RC_EDGE; e++) {
      const S64 m = av[e] ? -1 : 0;
      xs[e] = x[e] & m;
      for (w = 0; w < RC_SPT; w++) tt[w][e] = t[w][e] & m;
      for (j = 0; j < nsigns; j++) dd[j][e] = dl[j][e] & m;
    }
    for (i = 0, mask = 0; i < 1 << nsigns; i++) {
      if (i) {  // Gray code: one sign flips
        const S64* q = dd[j = __builtin_ctz((unsigned)i)];
        if (mask >> j & 1)
          for (e = 0; e < RC_EDGE; e++) xs[e] -= q[e];
        else
          for (e = 0; e < RC_EDGE; e++) xs[e] += q[e];
        mask ^= 1 << j;
      }
      for (w = 0; w < RC_SPT; w++) {
        S64 c = 0;
        for (e = 0; e < RC_EDGE; e++) {
          const S64 v = xs[e] - tt[w][e];
          c += v < 0 ? -v : v;
        }
        cost[w][mask] = c;
      }
    }
    nc = nsigns;
    fixed_mask = fixed_val = 0;
  }

  // the predicted sign of searched coefficient j (1 negative) by target w, given the ones decided,
  // and by how much the other one costs more (quarter samples)
  int predict(int w, int j, int* margin) const {
    const S64* cw = cost[w];
    S64 c0 = RC_COST_MAX, c1 = RC_COST_MAX, dd;
    int m, f;
    if (fixed_mask == (1 << j) - 1)  // (the signs before j decided, as the model codes them: the rest free)
      for (f = 0; f < 1 << (nc - j); f++) {
        m = fixed_val | f << j;
        if (f & 1) {
          if (cw[m] < c1) c1 = cw[m];
        } else if (cw[m] < c0)
          c0 = cw[m];
      }
    else
      for (m = 0; m < 1 << nc; m++) {
        if ((m & fixed_mask) != fixed_val) continue;
        if (m >> j & 1) {
          if (cw[m] < c1) c1 = cw[m];
        } else if (cw[m] < c0)
          c0 = cw[m];
      }
    dd = ((c0 > c1 ? c0 - c1 : c1 - c0) * RC_Q) >> RC_SBITS;
    *margin = dd > RC_MARGIN_MAX ? RC_MARGIN_MAX : (int)dd;
    return c1 < c0;
  }

  void decide(int j, int negative) {
    fixed_mask |= 1 << j;
    if (negative) fixed_val |= 1 << j;
  }
};

// the contribution of the value v at raster position r to the edge samples, added to x
static void rc_edge_add(S64* x, int r, S64 v) {
  const short *bv = rc_basis[r >> BLK_SHIFT], *bh = rc_basis[r & (BLK_W - 1)];
  int e;
  if (!v) return;
  for (e = 0; e < BLK_W; e++) x[e] += v * bv[0] * bh[e];
  for (e = 0; e < BLK_W; e++) x[BLK_W + e] += v * bv[e] * bh[0];
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
// half-sample offsets (fx, fy), as ReconCtx::tm_sad() takes them (SSE2's averages round the same: of four,
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

//--- the intra estimate -------------------------------------------------------------------------
// h264p's largest context, an intra macroblock's coefficients estimated from the reference: its
// blocks as the reference has them where its neighbours' motion puts it (refined by matching the
// decoded samples above and left of it, the template, against the reference), transformed as the
// encoder transformed the macroblock. What it would have coded had the picture only moved.

// the intra estimate (ReconCtx::intra_est() makes it)
struct RcEst {
  int ok;        // made (an intra macroblock, a reference)
  int dx, dy;    // where, in full samples
  int miss;      // how far the template is from the reference there: its mean difference x 4 (-1 none)
  int F[MB_BLOCKS][BLK_COEFS];  // the reference's blocks transformed (the decoder's scale: a flat 128 is 1024)
};

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

// what an inter block's prediction says: a mean difference against the quantiser, in RC_QR-ths of
// qscale (ReconCtx::qratio()); the vector by template matching: its candidates, the range of its
// search (ReconCtx::tm_search()); the prediction's spectrum: how much more its samples change one way
// than the other for its texture to run that way (ReconCtx::pred_spec())
constexpr int RC_QR = 8;
constexpr int RC_TM_CANDS = 3, RC_TM_RANGE = 2;
constexpr int RC_TD_RATIO = 2;

// The decoder in the loop as the model's contexts read it: the reads of each kind above. They
// change nothing in it, but skip_feat(), which makes a B-VOP macroblock's direct prediction in the
// picture (the macroblock's own replaces it).
struct ReconCtx : Recon {
  //--- sign prediction: the levels' values, the scans, the blocks' places; the search set up

  // the value the transform takes of level a (the actual level: for an intra block, the prediction
  // included) at raster position r of block n
  int deq(int n, int intra, int r, int a) const {
    int v, aa = a < 0 ? -a : a;
    if (!a) return 0;
    if (intra && !r) return a * (n < MB_LUMA ? y_dc_scale : c_dc_scale);
    if (mpeg_quant)
      v = intra ? (aa * 2 * qscale * intra_matrix[r]) >> QM_INTRA_SHIFT
                : ((2 * aa + 1) * 2 * qscale * inter_matrix[r]) >> QM_INTER_SHIFT;
    else
      v = aa * 2 * qscale + ((qscale - 1) | 1);
    return a < 0 ? -v : v;
  }

  // What an intra block n adds to its coded levels (as block() will; changes nothing): its DC
  // prediction (in levels) at 0, its AC prediction at the first column (*dir 0, the vertical scan)
  // or row (1, the horizontal one)
  void intra_pred(int n, int* dir, int P[BLK_COEFS]) const {
    const int pred = pred_dc(n, dir), scale = n < MB_LUMA ? y_dc_scale : c_dc_scale;
    const S16* av = ac_val[block_index[n]];
    const S8* qt = qscale_table;
    int i;
    memset(P, 0, BLK_COEFS * sizeof(int));
    P[0] = (pred + (scale >> 1)) / scale;
    if (!ac_pred) return;
    if (*dir == 0) {
      const int xy = mb_x - 1 + mb_y * mb_stride;
      av -= ACV;
      for (i = 1; i < BLK_W; i++)
        P[i << BLK_SHIFT] = mb_x == 0 || qscale == qt[xy] || n == 1 || n == 3
                                ? av[i] : rounded_div(av[i] * qt[xy], qscale);
    } else {
      const int xy = mb_x + mb_y * mb_stride - mb_stride;
      av -= ACV * block_wrap[n];
      for (i = 1; i < BLK_W; i++)
        P[i] = mb_y == 0 || qscale == qt[xy] || n == 2 || n == 3
                   ? av[i + BLK_W] : rounded_div(av[i + BLK_W] * qt[xy], qscale);
    }
  }

  // the scan of block n (an intra block with AC prediction: by its DC prediction's direction)
  const U8* scan(int n, int intra) const {
    int dir;
    if (!intra || !ac_pred) return intra_scan;
    pred_dc(n, &dir);
    return dir == 0 ? intra_v_scan : intra_h_scan;
  }

  // whether block n's signs can be predicted: an edge of it decoded, not the luma of a field DCT
  // macroblock
  int sign_can(int n) const {
    if (n < MB_LUMA) return !interlaced_dct && (mb_x || mb_y || n);
    return mb_x || mb_y;
  }

  // block n's top left sample, in its plane (field: the luma of a field DCT macroblock, whose blocks'
  // rows are a field's: by its first row's)
  void blk_xy(int n, int field, int* bx, int* by) const {
    if (n < MB_LUMA) {
      *bx = mb_x * MB_W + (n & 1) * BLK_W;
      *by = mb_y * MB_W + (n >> 1) * (field ? 1 : BLK_W);
    } else {
      *bx = mb_x * BLK_W;
      *by = mb_y * BLK_W;
    }
  }

  // The sign search sp of block n (intra, dcvlc and dc: an intra block's DC difference coded apart)
  // whose k coefficients are at the scan positions pos[] with the levels lev[], of which the first nc
  // signs are not known (lev[] their magnitudes there): its edges, then each combination's cost. 0:
  // no prediction.
  int sign_init(RcSign& sp, int n, int intra, int dcvlc, int dc, int k, const int* pos, const int* lev,
                int nc) const {
    const U8 *plane, *sc, *q;
    int P[BLK_COEFS], C[BLK_COEFS], cand[BLK_COEFS], stride, bx, by, e, j, r, dir = 0;
    sp.nc = 0;
    if (nc <= 0 || !sign_can(n)) return 0;
    plane = cur.plane(n);
    stride = n < MB_LUMA ? ly : lc;
    blk_xy(n, 0, &bx, &by);
    q = plane + (ptrdiff_t)by * stride + bx;  // the block
    for (e = 0; e < BLK_W; e++) {
      sp.av[e] = by > 0;
      sp.av[BLK_W + e] = bx > 0;
      if (by > 0) {
        sp.t[RC_T_EXTRAP][e] = (S64)(2 * q[e - stride] - q[e - 2 * stride]) * RC_S24;
        sp.t[RC_T_NEXT][e] = (S64)q[e - stride] * RC_S24;
      }
      if (bx > 0) {
        sp.t[RC_T_EXTRAP][BLK_W + e] = (S64)(2 * q[e * stride - 1] - q[e * stride - 2]) * RC_S24;
        sp.t[RC_T_NEXT][BLK_W + e] = (S64)q[e * stride - 1] * RC_S24;
      }
      sp.x[e] = intra ? 0 : (S64)q[e] * RC_S24;
      sp.x[BLK_W + e] = intra ? 0 : (S64)q[e * stride] * RC_S24;
    }
    if (nc > RC_SPK) nc = RC_SPK;
    if (nc > k) nc = k;
    sc = intra_scan;
    if (intra) {
      intra_pred(n, &dir, P);
      if (ac_pred) sc = dir == 0 ? intra_v_scan : intra_h_scan;
    } else
      memset(P, 0, sizeof P);
    memset(C, 0, sizeof C);
    memset(cand, 0, sizeof cand);
    if (intra && dcvlc) C[0] = dc;
    for (j = 0; j < k; j++) {
      r = sc[pos[j] & BLK_LAST];
      if (j < nc) cand[r] = 1 + j;
      else C[r] = lev[j];
    }
    for (r = 0; r < BLK_COEFS; r++)  // what the searched signs do not change
      if (!cand[r] && (P[r] || C[r])) rc_edge_add(sp.x, r, deq(n, intra, r, P[r] + C[r]));
    for (j = 0; j < nc; j++) {
      S64 t[RC_EDGE];
      int a, vp;
      r = sc[pos[j] & BLK_LAST];
      a = lev[j] < 0 ? -lev[j] : lev[j];
      vp = deq(n, intra, r, P[r] + a);
      rc_edge_add(sp.x, r, vp);
      memset(t, 0, sizeof t);
      rc_edge_add(t, r, deq(n, intra, r, P[r] - a) - vp);
      memcpy(sp.dl[j], t, sizeof t);
    }
    sp.search(nc);
    return nc;
  }

  //--- the template

  // the template's (the decoded samples above and left of the macroblock) summed difference from
  // ref at a displacement of (hx, hy) half samples; *n its samples (0: none decoded)
  int tm_sad(const U8* ref, int hx, int hy, int* n) const {
    constexpr int T = RC_TM_T;
    const U8* cy = cur.y;
    const int w = mb_w * MB_W, h = mb_h * MB_W, bx = mb_x * MB_W, by = mb_y * MB_W;
    const int ix = hx >> 1, iy = hy >> 1, fx = hx & 1, fy = hy & 1;
    int x, y, k = 0, s = 0;
    if (by >= T && bx >= T && bx - T + ix >= 0 && by - T + iy >= 0 && bx + MB_W + 1 + ix < w && by + MB_W + 1 + iy < h) {
      // (inside the picture: no clamping)
      const U8* r = ref + (ptrdiff_t)iy * ly + ix;
      for (y = by - T; y < by; y++) {
        const U8 *a = r + (ptrdiff_t)y * ly + bx;
        s += rc_sad16(cy + (ptrdiff_t)y * ly + bx, a, a + fy * ly, fx, fy);
      }
      for (y = by; y < by + MB_W; y++) {
        const U8 *c = cy + (ptrdiff_t)y * ly, *a = r + (ptrdiff_t)y * ly, *b = a + fy * ly;
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
        for (x = bx; x < bx + MB_W; x++, k++) s += abs(cy[(ptrdiff_t)y * ly + x] - tmv(x + ix, y + iy));
    if (bx >= T)
      for (y = by; y < by + MB_W; y++)
        for (x = bx - T; x < bx; x++, k++) s += abs(cy[(ptrdiff_t)y * ly + x] - tmv(x + ix, y + iy));
    *n = k;
    return s;
  }

  //--- the intra estimate

  // the luma blocks of the estimate (field: those of a field DCT macroblock)
  void est_luma(RcEst& e, int field) const {
    const RPic& r = last;
    const int w = mb_w * MB_W, h = mb_h * MB_W;
    int n, bx, by;
    for (n = 0; n < MB_LUMA; n++) {
      blk_xy(n, field, &bx, &by);
      rc_fdct(e.F[n], r.y, ly, w, h, bx + e.dx, by + e.dy, field ? 2 : 1);
    }
  }

  // The estimate for the current (intra) macroblock: the displacement among the vectors of the left,
  // above and above right macroblocks, the co-located one's in the reference and none, the one whose
  // template matches best, refined by a search of RC_EST_RANGE samples around it
  void intra_est(RcEst& e) const {
    const RPic& ref = last;
    const Motion& o = *mo;
    const S16 (*mv)[2] = o.cur.mv, (*rmv)[2] = o.ref.mv;  // (the motion state's: this VOP's, the reference's)
    const int qs = 1 + qpel, wrap = b8_stride, bi = block_index[0];
    int cand[RC_EST_CANDS][2], nc = 0, i, bx, by, best = -1, bn = 0, bs = 0, dx, dy;
    e.ok = 0;
    if (!ref.y) return;
    cand[nc][0] = cand[nc][1] = 0, nc++;
    if (mb_x > 0) cand[nc][0] = mv[bi - 1][0] >> qs, cand[nc][1] = mv[bi - 1][1] >> qs, nc++;
    if (mb_y > 0) {
      cand[nc][0] = mv[bi - wrap][0] >> qs, cand[nc][1] = mv[bi - wrap][1] >> qs, nc++;
      if (mb_x + 1 < mb_w)
        cand[nc][0] = mv[bi + 2 - wrap][0] >> qs, cand[nc][1] = mv[bi + 2 - wrap][1] >> qs, nc++;
    }
    cand[nc][0] = rmv[bi][0] >> qs, cand[nc][1] = rmv[bi][1] >> qs, nc++;
    bx = by = 0;
    for (i = 0; i < nc; i++) {
      int n;
      const int s = tm_sad(ref.y, 2 * cand[i][0], 2 * cand[i][1], &n);
      if (!n) break;  // (no template: the first candidate)
      if (best < 0 || s < bs) best = i, bn = n, bs = s, bx = cand[i][0], by = cand[i][1];
    }
    if (bn) {
      const int cx = bx, cy = by;
      for (dy = -RC_EST_RANGE; dy <= RC_EST_RANGE; dy++)
        for (dx = -RC_EST_RANGE; dx <= RC_EST_RANGE; dx++) {
          int n;
          const int s = dx || dy ? tm_sad(ref.y, 2 * (cx + dx), 2 * (cy + dy), &n) : bs;
          if (s < bs) bs = s, bx = cx + dx, by = cy + dy;
        }
    }
    e.dx = bx;
    e.dy = by;
    e.miss = bn ? RC_Q * bs / bn : -1;
    est_luma(e, 0);
    {
      const int w = mb_w * BLK_W, h = mb_h * BLK_W, cx = mb_x * BLK_W + (bx >> 1), cy = mb_y * BLK_W + (by >> 1);
      rc_fdct(e.F[BLK_CB], ref.u, lc, w, h, cx, cy, 1);
      rc_fdct(e.F[BLK_CR], ref.v, lc, w, h, cx, cy, 1);
    }
    e.ok = 1;
  }

  // the estimate's level at raster position r of block n, in quarter levels (signed): the actual
  // level, before the intra prediction
  int est_q4(const RcEst& e, int n, int r) const {
    const int f = e.F[n][r], a = f < 0 ? -f : f;
    int v;
    if (!r) {
      const int sc = n < MB_LUMA ? y_dc_scale : c_dc_scale;
      return (RC_Q * f + (f < 0 ? -sc / 2 : sc / 2)) / sc;
    }
    // (the inverse of the quantisation the decoder undoes: MPEG's level x qscale x weight / 8,
    // H.263's level x 2 qscale)
    if (mpeg_quant) v = RC_Q * (1 << (QM_INTRA_SHIFT - 1)) * a / (qscale * intra_matrix[r]);
    else v = RC_Q * a / (2 * qscale);
    return f < 0 ? -v : v;
  }

  //--- what an inter block's prediction says: where the prediction is textured, motion compensation
  // misses more; where its edges do not continue the decoded samples around the block, the residual
  // has to make up the difference

  // a mean difference (in quarter samples, at least 0) against the quantiser: in RC_QR-ths of qscale
  int qratio(int v4) const { return v4 * RC_QR / (RC_Q * qscale); }

  // Block n's prediction (in the picture, before its residual): tx its texture (the mean difference
  // between neighbouring samples), em how far its top row and left column are from the decoded
  // samples next to them (-1 no edge decoded), both qratio(); full: every edge decoded so far, else
  // only those on other macroblocks (as the coded block pattern is coded, before any block of the macroblock);
  // field: the luma in fields (a field DCT macroblock's blocks)
  void pred_feat(int n, int full, int field, int* tx, int* em) const {
    const U8* plane = cur.plane(n);
    const U8* q;
    int stride, bx, by, x, y, s = 0, e = 0, ne = 0, top, left;
    blk_xy(n, field, &bx, &by);
    if (n < MB_LUMA) {
      stride = ly << field;  // (from row to row of the block; by is a row of the picture)
      top = field ? 0 : (n >> 1) ? full : mb_y > 0;
      left = (n & 1) ? full : mb_x > 0;
    } else {
      stride = lc;
      top = mb_y > 0;
      left = mb_x > 0;
    }
    q = plane + (ptrdiff_t)by * (n < MB_LUMA ? ly : lc) + bx;
    for (y = 0; y < BLK_W; y++)
      for (x = 0; x < BLK_W; x++) {
        if (x < BLK_W - 1) s += abs(q[y * stride + x + 1] - q[y * stride + x]);
        if (y < BLK_W - 1) s += abs(q[(y + 1) * stride + x] - q[y * stride + x]);
      }
    if (top)
      for (x = 0; x < BLK_W; x++, ne++) e += abs(q[x] - q[x - stride]);
    if (left)
      for (y = 0; y < BLK_W; y++, ne++) e += abs(q[y * stride] - q[y * stride - 1]);
    *tx = qratio(s * RC_Q / RC_PAIRS);
    *em = ne ? qratio(e * RC_Q / ne) : -1;
  }

  // An inter block's coefficients as its edges ask for them: how far the prediction's top row and left
  // column are from the decoded samples next to them, projected onto each coefficient's pattern there
  // (least squares, a coefficient at a time), in quarter levels (signed) by raster position; 0 if no
  // edge of the block is decoded
  int edge_proj(int n, int P4[BLK_COEFS]) const {
    const U8* plane = cur.plane(n);
    const U8* q;
    S64 dd[RC_EDGE];
    int stride, bx, by, e, r, top, left, field = n < MB_LUMA && interlaced_dct;
    blk_xy(n, field, &bx, &by);
    if (n < MB_LUMA) {
      stride = ly << field;  // (from row to row of the block; by is a row of the picture)
      top = !field && by > 0;
    } else {
      stride = lc;
      top = by > 0;
    }
    left = bx > 0;
    if (!top && !left) return 0;
    q = plane + (ptrdiff_t)by * (n < MB_LUMA ? ly : lc) + bx;
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
      P4[r] = (int)(RC_Q * c / (2 * qscale));  // (in quarter levels: H.263's level x 2 qscale)
    }
    return 1;
  }

  //--- what the reference says of skipping

  // The current macroblock as a P-VOP skips it (the reference's, unmoved) or a B-VOP's direct mode
  // without a vector predicts it (the motion state's vectors, which the model has set so): zt how
  // far the decoded samples above and left of it are from the reference's there (P: the area changed
  // since; 0 none decoded), ze how far the prediction's edges are from them (-1 none); both
  // qratio(). A B-VOP's direct prediction is made in the picture, where the macroblock's own
  // replaces it.
  void skip_feat(int* zt, int* ze) {
    constexpr int T = RC_TM_T;
    const int bx = mb_x * MB_W, by = mb_y * MB_W;
    const U8 *cp = cur.y + (ptrdiff_t)by * ly + bx, *ref;
    int n = 0, s = 0, ne = 0, e = 0, x, y;
    *zt = *ze = -1;
    if (!last.y) return;
    if (pict_type == PT_B) {
      U8* dy = cur.y + (ptrdiff_t)by * ly + bx;
      U8* du = cur.u + (ptrdiff_t)mb_y * BLK_W * lc + mb_x * BLK_W;
      U8* dv = cur.v + (ptrdiff_t)mb_y * BLK_W * lc + mb_x * BLK_W;
      mpv_motion(dy, du, dv, 0, last, OP_PUT);
      mpv_motion(dy, du, dv, 1, next, OP_AVG);
      ref = cp;  // (the prediction in place)
    } else
      ref = last.y + (ptrdiff_t)by * ly + bx;
    if (by > 0) {
      for (x = 0; x < MB_W; x++, ne++) e += abs(ref[x] - cp[x - ly]);
      if (pict_type != PT_B)
        for (y = by >= T ? -T : -1; y < 0; y++)
          for (x = 0; x < MB_W; x++, n++) s += abs(cp[y * ly + x] - last.y[(ptrdiff_t)(by + y) * ly + bx + x]);
    }
    if (bx > 0) {
      for (y = 0; y < MB_W; y++, ne++) e += abs(ref[y * ly] - cp[y * ly - 1]);
      if (pict_type != PT_B)
        for (y = 0; y < MB_W; y++)
          for (x = bx >= T ? -T : -1; x < 0; x++, n++) s += abs(cp[y * ly + x] - last.y[(ptrdiff_t)(by + y) * ly + bx + x]);
    }
    *zt = n ? qratio(s * RC_Q / n) : -1;
    *ze = ne ? qratio(e * RC_Q / ne) : -1;
  }

  //--- a vector by template matching

  // The vector (in the stream's units: half or quarter samples) whose template matches best in ref:
  // among none, (ax, ay) and (bx, by) (in the stream's units; the vector's prediction and the
  // co-located one), in full samples, then RC_TM_RANGE around the best, then its half samples. 0:
  // no template.
  int tm_search(const RPic& ref, int ax, int ay, int bx_, int by_, int* mx, int* my) const {
    constexpr int R = 2 * RC_TM_RANGE;  // (in half samples)
    const int sh = qpel;  // (to half samples)
    int c[RC_TM_CANDS][2], i, n, best = 0, bcx, bcy, cx, cy, dx, dy, s;
    c[0][0] = c[0][1] = 0;
    c[1][0] = (ax >> sh) & ~1, c[1][1] = (ay >> sh) & ~1;
    c[2][0] = (bx_ >> sh) & ~1, c[2][1] = (by_ >> sh) & ~1;
    bcx = bcy = 0;
    for (i = 0; i < RC_TM_CANDS; i++) {
      s = tm_sad(ref.y, c[i][0], c[i][1], &n);
      if (!n) return 0;
      if (!i || s < best) best = s, bcx = c[i][0], bcy = c[i][1];
    }
    cx = bcx, cy = bcy;
    for (dy = -R; dy <= R; dy += 2)
      for (dx = -R; dx <= R; dx += 2) {
        if (!dx && !dy) continue;
        s = tm_sad(ref.y, cx + dx, cy + dy, &n);
        if (s < best) best = s, bcx = cx + dx, bcy = cy + dy;
      }
    cx = bcx, cy = bcy;
    for (dy = -1; dy <= 1; dy++)
      for (dx = -1; dx <= 1; dx++) {
        if (!dx && !dy) continue;
        s = tm_sad(ref.y, cx + dx, cy + dy, &n);
        if (s < best) best = s, bcx = cx + dx, bcy = cy + dy;
      }
    *mx = bcx * (1 << sh);
    *my = bcy * (1 << sh);
    return 1;
  }

  //--- the prediction's spectrum

  // the transform of block n's prediction (in the picture, before its residual: as the encoder saw
  // it), and which way its texture runs (*td: RC_TD_COLS its samples change more down the columns (at
  // least RC_TD_RATIO times), the residual's energy then in the first column; RC_TD_ROWS along the
  // rows; RC_TD_NEITHER)
  void pred_spec(int n, int field, int F[BLK_COEFS], int* td) const {
    const U8* q;
    int x, y, bx, by, sh = 0, sv = 0;
    blk_xy(n, field, &bx, &by);
    if (n < MB_LUMA) {
      rc_fdct(F, cur.y, ly, mb_w * MB_W, mb_h * MB_W, bx, by, field ? 2 : 1);
      q = cur.y + (ptrdiff_t)by * ly + bx;
      field = ly << field;
    } else {
      rc_fdct(F, cur.plane(n), lc, mb_w * BLK_W, mb_h * BLK_W, bx, by, 1);
      q = cur.plane(n) + (ptrdiff_t)by * lc + bx;
      field = lc;
    }
    for (y = 0; y < BLK_W; y++)
      for (x = 0; x < BLK_W; x++) {
        if (x < BLK_W - 1) sh += abs(q[y * field + x + 1] - q[y * field + x]);
        if (y < BLK_W - 1) sv += abs(q[(y + 1) * field + x] - q[y * field + x]);
      }
    *td = RC_TD_RATIO * sh < sv ? RC_TD_COLS : RC_TD_RATIO * sv < sh ? RC_TD_ROWS : RC_TD_NEITHER;
  }
};
