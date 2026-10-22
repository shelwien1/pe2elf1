// What a call of h264p has around it: the command line's options, the
// progress line on stderr and the messages that cut it, and its files
// (Output: written under a temporary name, renamed as the call ends well).

namespace h264p {

// The command line's options
struct Options {
  bool verbose;
  bool strict;
  bool progress;  // -p: progress even where stderr is not a terminal
  int level;      // c's model: -L
  int threads;    // c's threads: -t (more than one: the stream in parts, mt.inc)
  uint32_t part;  // and the size of a part, at least: -c

  // the defaults
  void Init() {
    verbose = strict = progress = false;
    level = kLevels[0];
    threads = 1;
    part = 4 << 20;
  }
};

// A lock between threads: a critical section (Windows), else a mutex
struct Lock {
#ifdef _WIN32
  CRITICAL_SECTION cs;
  void Init() { InitializeCriticalSection(&cs); }
  void Quit() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
#else
  pthread_mutex_t m;
  void Init() { pthread_mutex_init(&m, nullptr); }
  void Quit() { pthread_mutex_destroy(&m); }
  void lock() { pthread_mutex_lock(&m); }
  void unlock() { pthread_mutex_unlock(&m); }
#endif
};

// The time in nanoseconds, by a clock that only goes forward
int64_t clock_ns() {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (int64_t)(c.QuadPart / f.QuadPart * 1000000000 + c.QuadPart % f.QuadPart * 1000000000 / f.QuadPart);
#else
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (int64_t)t.tv_sec * 1000000000 + t.tv_nsec;
#endif
}

// Progress on stderr, as c and d go: a line put over itself every half
// second -- the bytes of the stream (raw) and of the compressed stream
// (packed) done so far, how far through its input the call is, the speed
// (of the stream) and the memory the process has now (its resident set)
// -- and at the end, how long it all took and the most memory it had.
// When stderr is a terminal, or with -p.
// what() gives the counts (of what_ctx); poll() puts a line if one is
// due, from any thread (with -t, the workers', as they code: the main
// thread waits on them). Messages go through say() (below), which puts
// them after the line, under the same lock: an error ends the lines.
struct Progress {
  const char* tag;  // enc, dec; none: no progress
  uint64_t total;   // the input's size (0: not known, no percentage)
  void (*what)(void* ctx, uint64_t& raw, uint64_t& packed, uint64_t& pos);  // pos of total: raw or packed
  void* what_ctx;
  int64_t t0;    // the start (clock_ns())
  int64_t last;  // the last line, ns after t0 (atomic)
  int halted;    // an error was reported: no more lines (atomic)
  Lock lock;     // stderr: one line or message at a time
  int len;       // the line's length (a shorter one is padded over it)
  bool up;       // a line is up, the cursor after it

  void Init() {
    tag = nullptr;
    total = 0;
    what = nullptr;
    what_ctx = nullptr;
    t0 = last = 0;
    halted = 0;
    lock.Init();
    len = 0;
    up = false;
  }
  void Quit() { lock.Quit(); }

