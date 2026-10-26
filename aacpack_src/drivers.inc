// The calls c and d: their coroutines (coro.inc) run on files, with a
// progress line, at the level asked for or the stream's, and on threads
// (mt.inc) with -t (h264p's drivers.inc).

namespace aacpack {

// a codec's coroutine on files: its input from f (what was read of it
// already, [pre, pre + npre), first), its output into g, and show(code)
// after each yield (what it tells; the progress line); how it ended
// (drive())
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
        if (!l) c.f_quit = 1;  // (the end: a 0-length buffer, and f_quit, which MTwrap reads)
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
// or where stderr is a terminal; and with -v, what the stream is
void progress_start(Progress& pr, const Options& opt, const char* tag, filehandle0& f) {
#ifdef _WIN32
  bool tty = _isatty(_fileno(stderr)) != 0;
#else
  bool tty = isatty(fileno(stderr)) != 0;
#endif
  if (opt.progress || tty) pr.start(tag, file_size(f.f));
  pr.verbose = opt.verbose;
}

// whether this build has level -level (else said)
bool has_level(int level) {
  for (int l : kLevels)
    if (l == level) return true;
  fprintf(stderr, "aacpack: no level -%d in this build\n", level);
  return false;
}

// What a run of a call takes, its progress line pr started (tag) on the input f
AacArgs call_start(Progress& pr, const Options& opt, const char* tag, filehandle0& f, const char* name) {
  pr.Init();
  progress_start(pr, opt, tag, f);
  AacArgs a;
  a.Init();
  a.verbose = opt.verbose;
  a.strict = opt.strict;
  a.format = opt.format;
  a.asc = opt.asc_n ? opt.asc : nullptr;
  a.asc_n = opt.asc_n;
  a.name = name;
  a.level = opt.level;
  a.order = opt.order;
  a.meter = &pr;
  a.threads = opt.threads;
  return a;
}
// and its end, r its status: the output committed (close_io(); keep: what
// was decoded stays after an error), then pr's line ended, done or not;
// the exit status
int call_end(int r, filehandle0& f, Output& o, bool keep, Progress& pr, uint64_t raw, uint64_t packed, uint64_t frames) {
  r = close_io(r, f, o, keep, &pr);
  pr.end(r, raw, packed, frames);
  pr.Quit();
  return r;
}

// A run of one model, C (AacpCoro<DEC, LV, CS>), on the call's files, its input
// from f (what was read of it already, [pre, pre + npre), first): its
// status, the stream's bytes (raw) and the compressed stream's (packed),
// and its frames; its counts on the progress line as it goes, and what it
// tells shown (show_told())
template <class C, int DEC>
void model_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& frames) {
  const C& m = *(const C*)c;
  pos = m.n_in;
  raw = DEC ? m.n_out : m.n_in;
  packed = DEC ? m.n_in : m.n_out;
  frames = m.frames;
}
template <class C, int DEC>
int run_model(const AacArgs& a, Progress& pr, filehandle0& f, Output& o, uint8_t* pre, size_t npre, uint64_t& raw,
              uint64_t& packed, uint64_t& frames) {
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
  model_counts<C, DEC>(m, raw, packed, pos, frames);
  if (a.verbose)
    say(&pr, "coroutine: %zu bytes of stack copied out at a yield, at most (room for %d)\n", m->depth,
        (int)Coroutine::STKPAD);
  m->Quit();
  coro_free(m);
  return r;
}

// c (DEC 0) or d of one model, at the level LV and in the order (1 the
// section data after the spectra): the bytes of the input read already
// (head, got of them) first
template <class LV, int DEC>
int run_lv(const char* in_name, const Options& opt, filehandle0& f, Output& o, uint8_t* head, size_t got, int order) {
  Progress pr;
  AacArgs a = call_start(pr, opt, DEC ? "dec" : "enc", f, in_name);
  uint64_t raw, packed, frames;
  int r = order ? run_model<AacpCoro<DEC, LV, 1>, DEC>(a, pr, f, o, head, got, raw, packed, frames)
                : run_model<AacpCoro<DEC, LV, 0>, DEC>(a, pr, f, o, head, got, raw, packed, frames);
  return call_end(r, f, o, DEC && raw > 0, pr, raw, packed, frames);  // (d: what was decoded before an error)
}
template <class LV>
struct CodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o) {
    return run_lv<LV, 0>(in_name, opt, f, o, nullptr, 0, opt.order);
  }
};
template <class LV>
struct DecodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o, uint8_t* head, size_t got, int order) {
    return run_lv<LV, 1>(in_name, opt, f, o, head, got, order);
  }
};

