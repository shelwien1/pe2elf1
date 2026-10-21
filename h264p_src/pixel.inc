// pixel.inc -- the pixel kernels of the decoder in the loop and of the
// model's features from its samples (recon.inc): the bilinear prediction
// and its SAD at several widths, Hadamard transforms, the sums of a block,
// gradients, row copies, transposition, and the AVX2 helpers they share
// (with integer vectors of 128 and 256 bits under one name, for the kernels
// written once for both). Each has AVX2 code and plain C++ beside it, which
// compute the same.

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

// The bilinear prediction by pmaddubsw, in steps: the byte pairs of a row
// (a, a': lo and hi, as unpacked) times their weights (the byte pair wa |
// wb << 8), those of the row below (c, c') with theirs added, then plus
// rnd, >> sh, packed back to bytes in order: (wa a + wb a' + wc c + wd c' +
// rnd) >> sh. The luma's quarter samples (sad_bil and the kernels after it:
// rnd 8, sh 4) and the chroma's eighths (mc_chroma: 32, 6).
static INLINE void bil_mul(__m256i plo, __m256i phi, __m256i w, __m256i& lo, __m256i& hi) {
  lo = _mm256_maddubs_epi16(plo, w), hi = _mm256_maddubs_epi16(phi, w);
}
static INLINE void bil_mac(__m256i plo, __m256i phi, __m256i w, __m256i& lo, __m256i& hi) {
  lo = _mm256_add_epi16(lo, _mm256_maddubs_epi16(plo, w)), hi = _mm256_add_epi16(hi, _mm256_maddubs_epi16(phi, w));
}
static INLINE __m256i bil_pack(__m256i lo, __m256i hi, __m256i rnd, int sh) {
  return _mm256_packus_epi16(_mm256_srli_epi16(_mm256_add_epi16(lo, rnd), sh), _mm256_srli_epi16(_mm256_add_epi16(hi, rnd), sh));
}
static INLINE void bil_mul(__m128i plo, __m128i phi, __m128i w, __m128i& lo, __m128i& hi) {
  lo = _mm_maddubs_epi16(plo, w), hi = _mm_maddubs_epi16(phi, w);
}
static INLINE void bil_mac(__m128i plo, __m128i phi, __m128i w, __m128i& lo, __m128i& hi) {
  lo = _mm_add_epi16(lo, _mm_maddubs_epi16(plo, w)), hi = _mm_add_epi16(hi, _mm_maddubs_epi16(phi, w));
}
static INLINE __m128i bil_pack(__m128i lo, __m128i hi, __m128i rnd, int sh) {
  return _mm_packus_epi16(_mm_srli_epi16(_mm_add_epi16(lo, rnd), sh), _mm_srli_epi16(_mm_add_epi16(hi, rnd), sh));
}
// of two whole rows (x0 and x1 the samples and those one on, y0 and y1 the
// same of the row below): the weighted pairs of both, packed
static INLINE __m256i bil_rows(__m256i x0, __m256i x1, __m256i y0, __m256i y1, __m256i wab, __m256i wcd, __m256i rnd, int sh) {
  __m256i lo, hi;
  bil_mul(_mm256_unpacklo_epi8(x0, x1), _mm256_unpackhi_epi8(x0, x1), wab, lo, hi);
  bil_mac(_mm256_unpacklo_epi8(y0, y1), _mm256_unpackhi_epi8(y0, y1), wcd, lo, hi);
  return bil_pack(lo, hi, rnd, sh);
}
static INLINE __m128i bil_rows(__m128i x0, __m128i x1, __m128i y0, __m128i y1, __m128i wab, __m128i wcd, __m128i rnd, int sh) {
  __m128i lo, hi;
  bil_mul(_mm_unpacklo_epi8(x0, x1), _mm_unpackhi_epi8(x0, x1), wab, lo, hi);
  bil_mac(_mm_unpacklo_epi8(y0, y1), _mm_unpackhi_epi8(y0, y1), wcd, lo, hi);
  return bil_pack(lo, hi, rnd, sh);
}

