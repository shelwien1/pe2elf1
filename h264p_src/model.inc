// The model of the compressor (h264p_cm.md): the IDX families of decisions
// and values, what each one is fed from, and the channel through which the
// syntax walk hands the model the values of a NAL unit (coding) or takes
// them from it (decoding).
//
// Both directions run the walk that writes the bitstream (W), so the model
// sees the same state at every value: the walk's own (neighbours, the
// current macroblock) and the model's (the macroblocks of the current
// picture, the last one at each address in a picture of the same kind, the
// QP). Coding, the values come from the log of the compressor's dry run;
// decoding, from the range decoder.
//
// The model is a template on the level (LV, lv<L>.inc): which of its parts
// the level has (the decoder in the loop, its searches, ...) and its
// families' classes (h264p_levels.md sections 6 and 7).

namespace h264p {

// A family's tables, mapped: zero is every table's starting state, and a
// page costs memory only once the coder reaches it.
template <class T>
static T* tc_table() {
  return (T*)tc_map(sizeof(T));
}

// --- the levels of this build (levels.inc)
#include "levels.inc"

// where the bits go (-v)
enum { STG_NAL, STG_HDR, STG_RAW, STG_SKIP, STG_MBT, STG_CBP, STG_IPM, STG_MVS, STG_MVD, STG_MVSG, STG_QPD, STG_CBF,
       STG_SIG, STG_LVL, STG_SGN, STG_PCM, STG_N };
static const char* const kStage[STG_N] = {"NAL units", "headers", "bytes", "skip, end", "mb_type", "cbp, 8x8",
                                          "intra modes", "sub, ref", "mvd", "mvd signs", "qp delta", "coeff counts",
                                          "significance", "levels", "signs", "pcm"};

// the fields of the nal family: the container and what the syntax leaves out
enum { F_MORE, F_TYPE, F_REF, F_ZEROS, F_VERB, F_LEN, F_PADCHG, F_PADON, F_PADFRAME, F_TRAIL, F_DROP, F_NBITS,
       F_TBIT, F_CZW, F_JUNK, F_EOFZ, F_CRC, F_ALIGN, F_N };

// macroblock classes (ca, cb, cc, cls)
enum { CL_NONE, CL_SKIP, CL_DIRECT, CL_16x16, CL_PART, CL_8x8, CL_INXN, CL_I16, CL_PCM };

// What the model keeps of a macroblock of the current picture.
struct MbInfo {
  uint32_t pic;  // Model::pic_ when it was coded
  uint8_t skip, cls, qp, cbp, t8, cpred, i16m;
  // the rows of coef (plane * 16 + block) and of dc (48 + plane) written;
  // the others are zero (they hold what an older macroblock wrote: read
  // them through cmask)
  uint64_t cmask;
  uint8_t modes[16];        // Intra4x4/8x8 modes of the 4x4 blocks, raster order (2 for other macroblocks)
  int8_t ref[2][4];         // ref_idx of the 8x8 blocks as coded
  int8_t rf[2][4];          // refIdxLX of the 8x8 blocks, -1 where list X is not used
  int16_t mvd[2][16][2];    // mvd of the 4x4 blocks
  int8_t tme[2][16][2];     // the mvd less the template match's (clamped), -128 none
  int16_t mv[2][16][2];     // mvLX of the 4x4 blocks, reconstructed
  int8_t coef[3][16][16];   // coefficients of the 4x4 blocks (Y, Cb, Cr) by scan position, clipped
  int8_t dc[3][16];         // Intra16x16 DC and chroma DC coefficients
};

// What it keeps of the last macroblock at an address in a picture of the
// same kind (slice type, reference or not, frame or field).
struct MbCol {
  uint8_t valid, skip, cls, qp, cbp, t8, cpred, field;
  uint8_t modes[16];
  int8_t ref[2][4];
  int8_t rf[2][4];
  int rpoc[2][4];         // the POCs of rf's pictures, in its slice's lists (INT_MIN none; levels with them)
  uint8_t mvq[2][16][2];  // tc_qlog |mvd|
  int16_t mv[2][16][2];
  uint16_t nzm[3];        // 4x4 blocks with coefficients
  uint8_t dcm;            // DC blocks with coefficients
  uint16_t cm;            // the rows of coef that may not be zero
  int8_t coef[16][16];    // luma coefficients, as MbInfo's
};

static INLINE int cm_int(i64 v, i64 lim) {
  if (v < -lim || v > lim) cm_fail("corrupt stream: a value of %" PRId64 " out of range", v);
  return (int)v;
}
// a signed value as a class: 0 zero, then -1, +1, -2, +2, -3..., by tc_qlog
static INLINE int sq2(i64 d) { return d == 0 ? 0 : d > 0 ? 2 * tc_qlog(d) : 2 * tc_qlog(d) - 1; }
// a measure x (scaled by k / d), -1 not known, as a variable: 0 not
// known, else 1 + its log, at most 1 + n
static INLINE int qlog1(i64 x, int n, i64 k = 1, i64 d = 1) { return x < 0 ? 0 : 1 + tc_clamp(tc_qlog(x * k / d), 0, n); }
// The outside predictions of a value's decisions (tc_fam::prpa): -1, none,
// where not set
struct NodePrp {
  int p[TC_NODE];
  void Init() {
    for (int& v : p) v = -1;
  }
  operator int*() { return p; }
};
static INLINE int sgn3(int v) { return v > 0 ? 1 : v < 0 ? 2 : 0; }
// what a sign is told of another value d, knowing its own magnitude m:
// d's sign, and whether m is |d|, below it or above (the mvg and qpg families)
static INLINE int rel_sign(int m, int d) { return sgn3(d) * 3 + (m == abs(d) ? 0 : m < abs(d) ? 1 : 2); }
static INLINE int popc4(int v) { return (v & 1) + (v >> 1 & 1) + (v >> 2 & 1) + (v >> 3 & 1); }
// nC as a class
static INLINE int nc_class(int nc) { return nc < 3 ? nc : nc < 5 ? 3 : nc < 9 ? 4 : 5; }
// 16 log2 v, the fraction linear (integers only: the model must come out the
// same everywhere)
static INLINE int lg16(i64 v) {
  if (v < 1) v = 1;
  int n = 0;
  while (v >> (n + 1)) n++;
  return 16 * n + (int)(n >= 4 ? (v >> (n - 4)) & 15 : (v << (4 - n)) & 15);
}
// a / b rounded down (b not 0)
static INLINE i64 div_floor(i64 a, i64 b) {
  if (b < 0) a = -a, b = -b;
  return a >= 0 ? a / b : -((-a + b - 1) / b);
}

// x264's adaptive quantization: in a picture the QP of a macroblock is the
// picture's plus a multiple of log2 of the macroblock's energy (and, in a
// reference picture, MB-tree's offset). A least-squares line of QP against
// 16 log2 energy, through the macroblocks of the picture that coded
// mb_qp_delta (the energy of their reconstruction), predicts the QP of the
// next from the energy of its prediction.
struct AqFit {
  i64 n, sx, sy, sxx, sxy;
  i64 lb;  // the slope the last picture's line ended with (x264's at first)
  void Init() {
    n = sx = sy = sxx = sxy = 0;
    lb = 266;
  }
  void clear() {
    i64 b;
    if (slope(b)) lb = b;
    n = sx = sy = sxx = sxy = 0;
  }
  void add(int x, int q) {
    n++;
    sx += x;
    sy += q;
    sxx += (i64)x * x;
    sxy += (i64)x * q;
  }
  // QP per unit of x, x 4096; false while there are too few points for it
  bool slope(i64& b) const {
    if (n < 9) return false;
    i64 den = n * sxx - sx * sx;
    if (den <= 0) return false;
    b = (n * sxy - sx * sy) * 4096 / den;
    b = b < 0 ? 0 : b > 768 ? 768 : b;
    return true;
  }
  // 16 x the QP of energy x on the line (with fewer than 9 points, on the
  // last picture's slope, x264's 1.0397 per doubling of the energy at
  // first); false while there is none
  bool predict(int x, int& q16) const {
    i64 b;
    if (!n || (!slope(b) && n >= 9)) return false;
    if (n < 9) b = lb;
    q16 = (int)((16 * sy + 16 * b * ((i64)x * n - sx) / 4096) / n);
    return true;
  }
};

// The shift of an inter luma block's prediction that its residual looks
// like (the sgn family): the levels of the prediction's horizontal and
// vertical gradients (lx, ly) fitted by least squares to those of the
// coefficients whose signs are known (cv), in integers (the format
// depending on no floating point)
struct ShiftFit {
  i64 sxx, sxy, syy, sxc, syc;
  void Init() { sxx = sxy = syy = sxc = syc = 0; }
  void add(i64 lx, i64 ly, i64 cv) { sxx += lx * lx, sxy += lx * ly, syy += ly * ly, sxc += lx * cv, syc += ly * cv; }
  // the shift fitted so far: (fx, fy) / fd, scaled into 40 bits; fd 0 none
  void fit(i64& fx, i64& fy, i64& fd) const {
    i64 det = sxx * syy - sxy * sxy, sum = sxx + syy + 1;
    if (det > 0 && det > (sum * sum >> 30)) {
      fx = syy * sxc - sxy * syc, fy = sxx * syc - sxy * sxc, fd = det;
    } else if (sxx + syy + 2 * sxy > 0) {
      // one direction only so far: along the gradients seen
      fx = fy = sxc + syc, fd = sxx + syy + 2 * sxy;
    } else {
      fx = fy = fd = 0;
      return;
    }
    while (Max(Max(fx < 0 ? -fx : fx, fy < 0 ? -fy : fy), fd) >= ((i64)1 << 40)) fx /= 2, fy /= 2, fd /= 2;
  }
};

// x264's AQ as the model follows it: per picture three lines (AqFit),
// through the macroblocks that coded mb_qp_delta, of the QP against the
// energy of their reconstruction (fit), of their prediction (fitp: inter)
// and of their proxy (fitx: intra); the energies of the macroblocks of
// the current and the last picture (16 log2, -1 not known); the QP of the
// current picture's that coded mb_qp_delta (-1 other); and per kind of
// picture how far the QP of the last one's was from its line (16ths,
// INT16_MIN not known)
struct AqModel {
  AqFit fit, fitp, fitx;
  int xp;  // the energy of the current macroblock's prediction (inter), or (xi) its proxy's (intra)
  bool xi;
  Buf<int16_t> x, xl, q, res[30];
  int key;
  void Init() {
    fit.Init(), fitp.Init(), fitx.Init();
    xp = -1;
    xi = false;
    x.Init(), xl.Init(), q.Init();
    for (auto& r : res) r.Init();
    key = -1;
  }
  void Quit() {
    x.Quit(), xl.Quit(), q.Quit();
    for (auto& r : res) r.Quit();
  }
  // a picture of kind k and n macroblocks begins: the last one's lines and
  // energies are the past
  void picture(int k, int n) {
    if (key >= 0) {
      Buf<int16_t>& r = res[key];
      r.fill(q.size(), INT16_MIN);
      for (size_t a = 0; a < q.size(); a++) {
        int q16;
        if (q[a] >= 0 && fit.predict(x[a], q16)) r[a] = (int16_t)tc_clamp(16 * q[a] - q16, -32767, 32767);
      }
    }
    xl.swap(x);
    x.fill(n, -1);
    q.fill(n, -1);
    fit.clear();
    fitp.clear();
    fitx.clear();
    key = k;
  }
  // macroblock a decoded: the energy e of its reconstruction, its QP qp,
  // and whether it coded mb_qp_delta
  void decoded(int a, int e, int qp, bool coded) {
    x[a] = (int16_t)e;
    if (!coded) return;
    fit.add(e, qp);
    if (xp >= 0) (xi ? fitx : fitp).add(xp, qp);
    q[a] = (int16_t)qp;
  }
  // the energy where the last picture's and the current one's left and
  // upper neighbours' put macroblock a (their mean; -1 none), the picture
  // w macroblocks wide
  int around(int a, int w) const {
    int xs = 0, xn = 0;
    if (xl.size() == x.size() && xl[a] >= 0) xs += xl[a], xn++;
    if (a % w && x[a - 1] >= 0) xs += x[a - 1], xn++;
    if (a >= w && x[a - w] >= 0) xs += x[a - w], xn++;
    return xn ? xs / xn : -1;
  }
};

// 16 x the quantizer's step of a QP (0.625 x 2^(qp/6))
static INLINE int qstep16(int qp) {
  static const int k[6] = {10, 11, 13, 14, 16, 18};
  int q = qp < 0 ? 0 : qp > 51 ? 51 : qp;
  return k[q % 6] << (q / 6);
}

// 16 x x264's lambda of a QP (x264_lambda_tab: 2^(qp/6 - 2), at least 1)
static INLINE int x264_lam16(int qp) {
  static const int k16[6] = {16, 18, 20, 23, 25, 29};
  int q = qp < 0 ? 0 : qp > 51 ? 51 : qp;
  return Max(16, (k16[q % 6] << (q / 6)) >> 2);
}

// P(LPS) of the CABAC context states, 12 bits: 2048 * (0.01875 / 0.5)^(s / 63)
static const int cab_lps[64] = {
    2048, 1944, 1845, 1752, 1663, 1578, 1498, 1422, 1350, 1281, 1216, 1154, 1096, 1040, 987, 937,
    890, 844, 802, 761, 722, 685, 651, 618, 586, 557, 528, 501, 476, 452, 429, 407,
    386, 367, 348, 330, 314, 298, 283, 268, 255, 242, 229, 218, 207, 196, 186, 177,
    168, 159, 151, 144, 136, 129, 123, 117, 111, 105, 100, 95, 90, 85, 81, 77,
};

// Sign prediction for a luma block: the cost of each combination of the
// signs of up to six coefficients is how far the block's reconstruction,
// along its top row and left column, is from continuing the two decoded rows
// and columns beyond them. The inverse transform is taken as linear (with
// its basis times 2 for 4x4 blocks, times 8 for 8x8 ones), so a combination
// is the sum of the prediction and the coefficients' contributions.
struct SignPred {
  static constexpr int kT4[4][4] = {{2, 2, 2, 2}, {2, 1, -1, -2}, {2, -2, -2, 2}, {1, -2, 2, -1}};
  static constexpr int kT8[8][8] = {{8, 8, 8, 8, 8, 8, 8, 8},       {12, 10, 6, 3, -3, -6, -10, -12},
                                    {8, 4, -4, -8, -8, -4, 4, 8},   {10, -3, -12, -6, 6, 12, 3, -10},
                                    {8, -8, -8, 8, 8, -8, -8, 8},   {6, -12, 3, 10, -10, -3, 12, -6},
                                    {4, -8, 8, -4, -4, 8, -8, 4},   {3, -6, 10, -12, 12, -10, 6, -3}};
  int n, off, qp, ne, nc;
  bool b8;
  int dc;  // 1 chroma DC (2x2, over the 8x8 block), 2 Intra16x16 DC (4x4, over the 16x16 one)
  int64_t scale, target[32], base[32], contrib[6][32], cost[64];
  bool avail[32];
  int fixed_mask, fixed_val;
  void Init() {
    n = 4, off = qp = 0, ne = 8, nc = 0;
    b8 = false;
    dc = 0;
    scale = 256;
    fixed_mask = fixed_val = 0;
  }
  // the basis at edge sample e of the coefficient at raster position r;
  // for DC blocks, the Hadamard pattern of the 4x4 block e is in
  int basis(int r, int e) const {
    int x = e < n ? e : 0, y = e < n ? 0 : e - n;
    if (dc == 1) {
      int b = (y >> 2) * 2 + (x >> 2);
      static const int kH2[4][4] = {{1, 1, 1, 1}, {1, -1, 1, -1}, {1, 1, -1, -1}, {1, -1, -1, 1}};
      return kH2[b][r];
    }
    if (dc == 2) {
      static const int kH4[4][4] = {{1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1}};
      return kH4[y >> 2][r >> 2] * kH4[r & 3][x >> 2];
    }
    int u = b8 ? r >> 3 : r >> 2, v = b8 ? r & 7 : r & 3;
    return b8 ? kT8[u][y] * kT8[v][x] : kT4[u][y] * kT4[v][x];
  }
  template <class R, class Geo>
  bool init(R& rec, const Geo& g, int cat, int q, int blk) {
    b8 = cat == 5;
    dc = cat == 3 ? 1 : cat == 0 ? 2 : 0;
    n = b8 || dc == 1 ? 8 : dc == 2 ? 16 : 4;
    ne = 2 * n;
    scale = b8 ? 4096 : 256;
    off = cat == 1 || cat == 4;
    qp = cat == 4 || cat == 3 ? rec.chroma_qp_of(g.plane - 1, q) : q;
    if ((cat == 4 || cat == 3) != (g.plane != 0) ||
        !rec.sign_edges(g.plane, dc ? 0 : g.x * 4, dc ? 0 : g.y * 4, n, scale, target, avail))
      return false;
    for (int e = 0; e < ne; e++) base[e] = 0;
    if (cat == 1 || cat == 4) {  // the DC of an Intra16x16 or chroma block is known
      int dcv = cat == 1 ? rec.dc16_at(q, blk) : rec.dcc_at(g.plane - 1, g.y * 2 + g.x, q);
      for (int e = 0; e < ne; e++) base[e] += (int64_t)dcv * 4;
    }
    nc = 0;
    fixed_mask = fixed_val = 0;
    return true;
  }
  int raster(int k) const { return dc == 1 ? k : b8 ? kZz8[k + off] : kZz4[k + off]; }
  // a coefficient's contribution to the samples, positive, times scale
  int64_t level(int k, int m) const {
    if (!dc) return Recon::dequant1(m, k + off, qp, b8);
    int ls = 16 * kNorm4[qp % 6][0];
    // chroma DC: ((f ls) << qp/6) >> 5; Intra16x16: (f ls) << qp/6 >> 6; and x4 for the basis
    return dc == 1 ? ((int64_t)m * ls << (qp / 6)) / 8 : ((int64_t)m * ls << (qp / 6)) / 16;
  }
  // a coefficient whose sign is known
  void add(int k, int c) {
    int64_t d = level(k, abs(c)) * (c < 0 ? -1 : 1);
    for (int e = 0; e < ne; e++) base[e] += d * basis(raster(k), e);
  }
  // one whose sign is predicted, positive
  void cand(int k, int m) {
    int64_t d = level(k, m);
    for (int e = 0; e < ne; e++) contrib[nc][e] = d * basis(raster(k), e);
    nc++;
  }
  void costs() {
    int64_t x[32];
    for (int e = 0; e < ne; e++) {
      x[e] = base[e];
      for (int j = 0; j < nc; j++) x[e] += contrib[j][e];
    }
    int mask = 0;
    for (int i = 0; i < (1 << nc); i++) {
      if (i) {  // Gray code: one sign flips
        int j = __builtin_ctz((unsigned)i);
        int64_t sgn = mask >> j & 1 ? 2 : -2;
        mask ^= 1 << j;
        for (int e = 0; e < ne; e++) x[e] += sgn * contrib[j][e];
      }
      int64_t cst = 0;
      for (int e = 0; e < ne; e++)
        if (avail[e]) cst += x[e] > target[e] ? x[e] - target[e] : target[e] - x[e];
      cost[mask] = cst;
    }
  }
  // the predicted sign (1 negative) of candidate j, given the ones decided,
  // and how much worse the other one is (quarter samples)
  int predict(int j, int& margin) const {
    int64_t c0 = INT64_MAX, c1 = INT64_MAX;
    for (int m = 0; m < (1 << nc); m++) {
      if ((m & fixed_mask) != fixed_val) continue;
      if (m >> j & 1)
        c1 = Min(c1, cost[m]);
      else
        c0 = Min(c0, cost[m]);
    }
    int64_t d = (c0 > c1 ? c0 - c1 : c1 - c0) * 4 / scale;
    margin = (int)Min<int64_t>(d, 1 << 20);
    return c1 < c0;
  }
  void decide(int j, int negative) {
    fixed_mask |= 1 << j;
    if (negative) fixed_val |= 1 << j;
  }
  // the level of the coefficient at scan position k the edges ask for: what
  // is left of the targets projected onto its basis, as a class (1 +
  // tc_qlog of half levels, 0 nothing)
  int estimate(int k) const {
    int r = raster(k);
    int64_t num = 0, den = 0;
    for (int e = 0; e < ne; e++)
      if (avail[e]) {
        int b = basis(r, e);
        num += (target[e] - base[e]) * b;
        den += b * b;
      }
    int64_t l1 = level(k, 1);
    if (!den || !l1) return 0;
    int64_t m2 = (num < 0 ? -num : num) * 2 / den / l1;
    return m2 ? 1 + Min(tc_qlog(m2), 14) : 0;
  }
};
constexpr int SignPred::kT4[4][4];
constexpr int SignPred::kT8[8][8];

// the weight of a quarter sample of vector difference (from mvp) against a
// sample of template difference in the mvd's template matching: x264's
// motion search weighs the mvd too (a fixed weight did better than its
// lambda by QP)
constexpr int kTmLambda = 2;
// the scan positions of the raster positions of 4x4 and 8x8 blocks (kZz4, kZz8 inverted)
struct ZzInv {
  uint8_t v4[16], v8[64];
};
static ZzInv zz_inv() {
  ZzInv z;
  for (int k = 0; k < 16; k++) z.v4[kZz4[k]] = (uint8_t)k;
  for (int k = 0; k < 64; k++) z.v8[kZz8[k]] = (uint8_t)k;
  return z;
}
static const ZzInv kZzInv = zz_inv();
static const uint8_t* const kInv4 = kZzInv.v4;
static const uint8_t* const kInv8 = kZzInv.v8;
// 4:2:2 chroma DC's scan, down the columns of its 2 x 4 (8.5.11.1), as
// raster positions and back; and the raster order itself (4:2:0's 2 x 2)
const uint8_t kZzC422[8] = {0, 2, 1, 4, 6, 3, 5, 7};
const uint8_t kInvC422[8] = {0, 2, 1, 5, 3, 6, 4, 7};
const uint8_t kRaster4[4] = {0, 1, 2, 3};

// The model of a level, coding or decoding: the range coder, which its
// families code through, is its base (and the coroutine of the run that
// base's, rcio.inc), its tables its own (mapped as it starts, unmapped as it
// ends)
// Bytes nothing else models (Model::raw()): an order 0 and an order 1
// counter and a match model mixed by a weight set the bits of the byte so
// far select (oggcomp's raw model and a match): the match finds the last
// three bytes in what was coded so far, after a dictionary of the user data
// x264 writes in an SEI message (its UUID and its options with their
// defaults). (Its counters and weights zero as it starts: the
// decompressor's must match.)
template <int f_DEC>
struct RawModel {
  static constexpr int ENC = f_DEC == 0;
  void Init() {
    memset(o0_, 0, sizeof o0_);
    memset(o1_, 0, sizeof o1_);
    memset(w_, 0, sizeof w_);
    memset(wc_, 0, sizeof wc_);
    prev_ = 0;
    buf_.Init();
    hash_.Init();
    ptr_ = dict_ = 0;
    len_ = 0;
    memset(mc_, 0, sizeof mc_);
  }
  void Quit() {
    buf_.Quit();
    hash_.Quit();
  }
  void start() {
    mx_.init(w_, wc_);
    buf_.clear();
  }

