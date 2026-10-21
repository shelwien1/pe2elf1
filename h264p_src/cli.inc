// What a call of h264p has around it: the command line's options, the
// progress line on stderr and the messages that cut it, and its files
// (Output: written under a temporary name, renamed as the call ends well).

namespace h264p {

// The command line's options
struct Options {
  bool verbose = false;
  bool strict = false;
  bool progress = false;    // -p: progress even where stderr is not a terminal
  int level = kLevels[0];   // c's model: -L
  int threads = 1;          // c's threads: -t (more than one: the stream in parts, mt.inc)
  uint32_t part = 4 << 20;  // and the size of a part, at least: -c
};

// Progress on stderr, as c and d go: a line put over itself every half
// second -- the bytes of the stream (raw) and of the compressed stream
// (packed) done so far, how far through its input the call is, the speed
// (of the stream) and the memory the process has now (its resident set)
// -- and at the end, how long it all took and the most memory it had.
// When stderr is a terminal, or with -p.
// what() gives the counts; poll() puts a line if one is due, from any
// thread (with -t, the workers', as they code: the main thread waits on
// them). Messages go through say() (below), which puts them after the
// line, under the same lock: an error ends the lines.
struct Progress {
  typedef std::chrono::steady_clock clk;
  const char* tag = nullptr;  // enc, dec; none: no progress
  uint64_t total = 0;         // the input's size (0: not known, no percentage)
  std::function<void(uint64_t& raw, uint64_t& packed, uint64_t& pos)> what;  // pos of total: raw or packed
  clk::time_point t0;
  std::atomic<int64_t> last{0};     // the last line, ns after t0
  std::atomic<bool> halted{false};  // an error was reported: no more lines
  std::mutex lock;                  // stderr: one line or message at a time
  int len = 0;                      // the line's length (a shorter one is padded over it)
  bool up = false;                  // a line is up, the cursor after it

