// pixel.inc -- the pixel kernels of the decoder in the loop and of the
// model's features from its samples (recon.inc): the SAD of a bilinear
// prediction at several widths, gradients, row copies, transposition, and
// the AVX2 helpers they share. Each has AVX2 code and plain C++ beside it,
// which compute the same.

namespace h264p {

static INLINE int clip1(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
static INLINE int clip3(int lo, int hi, int v) { return v < lo ? lo : v > hi ? hi : v; }
// the 16 vectors of a list's 4x4 blocks all alike
static INLINE bool one_mv(const int16_t mv[16][2]) {
#ifdef H264P_AVX2
  int32_t f;
  memcpy(&f, mv[0], 4);
  __m256i v = _mm256_set1_epi32(f);
  __m256i a = _mm256_cmpeq_epi32(_mm256_loadu_si256((const __m256i*)mv[0]), v);
  __m256i b = _mm256_cmpeq_epi32(_mm256_loadu_si256((const __m256i*)mv[8]), v);
  return _mm256_movemask_epi8(_mm256_and_si256(a, b)) == -1;
#else
  for (int b = 1; b < 16; b++)
    if (mv[b][0] != mv[0][0] || mv[b][1] != mv[0][1]) return false;
  return true;
#endif
}

#ifdef H264P_AVX2
// two rows of 16 samples (p0, p1) in one vector
static INLINE __m256i ld16x2(const uint8_t* p0, const uint8_t* p1) {
  return _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((const __m128i*)p0)), _mm_loadu_si128((const __m128i*)p1), 1);
}
// two rows of n (8 or 4) samples (p0, p1): of 8 in the two halves, of 4 in
// the low two quarters
static INLINE __m128i ld8x2(const uint8_t* p0, const uint8_t* p1, int n) {
  if (n == 8) return _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*)p0), _mm_loadl_epi64((const __m128i*)p1));
  int32_t u0, u1;
  memcpy(&u0, p0, 4);
  memcpy(&u1, p1, 4);
  return _mm_unpacklo_epi32(_mm_cvtsi32_si128(u0), _mm_cvtsi32_si128(u1));
}
// horizontal sums: of the two 64-bit lanes (_mm_sad_epu8's sums), of the
// four of _mm256_sad_epu8, and of the four or eight 32-bit lanes (as int)
static INLINE int sum64x2(__m128i v) { return _mm_cvtsi128_si32(_mm_add_epi64(v, _mm_unpackhi_epi64(v, v))); }
static INLINE int sad_sum(__m256i v) { return sum64x2(_mm_add_epi64(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1))); }
static INLINE int sum32x4(__m128i v) {
  v = _mm_add_epi32(v, _mm_shuffle_epi32(v, 0x4E));
  return _mm_cvtsi128_si32(_mm_add_epi32(v, _mm_shuffle_epi32(v, 0xB1)));
}
static INLINE int sum32x8(__m256i v) { return sum32x4(_mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1))); }
#endif

