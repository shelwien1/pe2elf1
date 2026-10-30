// cm_mix.inc -- cm.inc's cm_mix: a logistic mixer of N inputs.

template <int N> struct cm_mix {
  static_assert(N <= CM_MIX_MAX, "cm_mix: its AVX2 sum is exact for 32 inputs at most");
  i32 *__restrict w;
  u8 *__restrict c;
  int x[N], pr, row, ctxn;
  enum { W0 = (1 << CM_WBITS) / N };
  static constexpr int LO = 15;  // (the AVX2 sum: a weight as hi 2^LO + lo)
  void init(i32 *wt, u8 *cnt) {
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
#ifdef M4V_AVX2
    if(xp) {
      // exactly the sum below: each weight (with W0, under 2^29) as hi 2^15
      // + lo, lo in 0..32767, so that both sums of products stay in 32 bits
      const i32 *ww = w + row;
      const __m256i w0 = _mm256_set1_epi32(W0), mlo = _mm256_set1_epi32((1 << LO) - 1);
      __m256i sh = _mm256_setzero_si256(), sl = sh;
      for(i = 0; i + CM_V16 <= N; i += CM_V16) {
        __m256i va = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i)), w0);
        __m256i vb = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i + CM_V32)), w0);
        sh = _mm256_add_epi32(sh, _mm256_madd_epi16(_mm256_packs_epi32(_mm256_srai_epi32(va, LO), _mm256_srai_epi32(vb, LO)), xp->v[i / CM_V16]));
        sl = _mm256_add_epi32(sl, _mm256_madd_epi16(_mm256_packs_epi32(_mm256_and_si256(va, mlo), _mm256_and_si256(vb, mlo)), xp->v[i / CM_V16]));
      }
      __m128i h = _mm_add_epi32(_mm256_castsi256_si128(sh), _mm256_extracti128_si256(sh, 1));
      __m128i l = _mm_add_epi32(_mm256_castsi256_si128(sl), _mm256_extracti128_si256(sl, 1));
      if(N % CM_V16) {
        const __m128i w0h = _mm_set1_epi32(W0), mloh = _mm_set1_epi32((1 << LO) - 1);
        __m128i va = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - CM_V32)), w0h);
        __m128i vb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - CM_H32)), w0h);
        h = _mm_add_epi32(h, _mm_madd_epi16(_mm_packs_epi32(_mm_srai_epi32(va, LO), _mm_srai_epi32(vb, LO)), xp->t));
        l = _mm_add_epi32(l, _mm_madd_epi16(_mm_packs_epi32(_mm_and_si128(va, mloh), _mm_and_si128(vb, mloh)), xp->t));
      }
      h = _mm_add_epi32(h, _mm_shuffle_epi32(h, CM_SWAP64));
      l = _mm_add_epi32(l, _mm_shuffle_epi32(l, CM_SWAP64));
      s = (i64)_mm_cvtsi128_si32(_mm_add_epi32(h, _mm_shuffle_epi32(h, CM_SWAP32))) * (1 << LO) +
          _mm_cvtsi128_si32(_mm_add_epi32(l, _mm_shuffle_epi32(l, CM_SWAP32)));
    } else
#endif
    {
      (void)xp;
      CM_UNROLL
      for(i = 0; i < N; i++)
        s += (i64)(w[row + i] + W0) * xi[i];
    }
    pr = cm_squash((int)(s >> CM_WBITS));
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
      lr += (int)(((i64)mb * lr * cm_mrec[n]) >> CM_CBITS);
    }
    err = ((bit ? 0 : CM_PONE) - pr) * lr;
    // (clamped to +-4096, far beyond what data needs: a stream made to drive
    // a weight on and on must not overflow it)
