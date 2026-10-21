// The per-NAL step of the compressor and the decompressor (cmstream.inc):
// the context of a stream (its parameter sets, its macroblocks, x264's
// padding), the NAL unit's syntax through a channel, parsed or written by
// the same walk (code_nal), how its RBSP ends (Trailer), the padding
// detector, and the statistics of -v.

namespace h264p {

// the NAL unit types the walk knows (the others are stored verbatim)
bool parsed_type(int type) { return (type >= 5 && type <= 12) || type == 1; }

// What c did with the NAL units of a stream
struct Stats {
  long nals = 0, parsed = 0, raw = 0, trail = 0, pad = 0;
  std::map<std::string, long> reasons;

  // a NAL unit stored verbatim, and why
  void verbatim(const std::string& why) {
    raw++;
    reasons[why]++;
  }
  // and one with a header, the last: false if that ends the call (-s, where
  // the walk knows its type; said, and with -v said anyway)
  bool verbatim(int type, const std::string& why, bool verbose, bool strict, Progress* meter) {
    verbatim(why);
    if (!parsed_type(type)) return true;
    if (verbose) say(meter, "h264p: NAL unit %ld (type %d) stored verbatim: %s\n", nals - 1, type, why.c_str());
    if (!strict) return true;
    say_error(meter, "h264p: NAL unit %ld (type %d) cannot be converted: %s\n", nals - 1, type, why.c_str());
    return false;
  }
  // -v's report at the end: done, what was done to the NAL units not stored
  // verbatim; pads, what pad counts
  void report(const char* done, const char* pads, Progress* meter) const {
    say(meter, "%ld NAL units: %ld %s (%ld with irregular trailing bits), %ld stored verbatim\n", nals, parsed, done,
        trail, raw);
    if (pad) say(meter, "%ld %s\n", pad, pads);
    for (auto& r : reasons) say(meter, "  verbatim: %ld x %s\n", r.second, r.first.c_str());
  }
};

// x264 ends its CABAC data in its own way: after the rbsp_stop_one_bit and
// the alignment zero bits, it sets the last bit of the byte to bit
// (frame & 31) of 0x35a4e4f5, frame being the number of the picture in
// coding order from the start of the encoding, and it does the same before
// the samples of I_PCM macroblocks. The compressor finds that out
// (PadDetector) and codes where it starts or changes phase (set(): the
// next slice is in frame N); both sides then expect these bits.
struct X264Padding {
  bool on = false;
  bool hold = false;  // set(): the next slice is in frame `frame`
  int64_t frame = 0;  // the frame of the last slice

