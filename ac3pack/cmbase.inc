// cmbase.inc -- what the coder and the model families need around them: integer types, compiler
// hints, table mapping (h264p's cmbase.inc, cut down to what ac3pack uses).

// AVX2 (-march=x86-64-v3) for the mixers and the counters, where the build targets it; the plain
// C++ beside it, for other targets, gives the same results
#if defined(__AVX2__)
#define AC3P_AVX2 1
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
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CM_PREFETCH(p) __builtin_prefetch(p)
#else
#define UNLIKELY(x) (x)
#define CM_PREFETCH(p)
#endif
#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif


// 0..TC_QLOG_LIN - 1 as they are, then two buckets per octave (by the bit below the top one): a
// magnitude class
constexpr int TC_QLOG_LIN = 4;
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64)(x < 0 ? -x : x);
  int t;
  if (u < TC_QLOG_LIN) return (i32)u;
  t = ilog2(u);
  return 2 * t + (i32)((u >> (t - 1)) & 1);
}

// Tables are mapped, not allocated and filled: a table costs address space when it is made and
// memory only where the coder reaches it (zeroed pages). tc_map() and tc_unmap() are what the
// generated tables of a tuning build call (IDX/idx2inc.pl, UseNew 1).
static void* tc_map(sz n, const char* what = "a model table") {
  void* p = map_pages(n);
  if (!p) fatal("cannot map %llu KB for %s", (unsigned long long)((n + KIB - 1) / KIB), what);
  return p;
}
static void tc_unmap(void* p, sz n) { unmap_pages(p, n); }

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
constexpr int tc_hbits(int h) { return clip(h, 1, TC_HBITS_MAX); }
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
