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

namespace h264p {

// --- the families: decisions per context (ND, mantissa nodes MND), sign
// contexts (NG), and the variables of each one's IDX/<family>.idx
constexpr int H_nal_ND = TC_NODE, H_nal_MND = TC_MNODE, H_nal_NG = 32;
constexpr int H_hdr_ND = TC_NODE, H_hdr_MND = TC_MNODE, H_hdr_NG = 256;
constexpr int H_skp_ND = 3, H_skp_MND = 1, H_skp_NG = 1;
constexpr int H_mbt_ND = 29, H_mbt_MND = 1, H_mbt_NG = 1;
constexpr int H_cbp_ND = 7, H_cbp_MND = 1, H_cbp_NG = 1;
constexpr int H_ipm_ND = 19, H_ipm_MND = 1, H_ipm_NG = 1;
constexpr int H_mvs_ND = 16, H_mvs_MND = TC_MNODE, H_mvs_NG = 1;  // sub()'s tree of B sub_mb_type: nodes 1-15
constexpr int H_qpd_ND = TC_NODE, H_qpd_MND = TC_MNODE, H_qpd_NG = 1;
constexpr int H_cbf_ND = TC_NODE, H_cbf_MND = TC_MNODE, H_cbf_NG = 1;
constexpr int H_sig_ND = 1, H_sig_MND = 1, H_sig_NG = 1;
constexpr int H_lvl_ND = TC_NODE, H_lvl_MND = TC_MNODE, H_lvl_NG = 1;
constexpr int H_sgn_ND = 1, H_sgn_MND = 1, H_sgn_NG = 1;
constexpr int H_mvg_ND = 1, H_mvg_MND = 1, H_mvg_NG = 1;
constexpr int H_qpg_ND = 1, H_qpg_MND = 1, H_qpg_NG = 1;
constexpr int H_pcm_ND = TC_NODE, H_pcm_MND = TC_MNODE, H_pcm_NG = 3;

struct H_nal_v { int fld, ctx, p1, p2; };
struct H_hdr_v { int tag, fld, p1, p2, pos, p1r, sty, pr; };
struct H_skp_v { int st, a3, b3, col, c9, cd, run, cnt, eq, lf, af, td, en, sm, tm, tb, bd, qc, c1, k9, tx; };
struct H_mbt_v { int st, ca, cb, cc, nsk, en, td, sm, tm, tb, dh, bd, qc, im, ig, c1, tx; };
struct H_cbp_v { int inc, pc, cls, col, q6, st, en, td, pg, tp, em, pq; };
struct H_ipm_v { int k8, ma, mb, mp, blk, pv, col, dm, ds, pm, pr, dr, pq, mh, mc, mg, rg, p2; };
struct H_mvs_v { int kind, comp, lst, ma, mb, oth, ps, rf, pi, nr, pd, sp, tv, zd, cq, tq, rt, ea, eb, ta; };
struct H_qpd_v { int lnz, pc, cc, cls, dc, db, lz, st, ae, aq, ac, td, sq, xe, lq, bc, cl, kc; };
struct H_cbf_v { int cat, na, nb, intra, q6, np, pos, crcb, col, nc, kn, md, en, pg, em, pz, tn, pq, ch, ds, hv, ly, dq; };
struct H_sig_v { int cg, pos, rem, zl, md, nc, fld, q6, pg, em, pe, ps, tz, pq, hb, ft, h3, pr, sn, hv, pf; };
struct H_lvl_v { int cg, g1, e1, nc, pos, sa, sb, q6, tot, md, pm, pg, em, pe, pl, tz, pq, lh, lt, ls, hv, fq; };
struct H_sgn_v { int cg, pos, sa, sb, scb, mag, md, pp, mg, ez, ts, pq, gh, se, dm; };
struct H_mvg_v { int comp, lst, ts, zs, ds, sa, sb, mag, tq, ox, ns, es, as; };
struct H_qpg_v { int as, cs, bs, ls, rs, mag, cls, st, ss, xe, lq, bk; };
struct H_pcm_v { int pl, dp, pd, pd2; };

#include "MOD/nal_h.inc"
#include "MOD/hdr_h.inc"
#include "MOD/skp_h.inc"
#include "MOD/mbt_h.inc"
#include "MOD/cbp_h.inc"
#include "MOD/ipm_h.inc"
#include "MOD/mvs_h.inc"
#include "MOD/qpd_h.inc"
#include "MOD/cbf_h.inc"
#include "MOD/sig_h.inc"
#include "MOD/lvl_h.inc"
#include "MOD/sgn_h.inc"
#include "MOD/mvg_h.inc"
#include "MOD/qpg_h.inc"
#include "MOD/pcm_h.inc"

#include "MOD/nal_p.inc"
#include "MOD/hdr_p.inc"
#include "MOD/skp_p.inc"
#include "MOD/mbt_p.inc"
#include "MOD/cbp_p.inc"
#include "MOD/ipm_p.inc"
#include "MOD/mvs_p.inc"
#include "MOD/qpd_p.inc"
#include "MOD/cbf_p.inc"
#include "MOD/sig_p.inc"
#include "MOD/lvl_p.inc"
#include "MOD/sgn_p.inc"
#include "MOD/mvg_p.inc"
#include "MOD/qpg_p.inc"
#include "MOD/pcm_p.inc"

// A family's tables, mapped: zero is every table's starting state, and a
// page costs memory only once the coder reaches it.
template <class T>
static T* tc_table() {
  return new (tc_map(sizeof(T))) T;
}

#define H_WIRE(F)                                                                                                   \
  f_##F.wire(t_##F->H_##F##_A, H_##F##_a_Volume, t_##F->H_##F##_B, H_##F##_b_Volume, t_##F->H_##F##_C,              \
             H_##F##_c_Volume, t_##F->H_##F##_D, H_##F##_d_Volume, t_##F->H_##F##_E, H_##F##_e_Volume, t_##F->H_##F##_H, H_##F##_h_Volume, t_##F->H_##F##_I, H_##F##_i_Volume, t_##F->H_##F##_J, H_##F##_j_Volume, t_##F->H_##F##_K, H_##F##_k_Volume, t_##F->H_##F##_L, H_##F##_l_Volume, t_##F->H_##F##_MT, H_##F##_t_Volume,             \
             t_##F->H_##F##_S, t_##F->H_##F##_SC, H_##F##_s_Volume, t_##F->H_##F##_F, t_##F->H_##F##_FC,            \
             H_##F##_f_Volume, t_##F->H_##F##_W, H_##F##_m_Volume, t_##F->H_##F##_G, H_##F##_NG, H_##F##_g_Volume,  \
             t_##F->H_##F##_WM, H_##F##_n_Volume, t_##F->H_##F##_WC, t_##F->H_##F##_WMC, t_##F->H_##F##_W2,            \
             t_##F->H_##F##_W2C, H_##F##_w_Volume, t_##F->H_##F##_W3, t_##F->H_##F##_W3C, t_##F->H_##F##_WF, t_##F->H_##F##_WFC, H_##F##_rA, H_##F##_rB,                    \
             H_##F##_rC, H_##F##_rD, H_##F##_rE, H_##F##_rH, H_##F##_rI, H_##F##_rJ, H_##F##_rK, H_##F##_rL, H_##F##_rT, H_##F##_rG, H_##F##_mwA, H_##F##_mwB, H_##F##_mwC, H_##F##_mwD, H_##F##_mwE, H_##F##_mwH, H_##F##_mwI, H_##F##_mwJ, H_##F##_mwK, H_##F##_mwL,    \
             H_##F##_mwT, H_##F##_mwG, H_##F##_rS1, H_##F##_rS2, H_##F##_lr, H_##F##_mb, H_##F##_lrm, H_##F##_mbm,  \
             H_##F##_bw, H_##F##_mt, H_##F##_qs, H_##F##_qf, H_##F##_ND, H_##F##_MND, H_##F##_hA, H_##F##_hB, H_##F##_hD, H_##F##_hE, H_##F##_hH, H_##F##_hI, H_##F##_hJ, H_##F##_hK, H_##F##_hL,       \
             H_##F##_hS, H_##F##_hF, H_##F##_hM, H_##F##_hT, H_##F##_hN)

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
  uint8_t skip, cls, qp, cbp, t8, cpred, i16m, field;
  // the rows of coef (plane * 16 + block) and of dc (48 + plane) written;
  // the others are zero
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
static INLINE int sgn3(int v) { return v > 0 ? 1 : v < 0 ? 2 : 0; }
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
// a / b rounded down, b > 0
static INLINE i64 div_floor(i64 a, i64 b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

// x264's adaptive quantization: in a picture the QP of a macroblock is the
// picture's plus a multiple of log2 of the macroblock's energy (and, in a
// reference picture, MB-tree's offset). A least-squares line of QP against
// 16 log2 energy, through the macroblocks of the picture that coded
// mb_qp_delta (the energy of their reconstruction), predicts the QP of the
// next from the energy of its prediction.
struct AqFit {
  i64 n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
  i64 lb = 266;  // the slope the last picture's line ended with (x264's at first)
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
  return std::max(16, (k16[q % 6] << (q / 6)) >> 2);
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
  int n = 4, off = 0, qp = 0, ne = 8, nc = 0;
  bool b8 = false;
  int dc = 0;  // 1 chroma DC (2x2, over the 8x8 block), 2 Intra16x16 DC (4x4, over the 16x16 one)
  int64_t scale = 256, target[32], base[32], contrib[6][32], cost[64];
  bool avail[32];
  int fixed_mask = 0, fixed_val = 0;
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
      int dc = cat == 1 ? rec.dc16_at(q, blk) : rec.dcc_at(g.plane - 1, g.y * 2 + g.x, q);
      for (int e = 0; e < ne; e++) base[e] += (int64_t)dc * 4;
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
        c1 = std::min(c1, cost[m]);
      else
        c0 = std::min(c0, cost[m]);
    }
    int64_t d = (c0 > c1 ? c0 - c1 : c1 - c0) * 4 / scale;
    margin = (int)std::min<int64_t>(d, 1 << 20);
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
    return m2 ? 1 + std::min(tc_qlog(m2), 14) : 0;
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
  ZzInv() {
    for (int k = 0; k < 16; k++) v4[kZz4[k]] = (uint8_t)k;
    for (int k = 0; k < 64; k++) v8[kZz8[k]] = (uint8_t)k;
  }
};
static const ZzInv kZzInv;
static const uint8_t* const kInv4 = kZzInv.v4;
static const uint8_t* const kInv8 = kZzInv.v8;

template <int f_DEC>
struct Model {
  static constexpr int ENC = f_DEC == 0;
  static Rangecoder<f_DEC> rc;

  tc_fam<f_DEC> f_nal, f_hdr, f_skp, f_mbt, f_cbp, f_ipm, f_mvs, f_qpd, f_cbf, f_sig, f_lvl, f_sgn, f_mvg, f_qpg, f_pcm;
#ifndef H264P_NO_THREADS
  TcDefer* dq_ = nullptr;
  // (the compressor) the coding calls for the coder's thread q, or (null)
  // coded at once again
  void defer(TcDefer* q) {
    dq_ = q;
    for (tc_fam<f_DEC>* f : {&f_nal, &f_hdr, &f_skp, &f_mbt, &f_cbp, &f_ipm, &f_mvs, &f_qpd, &f_cbf, &f_sig, &f_lvl,
                             &f_sgn, &f_mvg, &f_qpg, &f_pcm})
      f->dq = q;
  }
#endif
  H_nal_T* t_nal;
  H_hdr_T* t_hdr;
  H_skp_T* t_skp;
  H_mbt_T* t_mbt;
  H_cbp_T* t_cbp;
  H_ipm_T* t_ipm;
  H_mvs_T* t_mvs;
  H_qpd_T* t_qpd;
  H_cbf_T* t_cbf;
  H_sig_T* t_sig;
  H_lvl_T* t_lvl;
  H_sgn_T* t_sgn;
  H_mvg_T* t_mvg;
  H_qpg_T* t_qpg;
  H_pcm_T* t_pcm;

  void init() {
    cm_tables();
    // (end_of_slice_flag 0 nearly for sure: as if seen a few times)
    for (cm_cnt& c : end_c_) c.p = (u16)(CM_CONE - 16), c.t = 8;
    for (int& p : hist_poc_) p = INT_MIN;
    std::fill(&hist_rpoc_[0][0][0], &hist_rpoc_[0][0][0] + sizeof hist_rpoc_ / sizeof(int), INT_MIN);
    std::fill(&col_rpoc_[0][0], &col_rpoc_[0][0] + sizeof col_rpoc_ / sizeof(int), INT_MIN);
#define H_MAP(F) (t_##F = tc_table<H_##F##_T>(), t_##F->H_##F##_Init(), H_WIRE(F))
    H_MAP(nal);
    H_MAP(hdr);
    H_MAP(skp);
    H_MAP(mbt);
    H_MAP(cbp);
    H_MAP(ipm);
    H_MAP(mvs);
    H_MAP(qpd);
    H_MAP(cbf);
    H_MAP(sig);
    H_MAP(lvl);
    H_MAP(sgn);
    H_MAP(mvg);
    H_MAP(qpg);
    H_MAP(pcm);
#undef H_MAP
    raw_mx_.init(raw_w_, raw_wc_, 256);
    rbuf_.clear();
  }
  u64 table_bytes() const {
    return (u64)t_nal->H_nal_Size + t_hdr->H_hdr_Size + t_skp->H_skp_Size + t_mbt->H_mbt_Size + t_cbp->H_cbp_Size +
           t_ipm->H_ipm_Size + t_mvs->H_mvs_Size + t_qpd->H_qpd_Size + t_cbf->H_cbf_Size + t_sig->H_sig_Size +
           t_lvl->H_lvl_Size + t_sgn->H_sgn_Size + t_mvg->H_mvg_Size + t_qpg->H_qpg_Size + t_pcm->H_pcm_Size;
  }

