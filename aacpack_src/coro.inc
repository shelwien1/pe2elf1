// aacpack's compressor (AacpCoro<0, LV, CS>) and decompressor (AacpCoro<1, LV, CS>)
// of a level and an order (CS: Model's f_CS) as coroutines of Lib3 (coro3b.inc), in its buffer protocol
// (h264p's coro.inc): the input comes in the buffers addinp() gives, a
// 0-length one with f_quit set at its end, and yield 1 asks for the next;
// the output goes into the buffer addout() gave, yield 2 handing it over
// full; yield 0 at the end (the last output in the buffer), yield 3 after
// an error (its note says it). What else it yields with tells its driver
// what to show (rcio.inc): a codec prints nothing. So drivers.inc's
// run_file() runs one on files (c, d), showing what it tells (show_told(),
// below).
//
// The coroutine is the model: AacpCoro derives from Model, the model from
// the range coder (fam.inc's tc_rc) and the coder from the coroutine's I/O
// (rcio.inc), whose buffers its bytes go through. A run is an object of its
// own, and shares nothing with another. One is made for a run (coro_new(),
// Init()), and its model's tables are mapped as the run starts
// (Model::start()); the stream's state (the walk, the window, its buffers)
// is allocated then too and freed as the run ends, or by Quit() wherever
// the coroutine was left, which frees the model's too (coro_free() after).

namespace aacpack {

template <int f_DEC, class LV, int f_CS>
struct AacpCoro : Model<f_DEC, LV, f_CS> {
  typedef typename Pick<f_DEC != 0, DecompressState, CompressState>::type State;

  AacArgs args;
  State* st;
  int status;  // the run's: 0, 1 (an error); 1 until it has ended

  uint32_t Init(const AacArgs* a) {
    this->coro_init();
    Model<f_DEC, LV, f_CS>::Init();
    args = *a;
    st = nullptr;
    status = 1;
    return 0;
  }
  void Quit() {
    if (st) st->Quit();
    mem_free(st);
    st = nullptr;
    Model<f_DEC, LV, f_CS>::Quit();
  }

  // The run, in a trap (err.inc) that an error ends it at: its note says
  // why, after what was decoded is written (d keeps it). The coroutine
  // yields from here, its stack shallow again.
  void do_process() {
    int r;
    this->io_init();
    ErrTrap t;
    if (!setjmp(t.jb)) {
      err_push(t);
      r = run(IntTag<f_DEC>());
      err_pop(t);
    } else {
      r = 1;
      if (st) written(IntTag<f_DEC>());
      this->set_note("aacpack: %s: %s", args.name, t.why);
    }
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? kYieldFail : kYieldEnd);
  }

  // compressing
  int run(IntTag<0>) {
    st = (CompressState*)mem_alloc(sizeof(CompressState));
    st->Init(this->source());
    return compress_stream<LV, f_CS>(*this, *st, args);
  }
  // decompressing: the magic, the version, the level and the order (this
  // model's), then the model on the rest
  int run(IntTag<1>) {
    st = (DecompressState*)mem_alloc(sizeof(DecompressState));
    st->Init();
    uint8_t head[sizeof kCmMagic + 2];
    int order = -1;
    int level = cm_level(head, this->read(head, sizeof head), args.name, this->note, sizeof this->note, &order);
    if (level < 0) return 1;
    if (level != LV::id || order != f_CS) {
      this->set_note("aacpack: %s: compressed at level -%d in order %d, not -%d in %d", args.name, level, order, LV::id, f_CS);
      return 1;
    }
    return decompress_stream<LV, f_CS>(*this, *st, args);
  }
  // after an error: decompressing, what was decoded, written
  void written(IntTag<0>) {}
  void written(IntTag<1>) { this->write(st->buf.data(), st->buf.size()), st->buf.clear(); }
};

// The coroutine of a call of c or d, on pages of its own -- zeroed, and
// aligned as Coroutine's members ask: made by coro_new() (plain data:
// Init() sets it up), and after its Quit() freed by coro_free()
template <class T>
T* coro_new() {
  static_assert(__is_trivially_constructible(T), "a coroutine is plain data, not constructed");
  return (T*)tc_map(sizeof(T), "a coroutine");
}
template <class T>
void coro_free(T* p) {
  tc_unmap(p, sizeof(T));
}

// Runs the coroutine c in the buffer protocol to its end: in() gives it
// its next input when it asks for it (kYieldIn), out(more) takes what it
// put in its output buffer when that is full (kYieldOut, more: and then
// gives it the next) or as it ends (kYieldEnd, or kYieldFail after an
// error: what drive() returns), and show(code) comes after each yield
// (what c tells, rcio.inc, and the progress line). The caller's traps
// (err.inc) are put aside while c runs, so that c's are not set on them.
template <class C, class In, class Out, class Show>
uint drive(C& c, In in, Out out, Show show) {
  for (;;) {
    ErrTrap* t = err_hold();
    uint r = c.coro_call(&c);
    err_resume(t);
    show(r);
    if (r == kYieldIn) {
      in();
      continue;
    }
    if (r >= kYieldAudio) continue;  // (told)
    out(r == kYieldOut);
    if (r != kYieldOut) return r;
  }
}

// -v's statistics of a run of codec c, at its end (kYieldStats), after the
// progress line pr
template <class LV, int f_CS>
void show_stats(AacpCoro<0, LV, f_CS>& c, Progress* pr) {
  c.st->st.report(pr);
  cm_report(c, c.n_in, c.n_out, pr);
}
template <class LV, int f_CS>
void show_stats(AacpCoro<1, LV, f_CS>& c, Progress* pr) {
  say(pr, "%" PRIu64 " units\n", c.st->units);
}
// What a driver shows of what its codec c, of a run with args a, tells
// (code, each yield), after the progress line pr: the stream, a message,
// the statistics (-v); and as the run ends, its error
template <int f_DEC, class LV, int f_CS>
void show_told(AacpCoro<f_DEC, LV, f_CS>& c, uint code, Progress* pr, const AacArgs& a) {
  if (code == kYieldAudio) {
    if (pr) pr->audio(c.audio);
  } else if (code == kYieldNote) {
    say(pr, "%s", c.note);
  } else if (code == kYieldFail && c.status == 1) {
    say_error(pr, "%s", c.note);
  } else if (code == kYieldStats && a.verbose) {
    show_stats(c, pr);
  }
}

}  // namespace aacpack