// Integer vectors of 128 bits (V128) and 256 (V256) under one name, for the
// kernels written once for both widths (mc_luma's half samples, weighted
// prediction): the 256-bit operations work in two halves of 128, which
// those kernels keep apart until they pack them back to bytes.
struct V128 {
  typedef __m128i T;
  static INLINE T zero() { return _mm_setzero_si128(); }
  static INLINE T set16(int v) { return _mm_set1_epi16((short)v); }
  static INLINE T set32(int v) { return _mm_set1_epi32(v); }
  static INLINE T add16(T a, T b) { return _mm_add_epi16(a, b); }
  static INLINE T sub16(T a, T b) { return _mm_sub_epi16(a, b); }
  static INLINE T mul16(T a, T b) { return _mm_mullo_epi16(a, b); }
  static INLINE T sra16(T a, int n) { return _mm_srai_epi16(a, n); }
  static INLINE T add32(T a, T b) { return _mm_add_epi32(a, b); }
  static INLINE T sra32(T a, int n) { return _mm_srai_epi32(a, n); }
  static INLINE T sra32(T a, __m128i n) { return _mm_sra_epi32(a, n); }
  static INLINE T madd(T a, T b) { return _mm_madd_epi16(a, b); }
  static INLINE T unlo8(T a, T b) { return _mm_unpacklo_epi8(a, b); }
  static INLINE T unhi8(T a, T b) { return _mm_unpackhi_epi8(a, b); }
  static INLINE T unlo16(T a, T b) { return _mm_unpacklo_epi16(a, b); }
  static INLINE T unhi16(T a, T b) { return _mm_unpackhi_epi16(a, b); }
  static INLINE T packs32(T a, T b) { return _mm_packs_epi32(a, b); }
  static INLINE T packus16(T a, T b) { return _mm_packus_epi16(a, b); }
  static INLINE T avg8(T a, T b) { return _mm_avg_epu8(a, b); }
  // n (8 or 4) samples at p in 16-bit lanes, and back to bytes, clipped
  static INLINE T ld_px(const uint8_t* p, int n) {
    if (n == 4) {
      int32_t t;
      memcpy(&t, p, 4);
      return _mm_unpacklo_epi8(_mm_cvtsi32_si128(t), zero());
    }
    return _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i*)p), zero());
  }
  static INLINE void st_px(uint8_t* p, T v, int n) {
    v = _mm_packus_epi16(v, zero());
    if (n == 4) {
      int32_t t = _mm_cvtsi128_si32(v);
      memcpy(p, &t, 4);
    } else {
      _mm_storel_epi64((__m128i*)p, v);
    }
  }
};
struct V256 {
  typedef __m256i T;
  static INLINE T zero() { return _mm256_setzero_si256(); }
  static INLINE T set16(int v) { return _mm256_set1_epi16((short)v); }
  static INLINE T set32(int v) { return _mm256_set1_epi32(v); }
  static INLINE T add16(T a, T b) { return _mm256_add_epi16(a, b); }
  static INLINE T sub16(T a, T b) { return _mm256_sub_epi16(a, b); }
  static INLINE T mul16(T a, T b) { return _mm256_mullo_epi16(a, b); }
  static INLINE T sra16(T a, int n) { return _mm256_srai_epi16(a, n); }
  static INLINE T add32(T a, T b) { return _mm256_add_epi32(a, b); }
  static INLINE T sra32(T a, int n) { return _mm256_srai_epi32(a, n); }
  static INLINE T sra32(T a, __m128i n) { return _mm256_sra_epi32(a, n); }
  static INLINE T madd(T a, T b) { return _mm256_madd_epi16(a, b); }
  static INLINE T unlo8(T a, T b) { return _mm256_unpacklo_epi8(a, b); }
  static INLINE T unhi8(T a, T b) { return _mm256_unpackhi_epi8(a, b); }
  static INLINE T unlo16(T a, T b) { return _mm256_unpacklo_epi16(a, b); }
  static INLINE T unhi16(T a, T b) { return _mm256_unpackhi_epi16(a, b); }
  static INLINE T packs32(T a, T b) { return _mm256_packs_epi32(a, b); }
  static INLINE T packus16(T a, T b) { return _mm256_packus_epi16(a, b); }
  static INLINE T avg8(T a, T b) { return _mm256_avg_epu8(a, b); }
  // 16 samples at p in 16-bit lanes, and back (n is 16)
  static INLINE T ld_px(const uint8_t* p, int) { return _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)p)); }
  static INLINE void st_px(uint8_t* p, T v, int) {
    _mm_storeu_si128((__m128i*)p, _mm_packus_epi16(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1)));
  }
};
// Weighted sample prediction (8.4.2.3) of the bytes of x and y in pairs:
// times (m0, m1), plus r, >> sh, plus o, clipped to bytes, in order
template <class V>
static INLINE typename V::T wsum(typename V::T x, typename V::T y, int m0, int m1, int r, int sh, int o) {
  typedef typename V::T T;
  const T m = V::set32((int)((unsigned)(m0 & 0xFFFF) | (unsigned)m1 << 16)), rv = V::set32(r), ov = V::set32(o), z = V::zero();
  const __m128i c = _mm_cvtsi32_si128(sh);
  T lo = V::unlo8(x, y), hi = V::unhi8(x, y);
  T a0 = V::madd(V::unlo8(lo, z), m), a1 = V::madd(V::unhi8(lo, z), m);
  T a2 = V::madd(V::unlo8(hi, z), m), a3 = V::madd(V::unhi8(hi, z), m);
  a0 = V::add32(V::sra32(V::add32(a0, rv), c), ov);
  a1 = V::add32(V::sra32(V::add32(a1, rv), c), ov);
  a2 = V::add32(V::sra32(V::add32(a2, rv), c), ov);
  a3 = V::add32(V::sra32(V::add32(a3, rv), c), ov);
  return V::packus16(V::packs32(a0, a1), V::packs32(a2, a3));
}
#endif
// the bilinear prediction of a sample from a, a' (the one right of it), c
// (below a) and c' by weights wa, wb, wc, wd: plus rnd, >> sh (as bil_mul
// and the steps after it)
static INLINE int bil1(int wa, int wb, int wc, int wd, int a, int a1, int c, int c1, int rnd, int sh) {
  return (wa * a + wb * a1 + wc * c + wd * c1 + rnd) >> sh;
}

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
          // (the row below only where fy: its weights are 0 else)
          __m256i x1 = fx ? ld(a + 1, a + as + 1) : d, lo, hi;
          bil_mul(_mm256_unpacklo_epi8(d, x1), _mm256_unpackhi_epi8(d, x1), wab, lo, hi);
          if (fy) {
            __m256i y0 = ld(a + as, a + 2 * as), y1 = fx ? ld(a + as + 1, a + 2 * as + 1) : y0;
            bil_mac(_mm256_unpacklo_epi8(y0, y1), _mm256_unpackhi_epi8(y0, y1), wcd, lo, hi);
          }
          d = bil_pack(lo, hi, r8, 4);
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
        // (rows of 4: their high halves are zero, and stay out of the sum)
        __m128i x1 = fx ? ld(a + 1, a + as + 1) : d, lo, hi;
        bil_mul(_mm_unpacklo_epi8(d, x1), n == 8 ? _mm_unpackhi_epi8(d, x1) : z, wab, lo, hi);
        if (fy) {
          __m128i y0 = ld(a + as, a + 2 * as), y1 = fx ? ld(a + as + 1, a + 2 * as + 1) : y0;
          bil_mac(_mm_unpacklo_epi8(y0, y1), n == 8 ? _mm_unpackhi_epi8(y0, y1) : z, wcd, lo, hi);
        }
        d = bil_pack(lo, hi, r8, 4);
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
      for (int i = 0; i < n; i++) s += abs(q[i] - bil1(wa, wb, wc, wd, a[i], a[i + 1], c[i], c[i + 1], 8, 4));
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
  return bil_rows(ld16x2(a, a + as), ld16x2(a + 1, a + as + 1), ld16x2(a + as, a + 2 * as), ld16x2(a + as + 1, a + 2 * as + 1),
                  kBilW.ab[fx + 4 * fy], kBilW.cd[fx + 4 * fy], _mm256_set1_epi16(8), 4);
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
    __m256i nlo = _mm256_unpacklo_epi8(y0, y1), nhi = _mm256_unpackhi_epi8(y0, y1), lo, hi;
    bil_mul(plo, phi, wab, lo, hi);
    bil_mac(nlo, nhi, wcd, lo, hi);
    acc = _mm256_add_epi64(acc, _mm256_sad_epu8(bil_pack(lo, hi, r8, 4), ld(q)));
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
    __m128i pb = pairs(a + (j + 1) * as), pc = pairs(a + (j + 2) * as), lo, hi;
    // (rows of 8: a row's pairs in lo, the next's in hi; of 4, both rows'
    // pairs in lo, and hi of zeros, as the upper half of the template)
    const __m128i z = _mm_setzero_si128();
    bil_mul(n == 8 ? pa : _mm_unpacklo_epi64(pa, pb), n == 8 ? pb : z, wab, lo, hi);
    bil_mac(n == 8 ? pb : _mm_unpacklo_epi64(pb, pc), n == 8 ? pc : z, wcd, lo, hi);
    __m128i d = bil_pack(lo, hi, r8, 4);
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
  // (n 4: the upper half is of the zeros beyond both rows, the same in both)
  __m128i d = bil_rows(ld8x2(a, a + as, n), ld8x2(a + 1, a + as + 1, n), ld8x2(a + as, a + 2 * as, n), ld8x2(a + as + 1, a + 2 * as + 1, n),
                       _mm256_castsi256_si128(kBilW.ab[fx + 4 * fy]), _mm256_castsi256_si128(kBilW.cd[fx + 4 * fy]), _mm_set1_epi16(8), 4);
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

