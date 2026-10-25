// The model of the compressor: the IDX families of decisions and values
// (famvars.inc), what each one is fed from, and the channel through which
// the syntax walk hands the model the values of a unit (coding) or takes
// them from it (decoding).
//
// Both directions run the walk that writes the bitstream (W), so the model
// sees the same state at every value: the walk's own (where it is, the
// channel's bands, codebooks, scalefactors and spectral values so far,
// syntax.inc's Walk::at and cd) and the model's (what it keeps of each
// channel's last frame, of each SBR channel's last envelopes, the last
// value of each field). Coding, the values come from the log of the
// compressor's parse; decoding, from the range decoder. The walk's state
// that the model reads is what the walk has set in the unit before the
// read: a unit's walk starts from the same state both ways (Walk::begin).
//
// The model is a template on the level (LV, lv<L>.inc): its families'
// classes.

namespace aacpack {

// A family's tables, mapped: zero is every table's starting state, and a
// page costs memory only once the coder reaches it.
template <class T>
static T* tc_table() {
  return (T*)tc_map(sizeof(T));
}

// --- the levels of this build (levels.inc)
#include "levels.inc"

// where the bits go (-v)
enum { STG_UNIT, STG_RAW, STG_HDR, STG_SEC, STG_SF, STG_SPEC, STG_ESC, STG_SGN, STG_SBR, STG_PS, STG_BITS, STG_N };
static const char* const kStage[STG_N] = {"units",   "bytes",   "fields", "sections", "scalefactors", "spectra",
                                          "escapes", "signs",   "SBR",    "PS",       "payloads"};

// the fields of the unt family: the stream's own
enum { UF_KIND, UF_RAWN, UF_ASCN, UF_SBRN, UF_SBRI, UF_CRC, UF_N };

// a signed value as a class: 0 zero, then -1, +1, -2, +2, -3..., by tc_qlog
static INLINE int sq2(i64 d) { return d == 0 ? 0 : d > 0 ? 2 * tc_qlog(d) : 2 * tc_qlog(d) - 1; }
static INLINE int sgn3(int v) { return v > 0 ? 1 : v < 0 ? 2 : 0; }
static INLINE int iabs(int v) { return v < 0 ? -v : v; }

// 2^(3 x / 16) in Q12 for x in -60..60: how a quantized magnitude scales
// with a scalefactor x smaller (the quantizer's step goes as 2^(x / 4), a
// magnitude as the step to the -3/4)
struct ScaleTab {
  int v[121];
  void Init() {
    for (int x = -60; x <= 60; x++) v[x + 60] = (int)lrint(4096.0 * pow(2.0, 3.0 * x / 16.0));
  }
};
static ScaleTab scale_make() {
  ScaleTab t;
  t.Init();
  return t;
}
static const ScaleTab kScale = scale_make();

// |q|^(4/3) in Q4 for |q| < 2048, by integer cube roots (the same everywhere,
// as the model must be); and 2^(x / 4) for x mod 4, in Q16
struct Pow43Tab {
  uint32_t v[2048];
  void Init() {
    for (u64 q = 0; q < 2048; q++) {
      const u64 x = q * q * q * q * 4096;  // (q^(4/3) * 16 = cbrt(q^4 * 16^3))
      u64 lo = 0, hi = (u64)1 << 21;
      while (lo < hi) {  // the largest r with r^3 <= x
        u64 mid = (lo + hi + 1) / 2;
        if (mid * mid * mid <= x) lo = mid; else hi = mid - 1;
      }
      v[q] = (uint32_t)(x - lo * lo * lo > (lo + 1) * (lo + 1) * (lo + 1) - x ? lo + 1 : lo);
    }
  }
};
static Pow43Tab pow43_make() {
  Pow43Tab t;
  t.Init();
  return t;
}
static const Pow43Tab kPow43 = pow43_make();
static const int kFrac4[4] = {65536, 77936, 92682, 110218};
// a quantized value q as the decoder has it, sign(q) |q|^(4/3) 2^(d / 4),
// in Q20 (d: its band's scalefactor less the one it is compared with, -60..60)
static INLINE i64 lin_q(int q, int d) {
  const int a = q < 0 ? (q < -2047 ? 2047 : -q) : (q > 2047 ? 2047 : q);
  i64 v = (i64)kPow43.v[a] * kFrac4[d & 3];
  const int sh = d >> 2;
  v = sh >= 0 ? v << sh : v >> -sh;
  return q < 0 ? -v : v;
}
// and back: the magnitude whose |q|^(4/3) is nearest |v|
static INLINE int q_of_lin(i64 v) {
  const i64 t = (v < 0 ? -v : v) >> 16;
  int lo = 0, hi = 2047;
  while (lo < hi) {
    int mid = (lo + hi) / 2;
    if ((i64)kPow43.v[mid] < t) lo = mid + 1; else hi = mid;
  }
  if (lo > 0 && t - (i64)kPow43.v[lo - 1] < (i64)kPow43.v[lo] - t) lo--;
  return lo;
}

// Bytes nothing else models (h264p's RawModel, without its dictionary): an
// order 0 and an order 1 counter and a match model mixed by a weight set
// the bits of the byte so far select: the match finds the last three bytes
// in the last kRing bytes coded. (Its counters and weights zero as it
// starts: the decompressor's must match.) A chunk of kPlainMin bytes or
// more may be coded as it is: plain().
const size_t kPlainMin = (size_t)1 << 16;
template <int f_DEC>
struct RawModel {
  static constexpr int ENC = f_DEC == 0;
  void Init() {
    memset(o0_, 0, sizeof o0_);
    memset(o1_, 0, sizeof o1_);
    memset(w_, 0, sizeof w_);
    memset(wc_, 0, sizeof wc_);
    prev_ = 0;
    ring_ = nullptr;
    hash_.Init();
    pos_ = ptr_ = 0;
    len_ = 0;
    memset(mc_, 0, sizeof mc_);
    memset(&plain_c_, 0, sizeof plain_c_);
    seen_.Init();
  }
  void Quit() {
    mem_free(ring_);
    ring_ = nullptr;
    hash_.Quit();
    seen_.Quit();
  }
  void start() {
    mx_.init(w_, wc_);
    pos_ = 0;
    if (!ring_) ring_ = (u8*)mem_alloc(kRing);  // (its pages taken as they are written)
    hash_.fill((size_t)1 << 16, -1);
    len_ = 0;
  }