// The sum of the absolute differences between n x rows samples of q (stride
// qs) and their bilinear prediction from a (stride as) at (fx, fy) quarter
// samples: ((4-fx)(4-fy) a + fx(4-fy) a' + (4-fx)fy c + fx fy c' + 8) >> 4, a'
// and c' right of a and c, c below a. Stops once the sum reaches budget (>
// 0), after a row or two, and returns budget then: what it returns is the
// same however far it went (the AVX2 code and the plain alike; a caller
// sums such parts against one budget); a must be readable n + 1 samples
// wide and rows + 1 down.
static int sad_bil(const uint8_t* q, int qs, const uint8_t* a, int as, int n, int rows, int fx, int fy, int budget) {
  int s = 0;
#ifdef H264P_AVX2
  if ((n == 16 || n == 8 || n == 4) && !(rows & 1)) {
    // two rows at a time: the byte pairs (a, a') and (c, c') through
    // pmaddubsw, wa a + wb a' and wc c + wd c' in 16-bit lanes
    const int wa = (4 - fx) * (4 - fy), wb = fx * (4 - fy), wc = (4 - fx) * fy, wd = fx * fy;
    if (n == 16) {
      auto ld = [](const uint8_t* p0, const uint8_t* p1) { return ld16x2(p0, p1); };
      const __m256i wab = _mm256_set1_epi16((short)(wa | wb << 8)), wcd = _mm256_set1_epi16((short)(wc | wd << 8));
      const __m256i r8 = _mm256_set1_epi16(8);
      __m256i acc = _mm256_setzero_si256();
      for (int j = 0; j < rows; j += 2, q += 2 * qs, a += 2 * as) {
        __m256i d = ld(a, a + as);
        if (fx | fy) {
          __m256i x1 = fx ? ld(a + 1, a + as + 1) : d;
          __m256i lo = _mm256_maddubs_epi16(_mm256_unpacklo_epi8(d, x1), wab);
          __m256i hi = _mm256_maddubs_epi16(_mm256_unpackhi_epi8(d, x1), wab);
          if (fy) {
            __m256i y0 = ld(a + as, a + 2 * as), y1 = fx ? ld(a + as + 1, a + 2 * as + 1) : y0;
            lo = _mm256_add_epi16(lo, _mm256_maddubs_epi16(_mm256_unpacklo_epi8(y0, y1), wcd));
            hi = _mm256_add_epi16(hi, _mm256_maddubs_epi16(_mm256_unpackhi_epi8(y0, y1), wcd));
          }
          d = _mm256_packus_epi16(_mm256_srli_epi16(_mm256_add_epi16(lo, r8), 4),
                                  _mm256_srli_epi16(_mm256_add_epi16(hi, r8), 4));
        }
        acc = _mm256_add_epi64(acc, _mm256_sad_epu8(d, ld(q, q + qs)));
        s = sad_sum(acc);
        if (s >= budget) break;
      }
      return std::min(s, budget);
    }
    // rows of 8 in the two halves, of 4 in the low two quarters
    auto ld = [n](const uint8_t* p0, const uint8_t* p1) { return ld8x2(p0, p1, n); };
    const __m128i wab = _mm_set1_epi16((short)(wa | wb << 8)), wcd = _mm_set1_epi16((short)(wc | wd << 8));
    const __m128i r8 = _mm_set1_epi16(8), z = _mm_setzero_si128();
    __m128i acc = z;
    for (int j = 0; j < rows; j += 2, q += 2 * qs, a += 2 * as) {
      __m128i d = ld(a, a + as);
      if (fx | fy) {
        __m128i x1 = fx ? ld(a + 1, a + as + 1) : d;
        __m128i lo = _mm_maddubs_epi16(_mm_unpacklo_epi8(d, x1), wab);
        __m128i hi = n == 8 ? _mm_maddubs_epi16(_mm_unpackhi_epi8(d, x1), wab) : z;
        if (fy) {
          __m128i y0 = ld(a + as, a + 2 * as), y1 = fx ? ld(a + as + 1, a + 2 * as + 1) : y0;
          lo = _mm_add_epi16(lo, _mm_maddubs_epi16(_mm_unpacklo_epi8(y0, y1), wcd));
          if (n == 8) hi = _mm_add_epi16(hi, _mm_maddubs_epi16(_mm_unpackhi_epi8(y0, y1), wcd));
        }
        d = _mm_packus_epi16(_mm_srli_epi16(_mm_add_epi16(lo, r8), 4),
                             n == 8 ? _mm_srli_epi16(_mm_add_epi16(hi, r8), 4) : z);
      }
      acc = _mm_add_epi64(acc, _mm_sad_epu8(d, ld(q, q + qs)));
      s = sum64x2(acc);
      if (s >= budget) break;
    }
    return std::min(s, budget);
  }
#endif
  int wa = (4 - fx) * (4 - fy), wb = fx * (4 - fy), wc = (4 - fx) * fy, wd = fx * fy;
  for (int j = 0; j < rows && s < budget; j++, q += qs, a += as) {
    const uint8_t* c = a + as;
    if (!(fx | fy))
      for (int i = 0; i < n; i++) s += abs(q[i] - a[i]);
    else
      for (int i = 0; i < n; i++) s += abs(q[i] - ((wa * a[i] + wb * a[i + 1] + wc * c[i] + wd * c[i + 1] + 8) >> 4));
  }
  return std::min(s, budget);
}
#ifdef H264P_AVX2
// the weights of the bilinear prediction at (fx, fy), by fx + 4 fy, as
// byte pairs for pmaddubsw: of the two samples above (ab) and below (cd)
struct BilW {
  __m256i ab[16], cd[16];
  BilW() {
    for (int f = 0; f < 16; f++) {
      int fx = f & 3, fy = f >> 2;
      ab[f] = _mm256_set1_epi16((short)((4 - fx) * (4 - fy) | fx * (4 - fy) << 8));
      cd[f] = _mm256_set1_epi16((short)((4 - fx) * fy | fx * fy << 8));
    }
  }
};
static const BilW kBilW;
// two rows of 16 samples from a (stride as) predicted bilinearly at (fx,
// fy), as sad_bil's, without a branch on the position
static INLINE __m256i bil16x2(const uint8_t* a, int as, int fx, int fy) {
  const __m256i wab = kBilW.ab[fx + 4 * fy], wcd = kBilW.cd[fx + 4 * fy];
  const __m256i r8 = _mm256_set1_epi16(8);
  __m256i x0 = ld16x2(a, a + as), x1 = ld16x2(a + 1, a + as + 1), y0 = ld16x2(a + as, a + 2 * as), y1 = ld16x2(a + as + 1, a + 2 * as + 1);
  __m256i lo = _mm256_add_epi16(_mm256_maddubs_epi16(_mm256_unpacklo_epi8(x0, x1), wab),
                                _mm256_maddubs_epi16(_mm256_unpacklo_epi8(y0, y1), wcd));
  __m256i hi = _mm256_add_epi16(_mm256_maddubs_epi16(_mm256_unpackhi_epi8(x0, x1), wab),
                                _mm256_maddubs_epi16(_mm256_unpackhi_epi8(y0, y1), wcd));
  return _mm256_packus_epi16(_mm256_srli_epi16(_mm256_add_epi16(lo, r8), 4), _mm256_srli_epi16(_mm256_add_epi16(hi, r8), 4));
}
// sad_bil of rows of 32 samples, a row at a time (each row's pairs of
// samples serve as the row above for one and below for the next), checked
// against budget every 4 rows (budget once it is reached, as sad_bil)
static int sad_bil32(const uint8_t* q, int qs, const uint8_t* a, int as, int rows, int fx, int fy, int budget) {
  auto ld = [](const uint8_t* p) { return _mm256_loadu_si256((const __m256i*)p); };
  __m256i acc = _mm256_setzero_si256();
  int s;
  if (!(fx | fy)) {
    for (int j = 0; j < rows; j++, q += qs, a += as) {
      acc = _mm256_add_epi64(acc, _mm256_sad_epu8(ld(a), ld(q)));
      if ((j & 3) == 3 && (s = sad_sum(acc)) >= budget) return budget;
    }
    return std::min(sad_sum(acc), budget);
  }
  const __m256i wab = kBilW.ab[fx + 4 * fy], wcd = kBilW.cd[fx + 4 * fy], r8 = _mm256_set1_epi16(8);
  __m256i x0 = ld(a), x1 = ld(a + 1);
  __m256i plo = _mm256_unpacklo_epi8(x0, x1), phi = _mm256_unpackhi_epi8(x0, x1);
  for (int j = 0; j < rows; j++, q += qs) {
    a += as;
    __m256i y0 = ld(a), y1 = ld(a + 1);
    __m256i nlo = _mm256_unpacklo_epi8(y0, y1), nhi = _mm256_unpackhi_epi8(y0, y1);
    __m256i lo = _mm256_add_epi16(_mm256_maddubs_epi16(plo, wab), _mm256_maddubs_epi16(nlo, wcd));
    __m256i hi = _mm256_add_epi16(_mm256_maddubs_epi16(phi, wab), _mm256_maddubs_epi16(nhi, wcd));
    __m256i d = _mm256_packus_epi16(_mm256_srli_epi16(_mm256_add_epi16(lo, r8), 4), _mm256_srli_epi16(_mm256_add_epi16(hi, r8), 4));
    acc = _mm256_add_epi64(acc, _mm256_sad_epu8(d, ld(q)));
    plo = nlo, phi = nhi;
    if ((j & 3) == 3 && (s = sad_sum(acc)) >= budget) return budget;
  }
  return std::min(sad_sum(acc), budget);
}
#endif
#ifdef H264P_AVX2
// sad_bil of a block of n x n (4 or 8), whole: two rows of n at a time
// (rows of 4 in the low two quarters, of 8 in the halves)
static INLINE int sad_bil_blk(const uint8_t* q, int qs, const uint8_t* a, int as, int n, int fx, int fy) {
  const __m128i wab = _mm256_castsi256_si128(kBilW.ab[fx + 4 * fy]), wcd = _mm256_castsi256_si128(kBilW.cd[fx + 4 * fy]);
  const __m128i r8 = _mm_set1_epi16(8);
  // (each row's pairs of samples, for it as the row above and below)
  auto ld = [n](const uint8_t* p) {
    if (n == 8) return _mm_loadl_epi64((const __m128i*)p);
    int32_t u;
    memcpy(&u, p, 4);
    return _mm_cvtsi32_si128(u);
  };
  auto pairs = [&](const uint8_t* p) { return _mm_unpacklo_epi8(ld(p), ld(p + 1)); };
  __m128i acc = _mm_setzero_si128(), pa = pairs(a);
  for (int j = 0; j < n; j += 2, q += 2 * qs) {
    __m128i pb = pairs(a + (j + 1) * as), pc = pairs(a + (j + 2) * as);
    __m128i lo, hi;
    if (n == 8) {
      lo = _mm_add_epi16(_mm_maddubs_epi16(pa, wab), _mm_maddubs_epi16(pb, wcd));
      hi = _mm_add_epi16(_mm_maddubs_epi16(pb, wab), _mm_maddubs_epi16(pc, wcd));
    } else {
      lo = _mm_add_epi16(_mm_maddubs_epi16(_mm_unpacklo_epi64(pa, pb), wab), _mm_maddubs_epi16(_mm_unpacklo_epi64(pb, pc), wcd));
      hi = _mm_setzero_si128();
    }
    __m128i d = _mm_packus_epi16(_mm_srli_epi16(_mm_add_epi16(lo, r8), 4), _mm_srli_epi16(_mm_add_epi16(hi, r8), 4));
    // (n 4: the upper half is of zeros in both)
    __m128i t = ld8x2(q, q + qs, n);
    acc = _mm_add_epi64(acc, _mm_sad_epu8(d, t));
    pa = pc;
  }
  return sum64x2(acc);
}
#endif
// A thin template: two rows of n (16, 8 or 4) samples at q (stride qs; none
// if null), loaded once for the many places of a search that sad_bil2
// matches it against
struct Tpl2 {
#ifdef H264P_AVX2
  __m256i v;  // (as sad_bil2 predicts the rows: of 16 in the two halves, else ld8x2's)
#else
  const uint8_t* q;
  int qs;
#endif
  int n;
  Tpl2(const uint8_t* q_, int qs_, int n_) : n(n_) {
#ifdef H264P_AVX2
    v = !q_ ? _mm256_setzero_si256() : n == 16 ? ld16x2(q_, q_ + qs_) : _mm256_inserti128_si256(_mm256_setzero_si256(), ld8x2(q_, q_ + qs_, n), 0);
#else
    q = q_, qs = qs_;
#endif
  }
};
// sad_bil of a thin template against two rows of t.n samples at a, whole,
// and without a branch on the position (fx, fy): the vectors of a search
// change position from one to the next
static INLINE int sad_bil2(const Tpl2& t, const uint8_t* a, int as, int fx, int fy) {
#ifdef H264P_AVX2
  if (t.n == 16) return sad_sum(_mm256_sad_epu8(bil16x2(a, as, fx, fy), t.v));
  const int n = t.n;
  const __m128i wab = _mm256_castsi256_si128(kBilW.ab[fx + 4 * fy]), wcd = _mm256_castsi256_si128(kBilW.cd[fx + 4 * fy]);
  const __m128i r8 = _mm_set1_epi16(8);
  __m128i x0 = ld8x2(a, a + as, n), x1 = ld8x2(a + 1, a + as + 1, n), y0 = ld8x2(a + as, a + 2 * as, n),
          y1 = ld8x2(a + as + 1, a + 2 * as + 1, n);
  __m128i lo = _mm_add_epi16(_mm_maddubs_epi16(_mm_unpacklo_epi8(x0, x1), wab), _mm_maddubs_epi16(_mm_unpacklo_epi8(y0, y1), wcd));
  __m128i hi = _mm_add_epi16(_mm_maddubs_epi16(_mm_unpackhi_epi8(x0, x1), wab), _mm_maddubs_epi16(_mm_unpackhi_epi8(y0, y1), wcd));
  __m128i d = _mm_packus_epi16(_mm_srli_epi16(_mm_add_epi16(lo, r8), 4), _mm_srli_epi16(_mm_add_epi16(hi, r8), 4));
  // (n 4: the upper half is of the zeros beyond both rows, the same in both)
  __m128i v = _mm_sad_epu8(d, _mm256_castsi256_si128(t.v));
  return sum64x2(v);
#else
  return sad_bil(t.q, t.qs, a, as, t.n, 2, fx, fy, INT_MAX);
#endif
}

