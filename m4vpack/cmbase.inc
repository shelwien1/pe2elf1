// cmbase.inc -- what the coder and the model families need around them: integer types, compiler
// hints, table mapping (h264p's cmbase.inc, cut down to what m4vpack uses).

// AVX2 (-march=x86-64-v3) for the mixers and the counters, where the build targets it; the plain
// C++ beside it, for other targets, gives the same results
#if defined(__AVX2__)
#define M4V_AVX2 1
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef size_t sz;

#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CM_PREFETCH(p) __builtin_prefetch(p)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define CM_PREFETCH(p)
#endif
#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

// An error that ends the program: a bug (an "internal" one), or a tuning build's parameters
// grown past what the tables can be (the optimizer then takes the run for the worst). Damaged
// data never comes here: the coder notes it (Coder::err) and the walk fails the chunk.
[[noreturn]] static void fatal(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("m4vpack: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(E_INTERNAL);
}
static void* xmalloc(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) fatal("out of memory");
  return p;
}
static void* xrealloc(void* p, size_t n) {
  void* q = realloc(p, n ? n : 1);
  if (!q) fatal("out of memory");
  return q;
}

// the place of u's top bit (u > 0)
static INLINE int tc_top_bit(u64 u) {
#if defined(__GNUC__) || defined(__clang__)
  return U64_BITS - 1 - __builtin_clzll(u);
#else
  int n = 0;
  while (u >>= 1) n++;
  return n;
#endif
}
// 0..TC_QLOG_LIN - 1 as they are, then two buckets per octave (by the bit below the top one): a
// magnitude class
constexpr int TC_QLOG_LIN = 4;
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64)(x < 0 ? -x : x);
  int t;
  if (u < TC_QLOG_LIN) return (i32)u;
  t = tc_top_bit(u);
  return 2 * t + (i32)((u >> (t - 1)) & 1);
}
constexpr int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Tables are mapped, not allocated and filled: a table costs address space when it is made and
// memory only where the coder reaches it (zeroed pages).
static void* tc_map(sz n, const char* what = "a model table") {
  void* p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) p = nullptr;
#endif
  if (!p) fatal("cannot map %llu KB for %s", (unsigned long long)((n + KIB - 1) / KIB), what);
  return p;
}
static inline void tc_unmap(void* p, sz n) {
  if (!p) return;
#ifdef _WIN32
  (void)n;
  VirtualFree(p, 0, MEM_RELEASE);
#else
  munmap(p, n);
#endif
}

// A Volume is a product of factor sizes; saturating, so that a check downstream sees at least
// what was asked for (IDX-FORMAT.md sec.6).
constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }
constexpr u64 TC_TBL_MAX = (u64)1 << 40;  // a table's entries, at most
static INLINE u64 tbl_n(u64 n) {
  if (!(n > 0 && n <= TC_TBL_MAX)) fatal("a model table of %llu entries", (unsigned long long)n);
  return n;
}
// Rows of a table: the Volume when it fits in 2^h rows, else 2^h rows into which the contexts
// are hashed (tc_hrow).
constexpr int TC_HBITS_MAX = U32_BITS - 1;
constexpr int tc_hbits(int h) { return h < 1 ? 1 : h > TC_HBITS_MAX ? TC_HBITS_MAX : h; }
constexpr sz tc_rows(long long vol, int h) { return vol <= (1LL << tc_hbits(h)) ? (sz)vol : (sz)1 << tc_hbits(h); }
// A hashed table's row for a context: the index multiplied out (by 2^32 over the golden ratio),
// taken from the top. No tag and no check: contexts that land on the same row share it.
constexpr u32 TC_HASH_MUL = 0x9E3779B1u;
static INLINE u32 tc_hrow(u32 idx, int bits) { return (idx * TC_HASH_MUL) >> (U32_BITS - bits); }


// type traits famcfg.inc uses (the C++ library's are not here)
template <class T>
struct NoRef {
  typedef T type;
};
template <class T>
struct NoRef<T&> {
  typedef T type;
};
template <class A, class B>
struct SameType {
  static const bool value = false;
};
template <class A>
struct SameType<A, A> {
  static const bool value = true;
};
template <class T>
struct Extent {
  static const size_t value = 0;
};
template <class T, size_t N>
struct Extent<T[N]> {
  static const size_t value = N;
};

#include "buf.inc"  // Buf: a growing array
