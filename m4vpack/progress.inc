// progress.inc -- a call's progress line on stderr, as h264p's: as c and d go, a line put over itself
// every half second -- the bytes of the stream (raw) and of the compressed stream (packed) done so
// far, how far through its input the call is, the VOPs, the speed (of the stream) and the memory the
// process has now (its resident set) -- and at the end, how long it all took and the most memory it
// had. Before the lines, what the video is, as the run tells it (VideoInfo, videoinfo.inc). Shown
// where stderr is a terminal, or with -p; not with -q (unless -p).
//
// counts() gives what to show (a function of the run's state, read from the thread that polls);
// poll() puts a line if one is due, from any thread (on threads, the workers', as they code: the
// main thread waits on them). An error's message goes through say_error() (below), which ends the
// line first, under the same lock: no line comes after it.

#ifdef _WIN32
#define PSAPI_VERSION 2  // (GetProcessMemoryInfo from kernel32)
#include <psapi.h>
#endif

constexpr S64 PROGRESS_EVERY = NS / 2;  // a line every half second

// a call's counts: raw, packed, pos (of the input's total: raw or packed) and the VOPs
typedef void (*ProgressCounts)(void* ctx, U64& raw, U64& packed, U64& pos, U64& vops);

struct Progress {
  constexpr static int LINE = 192;  // a line's room
  const char* tag;  // enc, dec, ver; none: no progress
  U64 total;        // the input's size (0: not known, no percentage)
  ProgressCounts what;
  void* what_ctx;
  VideoInfo shown;  // the video as said last (shown.width 0: none)
  S64 t0;           // the start (clock_ns())
  S64 last;         // the last line, ns after t0 (atomic)
  int halted;       // an error was reported: no more lines (atomic)
  Lock lk;          // stderr: one line or message at a time
  int len;          // the line's length (a shorter one is padded over it)
  bool up;          // a line is up, the cursor after it

  void Init() {
    tag = nullptr;
    total = 0;
    what = nullptr;
    what_ctx = nullptr;
    memset((void*)&shown, 0, sizeof shown);
    t0 = last = 0;
    halted = 0;
    lk.Init();
    len = 0;
    up = false;
  }
  void Quit() { lk.Quit(); }

  void start(const char* t, U64 size) {
    tag = t;
    total = size;
    t0 = clock_ns();
  }
  // the counts (none: no lines), from the thread that sets them
  void counts(ProgressCounts f, void* ctx) {
    what_ctx = ctx;
    __atomic_store_n(&what, f, __ATOMIC_SEQ_CST);
  }
  bool stopped() const { return __atomic_load_n(&halted, __ATOMIC_SEQ_CST) != 0; }
  // a line, if half a second has gone by since the last (one thread puts it)
  void poll() {
    ProgressCounts f = __atomic_load_n(&what, __ATOMIC_SEQ_CST);
    S64 t, l;
    U64 raw, packed, pos, vops;
    char b[LINE];
    size_t n = 0;
    if (!tag || !f || stopped()) return;
    t = clock_ns() - t0;
    l = __atomic_load_n(&last, __ATOMIC_RELAXED);
    if (t - l < PROGRESS_EVERY || !__atomic_compare_exchange_n(&last, &l, t, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
      return;
    f(what_ctx, raw, packed, pos, vops);
    put_text(b, sizeof b, n, "[%s] %llu > %llu", tag, (unsigned long long)raw, (unsigned long long)packed);
    if (total) {
      const double p = PERCENT * pos / total;
      put_text(b, sizeof b, n, "  %5.1f%%", p < PERCENT ? p : PERCENT);
    }
    put_text(b, sizeof b, n, "  %llu VOPs  %5.2f MB/s  %5.2fM RAM", (unsigned long long)vops,
             (double)raw / ((double)t / NS) / MIB, ram(false));
    lk.lock();
    if (!stopped()) show(b, (int)n, "");
    lk.unlock();
  }
  // the end, done (the main thread, no worker left; not after an error)
  void done(U64 raw, U64 packed, U64 vops) {
    char b[LINE];
    int n;
    if (!tag || stopped()) return;
    n = snprintf(b, sizeof b, "[%s] %llu > %llu  %5.1f%%  %llu VOPs  done in %.2fs  %5.2fM RAM at most", tag,
                 (unsigned long long)raw, (unsigned long long)packed, PERCENT, (unsigned long long)vops,
                 (double)(clock_ns() - t0) / NS, ram(true));
    lk.lock();
    show(b, n < (int)sizeof b ? n : (int)sizeof b - 1, "\n");
    up = false;
    lk.unlock();
  }
  // the end of a call, r its status: done, or the line ended after an error
  void end(int r, U64 raw, U64 packed, U64 vops) {
    if (r) end_line();
    else done(raw, packed, vops);
  }
  // what the video is, as a run tells it: said as it first comes and where it changes (the parts
  // on threads tell it each, from as many threads)
  void video(const VideoInfo& v) {
    if (!tag || !v.width) return;
    lk.lock();
    if (!shown.same(v) && !stopped()) {
      char b[LINE * 2];
      const bool first = !shown.width;
      shown = v;
      video_words(v, b, sizeof b);
      cut();
      fprintf(stderr, "[%s] %s: %s\n", tag, first ? "video" : "the video now", b);
    }
    lk.unlock();
  }
  // an error's message, after which no line comes, and no other error's (on threads, what follows
  // from the first)
  void error(const char* fmt, va_list ap) {
    lk.lock();
    if (!__atomic_exchange_n(&halted, 1, __ATOMIC_SEQ_CST)) {
      cut();
      vfprintf(stderr, fmt, ap);
    }
    lk.unlock();
  }
  // the line ended after an error (none comes after it)
  void end_line() {
    lk.lock();
    __atomic_store_n(&halted, 1, __ATOMIC_SEQ_CST);
    cut();
    lk.unlock();
  }
  // the line ended, where one is up (the lock held)
  void cut() {
    if (up) fputc('\n', stderr);
    up = false;
  }
  void show(const char* b, int n, const char* end) {
    const int l = up ? len : 0;
    fprintf(stderr, "\r%s%*s%s", b, l > n ? l - n : 0, "", end);
    fflush(stderr);
    len = n;
    up = true;
  }
  // the memory the process has (its resident set), in MB: now, or (peak) the most it has had.
  // (Where only the most is known, that.)
  static double ram(bool peak) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS m;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof m)) return 0;
    return (double)(peak ? m.PeakWorkingSetSize : m.WorkingSetSize) / MIB;
#else
#ifdef __linux__
    if (!peak) {
      long size, res;  // (pages)
      FILE* f = fopen("/proc/self/statm", "r");
      const bool ok = f && fscanf(f, "%ld %ld", &size, &res) == 2;
      if (f) fclose(f);
      if (ok) return res * (double)sysconf(_SC_PAGESIZE) / MIB;
    }
#endif
    struct rusage u;
    if (getrusage(RUSAGE_SELF, &u)) return 0;
#ifdef __APPLE__
    return u.ru_maxrss / (double)MIB;  // (bytes)
#else
    return u.ru_maxrss / (double)KIB;  // (KB)
#endif
#endif
  }
};

// An error's message on stderr, after the progress line of the call (p, if any), which it ends
static void say_error(Progress* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (p) p->error(fmt, ap);
  else vfprintf(stderr, fmt, ap);
  va_end(ap);
}