#ifdef M4V_AVX2
    if(xp) {
      // exactly the loop below: with err = eh 65536 + el, el in 0..65535 (and
      // |err| < 2^26: lr is at most 16383), (x err) >> 16 = x eh + ((x el) >> 16),
      // the latter the signed product's high half: el read as signed is el -
      // 65536 from 32768 on, which takes x 65536 off the product
      constexpr int H = CM_WBITS;  // (err's halves' bits; the products' high halves: of 16-bit lanes)
      const int eh = err >> H, el = err & ((1 << H) - 1);
      const __m256i veh = _mm256_set1_epi16((short)eh), vel = _mm256_set1_epi16((short)el);
      const __m256i corr = _mm256_set1_epi16(el >= 1 << (H - 1) ? -1 : 0);
      const __m256i wmax = _mm256_set1_epi32(CM_WMAX), wmin = _mm256_set1_epi32(-CM_WMAX);
      for(i = 0; i + CM_V16 <= N; i += CM_V16) {
        // (unpacking the packed lanes puts them back in order)
        __m256i xx = xp->v[i / CM_V16];
        __m256i lh = _mm256_add_epi16(_mm256_mulhi_epi16(xx, vel), _mm256_and_si256(xx, corr));
        __m256i pl = _mm256_mullo_epi16(xx, veh), ph = _mm256_mulhi_epi16(xx, veh);
        __m256i da = _mm256_add_epi32(_mm256_unpacklo_epi16(pl, ph), _mm256_srai_epi32(_mm256_unpacklo_epi16(lh, lh), H));
        __m256i db = _mm256_add_epi32(_mm256_unpackhi_epi16(pl, ph), _mm256_srai_epi32(_mm256_unpackhi_epi16(lh, lh), H));
        __m256i wa = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i)), da);
        __m256i wb = _mm256_add_epi32(_mm256_loadu_si256((const __m256i *)(ww + i + CM_V32)), db);
        wa = _mm256_max_epi32(_mm256_min_epi32(wa, wmax), wmin);
        wb = _mm256_max_epi32(_mm256_min_epi32(wb, wmax), wmin);
        _mm256_storeu_si256((__m256i *)(ww + i), wa);
        _mm256_storeu_si256((__m256i *)(ww + i + CM_V32), wb);
      }
      if(N % CM_V16) {
        const __m128i veh1 = _mm256_castsi256_si128(veh), vel1 = _mm256_castsi256_si128(vel), corr1 = _mm256_castsi256_si128(corr);
        __m128i xx = xp->t;
        __m128i lh = _mm_add_epi16(_mm_mulhi_epi16(xx, vel1), _mm_and_si128(xx, corr1));
        __m128i pl = _mm_mullo_epi16(xx, veh1), ph = _mm_mulhi_epi16(xx, veh1);
        __m128i da = _mm_add_epi32(_mm_unpacklo_epi16(pl, ph), _mm_srai_epi32(_mm_unpacklo_epi16(lh, lh), H));
        __m128i db = _mm_add_epi32(_mm_unpackhi_epi16(pl, ph), _mm_srai_epi32(_mm_unpackhi_epi16(lh, lh), H));
        __m128i wa = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - CM_V32)), da);
        __m128i wb = _mm_add_epi32(_mm_loadu_si128((const __m128i *)(ww + N - CM_H32)), db);
        wa = _mm_max_epi32(_mm_min_epi32(wa, _mm256_castsi256_si128(wmax)), _mm256_castsi256_si128(wmin));
        wb = _mm_max_epi32(_mm_min_epi32(wb, _mm256_castsi256_si128(wmax)), _mm256_castsi256_si128(wmin));
        _mm_storeu_si128((__m128i *)(ww + N - CM_V32), wa);
        _mm_storeu_si128((__m128i *)(ww + N - CM_H32), wb);
      }
      return;
    }
#endif
    (void)xp;
    CM_UNROLL
    for(i = 0; i < N; i++) {
      i64 v = (i64)ww[i] + (((i64)xi[i] * err) >> CM_WBITS);
      ww[i] = (i32)(v < -CM_WMAX ? -CM_WMAX : v > CM_WMAX ? CM_WMAX : v);
    }

  }
};
