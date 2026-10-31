// base.inc -- what every part of ac3pack (and IDX/famtune.cpp) needs first: the integer types, the
// units, the exit statuses and the sizes of the buffers.

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef short S16;
typedef unsigned int U32;
typedef int S32;
typedef unsigned long long U64;
typedef long long S64;

// Restrict pointers (__restrict): while one is in use, what it points at is reached through it
// alone, so the compiler may keep values in registers across stores through other pointers. rp<T>
// for any T; the common ones by name (pU8: U8*, pcU8: const U8*, ...).
template <class T>
using rp = T* __restrict;
typedef rp<U8> pU8;
typedef rp<const U8> pcU8;
typedef rp<int> pInt;
typedef rp<const int> pcInt;

#define countof(a) (int)(sizeof(a) / sizeof((a)[0]))
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }

// units
constexpr int BYTE_SHIFT = 3, BYTE_BITS = 1 << BYTE_SHIFT;
constexpr int BYTE_VALUES = 1 << BYTE_BITS;
constexpr U32 BYTE_MASK = BYTE_VALUES - 1;
constexpr int U32_BYTES = sizeof(U32), U64_BYTES = sizeof(U64);
constexpr int U32_BITS = U32_BYTES * BYTE_BITS, U64_BITS = U64_BYTES * BYTE_BITS;
constexpr U32 KIB = 1u << 10, MIB = 1u << 20;
constexpr double PERCENT = 100.0;  // (a share in percent)

// v clamped to [lo, hi]: v of a signed type of int's size or more, the bounds converted to it
template <class T>
struct NoDeduce {
  typedef T type;
};
template <class T>
constexpr T clip(T v, typename NoDeduce<T>::type lo, typename NoDeduce<T>::type hi) {
  static_assert(sizeof(T) >= sizeof(int) && T(-1) < T(0), "clip(): a signed type of int's size or more");
  return v < lo ? lo : v > hi ? hi : v;
}
// floor(log2(v)): the place of v's top bit (0 for 0)
static INLINE int ilog2(U64 v) {
#if defined(__GNUC__) || defined(__clang__)
  return v ? U64_BITS - 1 - __builtin_clzll(v) : 0;
#else
  int n = 0;
  while (v >>= 1) n++;
  return n;
#endif
}

// the exit statuses, and those of a run's errors (CoIo::fail()); E_INTERNAL: or out of memory
enum { E_USAGE = 1, E_READ = 2, E_WRITE = 3, E_DATA = 4, E_INTERNAL = 5 };

// The input is cut into frames and the bytes between them (reader.inc): its buffer holds the
// longest run of bytes handled at once (longer ones come in pieces), the reads come IREAD at a
// time, and a run takes its input and gives its output in buffers of IO_BUF.
#ifndef AC3P_SMALL_BUFFERS
constexpr U32 IBUF_SIZE = 16 * MIB;
constexpr U32 IREAD = 1 * MIB;
#else  // (tests: runs of bytes in pieces, reads of a few bytes)
constexpr U32 IBUF_SIZE = 64 * KIB;
constexpr U32 IREAD = 7;
#endif
constexpr U32 IO_BUF = 64 * KIB;