  cm_cnt o0_[256];
  cm_cnt o1_[256][256];
  i32 w_[256 * 4];
  u8 wc_[256];
  cm_mix<4> mx_;
  u32 prev_;
  Buf<u8> buf_;    // the dictionary, then the bytes coded
  Buf<i32> hash_;  // by the hash of three bytes, the place after them in buf_ (-1 none)
  size_t ptr_, dict_;  // where the match predicts from; the dictionary's size
  int len_;            // the match's length (0 none)
  cm_cnt mc_[16];      // whether the match's bit is right, by its length
  static u32 key(const u8* p) { return ((u32)p[0] * 0x9E3779B1u ^ (u32)p[1] * 0x85EBCA77u ^ (u32)p[2] * 0xC2B2AE3Du) >> 16; }
  void init_dict() {
    static const u8 kUuid[16] = {0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef};
    static const char kText[] =
        "x264 - core 164 - H.264/MPEG-4 AVC codec - Copyleft 2003-2024 - http://www.videolan.org/x264.html - options: "
        "cabac=1 ref=3 deblock=1:0:0 analyse=0x3:0x113 me=hex subme=7 psy=1 psy_rd=1.00:0.00 mixed_ref=1 me_range=16 "
        "chroma_me=1 trellis=1 8x8dct=1 cqm=0 deadzone=21,11 fast_pskip=1 chroma_qp_offset=-2 threads=8 "
        "lookahead_threads=1 sliced_threads=0 nr=0 decimate=1 interlaced=0 bluray_compat=0 constrained_intra=0 "
        "bframes=3 b_pyramid=2 b_adapt=1 b_bias=0 direct=1 weightb=1 open_gop=0 weightp=2 keyint=250 keyint_min=25 "
        "scenecut=40 intra_refresh=0 rc_lookahead=40 rc=crf mbtree=1 crf=23.0 qcomp=0.60 qpmin=0 qpmax=69 qpstep=4 "
        "ip_ratio=1.40 aq=1:1.00";
    buf_.assign(kUuid, 16);
    buf_.append((const u8*)kText, sizeof kText);  // with the terminating zero x264 writes
    hash_.fill((size_t)1 << 16, -1);
    for (size_t i = 3; i <= buf_.size(); i++) hash_[key(&buf_[i - 3])] = (i32)i;
    dict_ = buf_.size();
    len_ = 0;
  }
  // a byte coded (its value b coding; decoding, returned), by the coder rc
  u32 code(tc_rc<f_DEC>* rc, u32 b) {
    if (buf_.empty()) init_dict();
    cm_cnt* row = o1_[prev_];
    u32 node = 1;
    int pbyte = len_ && ptr_ < buf_.size() ? buf_[ptr_] : -1, ml = Min(len_, 15);
    for (int i = 7; i >= 0; i--) {
      cm_cnt c0 = o0_[node], c1 = row[node];
      mx_.set(0, cm_stretch(c0.P()));
      mx_.set(1, c1.t ? cm_stretch(c1.P()) : 0);
      mx_.set(2, 256);
      // the match's bit, while the byte so far agrees with its
      int mb = -1;
      if (pbyte >= 0 && (u32)(pbyte | 256) >> (i + 1) == node) mb = pbyte >> i & 1;
      int sm = mb < 0 ? 0 : cm_stretch(mc_[ml].P());
      mx_.set(3, mb < 0 ? 0 : mb ? sm : -sm);
      int bit = tc_bit(rc, mx_.mix(node), (int)(ENC && ((b >> i) & 1)));
      mx_.upd(bit, 24, 0);
      c0.upd(bit, 250, 0);
      c1.upd(bit, 60, 0);
      if (mb >= 0) mc_[ml].upd(bit == mb, 250, 0);
      o0_[node] = c0;
      row[node] = c1;
      node = node * 2 + (u32)bit;
    }
    prev_ = node & 255;
    // the match goes on or ends; then the bytes coded join what it searches
    // (the first 256 MB of them)
    if (pbyte >= 0 && (u32)pbyte == (node & 255))
      len_++, ptr_++;
    else
      len_ = 0;
    if (buf_.size() < ((size_t)1 << 28)) {
      buf_.push_back((u8)(node & 255));
      size_t n = buf_.size();
      if (n - dict_ >= 3) {
        u32 k = key(&buf_[n - 3]);
        if (!len_ && hash_[k] >= 0) ptr_ = (size_t)hash_[k], len_ = 1;
        hash_[k] = (i32)n;
      }
    }
    rc->tc_syms[STG_RAW]++;
    return node & 255;
  }
};

// Header syntax (Model::hval()): a value is coded as the difference to the
// last value of the same element (tag, place in the group, row in a run of
// the tag), kept per kind of slice, by the hdr family (code())
struct HeaderModel {
  struct HLast {
    i64 v, d1, d2;  // the last value, the last two differences
  };
  // the names of the groups, numbered as they come (the tags; 256 at
  // most): each name's text in names_ at the place tag_at_ has for it, by
  // the name's hash in tag_ix_ (its tag + 1, 0 none)
  Buf<char> names_;
  Buf<uint32_t> tag_at_;
  uint16_t tag_ix_[512];
  int tag_, fld_, lasttag_, runpos_;
  int sty_, last_sty_, ref_idc_;  // slot of the current NAL unit: slice type x reference, 10 none
  bool slice_nal_;
  bool is_slice_type_;
  // frame_num and pic_order_cnt_lsb wrap: their differences are taken
  // modulo 2^hbits (the bits of the u(n) to come, 0 none)
  bool is_wrap_;
  int hbits;
  Map32<HLast> hl_;  // by tag, place in the group, row in a run of the tag (up to 15), slice kind

  void Init() {
    names_.Init();
    tag_at_.Init();
    memset(tag_ix_, 0, sizeof tag_ix_);
    tag_ = fld_ = 0, lasttag_ = -1, runpos_ = 0;
    sty_ = last_sty_ = 10, ref_idc_ = 0;
    slice_nal_ = is_slice_type_ = is_wrap_ = false;
    hbits = 0;
    hl_.Init();
    last_sref_ = prev_sref_ = 2;
  }
  void Quit() {
    names_.Quit();
    tag_at_.Quit();
    hl_.Quit();
  }
  // the tag of the group name, given the next one if it is new
  int tag_of(const char* name) {
    uint32_t h = 2166136261u;  // (FNV-1a)
    for (const char* c = name; *c; c++) h = (h ^ (uint8_t)*c) * 16777619u;
    for (uint32_t i = h & 511;; i = (i + 1) & 511) {
      int k = tag_ix_[i];
      if (!k) {
        if (tag_at_.size() >= 256) cm_fail("internal: more than 256 header elements");
        tag_at_.push_back((uint32_t)names_.size());
        names_.append(name, strlen(name) + 1);
        tag_ix_[i] = (uint16_t)tag_at_.size();
        return (int)tag_at_.size() - 1;
      }
      if (!strcmp(names_.data() + tag_at_[k - 1], name)) return k - 1;
    }
  }

  // the start of a NAL unit coded by the walk
  int last_sref_, prev_sref_;  // whether the last slice NAL unit, and the one before, were references (2 none)
  void nal_begin(int type, int ref_idc) {
    slice_nal_ = type == 1 || type == 5;
    ref_idc_ = ref_idc;
    if (slice_nal_) prev_sref_ = last_sref_, last_sref_ = ref_idc != 0;
    sty_ = slice_nal_ ? last_sty_ : 10;
  }
  // a group of values (a syntax element, or a group of them), by its name
  void begin(const char* name) {
    tag_ = tag_of(name);
    is_slice_type_ = slice_nal_ && !strcmp(name, "slice_type");
    is_wrap_ = !strcmp(name, "frame_num") || !strcmp(name, "pic_order_cnt_lsb");
    fld_ = 0;
    runpos_ = tag_ == lasttag_ ? runpos_ + 1 : 0;
    lasttag_ = tag_;
  }
  // its next value (x, encoding; decoding, returned), by the hdr family f
  template <class F>
  i64 code(F& f, bool enc, i64 x) {
    HLast& h = hl_.get((u32)tag_ | (u32)(fld_ < 15 ? fld_ : 15) << 8 | (u32)(runpos_ < 15 ? runpos_ : 15) << 12 | (u32)sty_ << 16);
    H_hdr_v D;
    D.tag = tag_;
    D.fld = fld_;
    D.p1 = sq2(h.d1);
    D.p2 = sq2(h.d2);
    D.pos = tc_qlog(runpos_);
    D.p1r = (int)(h.v & 255);
    D.sty = sty_ < 9 ? sty_ : 9;
    D.pr = slice_nal_ ? prev_sref_ : 3;
    tcx X;
    f.make(D, X);
    i64 m = is_wrap_ && hbits > 0 && hbits < 31 ? (i64)1 << hbits : 0, dx = enc ? x - h.v : 0;
    hbits = 0;
    if (m && enc) dx = ((dx % m) + m + m / 2) % m - m / 2;  // into [-m/2, m/2)
    i64 d = f.codes(X, dx, (u32)tag_);
    x = m ? ((h.v + d) % m + m) % m : h.v + d;
    h.d2 = h.d1;
    h.d1 = d;
    h.v = x;
    fld_++;
    if (is_slice_type_) sty_ = last_sty_ = (int)(x % 5) * 2 + (ref_idc_ != 0);
    return x;
  }
};

// --- The model's histories: per kind of picture (slice type, reference or
// not, frame or top or bottom field: 30 kinds, pic_kind) what it keeps of
// the last macroblock coded at each address (MbCol), and which pictures
// those are; and, from the first P slice of a P picture on, the records of
// its kind as they were when the picture began, for the contexts that
// follow a block's vector into its reference (Model::mc_col, mc_nz). (The
// compressor stores verbatim a slice that would take the kinds' records
// past a bound: HistBound, cmstream.inc.)
struct History {
  Buf<MbCol> rec[30];
  Buf<uint8_t> skip[30];  // rec's valid && skip, a byte a macroblock (for skp_vars)
  MbCol* cur;             // rec of the current slice's kind
  // each kind's: the POC of the picture it is (INT_MIN none) and of the
  // picture of the kind before it, and the picture it is (Model::pic_):
  // its first slice of the kind made it so
  int poc[30], prev[30];
  uint32_t pic[30];
  // The records of kind key as they were when its picture began (if ok: a
  // P picture): rec[key] where keep's stamp is not snap, else keep and
  // coef, where before_write() keeps what views read of a record before it
  // is rewritten (size records; at a picture of another size, unused).
  // (Copying the records as the picture begins costs more: it touches
  // every record, cold; h264p_l1_speed.md section 6.8.)
  struct Keep {
    uint32_t stamp;  // kept for the picture that began with snap this
    uint8_t valid, cls;
    uint16_t cm, nzm;  // nzm[0]
  };
  struct Coef {
    int8_t r[16][16];  // (rows outside cm zero)
  };
  Buf<Keep> keep;
  Buf<Coef> coef;
  uint32_t snap;
  int key, size;
  bool ok;
  void Init() {
    for (int k = 0; k < 30; k++) rec[k].Init(), skip[k].Init(), poc[k] = prev[k] = INT_MIN, pic[k] = 0;
    cur = nullptr;
    keep.Init();
    coef.Init();
    snap = 0;
    key = size = 0;
    ok = false;
  }
  void Quit() {
    for (int k = 0; k < 30; k++) rec[k].Quit(), skip[k].Quit();
    keep.Quit();
    coef.Quit();
  }

  // a slice of kind k begins, in a picture of n macroblocks (newpic: its
  // first slice); p: a P slice whose picture's records are kept as they were
  void slice(int k, int n, bool newpic, bool p) {
    if ((int)rec[k].size() < n) rec[k].resize(n), skip[k].resize(n);
    cur = rec[k].data();
    // (from the picture's first P slice: a picture may begin with another)
    if (newpic) ok = false;
    if (p && !ok) {
      ok = true;
      if ((int)keep.size() < n) keep.resize(n), coef.resize(n);
      snap++;  // (nothing kept yet: every record is rec's)
      key = k;
      size = n;
    }
  }
  // kind k's first slice in picture p, of POC pc: the kind's records are
  // the picture's from here on
  void picture(int k, uint32_t p, int pc) {
    if (pic[k] == p) return;
    pic[k] = p;
    prev[k] = poc[k];
    poc[k] = pc;
  }
  // before the record of macroblock a of kind k is rewritten: what views
  // read of it (valid, cls, nzm[0], cm and the coefficient rows, those
  // outside cm zero as in rec), kept as it was when the picture began
  INLINE void before_write(int k, int a) {
    if (!ok || k != key || a >= size || keep[a].stamp == snap) return;
    const MbCol& s = rec[k][a];
    Keep& d = keep[a];
    for (uint32_t m = (uint32_t)(d.cm | s.cm); m; m &= m - 1) {
      int r = __builtin_ctz(m);
      memcpy(coef[a].r[r], s.coef[r], 16);
    }
    d = Keep{snap, s.valid, s.cls, s.cm, s.nzm[0]};
  }
  // the kind, other than k, whose records are the picture with POC pc (of
  // n macroblocks); -1 none
  int of_poc(int pc, int k, int n) const {
    for (int j = 0; j < 30; j++)
      if (j != k && poc[j] == pc && (int)rec[j].size() >= n) return j;
    return -1;
  }

  // What a view reads of a macroblock's record
  struct Ref {
    uint8_t valid, cls;
    uint16_t cm, nzm;          // nzm[0]
    const int8_t (*coef)[16];  // the luma coefficients (rows outside cm zero)
  };
  // the records of a picture, as they were: by address
  struct View {
    const MbCol* cur;   // a kind's records (none: no view)
    const Keep* keep;   // and those kept as they were, where rewritten since (snap)
    const Coef* coef;
    uint32_t id;
    void Init() {
      cur = nullptr;
      keep = nullptr;
      coef = nullptr;
      id = 0;
    }
    explicit operator bool() const { return cur != nullptr; }
    Ref at(int a) const {
      if (keep && keep[a].stamp == id) {
        const Keep& k = keep[a];
        return Ref{k.valid, k.cls, k.cm, k.nzm, coef[a].r};
      }
      const MbCol& c = cur[a];
      return Ref{c.valid, c.cls, c.cm, c.nzm[0], c.coef};
    }
  };
  // those of the picture with POC pc (none: false), from a slice of kind k
  // in a picture of n macroblocks whose kind's last picture had POC pp:
  // the current kind's as they were if it is that one, else another kind's
  View view(int pc, int pp, int k, int n) const {
    View v;
    v.Init();
    if (pc == pp && size == n) {
      if (ok && key == k) v.cur = rec[key].data(), v.keep = keep.data(), v.coef = coef.data(), v.id = snap;
      return v;
    }
    int j = of_poc(pc, k, n);
    if (j >= 0) v.cur = rec[j].data();
    return v;
  }
};

// --- The motion vectors as the decoder reconstructs them (8.4.1): the
// prediction a partition's mvd is coded against (pred), P_Skip's motion
// (pskip) and B direct's (direct). From the motion of the macroblocks
// around (their records, by the walk's neighbours), of the partitions of
// the current one decoded so far (its record and their shape) and of the
// decoder's colocated pictures; into the arrays it is given (a
// macroblock's record's, or the skip motion the model looks at first).
template <class R>
struct MotionPred {
  struct MvN {
    bool avail;  // the partition is there (it may be intra or not use the list)
    int ref, mv[2];
  };
  // the slice's walk, its macroblock states and their records (by
  // address), its decoder; the current macroblock: its address, its record
  // and the shape of its partitions (16x16, 16x8, 8x16, 8x8; the
  // sub-macroblocks')
  const WalkState* sc;
  const MbState* mbs;
  const MbInfo* info;
  const R* rec;
  int width;
  bool mbaff;
  int addr;
  const MbInfo* mi;
  int parts, subshape[4];
  void Init() {
    sc = nullptr;
    mbs = nullptr;
    info = nullptr;
    rec = nullptr;
    width = 1;
    mbaff = false;
    addr = 0;
    mi = nullptr;
    parts = 0;
    for (int& s : subshape) s = 0;
  }
  void slice(const WalkState& w, const MbInfo* inf, const R& r) {
    sc = &w, mbs = w.store_->mbs.data(), info = inf, rec = &r, width = w.width_, mbaff = w.mbaff_;
  }
  void begin(int a, const MbInfo* m) { addr = a, mi = m, parts = 0; }

