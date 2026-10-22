// The calls c and d: their coroutines (coro.inc, mt.inc) run on files,
// with a progress line, at the level asked for or the stream's.

namespace h264p {

// c and d on files, or stdin and stdout for "-" (Lib3's stdio file API)

// c's coroutine on files: its input from f (what was read of it already,
// [pre, pre + npre), first), its output into g, and show(code) after each
// yield (what c tells; the progress line); how it ended (drive())
template <class C, class Show>
uint run_file(C& c, filehandle0 f, filehandle0 g, Show show, uint8_t* pre = nullptr, size_t npre = 0) {
  const uint kIo = 1 << 16;  // the size of the input buffer and of the output buffer
  byte* ib = (byte*)mem_alloc(2 * kIo);
  byte* ob = ib + kIo;
  c.coro_init();
  c.addout(ob, kIo);
  if (npre) c.addinp(pre, (uint)npre);
  uint r = drive(
      c,
      [&]() {
        uint l = f.sread(ib, kIo);
        if (!l) c.f_quit = 1;
        c.addinp(ib, l);
      },
      [&](bool more) {
        uint l = c.getoutsize();
        if (l) g.writ(c.outbeg, l);
        if (more) c.addout(ob, kIo);
      },
      show);
  mem_free(ib);
  return r;
}

// A call's progress line, tagged tag, through the input f: shown with -p,
// or where stderr is a terminal
void progress_start(Progress& pr, const Options& opt, const char* tag, filehandle0& f) {
#ifdef _WIN32
  bool tty = _isatty(_fileno(stderr)) != 0;
#else
  bool tty = isatty(fileno(stderr)) != 0;
#endif
  if (opt.progress || tty) pr.start(tag, file_size(f.f));
}

// (the model pass checks the dry runs where the input can be read again and
// the output written again: regular files)
bool can_check(const char* in_name, const Output& o) { return strcmp(in_name, "-") && name_kind(in_name) == 1 && o.file; }

// whether this build has level -level (else said)
bool has_level(int level) {
  for (int l : kLevels)
    if (l == level) return true;
  fprintf(stderr, "h264p: no level -%d in this build\n", level);
  return false;
}

// What a run of a call takes (decompressing, it reads the level from the
// stream, and -s has nothing to do), its progress line pr started (tag)
// on the input f
H264pArgs call_start(Progress& pr, const Options& opt, const char* tag, filehandle0& f, const char* name) {
  pr.Init();
  progress_start(pr, opt, tag, f);
  H264pArgs a;
  a.Init();
  a.level = opt.level;
  a.verbose = opt.verbose;
  a.strict = opt.strict;
  a.name = name;
  a.meter = &pr;
  return a;
}
// and its end, r its status: the output committed (close_io(); keep: what
// was decoded stays after an error), then pr's line ended, done or not;
// the exit status
int call_end(int r, filehandle0& f, Output& o, bool keep, Progress& pr, uint64_t raw, uint64_t packed, uint64_t pics) {
  r = close_io(r, f, o, keep, &pr);
  pr.end(r, raw, packed, pics);
  pr.Quit();
  return r;
}

// A run of one model, C (H264pCoro<DEC, LV>), on the call's files, its
// input from f (what was read of it already, [pre, pre + npre), first):
// its status, the stream's bytes (raw) and the compressed stream's
// (packed), and its pictures; its counts on the progress line as it goes,
// and what it tells shown (show_told())
template <class C, int DEC>
void model_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& pics) {
  const C& m = *(const C*)c;
  pos = m.n_in;
  raw = DEC ? m.n_out : m.n_in;
  packed = DEC ? m.n_in : m.n_out;
  pics = m.pictures;
}
template <class C, int DEC>
int run_model(const H264pArgs& a, Progress& pr, filehandle0& f, Output& o, uint8_t* pre, size_t npre, uint64_t& raw,
              uint64_t& packed, uint64_t& pics) {
  C* m = coro_new<C>();
  pr.counts(model_counts<C, DEC>, m);
  m->Init(&a);
  run_file(
      *m, f, o.g,
      [&](uint code) {
        show_told(*m, code, &pr, a);
        pr.poll();
      },
      pre, npre);
  pr.counts(nullptr, nullptr);
  int r = m->status;
  uint64_t pos;
  model_counts<C, DEC>(m, raw, packed, pos, pics);
  if (a.verbose)
    say(&pr, "coroutine: %zu bytes of stack copied out at a yield, at most (room for %d)\n", m->depth,
        (int)Coroutine::STKPAD);
  m->Quit();
  coro_free(m);
  return r;
}

