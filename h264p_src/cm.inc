// The context mixing parts of oggcomp (balrogg-P-hash cm.inc): adaptive
// counters (cm_cnt), interpolated APMs (cm_apm) and logistic mixers
// (cm_mix), on 12-bit probabilities. Changed: the counters with a second,
// fast probability (cm_cnt2; cm_cnt2s keeps both stretched too), and the
// mixers' sums in AVX2 where the inputs are packed once for the mixers
// sharing them (cm_x16), to the same results (a family's ten counters of a
// decision too: tc_fam::bit_x).

namespace h264p {

constexpr int CM_PBITS = 12;
constexpr int CM_PONE = 1 << CM_PBITS;
constexpr int CM_PHALF = CM_PONE / 2;
constexpr int CM_STMAX = 2047;
constexpr int CM_CBITS = 16;
constexpr int CM_CONE = 1 << CM_CBITS;
constexpr int CM_TMAX = 4096;
constexpr int CM_AMAX = 255;
constexpr int CM_QMAX = 130;
//  The tables the counters, APMs and mixers read: the reciprocals of their
//  counts and squash and stretch -- constants, computed as the program
//  starts and only read after, by every model at once
struct cm_tabs {
  u16 rec[CM_TMAX + 2];
  u16 arec[CM_AMAX + 2];
  u16 mrec[CM_AMAX + 2];
  i16 st[CM_PONE + 1];  // (and one more, read in pairs)
  u16 sq[2 * CM_STMAX + 1];
  cm_tabs() {
    int i;
    for(i = 0; i <= CM_TMAX + 1; i++)
      rec[i] = (u16)(CM_CONE / (i + 2));
    for(i = 0; i <= CM_AMAX + 1; i++)
      arec[i] = (u16)(65536 / (i + 2));
    for(i = 0; i <= CM_AMAX + 1; i++)
      mrec[i] = (u16)(65535 / (i + 1));
    for(i = -CM_STMAX; i <= CM_STMAX; i++) {
      double v = CM_PONE / (1.0 + exp(-i / 256.0));
      int p = (int)(v + 0.5);
      if(p < 1)
        p = 1;
      if(p > CM_PONE - 1)
        p = CM_PONE - 1;
      sq[i + CM_STMAX] = (u16)p;
    }
    int j = 0;
    for(i = 0; i < CM_PONE; i++) {
      while(j < 2 * CM_STMAX && sq[j] < i)
        j++;
      st[i] = (i16)(j - CM_STMAX);
    }
    st[CM_PONE] = 0;
  }
};
static const cm_tabs cm_tab;
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
// cm_cnt2 with its two probabilities kept stretched, written at each update:
// a decision reads them with the counter instead of looking them up on its
// way to the mixer (-1's counters, four a family: h264p_l1_speed.md 6.4)
struct cm_cnt2s {
  cm_cnt2 c;
  i16 sp, sq;  // cm_stretch(P()), cm_stretch(Q())
  INLINE int P() const { return c.P(); }
  INLINE int Q() const { return c.Q(); }
  INLINE void upd(int bit, int lim, int mw) {
    c.upd(bit, lim, mw);
    sp = cm_st[c.P()];
    sq = cm_st[c.Q()];
  }
};
// an APM's count limit by its rate (1-15)
constexpr int cm_apm_limit(int rate) { return rate >= 8 ? CM_AMAX : (1 << rate) - 2 < 1 ? 1 : (1 << rate) - 2; }
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
  static int limit(int rate) { return cm_apm_limit(rate); }
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

// The inputs of a mixer of N (a multiple of 8) as 16-bit lanes, packed once
// for the mixers that share them: 16 at a time in the order
// _mm256_packs_epi32 leaves them (the weights are packed alike), the last 8
// in a half
template <int N> struct cm_x16 {
#ifdef H264P_AVX2
  __m256i v[N / 16 ? N / 16 : 1];
  __m128i t;
  INLINE void pack(const int *x) {
    for(int i = 0; i + 16 <= N; i += 16)
      v[i / 16] = _mm256_packs_epi32(_mm256_loadu_si256((const __m256i *)(x + i)), _mm256_loadu_si256((const __m256i *)(x + i + 8)));
    if(N % 16)
      t = _mm_packs_epi32(_mm_loadu_si128((const __m128i *)(x + N - 8)), _mm_loadu_si128((const __m128i *)(x + N - 4)));
  }
#else
  INLINE void pack(const int *) {}
#endif
};

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
  INLINE int mix(u32 ctx) { return mix(ctx, x, nullptr); }
  // with the inputs xi (packed in xp, N a multiple of 8, or null)
  INLINE int mix(u32 ctx, const int *xi, const cm_x16<N> *xp) {
    i64 s = 0;
    int i;
    ctxn = (int)ctx;
    row = (int)ctx * N;
#ifdef H264P_AVX2
    if(xp) {
      // exactly the sum below: each weight (with W0, under 2^29) as hi 2^15
      // + lo, lo in 0..32767, so that both sums of products stay in 32 bits
      const i32 *ww = w + row;
      const __m256i w0 = _mm256_set1_epi32(W0), m15 = _mm256_set1_epi32(0x7FFF);
      __m256i sh = _mm256_setzero_si256(), sl = sh;
      for(i = 0; i + 16 <= N; i += 16) {
        __m256i va = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i)), w0);
        __m256i vb = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i + 8)), w0);
        sh = _mm256_add_epi32(sh, _mm256_madd_epi16(_mm256_packs_epi32(_mm256_srai_epi32(va, 15), _mm256_srai_epi32(vb, 15)), xp->v[i / 16]));
        sl = _mm256_add_epi32(sl, _mm256_madd_epi16(_mm256_packs_epi32(_mm256_and_si256(va, m15), _mm256_and_si256(vb, m15)), xp->v[i / 16]));
      }
      __m128i h = _mm_add_epi32(_mm256_castsi256_si128(sh), _mm256_extracti128_si256(sh, 1));
      __m128i l = _mm_add_epi32(_mm256_castsi256_si128(sl), _mm256_extracti128_si256(sl, 1));
      if(N % 16) {
        const __m128i w0h = _mm_set1_epi32(W0), m15h = _mm_set1_epi32(0x7FFF);
        __m128i va = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - 8)), w0h);
        __m128i vb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - 4)), w0h);
        h = _mm_add_epi32(h, _mm_madd_epi16(_mm_packs_epi32(_mm_srai_epi32(va, 15), _mm_srai_epi32(vb, 15)), xp->t));
        l = _mm_add_epi32(l, _mm_madd_epi16(_mm_packs_epi32(_mm_and_si128(va, m15h), _mm_and_si128(vb, m15h)), xp->t));
      }
      h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0x4E));
      l = _mm_add_epi32(l, _mm_shuffle_epi32(l, 0x4E));
      s = (i64)_mm_cvtsi128_si32(_mm_add_epi32(h, _mm_shuffle_epi32(h, 0xB1))) * 32768 +
          _mm_cvtsi128_si32(_mm_add_epi32(l, _mm_shuffle_epi32(l, 0xB1)));
    } else
