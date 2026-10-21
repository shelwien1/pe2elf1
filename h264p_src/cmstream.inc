// The compressed stream of one model (c, d): its header, then the stream's
// syntax through the model (model.inc), NAL unit by NAL unit, after a dry
// run that parses and writes back each one, and a CRC-32 at the end.

namespace h264p {

const char kCmMagic[] = "h264c";  // compressed streams, followed by the version and level bytes
const int kCmVersion = 6;
// The CRC-32 at the end is coded XORed with this: a decoder fed past the
// end of its input decodes zeros, and 0 is the CRC-32 of an empty stream.
const uint32_t kCrcXor = 0x68323634;

// The level of a compressed stream from its first bytes, n of them read
// (the magic, the version, the level), or -1 (reported) if they are not
// h264p's
int cm_level(const uint8_t* head, size_t n, const char* name, Progress* meter = nullptr) {
  const size_t m = sizeof kCmMagic - 1;  // the magic's bytes; the version, the level after them
  if (!n || memcmp(head, kCmMagic, std::min(n, m))) {
    say_error(meter, "h264p: %s: not a stream h264p compressed\n", name);
    return -1;
  }
  if (n > m && head[m] != kCmVersion) {
    say_error(meter, "h264p: %s: compressed by version %d, this is version %d\n", name, (int)head[m], kCmVersion);
    return -1;
  }
  if (n < m + 2) {
    say_error(meter, "h264p: %s: the compressed stream is cut short\n", name);
    return -1;
  }
  return head[m + 1];
}

// What a run of the compressor or the decompressor takes: Init() (coro.inc),
// which MTwrap's workers have too (mt.inc)
struct H264pArgs {
  int level;        // compressing: the model's level (1-9)
  bool verbose;     // statistics on stderr
  bool strict;      // compressing: fail where a NAL unit of a supported type would be stored verbatim
  bool check;       // compressing: the model pass checks the slices it can; status kRestart if one does
                    // not come out as it is, and the input has to be compressed again without
  const char* name;  // the input's, for messages
  Progress* meter;   // the call's progress line, if one is shown (messages cut it)

  void Init() {
    level = 0;
    verbose = strict = check = false;
    name = "";
    meter = nullptr;
  }
};

// The compressor's dry run of a NAL unit (run()): parses it into the log,
// writes it back from the log and finds how it ends (t). False when it has
// to be stored verbatim (why). obs: see find_trailer. Its buffers only
// grow, for the next one.
//
// With check, a slice is not written back: it would come out as it is up
// to its stop bit, unless the walk writes other values, bits or bins than
// it reads, which the model pass checks (BinPolicy::rec, checked_bits) when this
// returns true with checked set; rbsp is then the RBSP. With CABAC, the
// bins of the parse go to rec, and each run of CABAC data must end in a 1
// bit (as written, the stop bit; its bits are then what an encoder writes
// for the bins).
struct DryRun {
  Trailer t;
  char why[kWhy];
  ValueLog log;
  BinRec rec;
  bool checked;            // the model pass checks it
  Buf<uint8_t> rbsp, esc;  // the RBSP; escaped again, to compare
  BitWriter bw, o;         // the NAL unit written back (and by the model pass); find_trailer()'s

  void Init() {
    t.Init();
    why[0] = 0;
    log.Init();
    rec.Init();
    checked = false;
    rbsp.Init();
    esc.Init();
    bw.Init();
    o.Init();
  }
  void Quit() {
    t.Quit();
    log.Quit();
    rec.Quit();
    rbsp.Quit();
    esc.Quit();
    bw.Quit();
    o.Quit();
  }

