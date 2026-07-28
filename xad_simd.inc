/* xad_simd.inc -- part of xadpcm.cpp; not standalone.
 *
 * The three model kernels: the NLMS dot product and weight update, and the
 * mixer's weight update.  Included before xad_pred.inc and xad_mix.inc, their
 * only callers.
 *
 * WHY THESE ARE BIT-EXACT.  xad_dot64 accumulates into i64 and two's-complement
 * addition is associative and commutative even under overflow, so splitting the
 * sum across lanes and folding at the end reproduces the scalar left-to-right
 * result exactly.  xad_adapt is elementwise, so there is nothing to reassociate.
 * Neither kernel rounds: every product is kept at full 64 bits.  Any 16-bit
 * packing, mullo_epi32, or 32-bit accumulation would change the output and is
 * deliberately absent.
 *
 * DISPATCH.  If the translation unit is already being compiled for AVX2 or
 * AVX-512F (ARCH=-march=haswell / native, which is what g.bat has always used),
 * the matching kernel is called directly and inlines into Pred.  Otherwise the
 * kernels carry target attributes and a one-time CPUID check picks between them
 * through a function pointer -- one portable binary, full speed on the hardware
 * that has it, at the cost of an indirect call per stage (8 per sample-channel,
 * against several hundred taps). */

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#define XAD_X86 1
#include <immintrin.h>
#else
#define XAD_X86 0
#endif

/* The scalar reference.  Also the portable fallback, and what the differential
   test in ktest.cpp checks the vector kernels against. */
static i64 xad_dot64_c(const i32* w, const i32* h, int n8) {
  i64 s = 0;
  for( int i = 0; i<n8; i++ )
    s += i64(w[i])*i64(h[i]);
  return s;
}

static void xad_adapt_c(i32* w, const i32* h, int n, i32 err, int sh) {
  for( int i = 0; i<n; i++ )
    w[i] += i32((i64(err)*i64(h[i]))>>sh);
}

/* The mixer's weight update: like xad_adapt but with the symmetric clamp, and
   with a trip count that is always MIX_STRIDE, so there is no tail. */
static void xad_mixupd_c(i32* w, const i32* st, int n, i32 el, int sh, int cl) {
  for( int i = 0; i<n; i++ ) {
    i32 nw = w[i]+i32((i64(el)*i64(st[i]))>>sh);
    w[i] = nw<-cl ? -cl : nw>cl ? cl : nw;
  }
}

#if XAD_X86

/* On the compile-time path the kernels are always_inline, which is v4 §1.3:
   Pred's stage lengths are IDXC constants in the Const build, so inlining puts a
   COMPILE-TIME trip count in each loop -- no loop control, full unroll, and the
   short stages (the B cascade's third is 17 taps) become straight-line code where
   reduction latency rather than throughput was the binding constraint.
   On the dispatched path they cannot be inlined -- their address is taken -- and
   there they carry the target attribute instead. */
#if defined(__AVX2__)
#define XAD_T_AVX2 INLINE
#else
#define XAD_T_AVX2 __attribute__((target("avx2")))
#endif
#if defined(__SSE4_1__)
#define XAD_T_SSE4 INLINE
#else
#define XAD_T_SSE4 __attribute__((target("sse4.1")))
#endif
#if defined(__AVX512F__)
#define XAD_T_AVX512 INLINE
#else
#define XAD_T_AVX512 __attribute__((target("avx512f")))
#endif

/* AVX2 lacks a 64-bit ARITHMETIC right shift (srli_epi64 is logical and
   corrupts negative products), so use the xor-sub identity, valid for the whole
   legal range s in [0,63]:
       sra(x,s) == ((x >>logical s) XOR b) - b,   b = 1 << (63-s)
   b is built as (i64)((u64)1<<(63-s)): at s=0 that bit is the sign bit, and
   forming it by shifting a SIGNED 1 would be undefined behaviour.
   Checked at both ends by ktest.cpp, which real audio never reaches: at s=0 the
   two 2^63 terms cancel modulo 2^64, at s=63 the result is 0 or -1. */