  // ------------------------------------------------------------------------
  // the container: a field of the nal family, with its own last two values
  // and a context c the caller gives
  i64 last_[F_N][2] = {};
  i64 nal(int fld, int c, i64 x) {
    tc_stage = STG_NAL;
    H_nal_v D = {fld, c, tc_qlog(last_[fld][0]), tc_qlog(last_[fld][1])};
    tcx X;
    H_nal_make(D, X);
    x = f_nal.code(X, ENC ? x : 0);
    last_[fld][1] = last_[fld][0];
    last_[fld][0] = x;
    tc_syms[STG_NAL]++;
    return x;
  }

  // bytes nothing else models: an order 0 and an order 1 counter and a
  // match model mixed by a weight set the bits of the byte so far select
  // (oggcomp's raw model and a match): the match finds the last three bytes
  // in what was coded so far, after a dictionary of the user data x264
  // writes in an SEI message (its UUID and its options with their defaults)
  cm_cnt raw_o0_[256];
  cm_cnt raw_o1_[256][256];
  i32 raw_w_[256 * 4];
  u8 raw_wc_[256];
  cm_mix<4> raw_mx_;
  u32 raw_prev_ = 0;
  std::vector<u8> rbuf_;           // the dictionary, then the bytes coded
  std::vector<i32> rhash_;         // by the hash of three bytes, the place after them in rbuf_ (-1 none)
  size_t rptr_ = 0, rdict_ = 0;    // where the match predicts from; the dictionary's size
  int rlen_ = 0;                   // the match's length (0 none)
  cm_cnt raw_mc_[16];              // whether the match's bit is right, by its length
  static u32 rkey(const u8* p) { return ((u32)p[0] * 0x9E3779B1u ^ (u32)p[1] * 0x85EBCA77u ^ (u32)p[2] * 0xC2B2AE3Du) >> 16; }
  void raw_init() {
    static const u8 kUuid[16] = {0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef};
    static const char kText[] =
        "x264 - core 164 - H.264/MPEG-4 AVC codec - Copyleft 2003-2024 - http://www.videolan.org/x264.html - options: "
        "cabac=1 ref=3 deblock=1:0:0 analyse=0x3:0x113 me=hex subme=7 psy=1 psy_rd=1.00:0.00 mixed_ref=1 me_range=16 "
        "chroma_me=1 trellis=1 8x8dct=1 cqm=0 deadzone=21,11 fast_pskip=1 chroma_qp_offset=-2 threads=8 "
        "lookahead_threads=1 sliced_threads=0 nr=0 decimate=1 interlaced=0 bluray_compat=0 constrained_intra=0 "
        "bframes=3 b_pyramid=2 b_adapt=1 b_bias=0 direct=1 weightb=1 open_gop=0 weightp=2 keyint=250 keyint_min=25 "
        "scenecut=40 intra_refresh=0 rc_lookahead=40 rc=crf mbtree=1 crf=23.0 qcomp=0.60 qpmin=0 qpmax=69 qpstep=4 "
        "ip_ratio=1.40 aq=1:1.00";
    rbuf_.assign(kUuid, kUuid + 16);
    rbuf_.insert(rbuf_.end(), kText, kText + sizeof kText);  // with the terminating zero x264 writes
    rhash_.assign((size_t)1 << 16, -1);
    for (size_t i = 3; i <= rbuf_.size(); i++) rhash_[rkey(&rbuf_[i - 3])] = (i32)i;
    rdict_ = rbuf_.size();
    rlen_ = 0;
  }
  u32 raw(u32 b) {
    tc_stage = STG_RAW;
#ifndef H264P_NO_THREADS
    if (!f_DEC && dq_) return dq_->raw(this, b), b;
#endif
    return raw_x(b);
  }
  u32 raw_x(u32 b) {
    if (rbuf_.empty()) raw_init();
    cm_cnt* row = raw_o1_[raw_prev_];
    u32 node = 1;
    int pbyte = rlen_ && rptr_ < rbuf_.size() ? rbuf_[rptr_] : -1, ml = std::min(rlen_, 15);
    for (int i = 7; i >= 0; i--) {
      cm_cnt c0 = raw_o0_[node], c1 = row[node];
      raw_mx_.set(0, cm_stretch(c0.P()));
      raw_mx_.set(1, c1.t ? cm_stretch(c1.P()) : 0);
      raw_mx_.set(2, 256);
      // the match's bit, while the byte so far agrees with its
      int mb = -1;
      if (pbyte >= 0 && (u32)(pbyte | 256) >> (i + 1) == node) mb = pbyte >> i & 1;
      int sm = mb < 0 ? 0 : cm_stretch(raw_mc_[ml].P());
      raw_mx_.set(3, mb < 0 ? 0 : mb ? sm : -sm);
      int bit = tc_bit<f_DEC>(raw_mx_.mix(node), (int)(ENC && ((b >> i) & 1)));
      raw_mx_.upd(bit, 24, 0);
      c0.upd(bit, 250, 0);
      c1.upd(bit, 60, 0);
      if (mb >= 0) raw_mc_[ml].upd(bit == mb, 250, 0);
      raw_o0_[node] = c0;
      row[node] = c1;
      node = node * 2 + (u32)bit;
    }
    raw_prev_ = node & 255;
    // the match goes on or ends; then the bytes coded join what it searches
    // (the first 256 MB of them)
    if (pbyte >= 0 && (u32)pbyte == (node & 255))
      rlen_++, rptr_++;
    else
      rlen_ = 0;
    if (rbuf_.size() < ((size_t)1 << 28)) {
      rbuf_.push_back((u8)(node & 255));
      size_t n = rbuf_.size();
      if (n - rdict_ >= 3) {
        u32 k = rkey(&rbuf_[n - 3]);
        if (!rlen_ && rhash_[k] >= 0) rptr_ = (size_t)rhash_[k], rlen_ = 1;
        rhash_[k] = (i32)n;
      }
    }
    tc_syms[STG_RAW]++;
    return node & 255;
  }

  // ------------------------------------------------------------------------
  // header syntax: a value is coded as the difference to the last value of
  // the same element (tag, place in the group, row in a run of the tag),
  // kept per kind of slice
  struct HLast {
    i64 v, d1, d2;  // the last value, the last two differences
  };
  std::map<std::string, int> tags_;
  int tag_ = 0, fld_ = 0, lasttag_ = -1, runpos_ = 0;
  int sty_ = 10, last_sty_ = 10, ref_idc_ = 0;  // slot of the current NAL unit: slice type x reference, 10 none
  bool slice_nal_ = false;
  bool is_slice_type_ = false;
  // frame_num and pic_order_cnt_lsb wrap: their differences are taken
  // modulo 2^hbits_ (the bits of the u(n) to come, 0 none)
  bool is_wrap_ = false;
  int hbits_ = 0;
  void hbits(int n) { hbits_ = n; }
  std::map<u32, HLast> hl_;  // by tag, place in the group, row in a run of the tag (up to 15), slice kind

  // the start of a NAL unit coded by the walk
  int last_sref_ = 2, prev_sref_ = 2;  // whether the last slice NAL unit, and the one before, were references (2 none)
  void nal_begin(int type, int ref_idc) {
    slice_nal_ = type == 1 || type == 5;
    ref_idc_ = ref_idc;
    if (slice_nal_) prev_sref_ = last_sref_, last_sref_ = ref_idc != 0;
    sty_ = slice_nal_ ? last_sty_ : 10;
  }
  void hbegin(const char* name) {
    auto it = tags_.find(name);
    if (it == tags_.end()) {
      if (tags_.size() >= 256) cm_fail("internal: more than 256 header elements");
      it = tags_.insert(std::make_pair(std::string(name), (int)tags_.size())).first;
    }
    tag_ = it->second;
    is_slice_type_ = slice_nal_ && it->first == "slice_type";
    is_wrap_ = it->first == "frame_num" || it->first == "pic_order_cnt_lsb";
    fld_ = 0;
    runpos_ = tag_ == lasttag_ ? runpos_ + 1 : 0;
    lasttag_ = tag_;
  }
  i64 hval(i64 x) {
    tc_stage = STG_HDR;
    HLast& h = hl_[(u32)tag_ | (u32)(fld_ < 15 ? fld_ : 15) << 8 | (u32)(runpos_ < 15 ? runpos_ : 15) << 12 | (u32)sty_ << 16];
    H_hdr_v D = {tag_, fld_, sq2(h.d1), sq2(h.d2), tc_qlog(runpos_), (int)(h.v & 255), sty_ < 9 ? sty_ : 9, slice_nal_ ? prev_sref_ : 3};
    tcx X;
    H_hdr_make(D, X);
    i64 m = is_wrap_ && hbits_ > 0 && hbits_ < 31 ? (i64)1 << hbits_ : 0, dx = ENC ? x - h.v : 0;
    hbits_ = 0;
    if (m && ENC) dx = ((dx % m) + m + m / 2) % m - m / 2;  // into [-m/2, m/2)
    i64 d = f_hdr.codes(X, dx, (u32)tag_);
    x = m ? ((h.v + d) % m + m) % m : h.v + d;
    h.d2 = h.d1;
    h.d1 = d;
    h.v = x;
    fld_++;
    tc_syms[STG_HDR]++;
    if (is_slice_type_) sty_ = last_sty_ = (int)(x % 5) * 2 + (ref_idc_ != 0);
    return x;
  }
  bool hmore(const char* next, bool m) {
    hbegin((std::string("+") + next).c_str());
    return hval(m) != 0;
  }

  // ------------------------------------------------------------------------
  // slice data
  SliceCoder* sc_ = nullptr;
  const MbState* mbs_ = nullptr;  // the walk's macroblock states, for addresses
  std::vector<MbInfo> info_;
  std::vector<MbCol> hist_[30];
  std::vector<uint8_t> hskip_[30];  // the same's valid && skip, a byte a macroblock (for skp_vars)
  MbCol* hcur_ = nullptr;  // hist_ of the current slice's kind
  std::vector<MbCol> hprev_;  // and as it was when the picture began (if hprev_ok_: a P picture)
  bool hprev_ok_ = false;
  uint32_t pic_ = 1;       // pictures so far
  SliceHeader last_h_;     // the last slice coded (no sps: none yet)
  int st_ = 0, key_ = 0;   // slice type x reference; st_ x frame/top/bottom field
  int width_ = 1, size_ = 0;
  bool mbaff_ = false;
  int qp_off_ = 0, qp_pred_ = 26, slice_qp_ = 26;
  // 1 + the QP of the picture's last macroblock of each kind (inter,
  // I_NxN, other) and energy class (qpd's xe), 0 none
  int lq_[3][19] = {};
  int last_qpd_ = 0, lz_ = 0;  // the last mb_qp_delta, the last nonzero one
  int qp_src_ = -1;            // the macroblock that coded it
  bool has_lz_ = false, qpd_coded_ = false;
  int run_ = 0, cnt_ = 0, prev_cnt_ = 0;  // skipped before, macroblocks in the slice so far, in the last one
  int slice_cnt_[30] = {};
  bool mb_open_ = false;
  int addr_ = 0;
  MbInfo* mi_ = nullptr;
  const MbCol* col_ = nullptr;
  const MbCol* col1_ = nullptr;  // B: the same macroblock in list 1's first reference (phist)
  int sub_[4] = {};
  int pmv_[2] = {};  // proxy_mv()
  i64 shx_ = 0, shy_ = 0, shd_ = 0;  // block(): the shift fitted to the macroblock's last inter luma block, (x, y) / d
  int tx_ = 0;  // skip_miss: the skip prediction's texture (class)
  H_mbt_v* mbt_d_ = nullptr;  // mb_type's contexts while it is coded
  int prx_v_[2] = {};       // the proxy's vector (quarter samples), as searched again (block)
  int prx_blk_ = 0;         // the luma block it was last searched for
  bool prx_chroma_ = false;  // and whether for the chroma
  int pxq_ = 0;      // proxy_mv(): how well its vector matches the samples around (1 + tc_qlog, 0 not known)
  bool pmv_ok_ = false;
  std::vector<int16_t> pxv_;  // the vectors of the current picture's proxies (INT16_MIN none)
  int sm_ = 0, tm_ = 0;  // skip_miss() of the macroblock, its template matching miss
  int16_t smv_[2][16][2];  // skip_miss(): the skip motion (smv_ok_: made)
  int8_t srf_[2][4];
  bool smv_ok_ = false;
  int mph_ = 1;          // ipm(): the macroblock's most-probable-mode flags so far, after a 1
  int cnth_ = 0;         // block(): the counts of the last two blocks, two bits each
  int tb_ = 0, dh_ = 0;  // skip_miss(): the best template match on list 0, the better list of the skip motion (B)
  int bd_ = 0;           // skip_miss(): the skip motion's two predictions apart (B)
  int tmb_[2] = {};      // that vector
  bool tmb_ok_ = false;
  int prev_mode_ = 9, mvx_ = 0;
  int parts_ = 0, subshape_[4] = {};  // partitions of the macroblock: 16x16, 16x8, 8x16, 8x8; sub-macroblock shapes
  int mvp_[2] = {}, sp_[2] = {}, pdv_[2] = {};  // the current partition: mvp, spread of the neighbours, col - mvp
  int ns_[2] = {};  // and the signs of the neighbours' vectors less mvp, summed
  bool pdn_ = false;                             // no col vector
  int tmv_[2] = {};                              // its vector by template matching (Recon::tm_search)
  bool tmn_ = true;                              // none
  int tmq_ = 0;                                  // its miss (Recon::tm_search)

