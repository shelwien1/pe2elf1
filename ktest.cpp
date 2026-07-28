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
#include "xad_msq.inc"

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

  /* v4 §5.5 -- the MS reciprocal path, brute-forced against the reference over
     the whole legal divisor domain.  d starts at 1, not 16: ms_delta_update
     floors at 16 but the per-block header path clamps only at 1, and d = 1 is
     exactly where the naive (1<<32)/d reciprocal would truncate to zero.

     diff is bounded by clip16(...) - P.  The plan puts |P| < 2^17, which holds
     for the standard coefficient table; a wav may carry its OWN table with
     |c| up to 32767, and then |P| <= 2*32767*32767/256 ~= 8.4e6.  So the sweep
     runs |diff| out to 2^24, well past either. */
  long nq = 0;
  {
    /* Exhaustive over d in [1,64] x every diff that can distinguish a quotient:
       |diff| out to 16*d+8 covers every cell and both sides of the +-8 clamp. */
    for( i32 d = 1; d<=64; d++ ) {
      u32 rcp = ms_rcp(d);
      for( i64 x = -(16*i64(d)+8); x<=16*i64(d)+8; x++ ) {
        i64 e1 = 0, e2 = 0;
        int r1 = ms_quantize_ref(x, d, e1);
        int r2 = ms_quantize_r(x, d, rcp, e2);
        int r3 = ms_quantize_nq(x, d, rcp);
        nq++;
        if( r1!=r2||e1!=e2||r1!=r3 ) {
          printf("ms_quantize mismatch d=%d diff=%lld\n", d, (long long)x);
          if( ++fail>20 ) return 1;
        }
      }
    }
    /* Beyond 64 the interesting diffs are only the ones next to a cell edge --
       that is the whole off-by-one risk in the reciprocal -- so sweep every
       power of two and its neighbours, a dense arithmetic sample, and the top of
       the domain, against diffs straddling k*d and k*d +- d/2. */
    for( i64 di = 65; di<=2796202; di++ ) {
      i32 d = i32(di);
      int lg = 63-__builtin_clzll(u64(d));
      i64 p2 = i64(1)<<lg;
      bool interesting = (d==p2)||(d==p2+1)||(d==p2-1)||(d%1361==0)||
                         (d>=2796200)||(d<=70);
      if( !interesting ) continue;
      u32 rcp = ms_rcp(d);
      for( i64 k = -10; k<=10; k++ )
        for( i64 o = -3; o<=3; o++ ) {
          i64 cand[3] = {k*i64(d)+o, k*i64(d)+d/2+o, k*i64(d)-d/2+o};
          for( int c = 0; c<3; c++ ) {
            i64 x = cand[c];
            if( x<-16777216||x>16777216 ) continue;
            i64 e1 = 0, e2 = 0;
            int r1 = ms_quantize_ref(x, d, e1);
            int r2 = ms_quantize_r(x, d, rcp, e2);
            int r3 = ms_quantize_nq(x, d, rcp);
            nq++;
            if( r1!=r2||e1!=e2||r1!=r3 ) {
              printf("ms_quantize mismatch d=%d diff=%lld\n", d, (long long)x);
              if( ++fail>20 ) return 1;
            }
          }
        }
      /* ...and the extreme magnitudes a non-standard coefficient table reaches */
      for( int sg = 0; sg<2; sg++ )
        for( i64 m = 8000000; m<=8400000; m += 97777 ) {
          i64 x = sg ? -m : m;
          i64 e1 = 0, e2 = 0;
          int r1 = ms_quantize_ref(x, d, e1);
          int r2 = ms_quantize_r(x, d, rcp, e2);
          nq++;
          if( r1!=r2||e1!=e2 ) {
            printf("ms_quantize mismatch d=%d diff=%lld\n", d, (long long)x);
            if( ++fail>20 ) return 1;
          }
        }
    }
  }

  printf("ktest: %s | %ld dot, %ld adapt, %ld ms_quantize cases, %ld failures\n",
         xad_isa(), ndot, nad, nq, fail);
  return fail ? 1 : 0;
}
