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
constexpr int H_mvs_ND = TC_NODE, H_mvs_MND = TC_MNODE, H_mvs_NG = 243;
constexpr int H_qpd_ND = TC_NODE, H_qpd_MND = TC_MNODE, H_qpd_NG = 1;
constexpr int H_cbf_ND = TC_NODE, H_cbf_MND = TC_MNODE, H_cbf_NG = 1;
constexpr int H_sig_ND = 1, H_sig_MND = 1, H_sig_NG = 1;
constexpr int H_lvl_ND = TC_NODE, H_lvl_MND = TC_MNODE, H_lvl_NG = 1;
constexpr int H_sgn_ND = 1, H_sgn_MND = 1, H_sgn_NG = 1;
constexpr int H_pcm_ND = TC_NODE, H_pcm_MND = TC_MNODE, H_pcm_NG = 3;

struct H_nal_v { int fld, ctx, p1, p2; };
struct H_hdr_v { int tag, fld, p1, p2, pos, p1r, sty; };
struct H_skp_v { int st, a3, b3, col, c9, cd, run, cnt, eq, lf, af, td, en, sm, tm, tb, bd, qc; };
struct H_mbt_v { int st, ca, cb, cc, nsk, en, td, sm, tm, tb, dh, bd, qc; };
struct H_cbp_v { int inc, pc, cls, col, q6, st, en, td, pg, tp, em, pq; };
struct H_ipm_v { int k8, ma, mb, mp, blk, pv, col, dm, ds, pm, pr, dr, pq; };
struct H_mvs_v { int kind, comp, lst, ma, mb, oth, ps, rf, pi, nr, pd, sp, tv, zd, cq, tq; };
struct H_qpd_v { int lnz, pc, cc, cls, dc, db, lz, st, ae, aq, ac; };
struct H_cbf_v { int cat, na, nb, intra, q6, np, pos, crcb, col, nc, kn, md, en, pg, em, pz, tn, pq; };
struct H_sig_v { int cg, pos, rem, zl, md, nc, fld, q6, pg, em, pe, ps, tz, pq, hb; };
struct H_lvl_v { int cg, g1, e1, nc, pos, sa, sb, q6, tot, md, pm, pg, em, pe, pl, tz, pq, lh; };
struct H_sgn_v { int cg, pos, sa, sb, scb, mag, md, pp, mg, ez, ts, pq; };
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
#include "MOD/pcm_p.inc"

// A family's tables, mapped: zero is every table's starting state, and a
// page costs memory only once the coder reaches it.
template <class T>
static T* tc_table() {
  return new (tc_map(sizeof(T))) T;
}

#define H_WIRE(F)                                                                                                   \
  f_##F.wire(t_##F->H_##F##_A, H_##F##_a_Volume, t_##F->H_##F##_B, H_##F##_b_Volume, t_##F->H_##F##_C,              \
             H_##F##_c_Volume, t_##F->H_##F##_D, H_##F##_d_Volume, t_##F->H_##F##_E, H_##F##_e_Volume, t_##F->H_##F##_H, H_##F##_h_Volume, t_##F->H_##F##_MT, H_##F##_t_Volume,             \
             t_##F->H_##F##_S, t_##F->H_##F##_SC, H_##F##_s_Volume, t_##F->H_##F##_F, t_##F->H_##F##_FC,            \
             H_##F##_f_Volume, t_##F->H_##F##_W, H_##F##_m_Volume, t_##F->H_##F##_G, H_##F##_NG, H_##F##_g_Volume,  \
             t_##F->H_##F##_WM, H_##F##_n_Volume, t_##F->H_##F##_WC, t_##F->H_##F##_WMC, t_##F->H_##F##_W2,            \
             t_##F->H_##F##_W2C, H_##F##_w_Volume, t_##F->H_##F##_W3, t_##F->H_##F##_W3C, t_##F->H_##F##_WF, t_##F->H_##F##_WFC, H_##F##_rA, H_##F##_rB,                    \
             H_##F##_rC, H_##F##_rD, H_##F##_rE, H_##F##_rH, H_##F##_rT, H_##F##_rG, H_##F##_mwA, H_##F##_mwB, H_##F##_mwC, H_##F##_mwD, H_##F##_mwE, H_##F##_mwH,    \
             H_##F##_mwT, H_##F##_mwG, H_##F##_rS1, H_##F##_rS2, H_##F##_lr, H_##F##_mb, H_##F##_lrm, H_##F##_mbm,  \
             H_##F##_bw, H_##F##_qs, H_##F##_qf, H_##F##_ND, H_##F##_MND, H_##F##_hA, H_##F##_hB, H_##F##_hD, H_##F##_hE, H_##F##_hH,       \
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
  uint8_t modes[16];        // Intra4x4/8x8 modes of the 4x4 blocks, raster order (2 for other macroblocks)
  int8_t ref[2][4];         // ref_idx of the 8x8 blocks as coded
  int8_t rf[2][4];          // refIdxLX of the 8x8 blocks, -1 where list X is not used
  int16_t mvd[2][16][2];    // mvd of the 4x4 blocks
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
  void clear() { n = sx = sy = sxx = sxy = 0; }
  void add(int x, int q) {
    n++;
    sx += x;
    sy += q;
    sxx += (i64)x * x;
    sxy += (i64)x * q;
  }
  // 16 x the QP of energy x on the line; false while there is none
  bool predict(int x, int& q16) const {
    if (n < 9) return false;
    i64 den = n * sxx - sx * sx;
    if (den <= 0) return false;
    i64 b = (n * sxy - sx * sy) * 4096 / den;  // QP per unit of x, x 4096
    b = b < 0 ? 0 : b > 768 ? 768 : b;
    q16 = (int)((16 * sy + 16 * b * ((i64)x * n - sx) / 4096) / n);
    return true;
  }
};

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
template <int f_DEC>
struct Model {
  static constexpr int ENC = f_DEC == 0;
  static Rangecoder<f_DEC> rc;

