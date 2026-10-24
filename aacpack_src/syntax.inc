// The AAC syntax below the containers, walked in both directions by the
// same code (aac2tsv's syntax.h, which follows aacdec: aacdec.inc,
// aacdec_float.inc, aacsbr.inc, aacps.inc): parsing (R), the walk reads its
// values from the bitstream and hands each to its channel; writing (W), it
// takes each from the channel and writes it (chan.inc). Both call the
// channel in the same order with the same arguments, and the walk's state
// at each call is the same: so a model reading it (model.inc) codes and
// decodes alike.
//
// Where the bitstream has a value derived (a count, a length, a CRC), the
// walk writing derives it too, and the channel has what tells it apart
// when it is not what the walk would derive (a flag, then the value). What
// does not parse is an error (err.inc): a syntax element of a
// raw_data_block that does not is kept as the bits of the rest of its
// frame (a rest), an extension payload as its bits, a unit as its bytes
// (the compressor's trap, units.inc).

namespace aacpack {

enum { TYPE_SCE, TYPE_CPE, TYPE_CCE, TYPE_LFE, TYPE_DSE, TYPE_PCE, TYPE_FIL, TYPE_END, TYPE_REST };
enum { EIGHT_SHORT_SEQUENCE = 2 };

static const int sample_rates[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
                                     16000, 12000, 11025, 8000,  7350,  0,     0,     0};

// PS header values that persist from frame to frame
struct PsHdr {
  int enable_iid, iid_quant, nr_iid_par, nr_ipdopd_par, enable_icc, nr_icc_par, enable_ext;
};

// the SBR header state of a channel element (all int: compared as bytes)
struct SbrState {
  int have_hdr;  // a header was received
  int ok;        // band tables valid for (sp, rate)
  int rate;      // the SBR sample rate the tables were computed for
  int amp_res;   // bs_amp_res of the header
  SbrSpectrum sp;
  SbrBands bands;
  PsHdr ps;
};

struct AdtsFixed {
  int id, layer, prot_absent, profile, sf_index, private_bit, chan_config, original, home;
};

// the syntax state carried from unit to unit (plain ints: copied, and
// compared as bytes)
struct SynState {
  int aot, sf_index, frame960, sbr_rate;  // the configuration of the stream
  int blocks;                             // its access units are parsed
  int er, chan_config;                    // ER syntax: 1 ER LC/LD, 2 ELD
  int have_fixed;                         // ADTS: the fixed header of the last frame
  AdtsFixed fixed;
  int latm_cfg;  // LATM: a StreamMuxConfig was received
  int mux_version, num_subframes, other_present, other_bits;
  SbrState sbr[3][16];  // [SCE, CPE, CCE][element_instance_tag]

  void reset() { memset((void*)this, 0, sizeof *this); }
  bool same(const SynState& o) const { return !memcmp(this, &o, sizeof *this); }
};

// a region of an ADTS CRC: [start, end) bits, at most max of them (0: all)
struct CrcReg {
  size_t start, end;
  int max;
};

struct Ics {
  int ws;  // window_sequence
  int max_sfb;
  int num_windows, num_groups, group_len[8];
  const uint16_t* swb;
  int num_swb;
};

struct SbrCh {
  int num_env, num_noise, amp_res;
  uint8_t freq_res[8], df_env[8], df_noise[2];
};

// An individual channel stream as the walk goes through it: what the
// model reads of the channel being coded (Walk::cur) and, of a CPE, of the
// one before it
struct ChanData {
  Ics ii;
  int gg;              // global_gain
  int cw;              // its CPE has a common window
  int ms_present;      // ms_mask_present (CPE)
  uint8_t bt[8 * 64];  // the codebook (section_data) of each band, at g * max_sfb + sfb
  int16_t sf[8 * 64];  // its scalefactor as the decoder has it: of the spectrum, intensity position or noise energy
  uint8_t ms[8 * 64];  // ms_used
  int coef[1024];      // the spectral values (window w's at w * 128), as far as walked
  int tns, pulse;      // present
};

// what the walk tells its channel it is at (Chan::note)
enum { N_BLOCK, N_ICS, N_ICS_END, N_SBR, N_SBR_END, N_UNIT_END };

static const int8_t sbr_ceil_log2[] = {0, 1, 2, 2, 3, 3};

struct Walk {
  enum { kSubs = 8 };  // counted payloads nested: LATM payload, fill element, SBR payload, its extension, PS's
  bool R;              // parsing; else writing
  Chan* ch;

  // parsing: the bytes, padded (BitReader)
  const uint8_t* buf;
  BitReader br;
  // writing: the unit, and the counted payloads it writes first (their
  // lengths come before them)
  BitWriter fw;
  BitWriter sub[kSubs];
  int wdepth;
  BitWriter* w;
  Buf<uint8_t> tmp;  // a bit string's bytes

  SynState st;

  // per raw_data_block
  int prev_che_type, prev_che_tag;
  Buf<CrcReg> crcr;
  // the compressor's probe: the SBR payloads that came before any SBR
  // header of their element, and the first header of each element
  int sbr_need[3][16], sbr_found[3][16];
  SbrState sbr_first[3][16];

  // where the walk is, for the model
  struct Pos {
    int type, tag, chn;   // the channel element (TYPE_*, its tag or, in ELD, its place), its channel
    int g, sfb, win, k;   // group, band, window, coefficient
    int cb;               // the section's codebook
    int env, dt, bal;     // SBR, PS: envelope, time differential, balance (SBR coupling's second channel)
    int nb;               // the bands of the SBR envelope or PS parameter
    const SbrCh* sc;      // the SBR channel's grid
    const SbrState* ss;   // and its header state
    const PsHdr* ph;
  } at;
  ChanData cd[2];  // the channel element's channels
  int cur;         // cd[cur] is walked
  int rests;       // rests parsed (the compressor's -s, -v)
  int sbr_seen, ps_seen;  // SBR and PS payloads walked (what the stream is, -v)

  void Init() {
    R = true;
    ch = nullptr;
    buf = nullptr;
    br.Init(nullptr, 0, 0);
    fw.Init();
    for (BitWriter& b : sub) b.Init();
    wdepth = 0;
    w = &fw;
    tmp.Init();
    st.reset();
    prev_che_type = -1, prev_che_tag = 0;
    crcr.Init();
    memset(sbr_need, 0, sizeof sbr_need);
    memset(sbr_found, 0, sizeof sbr_found);
    memset((void*)sbr_first, 0, sizeof sbr_first);
    memset((void*)&at, 0, sizeof at);
    memset((void*)cd, 0, sizeof cd);
    cur = 0;
    rests = sbr_seen = ps_seen = 0;
  }
  void Quit() {
    fw.Quit();
    for (BitWriter& b : sub) b.Quit();
    tmp.Quit();
    crcr.Quit();
  }
  // to parse bits of buf with channel c (R), or write a unit with it:
  // the walk's state the model reads as every unit starts, whichever walk
  // went before (the compressor parses a unit and writes it back before its
  // model codes it, the decompressor only decodes it)
  void begin() {
    prev_che_type = -1, prev_che_tag = 0;
    memset((void*)&at, 0, sizeof at);
    for (ChanData& d : cd) memset((void*)&d.ii, 0, sizeof d.ii), d.gg = d.cw = d.ms_present = 0;
    cur = 0;
  }
  void start_read(Chan* c, const uint8_t* b) {
    R = true;
    ch = c;
    buf = b;
    begin();
    c->begin(*this);
  }
  void start_write(Chan* c, size_t cap_bits) {
    R = false;
    ch = c;
    fw.clear();
    fw.cap = cap_bits;
    w = &fw;
    wdepth = 0;
    begin();
    c->begin(*this);
  }

  // ---------- errors ----------

  // a constraint of the syntax is violated: a bad bitstream (R), or bad values (W)
  [[noreturn]] NOINLINE void bad(const char* msg) { err_raise_s(msg); }
  [[noreturn]] NOINLINE void bad_value(int e, int64_t v) {
    err_raise("corrupt stream: %s of %" PRId64, kElemName[e], v);
  }

