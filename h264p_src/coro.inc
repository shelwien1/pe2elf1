// h264p's compressor (H264pCoro<0>) and decompressor (H264pCoro<1>) as
// coroutines of Lib3 (coro3b.inc), in its buffer protocol: the input comes
// in the buffers addinp() gives, a 0-length one with f_quit set at its end,
// and yield 1 asks for the next; the output goes into the buffer addout()
// gave, yield 2 handing it over full; yield 0 at the end (the last output in
// the buffer), yield 3 after an error (reported on stderr). So CoroFileProc
// runs one on files (c, d), CoroutinePair chains one with another stage (x:
// the two of them), and MTwrap could take them for its workers' Enc_class
// (Init(), do_process(), Quit(); the decompressor hands over only full
// output buffers but its last).
//
// The object is plain data, never constructed (CoroFileProc's static union,
// MTwrap's VirtualAlloc'd workers): Init() sets it up; a run's state (the
// model, the stream's buffers) is allocated as do_process() starts and freed
// as it ends, or by Quit() wherever the coroutine was left. A yield copies
// the coroutine's stack out into Coroutine::stk (STKPAD bytes), so one that
// would need more fails the run instead (before_yield()); the decompressor
// yields deep in the model, where the range decoder reads its input.
//
// (One at a time of each kind: the range coders, tc_rc<f_DEC>::rc, and the
// statistics of -v are the process's.)

// what Init() takes: the options of c and d
struct H264pArgs {
  int level;         // compressing: the model's level (1-9)
  bool verbose;      // statistics on stderr
  bool strict;       // compressing: fail where a NAL unit of a supported type would be stored verbatim
  bool check;        // compressing: the model pass checks the slices it can; status kRestart if one does
                     // not come out as it is, and the input has to be compressed again without
  const char* name;  // the input's, for messages
};

template <int f_DEC>
struct H264pCoro : Coroutine {
  typedef typename std::conditional<f_DEC != 0, DecompressState, CompressState>::type State;

  H264pArgs args;
  State* st;
  int status;            // the run's: 0, 1 (an error), kRestart; 1 until it has ended
  bool eof;              // the input has ended
  uint64_t n_in, n_out;  // the bytes read and written
  size_t depth;          // the stack a yield copied out, at most

  uint32_t Init(const H264pArgs* a) {
    coro_init();
    args = *a;
    st = nullptr;
    status = 1;
    depth = 0;
    return 0;
  }
  void Quit() {
    delete st;
    st = nullptr;
  }

  void do_process() {
    int r = 1;
    eof = false;
    n_in = n_out = 0;
    try {
      r = run(std::integral_constant<int, f_DEC>());
    } catch (const std::exception& e) {  // no memory for the model (nothing in a handler may yield)
      fprintf(stderr, "h264p: %s: %s\n", args.name, e.what());
    } catch (...) {
      fprintf(stderr, "h264p: %s: an unknown exception\n", args.name);
    }
    delete st;
    st = nullptr;
    if (args.verbose)
      fprintf(stderr, "coroutine: %zu bytes of stack copied out at a yield, at most (room for %d)\n", depth,
              (int)STKPAD);
    status = r;
    for (;;) yield(this, r ? 3 : 0);
  }

  // compressing
  int run(std::integral_constant<int, 0>) {
    st = new CompressState(source());
    Options opt;
    opt.level = args.level;
    opt.verbose = args.verbose;
    opt.strict = args.strict;
    switch (args.level) {
#define H264P_C(n) \
  case n:          \
    return compress_stream<Lv##n>(*st, sink(), opt, args.check, args.name);
      H264P_LEVELS(H264P_C)
#undef H264P_C
    }
    fprintf(stderr, "h264p: no level -%d in this build\n", args.level);
    return 1;
  }

  // decompressing: the magic, the version and the level, then the level's
  // model on the rest
  int run(std::integral_constant<int, 1>) {
    st = new DecompressState();
    uint8_t head[sizeof kCmMagic + 1];
    size_t n = read(head, sizeof head);
    if (n < sizeof kCmMagic || memcmp(head, kCmMagic, sizeof kCmMagic - 1)) {
      fprintf(stderr, "h264p: %s: not a stream h264p compressed\n", args.name);
      return 1;
    }
    if (head[sizeof kCmMagic - 1] != kCmVersion) {
      fprintf(stderr, "h264p: %s: compressed by version %d, this is version %d\n", args.name,
              (int)head[sizeof kCmMagic - 1], kCmVersion);
      return 1;
    }
    int level = n == sizeof head ? head[sizeof kCmMagic] : -1;
    Options opt;
    opt.verbose = args.verbose;
    switch (level) {
#define H264P_D(n) \
  case n:          \
    return decompress_stream<Lv##n>(*st, source(), sink(), opt, args.name);
      H264P_LEVELS(H264P_D)
#undef H264P_D
    }
    fprintf(stderr, "h264p: %s: compressed at level -%d, which this build has not\n", args.name, level);
    return 1;
  }

  // up to n bytes of the input, fewer only at its end (yield 1 for more)
  size_t read(uint8_t* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      if (inpptr >= inpend) {
        if (eof) break;
        before_yield();
        chkinp();
        if (inpptr >= inpend) {  // a 0-length input: the end
          eof = true;
          break;
        }
      }
      size_t l = std::min(n - k, (size_t)(inpend - inpptr));
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    n_in += k;
    return k;
  }

  // all n bytes into the output (yield 2 with each full buffer)
  void write(const uint8_t* p, size_t n) {
    while (n) {
      if (outptr >= outend) {
        before_yield();
        chkout();
        continue;
      }
      size_t l = std::min(n, (size_t)(outend - outptr));
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
      n_out += l;
    }
  }

  // the stack a yield is about to copy out, against Coroutine::stk's room
  void before_yield() {
    char here;
    size_t d = (size_t)((const char*)stkptrH - &here);
    if (d > depth) depth = d;
    if (d + 4096 > (size_t)STKPAD) cm_fail("internal: the coroutine's stack (%zu bytes) is too deep to yield", d);
  }

  static size_t read_cb(void* c, uint8_t* p, size_t n) { return ((H264pCoro*)c)->read(p, n); }
  static void write_cb(void* c, const uint8_t* p, size_t n) { ((H264pCoro*)c)->write(p, n); }
  ByteSource source() {
    ByteSource s;
    s.fn = read_cb;
    s.ctx = this;
    return s;
  }
  ByteSink sink() {
    ByteSink s;
    s.fn = write_cb;
    s.ctx = this;
    return s;
  }
};
