// The per-NAL step of the compressor and the decompressor (cmstream.inc):
// the context of a stream (its parameter sets, its macroblocks, x264's
// padding), the NAL unit's syntax through a channel, parsed or written by
// the same walk (code_nal), how its RBSP ends (Trailer), the padding
// detector, and the statistics of -v.

namespace h264p {

// the NAL unit types the walk knows (the others are stored verbatim)
bool parsed_type(int type) { return (type >= 5 && type <= 12) || type == 1; }

// Why a NAL unit is stored verbatim (said with -v), in a char[kWhy]
const int kWhy = 320;
void set_why(char* why, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(why, kWhy, fmt, ap);
  va_end(ap);
}

// What c did with the NAL units of a stream
struct Stats {
  long nals, parsed, raw, trail, pad;
  // why NAL units were stored verbatim: each reason once, with how many it
  // was, the first kReasons of them (a reason can have numbers in it: a
  // stream could make any number of them), the rest counted in others
  static const size_t kReasons = 64;
  struct Reason {
    char why[kWhy];
    long n;
  };
  Buf<Reason> reasons;
  long others;

  void Init() {
    nals = parsed = raw = trail = pad = 0;
    reasons.Init();
    others = 0;
  }
  void Quit() { reasons.Quit(); }

  // a NAL unit stored verbatim, and why
  void verbatim(const char* why) {
    raw++;
    for (Reason& r : reasons)
      if (!strncmp(r.why, why, kWhy - 1)) {
        r.n++;
        return;
      }
    if (reasons.size() == kReasons) {
      others++;
      return;
    }
    Reason r;
    set_why(r.why, "%s", why);
    r.n = 1;
    reasons.push_back(r);
  }
  // and one with a header, the last: false if that ends the call (-s, where
  // the walk knows its type; said, and with -v said anyway)
  bool verbatim(int type, const char* why, bool verbose, bool strict, Progress* meter) {
    verbatim(why);
    if (!parsed_type(type)) return true;
    if (verbose) say(meter, "h264p: NAL unit %ld (type %d) stored verbatim: %s\n", nals - 1, type, why);
    if (!strict) return true;
    say_error(meter, "h264p: NAL unit %ld (type %d) cannot be converted: %s\n", nals - 1, type, why);
    return false;
  }
  // -v's report at the end: done, what was done to the NAL units not stored
  // verbatim; pads, what pad counts (the reasons in order of their text)
  void report(const char* done, const char* pads, Progress* meter) {
    say(meter, "%ld NAL units: %ld %s (%ld with irregular trailing bits), %ld stored verbatim\n", nals, parsed, done,
        trail, raw);
    if (pad) say(meter, "%ld %s\n", pad, pads);
    if (!reasons.empty()) qsort(reasons.data(), reasons.size(), sizeof(Reason), by_why);
    for (const Reason& r : reasons) say(meter, "  verbatim: %ld x %s\n", r.n, r.why);
    if (others) say(meter, "  verbatim: %ld x for other reasons\n", others);
  }
  static int by_why(const void* a, const void* b) { return strcmp(((const Reason*)a)->why, ((const Reason*)b)->why); }
};

// x264 ends its CABAC data in its own way: after the rbsp_stop_one_bit and
// the alignment zero bits, it sets the last bit of the byte to bit
// (frame & 31) of 0x35a4e4f5, frame being the number of the picture in
// coding order from the start of the encoding, and it does the same before
// the samples of I_PCM macroblocks. The compressor finds that out
// (PadDetector) and codes where it starts or changes phase (set(): the
// next slice is in frame N); both sides then expect these bits.
struct X264Padding {
  bool on;
  bool hold;      // set(): the next slice is in frame `frame`
  int64_t frame;  // the frame of the last slice

  void Init() {
    on = hold = false;
    frame = 0;
  }
  // x264's padding bit of frame f; that of the next slice, which starts a
  // picture or not
  static int bit_of(int64_t f) { return 0x35a4e4f5u >> ((uint64_t)f & 31) & 1; }
  int bit(bool newpic) const { return on ? bit_of(frame + (newpic && !hold)) : 0; }
  void set(int64_t n) { on = true, hold = true, frame = n; }
  void next_slice(bool newpic) {
    frame += newpic && !hold;
    hold = false;
  }
};

// A slice with first_mb_in_slice 0, whose first bit is 1, starts a picture.
bool starts_picture(int type, const uint8_t* payload, size_t n) {
  return (type == 1 || type == 5) && n && (payload[0] & 0x80);
}

// State shared by all NAL units of a stream.
struct Context {
  ParamSets ps;
  MbStore mbs;
  X264Padding pad;
  SliceHeader sh;                // the slice's (code_nal())
  int slice_kind, slice_mbs;     // the last slice's picture (pic_kind, pic_mbs)
  ValueLog plog;                 // a verbatim parameter set's values (update_params())