  void start(const char* t, uint64_t size) {
    tag = t;
    total = size;
    t0 = clock_ns();
  }
  // the counts (none: no lines), from the thread that sets them
  void counts(void (*f)(void*, uint64_t&, uint64_t&, uint64_t&), void* ctx) {
    what_ctx = ctx;
    __atomic_store_n(&what, f, __ATOMIC_SEQ_CST);
  }
  bool stopped() const { return __atomic_load_n(&halted, __ATOMIC_SEQ_CST) != 0; }
  // a line, if half a second has gone by since the last (one thread puts it)
  void poll() {
    void (*f)(void*, uint64_t&, uint64_t&, uint64_t&) = __atomic_load_n(&what, __ATOMIC_SEQ_CST);
    if (!tag || !f || stopped()) return;
    int64_t t = clock_ns() - t0;
    int64_t l = __atomic_load_n(&last, __ATOMIC_RELAXED);
    if (t - l < 500000000 || !__atomic_compare_exchange_n(&last, &l, t, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) return;
    uint64_t raw, packed, pos;
    f(what_ctx, raw, packed, pos);
    double mbs = raw / (t * 1e-9) / (1 << 20);
    char b[192];
    int n = total ? snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.1f%%  %5.2f MB/s  %5.2fM RAM", tag, raw,
                             packed, Min(100.0, 100.0 * pos / total), mbs, ram(false))
                  : snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.2f MB/s  %5.2fM RAM", tag, raw, packed,
                             mbs, ram(false));
    lock.lock();
    if (!stopped()) show(b, n, "");
    lock.unlock();
  }
  // the end, done (the main thread, no worker left; not after an error)
  void done(uint64_t raw, uint64_t packed) {
    if (!tag || stopped()) return;
    double s = (clock_ns() - t0) * 1e-9;
    char b[192];
    int n = snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  100.0%%  done in %.2fs  %5.2fM RAM at most", tag, raw,
                     packed, s, ram(true));
    lock.lock();
    show(b, n, "\n");
    up = false;
    lock.unlock();
  }
  // the end of a call, r its status: done, or the line ended after an error
  void end(int r, uint64_t raw, uint64_t packed) {
    if (r)
      end_line();
    else
      done(raw, packed);
  }
  // a message (stop: an error, after which no line comes, and no other
  // error: with threads, what follows from the first)
  void say(bool stop, const char* fmt, va_list ap) {
    lock.lock();
    if (!stop || !__atomic_exchange_n(&halted, 1, __ATOMIC_SEQ_CST)) {
      cut();
      vfprintf(stderr, fmt, ap);
    }
    lock.unlock();
  }
  // the line ended after an error (none comes after it)
  void end_line() {
    lock.lock();
    __atomic_store_n(&halted, 1, __ATOMIC_SEQ_CST);
    cut();
    lock.unlock();
  }
  // the line ended, where one is up (the lock held)
  void cut() {
    if (up) fputc('\n', stderr);
    up = false;
  }
  void show(const char* b, int n, const char* end) {
    int l = up ? len : 0;
    fprintf(stderr, "\r%s%*s%s", b, l > n ? l - n : 0, "", end);
    fflush(stderr);
    len = n;
    up = true;
  }
  // the memory the process has (its resident set), in MB: now, or (peak)
  // the most it has had. (Where only the most is known, that.)
  static double ram(bool peak) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS m;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof m)) return 0;
    return (peak ? m.PeakWorkingSetSize : m.WorkingSetSize) / 1048576.0;
#else
#ifdef __linux__
    if (!peak) {
      long size, res;  // (pages)
      FILE* f = fopen("/proc/self/statm", "r");
      bool ok = f && fscanf(f, "%ld %ld", &size, &res) == 2;
      if (f) fclose(f);
      if (ok) return res * (double)sysconf(_SC_PAGESIZE) / 1048576.0;
    }
#endif
    struct rusage u;
    if (getrusage(RUSAGE_SELF, &u)) return 0;
#ifdef __APPLE__
    return u.ru_maxrss / 1048576.0;  // (bytes)
#else
    return u.ru_maxrss / 1024.0;  // (KB)
#endif
#endif
  }
};
// A message on stderr, after the progress line of the call (p, if any);
// and an error's, which ends the call: no progress line after it
void say_v(Progress* p, bool stop, const char* fmt, va_list ap) {
  if (p)
    p->say(stop, fmt, ap);
  else
    vfprintf(stderr, fmt, ap);
}
void say(Progress* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  say_v(p, false, fmt, ap);
  va_end(ap);
}
void say_error(Progress* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  say_v(p, true, fmt, ap);
  va_end(ap);
}
void progress_poll(Progress* p) {
  if (p) p->poll();
}

