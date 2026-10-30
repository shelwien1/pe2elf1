// replay.inc -- famtune.cpp's Replay: the replay as a coroutine.

// The replay as a coroutine, as the model codes (its coder writes through a coroutine's output,
// io.inc), whose output main() throws away: what counts is the size -s counts
struct Replay : CoIo {
  char** argv;
  int argc;
  Coder co;
  void Init(int c, char** v) {
    coro_init();
    argc = c, argv = v;
  }
  void do_process() {
    io_init();
    co.init(this, 1);
    co.stats = 1;
#define FT_FAM(F) !strcmp(argv[1], #F) ? replay<FtLv::F>(argv + 2, argc - 2, co) :
    M4V_FAMS(FT_FAM) fatal("no family %s", argv[1]);
#undef FT_FAM
    for (;;) yield(static_cast<Coroutine*>(this), Y_END);
  }
};