  void Init() {
    ps.Init();
    mbs.Init();
    pad.Init();
    sh.Init();
    slice_kind = -1, slice_mbs = 0;
    plog.Init();
  }
  void Quit() {
    ps.Quit();
    mbs.Quit();
    plog.Quit();
  }

  // a NAL unit done, its payload (after the header byte) as in the stream:
  // past a slice, x264's padding moves on
  void done(int type, const uint8_t* payload, size_t n) {
    if (type == 1 || type == 5) pad.next_slice(starts_picture(type, payload, n));
  }
};

// Decides when compress writes x264_padding rows, from the padding bits it
// observes: keeps the current hypothesis (no padding, or x264's padding in
// one of the 32 phases of the frame number relative to the picture count)
// while it predicts the observed bits. Otherwise it switches to x264's
// padding starting with frame 0 at the last SPS or x264 SEI, or to no
// padding, if that predicted all bits since, else to the hypothesis that
// predicted the longest run of the latest bits.
struct PadDetector {
  void Init() {
    pic_ = -1;
    n_ = 0;
    for (int64_t& l : last_) l = -1;
    cur_ = kOff;
    pref_ = 0;
    since_ = 0;
  }
  // a slice NAL unit, which starts a picture or not
  void slice(bool newpic) { pic_ += newpic; }
  // an SPS or x264's version SEI: the next picture may start an encoding
  void restart() {
    pref_ = (int)(-(uint64_t)(pic_ + 1) & 31);
    since_ = n_;
  }
  // the padding bit observed in the current slice; true when the
  // hypothesis changes
  bool observe(int bit) {
    for (int p = 0; p < 32; p++)
      if (predict(p) != bit) last_[p] = n_;
    if (bit) last_[kOff] = n_;
    if (last_[cur_] != n_++) return false;
    int best = pref_;
    if (last_[pref_] >= since_) {
      if (last_[kOff] < since_) {
        best = kOff;
      } else {
        if (last_[kOff] < last_[best]) best = kOff;
        for (int p = 0; p < 32; p++)
          if (last_[p] < last_[best]) best = p;
      }
    }
    cur_ = best;
    return true;
  }
  // the value of the x264_padding row for the current slice, -1 for none
  int64_t frame() const {
    if (cur_ == kOff) return -1;
    int64_t f = pic_ + cur_;
    return f < 0 ? f + 32 : f;
  }

  static const int kOff = 32;
  int predict(int p) const { return X264Padding::bit_of((int64_t)pic_ + p); }

