// h264p's compressor (H264pCoro<0, LV>) and decompressor (H264pCoro<1, LV>)
// of a level as coroutines of Lib3 (coro3b.inc), in its buffer protocol:
// the input comes in the buffers addinp() gives, a 0-length one with f_quit
// set at its end, and yield 1 asks for the next; the output goes into the
// buffer addout() gave, yield 2 handing it over full; yield 0 at the end
// (the last output in the buffer), yield 3 after an error (reported on
// stderr). So CoroFileProc runs one on files (c, d), CoroutinePair chains
// one with another stage (x: the two of them), and an MTwrap worker runs one
// on each part of a stream (mt.inc).
//
// The coroutine is the model: H264pCoro derives from Model, the model from
// the range coder (fam.inc's tc_rc) and the coder from the coroutine's I/O
// (rcio.inc), whose buffers its bytes go through. A run is an object of its
// own, and shares nothing with another, so that any number of them can go at
// once, on as many threads. One is constructed for a run, and its model is
// set up as do_process() starts (Model::init()); the stream's state (the
// parser's contexts, its buffers) is allocated then too and freed as the
// run ends, or by Quit() wherever the coroutine was left. The stack a yield
// copies out stays small (rcio.inc's CoroIo); the decompressor yields deep
// in the model, where the range decoder reads its input, and the compressor
// where the coder writes its output.

// what Init() takes: the options of c and d
struct H264pArgs {
  int level;         // compressing: the model's level (1-9)
  bool verbose;      // statistics on stderr
  bool strict;       // compressing: fail where a NAL unit of a supported type would be stored verbatim
  bool check;        // compressing: the model pass checks the slices it can; status kRestart if one does
                     // not come out as it is, and the input has to be compressed again without
  const char* name;  // the input's, for messages
  Progress* meter;   // the call's progress line, if one is shown (messages cut it)
};

template <int f_DEC, class LV>
struct H264pCoro : Model<f_DEC, LV> {
  typedef typename std::conditional<f_DEC != 0, DecompressState, CompressState>::type State;

  H264pArgs args;
  State* st = nullptr;
  int status = 1;  // the run's: 0, 1 (an error), kRestart; 1 until it has ended

  uint32_t Init(const H264pArgs* a) {
    this->coro_init();
    args = *a;
    return 0;
  }
  void Quit() {
    delete st;
    st = nullptr;
  }
  ~H264pCoro() { Quit(); }

  void do_process() {
    int r = 1;
    this->io_init();
    try {
      r = run(std::integral_constant<int, f_DEC>());
    } catch (const std::exception& e) {  // no memory for the model (nothing in a handler may yield)
      progress_stop(args.meter);
      fprintf(stderr, "h264p: %s: %s\n", args.name, e.what());
    } catch (...) {
      progress_stop(args.meter);
      fprintf(stderr, "h264p: %s: an unknown exception\n", args.name);
    }
    delete st;
    st = nullptr;
    if (args.verbose) {
      progress_cut(args.meter);
      fprintf(stderr, "coroutine: %zu bytes of stack copied out at a yield, at most (room for %d)\n", this->depth,
              (int)Coroutine::STKPAD);
    }
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? 3 : 0);
  }

  // compressing
  int run(std::integral_constant<int, 0>) {
    st = new CompressState(this->source());
    Options opt;
    opt.level = args.level;
    opt.verbose = args.verbose;
    opt.strict = args.strict;
    opt.meter = args.meter;
    return compress_stream<LV>(*this, *st, opt, args.check, args.name);
  }

  // decompressing: the magic, the version and the level (this model's),
  // then the model on the rest
  int run(std::integral_constant<int, 1>) {
    st = new DecompressState();
    uint8_t head[sizeof kCmMagic + 1];
    int level = cm_level(head, this->read(head, sizeof head), args.name, args.meter);
    if (level < 0) return 1;
    if (level != LV::id) {
      progress_stop(args.meter);
      fprintf(stderr, "h264p: %s: compressed at level -%d, not -%d\n", args.name, level, LV::id);
      return 1;
    }
    Options opt;
    opt.verbose = args.verbose;
    opt.meter = args.meter;
    return decompress_stream<LV>(*this, *st, opt, args.name);
  }
};