  bool run(const NalHead& h, Context& ctx, bool check, int& obs) {
    obs = -1;
    checked = false;
    if (!nal_rbsp(h, ctx, rbsp, esc, why)) return false;
    log.clear();
    LogChan ch;
    ch.Init(log, false);
    BinPolicy bp;
    bp.Init();
    if (check) rec.clear(), bp.rec = &rec;
    int pad;
    ErrTrap e;  // (an error: the NAL unit is stored verbatim)
    if (setjmp(e.jb)) {
      set_why(why, "%s", e.why);
      return false;
    }
    err_push(e);
    pad = read_nal(h.type, h.ref_idc, rbsp, ctx, ch, bp);
    err_pop(e);
    if (check && rec.slice && rec.stops) {
      bw.set(rbsp.data(), rec.end);
      if (find_trailer(bw, pad, rbsp, t, why, obs, o)) {
        checked = true;
        return true;
      }
    }
    log.rewind();
    ch.Init(log, true);
    return write_back(h, ctx, ch, rbsp, t, why, obs, bw, o);
  }
};

// The model pass of a NAL unit that the dry run did not write back (check):
// whether what it wrote (bw) is the RBSP up to its stop bit. Without its
// CABAC bins (nobins), it wrote the bits outside the runs of CABAC data, and
// checked the bins against those recorded.
bool checked_bits(const BitWriter& bw, bool nobins, const BinRec& rec, const Buf<uint8_t>& rbsp) {
  if (rec.bad || !rec.slice || (rec.segs.size() & 1)) return false;
  if (!nobins || rec.segs.empty()) return bw.same(rbsp.data(), rec.end);
  size_t k = 0, from = 0;
  for (size_t i = 0; i < rec.segs.size(); i += 2) {
    for (size_t b = from; b < rec.segs[i]; b++, k++)
      if (k >= bw.pos() || bw.at(k) != get_bit(rbsp, b)) return false;
    from = rec.segs[i + 1];
  }
  return k == bw.pos();
}

// CRC-32 (IEEE) of the stream, coded at the end of a compressed one
struct Crc32 {
  void Init() {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t x = i;
      for (int k = 0; k < 8; k++) x = x & 1 ? 0xEDB88320u ^ (x >> 1) : x >> 1;
      t[i] = x;
    }
    c = 0xFFFFFFFFu;
  }
  void add(const uint8_t* p, size_t n) {
    for (size_t i = 0; i < n; i++) c = t[(c ^ p[i]) & 255] ^ (c >> 8);
  }
  void add(const Buf<uint8_t>& v) { add(v.data(), v.size()); }
  void zeros(size_t n) {
    for (size_t i = 0; i < n; i++) c = t[c & 255] ^ (c >> 8);
  }
  uint32_t value() const { return ~c; }
  uint32_t t[256];
  uint32_t c;
};

void cm_report(const tc_rc<0>& e, u64 in_bytes, u64 out_bytes, u64 tables) {
  double tot = 0;
  for (int i = 0; i < STG_N; i++) tot += e.tc_bits[i];
  fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " MB of model tables\n", in_bytes, out_bytes,
          in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, tables >> 20);
  for (int i = 0; i < STG_N; i++) {
    if (!e.tc_syms[i]) continue;
    fprintf(stderr, "  %-13s %11.0f bytes %6.2f%%  %10" PRIu64 " decisions or values, %.3f bits each\n", kStage[i],
            e.tc_bits[i] / 8, tot > 0 ? 100.0 * e.tc_bits[i] / tot : 0.0, e.tc_syms[i],
            e.tc_bits[i] / (double)e.tc_syms[i]);
  }
}

// The compressed stream: the magic, the version and the model's level (c's
// -L; d takes the instance of the model for it), then one range coded run of
// decisions. The bytes before the first start code (junk), then for each
// NAL unit: that there is one (F_MORE), its type, nal_ref_idc and the zero
// bytes before its start code, a change of x264's padding bits (slices),
// whether it is stored verbatim (a type the walk knows); then its syntax
// through the model and the trailer, or its length and bytes. At the end,
// the zero bytes after the last NAL unit and the CRC-32 of the stream.
//
// check: the dry runs leave the CABAC slices they can to the model pass to
// check (dry_run); if one of them does not come out as it is, the model
// has coded it, and the run ends with kRestart (its trap, coro.inc).

// The model's records of the last picture of each kind (History and
// AqModel, model.inc: 535 bytes a macroblock) grow with the kinds of
// picture a stream has: a few, 4.2 MB each at 1080p. All 30 at the
// largest pictures (139,264 macroblocks a frame, half that a field) would
// take 1.4 GB, which 261 bytes can ask for (test/crafted/kinds.264). So a
// slice that would take them past H264P_HIST_MB is stored verbatim.
#ifndef H264P_HIST_MB
#define H264P_HIST_MB 512
#endif
struct HistBound {
  static constexpr uint64_t kPerMb = sizeof(MbCol) + 1 + sizeof(int16_t);  // History's rec and skip, AqModel's res
  static constexpr uint64_t kMax = (uint64_t)H264P_HIST_MB << 20;
  int mbs[30];  // the largest picture of each kind the model has had
  uint64_t bytes;