  // the place of the partition with the 4x4 block (x, y) of the current
  // macroblock in decoding order
  int part_ord(int x, int y) const {
    switch (parts) {
      case 0:
        return 0;
      case 1:
        return y >> 1;
      case 2:
        return x >> 1;
      default: {
        int i = (y >> 1) * 2 + (x >> 1), sh = subshape[i];
        return 4 * i + (sh == 0 ? 0 : sh == 1 ? y & 1 : sh == 2 ? x & 1 : (y & 1) * 2 + (x & 1));
      }
    }
  }
  // the motion of the neighbouring 4x4 block (x, y) of the current
  // macroblock, x in -1..4, y in -1..3, for a partition at place ord
  // (8.4.1.3.2); approximate at the corners of MBAFF macroblock pairs
  MvN nb(int list, int x, int y, int ord) const {
    MvN r = {false, -1, {0, 0}};
    const MbState* s;
    const MbInfo* m;
    int i = 0;
    if (x >= 0 && y >= 0) {
      if (x > 3 || part_ord(x, y) > ord) return r;  // not decoded yet
      s = sc->cur_;
      m = mi;
      i = y * 4 + x;
    } else if ((x < 0 && y < 0) || x > 3) {  // D or C, in the macroblock above left or right
      if (mbaff || addr < width) return r;
      int a = x < 0 ? (addr % width ? addr - width - 1 : -1) : (addr % width < width - 1 ? addr - width + 1 : -1);
      if (a < 0 || mbs[a].slice != sc->slice_id_) return r;
      s = &mbs[a];
      m = &info[a];
      i = x < 0 ? 15 : 12;
    } else {
      s = sc->nb4(x, y, i);
      if (!s) return r;
      m = &info[s - mbs];
    }
    r.avail = true;
    int ref = m->rf[list][(i >> 3) * 2 + ((i & 3) >> 1)];
    if (ref < 0) return r;
    r.ref = ref;
    r.mv[0] = m->mv[list][i][0];
    r.mv[1] = m->mv[list][i][1];
    if (mbaff && s != sc->cur_) {
      if (sc->cur_->field && !s->field) {
        r.mv[1] /= 2;
        r.ref *= 2;
      } else if (!sc->cur_->field && s->field) {
        r.mv[1] *= 2;
        r.ref >>= 1;
      }
    }
    return r;
  }
  static int median3(int a, int b, int c) { return a + b + c - Min(a, Min(b, c)) - Max(a, Max(b, c)); }
  // the prediction of the motion vector of a partition (8.4.1.3); nbs gets
  // its neighbours A, B and C
  void pred(int list, int x, int y, int w, int h, int ref, int* p, MvN* nbs = nullptr) const {
    int ord = part_ord(x, y);
    MvN a = nb(list, x - 1, y, ord), b = nb(list, x, y - 1, ord), c = nb(list, x + w, y - 1, ord);
    if (!c.avail) c = nb(list, x - 1, y - 1, ord);
    if (nbs) {
      nbs[0] = a;
      nbs[1] = b;
      nbs[2] = c;
    }
    pred(a, b, c, x, y, w, h, ref, p);
  }
  // the same from its neighbours A, B and C (C, D where C is not available)
  static void pred(const MvN& a, MvN b, MvN c, int x, int y, int w, int h, int ref, int* p) {
    const MvN* d = nullptr;
    if (w == 4 && h == 2)
      d = y == 0 ? (b.ref == ref ? &b : nullptr) : (a.ref == ref ? &a : nullptr);
    else if (w == 2 && h == 4)
      d = x == 0 ? (a.ref == ref ? &a : nullptr) : (c.ref == ref ? &c : nullptr);
    if (!d) {
      if (!b.avail && !c.avail && a.avail) b = c = a;
      int n = (a.ref == ref) + (b.ref == ref) + (c.ref == ref);
      if (n == 1) d = a.ref == ref ? &a : b.ref == ref ? &b : &c;
    }
    if (d) {
      p[0] = d->mv[0];
      p[1] = d->mv[1];
    } else {
      for (int k = 0; k < 2; k++) p[k] = median3(a.mv[k], b.mv[k], c.mv[k]);
    }
  }
  // the motion of a partition of list l (4x4 blocks x, y, w x h): its
  // reference ref and vector p, into the vectors mv and references rf
  static void set(int16_t (*mv)[16][2], int8_t (*rf)[4], int list, int x, int y, int w, int h, int ref, const int* p) {
    int16_t v[2] = {(int16_t)tc_clamp(p[0], -32768, 32767), (int16_t)tc_clamp(p[1], -32768, 32767)};
    if (w == 4 && h == 4) {
      for (int b = 0; b < 16; b++) memcpy(mv[list][b], v, sizeof v);
      memset(rf[list], ref, 4);
      return;
    }
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) memcpy(mv[list][yy * 4 + xx], v, sizeof v);
    for (int yy = y; yy < y + h; yy += 2)
      for (int xx = x; xx < x + w; xx += 2) rf[list][(yy >> 1) * 2 + (xx >> 1)] = (int8_t)ref;
  }
  // P_Skip (8.4.1.1)
  void pskip(int16_t (*mv)[16][2], int8_t (*rf)[4]) {
    parts = 0;
    MvN a = nb(0, -1, 0, 0), b = nb(0, 0, -1, 0);
    int p[2] = {0, 0};
    if (a.avail && b.avail && !(a.ref == 0 && !a.mv[0] && !a.mv[1]) && !(b.ref == 0 && !b.mv[0] && !b.mv[1])) {
      MvN c = nb(0, 4, -1, 0);
      if (!c.avail) c = nb(0, -1, -1, 0);
      pred(a, b, c, 0, 0, 4, 4, 0, p);
    }
    set(mv, rf, 0, 0, 0, 4, 4, 0, p);
  }
  // B direct, spatial (8.4.1.2.2), for the 8x8 blocks of mask (its
  // neighbours are those of the macroblock, outside it)
  static int minpos(int a, int b) { return a >= 0 && b >= 0 ? Min(a, b) : Max(a, b); }
  void direct(int mask, int16_t (*mv)[16][2], int8_t (*rf)[4]) {
    if (!sc->h_->direct_spatial_mv_pred_flag) {
      temporal(mask, mv, rf);
      return;
    }
    int saved = parts, ref[2], v[2][2] = {{0, 0}, {0, 0}};
    parts = 0;
    MvN n3[2][3];
    for (int l = 0; l < 2; l++) {
      MvN &a = n3[l][0], &b = n3[l][1], &c = n3[l][2];
      a = nb(l, -1, 0, 0), b = nb(l, 0, -1, 0), c = nb(l, 4, -1, 0);
      if (!c.avail) c = nb(l, -1, -1, 0);
      ref[l] = minpos(a.ref, minpos(b.ref, c.ref));
    }
    bool zero = ref[0] < 0 && ref[1] < 0;
    if (zero) ref[0] = ref[1] = 0;
    // (the macroblock's prediction, from the neighbours just found)
    for (int l = 0; l < 2; l++)
      if (!zero && ref[l] >= 0) pred(n3[l][0], n3[l][1], n3[l][2], 0, 0, 4, 4, ref[l], v[l]);
    bool lt = rec->col_long_term();
    // (with direct_8x8_inference the four 4x4 blocks of an 8x8 one have the
    // same colocated block: the 8x8 block at once)
    int n = rec->direct8x8 ? 2 : 1;
    // colZeroFlag, by 4x4 block (bit y * 4 + x): the colocated block is
    // still, which zeroes the vector of a list on reference 0
    int still = 0;
    if (!zero && !lt && (ref[0] == 0 || ref[1] == 0))
      for (int b8 = 0; b8 < 4; b8++)
        if (mask >> b8 & 1)
          for (int k = 0; k < 4; k += 3 * n - 2) {
            int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1), cr, cm[2];
            int64_t pid;
            if (rec->colocated(addr, y * 4 + x, cr, cm, pid) && cr == 0 && abs(cm[0]) <= 1 && abs(cm[1]) <= 1)
              still |= (n == 2 ? 0x33 : 1) << (y * 4 + x);
          }
    for (int l = 0; l < 2; l++) {
      if (ref[l] < 0) continue;
      int z[2] = {0, 0}, s = ref[l] == 0 ? still : 0;
      if (mask == 15 && (s == 0 || s == 0xFFFF)) {
        set(mv, rf, l, 0, 0, 4, 4, ref[l], s ? z : v[l]);
        continue;
      }
      for (int b8 = 0; b8 < 4; b8++)
        if (mask >> b8 & 1)
          for (int k = 0; k < 4; k += 3 * n - 2) {
            int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1);
            set(mv, rf, l, x, y, n, n, ref[l], s >> (y * 4 + x) & 1 ? z : v[l]);
          }
    }
    parts = saved;
  }
  // temporal direct (8.4.1.2.3)
  void temporal(int mask, int16_t (*mv)[16][2], int8_t (*rf)[4]) const {
    for (int b8 = 0; b8 < 4; b8++)
      if (mask >> b8 & 1)
        for (int k = 0; k < 4; k++) {
          int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1), cr, cm[2] = {0, 0}, r0 = 0;
          int64_t pid;
          if (rec->colocated(addr, y * 4 + x, cr, cm, pid))
            r0 = rec->map_col(pid);
          else
            cm[0] = cm[1] = 0;
          int dsf = rec->dist_scale(r0);
          int m0[2] = {(dsf * cm[0] + 128) >> 8, (dsf * cm[1] + 128) >> 8};
          int m1[2] = {m0[0] - cm[0], m0[1] - cm[1]};
          set(mv, rf, 0, x, y, 1, 1, r0, m0);
          set(mv, rf, 1, x, y, 1, 1, 0, m1);
        }
  }
};

template <int f_DEC, class LV>
struct Model : tc_rc<f_DEC> {
  static constexpr int ENC = f_DEC == 0;
  using tc_rc<f_DEC>::tc_stage;
  using tc_rc<f_DEC>::tc_syms;

