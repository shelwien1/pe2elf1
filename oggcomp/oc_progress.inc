//  A run's progress line on stderr, as m4vpack's (progress.inc): put over
//  itself every half second -- the bytes of the .ogg side done so far
//  (raw) and of the .oc side (packed), how far through its input the run
//  is, the speed (of the .ogg side), and the memory the process has now --
//  and at the end, how long it all took and the most memory it had.  Shown
//  where stderr is a terminal, or with -p; not with -q (unless -p).
//
//  counts() gives what to show: a function of the run's state, called
//  from the thread that puts the line.  poll() puts a line if one is due,
//  from any thread: on threads, the workers' as they code, and the main
//  thread's as it waits on them.  A message on stderr ends the line first
//  (ogc_msg_hook, oc_fatal.inc), so that it is not written over the line,
//  and no line comes after an error.
#ifdef _WIN32
#define PSAPI_VERSION 2  // GetProcessMemoryInfo from kernel32
#include <psapi.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <pthread.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#endif

constexpr i64 OC_NS = 1000000000;
constexpr double OC_MIB = 1048576.0;

//  The time in nanoseconds, by a clock that only goes forward.
static i64 oc_clock_ns(void) {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (i64)(c.QuadPart / f.QuadPart * OC_NS + c.QuadPart % f.QuadPart * OC_NS / f.QuadPart);
#else
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (i64)t.tv_sec * OC_NS + t.tv_nsec;
#endif
}

//  The size of the file named, 0 where it is not a regular file -- stdin,
//  a pipe -- and then there is no percentage.
static u64 oc_file_size(const char *path) {
  if(!path || !strcmp(path, "-"))
    return 0;
#ifdef _WIN32
  struct _stat64 s;
  if(_stat64(path, &s) || (s.st_mode & _S_IFMT) != _S_IFREG)
    return 0;
#else
  struct stat s;
  if(stat(path, &s) || !S_ISREG(s.st_mode))
    return 0;
#endif
  return (u64)s.st_size;
}

static int oc_stderr_tty(void) {
#ifdef _WIN32
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(fileno(stderr)) != 0;
#endif
}

//  A lock between threads: a critical section on Windows, else a mutex.
struct oc_lock {
#ifdef _WIN32
  CRITICAL_SECTION cs;
  void init() { InitializeCriticalSection(&cs); }
  void quit() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
#else
  pthread_mutex_t m;
  void init() { pthread_mutex_init(&m, nullptr); }
  void quit() { pthread_mutex_destroy(&m); }
  void lock() { pthread_mutex_lock(&m); }
  void unlock() { pthread_mutex_unlock(&m); }
#endif
};

//  What a run has done, for its line: the .ogg side's bytes (raw), the
//  .oc side's (packed), and on threads the parts done.
struct oc_counts {
  u64 raw, packed, parts;
};
typedef void (*oc_count_fn)(void *ctx, oc_counts &n);

struct oc_progress;
static oc_progress *oc_progress_now;
static void oc_progress_cut(void);

struct oc_progress {
  static constexpr i64 EVERY = OC_NS / 2;  // a line every half second
  static constexpr int LINE = 160;          // a line's room
  const char *tag;  // "enc", "dec"; none: no line
  int decode;       // the percentage is of the input: packed for d, raw for c
  int threads;      // 0: one model, no parts on the line
  u64 total;        // the input's size; 0, not known: no percentage
  oc_count_fn what;
  void *what_ctx;
  i64 t0;      // the start
  i64 last;    // the last line, ns after t0 (atomic)
  int halted;  // an error was said: no more lines (atomic)
  oc_lock lk;  // stderr: one line or message at a time
  int len;     // the line's length, which a shorter one pads over
  int up;      // a line is up, the cursor after it

