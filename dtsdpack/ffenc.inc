// ffenc.inc -- FFmpeg's DTS encoder's bit allocations (libavcodec/dcaenc.c, as of FFmpeg 6.1), made
// again from the WAV: its subband transform, the subbands' peaks, its psychoacoustic masking and its
// search for the noise level whose ABITS the frame's bits allow (assign_bits()), as it computes them
// but for the masking's MDCT, here in doubles through a 128-point FFT (FFmpeg's in 32-bit integers,
// which round at each step: a coefficient's centibels the same but where they cross a boundary),
// its rotations from tables (so that it is the same on every machine). Re-encoding FFmpeg's decode of its encode of
// `s.2.dts`'s first 3 MB, FFmpeg's encoder makes 99.99% of the ABITS this makes from that decode
// (in 625 of its 633 frames all of them); of the stream's own, both 97.2%: the decode isn't the
// encoder's input. Each frame's input the WAV's 512 samples from where the core's first block's
// analysis window ends, as FFmpeg's encoder's is, the 512 before them its history. The same
// integers as dcaenc.c's, made faster: the transform 8 values at a time (AVX2), the masking's sums
// over the bins that can change them, its bands side by side, and each subband's bits at each ABITS
// once a frame, not at each noise level the search tries (FFmpeg's encodes compressed 17-20% faster)
#include "ffenctab.inc"

struct FfSoft {
  S32 m, e;
};
static const FfSoft FE_STEP_INV[27] = {
    {0, 0},           {1342177360, 21}, {2147483647, 21}, {1342177360, 20}, {1819901661, 20}, {2147483647, 20},
    {1278263843, 19}, {1579032492, 19}, {1412817763, 18}, {1220162327, 17}, {1118482133, 16}, {1917391412, 16},
    {1766017772, 15}, {1525212826, 14}, {1290553940, 13}, {2097179000, 13}, {1677683200, 12}, {1497972244, 11},
    {1310893147, 10}, {1165354136, 9},  {1748031204, 9},  {1542092044, 8},  {1636178017, 7},  {1636178017, 6},
    {1636178017, 5},  {1636178017, 4},  {1636178017, 3},
};
static const FfSoft FE_SF_INV[128] = {
    {2147483647, 1},  {2147483647, 1},  {2147483647, 2},  {2147483647, 2},  {2147483647, 2},  {2147483647, 2},
    {1431655765, 2},  {1431655765, 2},  {1431655765, 2},  {2147483647, 3},  {2147483647, 3},  {1717986918, 3},
    {1431655765, 3},  {1227133513, 3},  {1227133513, 3},  {2147483647, 4},  {1717986918, 4},  {1561806289, 4},
    {1431655765, 4},  {1227133513, 4},  {2147483647, 5},  {1908874353, 5},  {1717986918, 5},  {1493901668, 5},
    {1321528398, 5},  {1145324612, 5},  {2021161080, 6},  {1808407282, 6},  {1561806289, 6},  {1374389534, 6},
    {1227133513, 6},  {2147483647, 7},  {1908874353, 7},  {1676084798, 7},  {1477838209, 7},  {1296593900, 7},
    {1145324612, 7},  {2021161080, 8},  {1773405851, 8},  {1561806289, 8},  {1374389534, 8},  {1216273924, 8},
    {2139127680, 9},  {1882725390, 9},  {1660893697, 9},  {1462116526, 9},  {1287484341, 9},  {1135859119, 9},
    {1999112050, 10}, {1762037865, 10}, {1552982525, 10}, {1367551775, 10}, {1205604855, 10}, {2124660150, 11},
    {1871509153, 11}, {1648443220, 11}, {1452459217, 11}, {1279990253, 11}, {1127704233, 11}, {1987368509, 12},
    {1750814693, 12}, {1542632939, 12}, {1359099663, 12}, {1197398995, 12}, {2109880792, 13}, {1858853132, 13},
    {1638006149, 13}, {1443165385, 13}, {1271479187, 13}, {1120235993, 13}, {1973767086, 14}, {1739045674, 14},
    {1532153461, 14}, {1349922194, 14}, {1189384493, 14}, {2095804865, 15}, {1846464029, 15}, {1626872524, 15},
    {1433347133, 15}, {1262853884, 15}, {1112619678, 15}, {1960569045, 16}, {1727349015, 16}, {1521881227, 16},
    {1340842289, 16}, {1181357555, 16}, {2081669156, 17}, {1834047752, 17}, {1615889229, 17}, {1423675973, 17},
    {1254322457, 17}, {1105123583, 17}, {1947330755, 18}, {1715693602, 18}, {1511607799, 18}, {1331801790, 18},
    {1173384427, 18}, {2067616532, 19}, {1821667648, 19}, {1604980024, 19}, {1414066955, 19}, {1245861410, 19},
    {1097665748, 19}, {1934193616, 20}, {1704119624, 20}, {1501412075, 20}, {1322817107, 20}, {1165466323, 20},
    {2053666205, 21}, {1809379407, 21}, {1594151671, 21}, {1404526328, 21}, {1237455941, 21}, {1090259329, 21},
    {1921143210, 22}, {1692621231, 22}, {1491281857, 22}, {1313892269, 22}, {1157603482, 22}, {2039810470, 23},
    {1797172644, 23}, {1583396912, 23}, {1395050052, 23}, {1229107276, 23}, {1082903494, 23}, {1082903494, 23},
    {1082903494, 23}, {1082903494, 23},
};
// (Table B.5's bits of a subband's 16 samples at each ABITS, -8 for none: its scale factor's)
static const int FE_BITS[27] = {-8,  28,  40,  48,  52,  60,  68,  76,  80,  96,  112, 128, 144, 160,
                                176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368};