  Recon rec_;  // the pictures, reconstructed
  // x264's AQ (AqFit): the line of the current picture; 16 log2 energy of
  // the macroblocks of the current and the last picture (-1 not known); the
  // QP of the current picture's that coded mb_qp_delta (-1 other); per kind
  // of picture, how far the QP of the last one's was from its line (16ths,
  // INT16_MIN not known)
  AqFit aq_fit_, aq_fitp_, aq_fitx_;
  int aq_xp_ = -1;  // 16 log2 energy of the current macroblock's prediction (inter)
  bool aq_xi_ = false;  // or of its proxy (intra; aq_fitx_)
  std::vector<int16_t> aq_x_, aq_xl_, aq_q_, aq_res_[30];
  int aq_key_ = -1;
  void aq_picture() {
    if (aq_key_ >= 0) {
      std::vector<int16_t>& r = aq_res_[aq_key_];
      r.assign(aq_q_.size(), INT16_MIN);
      for (size_t a = 0; a < aq_q_.size(); a++) {
        int q16;
        if (aq_q_[a] >= 0 && aq_fit_.predict(aq_x_[a], q16)) r[a] = (int16_t)tc_clamp(16 * aq_q_[a] - q16, -32767, 32767);
      }
    }
    aq_xl_.swap(aq_x_);
    aq_x_.assign(size_, -1);
    aq_q_.assign(size_, -1);
    aq_fit_.clear();
    aq_fitp_.clear();
    aq_fitx_.clear();
    aq_key_ = key_;
    pxv_.assign(size_ * 2, INT16_MIN);
  }
  int idx(const MbState* n) const { return (int)(n - mbs_); }
  // CABAC's probability of a bin in the walk's current state, as the outside
  // prediction of one of the model's decisions: of the bin being 0 (cab0) or
  // 1 (cab1). -1 without CABAC.
  int cab0(int ctx) const {
    if (!sc_->cabac_) return -1;
    const CabacCtx& c = sc_->ctx_[ctx];
    return c.mps ? cab_lps[c.state] : CM_PONE - cab_lps[c.state];
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
  int mbx_ = 0, mby_ = 0, height_ = 0;  // the macroblock's place (begin_mb), the picture's height (in macroblocks)
  // a macroblock of the current picture, coded already
  // in a B slice, the history of list 1's first reference, where it is
  // the last picture of a kind (by its POC) and nearer in time than the
  // last picture of the current kind; nullptr other slices or none
  int hist_poc_[30], prev_poc_ = INT_MIN, cur_poc_ = INT_MIN;
  // the POCs of each kind's last picture's references by list and index
  // (INT_MIN none), and those of the one before the current picture's;
  // frame references only (a field macroblock's ref_idx goes up to 63)
  int hist_rpoc_[30][2][32], col_rpoc_[2][32];
  int phist_key() const { return phk_; }
  int phk_ = -1;  // phist_key() of the slice (slice_begin)
  int phist_key_of_slice() const {
    if (sc_->stype_ != kB || !rec_.active || rec_.nlist[1] < 1 || !rec_.list[1][0]) return -1;
    int poc = rec_.list[1][0]->poc;
    for (int k = 0; k < 30; k++)
      if (k != key_ && hist_poc_[k] == poc && (int)hist_[k].size() >= size_) return k;
    return -1;
  }
  const MbCol* phist() const {
    int k = phist_key();
    return k < 0 ? nullptr : hist_[k].data();
  }
  const MbInfo* near_info(int dx, int dy) const {
    int a = mb_near(dx, dy);
    return a >= 0 && info_[a].pic == pic_ ? &info_[a] : nullptr;
  }

  // whether slice h begins a picture (7.4.1.2.4): what tells pictures
  // apart differs from the last slice coded
  bool new_picture(const SliceHeader& h) const {
    const SliceHeader& p = last_h_;
    if (!p.sps || h.sps != p.sps || h.pps != p.pps || h.frame_num != p.frame_num ||
        h.field_pic_flag != p.field_pic_flag || h.bottom_field_flag != p.bottom_field_flag ||
        !h.nal_ref_idc != !p.nal_ref_idc || (h.nal_unit_type == 5) != (p.nal_unit_type == 5))
      return true;
    if (h.sps->poc_type == 0 && (h.poc_lsb != p.poc_lsb || h.delta_poc_bottom != p.delta_poc_bottom)) return true;
    if (h.sps->poc_type == 1 && (h.delta_poc[0] != p.delta_poc[0] || h.delta_poc[1] != p.delta_poc[1])) return true;
    return h.nal_unit_type == 5 && h.idr_pic_id != p.idr_pic_id;
  }

  void slice_begin(SliceCoder& sc) {
    sc_ = &sc;
    mbs_ = sc.store_.mbs.data();
    const SliceHeader& h = sc.h_;
    bool newpic = new_picture(h);
    last_h_ = h;
    st_ = h.type() * 2 + (h.nal_ref_idc != 0);
    key_ = st_ * 3 + (h.field_pic_flag ? 1 + h.bottom_field_flag : 0);
    if (newpic) pic_++;
    width_ = sc.width_;
    size_ = sc.size_;
    mbaff_ = sc.mbaff_;
    if ((int)info_.size() < size_) info_.resize(size_);
    if ((int)hist_[key_].size() < size_) hist_[key_].resize(size_), hskip_[key_].resize(size_);
    hcur_ = hist_[key_].data();
    // (the macroblocks as they were for the P slices' mc_col, mc_nz only)
    if (newpic) hprev_ok_ = h.type() == kP || h.type() == kSP;
    if (newpic && hprev_ok_) hprev_.assign(hcur_, hcur_ + size_);
    qp_off_ = 6 * (sc.sps_.bit_depth_luma - 8);
    qp_pred_ = slice_qp_ = sc.pps_.pic_init_qp + h.slice_qp_delta;
    last_qpd_ = 0;
    has_lz_ = false;
    qp_src_ = -1;
    run_ = cnt_ = 0;
    prev_cnt_ = slice_cnt_[key_];
    mb_open_ = false;
    rec_.slice_begin(h, newpic);
    if (newpic) {
      prev_poc_ = hist_poc_[key_];
      cur_poc_ = hist_poc_[key_] = rec_.active ? rec_.cur->poc : INT_MIN;
      memcpy(col_rpoc_, hist_rpoc_[key_], sizeof col_rpoc_);
      for (int l = 0; l < 2; l++)
        for (int r = 0; r < 32; r++)
          hist_rpoc_[key_][l][r] = rec_.active && r < rec_.nlist[l] && rec_.list[l][r] ? rec_.list[l][r]->poc : INT_MIN;
    }
    phk_ = phist_key_of_slice();
    if (newpic) aq_picture();
    if (newpic) memset(lq_, 0, sizeof lq_);
  }
  void slice_end() { slice_cnt_[key_] = cnt_; }

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
    // (of the coefficients, the rows the last macroblock here wrote)
    for (uint64_t m = mi_->cmask; m; m &= m - 1) {
      int r = __builtin_ctzll(m);
      memset(r < 48 ? mi_->coef[r >> 4][r & 15] : mi_->dc[r - 48], 0, 16);
    }
    memset(mi_, 0, offsetof(MbInfo, coef));
    mi_->pic = pic_;
    mi_->qp = (uint8_t)(qp_pred_ + qp_off_);
    memset(mi_->modes, 2, sizeof mi_->modes);
    memset(mi_->rf, -1, sizeof mi_->rf);
    memset(mi_->tme, -128, sizeof mi_->tme);
    col_ = hcur_[addr_].valid ? &hcur_[addr_] : nullptr;
    // B: the same macroblock in list 1's first reference too
    col1_ = nullptr;
    if (const MbCol* p = phist()) col1_ = p[addr_].valid ? &p[addr_] : nullptr;
    parts_ = 0;
    rec_.mb_start(addr_);
    qpd_coded_ = false;
    prev_mode_ = 9;
    sm_ = tm_ = tb_ = dh_ = bd_ = 0;
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
    MbCol& c = hcur_[addr_];
    c.valid = 1;
    hskip_[key_][addr_] = mi_->skip != 0;
    c.skip = mi_->skip;
    c.cls = mi_->cls;
    c.qp = mi_->qp;
    c.cbp = mi_->cbp;
    c.t8 = mi_->t8;
    c.cpred = mi_->cpred;
    c.field = mi_->field = s.field;
    memcpy(c.modes, mi_->modes, sizeof c.modes);
    memcpy(c.ref, mi_->ref, sizeof c.ref);
    memcpy(c.rf, mi_->rf, sizeof c.rf);
    memcpy(c.mv, mi_->mv, sizeof c.mv);
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
      memcpy(c.coef[r], mi_->coef[0][r], 16);
    }
    c.cm = (uint16_t)mi_->cmask;
    mb_open_ = false;
    rec_.mb_end(addr_, mi_->cls, mi_->i16m, mi_->modes, mi_->cpred, mi_->t8, mi_->qp - qp_off_, mi_->rf, mi_->mv, s.nz[0]);
    if (rec_.active && (int)aq_x_.size() == size_) {
      int x = lg16(rec_.ac_energy(addr_));
      aq_x_[addr_] = (int16_t)x;
      if (qpd_coded_) {
        aq_fit_.add(x, mi_->qp - qp_off_);
        if (aq_xp_ >= 0) (aq_xi_ ? aq_fitx_ : aq_fitp_).add(aq_xp_, mi_->qp - qp_off_);
        aq_q_[addr_] = (int16_t)(mi_->qp - qp_off_);
      }
    }
  }