  tc_fam<f_DEC> f_nal, f_hdr, f_skp, f_mbt, f_cbp, f_ipm, f_mvs, f_qpd, f_cbf, f_sig, f_lvl, f_sgn, f_pcm;
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
  H_pcm_T* t_pcm;

  void init() {
    cm_tables();
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
    H_MAP(pcm);
#undef H_MAP
    raw_mx_.init(raw_w_, raw_wc_, 256);
  }
  u64 table_bytes() const {
    return (u64)t_nal->H_nal_Size + t_hdr->H_hdr_Size + t_skp->H_skp_Size + t_mbt->H_mbt_Size + t_cbp->H_cbp_Size +
           t_ipm->H_ipm_Size + t_mvs->H_mvs_Size + t_qpd->H_qpd_Size + t_cbf->H_cbf_Size + t_sig->H_sig_Size +
           t_lvl->H_lvl_Size + t_sgn->H_sgn_Size + t_pcm->H_pcm_Size;
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

  // bytes nothing else models: an order 0 and an order 1 counter mixed by
  // a weight set the bits of the byte so far select (oggcomp's raw model)
  cm_cnt raw_o0_[256];
  cm_cnt raw_o1_[256][256];
  i32 raw_w_[256 * 3];
  u8 raw_wc_[256];
  cm_mix<3> raw_mx_;
  u32 raw_prev_ = 0;
  u32 raw(u32 b) {
    cm_cnt* row = raw_o1_[raw_prev_];
    u32 node = 1;
    tc_stage = STG_RAW;
    for (int i = 7; i >= 0; i--) {
      cm_cnt c0 = raw_o0_[node], c1 = row[node];
      raw_mx_.set(0, cm_stretch(c0.P()));
      raw_mx_.set(1, c1.t ? cm_stretch(c1.P()) : 0);
      raw_mx_.set(2, 256);
      int bit = tc_bit<f_DEC>(raw_mx_.mix(node), (int)(ENC && ((b >> i) & 1)));
      raw_mx_.upd(bit, 24, 0);
      c0.upd(bit, 250, 0);
      c1.upd(bit, 60, 0);
      raw_o0_[node] = c0;
      row[node] = c1;
      node = node * 2 + (u32)bit;
    }
    raw_prev_ = node & 255;
    tc_syms[STG_RAW]++;
    return node & 255;
  }

  // ------------------------------------------------------------------------
  // header syntax: a value is coded as the difference to the last value of
  // the same element (tag, place in the group), kept per kind of slice
  struct HLast {
    i64 v, d1, d2;  // the last value, the last two differences
  };
  std::map<std::string, int> tags_;
  int tag_ = 0, fld_ = 0, lasttag_ = -1, runpos_ = 0;
  int sty_ = 10, last_sty_ = 10, ref_idc_ = 0;  // slot of the current NAL unit: slice type x reference, 10 none
  bool slice_nal_ = false;
  bool is_slice_type_ = false;
  HLast hl_[256][4][11] = {};

  // the start of a NAL unit coded by the walk
  void nal_begin(int type, int ref_idc) {
    slice_nal_ = type == 1 || type == 5;
    ref_idc_ = ref_idc;
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
    fld_ = 0;
    runpos_ = tag_ == lasttag_ ? runpos_ + 1 : 0;
    lasttag_ = tag_;
  }
  i64 hval(i64 x) {
    tc_stage = STG_HDR;
    HLast& h = hl_[tag_][fld_ < 3 ? fld_ : 3][sty_];
    H_hdr_v D = {tag_, fld_, sq2(h.d1), sq2(h.d2), tc_qlog(runpos_), (int)(h.v & 255), sty_ < 9 ? sty_ : 9};
    tcx X;
    H_hdr_make(D, X);
    i64 d = f_hdr.codes(X, ENC ? x - h.v : 0, (u32)tag_);
    x = h.v + d;
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
  MbCol* hcur_ = nullptr;  // hist_ of the current slice's kind
  uint32_t pic_ = 1;       // pictures so far
  int st_ = 0, key_ = 0;   // slice type x reference; st_ x frame/top/bottom field
  int width_ = 1, size_ = 0;
  bool mbaff_ = false;
  int qp_off_ = 0, qp_pred_ = 26;
  int last_qpd_ = 0, lz_ = 0;  // the last mb_qp_delta, the last nonzero one
  int qp_src_ = -1;            // the macroblock that coded it
  bool has_lz_ = false, qpd_coded_ = false;
  int run_ = 0, cnt_ = 0, prev_cnt_ = 0;  // skipped before, macroblocks in the slice so far, in the last one
  int slice_cnt_[30] = {};
  bool mb_open_ = false;
  int addr_ = 0;
  MbInfo* mi_ = nullptr;
  const MbCol* col_ = nullptr;
  int sub_[4] = {};
  int pmv_[2] = {};  // proxy_mv()
  int pxq_ = 0;      // proxy_mv(): how well its vector matches the samples around (1 + tc_qlog, 0 not known)
  bool pmv_ok_ = false;
  std::vector<int16_t> pxv_;  // the vectors of the current picture's proxies (INT16_MIN none)
  int sm_ = 0, tm_ = 0;  // skip_miss() of the macroblock, its template matching miss
  int tb_ = 0, dh_ = 0;  // skip_miss(): the best template match on list 0, the better list (B)
  int bd_ = 0;           // skip_miss(): the skip motion's two predictions apart (B)
  int tmb_[2] = {};      // that vector
  bool tmb_ok_ = false;
  int prev_mode_ = 9, mvx_ = 0;
  int parts_ = 0, subshape_[4] = {};  // partitions of the macroblock: 16x16, 16x8, 8x16, 8x8; sub-macroblock shapes
  int mvp_[2] = {}, sp_[2] = {}, pdv_[2] = {};  // the current partition: mvp, spread of the neighbours, col - mvp
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
  AqFit aq_fit_, aq_fitp_;
  int aq_xp_ = -1;  // 16 log2 energy of the current macroblock's prediction (inter)
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
    int x, y;
    if (!mbaff_) {
      x = addr_ % width_ + dx;
      y = addr_ / width_ + dy;
    } else {
      int p = addr_ >> 1;
      x = p % width_ + dx;
      y = (p / width_) * 2 + (addr_ & 1) + dy;
    }
    if (x < 0 || x >= width_ || y < 0 || y >= size_ / width_) return -1;
    return mbaff_ ? ((y >> 1) * width_ + x) * 2 + (y & 1) : y * width_ + x;
  }
  // a macroblock of the current picture, coded already
  const MbInfo* near_info(int dx, int dy) const {
    int a = mb_near(dx, dy);
    return a >= 0 && info_[a].pic == pic_ ? &info_[a] : nullptr;
  }

  void slice_begin(SliceCoder& sc) {
    sc_ = &sc;
    mbs_ = sc.store_.mbs.data();
    const SliceHeader& h = sc.h_;
    st_ = h.type() * 2 + (h.nal_ref_idc != 0);
    key_ = st_ * 3 + (h.field_pic_flag ? 1 + h.bottom_field_flag : 0);
    if (h.first_mb == 0) pic_++;
    width_ = sc.width_;
    size_ = sc.size_;
    mbaff_ = sc.mbaff_;
    if ((int)info_.size() < size_) info_.resize(size_);
    if ((int)hist_[key_].size() < size_) hist_[key_].resize(size_);
    hcur_ = hist_[key_].data();
    qp_off_ = 6 * (sc.sps_.bit_depth_luma - 8);
    qp_pred_ = sc.pps_.pic_init_qp + h.slice_qp_delta;
    last_qpd_ = 0;
    has_lz_ = false;
    qp_src_ = -1;
    run_ = cnt_ = 0;
    prev_cnt_ = slice_cnt_[key_];
    mb_open_ = false;
    rec_.slice_begin(h);
    if (h.first_mb == 0) aq_picture();
  }
  void slice_end() { slice_cnt_[key_] = cnt_; }

  void begin_mb() {
    addr_ = sc_->cur_addr_;
    mi_ = &info_[addr_];
    memset(mi_, 0, sizeof *mi_);
    mi_->pic = pic_;
    mi_->qp = (uint8_t)(qp_pred_ + qp_off_);
    memset(mi_->modes, 2, sizeof mi_->modes);
    memset(mi_->rf, -1, sizeof mi_->rf);
    col_ = hcur_[addr_].valid ? &hcur_[addr_] : nullptr;
    parts_ = 0;
    rec_.mb_start(addr_);
    qpd_coded_ = false;
    prev_mode_ = 9;
    sm_ = tm_ = tb_ = dh_ = bd_ = 0;
    tmb_ok_ = false;
    pmv_ok_ = false;
    mb_open_ = true;
    cnt_++;
  }
  // into the history of the slice's kind
  void finish_mb() {
    const MbState& s = *sc_->cur_;
    MbCol& c = hcur_[addr_];
    c.valid = 1;
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
    for (int l = 0; l < 2; l++)
      for (int b = 0; b < 16; b++)
        for (int k = 0; k < 2; k++) c.mvq[l][b][k] = (uint8_t)tc_qlog(mi_->mvd[l][b][k]);
    for (int p = 0; p < 3; p++) {
      c.nzm[p] = 0;
      for (int b = 0; b < 16; b++) c.nzm[p] |= (uint16_t)((s.nz[p][b] != 0) << b);
    }
    c.dcm = (uint8_t)(s.cbf_dc[0] | s.cbf_dc[1] << 1 | s.cbf_dc[2] << 2);
    memcpy(c.coef, mi_->coef[0], sizeof c.coef);
    mb_open_ = false;
    rec_.mb_end(addr_, mi_->cls, mi_->i16m, mi_->modes, mi_->cpred, mi_->t8, mi_->qp - qp_off_, mi_->rf, mi_->mv, s.nz[0]);
    if (rec_.active && (int)aq_x_.size() == size_) {
      int x = lg16(rec_.ac_energy(addr_));
      aq_x_[addr_] = (int16_t)x;
      if (qpd_coded_) {
        aq_fit_.add(x, mi_->qp - qp_off_);
        if (aq_xp_ >= 0) aq_fitp_.add(aq_xp_, mi_->qp - qp_off_);
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
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++) {
        int n = mb_near(dx, dy);
        D.c9 += n >= 0 && hcur_[n].valid && hcur_[n].skip;
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
  // first reference (from mvp, the neighbours' vectors, zero): false none;
  // miss as Recon::tm_search's
  bool tm_best(int l, int* mv, int& miss) const {
    MvN nbs[3];
    mv_pred(l, 0, 0, 4, 4, 0, mv, nbs);
    int cand[4][2], nc = 0;
    for (int j = 0; j < 3; j++)
      if (nbs[j].ref == 0) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
    cand[nc][0] = cand[nc][1] = 0, nc++;
    return rec_.tm_search(l, 0, 0, 0, 16, 16, mv, false, cand, nc, &miss);
  }
  int skip_miss() {
    tm_ = tb_ = dh_ = bd_ = 0;
    if (!rec_.active) return 0;
    // how well the reference can predict the samples around at best (list
    // 0), and which list does it better
    int m0, m1;
    tmb_ok_ = tm_best(0, tmb_, m0);
    if (tmb_ok_) tb_ = 1 + tc_clamp(tc_qlog(m0), 0, 15);
    int v1[2];
    if (tmb_ok_ && sc_->stype_ == kB && tm_best(1, v1, m1)) dh_ = m0 * 4 < m1 * 3 ? 1 : m1 * 4 < m0 * 3 ? 2 : 3;
    if (sc_->stype_ == kB)
      mv_direct(15);
    else
      mv_pskip();
    int tm = rec_.tm_miss(mi_->rf, mi_->mv);
    tm_ = tm < 0 ? 0 : 1 + tc_clamp(tc_qlog(tm), 0, 15);
    // B: how far apart the two predictions of the skip motion are; P: how
    // far the skip vector is from the best template match's
    int bd = -1;
    if (sc_->stype_ == kB)
      bd = rec_.bi_diff(mi_->rf, mi_->mv);
    else if (tmb_ok_)
      bd = abs(tmb_[0] - mi_->mv[0][0][0]) + abs(tmb_[1] - mi_->mv[0][0][1]);
    bd_ = bd < 0 ? 0 : 1 + tc_clamp(tc_qlog(bd), 0, 15);
    int ms = rec_.skip_miss(mi_->rf, mi_->mv);
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
      if (sc_->stype_ == kB)
        mv_direct(15);
      else
        mv_pskip();
      last_qpd_ = 0;
      run_++;
      finish_mb();
    }
    return v;
  }
  int end(int v, bool certain) {
    if (certain) return 1;  // nothing follows in the picture
    tc_stage = STG_SKIP;
    H_skp_v D = {};
    skp_vars(D);
    return skp_bit(D, 1, ENC && v);
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
    D.nsk = (a && a->skip) + (b && b->skip) + (c && c->skip) + (d && d->skip);
    D.en = map_en();
    D.td = map_td();
    D.sm = sm_;
    D.tm = tm_;
    D.tb = tb_;
    D.dh = dh_;
    D.bd = bd_;
    D.qc = tc_clamp(qp_pred_ / 3, 0, 17);
    tcx X;
    H_mbt_make(D, X);
    f_mbt.select(X);
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
      int cand[4][2], nc = 0;
      const int nb[3] = {mb_near(-1, 0), mb_near(0, -1), mb_near(1, -1)};
      for (int j = 0; j < 3; j++)
        if (nb[j] >= 0 && info_[nb[j]].pic == pic_ && pxv_[nb[j] * 2] != INT16_MIN)
          cand[nc][0] = pxv_[nb[j] * 2], cand[nc][1] = pxv_[nb[j] * 2 + 1], nc++;
      cand[nc][0] = cand[nc][1] = 0, nc++;
      int miss;
      if (rec_.tm_search(-1, 0, 0, 0, 16, 16, p, false, cand, nc, &miss)) pxq_ = 1 + tc_clamp(tc_qlog(miss), 0, 11);
      pxv_[addr_ * 2] = (int16_t)tc_clamp(p[0], -32767, 32767);
      pxv_[addr_ * 2 + 1] = (int16_t)tc_clamp(p[1], -32767, 32767);
    }
    rec_.proxy_build(p);
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
      int pt = mi_->cls == CL_INXN ? -1 : rec_.pred_texture(0, (b8 & 1) * 8, (b8 >> 1) * 8, 8);
      D.pg = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog(pt), 0, 15);
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
    int strength;
    D.dm = rec_.dimd(addr_, x * 4, y * 4, k8 ? 8 : 4, strength);
    D.ds = strength ? 1 + tc_clamp(tc_qlog(strength) - 6, 0, 8) : 0;
    proxy_mv();
    int pm = k8 ? rec_.proxy_mode(x >> 1, y >> 1, 8, pmv_[0], pmv_[1]) : rec_.proxy_mode(x, y, 4, pmv_[0], pmv_[1]);
    D.pm = pm < 0 ? 9 : pm;
    // the remainder (rem_intra4x4_pred_mode) the proxy's and DIMD's modes
    // would have, 8 none or the most probable mode
    D.pq = pxq_;
    D.pr = D.pm < 9 && D.pm != mpm ? D.pm - (D.pm > mpm) : 8;
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
    D.pr = D.dr = 8;
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
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++)
        for (int k = 0; k < 2; k++) mi_->mv[list][yy * 4 + xx][k] = (int16_t)tc_clamp(p[k], -32768, 32767);
    for (int yy = y; yy < y + h; yy += 2)
      for (int xx = x; xx < x + w; xx += 2) mi_->rf[list][(yy >> 1) * 2 + (xx >> 1)] = (int8_t)ref;
  }
  // P_Skip (8.4.1.1)
  void mv_pskip() {
    parts_ = 0;
    MvN a = mv_nb(0, -1, 0, 0), b = mv_nb(0, 0, -1, 0);
    int p[2] = {0, 0};
    if (a.avail && b.avail && !(a.ref == 0 && !a.mv[0] && !a.mv[1]) && !(b.ref == 0 && !b.mv[0] && !b.mv[1]))
      mv_pred(0, 0, 0, 4, 4, 0, p);
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
    for (int l = 0; l < 2; l++) {
      MvN a = mv_nb(l, -1, 0, 0), b = mv_nb(l, 0, -1, 0), c = mv_nb(l, 4, -1, 0);
      if (!c.avail) c = mv_nb(l, -1, -1, 0);
      ref[l] = minpos(a.ref, minpos(b.ref, c.ref));
    }
    bool zero = ref[0] < 0 && ref[1] < 0;
    if (zero) ref[0] = ref[1] = 0;
    for (int l = 0; l < 2; l++)
      if (!zero && ref[l] >= 0) mv_pred(l, 0, 0, 4, 4, ref[l], mv[l]);
    bool lt = rec_.col_long_term();
    for (int b8 = 0; b8 < 4; b8++)
      if (mask >> b8 & 1)
        for (int k = 0; k < 4; k++) {
          int x = (b8 & 1) * 2 + (k & 1), y = (b8 >> 1) * 2 + (k >> 1), cr, cm[2];
          int64_t pid;
          // colZeroFlag: the colocated block is still
          bool still = !zero && !lt && rec_.colocated(addr_, y * 4 + x, cr, cm, pid) && cr == 0 && abs(cm[0]) <= 1 &&
                       abs(cm[1]) <= 1;
          for (int l = 0; l < 2; l++) {
            if (ref[l] < 0) continue;
            int z[2] = {0, 0};
            set_mv(l, x, y, 1, 1, ref[l], still && ref[l] == 0 ? z : mv[l]);
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
  int mvd(int list, int comp, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1), ref = sc_->cur_->ref[list][b8];
    if (!comp) {
      MvN nbs[3];
      mv_pred(list, x, y, w, h, ref, mvp_, nbs);
      for (int k = 0; k < 2; k++) {
        sp_[k] = -1;
        for (int j = 0; j < 3; j++)
          if (nbs[j].ref >= 0) sp_[k] = std::max(sp_[k], abs(nbs[j].mv[k] - mvp_[k]));
        pdv_[k] = 0;
      }
      pdn_ = !col_ || col_->rf[list][b8] < 0;
      if (!pdn_)
        for (int k = 0; k < 2; k++) pdv_[k] = col_->mv[list][y * 4 + x][k] - mvp_[k];
      tmv_[0] = mvp_[0];
      tmv_[1] = mvp_[1];
      // starting from mvp, the neighbours' vectors on the same reference,
      // col's, zero
      int cand[5][2], nc = 0;
      for (int j = 0; j < 3; j++)
        if (nbs[j].ref == ref) cand[nc][0] = nbs[j].mv[0], cand[nc][1] = nbs[j].mv[1], nc++;
      if (!pdn_) cand[nc][0] = col_->mv[list][y * 4 + x][0], cand[nc][1] = col_->mv[list][y * 4 + x][1], nc++;
      cand[nc][0] = cand[nc][1] = 0, nc++;
      tmn_ = !rec_.tm_search(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, false, cand, nc, &tmq_, kTmLambda);
    } else if (!tmn_) {
      // the horizontal component known
      tmv_[0] = mvp_[0] + mvx_;
      tmn_ = !rec_.tm_search(list, ref, x * 4, y * 4, w * 4, h * 4, tmv_, true, nullptr, 0, &tmq_, kTmLambda);
    }
    int dv = tmn_ ? 0 : tmv_[comp] - mvp_[comp];
    const MbInfo* a = info(sc_->nb4(x - 1, y, ia));
    const MbInfo* b = info(sc_->nb4(x, y - 1, ib));
    int va = a ? a->mvd[list][ia][comp] : 0, vb = b ? b->mvd[list][ib][comp] : 0;
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
    tcx X;
    H_mvs_make(D, X);
    u32 sc = (u32)(sgn3(-mvp_[comp]) * 81 + sgn3(dv) * 27 + sgn3(pdv_[comp]) * 9 + sgn3(va) * 3 + sgn3(vb));
    tc_stage = STG_MVD;
    tc_syms[STG_MVD]++;
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    int base = comp ? 47 : 40;
    pr[0] = cab1(base + sc_->mvd_inc(list, comp, x, y));
    pr[1] = cab1(base + 3);
    pr[2] = cab1(base + 4);
    f_mvs.prpa = pr;
    v = cm_int(f_mvs.code(X, ENC ? abs(v) : 0), 1 << 30) * (ENC && v < 0 ? -1 : 1);
    f_mvs.prpa = nullptr;
    if (v) {
      tc_stage = STG_MVSG;
      tc_syms[STG_MVSG]++;
      if (f_mvs.sign(sc, v < 0)) v = -abs(v);
    }
    if (!comp) {
      mvx_ = v;
    } else {
      int mv[2] = {mvp_[0] + mvx_, mvp_[1] + v};
      set_mv(list, x, y, w, h, ref, mv);
    }
    int16_t s = (int16_t)tc_clamp(v, -32767, 32767);
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) mi_->mvd[list][yy * 4 + xx][comp] = s;
    return v;
  }

  // --- mb_qp_delta: the qpd family. The QP of the macroblock follows.
  static int qp_class(int d) { return 4 + tc_clamp(d, -3, 3); }
  int qpd(int v) {
    prepare();
    tc_stage = STG_QPD;
    aq_xp_ = -1;
    const MbInfo* b = info(sc_->mb_b_);
    H_qpd_v D = {};
    D.lnz = last_qpd_ != 0;
    D.pc = popc4(mi_->cbp & 15);
    D.cc = mi_->cbp >> 4;
    D.cls = mi_->cls;
    D.dc = col_ ? qp_class(col_->qp - qp_off_ - qp_pred_) : 0;
    D.db = b ? qp_class(b->qp - qp_off_ - qp_pred_) : 0;
    D.lz = has_lz_ ? 3 + tc_clamp(tc_sq(lz_), -3, 3) : 3;
    D.st = st_;
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
      }
      if (ok || (x >= 0 && aq_fit_.predict(x, q16))) D.aq = 7 + tc_clamp((int)div_floor(q16 - 16 * qp_pred_ + 8, 16), -6, 6);
      if ((int)aq_res_[key_].size() == size_ && aq_res_[key_][addr_] != INT16_MIN)
        D.ac = 7 + tc_clamp((int)div_floor(aq_res_[key_][addr_] + 4, 8), -6, 6);
    }
    tcx X;
    H_qpd_make(D, X);
    int pr[TC_NODE];
    for (int k = 0; k < TC_NODE; k++) pr[k] = -1;
    pr[0] = cab1(60 + (sc_->last_qpd_ != 0));
    f_qpd.prpa = pr;
    v = cm_int(f_qpd.codes(X, ENC ? v : 0, 0), 1 << 20);
    f_qpd.prpa = nullptr;
    int qp = ((qp_pred_ + v + 52 + 2 * qp_off_) % (52 + qp_off_) + (52 + qp_off_)) % (52 + qp_off_) - qp_off_;
    qp_pred_ = qp;
    qp_src_ = addr_;
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
  static int coef_at(const MbInfo* m, const Geo& g, int i, int j) {
    if (g.kind == 0) return m->dc[g.plane][j];
    if (g.kind == 3) {
      int s = j & 3, x = (i & 3 & ~1) + (s & 1), y = ((i >> 2) & ~1) + (s >> 1);
      return m->coef[g.plane][y * 4 + x][j >> 2];
    }
    return m->coef[g.plane][i][j + g.off];
  }
  void store_coef(const Geo& g, const int* c, int n) {
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
    int pt = g.kind == 0 ? rec_.pred_texture(g.plane, 0, 0, g.plane ? 8 : 16)
             : g.kind == 2 ? rec_.pred_texture(g.plane, g.x * 4, g.y * 4, 4)
                           : rec_.pred_texture(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4);
    int pg = pt < 0 ? 0 : 1 + tc_clamp(tc_qlog(pt), 0, 15);
    // and how far it is from continuing its surroundings
    int ms = g.kind == 0 ? rec_.edge_miss(g.plane, 0, 0, g.plane ? 8 : 16)
             : rec_.edge_miss(g.plane, g.x * 4, g.y * 4, g.kind == 3 ? 8 : 4);
    int em = ms < 0 ? 0 : 1 + tc_clamp(tc_qlog(ms), 0, 15);
    // an intra block: what its levels would be were the source what the
    // reference shows (Recon::proxy_levels), by scan position
    int plv[64];
    bool pok = false;
    if (mi_->cls >= CL_INXN && mi_->cls != CL_PCM && sc_->chroma_ == 1) {
      proxy_mv();
      int qp = mi_->qp - qp_off_;
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
    int pnz = 0;
    if (pok)
      for (int j = 0; j < n; j++) pnz += est(j) != 0;
    // a still luma block of a P picture: the same block's coefficients in
    // the last picture of the kind (its reference), by scan position
    bool tok = false;
    if (col_ && (sc_->stype_ == kP || sc_->stype_ == kSP) && mi_->cls < CL_INXN && g.plane == 0 &&
        (g.kind == 1 || g.kind == 2 || g.kind == 3)) {
      int b8 = (g.y >> 1) * 2 + (g.x >> 1);
      const int16_t* v = mi_->mv[0][g.y * 4 + g.x];
      tok = mi_->rf[0][b8] == 0 && abs(v[0]) <= 1 && abs(v[1]) <= 1 && col_->cls < CL_INXN;
    }
    auto tcf = [&](int j) {
      int k = j + g.off;
      if (g.kind == 3) {
        int q = k & 3, x = g.x + (q & 1), y = g.y + (q >> 1);
        return (int)col_->coef[y * 4 + x][k >> 2];
      }
      return (int)col_->coef[g.y * 4 + g.x][k];
    };
    int tnz = 0;
    if (tok)
      for (int j = 0; j < n; j++) tnz += tcf(j) != 0;
    // the number of nonzero coefficients
    tc_stage = STG_CBF;
    H_cbf_v D = {};
    D.pz = pok ? 1 + std::min(pnz, 15) : 0;
    // how far to trust it: how far the macroblock's luma decoded so far is
    // from the proxy (13-24), else how well its vector matched (1-12)
    int pq = 0;
    if (pok) {
      int pa = rec_.proxy_acc();
      pq = pa >= 0 ? 13 + tc_clamp(tc_qlog(pa), 0, 11) : pxq_;
    }
    D.pq = pq;
    D.tn = tok ? 1 + std::min(tnz, 4) : 0;
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
    int total = cm_int(f_cbf.code(X, ENC ? count_nonzero(c, n) - coded : 0), n) + coded;
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
    // where they are: per position, whether the coefficient is nonzero,
    // knowing how many are still to come
    tc_stage = STG_SIG;
    bool fld = sc_->field_mb();
    bool sig[64];
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
        // the last two positions' significance (4 at the start)
        {
          // the significance of the last eight positions (and where the
          // block starts)
          int h = 1;
          for (int k = 1; k <= 8 && k <= i; k++) h = h * 2 + sig[i - k];
          S.hb = h - 1;
        }
        S.tz = tok ? 1 + std::min(abs(tcf(i)), 3) : 0;
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
    int g1 = 0, e1 = 0, lhist = 0;  // lhist: the last levels coded less 1 (up to 3), two bits each
    H_lvl_v L = {cat, 0, 0, nc, 0, 0, 0, q, tc_qlog(cnt), md, 0, pg, em, 0};
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
      L.pe = spok && k < 4 ? sp.estimate(k) : 0;
      L.pl = pok ? 1 + std::min(abs(q4(k)) / 2, 10) : 0;
      L.tz = tok ? 1 + std::min(abs(tcf(k)), 3) : 0;
      L.sa = na.m ? std::min(abs(coef_at(na.m, g, na.i, k)), 3) : 0;
      L.sb = nb.m ? std::min(abs(coef_at(nb.m, g, nb.i, k)), 3) : 0;
      tcx X;
      H_lvl_make(L, X);
      int a = cm_int(f_lvl.code(X, ENC ? abs(c[k]) - 1 : 0), 1 << 24);
      mag[k] = a + 1;
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
      G.ts = tok ? 1 + sgn3(tcf(k)) : 0;
      tcx X;
      H_sgn_make(G, X);
      f_sgn.select(X);
      int sg = f_sgn.bit(0, ENC && c[k] < 0);
      c[k] = sg ? -mag[k] : mag[k];
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

// The channel between the walk and the model. Coding, it takes each value
// from the log of the dry run and has the model code it; decoding, the
// model decodes it. Either way the value goes back to the walk.
template <int f_DEC>
struct ModelChan : Chan {
  static constexpr int ENC = f_DEC == 0;
  ModelChan(Model<f_DEC>& m, LogChan* log) : Chan(true), m_(m), log_(log) {}

  void hbegin(const char* name) override { m_.hbegin(name); }
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