  int64_t pic_;        // pictures so far, the first one being 0
  int64_t n_;          // observations so far
  int64_t last_[33];   // the last observation each hypothesis mispredicted
  int cur_;
  int pref_;           // the phase that makes the picture after the restart frame 0
  int64_t since_;      // the first observation after the restart
};

// x264's version SEI: user_data_unregistered with its UUID
bool is_x264_sei(const uint8_t* p, size_t n) {
  static const uint8_t uuid[16] = {0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
                                   0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef};
  for (size_t i = 0; i + 16 <= n; i++)
    if (!memcmp(p + i, uuid, 16)) return true;
  return false;
}

// A NAL unit's header byte, and its payload after it (t and c)
struct NalHead {
  int ref_idc, type;
  const uint8_t* payload;
  size_t len;
  bool slice, newpic;  // a slice, and one that starts a picture
};
// whether a NAL unit has no header to read: empty, or forbidden_zero_bit set
bool headless(const Buf<uint8_t>& nal) { return nal.empty() || (nal[0] & 0x80); }
// The header of a NAL unit that has one, the padding detector told of it
NalHead nal_head(const Buf<uint8_t>& nal, PadDetector& det) {
  NalHead h;
  h.ref_idc = nal[0] >> 5 & 3, h.type = nal[0] & 31;
  h.payload = nal.data() + 1, h.len = nal.size() - 1;
  h.slice = h.type == 1 || h.type == 5;
  h.newpic = starts_picture(h.type, h.payload, h.len);
  if (h.slice) det.slice(h.newpic);
  if (h.type == 7 || (h.type == 6 && is_x264_sei(h.payload, h.len))) det.restart();
  return h;
}
// After a NAL unit's conversion, the padding bit it observed (obs, -1 if
// none): whether that changes the detector's hypothesis, x264's padding
// (pad) then set to the new one, to convert the NAL unit again with
bool repad(PadDetector& det, int obs, X264Padding& pad) {
  if (obs < 0 || !det.observe(obs)) return false;
  int64_t frame = det.frame();
  if (frame < 0)
    pad.on = false;
  else
    pad.set(frame);
  return true;
}

// The syntax of a NAL unit's RBSP through a channel, parsed from br or
// written to bw (with the rbsp_stop_one_bit), either way by the same walk
// (Syn's direction), a slice's bins as bp says; parameter sets are stored
// in the context. Returns x264's padding bit for the end of CABAC slice
// data, else -1. What cannot be parsed or written raises an error
// (err.inc), as the channel puts it.
int code_nal(int type, int ref_idc, Context& ctx, Chan& ch, BitReader* br, BitWriter* bw, const BinPolicy& bp) {
  Syn s;
  s.Init(br, bw, &ch);
  switch (type) {
    case 7:
      ctx.ps.next_sps.Init();
      code_sps(s, ctx.ps.next_sps);
      ctx.ps.define_sps();
      break;
    case 8:
      ctx.ps.next_pps.reset();
      code_pps(s, ctx.ps.next_pps, ctx.ps);
      ctx.ps.define_pps();
      break;
    case 6:
      code_sei(s);
      break;
    case 9:
      code_aud(s);
      break;
    case 12:
      code_filler(s);
      break;
    case 10:
    case 11:  // (empty: no stop bit either)
      if (br && br->size()) err_raise("end of sequence/stream NAL unit with data");
      return -1;
    case 1:
    case 5: {
      SliceHeader& h = ctx.sh;
      h.Init();
      h.nal_unit_type = type;
      h.nal_ref_idc = ref_idc;
      code_slice_header(s, h, ctx.ps);
      ctx.slice_kind = pic_kind(h), ctx.slice_mbs = pic_mbs(h);
      h.pad = ctx.pad.bit(h.first_mb == 0);
      if (br)
        read_slice_data(h, ctx.mbs, *br, ch, bp);
      else
        write_slice_data(h, ctx.mbs, ch, *bw, bp);  // with the stop bit
      return h.pps->entropy_coding_mode_flag ? h.pad : -1;
    }
    default:  // (writing, only the types parsed: the others are stored verbatim)
      if (br) err_raise("unsupported NAL unit type");
      err_raise("internal: NAL unit type %d written", type);
  }
  if (bw) bw->bit(1);  // rbsp_stop_one_bit
  return -1;
}
int read_nal(int type, int ref_idc, const Buf<uint8_t>& rbsp, Context& ctx, Chan& ch,
             const BinPolicy& bp = BinPolicy()) {
  BitReader br;
  br.Init(rbsp.data(), rbsp.size());
  return code_nal(type, ref_idc, ctx, ch, &br, nullptr, bp);
}
int write_syntax(int type, int ref_idc, Context& ctx, Chan& ch, BitWriter& bw, const BinPolicy& bp = BinPolicy()) {
  return code_nal(type, ref_idc, ctx, ch, nullptr, &bw, bp);
}

// The alignment zero bits, the last one being x264's padding bit if pad is 1.
void align(BitWriter& bw, int pad) { bw.put(pad > 0, (8 - (int)(bw.pos() & 7)) & 7); }

// How the RBSP of a NAL unit ends after the rbsp_stop_one_bit, when not with
// the expected alignment bits (e.g. from yet another CABAC flushing
// procedure): the last `drop` bits written are replaced by `bits`. Then come
// the cabac_zero_words.
struct Trailer {
  bool irregular;
  int64_t drop;
  Buf<uint8_t> bits;  // 0 or 1 each
  int64_t czw;

