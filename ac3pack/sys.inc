// sys.inc -- what the progress line (progress.inc) asks of the system, on POSIX and Windows: a
// clock, the size of an open file, whether stderr is a terminal; and text put into a buffer.

constexpr S64 NS = 1000000000;  // nanoseconds in a second

// The time in nanoseconds, by a clock that only goes forward
static S64 clock_ns(void) {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (S64)(c.QuadPart / f.QuadPart * NS + c.QuadPart % f.QuadPart * NS / f.QuadPart);
#else
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (S64)t.tv_sec * NS + t.tv_nsec;
#endif
}

// A line of text (b, of n bytes) after the k bytes it has: fmt's
static void put_text(char* b, size_t n, size_t& k, const char* fmt, ...) {
  va_list ap;
  int l;
  if (k >= n) return;
  va_start(ap, fmt);
  l = vsnprintf(b + k, n - k, fmt, ap);
  va_end(ap);
  if (l > 0) k = k + (size_t)l < n - 1 ? k + (size_t)l : n - 1;
}

// The size of the file f has open, 0 where it is not a regular file (a pipe: no percentage)
static U64 file_size(FILE* f) {
#ifdef _WIN32
  struct _stat64 s;
  if (_fstat64(_fileno(f), &s) || (s.st_mode & _S_IFMT) != _S_IFREG) return 0;
#else
  struct stat s;
  if (fstat(fileno(f), &s) || !S_ISREG(s.st_mode)) return 0;
#endif
  return (U64)s.st_size;
}

// whether stderr is a terminal
static int stderr_tty(void) {
#ifdef _WIN32
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(fileno(stderr)) != 0;
#endif
}