  // ---------- sub-writers ----------

  void push_sub() {
    if (wdepth == kSubs) bad("payloads nested too deep");
    BitWriter& b = sub[wdepth++];
    b.clear();
    b.cap = fw.cap;
    w = &b;
  }
  BitWriter& pop_sub() {
    BitWriter& b = sub[--wdepth];
    w = wdepth ? &sub[wdepth - 1] : &fw;
    return b;
  }
  size_t bitpos() const { return R ? br.pos : w->pos; }

  // ---------- values ----------

  // a field of n bits (n <= 32), its index i in a run of e
  INLINE uint32_t u(int e, int n, int i = 0) {
    if (!n) return 0;
    uint32_t v = R ? br.get(n) : 0;
    int64_t x = ch->val(e, n, v, i);
    if (!R) {
      if (UNLIKELY(x < 0 || (n < 32 && (x >> n)) || x > 0xffffffffll)) bad_value(e, x);
      w->put((uint32_t)x, n);
    }
    return (uint32_t)x;
  }
  // a field of n bits that the caller read already (parsing) or writes
  // (writing): its value
  INLINE int64_t pass(int e, int n, int64_t v, int i = 0) {
    if (!n) return 0;
    int64_t x = ch->val(e, n, R ? v : 0, i);
    if (!R && UNLIKELY(x < 0 || (n < 32 && (x >> n)) || x > 0xffffffffll)) bad_value(e, x);
    return x;
  }
  // a count or length the bitstream has otherwise, in [lo, hi]
  INLINE int64_t cnt(int e, int64_t v, int64_t lo, int64_t hi, int i = 0) {
    int64_t x = ch->val(e, 0, R ? v : 0, i);
    if (!R && UNLIKELY(x < lo || x > hi)) bad_value(e, x);
    return x;
  }
  // a Huffman coded value
  INLINE int hv(int e, const Huff& h, int i = 0) {
    int v = R ? h.decode_value(br) : 0;
    v = ch->hv(e, h, v, i);
    if (!R) {
      if (UNLIKELY(!h.can_encode_value(v))) bad_value(e, v);
      h.encode_value(*w, v);
    }
    return v;
  }
  // a string of nbits bits (parsing: from br)
  void bitstr(int e, size_t nbits) {
    size_t n = (nbits + 7) >> 3;
    if (R && nbits > br.left()) bad("bitstream overrun");
    if (!R && nbits > w->cap - Min(w->cap, w->pos)) bad("corrupt stream: a unit written past its largest size");
    tmp.resize(n + 8);
    if (R) {
      uint8_t* d = tmp.data();
      size_t p = br.pos;
      if (!(p & 7)) {
        memcpy(d, buf + (p >> 3), n);
      } else {
        int sh = (int)(p & 7);
        const uint8_t* s = buf + (p >> 3);
        for (size_t k = 0; k < n; k++) d[k] = (uint8_t)(s[k] << sh | s[k + 1] >> (8 - sh));
      }
      if (nbits & 7) d[n - 1] &= (uint8_t)(0xff00 >> (nbits & 7));
      br.pos += nbits;
    }
    ch->bits(e, tmp.data(), nbits);
    if (!R) w->put_range(tmp.data(), 0, nbits);
  }
  void note(int what) { ch->note(what); }

  // ---------- individual_channel_stream ----------

  void ltp_data(int max_sfb) {
    if (u(E_LTP_PRESENT, 1)) {
      u(E_LTP_LAG, 11);
      u(E_LTP_COEF, 3);
      int n = max_sfb < 40 ? max_sfb : 40;
      for (int k = 0; k < n; k++) u(E_LTP_USED, 1, k);
    }
  }

  void ics_info(Ics& ii, bool common_window) {
    ii.num_groups = 1;
    ii.group_len[0] = 1;
    if (st.er == 2) {  // ELD: no window_sequence, window_shape, predictor syntax
      ii.ws = 0;
      ii.num_windows = 1;
      ii.swb = st.frame960 ? ff_swb_offset_480[st.sf_index] : ff_swb_offset_512[st.sf_index];
      ii.num_swb = st.frame960 ? ff_aac_num_swb_480[st.sf_index] : ff_aac_num_swb_512[st.sf_index];
      ii.max_sfb = (int)u(E_MAX_SFB, 6);
      if (ii.max_sfb > ii.num_swb) bad("max_sfb exceeds the number of scalefactor bands");
      return;
    }
    u(E_ICS_RES, 1);
    ii.ws = (int)u(E_WIN_SEQ, 2);
    u(E_WIN_SHAPE, 1);
    if (st.aot == 23 && ii.ws) bad("ER AAC LD with a window sequence other than ONLY_LONG");
    if (ii.ws == EIGHT_SHORT_SEQUENCE) {
      ii.num_windows = 8;
      ii.swb = st.frame960 ? ff_swb_offset_120[st.sf_index] : ff_swb_offset_128[st.sf_index];
      ii.num_swb = ff_aac_num_swb_128[st.sf_index];
      ii.max_sfb = (int)u(E_MAX_SFB, 4);
      unsigned grouping = u(E_GROUPING, 7);
      for (int i = 6; i >= 0; i--) {
        if (grouping >> i & 1) {
          ii.group_len[ii.num_groups - 1]++;
        } else {
          ii.num_groups++;
          ii.group_len[ii.num_groups - 1] = 1;
        }
      }
    } else {
      ii.num_windows = 1;
      if (st.aot == 23) {
        ii.swb = st.frame960 ? ff_swb_offset_480[st.sf_index] : ff_swb_offset_512[st.sf_index];
        ii.num_swb = st.frame960 ? ff_aac_num_swb_480[st.sf_index] : ff_aac_num_swb_512[st.sf_index];
      } else {
        ii.swb = st.frame960 ? ff_swb_offset_960[st.sf_index] : ff_swb_offset_1024[st.sf_index];
        ii.num_swb = st.frame960 ? ff_aac_num_swb_960[st.sf_index] : ff_aac_num_swb_1024[st.sf_index];
      }
      ii.max_sfb = (int)u(E_MAX_SFB, 6);
      if (u(E_PRED_PRESENT, 1)) {
        if (st.aot == 23) bad("LTP in ER AAC LD");
        if (st.aot == 1) {
          if (u(E_PRED_RESET, 1)) u(E_PRED_RGROUP, 5);
          int n = ii.max_sfb < ff_aac_pred_sfb_max[st.sf_index] ? ii.max_sfb : ff_aac_pred_sfb_max[st.sf_index];
          for (int k = 0; k < n; k++) u(E_PRED_USED, 1, k);
        } else {
          ltp_data(ii.max_sfb);
          if (common_window) ltp_data(ii.max_sfb);
        }
      }
    }
    if (ii.max_sfb > ii.num_swb) bad("max_sfb exceeds the number of scalefactor bands");
  }

  void section_data(ChanData& c) {
    const Ics& ii = c.ii;
    if (!ii.max_sfb) return;
    const int bits = ii.ws == EIGHT_SHORT_SEQUENCE ? 3 : 5, esc = (1 << bits) - 1;
    for (int g = 0; g < ii.num_groups; g++) {
      at.g = g;
      int k = 0;
      while (k < ii.max_sfb) {
        at.sfb = k;
        int cb = 0, len = 0;
        if (R) {
          int incr;
          cb = (int)br.get(4);
          if (cb == 12) bad("reserved codebook 12");
          do {
            incr = (int)br.get(bits);
            len += incr;
          } while (incr == esc);
          if (k + len > ii.max_sfb) bad("section beyond max_sfb");
        }
        cb = (int)pass(E_SECT_CB, 4, cb, g);
        if (cb == 12) bad_value(E_SECT_CB, cb);
        at.cb = cb;
        len = (int)cnt(E_SECT_LEN, len, 0, ii.max_sfb - k, g);
        if (!R) {
          w->put(cb, 4);
          int l = len;
          while (l >= esc) {
            w->put(esc, bits);
            l -= esc;
          }
          w->put(l, bits);
        }
        for (int s = k; s < k + len; s++) c.bt[g * ii.max_sfb + s] = (uint8_t)cb;
        k += len;
      }
    }
  }

