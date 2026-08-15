
// -------------------------------------------------------------
// sh_common.inc -- the part of coder0.cpp that is not the model:
// C headers, the integer typedefs, the rangecoder (sh_v2f.inc), the
// IDX mapping runtime (sh_mapping.inc), the float helpers and the
// def_Config macro.  Split out verbatim so bmpc.cpp can share it;
// coder0.cpp includes this and sh_counter0.inc and is otherwise
// unchanged (book1 codes bit-identically).
// -------------------------------------------------------------

// C library headers
#include <stdlib.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>

// Intel SSE/AVX intrinsics to fix the denormal microcode penalty
#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline))
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
 #define restrict __restrict
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

#include "sh_v2f.inc"
static const int mSCALE = SCALE-1;
static const float iSCALE = 1.0f/SCALE;

#include "sh_mapping.inc"

// ---------------------------------------------

static inline float Max( float x, float d ) {
  return fmax(d,x);
}

static inline float clip(float x, float d) {
  return fminf(fmaxf(x, -d), d);
}

static inline float clamp(float p) {
  return fminf(fmaxf(p, 1.0f), float(mSCALE));
}

static inline float clamp(float x, float min_val, float max_val) {
  return fminf(fmaxf(x, min_val), max_val);
}

// --- Configuration Struct Declarations ---

#define def_Config(Config) struct Config {\
  static const float momentum_D, momentum_R, NW, inc, stepMax, minVal,maxVal, grad1_clip, grad2_clip, D_clip, R_clip, R0, hbeta, efw; };