// c of one model, on all of the stream, and again (a new model) without
// the model pass's checks if a slice did not come out as it is
template <class LV>
struct CodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o) {
    Progress pr;
    H264pArgs a = call_start(pr, opt, "enc", f, in_name);
    a.check = can_check(in_name, o);
    uint64_t raw, packed, pics;
    int r;
    while ((r = run_model<H264pCoro<0, LV>, 0>(a, pr, f, o, nullptr, 0, raw, packed, pics)) == kRestart) {
      a.check = false;
      if (!o.rewind() || fseek(f.f, 0, SEEK_SET)) {
        say_error(&pr, "h264p: cannot start again\n");
        r = 1;
        break;
      }
    }
    return call_end(r, f, o, false, pr, raw, packed, pics);
  }
};
// d of one model's stream, at its level: the bytes of the input read
// already (head, got of them) first
template <class LV>
struct DecodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o, uint8_t* head, size_t got) {
    Progress pr;
    H264pArgs a = call_start(pr, opt, "dec", f, in_name);
    uint64_t raw, packed, pics;
    int r = run_model<H264pCoro<1, LV>, 1>(a, pr, f, o, head, got, raw, packed, pics);
    return call_end(r, f, o, raw > 0, pr, raw, packed, pics);  // (what was decoded before an error)
  }
};

// what MTwrap's Init() returned, said (false: nothing to say)
bool mt_init_failed(uint r, int threads, Progress& pr) {
  if (r == 7)
    say_error(&pr, "h264p: cannot start %d threads\n", threads);
  else if (r)
    say_error(&pr, "h264p: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// (the counts of a run on threads on the progress line, of all its
// workers: pr's MtEnc, MtDec)
struct MtProg {
  Progress* pr;
  void* m;
};
void mt_enc_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& pics) {
  MtProg& p = *(MtProg*)c;
  MtWork w = mt_work(((MtEnc*)p.m)->M2);
  pos = raw = p.pr->total ? Min(w.in, p.pr->total) : w.in;  // (parts have their parameter sets copied in)
  packed = kMtHead + w.out;
  pics = w.pics;
}
void mt_dec_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& pics) {
  MtWork w = mt_work(*(MtDec*)((MtProg*)c)->m);
  raw = w.out;
  pos = packed = kMtHead + w.in;
  pics = w.pics;
}