static const int FE_LEVELS[27] = {1,     3,     5,     7,      9,      13,     17,     25,      32,
                                  64,    128,   256,   512,    1024,   2048,   4096,   8192,    16384,
                                  32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};

static INLINE S32 fe_mul32(S32 a, S32 b) { return (S32)(((S64)a * b + ((S64)1 << 31)) >> 32); }
static INLINE S32 fe_norm(S64 a, int bits) { return (S32)((a + ((S64)1 << (bits - 1))) >> bits); }
static INLINE S32 fe_cos(int x) {  // (dcaenc.c's COS_T(): its table's entry x & 2047, from its first 513)
  x &= 2047;
  const int i = x & 1023, s = x >= 1024 ? -1 : 1;
  return s * (i <= 512 ? FE_COS[i] : -FE_COS[1024 - i]);
}
static INLINE S32 fe_quant(S32 v, FfSoft q) { return (S32)(fe_mul32(v, q.m) + (1 << (q.e - 1))) >> q.e; }
static S32 fe_get_cb(S32 in) {
  int res = 0;
  in = in < 0 ? -in : in;
  for (int i = 1024; i > 0; i >>= 1)
    if (FE_CB_LEVEL[i + res] >= in) res += i;
  return -res;
}
static INLINE S32 fe_add_cb(S32 a, S32 b) {
  if (a < b) {
    const S32 t = a;
    a = b, b = t;
  }
  return a - b >= 256 ? a : a + FE_CB_ADD[a - b];
}
#ifdef DTSD_AVX2
// (fe_mul32() on 8 lanes: each product's high half, rounded, as the integers have it)
static INLINE __m256i fe_mul32_8(__m256i a, __m256i b) {
  const __m256i r = _mm256_set1_epi64x((S64)1 << 31);
  const __m256i pe = _mm256_add_epi64(_mm256_mul_epi32(a, b), r);
  const __m256i po = _mm256_add_epi64(_mm256_mul_epi32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32)), r);
  return _mm256_blend_epi32(_mm256_srli_epi64(pe, 32), po, 0xAA);
}
// (fe_add_cb() on 8 lanes: the larger plus tab's entry for the difference, FE_CB_ADD's to 255 and 0
// from 256)
static INLINE __m256i fe_add_cb8(__m256i a, __m256i b, const S32* tab) {
  const __m256i hi = _mm256_max_epi32(a, b);
  const __m256i d = _mm256_min_epi32(_mm256_sub_epi32(hi, _mm256_min_epi32(a, b)), _mm256_set1_epi32(256));
  return _mm256_add_epi32(hi, _mm256_i32gather_epi32(tab, d, 4));
}
#endif