  // The model of a run: Init() as its coroutine is made (on pages of its
  // own, zeroed: what Init() does not set is 0), start() as the run
  // starts (its tables mapped), and Quit() as it ends, or wherever the
  // coroutine was left (the tables unmapped, the memory freed)
  void Init() {
    tc_rc<f_DEC>::Init();
#define H_INIT(F) f_##F.Init(), t_##F = nullptr;
    H264P_FAMS(H_INIT)
#undef H_INIT
    run_trap = nullptr;
    memset(last_, 0, sizeof last_);
    raw_.Init();
    hdr_.Init();
    sc_ = nullptr;
    mbs_ = nullptr;
    info_.Init();
    hist_.Init();
    pic_ = 1;
    last_h_.Init();
    st_ = key_ = 0;
    width_ = 1, size_ = 0;
    mbaff_ = false;
    qp_off_ = 0, qp_pred_ = slice_qp_ = 26;
    memset(lq_, 0, sizeof lq_);
    last_qpd_ = lz_ = 0;
    qp_src_ = -1;
    has_lz_ = qpd_coded_ = false;
    run_ = cnt_ = prev_cnt_ = 0;
    memset(slice_cnt_, 0, sizeof slice_cnt_);
    mb_open_ = false;
    addr_ = 0;
    mi_ = nullptr;
    col_ = nullptr, col_skip_ = 0, col1_ = nullptr;
    sm_ = tm_ = tb_ = dh_ = bd_ = tx_ = 0;
    tmb_[0] = tmb_[1] = 0;
    tmb_ok_ = smv_ok_ = false;
    mbt_d_ = nullptr;
    for (int& v : sub_) v = 0;
    pmv_[0] = pmv_[1] = 0;
    pmv_ok_ = false;
    pxq_ = 0;
    prx_v_[0] = prx_v_[1] = 0;
    prx_blk_ = 0;
    prx_chroma_ = false;
    pxv_.Init();
    mph_ = 1, prev_mode_ = 9;
    for (int k = 0; k < 2; k++) mvp_[k] = sp_[k] = pdv_[k] = ns_[k] = tmv_[k] = 0;
    pdn_ = false;
    tmn_ = true;
    tmq_ = 0;
    talt_[0] = INT_MIN, talt_[1] = 0;
    mvx_ = 0;
    cnth_ = 0;
    shx_ = shy_ = shd_ = 0;
    rec_.Init();
    rec_.deblock = LV::deblock;
    mvpred_.Init();
    aq_.Init();
    mbx_ = mby_ = height_ = 0;
    prev_poc_ = cur_poc_ = INT_MIN;
    phk_ = -1;
  }
  void Quit() {
#define H_QUIT(F)                                                                    \
  if (t_##F) LV::F::quit(t_##F), tc_unmap(t_##F, sizeof(typename LV::F::T)), t_##F = nullptr; \
  f_##F.Quit();
    H264P_FAMS(H_QUIT)
#undef H_QUIT
    raw_.Quit();
    hdr_.Quit();
    info_.Quit();
    hist_.Quit();
    pxv_.Quit();
    rec_.Quit();
    aq_.Quit();
  }

  // each family's coder (f_nal ...) and tables (t_nal ...)
#define H_FAM(F)                          \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;
  H264P_FAMS(H_FAM)
#undef H_FAM
  // whether the level's family F reads its variable f: what it reads not is
  // not computed (where computing it has no other use)
#ifdef H264P_DUMP
#define H_USES(F, f) 1  // (the dump has every variable, for any index famtune tries)
#else
#define H_USES(F, f) ((LV::F::Uses >> (offsetof(H_##F##_v, f) / sizeof(int))) & 1)
#endif
  // (a level reads no variable its parts cannot make: famvars.inc)
#define H_PARTS(F)                                                                                          \
  static_assert(LV::recon || !(LV::F::Uses & H_##F##_needs<NEED_REC>()),                                    \
                "a level without the decoder reads a variable of " #F " that needs it");                    \
  static_assert(LV::recon || LV::refs || !(LV::F::Uses & H_##F##_needs<NEED_REFS>()),                       \
                "a level without the reference lists reads a variable of " #F " that needs them");          \
  static_assert(LV::search || !(LV::F::Uses & H_##F##_needs<NEED_SEARCH>()),                                \
                "a level without the template searches reads a variable of " #F " that needs them");        \
  static_assert(LV::proxy || !(LV::F::Uses & H_##F##_needs<NEED_PROXY>()),                                  \
                "a level without the intra proxy reads a variable of " #F " that needs it");                \
  static_assert(LV::signpred || !(LV::F::Uses & H_##F##_needs<NEED_SIGNP>()),                               \
                "a level without sign prediction reads a variable of " #F " that needs it");                \
  static_assert(LV::skippx || !(LV::F::Uses & H_##F##_needs<NEED_SKIPPX>()),                                \
                "a level without the skip prediction's pixels reads a variable of " #F " that needs them");  \
  static_assert(LV::mvp || !(LV::F::Uses & H_##F##_needs<NEED_MVP>()),                                      \
                "a level without the motion vectors reads a variable of " #F " that needs them");
  H264P_FAMS(H_PARTS)
#undef H_PARTS
  static_assert(LV::mvp || !(LV::refs || LV::recon), "the reference lists and the decoder need the motion vectors");

  void start() {
    // (end_of_slice_flag 0 nearly for sure: as if seen a few times)
    for (cm_cnt& c : end_c_) c.p = (u16)(CM_CONE - 16), c.t = 8;
#define H_MAP(F) t_##F = tc_table<typename LV::F::T>(), LV::F::init(t_##F), f_##F.wire(t_##F, this);
    H264P_FAMS(H_MAP)
#undef H_MAP
    raw_.start();
  }
  u64 table_bytes() const {
    u64 n = 0;
#define H_BYTES(F) n += LV::F::bytes(t_##F);
    H264P_FAMS(H_BYTES)
#undef H_BYTES
    return n;
  }
  ErrTrap* run_trap;  // the trap of the run (coro.inc): kRestart goes there

  // (a build with H264P_DUMP: a compression that starts over, kRestart,
  // drops what it dumped)
  void dump_discard() {
#define H_DISCARD(F) f_##F.dump_discard();
    H264P_FAMS(H_DISCARD)
#undef H_DISCARD
  }

  // ------------------------------------------------------------------------
  // the container: a field of the nal family, with its own last two values
  // and a context c the caller gives
  i64 last_[F_N][2];
  i64 nal(int fld, int c, i64 x) {
    tc_stage = STG_NAL;
    H_nal_v D;
    D.fld = fld;
    D.ctx = c;
    D.p1 = tc_qlog(last_[fld][0]);
    D.p2 = tc_qlog(last_[fld][1]);
    tcx X;
    f_nal.make(D, X);
    x = f_nal.code(X, ENC ? x : 0);
    last_[fld][1] = last_[fld][0];
    last_[fld][0] = x;
    tc_syms[STG_NAL]++;
    return x;
  }

  // bytes nothing else models (RawModel)
  RawModel<f_DEC> raw_;
  u32 raw(u32 b) {
    tc_stage = STG_RAW;
    return raw_.code(this, b);
  }

  // ------------------------------------------------------------------------
  // header syntax (HeaderModel), through the hdr family
  HeaderModel hdr_;
  void hbits(int n) { hdr_.hbits = n; }
  void nal_begin(int type, int ref_idc) { hdr_.nal_begin(type, ref_idc); }
  void hbegin(const char* name) { hdr_.begin(name); }
  i64 hval(i64 x) {
    tc_stage = STG_HDR;
    x = hdr_.code(f_hdr, ENC, x);
    tc_syms[STG_HDR]++;
    return x;
  }
  bool hmore(const char* next, bool m) {
    char name[96];
    snprintf(name, sizeof name, "+%s", next);
    hbegin(name);
    return hval(m) != 0;
  }

  // ------------------------------------------------------------------------
  // slice data
  // (the walk writing: decoding, its bins coded; coding, not, only the
  // contexts' states following them where the level reads them, cab0:
  // ModelChan::policy())
  typedef WalkState SC;
  SC* sc_;
  const MbState* mbs_;  // the walk's macroblock states, for addresses
  Buf<MbInfo> info_;
  History hist_;
  typedef History::Ref MbRef;
  typedef History::View HistView;
  uint32_t pic_;       // pictures so far
  SliceHeader last_h_;     // the last slice coded (no sps: none yet)
  int st_, key_;   // slice type x reference; st_ x frame/top/bottom field (pic_kind)
  int width_, size_;
  bool mbaff_;
  int qp_off_, qp_pred_, slice_qp_;
  // 1 + the QP of the picture's last macroblock of each kind (inter,
  // I_NxN, other) and energy class (qpd's xe), 0 none
  int lq_[3][19];
  int last_qpd_, lz_;  // the last mb_qp_delta, the last nonzero one
  int qp_src_;            // the macroblock that coded it
  bool has_lz_, qpd_coded_;
  int run_, cnt_, prev_cnt_;  // skipped before, macroblocks in the slice so far, in the last one
  int slice_cnt_[30];
  bool mb_open_;
  int addr_;
  MbInfo* mi_;
  // What the coding of one family leaves for others', each made where its
  // comment says and reset by begin_mb() unless it says otherwise: the
  // walk's order decides what is known when.
  // - the history's records (History): the macroblock's own, col_, and in
  //   a B slice the same macroblock's in list 1's first reference (col1_);
  //   col_'s skip as skp_vars() has it (col_skip_), for end(), which comes
  //   after finish_mb() rewrote col_'s record (col_ is null from there)
  const MbCol* col_;
  int col_skip_;
  const MbCol* col1_;
  // - skip_miss(), as the skp family codes mb_skip_flag: how well the skip
  //   prediction continues the samples around (sm_), its template matching
  //   miss (tm_), the best template match on list 0 (tb_; its vector tmb_),
  //   the better list of the skip motion (dh_; B), how far apart its two
  //   predictions are (bd_; B), its texture's class (tx_); and the skip
  //   motion itself (smv_, srf_: smv_ok_), which skip() takes
  int sm_, tm_, tb_, dh_, bd_, tx_;
  int tmb_[2];
  bool tmb_ok_;
  int16_t smv_[2][16][2];
  int8_t srf_[2][4];
  bool smv_ok_;
  // - mb_type: its contexts while it is coded (mbt_intra's), null after
  H_mbt_v* mbt_d_;
  // - sub(): the sub-macroblock types (set as coded)
  int sub_[4];
  // - proxy_mv(), made where a family first asks for it in the macroblock
  //   (pmv_ok_): where the neighbours' motion puts the macroblock in the
  //   reference (pmv_, whole samples) and how well its vector matches the
  //   samples around (pxq_: 1 + tc_qlog, 0 not known); the proxy's vector
  //   (prx_v_, quarter samples), as block() searches it again: for the luma
  //   block prx_blk_ last, and for the chroma (prx_chroma_); per picture,
  //   the vectors of its macroblocks' proxies (pxv_; INT16_MIN none)
  int pmv_[2];
  bool pmv_ok_;
  int pxq_;
  int prx_v_[2];
  int prx_blk_;
  bool prx_chroma_;
  Buf<int16_t> pxv_;
  // - ipm(): the macroblock's most-probable-mode flags so far, after a 1
  //   (mph_; made from its first block on), the last mode (prev_mode_)
  int mph_, prev_mode_;
  // - mvd(), the partition's prediction (mvd_pred, at its horizontal
  //   component; the vertical's reads it): mvp, the spread of the
  //   neighbours around it, col's vector less mvp (pdv_; pdn_: none), the
  //   signs of the neighbours' vectors less mvp, summed (ns_); the vector
  //   by template matching (tmv_; its miss tmq_; tmn_: none) and the
  //   runner-up's start (talt_; INT_MIN none); the horizontal mvd (mvx_)
  int mvp_[2], sp_[2], pdv_[2], ns_[2];
  bool pdn_;
  int tmv_[2];
  bool tmn_;
  int tmq_;
  int talt_[2];
  int mvx_;
  // - block(): the counts of the last two blocks, two bits each (cnth_,
  //   from one macroblock to the next); the shift fitted to the
  //   macroblock's last inter luma block ((shx_, shy_) / shd_)
  int cnth_;
  i64 shx_, shy_, shd_;

  // the pictures, reconstructed: by the decoder; their motion and the
  // reference lists alone (a level with refs but without the decoder); or
  // as never known
  typedef ReconT<LV::recon ? 2 : LV::refs ? 1 : 0> Rec;
  Rec rec_;
  // the motion vectors, as the decoder has them
  MotionPred<Rec> mvpred_;
  typedef typename MotionPred<Rec>::MvN MvN;
  // the decoder's parts the level has: its template searches, the intra
  // proxy (where it has them not, constants "not known")
  template <class... A>
  bool tm_search_(A&&... a) const {
    return LV::search && rec_.tm_search(static_cast<A&&>(a)...);
  }
  template <class... A>
  bool proxy_build_(A&&... a) {
    return LV::proxy && rec_.proxy_build(static_cast<A&&>(a)...);
  }
#define H_PROXY(F, R, NONE)                                  \
  template <class... A>                                      \
  R F##_(A&&... a) const {                                   \
    return LV::proxy ? rec_.F(static_cast<A&&>(a)...) : NONE; \
  }
  H_PROXY(proxy_nz, int, -1)
  H_PROXY(proxy_mode, int, -1)
  H_PROXY(proxy_i16mode, int, -1)
  H_PROXY(proxy_cmode, int, -1)
  H_PROXY(proxy_levels, bool, false)
  H_PROXY(proxy_dc, bool, false)
  H_PROXY(proxy_energy, int64_t, -1)
  H_PROXY(proxy_acc, int, -1)
#undef H_PROXY
  // x264's AQ
  AqModel aq_;
  int idx(const MbState* n) const { return (int)(n - mbs_); }
  // CABAC's probability of a bin in the walk's current state, as the outside
  // prediction of one of the model's decisions: of the bin being 0 (cab0) or
  // 1 (cab1). -1 without CABAC.
  int cab0(int ctx) const {
    if (!LV::cabpred || !sc_->cabac_) return -1;
    const CabacCtx& c = sc_->ctx_[ctx];
    return c.mps() ? cab_lps[c.state()] : CM_PONE - cab_lps[c.state()];
  }
  int cab1(int ctx) const {
    int p = cab0(ctx);
    return p < 0 ? -1 : CM_PONE - p;
  }
  const MbInfo* info(const MbState* n) const { return n ? &info_[idx(n)] : nullptr; }
  int cls_of(const MbState* n) const { return n ? info_[idx(n)].cls : (int)CL_NONE; }
  int q6() const { return mi_->qp / 6; }

  // the macroblock dx, dy macroblocks away (in MBAFF frames, rows of
  // macroblocks, not of pairs); -1 outside the picture
  int mb_near(int dx, int dy) const {
    int x = mbx_ + dx, y = mby_ + dy;
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return -1;
    return mbaff_ ? ((y >> 1) * width_ + x) * 2 + (y & 1) : y * width_ + x;
  }
  int mbx_, mby_, height_;  // the macroblock's place (begin_mb), the picture's height (in macroblocks)
  // the POCs of the current slice's kind's last picture and its picture
  // (History::prev, poc)
  int prev_poc_, cur_poc_;
  // in a B slice, the kind (not the current one) whose history is the
  // picture list 1's first reference is; -1 other slices or none (as the
  // slice begins)
  int phk_;
  int phist_key_of_slice() const {
    if (sc_->stype_ != kB || !rec_.motion() || rec_.nlist[1] < 1 || !rec_.list[1][0]) return -1;
    return hist_.of_poc(rec_.list[1][0]->poc, key_, size_);
  }
  // that history, nullptr none
  const MbCol* phist() const { return phk_ < 0 ? nullptr : hist_.rec[phk_].data(); }
  // the macroblock dx, dy away (mb_near()), of the current picture and
  // coded already; nullptr other
  const MbInfo* near_info(int dx, int dy) const {
    int a = mb_near(dx, dy);
    return a >= 0 && info_[a].pic == pic_ ? &info_[a] : nullptr;
  }

  // whether slice h begins a picture (7.4.1.2.4): what tells pictures
  // apart differs from the last slice coded
  bool new_picture(const SliceHeader& h) const {
    const SliceHeader& p = last_h_;
    if (!p.sps || h.pps->id != p.pps->id || h.frame_num != p.frame_num ||
        h.field_pic_flag != p.field_pic_flag || h.bottom_field_flag != p.bottom_field_flag ||
        !h.nal_ref_idc != !p.nal_ref_idc || (h.nal_unit_type == 5) != (p.nal_unit_type == 5))
      return true;
    if (h.sps->poc_type == 0 && (h.poc_lsb != p.poc_lsb || h.delta_poc_bottom != p.delta_poc_bottom)) return true;
    if (h.sps->poc_type == 1 && (h.delta_poc[0] != p.delta_poc[0] || h.delta_poc[1] != p.delta_poc[1])) return true;
    return h.nal_unit_type == 5 && h.idr_pic_id != p.idr_pic_id;
  }

  void slice_begin(SC& sc) {
    sc_ = &sc;
    mbs_ = sc.store_->mbs.data();
    const SliceHeader& h = *sc.h_;
    bool newpic = new_picture(h);
    last_h_ = h;
    key_ = pic_kind(h);
    st_ = key_ / 3;
    if (newpic) pic_++;
    width_ = sc.width_;
    size_ = sc.size_;
    mbaff_ = sc.mbaff_;
    if ((int)info_.size() < size_) info_.resize(size_);
    mvpred_.slice(sc, info_.data(), rec_);
    // (the macroblocks as they were for the P slices' mc_col, mc_nz only,
    // which need the reference lists: a level without them reads none)
    hist_.slice(key_, size_, newpic, (LV::refs || LV::recon) && (h.type() == kP || h.type() == kSP));
    qp_off_ = 6 * (sc.sps_->bit_depth_luma - 8);
    qp_pred_ = slice_qp_ = sc.pps_->pic_init_qp + h.slice_qp_delta;
    last_qpd_ = 0;
    has_lz_ = false;
    qp_src_ = -1;
    run_ = cnt_ = 0;
    prev_cnt_ = slice_cnt_[key_];
    mb_open_ = false;
    rec_.slice_begin(h, newpic);
    // (the first slice of its kind in the picture: the kind's history is
    // the picture's from here on)
    hist_.picture(key_, pic_, rec_.motion() ? rec_.cur->poc : INT_MIN);
    prev_poc_ = hist_.prev[key_];
    cur_poc_ = hist_.poc[key_];
    phk_ = phist_key_of_slice();
    if (newpic) {
      aq_.picture(key_, size_);
      pxv_.fill(size_ * 2, INT16_MIN);
      memset(lq_, 0, sizeof lq_);
    }
  }
  void slice_end() { slice_cnt_[key_] = cnt_; }
  // the end of the stream
  void end_stream() { rec_.end(); }

  void begin_mb() {
    addr_ = sc_->cur_addr_;
    if (!mbaff_) {
      mbx_ = addr_ % width_;
      mby_ = addr_ / width_;
    } else {
      int p = addr_ >> 1;
      mbx_ = p % width_;
      mby_ = (p / width_) * 2 + (addr_ & 1);
    }
    height_ = size_ / width_;
    mi_ = &info_[addr_];
    // (the next macroblock's record, on its way into the cache: the part
    // cleared below)
    if (addr_ + 1 < size_)
      for (size_t k = 0; k < offsetof(MbInfo, coef); k += 64) CM_PREFETCHW(reinterpret_cast<const char*>(mi_ + 1) + k);
    // (and the coefficients with cmask: the rows the last macroblock here
    // wrote stay until rewritten, unread)
    memset(mi_, 0, offsetof(MbInfo, coef));
    mi_->pic = pic_;
    mi_->qp = (uint8_t)(qp_pred_ + qp_off_);
    memset(mi_->modes, 2, sizeof mi_->modes);
    memset(mi_->rf, -1, sizeof mi_->rf);
    memset(mi_->tme, -128, sizeof mi_->tme);
    col_ = hist_.cur[addr_].valid ? &hist_.cur[addr_] : nullptr;
    col_skip_ = col_ ? 2 - col_->skip : 0;
    // B: the same macroblock in list 1's first reference too
    col1_ = nullptr;
    if (const MbCol* p = phist()) {
      col1_ = p[addr_].valid ? &p[addr_] : nullptr;
      if (addr_ + 1 < size_) CM_PREFETCH(&p[addr_ + 1]);  // (the next macroblock's, on its way into the cache)
    }
    mvpred_.begin(addr_, mi_);
    rec_.mb_start(addr_);
    qpd_coded_ = false;
    prev_mode_ = 9;
    sm_ = tm_ = tb_ = dh_ = bd_ = tx_ = 0;
    tmb_ok_ = false;
    pmv_ok_ = false;
    smv_ok_ = false;
    shx_ = shy_ = shd_ = 0;
    prx_chroma_ = false;
    prx_blk_ = 0;
    mb_open_ = true;
    cnt_++;
  }
  // into the history of the slice's kind
  void finish_mb() {
    const MbState& s = *sc_->cur_;
    if (LV::refs || LV::recon) hist_.before_write(key_, addr_);
    MbCol& c = hist_.cur[addr_];
    c.valid = 1;
    hist_.skip[key_][addr_] = mi_->skip != 0;
    c.skip = mi_->skip;
    c.cls = mi_->cls;
    c.qp = mi_->qp;
    c.cbp = mi_->cbp;
    c.t8 = mi_->t8;
    c.cpred = mi_->cpred;
    c.field = s.field;
    memcpy(c.modes, mi_->modes, sizeof c.modes);
    memcpy(c.ref, mi_->ref, sizeof c.ref);
    if (LV::mvp) {
      memcpy(c.rf, mi_->rf, sizeof c.rf);
      memcpy(c.mv, mi_->mv, sizeof c.mv);
    }
    if (LV::refs || LV::recon)
      for (int l = 0; l < 2; l++)
        for (int b8 = 0; b8 < 4; b8++) {
          int r = mi_->rf[l][b8];
          c.rpoc[l][b8] = rec_.motion() && r >= 0 && r < rec_.nlist[l] && rec_.list[l][r] ? rec_.list[l][r]->poc : INT_MIN;
        }
    static const decltype(mi_->mvd) kNoMvd = {};
    if (!memcmp(mi_->mvd, kNoMvd, sizeof kNoMvd)) {
      memset(c.mvq, 0, sizeof c.mvq);  // (a skipped macroblock: tc_qlog 0 throughout)
    } else {
      for (int l = 0; l < 2; l++)
        for (int b = 0; b < 16; b++)
          for (int k = 0; k < 2; k++) c.mvq[l][b][k] = (uint8_t)tc_qlog(mi_->mvd[l][b][k]);
    }
    for (int p = 0; p < 3; p++) {
#ifdef H264P_AVX2
      __m128i z = _mm_cmpeq_epi8(_mm_loadu_si128((const __m128i*)s.nz[p]), _mm_setzero_si128());
      c.nzm[p] = (uint16_t)~_mm_movemask_epi8(z);
#else
      c.nzm[p] = 0;
      for (int b = 0; b < 16; b++) c.nzm[p] |= (uint16_t)((s.nz[p][b] != 0) << b);
#endif
    }
    c.dcm = (uint8_t)(s.cbf_dc[0] | s.cbf_dc[1] << 1 | s.cbf_dc[2] << 2);
    // (the rows either wrote: the rest are zero in both)
    for (uint32_t m = c.cm | (uint32_t)(mi_->cmask & 0xFFFF); m; m &= m - 1) {
      int r = __builtin_ctz(m);
      if (mi_->cmask >> r & 1)
        memcpy(c.coef[r], mi_->coef[0][r], 16);
      else
        memset(c.coef[r], 0, 16);
    }
    c.cm = (uint16_t)mi_->cmask;
    col_ = nullptr;  // (that record is the macroblock's now)
    mb_open_ = false;
    rec_.mb_end(addr_, mi_->cls, mi_->i16m, mi_->modes, mi_->cpred, mi_->t8, mi_->qp - qp_off_, mi_->rf, mi_->mv, s.nz[0]);
    if (rec_.act() && (int)aq_.x.size() == size_) aq_.decoded(addr_, lg16(rec_.ac_energy(addr_)), mi_->qp - qp_off_, qpd_coded_);
  }

  // --- skip, end of slice, field decoding flag: the skp family
  // the ones in a map of 0 or 1 a macroblock (History::skip) in the 3x3 window
  // around the macroblock, by mb_near (none outside the picture)
  int win9(const uint8_t* m) const {
    int n = 0;
    if (mbaff_) {
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          int a = mb_near(dx, dy);
          n += a >= 0 && m[a];
        }
      return n;
    }
    const uint8_t* r = m + addr_;
    if (mbx_ > 0 && mbx_ + 1 < width_ && mby_ > 0 && mby_ + 1 < height_) {
      const uint8_t *a = r - width_, *b = r + width_;
      return a[-1] + a[0] + a[1] + r[-1] + r[0] + r[1] + b[-1] + b[0] + b[1];
    }
    int x0 = mbx_ > 0 ? -1 : 0, x1 = mbx_ + 1 < width_ ? 1 : 0;
    int y0 = mby_ > 0 ? -1 : 0, y1 = mby_ + 1 < height_ ? 1 : 0;
    for (int dy = y0; dy <= y1; dy++)
      for (int dx = x0; dx <= x1; dx++) n += r[dy * width_ + dx];
    return n;
  }
  void skp_vars(H_skp_v& D) const {
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    D.st = st_;
    D.a3 = a ? 2 - a->skip : 0;
    D.b3 = b ? 2 - b->skip : 0;
    D.col = col_skip_;  // (end() comes after finish_mb() rewrote col_'s record)
    D.c9 = win9(hist_.skip[key_].data());
    D.c1 = col1_ ? 2 - col1_->skip : 0;
    // and the skipped ones around it there (10 none)
    D.k9 = phk_ >= 0 ? win9(hist_.skip[phk_].data()) : 10;
    const MbInfo* c = near_info(1, -1);
    const MbInfo* d = near_info(-1, -1);
    D.cd = (c && c->skip) + (d && d->skip);
    D.run = run_;
    D.cnt = tc_qlog(cnt_);
    D.eq = !prev_cnt_ || cnt_ > prev_cnt_ ? 2 : cnt_ == prev_cnt_;
    D.lf = mbaff_ && sc_->left_ ? 1 + sc_->left_->field : 0;
    D.af = mbaff_ && sc_->above_ ? 1 + sc_->above_->field : 0;
    D.td = map_td();
    D.en = map_en();
    D.sm = D.tm = 0;
    D.qc = tc_clamp(qp_pred_ / 3, 0, 17);
  }
  // the best vector for the macroblock by template matching on list l's
  // first reference (from mvp, the neighbours' vectors, zero; in whole
  // samples): false none; miss as Recon::tm_search's
  bool tm_best(int l, int* mv, int& miss) const {
    MvN nbs[3];
    int mp[2];
    mvpred_.pred(l, 0, 0, 4, 4, 0, mp, nbs);
    return tm_best(l, nbs, mp, mv, miss);
  }
  // how well the prediction of the macroblock were it skipped continues
  // the samples above and to the left (1 + tc_qlog of Recon::skip_miss, 0
  // not known)
  int skip_miss() {
    tm_ = tb_ = dh_ = bd_ = tx_ = 0;
    if (!rec_.act() || !LV::skippx) return 0;
    // how well the reference can predict the samples around at best (list
    // 0)
    int m0;
    tmb_ok_ = tm_best(0, tmb_, m0);
    if (tmb_ok_) tb_ = 1 + tc_clamp(tc_qlog(m0), 0, 15);
    // (kept for skip(), should the macroblock be skipped; from no motion,
    // as a macroblock's record begins, the lists direct prediction leaves
    // unused)
    memset(smv_, 0, sizeof smv_);
    memset(srf_, -1, sizeof srf_);
    if (sc_->stype_ == kB)
      mvpred_.direct(15, smv_, srf_);
    else
      mvpred_.pskip(smv_, srf_);
    smv_ok_ = true;
    int per[2];
    int tm = rec_.tm_miss(srf_, smv_, per);
    tm_ = qlog1(tm, 15);
    // B: which list's prediction of the skip motion does that better, and
    // whether their average does better still
    if (sc_->stype_ == kB && per[0] >= 0 && per[1] >= 0) {
      dh_ = per[0] * 4 < per[1] * 3 ? 1 : per[1] * 4 < per[0] * 3 ? 2 : 3;
      int mn = Min(per[0], per[1]);
      dh_ += 3 * (tm * 4 < mn * 3 ? 1 : tm > mn ? 2 : 0);
    }
    // B: how far apart the two predictions of the skip motion are; P: how
    // far the skip vector is from the best template match's
    int bd = -1;
    if (sc_->stype_ == kB)
      bd = rec_.bi_diff(srf_, smv_);
    else if (tmb_ok_)
      bd = abs(tmb_[0] - smv_[0][0][0]) + abs(tmb_[1] - smv_[0][0][1]);
    bd_ = qlog1(bd, 15);
    int ms = rec_.skip_miss(srf_, smv_);
    // the texture of the skip prediction, per sample, against the step:
    // flat, its residual is likelier to quantize away
    {
      int t = ms < 0 ? -1 : rec_.mb_texture();
      tx_ = qlog1(t, 14, 16, qstep16(qp_pred_));
    }
    return qlog1(ms, 15);
  }
  // the last picture decoded at the current macroblock: how much it changed
  // (1 + tc_qlog, 0 not known), its energy (log2 classes, 0 not known)
  int map_td() const { return maps_ok() && rec_.maps.change[addr_] >= 0 ? 1 + tc_qlog(rec_.maps.change[addr_]) : 0; }
  int map_en() const { return maps_ok() ? 1 + tc_clamp(rec_.maps.energy[addr_] / 4 - 4, 0, 13) : 0; }
  // the maps of the last picture decoded fit the current one
  bool maps_ok() const { return rec_.maps.w == width_ && rec_.maps.w * rec_.maps.h == size_ && !mbaff_; }
  int skp_bit(const H_skp_v& D, int node, int v, int pr = -1) {
    tcx X;
    f_skp.make(D, X);
    f_skp.select(X);
    tc_syms[STG_SKIP]++;
    f_skp.prp = pr;
    return f_skp.bit(node, v);
  }
  int skip(int v) {
    begin_mb();
    tc_stage = STG_SKIP;
    H_skp_v D = {};
    skp_vars(D);
    D.sm = sm_ = skip_miss();
    D.tx = tx_;
    D.tm = tm_;
    D.tb = tb_;
    D.bd = bd_;
    v = skp_bit(D, 0, ENC && v, cab0(sc_->ctx_skip()));
    if (v) {
      mi_->skip = 1;
      mi_->cls = CL_SKIP;
      if (smv_ok_) {
        memcpy(mi_->mv, smv_, sizeof smv_);
        memcpy(mi_->rf, srf_, sizeof srf_);
      } else if (!LV::mvp) {
        // (a level without the motion keeps none)
      } else if (sc_->stype_ == kB) {
        mvpred_.direct(15, mi_->mv, mi_->rf);
      } else {
        mvpred_.pskip(mi_->mv, mi_->rf);
      }
      last_qpd_ = 0;
      run_++;
      finish_mb();
    }
    return v;
  }
  // end_of_slice_flag. After a slice of the kind that was the whole
  // picture, 0 nearly for sure: an adaptive counter of its own by the kind,
  // not the skip family's decision (which cost as much as the skip flag's);
  // else that decision, which knows where slices end
  cm_cnt end_c_[10];
  int end(int v, bool certain) {
    if (certain) return 1;  // nothing follows in the picture
    tc_stage = STG_SKIP;
    if (prev_cnt_ == size_) {
      tc_syms[STG_SKIP]++;
      cm_cnt& c = end_c_[st_];
      v = tc_bit(this, c.P(), ENC && v);
      c.upd(v, 1023, 0);
      return v;
    }
    H_skp_v D = {};
    skp_vars(D);
    return skp_bit(D, 1, ENC && v);
  }
  int field(int v) {
    tc_stage = STG_SKIP;
    H_skp_v D = {};
    skp_vars(D);
    D.col = col_ ? 1 + col_->field : 0;
    return skp_bit(D, 2, ENC && v, cab0(sc_->ctx_field()));
  }
  void mb_begin() {
    if (!mb_open_) begin_mb();
  }
  void mb_end() {
    if (!qpd_coded_) last_qpd_ = 0;
    run_ = 0;
    finish_mb();
  }

  // --- mb_type: the mbt family, as a tree of decisions (intra? partition,
  // prediction lists; for intra I_NxN, I_PCM, Intra16x16's cbp and mode)
  static int class_of(int stype, int t) {
    int it = -1;
    switch (stype) {
      case kI:
        it = t;
        break;
      case kSI:
        if (t == 0) return CL_INXN;
        it = t - 1;
        break;
      case kB:
        if (t < 23) return t == 0 ? CL_DIRECT : t <= 3 ? CL_16x16 : t == 22 ? CL_8x8 : CL_PART;
        it = t - 23;
        break;
      default:
        if (t < 5) return t == 0 ? CL_16x16 : t <= 2 ? CL_PART : CL_8x8;
        it = t - 5;
    }
    return it == 0 ? CL_INXN : it == 25 ? CL_PCM : CL_I16;
  }
  int mbt_bit(int node, int v, int pr = -1) {
    f_mbt.prp = pr;
    return f_mbt.bit(node, v);
  }
  // an intra mb_type in I slice numbering
  int mbt_intra(int it) {
    int st = sc_->stype_;
    bool islice = st == kI || st == kSI;
    int off = islice ? 3 : st == kB ? 32 : 17;
    if (mbt_bit(21, ENC && it == 0, cab1(sc_->ctx_mbt_i0(off, islice)))) return 0;
    if (mbt_bit(22, ENC && it == 25)) return 25;
    if (sc_->chroma_ == 1) {
      // Intra16x16: the mode that does best in the proxy, and by how much
      // it beats the next
      proxy_mv();
      int costs[4];
      int m = proxy_i16mode_(pmv_[0], pmv_[1], costs);
      if (m >= 0) {
        int c2 = INT_MAX;
        for (int k = 0; k < 4; k++)
          if (k != m) c2 = Min(c2, costs[k]);
        mbt_d_->im = 1 + m;
        mbt_d_->ig = c2 == INT_MAX ? 0 : 1 + tc_clamp(tc_qlog((c2 - costs[m]) * 32 / x264_lam16(qp_pred_)), 0, 11);
        tcx X;
        f_mbt.make(*mbt_d_, X);
        f_mbt.select(X);
      }
    }
    int x = ENC ? it - 1 : 0;
    int luma = mbt_bit(23, ENC && x / 12, cab0(SC::ctx_mbt_i(off, islice, 0)));
    int c = x / 4 % 3;
    int chroma = mbt_bit(24, ENC && c != 0, cab0(SC::ctx_mbt_i(off, islice, 1)));
    if (chroma) chroma += mbt_bit(25, ENC && c == 2, cab0(SC::ctx_mbt_i(off, islice, 2)));
    int p1 = mbt_bit(26, ENC && (x & 2), cab0(SC::ctx_mbt_i(off, islice, 3)));
    int p0 = mbt_bit(27 + p1, ENC && (x & 1), cab0(SC::ctx_mbt_i(off, islice, 4)));
    mi_->i16m = (uint8_t)(p1 * 2 + p0);
    return 1 + p1 * 2 + p0 + 4 * chroma + 12 * luma;
  }
  int mb_type(int t) {
    tc_stage = STG_MBT;
    const MbInfo* c = near_info(1, -1);
    const MbInfo* d = near_info(-1, -1);
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    H_mbt_v D = {};
    D.st = st_;
    D.ca = cls_of(a);
    D.cb = cls_of(b);
    D.cc = col_ ? col_->cls : (int)CL_NONE;
    D.c1 = col1_ ? 1 + col1_->cls : 0;
    D.nsk = (a && a->skip) + (b && b->skip) + (c && c->skip) + (d && d->skip);
    D.en = map_en();
    D.td = map_td();
    D.sm = sm_;
    D.tm = tm_;
    D.tb = tb_;
    D.dh = dh_;
    D.bd = bd_;
    D.qc = tc_clamp(qp_pred_ / 3, 0, 17);
    D.tx = tx_;
    tcx X;
    f_mbt.make(D, X);
    f_mbt.select(X);
    mbt_d_ = &D;
    int stype = sc_->stype_;
    f_mbt.prp = -1;
    switch (stype) {
      case kI:
        t = mbt_intra(t);
        break;
      case kSI:
        t = mbt_bit(20, ENC && t == 0) ? 0 : 1 + mbt_intra(t - 1);
        break;
      case kB:
        if (mbt_bit(1, ENC && t >= 23)) {
          t = 23 + mbt_intra(t - 23);
        } else if (mbt_bit(6, ENC && t == 0, cab1(sc_->ctx_mbt_b()))) {
          t = 0;
        } else if (mbt_bit(7, ENC && t <= 3)) {
          t = mbt_bit(8, ENC && t == 1) ? 1 : mbt_bit(9, ENC && t == 3) ? 3 : 2;
        } else if (mbt_bit(10, ENC && t == 22)) {
          t = 22;
        } else {
          // 16x8 or 8x16, and the prediction lists of the two partitions
          int q = ENC ? t - 4 : 0, p0 = ENC ? kBPartPred[q >> 1][0] : 0, p1 = ENC ? kBPartPred[q >> 1][1] : 0;
          int shape = mbt_bit(11, ENC && (q & 1));
          int r0 = mbt_bit(12, ENC && p0 == 1) ? 1 : mbt_bit(13, ENC && p0 == 3) ? 3 : 2;
          int r1 = mbt_bit(12 + 2 * r0, ENC && p1 == 1) ? 1 : mbt_bit(13 + 2 * r0, ENC && p1 == 3) ? 3 : 2;
          int pair = 0;
          while (kBPartPred[pair][0] != r0 || kBPartPred[pair][1] != r1) pair++;
          t = 4 + pair * 2 + shape;
        }
        break;
      default:
        if (mbt_bit(1, ENC && t >= 5, cab0(SC::kCtxMbtP)))
          t = 5 + mbt_intra(t - 5);
        else if (mbt_bit(2, ENC && t == 0))
          t = 0;
        else if (mbt_bit(3, ENC && t >= 3))
          t = mbt_bit(5, ENC && t == 4) ? 4 : 3;
        else
          t = mbt_bit(4, ENC && t == 2) ? 2 : 1;
    }
    mbt_d_ = nullptr;
    mi_->cls = (uint8_t)class_of(stype, t);
    if (stype == kB)
      mvpred_.parts = t <= 3 ? 0 : t == 22 ? 3 : t < 22 ? 1 + (t & 1) : 0;  // (16x8 even, 8x16 odd)
    else if (stype == kP || stype == kSP)
      mvpred_.parts = t < 3 ? t : 3;
    if (LV::mvp && stype == kB && t == 0) mvpred_.direct(15, mi_->mv, mi_->rf);
    tc_syms[STG_MBT]++;
    return t;
  }

  // --- coded_block_pattern and transform_size_8x8_flag: the cbp family
  int cbp_bit(H_cbp_v& D, int node, int v, int pr) {
    tcx X;
    f_cbp.make(D, X);
    f_cbp.select(X);
    f_cbp.prp = pr;
    return f_cbp.bit(node, v);
  }
  // the prediction of the macroblock, once all of it is known
  void prepare() { rec_.prepare(mi_->cls, mi_->i16m, mi_->modes, mi_->cpred, mi_->t8, mi_->rf, mi_->mv); }
  // where the neighbours' motion puts the macroblock in the reference
  // (whole samples), for an intra macroblock's proxy (Recon::proxy_mode,
  // proxy_levels)
  void proxy_mv() {
    if (pmv_ok_) return;
    pmv_ok_ = true;
    pxq_ = 0;
    int p[2] = {0, 0}, miss;
    bool inter = sc_->stype_ != kI && sc_->stype_ != kSI;
    MvN nbs[3];
    int mp[2] = {0, 0};
    if (inter) mvpred_.pred(0, 0, 0, 4, 4, 0, mp, nbs);
    if (tmb_ok_)
      p[0] = tmb_[0], p[1] = tmb_[1];
    else if (inter && !tm_best(0, nbs, mp, p, miss))
      p[0] = mp[0], p[1] = mp[1];
    // and the neighbours' proxies' vectors, zero
    if ((int)pxv_.size() == size_ * 2) {
      int cand[9][2], nc = 0;
      const int nb[3] = {mb_near(-1, 0), mb_near(0, -1), mb_near(1, -1)};
      for (int j = 0; j < 3; j++)
        if (nb[j] >= 0 && info_[nb[j]].pic == pic_ && pxv_[nb[j] * 2] != INT16_MIN)
          cand[nc][0] = pxv_[nb[j] * 2], cand[nc][1] = pxv_[nb[j] * 2 + 1], nc++;
      // the neighbours' vectors on list 0, scaled to its first reference's
      // distance
      if (inter) {
        for (int j = 0; j < 3; j++) {
          int dn = ref_dist(0, nbs[j].ref), dr = ref_dist(0, 0);
          if (nbs[j].ref >= 0 && dn && dr) {
            for (int k = 0; k < 2; k++) cand[nc][k] = mv_scale(nbs[j].mv[k], dr, dn);
            nc++;
          }
        }
      }
      // the temporal vector: temporal direct's (B), col's scaled by the
      // distance it spanned against the first reference's
      int tv[2];
      if (td_mv(0, 0, 1, 1, tv)) {
        cand[nc][0] = tv[0], cand[nc][1] = tv[1], nc++;
      } else if (col_ && col_->cls < CL_INXN && col_->rf[0][0] >= 0) {
        int d0 = ref_dist(0, 0), dr = d0;
        if (prev_poc_ != INT_MIN && col_->rpoc[0][0] != INT_MIN) d0 = poc_dist(prev_poc_, col_->rpoc[0][0]);
        for (int k = 0; k < 2; k++) cand[nc][k] = d0 && dr && d0 != dr ? mv_scale(col_->mv[0][5][k], dr, d0) : col_->mv[0][5][k];
        nc++;
      }
      cand[nc][0] = cand[nc][1] = 0, nc++;
      int miss;
      if (tm_search_(-1, 0, 0, 0, 16, 16, p, false, cand, nc, &miss, 0, 2)) pxq_ = 1 + tc_clamp(tc_qlog(miss), 0, 11);
      pxv_[addr_ * 2] = (int16_t)tc_clamp(p[0], -32767, 32767);
      pxv_[addr_ * 2 + 1] = (int16_t)tc_clamp(p[1], -32767, 32767);
    }
    proxy_build_(p);
    prx_v_[0] = p[0];
    prx_v_[1] = p[1];
    pmv_[0] = (p[0] + 2) >> 2;
    pmv_[1] = (p[1] + 2) >> 2;
  }
  // how well the motion of an inter macroblock predicts the decoded samples
  // around it (1 + tc_qlog of Recon::tm_miss; 0 not known, intra)
  int tm_pred() const {
    if (mi_->cls >= CL_INXN || !rec_.act()) return 0;
    int ms = rec_.tm_miss(mi_->rf, mi_->mv);
    return qlog1(ms, 15);
  }
  int cbp(int v) {
    prepare();
    tc_stage = STG_CBP;
    H_cbp_v D = {};
    D.tp = tm_pred();
    D.cls = mi_->cls;
    D.q6 = q6();
    D.st = st_;
    D.en = map_en();
    D.td = map_td();
    int luma = 0;
    for (int b8 = 0; b8 < 4; b8++) {
      D.inc = sc_->cbp_luma_inc(b8, luma);
      D.pc = popc4(luma);
      D.col = col_ ? 1 + (col_->cbp >> b8 & 1) : 0;
      {
        // or the 4x4 blocks with coefficients under where its vector puts it
        // (3 + where it moves it)
        int nz = mc_nz((b8 & 1) * 2, (b8 >> 1) * 2);
        if (nz >= 0) D.col = 3 + nz;
      }
      int pt = mi_->cls == CL_INXN ? -1 : rec_.pred_texture(0, (b8 & 1) * 8, (b8 >> 1) * 8, 8);
      D.pg = qlog1(pt, 15, 16, qstep16(mi_->qp - qp_off_));
      int ms = mi_->cls == CL_INXN ? -1 : rec_.edge_miss(0, (b8 & 1) * 8, (b8 >> 1) * 8, 8);
      D.em = qlog1(ms, 15);
      // intra: the levels the proxy picture would need in the block's modes
      D.pq = 0;
      if (mi_->cls == CL_INXN) {
        proxy_mv();
        int qp = mi_->qp - qp_off_, nz = 0, bx = (b8 & 1) * 2, by = (b8 >> 1) * 2;
        if (mi_->t8)
          nz = proxy_nz_(b8 & 1, b8 >> 1, 8, mi_->modes[by * 4 + bx], qp, pmv_[0], pmv_[1]);
        else
          for (int k = 0; k < 4 && nz >= 0; k++) {
            int x = bx + (k & 1), y = by + (k >> 1), z = proxy_nz_(x, y, 4, mi_->modes[y * 4 + x], qp, pmv_[0], pmv_[1]);
            nz = z < 0 ? -1 : nz + z;
          }
        D.pq = nz < 0 ? 0 : 1 + Min(nz, 5);
      }
      luma |= cbp_bit(D, b8, ENC && (v >> b8 & 1), cab0(SC::kCtxCbpLuma + D.inc)) << b8;
    }
    int c = 0;
    if (sc_->chroma_ == 1 || sc_->chroma_ == 2) {
      int pt = rec_.pred_texture(1, 0, 0, 8);
      if (pt >= 0) pt += rec_.pred_texture(2, 0, 0, 8);
      D.pg = qlog1(pt, 15);
      int ms = rec_.edge_miss(1, 0, 0, 8);
      if (ms >= 0) ms += Max(0, rec_.edge_miss(2, 0, 0, 8));
      D.em = qlog1(ms, 15);
      D.inc = sc_->cbp_chroma_inc(0);
      D.pc = popc4(luma);
      D.col = col_ ? 1 + (col_->cbp >> 4 != 0) : 0;
      // intra: the chroma levels of the proxy, DC and AC
      int pdc = -1, pac = -1;
      if (mi_->cls >= CL_INXN && sc_->chroma_ == 1) {
        proxy_mv();
        int qp = mi_->qp - qp_off_, lv[16];
        for (int pl = 1; pl <= 2; pl++) {
          int qc = rec_.chroma_qp_of(pl - 1, qp);
          if (!proxy_dc_(pl, qc, lv)) break;
          pdc = Max(pdc, 0);
          pac = Max(pac, 0);
          for (int k = 0; k < 4; k++) pdc += abs(lv[k]) >= 3;
          for (int b = 0; b < 4; b++) {
            proxy_levels_(pl, (b & 1) * 4, (b >> 1) * 4, qc, lv);
            for (int k = 1; k < 16; k++) pac += abs(lv[k]) >= 3;
          }
        }
      }
      D.pq = pdc < 0 ? 0 : 1 + Min(pdc + pac, 5);
      if (cbp_bit(D, 4, ENC && (v >> 4) != 0, cab0(SC::kCtxCbpChroma + D.inc))) {
        D.inc = sc_->cbp_chroma_inc(1);
        D.col = col_ ? 1 + (col_->cbp >> 4 == 2) : 0;
        D.pq = pac < 0 ? 0 : 1 + Min(pac, 5);
        c = 1 + cbp_bit(D, 5, ENC && (v >> 4) == 2, cab0(SC::kCtxCbpChroma + 4 + D.inc));
      }
    }
    tc_syms[STG_CBP]++;
    mi_->cbp = (uint8_t)(luma | c << 4);
    return mi_->cbp;
  }
  int t8x8(int v) {
    tc_stage = STG_CBP;
    H_cbp_v D = {};
    D.inc = sc_->inc_t8x8();
    D.pc = popc4(mi_->cbp & 15);
    D.cls = mi_->cls;
    D.col = col_ ? 1 + col_->t8 : 0;
    D.q6 = q6();
    D.st = st_;
    D.en = map_en();
    D.td = map_td();
    D.pg = 0;
    tc_syms[STG_CBP]++;
    v = cbp_bit(D, 6, ENC && v, cab0(SC::kCtxT8x8 + D.inc));
    mi_->t8 = (uint8_t)v;
    rec_.set_t8(v);  // (an inter macroblock's prediction is made by then, at cbp)
    return v;
  }

  // --- intra prediction modes: the ipm family. The actual Intra4x4/8x8
  // mode is coded, as "the most probable mode" or the rest in three bits.
  // the mode of the 4x4 block of a neighbour for the most probable mode:
  // 0-8, 9 DC by rule, 10 none; dc is set when both count as DC
  int nb_mode(const MbState* n, int i, bool& dc) const {
    if (!n) {
      dc = true;
      return 10;
    }
    const MbInfo& m = info_[idx(n)];
    if (m.cls < CL_INXN && sc_->pps_->constrained_intra_pred_flag) {
      dc = true;
      return 9;
    }
    return m.cls == CL_INXN ? m.modes[i] : 9;
  }
  int ipm(int blk, int v) {
    tc_stage = STG_IPM;
    bool k8 = mi_->t8;
    int x = k8 ? (blk & 1) * 2 : blk_x(blk), y = k8 ? (blk >> 1) * 2 : blk_y(blk);
    int ia, ib;
    const MbState* na = sc_->nb4(x - 1, y, ia);
    const MbState* nb = sc_->nb4(x, y - 1, ib);
    bool dc = false;
    int ma = nb_mode(na, ia, dc), mb = nb_mode(nb, ib, dc);
    int mpm = dc ? 2 : Min(ma > 8 ? 2 : ma, mb > 8 ? 2 : mb);
    H_ipm_v D = {};
    D.k8 = k8;
    D.ma = ma;
    D.mb = mb;
    D.mp = mpm;
    D.blk = blk;
    D.pv = prev_mode_;
    D.col = col_ && col_->cls == CL_INXN ? col_->modes[y * 4 + x] : 9;
    // the mode of the block above right, where decoded already (10 none)
    D.mc = 10;
    {
      int tx = x + (k8 ? 2 : 1), ty = y - 1;
      bool ok = ty < 0 || tx <= 3;
      if (ok && ty >= 0) ok = (k8 ? (ty >> 1) * 2 + (tx >> 1) : (ty >> 1) * 8 + (tx >> 1) * 4 + (ty & 1) * 2 + (tx & 1)) < blk;
      if (ok) {
        int ic;
        bool d2 = false;
        const MbState* nc;
        if (ty < 0 && tx > 3) {
          // in the macroblock above right (nb4 knows only those left and above)
          int a = mbaff_ || addr_ < width_ || addr_ % width_ == width_ - 1 ? -1 : addr_ - width_ + 1;
          nc = a >= 0 && mbs_[a].slice == sc_->slice_id_ ? &mbs_[a] : nullptr;
          ic = 12;
        } else {
          nc = sc_->nb4(tx, ty, ic);
        }
        D.mc = nb_mode(nc, ic, d2);
      }
    }
    int strength;
    D.dm = rec_.dimd(addr_, x * 4, y * 4, k8 ? 8 : 4, strength);
    D.ds = strength ? 1 + tc_clamp(tc_qlog(strength) - 6, 0, 8) : 0;
    proxy_mv();
    int sads[9];
    int pm = k8 ? proxy_mode_(x >> 1, y >> 1, 8, pmv_[0], pmv_[1], sads)
                : proxy_mode_(x, y, 4, pmv_[0], pmv_[1], sads);
    D.pm = pm < 0 ? 9 : pm;
    // how much worse the most probable mode does there, in halves of x264's
    // lambda of the predicted QP: an encoder takes it unless another mode
    // saves more than its three bits
    int lam16 = x264_lam16(qp_pred_);
    D.mg = 0;
    if (pm >= 0 && sads[mpm] < INT_MAX) D.mg = 1 + tc_clamp(tc_qlog((sads[mpm] - sads[pm]) * 32 / lam16), 0, 11);
    D.pq = pxq_;
    D.mh = blk ? mph_ & 15 : 16;  // the last most-probable-mode flags of the macroblock
    // for the remainder (rem_intra4x4_pred_mode): the proxy's best of the
    // modes but the most probable one (8 none), and by how much it beats the
    // next; DIMD's mode as a remainder (8 none or the most probable mode)
    D.pr = 8;
    D.p2 = 8;
    D.rg = 0;
    if (pm >= 0) {
      int b1 = -1, b2 = -1;
      for (int m = 0; m < 9; m++)
        if (m != mpm && sads[m] < INT_MAX) {
          if (b1 < 0 || sads[m] < sads[b1])
            b2 = b1, b1 = m;
          else if (b2 < 0 || sads[m] < sads[b2])
            b2 = m;
        }
      if (b1 >= 0) D.pr = b1 - (b1 > mpm);
      D.p2 = b2 >= 0 ? b2 - (b2 > mpm) : 8;
      if (b2 >= 0) D.rg = 1 + tc_clamp(tc_qlog((sads[b2] - sads[b1]) * 32 / lam16), 0, 11);
    }
    D.dr = D.dm < 9 && D.dm != mpm ? D.dm - (D.dm > mpm) : 8;
    tcx X;
    f_ipm.make(D, X);
    f_ipm.select(X);
    int r = -1;
    f_ipm.prp = cab0(68);
    if (!f_ipm.bit(0, ENC && v < 0)) {
      f_ipm.prp = cab0(69);
      int b2 = f_ipm.bit(1, ENC && (v >> 2 & 1));
      int b1 = f_ipm.bit(2 + b2, ENC && (v >> 1 & 1));
      int b0 = f_ipm.bit(4 + 2 * b2 + b1, ENC && (v & 1));
      r = b2 * 4 + b1 * 2 + b0;
    }
    int mode = r < 0 ? mpm : r < mpm ? r : r + 1;
    mph_ = (blk ? mph_ : 1) * 2 + (r < 0);
    if (k8)
      mi_->modes[y * 4 + x] = mi_->modes[y * 4 + x + 1] = mi_->modes[y * 4 + x + 4] = mi_->modes[y * 4 + x + 5] =
          (uint8_t)mode;
    else
      mi_->modes[y * 4 + x] = (uint8_t)mode;
    prev_mode_ = mode;
    tc_syms[STG_IPM]++;
    return r;
  }
  // intra_chroma_pred_mode of a neighbour, 4 for none
  int nb_cpred(const MbState* n) const {
    const MbInfo* m = info(n);
    return m && m->cls >= CL_INXN && m->cls != CL_PCM ? m->cpred : 4;
  }
  // the luma prediction in chroma's numbering (0 DC, 1 horizontal, 2 vertical, 3 plane)
  int luma_dir() const {
    if (mi_->cls == CL_I16) {
      static const int k16[4] = {2, 1, 0, 3};
      return k16[mi_->i16m & 3];
    }
    static const int k4[9] = {2, 1, 0, 2, 3, 2, 1, 2, 1};
    int n[4] = {0, 0, 0, 0};
    for (int i = 0; i < 16; i++) n[k4[mi_->modes[i] % 9]]++;
    int best = 0;
    for (int i = 1; i < 4; i++)
      if (n[i] > n[best]) best = i;
    return best;
  }
  int cpred(int v) {
    tc_stage = STG_IPM;
    H_ipm_v D = {};
    D.k8 = 2;
    D.ma = nb_cpred(sc_->mb_a_);
    D.mb = nb_cpred(sc_->mb_b_);
    D.mp = luma_dir();
    D.blk = 0;
    D.pv = 9;
    D.col = col_ && col_->cls >= CL_INXN && col_->cls != CL_PCM ? col_->cpred : 9;
    int strength;
    D.dm = rec_.dimd(addr_, 0, 0, 16, strength);
    D.ds = strength ? 1 + tc_clamp(tc_qlog(strength) - 6, 0, 8) : 0;
    D.pm = 9;
    D.mc = 10;
    D.pr = D.dr = 8;
    if (sc_->chroma_ == 1) {
      // the chroma mode that does best in the proxy, and how much worse DC
      // (the cheapest to code) does
      proxy_mv();
      int costs[4];
      int pc = proxy_cmode_(pmv_[0], pmv_[1], costs);
      if (pc >= 0) {
        D.pm = pc;
        int c0 = costs[0] < INT_MAX ? costs[0] : costs[pc];
        D.mg = 1 + tc_clamp(tc_qlog((c0 - costs[pc]) * 32 / x264_lam16(qp_pred_)), 0, 11);
      }
    }
    tcx X;
    f_ipm.make(D, X);
    f_ipm.select(X);
    int r = 0;
    f_ipm.prp = cab0(SC::kCtxCpred + sc_->inc_cpred());
    if (f_ipm.bit(16, ENC && v > 0)) {
      f_ipm.prp = cab0(SC::kCtxCpred + 3);
      r = f_ipm.bit(17, ENC && v > 1) ? 2 + f_ipm.bit(18, ENC && v > 2) : 1;
    }
    mi_->cpred = (uint8_t)r;
    tc_syms[STG_IPM]++;
    return r;
  }

  // --- sub_mb_type, ref_idx and mvd: the mvs family
  int sub(int i, int v) {
    tc_stage = STG_MVS;
    bool b = sc_->stype_ == kB;
    H_mvs_v D = {};
    D.kind = b;
    D.ma = i ? sub_[i - 1] + 1 : 0;
    D.pi = i;
    tcx X;
    f_mvs.make(D, X);
    f_mvs.select(X);
    f_mvs.prp = -1;
    f_mvs.prpa = nullptr;
    int node = 1;
    for (int k = b ? 3 : 1; k >= 0; k--) node = node * 2 + f_mvs.bit(node, ENC && (v >> k & 1));
    int r = node - (b ? 16 : 4);
    sub_[i] = r;
    mvpred_.subshape[i] = b ? kBSubShape[r % 13] : r;
    if (LV::mvp && i == 3 && b)
      mvpred_.direct((sub_[0] == 0) | (sub_[1] == 0) << 1 | (sub_[2] == 0) << 2 | (sub_[3] == 0) << 3, mi_->mv, mi_->rf);
    tc_syms[STG_MVS]++;
    return r;
  }

  // tm_best() from MotionPred::pred()'s vector of the macroblock (mp) and its
  // neighbours (nbs), which are the same whatever its partitions (they
  // are the macroblocks around)
  bool tm_best(int l, const MvN* nbs, const int* mp, int* mv, int& miss) const {
    mv[0] = mp[0], mv[1] = mp[1];
    int cand[4][2], nc = 0;
    for (int j = 0; j < 3; j++)
      if (nbs[j].ref == 0) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
    cand[nc][0] = cand[nc][1] = 0, nc++;
    return tm_search_(l, 0, 0, 0, 16, 16, mv, false, cand, nc, &miss, 0, 4);
  }
  static int part_size(int w, int h) { return w * h == 16 ? 0 : w * h == 8 ? 1 : w * h == 4 ? 2 : 3; }
  int ref(int list, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1);
    const MbInfo* a = info(sc_->nb4(x - 1, y, ia));
    const MbInfo* b = info(sc_->nb4(x, y - 1, ib));
    H_mvs_v D = {};
    D.kind = 2;
    D.comp = 0;
    D.lst = list;
    D.ma = a && a->cls < CL_INXN ? 1 + a->ref[list][(ia >> 3) * 2 + ((ia & 3) >> 1)] : 0;
    D.mb = b && b->cls < CL_INXN ? 1 + b->ref[list][(ib >> 3) * 2 + ((ib & 3) >> 1)] : 0;
    D.oth = list ? 1 + sc_->cur_->ref[0][b8] : 0;
    D.ps = part_size(w, h);
    D.rf = b8 ? 1 + mi_->ref[list][b8 - 1] : 0;
    D.pi = b8;
    D.nr = sc_->num_ref_[list] - 1;
    D.pd = col_ && col_->cls < CL_INXN ? 1 + col_->ref[list][b8] : 0;
    D.sp = 0;
    D.tv = 0;
    // the reference the template matches best (of the first four, by their
    // candidates alone), and by how much better than the first
    int p0[2] = {0, 0}, have_p0 = 0;  // MotionPred::pred()'s vector of the first reference
    if (rec_.act() && sc_->num_ref_[list] > 1) {
      int bm = INT_MAX, m0 = 0, rbest = -1;
      for (int r = 0; r < Min(sc_->num_ref_[list], 4); r++) {
        int p[2], miss, cand[4][2], nc = 0;
        MvN nbs[3];
        mvpred_.pred(list, x, y, w, h, r, p, nbs);
        if (!r) p0[0] = p[0], p0[1] = p[1], have_p0 = 1;
        for (int j = 0; j < 3; j++)
          if (nbs[j].ref == r) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
        cand[nc][0] = cand[nc][1] = 0, nc++;
        if (!tm_search_(list, r, x * 4, y * 4, w * 4, h * 4, p, false, cand, nc, &miss, kTmLambda, 8)) break;
        if (!r) m0 = miss;
        if (miss < bm) bm = miss, rbest = r;
      }
      if (rbest >= 0) {
        D.tv = 1 + rbest;
        D.tq = 1 + tc_clamp(tc_qlog(m0 - bm), 0, 11);
      }
    }
    {
      // the texture of the first reference at the predicted vector (as for
      // the mvd)
      int p[2];
      if (have_p0)
        p[0] = p0[0], p[1] = p0[1];
      else
        mvpred_.pred(list, x, y, w, h, 0, p);
      int t = rec_.ref_texture(list, 0, x * 4, y * 4, w * 4, h * 4, p);
      D.rt = t < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)t * 256 / (w * h * 16) * 16 / qstep16(qp_pred_)), 0, 14);
    }
    tcx X;
    f_mvs.make(D, X);
    NodePrp pr;
    pr.Init();
    pr[0] = cab0(SC::kCtxRef + sc_->ref_inc(list, x, y));
    pr[1] = cab0(SC::kCtxRef + 4);
    pr[2] = cab0(SC::kCtxRef + 5);
    if (pr[0] >= 0)
      for (int k = 0; k < 3; k++) pr[k] = CM_PONE - pr[k];
    f_mvs.prpa = pr;
    v = cm_int(f_mvs.code(X, ENC ? v : 0), 64);
    f_mvs.prpa = nullptr;
    for (int yy = y; yy < y + h; yy += 2)
      for (int xx = x; xx < x + w; xx += 2) mi_->ref[list][(yy >> 1) * 2 + (xx >> 1)] = (int8_t)v;
    tc_syms[STG_MVS]++;
    return v;
  }
  // a distance in POC, held to the 16 bits the spec allows (a stream past
  // them would have the scaling below overflow)
  static int poc_dist(int a, int b) {
    int64_t d = (int64_t)a - b;
    return (int)(d < -32768 ? -32768 : d > 32767 ? 32767 : d);
  }
  // the distance in POC from the current picture to list l's reference r
  // (0 not known), and a vector over distance dn scaled to distance dr
  int ref_dist(int l, int r) const {
    return rec_.motion() && r >= 0 && r < rec_.nlist[l] && rec_.list[l][r] ? poc_dist(cur_poc_, rec_.list[l][r]->poc) : 0;
  }
  static int mv_scale(int v, int dr, int dn) { return (int)div_floor((int64_t)v * dr * 2 + dn, 2 * (int64_t)dn); }
  // B: the vector the motion of list 1's first reference's co-located
  // block (as if it pointed at list 0's first reference) gives the 4x4
  // block (x, y) on list l's reference ref, as temporal direct does: false
  // none
  bool td_mv(int l, int ref, int x, int y, int* m) const {
    if (sc_->stype_ != kB || !col1_ || col1_->cls >= CL_INXN) return false;
    int b8 = (y >> 1) * 2 + (x >> 1), cl = col1_->rf[0][b8] >= 0 ? 0 : 1, cr = col1_->rf[cl][b8];
    int td = ref_dist(0, 0) - ref_dist(1, 0), tr = ref_dist(l, ref), k1 = phk_;
    // over the distance from that picture to its own reference, where known
    if (cr >= 0 && k1 >= 0 && col1_->rpoc[cl][b8] != INT_MIN && ref_dist(1, 0))
      td = poc_dist(hist_.poc[k1], col1_->rpoc[cl][b8]);
    if (cr < 0 || !td || !tr || !ref_dist(0, 0)) return false;
    for (int k = 0; k < 2; k++) m[k] = mv_scale(col1_->mv[cl][y * 4 + x][k], tr, td);
    return true;
  }
  int mvd(int list, int comp, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1), ref = sc_->cur_->ref[list][b8];
    if (!comp && !LV::mvp) {
      // (a level without the motion: no prediction, nor what depends on it)
      for (int k = 0; k < 2; k++) mvp_[k] = 0, sp_[k] = -1, ns_[k] = 0, pdv_[k] = 0;
      pdn_ = true;
      tmn_ = true;
    } else if (!comp) {
      mvd_pred(list, x, y, w, h, ref);
    } else if (!tmn_) {
      // the horizontal component known
      tmv_[0] = mvp_[0] + mvx_;
      tmn_ = !tm_search_(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, true, nullptr, 0, &tmq_, kTmLambda);
    }
    int dv = tmn_ ? 0 : tmv_[comp] - mvp_[comp];
    const MbInfo* a = info(sc_->nb4(x - 1, y, ia));
    const MbInfo* b = info(sc_->nb4(x, y - 1, ib));
    int va = a ? a->mvd[list][ia][comp] : 0, vb = b ? b->mvd[list][ib][comp] : 0;
    // how far A's and B's mvds were from their template matches'
    int ea = a ? a->tme[list][ia][comp] : -128, eb = b ? b->tme[list][ib][comp] : -128;
    H_mvs_v D = {};
    D.kind = 3;
    D.comp = comp;
    D.lst = list;
    D.ma = tc_qlog(va);
    D.mb = tc_qlog(vb);
    D.oth = comp ? tc_qlog(mvx_) : 0;
    D.ps = part_size(w, h);
    D.rf = ref;
    D.pi = b8;
    D.nr = sc_->num_ref_[list] - 1;
    D.pd = pdn_ ? 0 : 1 + tc_qlog(pdv_[comp]);
    D.sp = sp_[comp] < 0 ? 0 : 1 + tc_qlog(sp_[comp]);
    D.tv = tmn_ ? 0 : 1 + tc_clamp(sq2(dv), 0, 12);
    // the mvd were the vector zero; col's mvd
    D.zd = tc_clamp(sq2(-mvp_[comp]), 0, 12);
    D.cq = col_ && col_->cls < CL_INXN ? 1 + Min<int>(col_->mvq[list][y * 4 + x][comp], 8) : 0;
    D.tq = tmn_ ? 0 : 1 + tc_clamp(tc_qlog(tmq_), 0, 11);
    D.ta = tmn_ || talt_[0] == INT_MIN ? 0 : 1 + tc_clamp(sq2(talt_[comp] - mvp_[comp]), 0, 12);
    D.ea = ea == -128 ? 0 : 1 + Min(tc_qlog(ea), 5);
    D.eb = eb == -128 ? 0 : 1 + Min(tc_qlog(eb), 5);
    // the texture of the reference at the predicted vector across the
    // component (x: the differences along rows), per sample, against the
    // quantizer's step: where it is flat that way, a search for a better
    // vector gains nothing
    {
      int t = rec_.ref_texture(list, ref, x * 4, y * 4, w * 4, h * 4, mvp_, comp ? 2 : 1);
      D.rt = t < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)t * 512 / (w * h * 16) * 16 / qstep16(qp_pred_)), 0, 14);
    }
    tcx X;
    f_mvs.make(D, X);
    tc_stage = STG_MVD;
    tc_syms[STG_MVD]++;
    NodePrp pr;
    pr.Init();
    int base = SC::ctx_mvd(comp);
    pr[0] = cab1(base + sc_->mvd_inc(list, comp, x, y));
    pr[1] = cab1(base + 3);
    pr[2] = cab1(base + 4);
    f_mvs.prpa = pr;
    f_mvs.hint = tmn_ ? -1 : abs(dv);  // the template match's
    v = cm_int(f_mvs.code(X, ENC ? abs(v) : 0), 1 << 30) * (ENC && v < 0 ? -1 : 1);
    f_mvs.hint = -1;
    f_mvs.prpa = nullptr;
    if (v) {
      tc_stage = STG_MVSG;
      tc_syms[STG_MVSG]++;
      v = mvd_sign(list, comp, v, dv, va, vb, ea, eb, D.tq);
    }
    if (!comp) {
      mvx_ = v;
    } else if (LV::mvp) {
      int mv[2] = {mvp_[0] + mvx_, mvp_[1] + v};
      mvpred_.set(mi_->mv, mi_->rf, list, x, y, w, h, ref, mv);
    }
    int16_t s = (int16_t)tc_clamp(v, -32767, 32767);
    int8_t te = tmn_ ? (int8_t)-128 : (int8_t)tc_clamp(v - dv, -127, 127);
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) mi_->mvd[list][yy * 4 + xx][comp] = s, mi_->tme[list][yy * 4 + xx][comp] = te;
    return v;
  }
  // the prediction of a partition's vector (mvp_), and what the mvd is
  // coded against besides: how the neighbours spread around it (sp_, ns_),
  // col's vector (scaled to the reference; temporal direct's in B slices)
  // less it (pdv_; pdn_ none), the template match from it (tmv_, tmq_;
  // tmn_ none; talt_ the runner-up's start)
  void mvd_pred(int list, int x, int y, int w, int h, int ref) {
    int b8 = (y >> 1) * 2 + (x >> 1);
    MvN nbs[3];
    mvpred_.pred(list, x, y, w, h, ref, mvp_, nbs);
    for (int k = 0; k < 2; k++) {
      sp_[k] = -1;
      ns_[k] = 0;
      for (int j = 0; j < 3; j++)
        if (nbs[j].ref >= 0) {
          sp_[k] = Max(sp_[k], abs(nbs[j].mv[k] - mvp_[k]));
          ns_[k] += (nbs[j].mv[k] > mvp_[k]) - (nbs[j].mv[k] < mvp_[k]);
        }
      pdv_[k] = 0;
    }
    // col's vector; in B slices, temporal direct's where there is one
    int cm[2] = {0, 0};
    pdn_ = !col_ || col_->rf[list][b8] < 0;
    if (!pdn_) {
      cm[0] = col_->mv[list][y * 4 + x][0], cm[1] = col_->mv[list][y * 4 + x][1];
      // scaled by the reference's distance against that of col's own
      // (where not known, the first reference's)
      int d0 = ref_dist(list, 0), dr = ref_dist(list, ref);
      if (prev_poc_ != INT_MIN && col_->rpoc[list][b8] != INT_MIN) d0 = poc_dist(prev_poc_, col_->rpoc[list][b8]);
      if (d0 && dr && d0 != dr)
        for (int k = 0; k < 2; k++) cm[k] = mv_scale(cm[k], dr, d0);
    }
    int cc[2] = {cm[0], cm[1]};
    bool ccok = !pdn_;
    if (td_mv(list, ref, x, y, cm)) pdn_ = false;
    else ccok = false;
    if (!pdn_)
      for (int k = 0; k < 2; k++) pdv_[k] = cm[k] - mvp_[k];
    tmv_[0] = mvp_[0];
    tmv_[1] = mvp_[1];
    // starting from mvp, the neighbours' vectors on the same reference,
    // col's, zero
    int cand[8][2], nc = 0;
    for (int j = 0; j < 3; j++)
      if (nbs[j].ref == ref) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
    // and those on other references of the list, scaled to this one's
    // distance
    for (int j = 0; j < 3; j++) {
      int dn = ref_dist(list, nbs[j].ref), dr = ref_dist(list, ref);
      if (nbs[j].ref >= 0 && nbs[j].ref != ref && dn && dr) {
        for (int k = 0; k < 2; k++) cand[nc][k] = mv_scale(nbs[j].mv[k], dr, dn);
        nc++;
      }
    }
    if (!pdn_) cand[nc][0] = cm[0], cand[nc][1] = cm[1], nc++;
    if (ccok) cand[nc][0] = cc[0], cand[nc][1] = cc[1], nc++;  // and col's, temporal direct's taken
    cand[nc][0] = cand[nc][1] = 0, nc++;
    tmn_ = !tm_search_(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, false, cand, nc, &tmq_, kTmLambda, 1, 0, talt_);
  }
  // the sign of a nonzero mvd component v (the mvg family), knowing the
  // magnitude: whether it is the template match's (dv), the zero vector's,
  // col's; A's and B's mvds (va, vb) and how far they were from their
  // template matches' (ea, eb); the match's miss (tq)
  int mvd_sign(int list, int comp, int v, int dv, int va, int vb, int ea, int eb, int tq) {
    int m = abs(v);
    auto rel = [&](int d) { return rel_sign(m, d); };
    H_mvg_v G = {};
    G.comp = comp;
    G.lst = list;
    G.ts = tmn_ ? 9 : rel(dv);
    G.zs = rel(-mvp_[comp]);
    G.ds = pdn_ ? 9 : rel(pdv_[comp]);
    G.sa = sgn3(va);
    G.sb = sgn3(vb);
    G.mag = Min(tc_qlog(m), 7);
    G.tq = tq;
    G.ox = comp ? sgn3(mvx_) : 3;
    G.ns = 3 + ns_[comp];
    G.es = ea == -128 && eb == -128 ? 3 : sgn3((ea == -128 ? 0 : ea) + (eb == -128 ? 0 : eb));
    G.as = tmn_ || talt_[0] == INT_MIN ? 9 : rel(talt_[comp] - mvp_[comp]);
    tcx Y;
    f_mvg.make(G, Y);
    f_mvg.select(Y);
    f_mvg.prp = -1;
    if (f_mvg.bit(0, ENC && v < 0)) v = -m;
    return v;
  }

  // the macroblocks, as they were, of the picture list l's reference r is:
  // the last picture of a kind with its POC (none: false); by address, what
  // mc_col() and mc_nz() read of each
  HistView ref_hist(int l, int r) const {
    if (!rec_.motion() || r < 0 || r >= rec_.nlist[l] || !rec_.list[l][r]) return HistView();
    return hist_.view(rec_.list[l][r]->poc, prev_poc_, key_, size_);
  }
  // What mc_col() and mc_nz() share: for an inter macroblock of a P
  // picture (not MBAFF) whose block at (x, y) (4x4 blocks) refers to the
  // last picture of a kind, that picture's macroblocks as they were (hr),
  // and where the block's vector puts its sample (off, off) (px, py, in
  // the picture's samples, not clipped; still: the vector moves it less
  // than a sample); false none
  bool mc_ref(int x, int y, int off, HistView& hr, int& px, int& py, int& still) const {
    if ((sc_->stype_ != kP && sc_->stype_ != kSP) || mi_->cls >= CL_INXN || mbaff_) return false;
    hr = ref_hist(0, mi_->rf[0][(y >> 1) * 2 + (x >> 1)]);
    if (!hr) return false;
    const int16_t* v = mi_->mv[0][y * 4 + x];
    still = abs(v[0]) <= 1 && abs(v[1]) <= 1;
    px = addr_ % width_ * 16 + x * 4 + (still ? 0 : (int)div_floor(v[0] + 2, 4)) + off;
    py = addr_ / width_ * 16 + y * 4 + (still ? 0 : (int)div_floor(v[1] + 2, 4)) + off;
    return true;
  }
  // The block of the reference, as it was, that the vector of the current
  // macroblock's block of n x n samples at (x, y) (4x4 blocks) points at,
  // for an inter macroblock of a P picture whose reference is the last
  // picture of a kind: the macroblock there, and (bx, by) the block's place
  // in it (4x4 blocks, the top left one of an 8x8 block); moved, the vector
  // moves it; false none.
  bool mc_col(int x, int y, int n, int& bx, int& by, int& moved, MbRef& c) const {
    HistView hr;
    hr.Init();
    int px, py, still;
    if (!mc_ref(x, y, n / 2, hr, px, py, still)) return false;
    px = tc_clamp(px, 0, width_ * 16 - 1);
    py = tc_clamp(py, 0, size_ / width_ * 16 - 1);
    c = hr.at((py >> 4) * width_ + (px >> 4));
    if (!c.valid) return false;
    moved = !still;
    bx = (px & 15) >> 2;
    by = (py & 15) >> 2;
    if (n == 8) bx &= 2, by &= 2;
    return true;
  }
  // the 4x4 luma blocks with coefficients, of the reference, under where
  // the vector of the current macroblock's 8x8 block at (x, y) (4x4 blocks)
  // puts it, as mc_col: 0-4 still, 5-9 moved; -1 none
  int mc_nz(int x, int y) const {
    HistView hr;
    hr.Init();
    int px, py, still;
    if (!mc_ref(x, y, 2, hr, px, py, still)) return -1;
    int nz = 0, h = size_ / width_ * 16;
    for (int k = 0; k < 4; k++) {
      int qx = tc_clamp(px + (k & 1) * 4, 0, width_ * 16 - 1), qy = tc_clamp(py + (k >> 1) * 4, 0, h - 1);
      MbRef c = hr.at((qy >> 4) * width_ + (qx >> 4));
      if (!c.valid) return -1;
      nz += c.nzm >> (((qy & 15) >> 2) * 4 + ((qx & 15) >> 2)) & 1;
    }
    return nz + 5 * !still;
  }
  // the macroblock the current one's content was at in its reference, by
  // its mean vector (intra: the proxy's)
  int mc_addr() {
    int sx = 0, sy = 0;
    if (mi_->cls < CL_INXN) {
      for (int b = 0; b < 16; b++) {
        int l = mi_->rf[0][(b >> 3) * 2 + ((b & 3) >> 1)] >= 0 ? 0 : 1;
        sx += mi_->mv[l][b][0];
        sy += mi_->mv[l][b][1];
      }
      sx = (int)div_floor(sx + 512, 1024);
      sy = (int)div_floor(sy + 512, 1024);
    } else if (pmv_ok_) {
      sx = (int)div_floor(pmv_[0] + 8, 16);
      sy = (int)div_floor(pmv_[1] + 8, 16);
    }
    int x = tc_clamp(addr_ % width_ + sx, 0, width_ - 1), y = tc_clamp(addr_ / width_ + sy, 0, size_ / width_ - 1);
    return y * width_ + x;
  }
  // --- mb_qp_delta: the qpd family. The QP of the macroblock follows.
  static int qp_class(int d) { return 4 + tc_clamp(d, -3, 3); }
  // the kind of a macroblock's class: inter, I_NxN, other
  static int kind3(int cls) { return cls < CL_INXN ? 0 : cls == CL_INXN ? 1 : 2; }
  int qpd(int v) {
    prepare();
    tc_stage = STG_QPD;
    aq_.xp = -1;
    aq_.xi = false;
    const MbInfo* b = info(sc_->mb_b_);
    H_qpd_v D = {};
    qpd_vars(D, b);
    int aqd = qpd_aq(D);
    tcx X;
    f_qpd.make(D, X);
    NodePrp pr;
    pr.Init();
    pr[0] = cab1(sc_->ctx_qpd());
    f_qpd.prpa = pr;
    f_qpd.hint = aqd == INT_MIN ? -1 : abs(aqd);
    {
      int m = cm_int(f_qpd.code(X, ENC ? abs(v) : 0), 1 << 20);
      bool neg = ENC && v < 0;
      v = m ? qpd_sign(D, b, aqd, m, neg) : 0;
    }
    f_qpd.hint = -1;
    f_qpd.prpa = nullptr;
    int qp = ((qp_pred_ + v + 52 + 2 * qp_off_) % (52 + qp_off_) + (52 + qp_off_)) % (52 + qp_off_) - qp_off_;
    qp_pred_ = qp;
    qp_src_ = addr_;
    lq_[kind3(mi_->cls)][D.xe] = qp + 1;
    mi_->qp = (uint8_t)(qp + qp_off_);
    last_qpd_ = v;
    if (v) {
      lz_ = v;
      has_lz_ = true;
    }
    qpd_coded_ = true;
    tc_syms[STG_QPD]++;
    return v;
  }
  // the qpd family's variables but x264's AQ's: the deltas of the
  // macroblocks around and of the last one, the slice's, the picture's
  // change there
  void qpd_vars(H_qpd_v& D, const MbInfo* b) const {
    D.lnz = last_qpd_ != 0;
    D.pc = popc4(mi_->cbp & 15);
    D.cc = mi_->cbp >> 4;
    D.cls = mi_->cls;
    D.dc = col_ ? qp_class(col_->qp - qp_off_ - qp_pred_) : 0;
    D.db = b ? qp_class(b->qp - qp_off_ - qp_pred_) : 0;
    // and where B is of the same kind
    D.bc = b && !b->skip && kind3(b->cls) == kind3(mi_->cls) ? 7 + tc_clamp(b->qp - qp_off_ - qp_pred_, -6, 6) : 0;
    D.cl = col_ && !col_->skip && kind3(col_->cls) == kind3(mi_->cls) ? 7 + tc_clamp(col_->qp - qp_off_ - qp_pred_, -6, 6) : 0;
    {
      const MbInfo* c = near_info(1, -1);
      D.kc = c && !c->skip && kind3(c->cls) == kind3(mi_->cls) ? 7 + tc_clamp(c->qp - qp_off_ - qp_pred_, -6, 6) : 0;
    }
    D.lz = has_lz_ ? 3 + tc_clamp(tc_sq(lz_), -3, 3) : 3;
    D.st = st_;
    D.td = map_td();
    D.sq = 7 + tc_clamp(slice_qp_ - qp_pred_, -6, 6);
    // x264's adaptive quantization: the QP follows the energy of the
    // macroblock, which the last picture shows; the change since the one
    // that set the predicted QP
    D.ae = 0;
    if (maps_ok()) {
      int e0 = qp_src_ >= 0 ? rec_.maps.energy[qp_src_] : rec_.maps.energy_mean;
      D.ae = 7 + tc_clamp((rec_.maps.energy[addr_] - e0 + 2 * (rec_.maps.energy[addr_] > e0) - 2 * (rec_.maps.energy[addr_] < e0)) / 4, -6, 6);
    }
  }
  // and x264's AQ's (AqModel): the QP its line gives the energy of the
  // prediction (inter) or the proxy (intra), or of the last picture there,
  // less the predicted QP; how far the last picture of the kind was from
  // its line there; the energy's class and the QP of the last macroblock
  // of it. The change the line asks for (INT_MIN none).
  int qpd_aq(H_qpd_v& D) {
    D.aq = D.ac = 0;
    int aqd = INT_MIN;
    if (rec_.act() && (int)aq_.x.size() == size_) {
      // intra: the mean of the last picture's there and the left and upper
      // neighbours'
      int x = aq_.around(addr_, width_);
      int q16;
      bool ok = false;
      if (mi_->cls < 6) {
        aq_.xp = lg16(rec_.ac_energy(addr_));
        ok = aq_.fitp.predict(aq_.xp, q16);
        if (!ok) x = aq_.xp;
      } else if (mi_->cls != CL_PCM && sc_->chroma_ == 1) {
        // intra: the energy of its proxy, a reference's block
        proxy_mv();
        int64_t e = proxy_energy_();
        if (e >= 0) {
          aq_.xp = lg16(e);
          aq_.xi = true;
          ok = aq_.fitx.predict(aq_.xp, q16) || aq_.fitp.predict(aq_.xp, q16);
        }
      }
      D.xe = aq_.xp < 0 ? 0 : 1 + Min(aq_.xp / 16, 17);
      // the QP of the last macroblock of the class: an encoder's QP is its
      // rate control's plus what the energy adds
      int lk = kind3(mi_->cls);
      if (lq_[lk][D.xe]) D.lq = 7 + tc_clamp(lq_[lk][D.xe] - 1 - qp_pred_, -6, 6);
      if (ok || (x >= 0 && aq_.fit.predict(x, q16))) {
        aqd = (int)div_floor(q16 - 16 * qp_pred_ + 8, 16);
        D.aq = 7 + tc_clamp(aqd, -6, 6);
      }
      int ma = mc_addr();
      if ((int)aq_.res[key_].size() == size_ && aq_.res[key_][ma] != INT16_MIN)
        D.ac = 7 + tc_clamp((int)div_floor(aq_.res[key_][ma] + 4, 8), -6, 6);
    }
    return aqd;
  }
  // the sign of a nonzero mb_qp_delta (the qpg family), knowing its
  // magnitude m: on which side of the line's delta, col's and B's it puts
  // the QP
  int qpd_sign(const H_qpd_v& D, const MbInfo* b, int aqd, int m, bool neg) {
    auto rel = [&](int d) { return rel_sign(m, d); };
    H_qpg_v G = {};
    G.as = aqd == INT_MIN ? 9 : rel(aqd);
    G.cs = col_ ? rel(col_->qp - qp_off_ - qp_pred_) : 9;
    G.bs = b ? rel(b->qp - qp_off_ - qp_pred_) : 9;
    G.ls = has_lz_ ? sgn3(lz_) : 0;
    G.rs = D.ac ? sgn3(D.ac - 7) : 3;
    G.mag = Min(tc_qlog(m), 7);
    G.cls = mi_->cls;
    G.st = st_;
    G.ss = rel(slice_qp_ - qp_pred_);
    G.xe = D.xe;
    G.lq = D.lq ? rel(D.lq - 7) : 9;
    G.bk = D.bc ? rel(D.bc - 7) : 9;
    tcx Y;
    f_qpg.make(G, Y);
    f_qpg.select(Y);
    f_qpg.prp = -1;
    return f_qpg.bit(0, neg) ? -m : m;
  }

  // --- blocks of coefficients: whether a block has a nonzero one (cbf),
  // the significance map (sig), the levels in reverse scan order (lvl) and
  // the signs (sgn)
  struct Geo {
    int plane, x, y;  // 4x4 position of the block (of the top left one of an 8x8 block)
    int kind;         // 0 DC, 1 4x4 or AC, 2 chroma AC (4:2:0, 4:2:2), 3 8x8
    int off;          // first scan position in the store: 1 for AC blocks
    int pos;          // place in the 8x8 block (in the macroblock for chroma)
  };
  // the scan of a block's coefficients as raster positions in its w x h,
  // and back: 8x8 and 4x4 zigzag (Intra16x16's DC, and 4:4:4's chroma DC,
  // too), 4:2:0's chroma DC 2 x 2 in raster order, 4:2:2's 2 x 4 in its own
  struct Scan {
    int w, h;
    const uint8_t *zz, *inv;
  };
  Scan scan_of(const Geo& g) const {
    if (g.kind == 3) return Scan{8, 8, kZz8, kInv8};
    if (g.kind == 0 && g.plane && sc_->chroma_ != 3)
      return sc_->chroma_ == 2 ? Scan{2, 4, kZzC422, kInvC422} : Scan{2, 2, kRaster4, kRaster4};
    return Scan{4, 4, kZz4, kInv4};
  }
  Geo geo(int cat, int idx) const {
    Geo g = {0, 0, 0, 1, 0, 0};
    g.plane = cat < 6 ? 0 : cat < 10 ? 1 : 2;
    // c: 0 DC, 1 AC, 2 4x4, 3 chroma DC, 4 chroma AC, 5 8x8 (cats 6-9 and 10-13: DC, AC, 4x4, 8x8)
    int c = cat < 6 ? cat : (cat - 6) % 4 == 3 ? 5 : (cat - 6) % 4;
    switch (c) {
      case 0:
        g.kind = 0;
        break;
      case 3:
        g.kind = 0;
        g.plane = 1 + idx;
        g.pos = idx;
        break;
      case 1:
      case 2:
        g.x = blk_x(idx);
        g.y = blk_y(idx);
        g.off = c == 1;
        g.pos = idx & 3;
        break;
      case 4: {
        int nb = sc_->ch_blocks_, b = idx % nb;
        g.plane = 1 + idx / nb;
        g.kind = 2;
        g.x = b & 1;
        g.y = b >> 1;
        g.off = 1;
        g.pos = b & 3;
        break;
      }
      default:
        g.kind = 3;
        g.x = (idx & 1) * 2;
        g.y = (idx >> 1) * 2;
        g.pos = idx;
    }
    return g;
  }
  // a neighbouring block: its macroblock and its 4x4 block (raster)
  struct Nb {
    const MbInfo* m;
    const MbState* s;
    int i;
  };
  Nb nb_block(const Geo& g, int k) const {
    Nb n = {nullptr, nullptr, 0};
    if (g.kind == 0) {
      n.s = k ? sc_->mb_b_ : sc_->mb_a_;
    } else if (g.kind == 2) {
      n.s = k ? sc_->nbc(g.x, g.y - 1, n.i) : sc_->nbc(g.x - 1, g.y, n.i);
    } else {
      n.s = k ? sc_->nb4(g.x, g.y - 1, n.i) : sc_->nb4(g.x - 1, g.y, n.i);
    }
    n.m = info(n.s);
    return n;
  }
  // x264_decimate_score16 of a 4x4 block's levels (scan order): 9 with a
  // level beyond 1, else by the zeros before each
  static int decimate_score(const int8_t* c) {
    static const int ds[16] = {3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int k = 15, score = 0;
    while (k >= 0 && !c[k]) k--;
    while (k >= 0) {
      if (abs(c[k--]) > 1) return 9;
      int run = 0;
      while (k >= 0 && !c[k]) k--, run++;
      score += ds[run];
    }
    return score;
  }
  // the coefficient at scan position j of a block like g's in macroblock
  // m, at 4x4 block i
  static int coef_at(const MbInfo* m, const Geo& g, int i, int j) {
    if (g.kind == 0) return m->cmask >> (48 + g.plane) & 1 ? m->dc[g.plane][j] : 0;
    if (g.kind == 3) {
      int s = j & 3, x = (i & 3 & ~1) + (s & 1), y = ((i >> 2) & ~1) + (s >> 1);
      i = y * 4 + x, j >>= 2;
    } else {
      j += g.off;
    }
    return m->cmask >> (g.plane * 16 + i) & 1 ? m->coef[g.plane][i][j] : 0;
  }
  void store_coef(const Geo& g, const int* c, int n) {
    // (the rows it covers; where they are not written yet, a block all zero
    // is there already)
    uint64_t rows = g.kind == 0   ? (uint64_t)1 << (48 + g.plane)
                    : g.kind == 3 ? (uint64_t)0x33 << (g.plane * 16 + g.y * 4 + g.x)
                                  : (uint64_t)1 << (g.plane * 16 + (g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x));
    if (!(mi_->cmask & rows)) {
      int k = 0, m = g.kind == 3 ? 64 : g.kind == 0 ? Min(n, 16) : n;
      while (k < m && !c[k]) k++;
      if (k == m) return;
    }
    // (rows not written yet hold an older macroblock's)
    for (uint64_t m = rows & ~mi_->cmask; m; m &= m - 1) {
      int r = __builtin_ctzll(m);
      memset(r < 48 ? mi_->coef[r >> 4][r & 15] : mi_->dc[r - 48], 0, 16);
    }
    mi_->cmask |= rows;
    int8_t* d;
    if (g.kind == 0) {
      d = mi_->dc[g.plane];
      for (int j = 0; j < n && j < 16; j++) d[j] = (int8_t)tc_clamp(c[j], -127, 127);
    } else if (g.kind == 3) {
      for (int j = 0; j < 64; j++) {
        int s = j & 3;
        mi_->coef[g.plane][(g.y + (s >> 1)) * 4 + g.x + (s & 1)][j >> 2] = (int8_t)tc_clamp(c[j], -127, 127);
      }
    } else {
      d = mi_->coef[g.plane][g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x];
      for (int j = 0; j < n; j++) d[j + g.off] = (int8_t)tc_clamp(c[j], -127, 127);
    }
  }
  // nonzero coefficients of a neighbouring block, 9 for none
  int nb_count(const Nb& n, const Geo& g) const {
    if (!n.s) return 9;
    if (g.kind == 0) return n.s->cbf_dc[g.plane];
    return Min<int>(n.s->nz[g.plane][n.i], 8);
  }
  int block_nc(const Geo& g) const {
    if (g.kind == 0) return g.plane && sc_->chroma_ != 3 ? 0 : sc_->nc_luma(g.plane, 0, 0);
    if (g.kind == 2) return sc_->nc_chroma(g.plane - 1, g.x, g.y);
    return sc_->nc_luma(g.plane, g.x, g.y);
  }
  // the prediction of a block: its Intra4x4/8x8 mode (0-8), Intra16x16 (9-12)
  // or chroma (13-16) mode, or for inter blocks the size of the mvd (17-19)
  int block_mode(const Geo& g) const {
    if (mi_->cls == CL_INXN && (g.plane == 0 || sc_->chroma_ == 3)) return mi_->modes[g.y * 4 + g.x];
    if (mi_->cls == CL_I16 && (g.plane == 0 || sc_->chroma_ == 3)) return 9 + mi_->i16m;
    if (mi_->cls >= CL_INXN) return 13 + mi_->cpred;
    // a chroma block's luma 4x4 block: 4:2:0 halves both ways, 4:2:2 the width
    int x = g.kind == 2 ? g.x * 2 : g.x, y = g.kind == 2 ? (sc_->chroma_ == 2 ? g.y : g.y * 2) : g.y;
    if (g.kind == 0) x = y = 0;
    int m = abs(mi_->mvd[0][y * 4 + x][0]) + abs(mi_->mvd[0][y * 4 + x][1]) + abs(mi_->mvd[1][y * 4 + x][0]) +
            abs(mi_->mvd[1][y * 4 + x][1]);
    return 17 + (m > 0) + (m > 8);
  }
  // The context of a block of coefficients as block() codes it: what the
  // families of its stages (cbf, sig, lvl, sgn) read of the block, its
  // prediction and the pictures, and what the stages coded
  struct BlockCx {
    int cat, idx, n;
    Geo g;
    Scan sg;
    Nb na, nb;      // the blocks A and B
    int q, nc, md;  // the QP / 6, nC's class, the prediction's mode (block_mode)
    // the prediction: which way its texture runs (hv), how much of it there
    // is (pga; pg against the quantizer's step), how far it is from
    // continuing its surroundings (em)
    int hv, pga, pg, em;
    // an intra block's proxy (pok): what its levels would be were the
    // source what the reference shows, by scan position in quarters (plv);
    // the nonzero of their estimates from each position on (esuf; pnz the
    // block's); how far to trust it (pq)
    bool pok;
    int plv[64], pnz, esuf[65], pq;
    // a luma block of a P picture on its first reference (tok): the block
    // its vector points at in the last picture of the kind (tcol, its place
    // tbx, tby in it; tmv: the vector moves it), its nonzero coefficients
    // (tnz; tany: any)
    bool tok, tany;
    int tmv, tbx, tby, tnz;
    MbRef tcol;
    // as coded: the count, the significance, the last, the magnitudes
    int total, last;
    bool sig[64];
    int mag[64];
    // the proxy's level at scan position j, in quarters, and as the level
    // an encoder would code
    int q4(int j) const { return plv[Min(j + g.off, g.kind == 3 ? 63 : g.kind == 0 && g.plane ? 3 : 15)]; }
    int est(int j) const {
      int a = (3 * abs(q4(j)) + 4) / 12;
      return q4(j) < 0 ? -a : a;
    }
    // the coefficient at scan position j of the block under the vector
    int tcf(int j) const {
      if (!tany) return 0;
      int k = j + g.off;
      if (g.kind == 3) {
        int r = k & 3, x = tbx + (r & 1), y = tby + (r >> 1);
        return (int)tcol.coef[y * 4 + x][k >> 2];
      }
      return (int)tcol.coef[tby * 4 + tbx][k];
    }
  };
  // a block of coefficients (c, n of them; coded: as coded_block_flag)
  void block(int cat, int idx, int* c, int n, bool coded) {
    prepare();
    if (cat == 2 || cat == 5) rec_.luma_ready(idx, mi_->qp - qp_off_);
    if (cat == 4 && sc_->chroma_ == 1) rec_.chroma_ready(idx >> 2 & 1, idx & 3, mi_->qp - qp_off_);
    BlockCx b;
    block_begin(b, cat, idx, n);
    block_proxy(b);
    block_col(b);
    b.total = block_count(b, c, coded);
    if (b.total) {
      // what the samples around the block ask of it (sign prediction, and
      // the level estimates)
      SignPred sp;
      sp.Init();
      bool spok = LV::signpred && cat <= 5 && (cat != 3 || sc_->chroma_ == 1) && sp.init(rec_, b.g, cat, mi_->qp - qp_off_, idx);
      block_sig(b, c, spok, sp);
      block_levels(b, c, spok, sp);
      block_signs(b, c, spok, sp);
    } else {
      for (int i = 0; i < n; i++) c[i] = 0;
    }
    store_coef(b.g, c, n);
    if (b.total) rec_.coef(cat, idx, c, n);
    rec_.luma_add(cat, idx, mi_->qp - qp_off_);
    if (cat == 4 && sc_->chroma_ == 1) rec_.chroma_add(idx >> 2 & 1, idx & 3, mi_->qp - qp_off_);
  }
  // block(): the block's place and neighbours, and what its prediction is
  // like
  INLINE void block_begin(BlockCx& b, int cat, int idx, int n) {
    b.cat = cat, b.idx = idx, b.n = n;
    b.g = geo(cat, idx);
    const Geo& g = b.g;
    b.sg = scan_of(g);
    b.na = nb_block(g, 0), b.nb = nb_block(g, 1);
    b.q = q6();
    b.nc = nc_class(block_nc(g)), b.md = block_mode(g);
    // the texture of the block's prediction
    int gxy[2] = {0, 0};
    int pt = g.kind == 0 ? rec_.pred_texture(g.plane, 0, 0, g.plane ? 8 : 16, gxy)
             : g.kind == 2 ? rec_.pred_texture(g.plane, g.x * 4, g.y * 4, 4, gxy)
                           : rec_.pred_texture(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4, gxy);
    // which way it runs (a motion error along it leaves a residual of
    // those frequencies): 4 + log2 of the differences along rows over those
    // along columns, -3 to 3 (0 flat or none)
    b.hv = 0;
    if (pt > 0) {
      int x = gxy[0] + 1, y = gxy[1] + 1, l = 0;
      while (x >= 2 * y && l < 3) x /= 2, l++;
      while (y >= 2 * x && l > -3) y /= 2, l--;
      b.hv = 4 + l;
    }
    // (for the count and the significance, against the quantizer's step)
    b.pga = qlog1(pt, 15);
    int qs = qstep16(mi_->qp - qp_off_);
    b.pg = qlog1(pt, 15, 16, qs);
    // and how far it is from continuing its surroundings (against the step
    // too)
    int ms = g.kind == 0 ? rec_.edge_miss(g.plane, 0, 0, g.plane ? 8 : 16)
             : rec_.edge_miss(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4);
    b.em = qlog1(ms, 15, 256, qs);
  }
  // block(): an intra block's proxy levels (BlockCx::pok)
  INLINE void block_proxy(BlockCx& b) {
    const Geo& g = b.g;
    b.pok = false;
    if (mi_->cls >= CL_INXN && mi_->cls != CL_PCM && sc_->chroma_ == 1) {
      proxy_mv();
      int qp = mi_->qp - qp_off_;
      // the proxy again: before the chroma against the luma, decoded by
      // then (that alone), before each later luma block of an Intra4x4/8x8
      // macroblock with the blocks decoded so far added to the template
      if ((int)pxv_.size() == size_ * 2 && pxv_[addr_ * 2] != INT16_MIN) {
        int inner = 0;
        if (g.plane && !prx_chroma_) {
          prx_chroma_ = true;
          rec_.luma_finish(qp, mi_->t8);
          inner = 1;
        } else if (!g.plane && mi_->cls == CL_INXN && (b.cat == 2 || b.cat == 5) && b.idx > prx_blk_) {
          prx_blk_ = b.idx;
          inner = 2;
        }
        int p[2] = {prx_v_[0], prx_v_[1]};
        if (inner && tm_search_(-1, 0, 0, 0, 16, 16, p, false, nullptr, 0, nullptr, 0, 1, inner)) {
          proxy_build_(p);
          prx_v_[0] = p[0];
          prx_v_[1] = p[1];
        }
      }
      if (g.kind == 0)
        b.pok = proxy_dc_(g.plane, g.plane ? rec_.chroma_qp_of(g.plane - 1, qp) : qp, b.plv);
      else
        b.pok = proxy_levels_(g.kind == 2 ? g.plane : 0, g.x * 4, g.y * 4,
                              g.kind == 2 ? rec_.chroma_qp_of(g.plane - 1, qp) : qp, b.plv, g.kind == 3 ? 8 : 4);
    }
    // (their estimates' nonzero from each position on)
    b.pnz = 0;
    b.esuf[b.n] = 0;
    if (LV::proxy && b.pok) {
      for (int j = b.n - 1; j >= 0; j--) b.esuf[j] = b.esuf[j + 1] + (b.est(j) != 0);
      b.pnz = b.esuf[0];
    }
  }
  // block(): a luma block of a P picture on its first reference: the block
  // its vector points at in the last picture of the kind (BlockCx::tok)
  INLINE void block_col(BlockCx& b) {
    const Geo& g = b.g;
    b.tmv = b.tbx = b.tby = 0;
    b.tcol = MbRef();
    b.tok = g.plane == 0 && (g.kind == 1 || g.kind == 3) &&
            mc_col(g.x, g.y, g.kind == 3 ? 8 : 4, b.tbx, b.tby, b.tmv, b.tcol) && b.tcol.cls < CL_INXN;
    // (where none of its rows were written, all zero)
    b.tany = b.tok && b.tcol.cm >> (b.tby * 4 + b.tbx) & (g.kind == 3 ? 0x33 : 1);
    b.tnz = 0;
    if (b.tany)
      for (int j = 0; j < b.n; j++) b.tnz += b.tcf(j) != 0;
  }

  // --- the cbf family: the number of a block's nonzero coefficients
  INLINE int block_count(BlockCx& b, const int* c, bool coded) {
    const Geo& g = b.g;
    // (the proxy and the reference's block where the level has them: else
    // constants false)
    const bool pok = LV::proxy && b.pok, tok = (LV::refs || LV::recon) && b.tok;
    tc_stage = STG_CBF;
    H_cbf_v D = {};
    D.pz = pok ? 1 + Min(b.pnz, 15) : 0;
    D.hv = b.hv;
    D.ch = cnth_ & 15;  // the counts of the last two blocks coded (up to 3 each)
    // how far to trust the proxy: how far the macroblock's luma decoded so
    // far is from it (13-24), else how well its vector matched (1-12)
    b.pq = 0;
    if (pok) {
      int pa = proxy_acc_();
      b.pq = pa >= 0 ? 13 + tc_clamp(tc_qlog(pa), 0, 11) : pxq_;
    }
    D.pq = b.pq;
    D.tn = tok ? 1 + 5 * b.tmv + Min(b.tnz, 4) : 0;
    D.cat = b.cat;
    D.na = nb_count(b.na, g);
    D.nb = nb_count(b.nb, g);
    D.intra = mi_->cls >= CL_INXN;
    D.q6 = b.q;
    D.np = cbf_np(g, b.idx);
    D.pos = g.pos;
    D.ds = cbf_ds(g, b.cat, b.idx);
    D.ly = cbf_ly(g);
    D.dq = cbf_dq(g, b.cat);
    D.crcb = cbf_crcb(g);
    D.col = cbf_col(g);
    D.nc = b.nc;
    D.kn = coded;
    D.md = b.md;
    D.en = map_en();
    D.pg = b.pg;
    D.em = b.em;
    tcx X;
    f_cbf.make(D, X);
    NodePrp pr;
    pr.Init();
    if (!coded && sc_->cabac_) {
      int inc = g.kind == 0 ? sc_->cbf_inc_dc(g.plane)
                : g.kind == 2 ? sc_->cbf_inc_chroma_ac(g.plane - 1, g.x, g.y)
                : g.kind == 3 ? sc_->cbf_inc_8x8(g.plane, b.idx)
                              : sc_->cbf_inc_luma(g.plane, g.x, g.y);
      pr[0] = cab1(kCbfBase[b.cat] + inc);
    }
    f_cbf.prpa = pr;
    // the intra estimate's count, or that of the block under the vector
    f_cbf.hint = pok ? Max(b.pnz - (int)coded, 0) : tok ? Max(b.tnz - (int)coded, 0) : -1;
    int total = cm_int(f_cbf.code(X, ENC ? count_nonzero(c, b.n) - coded : 0), b.n) + coded;
    f_cbf.hint = -1;
    cnth_ = (cnth_ * 4 + Min(total, 3)) & 15;
    f_cbf.prpa = nullptr;
    if (total > b.n) cm_fail("corrupt stream: %d coefficients in a block of %d", total, b.n);
    tc_syms[STG_CBF]++;
    return total;
  }
  // the blocks before it with coefficients, in its 8x8 block (4x4 blocks)
  // or its component (chroma AC)
  int cbf_np(const Geo& g, int idx) const {
    const MbState& cur = *sc_->cur_;
    int np = 0;
    if (g.kind == 1)
      for (int b = idx & ~3; b < idx; b++) np += cur.nz[g.plane][blk_y(b) * 4 + blk_x(b)] != 0;
    else if (g.kind == 2)
      for (int b = idx - idx % sc_->ch_blocks_; b < idx; b++) np += cur.nz[g.plane][b % sc_->ch_blocks_] != 0;
    return np;
  }
  // x264's decimation (an inter luma 4x4 block of a P or B slice): an 8x8
  // block whose 4x4 blocks score under 4 (6 when it is the macroblock's
  // only one) is zeroed, so a coded one's blocks make that up; 1 + what is
  // still owed after the blocks before this one (0 not such a block)
  int cbf_ds(const Geo& g, int cat, int idx) const {
    if (g.kind != 1 || g.plane != 0 || cat != 2 || mi_->cls >= CL_INXN || (sc_->stype_ != kP && sc_->stype_ != kB)) return 0;
    int score = 0, req = popc4(mi_->cbp & 15) == 1 ? 6 : 4;
    for (int b = idx & ~3; b < idx; b++) {
      int r = blk_y(b) * 4 + blk_x(b);
      if (mi_->cmask >> r & 1) score += decimate_score(mi_->coef[0][r]);
    }
    return 1 + Max(0, req - score);
  }
  // a 4:2:0 chroma block: the coefficients of the luma under it (its 8x8
  // block; for DC, the macroblock's), 1 + tc_qlog (0 luma)
  int cbf_ly(const Geo& g) const {
    if (!g.plane || sc_->chroma_ != 1) return 0;
    const MbState& cur = *sc_->cur_;
    int n = 0;
    for (int b = 0; b < 16; b++)
      if (g.kind == 0 || ((b & 3) >> 1 == g.x && (b >> 2) >> 1 == g.y)) n += cur.nz[0][b];
    return 1 + Min(tc_qlog(n), 9);
  }
  // a 4:2:0 chroma AC block: its DC, from the chroma DC levels' 2x2
  // transform; an Intra16x16 luma AC block, from the luma DC levels' 4x4
  // one (frame scan); 1 + tc_qlog (0 not such a block)
  int cbf_dq(const Geo& g, int cat) const {
    if (g.kind == 2 && sc_->chroma_ == 1) {
      static const int8_t z[16] = {};
      const int8_t* d = mi_->cmask >> (48 + g.plane) & 1 ? mi_->dc[g.plane] : z;
      return 1 + Min(tc_qlog(d[0] + (g.x ? -d[1] : d[1]) + (g.y ? -d[2] : d[2]) + (g.x ^ g.y ? -d[3] : d[3])), 7);
    }
    if (cat != 1) return 0;
    static const int kH[4][4] = {{1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1}};
    const int8_t* d = mi_->dc[0];
    int v = 0;
    if (mi_->cmask >> 48 & 1)
      for (int k = 0; k < 16; k++)
        if (d[k]) v += kH[g.y][kZz4[k] >> 2] * d[k] * kH[kZz4[k] & 3][g.x];
    return 1 + Min(tc_qlog(v), 7);
  }
  // a Cr block: 1 + whether the Cb block there has coefficients (0 other)
  int cbf_crcb(const Geo& g) const {
    const MbState& cur = *sc_->cur_;
    if (g.plane != 2) return 0;
    if (sc_->chroma_ != 3) return 1 + (g.kind == 0 ? cur.cbf_dc[1] : cur.nz[1][g.y * 2 + g.x] != 0);
    return 1 + (g.kind == 0 ? cur.cbf_dc[1] : cur.nz[1][g.y * 4 + g.x] != 0);
  }
  // whether the block had coefficients in the last macroblock of the kind
  // there (col_; 2 none)
  int cbf_col(const Geo& g) const {
    if (!col_) return 2;
    return g.kind == 0 ? col_->dcm >> g.plane & 1 : col_->nzm[g.plane] >> (g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x) & 1;
  }

  // --- the sig family: where a block's nonzero coefficients are, per
  // position whether the coefficient is nonzero, knowing how many are still
  // to come (into BlockCx::sig, last)
  INLINE void block_sig(BlockCx& b, const int* c, bool spok, SignPred& sp) {
    const Geo& g = b.g;
    const Scan& sg = b.sg;
    const Nb &na = b.na, &nb = b.nb;
    const int cat = b.cat, n = b.n;
    const bool pok = LV::proxy && b.pok, tok = (LV::refs || LV::recon) && b.tok;  // (as block_count)
    // the prediction's own levels by position (its texture by frequency):
    // an inter residual's follow them
    int pfl[64];
    bool pfok = mi_->cls < CL_INXN && g.kind != 0 &&
                rec_.pred_levels(g.plane, g.x * 4, g.y * 4,
                                 g.plane ? rec_.chroma_qp_of(g.plane - 1, mi_->qp - qp_off_) : mi_->qp - qp_off_, pfl,
                                 g.kind == 3 ? 8 : 4);
    tc_stage = STG_SIG;
    bool* sig = b.sig;
    memset(sig, 0, sizeof b.sig);
    int rem = b.total, last = 0;
    H_sig_v S = {};
    S.cg = cat;
    S.md = b.md;
    S.nc = b.nc;
    S.fld = sc_->field_mb();
    S.q6 = b.q;
    S.pg = b.pg;
    S.em = b.em;
    S.pq = b.pq;
    for (int i = 0; rem; i++) {
      if (n - i == rem) {
        sig[i] = true;
      } else {
        S.pos = i;
        S.rem = tc_qlog(rem);
        S.zl = tc_qlog(n - i - rem);
        S.pe = spok && i < 4 ? sp.estimate(i) : 0;
        S.ps = pok ? 1 + Min(abs(b.q4(i)), 6) : 0;
        // the estimate's levels from here on, against the coefficients left
        S.pr = pok ? 1 + 3 + tc_clamp(b.esuf[i] - rem, -3, 3) : 0;
        // the last two positions' significance (4 at the start)
        if (H_USES(sig, ft)) {
          // the frequencies left of and above it, where coded already (0
          // not, 1 zero, 2 not zero)
          int r = sg.zz[i + g.off], fx = r % sg.w, fy = r / sg.w, fl = 0, fa = 0;
          auto known = [&](int rr) {
            int jj = sg.inv[rr] - g.off;
            return jj >= 0 && jj < i ? 1 + sig[jj] : 0;
          };
          if (fx) fl = known(r - 1);
          if (fy) fa = known(r - sg.w);
          S.ft = fl * 3 + fa;
        }
        if (H_USES(sig, hb)) {
          // the significance of the last eight positions (and where the
          // block starts)
          int h = 1;
          for (int k = 1; k <= 8 && k <= i; k++) h = h * 2 + sig[i - k];
          S.hb = h - 1;
        }
        if (H_USES(sig, h3)) {
          // and of the last three
          int h3 = 1;
          for (int k = 1; k <= 3 && k <= i; k++) h3 = h3 * 2 + sig[i - k];
          S.h3 = h3 - 1;
        }
        S.tz = tok ? 1 + 4 * b.tmv + Min(abs(b.tcf(i)), 3) : 0;
        S.hv = b.hv;
        S.pf = pfok ? 1 + Min(tc_qlog(abs(pfl[Min(i + g.off, g.kind == 3 ? 63 : 15)])), 10) : 0;
        // the same position in the blocks A and B (0 none, 1 zero, 2 not)
        if (H_USES(sig, sn))
          S.sn = (na.m ? 1 + (coef_at(na.m, g, na.i, i) != 0) : 0) * 3 + (nb.m ? 1 + (coef_at(nb.m, g, nb.i, i) != 0) : 0);
        tcx Y;
        f_sig.make(S, Y);
        f_sig.select(Y);
        f_sig.prp = cab0(sc_->ctx_sig(cat, i));
        sig[i] = f_sig.bit(0, ENC && c[i] != 0);
        tc_syms[STG_SIG]++;
      }
      if (sig[i]) {
        rem--;
        last = i;
      }
    }
    b.last = last;
  }

  // --- the lvl family: the magnitudes of a block's nonzero coefficients,
  // from the last back (into BlockCx::mag)
  INLINE void block_levels(BlockCx& b, const int* c, bool spok, SignPred& sp) {
    const Geo& g = b.g;
    const Scan& sg = b.sg;
    const Nb &na = b.na, &nb = b.nb;
    const int cat = b.cat, n = b.n, last = b.last;
    const bool* sig = b.sig;
    int* mag = b.mag;
    const bool pok = LV::proxy && b.pok, tok = (LV::refs || LV::recon) && b.tok;  // (as block_count)
    tc_stage = STG_LVL;
    int g1 = 0, e1 = 0, lhist = 0, lsum = 0, esum = 0;  // lhist: the last levels coded less 1 (up to 3), two bits each
    H_lvl_v L = {};
    L.cg = cat;
    L.nc = b.nc;
    L.q6 = b.q;
    L.tot = tc_qlog(b.total);
    L.md = b.md;
    L.pg = b.pga;
    L.em = b.em;
    L.pq = b.pq;
    NodePrp pr;
    pr.Init();
    f_lvl.prpa = pr;
    for (int k = last; k >= 0; k--) {
      if (!sig[k]) continue;
      L.g1 = g1;
      L.e1 = e1;
      pr[0] = cab1(SC::ctx_abs0(cat, g1, e1));
      pr[1] = pr[2] = cab1(SC::ctx_abs1(cat, g1));
      L.pos = tc_qlog(k);
      {
        // the texture's direction, and whether the coefficient is a
        // horizontal (1) or vertical (2) frequency or both (3) or DC (0)
        int r = sg.zz[k + g.off];
        L.hv = b.hv;
        L.fq = (r % sg.w > 0) + 2 * (r / sg.w > 0);
      }
      L.pe = spok && k < 4 ? sp.estimate(k) : 0;
      L.pl = pok ? 1 + Min(abs(b.q4(k)) / 2, 10) : 0;
      // intra: the levels so far against the estimate's there
      L.ls = pok ? 7 + tc_clamp(lsum - esum, -6, 6) : 0;
      {
        // the levels right of and below it (higher frequencies, coded already)
        int r = sg.zz[k + g.off], fx = r % sg.w, fy = r / sg.w, sum = 0, nn = 0;
        auto lv = [&](int rr) {
          int j = sg.inv[rr] - g.off;
          if (j > k && j <= last && sig[j]) sum += mag[j];  // none significant beyond last
          nn += j > k && j < n;
        };
        if (fx + 1 < sg.w) lv(r + 1);
        if (fy + 1 < sg.h) lv(r + sg.w);
        L.lt = nn ? 1 + Min(sum, 6) : 0;
      }
      L.tz = tok ? 1 + 4 * b.tmv + Min(abs(b.tcf(k)), 3) : 0;
      L.sa = na.m ? Min(abs(coef_at(na.m, g, na.i, k)), 3) : 0;
      L.sb = nb.m ? Min(abs(coef_at(nb.m, g, nb.i, k)), 3) : 0;
      tcx X;
      f_lvl.make(L, X);
      // the intra estimate's level there
      f_lvl.hint = pok ? Max(abs(b.est(k)) - 1, 0) : -1;
      int a = cm_int(f_lvl.code(X, ENC ? abs(c[k]) - 1 : 0), kMaxLevel - 1);
      f_lvl.hint = -1;
      mag[k] = a + 1;
      lsum += a + 1;
      if (pok) esum += abs(b.est(k));
      L.pm = Min(a + 1, 4);
      lhist = (lhist * 4 + Min(a, 3)) & 63;
      L.lh = 1 + lhist;
      if (a)
        g1++;
      else
        e1++;
      tc_syms[STG_LVL]++;
    }
    f_lvl.prpa = nullptr;
  }

  // --- the sgn family: the signs of a block's nonzero coefficients. Of a
  // luma block's first ones in scan order (sign prediction: spok), coded
  // after the others, each with the sign that makes the block continue its
  // surroundings best; of an inter luma block's, with the shift of its
  // prediction its residual looks like, fitted to the signs known so far
  // (from the macroblock's last block before any)
  struct SgnCx {
    bool gok;              // the prediction's gradients' levels (glx, gly) there
    int glx[64], gly[64];
    ShiftFit fit;
    int ghist;             // the last two signs coded in the block (0 none, 1 +, 2 -)
    int sagree;            // the signs the intra estimate got right less those it got wrong
    void Init() {
      fit.Init();
      ghist = sagree = 0;
    }
  };
  INLINE void block_signs(BlockCx& b, int* c, bool spok, SignPred& sp) {
    const Geo& g = b.g;
    tc_stage = STG_SGN;
    H_sgn_v G = {};
    G.cg = b.cat;
    G.scb = 3;
    G.md = b.md;
    G.pq = b.pq;
    int npred = 0, pk[6];
    if (spok)
      for (int k = 0; k <= b.last && npred < 6; k++)
        if (b.sig[k]) pk[npred++] = k;
    SgnCx s;
    s.Init();
    s.gok = mi_->cls < CL_INXN && g.plane == 0 && (b.cat == 2 || b.cat == 5) &&
            rec_.pred_grad_levels(0, g.x * 4, g.y * 4, b.cat == 5 ? 8 : 4, mi_->qp - qp_off_, s.glx, s.gly);
    if (s.gok)
      for (int k = 0; k < b.n; k++) s.glx[k] = tc_clamp(s.glx[k], -2047, 2047), s.gly[k] = tc_clamp(s.gly[k], -2047, 2047);
    for (int k = b.last; k >= 0; k--) {
      if (!b.sig[k]) {
        if (!ENC) c[k] = 0;
        continue;
      }
      bool predicted = false;
      for (int j = 0; j < npred; j++) predicted |= pk[j] == k;
      if (!predicted) {
        sign1(b, s, G, c, k);
        if (npred) sp.add(k, c[k]);
      }
    }
    if (npred) {
      for (int j = 0; j < npred; j++) sp.cand(pk[j], b.mag[pk[j]]);
      sp.costs();
      for (int j = 0; j < npred; j++) {
        int margin, pred = sp.predict(j, margin);
        G.pp = 1 + pred;
        G.mg = 1 + tc_clamp(tc_qlog(margin), 0, 14);
        sp.decide(j, sign1(b, s, G, c, pk[j]));
      }
      G.pp = G.mg = 0;
    }
    for (int k = b.last + 1; k < b.n; k++) c[k] = 0;
    if (s.gok) {
      // the shift fitted to the whole block, for the next block's first signs
      i64 fx, fy, fd;
      s.fit.fit(fx, fy, fd);
      if (fd) shx_ = fx, shy_ = fy, shd_ = fd;
    }
  }
  // the sign of the coefficient at scan position k: coded, its value into c
  INLINE int sign1(const BlockCx& b, SgnCx& s, H_sgn_v& G, int* c, int k) {
    const Geo& g = b.g;
    const bool pok = LV::proxy && b.pok, tok = (LV::refs || LV::recon) && b.tok;  // (as block_count)
    G.pos = k == 0 ? 0 : k < 3 ? 1 : k < 6 ? 2 : 3;
    G.sa = b.na.m ? sgn3(coef_at(b.na.m, g, b.na.i, k)) : 3;
    G.sb = b.nb.m ? sgn3(coef_at(b.nb.m, g, b.nb.i, k)) : 3;
    if (g.plane == 2) {
      Geo gb = g;
      gb.plane = 1;
      G.scb = sgn3(coef_at(mi_, gb, g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x, k));
    }
    G.mag = Min(b.mag[k], 3) - 1;
    G.ez = pok ? (b.q4(k) ? (b.q4(k) > 0 ? 2 : 3) + 2 * (abs(b.q4(k)) >= 4) : 1) : 0;
    G.ts = tok ? 1 + 3 * b.tmv + sgn3(b.tcf(k)) : 0;
    G.gh = s.ghist % 9;
    // intra: how often the estimate's signs were right in the block
    G.se = pok ? 4 + tc_clamp(s.sagree, -3, 3) : 0;
    // what the shift says of this sign, and how surely (its level against a
    // quarter of the coefficient's, and against all of it)
    G.dm = 0;
    if (s.gok) {
      i64 fx, fy, fd;
      s.fit.fit(fx, fy, fd);
      if (!fd) fx = shx_, fy = shy_, fd = shd_;
      i64 r = fx * s.glx[k] + fy * s.gly[k], ra = r < 0 ? -r : r, m = Min(b.mag[k], 4096);
      if (fd && r) G.dm = 1 + (r < 0) + 2 * (ra > fd * m) + 2 * (ra > 4 * fd * m);
    }
    tcx X;
    f_sgn.make(G, X);
    f_sgn.select(X);
    int sg = f_sgn.bit(0, ENC && c[k] < 0);
    s.ghist = s.ghist % 3 * 3 + 1 + sg;
    if (pok && b.q4(k)) s.sagree += (b.q4(k) < 0) == (sg != 0) ? 1 : -1;
    c[k] = sg ? -b.mag[k] : b.mag[k];
    if (s.gok) s.fit.add(s.glx[k], s.gly[k], tc_clamp(4 * c[k], -4095, 4095));
    tc_syms[STG_SGN]++;
    return sg;
  }

  // --- I_PCM: the alignment bits, and the samples as differences to the
  // one to the left (above, in the first column), modulo the bit depth: the
  // pcm family
  void pcm(int& align, int nb, int* s, int n) {
    align = (int)nal(F_ALIGN, nb, ENC ? align : 0);
    tc_stage = STG_PCM;
    int ch = sc_->chroma_;
    int cw = ch == 3 ? 16 : 8, chh = ch == 1 ? 8 : 16;
    int i = 0;
    for (int p = 0; p < (ch ? 3 : 1); p++) {
      int w = p ? cw : 16, hh = p ? chh : 16;
      int depth = p ? sc_->sps_->bit_depth_chroma : sc_->sps_->bit_depth_luma;
      i64 d1 = 0, d2 = 0;
      for (int y = 0; y < hh; y++)
        for (int x = 0; x < w; x++, i++) {
          if (i >= n) cm_fail("internal: I_PCM sample count");
          int pred = x ? s[i - 1] : y ? s[i - w] : 1 << (depth - 1);
          int half = 1 << (depth - 1), mask = (1 << depth) - 1;
          H_pcm_v D;
          D.pl = p;
          D.dp = depth - 8;
          D.pd = sq2(d1);
          D.pd2 = sq2(d2);
          tcx X;
          f_pcm.make(D, X);
          i64 d = f_pcm.codes(X, ENC ? ((s[i] - pred + half) & mask) - half : 0, (u32)p);
          s[i] = (pred + cm_int(d, 1 << 16)) & mask;
          d2 = d1;
          d1 = d;
          tc_syms[STG_PCM]++;
        }
    }
    if (i != n) cm_fail("internal: I_PCM sample count");
    mi_->cbp = 0x2f;
    for (int k = 0; k < n && k < 384; k++) rec_.pcm_s[k] = s[k];
  }
};
#undef H_USES



// The channel between the walk and the model. Coding, it takes each value
// from the log of the dry run and has the model code it; decoding, the
// model decodes it. Either way the value goes back to the walk.
template <int f_DEC, class LV>
struct ModelChan : Chan {
  static constexpr int ENC = f_DEC == 0;
  // (log: coding, the dry run's values)
  void Init(Model<f_DEC, LV>& m, LogChan* log) {
    W = true;
    m_ = &m;
    log_ = log;
  }
  // how the walk codes the bins with the model: coding, they are dropped,
  // so it need not code them (where the level reads the CABAC states,
  // those still follow the bins); decoding, they are the stream's
  static BinPolicy policy() {
    BinPolicy p;
    p.Init();
    p.nobins = ENC;
    p.states = ENC && LV::cabpred;
    return p;
  }

  void hbegin(const char* name) override { m_->hbegin(name); }
  void hbits(int n) override { m_->hbits(n); }
  int64_t hval(int64_t) override { return m_->hval(ENC ? log_->hval(0) : 0); }
  void hbytes(uint8_t* b, size_t n) override {
    if (ENC) log_->hbytes(b, n);
    for (size_t i = 0; i < n; i++) b[i] = (uint8_t)m_->raw(b[i]);
  }
  void hend() override {}
  bool hmore(const char* next, bool) override { return m_->hmore(next, ENC && log_->hmore(next, false)); }
  void slice_begin(WalkState& ws) override { m_->slice_begin(ws); }
  void slice_end() override { m_->slice_end(); }
  int skip(int) override { return m_->skip(ENC ? log_->skip(0) : 0); }
  int end(int, bool certain) override { return m_->end(ENC ? log_->end(0, certain) : 0, certain); }
  void mb_begin() override { m_->mb_begin(); }
  void mb_end() override { m_->mb_end(); }
  int field(int) override { return m_->field(ENC ? log_->field(0) : 0); }
  int mb_type(int) override { return m_->mb_type(ENC ? log_->mb_type(0) : 0); }
  int t8x8(int) override { return m_->t8x8(ENC ? log_->t8x8(0) : 0); }
  int ipm(int blk, int) override { return m_->ipm(blk, ENC ? log_->ipm(blk, 0) : 0); }
  int cpred(int) override { return m_->cpred(ENC ? log_->cpred(0) : 0); }
  int sub(int i, int) override { return m_->sub(i, ENC ? log_->sub(i, 0) : 0); }
  int ref(int list, int x, int y, int w, int h, int) override {
    return m_->ref(list, x, y, w, h, ENC ? log_->ref(list, x, y, w, h, 0) : 0);
  }
  int mvd(int list, int comp, int x, int y, int w, int h, int) override {
    return m_->mvd(list, comp, x, y, w, h, ENC ? log_->mvd(list, comp, x, y, w, h, 0) : 0);
  }
  int cbp(int) override { return m_->cbp(ENC ? log_->cbp(0) : 0); }
  int qpd(int) override { return m_->qpd(ENC ? log_->qpd(0) : 0); }
  void block(int cat, int idx, int* c, int n, bool coded) override {
    if (ENC) log_->block(cat, idx, c, n, coded);
    m_->block(cat, idx, c, n, coded);
  }
  void pcm(int& align, int& nb, int* s, int n) override {
    if (ENC) log_->pcm(align, nb, s, n);
    m_->pcm(align, nb, s, n);
  }
  [[noreturn]] void fail(const char* why) override { err_raise("%s%s", ENC ? "internal: " : "corrupt stream: ", why); }

  Model<f_DEC, LV>* m_;
  LogChan* log_;
};

}  // namespace h264p
