// The calls c and d: their coroutines (coro.inc) run on files, with a
// progress line, at the level asked for or the stream's (h264p's
// drivers.inc, but for its threads).

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
  a.level = opt.level;
  a.verbose = opt.verbose;
  a.strict = opt.strict;
  a.format = opt.format;
  a.asc = opt.asc_n ? opt.asc : nullptr;
  a.asc_n = opt.asc_n;
  a.name = name;
  a.meter = &pr;
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

// A run of one model, C (AacpCoro<DEC, LV>), on the call's files, its input
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

// c of one model
template <class LV>
struct CodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o) {
    Progress pr;
    AacArgs a = call_start(pr, opt, "enc", f, in_name);
    uint64_t raw, packed, frames;
    int r = run_model<AacpCoro<0, LV>, 0>(a, pr, f, o, nullptr, 0, raw, packed, frames);
    return call_end(r, f, o, false, pr, raw, packed, frames);
  }
};
// d of one model's stream, at its level: the bytes of the input read
// already (head, got of them) first
template <class LV>
struct DecodeLv {
  static int run(const char* in_name, const Options& opt, filehandle0& f, Output& o, uint8_t* head, size_t got) {
    Progress pr;
    AacArgs a = call_start(pr, opt, "dec", f, in_name);
    uint64_t raw, packed, frames;
    int r = run_model<AacpCoro<1, LV>, 1>(a, pr, f, o, head, got, raw, packed, frames);
    return call_end(r, f, o, raw > 0, pr, raw, packed, frames);  // (what was decoded before an error)
  }
};

// c
int code(const char* in_name, const char* out_name, const Options& opt) {
  if (!has_level(opt.level)) return 2;
  filehandle0 f;
  Output o;
  o.Init();
  if (!open_io(in_name, out_name, f, o)) return 1;
  return with_level<CodeLv>(opt.level, in_name, opt, f, o);
}

// d: the first bytes say its level
int decode(const char* in_name, const char* out_name, const Options& opt) {
  filehandle0 f;
  Output o;
  o.Init();
  if (!open_io(in_name, out_name, f, o)) return 1;
  uint8_t head[sizeof kCmMagic + 1];
  size_t got = fread(head, 1, sizeof head, f.f);
  char msg[512];
  int level = cm_level(head, got, in_name, msg, sizeof msg);
  if (level < 0) {
    fputs(msg, stderr);
    return close_io(1, f, o, false);
  }
  int r = with_level<DecodeLv>(level, in_name, opt, f, o, head, got);
  if (r != kNoLevel) return r;
  fprintf(stderr, "aacpack: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return close_io(1, f, o, false);
}

}  // namespace aacpack
