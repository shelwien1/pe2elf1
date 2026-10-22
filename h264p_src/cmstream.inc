// The compressed stream of one model (c, d): its header, then the stream's
// syntax through the model (model.inc), NAL unit by NAL unit, after a dry
// run that parses and writes back each one, and a CRC-32 at the end.

namespace h264p {

const char kCmMagic[] = "h264c";  // compressed streams, followed by the version and level bytes
const int kCmVersion = 8;
// The CRC-32 at the end is coded XORed with this: a decoder fed past the
// end of its input decodes zeros, and 0 is the CRC-32 of an empty stream.
const uint32_t kCrcXor = 0x68323634;

// whether v, a compressed stream's version byte, is this version (else why,
// a line, in msg, of mn bytes, for the caller to say)
bool cm_version_ok(int v, const char* name, char* msg, size_t mn) {
  if (v == kCmVersion) return true;
  snprintf(msg, mn, "h264p: %s: compressed by version %d, this is version %d\n", name, v, kCmVersion);
  return false;
}
// The level of a compressed stream from its first bytes, n of them read
// (the magic, the version, the level), or -1 if they are not h264p's (why
// in msg)
int cm_level(const uint8_t* head, size_t n, const char* name, char* msg, size_t mn) {
  const size_t m = sizeof kCmMagic - 1;  // the magic's bytes; the version, the level after them
  if (!n || memcmp(head, kCmMagic, Min(n, m))) {
    snprintf(msg, mn, "h264p: %s: not a stream h264p compressed\n", name);
    return -1;
  }
  if (n > m && !cm_version_ok(head[m], name, msg, mn)) return -1;
  if (n < m + 2) {
    snprintf(msg, mn, "h264p: %s: the compressed stream is cut short\n", name);
    return -1;
  }
  return head[m + 1];
}

// What a run of the compressor or the decompressor takes: Init() (coro.inc),
// which MTwrap's workers have too (mt.inc)
struct H264pArgs {
  int level;        // compressing: the model's level (1-9)
  bool verbose;     // statistics, and the messages they come with (the codec tells them: coro.inc)
  bool strict;      // compressing: fail where a NAL unit of a supported type would be stored verbatim
  bool check;       // compressing: the model pass checks the slices it can; status kRestart if one does
                    // not come out as it is, and the input has to be compressed again without
  const char* name;  // the input's, for messages
  Progress* meter;   // the call's progress line, where its driver shows what the codec tells (coro.inc)
  // compressing a part of -t: the index in the stream of its first NAL
  // unit, and the parameter sets copied into it, ncopy NAL units from its
  // copy_at (for messages: Stats)
  uint64_t nal0, copy_at, ncopy;

