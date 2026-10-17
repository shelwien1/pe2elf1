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
constexpr int H_mvs_ND = TC_NODE, H_mvs_MND = TC_MNODE, H_mvs_NG = 27;
constexpr int H_qpd_ND = TC_NODE, H_qpd_MND = TC_MNODE, H_qpd_NG = 1;
constexpr int H_cbf_ND = 1, H_cbf_MND = 1, H_cbf_NG = 1;
constexpr int H_sig_ND = 2, H_sig_MND = 1, H_sig_NG = 1;
constexpr int H_lvl_ND = TC_NODE, H_lvl_MND = TC_MNODE, H_lvl_NG = 1;
constexpr int H_sgn_ND = 1, H_sgn_MND = 1, H_sgn_NG = 1;
constexpr int H_pcm_ND = TC_NODE, H_pcm_MND = TC_MNODE, H_pcm_NG = 3;

struct H_nal_v { int fld, ctx, p1, p2; };
struct H_hdr_v { int tag, fld, p1, p2, pos, p1r, sty; };
struct H_skp_v { int st, a3, b3, col, c9, cd, run, cnt, eq, lf, af; };
struct H_mbt_v { int st, ca, cb, cc, nsk; };
struct H_cbp_v { int inc, pc, cls, col, q6, st; };
struct H_ipm_v { int k8, ma, mb, mp, blk, pv, col; };
struct H_mvs_v { int kind, comp, lst, ma, mb, oth, ps, rf, pi, nr, col; };
struct H_qpd_v { int lnz, pc, cc, cls, dc, db, lz, st; };
struct H_cbf_v { int cat, na, nb, intra, q6, np, pos, crcb, col; };
struct H_sig_v { int cg, pos, nc, cnt, fld, q6; };
struct H_lvl_v { int cg, g1, e1, nc, pos, sa, sb, q6, tot; };
struct H_sgn_v { int cg, pos, sa, sb, scb, mag; };
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
             H_##F##_c_Volume, t_##F->H_##F##_D, H_##F##_d_Volume, t_##F->H_##F##_MT, H_##F##_t_Volume,             \
             t_##F->H_##F##_S, t_##F->H_##F##_SC, H_##F##_s_Volume, t_##F->H_##F##_F, t_##F->H_##F##_FC,            \
             H_##F##_f_Volume, t_##F->H_##F##_W, H_##F##_m_Volume, t_##F->H_##F##_G, H_##F##_NG, H_##F##_g_Volume,  \
             t_##F->H_##F##_WM, H_##F##_n_Volume, t_##F->H_##F##_WC, t_##F->H_##F##_WMC, H_##F##_rA, H_##F##_rB,    \
             H_##F##_rC, H_##F##_rD, H_##F##_rT, H_##F##_rG, H_##F##_mwA, H_##F##_mwB, H_##F##_mwC, H_##F##_mwD,    \
             H_##F##_mwT, H_##F##_mwG, H_##F##_rS1, H_##F##_rS2, H_##F##_lr, H_##F##_mb, H_##F##_lrm, H_##F##_mbm,  \
             H_##F##_bw, H_##F##_qs, H_##F##_qf, H_##F##_ND, H_##F##_MND, H_##F##_hA, H_##F##_hB, H_##F##_hD,       \
             H_##F##_hS, H_##F##_hF, H_##F##_hM, H_##F##_hT, H_##F##_hN)

// where the bits go (-v)
enum { STG_NAL, STG_HDR, STG_RAW, STG_SKIP, STG_MBT, STG_CBP, STG_IPM, STG_MVS, STG_QPD, STG_CBF, STG_SIG, STG_LVL,
       STG_SGN, STG_PCM, STG_N };
static const char* const kStage[STG_N] = {"NAL units", "headers", "bytes", "skip, end", "mb_type", "cbp, 8x8",
                                          "intra modes", "motion", "qp delta", "block flags", "significance",
                                          "levels", "signs", "pcm"};

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
  int8_t ref[2][4];         // ref_idx of the 8x8 blocks
  int16_t mvd[2][16][2];    // mvd of the 4x4 blocks
  int8_t coef[3][16][16];   // coefficients of the 4x4 blocks (Y, Cb, Cr) by scan position, clipped
  int8_t dc[3][16];         // Intra16x16 DC and chroma DC coefficients
};