  void Init() {
    for (int& n : mbs) n = 0;
    bytes = 0;
  }
  bool fits(int k, int n) const { return n <= mbs[k] || bytes + (uint64_t)(n - mbs[k]) * kPerMb <= kMax; }
  void add(int k, int n) {
    if (n > mbs[k]) bytes += (uint64_t)(n - mbs[k]) * kPerMb, mbs[k] = n;
  }
};

// What a compression keeps from one NAL unit to the next besides its
// model, on the heap: the stack of its coroutine (coro.inc), which each
// yield copies, stays small, and Quit() frees all of it wherever the
// coroutine was left
struct CompressState {
  Context ctx;
  PadDetector det;
  Crc32 crc;
  Stats st;
  DryRun dr;
  HistBound hist;
  Buf<uint8_t> nal;
  AnnexBReader rd;  // the input

  void Init(ByteSource in) {
    ctx.Init();
    det.Init();
    crc.Init();
    st.Init();
    dr.Init();
    hist.Init();
    nal.Init();
    rd.Init(in);
  }
  void Quit() {
    ctx.Quit();
    st.Quit();
    dr.Quit();
    nal.Quit();
    rd.Quit();
  }
};

// and a decompression
struct DecompressState {
  Context ctx;
  Crc32 crc;
  Buf<uint8_t> buf;      // the stream decoded, not yet written
  Buf<uint8_t> b, rbsp;  // a NAL unit stored verbatim, and its RBSP
  BitWriter bw;          // a NAL unit's RBSP written
  Trailer t;             // and how it ends

  void Init() {
    ctx.Init();
    crc.Init();
    buf.Init();
    b.Init();
    rbsp.Init();
    bw.Init();
    t.Init();
  }
  void Quit() {
    ctx.Quit();
    buf.Quit();
    b.Quit();
    rbsp.Quit();
    bw.Quit();
    t.Quit();
  }
};

// The model pass of a NAL unit that parsed (h, its dry run's in s.dr):
// its syntax through the model, then what the syntax leaves out of it (its
// trailer). False where the check finds it would not come out as it is.
template <class LV>
bool compress_parsed(Model<0, LV>& m, CompressState& s, const NalHead& h) {
  DryRun& dr = s.dr;
  const Trailer& t = dr.t;
  dr.log.rewind();
  LogChan log;
  log.Init(dr.log, true);
  ModelChan<0, LV> ch;
  ch.Init(m, &log);
  BinPolicy bp = ch.policy();
  m.nal_begin(h.type, h.ref_idc);
  BitWriter& bw = dr.bw;
  bw.clear();
  if (dr.checked) {
    // (whatever fails, the NAL unit would not come out as it is)
    bp.rec = &dr.rec;
    ErrTrap e;
    if (setjmp(e.jb)) return false;
    err_push(e);
    write_syntax(h.type, h.ref_idc, s.ctx, ch, bw, bp);
    err_pop(e);
    if (!log.at_end() || !checked_bits(bw, bp.nobins, dr.rec, dr.rbsp)) return false;
  } else {
    write_syntax(h.type, h.ref_idc, s.ctx, ch, bw, bp);
    if (!log.at_end()) cm_fail("internal: values left over");
  }
  m.nal(F_TRAIL, h.type, t.irregular);
  if (t.irregular) {
    m.nal(F_DROP, 0, t.drop);
    m.nal(F_NBITS, 0, (i64)t.bits.size());
    for (uint8_t b : t.bits) m.nal(F_TBIT, 0, b);
  }
  m.nal(F_CZW, h.type, t.czw);
  s.st.parsed++;
  s.st.trail += t.irregular;
  return true;
}