struct FfEnc {
  static constexpr int NB = 32, NS = 16, MAXC = 6, NMG = 11;
  int nch, rate_i, frame_bits, lfe;
  int worst;  // the search's start: the last frame's noise (c->worst_quantization_noise)
  S32 hist[MAXC][512], in[MAXC][512];
  S32 sub[MAXC][NB][NS];
  S32 peak[MAXC][NB], mask[NB];
  int abits[MAXC][NB], sel_a[MAXC];
  double tw_c[128], tw_s[128], fw_c[64], fw_s[64];  // (the MDCT's rotations, its FFT's)
  S32 ct[32][NB];  // (the transform's cosines: acc[16 + i]'s for band b, ct[i][b]; the struct is
                   // malloc()'s, so not 32-byte aligned)
  int aj[3][25][2];            // (the bins where FE_AUF[r][i] is above -1400: from aj[r][i][0] to before [1])
  U32 mo_ok[MAXC][NB];             // (each subband's ABITS from 1 to 10 whose bits mo_bits has, this frame)
  U32 mo_bits[MAXC][NB][10][8];    // (its 16 indexes' bits by each code book of the ABITS's group)
  S32 cb_add[257];                 // (FE_CB_ADD, and 0 at 256: fe_add_cb8()'s)
  S32 jwin[512];                   // (calc_power()'s window)
  int cb_lo[31], cb_hi[31];        // (FE_CB_LEVEL's last entry at 2^(k+1) or more (0 if none), and at 2^k)
#ifdef DTSD_AVX2
  S16 auf_t[3][256][32];           // (FE_AUF by bin, its 25 bands side by side, -3500 past them)
  int aj_all[3][2];                // (the bins any band's sum takes: aj's least and its last)
#endif
  int noise, fz;           // (the frame's, as found, and whether ABITS 0 was forbidden)