// --- The luma prediction (8.4.2.2.1). The half samples a quarter-sample
// position c (fx + 4 fy) takes: b between two whole samples across, h
// between two down, j between four; b from the row below where fy is 3
// and fx not 0 (boff), h from the column right where fx is 3 and fy not 0
// (hoff).
struct Halves {
  bool b, h, j;
  int boff, hoff;
  explicit Halves(int c)
      : b(0xE0EE >> c & 1), h(0xBBB0 >> c & 1), j(0x4E40 >> c & 1), boff(c >= 13), hoff(c == 7 || c == 11 || c == 15) {}
};
#ifdef H264P_AVX2
// mc_halves of w columns (V256: 16; V128: 8 or 4) in 16-bit lanes, j's sums
// in 32 bits
template <class V>
static INLINE void mc_halves_v(const uint8_t* g, int gs, int w, int h, const Halves& n, uint8_t* bb, uint8_t* hh, uint8_t* jj) {
  typedef typename V::T T;
  const T k5 = V::set16(5), k20 = V::set16(20), k16 = V::set16(16);
  const T p1m5 = V::set32((int)0xFFFB0001u), m5p1 = V::set32((int)0x0001FFFBu), k512 = V::set32(512);
  auto ld = [w](const uint8_t* p) { return V::ld_px(p, w); };
  auto st = [w](uint8_t* p, T v) { V::st_px(p, v, w); };
  auto t6 = [&](T a, T b, T c, T d, T e, T f) {
    return V::add16(V::sub16(V::add16(a, f), V::mul16(V::add16(b, e), k5)), V::mul16(V::add16(c, d), k20));
  };
  auto across = [&](const uint8_t* p) { return t6(ld(p - 2), ld(p - 1), ld(p), ld(p + 1), ld(p + 2), ld(p + 3)); };
  auto r5 = [&](T v) { return V::sra16(V::add16(v, k16), 5); };
  if (n.j) {
    T r0 = across(g - 2 * gs), r1 = across(g - gs), r2 = across(g), r3 = across(g + gs), r4 = across(g + 2 * gs);
    for (int j = 0; j < h; j++) {
      T r5v = across(g + (j + 3) * gs);
      // (the pairs of rows interleaved by columns 0-3 and 4-7 of each 8:
      // packed back in order)
      T lo = V::add32(V::add32(V::madd(V::unlo16(r0, r1), p1m5), V::madd(V::unlo16(r2, r3), k20)), V::madd(V::unlo16(r4, r5v), m5p1));
      T hi = V::add32(V::add32(V::madd(V::unhi16(r0, r1), p1m5), V::madd(V::unhi16(r2, r3), k20)), V::madd(V::unhi16(r4, r5v), m5p1));
      st(jj + j * 16, V::packs32(V::sra32(V::add32(lo, k512), 10), V::sra32(V::add32(hi, k512), 10)));
      if (n.b) st(bb + j * 16, r5(n.boff ? r3 : r2));
      r0 = r1, r1 = r2, r2 = r3, r3 = r4, r4 = r5v;
    }
  } else if (n.b) {
    for (int j = 0; j < h; j++) st(bb + j * 16, r5(across(g + (j + n.boff) * gs)));
  }
  if (n.h) {
    const uint8_t* p = g + n.hoff;
    T v0 = ld(p - 2 * gs), v1 = ld(p - gs), v2 = ld(p), v3 = ld(p + gs), v4 = ld(p + 2 * gs);
    for (int j = 0; j < h; j++) {
      T v5 = ld(p + (j + 3) * gs);
      st(hh + j * 16, r5(t6(v0, v1, v2, v3, v4, v5)));
      v0 = v1, v1 = v2, v2 = v3, v3 = v4, v4 = v5;
    }
  }
}
#endif
// The half samples position c takes, rounded and clipped, w x h of each
// (stride 16: bb, hh, jj), from the whole samples at g (stride gs, read 2
// before them and 3 after, across and down)
static void mc_halves(const uint8_t* g, int gs, int w, int h, int c, uint8_t* bb, uint8_t* hh, uint8_t* jj) {
  const Halves n(c);
#ifdef H264P_AVX2
  if (w == 16)
    mc_halves_v<V256>(g, gs, w, h, n, bb, hh, jj);
  else
    mc_halves_v<V128>(g, gs, w, h, n, bb, hh, jj);
#else
#define T6(p, s) ((p)[-2 * (s)] - 5 * (p)[-(s)] + 20 * (p)[0] + 20 * (p)[s] - 5 * (p)[2 * (s)] + (p)[3 * (s)])
  int b1[21][16];
  if (n.j) {
    for (int j = -2; j < h + 3; j++)
      for (int i = 0; i < w; i++) b1[j + 2][i] = T6(g + j * gs + i, 1);
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++)
        jj[j * 16 + i] = (uint8_t)clip1((b1[j][i] - 5 * b1[j + 1][i] + 20 * b1[j + 2][i] + 20 * b1[j + 3][i] -
                                         5 * b1[j + 4][i] + b1[j + 5][i] + 512) >> 10);
  } else if (n.b) {
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++) b1[j + n.boff + 2][i] = T6(g + (j + n.boff) * gs + i, 1);
  }
  if (n.b)
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++) bb[j * 16 + i] = (uint8_t)clip1((b1[j + n.boff + 2][i] + 16) >> 5);
  if (n.h)
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++) hh[j * 16 + i] = (uint8_t)clip1((T6(g + j * gs + i + n.hoff, gs) + 16) >> 5);
#undef T6
#endif
}
// The samples of position c (not 0), w x h into o (stride 16): each the
// rounded mean of two of the whole samples (at g, stride gs) and the half
// ones (bb, hh, jj), or of one with itself
static void mc_mean(int c, const uint8_t* g, int gs, const uint8_t* bb, const uint8_t* hh, const uint8_t* jj, int w, int h,
                    uint8_t* o) {
  const uint8_t *s1, *s2;
  int t1 = 16, t2 = 16;
  switch (c) {
    case 1: s1 = g, t1 = gs, s2 = bb; break;
    case 2: s1 = s2 = bb; break;
    case 3: s1 = bb, s2 = g + 1, t2 = gs; break;
    case 4: s1 = g, t1 = gs, s2 = hh; break;
    case 5: case 7: s1 = bb, s2 = hh; break;
    case 6: s1 = bb, s2 = jj; break;
    case 8: s1 = s2 = hh; break;
    case 9: case 11: s1 = hh, s2 = jj; break;
    case 10: s1 = s2 = jj; break;
    case 12: s1 = hh, s2 = g + gs, t2 = gs; break;
    case 14: s1 = jj, s2 = bb; break;
    default: s1 = hh, s2 = bb; break;  // 13, 15
  }
  for (int j = 0; j < h; j++) {
    const uint8_t *__restrict a = s1 + j * t1, *__restrict b = s2 + j * t2;
    uint8_t* __restrict out = o + j * 16;
#ifdef H264P_AVX2
    if (w == 16) {
      _mm_storeu_si128((__m128i*)out, _mm_avg_epu8(_mm_loadu_si128((const __m128i*)a), _mm_loadu_si128((const __m128i*)b)));
      continue;
    }
    if (w == 8) {
      _mm_storel_epi64((__m128i*)out, _mm_avg_epu8(_mm_loadl_epi64((const __m128i*)a), _mm_loadl_epi64((const __m128i*)b)));
      continue;
    }
#endif
    for (int i = 0; i < w; i++) out[i] = (uint8_t)((a[i] + b[i] + 1) >> 1);
  }
}
// The prediction of w x h luma samples at (x, y) of the plane RP (stride
// ps, readable pm out of its W x H) displaced by (mvx, mvy) quarter
// samples, into o (stride 16)
static void mc_luma_p(const uint8_t* RP, int ps, int pm, int W, int H, int x, int y, int w, int h, int mvx, int mvy, uint8_t* o) {
#ifdef H264P_AVX2
  if ((w == 2 || w == 12) && (mvx & 3 || mvy & 3)) {
    // (as 4 or 16 wide, the widths it does at a time: each column of the
    // prediction is what it is in a wider block)
    uint8_t t[16 * 16];
    mc_luma_p(RP, ps, pm, W, H, x, y, w == 2 ? 4 : 16, h, mvx, mvy, t);
    copy_rows(o, 16, t, 16, w, h);
    return;
  }
#endif
  int xi = x + (mvx >> 2), yi = y + (mvy >> 2), fx = mvx & 3, fy = mvy & 3;
  // the samples from (xi - 2, yi - 2) on: in the picture (padded), or
  // copied with the edges repeated
  uint8_t tmp[21 * 21];
  const uint8_t* g;
  int gs;
  if (xi - 2 >= -pm && yi - 2 >= -pm && xi + w + 3 <= W + pm && yi + h + 3 <= H + pm) {
    g = RP + (yi - 2) * ps + xi - 2;
    gs = ps;
  } else {
    for (int j = 0; j < h + 5; j++) {
      const uint8_t* row = RP + clip3(0, H - 1, yi - 2 + j) * ps;
      for (int i = 0; i < w + 5; i++) tmp[j * 21 + i] = row[clip3(0, W - 1, xi - 2 + i)];
    }
    g = tmp;
    gs = 21;
  }
  g += 2 * gs + 2;  // at (xi, yi)
  if (!fx && !fy) {
    copy_rows(o, 16, g, gs, w, h);
    return;
  }
  uint8_t bb[16 * 16], hh[16 * 16], jj[16 * 16];
  mc_halves(g, gs, w, h, fy * 4 + fx, bb, hh, jj);
  mc_mean(fy * 4 + fx, g, gs, bb, hh, jj, w, h, o);
}