  // the padding bit of the next slice, which starts a picture or not
  int bit(bool newpic) const {
    if (!on) return 0;
    uint64_t f = (uint64_t)frame + (newpic && !hold);
    return 0x35a4e4f5u >> (f & 31) & 1;
  }
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
  int slice_kind = -1, slice_mbs = 0;  // the last slice's picture (pic_kind, pic_mbs)

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
  PadDetector() {
    for (int64_t& l : last_) l = -1;
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
  int predict(int p) const { return 0x35a4e4f5u >> (((uint64_t)pic_ + p) & 31) & 1; }

  int64_t pic_ = -1;  // pictures so far, the first one being 0
  int64_t n_ = 0;     // observations so far
  int64_t last_[33];  // the last observation each hypothesis mispredicted
  int cur_ = kOff;
  int pref_ = 0;       // the phase that makes the picture after the restart frame 0
  int64_t since_ = 0;  // the first observation after the restart
};

// x264's version SEI: user_data_unregistered with its UUID
bool is_x264_sei(const uint8_t* p, size_t n) {
  static const uint8_t uuid[16] = {0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
                                   0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef};
  return std::search(p, p + n, uuid, uuid + 16) != p + n;
}

// A NAL unit's header byte, and its payload after it (t and c)
struct NalHead {
  int ref_idc, type;
  const uint8_t* payload;
  size_t len;
  bool slice, newpic;  // a slice, and one that starts a picture
};
// whether a NAL unit has no header to read: empty, or forbidden_zero_bit set
bool headless(const std::vector<uint8_t>& nal) { return nal.empty() || (nal[0] & 0x80); }
// The header of a NAL unit that has one, the padding detector told of it
NalHead nal_head(const std::vector<uint8_t>& nal, PadDetector& det) {
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
  Syn s = br ? Syn(br, &ch) : Syn(&ch, bw);
  switch (type) {
    case 7: {
      auto sps = std::make_shared<Sps>();
      code_sps(s, *sps);
      ctx.ps.sps[sps->id] = sps;
      break;
    }
    case 8: {
      auto pps = std::make_shared<Pps>();
      code_pps(s, *pps, ctx.ps);
      ctx.ps.pps[pps->id] = pps;
      break;
    }
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
      SliceHeader h;
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
int read_nal(int type, int ref_idc, const std::vector<uint8_t>& rbsp, Context& ctx, Chan& ch,
             const BinPolicy& bp = BinPolicy()) {
  BitReader br(rbsp.data(), rbsp.size());
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
  bool irregular = false;
  int64_t drop = 0;
  std::string bits;  // of '0' and '1'
  int64_t czw = 0;
};

// The rest of the RBSP after the rbsp_stop_one_bit, as the trailer says.
void apply_trailer(const Trailer& t, BitWriter& bw, int pad) {
  if (t.irregular) {
    bw.truncate(bw.pos() - (size_t)t.drop);
    for (char c : t.bits) bw.bit(c - '0');
  } else {
    align(bw, pad);
  }
  bw.append(2 * (size_t)t.czw, 0);
}

int get_bit(const std::vector<uint8_t>& v, size_t i) { return v[i >> 3] >> (7 - (i & 7)) & 1; }

// Compares the RBSP written up to and including the stop bit (bw; pad as
// write_syntax returned it) with the original one, and finds the trailer
// that makes them equal. False when there is none. obs is the bit where x264
// puts its padding bit in a CABAC slice, -1 if there is none.
bool find_trailer(const BitWriter& bw, int pad, const std::vector<uint8_t>& rbsp, Trailer& t, std::string& why,
                  int& obs) {
  t = Trailer();
  size_t pbits = bw.pos();
  // the last bit of the byte with the stop bit, unless it is the stop bit
  obs = -1;
  if (pad >= 0 && (pbits & 7) && (pbits - 1) / 8 < rbsp.size()) obs = rbsp[(pbits - 1) / 8] & 1;
  BitWriter o = bw;
  align(o, pad);
  if (o.data() == rbsp) return true;
  // the end differs: cabac_zero_words, or trailing bits that are not the
  // expected ones
  size_t zeros = 0;
  while (zeros < rbsp.size() && rbsp[rbsp.size() - 1 - zeros] == 0) zeros++;
  t.czw = (int64_t)(zeros / 2);
  if (t.czw > (1 << 26)) {
    why = "too many cabac_zero_words";
    return false;
  }
  std::vector<uint8_t> r(rbsp.begin(), rbsp.end() - 2 * (size_t)t.czw);
  if (o.data() != r) {
    // compare our bits up to and including the stop bit
    size_t rbits = r.size() * 8, d = 0;
    while (d < pbits && d < rbits && bw.at(d) == get_bit(r, d)) d++;
    if (pbits - d > 64 || rbits - d > 4096) {
      why = "re-encoding differs at bit " + std::to_string(d) + " of " + std::to_string(rbits);
      return false;
    }
    t.irregular = true;
    t.drop = (int64_t)(pbits - d);
    for (size_t i = d; i < rbits; i++) t.bits += (char)('0' + get_bit(r, i));
  }
  BitWriter chk = bw;
  apply_trailer(t, chk, pad);
  if (chk.data() != rbsp) {
    why = "re-encoding differs";
    return false;
  }
  return true;
}

// SPS and PPS stored verbatim still define parameter sets (where they
// parse: an error is of no matter)
void update_params(int type, const std::vector<uint8_t>& rbsp, Context& ctx) {
  if (type != 7 && type != 8) return;
  LogChan ch;
  ErrTrap t;
  if (setjmp(t.jb)) return;
  err_push(t);
  read_nal(type, 0, rbsp, ctx, ch);
  err_pop(t);
}

// The RBSP of a NAL unit's payload, to parse (rbsp), or false when the NAL
// unit is to be stored verbatim (why): a type the walk does not know, or
// emulation prevention that escape() would not write (an SPS or PPS still
// defines its parameter set)
bool nal_rbsp(const NalHead& h, Context& ctx, std::vector<uint8_t>& rbsp, std::string& why) {
  rbsp = unescape(h.payload, h.len);
  if (!parsed_type(h.type)) {
    why = "NAL unit type " + std::to_string(h.type);
    return false;
  }
  if (escape(rbsp) != std::vector<uint8_t>(h.payload, h.payload + h.len)) {
    why = "irregular emulation prevention";
    update_params(h.type, rbsp, ctx);
    return false;
  }
  return true;
}

// A NAL unit written back from the values its parse gave the channel ch,
// which hands them out again (at_end() when it has no more), and the
// trailer that makes it its RBSP; false when there is none, or the writing
// fails (why). obs: see find_trailer.
template <class Ch>
bool write_back(const NalHead& h, Context& ctx, Ch& ch, const std::vector<uint8_t>& rbsp, Trailer& t,
                std::string& why, int& obs) {
  BitWriter bw;
  ErrTrap e;
  if (setjmp(e.jb)) {
    why = std::string("re-encoding failed: ") + e.why;
    return false;
  }
  err_push(e);
  int pad = write_syntax(h.type, h.ref_idc, ctx, ch, bw);
  if (!ch.at_end()) err_raise("values left over");
  err_pop(e);
  return find_trailer(bw, pad, rbsp, t, why, obs);
}

// The output of d: n zero bytes appended to buf, spill() after each
// MB of them (it hands buf on once it is full)
template <class F>
void put_zeros(std::vector<uint8_t>& buf, uint64_t n, F spill) {
  while (n) {
    size_t k = (size_t)std::min<uint64_t>(n, 1 << 20);
    buf.insert(buf.end(), k, 0);
    n -= k;
    spill();
  }
}
// the zero bytes before a NAL unit and its start code
template <class F>
void put_start(std::vector<uint8_t>& buf, uint64_t zeros, F spill) {
  static const uint8_t sc[3] = {0, 0, 1};
  put_zeros(buf, zeros, spill);
  buf.insert(buf.end(), sc, sc + 3);
}
// a NAL unit's payload stored verbatim (an SPS or PPS still defines its
// parameter set)
void put_verbatim(std::vector<uint8_t>& buf, int type, const std::vector<uint8_t>& b, Context& ctx) {
  buf.insert(buf.end(), b.begin(), b.end());
  update_params(type, unescape(b.data(), b.size()), ctx);
}

}  // namespace h264p