  int a_lo[2][27], a_hi[2][27];  // (the SNRs each ABITS takes, with ABITS 0 allowed and not)
  void init() {
    worst = -2047;
    for (int i = 0; i < 32; i++)
      for (int b = 0; b < NB; b++) ct[i][b] = fe_cos(((2 * b + 1) * (2 * (i + 32) + 1)) << 3);
    for (int r = 0; r < 3; r++)
      for (int i = 0; i < 25; i++) {
        int lo = 256, hi = 0;
        for (int j = 0; j < 256; j++)
          if (FE_AUF[r][i][j] > -1400) lo = imin(lo, j), hi = j + 1;
        aj[r][i][0] = lo, aj[r][i][1] = hi;
      }
#ifdef DTSD_AVX2
    for (int r = 0; r < 3; r++) {
      aj_all[r][0] = 256, aj_all[r][1] = 0;
      for (int i = 0; i < 25; i++)
        if (aj[r][i][0] < aj[r][i][1]) aj_all[r][0] = imin(aj_all[r][0], aj[r][i][0]), aj_all[r][1] = imax(aj_all[r][1], aj[r][i][1]);
      for (int j = 0; j < 256; j++)
        for (int i = 0; i < 32; i++) auf_t[r][j][i] = i < 25 ? FE_AUF[r][i][j] : (S16)-3500;
    }
#endif
    for (int d = 0; d < 257; d++) cb_add[d] = d < 256 ? FE_CB_ADD[d] : 0;
    for (int i = 0; i < 512; i++) jwin[i] = 0x3fffffff - (fe_cos(4 * i + 2) >> 1);
    for (int k = 0; k < 31; k++) {
      cb_lo[k] = cb_hi[k] = 0;
      for (int r = 0; r < 2048; r++) {
        if ((S64)FE_CB_LEVEL[r] >= (S64)1 << (k + 1)) cb_lo[k] = r;
        if ((S64)FE_CB_LEVEL[r] >= (S64)1 << k) cb_hi[k] = r;
      }
    }
    for (int n = 0; n < 128; n++) tw_c[n] = FE_MDCT_TW[2 * n] / 2147483648.0, tw_s[n] = FE_MDCT_TW[2 * n + 1] / 2147483648.0;
    for (int k = 0; k < 64; k++) fw_c[k] = fe_cos(16 * k) / 2147483648.0, fw_s[k] = fe_cos(512 - 16 * k) / 2147483648.0;
    for (int f = 0; f < 2; f++) {
      for (int a = 0; a < 27; a++) a_lo[f][a] = 1 << 20, a_hi[f][a] = -(1 << 20);
      for (int sn = -2048; sn <= 1400; sn++) {
        const int a = abits_at(sn, f);
        a_lo[f][a] = imin(a_lo[f][a], sn), a_hi[f][a] = imax(a_hi[f][a], sn);
      }
      a_lo[f][f ? 1 : 0] = -(1 << 20), a_hi[f][26] = 1 << 20;
    }
  }
  // a frame's parameters: its channels, its sample rate's table (FE_AUF's), its bits, its LFE
  void set(int channels, int sri, int fbits, int has_lfe) { nch = channels, rate_i = sri, frame_bits = fbits, lfe = has_lfe; }
  // (init_quantization_noise()'s ABITS at SNR sn, with ABITS 0 forbidden or not)
  static int abits_at(int sn, int fz) {
    return sn >= 1312  ? 26
         : sn >= 222   ? 8 + fe_mul32(sn - 222, 69000000)
         : sn >= 0     ? 2 + fe_mul32(sn, 106000000)
         : fz || sn >= -140 ? 1
                            : 0;
  }
  // channel ch's subband b's ABITS: how near its SNR lies its range's nearer end (a bucket: 0, 1,
  // 2, 3, 4-5, 6-7, 8-11, 12-15, 16-23, 24-31, more), and that end's side (*dir 1 above, -1 below)
  int margin(int ch, int b, int* dir) const {
    const int a = abits[ch][b], sn = peak[ch][b] - mask[b] - noise;
    const int dl = sn - a_lo[fz][a], dh = a_hi[fz][a] - sn, m = imin(dl, dh);
    *dir = dh < dl ? 1 : -1;
    return m < 4 ? m : m < 8 ? 4 + (m >= 6) : m < 16 ? 6 + (m >= 12) : m < 32 ? 8 + (m >= 24) : 10;
  }
  // (fe_get_cb(): the last entry at |in| or more, searched for among those of its binade alone,
  // from the last one at twice its binade's least on (FE_CB_LEVEL falls by 2^(1/200) an entry: some
  // 60 in a binade, under 64))
  S32 get_cb(S32 in) const {
    const S32 v = in < 0 ? (S32)(0u - (U32)in) : in;  // (-2^31 stays so: every entry at it or more, as 0)
    if (v <= 0) return -2047;
    const U32 a = (U32)v;
    const int k = 31 - __builtin_clz(a), hi = cb_hi[k];
    int res = cb_lo[k];
    for (int i = 32; i > 0; i >>= 1)
      if (res + i <= hi && FE_CB_LEVEL[res + i] >= (S32)a) res += i;
    return -res;
  }
  // (subband_transform())
  // (its window's 512 samples from hs on, circularly: here twice over in h, so that they lie in a
  // row; the sums in 32 bits, wrapping, in any order: the same integers)
  void transform() {
    for (int ch = 0; ch < nch; ch++) {
      alignas(32) S32 h[1024];
      memcpy(h, hist[ch], 512 * sizeof(S32)), memcpy(h + 512, hist[ch], 512 * sizeof(S32));
      int hs = 0;
      for (int s = 0; s < NS; s++) {
        alignas(32) S32 acc[64], r[NB];
#ifdef DTSD_AVX2
        for (int k = 0; k < 64; k += 8) {
          __m256i a = _mm256_setzero_si256();
          for (int m = 0; m < 8; m++)
            a = _mm256_add_epi32(a, fe_mul32_8(_mm256_loadu_si256((const __m256i*)(h + hs + k + 64 * m)),
                                               _mm256_loadu_si256((const __m256i*)(FE_INTERP + k + 64 * m))));
          _mm256_store_si256((__m256i*)(acc + k), a);
        }
#else
        for (int k = 0; k < 64; k++) {
          U32 a = 0;
          for (int m = 0; m < 8; m++) a += (U32)fe_mul32(h[hs + k + 64 * m], FE_INTERP[k + 64 * m]);
          acc[k] = (S32)a;
        }
#endif
        for (int k = 16; k < 32; k++) acc[k] = (S32)((U32)acc[k] - (U32)acc[31 - k]);
        for (int k = 32; k < 48; k++) acc[k] = (S32)((U32)acc[k] + (U32)acc[95 - k]);
#ifdef DTSD_AVX2
        for (int b = 0; b < NB; b += 8) {
          __m256i a = _mm256_setzero_si256();
          for (int i = 0; i < 32; i++)
            a = _mm256_add_epi32(a, _mm256_srai_epi32(fe_mul32_8(_mm256_set1_epi32(acc[16 + i]),
                                                                 _mm256_loadu_si256((const __m256i*)(ct[i] + b))), 3));
          _mm256_store_si256((__m256i*)(r + b), a);
        }
#else
        for (int b = 0; b < NB; b++) {
          U32 a = 0;
          for (int i = 0; i < 32; i++) a += (U32)(fe_mul32(acc[16 + i], ct[i][b]) >> 3);
          r[b] = (S32)a;
        }
#endif
        for (int b = 0; b < NB; b++) sub[ch][b][s] = ((b + 1) & 2) ? (S32)(0u - (U32)r[b]) : r[b];
        for (int i = 0; i < 32; i++) h[i + hs] = h[i + hs + 512] = in[ch][s * 32 + i];
        hs = (hs + 32) & 511;
      }
    }
  }
  // a 128-point FFT (e^-2 pi i n k / 128), in place
  void fft128(double* re, double* im) const {
    for (int i = 1, j = 0; i < 128; i++) {
      int bit = 64;
      for (; j & bit; bit >>= 1) j ^= bit;
      j |= bit;
      if (i < j) {
        double t = re[i];
        re[i] = re[j], re[j] = t, t = im[i], im[i] = im[j], im[j] = t;
      }
    }
    for (int len = 2; len <= 128; len <<= 1)
      for (int i = 0; i < 128; i += len)
        for (int k = 0; k < len / 2; k++) {
          const double c = fw_c[k * (128 / len)], s = fw_s[k * (128 / len)];
          const int a = i + k, b = a + len / 2;
          const double xr = re[b] * c + im[b] * s, xi = im[b] * c - re[b] * s;
          re[b] = re[a] - xr, im[b] = im[a] - xi;
          re[a] += xr, im[a] += xi;
        }
  }
  // the MDCT of d's 512 values (X[i] the sum of d[j] cos(pi (2 j + 1 + 256)(2 i + 1) / 1024)): the
  // DCT-IV of their fold, by the FFT of its values in pairs
  void mdct(const S32* d, double* X) const {
    double r[256], zr[128], zi[128];
    for (int n = 0; n < 128; n++) r[n] = -(double)d[383 - n] - (double)d[384 + n];
    for (int n = 128; n < 256; n++) r[n] = (double)d[n - 128] - (double)d[383 - n];
    for (int n = 0; n < 128; n++) {
      const double a = r[2 * n], b = r[255 - 2 * n], c = tw_c[n], s = tw_s[n];
      zr[n] = a * c + b * s, zi[n] = b * c - a * s;
    }
    fft128(zr, zi);
    for (int k = 0; k < 128; k++) {
      const double c = tw_c[k], s = tw_s[k];
      X[2 * k] = zr[k] * c + zi[k] * s;
      X[255 - 2 * k] = zr[k] * s - zi[k] * c;
    }
  }
  // (calc_power(), the MDCT in doubles (FFmpeg's int32 one's a 64th of it), adjust_jnd())
  void jnd(const S32* x, S32* out) {
    S32 d[512], power[256], un[256];
    double X[256];
    for (int i = 0; i < 512; i++) d[i] = fe_norm(fe_mul32(x[i], jwin[i]), 4);
    mdct(d, X);
    for (int i = 0; i < 256; i++) {
      const S32 cb = get_cb((S32)floor(X[i] / 64 + 0.5));
      power[i] = fe_add_cb(cb, cb);
    }
    for (int j = 0; j < 256; j++) un[j] = -2047;
    const S16(*auf)[256] = FE_AUF[rate_i];
    // (den starts at -1114 and never falls, and a power is at most 30 (a centibel value's double):
    // a bin whose FE_AUF entry is -1400 or less adds nothing to it, so the sum goes over the others;
    // with AVX2 the 25 bands' sums side by side, 8 to a vector, each over the bins any band's takes)
#ifdef DTSD_AVX2
    alignas(32) S32 dens[32];
    {
      const S16(*at)[32] = auf_t[rate_i];
      __m256i d0 = _mm256_set1_epi32(-1114), d1 = d0, d2 = d0, d3 = d0;
      for (int j = aj_all[rate_i][0]; j < aj_all[rate_i][1]; j++) {
        const __m256i pw = _mm256_set1_epi32(power[j]);
        const __m128i* const r = (const __m128i*)at[j];
        d0 = fe_add_cb8(d0, _mm256_add_epi32(pw, _mm256_cvtepi16_epi32(_mm_loadu_si128(r))), cb_add);
        d1 = fe_add_cb8(d1, _mm256_add_epi32(pw, _mm256_cvtepi16_epi32(_mm_loadu_si128(r + 1))), cb_add);
        d2 = fe_add_cb8(d2, _mm256_add_epi32(pw, _mm256_cvtepi16_epi32(_mm_loadu_si128(r + 2))), cb_add);
        d3 = fe_add_cb8(d3, _mm256_add_epi32(pw, _mm256_cvtepi16_epi32(_mm_loadu_si128(r + 3))), cb_add);
      }
      _mm256_store_si256((__m256i*)dens, d0), _mm256_store_si256((__m256i*)(dens + 8), d1);
      _mm256_store_si256((__m256i*)(dens + 16), d2), _mm256_store_si256((__m256i*)(dens + 24), d3);
    }
#endif
    for (int i = 0; i < 25; i++) {
#ifdef DTSD_AVX2
      const __m256i nd = _mm256_set1_epi32(-dens[i]);
      for (int j = 0; j < 256; j += 8) {
        const __m256i t = _mm256_add_epi32(nd, _mm256_cvtepi16_epi32(_mm_loadu_si128((const __m128i*)(auf[i] + j))));
        _mm256_storeu_si256((__m256i*)(un + j), fe_add_cb8(_mm256_loadu_si256((const __m256i*)(un + j)), t, cb_add));
      }
#else
      S32 den = -1114;
      for (int j = aj[rate_i][i][0]; j < aj[rate_i][i][1]; j++) den = fe_add_cb(den, power[j] + auf[i][j]);
      for (int j = 0; j < 256; j++) un[j] = fe_add_cb(un[j], -den + auf[i][j]);
#endif
    }
    for (int j = 0; j < 256; j++) out[j] = fe_add_cb(out[j], -un[j] + 1114 - 928);
  }
  // (calc_masking(): the bands' masking, from the two half frames' curves, the least of them)
  void masking() {
    S32 mc[2][256], eff[256];
    for (int s = 0; s < 2; s++)
      for (int i = 0; i < 256; i++) mc[s][i] = -2047;
    for (int s = 0; s < 2; s++)
      for (int ch = 0; ch < nch; ch++) {
        S32 d[512];
        int i = 0, k = 128 + 256 * s;
        for (; k < 512; i++, k++) d[i] = hist[ch][k];
        for (k -= 512; i < 512; i++, k++) d[i] = in[ch][k];
        jnd(d, mc[s]);
      }
    for (int i = 0; i < 256; i++) eff[i] = imin(imin(mc[0][i], mc[1][i]), 2048);
    for (int b = 0; b < NB; b++) {
      S32 m = 2048;
      if (b == 0) {
        for (int f = 0; f < 4; f++) m = imin(m, eff[f] - 0);
      } else {
        for (int f = 0; f < 8; f++) m = imin(m, eff[8 * b - 4 + f] - FE_SPECTRUM[7 - f]);
      }
      if (b == 31) {
        for (int f = 0; f < 4; f++) m = imin(m, eff[252 + f] - 0);
      } else {
        for (int f = 0; f < 8; f++) m = imin(m, eff[8 * b + 4 + f] - FE_SPECTRUM[f]);
      }
      mask[b] = m;
    }
  }
  void peaks() {
    for (int ch = 0; ch < nch; ch++)
      for (int b = 0; b < NB; b++) {
        S32 m = 0;
        for (int s = 0; s < NS; s++) {
          const S32 v = sub[ch][b][s] < 0 ? -sub[ch][b][s] : sub[ch][b][s];
          m = imax(m, v);
        }
        peak[ch][b] = get_cb(m);
      }
  }
  // (calc_one_scale())
  static int one_scale(S32 peak_cb, int a, FfSoft* qq) {
    const S32 pk = FE_CB_LEVEL[-peak_cb];
    int ns = 127;
    for (int t = 64; t > 0; t >>= 1) {
      if (FE_SF_INV[ns - t].e + FE_STEP_INV[a].e <= 17) continue;
      FfSoft o{fe_mul32(FE_SF_INV[ns - t].m, FE_STEP_INV[a].m), FE_SF_INV[ns - t].e + FE_STEP_INV[a].e - 17};
      if ((FE_LEVELS[a] - 1) / 2 < fe_quant(pk, o)) continue;
      ns -= t;
    }
    if (ns >= 125) ns = 124;
    *qq = FfSoft{fe_mul32(FE_SF_INV[ns].m, FE_STEP_INV[a].m), FE_SF_INV[ns].e + FE_STEP_INV[a].e - 17};
    return ns;
  }
  // the least ABITS from 1 to 8 whose quantizer's lattice channel ch's subband b's values fit (0
  // none): the decode's values are indexes times the step, and at ABITS 1-6 the peak's index the
  // quantizer's top level (the least scale factor that keeps the encoder's peak at that level puts
  // it there), so that the values are multiples of the peak over that level; where they are, within
  // 0.08 of a step at the root of their mean square. The decoded peak is the index times the step,
  // under the encoder's (by 30 centibels at ABITS 1, 14 at 2, 8 at 3), so that the ABITS made from
  // it are often one under the core's; the lattice tells them apart: on FFmpeg's decode of its
  // encode of `s.2.dts`'s first 3 MB, with the side of the ABITS's range its SNR is nearer, the
  // ABITS 39% smaller, `open bitrate.dts`'s encode's 19% (its values on its own input's lattice)
  int lattice(int ch, int b) const {
    static const int LS[9] = {0, 1, 2, 3, 4, 6, 8, 12, 15};  // ((FE_LEVELS[a] - 1) / 2)
    S64 m = 0;
    for (int s = 0; s < NS; s++) {
      const S64 x = sub[ch][b][s] < 0 ? -(S64)sub[ch][b][s] : sub[ch][b][s];
      if (x > m) m = x;
    }
    if (!m) return 0;
    for (int a = 1; a <= 8; a++) {
      S128 e = 0;
      for (int s = 0; s < NS; s++) {
        const S64 x = (sub[ch][b][s] < 0 ? -(S64)sub[ch][b][s] : sub[ch][b][s]) * LS[a] % m, d = x < m - x ? x : m - x;
        e += (S128)d * d;
      }
      if (625 * e <= 64 * (S128)m * m) return a;  // (16 times 0.08 squared: 64/625)
    }
    return 0;
  }
  // the least scale factor whose step at ABITS a puts channel ch's subband b's peak within k
  // 32nds of the quantizer's top level (as one_scale() searches; the decode's peak is an index
  // times the encoder's step, the index at most that level, and the encoder's scale factor the
  // least that keeps the peak there: on FFmpeg's decode of its encode of `s.2.dts`'s first 3 MB,
  // at 3/32 the core's in 95.9% of its scale factors, one_scale()'s 74.0%; k under 4, as the
  // table's entries are 1.124 times the last, and at ABITS 1 the peak is the step)
  int scale_within(int ch, int b, int a, int k) const {
    S64 m = 0;
    for (int s = 0; s < NS; s++) {
      const S64 x = sub[ch][b][s] < 0 ? -(S64)sub[ch][b][s] : sub[ch][b][s];
      if (x > m) m = x;
    }
    return scale_within_peak(m, a, k);
  }
  // (the same of a peak m)
  static int scale_within_peak(S64 m, int a, int k) {
    const S64 top = 32 * (S64)((FE_LEVELS[a] - 1) / 2) + k;
    int ns = 127;
    for (int t = 64; t > 0; t >>= 1) {
      const int e = FE_SF_INV[ns - t].e + FE_STEP_INV[a].e - 17;
      if (e <= 0) continue;
      const S64 om = fe_mul32(FE_SF_INV[ns - t].m, FE_STEP_INV[a].m);
      if ((S128)m * om * 32 > ((S128)top << (32 + e))) continue;
      ns -= t;
    }
    return imin(ns, 124);
  }
  // (init_quantization_noise(): the ABITS at noise n, the bits they take; USED_1ABITS 1, USED_26ABITS 4)
  int consumed;
  int try_noise(int n, int fz) {
    int ret = 4 | 1;
    consumed = 132 + 333 * nch + (lfe ? 72 : 0);
    for (int ch = 0; ch < nch; ch++) {
      for (int b = 0; b < NB; b++) {
        const int s = peak[ch][b] - mask[b] - n, a = abits_at(s, fz);
        ret &= a == 26 ? ~1 : a == 1 && s < 0 ? ~4 : ~(4 | 1);
        abits[ch][b] = a;
      }
      // (set_best_abits_code())
      int bs = 6, bb = NB * 5;
      bool huff = true;
      for (int b = 0; b < NB; b++) huff &= abits[ch][b] >= 1 && abits[ch][b] <= 12;
      if (huff)
        for (int k = 0; k < 5; k++) {
          int t = 0;
          for (int b = 0; b < NB; b++) t += BA[k].bits(abits[ch][b]);
          if (t < bb) bb = t, bs = k;
        }
      sel_a[ch] = bs;
      consumed += bb;
    }
    // (each subband's indexes' bits at each ABITS once a frame: they go by its peak and ABITS alone)
    U32 vc[MAXC][10][7], cc[MAXC][10];
    memset(vc, 0, sizeof vc), memset(cc, 0, sizeof cc);
    int bits = 0;
    for (int ch = 0; ch < nch; ch++)
      for (int b = 0; b < NB; b++) {
        const int a = abits[ch][b];
        if (a && a <= 10) {
          U32* const mb = mo_bits[ch][b][a - 1];
          if (!(mo_ok[ch][b] >> a & 1)) {
            FfSoft qq;
            one_scale(peak[ch][b], a, &qq);
            S32 qv[NS];
            for (int s = 0; s < NS; s++) qv[s] = fe_quant(sub[ch][b][s], qq);
            for (int t = 0; t < QB_GROUP[a - 1]; t++) {
              U32 x = 0;
              for (int s = 0; s < NS; s++) x += (U32)QB[a - 1][t].bits(qv[s]);
              mb[t] = x;
            }
            mo_ok[ch][b] |= 1u << a;
          }
          for (int t = 0; t < QB_GROUP[a - 1]; t++) vc[ch][a - 1][t] += mb[t];
          cc[ch][a - 1] += (U32)FE_BITS[a];
        } else
          bits += FE_BITS[a];
      }
    // (set_best_code())
    for (int ch = 0; ch < nch; ch++)
      for (int i = 0; i < 10; i++) {
        if (!vc[ch][i][0]) continue;
        U32 best = vc[ch][i][0];
        for (int t = 0; t < QB_GROUP[i]; t++)
          if (best > vc[ch][i][t] && vc[ch][i][t]) best = vc[ch][i][t];
        bits += best + 2 < cc[ch][i] ? (int)best + 2 : (int)cc[ch][i];
      }
    consumed += bits;
    return ret;
  }
  // (assign_bits(): the least noise whose bits fit the frame's)
  void assign() {
    int low, high, used = 0, fz = 1;
  restart:
    try_noise(worst, fz);
    low = high = worst;
    if (consumed > frame_bits) {
      while (consumed > frame_bits) {
        if (used == 1 && fz) {
          fz = 0;
          goto restart;
        }
        low = high;
        high += 128;
        used = try_noise(high, fz);
      }
    } else {
      while (consumed <= frame_bits) {
        high = low;
        if (used == 4) goto out;
        low -= 128;
        used = try_noise(low, fz);
      }
    }
    for (int down = 64; down; down >>= 1) {
      try_noise(high - down, fz);
      if (consumed <= frame_bits) high -= down;
    }
    try_noise(high, fz);
  out:
    worst = high;
    noise = high, this->fz = fz;
  }
  // a frame: in[] its input, hist[] the 512 samples before
  void frame() {
    transform();
    masking();
    peaks();
    memset(mo_ok, 0, sizeof mo_ok);
    assign();
  }
};