// A NAL unit of the stream (zeros before it; prev_type, the type of the
// one before it): its dry run, then the model pass, on one context (the
// parameter sets and the macroblocks of either are the same; x264's
// padding moves on to the next slice after both). 0, or 1 after an error
// (reported).
template <class LV>
int compress_nal(Model<0, LV>& m, CompressState& s, const H264pArgs& a, const Buf<uint8_t>& nal, size_t zeros,
                 int& prev_type) {
  static const uint8_t sc[3] = {0, 0, 1};
  Stats& st = s.st;
  Context& ctx = s.ctx;
  s.crc.zeros(zeros);
  s.crc.add(sc, 3);
  s.crc.add(nal);
  st.nals++;
  m.nal(F_MORE, 0, 1);
  if (headless(nal)) {
    m.nal(F_TYPE, prev_type, 32);
    m.nal(F_ZEROS, 32, (i64)zeros);
    m.nal(F_LEN, 32, (i64)nal.size());
    for (uint8_t b : nal) m.raw(b);
    st.verbatim("empty or forbidden_zero_bit");
    return 0;
  }
  NalHead h = nal_head(nal, s.det);
  int obs;
  bool ok = s.dr.run(h, ctx, a.check, obs);
  if (ok && h.slice && !s.hist.fits(ctx.slice_kind, ctx.slice_mbs)) {
    ok = false;
    set_why(s.dr.why, "the model's records of the kinds of picture would pass %lld MB", (long long)H264P_HIST_MB);
  }
  bool padchg = ok && repad(s.det, obs, ctx.pad);
  if (padchg) {
    ok = s.dr.run(h, ctx, a.check, obs);  // once more, with padding bits that predict this one
    st.pad++;
  }
  if (ok && h.slice) s.hist.add(ctx.slice_kind, ctx.slice_mbs);
  m.nal(F_TYPE, prev_type, h.type);
  m.nal(F_REF, h.type, h.ref_idc);
  m.nal(F_ZEROS, h.type, (i64)zeros);
  prev_type = h.type;
  if (h.slice) {
    m.nal(F_PADCHG, 0, padchg);
    if (padchg) {
      m.nal(F_PADON, 0, ctx.pad.on);
      if (ctx.pad.on) m.nal(F_PADFRAME, 0, ctx.pad.frame);
    }
  }
  if (parsed_type(h.type)) m.nal(F_VERB, h.type, !ok);
  if (ok) {
    if (!compress_parsed(m, s, h)) {
      if (a.verbose)
        say(a.meter, "h264p: NAL unit %ld would not come out as it is; compressing again, writing back each\n",
            st.nals - 1);
      err_jump(*m.run_trap, kRestart);
    }
  } else {
    // (an SPS or PPS stored verbatim still defines its parameter set
    // where it could be parsed: the dry run has it so)
    m.nal(F_LEN, h.type, (i64)h.len);
    for (size_t i = 0; i < h.len; i++) m.raw(h.payload[i]);
    if (!st.verbatim(h.type, s.dr.why, a.verbose, a.strict, a.meter)) return 1;
  }
  ctx.done(h.type, h.payload, h.len);
  return 0;
}

// Compresses the stream s.rd reads with the model m, a new one, into its
// output (m is the range coder too, and the coroutine whose output it is,
// coro.inc): 0, or 1 after a NAL unit -s would not store verbatim
// (reported). An error, or a slice the check finds would not come out as
// it is, ends the run at its trap instead (coro.inc).
template <class LV>
int compress_stream(Model<0, LV>& m, CompressState& s, const H264pArgs& a) {
  Crc32& crc = s.crc;
  AnnexBReader& rd = s.rd;
  const uint8_t head[2] = {(uint8_t)kCmVersion, (uint8_t)LV::id};
  m.write((const uint8_t*)kCmMagic, sizeof kCmMagic - 1);
  m.write(head, 2);
  m.init();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  Buf<uint8_t>& nal = s.nal;
  size_t zeros;
  bool more = rd.next(zeros, nal);
  const Buf<uint8_t>& junk = rd.junk();
  m.nal(F_JUNK, 0, (i64)junk.size());
  for (uint8_t b : junk) m.raw(b);
  crc.add(junk);
  int prev_type = 0;
  for (; more; more = rd.next(zeros, nal))
    if (compress_nal(m, s, a, nal, zeros, prev_type)) return 1;
  m.nal(F_MORE, 0, 0);
  m.nal(F_EOFZ, 0, (i64)rd.eof_zeros());
  crc.zeros(rd.eof_zeros());
  m.nal(F_CRC, 0, crc.value() ^ kCrcXor);
  m.end_stream();
  m.rc_Quit();
  if (a.verbose) {
    s.st.report("coded", "changes of x264's padding bits", a.meter);
    cm_report(m, m.n_in, m.n_out, m.table_bytes());
  }
  return 0;
}

// a count coded in a compressed stream
size_t cm_count(i64 v, i64 lim) {
  if (v < 0 || v > lim) cm_fail("corrupt stream: a count of %" PRId64, v);
  return (size_t)v;
}