  cm_cnt o0_[256];
  cm_cnt o1_[256][256];
  i32 w_[256 * 4];
  u8 wc_[256];
  cm_mix<4> mx_;
  u32 prev_;
  // the bytes coded: the one at place a at ring_[a % kRing], while a is
  // one of the last kRing places
  static constexpr uint64_t kRing = (uint64_t)1 << 24;
  u8* ring_;
  Buf<int64_t> hash_;    // by the hash of three bytes, the place after them (-1 none)
  uint64_t pos_, ptr_;   // the next place; where the match predicts from
  int len_;              // the match's length (0 none), up to 15
  u8 at(uint64_t a) const { return ring_[a & (kRing - 1)]; }
  cm_cnt mc_[16];        // whether the match's bit is right, by its length
  cm_cnt plain_c_;       // whether a chunk's bytes are coded as they are (plain())
  Buf<u32> seen_;        // compressing: a chunk's places by the hash of three bytes (random())
  static u32 key(const u8* p) {
    return ((u32)p[0] * 0x9E3779B1u ^ (u32)p[1] * 0x85EBCA77u ^ (u32)p[2] * 0xC2B2AE3Du) >> 16;
  }
  // a byte coded (its value b coding; decoding, returned), by the coder rc
  u32 code(tc_rc<f_DEC>* rc, u32 b) {
    cm_cnt* row = o1_[prev_];
    u32 node = 1;
    int pbyte = len_ && ptr_ < pos_ && pos_ - ptr_ <= kRing ? at(ptr_) : -1, ml = len_;
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
    // the match goes on or ends; then the byte joins what it searches
    if (pbyte >= 0 && (u32)pbyte == (node & 255))
      len_ += len_ < 15, ptr_++;
    else
      len_ = 0;
    ring_[pos_++ & (kRing - 1)] = (u8)(node & 255);
    if (pos_ >= 3) {
      const u8 last[3] = {at(pos_ - 3), at(pos_ - 2), at(pos_ - 1)};
      u32 k = key(last);
      if (!len_ && hash_[k] >= 0) ptr_ = (uint64_t)hash_[k], len_ = 1;
      hash_[k] = (int64_t)pos_;
    }
    rc->tc_syms[rc->tc_stage]++;
    return node & 255;
  }

  // A chunk of bytes of kPlainMin or more is coded as it is, a byte a
  // uniform step of the coder (plain_byte()), or by code() above, as one
  // decision of its own says (plain(), its value v coding; decoding,
  // returned). Compressing, random() decides: bytes the model could not
  // code in fewer than 8 bits each. As they are, they still go where the
  // match searches.
  int plain(tc_rc<f_DEC>* rc, int v) {
    int b = tc_bit(rc, plain_c_.P(), v);
    plain_c_.upd(b, 30, 0);
    return b;
  }
  u32 plain_byte(tc_rc<f_DEC>* rc, u32 b) {
    if (ENC && rc->tc_verbose) rc->tc_bits[rc->tc_stage] += 8;
    b = rc->rc_Byte(b);
    prev_ = b;
    len_ = 0;
    ring_[pos_++ & (kRing - 1)] = (u8)b;
    if (pos_ >= 3) {
      const u8 last[3] = {at(pos_ - 3), at(pos_ - 2), at(pos_ - 1)};
      hash_[key(last)] = (int64_t)pos_;
    }
    rc->tc_syms[rc->tc_stage]++;
    return b;
  }
  // whether the n bytes of p (kPlainMin or more) are better coded as they
  // are (h264p's: the entropy of order 0, of the differences, in windows
  // and of order 1 near 8 bits, and next to nothing the match would find)
  bool random(const u8* p, size_t n) {
    u32 h0[256] = {}, hd[256] = {}, rows[256] = {};
    for (size_t i = 0; i < n; i++) h0[p[i]]++, hd[(u8)(p[i] - (i ? p[i - 1] : 0))]++;
    auto bits = [](const u32* h, int m, double t) {  // -sum h log2(h/t) over m counts
      double s = 0;
      for (int k = 0; k < m; k++)
        if (h[k]) s -= h[k] * log2(h[k] / t);
      return s;
    };
    const double dn = (double)n, f0 = 7.985 - 184.0 / dn, f1 = 7.985 - 1.3 * 47100.0 / dn;
    if (bits(h0, 256, dn) / dn < f0 || bits(hd, 256, dn) / dn < f0) return false;
    const size_t kWin = 4096, nw = n / kWin;
    double sw = 0;
    for (size_t w = 0; w < nw; w++) {
      u32 hw[256] = {};
      for (size_t i = w * kWin; i < (w + 1) * kWin; i++) hw[p[i]]++;
      sw += bits(hw, 256, (double)kWin);
    }
    if (sw / (double)(nw * kWin) < 7.985 - (8 - 7.954)) return false;
    seen_.fill((size_t)1 << 16, 0);
    for (size_t i = 1; i < n; i++) seen_[(u32)p[i - 1] << 8 | p[i]]++, rows[p[i - 1]]++;
    if ((bits(seen_.data(), 1 << 16, dn - 1) - bits(rows, 256, dn - 1)) / (dn - 1) < f1) return false;
    seen_.fill((size_t)1 << 16, ~0u);
    auto byte_at = [&](uint64_t a) -> int { return a >= pos_ ? p[a - pos_] : at(a); };
    uint64_t ptr = ptr_, pos = pos_;
    int len = len_;
    size_t hits = 0;
    for (size_t j = 0; j < n; j++) {
      int pb = len && ptr < pos && pos - ptr <= kRing ? byte_at(ptr) : -1;
      if (pb == p[j])
        hits++, len += len < 15, ptr++;
      else
        len = 0;
      pos++;
      if (pos >= 3) {
        const u8 last[3] = {(u8)byte_at(pos - 3), (u8)byte_at(pos - 2), (u8)byte_at(pos - 1)};
        u32 k = key(last);
        int64_t h = seen_[k] != ~0u ? (int64_t)(pos_ + seen_[k]) : hash_[k];
        if (!len && h >= 0) ptr = (uint64_t)h, len = 1;
        seen_[k] = (u32)(pos - pos_);
      }
    }
    return hits * 512 <= n * 3;
  }
};

// What the model keeps of a channel's last frame (by its slot: the channel
// element's type, tag and channel), for the contexts of the next
struct ChanHist {
  int valid;  // a frame of the channel was walked
  int ws, max_sfb, num_groups, num_windows, num_swb;
  int group_len[8];
  int gg;
  int last_esc;
  uint8_t bt[8 * 64];   // the codebook of each band
  int16_t sf[8 * 64];   // and its scalefactor (Walk's ChanData::sf)
  int8_t sfd[8 * 64];   // its scalefactor's delta as coded (0 none)
  uint8_t secb[8 * 64]; // a section starts at the band
  uint8_t msb[64];      // long windows, of a CPE: the band was M/S
  int coef[1024];       // the spectral values
  // long windows: each place's magnitude, an average over the frames that
  // fades by 3/4 a frame (in 1/16; mp3c's av_MDCT), of av_n frames of av_swb
  // bands, as the band's scalefactor avsf quantizes it (INT16_MIN: not coded)
  uint16_t av[1024];
  int16_t avsf[64];
  int av_n, av_swb;
  // and short windows' the same, over the windows (fading by 7/8 a window)
  uint16_t av8[128];
  int16_t av8sf[16];
  int av8_n, av8_swb;
};
// and what it keeps of a channel as it is coded
struct CurChan {
  int slot;
  int off[3];      // the scalefactors' offsets as the walk has them: of the spectrum, intensity, noise
  int d1[3], d2[3];  // the last two deltas of each kind
  int pc;          // the codebook of the last section of the group (16 none)
  int last_esc;
  int bkey, bnz, bsum;  // the band (window * 64 + band) the counts are of: its nonzero values and their sum so far
  int8_t sfd[8 * 64];
  uint8_t secb[8 * 64];
};
static const int kSlots = 4 * 16 * 2;  // (channel element type, tag, channel)

// What it keeps of an SBR channel's last envelopes and noise floors, and
// a PS element's last parameters (by the SBR element's slot and channel)
struct SbrHist {
  int8_t last[8][64];   // the last value at each band: envelope, noise, of a balance channel; iid, icc, ipd, opd
  uint8_t have[8];      // a value of the kind was coded
  int16_t lv[2][64];    // the levels of the last envelope (envelope, noise), as the deltas leave them
  uint8_t lvn[2];       // its bands (0 none)
};

template <int f_DEC, class LV>
struct Model : tc_rc<f_DEC> {
  static constexpr int ENC = f_DEC == 0;
  using tc_rc<f_DEC>::tc_stage;
  using tc_rc<f_DEC>::tc_syms;
  using tc_rc<f_DEC>::tc_bits;
  using tc_rc<f_DEC>::tc_verbose;

