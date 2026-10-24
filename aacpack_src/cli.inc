// What a call of aacpack has around it: the command line's options, the
// progress line on stderr and the messages that cut it, and its files
// (Output: written under a temporary name, renamed as the call ends well),
// as h264p's cli.inc.

namespace aacpack {

// The command line's options
struct Options {
  bool verbose;
  bool strict;
  bool progress;  // -p: progress even where stderr is not a terminal
  int level;      // c's model: -L
  int format;     // c: the container to look for (F_*: -adts, -latm, -adif)
  uint8_t asc[64];  // c: -asc, the AudioSpecificConfig of raw access units
  size_t asc_n;

  // the defaults
  void Init() {
    verbose = strict = progress = false;
    level = kLevels[0];
    format = 0;
    asc_n = 0;
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
// What the stream is (rcio.inc's AudioInfo), in words, into b (n bytes):
// its container, object type, sampling rate, channels, SBR and PS
void audio_words(const AudioInfo& a, char* b, size_t n) {
  static const char* const kCont[7] = {"?", "ADTS", "LOAS/LATM", "ADIF", "AUS1", "raw access units", "MP4"};
  static const char* const kAot[] = {"?", "AAC Main", "AAC LC", "AAC SSR", "AAC LTP", "SBR", "AAC Scalable"};
  static const int kRate[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
  size_t k = 0;
  put_text(b, n, k, "%s", kCont[a.container >= 0 && a.container < 7 ? a.container : 0]);
  if (a.aot >= 1 && a.aot <= 6)
    put_text(b, n, k, ", %s", kAot[a.aot]);
  else if (a.aot == 17)
    put_text(b, n, k, ", ER AAC LC");
  else if (a.aot == 23)
    put_text(b, n, k, ", ER AAC LD");
  else if (a.aot == 39)
    put_text(b, n, k, ", ER AAC ELD");
  else
    put_text(b, n, k, ", object type %d", a.aot);
  if (a.sbr) put_text(b, n, k, a.ps ? " + SBR + PS (HE-AAC v2)" : " + SBR (HE-AAC)");
  if (a.sf_index >= 0 && a.sf_index < 13) put_text(b, n, k, ", %d Hz", kRate[a.sf_index]);
  if (a.chan_config)
    put_text(b, n, k, ", channel configuration %d", a.chan_config);
  else
    put_text(b, n, k, ", channels by a PCE");
  if (a.frame960) put_text(b, n, k, ", 960-sample frames");
}

// Progress on stderr, as c and d go: a line put over itself every half
// second -- the bytes of the stream (raw) and of the compressed stream
// (packed) done so far, how far through its input the call is, the
// frames, the speed (of the stream) and the memory the process has now
// (its resident set) -- and at the end, how long it all took and the most
// memory it had. When stderr is a terminal, or with -p. Before the lines,
// what the stream is, as the codec tells it (audio()).
// what() gives the counts (of what_ctx); poll() puts a line if one is
// due, from any thread (with -t, the workers', as they code: the main
// thread waits on them). Messages go through say() (below), which puts
// them after the line, under the same lock: an error ends the lines.
struct Progress {
  const char* tag;  // enc, dec; none: no progress
  uint64_t total;   // the input's size (0: not known, no percentage)
  // pos of total: raw or packed; pics: the frames
  void (*what)(void* ctx, uint64_t& raw, uint64_t& packed, uint64_t& pos, uint64_t& pics);
  void* what_ctx;
  AudioInfo shown;  // the stream so far (AudioInfo::add(); shown.container 0: none)
  AudioInfo said[8];  // what was said of it (an MP4 file's tracks take turns: each once)
  int nsaid;
  bool verbose;     // -v: the stream said where there is no line too, and its changes
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
    memset((void*)&shown, 0, sizeof shown);
    nsaid = 0;
    verbose = false;
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
    uint64_t raw, packed, pos, pics;
    f(what_ctx, raw, packed, pos, pics);
    double mbs = raw / (t * 1e-9) / (1 << 20);
    char b[192];
    size_t n = 0;
    put_text(b, sizeof b, n, "[%s] %" PRIu64 " > %" PRIu64, tag, raw, packed);
    if (total) put_text(b, sizeof b, n, "  %5.1f%%", Min(100.0, 100.0 * pos / total));
    put_text(b, sizeof b, n, "  %" PRIu64 " frames  %5.2f MB/s  %5.2fM RAM", pics, mbs, ram(false));
    lock.lock();
    if (!stopped()) show(b, (int)n, "");
    lock.unlock();
  }
  // the end, done (the main thread, no worker left; not after an error)
  void done(uint64_t raw, uint64_t packed, uint64_t pics) {
    if (!tag || stopped()) return;
    double s = (clock_ns() - t0) * 1e-9;
    char b[192];
    int n = snprintf(b, sizeof b,
                     "[%s] %" PRIu64 " > %" PRIu64 "  100.0%%  %" PRIu64 " frames  done in %.2fs  %5.2fM RAM at most",
                     tag, raw, packed, pics, s, ram(true));
    lock.lock();
    show(b, n, "\n");
    up = false;
    lock.unlock();
  }
  // the end of a call, r its status: done, or the line ended after an error
  void end(int r, uint64_t raw, uint64_t packed, uint64_t pics) {
    if (r)
      end_line();
    else
      done(raw, packed, pics);
  }
  // what the stream is, as a codec tells it (coro.inc's show_told()):
  // said as it first comes, where the progress line is shown or with -v
  // (verbose), and with -v where it adds to what was said (once each)
  void audio(const AudioInfo& v) {
    if (!tag && !verbose) return;
    lock.lock();
    bool first = !shown.container;
    bool fresh = false;  // (what it is now was not said before)
    if (shown.add(v)) {
      fresh = true;
      for (int i = 0; i < nsaid && fresh; i++) fresh = memcmp(&said[i], &shown, sizeof shown) != 0;
      if (fresh && nsaid < 8) said[nsaid++] = shown;
    }
    if (fresh && (first || verbose) && !stopped()) {
      char b[400];
      audio_words(shown, b, sizeof b);
      cut();
      if (tag) fprintf(stderr, "[%s] ", tag);
      fprintf(stderr, "%s: %s\n", first ? "audio" : "the audio now", b);
    }
    lock.unlock();
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

// The output of a call: "-" is stdout. A regular file, or a new one, is
// written under a temporary name beside it and renamed to its own as the
// call ends well, so that a failed call leaves what was there; anything else
// (a device, a pipe) is written as it is. A name that opens one of the
// program's descriptors again (/dev/stdout, name_fd()) is written through
// that descriptor, where the shell put it (">>", a file it also writes to),
// as "-" is. Never the input. (Quit() closes
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
  bool file;  // a regular file is written
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
      fprintf(stderr, "aacpack: %s: the input and the output are the same file\n", in_name);
      Quit();
      return false;
    }
    int fd = name_fd(name);
    if (fd >= 0) {
      int d = fd == 1 ? -1 : dup(fd);
      g.f = fd == 1 ? stdout : d < 0 ? nullptr : fdopen(d, "wb");
      if (!g.f) {
        int e = errno;
        if (d >= 0) ::close(d);
        fprintf(stderr, "aacpack: cannot write %s: %s\n", name, strerror(e));
        Quit();
        return false;
      }
      return true;
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
      fprintf(stderr, "aacpack: cannot create %s: %s\n", name, strerror(errno));
      Quit();
      return false;
    }
    return true;
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
      say_error(p, "aacpack: %s: write error\n", name);
      r = 1;
    }
    bool stay = !r || (keep && !werr);
    if (tmp) {
      if (stay && !move_file(tmp, dest)) {
        say_error(p, "aacpack: cannot rename %s to %s: %s\n", tmp, dest, strerror(errno));
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
    fprintf(stderr, "aacpack: cannot open %s: %s\n", in_name, strerror(errno));
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
    say_error(p, "aacpack: read error\n");
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

}  // namespace aacpack