XAD_T_AVX2 static __m256i xad_sra64_avx2(__m256i v, __m128i cnt, __m256i b) {
  return _mm256_sub_epi64(_mm256_xor_si256(_mm256_srl_epi64(v, cnt), b), b);
}

XAD_T_AVX2 static i64 xad_dot64_avx2(const i32* __restrict w, const i32* __restrict h, int n8) {
  __m256i a0 = _mm256_setzero_si256(), a1 = _mm256_setzero_si256();
  for( int i = 0; i<n8; i += 8 ) {
    __m256i wv = _mm256_load_si256((const __m256i*)(w+i));   // 64B-aligned by Pred
    __m256i hv = _mm256_loadu_si256((const __m256i*)(h+i));  // sliding cursor: unaligned
    /* mul_epi32 reads the LOW 32 bits of each 64-bit lane as signed, so the
       even elements go straight in and the odd ones are brought down by a
       logical shift -- the zero-extension it introduces is never read. */
    a0 = _mm256_add_epi64(a0, _mm256_mul_epi32(wv, hv));
    a1 = _mm256_add_epi64(a1, _mm256_mul_epi32(_mm256_srli_epi64(wv, 32),
                                               _mm256_srli_epi64(hv, 32)));
  }
  __m256i a = _mm256_add_epi64(a0, a1);
  __m128i s = _mm_add_epi64(_mm256_castsi256_si128(a), _mm256_extracti128_si256(a, 1));
  return (i64)_mm_cvtsi128_si64(s)+(i64)_mm_extract_epi64(s, 1);
}

XAD_T_AVX2 static void xad_adapt_avx2(i32* __restrict w, const i32* __restrict h, int n, i32 err, int sh) {
  __m256i e = _mm256_set1_epi32(err);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  __m256i b = _mm256_set1_epi64x((i64)((u64)1<<(63-sh)));
  int i = 0, nf = n&~7;
  for( ; i<nf; i += 8 ) {
    __m256i hv = _mm256_loadu_si256((const __m256i*)(h+i));
    __m256i pe = xad_sra64_avx2(_mm256_mul_epi32(e, hv), cnt, b);
    __m256i po = xad_sra64_avx2(_mm256_mul_epi32(e, _mm256_srli_epi64(hv, 32)), cnt, b);
    /* Take the low 32 bits of each 64-bit result -- exactly the scalar i32()
       truncation -- and add with wrapping, exactly the scalar +=. */
    __m256i r = _mm256_blend_epi32(pe, _mm256_slli_epi64(po, 32), 0xAA);
    _mm256_store_si256((__m256i*)(w+i),
                       _mm256_add_epi32(_mm256_load_si256((const __m256i*)(w+i)), r));
  }
  /* Tail, at most 7 taps.  Not padded away: adapt WRITES, and letting it write
     past n would make w[n..n8) nonzero, which dot -- which runs to n8 and
     depends on those lanes being zero -- would then pick up. */
  for( ; i<n; i++ )
    w[i] += i32((i64(err)*i64(h[i]))>>sh);
}

XAD_T_SSE4 static i64 xad_dot64_sse4(const i32* __restrict w, const i32* __restrict h, int n8) {
  __m128i a0 = _mm_setzero_si128(), a1 = _mm_setzero_si128();
  for( int i = 0; i<n8; i += 4 ) {
    __m128i wv = _mm_load_si128((const __m128i*)(w+i));
    __m128i hv = _mm_loadu_si128((const __m128i*)(h+i));
    a0 = _mm_add_epi64(a0, _mm_mul_epi32(wv, hv));
    a1 = _mm_add_epi64(a1, _mm_mul_epi32(_mm_srli_epi64(wv, 32), _mm_srli_epi64(hv, 32)));
  }
  __m128i s = _mm_add_epi64(a0, a1);
  return (i64)_mm_cvtsi128_si64(s)+(i64)_mm_extract_epi64(s, 1);
}