  // the scalefactors, and each band's as the decoder has it (sf), from the
  // offsets of aacdec's decode_scalefactors()
  void scale_factor_data(ChanData& c) {
    const Ics& ii = c.ii;
    if (!ii.max_sfb) return;
    bool noise_pcm = true;
    int off_sf = c.gg, off_is = 0, off_noise = c.gg - 90;
    for (int g = 0; g < ii.num_groups; g++) {
      at.g = g;
      for (int sfb = 0; sfb < ii.max_sfb; sfb++) {
        int idx = g * ii.max_sfb + sfb, cb = c.bt[idx];
        at.sfb = sfb;
        at.cb = cb;
        if (!cb) {
          c.sf[idx] = 0;
          continue;
        }
        if (cb == 13 && noise_pcm) {
          noise_pcm = false;
          off_noise += (int)u(E_NOISE_NRG, 9) - 256;  // dpcm_noise_nrg of the first noise band
          c.sf[idx] = (int16_t)off_noise;
        } else {
          int d = hv(E_SF, huff_sf, sfb);  // scalefactor / intensity / noise delta
          if (cb == 13)
            off_noise += d, c.sf[idx] = (int16_t)off_noise;
          else if (cb >= 14)
            off_is += d, c.sf[idx] = (int16_t)off_is;
          else
            off_sf += d, c.sf[idx] = (int16_t)off_sf;
        }
      }
    }
  }

  void pulse_data() {
    int np = (int)u(E_PULSE_N, 2);  // number_pulse
    u(E_PULSE_START, 6);            // pulse_start_sfb
    for (int i = 0; i <= np; i++) {
      u(E_PULSE_OFF, 5, i);  // pulse_offset
      u(E_PULSE_AMP, 4, i);  // pulse_amp
    }
  }

  void tns_data(const Ics& ii) {
    const bool is8 = ii.ws == EIGHT_SHORT_SEQUENCE;
    for (int wi = 0; wi < ii.num_windows; wi++) {
      at.win = wi;
      int nf = (int)u(E_TNS_NFILT, is8 ? 1 : 2, wi);
      if (!nf) continue;
      int coef_res = (int)u(E_TNS_RES, 1);
      for (int f = 0; f < nf; f++) {
        u(E_TNS_LEN, is8 ? 4 : 6, f);  // length
        int order = (int)u(E_TNS_ORDER, is8 ? 3 : 5, f);
        if (!order) continue;
        u(E_TNS_DIR, 1, f);  // direction
        int compress = (int)u(E_TNS_COMP, 1, f);
        int clen = coef_res + 3 - compress;
        for (int i = 0; i < order; i++) u(E_TNS_COEF, clen, i);
      }
    }
  }

  void gain_control_data(const Ics& ii) {
    static const uint8_t gain_mode[4][3] = {{1, 0, 5}, {2, 1, 2}, {8, 0, 2}, {2, 1, 5}};
    const uint8_t* m = gain_mode[ii.ws];
    int max_band = (int)u(E_GC_MAXBAND, 2);
    for (int bd = 0; bd < max_band; bd++)
      for (int wd = 0; wd < m[0]; wd++) {
        int an = (int)u(E_GC_ADJN, 3, wd);
        for (int ad = 0; ad < an; ad++) {
          u(E_GC_ALEV, 4, ad);                         // alevcode
          u(E_GC_ALOC, (wd == 0 && m[1]) ? 4 : m[2], ad);  // aloccode
        }
      }
  }

  void dec_tuple(int cb, int* c) {
    const SpecCb& p = spec_cb[cb - 1];
    int idx = huff_spec[cb - 1].decode(br);
    int v[4];
    if (p.dim == 4) {
      v[0] = idx / 27;
      v[1] = idx / 9 % 3;
      v[2] = idx / 3 % 3;
      v[3] = idx % 3;
    } else {
      v[0] = idx / p.range;
      v[1] = idx % p.range;
    }
    if (p.is_signed) {
      for (int j = 0; j < p.dim; j++) c[j] = v[j] - p.off;
      return;
    }
    int sgn[4];
    for (int j = 0; j < p.dim; j++) sgn[j] = v[j] ? (int)br.get1() : 0;
    if (cb == 11)
      for (int j = 0; j < 2; j++)
        if (v[j] == 16) {
          int n = 0;
          while (br.get1())
            if (++n > 8) bad("escape sequence too long");
          v[j] = (1 << (n + 4)) + (int)br.get(n + 4);
        }
    for (int j = 0; j < p.dim; j++) c[j] = sgn[j] ? -v[j] : v[j];
  }

  void enc_tuple(int cb, const int* c) {
    const SpecCb& p = spec_cb[cb - 1];
    int idx = 0;
    for (int j = 0; j < p.dim; j++) {
      int x = c[j], m = x < 0 ? -x : x, a;
      if (p.is_signed) {
        if (m > p.maxval) bad_value(E_SPEC, x);
        a = x + p.off;
      } else if (cb == 11) {
        if (m > 8191) bad_value(E_SPEC, x);
        a = m >= 16 ? 16 : m;
      } else {
        if (m > p.maxval) bad_value(E_SPEC, x);
        a = m;
      }
      idx = idx * p.range + a;
    }
    huff_spec[cb - 1].encode_value(*w, idx);
    if (p.is_signed) return;
    for (int j = 0; j < p.dim; j++)
      if (c[j]) w->put(c[j] < 0, 1);
    if (cb == 11)
      for (int j = 0; j < 2; j++) {
        int m = c[j] < 0 ? -c[j] : c[j];
        if (m < 16) continue;
        int n = 0;
        while (m >= (1 << (n + 5))) n++;
        w->put((1u << (n + 1)) - 2, n + 1);  // n ones and a zero
        w->put(m - (1 << (n + 4)), n + 4);
      }
  }

  void spectral_data(ChanData& c) {
    const Ics& ii = c.ii;
    memset(c.coef, 0, sizeof c.coef);
    bool any = false;
    for (int k = 0; k < ii.num_groups * ii.max_sfb; k++) any |= c.bt[k] && c.bt[k] < 13;
    if (!any) return;
    int win = 0;
    for (int g = 0; g < ii.num_groups; g++) {
      at.g = g;
      for (int sfb = 0; sfb < ii.max_sfb; sfb++) {
        int cb = c.bt[g * ii.max_sfb + sfb];
        if (!cb || cb >= 13) continue;
        int a = ii.swb[sfb], b = ii.swb[sfb + 1];
        const int dim = spec_cb[cb - 1].dim;
        at.sfb = sfb;
        at.cb = cb;
        for (int wi = 0; wi < ii.group_len[g]; wi++) {
          at.win = win + wi;
          int* cc = c.coef + (win + wi) * 128;
          for (int k = a; k < b; k += dim) {
            at.k = k;
            if (R) dec_tuple(cb, cc + k);
            ch->spec(cb, win + wi, k, cc + k, dim);
            if (!R) enc_tuple(cb, cc + k);
          }
        }
      }
      win += ii.group_len[g];
    }
  }

  // individual_channel_stream(common_window, 0) of the channel cd[cur]
  void ics(bool common_window) {
    ChanData& c = cd[cur];
    c.tns = c.pulse = 0;
    note(N_ICS);
    c.gg = (int)u(E_GLOBAL_GAIN, 8);
    if (!common_window) ics_info(c.ii, false);
    section_data(c);
    scale_factor_data(c);
    if (st.er != 2 && u(E_PULSE_PRESENT, 1)) {
      c.pulse = 1;
      pulse_data();
    }
    if (!st.er) {
      if (u(E_TNS_PRESENT, 1)) {
        c.tns = 1;
        tns_data(c.ii);
      }
      if (u(E_GC_PRESENT, 1)) gain_control_data(c.ii);
    } else {  // ER: tns_data() after gain_control_data(), which ER does not use
      c.tns = (int)u(E_TNS_PRESENT, 1);
      if (st.er != 2) {  // gain_control_data_present
        if (!R)
          w->put(0, 1);
        else if (br.get1())
          bad("gain control data in an ER stream");
      }
      if (c.tns) tns_data(c.ii);
    }
    spectral_data(c);
    note(N_ICS_END);
  }