// what MTwrap's Init() returned, said (false: nothing to say)
bool mt_init_failed(uint r, int threads, Progress& pr) {
  if (r == 7)
    say_error(&pr, "aacpack: cannot start %d threads\n", threads);
  else if (r)
    say_error(&pr, "aacpack: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// (the counts of a run on threads on the progress line, of all its
// workers: pr's MtEnc, MtDec)
struct MtProg {
  Progress* pr;
  void* m;
};
void mt_enc_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& frames) {
  MtWork w = mt_work(((MtEnc*)((MtProg*)c)->m)->M2);
  pos = raw = w.in;
  packed = kMtHead + w.out;
  frames = w.frames;
}
void mt_dec_counts(void* c, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& frames) {
  MtWork w = mt_work(*(MtDec*)((MtProg*)c)->m);
  raw = w.out;
  pos = packed = kMtHead + w.in;
  frames = w.frames;
}

// c -t N: the stream in parts on N threads (mt.inc), after aacpack's header
int code_mt(const char* in_name, const Options& opt, filehandle0& f, Output& o) {
  Progress pr;
  AacArgs a = call_start(pr, opt, "enc", f, in_name);
  a.verbose = false;  // (no statistics for each part)
  MtEnc* e = coro_new<MtEnc>();
  MtProg mp = {&pr, e};
  pr.counts(mt_enc_counts, &mp);
  e->M1.Init(opt.part, mt_cap(opt.part), &a);
  if (mt_init_failed(e->M2.Init(opt.threads, kMtRing, &a, 1), opt.threads, pr)) {
    e->M1.Quit();  // (threads already started wait on input: they end with the process, e as they have it)
    return call_end(1, f, o, false, pr, 0, 0, 0);
  }
  uint8_t head[kMtHead];
  mt_head(head, opt.threads, opt.part);
  o.g.writ(head, sizeof head);
  int r = run_file(*e, f, o.g, [&](uint) { pr.poll(); }) || mt_failed(e->M2);
  if (r && !mt_failed(e->M2)) say_error(&pr, "aacpack: %s: internal: a thread stopped early\n", in_name);
  MtCounts n = mt_counts(*e);
  uint64_t cuts[AacSplit::CUT_N];
  memcpy(cuts, e->M1.st->cuts, sizeof cuts);
  e->M2.Quit();  // (the threads joined)
  e->M1.Quit();
  pr.counts(nullptr, nullptr);
  coro_free(e);
  r = call_end(r, f, o, false, pr, n.raw, n.packed, n.frames);
  if (!r && opt.verbose)
    fprintf(stderr,
            "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " parts on %d threads: %" PRIu64
            " cut before a unit that parses the same at a stream's start, %" PRIu64 " at a unit past the cap, %" PRIu64
            " in bytes\n",
            n.raw, n.packed, n.raw ? 100.0 * n.packed / n.raw : 0.0, n.parts, opt.threads, cuts[AacSplit::CUT_CLEAN],
            cuts[AacSplit::CUT_CAP], cuts[AacSplit::CUT_BYTES]);
  return r;
}

// c
int code(const char* in_name, const char* out_name, const Options& opt) {
  if (!has_level(opt.level)) return 2;
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
  char msg[512];
  if (got > sizeof kMtMagic - 1 && !cm_version_ok(head[sizeof kMtMagic - 1], in_name, msg, sizeof msg)) {
    fputs(msg, stderr);
    return close_io(1, f, o, false);
  }
  if (threads < 1 || threads > 64) {
    fprintf(stderr, "aacpack: %s: a broken or truncated stream\n", in_name);
    return close_io(1, f, o, false);
  }
  Progress pr;
  AacArgs a = call_start(pr, opt, "dec", f, in_name);
  a.verbose = false;  // (no statistics for each part)
  a.part_max = mt_part_max((get_le(head + sizeof kMtMagic + 1, 4) + 1) << 10);  // (the part size in KB, rounded down)
  a.threads = threads;
  MtDec* d = coro_new<MtDec>();
  MtProg mp = {&pr, d};
  pr.counts(mt_dec_counts, &mp);
  if (mt_init_failed(d->Init(threads, kMtRing, &a, 1), threads, pr)) return call_end(1, f, o, false, pr, 0, 0, 0);
  int r = run_file(*d, f, o.g, [&](uint) { pr.poll(); }) || mt_failed(*d) || !mt_dealt(*d);
  if (r && !mt_failed(*d)) say_error(&pr, "aacpack: %s: a broken or truncated stream\n", in_name);
  MtCounts n = mt_counts(*d);
  d->Quit();
  pr.counts(nullptr, nullptr);
  coro_free(d);
  r = call_end(r, f, o, n.raw > 0, pr, n.raw, n.packed, n.frames);  // (what was decoded before an error)
  if (!r && opt.verbose) fprintf(stderr, "%" PRIu64 " bytes, %" PRIu64 " parts on %d threads\n", n.raw, n.parts, threads);
  return r;
}

// d: the first bytes say which it is, a stream of one model (and its level
// and order) or of parts on threads (kMtMagic)
int decode(const char* in_name, const char* out_name, const Options& opt) {
  filehandle0 f;
  Output o;
  o.Init();
  if (!open_io(in_name, out_name, f, o)) return 1;
  uint8_t head[kMtHead];
  static_assert(sizeof head >= sizeof kCmMagic + 2, "the header read ahead holds a stream's of one model");
  size_t got = fread(head, 1, sizeof kCmMagic + 2, f.f);
  if (got >= sizeof kMtMagic - 1 && !memcmp(head, kMtMagic, sizeof kMtMagic - 1)) {
    got += fread(head + got, 1, sizeof head - got, f.f);
    return decode_mt(in_name, opt, f, o, head, got);
  }
  char msg[512];
  int order = 1;
  int level = cm_level(head, got, in_name, msg, sizeof msg, &order);
  if (level < 0) {
    fputs(msg, stderr);
    return close_io(1, f, o, false);
  }
  int r = with_level<DecodeLv>(level, in_name, opt, f, o, head, got, order);
  if (r != kNoLevel) return r;
  fprintf(stderr, "aacpack: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return close_io(1, f, o, false);
}

}  // namespace aacpack
