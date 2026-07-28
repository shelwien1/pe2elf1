/* ktest.cpp -- differential test for the NLMS kernels (v4 §4.4).
 *
 * Every vector kernel in xad_simd.inc claims to be BIT-EXACT against the scalar
 * one, and the compressed-output md5 only proves that over the (w, h, err, sh)
 * tuples that real audio happens to produce.  This hits the ones it does not:
 * full i32 range including the products that overflow i64's low half, err = 0
 * and INT32_MIN, and above all the two ends of the xor-sub arithmetic-shift
 * identity -- sh = 0 and sh = 63 -- which the codec's own clamp
 * (PRED_SHMAX = 47) means it can never reach.
 *
 * Not part of the xadpcm build: build and run it with ./ktest.sh.
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
typedef uint8_t u8; typedef uint32_t u32; typedef uint64_t u64;
typedef int32_t i32; typedef int64_t i64;
#ifdef __GNUC__
 #define INLINE __attribute__((always_inline)) inline
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE __forceinline
 #define ALIGN(n) __declspec(align(n))
#endif
#include "xad_simd.inc"

enum { NMAX = 520 };
static ALIGN(64) i32 W0[NMAX], W1[NMAX], W2[NMAX], H[NMAX+8];

static u64 rs = 0x243F6A8885A308D3ull;
static u32 rnd() { rs ^= rs<<13; rs ^= rs>>7; rs ^= rs<<17; return u32(rs>>17); }

// a spread that hits both tiny values and full-range ones, and the extremes
static i32 rnd32() {
  switch( rnd()&7 ) {
  case 0: return 0;
  case 1: return i32(INT32_MIN);
  case 2: return i32(INT32_MAX);
  case 3: return i32(rnd()&0xFF)-128;
  case 4: return i32(rnd()&0xFFFF)-32768;
  default: return i32(rnd());
  }
}

int main() {
  xad_simd_init();
  long fail = 0, ndot = 0, nad = 0;
#if XAD_X86
  const int have_avx2 = __builtin_cpu_supports("avx2");
  const int have_sse4 = __builtin_cpu_supports("sse4.1");
#else
  const int have_avx2 = 0, have_sse4 = 0;
  fprintf(stderr, "ktest: no x86 kernels in this build; scalar only\n");
#endif

  for( long it = 0; it<200000; it++ ) {
    int n8 = 8*(1+int(rnd()%64));                       // dot always runs a whole vector count
    for( int i = 0; i<n8; i++ ) { W0[i] = rnd32(); H[i] = rnd32(); }
    i64 ref = xad_dot64_c(W0, H, n8);
    ndot++;
#if XAD_X86
    if( have_avx2 && xad_dot64_avx2(W0, H, n8)!=ref ) { printf("dot avx2 mismatch n8=%d\n", n8); fail++; }
    if( have_sse4 && xad_dot64_sse4(W0, H, n8)!=ref ) { printf("dot sse4 mismatch n8=%d\n", n8); fail++; }
#endif
    // dot from an UNALIGNED history cursor, which is how Pred always calls it
    if( n8>8 ) {
      i64 r2 = xad_dot64_c(W0, H+1, n8-8);
#if XAD_X86
      if( have_avx2 && xad_dot64_avx2(W0, H+1, n8-8)!=r2 ) { printf("dot avx2 unaligned mismatch\n"); fail++; }
      if( have_sse4 && xad_dot64_sse4(W0, H+1, n8-8)!=r2 ) { printf("dot sse4 unaligned mismatch\n"); fail++; }
#endif
    }
  }

  for( long it = 0; it<200000; it++ ) {
    int n = 1+int(rnd()%512);                           // adapt runs to an exact n, tail included
    int sh = int(rnd()%64);                             // the full legal range of the identity
    if( (it&15)==0 ) sh = 0;                            // ...and both ends, often
    if( (it&15)==1 ) sh = 63;
    i32 err = rnd32();
    if( (it&31)==0 ) err = 0;
    for( int i = 0; i<n+8; i++ ) H[i] = rnd32();
    for( int i = 0; i<n; i++ ) { i32 v = rnd32(); W0[i] = W1[i] = W2[i] = v; }
    xad_adapt_c(W0, H, n, err, sh);
    nad++;
#if XAD_X86
    if( have_avx2 ) {
      xad_adapt_avx2(W1, H, n, err, sh);
      if( memcmp(W0, W1, size_t(n)*4) ) { printf("adapt avx2 mismatch n=%d sh=%d err=%d\n", n, sh, err); fail++; }
    }
    if( have_sse4 ) {
      xad_adapt_sse4(W2, H, n, err, sh);
      if( memcmp(W0, W2, size_t(n)*4) ) { printf("adapt sse4 mismatch n=%d sh=%d err=%d\n", n, sh, err); fail++; }
    }
#endif
  }

  printf("ktest: %s | %ld dot cases, %ld adapt cases, %ld failures\n",
         xad_isa(), ndot, nad, fail);
  return fail ? 1 : 0;
}
