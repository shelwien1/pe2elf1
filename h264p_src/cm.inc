// The context mixing parts of oggcomp (balrogg-P-hash cm.inc), unchanged but
// for this comment: adaptive counters (cm_cnt), interpolated APMs (cm_apm)
// and logistic mixers (cm_mix), on 12-bit probabilities.

namespace h264p {

constexpr int CM_PBITS = 12;
constexpr int CM_PONE = 1 << CM_PBITS;
constexpr int CM_PHALF = CM_PONE / 2;
constexpr int CM_STMAX = 2047;
constexpr int CM_CBITS = 16;
constexpr int CM_CONE = 1 << CM_CBITS;
constexpr int CM_TMAX = 4096;
static u16 cm_rec[CM_TMAX + 2];
constexpr int CM_AMAX = 255;
static u16 cm_arec[CM_AMAX + 2];
static u16 cm_mrec[CM_AMAX + 2];
static i16 cm_st[CM_PONE];
static u16 cm_sq[2 * CM_STMAX + 1];
constexpr int CM_QMAX = 130;
static void cm_tables(void) {
  int i;
  for(i = 0; i <= CM_TMAX + 1; i++)
    cm_rec[i] = (u16)(CM_CONE / (i + 2));
  for(i = 0; i <= CM_AMAX + 1; i++)
    cm_arec[i] = (u16)(65536 / (i + 2));
  for(i = 0; i <= CM_AMAX + 1; i++)
    cm_mrec[i] = (u16)(65535 / (i + 1));
  for(i = -CM_STMAX; i <= CM_STMAX; i++) {
    double v = CM_PONE / (1.0 + exp(-i / 256.0));
    int p = (int)(v + 0.5);
    if(p < 1)
      p = 1;
    if(p > CM_PONE - 1)
      p = CM_PONE - 1;
    cm_sq[i + CM_STMAX] = (u16)p;
  }
  {
    int j = 0;
    for(i = 0; i < CM_PONE; i++) {
      while(j < 2 * CM_STMAX && cm_sq[j] < i)
        j++;
      cm_st[i] = (i16)(j - CM_STMAX);
    }
  }
}
static INLINE int cm_squash(int x) {
  x = x < -CM_STMAX ? -CM_STMAX : x > CM_STMAX ? CM_STMAX : x;
  return cm_sq[x + CM_STMAX];
}
static INLINE int cm_stretch(int p) { return cm_st[p]; }
struct cm_cnt {
  u16 p, t;
  INLINE int P() const {
    int v = t ? p >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE void upd(int bit, int lim, int mw) {
    int target = bit ? mw : CM_CONE - 1 - mw;
    int cur = t ? (int)p : CM_CONE / 2;
    t = (u16)(t + (t < lim));
    p = (u16)(cur + (((target - cur) * cm_rec[t]) >> CM_CBITS));
  }
};
// a counter of a family's main tables: the adaptive-rate probability of
// cm_cnt and one more with a fixed fast rate (1/8)
struct cm_cnt2 {
  u16 p, t, q;
  INLINE int P() const {
    int v = t ? p >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE int Q() const {
    int v = t ? q >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE void upd(int bit, int lim, int mw) {
    int target = bit ? mw : CM_CONE - 1 - mw;
    int cur = t ? (int)p : CM_CONE / 2, cq = t ? (int)q : CM_CONE / 2;
    t = (u16)(t + (t < lim));
    p = (u16)(cur + (((target - cur) * cm_rec[t]) >> CM_CBITS));
    q = (u16)(cq + ((target - cq) >> 3));
  }
};
struct cm_apm {
  i16 *__restrict t;
  u8 *__restrict c;
  sz n;
  int idx, bkt, q;
  u16 ident[CM_QMAX];
  void init(i16 *tab, u8 *cnt, sz nctx, int qq) {
    int j;
    t = tab;
    c = cnt;
    n = nctx;
    idx = 0;
    bkt = 0;
    q = qq < 3 ? 3 : qq > CM_QMAX ? CM_QMAX : qq;
    for(j = 0; j < q; j++) {
      int x = j * 4096 / (q - 1) - CM_STMAX - 1;
      ident[j] = (u16)cm_squash(x);
    }
  }
  INLINE int pp(int pr, u32 ctx) {
    int s = cm_stretch(pr) + CM_STMAX;
    int pos = s * (q - 1);
    int w = (pos & 4095) >> 5;
    bkt = pos >> 12;
    idx = (int)(ctx * (u32)q) + bkt;
    return (((int)ident[bkt] + t[idx]) * (128 - w) + ((int)ident[bkt + 1] + t[idx + 1]) * w) >> 7;
  }
  INLINE void upd1(int i, int b, int g, int lim) {
    u8 *__restrict cc = c;
    i16 *__restrict tt = t;
    int k = cc[i];
    int v = (int)ident[b] + tt[i];
    cc[i] = (u8)(k + (k < lim));
    v += ((g - v) * cm_arec[k]) >> 16;
    tt[i] = (i16)(v - (int)ident[b]);
  }
  static int limit(int rate) {
    int lim = rate >= 8 ? CM_AMAX : (1 << rate) - 2;
    return lim < 1 ? 1 : lim;
  }
  INLINE void upd(int bit, int lim) {
    int g = bit ? 0 : CM_PONE;
    upd1(idx, bkt, g, lim);
    upd1(idx + 1, bkt + 1, g, lim);
  }
};

#if defined(__clang__)
#define CM_UNROLL _Pragma("unroll")
#elif defined(__GNUC__)
#define CM_UNROLL _Pragma("GCC unroll 8")
#else
#define CM_UNROLL
#endif

template <int N> struct cm_mix {
  i32 *__restrict w;
  u8 *__restrict c;
  int x[N], pr, row, ctxn;
  enum { W0 = (1 << 16) / N };
  void init(i32 *wt, u8 *cnt, sz nctx) {
    (void)nctx;
    w = wt;
    c = cnt;
    row = 0;
    ctxn = 0;
    pr = CM_PHALF;
  }
  INLINE void set(int k, int st) { x[k] = st; }
#ifdef H264P_SSE2
  // the inputs (stretched probabilities, within 16 bits) 8 at a time
  INLINE __m128i x16(int i) const {
    return _mm_packs_epi32(_mm_loadu_si128((const __m128i *)(x + i)), _mm_loadu_si128((const __m128i *)(x + i + 4)));
  }
  static INLINE int hsum(__m128i v) {
    v = _mm_add_epi32(v, _mm_shuffle_epi32(v, 0x4E));
    return _mm_cvtsi128_si32(_mm_add_epi32(v, _mm_shuffle_epi32(v, 0xB1)));
  }
#endif
  INLINE int mix(u32 ctx) {
    i64 s = 0;
    int i;
    ctxn = (int)ctx;
    row = (int)ctx * N;
#ifdef H264P_SSE2
    if(N % 8 == 0) {
      // exactly the sum below: each weight (with W0, under 2^29) as hi 2^15
      // + lo, lo in 0..32767, so that both sums of products stay in 32 bits
      const __m128i w0 = _mm_set1_epi32(W0), m15 = _mm_set1_epi32(0x7FFF);
      __m128i sh = _mm_setzero_si128(), sl = sh;
      for(i = 0; i < N; i += 8) {
        __m128i xx = x16(i);
        __m128i va = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(w + row + i)), w0);
        __m128i vb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(w + row + i + 4)), w0);
        sh = _mm_add_epi32(sh, _mm_madd_epi16(_mm_packs_epi32(_mm_srai_epi32(va, 15), _mm_srai_epi32(vb, 15)), xx));
        sl = _mm_add_epi32(sl, _mm_madd_epi16(_mm_packs_epi32(_mm_and_si128(va, m15), _mm_and_si128(vb, m15)), xx));
      }
      s = (i64)hsum(sh) * 32768 + hsum(sl);
    } else
#endif
    {
      CM_UNROLL
      for(i = 0; i < N; i++)
        s += (i64)(w[row + i] + W0) * x[i];
    }
    pr = cm_squash((int)(s >> 16));
    pr = pr < 1 ? 1 : pr > CM_PONE - 1 ? CM_PONE - 1 : pr;
    return pr;
  }
  INLINE void upd(int bit, int lr, int mb) {
    int err, i, n;
    i32 *__restrict ww = w + row;
    if(mb) {
      u8 *__restrict cc = c;
      n = cc[ctxn];
      cc[ctxn] = (u8)(n + (n < CM_AMAX));
      lr += (int)(((i64)mb * lr * cm_mrec[n]) >> 16);
    }
    err = ((bit ? 0 : CM_PONE) - pr) * lr;
    // (clamped to +-4096, far beyond what data needs: a stream made to drive
    // a weight on and on must not overflow it)
#ifdef H264P_SSE2
    if(N % 8 == 0) {
      // exactly the loop below: with err = eh 65536 + el, el in 0..65535 (and
      // |err| < 2^26: lr is at most 16383), (x err) >> 16 = x eh + ((x el) >> 16),
      // the latter the signed product's high half: el read as signed is el -
      // 65536 from 32768 on, which takes x 65536 off the product
      const int eh = err >> 16, el = err & 0xFFFF;
      const __m128i veh = _mm_set1_epi16((short)eh), vel = _mm_set1_epi16((short)el);
      const __m128i corr = _mm_set1_epi16(el >= 32768 ? -1 : 0);
      const __m128i wmax = _mm_set1_epi32(1 << 28), wmin = _mm_set1_epi32(-(1 << 28));
      for(i = 0; i < N; i += 8) {
        __m128i xx = x16(i);
        __m128i lh = _mm_add_epi16(_mm_mulhi_epi16(xx, vel), _mm_and_si128(xx, corr));
        __m128i pl = _mm_mullo_epi16(xx, veh), ph = _mm_mulhi_epi16(xx, veh);
        __m128i da = _mm_add_epi32(_mm_unpacklo_epi16(pl, ph), _mm_srai_epi32(_mm_unpacklo_epi16(lh, lh), 16));
        __m128i db = _mm_add_epi32(_mm_unpackhi_epi16(pl, ph), _mm_srai_epi32(_mm_unpackhi_epi16(lh, lh), 16));
        __m128i wa = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + i)), da);
        __m128i wb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + i + 4)), db);
        __m128i ga = _mm_cmpgt_epi32(wa, wmax), gb = _mm_cmpgt_epi32(wb, wmax);
        wa = _mm_or_si128(_mm_and_si128(ga, wmax), _mm_andnot_si128(ga, wa));
        wb = _mm_or_si128(_mm_and_si128(gb, wmax), _mm_andnot_si128(gb, wb));
        ga = _mm_cmplt_epi32(wa, wmin), gb = _mm_cmplt_epi32(wb, wmin);
        wa = _mm_or_si128(_mm_and_si128(ga, wmin), _mm_andnot_si128(ga, wa));
        wb = _mm_or_si128(_mm_and_si128(gb, wmin), _mm_andnot_si128(gb, wb));
        _mm_storeu_si128((__m128i *)(ww + i), wa);
        _mm_storeu_si128((__m128i *)(ww + i + 4), wb);
      }
      return;
    }
#endif
    CM_UNROLL
    for(i = 0; i < N; i++) {
      i64 v = (i64)ww[i] + (((i64)x[i] * err) >> 16);
      ww[i] = (i32)(v < -(1 << 28) ? -(1 << 28) : v > (1 << 28) ? (1 << 28) : v);
    }
  }
};

}  // namespace h264p