XAD_T_SSE4 static void xad_adapt_sse4(i32* __restrict w, const i32* __restrict h, int n, i32 err, int sh) {
  __m128i e = _mm_set1_epi32(err);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  __m128i b = _mm_set1_epi64x((i64)((u64)1<<(63-sh)));
  int i = 0, nf = n&~3;
  for( ; i<nf; i += 4 ) {
    __m128i hv = _mm_loadu_si128((const __m128i*)(h+i));
    __m128i pe = _mm_sub_epi64(_mm_xor_si128(_mm_srl_epi64(_mm_mul_epi32(e, hv), cnt), b), b);
    __m128i po = _mm_sub_epi64(_mm_xor_si128(_mm_srl_epi64(_mm_mul_epi32(e, _mm_srli_epi64(hv, 32)), cnt), b), b);
    __m128i r = _mm_blend_epi16(pe, _mm_slli_epi64(po, 32), 0xCC);
    _mm_store_si128((__m128i*)(w+i), _mm_add_epi32(_mm_load_si128((const __m128i*)(w+i)), r));
  }
  for( ; i<n; i++ )
    w[i] += i32((i64(err)*i64(h[i]))>>sh);
}

XAD_T_AVX2 static void xad_mixupd_avx2(i32* __restrict w, const i32* __restrict st,
                                       int n, i32 el, int sh, int cl) {
  __m256i e = _mm256_set1_epi32(el);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  __m256i b = _mm256_set1_epi64x((i64)((u64)1<<(63-sh)));
  __m256i lo = _mm256_set1_epi32(-cl), hi = _mm256_set1_epi32(cl);
  for( int i = 0; i<n; i += 8 ) {
    __m256i sv = _mm256_load_si256((const __m256i*)(st+i));
    __m256i pe = xad_sra64_avx2(_mm256_mul_epi32(e, sv), cnt, b);
    __m256i po = xad_sra64_avx2(_mm256_mul_epi32(e, _mm256_srli_epi64(sv, 32)), cnt, b);
    __m256i r = _mm256_blend_epi32(pe, _mm256_slli_epi64(po, 32), 0xAA);
    __m256i nw = _mm256_add_epi32(_mm256_load_si256((const __m256i*)(w+i)), r);
    _mm256_store_si256((__m256i*)(w+i), _mm256_max_epi32(lo, _mm256_min_epi32(hi, nw)));
  }
}

XAD_T_SSE4 static void xad_mixupd_sse4(i32* __restrict w, const i32* __restrict st,
                                       int n, i32 el, int sh, int cl) {
  __m128i e = _mm_set1_epi32(el);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  __m128i b = _mm_set1_epi64x((i64)((u64)1<<(63-sh)));
  __m128i lo = _mm_set1_epi32(-cl), hi = _mm_set1_epi32(cl);
  for( int i = 0; i<n; i += 4 ) {
    __m128i sv = _mm_load_si128((const __m128i*)(st+i));
    __m128i pe = _mm_sub_epi64(_mm_xor_si128(_mm_srl_epi64(_mm_mul_epi32(e, sv), cnt), b), b);
    __m128i po = _mm_sub_epi64(_mm_xor_si128(_mm_srl_epi64(_mm_mul_epi32(e, _mm_srli_epi64(sv, 32)), cnt), b), b);
    __m128i r = _mm_blend_epi16(pe, _mm_slli_epi64(po, 32), 0xCC);
    __m128i nw = _mm_add_epi32(_mm_load_si128((const __m128i*)(w+i)), r);
    _mm_store_si128((__m128i*)(w+i), _mm_max_epi32(lo, _mm_min_epi32(hi, nw)));
  }
}

