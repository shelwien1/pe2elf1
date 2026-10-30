// cm.inc -- the context mixing parts the families code with: adaptive counters (cm_cnt; cm_cnt2
// with a second, fast probability; cm_cnt2s keeping both stretched), interpolated APMs (cm_apm)
// and logistic mixers (cm_mix), on 12-bit probabilities. h264p's cm.inc as it is (from oggcomp,
// balrogg-P-hash): the mixers' sums in AVX2 where the build targets it (-march=x86-64-v3), the
// inputs packed once for the mixers sharing them (cm_x16), to the same results as the plain C++.

constexpr int CM_PBITS = 12;
constexpr int CM_PONE = 1 << CM_PBITS;
constexpr int CM_PHALF = CM_PONE / 2;
constexpr int CM_STMAX = 2047;
constexpr int CM_CBITS = 16;
constexpr int CM_CONE = 1 << CM_CBITS;
constexpr int CM_TMAX = 4096;
constexpr int CM_AMAX = 255;
constexpr int CM_QMIN = 3, CM_QMAX = 130;
// the stretched domain (-CM_STMAX - 1 .. CM_STMAX), its width as bits; squash's slope there: the
// logistic of x / CM_SUNIT
constexpr int CM_SBITS = 12, CM_SUNIT = 256;
static_assert(2 * (CM_STMAX + 1) == 1 << CM_SBITS, "cm: the stretched domain");
// an APM: its interpolation weights' bits; the rates past which it counts to CM_AMAX
constexpr int CM_APM_WBITS = 7, CM_APM_RATE_MAX = 15, CM_APM_RATE_FULL = 8;
// cm_cnt2's fast probability: its fixed rate, 1 / 2^CM_FAST_SHIFT
constexpr int CM_FAST_SHIFT = 3;
// a mixer: its weights' fraction bits (a weight of 1 is 1 << CM_WBITS), their clamp; its inputs, at
// most (its AVX2 sum is exact for so many)
constexpr int CM_WBITS = 16, CM_WMAX = 1 << 28, CM_MIX_MAX = 32;
// an APM's width, its interpolation points: CM_QMIN to CM_QMAX, as the tables of
// it are sized (the templates' * cm_apm_q(qs)) and as cm_apm takes it
constexpr int cm_apm_q(int q) { return q < CM_QMIN ? CM_QMIN : q > CM_QMAX ? CM_QMAX : q; }
#include "cm_tabs.inc"  // cm_tabs: the tables they read
static const cm_tabs cm_tab = cm_make_tabs();
static const u16 (&cm_rec)[CM_TMAX + 2] = cm_tab.rec;
static const u16 (&cm_arec)[CM_AMAX + 2] = cm_tab.arec;
static const u16 (&cm_mrec)[CM_AMAX + 2] = cm_tab.mrec;
static const i16 (&cm_st)[CM_PONE + 1] = cm_tab.st;
static const u16 (&cm_sq)[2 * CM_STMAX + 1] = cm_tab.sq;
static INLINE int cm_squash(int x) {
  x = x < -CM_STMAX ? -CM_STMAX : x > CM_STMAX ? CM_STMAX : x;
  return cm_sq[x + CM_STMAX];
}
static INLINE int cm_stretch(int p) { return cm_st[p]; }
#include "cm_cnt.inc"  // cm_cnt: an adaptive counter
#include "cm_cnt2.inc"  // cm_cnt2: with a fast probability
#include "cm_cnt2s.inc"  // cm_cnt2s: kept stretched
#include "cm_apm.inc"  // cm_apm: an interpolated APM

#if defined(__clang__)
#define CM_UNROLL _Pragma("unroll")
#elif defined(__GNUC__)
#define CM_UNROLL _Pragma("GCC unroll 8")
#else
#define CM_UNROLL
#endif

#ifdef M4V_AVX2
// AVX2's lanes: 16-bit ones in a vector (CM_V16), 32-bit ones (CM_V32), those in a half (CM_H32);
// _mm_shuffle_epi32's swaps of 64-bit halves and of 32-bit neighbours
constexpr int CM_V16 = sizeof(__m256i) / sizeof(i16), CM_V32 = sizeof(__m256i) / sizeof(i32);
constexpr int CM_H32 = sizeof(__m128i) / sizeof(i32);
constexpr int CM_SWAP64 = _MM_SHUFFLE(1, 0, 3, 2), CM_SWAP32 = _MM_SHUFFLE(2, 3, 0, 1);
#endif

#include "cm_x16.inc"  // cm_x16: a mixer's inputs packed

#include "cm_mix.inc"  // cm_mix: a logistic mixer