// c -t N: the stream in parts on N threads (mt.inc), after h264p's header
int code_mt(const char* in_name, const Options& opt, filehandle0& f, Output& o) {
  Progress pr;
  H264pArgs a = call_start(pr, opt, "enc", f, in_name);
  a.verbose = false;  // (no statistics for each part)
  MtEnc* e = coro_new<MtEnc>();
  MtProg mp = {&pr, e};
  pr.counts(mt_enc_counts, &mp);
  e->M1.Init(opt.part, 1, mt_cap(opt.part));
  if (mt_init_failed(e->M2.Init(opt.threads, kMtRing, &a, 1), opt.threads, pr)) {
    e->M1.Quit();  // (threads already started wait on input: they end with the process, e as they have it)
    return call_end(1, f, o, false, pr, 0, 0, 0);
  }
  uint8_t head[kMtHead];
  mt_head(head, opt.threads, opt.part);
  fwrite(head, 1, sizeof head, o.g.f);
  int r = run_file(*e, f, o.g, [&](uint) { pr.poll(); }) || mt_failed(e->M2);
  if (r && !mt_failed(e->M2)) say_error(&pr, "h264p: %s: internal: a thread stopped early\n", in_name);
  MtCounts n = mt_counts(*e);
  e->M2.Quit();  // (the threads joined)
  e->M1.Quit();
  pr.counts(nullptr, nullptr);
  coro_free(e);
  r = call_end(r, f, o, false, pr, n.raw, n.packed, n.pics);
  if (!r && opt.verbose)
    fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " parts on %d threads\n", n.raw, n.packed,
            n.raw ? 100.0 * n.packed / n.raw : 0.0, n.parts, opt.threads);
  return r;
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
  filehandle0 f;
  Output o;
  o.Init();
  if (!open_io(in_name, out_name, f, o)) return 1;
  if (opt.threads > 1) return code_mt(in_name, opt, f, o);
  return with_level<CodeLv>(opt.level, in_name, opt, f, o);
}

// d of the parts on threads: as many threads as compressed them
int decode_mt(const char* in_name, const Options& opt, filehandle0& f, Output& o, const uint8_t* head, size_t got) {
  int threads = got == (size_t)kMtHead ? head[sizeof kMtMagic] : 0;
  if (!recdump_ok(false)) return close_io(1, f, o, false);
  char msg[512];
  if (got > sizeof kMtMagic - 1 && !cm_version_ok(head[sizeof kMtMagic - 1], in_name, msg, sizeof msg)) {
    fputs(msg, stderr);
    return close_io(1, f, o, false);
  }
  if (threads < 1 || threads > 64) {
    fprintf(stderr, "h264p: %s: a broken or truncated stream\n", in_name);
    return close_io(1, f, o, false);
  }
  Progress pr;
  H264pArgs a = call_start(pr, opt, "dec", f, in_name);
  a.verbose = false;  // (no statistics for each part)
  MtDec* d = coro_new<MtDec>();
  MtProg mp = {&pr, d};
  pr.counts(mt_dec_counts, &mp);
  if (mt_init_failed(d->Init(threads, kMtRing, &a, 1), threads, pr)) return call_end(1, f, o, false, pr, 0, 0, 0);
  int r = run_file(*d, f, o.g, [&](uint) { pr.poll(); }) || mt_failed(*d);
  if (r && !mt_failed(*d)) say_error(&pr, "h264p: %s: a broken or truncated stream\n", in_name);
  MtCounts n = mt_counts(*d);
  d->Quit();
  pr.counts(nullptr, nullptr);
  coro_free(d);
  r = call_end(r, f, o, n.raw > 0, pr, n.raw, n.packed, n.pics);  // (what was decoded before an error)
  if (!r && opt.verbose) fprintf(stderr, "%" PRIu64 " bytes, %" PRIu64 " parts on %d threads\n", n.raw, n.parts, threads);
  return r;
}

// d: the first bytes say which it is, a stream of one model (and its level)
// or of parts on threads (kMtMagic)
int decode(const char* in_name, const char* out_name, const Options& opt) {
  filehandle0 f;
  Output o;
  o.Init();
  if (!open_io(in_name, out_name, f, o)) return 1;
  uint8_t head[kMtHead];
  size_t got = fread(head, 1, sizeof head, f.f);
  if (got >= sizeof kMtMagic - 1 && !memcmp(head, kMtMagic, sizeof kMtMagic - 1))
    return decode_mt(in_name, opt, f, o, head, got);
  char msg[512];
  int level = cm_level(head, got, in_name, msg, sizeof msg);
  if (level < 0) {
    fputs(msg, stderr);
    return close_io(1, f, o, false);
  }
  int r = with_level<DecodeLv>(level, in_name, opt, f, o, head, got);
  if (r != kNoLevel) return r;
  fprintf(stderr, "h264p: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return close_io(1, f, o, false);
}

}  // namespace h264p
