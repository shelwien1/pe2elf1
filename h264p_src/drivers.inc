// The calls c and d: their coroutines (coro.inc, mt.inc) run on files,
// with a progress line, at the level asked for or the stream's.

namespace h264p {

// c and d on files, or stdin and stdout for "-" (Lib3's stdio file API)

// c's coroutine on files: its input from f (what was read of it already,
// [pre, pre + npre), first), its output into g, and a look at the progress
// line after each yield; how it ended (drive())
template <class C>
uint run_file(C& c, filehandle f, filehandle g, Progress& pr, uint8_t* pre = nullptr, size_t npre = 0) {
  std::vector<byte> ib(1 << 16), ob(1 << 16);
  c.coro_init();
  c.addout(ob.data(), (uint)ob.size());
  if (npre) c.addinp(pre, (uint)npre);
  return drive(
      c,
      [&]() {
        uint l = f.sread(ib.data(), (uint)ib.size());
        if (!l) c.f_quit = 1;
        c.addinp(ib.data(), l);
      },
      [&](bool more) {
        uint l = c.getoutsize();
        if (l) g.writ(c.outbeg, l);
        if (more) c.addout(ob.data(), (uint)ob.size());
      },
      [&]() { pr.poll(); });
}

// A call's progress line, tagged tag, through the input f: shown with -p,
// or where stderr is a terminal
void progress_start(Progress& pr, const Options& opt, const char* tag, filehandle& f) {
#ifdef _WIN32
  bool tty = _isatty(_fileno(stderr)) != 0;
#else
  bool tty = isatty(fileno(stderr)) != 0;
#endif
  if (opt.progress || tty) pr.start(tag, file_size(f.f));
}

// (the model pass checks the dry runs where the input can be read again and
// the output written again: regular files)
bool can_check(const char* in_name, const Output& o) { return strcmp(in_name, "-") && is_regular(in_name) && o.file; }

// whether this build has level -level (else said)
bool has_level(int level) {
  if (std::find(std::begin(kLevels), std::end(kLevels), level) != std::end(kLevels)) return true;
  fprintf(stderr, "h264p: no level -%d in this build\n", level);
  return false;
}

// What the compressor's run of a call takes, and the decompressor's
H264pArgs enc_args(const Options& opt, const char* name, Progress* pr) {
  H264pArgs a;
  a.level = opt.level;
  a.verbose = opt.verbose;
  a.strict = opt.strict;
  a.name = name;
  a.meter = pr;
  return a;
}
H264pArgs dec_args(const Options& opt, const char* name, Progress* pr) {
  H264pArgs a;
  a.verbose = opt.verbose;
  a.name = name;
  a.meter = pr;
  return a;
}

// c of one model, H264pCoro<0, LV>, on all of the stream, and again
// without the model pass's checks if a slice did not come out as it is
template <class LV>
int code_one(const char* in_name, const Options& opt, filehandle& f, Output& o) {
  Progress pr;
  progress_start(pr, opt, "enc", f);
  H264pArgs a = enc_args(opt, in_name, &pr);
  a.check = can_check(in_name, o);
  for (;;) {
    int r;
    uint64_t raw, packed;
    {
      CoroObj<H264pCoro<0, LV>> c;  // (a new model each time)
      H264pCoro<0, LV>& e = *c.get();
      pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) { pos = raw = e.n_in, packed = e.n_out; };
      e.Init(&a);
      run_file(e, f, o.g, pr);
      pr.what = nullptr;
      r = e.status;
      raw = e.n_in, packed = e.n_out;
    }
    if (r != kRestart) {
      pr.end(r, raw, packed);
      return close_io(r, f, o, false);
    }
    a.check = false;
    if (!o.rewind() || fseek(f.f, 0, SEEK_SET)) {
      fprintf(stderr, "h264p: cannot start again\n");
      return close_io(1, f, o, false);
    }
  }
}
template <class LV>
struct CodeLv {
  static int run(const char* in_name, const Options& opt, filehandle& f, Output& o) {
    return code_one<LV>(in_name, opt, f, o);
  }
};

// what MTwrap's Init() returned, said (false: nothing to say)
bool mt_init_failed(uint r, int threads) {
  if (r == 7)
    fprintf(stderr, "h264p: cannot start %d threads\n", threads);
  else if (r)
    fprintf(stderr, "h264p: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// c -t N: the stream in parts on N threads (mt.inc), after h264p's header
int code_mt(const char* in_name, const Options& opt, filehandle& f, Output& o) {
  Progress pr;
  progress_start(pr, opt, "enc", f);
  H264pArgs a = enc_args(opt, in_name, &pr);
  a.verbose = false;  // (no statistics for each part)
  CoroObj<MtEnc> c;
  MtEnc& e = *c.get();
  pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) {
    mt_done(e.M2, raw, packed);
    pos = raw = pr.total ? std::min(raw, pr.total) : raw;  // (parts have their parameter sets copied in)
    packed += kMtHead;
  };
  e.M1.Init(opt.part, 1, mt_cap(opt.part));
  if (mt_init_failed(e.M2.Init(opt.threads, kMtRing, &a, 1), opt.threads))
    return close_io(1, f, o, false);  // (threads already started end with the process)
  uint8_t head[kMtHead];
  mt_head(head, opt.threads, opt.part);
  fwrite(head, 1, sizeof head, o.g.f);
  int r = run_file(e, f, o.g, pr) || mt_failed(e.M2);
  mt_drain(e.M2);  // (the coder's threads, where the splitter stopped early)
  bool said = mt_failed(e.M2);
  MtCounts n = mt_counts(e);
  e.M2.Quit();  // (the threads joined)
  e.M1.Quit();
  pr.end(r, n.raw, n.packed);
  if (r && !said) fprintf(stderr, "h264p: %s: internal: a thread stopped early\n", in_name);
  if (!r && opt.verbose)
    fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " parts on %d threads\n", n.raw, n.packed,
            n.raw ? 100.0 * n.packed / n.raw : 0.0, n.parts, opt.threads);
  return close_io(r, f, o, false);
}