// What it keeps of the last macroblock at an address in a picture of the
// same kind (slice type, reference or not, frame or field).
struct MbCol {
  uint8_t valid, skip, cls, qp, cbp, t8, cpred, field;
  uint8_t modes[16];
  int8_t ref[2][4];
  uint8_t mvq[2][16][2];  // tc_qlog |mvd|
  uint16_t nzm[3];        // 4x4 blocks with coefficients
  uint8_t dcm;            // DC blocks with coefficients
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
  bool has_lz_ = false, qpd_coded_ = false;
  int run_ = 0, cnt_ = 0, prev_cnt_ = 0;  // skipped before, macroblocks in the slice so far, in the last one
  int slice_cnt_[30] = {};
  bool mb_open_ = false;
  int addr_ = 0;
  MbInfo* mi_ = nullptr;
  const MbCol* col_ = nullptr;
  int sub_[4] = {};
  int prev_mode_ = 9, mvx_ = 0;

  int idx(const MbState* n) const { return (int)(n - mbs_); }
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
    run_ = cnt_ = 0;
    prev_cnt_ = slice_cnt_[key_];
    mb_open_ = false;
  }
  void slice_end() { slice_cnt_[key_] = cnt_; }

  void begin_mb() {
    addr_ = sc_->cur_addr_;
    mi_ = &info_[addr_];
    memset(mi_, 0, sizeof *mi_);
    mi_->pic = pic_;
    mi_->qp = (uint8_t)(qp_pred_ + qp_off_);
    memset(mi_->modes, 2, sizeof mi_->modes);
    col_ = hcur_[addr_].valid ? &hcur_[addr_] : nullptr;
    qpd_coded_ = false;
    prev_mode_ = 9;
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
    for (int l = 0; l < 2; l++)
      for (int b = 0; b < 16; b++)
        for (int k = 0; k < 2; k++) c.mvq[l][b][k] = (uint8_t)tc_qlog(mi_->mvd[l][b][k]);
    for (int p = 0; p < 3; p++) {
      c.nzm[p] = 0;
      for (int b = 0; b < 16; b++) c.nzm[p] |= (uint16_t)((s.nz[p][b] != 0) << b);
    }
    c.dcm = (uint8_t)(s.cbf_dc[0] | s.cbf_dc[1] << 1 | s.cbf_dc[2] << 2);
    mb_open_ = false;
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
  }
  int skp_bit(const H_skp_v& D, int node, int v) {
    tcx X;
    H_skp_make(D, X);
    f_skp.select(X);
    tc_syms[STG_SKIP]++;
    return f_skp.bit(node, v);
  }
  int skip(int v) {
    begin_mb();
    tc_stage = STG_SKIP;
    H_skp_v D;
    skp_vars(D);
    v = skp_bit(D, 0, ENC && v);
    if (v) {
      mi_->skip = 1;
      mi_->cls = CL_SKIP;
      last_qpd_ = 0;
      run_++;
      finish_mb();
    }
    return v;
  }
  int end(int v, bool certain) {
    if (certain) return 1;  // nothing follows in the picture
    tc_stage = STG_SKIP;
    H_skp_v D;
    skp_vars(D);
    return skp_bit(D, 1, ENC && v);
  }
  int field(int v) {
    tc_stage = STG_SKIP;
    H_skp_v D;
    skp_vars(D);
    D.col = col_ ? 1 + col_->field : 0;
    return skp_bit(D, 2, ENC && v);
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
  int mbt_bit(int node, int v) { return f_mbt.bit(node, v); }
  // an intra mb_type in I slice numbering
  int mbt_intra(int it) {
    if (mbt_bit(21, ENC && it == 0)) return 0;
    if (mbt_bit(22, ENC && it == 25)) return 25;
    int x = ENC ? it - 1 : 0;
    int luma = mbt_bit(23, ENC && x / 12);
    int c = x / 4 % 3;
    int chroma = mbt_bit(24, ENC && c != 0);
    if (chroma) chroma += mbt_bit(25, ENC && c == 2);
    int p1 = mbt_bit(26, ENC && (x & 2));
    int p0 = mbt_bit(27 + p1, ENC && (x & 1));
    mi_->i16m = (uint8_t)(p1 * 2 + p0);
    return 1 + p1 * 2 + p0 + 4 * chroma + 12 * luma;
  }
  int mb_type(int t) {
    tc_stage = STG_MBT;
    const MbInfo* c = near_info(1, -1);
    const MbInfo* d = near_info(-1, -1);
    const MbState* a = sc_->mb_a_;
    const MbState* b = sc_->mb_b_;
    H_mbt_v D;
    D.st = st_;
    D.ca = cls_of(a);
    D.cb = cls_of(b);
    D.cc = col_ ? col_->cls : (int)CL_NONE;
    D.nsk = (a && a->skip) + (b && b->skip) + (c && c->skip) + (d && d->skip);
    tcx X;
    H_mbt_make(D, X);
    f_mbt.select(X);
    int stype = sc_->stype_;
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
        } else if (mbt_bit(6, ENC && t == 0)) {
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
        if (mbt_bit(1, ENC && t >= 5))
          t = 5 + mbt_intra(t - 5);
        else if (mbt_bit(2, ENC && t == 0))
          t = 0;
        else if (mbt_bit(3, ENC && t >= 3))
          t = mbt_bit(5, ENC && t == 4) ? 4 : 3;
        else
          t = mbt_bit(4, ENC && t == 2) ? 2 : 1;
    }
    mi_->cls = (uint8_t)class_of(stype, t);
    tc_syms[STG_MBT]++;
    return t;
  }

  // --- coded_block_pattern and transform_size_8x8_flag: the cbp family
  int cbp_bit(H_cbp_v& D, int node, int v) {
    tcx X;
    H_cbp_make(D, X);
    f_cbp.select(X);
    return f_cbp.bit(node, v);
  }
  int cbp(int v) {
    tc_stage = STG_CBP;
    H_cbp_v D;
    D.cls = mi_->cls;
    D.q6 = q6();
    D.st = st_;
    int luma = 0;
    for (int b8 = 0; b8 < 4; b8++) {
      D.inc = sc_->cbp_luma_inc(b8, luma);
      D.pc = popc4(luma);
      D.col = col_ ? 1 + (col_->cbp >> b8 & 1) : 0;
      luma |= cbp_bit(D, b8, ENC && (v >> b8 & 1)) << b8;
    }
    int c = 0;
    if (sc_->chroma_ == 1 || sc_->chroma_ == 2) {
      D.inc = sc_->cbp_chroma_inc(0);
      D.pc = popc4(luma);
      D.col = col_ ? 1 + (col_->cbp >> 4 != 0) : 0;
      if (cbp_bit(D, 4, ENC && (v >> 4) != 0)) {
        D.inc = sc_->cbp_chroma_inc(1);
        D.col = col_ ? 1 + (col_->cbp >> 4 == 2) : 0;
        c = 1 + cbp_bit(D, 5, ENC && (v >> 4) == 2);
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
    H_cbp_v D;
    D.inc = (a && a->t8x8) + (b && b->t8x8);
    D.pc = popc4(mi_->cbp & 15);
    D.cls = mi_->cls;
    D.col = col_ ? 1 + col_->t8 : 0;
    D.q6 = q6();
    D.st = st_;
    tc_syms[STG_CBP]++;
    v = cbp_bit(D, 6, ENC && v);
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
    H_ipm_v D;
    D.k8 = k8;
    D.ma = ma;
    D.mb = mb;
    D.mp = mpm;
    D.blk = blk;
    D.pv = prev_mode_;
    D.col = col_ && col_->cls == CL_INXN ? col_->modes[y * 4 + x] : 9;
    tcx X;
    H_ipm_make(D, X);
    f_ipm.select(X);
    int r = -1;
    if (!f_ipm.bit(0, ENC && v < 0)) {
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
    H_ipm_v D;
    D.k8 = 2;
    D.ma = nb_cpred(sc_->mb_a_);
    D.mb = nb_cpred(sc_->mb_b_);
    D.mp = luma_dir();
    D.blk = 0;
    D.pv = 9;
    D.col = col_ && col_->cls >= CL_INXN && col_->cls != CL_PCM ? col_->cpred : 9;
    tcx X;
    H_ipm_make(D, X);
    f_ipm.select(X);
    int r = 0;
    if (f_ipm.bit(16, ENC && v > 0)) r = f_ipm.bit(17, ENC && v > 1) ? 2 + f_ipm.bit(18, ENC && v > 2) : 1;
    mi_->cpred = (uint8_t)r;
    tc_syms[STG_IPM]++;
    return r;
  }

  // --- sub_mb_type, ref_idx and mvd: the mvs family
  int sub(int i, int v) {
    tc_stage = STG_MVS;
    bool b = sc_->stype_ == kB;
    H_mvs_v D = {b, 0, 0, i ? sub_[i - 1] + 1 : 0, 0, 0, 0, 0, i, 0, 0};
    tcx X;
    H_mvs_make(D, X);
    f_mvs.select(X);
    int node = 1;
    for (int k = b ? 3 : 1; k >= 0; k--) node = node * 2 + f_mvs.bit(node, ENC && (v >> k & 1));
    int r = node - (b ? 16 : 4);
    sub_[i] = r;
    tc_syms[STG_MVS]++;
    return r;
  }
  static int part_size(int w, int h) { return w * h == 16 ? 0 : w * h == 8 ? 1 : w * h == 4 ? 2 : 3; }
  int ref(int list, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1);
    const MbInfo* a = info(sc_->nb4(x - 1, y, ia));
    const MbInfo* b = info(sc_->nb4(x, y - 1, ib));
    H_mvs_v D;
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
    D.col = col_ && col_->cls < CL_INXN ? 1 + col_->ref[list][b8] : 0;
    tcx X;
    H_mvs_make(D, X);
    v = cm_int(f_mvs.code(X, ENC ? v : 0), 64);
    for (int yy = y; yy < y + h; yy += 2)
      for (int xx = x; xx < x + w; xx += 2) mi_->ref[list][(yy >> 1) * 2 + (xx >> 1)] = (int8_t)v;
    tc_syms[STG_MVS]++;
    return v;
  }
  int mvd(int list, int comp, int x, int y, int w, int h, int v) {
    tc_stage = STG_MVS;
    int ia, ib, b8 = (y >> 1) * 2 + (x >> 1);
    const MbInfo* a = info(sc_->nb4(x - 1, y, ia));
    const MbInfo* b = info(sc_->nb4(x, y - 1, ib));
    int va = a ? a->mvd[list][ia][comp] : 0, vb = b ? b->mvd[list][ib][comp] : 0;
    H_mvs_v D;
    D.kind = 3;
    D.comp = comp;
    D.lst = list;
    D.ma = tc_qlog(va);
    D.mb = tc_qlog(vb);
    D.oth = comp ? tc_qlog(mvx_) : 0;
    D.ps = part_size(w, h);
    D.rf = sc_->cur_->ref[list][b8];
    D.pi = b8;
    D.nr = sc_->num_ref_[list] - 1;
    D.col = col_ ? col_->mvq[list][y * 4 + x][comp] : 0;
    tcx X;
    H_mvs_make(D, X);
    u32 sc = (u32)(sgn3(va) * 3 + sgn3(vb) + (comp ? 9 * sgn3(mvx_) : 0));
    v = cm_int(f_mvs.codes(X, ENC ? v : 0, sc), 1 << 30);
    if (!comp) mvx_ = v;
    int16_t s = (int16_t)tc_clamp(v, -32767, 32767);
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) mi_->mvd[list][yy * 4 + xx][comp] = s;
    tc_syms[STG_MVS]++;
    return v;
  }

  // --- mb_qp_delta: the qpd family. The QP of the macroblock follows.
  static int qp_class(int d) { return 4 + tc_clamp(d, -3, 3); }
  int qpd(int v) {
    tc_stage = STG_QPD;
    const MbInfo* b = info(sc_->mb_b_);
    H_qpd_v D;
    D.lnz = last_qpd_ != 0;
    D.pc = popc4(mi_->cbp & 15);
    D.cc = mi_->cbp >> 4;
    D.cls = mi_->cls;
    D.dc = col_ ? qp_class(col_->qp - qp_off_ - qp_pred_) : 0;
    D.db = b ? qp_class(b->qp - qp_off_ - qp_pred_) : 0;
    D.lz = has_lz_ ? 3 + tc_clamp(tc_sq(lz_), -3, 3) : 3;
    D.st = st_;
    tcx X;
    H_qpd_make(D, X);
    v = cm_int(f_qpd.codes(X, ENC ? v : 0, 0), 1 << 20);
    int qp = ((qp_pred_ + v + 52 + 2 * qp_off_) % (52 + qp_off_) + (52 + qp_off_)) % (52 + qp_off_) - qp_off_;
    qp_pred_ = qp;
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
  // nonzero coefficients of a neighbouring block, 5 for none
  int nb_count(const Nb& n, const Geo& g) const {
    if (!n.s) return 5;
    if (g.kind == 0) return n.s->cbf_dc[g.plane];
    return std::min<int>(n.s->nz[g.plane][n.i], 4);
  }
  int block_nc(const Geo& g) const {
    if (g.kind == 0) return g.plane && sc_->chroma_ != 3 ? 0 : sc_->nc_luma(g.plane, 0, 0);
    if (g.kind == 2) return sc_->nc_chroma(g.plane - 1, g.x, g.y);
    return sc_->nc_luma(g.plane, g.x, g.y);
  }
  void block(int cat, int idx, int* c, int n, bool coded) {
    Geo g = geo(cat, idx);
    const MbState& cur = *sc_->cur_;
    Nb na = nb_block(g, 0), nb = nb_block(g, 1);
    int q = q6();
    if (!coded) {
      tc_stage = STG_CBF;
      H_cbf_v D;
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
      if (col_) D.col = g.kind == 0 ? col_->dcm >> g.plane & 1 : col_->nzm[g.plane] >> (g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x) & 1;
      tcx X;
      H_cbf_make(D, X);
      f_cbf.select(X);
      tc_syms[STG_CBF]++;
      if (!f_cbf.bit(0, ENC && count_nonzero(c, n) != 0)) {
        for (int i = 0; i < n; i++) c[i] = 0;
        store_coef(g, c, n);
        return;
      }
    }
    // the significance map, CABAC's way: per position whether the
    // coefficient is nonzero, and then whether it is the last one
    tc_stage = STG_SIG;
    int nc = nc_class(block_nc(g));
    bool fld = sc_->field_mb();
    int last = n - 1;
    if (ENC)
      while (last > 0 && !c[last]) last--;
    bool sig[64];
    int cnt = 0, i;
    H_sig_v S = {cat, 0, nc, 0, fld, q};
    for (i = 0; i < n - 1; i++) {
      S.pos = i;
      S.cnt = cnt;
      tcx X;
      H_sig_make(S, X);
      f_sig.select(X);
      sig[i] = f_sig.bit(0, ENC && c[i] != 0);
      tc_syms[STG_SIG]++;
      if (sig[i]) {
        cnt++;
        if (f_sig.bit(1, ENC && i == last)) break;
      }
    }
    if (i == n - 1) {
      sig[i] = true;
      cnt++;
    }
    last = i;
    // the magnitudes, from the last coefficient back
    tc_stage = STG_LVL;
    int mag[64];
    int g1 = 0, e1 = 0;
    H_lvl_v L = {cat, 0, 0, nc, 0, 0, 0, q, tc_qlog(cnt)};
    for (int k = last; k >= 0; k--) {
      if (!sig[k]) continue;
      L.g1 = g1;
      L.e1 = e1;
      L.pos = tc_qlog(k);
      L.sa = na.m ? std::min(abs(coef_at(na.m, g, na.i, k)), 3) : 0;
      L.sb = nb.m ? std::min(abs(coef_at(nb.m, g, nb.i, k)), 3) : 0;
      tcx X;
      H_lvl_make(L, X);
      int a = cm_int(f_lvl.code(X, ENC ? abs(c[k]) - 1 : 0), 1 << 24);
      mag[k] = a + 1;
      if (a)
        g1++;
      else
        e1++;
      tc_syms[STG_LVL]++;
    }
    // the signs
    tc_stage = STG_SGN;
    H_sgn_v G = {cat, 0, 0, 0, 3, 0};
    bool cr = g.plane == 2;
    for (int k = last; k >= 0; k--) {
      if (!sig[k]) {
        if (!ENC) c[k] = 0;
        continue;
      }
      G.pos = k == 0 ? 0 : k < 3 ? 1 : k < 6 ? 2 : 3;
      G.sa = na.m ? sgn3(coef_at(na.m, g, na.i, k)) : 3;
      G.sb = nb.m ? sgn3(coef_at(nb.m, g, nb.i, k)) : 3;
      if (cr) {
        Geo gb = g;
        gb.plane = 1;
        G.scb = sgn3(coef_at(mi_, gb, g.kind == 2 ? g.y * 2 + g.x : g.y * 4 + g.x, k));
      }
      G.mag = std::min(mag[k], 3) - 1;
      tcx X;
      H_sgn_make(G, X);
      f_sgn.select(X);
      int s = f_sgn.bit(0, ENC && c[k] < 0);
      c[k] = s ? -mag[k] : mag[k];
      tc_syms[STG_SGN]++;
    }
    for (int k = last + 1; k < n; k++) c[k] = 0;
    store_coef(g, c, n);
  }

  // --- I_PCM: the alignment bits, and the samples as differences to the
  // one to the left (above, in the first column): the pcm family
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
          H_pcm_v D = {p, depth - 8, sq2(d1), sq2(d2)};
          tcx X;
          H_pcm_make(D, X);
          i64 d = f_pcm.codes(X, ENC ? s[i] - pred : 0, (u32)p);
          s[i] = pred + cm_int(d, 1 << 16);
          d2 = d1;
          d1 = d;
          tc_syms[STG_PCM]++;
        }
    }
    if (i != n) cm_fail("internal: I_PCM sample count");
    mi_->cbp = 0x2f;
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