// The sums of the absolute differences between neighbouring samples of n
// x n (16, 8 or 4) at p: across (gx: n - 1 pairs a row) and down (gy)
static void grads(const uint8_t* p, int stride, int n, int& gx, int& gy) {
#ifdef H264P_AVX2
  static const uint8_t kOnes[32] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  auto ld = [n](const uint8_t* q) {
    if (n == 16) return _mm_loadu_si128((const __m128i*)q);
    if (n == 8) return _mm_loadl_epi64((const __m128i*)q);
    int32_t u;
    memcpy(&u, q, 4);
    return _mm_cvtsi32_si128(u);
  };
  // (a row against itself one on, its last sample out of both)
  const __m128i m = _mm_loadu_si128((const __m128i*)(kOnes + 17 - n));
  __m128i ax = _mm_setzero_si128(), ay = ax, x = ld(p);
  for (int j = 0; j < n; j++) {
    ax = _mm_add_epi64(ax, _mm_sad_epu8(_mm_and_si128(x, m), _mm_srli_si128(x, 1)));
    if (j + 1 < n) {
      __m128i y = ld(p + (j + 1) * stride);
      ay = _mm_add_epi64(ay, _mm_sad_epu8(x, y));
      x = y;
    }
  }
  gx = sum64x2(ax);
  gy = sum64x2(ay);
#else
  gx = gy = 0;
  for (int j = 0; j < n; j++)
    for (int i = 0; i < n; i++) {
      if (i + 1 < n) gx += abs(p[j * stride + i + 1] - p[j * stride + i]);
      if (j + 1 < n) gy += abs(p[(j + 1) * stride + i] - p[j * stride + i]);
    }
#endif
}