// The 4-point Hadamard transform of x[0], x[xs], x[2 xs], x[3 xs] into
// y[0], y[ys], ... in the order of the DC transforms (8.5.10): x0 + x1 +
// x2 + x3, x0 + x1 - x2 - x3, x0 - x1 - x2 + x3, x0 - x1 + x2 - x3
static INLINE void hadamard4(const int* x, int xs, int* y, int ys) {
  int a = x[0], b = x[xs], c = x[2 * xs], d = x[3 * xs];
  y[0] = a + b + c + d;
  y[ys] = a + b - c - d;
  y[2 * ys] = a - b - c + d;
  y[3 * ys] = a - b + c - d;
}
// and the 2x2 one of the chroma DC (8.5.11.1), x in raster order
static INLINE void hadamard2x2(const int* x, int* f) {
  f[0] = x[0] + x[1] + x[2] + x[3];
  f[1] = x[0] - x[1] + x[2] - x[3];
  f[2] = x[0] + x[1] - x[2] - x[3];
  f[3] = x[0] - x[1] - x[2] + x[3];
}

// x264's SATD of an n x n block (n a multiple of 4) against a prediction
// (strides sa, sb), by 4x4, halved
static int satd_nxn(const uint8_t* a, int sa, const uint8_t* b, int sb, int n) {
#ifdef H264P_AVX2
  if (n == 16 || n == 8 || n == 4) {
    // four rows at a time, the 4x4 blocks side by side in 16-bit lanes: the
    // transform down by adds across the rows, across by adds of lanes
    // swapped in pairs and in pairs of pairs (the order and signs of the
    // coefficients do not change the sum of their magnitudes)
    __m256i acc = _mm256_setzero_si256();
    const __m256i one = _mm256_set1_epi16(1);
    for (int y = 0; y < n; y += 4) {
      __m256i r[4];
      for (int j = 0; j < 4; j++) {
        const uint8_t *p = a + (y + j) * sa, *q = b + (y + j) * sb;
        if (n == 16) {
          r[j] = _mm256_sub_epi16(_mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)p)),
                                  _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i*)q)));
        } else {
          // (in the low half, the high one zero)
          __m128i d;
          if (n == 8) {
            d = _mm_sub_epi16(_mm_cvtepu8_epi16(_mm_loadl_epi64((const __m128i*)p)), _mm_cvtepu8_epi16(_mm_loadl_epi64((const __m128i*)q)));
          } else {
            int32_t u, v;
            memcpy(&u, p, 4);
            memcpy(&v, q, 4);
            d = _mm_sub_epi16(_mm_cvtepu8_epi16(_mm_cvtsi32_si128(u)), _mm_cvtepu8_epi16(_mm_cvtsi32_si128(v)));
          }
          r[j] = _mm256_inserti128_si256(_mm256_setzero_si256(), d, 0);
        }
      }
      __m256i a0 = _mm256_add_epi16(r[0], r[1]), a1 = _mm256_sub_epi16(r[0], r[1]);
      __m256i a2 = _mm256_add_epi16(r[2], r[3]), a3 = _mm256_sub_epi16(r[2], r[3]);
      __m256i v[4] = {_mm256_add_epi16(a0, a2), _mm256_add_epi16(a1, a3), _mm256_sub_epi16(a0, a2), _mm256_sub_epi16(a1, a3)};
      for (int j = 0; j < 4; j++) {
        __m256i x = v[j], t = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(x, 0xB1), 0xB1);
        x = _mm256_blend_epi16(_mm256_add_epi16(x, t), _mm256_sub_epi16(x, t), 0xAA);
        t = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(x, 0x4E), 0x4E);
        x = _mm256_blend_epi16(_mm256_add_epi16(x, t), _mm256_sub_epi16(x, t), 0xCC);
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(_mm256_abs_epi16(x), one));
      }
    }
    return (sum32x8(acc) + 1) >> 1;
  }