  // ---------- channel elements ----------

  void sce_lfe(int type, int place) {
    size_t s = bitpos();
    int tag = st.er == 2 ? 0 : (int)u(type == TYPE_SCE ? E_SCE_TAG : E_LFE_TAG, 4);  // no element_instance_tag in ELD
    at.type = type, at.tag = st.er == 2 ? place : tag, at.chn = 0;
    cur = 0;
    cd[0].cw = 0, cd[0].ms_present = 0;
    ics(false);
    crcr.push_back(CrcReg{s, bitpos(), 192});
    prev_che_type = type;
    prev_che_tag = tag;
  }

  void cpe(int place) {
    size_t s = bitpos();
    int tag = st.er == 2 ? 0 : (int)u(E_CPE_TAG, 4);
    at.type = TYPE_CPE, at.tag = st.er == 2 ? place : tag, at.chn = 0;
    cur = 0;
    int cw = st.er == 2 ? 1 : (int)u(E_CPE_CW, 1);  // common_window (always in ELD)
    cd[0].cw = cd[1].cw = cw;
    int msp = 0;
    if (cw) {
      ics_info(cd[0].ii, true);
      cd[1].ii = cd[0].ii;
      const Ics& ii = cd[0].ii;
      msp = (int)u(E_MS_PRESENT, 2);  // ms_mask_present
      for (int g = 0; g < ii.num_groups; g++) {
        at.g = g;
        for (int sfb = 0; sfb < ii.max_sfb; sfb++)
          cd[0].ms[g * ii.max_sfb + sfb] = (uint8_t)(msp == 1 ? u(E_MS_USED, 1, sfb) : msp == 2);
      }
    }
    cd[0].ms_present = cd[1].ms_present = msp;
    if (msp) memcpy(cd[1].ms, cd[0].ms, sizeof cd[0].ms);
    ics(cw != 0);
    size_t s2 = bitpos();
    at.chn = 1;
    cur = 1;
    ics(cw != 0);
    size_t e = bitpos();
    crcr.push_back(CrcReg{s, e, 192});
    crcr.push_back(CrcReg{s2, e, 128});
    prev_che_type = TYPE_CPE;
    prev_che_tag = tag;
  }

  void cce() {
    size_t s = bitpos();
    int tag = (int)u(E_CCE_TAG, 4);
    at.type = TYPE_CCE, at.tag = tag, at.chn = 0;
    cur = 0;
    cd[0].cw = 0, cd[0].ms_present = 0;
    int ind_sw = (int)u(E_CCE_INDSW, 1);  // ind_sw_cce_flag
    int nc = (int)u(E_CCE_NCOUP, 3);      // num_coupled_elements
    int num_gain = 0;
    for (int c = 0; c <= nc; c++) {
      num_gain++;
      int is_cpe = (int)u(E_CCE_ISCPE, 1, c);  // cc_target_is_cpe
      u(E_CCE_TTAG, 4, c);                     // cc_target_tag_select
      if (is_cpe && u(E_CCE_LR, 2, c) == 3)    // cc_l, cc_r
        num_gain++;
    }
    u(E_CCE_DOMAIN, 1);  // cc_domain
    u(E_CCE_GSIGN, 1);   // gain_element_sign
    u(E_CCE_GSCALE, 2);  // gain_element_scale
    ics(false);
    const ChanData& c0 = cd[0];
    const Ics& ii = c0.ii;
    for (int c = 1; c < num_gain; c++) {
      int cge = ind_sw ? 1 : (int)u(E_CCE_CGEP, 1, c);  // common_gain_element_present
      if (cge) {
        hv(E_CCE_GAIN, huff_sf, 0);
      } else {
        for (int g = 0; g < ii.num_groups; g++)
          for (int sfb = 0; sfb < ii.max_sfb; sfb++)
            if (c0.bt[g * ii.max_sfb + sfb]) hv(E_CCE_GAIN, huff_sf, 1 + sfb);
      }
    }
    crcr.push_back(CrcReg{s, bitpos(), 192});
    prev_che_type = TYPE_CCE;
    prev_che_tag = tag;
  }

  // ---------- DSE, PCE ----------

  void dse(size_t blk_start) {
    size_t s = bitpos();
    u(E_DSE_TAG, 4);
    int al = 0, n = 0, nb = 0;
    unsigned ab = 0;
    if (R) {
      al = (int)br.get1();
      n = (int)br.get(8);
      if (n == 255) n += (int)br.get(8);
      nb = al ? (int)((blk_start - br.pos) & 7) : 0;
      ab = br.get(nb);
      if ((size_t)n * 8 > br.left()) bad("data stream element overrun");
    }
    al = (int)pass(E_DSE_ALIGNF, 1, al);
    n = (int)cnt(E_DSE_N, n, 0, 510);
    if (!R) {
      w->put(al, 1);
      if (n < 255) {
        w->put(n, 8);
      } else {
        w->put(255, 8);
        w->put(n - 255, 8);
      }
      nb = al ? (int)((blk_start - w->pos) & 7) : 0;
    }
    ab = (unsigned)pass(E_DSE_AB, nb, ab);
    if (!R) w->put(ab, nb);
    bitstr(E_DSE_DATA, (size_t)n * 8);
    crcr.push_back(CrcReg{s, bitpos(), 0});
  }

  // program_config_element(); its byte_alignment() relative to align_ref
  void pce_body(size_t align_ref, int* object_type = nullptr, int* sf_index = nullptr) {
    u(E_PCE_TAG, 4);
    int ot = (int)u(E_PCE_OT, 2);
    int sfi = (int)u(E_PCE_SFI, 4);
    if (object_type) *object_type = ot;
    if (sf_index) *sf_index = sfi;
    int nf = (int)u(E_PCE_NF, 4), ns = (int)u(E_PCE_NS, 4), nb = (int)u(E_PCE_NB, 4), nl = (int)u(E_PCE_NL, 2),
        na = (int)u(E_PCE_NA, 3), nc = (int)u(E_PCE_NC, 4);
    if (u(E_PCE_MONO_P, 1)) u(E_PCE_MONO, 4);
    if (u(E_PCE_STEREO_P, 1)) u(E_PCE_STEREO, 4);
    if (u(E_PCE_MATRIX_P, 1)) {
      u(E_PCE_MATRIX, 2);  // matrix_mixdown_idx
      u(E_PCE_PSEUDO, 1);  // pseudo_surround_enable
    }
    for (int i = 0; i < nf + ns + nb; i++) {
      u(E_PCE_ISCPE, 1, i);  // *_element_is_cpe
      u(E_PCE_ETAG, 4, i);   // *_element_tag_select
    }
    for (int i = 0; i < nl; i++) u(E_PCE_LTAG, 4, i);
    for (int i = 0; i < na; i++) u(E_PCE_ATAG, 4, i);
    for (int i = 0; i < nc; i++) {
      u(E_PCE_CCIND, 1, i);  // cc_element_is_ind_sw
      u(E_PCE_CCTAG, 4, i);  // valid_cc_element_tag_select
    }
    int abits = (int)((align_ref - bitpos()) & 7);
    u(E_PCE_AB, abits);
    int clen = 0;
    if (R) {
      clen = (int)br.get(8);
      if ((size_t)clen * 8 > br.left()) bad("program config element overrun");
    }
    clen = (int)cnt(E_PCE_COMN, clen, 0, 255);
    if (!R) w->put(clen, 8);
    bitstr(E_PCE_COM, (size_t)clen * 8);
  }

