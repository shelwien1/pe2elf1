
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
//#undef EOF

//#pragma pack(1)

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

template <class T,int N> constexpr int DIM( T (&wr)[N] ) { return sizeof(wr)/sizeof(wr[0]); };

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

#define if_e0(x) if(__builtin_expect((x),0))
#define if_e1(x) if(__builtin_expect((x),1))

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #define __builtin_expect(x,y) (x)
#endif

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 13))
  #define __assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
 #else
  #define __assume(x) (x)
 #endif
#endif

#if defined(__x86_64) || defined(_M_X64)
 #define X64
#else
 #undef X64
#endif