#endif
  int s = 0;
  for (int by4 = 0; by4 < n; by4 += 4)
    for (int bx4 = 0; bx4 < n; bx4 += 4) {
      int t[16], f[16];
      for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) t[y * 4 + x] = a[(by4 + y) * sa + bx4 + x] - b[(by4 + y) * sb + bx4 + x];
      for (int y = 0; y < 4; y++) hadamard4(t + y * 4, 1, t + y * 4, 1);
      for (int x = 0; x < 4; x++) hadamard4(t + x, 4, f + x, 4);
      for (int i = 0; i < 16; i++) s += abs(f[i]);
    }
  return (s + 1) >> 1;
}

// The sum of the samples of an n x n block at p (n 16 or 8) and the sum of
// their squares; sum4 (if given, n 16) gets the sums of its 4x4 blocks, in
// raster order
static void blk_sums(const uint8_t* p, int stride, int n, int64_t& s1, int64_t& s2, int* sum4 = nullptr) {
#ifdef H264P_AVX2
  if (sum4) {
    // a row's sums of four by pmaddubsw and pmaddwd, its squares by
    // pmaddwd
    const __m128i o8 = _mm_set1_epi8(1), o16 = _mm_set1_epi16(1), z = _mm_setzero_si128();
    __m128i sq = z;
    for (int by = 0; by < 4; by++) {
      __m128i acc = z;
      for (int y = 0; y < 4; y++) {
        __m128i r = _mm_loadu_si128((const __m128i*)(p + (by * 4 + y) * stride));
        acc = _mm_add_epi32(acc, _mm_madd_epi16(_mm_maddubs_epi16(r, o8), o16));
        __m128i lo = _mm_unpacklo_epi8(r, z), hi = _mm_unpackhi_epi8(r, z);
        sq = _mm_add_epi32(sq, _mm_add_epi32(_mm_madd_epi16(lo, lo), _mm_madd_epi16(hi, hi)));
      }
      _mm_storeu_si128((__m128i*)&sum4[by * 4], acc);
    }
    s1 = 0;
    for (int b = 0; b < 16; b++) s1 += sum4[b];
    s2 = sum32x4(sq);
    return;
  }
  // the sums by psadbw against zeros, the squares by pmaddwd: two rows of
  // 16 a vector, or four of 8
  const __m256i z = _mm256_setzero_si256();
  __m256i a1 = z, a2 = z;
  auto sums = [&](__m256i x) {
    a1 = _mm256_add_epi64(a1, _mm256_sad_epu8(x, z));
    __m256i lo = _mm256_unpacklo_epi8(x, z), hi = _mm256_unpackhi_epi8(x, z);
    a2 = _mm256_add_epi32(a2, _mm256_add_epi32(_mm256_madd_epi16(lo, lo), _mm256_madd_epi16(hi, hi)));
  };
  if (n == 16) {
    for (int y = 0; y < 16; y += 2) sums(ld16x2(p + y * stride, p + (y + 1) * stride));
  } else {
    for (int y = 0; y < 8; y += 4) {
      __m128i r0 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*)(p + y * stride)), _mm_loadl_epi64((const __m128i*)(p + (y + 1) * stride)));
      __m128i r1 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*)(p + (y + 2) * stride)), _mm_loadl_epi64((const __m128i*)(p + (y + 3) * stride)));
      sums(_mm256_inserti128_si256(_mm256_castsi128_si256(r0), r1, 1));
    }
  }
  s1 = sad_sum(a1);
  s2 = sum32x8(a2);
#else
  s1 = s2 = 0;
  for (int b = 0; b < n * n / 16; b++) {
    const uint8_t* q = p + (b / (n / 4)) * 4 * stride + (b % (n / 4)) * 4;
    int bs1 = 0;
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++) {
        int v = q[y * stride + x];
        bs1 += v;
        s2 += v * v;
      }
    s1 += bs1;
    if (sum4) sum4[b] = bs1;
  }
#endif
}

}  // namespace h264p