  void pce(size_t blk_start) {
    size_t s = bitpos();
    pce_body(blk_start);
    crcr.push_back(CrcReg{s, bitpos(), 0});
  }

  // ---------- FIL: extension payloads ----------

  // dynamic_range_info() after the extension type: the bytes the decoder
  // takes it for
  int drc() {
    int n = 1, nb = 1;
    if (u(E_DRC_PTP, 1)) {  // pce_tag_present
      u(E_DRC_PTAG, 4);     // pce_instance_tag
      u(E_DRC_PRES, 4);     // drc_tag_reserved_bits
      n++;
    }
    if (u(E_DRC_EXP, 1)) {  // excluded_chns_present
      int ne = 0;
      for (;;) {
        u(E_DRC_EXM, 7, ne / 7);  // exclude_mask
        ne += 7;
        if (ne >= 57 || !u(E_DRC_EXA, 1, ne / 7)) break;  // additional_excluded_chns (as aacdec)
      }
      n += ne / 7;
    }
    if (u(E_DRC_BP, 1)) {              // drc_bands_present
      int incr = (int)u(E_DRC_BINC, 4);  // drc_band_incr
      u(E_DRC_INTERP, 4);              // drc_interpolation_scheme
      n++;
      nb += incr;
      for (int i = 0; i < nb; i++) {
        u(E_DRC_BTOP, 8, i);  // drc_band_top
        n++;
      }
    }
    if (u(E_DRC_PRLP, 1)) {  // prog_ref_level_present
      u(E_DRC_PRL, 7);       // prog_ref_level
      u(E_DRC_PRLR, 1);      // prog_ref_level_reserved_bits
      n++;
    }
    for (int i = 0; i < nb; i++) {
      u(E_DRC_SGN, 1, i);  // dyn_rng_sgn
      u(E_DRC_CTL, 7, i);  // dyn_rng_ctl
      n++;
    }
    return n;
  }

  // parsing: an extension payload of type t, its 8 cnt - 4 bits after the
  // type at start, as they are
  void ext_r(int t, int n, size_t start, size_t pend) {
    br.pos = start;
    cnt(E_PAY_KIND, PK_EXT, 0, PK_N - 1);
    pass(E_EXT_TYPE, 4, t);
    cnt(E_EXT_N, n, 1, 1 << 24);
    bitstr(E_EXT, pend - start);
  }

  // One extension_payload with cnt bytes left (parsing: aacdec
  // decode_extension_payload(); writing, what the channel says): its
  // bytes, *all where it takes the rest
  int payload(int cnt_left, bool* all) {
    *all = true;
    if (!R) return payload_w(all);
    size_t ps = br.pos, pend = ps + 8 * (size_t)cnt_left, save_end = br.end;
    int t = (int)br.get(4);
    size_t start = br.pos;
    if (t == 11) {
      size_t mark = ch->mark();
      br.end = pend;
      ErrTrap tr;
      if (!setjmp(tr.jb)) {
        err_push(tr);
        cnt(E_PAY_KIND, PK_DRC, 0, PK_N - 1);
        int n = drc();
        if (n > cnt_left) err_raise_s("dynamic_range_info longer than its fill element");
        err_pop(tr);
        br.end = save_end;
        *all = false;
        return n;
      }
      ch->rollback(mark);
      br.end = save_end;
      ext_r(t, cnt_left, start, pend);
      return cnt_left;
    }
    if (t == 13 || t == 14) {
      sbr_payload_r(t, cnt_left, start, pend);
      return cnt_left;
    }
    // padding: a zero nibble and cnt - 1 equal bytes, as EXT_FILL_DATA
    // (fill_nibble 0000, fill_byte 10100101) or zeros
    bool uniform = br.peek(4) == 0;
    unsigned byte = 0xa5;
    if (t != 1 && cnt_left > 1) {
      BitReader b;
      b.Init(buf, start + 4, pend);
      byte = b.get(8);
    }
    for (size_t p = start + 4; uniform && p < pend; p += 8) {
      BitReader b;
      b.Init(buf, p, pend);
      uniform = b.get(8) == byte;
    }
    if (uniform) {
      cnt(E_PAY_KIND, PK_FILL, 0, PK_N - 1);
      cnt(E_FILL_N, cnt_left, 1, 1 << 24);
      pass(E_FILL_TYPE, 4, t);
      if (t != 1 && cnt_left > 1) pass(E_FILL_BYTE, 8, byte);
      br.pos = pend;
      return cnt_left;
    }
    ext_r(t, cnt_left, start, pend);
    return cnt_left;
  }

  int payload_w(bool* all) {
    int k = (int)cnt(E_PAY_KIND, 0, 0, PK_N - 1);
    switch (k) {
      case PK_DRC:
        w->put(11, 4);
        *all = false;
        return drc();
      case PK_SBR:
      case PK_SBRC:
        return sbr_payload_w(k);
      case PK_FILL: {
        int n = (int)cnt(E_FILL_N, 0, 1, 1 << 24);
        int t = (int)pass(E_FILL_TYPE, 4, 0);
        int byte = t != 1 && n > 1 ? (int)pass(E_FILL_BYTE, 8, 0) : 0xa5;
        w->put(t, 4);
        w->put(0, 4);
        for (int i = 1; i < n; i++) w->put(byte, 8);
        return n;
      }
      default: {  // PK_EXT
        int t = (int)pass(E_EXT_TYPE, 4, 0);
        int n = (int)cnt(E_EXT_N, 0, 1, 1 << 24);
        w->put(t, 4);
        bitstr(E_EXT, 8 * (size_t)n - 4);
        return n;
      }
    }
  }

  void fil() {
    if (R) {
      int count = (int)br.get(4), c = count, escf = 0;
      if (count == 15) {
        c = 15 + (int)br.get(8) - 1;
        if (c < 15) escf = 1;  // 14 written with the escape
      }
      if ((size_t)c * 8 > br.left()) bad("fill element overrun");
      pass(E_FIL_ESC, 1, escf);
      for (;;) {
        int more = c > 0;
        pass(E_FIL_MORE, 1, more);
        if (!more) break;
        bool all;
        c -= payload(c, &all);
        if (all) break;
      }
      return;
    }
    int escf = (int)pass(E_FIL_ESC, 1, 0);
    push_sub();
    int total = 0;
    for (;;) {
      if (!pass(E_FIL_MORE, 1, 0)) break;
      bool all;
      total += payload(0, &all);
      if (all) break;
    }
    BitWriter& s = pop_sub();
    if (escf) {
      if (total != 14) bad("corrupt stream: the escape of a fill element of other than 14 bytes");
      w->put(15, 4);
      w->put(0, 8);
    } else if (total < 15) {
      w->put(total, 4);
    } else {
      if (total > 269) bad("corrupt stream: a fill element longer than 269 bytes");
      w->put(15, 4);
      w->put(total - 14, 8);
    }
    w->append(s);
  }

  // ---------- SBR ----------

  SbrState* sbr_state() {
    int k = prev_che_type == TYPE_SCE ? 0 : prev_che_type == TYPE_CPE ? 1 : prev_che_type == TYPE_CCE ? 2 : -1;
    return k < 0 ? nullptr : &st.sbr[k][prev_che_tag];
  }

  void sbr_header(SbrState& s, SbrSpectrum& sp) {
    s.amp_res = (int)u(E_SBRH_AMP, 1);  // bs_amp_res
    sp.start_freq = (int)u(E_SBRH_START, 4);
    sp.stop_freq = (int)u(E_SBRH_STOP, 4);
    sp.xover_band = (int)u(E_SBRH_XOVER, 3);
    u(E_SBRH_RES, 2);  // bs_reserved
    int e1 = (int)u(E_SBRH_X1, 1), e2 = (int)u(E_SBRH_X2, 1);  // bs_header_extra_1, _2
    if (e1) {
      sp.freq_scale = (int)u(E_SBRH_FSCALE, 2);
      sp.alter_scale = (int)u(E_SBRH_ALTER, 1);
      sp.noise_bands = (int)u(E_SBRH_NOISEB, 2);
    } else {
      sp.freq_scale = 2;
      sp.alter_scale = 1;
      sp.noise_bands = 2;
    }
    if (e2) {
      u(E_SBRH_LIMB, 2);    // bs_limiter_bands
      u(E_SBRH_LIMG, 2);    // bs_limiter_gains
      u(E_SBRH_INTERP, 1);  // bs_interpol_freq
      u(E_SBRH_SMOOTH, 1);  // bs_smoothing_mode
    }
  }

