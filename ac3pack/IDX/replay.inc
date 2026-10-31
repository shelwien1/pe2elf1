// replay.inc -- famtune.cpp's Replay: the replay as a coroutine.

// The replay as a coroutine, as the model codes (it is the range coder, whose bytes go to its
// output, io.inc), whose output main() throws away: what counts is the size -s counts
struct Replay : CoIo<Coder<0> > {
  char** argv;
  int argc;
  void Init(int c, char** v) {
    coro_init();
    io_init();
    argc = c, argv = v;
    stats = 1;
  }
  void do_process() {
    coder_start();
#define FT_FAM(F) !strcmp(argv[1], #F) ? replay<FtLv::F>(argv + 2, argc - 2, *this) :
    AC3P_FAMS(FT_FAM) fatal("no family %s", argv[1]);
#undef FT_FAM
    for (;;) yield(static_cast<Coroutine*>(this), Y_END);
  }
};
