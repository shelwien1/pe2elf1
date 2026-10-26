// What a call of mp3pack has around it, as aacpack's cli.inc: the command
// line's options, the progress line on stderr and the messages that cut
// it, and its files -- an output is written under a temporary name and
// renamed as the call ends well. c has two outputs (the compressed stream,
// the outside data) and d two inputs.

namespace mp3pack {

// The command line's options
struct Options {
  bool verbose;
  bool progress;  // -p: progress even where stderr is not a terminal
  int threads;    // c's threads: -t (more than one: the frames in parts, mt.inc)
  uint32_t part;  // and the size of a part of the input, at least: -c

  void Init() {
    verbose = progress = false;
    threads = 1;
    part = 1 << 20;
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

// A line of text (b, of n bytes) after the k bytes it has: fmt's
void put_text(char* b, size_t n, size_t& k, const char* fmt, ...) {
  if (k >= n) return;
  va_list ap;
  va_start(ap, fmt);
  int l = vsnprintf(b + k, n - k, fmt, ap);
  va_end(ap);
  if (l > 0) k = Min(n - 1, k + (size_t)l);
}

// Progress on stderr, as c and d go (aacpack's): a line put over itself
// every half second -- the bytes of the file (raw) and of the compressed
// stream (packed) done so far, how far through its input the call is, the
// frames, the speed and the memory the process has now -- and at the end,
// how long it all took and the most memory it had. When stderr is a
// terminal, or with -p. what() gives the counts (of what_ctx); poll() puts
// a line if one is due, from any thread (with -t, the workers', as they
// code). Messages go through say() (below), which puts them after the
// line, under the same lock: an error ends the lines.
struct Progress {
  const char* tag;  // enc, dec; none: no progress
  uint64_t total;   // the input's size (0: not known, no percentage)
  // pos of total: raw or packed; frames: the frames
  void (*what)(void* ctx, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& frames);
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
  void counts(void (*f)(void*, uint64_t&, uint64_t&, uint64_t&, uint64_t&), void* ctx) {
    what_ctx = ctx;
    __atomic_store_n(&what, f, __ATOMIC_SEQ_CST);
  }
  bool stopped() const { return __atomic_load_n(&halted, __ATOMIC_SEQ_CST) != 0; }
  // a line, if half a second has gone by since the last (one thread puts it)
  void poll() {
    void (*f)(void*, uint64_t&, uint64_t&, uint64_t&, uint64_t&) = __atomic_load_n(&what, __ATOMIC_SEQ_CST);
    if (!tag || !f || stopped()) return;
    int64_t t = clock_ns() - t0;
    int64_t l = __atomic_load_n(&last, __ATOMIC_RELAXED);
    if (t - l < 500000000 || !__atomic_compare_exchange_n(&last, &l, t, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) return;
    uint64_t raw, packed, pos, frames;
    f(what_ctx, raw, packed, pos, frames);
    double mbs = raw / (t * 1e-9) / (1 << 20);
    char b[192];
    size_t n = 0;
    put_text(b, sizeof b, n, "[%s] %" PRIu64 " > %" PRIu64, tag, raw, packed);
    if (total) put_text(b, sizeof b, n, "  %5.1f%%", Min(100.0, 100.0 * pos / total));
    put_text(b, sizeof b, n, "  %" PRIu64 " frames  %5.2f MB/s  %5.2fM RAM", frames, mbs, ram(false));
    lock.lock();
    if (!stopped()) show(b, (int)n, "");
    lock.unlock();
  }
  // the end, done (the main thread, no worker left; not after an error)
  void done(uint64_t raw, uint64_t packed, uint64_t frames) {
    if (!tag || stopped()) return;
    double s = (clock_ns() - t0) * 1e-9;
    char b[192];
    int n = snprintf(b, sizeof b,
                     "[%s] %" PRIu64 " > %" PRIu64 "  100.0%%  %" PRIu64 " frames  done in %.2fs  %5.2fM RAM at most",
                     tag, raw, packed, frames, s, ram(true));
    lock.lock();
    show(b, n, "\n");
    up = false;
    lock.unlock();
  }
  // the end of a call, r its status: done, or the line ended after an error
  void end(int r, uint64_t raw, uint64_t packed, uint64_t frames) {
    if (r)
      end_line();
    else
      done(raw, packed, frames);
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
// whether the descriptor fd of this process writes the regular file f reads
bool same_fd(FILE* f, int fd) {
#ifdef _WIN32
  struct _stati64 a, b;
  return _fstati64(fd, &a) == 0 && _fstati64(_fileno(f), &b) == 0 && (a.st_mode & _S_IFMT) == _S_IFREG &&
         a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_ino != 0;
#else
  struct stat a, b;
  return fstat(fd, &a) == 0 && fstat(fileno(f), &b) == 0 && S_ISREG(a.st_mode) && a.st_dev == b.st_dev &&
         a.st_ino == b.st_ino;
#endif
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
// whether two names are one file that is there (a link, another path)
bool same_names(const char* a, const char* b) {
  if (!strcmp(a, b)) return true;
#ifdef _WIN32
  FILE* f = fopen(a, "rb");
  bool r = f && same_file(f, b);
  if (f) fclose(f);
  return r;
#else
  struct stat x, y;
  return stat(a, &x) == 0 && stat(b, &y) == 0 && x.st_dev == y.st_dev && x.st_ino == y.st_ino;
#endif
}

// the descriptor of this process that name opens again: /dev/stdout,
// /dev/stderr, /dev/fd/N, /proc/self/fd/N or /proc/PID/fd/N, a link to one
// of them, or the file stdout has open; -1 none
int name_fd(const char* name) {
#ifdef _WIN32
  (void)name;
  return -1;
#else
  if (same_file(stdout, name)) return 1;
  char a[4096], b[4096], self[48];
  snprintf(self, sizeof self, "/proc/%ld/fd/", (long)getpid());
  const char* dirs[] = {"/dev/fd/", "/proc/self/fd/", self};
  if (strlen(name) >= sizeof a) return -1;
  strcpy(a, name);
  for (int hop = 0; hop < 40; hop++) {  // (the links followed one by one)
    int fd = !strcmp(a, "/dev/stdout") ? 1 : !strcmp(a, "/dev/stderr") ? 2 : -1;
    for (const char* d : dirs) {
      size_t m = strlen(d);
      if (!strncmp(a, d, m) && a[m] && strlen(a + m) <= 9 && strspn(a + m, "0123456789") == strlen(a + m)) fd = atoi(a + m);
    }
    if (fd >= 0) return fcntl(fd, F_GETFD) < 0 ? -1 : fd;
    ssize_t k = readlink(a, b, sizeof b - 1);
    if (k < 0) return -1;
    b[k] = 0;
    const char* s = b[0] == '/' ? nullptr : strrchr(a, '/');  // (relative: to the link's directory)
    size_t at = s ? (size_t)(s - a) + 1 : 0;
    if (at + (size_t)k >= sizeof a) return -1;
    memcpy(a + at, b, (size_t)k + 1);
  }
  return -1;
#endif
}

// An output of a call (aacpack's): "-" is stdout. A regular file, or a new
// one, is written under a temporary name beside it and renamed to its own
// as the call ends well, so that a failed call leaves what was there (d
// keeps what it restored before an error only where there was nothing);
// anything else (a device, a pipe) is written as it is. A name that opens
// one of the program's descriptors again (/dev/stdout, name_fd()) is
// written through that descriptor, as "-" is. Never an input, and never
// through a link to nothing. (Quit() closes and removes what close() did
// not: the program ends with Quit() of the call's outputs (at exit), and
// on POSIX on a signal that ends it, with their temporary files removed.)
#if !defined(_WIN32) && !defined(O_NOFOLLOW)
#define O_NOFOLLOW 0  // (where a link cannot be refused: as fopen())
#endif
struct Output;
const int kOutputs = 2;
Output* open_outputs[kOutputs];  // the call's (the main thread's)

struct Output {
  const char* name;  // as given
  char* dest;        // the file (mem_alloc()'d), and the temporary one written (none: name itself)
  char* volatile tmp;
  bool file;   // a regular file is written
  bool fresh;  // and there was none
  FILE* g;

  void Init() {
    name = nullptr;
    dest = tmp = nullptr;
    file = fresh = false;
    g = nullptr;
  }
  void Quit() {
    if (g && g != stdout) fclose(g);
    g = nullptr;
    char* t = tmp;
    tmp = nullptr;  // (before it is freed: quit_on_signal() reads it)
    if (t) remove(t);
    mem_free(t);
    mem_free(dest);
    dest = nullptr;
    for (int i = 0; i < kOutputs; i++)
      if (open_outputs[i] == this) open_outputs[i] = nullptr;
  }
  static char* str_dup(const char* s) { return (char*)memcpy(mem_alloc(strlen(s) + 1), s, strlen(s) + 1); }

  // A file of its own beside dest, new (none of that name made before, not
  // through a link): dest.PID.tmp, or where that name is too long
  // .mp3pack.PID.tmp in dest's directory, each with a number more where
  // one is there; tmp its name once it is made (quit_on_signal() removes
  // tmp), null where none can be (errno why)
  FILE* make_tmp(long pid) {
    const char* slash = strrchr(dest, '/');
#ifdef _WIN32
    const char* bs = strrchr(dest, '\\');
    if (bs && (!slash || bs > slash)) slash = bs;
#endif
    const size_t dn = slash ? (size_t)(slash - dest) + 1 : 0, n = strlen(dest) + 48;
    char* name_of = (char*)mem_alloc(n);
    for (int k = 0; k < 64; k++) {
      for (int shortname = 0; shortname < 2; shortname++) {
        char num[24] = "";
        if (k) snprintf(num, sizeof num, "-%d", k);
        if (!shortname)
          snprintf(name_of, n, "%s.%ld%s.tmp", dest, pid, num);
        else
          snprintf(name_of, n, "%.*s.mp3pack.%ld%s.tmp", (int)dn, dest, pid, num);
#ifdef _WIN32
        int fd = _open(name_of, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
        int fd = ::open(name_of, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0666);
#endif
        if (fd >= 0) {
          tmp = name_of;
          FILE* f = fdopen(fd, "wb");
          if (f) return f;
          int e = errno;
          ::close(fd);
          tmp = nullptr;
          remove(name_of);
          mem_free(name_of);
          errno = e;
          return nullptr;
        }
        if (errno != ENAMETOOLONG) break;  // (else the short name)
      }
      if (errno != EEXIST) break;  // (else one more)
    }
    int e = errno;
    mem_free(name_of);
    errno = e;
    return nullptr;
  }

  // out_name opened, as none of the inputs ins[0..nin) (false: said, and Quit())
  bool open(const char* out_name, FILE* const* ins, int nin, const char* const* in_names) {
    for (int i = 0; i < kOutputs; i++)
      if (!open_outputs[i]) {
        open_outputs[i] = this;
        break;
      }
    name = out_name;
    int fd = !strcmp(name, "-") ? 1 : name_fd(name);
    for (int i = 0; i < nin; i++)
      if (fd >= 0 ? same_fd(ins[i], fd) : same_file(ins[i], name)) {
        fprintf(stderr, "mp3pack: %s: an input and an output are the same file\n", in_names[i]);
        Quit();
        return false;
      }
    if (fd >= 0) {
      int d = fd == 1 ? -1 : dup(fd);
      g = fd == 1 ? stdout : d < 0 ? nullptr : fdopen(d, "wb");
      if (!g) {
        int e = errno;
        if (d >= 0) ::close(d);
        fprintf(stderr, "mp3pack: cannot write %s: %s\n", name, strerror(e));
        Quit();
        return false;
      }
      return true;
    }
    unsigned mode = 0;
    int kind = name_kind(name, &mode);
    fresh = kind < 0;
    file = fresh || kind == 1;
#ifndef _WIN32
    struct stat ls;
    if (fresh && lstat(name, &ls) == 0 && S_ISLNK(ls.st_mode)) {
      fprintf(stderr, "mp3pack: %s: a link to a file that is not there\n", name);
      Quit();
      return false;
    }
#endif
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
      g = make_tmp(pid);
      if (!g) {
        fprintf(stderr, "mp3pack: cannot create a file beside %s to write it: %s\n", name, strerror(errno));
        Quit();
        return false;
      }
#ifndef _WIN32
      if (!fresh) (void)!fchmod(fileno(g), mode & 07777);  // (as private as the file it replaces)
#endif
      return true;
    }
    g = fopen(name, "wb");  // (a device, a pipe)
    if (!g) {
      fprintf(stderr, "mp3pack: cannot create %s: %s\n", name, strerror(errno));
      Quit();
      return false;
    }
    return true;
  }
  // all n bytes of p (false: a write error, said at close())
  bool put(const void* p, size_t n) { return !n || fwrite(p, 1, n, g) == n; }
  // The end of the call, r its status so far: what it wrote becomes the
  // output, or after an error is removed, unless keep (d keeps what it
  // restored, if it could write it, where there was no file); the exit
  // status. (Messages after p's line, if any.)
  int close(int r, bool keep, Progress* p) {
    bool werr = false;
    if (g == stdout)
      werr = fflush(stdout) != 0 || ferror(stdout) != 0;
    else if (g) {
      werr = ferror(g) != 0;
      werr = fclose(g) != 0 || werr;  // (closed whatever ferror says)
    }
    g = nullptr;
    if (!r && werr) {
      say_error(p, "mp3pack: %s: write error\n", name);
      r = 1;
    }
    bool stay = !r || (keep && !werr && (fresh || !file));
    if (r && keep && !werr && file && !fresh) say_error(p, "mp3pack: %s was there: left as it was\n", name);
    if (tmp) {
      if (stay && !move_file(tmp, dest)) {
        say_error(p, "mp3pack: cannot rename %s to %s: %s\n", tmp, dest, strerror(errno));
        r = 1, stay = false;
      }
      char* t = tmp;
      tmp = nullptr;  // (before it is freed: quit_on_signal() reads it)
      if (!stay) remove(t);
      mem_free(t);
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

void quit_outputs() {
  for (int i = 0; i < kOutputs; i++)
    if (open_outputs[i]) open_outputs[i]->Quit();
}
const bool outputs_hooked = !atexit(quit_outputs);  // (as the program starts)
#ifndef _WIN32
// a signal that ends the program: the temporary files of the call's
// outputs go first (only what is safe in a signal handler)
void quit_on_signal(int sig) {
  for (int i = 0; i < kOutputs; i++) {
    Output* o = open_outputs[i];
    const char* t = o ? o->tmp : nullptr;
    if (t) unlink(t);
  }
  signal(sig, SIG_DFL);
  raise(sig);
}
bool hook_signals() {
  // (each that ends a process by default)
  const int sigs[] = {SIGINT, SIGTERM, SIGHUP, SIGPIPE, SIGQUIT, SIGALRM, SIGUSR1, SIGUSR2, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF};
  for (int s : sigs)
    if (signal(s, quit_on_signal) == SIG_IGN) signal(s, SIG_IGN);  // (one ignored, as by nohup, stays so)
  return true;
}
const bool signals_hooked = hook_signals();  // (as the program starts)
#endif

// an input of a call: "-" is stdin (null: said)
FILE* open_input(const char* name) {
  FILE* f = !strcmp(name, "-") ? stdin : fopen(name, "rb");
  if (!f) fprintf(stderr, "mp3pack: cannot open %s: %s\n", name, strerror(errno));
  return f;
}
// and its end: false after a read error (said)
bool close_input(FILE* f, const char* name, Progress* p) {
  bool ok = !ferror(f);
  if (!ok) say_error(p, "mp3pack: %s: read error\n", name);
  if (f != stdin) fclose(f);
  return ok;
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

}  // namespace mp3pack