  // the band tables after an optional header (aacdec read_sbr_header() /
  // sbr_reset(): computed again when the spectrum parameters or the rate change)
  void sbr_tables(SbrState& s, bool hdr, const SbrSpectrum& sp) {
    if (hdr) {
      if (!s.have_hdr || sp != s.sp || s.rate != st.sbr_rate) {
        s.sp = sp;
        s.rate = st.sbr_rate;
        s.ok = sbr_compute_bands(st.sbr_rate, sp, s.bands);
      }
      s.have_hdr = 1;
    } else if (s.have_hdr && s.rate != st.sbr_rate) {
      s.rate = st.sbr_rate;
      s.ok = sbr_compute_bands(st.sbr_rate, s.sp, s.bands);
    }
    if (!s.have_hdr || !s.ok) bad("SBR data without a valid SBR header");
  }

  void sbr_grid(SbrState& s, SbrCh& c) {
    int fc = (int)u(E_SGRID_CLASS, 2);  // bs_frame_class
    int ne = 1;
    c.amp_res = s.amp_res;
    switch (fc) {
      case 0: {  // FIXFIX
        ne = 1 << u(E_SGRID_NENV, 2);
        if (ne > 5) bad("too many SBR envelopes");
        int fr = (int)u(E_SGRID_FR, 1);
        for (int e = 0; e < ne; e++) c.freq_res[e] = (uint8_t)fr;
        if (ne == 1) c.amp_res = 0;
        break;
      }
      case 1: {  // FIXVAR
        u(E_SGRID_ABS, 2);              // bs_abs_bord
        int nrt = (int)u(E_SGRID_NREL, 2);  // bs_num_rel
        ne = nrt + 1;
        for (int i = 0; i < nrt; i++) u(E_SGRID_REL, 2, i);  // bs_rel_bord
        u(E_SGRID_PTR, sbr_ceil_log2[ne]);                   // bs_pointer
        for (int i = 0; i < ne; i++) c.freq_res[ne - 1 - i] = (uint8_t)u(E_SGRID_FRB, 1, i);  // last envelope first
        break;
      }
      case 2: {  // VARFIX
        u(E_SGRID_ABS, 2);
        int nrl = (int)u(E_SGRID_NREL, 2);
        ne = nrl + 1;
        for (int i = 0; i < nrl; i++) u(E_SGRID_REL, 2, i);
        u(E_SGRID_PTR, sbr_ceil_log2[ne]);
        for (int i = 0; i < ne; i++) c.freq_res[i] = (uint8_t)u(E_SGRID_FRB, 1, i);
        break;
      }
      default: {  // VARVAR
        u(E_SGRID_ABS, 2, 0);  // bs_abs_bord_0
        u(E_SGRID_ABS, 2, 1);  // bs_abs_bord_1
        int nrl = (int)u(E_SGRID_NREL, 2, 0), nrt = (int)u(E_SGRID_NREL, 2, 1);
        ne = nrl + nrt + 1;
        if (ne > 5) bad("too many SBR envelopes");
        for (int i = 0; i < nrl + nrt; i++) u(E_SGRID_REL, 2, i);  // bs_rel_bord_0, _1
        u(E_SGRID_PTR, sbr_ceil_log2[ne]);
        for (int i = 0; i < ne; i++) c.freq_res[i] = (uint8_t)u(E_SGRID_FRB, 1, i);
        break;
      }
    }
    c.num_env = ne;
    c.num_noise = ne > 1 ? 2 : 1;
  }

  void sbr_dtdf(SbrCh& c) {
    for (int e = 0; e < c.num_env; e++) c.df_env[e] = (uint8_t)u(E_SDF_ENV, 1, e);
    for (int e = 0; e < c.num_noise; e++) c.df_noise[e] = (uint8_t)u(E_SDF_NOISE, 1, e);
  }

  void sbr_invf(const SbrState& s) {
    for (int i = 0; i < s.bands.n_q; i++) u(E_SINVF, 2, i);  // bs_invf_mode
  }

  void sbr_envelope(const SbrState& s, const SbrCh& c, bool bal) {
    const Huff *th, *fh;
    int startbits;
    if (bal) {
      th = &huff_sbr[c.amp_res ? T_HUFFMAN_ENV_BAL_3_0DB : T_HUFFMAN_ENV_BAL_1_5DB];
      fh = &huff_sbr[c.amp_res ? F_HUFFMAN_ENV_BAL_3_0DB : F_HUFFMAN_ENV_BAL_1_5DB];
      startbits = c.amp_res ? 5 : 6;
    } else {
      th = &huff_sbr[c.amp_res ? T_HUFFMAN_ENV_3_0DB : T_HUFFMAN_ENV_1_5DB];
      fh = &huff_sbr[c.amp_res ? F_HUFFMAN_ENV_3_0DB : F_HUFFMAN_ENV_1_5DB];
      startbits = c.amp_res ? 6 : 7;
    }
    at.sc = &c, at.ss = &s, at.bal = bal;
    for (int e = 0; e < c.num_env; e++) {
      int nb = s.bands.n[c.freq_res[e]];
      at.env = e, at.dt = c.df_env[e], at.nb = nb;
      if (c.df_env[e]) {
        for (int j = 0; j < nb; j++) hv(E_SENV, *th, j);
      } else {
        u(E_SENV_START, startbits);  // bs_env_start_value
        for (int j = 1; j < nb; j++) hv(E_SENV, *fh, j);
      }
    }
  }

  void sbr_noise(const SbrState& s, const SbrCh& c, bool bal) {
    const Huff& th = huff_sbr[bal ? T_HUFFMAN_NOISE_BAL_3_0DB : T_HUFFMAN_NOISE_3_0DB];
    const Huff& fh = huff_sbr[bal ? F_HUFFMAN_ENV_BAL_3_0DB : F_HUFFMAN_ENV_3_0DB];
    at.sc = &c, at.ss = &s, at.bal = bal;
    for (int i = 0; i < c.num_noise; i++) {
      at.env = i, at.dt = c.df_noise[i], at.nb = s.bands.n_q;
      if (c.df_noise[i]) {
        for (int j = 0; j < s.bands.n_q; j++) hv(E_SNOISE, th, j);
      } else {
        u(E_SNOISE_START, 5);  // bs_noise_start_value
        for (int j = 1; j < s.bands.n_q; j++) hv(E_SNOISE, fh, j);
      }
    }
  }

  void sbr_harmonic(const SbrState& s) {
    if (u(E_SHARM_F, 1))  // bs_add_harmonic_flag
      for (int i = 0; i < s.bands.n[1]; i++) u(E_SHARM, 1, i);
  }

  // an extension's size in bytes (4 bits, 8 more from 15), written before it
  void write_ext_size(int n) {
    if (n < 15) {
      w->put(n, 4);
    } else {
      if (n > 270) bad("corrupt stream: an SBR or PS extension longer than 270 bytes");
      w->put(15, 4);
      w->put(n - 15, 8);
    }
  }
  int64_t read_ext_size() {
    int n = (int)br.get(4);
    if (n == 15) n += (int)br.get(8);
    return 8 * (int64_t)n;
  }