  void start(const char* t, uint64_t size) {
    tag = t;
    total = size;
    t0 = clk::now();
  }
  // a line, if half a second has gone by since the last (one thread puts it)
  void poll() {
    if (!tag || !what || halted) return;
    int64_t t = std::chrono::duration_cast<std::chrono::nanoseconds>(clk::now() - t0).count();
    int64_t l = last.load(std::memory_order_relaxed);
    if (t - l < 500000000 || !last.compare_exchange_strong(l, t)) return;
    uint64_t raw, packed, pos;
    what(raw, packed, pos);
    double mbs = raw / (t * 1e-9) / (1 << 20);
    char b[192];
    int n = total ? snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.1f%%  %5.2f MB/s  %5.2fM RAM", tag, raw,
                             packed, std::min(100.0, 100.0 * pos / total), mbs, ram(false))
                  : snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.2f MB/s  %5.2fM RAM", tag, raw, packed,
                             mbs, ram(false));
    std::lock_guard<std::mutex> g(lock);
    if (!halted) show(b, n, "");
  }
  // the end, done (the main thread, no worker left; not after an error)
  void done(uint64_t raw, uint64_t packed) {
    if (!tag || halted) return;
    double s = std::chrono::duration<double>(clk::now() - t0).count();
    char b[192];
    int n = snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  100.0%%  done in %.2fs  %5.2fM RAM at most", tag, raw,
                     packed, s, ram(true));
    std::lock_guard<std::mutex> g(lock);
    show(b, n, "\n");
    up = false;
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
    std::lock_guard<std::mutex> g(lock);
    if (stop && halted.exchange(true)) return;
    cut();
    vfprintf(stderr, fmt, ap);
  }
  // the line ended after an error (none comes after it)
  void end_line() {
    std::lock_guard<std::mutex> g(lock);
    halted = true;
    cut();
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
// A message on stderr, after the progress line of the call (p, if any)
void say(Progress* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (p)
    p->say(false, fmt, ap);
  else
    vfprintf(stderr, fmt, ap);
  va_end(ap);
}
// and an error's, which ends the call: no progress line after it
void say_error(Progress* p, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (p)
    p->say(true, fmt, ap);
  else
    vfprintf(stderr, fmt, ap);
  va_end(ap);
}
void progress_poll(Progress* p) {
  if (p) p->poll();
}

// whether there is a file name, and whether it is a regular file
bool exists(const char* name) {
#ifdef _WIN32
  struct _stati64 s;
  return _stati64(name, &s) == 0;
#else
  struct stat s;
  return stat(name, &s) == 0;
#endif
}
bool is_regular(const char* name) {
#ifdef _WIN32
  struct _stati64 s;
  return _stati64(name, &s) == 0 && (s.st_mode & _S_IFMT) == _S_IFREG;
#else
  struct stat s;
  return stat(name, &s) == 0 && S_ISREG(s.st_mode);
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

// The output of a call: "-" is stdout. A regular file, or a new one, is
// written under a temporary name beside it and renamed to its own as the
// call ends well, so that a failed call leaves what was there; anything else
// (a device, a pipe) is written as it is. Never the input.
struct Output {
  const char* name = nullptr;  // as given
  std::string dest, tmp;       // the file, and the temporary one written (none: name itself)
  bool file = false;           // a regular file is written: it can be written again (kRestart)
  filehandle g = filehandle();

  bool open(const char* out_name, FILE* in, const char* in_name) {
    name = out_name;
    if (!strcmp(name, "-")) {
      g.f = stdout;
      return true;
    }
    if (same_file(in, name)) {
      fprintf(stderr, "h264p: %s: the input and the output are the same file\n", in_name);
      return false;
    }
    bool fresh = !exists(name);
    file = fresh || is_regular(name);
    if (file) {
      dest = name;
#ifdef _WIN32
      long pid = (long)_getpid();
#else
      long pid = (long)getpid();
      if (char* real = fresh ? nullptr : realpath(name, nullptr)) {  // (through a link: beside the file)
        dest = real;
        free(real);
      }
#endif
      tmp = dest + "." + std::to_string(pid) + ".tmp";
      g.f = fopen(tmp.c_str(), "wb");
      if (!g.f) tmp.clear();  // (no room for it beside the file: the file itself)
    }
    if (!g.f) g.f = fopen(name, "wb");
    if (!g.f) {
      fprintf(stderr, "h264p: cannot create %s: %s\n", name, strerror(errno));
      return false;
    }
    return true;
  }
  // empty again, for the call to start over (kRestart)
  bool rewind() {
    fclose(g.f);
    g.f = fopen(tmp.empty() ? name : tmp.c_str(), "wb");
    return g.f != nullptr;
  }
  // The end of the call, r its status so far: what it wrote becomes the
  // output, or after an error is removed, unless keep (d keeps what it
  // restored); the exit status
  int close(int r, bool keep) {
    bool werr = false;
    if (g.f == stdout)
      werr = fflush(stdout) != 0 || ferror(stdout) != 0;
    else if (g.f) {
      werr = ferror(g.f) != 0;
      werr = fclose(g.f) != 0 || werr;  // (closed whatever ferror says)
    }
    g.f = nullptr;
    if (!r && werr) {
      fprintf(stderr, "h264p: %s: write error\n", name);
      r = 1;
    }
    bool stay = !r || keep;
    if (!tmp.empty()) {
      if (stay && !move_file(tmp.c_str(), dest.c_str())) {
        fprintf(stderr, "h264p: cannot rename %s to %s: %s\n", tmp.c_str(), dest.c_str(), strerror(errno));
        r = 1, stay = false;
      }
      if (!stay) remove(tmp.c_str());
      tmp.clear();
    } else if (!stay && file) {
      remove(name);
    }
    return r ? 1 : 0;
  }
  // (a call that ends by an exception leaves no temporary file)
  ~Output() {
    if (g.f && g.f != stdout) fclose(g.f);
    if (!tmp.empty()) remove(tmp.c_str());
  }
  static bool move_file(const char* from, const char* to) {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(from, to) == 0;
#endif
  }
};

// a call's input (f) and output (o); false after a message
bool open_io(const char* in_name, const char* out_name, filehandle& f, Output& o) {
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
int close_io(int r, filehandle& f, Output& o, bool keep) {
  if (!r && ferror(f.f)) {
    fprintf(stderr, "h264p: read error\n");
    r = 1;
  }
  if (f.f != stdin) fclose(f.f);
  return o.close(r, keep);
}

void put(FILE* f, const std::string& s) { fwrite(s.data(), 1, s.size(), f); }

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