// H264P_RECDUMP (recon.inc) is the reconstruction of one model: whether the
// call runs one (else said)
bool recdump_ok(bool one) {
  if (one || !getenv("H264P_RECDUMP")) return true;
  fprintf(stderr, "h264p: H264P_RECDUMP takes one model: c or d without -t\n");
  return false;
}

// c
int code(const char* in_name, const char* out_name, const Options& opt) {
  if (!has_level(opt.level) || !recdump_ok(opt.threads == 1)) return 2;
#ifdef H264P_DUMP
  if (opt.threads > 1) {  // (the parts' models would write the dumps at once)
    fprintf(stderr, "h264p: a build with H264P_DUMP dumps one model: without -t\n");
    return 2;
  }
#endif
  filehandle f;
  Output o;
  if (!open_io(in_name, out_name, f, o)) return 1;
  if (opt.threads > 1) return code_mt(in_name, opt, f, o);
  return with_level<CodeLv>(opt.level, in_name, opt, f, o);
}

// d of one model's stream, at its level: the bytes of the input read
// already (head, got of them) first
template <class LV>
struct DecodeLv {
  static int run(const char* in_name, const Options& opt, filehandle& f, Output& o, uint8_t* head, size_t got) {
    Progress pr;
    progress_start(pr, opt, "dec", f);
    H264pArgs a = dec_args(opt, in_name, &pr);
    int r;
    uint64_t raw, packed;
    {
      CoroObj<H264pCoro<1, LV>> d;
      pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) { raw = d->n_out, pos = packed = d->n_in; };
      d->Init(&a);
      run_file(*d.get(), f, o.g, pr, head, got);
      pr.what = nullptr;
      r = d->status;
      raw = d->n_out, packed = d->n_in;
    }
    pr.end(r, raw, packed);
    return close_io(r, f, o, raw > 0);  // (what was decoded before an error)
  }
};

// d of the parts on threads: as many threads as compressed them
int decode_mt(const char* in_name, const Options& opt, filehandle& f, Output& o, const uint8_t* head, size_t got) {
  int threads = got == (size_t)kMtHead ? head[sizeof kMtMagic] : 0;
  if (!recdump_ok(false)) return close_io(1, f, o, false);
  if (got > sizeof kMtMagic - 1 && head[sizeof kMtMagic - 1] != kCmVersion) {
    fprintf(stderr, "h264p: %s: compressed by version %d, this is version %d\n", in_name, (int)head[sizeof kMtMagic - 1],
            kCmVersion);
    return close_io(1, f, o, false);
  }
  if (threads < 1 || threads > 64) {
    fprintf(stderr, "h264p: %s: a broken or truncated stream\n", in_name);
    return close_io(1, f, o, false);
  }
  Progress pr;
  progress_start(pr, opt, "dec", f);
  H264pArgs a = dec_args(opt, in_name, &pr);
  a.verbose = false;  // (no statistics for each part)
  CoroObj<MtDec> d;
  pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) {
    mt_done(*d.get(), packed, raw);
    pos = packed += kMtHead;
  };
  if (mt_init_failed(d->Init(threads, kMtRing, &a, 1), threads)) return close_io(1, f, o, false);
  int r = run_file(*d.get(), f, o.g, pr) || mt_failed(*d.get());
  mt_drain(*d.get());
  bool said = mt_failed(*d.get());
  MtCounts n = mt_counts(*d.get());
  d->Quit();
  pr.end(r, n.raw, n.packed);
  if (r && !said) fprintf(stderr, "h264p: %s: a broken or truncated stream\n", in_name);
  if (!r && opt.verbose) fprintf(stderr, "%" PRIu64 " bytes, %" PRIu64 " parts on %d threads\n", n.raw, n.parts, threads);
  return close_io(r, f, o, n.raw > 0);
}

// d: the first bytes say which it is, a stream of one model (and its level)
// or of parts on threads (kMtMagic)
int decode(const char* in_name, const char* out_name, const Options& opt) {
  filehandle f;
  Output o;
  if (!open_io(in_name, out_name, f, o)) return 1;
  uint8_t head[kMtHead];
  size_t got = fread(head, 1, sizeof head, f.f);
  if (got >= sizeof kMtMagic - 1 && !memcmp(head, kMtMagic, sizeof kMtMagic - 1))
    return decode_mt(in_name, opt, f, o, head, got);
  int level = cm_level(head, got, in_name);
  int r = level < 0 ? 1 : with_level<DecodeLv>(level, in_name, opt, f, o, head, got);
  if (r >= 0) return r;
  fprintf(stderr, "h264p: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return close_io(1, f, o, false);
}

}  // namespace h264p
