// h264p's compressor (H264pCoro<0, LV>) and decompressor (H264pCoro<1, LV>)
// of a level as coroutines of Lib3 (coro3b.inc), in its buffer protocol:
// the input comes in the buffers addinp() gives, a 0-length one with f_quit
// set at its end, and yield 1 asks for the next; the output goes into the
// buffer addout() gave, yield 2 handing it over full; yield 0 at the end
// (the last output in the buffer), yield 3 after an error (its note says
// it). What else it yields with tells its driver what to show (rcio.inc):
// a codec prints nothing. So drivers.inc's run_file() runs one on files
// (c, d), and an MTwrap worker runs one on each part of a stream (mt.inc's
// run_part()), each showing what it tells (show_told(), below).
//
// The coroutine is the model: H264pCoro derives from Model, the model from
// the range coder (fam.inc's tc_rc) and the coder from the coroutine's I/O
// (rcio.inc), whose buffers its bytes go through. A run is an object of its
// own, and shares nothing with another, so that any number of them can go at
// once, on as many threads. One is made for a run (coro_new(), Init()),
// and its model's tables are mapped as the run starts (Model::start()); the
// stream's state (the parser's contexts, its buffers) is allocated then too
// and freed as the run ends, or by Quit() wherever the coroutine was left,
// which frees the model's too (coro_free() after). The stack a yield
// copies out stays small (rcio.inc's CoroIo); the decompressor yields deep
// in the model, where the range decoder reads its input, and the compressor
// where the coder writes its output.

namespace h264p {

template <int f_DEC, class LV>
struct H264pCoro : Model<f_DEC, LV> {
  typedef typename Pick<f_DEC != 0, DecompressState, CompressState>::type State;

  H264pArgs args;
  State* st;
  int status;  // the run's: 0, 1 (an error), kRestart; 1 until it has ended

  uint32_t Init(const H264pArgs* a) {
    this->coro_init();
    Model<f_DEC, LV>::Init();
    args = *a;
    st = nullptr;
    status = 1;
    return 0;
  }
  void Quit() {
    if (st) st->Quit();
    mem_free(st);
    st = nullptr;
    Model<f_DEC, LV>::Quit();
  }

  // The run, in a trap (err.inc) that an error ends it at: its note says
  // why, after what was decoded is written (d keeps it); or kRestart, a
  // slice the model pass's check found would not come out as it is
  // (compress_nal()), what it dumped then dropped. The coroutine yields from
  // here, its stack shallow again.
  void do_process() {
    int r;
    this->io_init();
    ErrTrap t;
    if (!setjmp(t.jb)) {
      err_push(t);
      this->run_trap = &t;
      r = run(IntTag<f_DEC>());
      err_pop(t);
    } else if (t.code == kRestart) {
      r = kRestart;
      this->dump_discard();  // (what it dumped goes: it starts over)
    } else {
      r = 1;
      if (st) written(IntTag<f_DEC>());
      this->set_note("h264p: %s: %s", args.name, t.why);
    }
    this->run_trap = nullptr;
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? kYieldFail : kYieldEnd);
  }

  // compressing
  int run(IntTag<0>) {
    st = (CompressState*)mem_alloc(sizeof(CompressState));
    st->Init(this->source());
    return compress_stream<LV>(*this, *st, args);
  }

  // decompressing: the magic, the version and the level (this model's),
  // then the model on the rest
  int run(IntTag<1>) {
    st = (DecompressState*)mem_alloc(sizeof(DecompressState));
    st->Init();
    uint8_t head[sizeof kCmMagic + 1];
    int level = cm_level(head, this->read(head, sizeof head), args.name, this->note, sizeof this->note);
    if (level < 0) return 1;
    if (level != LV::id) {
      this->set_note("h264p: %s: compressed at level -%d, not -%d", args.name, level, LV::id);
      return 1;
    }
    return decompress_stream<LV>(*this, *st, args);
  }
  // after an error: decompressing, what was decoded, written
  void written(IntTag<0>) {}
  void written(IntTag<1>) { this->write(st->buf.data(), st->buf.size()), st->buf.clear(); }
};

// The coroutine of a call of c or d (or of a part, mt.inc), on pages of
// its own -- zeroed, and aligned as CoroutinePair's members ask: made by
// coro_new() (plain data: Init() sets it up), and after its Quit() freed
// by coro_free()
template <class T>
T* coro_new() {
  static_assert(__is_trivially_constructible(T), "a coroutine is plain data, not constructed");
  return (T*)tc_map(sizeof(T), "a coroutine");
}
template <class T>
void coro_free(T* p) {
  tc_unmap(p, sizeof(T));
}

// Runs the coroutine c in coro.inc's buffer protocol to its end: in() gives
// it its next input when it asks for it (kYieldIn), out(more) takes what
// it put in its output buffer when that is full (kYieldOut, more: and then
// gives it the next) or as it ends (kYieldEnd, or kYieldFail after an
// error: what drive() returns), and show(code) comes after each yield
// (what c tells, rcio.inc, and the progress line). The caller's traps
// (err.inc) are put aside while c runs, so that c's are not set on them:
// c's errors end at c's own (a part's model in a worker's).
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
    if (r >= kYieldVideo) continue;  // (told)
    out(r == kYieldOut);
    if (r != kYieldOut) return r;
  }
}

// -v's statistics of a run of codec c, at its end (kYieldStats), after the
// progress line pr: c.st's, and its coder's
template <class LV>
void show_stats(H264pCoro<0, LV>& c, Progress* pr) {
  c.st->st.report("coded", "changes of x264's padding bits", pr);
  cm_report(c, c.n_in, c.n_out, c.table_bytes(), pr);
}
template <class LV>
void show_stats(H264pCoro<1, LV>& c, Progress* pr) {
  say(pr, "%" PRIu64 " NAL units\n", c.st->nals);
}
// What a driver shows of what its codec c, of a run with args a, tells
// (code, each yield), after the progress line pr: the video, a message,
// the statistics (-v); and as the run ends, its error (not a restart).
// (From any thread: pr's lock orders them.)
template <int f_DEC, class LV>
void show_told(H264pCoro<f_DEC, LV>& c, uint code, Progress* pr, const H264pArgs& a) {
  if (code == kYieldVideo) {
    if (pr) pr->video(c.video, a.verbose);
  } else if (code == kYieldNote) {
    say(pr, "%s", c.note);
  } else if (code == kYieldFail && c.status == 1) {
    say_error(pr, "%s", c.note);
  } else if (code == kYieldStats && a.verbose) {
    show_stats(c, pr);
  }
}

}  // namespace h264p