// Decompresses with the model m, a new one, what is left of its input
// after the magic, the version and the level, into its output (m is the
// range decoder too, and the coroutine whose input it reads, coro.inc): 0.
// An error ends the run at its trap (coro.inc), which writes what was
// decoded (s.buf) and says why.
template <class LV>
int decompress_stream(Model<1, LV>& m, DecompressState& s, const H264pArgs& a) {
  Buf<uint8_t>& buf = s.buf;
  Crc32& crc = s.crc;
  long nals = 0;
  auto flush = [&]() {
    crc.add(buf);
    m.write(buf.data(), buf.size());
    buf.clear();
  };
  auto spill = [&]() {
    if (buf.size() > (1 << 16)) flush();
  };
  m.init();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  Context& ctx = s.ctx;
  for (size_t n = cm_count(m.nal(F_JUNK, 0, 0), (i64)1 << 40); n; n--) buf.push_back((uint8_t)m.raw(0));
  int prev_type = 0;
  while (m.nal(F_MORE, 0, 0)) {
    nals++;
    int type = (int)cm_count(m.nal(F_TYPE, prev_type, 0), 32);
    if (type == 32) {
      put_start(buf, cm_count(m.nal(F_ZEROS, 32, 0), (i64)1 << 40), spill);
      for (size_t n = cm_count(m.nal(F_LEN, 32, 0), (i64)1 << 40); n; n--) buf.push_back((uint8_t)m.raw(0));
      continue;
    }
    int ref_idc = (int)cm_count(m.nal(F_REF, type, 0), 3);
    put_start(buf, cm_count(m.nal(F_ZEROS, type, 0), (i64)1 << 40), spill);
    buf.push_back((uint8_t)(ref_idc << 5 | type));
    prev_type = type;
    size_t start = buf.size();
    if ((type == 1 || type == 5) && m.nal(F_PADCHG, 0, 0)) {
      if (m.nal(F_PADON, 0, 0))
        ctx.pad.set((int64_t)cm_count(m.nal(F_PADFRAME, 0, 0), INT32_MAX));
      else
        ctx.pad.on = false;
    }
    if (!parsed_type(type) || m.nal(F_VERB, type, 0)) {
      Buf<uint8_t>& b = s.b;  // grown as the bytes come: the length may be corrupt
      b.clear();
      for (size_t n = cm_count(m.nal(F_LEN, type, 0), (i64)1 << 40); n; n--) b.push_back((uint8_t)m.raw(0));
      put_verbatim(buf, type, b, ctx, s.rbsp);
    } else {
      ModelChan<1, LV> ch;
      ch.Init(m, nullptr);
      m.nal_begin(type, ref_idc);
      BitWriter& bw = s.bw;
      bw.clear();
      int pad = write_syntax(type, ref_idc, ctx, ch, bw, ch.policy());
      Trailer& t = s.t;
      t.reset();
      t.irregular = m.nal(F_TRAIL, type, 0) != 0;
      if (t.irregular) {
        t.drop = (int64_t)cm_count(m.nal(F_DROP, 0, 0), (i64)bw.pos());
        t.bits.resize(cm_count(m.nal(F_NBITS, 0, 0), 1 << 16));
        for (uint8_t& b : t.bits) b = (uint8_t)cm_count(m.nal(F_TBIT, 0, 0), 1);
        if ((bw.pos() - (size_t)t.drop + t.bits.size()) & 7) cm_fail("corrupt stream: trailing bits");
      }
      t.czw = (int64_t)cm_count(m.nal(F_CZW, type, 0), 1 << 26);
      apply_trailer(t, bw, pad);
      const Buf<uint8_t>& e = bw.data();
      escape(e.data(), e.size(), buf);
    }
    ctx.done(type, buf.data() + start, buf.size() - start);
    spill();
  }
  put_zeros(buf, cm_count(m.nal(F_EOFZ, 0, 0), (i64)1 << 40), spill);
  m.end_stream();
  flush();
  if (m.nal(F_CRC, 0, 0) != (crc.value() ^ kCrcXor)) cm_fail("CRC mismatch: the stream did not decompress correctly");
  // the coder's flush leaves off the last bytes, so the decoder always
  // reads past the end of a whole stream
  if (!m.over) cm_fail("corrupt stream: data after its end");
  if (a.verbose) say(a.meter, "%ld NAL units\n", nals);
  return 0;
}

}  // namespace h264p