// what the file name is: -1 nothing, 1 a regular file, 0 anything else;
// and its mode
int name_kind(const char* name, unsigned* mode = nullptr) {
#ifdef _WIN32
  struct _stati64 s;
  if (_stati64(name, &s)) return -1;
  bool reg = (s.st_mode & _S_IFMT) == _S_IFREG;
#else
  struct stat s;
  if (stat(name, &s)) return -1;
  bool reg = S_ISREG(s.st_mode);
#endif
  if (mode) *mode = (unsigned)s.st_mode;
  return reg;
}
// whether name is the file f has open
bool same_file(FILE* f, const char* name) {
#ifdef _WIN32
  HANDLE h = CreateFileA(name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  BY_HANDLE_FILE_INFORMATION a, b;
  bool r = GetFileInformationByHandle(h, &a) && GetFileInformationByHandle((HANDLE)_get_osfhandle(_fileno(f)), &b) &&
           a.dwVolumeSerialNumber == b.dwVolumeSerialNumber && a.nFileIndexHigh == b.nFileIndexHigh &&
           a.nFileIndexLow == b.nFileIndexLow;
  CloseHandle(h);
  return r;
#else
  struct stat a, b;
  return stat(name, &a) == 0 && fstat(fileno(f), &b) == 0 && a.st_dev == b.st_dev && a.st_ino == b.st_ino;
#endif
}

// The output of a call: "-" is stdout. A regular file, or a new one, is
// written under a temporary name beside it and renamed to its own as the
// call ends well, so that a failed call leaves what was there; anything else
// (a device, a pipe) is written as it is. Never the input. (Quit() closes
// and removes what close() did not: the program ends on an error outside a
// run, err.inc, with Quit() of the call's output, and on POSIX on a signal
// that ends it, with its temporary file removed.)
struct Output;
Output* open_output = nullptr;  // the call's (the main thread's)
void quit_open_output();

struct Output {
  const char* name;  // as given
  char* dest;        // the file (mem_alloc()'d), and the temporary one written (none: name itself)
  char* tmp;
  bool file;  // a regular file is written: it can be written again (kRestart)
  filehandle0 g;

  void Init() {
    name = nullptr;
    dest = tmp = nullptr;
    file = false;
    g.f = nullptr;
  }
  void Quit() {
    if (g.f && g.f != stdout) fclose(g.f);
    g.f = nullptr;
    char* t = tmp;
    tmp = nullptr;  // (before it is freed: quit_on_signal() reads it)
    if (t) remove(t);
    mem_free(t);
    mem_free(dest);
    dest = nullptr;
    if (open_output == this) open_output = nullptr;
  }
  static char* str_dup(const char* s) { return (char*)memcpy(mem_alloc(strlen(s) + 1), s, strlen(s) + 1); }

  // (false: said, and Quit())
  bool open(const char* out_name, FILE* in, const char* in_name) {
    open_output = this;
    name = out_name;
    if (!strcmp(name, "-")) {
      g.f = stdout;
      return true;
    }
    if (same_file(in, name)) {
      fprintf(stderr, "h264p: %s: the input and the output are the same file\n", in_name);
      Quit();
      return false;
    }
    unsigned mode = 0;
    int kind = name_kind(name, &mode);
    bool fresh = kind < 0;
    file = fresh || kind == 1;
    if (file) {
#ifdef _WIN32
      long pid = (long)_getpid();
      dest = str_dup(name);
#else
      long pid = (long)getpid();
      char* real = fresh ? nullptr : realpath(name, nullptr);  // (through a link: beside the file)
      dest = str_dup(real ? real : name);
      free(real);
#endif
      size_t n = strlen(dest) + 32;
      tmp = (char*)mem_alloc(n);
      snprintf(tmp, n, "%s.%ld.tmp", dest, pid);
      g.f = fopen(tmp, "wb");
      if (!g.f) mem_free(tmp), tmp = nullptr;  // (no room for it beside the file: the file itself)
#ifndef _WIN32
      if (g.f && !fresh) (void)!fchmod(fileno(g.f), mode & 07777);  // (as private as the file it replaces)
#endif
    }
    if (!g.f) g.f = fopen(name, "wb");
    if (!g.f) {
      fprintf(stderr, "h264p: cannot create %s: %s\n", name, strerror(errno));
      Quit();
      return false;
    }
    return true;
  }
  // empty again, for the call to start over (kRestart)
  bool rewind() {
    fclose(g.f);
    g.f = fopen(tmp ? tmp : name, "wb");
    return g.f != nullptr;
  }
  // The end of the call, r its status so far: what it wrote becomes the
  // output, or after an error is removed, unless keep (d keeps what it
  // restored, if it could write it); the exit status. (Messages after p's
  // line, if any.)
  int close(int r, bool keep, Progress* p) {
    bool werr = false;
    if (g.f == stdout)
      werr = fflush(stdout) != 0 || ferror(stdout) != 0;
    else if (g.f) {
      werr = ferror(g.f) != 0;
      werr = fclose(g.f) != 0 || werr;  // (closed whatever ferror says)
    }
    g.f = nullptr;
    if (!r && werr) {
      say_error(p, "h264p: %s: write error\n", name);
      r = 1;
    }
    bool stay = !r || (keep && !werr);
    if (tmp) {
      if (stay && !move_file(tmp, dest)) {
        say_error(p, "h264p: cannot rename %s to %s: %s\n", tmp, dest, strerror(errno));
        r = 1, stay = false;
      }
      if (!stay) remove(tmp);
      mem_free(tmp);
      tmp = nullptr;
    } else if (!stay && file) {
      remove(name);
    }
    Quit();
    return r ? 1 : 0;
  }
  static bool move_file(const char* from, const char* to) {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(from, to) == 0;
#endif
  }
};

void quit_open_output() {
  if (open_output) open_output->Quit();
}
const bool open_output_hooked = !atexit(quit_open_output);  // (as the program starts)
#ifndef _WIN32
// a signal that ends the program: the temporary file of the call's output
// goes first (only what is safe in a signal handler)
void quit_on_signal(int sig) {
  Output* o = open_output;
  const char* t = o ? o->tmp : nullptr;
  if (t) unlink(t);
  signal(sig, SIG_DFL);
  raise(sig);
}
bool hook_signals() {
  const int sigs[] = {SIGINT, SIGTERM, SIGHUP, SIGPIPE};
  for (int s : sigs)
    if (signal(s, quit_on_signal) == SIG_IGN) signal(s, SIG_IGN);  // (one ignored, as by nohup, stays so)
  return true;
}
const bool signals_hooked = hook_signals();  // (as the program starts)
#endif

// a call's input (f) and output (o); false after a message
bool open_io(const char* in_name, const char* out_name, filehandle0& f, Output& o) {
  f.open(in_name);
  if (!f.f) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return false;
  }
  if (!o.open(out_name, f.f, in_name)) {
    if (f.f != stdin) fclose(f.f);
    return false;
  }
  return true;
}
// and their end, r the call's status so far: the exit status (Output::close)
int close_io(int r, filehandle0& f, Output& o, bool keep, Progress* p = nullptr) {
  if (!r && ferror(f.f)) {
    say_error(p, "h264p: read error\n");
    r = 1;
  }
  if (f.f != stdin) fclose(f.f);
  return o.close(r, keep, p);
}

// the size of the file f reads, 0 if it is not a regular file (a pipe)
uint64_t file_size(FILE* f) {
#ifdef _WIN32
  struct _stati64 s;
  return _fstati64(_fileno(f), &s) == 0 && (s.st_mode & _S_IFMT) == _S_IFREG ? (uint64_t)s.st_size : 0;
#else
  struct stat s;
  return fstat(fileno(f), &s) == 0 && S_ISREG(s.st_mode) ? (uint64_t)s.st_size : 0;
#endif
}

}  // namespace h264p