  void ps_ipdopd(PsHdr& h, int ne) {
    int en = (int)u(E_PS_EIPD, 1);  // enable_ipdopd
    at.ph = &h;
    if (en)
      for (int e = 0; e < ne; e++) {
        at.env = e, at.nb = h.nr_ipdopd_par;
        int dt = (int)u(E_PS_IPD_DT, 1, e);
        at.dt = dt;
        for (int b = 0; b < h.nr_ipdopd_par; b++) hv(E_PS_IPD, huff_ps[dt ? PS_IPD_DT : PS_IPD_DF], b);
        dt = (int)u(E_PS_OPD_DT, 1, e);
        at.dt = dt;
        for (int b = 0; b < h.nr_ipdopd_par; b++) hv(E_PS_OPD, huff_ps[dt ? PS_OPD_DT : PS_OPD_DF], b);
      }
    u(E_PS_RES, 1);  // reserved_ps
  }

  // ps_data() (aacdec ff_ps_read_data())
  void ps_data(PsHdr& h) {
    ps_seen = 1;
    if (u(E_PS_HDR, 1)) {  // enable_ps_header
      h.enable_iid = (int)u(E_PS_EIID, 1);
      if (h.enable_iid) {
        int m = (int)u(E_PS_IIDM, 3);  // iid_mode
        if (m > 5) bad("reserved iid_mode");
        h.nr_iid_par = nr_iidicc_par_tab[m];
        h.iid_quant = m > 2;
        h.nr_ipdopd_par = nr_iidopd_par_tab[m];
      }
      h.enable_icc = (int)u(E_PS_EICC, 1);
      if (h.enable_icc) {
        int m = (int)u(E_PS_ICCM, 3);  // icc_mode
        if (m > 5) bad("reserved icc_mode");
        h.nr_icc_par = nr_iidicc_par_tab[m];
      }
      h.enable_ext = (int)u(E_PS_EEXT, 1);
    }
    at.ph = &h;
    int fc = (int)u(E_PS_FC, 1);                    // frame_class
    int ne = num_env_tab[fc][u(E_PS_NENV, 2)];      // num_env_idx
    if (fc)
      for (int e = 0; e < ne; e++) u(E_PS_BORDER, 5, e);  // border_position
    if (h.enable_iid)
      for (int e = 0; e < ne; e++) {
        at.env = e, at.nb = h.nr_iid_par;
        int dt = (int)u(E_PS_IID_DT, 1, e);
        at.dt = dt;
        const Huff& t = huff_ps[dt ? (h.iid_quant ? PS_IID_DT1 : PS_IID_DT0) : (h.iid_quant ? PS_IID_DF1 : PS_IID_DF0)];
        for (int b = 0; b < h.nr_iid_par; b++) hv(E_PS_IID, t, b);
      }
    if (h.enable_icc)
      for (int e = 0; e < ne; e++) {
        at.env = e, at.nb = h.nr_icc_par;
        int dt = (int)u(E_PS_ICC_DT, 1, e);
        at.dt = dt;
        for (int b = 0; b < h.nr_icc_par; b++) hv(E_PS_ICC, huff_ps[dt ? PS_ICC_DT : PS_ICC_DF], b);
      }
    if (!h.enable_ext) return;
    if (R) {
      int64_t left = read_ext_size();
      if ((size_t)left > br.left()) bad("PS extension overrun");
      for (;;) {
        int more = left > 7;
        pass(E_PSX_MORE, 1, more);
        if (!more) break;
        int id = (int)u(E_PSX_ID, 2);  // ps_extension_id
        left -= 2;
        if (id == 0) {
          size_t p0 = br.pos;
          ps_ipdopd(h, ne);
          left -= (int64_t)(br.pos - p0);
        }
      }
      if (left < 0) bad("PS extension overflow");
      u(E_PSX_FILL, (int)left);
    } else {
      push_sub();
      for (;;) {
        if (!pass(E_PSX_MORE, 1, 0)) break;
        if (u(E_PSX_ID, 2) == 0) ps_ipdopd(h, ne);
      }
      u(E_PSX_FILL, (int)((8 - w->pos % 8) % 8));
      BitWriter& s = pop_sub();
      write_ext_size((int)(s.pos / 8));
      w->append(s);
    }
  }

  void sbr_extension(SbrState& s) {
    if (R) {
      int64_t left = read_ext_size();
      if ((size_t)left > br.left()) bad("SBR extension overrun");
      for (;;) {
        int more = left > 7;
        pass(E_SBRX_MORE, 1, more);
        if (!more) break;
        int id = (int)u(E_SBRX_ID, 2);  // bs_extension_id
        left -= 2;
        if (id == 2) {  // EXTENSION_ID_PS
          size_t p0 = br.pos, save_end = br.end;
          br.end = br.pos + left;
          ps_data(s.ps);
          br.end = save_end;
          left -= (int64_t)(br.pos - p0);
        } else {
          cnt(E_SBRXE_N, left, 0, 8 * 270);
          bitstr(E_SBRXE, (size_t)left);
          left = 0;
          break;
        }
      }
      u(E_SBRX_FILL, (int)left);
    } else {
      push_sub();
      for (;;) {
        if (!pass(E_SBRX_MORE, 1, 0)) break;
        if (u(E_SBRX_ID, 2) == 2) {
          ps_data(s.ps);
        } else {
          size_t n = (size_t)cnt(E_SBRXE_N, 0, 0, 8 * 270);
          bitstr(E_SBRXE, n);
          if (w->pos % 8) bad("corrupt stream: an SBR extension that does not end on a byte");
          break;
        }
      }
      u(E_SBRX_FILL, (int)((8 - w->pos % 8) % 8));
      BitWriter& b = pop_sub();
      write_ext_size((int)(b.pos / 8));
      w->append(b);
    }
  }

  // sbr_data() for the element type of the channel element before it
  void sbr_data(SbrState& s) {
    SbrCh c0, c1;
    memset((void*)&c0, 0, sizeof c0);
    memset((void*)&c1, 0, sizeof c1);
    at.ss = &s;
    at.bal = 0;
    if (prev_che_type != TYPE_CPE) {
      at.chn = 0;
      if (u(E_SBRD_EXTRA, 1)) u(E_SBRD_RES, 4);  // bs_data_extra, bs_reserved
      sbr_grid(s, c0);
      sbr_dtdf(c0);
      sbr_invf(s);
      sbr_envelope(s, c0, false);
      sbr_noise(s, c0, false);
      sbr_harmonic(s);
    } else {
      at.chn = 0;
      if (u(E_SBRD_EXTRA, 1)) u(E_SBRD_RES, 8);  // bs_data_extra, bs_reserved x2
      int coupling = (int)u(E_SBRD_COUP, 1);   // bs_coupling
      if (coupling) {
        sbr_grid(s, c0);
        c1 = c0;
        sbr_dtdf(c0);
        at.chn = 1;
        sbr_dtdf(c1);
        at.chn = 0;
        sbr_invf(s);
        sbr_envelope(s, c0, false);
        sbr_noise(s, c0, false);
        at.chn = 1;
        sbr_envelope(s, c1, true);
        sbr_noise(s, c1, true);
      } else {
        sbr_grid(s, c0);
        at.chn = 1;
        sbr_grid(s, c1);
        at.chn = 0;
        sbr_dtdf(c0);
        at.chn = 1;
        sbr_dtdf(c1);
        at.chn = 0;
        sbr_invf(s);
        at.chn = 1;
        sbr_invf(s);
        at.chn = 0;
        sbr_envelope(s, c0, false);
        at.chn = 1;
        sbr_envelope(s, c1, false);
        at.chn = 0;
        sbr_noise(s, c0, false);
        at.chn = 1;
        sbr_noise(s, c1, false);
      }
      at.chn = 0;
      sbr_harmonic(s);
      at.chn = 1;
      sbr_harmonic(s);
    }
    at.chn = 0;
    if (u(E_SBRX_P, 1)) sbr_extension(s);  // bs_extended_data
    at.sc = nullptr;
  }

