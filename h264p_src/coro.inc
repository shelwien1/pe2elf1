// h264p's compressor (H264pCoro<0, LV>) and decompressor (H264pCoro<1, LV>)
// of a level as coroutines of Lib3 (coro3b.inc), in its buffer protocol:
// the input comes in the buffers addinp() gives, a 0-length one with f_quit
// set at its end, and yield 1 asks for the next; the output goes into the
// buffer addout() gave, yield 2 handing it over full; yield 0 at the end
// (the last output in the buffer), yield 3 after an error (reported on
// stderr). So drivers.inc's run_file() runs one on files (c, d), and an
// MTwrap worker runs one on each part of a stream (mt.inc's run_part()).
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
  typedef typename std::conditional<f_DEC != 0, DecompressState, CompressState>::type State;

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

  // The run, in a trap (err.inc) that an error ends it at: said, after
  // what was decoded is written (d keeps it); or kRestart, a slice the
  // model pass's check found would not come out as it is (compress_nal()),
  // what it dumped then dropped. The coroutine yields from here, its stack
  // shallow again.
  void do_process() {
    int r;
    this->io_init();
    ErrTrap t;
    if (!setjmp(t.jb)) {
      err_push(t);
      this->run_trap = &t;
      r = run(std::integral_constant<int, f_DEC>());
      err_pop(t);
    } else if (t.code == kRestart) {
      r = kRestart;
      this->dump_discard();  // (what it dumped goes: it starts over)
    } else {
      r = 1;
      if (st) written(std::integral_constant<int, f_DEC>());
      say_error(args.meter, "h264p: %s: %s\n", args.name, t.why);
    }
    this->run_trap = nullptr;
    Quit();
    if (args.verbose) {
      say(args.meter, "coroutine: %zu bytes of stack copied out at a yield, at most (room for %d)\n", this->depth,
              (int)Coroutine::STKPAD);
    }
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? 3 : 0);
  }

  // compressing
  int run(std::integral_constant<int, 0>) {
    st = (CompressState*)mem_alloc(sizeof(CompressState));
    st->Init(this->source());
    return compress_stream<LV>(*this, *st, args);
  }

  // decompressing: the magic, the version and the level (this model's),
  // then the model on the rest
  int run(std::integral_constant<int, 1>) {
    st = (DecompressState*)mem_alloc(sizeof(DecompressState));
    st->Init();
    uint8_t head[sizeof kCmMagic + 1];
    int level = cm_level(head, this->read(head, sizeof head), args.name, args.meter);
    if (level < 0) return 1;
    if (level != LV::id) {
      say_error(args.meter, "h264p: %s: compressed at level -%d, not -%d\n", args.name, level, LV::id);
      return 1;
    }
    return decompress_stream<LV>(*this, *st, args);
  }
  // after an error: decompressing, what was decoded, written
  void written(std::integral_constant<int, 0>) {}
  void written(std::integral_constant<int, 1>) { this->write(st->buf.data(), st->buf.size()), st->buf.clear(); }
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
// it its next input when it asks for it (yield 1), out(more) takes what it
// put in its output buffer when that is full (yield 2, more: and then gives
// it the next) or as it ends (yield 0, or 3 after an error: what drive()
// returns), and tick() comes after each yield (the progress line)
template <class C, class In, class Out, class Tick>
uint drive(C& c, In in, Out out, Tick tick) {
  for (;;) {
    uint r = c.coro_call(&c);
    tick();
    if (r == 1) {
      in();
      continue;
    }
    out(r == 2);
    if (r != 2) return r;
  }
}

}  // namespace h264p