  void init() {
    tag = nullptr;
    decode = threads = 0;
    total = 0;
    what = nullptr;
    what_ctx = nullptr;
    t0 = last = 0;
    halted = 0;
    lk.init();
    len = up = 0;
  }
  void quit() {
    if(oc_progress_now == this) {
      oc_progress_now = nullptr;
      ogc_msg_hook = nullptr;
    }
    lk.quit();
  }
  //  The line from now on, for a run tagged t over an input of size bytes.
  void start(const char *t, int dec, int nthreads, u64 size) {
    tag = t;
    decode = dec;
    threads = nthreads;
    total = size;
    t0 = oc_clock_ns();
    oc_progress_now = this;
    ogc_msg_hook = oc_progress_cut;
  }
  //  What to count (none: no lines), from the thread that sets it.
  void counts(oc_count_fn f, void *ctx) {
    __atomic_store_n(&what_ctx, ctx, __ATOMIC_SEQ_CST);
    __atomic_store_n(&what, f, __ATOMIC_SEQ_CST);
  }
  int stopped() const { return __atomic_load_n(&halted, __ATOMIC_SEQ_CST) != 0; }
  void line(char *b, sz n, const oc_counts &c, i64 t, int done) {
    sz k = 0;
    const double secs = (double)t / OC_NS;
    k += (sz)snprintf(b + k, n - k, "[%s] %" PRIu64 " > %" PRIu64, tag, decode ? c.packed : c.raw, decode ? c.raw : c.packed);
    if(total || done) {
      double p = done ? 100.0 : 100.0 * (double)(decode ? c.packed : c.raw) / (double)total;
      k += (sz)snprintf(b + k, n - k, "  %5.1f%%", p < 100.0 ? p : 100.0);
    }
    if(threads && k < n)
      k += (sz)snprintf(b + k, n - k, "  %" PRIu64 " part%s on %d threads", c.parts, c.parts == 1 ? "" : "s", threads);
    if(k < n) {
      if(done)
        snprintf(b + k, n - k, "  done in %.2fs  %.1fM RAM at most", secs, ram(1));
      else
        snprintf(b + k, n - k, "  %.2f MB/s  %.1fM RAM", secs > 0 ? (double)c.raw / secs / OC_MIB : 0.0, ram(0));
    }
  }
  //  A line, if half a second has gone by since the last; from any thread,
  //  one of them putting it.
  void poll() {
    oc_count_fn f = __atomic_load_n(&what, __ATOMIC_SEQ_CST);
    void *ctx = __atomic_load_n(&what_ctx, __ATOMIC_SEQ_CST);
    oc_counts c;
    char b[LINE];
    i64 t, l;
    if(!tag || !f || stopped())
      return;
    t = oc_clock_ns() - t0;
    l = __atomic_load_n(&last, __ATOMIC_RELAXED);
    if(t - l < EVERY || !__atomic_compare_exchange_n(&last, &l, t, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
      return;
    f(ctx, c);
    line(b, sizeof b, c, t, 0);
    lk.lock();
    if(!stopped())
      show(b, "");
    lk.unlock();
  }
  //  The end, done: the main thread, no worker left, no error.
  void done(const oc_counts &c) {
    char b[LINE];
    if(!tag || stopped())
      return;
    line(b, sizeof b, c, oc_clock_ns() - t0, 1);
    lk.lock();
    show(b, "\n");
    up = 0;
    lk.unlock();
  }
  //  The line ended, where one is up, and no line after it: an error.
  void cut() {
    lk.lock();
    __atomic_store_n(&halted, 1, __ATOMIC_SEQ_CST);
    if(up)
      fputc('\n', stderr);
    up = 0;
    lk.unlock();
  }
  void show(const char *b, const char *end) {
    const int n = (int)strlen(b), l = up ? len : 0;
    fprintf(stderr, "\r%s%*s%s", b, l > n ? l - n : 0, "", end);
    fflush(stderr);
    len = n;
    up = 1;
  }
  //  The memory the process has (its resident set), in MB: now, or (peak)
  //  the most it has had; where only the most is known, that.
  static double ram(int peak) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS m;
    if(!GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof m))
      return 0;
    return (double)(peak ? m.PeakWorkingSetSize : m.WorkingSetSize) / OC_MIB;
#else
#ifdef __linux__
    if(!peak) {
      long size, res;
      FILE *f = fopen("/proc/self/statm", "r");
      const int ok = f && fscanf(f, "%ld %ld", &size, &res) == 2;
      if(f)
        fclose(f);
      if(ok)
        return (double)res * (double)sysconf(_SC_PAGESIZE) / OC_MIB;
    }
#endif
    struct rusage u;
    if(getrusage(RUSAGE_SELF, &u))
      return 0;
#ifdef __APPLE__
    return (double)u.ru_maxrss / OC_MIB;
#else
    return (double)u.ru_maxrss / 1024.0;
#endif
#endif
  }
};
//  ogc_msg_hook: the line ends before a message, which none follows.
static void oc_progress_cut(void) {
  if(oc_progress_now)
    oc_progress_now->cut();
}
