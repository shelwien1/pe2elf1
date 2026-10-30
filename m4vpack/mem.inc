// mem.inc -- memory, and how the program ends when it can't go on: arrays that grow (grow()),
// zeroed pages for the models' tables and the runs (map_pages(), unmap_pages()), and die(), which
// says why after the progress line (die_hook) and exits: out of memory (no_memory()), a bug
// (fatal()).

// what die() calls first: the progress line's end (progress.inc), where there is one
static void (*die_hook)(void) = nullptr;

[[noreturn]] static void die_v(int code, const char* fmt, va_list ap) {
  if (die_hook) die_hook();
  fputs("m4vpack: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  exit(code);
}
// The end of the program, its exit status code, fmt's message saying why
[[noreturn]] static void die(int code, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  die_v(code, fmt, ap);
}
// An error that ends the program: a bug (an "internal" one), or a tuning build's parameters grown
// past what the tables can be (the optimizer then takes the run for the worst). Damaged data never
// comes here: the coder notes it (Coder::err) and the walk fails the chunk.
[[noreturn]] static void fatal(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  die_v(E_INTERNAL, fmt, ap);
}
[[noreturn]] static void no_memory(void) { die(E_INTERNAL, "out of memory"); }

static void* xmalloc(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) no_memory();
  return p;
}

// *p, room T's, grown to n T's at least: doubled, from first T's (0: no memory, *p as it was)
template <class T>
static int grow(T** p, size_t* room, size_t n, size_t first = 1) {
  size_t r = *room ? *room : first;
  T* q;
  if (n <= *room) return 1;
  while (r < n) r = r <= (size_t)-1 / 2 / sizeof(T) ? 2 * r : n;
  if (!(q = (T*)realloc(*p, r * sizeof(T)))) return 0;
  *p = q;
  *room = r;
  return 1;
}

// n bytes of zeroed pages, which take memory only where they are written (0: no memory); and
// unmapped
static void* map_pages(size_t n) {
  void* p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) p = nullptr;
#endif
  return p;
}
static void unmap_pages(void* p, size_t n) {
  if (!p) return;
#ifdef _WIN32
  (void)n;
  VirtualFree(p, 0, MEM_RELEASE);
#else
  munmap(p, n);
#endif
}
