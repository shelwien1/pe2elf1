
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

// Aligned allocation.  The C11/C++17 aligned_alloc pairs with free(), which
// Windows cannot do -- _aligned_malloc blocks carry a header free() knows
// nothing about -- so every such block goes back through aligned_free(),
// which is the one call that means the right thing on both.
#if defined(_WIN32) || defined(_MSC_VER)
#include <malloc.h>

// Wrapper to match standard C11/C++17 aligned_alloc signature
inline void* aligned_alloc( size_t alignment, size_t size ) {
    return _aligned_malloc( size, alignment );
}

inline void aligned_free( void* ptr ) {
    _aligned_free( ptr );
}

#else
#include <cstdlib>

// Standard C++17 fallback (aligned_alloc uses std::free for deallocation)
inline void aligned_free( void* ptr ) {
    std::free( ptr );
}

#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline))
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// Both spell it the same way, and the C keyword is not one in C++.  Defined
// after the system headers, which use __restrict directly.
#define restrict __restrict

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

// -------------------------------------------------------------
// Fast float exp/log.
//
// The counter spends most of its time here: every Predict/C_Update pair
// runs about ten of them, and with six residual models per bit that is
// ~1e9 calls on a 2 MB image -- measured as 64% of decode.  libm's expf
// and logf are correctly rounded and cost 20-40 cycles; the model does not
// need correct rounding, it needs a smooth monotone function it can be
// consistent about.  These are the classic exponent-field constructions
// with a minimax polynomial on the mantissa, ~1e-5 relative, ~6 cycles,
// and branch free.
//
// Consistency, not accuracy, is what keeps the codec correct: the encoder
// and the decoder run the same instructions, so any deterministic function
// round-trips.  What accuracy buys is compression, and at 1e-5 it costs
// nothing measurable.
// -------------------------------------------------------------
static INLINE float fexp2f( float x ) {
  x = fminf(fmaxf(x,-126.0f),126.0f);
  float xi = floorf(x);
  float f  = x - xi;
  // minimax cubic for 2^f on [0,1)
  float p = 1.0f + f*(0.6959955f + f*(0.2262697f + f*0.0782804f));
  union { float f; int i; } u;
  u.i = (int(xi) + 127) << 23;
  return p * u.f;
}
static INLINE float flog2f( float x ) {
  union { float f; uint i; } u;
  u.f = x;
  float e = float(int(u.i >> 23) - 127);
  u.i = (u.i & 0x007FFFFFu) | 0x3F800000u;      // mantissa into [1,2)
  float m = u.f;
  // minimax quartic for log2(m) on [1,2)
  float p = -1.7417939f + m*(2.8212026f + m*(-1.4699568f + m*(0.4478679f
            + m*(-0.0565717f))));
  return e + p;
}
static INLINE float fexpf( float x ) { return fexp2f( x * 1.4426950409f ); }
static INLINE float flogf( float x ) { return flog2f( x ) * 0.6931471806f; }

// --- Configuration Struct Declarations ---

#define def_Config(Config) struct Config {\
  static const float momentum_D, momentum_R, NW, inc, stepMax, minVal,maxVal, grad1_clip, grad2_clip, D_clip, R_clip, R0, hbeta, efw; };