  // parsing: sbr_extension_data() in [start, pend) after the extension
  // type; kept as bits where it does not parse
  void sbr_payload_r(int t, int n, size_t start, size_t pend) {
    SbrState* ps = sbr_state();
    if (!ps) {  // no SCE, CPE or CCE before it: not decodable
      ext_r(t, n, start, pend);
      return;
    }
    SbrState s = *ps;
    const size_t mark = ch->mark(), save_end = br.end;
    const int k = (int)(ps - &st.sbr[0][0]) / 16, tag = prev_che_tag;
    const bool had_hdr = s.have_hdr != 0;
    volatile bool hdr = false;
    br.end = pend;
    ErrTrap tr;
    if (!setjmp(tr.jb)) {
      err_push(tr);
      cnt(E_PAY_KIND, t == 14 ? PK_SBRC : PK_SBR, 0, PK_N - 1);
      note(N_SBR);
      unsigned crc = t == 14 ? br.get(10) : 0;  // bs_sbr_crc_bits
      SbrSpectrum sp;
      memset((void*)&sp, 0, sizeof sp);
      hdr = u(E_SBR_HDR, 1) != 0;  // bs_header_flag
      if (hdr) sbr_header(s, sp);
      sbr_tables(s, hdr, sp);
      sbr_data(s);
      size_t pad = pend - br.pos;
      if (pad >= 8 || (pad && br.peek((int)pad))) {
        cnt(E_SBRPAD_N, (int64_t)pad, 1, 8 * 270);
        bitstr(E_SBRPAD, pad);
      } else {
        cnt(E_SBRPAD_N, 0, 0, 8 * 270);
      }
      br.pos = pend;
      if (t == 14) {
        int ok = crc10_bits(buf, start + 10, pend - start - 10) == crc;
        pass(E_SBR_CRC_OK, 1, ok);
        if (!ok) pass(E_SBR_CRC, 10, crc);
      }
      note(N_SBR_END);
      err_pop(tr);
      br.end = save_end;
      *ps = s;
      sbr_seen = 1;
      if (!had_hdr && !sbr_found[k][tag]) {
        sbr_found[k][tag] = 1;
        sbr_first[k][tag] = s;
      }
      return;
    }
    if (!had_hdr && !hdr) sbr_need[k][tag]++;
    ch->rollback(mark);
    br.end = save_end;
    ext_r(t, n, start, pend);
  }

  // writing: an SBR payload with its extension type; its bytes
  int sbr_payload_w(int kind) {
    int t = kind == PK_SBRC ? 14 : 13;
    SbrState* ps = sbr_state();
    if (!ps) bad("corrupt stream: SBR data not after an SCE, CPE or CCE");
    SbrState s = *ps;
    note(N_SBR);
    push_sub();
    w->put(t, 4);
    if (t == 14) w->put(0, 10);
    SbrSpectrum sp;
    memset((void*)&sp, 0, sizeof sp);
    bool hdr = u(E_SBR_HDR, 1) != 0;
    if (hdr) sbr_header(s, sp);
    sbr_tables(s, hdr, sp);
    sbr_data(s);
    size_t pn = (size_t)cnt(E_SBRPAD_N, 0, 0, 8 * 270);
    if (pn)
      bitstr(E_SBRPAD, pn);
    else
      w->put(0, (int)((8 - w->pos % 8) % 8));
    if (w->pos % 8) bad("corrupt stream: an SBR payload that does not end on a byte");
    if (t == 14) {
      unsigned crc = pass(E_SBR_CRC_OK, 1, 0) ? crc10_bits(w->b.p, 14, w->pos - 14) : (unsigned)pass(E_SBR_CRC, 10, 0);
      w->patch(4, crc, 10);
    }
    note(N_SBR_END);
    BitWriter& b = pop_sub();
    w->append(b);
    *ps = s;
    sbr_seen = 1;
    return (int)(b.pos / 8);
  }

  // an SBR header assumed for the payloads of an element that come before
  // its first header (the compressor's probe finds it, units.inc)
  static void sbr_assume(SbrState& s, int amp_res, const SbrSpectrum& sp) {
    memset((void*)&s, 0, sizeof s);
    s.have_hdr = 1;
    s.amp_res = amp_res;
    s.sp = sp;  // (rate 0: the tables are computed when first used)
  }

  // ---------- raw_data_block ----------

  // true when the element was END
  bool element(int t, size_t blk_start, bool align_end, int place) {
    switch (t) {
      case TYPE_SCE: sce_lfe(TYPE_SCE, place); break;
      case TYPE_LFE: sce_lfe(TYPE_LFE, place); break;
      case TYPE_CPE: cpe(place); break;
      case TYPE_CCE: cce(); break;
      case TYPE_DSE: dse(blk_start); break;
      case TYPE_PCE: pce(blk_start); break;
      case TYPE_FIL: fil(); break;
      default:
        if (align_end) u(E_END_ALIGN, (int)((blk_start - bitpos()) & 7));  // byte_alignment()
        return true;
    }
    return false;
  }

  // the rest of the frame from bit p, as its bits (parsing)
  void rest_r(size_t p, size_t end) {
    rests++;
    br.pos = p;
    cnt(E_REST_N, (int64_t)(end - p), 0, (int64_t)1 << 40);
    bitstr(E_REST, end - p);
  }
  void rest_w() {
    size_t n = (size_t)cnt(E_REST_N, 0, 0, (int64_t)fw.cap);
    bitstr(E_REST, n);
  }

  // An ER access unit (aacdec aac_decode_er_frame()): the channel elements
  // of the channel configuration, without element ids and END, then
  // extension payloads, each taking the whole bytes left, while 8 or more
  // bits are left (fdk-aac aacDecoder_ParseExplicitMpsAndSbr(); aacdec
  // ignores the rest of the frame). False when the rest is a rest.
  bool er_frame(size_t frame_end) {
    prev_che_type = -1;
    prev_che_tag = 0;
    crcr.clear();
    note(N_BLOCK);
    int place = 0;
    for (const char* p = er_layout[st.chan_config]; *p; p++, place++) {
      int type = *p == 'S' ? TYPE_SCE : *p == 'C' ? TYPE_CPE : TYPE_LFE;
      if (R) {
        const size_t es = br.pos, mark = ch->mark();
        ErrTrap tr;
        if (!setjmp(tr.jb)) {
          err_push(tr);
          pass(E_ER_REST, 1, 0);
          element(type, 0, false, place);
          err_pop(tr);
          continue;
        }
        ch->rollback(mark);
        br.end = frame_end;
        pass(E_ER_REST, 1, 1);
        rest_r(es, frame_end);
        return false;
      }
      if (pass(E_ER_REST, 1, 0)) {
        rest_w();
        return false;
      }
      element(type, 0, false, place);
    }
    for (;;) {
      int more = R && frame_end - br.pos > 7;
      more = (int)pass(E_ER_MORE, 1, more);
      if (!more) break;
      bool all;
      payload(R ? (int)((frame_end - br.pos) >> 3) : 0, &all);
      if (all) break;
    }
    return true;
  }

  // false when the frame was finished by a rest
  bool raw_data_block(size_t frame_end, bool align_end) {
    const size_t blk_start = bitpos();
    prev_che_type = -1;
    prev_che_tag = 0;
    crcr.clear();
    note(N_BLOCK);
    for (int place = 0;; place++) {
      if (R) {
        const size_t es = br.pos, mark = ch->mark(), cmark = crcr.size();
        const int pt = prev_che_type, pg = prev_che_tag;
        ErrTrap tr;
        if (!setjmp(tr.jb)) {
          err_push(tr);
          int t = (int)br.get(3);
          cnt(E_ELEM, t, 0, TYPE_REST);
          bool end = element(t, blk_start, align_end, place);
          err_pop(tr);
          if (end) return true;
          continue;
        }
        ch->rollback(mark);
        crcr.n = cmark;
        prev_che_type = pt;
        prev_che_tag = pg;
        br.end = frame_end;
        cnt(E_ELEM, TYPE_REST, 0, TYPE_REST);
        rest_r(es, frame_end);
        return false;
      }
      int t = (int)cnt(E_ELEM, 0, 0, TYPE_REST);
      if (t == TYPE_REST) {
        rest_w();
        return false;
      }
      w->put(t, 3);
      if (element(t, blk_start, align_end, place)) return true;
    }
  }
};

}  // namespace aacpack