// rows of n samples (any n; 16, 8, 4 and 2 inlined), from s to d
static INLINE void copy_rows(uint8_t* d, int ds, const uint8_t* s, int ss, int n, int rows) {
  switch (n) {
    case 16:
      for (int j = 0; j < rows; j++) memcpy(d + j * ds, s + j * ss, 16);
      break;
    case 8:
      for (int j = 0; j < rows; j++) memcpy(d + j * ds, s + j * ss, 8);
      break;
    case 4:
      for (int j = 0; j < rows; j++) memcpy(d + j * ds, s + j * ss, 4);
      break;
    case 2:
      for (int j = 0; j < rows; j++) memcpy(d + j * ds, s + j * ss, 2);
      break;
    default:
      for (int j = 0; j < rows; j++) memcpy(d + j * ds, s + j * ss, n);
  }
}

// 16 x 16 samples at s (stride ss) transposed into d (stride ds)
static void transpose16(const uint8_t* s, int ss, uint8_t* d, int ds) {
#ifdef H264P_AVX2
  // interleaved bytes, words, dwords and qwords of rows ever further apart
  __m128i r[16], t[16];
  for (int i = 0; i < 16; i++) r[i] = _mm_loadu_si128((const __m128i*)(s + (size_t)i * ss));
  for (int i = 0; i < 8; i++) {
    t[i] = _mm_unpacklo_epi8(r[2 * i], r[2 * i + 1]);
    t[i + 8] = _mm_unpackhi_epi8(r[2 * i], r[2 * i + 1]);
  }
  for (int h = 0; h < 2; h++)
    for (int i = 0; i < 4; i++) {
      r[h * 8 + i] = _mm_unpacklo_epi16(t[h * 8 + 2 * i], t[h * 8 + 2 * i + 1]);
      r[h * 8 + i + 4] = _mm_unpackhi_epi16(t[h * 8 + 2 * i], t[h * 8 + 2 * i + 1]);
    }
  for (int q = 0; q < 4; q++)
    for (int i = 0; i < 2; i++) {
      t[q * 4 + i] = _mm_unpacklo_epi32(r[q * 4 + 2 * i], r[q * 4 + 2 * i + 1]);
      t[q * 4 + i + 2] = _mm_unpackhi_epi32(r[q * 4 + 2 * i], r[q * 4 + 2 * i + 1]);
    }
  // (t[4q + 2p]: rows 0-7 of columns 4q + 2p and the next, t[4q + 2p + 1]
  // rows 8-15)
  for (int c = 0; c < 16; c += 2) {
    int k = c / 4 * 4 + (c / 2 & 1) * 2;
    _mm_storeu_si128((__m128i*)(d + (size_t)c * ds), _mm_unpacklo_epi64(t[k], t[k + 1]));
    _mm_storeu_si128((__m128i*)(d + (size_t)(c + 1) * ds), _mm_unpackhi_epi64(t[k], t[k + 1]));
  }
#else
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 16; j++) d[(size_t)i * ds + j] = s[(size_t)j * ss + i];
#endif
}

}  // namespace h264p
