// What the coder and the model families need around them: integer types,
// compiler hints, table allocation and the error of a corrupt stream. From
// oggcomp (balrogg-P-hash: oc_platform.inc, tc_base.inc) by way of h264p,
// cut down to what aacpack uses.

// (the system headers it needs, <sys/mman.h> or <windows.h>, <cinttypes>,
// <cmath>, <cstdarg> and with AVX2 <immintrin.h>, come first, before Lib3's,
// whose setjmp and ALIGN macros must not reach them: aacpack.inc)

// AVX2 (x86-64-v3, what the build targets) for the mixers and the
// counters; the plain C++ beside it, for other targets, gives the same
// results
#if defined(__AVX2__)
#define AACPACK_AVX2 1
#endif

// (INLINE and NOINLINE as Lib3/common.inc has them, where it came first)
#if defined(__GNUC__) || defined(__clang__)
#ifndef INLINE
#define INLINE inline __attribute__((__always_inline__))
#endif
#ifndef NOINLINE
#define NOINLINE __attribute__((__noinline__))
#endif
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CM_PREFETCH(p) __builtin_prefetch(p)
#define CM_PREFETCHW(p) __builtin_prefetch(p, 1)
#else
#ifndef INLINE
#define INLINE inline
#endif
#ifndef NOINLINE
#define NOINLINE
#endif
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define CM_PREFETCH(p)
#define CM_PREFETCHW(p)
#endif

#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

namespace aacpack {

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef size_t sz;
typedef u8 byte;
typedef u16 word;  // idx2inc.pl's maps of variables with more than 256 values
typedef u32 uint;

// 0..3 as they are, then two buckets per octave: a magnitude class
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64)(x < 0 ? -x : x);
  if (u < 4) return (i32)u;
#if defined(__GNUC__) || defined(__clang__)
  int n = 62 - __builtin_clzll(u);
#else
  int n = -1;
  for (u64 t = u; t > 1; t >>= 1) n++;
#endif
  return 2 + n * 2 + (i32)((u >> n) & 1);
}
static INLINE i32 tc_sq(i32 v) {
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
constexpr int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Tables are mapped, not allocated and filled: a table costs address space
// when it is made and memory only where the coder reaches it.
static void* tc_map(sz n, const char* what = "a model table") {
  void* p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) p = nullptr;
#endif
  if (!p && n >> 20) err_raise("cannot map %" PRIu64 " MB for %s", (u64)(n >> 20), what);
  if (!p) err_raise("cannot map %" PRIu64 " KB for %s", (u64)((n + 1023) >> 10), what);
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

// A Volume is a product of factor sizes; saturating, so that a check
// downstream sees at least what was asked for (IDX-FORMAT.md sec.6).
constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }
static INLINE u64 tbl_n(u64 n) {
  if (!(n > 0 && n <= ((u64)1 << 40))) err_raise("a model table of %" PRIu64 " entries", n);
  return n;
}
// Rows of a table: the Volume when it fits in 2^h rows, else 2^h rows into
// which the contexts are hashed (tc_hrow).
constexpr int tc_hbits(int h) { return h < 1 ? 1 : h > 31 ? 31 : h; }
constexpr sz tc_rows(long long vol, int h) { return vol <= (1LL << tc_hbits(h)) ? (sz)vol : (sz)1 << tc_hbits(h); }
// A hashed table's row for a context: the index multiplied out, taken from
// the top. No tag and no check: contexts that land on the same row share it.
static INLINE u32 tc_hrow(u32 idx, int bits) { return (idx * 0x9E3779B1u) >> (32 - bits); }

}  // namespace aacpack
