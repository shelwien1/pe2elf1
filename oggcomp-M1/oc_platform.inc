#include <inttypes.h>
#include <math.h>
#include <new>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef size_t sz;
typedef u8 byte;
typedef u16 word;
typedef u32 uint;
typedef u64 qword;

#if defined(__GNUC__) || defined(__clang__)
static qword g_prog_in __attribute__((__unused__));
#else
static qword g_prog_in;
#endif

#undef __min
#undef __max
#ifndef __min
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ALIGN
#if defined(__GNUC__) || defined(__clang__)
#define ALIGN(n) __attribute__((aligned(n)))
#else
#define ALIGN(n) __attribute__((align(n)))
#endif
#endif
#define if_e0(x) if(__builtin_expect((x), 0))
#define if_e1(x) if(__builtin_expect((x), 1))
#ifndef _MSC_VER
#define __assume(x)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
  do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
    if(!(x))                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
      __builtin_unreachable();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
  } while(0)
#endif
template <class T, int N> constexpr uint DIM(T (&)[N]) { return N; }
#if defined(__x86_64) || defined(_M_X64)
#define X64
#endif
#if (defined(__GNUC__) || defined(__clang__)) && !defined(OGC_NO_ATTRS)
#define INLINE inline __attribute__((__always_inline__))
#define NOINLINE __attribute__((__noinline__))
#define HOT __attribute__((__hot__))
#define FLATTEN __attribute__((__flatten__))
#define NORETURN __attribute__((__noreturn__))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define OGC_PRINTF(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define INLINE inline
#define NOINLINE
#define HOT
#define FLATTEN
#define NORETURN
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define OGC_PRINTF(f, a)
#endif
static INLINE u32 ogc_ilog(u32 v) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(OGC_NO_ATTRS)
  return v ? 32 - (u32)__builtin_clz(v) : 0;
#else
  u32 n = 0;
  while(v) {
    n++;
    v >>= 1;
  }
  return n;
#endif
}

#ifdef _WIN32
#include <windows.h>
//  stdio unless asked otherwise.  -DFILE_API_WIN takes Lib3/file_api_win.inc
//  instead, which is CreateFile/ReadFile/WriteFile and no FILE* anywhere --
//  so nothing here may reach into filehandle::f.  Read and write errors are
//  asked for as f.error(), which both backends answer.
#ifndef FILE_API_WIN
#define FILE_API_STD 1
#endif
#else
#define FILE_API_STD 1
#include <sys/mman.h>
#endif