  void Init() {
    level = 0;
    verbose = strict = check = false;
    name = "";
    meter = nullptr;
    nal0 = copy_at = ncopy = 0;
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
      if (k >= bw.pos() || bw.at(k) != bit_at(rbsp.data(), b)) return false;
    from = rec.segs[i + 1];
  }
  return k == bw.pos();
}

// What the video is, as the slice just parsed (ctx.sh) and its parameter
// sets say (none: all zero)
VideoInfo video_of(const Context& ctx) {
  VideoInfo v;
  memset((void*)&v, 0, sizeof v);
  const SliceHeader& h = ctx.sh;
  const Sps* q = h.sps;
  const Pps* p = h.pps;
  if (!q || !p) return v;
  int ct = q->chroma_array_type();
  int cx = ct == 1 || ct == 2 ? 2 : 1, cy = (ct == 1 ? 2 : 1) * (2 - q->frame_mbs_only_flag);  // CropUnitX, CropUnitY
  v.width = Max(0, 16 * q->width_mbs - cx * (q->crop_left + q->crop_right));
  v.height = Max(0, 16 * q->frame_height_mbs() - cy * (q->crop_top + q->crop_bottom));
  v.profile = q->profile_idc, v.constraints = q->constraint_flags, v.level = q->level_idc;
  v.chroma = q->chroma_format_idc, v.depth_luma = q->bit_depth_luma, v.depth_chroma = q->bit_depth_chroma;
  v.frames_only = q->frame_mbs_only_flag, v.mbaff = q->mb_adaptive_frame_field_flag;
  v.refs = q->max_num_ref_frames, v.lossless = q->transform_bypass;
  v.num_units_in_tick = q->num_units_in_tick, v.time_scale = q->time_scale;
  v.entropy = 1 << p->entropy_coding_mode_flag;
  v.weighted_b = p->weighted_bipred_idc ? 1 << p->weighted_bipred_idc : 0;
  v.slice_groups = p->num_slice_groups, v.field = h.field_pic_flag, v.transform_8x8 = p->transform_8x8_mode_flag;
  v.weighted_p = p->weighted_pred_flag, v.constrained_intra = p->constrained_intra_pred_flag;
  v.scaling = q->scaling_matrix || p->scaling_matrix;
  return v;
}
// told to the driver (kYieldVideo), after a slice parsed, where it adds to
// what was told (VideoInfo::add()); and the progress, at each picture
// (kYieldTick)
void tell_video(CoroIo& io, const Context& ctx) {
  VideoInfo v = video_of(ctx);
  if (v.width && io.video.add(v)) io.tell(kYieldVideo);
}
void tell_tick(CoroIo& io, const Context& ctx) {
  if (ctx.pictures == io.pictures) return;
  io.pictures = ctx.pictures;
  io.tell(kYieldTick);
}

// -v's statistics of a compression's coder e (said after the progress line,
// meter's, if any)
void cm_report(const tc_rc<0>& e, u64 in_bytes, u64 out_bytes, u64 tables, Progress* meter) {
  double tot = 0;
  for (int i = 0; i < STG_N; i++) tot += e.tc_bits[i];
  say(meter, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " MB of model tables\n", in_bytes, out_bytes,
      in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, tables >> 20);
  for (int i = 0; i < STG_N; i++) {
    if (!e.tc_syms[i]) continue;
    say(meter, "  %-13s %11.0f bytes %6.2f%%  %10" PRIu64 " decisions or values, %.3f bits each\n", kStage[i],
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
// through the model and the trailer, or its bytes. At the end, the zero
// bytes after the last NAL unit and the CRC-32 of the stream. Bytes stored
// as they are come in chunks (code_raw()), and a NAL unit longer than kHold
// is stored so as it is read: neither side holds one whole.
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
  Stats st;
  DryRun dr;
  HistBound hist;
  Buf<uint8_t> nal;
  AnnexBReader rd;  // the input

  void Init(ByteSource in) {
    ctx.Init();
    det.Init();
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
  uint64_t nals;         // the NAL units decoded (-v)
  Buf<uint8_t> buf;      // the stream decoded, not yet written
  Buf<uint8_t> b, rbsp;  // a NAL unit stored verbatim, and its RBSP
  BitWriter bw;          // a NAL unit's RBSP written
  Trailer t;             // and how it ends

  void Init() {
    ctx.Init();
    crc.Init();
    nals = 0;
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

// a count coded in a compressed stream
size_t cm_count(i64 v, i64 lim) {
  if (v < 0 || v > lim) err_raise("corrupt stream: a count of %" PRId64, v);
  return (size_t)v;
}
// A count of zero bytes (field fld, context c), any up to 2^64 - 1: in
// pieces of at most 2^40, one of 2^40 followed by another
const i64 kCountPiece = (i64)1 << 40;
template <class M>
void code_count(M& m, int fld, int c, uint64_t v) {
  for (;;) {
    i64 k = (i64)Min<uint64_t>(v, (uint64_t)kCountPiece);
    m.nal(fld, c, k);
    v -= (uint64_t)k;
    if (k < kCountPiece) return;
  }
}
template <class M>
uint64_t decode_count(M& m, int fld, int c) {
  uint64_t v = 0;
  for (;;) {
    size_t k = cm_count(m.nal(fld, c, 0), kCountPiece);
    v += k;
    if ((i64)k < kCountPiece) return v;
  }
}

// Bytes coded as they are, in chunks: the count of each (field fld,
// context c), up to kChunk, then its bytes; a chunk of kChunk is followed
// by another (the last one shorter, maybe empty). So no run of them, however
// long, is held whole: here b's from b[i] on, then what is left of the
// reader's segment, if one is given, b holding a chunk of it at a time.
// Decoded, each byte goes to put().
const size_t kChunk = (size_t)1 << 20;
// what is left of rd's segment read onto b, up to n bytes in all: b's size
// (b grows as they come: n may be far more than there are)
size_t read_more(AnnexBReader& rd, Buf<uint8_t>& b, size_t n) {
  while (b.size() < n) {
    size_t k = b.size(), want = Min(n - k, (size_t)1 << 16);
    b.room(k + want);
    size_t r = rd.read(b.data() + k, want);
    if (!r) break;
    b.n = k + r;
  }
  return b.size();
}
template <class M>
void code_raw(M& m, int fld, int c, Buf<uint8_t>& b, size_t i, AnnexBReader* rd = nullptr) {
  for (;;) {
    if (rd && b.size() - i < kChunk) {
      b.erase_front(i), i = 0;
      read_more(*rd, b, kChunk);
    }
    size_t k = Min(b.size() - i, kChunk);
    m.nal(fld, c, (i64)k);
    for (size_t j = 0; j < k; j++) m.raw(b[i + j]);
    i += k;
    if (k < kChunk) return;
  }
}
template <class M, class F>
void decode_raw(M& m, int fld, int c, F put) {
  for (;;) {
    size_t k = cm_count(m.nal(fld, c, 0), (i64)kChunk);
    for (size_t j = 0; j < k; j++) put((uint8_t)m.raw(0));
    if (k < kChunk) return;
  }
}
// How a NAL unit that parsed ends (its trailer t), coded, or decoded into
// t: the same calls of the model either way. (The bounds are those of
// find_trailer(), within which c codes it; d checks t against what it
// wrote, which c's model pass may not have: BinPolicy::nobins.)
template <class M>
void code_trailer(M& m, int type, Trailer& t) {
  t.irregular = m.nal(F_TRAIL, type, t.irregular) != 0;
  if (t.irregular) {
    t.drop = (int64_t)cm_count(m.nal(F_DROP, 0, t.drop), 64);
    t.bits.resize(cm_count(m.nal(F_NBITS, 0, (i64)t.bits.size()), 1 << 16));
    for (uint8_t& b : t.bits) b = (uint8_t)cm_count(m.nal(F_TBIT, 0, b), 1);
  }
  t.czw = (int64_t)cm_count(m.nal(F_CZW, type, t.czw), 1 << 26);
}

// The model pass of a NAL unit that parsed (h, its dry run's in s.dr):
// its syntax through the model, then what the syntax leaves out of it (its
// trailer). False where the check finds it would not come out as it is.
template <class LV>
bool compress_parsed(Model<0, LV>& m, CompressState& s, const NalHead& h) {
  DryRun& dr = s.dr;
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
    if (!log.at_end()) err_raise("internal: values left over");
  }
  code_trailer(m, h.type, dr.t);
  s.st.parsed++;
  s.st.trail += dr.t.irregular;
  return true;
}

// A NAL unit longer than this is stored verbatim as it is read (in chunks,
// code_raw()), not held whole to be parsed. (A slice of the largest
// picture in 8-bit 4:2:0 I_PCM macroblocks takes 52 MB.)
const size_t kHold = (size_t)1 << 26;

// A NAL unit of the stream (zeros before it; its bytes in nal, all of them,
// or with lng the first kHold + 1, the rest still in the reader; prev_type,
// the type of the one before it): its dry run, then the model pass, on one
// context (the parameter sets and the macroblocks of either are the same;
// x264's padding moves on to the next slice after both). 0, or 1 after an
// error (reported).
template <class LV>
int compress_nal(Model<0, LV>& m, CompressState& s, const H264pArgs& a, Buf<uint8_t>& nal, uint64_t zeros, bool lng,
                 int& prev_type) {
  Stats& st = s.st;
  Context& ctx = s.ctx;
  AnnexBReader* rest = lng ? &s.rd : nullptr;
  st.nals++;
  m.nal(F_MORE, 0, 1);
  if (headless(nal)) {
    m.nal(F_TYPE, prev_type, 32);
    code_count(m, F_ZEROS, 32, zeros);
    code_raw(m, F_LEN, 32, nal, 0, rest);
    st.verbatim("empty or forbidden_zero_bit");
    return 0;
  }
  NalHead h = nal_head(nal, s.det);
  int obs = -1;
  bool ok = false;
  if (lng)
    set_why(s.dr.why, "longer than %d MB", (int)(kHold >> 20));
  else
    ok = s.dr.run(h, ctx, a.check, obs);
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
  code_count(m, F_ZEROS, h.type, zeros);
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
      if (a.verbose) {
        char w[64];
        st.name_last(w, sizeof w);
        m.tell_note("h264p: %s would not come out as it is; compressing again, writing back each", w);
      }
      err_jump(*m.run_trap, kRestart);
    }
    if (h.slice) tell_video(m, ctx);
    ctx.done(h.type, h.payload, h.len);
    return 0;
  }
  // (an SPS or PPS stored verbatim still defines its parameter set where
  // it could be parsed: the dry run has it so; a long one is not parsed.
  // Done before its bytes, which a long one's reading moves.)
  ctx.done(h.type, h.payload, h.len);
  code_raw(m, F_LEN, h.type, nal, 1, rest);
  return st.verbatim(h.type, s.dr.why, a.verbose, a.strict, m) ? 0 : 1;
}

// Compresses the stream s.rd reads with the model m, a new one, into its
// output (m is the range coder too, and the coroutine whose output it is,
// coro.inc, which tells its driver what the video is, each picture, and
// at the end that its statistics are ready, s.st and its coder's): 0, or
// 1 after a NAL unit -s would not store verbatim (why in m's note). An
// error, or a slice the check finds would not come out as it is, ends the
// run at its trap instead (coro.inc).
template <class LV>
int compress_stream(Model<0, LV>& m, CompressState& s, const H264pArgs& a) {
  AnnexBReader& rd = s.rd;
  const uint8_t head[2] = {(uint8_t)kCmVersion, (uint8_t)LV::id};
  m.write((const uint8_t*)kCmMagic, sizeof kCmMagic - 1);
  m.write(head, 2);
  m.start();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  s.st.nal0 = a.nal0, s.st.copy_at = a.copy_at, s.st.ncopy = a.ncopy;
  Buf<uint8_t>& nal = s.nal;
  nal.clear();
  code_raw(m, F_JUNK, 0, nal, 0, &rd);
  int prev_type = 0;
  uint64_t zeros;
  while (rd.next(zeros)) {
    nal.clear();
    bool lng = read_more(rd, nal, kHold + 1) > kHold;
    if (compress_nal(m, s, a, nal, zeros, lng, prev_type)) return 1;
    tell_tick(m, s.ctx);
  }
  m.nal(F_MORE, 0, 0);
  code_count(m, F_EOFZ, 0, rd.eof_zeros());
  m.nal(F_CRC, 0, rd.crc() ^ kCrcXor);
  m.end_stream();
  m.rc_Quit();
  m.tell(kYieldStats);
  return 0;
}

// Decompresses with the model m, a new one, what is left of its input
// after the magic, the version and the level, into its output (m is the
// range decoder too, and the coroutine whose input it reads, coro.inc,
// which tells its driver what compress_stream() tells, its statistics
// s.nals): 0. An error ends the run at its trap (coro.inc), which writes
// what was decoded (s.buf), and the error is m's note.
template <class LV>
int decompress_stream(Model<1, LV>& m, DecompressState& s, const H264pArgs& a) {
  Buf<uint8_t>& buf = s.buf;
  Crc32& crc = s.crc;
  auto flush = [&]() {
    crc.add(buf);
    m.write(buf.data(), buf.size());
    buf.clear();
  };
  auto spill = [&]() {
    if (buf.size() > (1 << 16)) flush();
  };
  auto out = [&](uint8_t x) {
    buf.push_back(x);
    spill();
  };
  m.start();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  Context& ctx = s.ctx;
  decode_raw(m, F_JUNK, 0, out);
  int prev_type = 0;
  while (m.nal(F_MORE, 0, 0)) {
    s.nals++;
    int type = (int)cm_count(m.nal(F_TYPE, prev_type, 0), 32);
    if (type == 32) {
      put_start(buf, decode_count(m, F_ZEROS, 32), spill);
      decode_raw(m, F_LEN, 32, out);
      spill();
      continue;
    }
    int ref_idc = (int)cm_count(m.nal(F_REF, type, 0), 3);
    put_start(buf, decode_count(m, F_ZEROS, type), spill);
    buf.push_back((uint8_t)(ref_idc << 5 | type));
    prev_type = type;
    if ((type == 1 || type == 5) && m.nal(F_PADCHG, 0, 0)) {
      if (m.nal(F_PADON, 0, 0))
        ctx.pad.set((int64_t)cm_count(m.nal(F_PADFRAME, 0, 0), 31));
      else
        ctx.pad.on = false;
    }
    if (!parsed_type(type) || m.nal(F_VERB, type, 0)) {
      // its payload held while the NAL unit has at most kHold bytes (an SPS
      // or PPS then defines its parameter set), else put out as it comes
      Buf<uint8_t>& b = s.b;
      b.clear();
      bool held = true;
      decode_raw(m, F_LEN, type, [&](uint8_t x) {
        if (held && b.size() == kHold - 1) {  // (a long one: what is held goes out first)
          flush();
          crc.add(b);
          m.write(b.data(), b.size());
          held = false;
        }
        if (held)
          b.push_back(x);
        else
          out(x);
      });
      if (held) put_verbatim(buf, type, b, ctx, s.rbsp);
      ctx.done(type, b.data(), b.size());
    } else {
      size_t start = buf.size();
      ModelChan<1, LV> ch;
      ch.Init(m, nullptr);
      m.nal_begin(type, ref_idc);
      BitWriter& bw = s.bw;
      bw.clear();
      int pad = write_syntax(type, ref_idc, ctx, ch, bw, ch.policy());
      Trailer& t = s.t;
      t.reset();
      code_trailer(m, type, t);
      if (t.irregular && ((size_t)t.drop > bw.pos() || (bw.pos() - (size_t)t.drop + t.bits.size()) & 7))
        err_raise("corrupt stream: trailing bits");
      apply_trailer(t, bw, pad);
      const Buf<uint8_t>& e = bw.data();
      escape(e.data(), e.size(), buf);
      if (type == 1 || type == 5) tell_video(m, ctx);
      ctx.done(type, buf.data() + start, buf.size() - start);
    }
    spill();
    tell_tick(m, ctx);
  }
  put_zeros(buf, decode_count(m, F_EOFZ, 0), spill);
  m.end_stream();
  flush();
  if (m.nal(F_CRC, 0, 0) != (crc.value() ^ kCrcXor)) err_raise("CRC mismatch: the stream did not decompress correctly");
  if (!m.input_done()) err_raise("corrupt stream: data after its end");
  m.tell(kYieldStats);
  return 0;
}

}  // namespace h264p