  // The model of a run: Init() as its coroutine is made (on pages of its
  // own, zeroed: what Init() does not set is 0), start() as the run
  // starts (its tables mapped), and Quit() as it ends, or wherever the
  // coroutine was left (the tables unmapped, the memory freed)
  void Init() {
    tc_rc<f_DEC>::Init();
#define H_INIT(F) f_##F.Init(), t_##F = nullptr;
    AAC_FAMS(H_INIT)
#undef H_INIT
    memset(last_, 0, sizeof last_);
    raw_.Init();
    hl_.Init();
    lasttag_ = -1, runpos_ = 0;
    lastslot_ = lastfi_ = -1, lastv_ = 0;
    res_.n = res_.bits = res_.first = 0, res_.last = -1, res_.unit = 0;
    hist_.Init();
    sbrh_.Init();
    memset((void*)cc_, 0, sizeof cc_);
    memset(ebits_, 0, sizeof ebits_);
    memset(esyms_, 0, sizeof esyms_);
    memset(cur_env_, 0, sizeof cur_env_);
    memset(lvcur_, 0, sizeof lvcur_);
  }
  void Quit() {
#define H_QUIT(F)                                                                                \
  if (t_##F) LV::F::quit(t_##F), tc_unmap(t_##F, sizeof(typename LV::F::T)), t_##F = nullptr; \
  f_##F.Quit();
    AAC_FAMS(H_QUIT)
#undef H_QUIT
    raw_.Quit();
    hl_.Quit();
    hist_.Quit();
    sbrh_.Quit();
  }

  // each family's coder (f_unt ...) and tables (t_unt ...)
#define H_FAM(F)                       \
  tc_fam<f_DEC, typename LV::F> f_##F; \
  typename LV::F::T* t_##F;
  AAC_FAMS(H_FAM)
#undef H_FAM

  void start() {
#define H_MAP(F) t_##F = tc_table<typename LV::F::T>(), LV::F::init(t_##F), f_##F.wire(t_##F, this);
    AAC_FAMS(H_MAP)
#undef H_MAP
    raw_.start();
    hist_.resize(kSlots);
    sbrh_.resize(kSlots);
    memset((void*)&last0_, 0, sizeof last0_);
  }
  u64 table_bytes() const {
    u64 n = 0;
#define H_BYTES(F) n += LV::F::bytes(t_##F);
    AAC_FAMS(H_BYTES)
#undef H_BYTES
    return n;
  }

  // ------------------------------------------------------------------------
  // the stream's own values: a field of the unt family, with its own last
  // two values and a context c the caller gives
  i64 last_[UF_N][2];
  i64 unit(int fld, int c, i64 x) {
    tc_stage = STG_UNIT;
    H_unt_v D;
    D.fld = fld;
    D.ctx = c;
    D.p1 = tc_qlog(last_[fld][0]);
    D.p2 = tc_qlog(last_[fld][1]);
    tcx X;
    f_unt.make(D, X);
    x = f_unt.code(X, ENC ? x : 0);
    last_[fld][1] = last_[fld][0];
    last_[fld][0] = x;
    tc_syms[STG_UNIT]++;
    return x;
  }

  // bytes nothing else models (RawModel)
  RawModel<f_DEC> raw_;
  u32 raw(u32 b) {
    tc_stage = STG_RAW;
    return raw_.code(this, b);
  }
  int raw_plain(int v) {
    tc_stage = STG_RAW;
    return raw_.plain(this, v);
  }
  u32 raw_byte(u32 b) {
    tc_stage = STG_RAW;
    return raw_.plain_byte(this, b);
  }
  bool raw_random(const u8* p, size_t n) { return raw_.random(p, n); }

  // -v: where the bits of each element went (coding)
  double ebits_[E_N];
  u64 esyms_[E_N];
  INLINE double bits_now() const {
    double s = 0;
    for (int i = 0; i < STG_N; i++) s += tc_bits[i];
    return s;
  }

  // ------------------------------------------------------------------------
  // The walk's state as the model reads it

  // the slot of the walk's channel element and channel
  static int slot_of(const Walk& w) {
    int t = w.at.type == TYPE_SCE ? 0 : w.at.type == TYPE_CPE ? 1 : w.at.type == TYPE_CCE ? 2 : 3;
    return (t * 16 + (w.at.tag & 15)) * 2 + (w.at.chn & 1);
  }
  Buf<ChanHist> hist_;
  CurChan cc_[2];  // the channel element's channels as they are coded
  ChanHist last0_;  // a CPE's first channel's last frame, as its second is coded (hist_ has this one's)
  // the walk's channel, its record of the last frame (valid: one the
  // current frame's bands can be read against), the other channel of the CPE
  static const ChanData& wcd(const Walk& w) { return w.cd[w.cur]; }
  CurChan& cur(const Walk& w) { return cc_[w.cur & 1]; }
  // the last frame of the channel, where its windows are the current
  // frame's kind (long, short) and its bands the same
  const ChanHist* last_of(const Walk& w) const {
    const ChanHist& h = hist_[cc_[w.cur & 1].slot];
    const Ics& ii = w.cd[w.cur].ii;
    if (!h.valid || (h.ws == EIGHT_SHORT_SEQUENCE) != (ii.ws == EIGHT_SHORT_SEQUENCE) || h.num_swb != ii.num_swb) return nullptr;
    return &h;
  }
  // the first channel of the CPE whose second is walked (common window:
  // its bands are this one's)
  static const ChanData* other_of(const Walk& w) {
    if (w.at.type != TYPE_CPE || w.cur != 1) return nullptr;
    const ChanData& o = w.cd[0];
    if (!o.cw) return nullptr;
    return &o;
  }

  void walk_note(const Walk& w, int what) {
    if (what == N_ICS) {
      CurChan& c = cur(w);
      c.slot = slot_of(w);
      if (w.at.type == TYPE_CPE && w.at.chn == 0) last0_ = hist_[c.slot];
      c.off[0] = c.off[1] = c.off[2] = 0;
      for (int k = 0; k < 3; k++) c.d1[k] = c.d2[k] = 0;
      c.pc = 16;
      c.last_esc = hist_[c.slot].valid ? hist_[c.slot].last_esc : 0;
      c.bkey = -1;
      memset(c.sfd, 0, sizeof c.sfd);
      memset(c.secb, 0, sizeof c.secb);
    } else if (what == N_ICS_END) {
      const CurChan& c = cur(w);
      const ChanData& d = wcd(w);
      ChanHist& h = hist_[c.slot];
      h.valid = 1;
      h.ws = d.ii.ws;
      h.max_sfb = d.ii.max_sfb;
      h.num_groups = d.ii.num_groups;
      h.num_windows = d.ii.num_windows;
      h.num_swb = d.ii.num_swb;
      memcpy(h.group_len, d.ii.group_len, sizeof h.group_len);
      h.gg = d.gg;
      h.last_esc = c.last_esc;
      int nb = d.ii.num_groups * d.ii.max_sfb;
      memset(h.bt, 0, sizeof h.bt);
      memset(h.sf, 0, sizeof h.sf);
      memcpy(h.bt, d.bt, (size_t)nb);
      memcpy(h.sf, d.sf, (size_t)nb * sizeof(int16_t));
      memcpy(h.sfd, c.sfd, sizeof h.sfd);
      memcpy(h.secb, c.secb, sizeof h.secb);
      memset(h.msb, 0, sizeof h.msb);
      const ChanData& d0 = w.cd[0];
      if (w.at.type == TYPE_CPE && d0.ms_present && d.ii.num_windows == 1)
        for (int b = 0; b < d.ii.max_sfb && b < 64; b++) h.msb[b] = d0.ms[b];
      memcpy(h.coef, d.coef, sizeof h.coef);
      if (d.ii.num_windows == 1) {
        if (h.av_swb != d.ii.num_swb) {
          memset(h.av, 0, sizeof h.av), h.av_n = 0, h.av_swb = d.ii.num_swb;
          for (int b = 0; b < 64; b++) h.avsf[b] = INT16_MIN;
        }
        for (int b = 0; b < d.ii.num_swb && b < 64; b++) {
          // (a band coded now: the average to its scalefactor first)
          int f = 4096;
          if (b < d.ii.max_sfb && d.bt[b] && d.bt[b] < 13) {
            if (h.avsf[b] != INT16_MIN) f = kScale.v[tc_clamp(h.avsf[b] - d.sf[b], -60, 60) + 60];
            h.avsf[b] = d.sf[b];
          }
          for (int k = d.ii.swb[b]; k < d.ii.swb[b + 1]; k++) {
            const i64 m = Min(iabs(d.coef[k]), 4095), v = ((i64)h.av[k] * f) >> 12;
            h.av[k] = (uint16_t)Min<i64>(65535, v - (v >> 2) + 4 * m);
          }
        }
        h.av_n = Min(h.av_n + 1, 1 << 20);
      } else {
        if (h.av8_swb != d.ii.num_swb) {
          memset(h.av8, 0, sizeof h.av8), h.av8_n = 0, h.av8_swb = d.ii.num_swb;
          for (int b = 0; b < 16; b++) h.av8sf[b] = INT16_MIN;
        }
        for (int wn = 0, g = 0, x = d.ii.group_len[0]; wn < 8; wn++) {
          if (wn >= x && g < d.ii.num_groups - 1) x += d.ii.group_len[++g];
          for (int b = 0; b < d.ii.num_swb && b < 16; b++) {
            int f = 4096;
            const int i = g * d.ii.max_sfb + b;
            if (b < d.ii.max_sfb && d.bt[i] && d.bt[i] < 13) {
              if (h.av8sf[b] != INT16_MIN) f = kScale.v[tc_clamp(h.av8sf[b] - d.sf[i], -60, 60) + 60];
              h.av8sf[b] = d.sf[i];
            }
            for (int k = d.ii.swb[b]; k < d.ii.swb[b + 1]; k++) {
              const i64 m = Min(iabs(d.coef[wn * 128 + k]), 4095), v = ((i64)h.av8[k] * f) >> 12;
              h.av8[k] = (uint16_t)Min<i64>(65535, v - (v >> 3) + 2 * m);
            }
          }
        }
        h.av8_n = Min(h.av8_n + 1, 1 << 20);
      }
    }
  }

  // ------------------------------------------------------------------------
  // a field of the syntax (the walk's val()): coded as the difference to
  // the last value of the element at the index in the channel, by the hdr
  // family
  struct HLast {
    i64 v, d1, d2;  // the last value, the last two differences
  };
  Map32<HLast> hl_;
  int lasttag_, runpos_;
  int lastslot_, lastfi_;  // the last field's slot and index, and its value
  i64 lastv_;
  // ADTS's adts_buffer_fullness, an encoder's bit reservoir after the frame
  // in units of 32 bits a channel (0x7ff: none, VBR), given after the
  // frame: the reservoir gains a mean frame's bits and loses the frame's.
  // The mean from the frames since the first of the run (n of them after
  // it, their bits): the fullness then is the first's, plus what n mean
  // frames bring, less their bits
  struct AdtsRes {
    i64 n, bits, first, last;
    int unit;
  } res_;
  i64 fullness_guess(const Walk& w) {
    static const int kChans[8] = {0, 1, 2, 3, 4, 5, 6, 8};
    AdtsRes& r = res_;
    const int u = 32 * kChans[w.st.fixed.chan_config & 7];
    if (!u || u != r.unit || r.last < 0 || r.last == 0x7ff) return r.last < 0 ? 0 : r.last;
    if (!r.n) return r.last;
    // last + (mean - bits) / u, mean = (bits so far + u (last - first)) / n: rounded
    const i64 num = r.bits + (i64)u * (r.last - r.first) - (i64)w.fw.pos * r.n, den = (i64)u * r.n;
    const i64 q = num >= 0 ? (2 * num + den) / (2 * den) : -((-2 * num + den) / (2 * den));
    return tc_clamp(r.last + q, (i64)0, (i64)0x7fe);
  }
  void fullness_seen(const Walk& w, i64 x) {
    static const int kChans[8] = {0, 1, 2, 3, 4, 5, 6, 8};
    AdtsRes& r = res_;
    const int u = 32 * kChans[w.st.fixed.chan_config & 7];
    // (a run starts again where the fullness is none, the channels change, or it strays)
    if (u != r.unit || r.last < 0 || x == 0x7ff || r.last == 0x7ff || (r.n && iabs((int)(x - fullness_guess(w))) > 64)) {
      r.n = r.bits = 0;
      r.first = x;
      r.unit = u;
    } else {
      r.n++;
      r.bits += (i64)w.fw.pos;
      if (r.n > ((i64)1 << 20)) r.n /= 2, r.bits /= 2, r.first = x - (x - r.first) / 2;  // (no overflow)
    }
    r.last = x;
  }
  i64 hdr_val(const Walk& w, int e, int n, i64 x, int i) {
    tc_stage = STG_HDR;
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    // (the containers' fields not by channel)
    int slot = e >= E_ELEM ? slot_of(w) : 0;
    int fi = i < 63 ? i : 63;
    HLast& h = hl_.get((u32)e | (u32)fi << 8 | (u32)slot << 14);
    const bool next = e == lasttag_ && slot == lastslot_ && fi == lastfi_ + 1;  // (the index after the last value's)
    runpos_ = e == lasttag_ ? Min(runpos_ + 1, 15) : 0;
    lasttag_ = e;
    H_hdr_v D;
    D.tag = e;
    D.fld = fi;
    D.p1 = sq2(h.d1);
    D.p2 = sq2(h.d2);
    D.p1r = (int)(h.v & 255);
    D.pos = tc_qlog(runpos_);
    D.nb = n < 32 ? n : 32;
    const ChanData& cd = wcd(w);
    D.ws = e >= E_ELEM && cd.ii.num_windows ? cd.ii.ws : 4;
    D.ch = e >= E_ELEM ? w.at.chn : 2;
    D.pb = next ? 1 + (int)(lastv_ & 7) : 0;
    // (the second channel's global_gain against the first's of its CPE; the
    // fullness against its guess)
    const i64 base = e == E_GLOBAL_GAIN && w.at.type == TYPE_CPE && w.cur == 1 ? w.cd[0].gg
                     : e == E_ADTS_FULL                                         ? fullness_guess(w)
                                                                                : h.v;
    // the sign by the element, and whether the value is at the bottom of its
    // range or at the top (a field's: a bit's change has one sign)
    static_assert(E_N <= H_hdr_NG / 3, "the fields' sign contexts: 3 an element");
    const int bound = base <= 0 ? 1 : n > 0 && n < 32 && base >= ((i64)1 << n) - 1 ? 2 : 0;
    tcx X;
    f_hdr.make(D, X);
    i64 dx = ENC ? x - base : 0;
    i64 d = f_hdr.codes(X, dx, (u32)(e * 3 + bound));
    x = base + d;
    h.d2 = h.d1;
    h.d1 = d;
    h.v = x;
    lastslot_ = slot, lastfi_ = fi, lastv_ = x;
    if (e == E_ADTS_FULL) fullness_seen(w, x);
    tc_syms[STG_HDR]++;
    if (ENC && tc_verbose) ebits_[e] += bits_now() - b0, esyms_[e]++;
    return x;
  }

  // ------------------------------------------------------------------------
  // section_data: a section's codebook (its bits, by a tree), then its
  // length (a decision at each band: whether the section goes on)

  // the codebook of the band in the last frame (16 not known)
  static int last_cb(const ChanHist* h, int g, int sfb) {
    if (!h || g >= h->num_groups || sfb >= h->max_sfb) return 16;
    return h->bt[g * h->max_sfb + sfb];
  }
  void sec_vars(const Walk& w, H_sec_v& D, int g, int sfb) {
    const ChanData& cd = wcd(w);
    const Ics& ii = cd.ii;
    const ChanHist* h = last_of(w);
    const ChanData* o = other_of(w);
    D.pc = cur(w).pc;
    D.fc = last_cb(h, g, sfb);
    D.gc = g > 0 ? cd.bt[(g - 1) * ii.max_sfb + sfb] : 16;
    D.oc = o ? o->bt[g * ii.max_sfb + sfb] : 16;
    D.sb = sfb;
    D.rem = ii.max_sfb - sfb;
    D.ws = ii.ws;
    D.gr = g;
  }
  int sect_cb(const Walk& w, int v, int g) {
    tc_stage = STG_SEC;
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    H_sec_v D;
    sec_vars(w, D, g, w.at.sfb);
    D.kd = 0;
    D.cb = 16;
    D.run = 0;
    D.fn = 0;
    tcx X;
    f_sec.make(D, X);
    f_sec.select(X);
    int node = 1;
    for (int j = 3; j >= 0; j--) node = node * 2 + f_sec.bit(node, ENC ? (v >> j) & 1 : 0);
    tc_syms[STG_SEC]++;
    if (ENC && tc_verbose) ebits_[E_SECT_CB] += bits_now() - b0, esyms_[E_SECT_CB]++;
    return node - 16;
  }
  int sect_len(const Walk& w, int len, int g) {
    tc_stage = STG_SEC;
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    const ChanData& cd = wcd(w);
    const Ics& ii = cd.ii;
    const ChanHist* h = last_of(w);
    const int k = w.at.sfb, cb = w.at.cb;
    int n = 0;
    while (k + n < ii.max_sfb) {
      int b = k + n;
      H_sec_v D;
      sec_vars(w, D, g, b);
      D.kd = 1;
      D.cb = cb;
      D.run = n;
      // the last frame at the band: the codebook's, and whether a section started there
      int lc = last_cb(h, g, b);
      D.fn = lc == 16 ? 0 : 1 + (lc == cb ? 0 : 2) + (h->secb[g * h->max_sfb + b] ? 1 : 0);
      tcx X;
      f_sec.make(D, X);
      f_sec.select(X);
      if (!f_sec.bit(0, ENC && n < len)) break;
      n++;
    }
    CurChan& c = cur(w);
    if (k < ii.max_sfb) c.secb[g * ii.max_sfb + k] = 1;
    c.pc = cb;
    if (k + n >= ii.max_sfb) c.pc = 16;  // (the group's last: the next group starts afresh)
    tc_syms[STG_SEC]++;
    if (ENC && tc_verbose) ebits_[E_SECT_LEN] += bits_now() - b0, esyms_[E_SECT_LEN]++;
    return n;
  }

  // ------------------------------------------------------------------------
  // scale_factor_data: the deltas (scf); global_gain and the first noise
  // energy set the offsets they are deltas of, as the walk's
  int sf_val(const Walk& w, int v, int sfb) {
    tc_stage = STG_SF;
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    const ChanData& cd = wcd(w);
    const Ics& ii = cd.ii;
    CurChan& c = cur(w);
    const int g = w.at.g, cb = w.at.cb, idx = g * ii.max_sfb + sfb;
    const int kd = cb == 13 ? 2 : cb >= 14 ? 1 : 0;
    const ChanHist* h = last_of(w);
    const ChanData* o = other_of(w);
    H_scf_v D;
    D.kd = kd;
    D.sb = sfb;
    D.d1 = sq2(c.d1[kd]);
    D.d2 = sq2(c.d2[kd]);
    int lb = h && g < h->num_groups && sfb < h->max_sfb ? h->bt[g * h->max_sfb + sfb] : -1;
    bool same = lb >= 0 && (lb == 13 ? 2 : lb >= 14 ? 1 : lb ? 0 : -1) == kd;
    int pa = same ? h->sf[g * h->max_sfb + sfb] - c.off[kd] : 0;
    D.pd = same ? 1 + sq2(h->sfd[g * h->max_sfb + sfb]) : 0;
    D.pa = same ? 1 + sq2(pa) : 0;
    D.cb = cb;
    D.oc = o && o->bt[idx] && ((o->bt[idx] == 13 ? 2 : o->bt[idx] >= 14 ? 1 : 0) == kd) ? 1 + sq2(cc_[0].sfd[idx]) : 0;
    D.ws = ii.ws;
    D.gr = g;
    D.gd = h ? 1 + sq2(cd.gg - h->gg) : 0;
    D.pv = sq2(c.off[kd] - (kd == 1 ? 0 : cd.gg));
    tcx X;
    f_scf.make(D, X);
    int d = (int)f_scf.codes(X, v, (u32)(kd * 3 + sgn3(pa)));
    c.off[kd] += d;
    c.d2[kd] = c.d1[kd];
    c.d1[kd] = d;
    c.sfd[idx] = (int8_t)d;
    tc_syms[STG_SF]++;
    if (ENC && tc_verbose) ebits_[E_SF] += bits_now() - b0, esyms_[E_SF]++;
    return d;
  }

  // ------------------------------------------------------------------------
  // spectral_data: each value's magnitude (spc: in unary, above j at node
  // j; codebook 11's escapes less 16 by esc) and sign (sgn)
  void spec(const Walk& w, int cb, int win, int k, int* c, int dim) {
    for (int j = 0; j < dim; j++) c[j] = coef(w, cb, win, k + j, j, c[j]);
  }
  int coef(const Walk& w, int cb, int win, int k, int tp, int v) {
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    const ChanData& cd = wcd(w);
    const Ics& ii = cd.ii;
    CurChan& c = cur(w);
    const int sfb = w.at.sfb, g = w.at.g;
    const int* cc = cd.coef + win * 128;  // (this window's, up to k)
    const int a = ii.swb[sfb];
    const int bkey = win * 64 + sfb;
    if (c.bkey != bkey) c.bkey = bkey, c.bnz = 0, c.bsum = 0;
    // the place in the last frame, or the window before
    const ChanHist* h = last_of(w);
    const int* pp = nullptr;  // the values at the place before, by k
    int psf = INT_MIN;        // and the band's scalefactor there
    if (ii.ws == EIGHT_SHORT_SEQUENCE) {
      if (win > 0) {  // the window before, of this frame (its group's band)
        pp = cd.coef + (win - 1) * 128;
        int pg = 0;
        for (int x = 0; pg < ii.num_groups - 1; pg++) {
          x += ii.group_len[pg];
          if (win - 1 < x) break;
        }
        int pb = cd.bt[pg * ii.max_sfb + sfb];
        if (pb && pb < 13) psf = cd.sf[pg * ii.max_sfb + sfb];
      } else if (h && h->num_windows == 8) {  // the last window of the last frame
        pp = h->coef + 7 * 128;
        if (sfb < h->max_sfb) {
          int i = (h->num_groups - 1) * h->max_sfb + sfb;
          if (h->bt[i] && h->bt[i] < 13) psf = h->sf[i];
        }
      }
    } else if (h) {
      pp = h->coef;
      if (sfb < h->max_sfb && h->bt[sfb] && h->bt[sfb] < 13) psf = h->sf[sfb];
    }
    const int sfc = cd.sf[g * ii.max_sfb + sfb];
    const ChanData* o = other_of(w);
    const int oval = o ? o->coef[win * 128 + k] : 0;
    const int pv = pp ? pp[k] : 0;
    // A CPE's band of long windows, M/S now and L/R in the last frame or the
    // other way: the last frame's values about the place in this frame's
    // terms (L = M + S, R = M - S; M = (L + R) / 2, S = (L - R) / 2), from both
    // channels' values and scalefactors there, as this band quantizes them
    int conv[3] = {-1, -1, -1};
    if (ii.ws != EIGHT_SHORT_SEQUENCE && h && w.at.type == TYPE_CPE && sfb < h->max_sfb) {
      const ChanData& d0 = w.cd[0];
      const int now = d0.ms_present ? d0.ms[sfb] : 0;
      const ChanHist* o0 = w.at.chn ? &last0_ : &hist_[c.slot];
      const ChanHist* o1 = &hist_[c.slot | 1];
      auto coded = [&](const ChanHist* x) {
        return x->valid && x->num_windows == 1 && x->num_swb == ii.num_swb && sfb < x->max_sfb && x->bt[sfb] &&
               x->bt[sfb] < 13;
      };
      if (now != h->msb[sfb] && coded(o0) && coded(o1)) {
        const int d0s = tc_clamp(o0->sf[sfb] - sfc, -60, 60), d1s = tc_clamp(o1->sf[sfb] - sfc, -60, 60);
        for (int j = -1; j <= 1; j++) {
          if (k + j < 0 || k + j > 1023) continue;
          const i64 x0 = lin_q(o0->coef[k + j], d0s), x1 = lin_q(o1->coef[k + j], d1s);
          const i64 y = w.at.chn ? x0 - x1 : x0 + x1;
          conv[j + 1] = q_of_lin(now ? y / 2 : y);
        }
      }
    }

    tc_stage = STG_SPEC;
    H_spc_v D;
    D.cb = cb;
    D.sb = sfb;
    D.bp = k - a;
    D.a1 = k > 0 ? Min(iabs(cc[k - 1]), 15) : 0;
    D.a2 = k > 1 ? Min(iabs(cc[k - 2]), 15) : 0;
    {
      int s = 0;
      for (int j = 1; j <= 4 && j <= k; j++) s += iabs(cc[k - j]);
      D.a3 = tc_qlog(s);
    }
    D.pf = pp ? 1 + Min(iabs(pv), 14) : 0;
    const int klast = ii.ws == EIGHT_SHORT_SEQUENCE ? 127 : 1023;  // (the window's last place)
    D.pn = pp ? 1 + tc_qlog(iabs(k > 0 ? pp[k - 1] : 0) + 2 * iabs(pv) + iabs(k < klast ? pp[k + 1] : 0)) : 0;
    if (conv[1] >= 0) {  // (the values converted: their scale is this band's already)
      D.pf = 1 + Min(conv[1], 14);
      D.pn = 1 + tc_qlog(Max(conv[0], 0) + 2 * conv[1] + Max(conv[2], 0));
      D.ex = 1 + tc_qlog((i64)4 * conv[1]);
      D.ds = psf != INT_MIN ? 1 + sq2(sfc - psf) : 0;
    } else if (pp && psf != INT_MIN) {
      int dsf = tc_clamp(psf - sfc, -60, 60);
      D.ex = 1 + tc_qlog(((i64)4 * iabs(pv) * kScale.v[dsf + 60]) >> 12);
      D.ds = 1 + sq2(sfc - psf);
    } else {
      D.ex = 0;
      D.ds = 0;
    }
    D.oc = o ? 1 + Min(iabs(oval), 14) : 0;
    D.ms = o && o->ms_present ? o->ms[g * ii.max_sfb + sfb] : 2;
    D.ws = ii.ws == EIGHT_SHORT_SEQUENCE;
    D.tp = tp;
    D.nz = Min(c.bnz, 7);
    D.qn = tc_qlog(c.bsum);
    D.kq = tc_qlog(k);
    D.av = 0;
    i64 avs = 0;
    {
      const ChanHist& hh = hist_[c.slot];
      if (ii.ws != EIGHT_SHORT_SEQUENCE && hh.av_n && hh.av_swb == ii.num_swb) {
        i64 v = hh.av[k];
        if (hh.avsf[sfb] != INT16_MIN) v = (v * kScale.v[tc_clamp(hh.avsf[sfb] - sfc, -60, 60) + 60]) >> 12;
        D.av = 1 + Min(tc_qlog(v), 30);
        avs = v;
      } else if (ii.ws == EIGHT_SHORT_SEQUENCE && hh.av8_n && hh.av8_swb == ii.num_swb && sfb < 16) {
        i64 v = hh.av8[k];
        if (hh.av8sf[sfb] != INT16_MIN) v = (v * kScale.v[tc_clamp(hh.av8sf[sfb] - sfc, -60, 60) + 60]) >> 12;
        D.av = 1 + Min(tc_qlog(v), 30);
        avs = v;
      }
    }
    // (the last frame's value scaled by the band's scalefactor change, or in this frame's terms)
    int pfs = iabs(pv);
    if (conv[1] >= 0)
      pfs = conv[1];
    else if (pp && psf != INT_MIN)
      pfs = (int)(((i64)iabs(pv) * kScale.v[tc_clamp(psf - sfc, -60, 60) + 60]) >> 12);
    {
      i64 e = avs + (pp ? 16 * (i64)pfs : 0) + 8 * (i64)(k > 0 ? iabs(cc[k - 1]) : 0) + 8 * (i64)(k > 1 ? iabs(cc[k - 2]) : 0) + (o ? 16 * (i64)iabs(oval) : 0);
      D.xp = 1 + Min(tc_qlog(e), 39);
    }
    tcx X;
    f_spc.make(D, X);
    f_spc.select(X);
    const int m = ENC ? iabs(v) : 0;
    const int lim = cb == 11 ? 16 : spec_cb[cb - 1].maxval;
    int mm = 0;
    while (mm < lim && f_spc.bit(mm, ENC && m > mm)) mm++;
    tc_syms[STG_SPEC]++;
    if (cb == 11 && mm == 16) {  // an escape: the rest by esc
      tc_stage = STG_ESC;
      H_esc_v E;
      E.kq = tc_qlog(k);
      E.a1 = k > 0 ? tc_qlog(iabs(cc[k - 1])) : 0;
      E.pf = pp ? 1 + tc_qlog(pfs) : 0;
      E.oc = o ? 1 + tc_qlog(iabs(oval)) : 0;
      E.ws = D.ws;
      E.la = tc_qlog(c.last_esc);
      E.sb = sfb;
      E.av = D.av;
      tcx Y;
      f_esc.make(E, Y);
      i64 r = f_esc.code(Y, ENC ? m - 16 : 0);
      if (r > 8191 - 16) err_raise_s("corrupt stream: a spectral escape out of range");
      mm = 16 + (int)r;
      c.last_esc = mm;
      tc_syms[STG_ESC]++;
    }
    int s = 0;
    if (mm) {
      tc_stage = STG_SGN;
      H_sgn_v S;
      S.cb = cb;
      S.oc = o ? sgn3(oval) : 0;
      S.ms = D.ms;
      S.s1 = k > 0 ? sgn3(cc[k - 1]) : 0;
      S.s2 = k > 1 ? sgn3(cc[k - 2]) : 0;
      S.pf = pp ? sgn3(pv) : 0;
      S.mg = Min(mm, 7);
      S.kq = D.kq;
      S.ws = D.ws;
      tcx Z;
      f_sgn.make(S, Z);
      f_sgn.select(Z);
      s = f_sgn.bit(0, ENC && v < 0);
      tc_syms[STG_SGN]++;
      c.bnz++;
      c.bsum += mm;
    }
    if (ENC && tc_verbose) ebits_[E_SPEC] += bits_now() - b0, esyms_[E_SPEC]++;
    return s ? -mm : mm;
  }

  // ------------------------------------------------------------------------
  // SBR envelopes, noise floors, their start values, and PS parameters (sbr)
  Buf<SbrHist> sbrh_;
  // the SBR element's slot (the channel element before the payload)
  static int sbr_slot(const Walk& w) {
    int t = w.prev_che_type == TYPE_SCE ? 0 : w.prev_che_type == TYPE_CPE ? 1 : 2;
    return (t * 16 + (w.prev_che_tag & 15)) * 2 + (w.at.chn & 1);
  }
  i64 sbr_val(const Walk& w, int e, i64 v, int band, int n) {
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    bool ps = e >= E_PS_HDR;
    tc_stage = ps ? STG_PS : STG_SBR;
    SbrHist& sh = sbrh_[sbr_slot(w)];
    int kd, hk, lvk = -1;
    switch (e) {
      case E_SENV: kd = (w.at.bal ? 4 : 0) + w.at.dt, hk = w.at.bal ? 2 : 0, lvk = w.at.bal ? -1 : 0; break;
      case E_SNOISE: kd = (w.at.bal ? 6 : 2) + w.at.dt, hk = w.at.bal ? 3 : 1, lvk = w.at.bal ? -1 : 1; break;
      case E_PS_IID: kd = 8 + w.at.dt, hk = 4; break;
      case E_PS_ICC: kd = 10 + w.at.dt, hk = 5; break;
      case E_PS_IPD: kd = 12, hk = 6; break;
      case E_PS_OPD: kd = 13, hk = 7; break;
      case E_SENV_START: kd = 14, hk = w.at.bal ? 2 : 0, lvk = w.at.bal ? -1 : 0; break;
      default: kd = 14, hk = w.at.bal ? 3 : 1, lvk = w.at.bal ? -1 : 1; break;  // E_SNOISE_START
    }
    const int bd = band < 63 ? band : 63;
    H_sbr_v D;
    D.kd = kd;
    D.bd = bd;
    D.nb = w.at.nb;
    D.d1 = band > 0 && bd - 1 < 64 ? sq2(cur_env_[hk][bd - 1]) : 0;
    D.d2 = band > 1 ? sq2(cur_env_[hk][bd - 2]) : 0;
    D.pv = sh.have[hk] ? 1 + sq2(sh.last[hk][bd]) : 0;
    D.ar = w.at.sc ? w.at.sc->amp_res : 0;
    D.fr = w.at.sc && w.at.env < 8 ? w.at.sc->freq_res[w.at.env] : 0;
    D.en = w.at.env;
    D.ch = w.at.chn;
    const int nbw = Max(1, Min(w.at.nb, 64));
    // (the band at the place of this one in the last envelope, whose bands may be fewer or more)
    const int pb = lvk >= 0 && sh.lvn[lvk] ? Min(bd * sh.lvn[lvk] / nbw, 63) : 0;
    D.lv = lvk >= 0 && sh.lvn[lvk] ? 1 + tc_qlog(sh.lv[lvk][pb]) : 0;
    D.q = w.at.ph ? w.at.ph->iid_quant : 0;
    tcx X;
    f_sbr.make(D, X);
    i64 x;
    if (kd == 14) {
      x = f_sbr.code(X, ENC ? v : 0);
      (void)n;
    } else {
      x = f_sbr.codes(X, v, (u32)(kd * 3 + sgn3(sh.have[hk] ? sh.last[hk][bd] : 0)));
    }
    const int xv = (int)Max<i64>(-127, Min<i64>(x, 127));
    if (kd != 14) {
      cur_env_[hk][bd] = (int8_t)xv;
      sh.last[hk][bd] = (int8_t)xv;
      sh.have[hk] = 1;
    } else {
      cur_env_[hk][0] = 0;  // (a start value: no delta at the first band)
    }
    // the levels the deltas leave (envelope and noise of a channel that is
    // not a balance one): across frequency from the start value, in time
    // from the last envelope's
    if (lvk >= 0) {
      if (e == E_SENV_START || e == E_SNOISE_START)
        lvcur_[lvk][0] = (int16_t)Max<i64>(-1000, Min<i64>(x, 1000));
      else if (!w.at.dt)
        lvcur_[lvk][bd] = (int16_t)tc_clamp((bd > 0 ? lvcur_[lvk][bd - 1] : 0) + xv, -1000, 1000);
      else
        lvcur_[lvk][bd] = (int16_t)tc_clamp((sh.lvn[lvk] ? sh.lv[lvk][pb] : 0) + xv, -1000, 1000);
      if (bd + 1 >= nbw) {  // the envelope's last band: its levels are the last envelope's now
        memcpy(sh.lv[lvk], lvcur_[lvk], sizeof sh.lv[lvk]);
        sh.lvn[lvk] = (uint8_t)nbw;
      }
    }
    tc_syms[ps ? STG_PS : STG_SBR]++;
    if (ENC && tc_verbose) ebits_[e] += bits_now() - b0, esyms_[e]++;
    return x;
  }
  int8_t cur_env_[8][64];  // the values of the envelope being coded, by kind
  int16_t lvcur_[2][64];

  // ------------------------------------------------------------------------
  // The walk's values (ModelChan)
  i64 val(const Walk& w, int e, int n, i64 x, int i) {
    switch (e) {
      case E_SECT_CB: return sect_cb(w, (int)x, i);
      case E_SECT_LEN: return sect_len(w, (int)x, i);
      case E_SENV_START:
      case E_SNOISE_START: return sbr_val(w, e, x, 0, n);
      case E_GLOBAL_GAIN: {
        x = hdr_val(w, e, n, x, i);
        CurChan& c = cur(w);
        c.off[0] = (int)x, c.off[1] = 0, c.off[2] = (int)x - 90;
        return x;
      }
      case E_NOISE_NRG: {
        x = hdr_val(w, e, n, x, i);
        cur(w).off[2] += (int)x - 256;
        return x;
      }
      default: return hdr_val(w, e, n, x, i);
    }
  }
  int hv(const Walk& w, int e, const Huff& h, int v, int i) {
    (void)h;
    switch (e) {
      case E_SF: return sf_val(w, v, i);
      case E_SENV:
      case E_SNOISE:
      case E_PS_IID:
      case E_PS_ICC:
      case E_PS_IPD:
      case E_PS_OPD: return (int)sbr_val(w, e, v, i, 0);
      default: return (int)hdr_val(w, e, 0, v, i);
    }
  }
  // bit strings: their bytes (the last one's bits past them zero), by the
  // byte model
  void bits(const Walk& w, int e, uint8_t* b, size_t nbits) {
    (void)w;
    double b0 = ENC && tc_verbose ? bits_now() : 0;
    size_t n = (nbits + 7) >> 3;
    tc_stage = STG_BITS;
    for (size_t k = 0; k < n; k++) b[k] = (uint8_t)raw_.code(this, ENC ? b[k] : 0);
    if (ENC && tc_verbose) ebits_[e] += bits_now() - b0, esyms_[e] += n;
  }
};

// The channel of the walk that codes (decodes) its values with the model
// (log: coding, the values the parse logged)
template <int f_DEC, class LV>
struct ModelChan : Chan {
  static constexpr int ENC = f_DEC == 0;
  void Init(Model<f_DEC, LV>& m, LogChan* log) {
    W = true;
    m_ = &m;
    log_ = log;
    w_ = nullptr;
  }
  void begin(Walk& w) override { w_ = &w; }
  int64_t val(int e, int n, int64_t, int i) override { return m_->val(*w_, e, n, ENC ? log_->l_->get() : 0, i); }
  int hv(int e, const Huff& h, int, int i) override { return m_->hv(*w_, e, h, ENC ? (int)log_->l_->get() : 0, i); }
  void spec(int cb, int win, int k, int* c, int dim) override {
    if (ENC)
      for (int j = 0; j < dim; j++) c[j] = (int)log_->l_->get();
    m_->spec(*w_, cb, win, k, c, dim);
  }
  void bits(int e, uint8_t* b, size_t nbits) override {
    if (ENC) log_->bits(e, b, nbits);
    m_->bits(*w_, e, b, nbits);
  }
  void note(int what) override { m_->walk_note(*w_, what); }

  Model<f_DEC, LV>* m_;
  LogChan* log_;
  Walk* w_;
};

}  // namespace aacpack