/* AVX-512F.  Worth its own set for one reason beyond width: _mm512_sra_epi64 is
   a real 64-bit ARITHMETIC shift, so adapt drops the xor-sub identity entirely.
   MEASURED 9-10% faster than the AVX2 set on the development machine (8.39 s vs
   9.29 s on the encode pair), bit-identical output -- so it is the default where
   the hardware has it.  512-bit code does downclock the core on several server
   parts, and that is machine-specific rather than something this file can know,
   so there is an opt-out: -DXAD_NO_AVX512 at compile time, or XAD_AVX512=0 in
   the environment for the runtime-dispatched build.
   The stage lengths are multiples of 8 but not of 16, so these carry a tail of
   at most 8 taps, handled scalar.  Reassociating the dot across a different
   lane count is still exact: i64 addition is associative under wraparound. */
XAD_T_AVX512 static i64 xad_dot64_avx512(const i32* __restrict w, const i32* __restrict h, int n8) {
  __m512i a0 = _mm512_setzero_si512(), a1 = _mm512_setzero_si512();
  int i = 0;
  for( ; i+16<=n8; i += 16 ) {
    __m512i wv = _mm512_load_si512((const void*)(w+i));
    __m512i hv = _mm512_loadu_si512((const void*)(h+i));
    a0 = _mm512_add_epi64(a0, _mm512_mul_epi32(wv, hv));
    a1 = _mm512_add_epi64(a1, _mm512_mul_epi32(_mm512_srli_epi64(wv, 32),
                                               _mm512_srli_epi64(hv, 32)));
  }
  i64 s = _mm512_reduce_add_epi64(_mm512_add_epi64(a0, a1));
  for( ; i<n8; i++ )
    s += i64(w[i])*i64(h[i]);
  return s;
}

XAD_T_AVX512 static void xad_adapt_avx512(i32* __restrict w, const i32* __restrict h, int n, i32 err, int sh) {
  __m512i e = _mm512_set1_epi32(err);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  int i = 0, nf = n&~15;
  for( ; i<nf; i += 16 ) {
    __m512i hv = _mm512_loadu_si512((const void*)(h+i));
    __m512i pe = _mm512_sra_epi64(_mm512_mul_epi32(e, hv), cnt);
    __m512i po = _mm512_sra_epi64(_mm512_mul_epi32(e, _mm512_srli_epi64(hv, 32)), cnt);
    __m512i r = _mm512_mask_blend_epi32(0xAAAA, pe, _mm512_slli_epi64(po, 32));
    _mm512_storeu_si512((void*)(w+i),
                        _mm512_add_epi32(_mm512_loadu_si512((const void*)(w+i)), r));
  }
  for( ; i<n; i++ )
    w[i] += i32((i64(err)*i64(h[i]))>>sh);
}

XAD_T_AVX512 static void xad_mixupd_avx512(i32* __restrict w, const i32* __restrict st,
                                           int n, i32 el, int sh, int cl) {
  __m512i e = _mm512_set1_epi32(el);
  __m128i cnt = _mm_cvtsi32_si128(sh);
  __m512i lo = _mm512_set1_epi32(-cl), hi = _mm512_set1_epi32(cl);
  for( int i = 0; i<n; i += 16 ) {
    __m512i sv = _mm512_load_si512((const void*)(st+i));
    __m512i pe = _mm512_sra_epi64(_mm512_mul_epi32(e, sv), cnt);
    __m512i po = _mm512_sra_epi64(_mm512_mul_epi32(e, _mm512_srli_epi64(sv, 32)), cnt);
    __m512i r = _mm512_mask_blend_epi32(0xAAAA, pe, _mm512_slli_epi64(po, 32));
    __m512i nw = _mm512_add_epi32(_mm512_load_si512((const void*)(w+i)), r);
    _mm512_store_si512((void*)(w+i), _mm512_max_epi32(lo, _mm512_min_epi32(hi, nw)));
  }
}

#endif // XAD_X86

// ---------------------------------------------------------------- selection

#if XAD_X86 && defined(__AVX512F__) && !defined(XAD_NO_AVX512)