#endif
    {
      (void)xp;
      CM_UNROLL
      for(i = 0; i < N; i++)
        s += (i64)(w[row + i] + W0) * xi[i];
    }
    pr = cm_squash((int)(s >> 16));
    pr = pr < 1 ? 1 : pr > CM_PONE - 1 ? CM_PONE - 1 : pr;
    return pr;
  }
  INLINE void upd(int bit, int lr, int mb) { upd(bit, lr, mb, x, nullptr); }
  INLINE void upd(int bit, int lr, int mb, const int *xi, const cm_x16<N> *xp) {
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
#ifdef H264P_AVX2
    if(xp) {
      // exactly the loop below: with err = eh 65536 + el, el in 0..65535 (and
      // |err| < 2^26: lr is at most 16383), (x err) >> 16 = x eh + ((x el) >> 16),
      // the latter the signed product's high half: el read as signed is el -
      // 65536 from 32768 on, which takes x 65536 off the product
      const int eh = err >> 16, el = err & 0xFFFF;
      const __m256i veh = _mm256_set1_epi16((short)eh), vel = _mm256_set1_epi16((short)el);
      const __m256i corr = _mm256_set1_epi16(el >= 32768 ? -1 : 0);
      const __m256i wmax = _mm256_set1_epi32(1 << 28), wmin = _mm256_set1_epi32(-(1 << 28));
      for(i = 0; i + 16 <= N; i += 16) {
        // (unpacking the packed lanes puts them back in order)
        __m256i xx = xp->v[i / 16];
        __m256i lh = _mm256_add_epi16(_mm256_mulhi_epi16(xx, vel), _mm256_and_si256(xx, corr));
        __m256i pl = _mm256_mullo_epi16(xx, veh), ph = _mm256_mulhi_epi16(xx, veh);
        __m256i da = _mm256_add_epi32(_mm256_unpacklo_epi16(pl, ph), _mm256_srai_epi32(_mm256_unpacklo_epi16(lh, lh), 16));
        __m256i db = _mm256_add_epi32(_mm256_unpackhi_epi16(pl, ph), _mm256_srai_epi32(_mm256_unpackhi_epi16(lh, lh), 16));
        __m256i wa = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i)), da);
        __m256i wb = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i + 8)), db);
        wa = _mm256_max_epi32(_mm256_min_epi32(wa, wmax), wmin);
        wb = _mm256_max_epi32(_mm256_min_epi32(wb, wmax), wmin);
        _mm256_storeu_si256((__m256i *)(ww + i), wa);
        _mm256_storeu_si256((__m256i *)(ww + i + 8), wb);
      }
      if(N % 16) {
        const __m128i veh1 = _mm256_castsi256_si128(veh), vel1 = _mm256_castsi256_si128(vel), corr1 = _mm256_castsi256_si128(corr);
        __m128i xx = xp->t;
        __m128i lh = _mm_add_epi16(_mm_mulhi_epi16(xx, vel1), _mm_and_si128(xx, corr1));
        __m128i pl = _mm_mullo_epi16(xx, veh1), ph = _mm_mulhi_epi16(xx, veh1);
        __m128i da = _mm_add_epi32(_mm_unpacklo_epi16(pl, ph), _mm_srai_epi32(_mm_unpacklo_epi16(lh, lh), 16));
        __m128i db = _mm_add_epi32(_mm_unpackhi_epi16(pl, ph), _mm_srai_epi32(_mm_unpackhi_epi16(lh, lh), 16));
        __m128i wa = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - 8)), da);
        __m128i wb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - 4)), db);
        wa = _mm_max_epi32(_mm_min_epi32(wa, _mm256_castsi256_si128(wmax)), _mm256_castsi256_si128(wmin));
        wb = _mm_max_epi32(_mm_min_epi32(wb, _mm256_castsi256_si128(wmax)), _mm256_castsi256_si128(wmin));
        _mm_storeu_si128((__m128i *)(ww + N - 8), wa);
        _mm_storeu_si128((__m128i *)(ww + N - 4), wb);
      }
      return;
    }
#endif
    (void)xp;
    CM_UNROLL
    for(i = 0; i < N; i++) {
      i64 v = (i64)ww[i] + (((i64)xi[i] * err) >> 16);
      ww[i] = (i32)(v < -(1 << 28) ? -(1 << 28) : v > (1 << 28) ? (1 << 28) : v);
    }
  }
};

}  // namespace h264p