  void Init() {
    bits.Init();
    reset();
  }
  void Quit() { bits.Quit(); }
  // the expected end (the room for bits stays)
  void reset() {
    irregular = false;
    drop = 0;
    bits.clear();
    czw = 0;
  }
};

// The rest of the RBSP after the rbsp_stop_one_bit, as the trailer says.
void apply_trailer(const Trailer& t, BitWriter& bw, int pad) {
  if (t.irregular) {
    bw.truncate(bw.pos() - (size_t)t.drop);
    for (uint8_t b : t.bits) bw.bit(b);
  } else {
    align(bw, pad);
  }
  bw.append(2 * (size_t)t.czw, 0);
}

int get_bit(const Buf<uint8_t>& v, size_t i) { return v[i >> 3] >> (7 - (i & 7)) & 1; }


// Compares the RBSP written up to and including the stop bit (bw; pad as
// write_syntax returned it) with the original one, and finds the trailer
// that makes them equal. False when there is none (why). obs is the bit
// where x264 puts its padding bit in a CABAC slice, -1 if there is none.
// (o: room for what it writes)
bool find_trailer(const BitWriter& bw, int pad, const Buf<uint8_t>& rbsp, Trailer& t, char* why, int& obs,
                  BitWriter& o) {
  t.reset();
  size_t pbits = bw.pos();
  // the last bit of the byte with the stop bit, unless it is the stop bit
  obs = -1;
  if (pad >= 0 && (pbits & 7) && (pbits - 1) / 8 < rbsp.size()) obs = rbsp[(pbits - 1) / 8] & 1;
  o.copy(bw);
  align(o, pad);
  if (o.data().same(rbsp)) return true;
  // the end differs: cabac_zero_words, or trailing bits that are not the
  // expected ones
  size_t zeros = 0;
  while (zeros < rbsp.size() && rbsp[rbsp.size() - 1 - zeros] == 0) zeros++;
  t.czw = (int64_t)(zeros / 2);
  if (t.czw > (1 << 26)) {
    set_why(why, "too many cabac_zero_words");
    return false;
  }
  size_t rn = rbsp.size() - 2 * (size_t)t.czw;  // (the RBSP without them)
  if (!o.data().same(rbsp.data(), rn)) {
    // compare our bits up to and including the stop bit
    size_t rbits = rn * 8, d = 0;
    while (d < pbits && d < rbits && bw.at(d) == get_bit(rbsp, d)) d++;
    if (pbits - d > 64 || rbits - d > 4096) {
      set_why(why, "re-encoding differs at bit %zu of %zu", d, rbits);
      return false;
    }
    t.irregular = true;
    t.drop = (int64_t)(pbits - d);
    for (size_t i = d; i < rbits; i++) t.bits.push_back((uint8_t)get_bit(rbsp, i));
  }
  o.copy(bw);
  apply_trailer(t, o, pad);
  if (!o.data().same(rbsp)) {
    set_why(why, "re-encoding differs");
    return false;
  }
  return true;
}

// SPS and PPS stored verbatim still define parameter sets (where they
// parse: an error is of no matter)
void update_params(int type, const Buf<uint8_t>& rbsp, Context& ctx) {
  if (type != 7 && type != 8) return;
  ctx.plog.clear();
  LogChan ch;
  ch.Init(ctx.plog, false);
  ErrTrap t;
  if (!setjmp(t.jb)) {
    err_push(t);
    read_nal(type, 0, rbsp, ctx, ch);
    err_pop(t);
  }
}

// The RBSP of a NAL unit's payload, to parse (rbsp), or false when the NAL
// unit is to be stored verbatim (why): a type the walk does not know, or
// emulation prevention that escape() would not write (into esc; an SPS or
// PPS still defines its parameter set)
bool nal_rbsp(const NalHead& h, Context& ctx, Buf<uint8_t>& rbsp, Buf<uint8_t>& esc, char* why) {
  if (!parsed_type(h.type)) {
    set_why(why, "NAL unit type %d", h.type);
    return false;
  }
  unescape(h.payload, h.len, rbsp);
  esc.clear();
  escape(rbsp.data(), rbsp.size(), esc);
  if (!esc.same(h.payload, h.len)) {
    set_why(why, "irregular emulation prevention");
    update_params(h.type, rbsp, ctx);
    return false;
  }
  return true;
}

// A NAL unit written back (into bw) from the values its parse gave the
// channel ch, which hands them out again (at_end() when it has no more),
// and the trailer that makes it its RBSP; false when there is none, or the
// writing fails (why). obs: see find_trailer (o, its room).
template <class Ch>
bool write_back(const NalHead& h, Context& ctx, Ch& ch, const Buf<uint8_t>& rbsp, Trailer& t, char* why, int& obs,
                BitWriter& bw, BitWriter& o) {
  bw.clear();
  ErrTrap e;
  if (setjmp(e.jb)) {
    set_why(why, "re-encoding failed: %s", e.why);
    return false;
  }
  err_push(e);
  int pad = write_syntax(h.type, h.ref_idc, ctx, ch, bw);
  if (!ch.at_end()) err_raise("values left over");
  err_pop(e);
  return find_trailer(bw, pad, rbsp, t, why, obs, o);
}

// The output of d: n zero bytes appended to buf, spill() after each
// MB of them (it hands buf on once it is full)
template <class F>
void put_zeros(Buf<uint8_t>& buf, uint64_t n, F spill) {
  while (n) {
    size_t k = (size_t)Min<uint64_t>(n, 1 << 20);
    buf.append(k, 0);
    n -= k;
    spill();
  }
}
// the zero bytes before a NAL unit and its start code
template <class F>
void put_start(Buf<uint8_t>& buf, uint64_t zeros, F spill) {
  static const uint8_t sc[3] = {0, 0, 1};
  put_zeros(buf, zeros, spill);
  buf.append(sc, 3);
}
// a NAL unit's payload stored verbatim (an SPS or PPS still defines its
// parameter set: rbsp, room for it)
void put_verbatim(Buf<uint8_t>& buf, int type, const Buf<uint8_t>& b, Context& ctx, Buf<uint8_t>& rbsp) {
  buf.append(b.data(), b.size());
  if (type != 7 && type != 8) return;
  unescape(b.data(), b.size(), rbsp);
  update_params(type, rbsp, ctx);
}

}  // namespace h264p