static INLINE i64 xad_dot(const i32* w, const i32* h, int n8) {
  return xad_dot64_avx512(w, h, n8);
}
static INLINE void xad_adapt(i32* w, const i32* h, int n, i32 err, int sh) {
  xad_adapt_avx512(w, h, n, err, sh);
}
static INLINE void xad_mixupd(i32* w, const i32* st, int n, i32 el, int sh, int cl) {
  xad_mixupd_avx512(w, st, n, el, sh, cl);
}
static const char* xad_isa() { return "avx512f (compile-time)"; }
static void xad_simd_init() {}

#elif XAD_X86 && defined(__AVX2__)

// built for AVX2: direct and inlinable, no indirection at all
static INLINE i64 xad_dot(const i32* w, const i32* h, int n8) {
  return xad_dot64_avx2(w, h, n8);
}
static INLINE void xad_adapt(i32* w, const i32* h, int n, i32 err, int sh) {
  xad_adapt_avx2(w, h, n, err, sh);
}
static INLINE void xad_mixupd(i32* w, const i32* st, int n, i32 el, int sh, int cl) {
  xad_mixupd_avx2(w, st, n, el, sh, cl);
}
static const char* xad_isa() { return "avx2 (compile-time)"; }
static void xad_simd_init() {}

#elif XAD_X86

typedef i64 (*xad_dot_fn)(const i32*, const i32*, int);
typedef void (*xad_adapt_fn)(i32*, const i32*, int, i32, int);
typedef void (*xad_mixupd_fn)(i32*, const i32*, int, i32, int, int);
static xad_dot_fn xad_dot_p = xad_dot64_c;
static xad_adapt_fn xad_adapt_p = xad_adapt_c;
static xad_mixupd_fn xad_mixupd_p = xad_mixupd_c;
static const char* xad_isa_name = "scalar";

static void xad_simd_init() {
  __builtin_cpu_init();
  /* AVX-512 first where it exists -- measured 9-10% over AVX2 here.  XAD_AVX512=0
     in the environment forces AVX2, for parts where 512-bit downclocking makes
     that the wrong trade; the same A/B is how the default was chosen. */
  const char* no512 = getenv("XAD_AVX512");
  if( !(no512&&no512[0]=='0')&&__builtin_cpu_supports("avx512f") ) {
    xad_dot_p = xad_dot64_avx512;
    xad_adapt_p = xad_adapt_avx512;
    xad_mixupd_p = xad_mixupd_avx512;
    xad_isa_name = "avx512f (runtime)";
  } else if( __builtin_cpu_supports("avx2") ) {
    xad_dot_p = xad_dot64_avx2;
    xad_adapt_p = xad_adapt_avx2;
    xad_mixupd_p = xad_mixupd_avx2;
    xad_isa_name = "avx2 (runtime)";
  } else if( __builtin_cpu_supports("sse4.1") ) {
    xad_dot_p = xad_dot64_sse4;
    xad_adapt_p = xad_adapt_sse4;
    xad_mixupd_p = xad_mixupd_sse4;
    xad_isa_name = "sse4.1 (runtime)";
  }
}
static INLINE i64 xad_dot(const i32* w, const i32* h, int n8) {
  return xad_dot_p(w, h, n8);
}
static INLINE void xad_adapt(i32* w, const i32* h, int n, i32 err, int sh) {
  xad_adapt_p(w, h, n, err, sh);
}
static INLINE void xad_mixupd(i32* w, const i32* st, int n, i32 el, int sh, int cl) {
  xad_mixupd_p(w, st, n, el, sh, cl);
}
static const char* xad_isa() { return xad_isa_name; }

#else

static INLINE i64 xad_dot(const i32* w, const i32* h, int n8) {
  return xad_dot64_c(w, h, n8);
}
static INLINE void xad_adapt(i32* w, const i32* h, int n, i32 err, int sh) {
  xad_adapt_c(w, h, n, err, sh);
}
static INLINE void xad_mixupd(i32* w, const i32* st, int n, i32 el, int sh, int cl) {
  xad_mixupd_c(w, st, n, el, sh, cl);
}
static const char* xad_isa() { return "scalar (portable)"; }
static void xad_simd_init() {}

#endif
