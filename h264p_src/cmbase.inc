// What the coder and the model families need around them: integer types,
// compiler hints, table allocation and the error of a corrupt stream. From
// oggcomp (balrogg-P-hash: oc_platform.inc, tc_base.inc), cut down to what
// h264p uses.

#ifndef _WIN32
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#include <cinttypes>
#include <cmath>
#include <cstdarg>

#if defined(__GNUC__) || defined(__clang__)
#define INLINE inline __attribute__((__always_inline__))
#define NOINLINE __attribute__((__noinline__))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CM_PREFETCH(p) __builtin_prefetch(p)
#else
#define INLINE inline
#define NOINLINE
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define CM_PREFETCH(p)
#endif

#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

namespace h264p {

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
typedef u64 qword;

// The compressed stream cannot be decoded, or a model table cannot be had.
struct CmError : std::runtime_error {
  explicit CmError(const std::string& s) : std::runtime_error(s) {}
};

[[noreturn]] static void cm_fail(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  throw CmError(buf);
}

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
static INLINE int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Tables are mapped, not allocated and filled: a table costs address space
// when it is made and memory only where the coder reaches it.
static void* tc_map(sz n) {
  void* p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) p = nullptr;
#endif
  if (!p) cm_fail("cannot map %" PRIu64 " MB for a model table", (u64)(n >> 20));
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
  if (!(n > 0 && n <= ((u64)1 << 40))) cm_fail("a model table of %" PRIu64 " entries", n);
  return n;
}
// Rows of a table: the Volume when it fits in 2^h rows, else 2^h rows into
// which the contexts are hashed (tc_hrow).
constexpr int tc_hbits(int h) { return h < 1 ? 1 : h > 31 ? 31 : h; }
constexpr sz tc_rows(long long vol, int h) { return vol <= (1LL << tc_hbits(h)) ? (sz)vol : (sz)1 << tc_hbits(h); }
// A hashed table's row for a context: the index multiplied out, taken from
// the top. No tag and no check: contexts that land on the same row share it.
static INLINE u32 tc_hrow(u32 idx, int bits) { return (idx * 0x9E3779B1u) >> (32 - bits); }

}  // namespace h264p