  // --- skip, end of slice, field decoding flag: the skp family
  void skp_vars(H_skp_v& D) const {
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    D.st = st_;
    D.a3 = a ? 2 - a->skip : 0;
    D.b3 = b ? 2 - b->skip : 0;
    D.col = col_ ? 2 - col_->skip : 0;
    D.c9 = 0;
    const uint8_t* hs = hskip_[key_].data();
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++) {
        int n = mb_near(dx, dy);
        D.c9 += n >= 0 && hs[n];
      }
    D.c1 = col1_ ? 2 - col1_->skip : 0;
    // and the skipped ones around it there (10 none)
    D.k9 = 10;
    if (phk_ >= 0) {
      const uint8_t* p = hskip_[phk_].data();
      D.k9 = 0;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          int n = mb_near(dx, dy);
          D.k9 += n >= 0 && p[n];
        }
    }
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
  // how well the prediction of the macroblock were it skipped continues
  // the samples above and to the left (1 + tc_qlog of Recon::skip_miss, 0
  // not known)
  // the best vector for the macroblock by template matching on list l's
  // first reference (from mvp, the neighbours' vectors, zero; in whole
  // samples): false none; miss as Recon::tm_search's
  bool tm_best(int l, int* mv, int& miss) const {
    MvN nbs[3];
    mv_pred(l, 0, 0, 4, 4, 0, mv, nbs);
    int cand[4][2], nc = 0;
    for (int j = 0; j < 3; j++)
      if (nbs[j].ref == 0) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
    cand[nc][0] = cand[nc][1] = 0, nc++;
    return rec_.tm_search(l, 0, 0, 0, 16, 16, mv, false, cand, nc, &miss, 0, 4);
  }
  int skip_miss() {
    tm_ = tb_ = dh_ = bd_ = 0;
    if (!rec_.active) return 0;
    // how well the reference can predict the samples around at best (list
    // 0)
    int m0;
    tmb_ok_ = tm_best(0, tmb_, m0);
    if (tmb_ok_) tb_ = 1 + tc_clamp(tc_qlog(m0), 0, 15);
    if (sc_->stype_ == kB)
      mv_direct(15);
    else
      mv_pskip();
    // (kept for skip(), should the macroblock be skipped)
    memcpy(smv_, mi_->mv, sizeof smv_);
    memcpy(srf_, mi_->rf, sizeof srf_);
    smv_ok_ = true;
    int per[2];
    int tm = rec_.tm_miss(mi_->rf, mi_->mv, per);
    tm_ = tm < 0 ? 0 : 1 + tc_clamp(tc_qlog(tm), 0, 15);
    // B: which list's prediction of the skip motion does that better, and
    // whether their average does better still
    if (sc_->stype_ == kB && per[0] >= 0 && per[1] >= 0) {
      dh_ = per[0] * 4 < per[1] * 3 ? 1 : per[1] * 4 < per[0] * 3 ? 2 : 3;
      int mn = std::min(per[0], per[1]);
      dh_ += 3 * (tm * 4 < mn * 3 ? 1 : tm > mn ? 2 : 0);
    }
    // B: how far apart the two predictions of the skip motion are; P: how
    // far the skip vector is from the best template match's
    int bd = -1;
    if (sc_->stype_ == kB)
      bd = rec_.bi_diff(mi_->rf, mi_->mv);
    else if (tmb_ok_)
      bd = abs(tmb_[0] - mi_->mv[0][0][0]) + abs(tmb_[1] - mi_->mv[0][0][1]);
    bd_ = bd < 0 ? 0 : 1 + tc_clamp(tc_qlog(bd), 0, 15);
    int ms = rec_.skip_miss(mi_->rf, mi_->mv);
    // the texture of the skip prediction, per sample, against the step:
    // flat, its residual is likelier to quantize away
    {
      int t = ms < 0 ? -1 : rec_.mb_texture();
      tx_ = t < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)t * 16 / qstep16(qp_pred_)), 0, 14);
    }
    memset(mi_->mv, 0, sizeof mi_->mv);
    memset(mi_->rf, -1, sizeof mi_->rf);
    parts_ = 0;
    return ms < 0 ? 0 : 1 + tc_clamp(tc_qlog(ms), 0, 15);
  }
  // the last picture decoded at the current macroblock: how much it changed
  // (1 + tc_qlog, 0 not known), its energy (log2 classes, 0 not known)
  int map_td() const { return maps_ok() && rec_.change[addr_] >= 0 ? 1 + tc_qlog(rec_.change[addr_]) : 0; }
  int map_en() const { return maps_ok() ? 1 + tc_clamp(rec_.energy[addr_] / 4 - 4, 0, 13) : 0; }
  // the maps of the last picture decoded fit the current one
  bool maps_ok() const { return rec_.maps_w == width_ && rec_.maps_w * rec_.maps_h == size_ && !mbaff_; }
  int skp_bit(const H_skp_v& D, int node, int v, int pr = -1) {
    tcx X;
    H_skp_make(D, X);
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
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    int ctx = (sc_->stype_ == kB ? 24 : 11) + (a && !a->skip) + (b && !b->skip);
    v = skp_bit(D, 0, ENC && v, cab0(ctx));
    if (v) {
      mi_->skip = 1;
      mi_->cls = CL_SKIP;
      if (smv_ok_) {
        memcpy(mi_->mv, smv_, sizeof smv_);
        memcpy(mi_->rf, srf_, sizeof srf_);
        parts_ = 0;
      } else if (sc_->stype_ == kB) {
        mv_direct(15);
      } else {
        mv_pskip();
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
#ifndef H264P_NO_THREADS
      if (!f_DEC && dq_) return dq_->flag(this, st_, ENC && v), v;
#endif
      return end_x(st_, v);
    }
    H_skp_v D = {};
    skp_vars(D);
    return skp_bit(D, 1, ENC && v);
  }
  int end_x(int ctx, int v) {
    cm_cnt& c = end_c_[ctx];
    v = tc_bit<f_DEC>(c.P(), ENC && v);
    c.upd(v, 1023, 0);
    return v;
  }
  int field(int v) {
    tc_stage = STG_SKIP;
    H_skp_v D = {};
    skp_vars(D);
    D.col = col_ ? 1 + col_->field : 0;
    int ctx = 70 + (sc_->left_ && sc_->left_->field) + (sc_->above_ && sc_->above_->field);
    return skp_bit(D, 2, ENC && v, cab0(ctx));
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
    int ctx0 = off;
    if (islice) {
      const MbState* a = sc_->mb_a_;
      const MbState* b = sc_->mb_b_;
      ctx0 += (a && !a->inxn) + (b && !b->inxn);
    }
    if (mbt_bit(21, ENC && it == 0, cab1(ctx0))) return 0;
    if (mbt_bit(22, ENC && it == 25)) return 25;
    if (sc_->chroma_ == 1 && mbt_d_) {
      // Intra16x16: the mode that does best in the proxy, and by how much
      // it beats the next
      proxy_mv();
      int costs[4];
      int m = rec_.proxy_i16mode(pmv_[0], pmv_[1], costs);
      if (m >= 0) {
        int c2 = INT_MAX;
        for (int k = 0; k < 4; k++)
          if (k != m) c2 = std::min(c2, costs[k]);
        mbt_d_->im = 1 + m;
        mbt_d_->ig = c2 == INT_MAX ? 0 : 1 + tc_clamp(tc_qlog((c2 - costs[m]) * 32 / x264_lam16(qp_pred_)), 0, 11);
        tcx X;
        H_mbt_make(*mbt_d_, X);
        f_mbt.select(X);
      }
    }
    int x = ENC ? it - 1 : 0;
    int luma = mbt_bit(23, ENC && x / 12, cab0(off + (islice ? 3 : 1)));
    int c = x / 4 % 3;
    int chroma = mbt_bit(24, ENC && c != 0, cab0(off + (islice ? 4 : 2)));
    if (chroma) chroma += mbt_bit(25, ENC && c == 2, cab0(off + (islice ? 5 : 2)));
    int p1 = mbt_bit(26, ENC && (x & 2), cab0(off + (islice ? 6 : 3)));
    int p0 = mbt_bit(27 + p1, ENC && (x & 1), cab0(off + (islice ? 7 : 3)));
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
    H_mbt_make(D, X);
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
        } else if (mbt_bit(6, ENC && t == 0, cab1(27 + (a && !a->direct16) + (b && !b->direct16)))) {
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
        if (mbt_bit(1, ENC && t >= 5, cab0(14)))
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
      parts_ = t <= 3 ? 0 : t == 22 ? 3 : t < 22 ? 2 - (t & 1) : 0;
    else if (stype == kP || stype == kSP)
      parts_ = t < 3 ? t : 3;
    if (stype == kB && t == 0) mv_direct(15);
    tc_syms[STG_MBT]++;
    return t;
  }

  // --- coded_block_pattern and transform_size_8x8_flag: the cbp family
  int cbp_bit(H_cbp_v& D, int node, int v, int pr) {
    tcx X;
    H_cbp_make(D, X);
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
    if (tmb_ok_)
      p[0] = tmb_[0], p[1] = tmb_[1];
    else if (sc_->stype_ != kI && sc_->stype_ != kSI && !tm_best(0, p, miss))
      mv_pred(0, 0, 0, 4, 4, 0, p);
    // and the neighbours' proxies' vectors, zero
    if ((int)pxv_.size() == size_ * 2) {
      int cand[9][2], nc = 0;
      const int nb[3] = {mb_near(-1, 0), mb_near(0, -1), mb_near(1, -1)};
      for (int j = 0; j < 3; j++)
        if (nb[j] >= 0 && info_[nb[j]].pic == pic_ && pxv_[nb[j] * 2] != INT16_MIN)
          cand[nc][0] = pxv_[nb[j] * 2], cand[nc][1] = pxv_[nb[j] * 2 + 1], nc++;
      // the neighbours' vectors on list 0, scaled to its first reference's
      // distance
      if (sc_->stype_ != kI && sc_->stype_ != kSI) {
        MvN nbs[3];
        int mp[2];
        mv_pred(0, 0, 0, 4, 4, 0, mp, nbs);
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
        int cr = col_->rf[0][0], d0 = ref_dist(0, 0), dr = d0;
        if (prev_poc_ != INT_MIN && cr < 32 && col_rpoc_[0][cr] != INT_MIN) d0 = prev_poc_ - col_rpoc_[0][cr];
        for (int k = 0; k < 2; k++) cand[nc][k] = d0 && dr && d0 != dr ? mv_scale(col_->mv[0][5][k], dr, d0) : col_->mv[0][5][k];
        nc++;
      }
      cand[nc][0] = cand[nc][1] = 0, nc++;
      int miss;
      if (rec_.tm_search(-1, 0, 0, 0, 16, 16, p, false, cand, nc, &miss, 0, 2)) pxq_ = 1 + tc_clamp(tc_qlog(miss), 0, 11);
      pxv_[addr_ * 2] = (int16_t)tc_clamp(p[0], -32767, 32767);
      pxv_[addr_ * 2 + 1] = (int16_t)tc_clamp(p[1], -32767, 32767);
    }
    rec_.proxy_build(p);
    prx_v_[0] = p[0];
    prx_v_[1] = p[1];
    pmv_[0] = (p[0] + 2) >> 2;
    pmv_[1] = (p[1] + 2) >> 2;
  }
  // how well the motion of an inter macroblock predicts the decoded samples
  // around it (1 + tc_qlog of Recon::tm_miss; 0 not known, intra)
  int tm_pred() const {
    if (mi_->cls >= CL_INXN || !rec_.active) return 0;
    int ms = rec_.tm_miss(mi_->rf, mi_->mv);
    return ms < 0 ? 0 : 1 + tc_clamp(tc_qlog(ms), 0, 15);
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
      D.pg = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)pt * 16 / qstep16(mi_->qp - qp_off_)), 0, 15);
      int ms = mi_->cls == CL_INXN ? -1 : rec_.edge_miss(0, (b8 & 1) * 8, (b8 >> 1) * 8, 8);
      D.em = ms < 0 ? 0 : 1 + tc_clamp(tc_qlog(ms), 0, 15);
      // intra: the levels the proxy picture would need in the block's modes
      D.pq = 0;
      if (mi_->cls == CL_INXN) {
        proxy_mv();
        int qp = mi_->qp - qp_off_, nz = 0, bx = (b8 & 1) * 2, by = (b8 >> 1) * 2;
        if (mi_->t8)
          nz = rec_.proxy_nz(b8 & 1, b8 >> 1, 8, mi_->modes[by * 4 + bx], qp, pmv_[0], pmv_[1]);
        else
          for (int k = 0; k < 4 && nz >= 0; k++) {
            int x = bx + (k & 1), y = by + (k >> 1), z = rec_.proxy_nz(x, y, 4, mi_->modes[y * 4 + x], qp, pmv_[0], pmv_[1]);
            nz = z < 0 ? -1 : nz + z;
          }
        D.pq = nz < 0 ? 0 : 1 + std::min(nz, 5);
      }
      luma |= cbp_bit(D, b8, ENC && (v >> b8 & 1), cab0(73 + D.inc)) << b8;
    }
    int c = 0;
    if (sc_->chroma_ == 1 || sc_->chroma_ == 2) {
      int pt = rec_.pred_texture(1, 0, 0, 8);
      if (pt >= 0) pt += rec_.pred_texture(2, 0, 0, 8);
      D.pg = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog(pt), 0, 15);
      int ms = rec_.edge_miss(1, 0, 0, 8);
      if (ms >= 0) ms += std::max(0, rec_.edge_miss(2, 0, 0, 8));
      D.em = ms < 0 ? 0 : 1 + tc_clamp(tc_qlog(ms), 0, 15);
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
          if (!rec_.proxy_dc(pl, qc, lv)) break;
          pdc = std::max(pdc, 0);
          pac = std::max(pac, 0);
          for (int k = 0; k < 4; k++) pdc += abs(lv[k]) >= 3;
          for (int b = 0; b < 4; b++) {
            rec_.proxy_levels(pl, (b & 1) * 4, (b >> 1) * 4, qc, lv);
            for (int k = 1; k < 16; k++) pac += abs(lv[k]) >= 3;
          }
        }
      }
      D.pq = pdc < 0 ? 0 : 1 + std::min(pdc + pac, 5);
      if (cbp_bit(D, 4, ENC && (v >> 4) != 0, cab0(77 + D.inc))) {
        D.inc = sc_->cbp_chroma_inc(1);
        D.col = col_ ? 1 + (col_->cbp >> 4 == 2) : 0;
        D.pq = pac < 0 ? 0 : 1 + std::min(pac, 5);
        c = 1 + cbp_bit(D, 5, ENC && (v >> 4) == 2, cab0(81 + D.inc));
      }
    }
    tc_syms[STG_CBP]++;
    mi_->cbp = (uint8_t)(luma | c << 4);
    return mi_->cbp;
  }
  int t8x8(int v) {
    tc_stage = STG_CBP;
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    H_cbp_v D = {};
    D.inc = (a && a->t8x8) + (b && b->t8x8);
    D.pc = popc4(mi_->cbp & 15);
    D.cls = mi_->cls;
    D.col = col_ ? 1 + col_->t8 : 0;
    D.q6 = q6();
    D.st = st_;
    D.en = map_en();
    D.td = map_td();
    D.pg = 0;
    tc_syms[STG_CBP]++;
    v = cbp_bit(D, 6, ENC && v, cab0(399 + D.inc));
    mi_->t8 = (uint8_t)v;
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
    if (m.cls < CL_INXN && sc_->pps_.constrained_intra_pred_flag) {
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
    int mpm = dc ? 2 : std::min(ma > 8 ? 2 : ma, mb > 8 ? 2 : mb);
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
    int pm = k8 ? rec_.proxy_mode(x >> 1, y >> 1, 8, pmv_[0], pmv_[1], sads)
                : rec_.proxy_mode(x, y, 4, pmv_[0], pmv_[1], sads);
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
    H_ipm_make(D, X);
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
      int pc = rec_.proxy_cmode(pmv_[0], pmv_[1], costs);
      if (pc >= 0) {
        D.pm = pc;
        int c0 = costs[0] < INT_MAX ? costs[0] : costs[pc];
        D.mg = 1 + tc_clamp(tc_qlog((c0 - costs[pc]) * 32 / x264_lam16(qp_pred_)), 0, 11);
      }
    }
    tcx X;
    H_ipm_make(D, X);
    f_ipm.select(X);
    int r = 0;
    auto cond = [](const MbState* n) { return n && n->intra && !n->pcm && n->cpred != 0; };
    f_ipm.prp = cab0(64 + cond(sc_->mb_a_) + cond(sc_->mb_b_));
    if (f_ipm.bit(16, ENC && v > 0)) {
      f_ipm.prp = cab0(67);
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
    H_mvs_v D = {b, 0, 0, i ? sub_[i - 1] + 1 : 0, 0, 0, 0, 0, i, 0, 0, 0};
    tcx X;
    H_mvs_make(D, X);
    f_mvs.select(X);
    f_mvs.prp = -1;
    f_mvs.prpa = nullptr;
    int node = 1;
    for (int k = b ? 3 : 1; k >= 0; k--) node = node * 2 + f_mvs.bit(node, ENC && (v >> k & 1));
    int r = node - (b ? 16 : 4);
    sub_[i] = r;
    subshape_[i] = b ? kBSubShape[r % 13] : r;
    if (i == 3 && b) mv_direct((sub_[0] == 0) | (sub_[1] == 0) << 1 | (sub_[2] == 0) << 2 | (sub_[3] == 0) << 3);
    tc_syms[STG_MVS]++;
    return r;
  }

  // --- motion vectors, reconstructed as the decoder does (8.4.1): what the
  // mvd is coded against
  struct MvN {
    bool avail;  // the partition is there (it may be intra or not use the list)
    int ref, mv[2];
  };
  // the place of the partition with the 4x4 block (x, y) of the current
  // macroblock in decoding order
  int part_ord(int x, int y) const {
    switch (parts_) {
      case 0:
        return 0;
      case 1:
        return y >> 1;
      case 2:
        return x >> 1;
      default: {
        int i = (y >> 1) * 2 + (x >> 1), sh = subshape_[i];
        return 4 * i + (sh == 0 ? 0 : sh == 1 ? y & 1 : sh == 2 ? x & 1 : (y & 1) * 2 + (x & 1));
      }
    }
  }
  // the motion of the neighbouring 4x4 block (x, y) of the current
  // macroblock, x in -1..4, y in -1..3, for a partition at place ord
  // (8.4.1.3.2); approximate at the corners of MBAFF macroblock pairs
  MvN mv_nb(int list, int x, int y, int ord) const {
    MvN r = {false, -1, {0, 0}};
    const MbState* s;
    const MbInfo* m;
    int i = 0;
    if (x >= 0 && y >= 0) {
      if (x > 3 || part_ord(x, y) > ord) return r;  // not decoded yet
      s = sc_->cur_;
      m = mi_;
      i = y * 4 + x;
    } else if ((x < 0 && y < 0) || x > 3) {  // D or C, in the macroblock above left or right
      if (mbaff_ || addr_ < width_) return r;
      int a = x < 0 ? (addr_ % width_ ? addr_ - width_ - 1 : -1) : (addr_ % width_ < width_ - 1 ? addr_ - width_ + 1 : -1);
      if (a < 0 || mbs_[a].slice != sc_->slice_id_) return r;
      s = &mbs_[a];
      m = &info_[a];
      i = x < 0 ? 15 : 12;
    } else {
      s = sc_->nb4(x, y, i);
      if (!s) return r;
      m = &info_[idx(s)];
    }
    r.avail = true;
    int ref = m->rf[list][(i >> 3) * 2 + ((i & 3) >> 1)];
    if (ref < 0) return r;
    r.ref = ref;
    r.mv[0] = m->mv[list][i][0];
    r.mv[1] = m->mv[list][i][1];
    if (mbaff_ && s != sc_->cur_) {
      if (sc_->cur_->field && !s->field) {
        r.mv[1] /= 2;
        r.ref *= 2;
      } else if (!sc_->cur_->field && s->field) {
        r.mv[1] *= 2;
        r.ref >>= 1;
      }
    }
    return r;
  }
  static int median3(int a, int b, int c) { return a + b + c - std::min(a, std::min(b, c)) - std::max(a, std::max(b, c)); }
  // the prediction of the motion vector of a partition (8.4.1.3); nbs gets
  // its neighbours A, B and C
  void mv_pred(int list, int x, int y, int w, int h, int ref, int* p, MvN* nbs = nullptr) const {
    int ord = part_ord(x, y);
    MvN a = mv_nb(list, x - 1, y, ord), b = mv_nb(list, x, y - 1, ord), c = mv_nb(list, x + w, y - 1, ord);
    if (!c.avail) c = mv_nb(list, x - 1, y - 1, ord);
    if (nbs) {
      nbs[0] = a;
      nbs[1] = b;
      nbs[2] = c;
    }
    mv_pred(a, b, c, x, y, w, h, ref, p);
  }
  // the same from its neighbours A, B and C (C, D where C is not available)
  static void mv_pred(const MvN& a, MvN b, MvN c, int x, int y, int w, int h, int ref, int* p) {
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
  void set_mv(int list, int x, int y, int w, int h, int ref, const int* p) {
    int16_t v[2] = {(int16_t)tc_clamp(p[0], -32768, 32767), (int16_t)tc_clamp(p[1], -32768, 32767)};
    if (w == 4 && h == 4) {
      for (int b = 0; b < 16; b++) memcpy(mi_->mv[list][b], v, sizeof v);
      memset(mi_->rf[list], ref, 4);
      return;
    }
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) memcpy(mi_->mv[list][yy * 4 + xx], v, sizeof v);
    for (int yy = y; yy < y + h; yy += 2)
      for (int xx = x; xx < x + w; xx += 2) mi_->rf[list][(yy >> 1) * 2 + (xx >> 1)] = (int8_t)ref;
  }
  // P_Skip (8.4.1.1)
  void mv_pskip() {
    parts_ = 0;
    MvN a = mv_nb(0, -1, 0, 0), b = mv_nb(0, 0, -1, 0);
    int p[2] = {0, 0};
    if (a.avail && b.avail && !(a.ref == 0 && !a.mv[0] && !a.mv[1]) && !(b.ref == 0 && !b.mv[0] && !b.mv[1])) {
      MvN c = mv_nb(0, 4, -1, 0);
      if (!c.avail) c = mv_nb(0, -1, -1, 0);
      mv_pred(a, b, c, 0, 0, 4, 4, 0, p);
    }
    set_mv(0, 0, 0, 4, 4, 0, p);
  }
  // B direct, spatial (8.4.1.2.2), for the 8x8 blocks of mask; without
  // colZeroFlag, which needs the motion of the first picture of list 1.
  // Temporal direct is taken for spatial.
  static int minpos(int a, int b) { return a >= 0 && b >= 0 ? std::min(a, b) : std::max(a, b); }
  void mv_direct(int mask) {
    if (!sc_->h_.direct_spatial_mv_pred_flag) {
      mv_temporal(mask);
      return;
    }
    int saved = parts_, ref[2], mv[2][2] = {{0, 0}, {0, 0}};
    parts_ = 0;
    MvN nb[2][3];
    for (int l = 0; l < 2; l++) {
      MvN &a = nb[l][0], &b = nb[l][1], &c = nb[l][2];
      a = mv_nb(l, -1, 0, 0), b = mv_nb(l, 0, -1, 0), c = mv_nb(l, 4, -1, 0);
      if (!c.avail) c = mv_nb(l, -1, -1, 0);
      ref[l] = minpos(a.ref, minpos(b.ref, c.ref));
    }
    bool zero = ref[0] < 0 && ref[1] < 0;
    if (zero) ref[0] = ref[1] = 0;
    // (the macroblock's prediction, from the neighbours just found)
    for (int l = 0; l < 2; l++)
      if (!zero && ref[l] >= 0) mv_pred(nb[l][0], nb[l][1], nb[l][2], 0, 0, 4, 4, ref[l], mv[l]);
    bool lt = rec_.col_long_term();
    // (with direct_8x8_inference the four 4x4 blocks of an 8x8 one have the
    // same colocated block: the 8x8 block at once)
    int n = rec_.direct8x8 ? 2 : 1;
    // colZeroFlag, by 4x4 block (bit y * 4 + x): the colocated block is
    // still, which zeroes the vector of a list on reference 0
    int still = 0;
    if (!zero && !lt && (ref[0] == 0 || ref[1] == 0))
      for (int b8 = 0; b8 < 4; b8++)
        if (mask >> b8 & 1)
          for (int k = 0; k < 4; k += 3 * n - 2) {
            int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1), cr, cm[2];
            int64_t pid;
            if (rec_.colocated(addr_, y * 4 + x, cr, cm, pid) && cr == 0 && abs(cm[0]) <= 1 && abs(cm[1]) <= 1)
              still |= (n == 2 ? 0x33 : 1) << (y * 4 + x);
          }
    for (int l = 0; l < 2; l++) {
      if (ref[l] < 0) continue;
      int z[2] = {0, 0}, s = ref[l] == 0 ? still : 0;
      if (mask == 15 && (s == 0 || s == 0xFFFF)) {
        set_mv(l, 0, 0, 4, 4, ref[l], s ? z : mv[l]);
        continue;
      }
      for (int b8 = 0; b8 < 4; b8++)
        if (mask >> b8 & 1)
          for (int k = 0; k < 4; k += 3 * n - 2) {
            int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1);
            set_mv(l, x, y, n, n, ref[l], s >> (y * 4 + x) & 1 ? z : mv[l]);
          }
    }
    parts_ = saved;
  }
  // temporal direct (8.4.1.2.3)
  void mv_temporal(int mask) {
    for (int b8 = 0; b8 < 4; b8++)
      if (mask >> b8 & 1)
        for (int k = 0; k < 4; k++) {
          int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1), cr, cm[2] = {0, 0}, r0 = 0;
          int64_t pid;
          if (rec_.colocated(addr_, y * 4 + x, cr, cm, pid))
            r0 = rec_.map_col(pid);
          else
            cm[0] = cm[1] = 0;
          int dsf = rec_.dist_scale(r0);
          int m0[2] = {(dsf * cm[0] + 128) >> 8, (dsf * cm[1] + 128) >> 8};
          int m1[2] = {m0[0] - cm[0], m0[1] - cm[1]};
          set_mv(0, x, y, 1, 1, r0, m0);
          set_mv(1, x, y, 1, 1, 0, m1);
        }
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
    if (rec_.active && sc_->num_ref_[list] > 1) {
      int bm = INT_MAX, m0 = 0, rbest = -1;
      for (int r = 0; r < std::min(sc_->num_ref_[list], 4); r++) {
        int p[2], miss, cand[4][2], nc = 0;
        MvN nbs[3];
        mv_pred(list, x, y, w, h, r, p, nbs);
        for (int j = 0; j < 3; j++)
          if (nbs[j].ref == r) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
        cand[nc][0] = cand[nc][1] = 0, nc++;
        if (!rec_.tm_search(list, r, x * 4, y * 4, w * 4, h * 4, p, false, cand, nc, &miss, kTmLambda, 8)) break;
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
      MvN nbs[3];
      mv_pred(list, x, y, w, h, 0, p, nbs);
      int t = rec_.ref_texture(list, 0, x * 4, y * 4, w * 4, h * 4, p);
      D.rt = t < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)t * 256 / (w * h * 16) * 16 / qstep16(qp_pred_)), 0, 14);
    }
    tcx X;
    H_mvs_make(D, X);
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    pr[0] = cab0(54 + sc_->ref_inc(list, x, y));
    pr[1] = cab0(58);
    pr[2] = cab0(59);
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
  // the distance in POC from the current picture to list l's reference r
  // (0 not known), and a vector over distance dn scaled to distance dr
  int ref_dist(int l, int r) const {
    return rec_.active && r >= 0 && r < rec_.nlist[l] && rec_.list[l][r] ? cur_poc_ - rec_.list[l][r]->poc : 0;
  }
  static int mv_scale(int v, int dr, int dn) { return (int)div_floor((int64_t)v * dr * 2 + dn, 2 * dn); }
  // B: the vector the motion of list 1's first reference's co-located
  // block (as if it pointed at list 0's first reference) gives the 4x4
  // block (x, y) on list l's reference ref, as temporal direct does: false
  // none
  bool td_mv(int l, int ref, int x, int y, int* m) const {
    if (sc_->stype_ != kB || !col1_ || col1_->cls >= CL_INXN) return false;
    int b8 = (y >> 1) * 2 + (x >> 1), cl = col1_->rf[0][b8] >= 0 ? 0 : 1, cr = col1_->rf[cl][b8];
    int td = ref_dist(0, 0) - ref_dist(1, 0), tr = ref_dist(l, ref), k1 = phist_key();
    // over the distance from that picture to its own reference, where known
    if (cr >= 0 && cr < 32 && k1 >= 0 && hist_rpoc_[k1][cl][cr] != INT_MIN && ref_dist(1, 0))
      td = hist_poc_[k1] - hist_rpoc_[k1][cl][cr];
    if (cr < 0 || !td || !tr || !ref_dist(0, 0)) return false;
    for (int k = 0; k < 2; k++) m[k] = mv_scale(col1_->mv[cl][y * 4 + x][k], tr, td);
    return true;
  }
  int talt_[2] = {INT_MIN, 0};  // the template search's runner-up start (INT_MIN none)
  int mvd(int list, int comp, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1), ref = sc_->cur_->ref[list][b8];
    if (!comp) {
      MvN nbs[3];
      mv_pred(list, x, y, w, h, ref, mvp_, nbs);
      for (int k = 0; k < 2; k++) {
        sp_[k] = -1;
        ns_[k] = 0;
        for (int j = 0; j < 3; j++)
          if (nbs[j].ref >= 0) {
            sp_[k] = std::max(sp_[k], abs(nbs[j].mv[k] - mvp_[k]));
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
        int cr = col_->rf[list][b8], d0 = ref_dist(list, 0), dr = ref_dist(list, ref);
        if (prev_poc_ != INT_MIN && cr < 32 && col_rpoc_[list][cr] != INT_MIN) d0 = prev_poc_ - col_rpoc_[list][cr];
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
      tmn_ = !rec_.tm_search(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, false, cand, nc, &tmq_, kTmLambda, 1, 0, talt_);
    } else if (!tmn_) {
      // the horizontal component known
      tmv_[0] = mvp_[0] + mvx_;
      tmn_ = !rec_.tm_search(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, true, nullptr, 0, &tmq_, kTmLambda);
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
    D.cq = col_ && col_->cls < CL_INXN ? 1 + std::min<int>(col_->mvq[list][y * 4 + x][comp], 8) : 0;
    D.tq = tmn_ ? 0 : 1 + tc_clamp(tc_qlog(tmq_), 0, 11);
    D.ta = tmn_ || talt_[0] == INT_MIN ? 0 : 1 + tc_clamp(sq2(talt_[comp] - mvp_[comp]), 0, 12);
    D.ea = ea == -128 ? 0 : 1 + std::min(tc_qlog(ea), 5);
    D.eb = eb == -128 ? 0 : 1 + std::min(tc_qlog(eb), 5);
    // the texture of the reference at the predicted vector across the
    // component (x: the differences along rows), per sample, against the
    // quantizer's step: where it is flat that way, a search for a better
    // vector gains nothing
    {
      int t = rec_.ref_texture(list, ref, x * 4, y * 4, w * 4, h * 4, mvp_, comp ? 2 : 1);
      D.rt = t < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)t * 512 / (w * h * 16) * 16 / qstep16(qp_pred_)), 0, 14);
    }
    tcx X;
    H_mvs_make(D, X);
    tc_stage = STG_MVD;
    tc_syms[STG_MVD]++;
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    int base = comp ? 47 : 40;
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
      {
        // the sign (the mvg family), knowing the magnitude: whether it is
        // the template match's, the zero vector's, col's
        int m = abs(v);
        auto rel = [&](int d) { return sgn3(d) * 3 + (m == abs(d) ? 0 : m < abs(d) ? 1 : 2); };
        H_mvg_v G = {};
        G.comp = comp;
        G.lst = list;
        G.ts = tmn_ ? 9 : rel(dv);
        G.zs = rel(-mvp_[comp]);
        G.ds = pdn_ ? 9 : rel(pdv_[comp]);
        G.sa = sgn3(va);
        G.sb = sgn3(vb);
        G.mag = std::min(tc_qlog(m), 7);
        G.tq = D.tq;
        G.ox = comp ? sgn3(mvx_) : 3;
        G.ns = 3 + ns_[comp];
        G.es = ea == -128 && eb == -128 ? 3 : sgn3((ea == -128 ? 0 : ea) + (eb == -128 ? 0 : eb));
        G.as = tmn_ || talt_[0] == INT_MIN ? 9 : rel(talt_[comp] - mvp_[comp]);
        tcx Y;
        H_mvg_make(G, Y);
        f_mvg.select(Y);
        f_mvg.prp = -1;
        if (f_mvg.bit(0, ENC && v < 0)) v = -m;
      }
    }
    if (!comp) {
      mvx_ = v;
    } else {
      int mv[2] = {mvp_[0] + mvx_, mvp_[1] + v};
      set_mv(list, x, y, w, h, ref, mv);
    }
    int16_t s = (int16_t)tc_clamp(v, -32767, 32767);
    int8_t te = tmn_ ? (int8_t)-128 : (int8_t)tc_clamp(v - dv, -127, 127);
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) mi_->mvd[list][yy * 4 + xx][comp] = s, mi_->tme[list][yy * 4 + xx][comp] = te;
    return v;
  }

  // the macroblocks, as they were, of the picture list l's reference r is:
  // the last picture of a kind with its POC; nullptr none
  const MbCol* ref_hist(int l, int r) const {
    if (!rec_.active || r < 0 || r >= rec_.nlist[l] || !rec_.list[l][r]) return nullptr;
    int poc = rec_.list[l][r]->poc;
    if (poc == prev_poc_ && (int)hprev_.size() == size_) return hprev_ok_ ? hprev_.data() : nullptr;
    for (int k = 0; k < 30; k++)
      if (k != key_ && hist_poc_[k] == poc && (int)hist_[k].size() >= size_) return hist_[k].data();
    return nullptr;
  }
  // The block of the reference, as it was, that the vector of the current
  // macroblock's block of n x n samples at (x, y) (4x4 blocks) points at,
  // for an inter macroblock of a P picture whose reference is the last
  // picture of a kind: the macroblock there, and (bx, by) the block's place
  // in it (4x4 blocks, the top left one of an 8x8 block); moved, the vector
  // moves it; nullptr none.
  const MbCol* mc_col(int x, int y, int n, int& bx, int& by, int& moved) const {
    if ((sc_->stype_ != kP && sc_->stype_ != kSP) || mi_->cls >= CL_INXN || mbaff_) return nullptr;
    const MbCol* hr = ref_hist(0, mi_->rf[0][(y >> 1) * 2 + (x >> 1)]);
    if (!hr) return nullptr;
    const int16_t* v = mi_->mv[0][y * 4 + x];
    int still = abs(v[0]) <= 1 && abs(v[1]) <= 1;
    int px = addr_ % width_ * 16 + x * 4 + (still ? 0 : (int)div_floor(v[0] + 2, 4)) + n / 2;
    int py = addr_ / width_ * 16 + y * 4 + (still ? 0 : (int)div_floor(v[1] + 2, 4)) + n / 2;
    px = tc_clamp(px, 0, width_ * 16 - 1);
    py = tc_clamp(py, 0, size_ / width_ * 16 - 1);
    const MbCol& c = hr[(py >> 4) * width_ + (px >> 4)];
    if (!c.valid) return nullptr;
    moved = !still;
    bx = (px & 15) >> 2;
    by = (py & 15) >> 2;
    if (n == 8) bx &= 2, by &= 2;
    return &c;
  }
  // the 4x4 luma blocks with coefficients, of the reference, under where
  // the vector of the current macroblock's 8x8 block at (x, y) (4x4 blocks)
  // puts it, as mc_col: 0-4 still, 5-9 moved; -1 none
  int mc_nz(int x, int y) const {
    if ((sc_->stype_ != kP && sc_->stype_ != kSP) || mi_->cls >= CL_INXN || mbaff_) return -1;
    const MbCol* hr = ref_hist(0, mi_->rf[0][(y >> 1) * 2 + (x >> 1)]);
    if (!hr) return -1;
    const int16_t* v = mi_->mv[0][y * 4 + x];
    int still = abs(v[0]) <= 1 && abs(v[1]) <= 1, nz = 0, h = size_ / width_ * 16;
    int px = addr_ % width_ * 16 + x * 4 + (still ? 0 : (int)div_floor(v[0] + 2, 4)) + 2;
    int py = addr_ / width_ * 16 + y * 4 + (still ? 0 : (int)div_floor(v[1] + 2, 4)) + 2;
    for (int k = 0; k < 4; k++) {
      int qx = tc_clamp(px + (k & 1) * 4, 0, width_ * 16 - 1), qy = tc_clamp(py + (k >> 1) * 4, 0, h - 1);
      const MbCol& c = hr[(qy >> 4) * width_ + (qx >> 4)];
      if (!c.valid) return -1;
      nz += c.nzm[0] >> (((qy & 15) >> 2) * 4 + ((qx & 15) >> 2)) & 1;
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
  int qpd(int v) {
    prepare();
    tc_stage = STG_QPD;
    aq_xp_ = -1;
    aq_xi_ = false;
    const MbInfo* b = info(sc_->mb_b_);
    H_qpd_v D = {};
    D.lnz = last_qpd_ != 0;
    D.pc = popc4(mi_->cbp & 15);
    D.cc = mi_->cbp >> 4;
    D.cls = mi_->cls;
    D.dc = col_ ? qp_class(col_->qp - qp_off_ - qp_pred_) : 0;
    D.db = b ? qp_class(b->qp - qp_off_ - qp_pred_) : 0;
    // and where B is of the same kind (inter, I_NxN, other)
    auto kind3 = [](int c) { return c < CL_INXN ? 0 : c == CL_INXN ? 1 : 2; };
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
      int e0 = qp_src_ >= 0 ? rec_.energy[qp_src_] : rec_.energy_mean;
      D.ae = 7 + tc_clamp((rec_.energy[addr_] - e0 + 2 * (rec_.energy[addr_] > e0) - 2 * (rec_.energy[addr_] < e0)) / 4, -6, 6);
    }
    // and the QP its line gives the energy of the prediction (inter) or of
    // the last picture there, less the predicted QP; how far the last
    // picture of the kind was from its line there
    D.aq = D.ac = 0;
    int aqd = INT_MIN;  // the change the line asks for
    if (rec_.active && (int)aq_x_.size() == size_) {
      // intra: the mean of the last picture's there and the left and upper
      // neighbours'
      int x = -1, xs = 0, xn = 0;
      if ((int)aq_xl_.size() == size_ && aq_xl_[addr_] >= 0) xs += aq_xl_[addr_], xn++;
      if (addr_ % width_ && aq_x_[addr_ - 1] >= 0) xs += aq_x_[addr_ - 1], xn++;
      if (addr_ >= width_ && aq_x_[addr_ - width_] >= 0) xs += aq_x_[addr_ - width_], xn++;
      if (xn) x = xs / xn;
      int q16;
      bool ok = false;
      if (mi_->cls < 6) {
        aq_xp_ = lg16(rec_.ac_energy(addr_));
        ok = aq_fitp_.predict(aq_xp_, q16);
        if (!ok) x = aq_xp_;
      } else if (mi_->cls != CL_PCM && sc_->chroma_ == 1) {
        // intra: the energy of its proxy, a reference's block
        proxy_mv();
        int64_t e = rec_.proxy_energy();
        if (e >= 0) {
          aq_xp_ = lg16(e);
          aq_xi_ = true;
          ok = aq_fitx_.predict(aq_xp_, q16) || aq_fitp_.predict(aq_xp_, q16);
        }
      }
      D.xe = aq_xp_ < 0 ? 0 : 1 + std::min(aq_xp_ / 16, 17);
      // the QP of the last macroblock of the class: an encoder's QP is its
      // rate control's plus what the energy adds
      int lk = mi_->cls < CL_INXN ? 0 : mi_->cls == CL_INXN ? 1 : 2;
      if (lq_[lk][D.xe]) D.lq = 7 + tc_clamp(lq_[lk][D.xe] - 1 - qp_pred_, -6, 6);
      if (ok || (x >= 0 && aq_fit_.predict(x, q16))) {
        aqd = (int)div_floor(q16 - 16 * qp_pred_ + 8, 16);
        D.aq = 7 + tc_clamp(aqd, -6, 6);
      }
      int ma = mc_addr();
      if ((int)aq_res_[key_].size() == size_ && aq_res_[key_][ma] != INT16_MIN)
        D.ac = 7 + tc_clamp((int)div_floor(aq_res_[key_][ma] + 4, 8), -6, 6);
    }
    tcx X;
    H_qpd_make(D, X);
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    pr[0] = cab1(60 + (sc_->last_qpd_ != 0));
    f_qpd.prpa = pr;
    f_qpd.hint = aqd == INT_MIN ? -1 : abs(aqd);
    {
      int m = cm_int(f_qpd.code(X, ENC ? abs(v) : 0), 1 << 20);
      bool neg = ENC && v < 0;
      v = m;
      if (m) {
        // the sign (the qpg family), knowing the magnitude: on which side of
        // the line's delta, col's and B's it puts the QP
        auto rel = [&](int d) { return sgn3(d) * 3 + (m == abs(d) ? 0 : m < abs(d) ? 1 : 2); };
        H_qpg_v G = {};
        G.as = aqd == INT_MIN ? 9 : rel(aqd);
        G.cs = col_ ? rel(col_->qp - qp_off_ - qp_pred_) : 9;
        G.bs = b ? rel(b->qp - qp_off_ - qp_pred_) : 9;
        G.ls = has_lz_ ? sgn3(lz_) : 0;
        G.rs = D.ac ? sgn3(D.ac - 7) : 3;
        G.mag = std::min(tc_qlog(m), 7);
        G.cls = mi_->cls;
        G.st = st_;
        G.ss = rel(slice_qp_ - qp_pred_);
        G.xe = D.xe;
        G.lq = D.lq ? rel(D.lq - 7) : 9;
        G.bk = D.bc ? rel(D.bc - 7) : 9;
        tcx Y;
        H_qpg_make(G, Y);
        f_qpg.select(Y);
        f_qpg.prp = -1;
        v = f_qpg.bit(0, neg) ? -m : m;
      }
    }
    f_qpd.hint = -1;
    f_qpd.prpa = nullptr;
    int qp = ((qp_pred_ + v + 52 + 2 * qp_off_) % (52 + qp_off_) + (52 + qp_off_)) % (52 + qp_off_) - qp_off_;
    qp_pred_ = qp;
    qp_src_ = addr_;
    lq_[mi_->cls < CL_INXN ? 0 : mi_->cls == CL_INXN ? 1 : 2][D.xe] = qp + 1;
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

  // --- blocks of coefficients: whether a block has a nonzero one (cbf),
  // the significance map (sig), the levels in reverse scan order (lvl) and
  // the signs (sgn)
  struct Geo {
    int plane, x, y;  // 4x4 position of the block (of the top left one of an 8x8 block)
    int kind;         // 0 DC, 1 4x4 or AC, 2 chroma AC (4:2:0, 4:2:2), 3 8x8
    int off;          // first scan position in the store: 1 for AC blocks
    int pos;          // place in the 8x8 block (in the macroblock for chroma)
  };
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
  // the coefficient at scan position j of a block like g's in macroblock
  // m, at 4x4 block i
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
      int k = 0, m = g.kind == 3 ? 64 : g.kind == 0 ? std::min(n, 16) : n;
      while (k < m && !c[k]) k++;
      if (k == m) return;
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
    return std::min<int>(n.s->nz[g.plane][n.i], 8);
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
  void block(int cat, int idx, int* c, int n, bool coded) {
    prepare();
    if (cat == 2 || cat == 5) rec_.luma_ready(idx, mi_->qp - qp_off_);
    if (cat == 4 && sc_->chroma_ == 1) rec_.chroma_ready(idx >> 2 & 1, idx & 3, mi_->qp - qp_off_);
    Geo g = geo(cat, idx);
    const MbState& cur = *sc_->cur_;
    Nb na = nb_block(g, 0), nb = nb_block(g, 1);
    int q = q6();
    int nc = nc_class(block_nc(g)), md = block_mode(g);
    // the texture of the block's prediction
    int gxy[2] = {0, 0};
    int pt = g.kind == 0 ? rec_.pred_texture(g.plane, 0, 0, g.plane ? 8 : 16, gxy)
             : g.kind == 2 ? rec_.pred_texture(g.plane, g.x * 4, g.y * 4, 4, gxy)
                           : rec_.pred_texture(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4, gxy);
    // which way the prediction's texture runs (a motion error along it
    // leaves a residual of those frequencies): 4 + log2 of the differences
    // along rows over those along columns, -3 to 3 (0 flat or none)
    int hv = 0;
    if (pt > 0) {
      int a = gxy[0] + 1, b = gxy[1] + 1, l = 0;
      while (a >= 2 * b && l < 3) a /= 2, l++;
      while (b >= 2 * a && l > -3) b /= 2, l--;
      hv = 4 + l;
    }
    // (for the count and the significance, against the quantizer's step)
    int pga = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog(pt), 0, 15);
    int qs = qstep16(mi_->qp - qp_off_);
    int pg = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)pt * 16 / qs), 0, 15);
    // and how far it is from continuing its surroundings (against the
    // step too)
    int ms = g.kind == 0 ? rec_.edge_miss(g.plane, 0, 0, g.plane ? 8 : 16)
             : rec_.edge_miss(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4);
    int em = ms < 0 ? 0 : 1 + tc_clamp(tc_qlog((int64_t)ms * 256 / qs), 0, 15);
    // an intra block: what its levels would be were the source what the
    // reference shows (Recon::proxy_levels), by scan position
    int plv[64];
    bool pok = false;
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
        } else if (!g.plane && mi_->cls == CL_INXN && (cat == 2 || cat == 5) && idx > prx_blk_) {
          prx_blk_ = idx;
          inner = 2;
        }
        int p[2] = {prx_v_[0], prx_v_[1]};
        if (inner && rec_.tm_search(-1, 0, 0, 0, 16, 16, p, false, nullptr, 0, nullptr, 0, 1, inner)) {
          rec_.proxy_build(p);
          prx_v_[0] = p[0];
          prx_v_[1] = p[1];
        }
      }
      if (g.kind == 0)
        pok = rec_.proxy_dc(g.plane, g.plane ? rec_.chroma_qp_of(g.plane - 1, qp) : qp, plv);
      else
        pok = rec_.proxy_levels(g.kind == 2 ? g.plane : 0, g.x * 4, g.y * 4,
                                g.kind == 2 ? rec_.chroma_qp_of(g.plane - 1, qp) : qp, plv, g.kind == 3 ? 8 : 4);
    }
    // in quarters (q4), and as the level an encoder would code
    auto q4 = [&](int j) { return plv[std::min(j + g.off, g.kind == 3 ? 63 : g.kind == 0 && g.plane ? 3 : 15)]; };
    auto est = [&](int j) {
      int a = (3 * abs(q4(j)) + 4) / 12;
      return q4(j) < 0 ? -a : a;
    };
    // (from each position on)
    int pnz = 0, esuf[65];
    esuf[n] = 0;
    if (pok) {
      for (int j = n - 1; j >= 0; j--) esuf[j] = esuf[j + 1] + (est(j) != 0);
      pnz = esuf[0];
    }
    // a luma block of a P picture on its first reference: the coefficients,
    // by scan position, of the block its vector points at in the last
    // picture of the kind (that reference); tmv: the vector moves it
    int tmv = 0, tbx = 0, tby = 0;
    const MbCol* tcol = g.plane == 0 && (g.kind == 1 || g.kind == 2 || g.kind == 3)
                            ? mc_col(g.x, g.y, g.kind == 3 ? 8 : 4, tbx, tby, tmv) : nullptr;
    if (tcol && tcol->cls >= CL_INXN) tcol = nullptr;
    bool tok = tcol != nullptr;
    // (where none of its rows were written, all zero)
    bool tany = tok && tcol->cm >> (tby * 4 + tbx) & (g.kind == 3 ? 0x33 : 1);
    auto tcf = [&](int j) {
      if (!tany) return 0;
      int k = j + g.off;
      if (g.kind == 3) {
        int q = k & 3, x = tbx + (q & 1), y = tby + (q >> 1);
        return (int)tcol->coef[y * 4 + x][k >> 2];
      }
      return (int)tcol->coef[tby * 4 + tbx][k];
    };
    int tnz = 0;
    if (tany)
      for (int j = 0; j < n; j++) tnz += tcf(j) != 0;
    // the number of nonzero coefficients
    tc_stage = STG_CBF;
    H_cbf_v D = {};
    D.pz = pok ? 1 + std::min(pnz, 15) : 0;
    D.hv = hv;
    D.ch = cnth_ & 15;  // the counts of the last two blocks coded (up to 3 each)
    // how far to trust it: how far the macroblock's luma decoded so far is
    // from the proxy (13-24), else how well its vector matched (1-12)
    int pq = 0;
    if (pok) {
      int pa = rec_.proxy_acc();
      pq = pa >= 0 ? 13 + tc_clamp(tc_qlog(pa), 0, 11) : pxq_;
    }
    D.pq = pq;
    D.tn = tok ? 1 + 5 * tmv + std::min(tnz, 4) : 0;
    D.cat = cat;
    D.na = nb_count(na, g);
    D.nb = nb_count(nb, g);
    D.intra = mi_->cls >= CL_INXN;
    D.q6 = q;
    D.np = 0;
    if (g.kind == 1)
      for (int b = idx & ~3; b < idx; b++) D.np += cur.nz[g.plane][blk_y(b) * 4 + blk_x(b)] != 0;
    else if (g.kind == 2)
      for (int b = idx - idx % sc_->ch_blocks_; b < idx; b++) {
        int bb = b % sc_->ch_blocks_;
        D.np += cur.nz[g.plane][(bb >> 1) * 2 + (bb & 1)] != 0;
      }
    D.pos = g.pos;
    // x264's decimation (an inter luma 4x4 block of a P or B slice): an 8x8
    // block whose 4x4 blocks score under 4 (6 when it is the macroblock's
    // only one) is zeroed, so a coded one's blocks make that up; what is
    // still owed, after the blocks before this one
    D.ds = 0;
    if (g.kind == 1 && g.plane == 0 && cat == 2 && mi_->cls < CL_INXN && (sc_->stype_ == kP || sc_->stype_ == kB)) {
      int score = 0, req = popc4(mi_->cbp & 15) == 1 ? 6 : 4;
      for (int b = idx & ~3; b < idx; b++) score += decimate_score(mi_->coef[0][blk_y(b) * 4 + blk_x(b)]);
      D.ds = 1 + std::max(0, req - score);
    }
    // a 4:2:0 chroma block: the coefficients of the luma under it (its 8x8
    // block; for DC, the macroblock's), 1 + tc_qlog (0 luma)
    D.ly = 0;
    if (g.plane && sc_->chroma_ == 1) {
      int n = 0;
      for (int b = 0; b < 16; b++)
        if (g.kind == 0 || ((b & 3) >> 1 == g.x && (b >> 2) >> 1 == g.y)) n += cur.nz[0][b];
      D.ly = 1 + std::min(tc_qlog(n), 9);
    }
    // a 4:2:0 chroma AC block: its DC, from the chroma DC levels' 2x2
    // transform; an Intra16x16 luma AC block, from the luma DC levels' 4x4
    // one (frame scan); 1 + tc_qlog (0 not such a block)
    D.dq = 0;
    if (g.kind == 2 && sc_->chroma_ == 1) {
      const int8_t* d = mi_->dc[g.plane];
      D.dq = 1 + std::min(tc_qlog(d[0] + (g.x ? -d[1] : d[1]) + (g.y ? -d[2] : d[2]) + (g.x ^ g.y ? -d[3] : d[3])), 7);
    } else if (cat == 1) {
      static const int kH[4][4] = {{1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1}};
      const int8_t* d = mi_->dc[0];
      int v = 0;
      for (int k = 0; k < 16; k++)
        if (d[k]) v += kH[g.y][kZz4[k] >> 2] * d[k] * kH[kZz4[k] & 3][g.x];
      D.dq = 1 + std::min(tc_qlog(v), 7);
    }
    D.crcb = 0;
    if (g.plane == 2 && sc_->chroma_ != 3)
      D.crcb = 1 + (g.kind == 0 ? cur.cbf_dc[1] : cur.nz[1][g.y * 2 + g.x] != 0);
    else if (g.plane == 2)
      D.crcb = 1 + (g.kind == 0 ? cur.cbf_dc[1] : cur.nz[1][g.y * 4 + g.x] != 0);
    D.col = 2;
    if (col_)
      D.col = g.kind == 0 ? col_->dcm >> g.plane & 1
                          : col_->nzm[g.plane] >> (g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x) & 1;
    D.nc = nc;
    D.kn = coded;
    D.md = md;
    D.en = map_en();
    D.pg = pg;
    D.em = em;
    tcx X;
    H_cbf_make(D, X);
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    if (!coded && sc_->cabac_) {
      int inc = g.kind == 0 ? sc_->cbf_inc_dc(g.plane)
                : g.kind == 2 ? sc_->cbf_inc_chroma_ac(g.plane - 1, g.x, g.y)
                : g.kind == 3 ? sc_->cbf_inc_8x8(g.plane, idx)
                              : sc_->cbf_inc_luma(g.plane, g.x, g.y);
      pr[0] = cab1(kCbfBase[cat] + inc);
    }
    f_cbf.prpa = pr;
    // the intra estimate's count, or that of the block under the vector
    f_cbf.hint = pok ? std::max(pnz - (int)coded, 0) : tok ? std::max(tnz - (int)coded, 0) : -1;
    int total = cm_int(f_cbf.code(X, ENC ? count_nonzero(c, n) - coded : 0), n) + coded;
    f_cbf.hint = -1;
    cnth_ = (cnth_ * 4 + std::min(total, 3)) & 15;
    f_cbf.prpa = nullptr;
    if (total > n) cm_fail("corrupt stream: %d coefficients in a block of %d", total, n);
    tc_syms[STG_CBF]++;
    if (!total) {
      for (int i = 0; i < n; i++) c[i] = 0;
      store_coef(g, c, n);
      rec_.luma_add(cat, idx, mi_->qp - qp_off_);
      if (cat == 4 && sc_->chroma_ == 1) rec_.chroma_add(idx >> 2 & 1, idx & 3, mi_->qp - qp_off_);
      return;
    }
    // what the samples around the block ask of it (sign prediction, below,
    // and the level estimates)
    SignPred sp;
    bool spok = cat <= 5 && (cat != 3 || sc_->chroma_ == 1) && sp.init(rec_, g, cat, mi_->qp - qp_off_, idx);
    // the prediction's own levels by position (its texture by frequency):
    // an inter residual's follow them
    int pfl[64];
    bool pfok = mi_->cls < CL_INXN && g.kind != 0 &&
                rec_.pred_levels(g.plane, g.x * 4, g.y * 4,
                                 g.plane ? rec_.chroma_qp_of(g.plane - 1, mi_->qp - qp_off_) : mi_->qp - qp_off_, pfl,
                                 g.kind == 3 ? 8 : 4);
    // where they are: per position, whether the coefficient is nonzero,
    // knowing how many are still to come
    tc_stage = STG_SIG;
    bool fld = sc_->field_mb();
    bool sig[64] = {};
    int rem = total, last = 0, cnt = total;
    H_sig_v S = {cat, 0, 0, 0, md, nc, fld, q, pg, em, 0};
    S.pq = pq;
    int sig_base = kSigBase[fld][cat], numc8x8 = sc_->chroma_ == 2 ? 2 : 1;
    bool b8x8 = cat == 5 || cat == 9 || cat == 13;
    for (int i = 0; rem; i++) {
      if (n - i == rem) {
        sig[i] = true;
      } else {
        S.pos = i;
        S.rem = tc_qlog(rem);
        S.zl = tc_qlog(n - i - rem);
        S.pe = spok && i < 4 ? sp.estimate(i) : 0;
        S.ps = pok ? 1 + std::min(abs(q4(i)), 6) : 0;
        // the estimate's levels from here on, against the coefficients left
        S.pr = pok ? 1 + 3 + tc_clamp(esuf[i] - rem, -3, 3) : 0;
        // the last two positions' significance (4 at the start)
        {
          // the frequencies left of and above it, where coded already (0
          // not, 1 zero, 2 not zero)
          int N = g.kind == 3 ? 8 : g.kind == 0 && g.plane ? 2 : 4, k = i + g.off;
          int r = N == 8 ? kZz8[k] : N == 4 ? kZz4[k] : k, fx = r % N, fy = r / N, fl = 0, fa = 0;
          auto known = [&](int rr) {
            int kk = N == 8 ? kInv8[rr] : N == 4 ? kInv4[rr] : rr, jj = kk - g.off;
            return jj >= 0 && jj < i ? 1 + sig[jj] : 0;
          };
          if (fx) fl = known(r - 1);
          if (fy) fa = known(r - N);
          S.ft = fl * 3 + fa;
        }
        {
          // the significance of the last eight positions (and where the
          // block starts)
          int h = 1;
          for (int k = 1; k <= 8 && k <= i; k++) h = h * 2 + sig[i - k];
          S.hb = h - 1;
          // and of the last three
          int h3 = 1;
          for (int k = 1; k <= 3 && k <= i; k++) h3 = h3 * 2 + sig[i - k];
          S.h3 = h3 - 1;
        }
        S.tz = tok ? 1 + 4 * tmv + std::min(abs(tcf(i)), 3) : 0;
        S.hv = hv;
        S.pf = pfok ? 1 + std::min(tc_qlog(abs(pfl[std::min(i + g.off, g.kind == 3 ? 63 : 15)])), 10) : 0;
        // the same position in the blocks A and B (0 none, 1 zero, 2 not)
        S.sn = (na.m ? 1 + (coef_at(na.m, g, na.i, i) != 0) : 0) * 3 + (nb.m ? 1 + (coef_at(nb.m, g, nb.i, i) != 0) : 0);
        tcx Y;
        H_sig_make(S, Y);
        f_sig.select(Y);
        f_sig.prp = cab0(sig_base + (cat == 3 ? std::min(i / numc8x8, 2) : b8x8 ? (fld ? kSig8x8Field[i] : kSig8x8Frame[i]) : i));
        sig[i] = f_sig.bit(0, ENC && c[i] != 0);
        tc_syms[STG_SIG]++;
      }
      if (sig[i]) {
        rem--;
        last = i;
      }
    }
    // the magnitudes, from the last coefficient back
    tc_stage = STG_LVL;
    int mag[64];
    int g1 = 0, e1 = 0, lhist = 0, lsum = 0, esum = 0;  // lhist: the last levels coded less 1 (up to 3), two bits each
    H_lvl_v L = {cat, 0, 0, nc, 0, 0, 0, q, tc_qlog(cnt), md, 0, pga, em, 0};
    L.pq = pq;
    int abs_base = kAbsBase[cat];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    f_lvl.prpa = pr;
    for (int k = last; k >= 0; k--) {
      if (!sig[k]) continue;
      L.g1 = g1;
      L.e1 = e1;
      pr[0] = cab1(abs_base + (g1 ? 0 : std::min(4, 1 + e1)));
      pr[1] = pr[2] = cab1(abs_base + 5 + std::min(4 - (cat == 3), g1));
      L.pos = tc_qlog(k);
      {
        // the texture's direction, and whether the coefficient is a
        // horizontal (1) or vertical (2) frequency or both (3) or DC (0)
        int N = g.kind == 3 ? 8 : g.kind == 0 && g.plane ? 2 : 4, kk = k + g.off;
        int r = N == 8 ? kZz8[kk] : N == 4 ? kZz4[kk] : kk;
        L.hv = hv;
        L.fq = (r % N > 0) + 2 * (r / N > 0);
      }
      L.pe = spok && k < 4 ? sp.estimate(k) : 0;
      L.pl = pok ? 1 + std::min(abs(q4(k)) / 2, 10) : 0;
      // intra: the levels so far against the estimate's there
      L.ls = pok ? 7 + tc_clamp(lsum - esum, -6, 6) : 0;
      {
        // the levels right of and below it (higher frequencies, coded already)
        int N = g.kind == 3 ? 8 : g.kind == 0 && g.plane ? 2 : 4, kk = k + g.off;
        int r = N == 8 ? kZz8[kk] : N == 4 ? kZz4[kk] : kk, fx = r % N, fy = r / N, sum = 0, cnt = 0;
        auto lv = [&](int rr) {
          int j = (N == 8 ? kInv8[rr] : N == 4 ? kInv4[rr] : rr) - g.off;
          if (j > k && j <= last && sig[j]) sum += mag[j];  // none significant beyond last
          cnt += j > k && j < n;
        };
        if (fx + 1 < N) lv(r + 1);
        if (fy + 1 < N && (N != 2 || r + N < n + g.off)) lv(r + N);
        L.lt = cnt ? 1 + std::min(sum, 6) : 0;
      }
      L.tz = tok ? 1 + 4 * tmv + std::min(abs(tcf(k)), 3) : 0;
      L.sa = na.m ? std::min(abs(coef_at(na.m, g, na.i, k)), 3) : 0;
      L.sb = nb.m ? std::min(abs(coef_at(nb.m, g, nb.i, k)), 3) : 0;
      tcx X;
      H_lvl_make(L, X);
      // the intra estimate's level there
      f_lvl.hint = pok ? std::max(abs(est(k)) - 1, 0) : -1;
      int a = cm_int(f_lvl.code(X, ENC ? abs(c[k]) - 1 : 0), 1 << 24);
      f_lvl.hint = -1;
      mag[k] = a + 1;
      lsum += a + 1;
      if (pok) esum += abs(est(k));
      L.pm = std::min(a + 1, 4);
      lhist = (lhist * 4 + std::min(a, 3)) & 63;
      L.lh = 1 + lhist;
      if (a)
        g1++;
      else
        e1++;
      tc_syms[STG_LVL]++;
    }
    f_lvl.prpa = nullptr;
    // the signs
    tc_stage = STG_SGN;
    H_sgn_v G = {cat, 0, 0, 0, 3, 0, md, 0, 0};
    G.pq = pq;
    bool cr = g.plane == 2;
    // sign prediction (luma blocks): the signs of the first nonzero
    // coefficients in scan order are coded after the others, each with the
    // sign that makes the block continue its surroundings best
    int npred = 0, pk[6];
    if (spok)
      for (int k = 0; k <= last && npred < 6; k++)
        if (sig[k]) pk[npred++] = k;
    int ghist = 0;  // the last two signs coded in the block (0 none, 1 +, 2 -)
    // an inter luma block: its residual taken for a small shift of its
    // prediction, the shift fitted (least squares) to the coefficients whose
    // signs are known so far (from the macroblock's last block before any)
    // (in integers, the format depending on no floating point)
    int glx[64], gly[64];
    bool gok = mi_->cls < CL_INXN && g.plane == 0 && (cat == 2 || cat == 5) &&
               rec_.pred_grad_levels(0, g.x * 4, g.y * 4, cat == 5 ? 8 : 4, mi_->qp - qp_off_, glx, gly);
    if (gok)
      for (int k = 0; k < n; k++) glx[k] = tc_clamp(glx[k], -2047, 2047), gly[k] = tc_clamp(gly[k], -2047, 2047);
    i64 sxx = 0, sxy = 0, syy = 0, sxc = 0, syc = 0;
    // the shift fitted so far: (fx, fy) / fd, scaled into 40 bits; fd 0 none
    auto fit = [&](i64& fx, i64& fy, i64& fd) {
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
      while (std::max(std::max(fx < 0 ? -fx : fx, fy < 0 ? -fy : fy), fd) >= ((i64)1 << 40)) fx /= 2, fy /= 2, fd /= 2;
    };
    int sagree = 0;  // the signs the intra estimate got right less those it got wrong
    auto code_sign = [&](int k) {
      G.pos = k == 0 ? 0 : k < 3 ? 1 : k < 6 ? 2 : 3;
      G.sa = na.m ? sgn3(coef_at(na.m, g, na.i, k)) : 3;
      G.sb = nb.m ? sgn3(coef_at(nb.m, g, nb.i, k)) : 3;
      if (cr) {
        Geo gb = g;
        gb.plane = 1;
        G.scb = sgn3(coef_at(mi_, gb, g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x, k));
      }
      G.mag = std::min(mag[k], 3) - 1;
      G.ez = pok ? (q4(k) ? (q4(k) > 0 ? 2 : 3) + 2 * (abs(q4(k)) >= 4) : 1) : 0;
      G.ts = tok ? 1 + 3 * tmv + sgn3(tcf(k)) : 0;
      G.gh = ghist % 9;
      // intra: how often the estimate's signs were right in the block
      G.se = pok ? 4 + tc_clamp(sagree, -3, 3) : 0;
      // what that shift says of this sign, and how surely (its level
      // against a quarter of the coefficient's, and against all of it)
      G.dm = 0;
      if (gok) {
        i64 fx, fy, fd;
        fit(fx, fy, fd);
        if (!fd) fx = shx_, fy = shy_, fd = shd_;
        i64 r = fx * glx[k] + fy * gly[k], ra = r < 0 ? -r : r, m = std::min(mag[k], 4096);
        if (fd && r) G.dm = 1 + (r < 0) + 2 * (ra > fd * m) + 2 * (ra > 4 * fd * m);
      }
      tcx X;
      H_sgn_make(G, X);
      f_sgn.select(X);
      int sg = f_sgn.bit(0, ENC && c[k] < 0);
      ghist = ghist % 3 * 3 + 1 + sg;
      if (pok && q4(k)) sagree += (q4(k) < 0) == (sg != 0) ? 1 : -1;
      c[k] = sg ? -mag[k] : mag[k];
      if (gok) {
        i64 lx = glx[k], ly = gly[k], cv = tc_clamp(4 * c[k], -4095, 4095);
        sxx += lx * lx, sxy += lx * ly, syy += ly * ly, sxc += lx * cv, syc += ly * cv;
      }
      tc_syms[STG_SGN]++;
      return sg;
    };
    for (int k = last; k >= 0; k--) {
      if (!sig[k]) {
        if (!ENC) c[k] = 0;
        continue;
      }
      bool predicted = false;
      for (int j = 0; j < npred; j++) predicted |= pk[j] == k;
      if (!predicted) {
        code_sign(k);
        if (npred) sp.add(k, c[k]);
      }
    }
    if (npred) {
      for (int j = 0; j < npred; j++) sp.cand(pk[j], mag[pk[j]]);
      sp.costs();
      for (int j = 0; j < npred; j++) {
        int margin, pred = sp.predict(j, margin);
        G.pp = 1 + pred;
        G.mg = 1 + tc_clamp(tc_qlog(margin), 0, 14);
        sp.decide(j, code_sign(pk[j]));
      }
      G.pp = G.mg = 0;
    }
    for (int k = last + 1; k < n; k++) c[k] = 0;
    if (gok) {
      // the shift fitted to the whole block, for the next block's first signs
      i64 fx, fy, fd;
      fit(fx, fy, fd);
      if (fd) shx_ = fx, shy_ = fy, shd_ = fd;
    }
    store_coef(g, c, n);
    rec_.coef(cat, idx, c, n);
    rec_.luma_add(cat, idx, mi_->qp - qp_off_);
    if (cat == 4 && sc_->chroma_ == 1) rec_.chroma_add(idx >> 2 & 1, idx & 3, mi_->qp - qp_off_);
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
      int depth = p ? sc_->sps_.bit_depth_chroma : sc_->sps_.bit_depth_luma;
      i64 d1 = 0, d2 = 0;
      for (int y = 0; y < hh; y++)
        for (int x = 0; x < w; x++, i++) {
          if (i >= n) cm_fail("internal: I_PCM sample count");
          int pred = x ? s[i - 1] : y ? s[i - w] : 1 << (depth - 1);
          int half = 1 << (depth - 1), mask = (1 << depth) - 1;
          H_pcm_v D = {p, depth - 8, sq2(d1), sq2(d2)};
          tcx X;
          H_pcm_make(D, X);
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

template <int f_DEC>
Rangecoder<f_DEC> Model<f_DEC>::rc;

#ifndef H264P_NO_THREADS
// TcDefer's records, coded, and their rows fetched ahead
static void tc_exec(TcDefer::Rec& r) {
  if (r.op == TcDefer::RAW) {
    static_cast<Model<0>*>(r.obj)->raw_x((u32)r.val);
    return;
  }
  if (r.op == TcDefer::FLAG) {
    static_cast<Model<0>*>(r.obj)->end_x(r.node, (int)r.val);
    return;
  }
  tc_fam<0>& f = *static_cast<tc_fam<0>*>(r.obj);
  switch (r.op) {
    case TcDefer::SEL:
      f.select_x(r.x);
      break;
    case TcDefer::BIT:
      f.cprp = r.prp, f.cprpa = nullptr;
      f.bit_x(r.node, (int)r.val);
      break;
    case TcDefer::CODE:
    case TcDefer::CODES:
      f.cprp = r.prp, f.cprpa = r.pa ? r.prpa : nullptr, f.chint = r.hint;
      if (r.op == TcDefer::CODE)
        f.code_x(r.x, r.val);
      else
        f.codes_x(r.x, r.val, (u32)r.node);
      break;
    case TcDefer::SIGN:
      f.sign_x((u32)r.node, (int)r.val);
      break;
    default:
      break;
  }
}
static void tc_pre(const TcDefer::Rec& r) {
  if (r.op == TcDefer::SEL || r.op == TcDefer::CODE || r.op == TcDefer::CODES) static_cast<const tc_fam<0>*>(r.obj)->pre(r.x);
}
#endif

// The channel between the walk and the model. Coding, it takes each value
// from the log of the dry run and has the model code it; decoding, the
// model decodes it. Either way the value goes back to the walk.
template <int f_DEC>
struct ModelChan : Chan {
  static constexpr int ENC = f_DEC == 0;
  ModelChan(Model<f_DEC>& m, LogChan* log) : Chan(true), m_(m), log_(log) {}

  void hbegin(const char* name) override { m_.hbegin(name); }
  void hbits(int n) override { m_.hbits(n); }
  int64_t hval(int64_t) override { return m_.hval(ENC ? log_->hval(0) : 0); }
  void hbytes(std::vector<uint8_t>& b) override {
    if (ENC) log_->hbytes(b);
    for (auto& x : b) x = (uint8_t)m_.raw(x);
  }
  void hend() override {}
  bool hmore(const char* next, bool) override { return m_.hmore(next, ENC && log_->hmore(next, false)); }
  void slice_begin(SliceCoder& sc) override { m_.slice_begin(sc); }
  void slice_end() override { m_.slice_end(); }
  int skip(int) override { return m_.skip(ENC ? log_->skip(0) : 0); }
  int end(int, bool certain) override { return m_.end(ENC ? log_->end(0, certain) : 0, certain); }
  void mb_begin() override { m_.mb_begin(); }
  void mb_end() override { m_.mb_end(); }
  int field(int) override { return m_.field(ENC ? log_->field(0) : 0); }
  int mb_type(int) override { return m_.mb_type(ENC ? log_->mb_type(0) : 0); }
  int t8x8(int) override { return m_.t8x8(ENC ? log_->t8x8(0) : 0); }
  int ipm(int blk, int) override { return m_.ipm(blk, ENC ? log_->ipm(blk, 0) : 0); }
  int cpred(int) override { return m_.cpred(ENC ? log_->cpred(0) : 0); }
  int sub(int i, int) override { return m_.sub(i, ENC ? log_->sub(i, 0) : 0); }
  int ref(int list, int x, int y, int w, int h, int) override {
    return m_.ref(list, x, y, w, h, ENC ? log_->ref(list, x, y, w, h, 0) : 0);
  }
  int mvd(int list, int comp, int x, int y, int w, int h, int) override {
    return m_.mvd(list, comp, x, y, w, h, ENC ? log_->mvd(list, comp, x, y, w, h, 0) : 0);
  }
  int cbp(int) override { return m_.cbp(ENC ? log_->cbp(0) : 0); }
  int qpd(int) override { return m_.qpd(ENC ? log_->qpd(0) : 0); }
  void block(int cat, int idx, int* c, int n, bool coded) override {
    if (ENC) log_->block(cat, idx, c, n, coded);
    m_.block(cat, idx, c, n, coded);
  }
  void pcm(int& align, int nb, int* s, int n) override {
    if (ENC) log_->pcm(align, nb, s, n);
    m_.pcm(align, nb, s, n);
  }
  [[noreturn]] void fail(const std::string& msg) override {
    throw CmError(std::string(ENC ? "internal: " : "corrupt stream: ") + msg);
  }

  Model<f_DEC>& m_;
  LogChan* log_;
};

}  // namespace h264p
