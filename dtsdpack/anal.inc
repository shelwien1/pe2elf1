// anal.inc -- what the WAV says of the core: the subband samples of its audio by the encoder's
// analysis filterbank (in integers: c and d compute them alike everywhere), the indexes they
// quantize to as the core's step sizes, scale factors and ADPCM predictions say, the scale factors
// their RMS makes, and how the core's channels and blocks line up with the WAV's (c finds it, the
// header keeps it).

constexpr int AQ_F = 16;  // the analysis's subband samples: in 2^-AQ_F of the decoder's units
constexpr int QB_N = 16;  // the buckets of how near the rounding boundary a sample lies (near_bucket())
constexpr int SB_N = 8;   // and a scale factor's RMS the midpoint between two table entries

#include "tables.inc"
constexpr int SF7_N = 125, SF6_N = 63;  // the scale factor tables' entries (FFmpeg pads them with zeros)
// the scale factors a channel's SHUFF has: their table's entries the RMS rule picks from
static INLINE int sf_entries(int shuff) { return shuff == 6 ? SF7_N : SF6_N; }
// where the run of scale factor i's value in table tab (nt entries) ends: 0 none (no other entry
// has it); 1, 2 one entry below, above (a run of 2, i its last, its first); 3 both ways (i inside
// the run); 4, 5 two below, above (a run of 3); 6, 7 three or more. The RMS rule takes a run's
// first for an RMS below its value and its last from the value on, and so does the encoder; where
// its arithmetic moves the RMS across the value, its scale factor is the run's other end (`s.2.dts`'s
// HF VQ subbands', quiet, mostly there: 43% of the rule's misses)
static INLINE int sf_run(const U32* tab, int nt, int i) {
  int lo = i, hi = i;
  while (lo > 0 && tab[lo - 1] == tab[i]) lo--;
  while (hi + 1 < nt && tab[hi + 1] == tab[i]) hi++;
  if (lo == hi) return 0;
  if (lo < i && i < hi) return 3;
  const int k = hi - lo < 3 ? hi - lo : 3;
  return k == 1 ? (lo < i ? 1 : 2) : 2 * k + (lo < i ? 0 : 1);
}

static INLINE S32 clip23(S64 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : (S32)a; }
static INLINE S64 rshift(S64 a, int bits) { return bits > 0 ? (a + (1LL << (bits - 1))) >> bits : a; }
typedef __int128 S128;
typedef unsigned __int128 U128;
// x times 2^k (C++11 leaves the left shift of a negative value undefined)
static INLINE S64 shl64(S64 x, int k) { return (S64)((U64)x << k); }
static INLINE S128 shl128(S128 x, int k) { return (S128)((U128)x << k); }
// the floor square root of t (IEEE's sqrt the start, the integer steps exact)
static INLINE S64 isqrt128(U128 t) {
  S64 q = (S64)sqrt((double)t);
  while (q > 0 && (U128)((S128)q * q) > t) q--;
  while ((U128)((S128)(q + 1) * (q + 1)) <= t) q++;
  return q;
}

// How the core's channels and blocks line up with the WAV's: the analysis of block t (the core's
// t-th subband sample since the stream's first frame) ends before sample 32 t + off of the WAV, and
// map[ch] is the WAV's channel of core channel ch (-1 none, as for every channel without a WAV)
// How the core lines up with the WAV: the offset, and each core channel as the WAV's channel map
// times g1 plus channel map2 times g2 (Q23; map2 -1: map's alone): a downmix, as a 7.1 stream's
// core surrounds are (each its side and rear surround, all the core's channels at -3 dB), or
// 6.1's (the surround and the back centre at -3 dB). up: the WAV's samples to the core's (1, or 2
// for a WAV at twice its rate, whose 64-band analysis's lower half is the core: a 96 kHz stream's)
constexpr S32 BIND_ONE = 1 << 23;
struct Bind {
  S32 off;
  S8 up, dec;  // (the WAV's samples to the core's for its analysis, 1 or 2; to those analysed, 1 or 2)
  S8 dsh;      // (dec 2: the stream's resolution, the WAV's samples' 0 bits at the bottom: wav.inc)
  S8 map[NCH];   // each core channel's WAV channel (-1 none)
  S8 map2[NCH];  // the second of its mix (-1 none)
  S32 g1[NCH], g2[NCH];  // their gains (Q23)
  S8 lfe;        // the LFE's (-1 none)
  void none() {
    off = 0;
    up = dec = 1;
    dsh = 0;
    for (int ch = 0; ch < NCH; ch++) map[ch] = map2[ch] = -1, g1[ch] = BIND_ONE, g2[ch] = 0;
    lfe = -1;
  }
  // whether core channel ch is bound to the WAV's (nw) channels
  bool bound(int ch, int nw) const { return map[ch] >= 0 && map[ch] < nw && map2[ch] < nw; }
};

// The first nb subband samples of an M-band analysis of the 16 M samples of WAV channel c before
// sample o, as the encoder's makes them (FFmpeg dcaenc's transform, by the prototype filter fir),
// in 2^-AQ_F of the decoder's units (its gain, 2, in): x[nb]. (M 32 the core's, by its prototype;
// 64 the 64-band filterbank's, X96's, whose lower 32 bands are a 96 kHz stream's core.)
// (the prototype filters' and the cosines' halves, for analyze_m()'s 64-bit sums: f = h 2^20 + l,
// c = h 2^31 + l, l from 0)
struct SplitTabs {
  S32 fh[3][1024], fl[3][1024];  // FIR_NONPERFECT, FIR_PERFECT, FIR_64
  S32 ch32[32][32], cl32[32][32], ch64[64][64], cl64[64][64];
  SplitTabs() {
    const S64* const f[3] = {FIR_NONPERFECT, FIR_PERFECT, FIR_64};
    for (int t = 0; t < 3; t++)
      for (int m = 0; m < (t == 2 ? 1024 : 512); m++) fh[t][m] = (S32)(f[t][m] >> 20), fl[t][m] = (S32)(f[t][m] & 0xFFFFF);
    for (int b = 0; b < 32; b++)
      for (int i = 0; i < 32; i++) ch32[b][i] = (S32)(COSQ[b][i] >> 31), cl32[b][i] = (S32)(COSQ[b][i] & 0x7FFFFFFF);
    for (int b = 0; b < 64; b++)
      for (int i = 0; i < 64; i++) ch64[b][i] = (S32)(COSQ64[b][i] >> 31), cl64[b][i] = (S32)(COSQ64[b][i] & 0x7FFFFFFF);
  }
};
static const SplitTabs split_tabs;

// (the cosines in doubles, rounded, by sample then band: cd32[i][b] COSQ[b][i]; and each band's sum
// of |cos| as doubles, a little up: analyze_sp()'s screen; made where first needed)
struct CosD {
  alignas(32) double cd32[32][32], cd64[64][64];
  double cs32[32], cs64[64];
  bool made[2];
  template <int M>
  void make(const S64 (*cq)[M], double (*cd)[M], double* cs) {
    for (int b = 0; b < M; b++) {
      double t = 0;
      for (int i = 0; i < M; i++) cd[i][b] = (double)cq[b][i], t += fabs((double)cq[b][i]);
      cs[b] = t * (1 + 1e-12);
    }
  }
};
static CosD cos_d;
template <int M>
static void analyze_sp(const S32* sp, const S64* fir, const S64 (*cq)[M], S64* x, int nb);
template <int M>
static void analyze_m(const WavIn& w, int c, S64 o, const S64* fir, const S64 (*cq)[M], S64* x, int nb) {
  constexpr int L = 16 * M;
  S32 buf[L];
  analyze_sp<M>(w.span(c, o - L, L, buf), fir, cq, x, nb);
}
// (the same of the 16 M samples sp)
template <int M>
static void analyze_sp(const S32* sp, const S64* fir, const S64 (*cq)[M], S64* x, int nb) {
  constexpr int L = 16 * M, H = M / 2;
  S32 mx = 0;
  for (int m = 0; m < L; m++) mx |= sp[m] < 0 ? ~sp[m] : sp[m];
  S64 a[M];
  if (mx < (1 << 24)) {  // (samples of 25 bits at most: every sum in 64 bits, the halves' products, the same;
    // the sums s h 2^20 + s l folded as their halves, then floored: s h + (s l >> 20), within 2^48)
    const int t = fir == FIR_64 ? 2 : fir == FIR_PERFECT ? 1 : 0;
    const S32 *fh = split_tabs.fh[t], *fl = split_tabs.fl[t];
    S64 sh[2 * M], sl[2 * M];
    for (int i = 0; i < 2 * M; i++) sh[i] = sl[i] = 0;
    for (int k = 0; k < L; k += 2 * M)
      for (int i = 0; i < 2 * M; i++) sh[i] += (S64)sp[k + i] * fh[k + i], sl[i] += (S64)sp[k + i] * fl[k + i];
    for (int k = H; k < M; k++) sh[k] -= sh[M - 1 - k], sl[k] -= sl[M - 1 - k];
    for (int k = M; k < M + H; k++) sh[k] += sh[3 * M - 1 - k], sl[k] += sl[3 * M - 1 - k];
    for (int i = 0; i < M; i++) a[i] = sh[i + H] + (sl[i + H] >> 20);
  } else {
    S128 acc[2 * M];
    for (int i = 0; i < 2 * M; i++) acc[i] = 0;
    for (int m = 0; m < L; m++) acc[m & (2 * M - 1)] += (S128)sp[m] * fir[m];
    for (int k = H; k < M; k++) acc[k] -= acc[M - 1 - k];
    for (int k = M; k < M + H; k++) acc[k] += acc[3 * M - 1 - k];
    for (int i = 0; i < M; i++) a[i] = (S64)(acc[i + H] >> 20);
  }
  S64 amax = 0;
  for (int i = 0; i < M; i++) amax |= a[i] < 0 ? ~a[i] : a[i];  // (2^-24 of a sample)
  if (amax < ((S64)1 << 46)) {  // (a = h 2^23 + l: the products in 56 bits, 64 of them in 62)
    const S32(*chp)[M] = (const S32(*)[M])(M == 64 ? &split_tabs.ch64[0][0] : &split_tabs.ch32[0][0]);
    const S32(*clp)[M] = (const S32(*)[M])(M == 64 ? &split_tabs.cl64[0][0] : &split_tabs.cl32[0][0]);
    S32 ah[M], al[M];
    bool split = false;
    auto exact = [&](int b) {  // (band b's sum exactly, in halves)
      if (!split) {
        for (int i = 0; i < M; i++) ah[i] = (S32)(a[i] >> 23), al[i] = (S32)(a[i] & 0x7FFFFF);
        split = true;
      }
      S64 hh = 0, hl = 0, lh = 0, ll = 0;
      for (int i = 0; i < M; i++) {
        hh += (S64)ah[i] * chp[b][i], hl += (S64)ah[i] * clp[b][i];
        lh += (S64)al[i] * chp[b][i], ll += (S64)al[i] * clp[b][i];
      }
      const S128 r = (S128)hh * ((S64)1 << 54) + (S128)hl * (1 << 23) + (S128)lh * ((S64)1 << 31) + ll;
      return (S64)((r + ((S128)1 << (84 - AQ_F))) >> (85 - AQ_F));  // (2 r: 2^-(24 + 62) to 2^-AQ_F)
    };
    // (first in doubles: the sum's t = r / 2^(85 - AQ_F) within (M + 4) 2^-53 of the sum of
    // |a| |cos| (the cosines' rounding and the sums', Higham's dot product bound, FMA's or not),
    // max |a| times the band's sum of |cos| at most; where t + 1/2 lies past that and 2^-12 (its
    // own rounding, |t| < 2^40) from an integer, its floor is the exact sum's; else exactly)
    const int mi = M == 64;
    if (!cos_d.made[mi]) {
      if (M == 64) cos_d.make<64>(COSQ64, cos_d.cd64, cos_d.cs64);
      else cos_d.make<32>(COSQ, cos_d.cd32, cos_d.cs32);
      cos_d.made[mi] = true;
    }
    const double(*cd)[M] = (const double(*)[M])(M == 64 ? &cos_d.cd64[0][0] : &cos_d.cd32[0][0]);
    const double* const cs = M == 64 ? cos_d.cs64 : cos_d.cs32;
    const double SC = 1.0 / (double)((U64)1 << 35) / (double)((U64)1 << (85 - AQ_F - 35));  // (2^-(85 - AQ_F), exactly)
    const double U = 1.0 / 9007199254740992.0, E12 = 1.0 / 4096.0, T40 = 1099511627776.0;  // (2^-53, 2^-12, 2^40)
    const double ek = (M + 4) * U * ((double)amax + 1) * SC * (1 + 1e-12);
    alignas(32) double xr[M], ad[M];
    for (int i = 0; i < M; i++) ad[i] = (double)a[i];
#ifdef DTSD_AVX2  // (16 bands at a time, in registers)
    for (int b0 = 0; b0 < nb; b0 += 16) {
      __m256d s0 = _mm256_setzero_pd(), s1 = s0, s2 = s0, s3 = s0;
      for (int i = 0; i < M; i++) {
        const __m256d ai = _mm256_set1_pd(ad[i]);
        const double* const ci = cd[i] + b0;
        s0 = _mm256_fmadd_pd(ai, _mm256_load_pd(ci), s0), s1 = _mm256_fmadd_pd(ai, _mm256_load_pd(ci + 4), s1);
        s2 = _mm256_fmadd_pd(ai, _mm256_load_pd(ci + 8), s2), s3 = _mm256_fmadd_pd(ai, _mm256_load_pd(ci + 12), s3);
      }
      _mm256_store_pd(xr + b0, s0), _mm256_store_pd(xr + b0 + 4, s1);
      _mm256_store_pd(xr + b0 + 8, s2), _mm256_store_pd(xr + b0 + 12, s3);
    }
#else
    for (int b = 0; b < nb; b++) {
      double t = 0;
      for (int i = 0; i < M; i++) t += ad[i] * cd[i][b];
      xr[b] = t;
    }
#endif
    for (int b = 0; b < nb; b++) {
      const double t = xr[b] * SC, et = ek * cs[b] + E12;
      if (t < T40 && t > -T40) {
        const double u = t + 0.5, f = floor(u);
        if (u - f > et && f + 1 - u > et) {
          x[b] = (S64)f;
          continue;
        }
      }
      x[b] = exact(b);
    }
    return;
  }
  for (int b = 0; b < nb; b++) {
    S128 r = 0;
    for (int i = 0; i < M; i++) r += (S128)a[i] * cq[b][i];
    x[b] = (S64)((r + ((S128)1 << (84 - AQ_F))) >> (85 - AQ_F));  // (2 r: 2^-(24 + 62) to 2^-AQ_F)
  }
}
// the analysis of the WAV's samples to the core's up: 32 bands by the core's prototype fir (up
// 1), 64 by the 64-band one (2: a WAV at twice the core's rate); its first nb bands into x
static void analyze_raw(const WavIn& w, int c, S64 o, int up, const S64* fir, S64* x, int nb) {
  if (up == 2) analyze_m<64>(w, c, o, FIR_64, COSQ64, x, nb);
  else analyze_m<32>(w, c, o, fir, COSQ, x, nb);
}
static void analyze(const WavIn& w, int c, S64 o, int up, const S64* fir, S64* x, int nb = 32) {
  if (w.ana_n && up == w.ana_up && fir == w.ana_fir && nb <= 32 && o <= w.ana_got && c < w.nch) {  // (made already)
    const S64 d = o - w.ana_off, st = 32 * (S64)up;
    if (d >= 0 && d % st == 0) {
      const S64 t = d / st;
      int lo = 0, hi = w.ana_n - 1;
      while (lo < hi) {
        const int m = (lo + hi) >> 1;
        if (w.ana_t[m] < t) lo = m + 1;
        else hi = m;
      }
      if (w.ana_t[lo] == t) {
        memcpy(x, w.ana + ((size_t)lo * w.nch + c) * 32, sizeof(S64) * nb);
        return;
      }
    }
  }
  analyze_raw(w, c, o, up, fir, x, nb);
}

// the first nb subband samples of core channel ch as the binding b makes them from the WAV
// (analyze()'s): its channel's, or the mix's; a lossless WAV's (pcm) as its encoder made them: at
// the core's rate, mixed in the PCM, each sample's sum rounded to the nearest, a half up, then
// analyzed; at twice it (without X96: the caller's), by a 64-band analysis 1 - 1.4e-4 times this one
static void analyze_bound(const WavIn& w, const Bind& b, int ch, S64 o, const S64* fir, S64* x, int nb = 32, bool pcm = false) {
  if (pcm && b.up == 1 && !(b.map2[ch] < 0 && b.g1[ch] == BIND_ONE)) {
    constexpr int L = 512;
    S32 b1[L], b2[L], m[L];
    const S32* s1 = w.span(b.map[ch], o - L, L, b1);
    const S32* s2 = b.map2[ch] >= 0 ? w.span(b.map2[ch], o - L, L, b2) : nullptr;
    for (int i = 0; i < L; i++)
      m[i] = (S32)(((S64)s1[i] * b.g1[ch] + (s2 ? (S64)s2[i] * b.g2[ch] : 0) + (1 << 22)) >> 23);
    analyze_sp<32>(m, fir, COSQ, x, nb);
    return;
  }
  analyze(w, b.map[ch], o, b.up, fir, x, nb);
  if (!(b.map2[ch] < 0 && b.g1[ch] == BIND_ONE)) {
    S64 x2[64];
    if (b.map2[ch] >= 0) analyze(w, b.map2[ch], o, b.up, fir, x2, nb);
    for (int k = 0; k < nb; k++) {
      const S128 v = (S128)x[k] * b.g1[ch] + (b.map2[ch] >= 0 ? (S128)x2[k] * b.g2[ch] : 0);
      x[k] = (S64)((v + (1 << 22)) >> 23);
    }
  }
  // (the 64-band analysis by FFmpeg's prototype makes the encoder's index 1.66% of the time, 0.35%
  // at 1 - 1.4e-4 times it, nearer at no other gain: 150324 2^-30 of it off)
  if (pcm && b.up == 2)
    for (int k = 0; k < nb; k++) x[k] -= rshift(x[k] * 150324, 30);
}
// a gain (as a fit makes it) as the downmix table's entries either side of it (Q23; a negative
// gain the entries' negatives: the coefficients have a sign), where within 2% of it, the nearer
// first: their number (0-2) into out
static int gain_near(double g, S32* out) {
  const double a = fabs(g) * 32768;
  int lo = 0, hi = 241;
  while (lo < hi) {  // (the last entry at most |g|)
    const int m = (lo + hi + 1) >> 1;
    if (DMIX[m] <= a) lo = m;
    else hi = m - 1;
  }
  int c[2] = {lo, lo + 1}, n = 0;
  if (lo == 241 || fabs(DMIX[lo] - a) > fabs(DMIX[lo + 1] - a)) c[0] = lo + 1, c[1] = lo;
  for (int k = 0; k < 2; k++)
    if (c[k] >= 1 && c[k] <= 241 && fabs(DMIX[c[k]] - a) <= 0.02 * DMIX[c[k]])
      out[n++] = g < 0 ? -((S32)DMIX[c[k]] << 8) : (S32)DMIX[c[k]] << 8;
  return n;
}

// --- the decoder's subband samples (FFmpeg dca_core.c's, bit-exact) ---------------------------------

// step size times scale factor si of channel ch's ABITS a (adjusted where SEL picks a Huffman code)
static INLINE S64 step_for(const CoreRec& c, int ch, int a, int si) {
  S64 scale = c.shuff[ch] == 6 ? SF7[si & 127] : SF6[si & 63];
  if (sample_coding(c, ch, a) == 1) scale = clip23((S64)SF_ADJ[c.adj[ch][a - 1]] * scale >> 22);
  return (S64)LOSSY_QUANT[a] * scale;
}
// step size times scale factor of channel ch's subband b in subsubframe ssf (its transient's
// second scale factor from its subsubframe on)
static INLINE S64 step_scale(const CoreRec& c, const CoreSub& s, int ch, int b, int ssf) {
  const int tm = s.tmode[ch][b];
  return step_for(c, ch, s.abits[ch][b], s.scale[ch][b][tm && ssf >= tm ? 1 : 0]);
}
static INLINE S32 dequantize(S32 q, S64 ss) {
  int shift = 0;
  if (ss > (1 << 23)) {
    shift = ilog2((U64)(ss >> 23)) + 1;
    ss >>= shift;
  }
  return clip23(rshift((S64)q * ss, 22 - shift));
}
// the ADPCM prediction of predictor pvq from the 4 samples before (h[0] the last)
static INLINE S32 adpcm_pred(int pvq, const S32* h) {
  S64 p = 0;
  for (int k = 0; k < 4; k++) p += (S64)h[k] * ADPCM_VB[pvq][k];
  return clip23(rshift(p, 13));
}
// the same in the analysis's units, from its samples (h[0] the last): the open loop's
static INLINE S64 adpcm_pred_x(int pvq, const S64* h) {
  S128 p = 0;
  for (int k = 0; k < 4; k++) p += (S128)h[k] * ADPCM_VB[pvq][k];
  return (S64)((p + (1 << 12)) >> 13);
}
// the indexes ABITS a of channel ch can have
static INLINE void quant_range(const CoreRec& c, int ch, int a, S64& lo, S64& hi) {
  const int how = sample_coding(c, ch, a);
  if (how == 3) {
    hi = (BLOCK_LEVELS[a - 1] - 1) / 2, lo = -hi;
  } else if (how == 2) {
    hi = (1LL << (a - 4)) - 1, lo = -(1LL << (a - 4));
  } else {
    const HuffBook& hb = QB[a - 1][c.sel[ch][a - 1]];
    lo = hb.minlev, hi = hb.minlev + hb.nlev - 1;
  }
}

// The index the audio's subband sample v (2^-AQ_F, its ADPCM prediction taken off) quantizes to at
// step and scale ss (round to nearest, halves away from 0, as dtsreq found the encoder), in lo..hi;
// *alt the index on the other side of the rounding boundary nearest the value, *dist how far the
// value lies from that boundary (in 2^-(AQ_F + 22))
static INLINE S64 requant(S64 v, S64 ss, S64 lo, S64 hi, S128* dist, S64* alt) {
  if (ss <= 0) {  // (a scale factor of 0: the 7-bit table's last 3 entries)
    *dist = 0;
    *alt = lo > 0 ? lo : hi < 0 ? hi : 0;
    return *alt;
  }
  const S128 num = shl128(v, 22), den = (S128)ss << AQ_F;
  const int neg = num < 0;
  const S128 a = neg ? -num : num;
  const S128 qa = !(a >> 61) && !(den >> 61) ? (S128)((2 * (U64)a + (U64)den) / (2 * (U64)den))  // (in 64 bits
                                              : (2 * a + den) / (2 * den);  // where it fits) floor(a / den + 1/2)
  const S128 r = a - qa * den;                 // -den/2 .. den/2
  *dist = den / 2 - (r < 0 ? -r : r);
  S64 q = (S64)qa, o = r >= 0 ? q + 1 : q - 1;
  if (neg) q = -q, o = -o;
  *alt = o < lo ? lo : o > hi ? hi : o;
  return q < lo ? lo : q > hi ? hi : q;
}
// how near a distance d lies to 0, against a unit u (both in the same units): floor(log2(d / u)) +
// k, in 0..QB_N - 1
static INLINE int bitlen128(U128 x) {
  const U64 h = (U64)(x >> 64), l = (U64)x;
  return h ? 128 - __builtin_clzll(h) : l ? 64 - __builtin_clzll(l) : 0;
}
static INLINE int near_bucket(S128 d, S128 u, int k) {
  if (u <= 0) return QB_N - 1;
  if (d <= 0) return 0;
  // (floor(log2(d / u)): the bit lengths' difference e, or e - 1 where d < u 2^e)
  int e = bitlen128((U128)d) - bitlen128((U128)u);
  if (e >= 0 ? d < shl128(u, e) : shl128(d, -e) < u) e--;
  return clip(e + k, 0, QB_N - 1);
}

// The scale factor of table tab (nt entries) nearest (linearly) to the RMS of the n values v
// (2^-AQ_F), as dtsreq found the encoder's; *near how near the RMS lies to the midpoint between it
// and the next entry the other way (0 there, SB_N - 1 far)
static int scale_pred_ms(S128 ms, const U32* tab, int nt, int* near);
static int scale_pred(const S64* v, int n, const U32* tab, int nt, int* near) {
  S128 ss = 0;
  if (n <= 0) return *near = SB_N - 1, 0;
  for (int j = 0; j < n; j++) ss += (S128)v[j] * v[j];
  return scale_pred_ms(ss / n, tab, nt, near);
}
// the same of their mean square ms (2^-2AQ_F)
static int scale_pred_ms(S128 ms, const U32* tab, int nt, int* near) {
  auto sq = [](S128 x) { return x * x; };
  int lo = 0, hi = nt - 1;  // the last entry whose square is <= ms (0 where none)
  while (lo < hi) {
    const int m = (lo + hi + 1) >> 1;
    if (sq((S128)tab[m] << AQ_F) <= ms) lo = m;
    else hi = m - 1;
  }
  if (lo == nt - 1 || sq((S128)tab[lo] << AQ_F) > ms) {  // (past an end)
    *near = SB_N - 1;
    return lo;
  }
  // between tab[lo] and tab[lo + 1]: which nearer, and how far from their midpoint (the RMS by its
  // floor square root; IEEE's sqrt the start, which the integer steps make exact)
  const S128 mid2 = sq(((S128)tab[lo] + tab[lo + 1]) << (AQ_F - 1));
  const int p = ms >= mid2 ? lo + 1 : lo;
  S64 r = (S64)sqrt((double)ms);
  while (r > 0 && sq(r) > ms) r--;
  while (sq((S128)r + 1) <= ms) r++;
  const S64 mid = ((S64)tab[lo] + tab[lo + 1]) << (AQ_F - 1), step = ((S64)tab[lo + 1] - tab[lo]) << AQ_F;
  const S64 dist = r > mid ? r - mid : mid - r;  // 0 .. step / 2
  *near = step ? imin((int)(dist * 2 * SB_N / step), SB_N - 1) : SB_N - 1;
  return p;
}

// Where the RMS of the n values v (2^-AQ_F) lies in the bucket of tab's entry i (nt entries; the
// RMS rule's: the RMS nearer it than either neighbour), in quarters: 0-3 (4 none: an end's bucket)
static int scale_quarter(const S64* v, int n, const U32* tab, int nt, int i) {
  if (n <= 0 || i <= 0 || i >= nt - 1) return 4;
  S128 ss = 0;
  for (int j = 0; j < n; j++) ss += (S128)v[j] * v[j];
  const S128 ms = ss / n;
  auto sq = [](S128 x) { return x * x; };
  S64 r = (S64)sqrt((double)ms);  // (the floor square root, as scale_pred()'s)
  while (r > 0 && sq(r) > ms) r--;
  while (sq((S128)r + 1) <= ms) r++;
  const S64 lm = ((S64)tab[i - 1] + tab[i]) << (AQ_F - 1), um = ((S64)tab[i] + tab[i + 1]) << (AQ_F - 1);
  if (um <= lm) return 4;
  return clip((int)((S128)(r - lm) * 4 / (um - lm)), 0, 3);
}

// The scale factor another encoder's rule picks (a 96/24 stream's core: model_smp.inc's code_scale()
// takes the rule that has been right more often): the first entry of tab at least 0.99 times the
// RMS of the n values v (2^-AQ_F)
static int scale_ceil(const S64* v, int n, const U32* tab, int nt) {
  S128 ss = 0;
  if (n <= 0) return 0;
  for (int j = 0; j < n; j++) ss += (S128)v[j] * v[j];
  const S128 ms = ss / n * 9801;  // (0.99^2, times 10^4)
  int lo = 0, hi = nt - 1;  // the first entry whose square, times 10^4, is >= that
  while (lo < hi) {
    const int m = (lo + hi) >> 1;
    const S128 t = (S128)tab[m] << AQ_F;
    if (t * t * 10000 >= ms) hi = m;
    else lo = m + 1;
  }
  return lo;
}

// How far a decoded WAV's values (the ADPCM prediction off) lie from what the decoder makes of them
// at step and scale ss, value by value: the index nearest each, floor(|64 v| / ss + 1/2) with v's
// sign, in lo..hi, dequantized as it does (dequantize()); the index by a reciprocal, then made exact
struct QErr {
  S64 ss, lo, hi, ssh;  // (ssh, shift: dequantize()'s)
  int shift;
  double inv;
  void set(S64 ss_, S64 lo_, S64 hi_) {
    ss = ss_, lo = lo_, hi = hi_, ssh = ss, shift = 0;
    if (ss > (1 << 23)) {
      shift = ilog2((U64)(ss >> 23)) + 1;
      ssh = ss >> shift;
    }
    inv = 1.0 / (double)ss;
  }
  // value v's squared distance; *nz set where its index isn't 0
  INLINE S128 err(S64 v, int* nz) const {
    const S64 d = dev(v, nz);
    return (S128)d * d;
  }
  // (and the index alone, as requant()'s)
  INLINE S64 index(S64 v) const {
    const S64 u = v * 64, a = u < 0 ? -u : u, t = 2 * a + ss;
    S64 q = (S64)((double)a * inv + 0.5), r = t - 2 * q * ss;
    if (r < 0) q--, r += 2 * ss;
    else if (r >= 2 * ss) q++, r -= 2 * ss;
    if (r < 0 || r >= 2 * ss) {
      while (q > 0 && 2 * q * ss > t) q--;
      while (2 * (q + 1) * ss <= t) q++;
    }
    if (u < 0) q = -q;
    return q < lo ? lo : q > hi ? hi : q;
  }
  // (and the distance itself, v less its index's point)
  INLINE S64 dev(S64 v, int* nz) const {
    const S64 u = v * 64, a = u < 0 ? -u : u, t = 2 * a + ss;  // (the index: floor(t / 2ss))
    S64 q = (S64)((double)a * inv + 0.5), r = t - 2 * q * ss;  // (the estimate's rest: in 0..2ss where exact)
    if (r < 0) q--, r += 2 * ss;  // (it is within 1 of it but where the values are past 2^50)
    else if (r >= 2 * ss) q++, r -= 2 * ss;
    if (r < 0 || r >= 2 * ss) {
      while (q > 0 && 2 * q * ss > t) q--;
      while (2 * (q + 1) * ss <= t) q++;
    }
    if (u < 0) q = -q;
    q = q < lo ? lo : q > hi ? hi : q;
    *nz |= q != 0;
    return v - shl64(clip23(rshift(q * ssh, 22 - shift)), AQ_F);
  }
};
// The values whose index (QErr::index()'s, at step ss, in lo..hi) is t: *xlo to *xhi (none where
// *xlo > *xhi): the index is floor((128 |x| + ss) / 2 ss) with x's sign, so a positive t's are from
// ceil((2 t - 1) ss / 128) to ceil((2 t + 1) ss / 128) - 1, 0's to ceil(ss / 128) - 1 either side;
// lo's and hi's on to the end
static void index_cell(S64 t, S64 ss, S64 lo, S64 hi, S64* xlo, S64* xhi) {
  constexpr S64 BIG = (S64)1 << 62;
  if (t < lo || t > hi) {
    *xlo = 1, *xhi = 0;
    return;
  }
  const S64 at = t < 0 ? -t : t;
  auto up = [&](S64 m) { return (S64)(((S128)m * ss + 127) >> 7); };  // (ceil(m ss / 128), m >= 0)
  const S64 a = at ? up(2 * at - 1) : 0, b = up(2 * at + 1) - 1;  // (|x| from a to b)
  if (t > 0) *xlo = a, *xhi = b;
  else if (t < 0) *xlo = -b, *xhi = -a;
  else *xlo = -b, *xhi = b;
  if (t == hi) *xhi = BIG;
  if (t == lo) *xlo = -BIG;
}
// the squared distances' sum of the values v (n of them, 2^-AQ_F; 2^-2AQ_F): 4 at a time in
// doubles where |v| < 2^44 and ss < 2^48 (each index's estimate within 1 of it, and the rest
// t - 2 q ss that tells it exact, as dev()'s; the point as dequantize() makes it, and the distance,
// integers within 2^52), the squares in 128 bits
static S128 quant_err(const S64* v, int n, S64 ss, S64 lo, S64 hi) {
  QErr q;
  q.set(ss, lo, hi);
  S128 e = 0;
  int nz = 0, j = 0;
#ifdef DTSD_AVX2
  S64 mx = 0;
  for (int k = 0; k < n; k++) mx |= v[k] < 0 ? -v[k] : v[k];
  if (mx < (S64)1 << 44 && ss < (S64)1 << 48) {
    const int sh = 22 - q.shift;
    const __m256d vinv = _mm256_set1_pd(q.inv), half = _mm256_set1_pd(0.5), dss = _mm256_set1_pd((double)ss),
                  dss2 = _mm256_set1_pd(2.0 * (double)ss), one = _mm256_set1_pd(1.0), zero = _mm256_setzero_pd(),
                  vlo = _mm256_set1_pd((double)lo), vhi = _mm256_set1_pd((double)hi), vssh = _mm256_set1_pd((double)q.ssh),
                  vdh = _mm256_set1_pd(sh > 0 ? (double)((U64)1 << (sh - 1)) : 0.0),
                  vdsc = _mm256_set1_pd(sh > 0 ? 1.0 / (double)((U64)1 << sh) : 1.0), c23lo = _mm256_set1_pd(-8388608.0),
                  c23hi = _mm256_set1_pd(8388607.0), f16 = _mm256_set1_pd(65536.0), mg = _mm256_set1_pd(6755399441055744.0),
                  sgn = _mm256_set1_pd(-0.0);
    const __m256i mgi = _mm256_castpd_si256(mg);
    for (; j + 4 <= n; j += 4) {
      alignas(32) double vv[4];
      for (int k = 0; k < 4; k++) vv[k] = (double)v[j + k];
      const __m256d vd = _mm256_load_pd(vv), a = _mm256_mul_pd(_mm256_andnot_pd(sgn, vd), _mm256_set1_pd(64.0));
      const __m256d t = _mm256_add_pd(_mm256_add_pd(a, a), dss);
      __m256d qa = _mm256_floor_pd(_mm256_add_pd(_mm256_mul_pd(a, vinv), half));
      const __m256d r = _mm256_sub_pd(t, _mm256_mul_pd(dss2, qa));  // (exact: integers within 2^53)
      qa = _mm256_sub_pd(qa, _mm256_and_pd(_mm256_cmp_pd(r, zero, _CMP_LT_OQ), one));
      qa = _mm256_add_pd(qa, _mm256_and_pd(_mm256_cmp_pd(r, dss2, _CMP_GE_OQ), one));
      const __m256d qv = _mm256_min_pd(_mm256_max_pd(_mm256_or_pd(qa, _mm256_and_pd(sgn, vd)), vlo), vhi);
      __m256d y = _mm256_mul_pd(qv, vssh);
      if (sh > 0) y = _mm256_floor_pd(_mm256_mul_pd(_mm256_add_pd(y, vdh), vdsc));
      y = _mm256_min_pd(_mm256_max_pd(y, c23lo), c23hi);
      const __m256d d = _mm256_sub_pd(vd, _mm256_mul_pd(y, f16));
      alignas(32) S64 di[4];
      _mm256_store_si256((__m256i*)di, _mm256_sub_epi64(_mm256_castpd_si256(_mm256_add_pd(d, mg)), mgi));
      for (int k = 0; k < 4; k++) e += (S128)di[k] * di[k];
    }
  }
#endif
  for (; j < n; j++) e += q.err(v[j], &nz);
  return e;
}
// the same of a high-frequency VQ vector q at scale factor sc: its entries as the decoder scales them
static S128 vq_err(const S64* v, int n, int q, S64 sc) {
  S128 e = 0;
  for (int j = 0; j < n; j++) {
    const S64 d = v[j] - shl64(clip23((HF_VQ[q][j] * sc + 8) >> 4), AQ_F);
    e += (S128)d * d;
  }
  return e;
}

// the scale factor of table tab (nt entries) nearest (linearly) to t (2^-AQ_F)
static int scale_near(S64 t, const U32* tab, int nt) {
  int lo = 0, hi = nt - 1;  // (the last entry at most t)
  while (lo < hi) {
    const int m = (lo + hi + 1) >> 1;
    if (((S128)tab[m] << AQ_F) <= t) lo = m;
    else hi = m - 1;
  }
  if (lo < nt - 1 && 2 * (S128)t >= (((S128)tab[lo] + tab[lo + 1]) << AQ_F)) lo++;
  return lo;
}

// a bucket of x / y, |x| <= y, as -1..1 in n steps (0..n - 1); n / 2 where y is 0
static INLINE int ratio_bucket(S128 x, S128 y, int n) {
  if (y <= 0) return n / 2;
  const S128 k = (x + y) * n / (2 * y);
  return k < 0 ? 0 : k >= n ? n - 1 : (int)k;
}

// --- the encoder's codebook searches (as found; in integers) ---------------------------------------

// a gain in quarters of halvings: num / den against 2^(k / 4) (0-63; 63 where den is 0)
static int gain4(S128 num, S128 den) {
  static const S64 Q[4] = {65536, 77936, 92682, 110218};  // 2^(i / 4) in Q16
  if (den <= 0) return 63;
  int k = 0;
  while (k < 63 && den * Q[(k + 1) & 3] << ((k + 1) >> 2) <= num << 16) k++;
  return k;
}
// The 4 ADPCM coefficients the n samples x (2^-AQ_F) predict themselves by best: the normal
// equations of their autocorrelation (no window), solved by Levinson-Durbin in Q28, into alpha in
// Q24 (alpha[k] multiplies the sample k + 1 before, as the codebook's entries do); *gain the
// prediction gain in halvings (0-15). The encoder's predictor is then the codebook entry nearest
// alpha (pvq_dist()), over the subframe's samples and the 4 before them.
static void lpc4(const S64* x, int n, S64* alpha, int* gain, int* gain4_ = nullptr) {
  S128 r[5];
  for (int l = 0; l <= 4; l++) {
    r[l] = 0;
    for (int i = l; i < n; i++) r[l] += (S128)x[i] * x[i - l];
  }
  for (int m = 0; m < 4; m++) alpha[m] = 0;
  *gain = 0;
  if (r[0] <= 0) return;
  int sh = 0;  // (r[0] to 40 bits; |r[l]| <= r[0])
  while ((r[0] >> sh) >= ((S128)1 << 40)) sh++;
  S64 R[5], a[5] = {(S64)1 << 28, 0, 0, 0, 0};
  for (int l = 0; l <= 4; l++) R[l] = (S64)(r[l] >> sh);
  S64 err = R[0];
  for (int i = 1; i <= 4 && err > 0; i++) {
    S128 acc = shl128(R[i], 28);
    for (int j = 1; j < i; j++) acc += (S128)a[j] * R[i - j];
    const S128 kq = -acc / err;
    if (kq >= ((S128)1 << 28) || kq <= -((S128)1 << 28)) break;  // (unstable: the order so far)
    const S64 k = (S64)kq;
    S64 t[5];
    for (int j = 0; j <= 4; j++) t[j] = a[j];
    for (int j = 1; j < i; j++) a[j] = t[j] + (S64)(((S128)k * t[i - j]) >> 28);
    a[i] = k;
    err -= (S64)(((((S128)err * k) >> 28) * k) >> 28);
  }
  for (int m = 0; m < 4; m++) {
    const S64 v = (-a[m + 1] + 8) >> 4;
    alpha[m] = v < -((S64)8 << 24) ? -((S64)8 << 24) : v > ((S64)8 << 24) ? ((S64)8 << 24) : v;
  }
  int g = 0;
  if (err <= 0) g = 15;
  else
    while (g < 15 && (err << (g + 1)) <= R[0]) g++;
  *gain = g;
  if (gain4_) *gain4_ = err <= 0 ? 63 : gain4(R[0], err);
}
// the ADPCM codebook entry q's prediction gain over the n samples x (2^-AQ_F), the 4 before them h
// (h[0] the last), in the open loop (gain4())
static int pvq_gain4(int q, const S64* h, const S64* x, int n) {
  S64 hh[4] = {h[0], h[1], h[2], h[3]};
  S128 se = 0, ee = 0;
  for (int j = 0; j < n; j++) {
    const S64 e = x[j] - adpcm_pred_x(q, hh);
    se += (S128)x[j] * x[j];
    ee += (S128)e * e;
    hh[3] = hh[2], hh[2] = hh[1], hh[1] = hh[0], hh[0] = x[j];
  }
  return gain4(se, ee);
}
// the ADPCM codebook entry q's distance from the coefficients alpha (Q24)
static INLINE S64 pvq_dist(int q, const S64* alpha) {
  S64 d = 0;
  for (int m = 0; m < 4; m++) {
    const S64 e = alpha[m] - shl64(ADPCM_VB[q][m], 11);
    d += e * e;
  }
  return d;
}
// every entry's (into d[4096]): as pvq_dist()'s, in 32-bit differences (alpha within 2^27, lpc4()'s;
// the entries times 2^11 within 2^26), the code book by coefficient
struct PvqT {
  S32 v[4][4096];
  PvqT() {
    for (int q = 0; q < 4096; q++)
      for (int m = 0; m < 4; m++) v[m][q] = ADPCM_VB[q][m] * (1 << 11);
  }
};
static const PvqT pvq_t;
static void pvq_dists(const S64* alpha, S64* d) {
  const S32 a0 = (S32)alpha[0], a1 = (S32)alpha[1], a2 = (S32)alpha[2], a3 = (S32)alpha[3];
  const S32 *v0 = pvq_t.v[0], *v1 = pvq_t.v[1], *v2 = pvq_t.v[2], *v3 = pvq_t.v[3];
  for (int q = 0; q < 4096; q++) {
    const S32 e0 = a0 - v0[q], e1 = a1 - v1[q], e2 = a2 - v2[q], e3 = a3 - v3[q];
    d[q] = (S64)e0 * e0 + (S64)e1 * e1 + (S64)e2 * e2 + (S64)e3 * e3;
  }
}
#include "pvqblocks.inc"
// The DTS-HD Master Audio encoders search the code book in two stages: the nearest of 64 codewords
// of their own, each a block of 64 entries (64 g to 64 g + 63), then the block's entry nearest the
// coefficients (the nearest of all is their predictor 88-91% of the time, the two stages 99.6-99.8%,
// by the codewords pvqblocks.inc learns from their streams). Block g's codeword's distance from the
// coefficients alpha (Q24; the codewords in 2^-16, within 2^27 as alpha is)
static INLINE S64 pvq_block_dist(int g, const S64* alpha) {
  S64 d = 0;
  for (int m = 0; m < 4; m++) {
    const S64 e = alpha[m] - (S64)PV_GC[g][m] * 256;
    d += e * e;
  }
  return d;
}

// The HF VQ code book as the encoders search it: groups of 32 entries, the group whose centroid is
// nearest first, then the entry of the group nearest; the samples x (n, 2^-AQ_F) as 16 / sc times
// them (sc the subband's scale factor), against the entries as they are. A group's centroid is the
// mean of its entries each scaled to unit energy, times 0.9 the entries' mean norm (61.16): their
// plain mean makes the encoders' first group 84.4% of `s.2.dts`'s first 3 MB's time, this 87.3%
// (79.1 and 81.6% of 3 MB from its middle; `Master Audio 5.0 96khz.dts`'s 86.2 and 88.8%).
// HF_GC: those, in 512ths (exactly: none lies within 1/1000 of a rounding boundary)
static const S16 HF_GC[32][32] = {
    {-399, 180, 962, 395, -460, -1024, -143, 604, -1553, -1388, -721, -1112, 1088, -2286, 1737, -476, 317, -203, 214, 189, -1298, -63, -16, -2752, -2103, -510, -4123, 779, 722, 380, 270, -753},
    {-1236, 183, 790, 384, -673, -731, -470, 443, -264, -840, -925, -588, -513, -720, 561, -512, -1046, 344, -791, -1021, 37, 368, 460, -497, -587, -716, -1524, -1324, 4042, -4472, 15703, 3249},
    {-3815, -13299, 1809, 8879, 572, -142, 151, -3515, -1843, -714, 1546, -358, 1234, -1748, -2008, 215, -256, 1476, 236, 846, -523, 464, 50, -96, 1143, 153, 1143, 895, 599, 758, 955, -837},
    {-1175, 459, 940, 125, -229, -583, -291, 115, 85, 1255, -51, -378, -40, 65, 1011, 91, 954, 74, -539, 88, -614, -255, -176, -564, -2474, 1447, -919, 1117, -2399, 1168, -16580, -233},
    {525, 309, -59, -758, -716, 902, 407, 672, 227, -458, -42, 31, 674, 922, 798, -802, 407, 127, 15186, -7285, 832, 3813, 817, -470, 844, 137, -355, -721, 293, 783, 456, 82},
    {-5857, 4492, -6688, 7198, -8567, 5255, -3485, 4238, -584, 4712, 1144, 1825, 417, -727, 537, -615, -312, -981, 1025, -1556, 545, 514, -348, 176, 100, -577, -486, -133, -188, -1031, 1099, -519},
    {-3004, -2216, 1705, -1365, 5085, 4763, -3545, -39, -1849, -7319, 7262, 6259, -2580, 292, -997, -1953, 1367, 2739, -2546, 271, -776, -2545, 19, 994, -3360, -1360, -2602, -2869, -835, -1086, 389, -136},
    {8620, 3197, 6572, -11102, 2653, -2521, -1003, 960, -3690, 1567, 438, 2238, -1018, -2265, 1079, 768, 345, 909, 290, -1207, 2016, 584, -500, -1624, -699, 194, -2013, 1010, 251, 527, 540, 494},
    {-1577, 1756, 1354, -1698, -2307, -2179, 4143, -2314, 3198, -1603, 1909, 2597, -6386, 127, -6720, 6143, 3598, -481, 4670, -2647, 739, 1048, -2463, 1519, -1218, 700, 1092, 1089, 176, 449, -1283, 2077},
    {8893, 6058, -962, 5734, -6009, -2330, -4260, -5096, -340, -2389, 3514, 1792, 1417, 2575, 2043, 1868, -1190, -674, -1612, -612, 1060, -1511, 1422, -1718, 919, 1597, -1437, -45, -83, -201, 999, -1248},
    {612, 38, 427, -225, -205, 82, -170, 31, -514, 533, -803, 239, -145, -838, 237, 628, -1094, -516, -10002, -12467, -1454, 9842, 1790, 519, -898, 403, -684, 652, 120, -328, 136, 106},
    {1128, 342, -570, 1259, -739, 438, -257, 1084, -383, 780, -1006, -219, -25, -600, -186, -177, 15, 2057, -2548, 16196, 271, -9030, 899, -1000, 1025, 175, -236, 1518, -196, 630, 823, -91},
    {13561, -7242, 1353, 191, 4064, -3313, 1623, -2503, 3158, -1888, 1535, -1333, 1653, -2539, 483, -2108, 1197, 435, 397, -865, -516, 389, -321, 740, 98, -27, -637, -9, -373, 668, -1881, -835},
    {-7790, 7048, -53, 8121, 1334, 355, 5653, -3608, 4543, -6848, 138, -3111, 62, 702, 201, 1375, -1049, 478, -1392, 1753, 328, 655, 1630, -629, 1799, -2675, 182, -2144, -330, 144, -545, 856},
    {-6396, 3745, -5328, -11603, -3211, -3725, 4093, 755, -1726, 2253, 1857, -1709, 1475, 2043, 2589, -44, 456, 473, -738, -236, 641, 375, 1270, -684, -52, -1376, 632, -738, -2138, -535, 999, -490},
    {594, -823, 244, -705, -348, -428, -836, -188, -319, 692, 521, 54, -26, -42, -1377, -653, 1111, -935, -19560, 1249, -1451, -4705, -352, -594, -329, 1033, -929, 936, 393, -358, -22, -484},
    {7909, 15600, -2410, -480, 731, 746, 1811, 2115, 598, 576, 443, -716, -432, -1570, 762, -132, 227, 76, 525, -942, -522, 778, 271, -392, 460, 196, 538, -631, 280, 137, -774, -535},
    {690, 3699, 2524, -1605, 1917, 5805, -1607, -3096, -326, 3898, -3591, -11982, 2626, 3448, -2327, -1917, 168, -954, -893, 153, -540, -1179, -1017, 1251, -1618, -492, 681, -278, -273, -665, -1229, -784},
    {-14662, -7842, 1120, -4066, 1573, -184, 1645, 728, 1231, 619, -2138, 1443, 1217, 75, 858, -447, -20, 198, 1241, -992, -132, 729, -91, -462, -289, -493, -1034, -846, 995, 375, 277, 737},
    {-3793, -754, -1101, 6094, 3997, -11382, -6112, 2918, 3582, 2864, -835, -4246, -1941, 2477, 1154, -2035, 1734, -799, -887, 891, 359, -164, 379, -236, -1381, 607, 588, -240, 330, -1, 909, -654},
    {-9231, 14285, 2021, -865, -31, -2266, -2483, -407, -1422, 396, -16, 3179, -290, -1118, -393, -1237, -986, 299, -353, 541, -501, -316, 690, 23, -563, -357, -894, 481, 1136, -1083, 102, 263},
    {5098, 1327, 3338, 7472, 3186, 4619, 5500, 3832, -129, 7826, 2602, 1818, -858, -54, -344, -236, -516, -1522, 9, -2216, -1999, -1329, 819, -600, 834, -4366, -451, -46, 743, -12, 3267, 1093},
    {-308, -491, -1039, -101, -218, 422, -765, -1553, -1932, -1693, -974, -1381, -641, -132, 1694, -549, 2358, 1210, 380, -1179, 225, 3814, 2909, 1706, 2788, 2319, 9855, 282, 7809, 5943, 772, 3262},
    {4348, -14843, 1055, -5588, -4119, 995, -2759, 3946, -794, 1585, -674, -1115, 284, -1226, 732, 633, -111, 477, 431, 102, 1102, -64, -462, -865, -577, 797, -935, 1396, -379, 383, 1049, -34},
    {-1245, 22, -140, -714, -255, 1011, -322, -243, 882, 792, 992, 770, 683, -1329, 529, 435, -255, -706, 1434, 271, 910, 1243, 252, -205, -431, -3579, -471, -351, 3556, 342, -3161, -15362},
    {690, -822, -602, 670, 2288, -927, -477, -166, -461, 1454, 657, 1285, 940, -1557, -1269, 1494, -1267, -1553, -1803, 1424, 2119, -1224, 3211, -1689, 3364, -2458, 759, -3865, 8993, -4430, -8930, 1304},
    {3362, -508, -800, -3087, -3338, 4060, 3151, 2079, 2311, -5355, -5572, 2260, 5186, -107, 1407, -5407, -2241, 966, 2239, 1103, 4213, -5925, -3572, 2348, 2528, 1528, 1192, -3025, -2213, 1274, 424, 2574},
    {381, -822, -514, -94, 377, -123, -1393, 1319, -2298, 2678, -1240, 1399, -920, 1747, -5169, 3221, -7002, 3611, -4343, 5248, -2409, 3476, -6010, 3685, -3652, 6628, -534, -168, -1789, -655, -371, 2274},
    {2830, -5853, -8964, 1356, 883, 3987, 3941, -4337, 222, -2245, -3599, 187, -2408, 3680, 4777, 2303, -1957, -4892, -1573, 1496, -1577, 493, 2195, 719, 349, -2555, -1747, 1069, -536, -604, 511, 737},
    {-841, 790, 1178, -113, 412, 282, -1383, -56, -831, 381, -244, 109, 2723, -1158, -635, -158, 1900, 48, -1189, -1516, -453, 750, -335, -1378, -1780, -1099, -1398, 2606, -3787, 393, -1905, 15417},
    {1137, 134, -126, 1636, 1086, -863, -1383, 502, -231, -114, -1873, -1076, 328, -875, 2682, 468, 323, 283, 153, -1142, -404, 571, -674, -2062, 3514, 3179, 1826, 5428, -11977, -3359, -2876, -4280},
    {1528, -639, -773, -881, 525, 283, 747, -1480, -1053, -1006, 750, 27, 1076, -14, -807, -632, 548, -504, 1641, 333, -1451, 8, -331, 1801, 195, -339, -310, -1405, -5535, 6110, 12581, -6133},
};
// group g's distance (in 2^(AQ_F + 13) sc units)
static INLINE S128 hf_group_dist(int g, const S64* x, int n, S64 sc) {
  S128 d = 0;
  for (int j = 0; j < n; j++) {
    const S64 e = x[j] * 8192 - shl64((S64)HF_GC[g][j] * sc, AQ_F);  // (|x| < 2^40, |HF_GC| < 2^14: 2^54)
    d += (S128)e * e;
  }
  return d;
}
// The DTS-HD Master Audio encoders' group of a lossless WAV's 16 samples (their streams' subframes'
// length) isn't the centroids' nearest: their silences (the samples all 0, 5-30% of the vectors)
// take group 0's entry 0, and most of the other misses are near ties. A score learned from five
// of their streams does better (hfgroups.inc, tools/hfgroups.py's from ./mk.sh hfdump's dumps): of
// W (in 512ths) and e (in 2^-18), the least |16 x / sc - W_g|^2 + e_g, in hf_group_dist()'s units
// (another length: hf_group_dist()). Each stream left out of the learning, its first group is the
// encoder's in 81.6-96.7% of its vectors, against 61.3-89.0% by the centroids.
#include "hfgroups.inc"
static INLINE S128 hf_group_score(const S32 (*W)[16], const S64* E, int g, const S64* x, int n, S64 sc) {
  if (n != 16) return hf_group_dist(g, x, n, sc);
  S128 d = 0;
  for (int j = 0; j < 16; j++) {
    const S64 e = x[j] * 8192 - shl64((S64)W[g][j] * sc, AQ_F);  // (|W| < 2^16: 2^56)
    d += (S128)e * e;
  }
  return d + shl128((S128)E[g] * sc * sc, 32);
}
// 16 times log2(x) (x > 0), its fraction by the 4 bits after the top one
static INLINE int lg16(U64 x) {
  const int e = ilog2(x);
  return 16 * e + (int)((e >= 4 ? x >> (e - 4) : x << (4 - e)) & 15);
}
// lg16() of x > 0 of 128 bits
static INLINE int lg16w(U128 x) {
  const U64 h = (U64)(x >> 64);
  if (!h) return lg16((U64)x);
  const int e = 127 - __builtin_clzll(h);  // (>= 64: its 4 bits after the top one within x >> 60)
  return 16 * e + (int)((U64)(x >> (e - 4)) & 15);
}
// near_bucket()'s nearness with its fraction, of a distance and a unit by their lg16n() (ld, lu):
// 16 log2(d / u) + 16 k, as the bucket at its floor (*i, 0..QB_N - 1) and the sixteenths of the way
// from it to the next (*w; 0 at either end)
constexpr int LG16_NONE = -(1 << 30);  // (lg16n() of a value <= 0)
static INLINE void near_frac(int ld, int lu, int k, int* i, int* w) {
  *w = 0;
  if (lu == LG16_NONE || ld == LG16_NONE) {
    *i = lu == LG16_NONE ? QB_N - 1 : 0;
    return;
  }
  const int t = ld - lu + 16 * k;
  *i = t < 0 ? 0 : t >> 4;
  if (t > 0 && *i < QB_N - 1) *w = t & 15;
  if (*i > QB_N - 1) *i = QB_N - 1;
}
static INLINE int lg16n(S128 x) { return x > 0 ? lg16w((U128)x) : LG16_NONE; }
// log2 and 2^x in 2^-16 units by tables of 256 made in integers alone, so that every build has
// the same: lg[i] = 2^16 log2(1 + (i + 1/2) / 256), the fraction's bits by squaring;
// ex[i] = 2^31 2^((i + 1/2) / 256), the product of the 2^(2^-k) of its bits, each the root of
// the one before (isqrt128())
struct Lg2Tab {
  S32 lg[256];
  U32 ex[256];
  Lg2Tab() {
    for (int i = 0; i < 256; i++) {
      U64 x = (U64)(513 + 2 * i) << 53;  // (in [1, 2) at 2^62)
      int f = 0;
      for (int b = 0; b < 18; b++) {
        x = (U64)((U128)x * x >> 62);
        f = 2 * f + (int)(x >> 63);
        if (x >> 63) x >>= 1;
      }
      lg[i] = (f + 2) >> 2;
    }
    U64 r[10];  // (2^(2^-k) at 2^62)
    r[0] = (U64)1 << 63;
    for (int k = 1; k < 10; k++) r[k] = (U64)isqrt128((U128)r[k - 1] << 62);
    for (int i = 0; i < 256; i++) {
      U64 p = (U64)1 << 62;
      for (int k = 1; k <= 9; k++)
        if ((2 * i + 1) >> (9 - k) & 1) p = (U64)((U128)p * r[k] >> 62);
      ex[i] = (U32)((p + ((U64)1 << 30)) >> 31);
    }
  }
  // 2^16 log2(x), x >= 1
  INLINE S32 log2x(U64 x) const {
    const int e = ilog2(x);
    return (e << 16) + lg[(x << (63 - e)) >> 55 & 255];
  }
  // 2^(l / 2^16) in CO_ONE units (l <= 0: a probability), within 1..CO_ONE - 1
  INLINE U32 prob(S64 l) const {
    const S64 q = l + ((S64)CO_PBITS << 16);
    if (q < 0) return 1;
    const U64 v = (((U64)ex[q >> 8 & 255] << (q >> 16)) + ((U64)1 << 30)) >> 31;
    return (U32)(v < 1 ? 1 : v > CO_ONE - 1 ? CO_ONE - 1 : v);
  }
};
static const Lg2Tab lg2tab;
// X96's steps: each LOSSY_QUANT entry times each SF6 scale factor (x96_ss()'s, 2^-22), and their
// lg16(); made where first needed (make())
struct JxSteps {
  S64 v[32 * 64];
  double it[32 * 64], t[32 * 64], tb[32 * 64];  // (2^22 / v, v / 2^22 and 4e-7 of that: jx_pick()'s test)
  S16 lg[32 * 64];
  // (the steps by their lg16(), each value's in ascending k: by[at[l - lg0]] to by[at[l - lg0 + 1]];
  // jx_try() goes through only those of the lg16() values a subband has seen)
  static constexpr int NL = 1024;
  int lg0, nl;
  S16 by[32 * 64];
  int at[NL + 1];
  bool made;
  void make() {  // (once: where a stream first has X96 and the WAV, smp_subframe())
    if (made) return;
    made = true;
    for (int a = 0; a < 32; a++)
      for (int i = 0; i < 64; i++) {
        const int k = 64 * a + i;
        v[k] = (S64)LOSSY_QUANT[a] * SF6[i];
        lg[k] = (S16)(v[k] > 0 ? lg16((U64)v[k]) : -1);
        it[k] = v[k] > 0 ? 4194304.0 / (double)v[k] : 0, t[k] = (double)v[k] / 4194304.0, tb[k] = 4e-7 * t[k];
      }
    int mn = 1 << 30, mx = -1;
    for (int k = 0; k < 32 * 64; k++)
      if (v[k] > 0) mn = lg[k] < mn ? lg[k] : mn, mx = lg[k] > mx ? lg[k] : mx;
    lg0 = mn, nl = mx - mn + 1;
    for (int l = 0; l <= nl; l++) at[l] = 0;
    for (int k = 0; k < 32 * 64; k++)
      if (v[k] > 0) at[lg[k] - lg0 + 1]++;
    for (int l = 0; l < nl; l++) at[l + 1] += at[l];
    int pos[NL];
    for (int l = 0; l < nl; l++) pos[l] = at[l];
    for (int k = 0; k < 32 * 64; k++)
      if (v[k] > 0) by[pos[lg[k] - lg0]++] = (S16)k;
  }
};
static JxSteps jx_steps;
// The HF VQ code book's entries by their first value: v[e] the e-th distinct one (in 1/16 of a
// scale factor, 128 added), bit k of m[e] set where entry k's is that (jx_search_vq()'s)
struct HfFirst {
  int nv;
  U8 v[256];
  U64 m[256][16];
  HfFirst() {
    int at[256];
    for (int e = 0; e < 256; e++) at[e] = -1;
    nv = 0;
    for (int k = 0; k < 1024; k++) {
      const int e = HF_VQ[k][0] + 128;
      if (at[e] < 0) {
        at[e] = nv, v[nv] = (U8)e;
        for (int x = 0; x < 16; x++) m[nv][x] = 0;
        nv++;
      }
      m[at[e]][k >> 6] |= 1ull << (k & 63);
    }
  }
};
static const HfFirst hf_first;
// The same at each of the first 16 places: v[j][e] the e-th distinct value at place j, bit k of
// m[j][e] set where entry k's is that; made where first asked for (jx_search_vq()'s)
struct HfPlaces {
  int nv[16];
  S8 v[16][128];
  U64 m[16][128][16];
  bool made;
  void make() {
    for (int j = 0; j < 16; j++) {
      int at[256];
      for (int e = 0; e < 256; e++) at[e] = -1;
      nv[j] = 0;
      for (int k = 0; k < 1024; k++) {
        const int e = HF_VQ[k][j] + 128;
        if (at[e] < 0) {
          at[e] = nv[j], v[j][nv[j]] = HF_VQ[k][j];
          for (int x = 0; x < 16; x++) m[j][nv[j]][x] = 0;
          nv[j]++;
        }
        m[j][at[e]][k >> 6] |= 1ull << (k & 63);
      }
    }
    made = true;
  }
};
static HfPlaces hf_places;

// entry q's distance (in 2^(AQ_F + 4) sc units)
static INLINE S128 hf_dist(int q, const S64* x, int n, S64 sc) {
  S128 d = 0;
  for (int j = 0; j < n; j++) {
    const S64 e = x[j] * 16 - shl64((S64)HF_VQ[q][j] * sc, AQ_F);  // (2^47)
    d += (S128)e * e;
  }
  return d;
}

// The encoder's values are a little off the WAV's (its arithmetic; a lossless WAV's top subband's
// leak), so where its search's first isn't its choice, its choice is the one whose boundary with
// the first lies nearest the values, rather than the one nearest them: the others in the order of
// their key's excess over the first's over the distance between their entries (the boundary's
// distance from the values; the first's -1, and an equal entry's, as the encoder takes the lowest
// index of equals; the groups' as well, hf_bound_grp(): `s.2.dts`'s core 2.3% smaller, its top
// subband's HF VQ 31.5 KB to 29.5, 3 MB of its middle 1.7%). Group g's entries, by their distances
// k (hf_dist()'s, over their first n values), into o.
static void hf_bound_vec(const S128* k, int g, int n, S128* o) {
  int f = 0;
  for (int i = 1; i < 32; i++) f = k[i] < k[f] ? i : f;
  const S8* const vf = HF_VQ[g * 32 + f];
  for (int i = 0; i < 32; i++) {
    const S8* const vi = HF_VQ[g * 32 + i];
    S32 s2 = 0;  // (the squared distance over the first n values, exactly: a double's as well)
    for (int j = 0; j < n; j++) {
      const S32 e = vi[j] - vf[j];
      s2 += e * e;
    }
    const double d = sqrt((double)s2);
    o[i] = i == f || d == 0 ? -1 : (S128)((double)(k[i] - k[f]) / d);  // (within 2^99)
  }
}
// The same of the groups by their scores k, the distance between their W (32 of n values, each
// group's at W + w g) to the power p (1, or 1.5: d sqrt(d), IEEE's sqrt exact everywhere)
template <class T>
static void hf_bound_grp(const S128* k, const T* W, int w, int n, bool p15, S128* o) {
  int f = 0;
  for (int g = 1; g < 32; g++) f = k[g] < k[f] ? g : f;
  for (int g = 0; g < 32; g++) {
    double s2 = 0;
    for (int j = 0; j < n; j++) {
      const double e = (double)W[g * w + j] - (double)W[f * w + j];
      s2 += e * e;
    }
    const double d = sqrt(s2);
    o[g] = g == f ? -1 : s2 == 0 ? (k[g] == k[f] ? -1 : (S128)1 << 120) : (S128)((double)(k[g] - k[f]) / (p15 ? d * sqrt(d) : d));  // (within 2^118)
  }
}
// (the same by the distances' powers D, each group's with each other's)
static void hf_bound_grp_d(const S128* k, const double (*D)[32], S128* o) {
  int f = 0;
  for (int g = 1; g < 32; g++) f = k[g] < k[f] ? g : f;
  for (int g = 0; g < 32; g++) {
    const double d = D[g][f];
    o[g] = g == f ? -1 : d == 0 ? (k[g] == k[f] ? -1 : (S128)1 << 120) : (S128)((double)(k[g] - k[f]) / d);
  }
}

// the entry most nearly a multiple of the samples x (n of them, at most 32): the one whose
// projection leaves the least, (x e)^2 / (e e) the most (the first of equals); its scale is free
struct HfEnergies {
  S32 e[4][1024];             // each entry's energy over its first 8, 16, 24, 32 values
  double inv[4][1024], nz[4][1024];  // 1 over its square root (0 where it is 0); whether it isn't 0
  HfEnergies() {
    for (int q = 0; q < 1024; q++) {
      S32 s = 0;
      for (int j = 0; j < 32; j++) {
        s += HF_VQ[q][j] * HF_VQ[q][j];
        if ((j & 7) == 7) e[j >> 3][q] = s, inv[j >> 3][q] = s > 0 ? 1 / sqrt((double)s) : 0, nz[j >> 3][q] = s > 0;
      }
    }
  }
};
static const HfEnergies hf_energies;
// The dot products of x (n values, at most 32) with each entry's first n, exactly, and as doubles:
// *dot and *dd, 1024 of each (valid to the next call); in doubles where |x| < 2^40 (whether: the
// result; a product within 2^47, the sums 2^52, integers a double holds whatever their order), a
// group's 32 entries at a time, as HfT32 has them; else in 64 bits. (The values' kept, 4 of them
// by a hash: a subband's scale factor search, then its vector's, take the same.)
struct HfT32 {
  double t[32][32][32];  // group g's entries' value j: t[g][j]
  HfT32() {
    for (int q = 0; q < 1024; q++)
      for (int j = 0; j < 32; j++) t[q >> 5][j][q & 31] = HF_VQ[q][j];
  }
};
static const HfT32 hf_t32;
static bool hf_dots(const S64* x, int n, const S64** dot, const double** dd) {
  struct Kept {
    S64 x[32], d[1024];
    double f[1024];
    int n;
    bool small;
  };
  static Kept kept[4];
  static bool init = false;
  if (!init) {
    for (int i = 0; i < 4; i++) kept[i].n = -1;
    init = true;
  }
  U64 h = (U64)n;
  for (int j = 0; j < n; j++) h = (h ^ (U64)x[j]) * 0x9E3779B97F4A7C15ull;
  Kept& e = kept[h >> 62];
  *dot = e.d, *dd = e.f;
  if (e.n == n && !memcmp(x, e.x, sizeof(S64) * n)) return e.small;
  S64 mx = 0;
  for (int j = 0; j < n; j++) mx |= x[j] < 0 ? -x[j] : x[j];
  if (mx >= (S64)1 << 40) {
    for (int q = 0; q < 1024; q++) {
      S64 d = 0;
      for (int j = 0; j < n; j++) d += x[j] * HF_VQ[q][j];
      e.d[q] = d, e.f[q] = (double)d;
    }
    e.small = false;
  } else {
    double xd[32];
    for (int j = 0; j < n; j++) xd[j] = (double)x[j];
#ifdef DTSD_AVX2  // (8 vectors of 4 sums a group, in registers; integers all, whatever the rounding: FMA's
                  // the same; to 64 bits by the magic number 1.5 2^52, as |sum| < 2^51: |x| < 2^40 times
                  // an entry's sum of |values|, 451 at most)
    const __m256d mg = _mm256_set1_pd(6755399441055744.0);
    const __m256i mgi = _mm256_castpd_si256(mg);
    for (int g = 0; g < 32; g++) {
      __m256d a0 = _mm256_setzero_pd(), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0;
      for (int j = 0; j < n && mx; j++) {
        const double* const t = hf_t32.t[g][j];
        const __m256d v = _mm256_set1_pd(xd[j]);
        a0 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t), a0), a1 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 4), a1);
        a2 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 8), a2), a3 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 12), a3);
        a4 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 16), a4), a5 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 20), a5);
        a6 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 24), a6), a7 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 28), a7);
      }
      const __m256d av[8] = {a0, a1, a2, a3, a4, a5, a6, a7};
      for (int k = 0; k < 8; k++) {
        _mm256_storeu_pd(e.f + g * 32 + 4 * k, av[k]);
        _mm256_storeu_si256((__m256i*)(e.d + g * 32 + 4 * k), _mm256_sub_epi64(_mm256_castpd_si256(_mm256_add_pd(av[k], mg)), mgi));
      }
    }
#else
    for (int g = 0; g < 32; g++) {
      double a[32];
      for (int i = 0; i < 32; i++) a[i] = 0;
      for (int j = 0; j < n && mx; j++) {
        const double* const t = hf_t32.t[g][j];
        for (int i = 0; i < 32; i++) a[i] += xd[j] * t[i];
      }
      for (int i = 0; i < 32; i++) e.d[g * 32 + i] = (S64)a[i], e.f[g * 32 + i] = a[i];
    }
#endif
    e.small = true;
  }
  e.n = n;
  memcpy(e.x, x, sizeof(S64) * n);
  return e.small;
}
// The distances (hf_dist()'s) of group g's 32 entries from the values x (n of them) at once,
// exactly, into d: (16 x - 2^16 sc v)^2 summed is 256 x x - 2^21 sc x v + 2^32 sc^2 v v, the dot
// products in doubles where |x| < 2^40 (a product within 2^47, the sums 2^52: integers a double
// holds, whatever their order), a group at a time as hf_dots() makes them; else entry by entry
static void hf_dists(int g, const S64* x, int n, S64 sc, S128* d) {
  S64 mx = 0;
  for (int j = 0; j < n; j++) mx |= x[j] < 0 ? -x[j] : x[j];
  if (mx >= (S64)1 << 40) {
    for (int i = 0; i < 32; i++) d[i] = hf_dist(g * 32 + i, x, n, sc);
    return;
  }
  S128 en = 0;
  for (int j = 0; j < n; j++) en += (S128)(x[j] * 16) * (x[j] * 16);
  alignas(32) double a[32];
#ifdef DTSD_AVX2
  __m256d a0 = _mm256_setzero_pd(), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0;
  for (int j = 0; j < n; j++) {
    const double* const t = hf_t32.t[g][j];
    const __m256d v = _mm256_set1_pd((double)x[j]);
    a0 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t), a0), a1 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 4), a1);
    a2 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 8), a2), a3 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 12), a3);
    a4 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 16), a4), a5 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 20), a5);
    a6 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 24), a6), a7 = _mm256_fmadd_pd(v, _mm256_loadu_pd(t + 28), a7);
  }
  _mm256_store_pd(a, a0), _mm256_store_pd(a + 4, a1), _mm256_store_pd(a + 8, a2), _mm256_store_pd(a + 12, a3);
  _mm256_store_pd(a + 16, a4), _mm256_store_pd(a + 20, a5), _mm256_store_pd(a + 24, a6), _mm256_store_pd(a + 28, a7);
#else
  for (int i = 0; i < 32; i++) a[i] = 0;
  for (int j = 0; j < n; j++) {
    const double xj = (double)x[j], *const t = hf_t32.t[g][j];
    for (int i = 0; i < 32; i++) a[i] += xj * t[i];
  }
#endif
  const S32* const ee = (n & 7) ? nullptr : hf_energies.e[n / 8 - 1] + g * 32;
  for (int i = 0; i < 32; i++) {
    S32 e2 = 0;
    if (ee) e2 = ee[i];
    else
      for (int j = 0; j < n; j++) e2 += HF_VQ[g * 32 + i][j] * HF_VQ[g * 32 + i][j];
    d[i] = en - shl128((S128)(S64)a[i] * sc, 21) + shl128((S128)e2 * sc * sc, 32);
  }
}
// The same of the 32 groups' centroids (hf_group_dist()'s): (2^13 x - 2^16 sc c)^2 summed is 2^26
// x x - 2^30 sc x c + 2^32 sc^2 c c, the dot products in 64 bits (|x| < 2^40: within 2^59)
static void hf_group_dists(const S64* x, int n, S64 sc, S128* d) {
  S64 mx = 0;
  for (int j = 0; j < n; j++) mx |= x[j] < 0 ? -x[j] : x[j];
  if (mx >= (S64)1 << 40) {
    for (int g = 0; g < 32; g++) d[g] = hf_group_dist(g, x, n, sc);
    return;
  }
  S128 en = 0;
  for (int j = 0; j < n; j++) en += (S128)x[j] * x[j];
  en = shl128(en, 26);
  for (int g = 0; g < 32; g++) {
    S64 dt = 0, e2 = 0;
    for (int j = 0; j < n; j++) dt += x[j] * HF_GC[g][j], e2 += (S64)HF_GC[g][j] * HF_GC[g][j];
    d[g] = en - shl128((S128)dt * sc, 30) + shl128((S128)e2 * sc * sc, 32);
  }
}
static int hf_shape(const S64* x, int n) {
  const int k = (n + 7) / 8 - 1;
  const S64* dot;
  const double* dd;
  double sq[1024];
  const bool small = hf_dots(x, n, &dot, &dd);
  // (their x e over |e| in doubles first, -1 where e e is 0; then those within 2^-40 of the most,
  // in order, exactly: of them, the first of the best, as comparing all of them finds it)
  double top = -1;
  if (small) {
    const double *iv = hf_energies.inv[k], *nz = hf_energies.nz[k];
#ifdef DTSD_AVX2  // (the same, 4 at a time)
    const __m256d s8 = _mm256_set1_pd(1.0 / 256), sgn = _mm256_set1_pd(-0.0), one = _mm256_set1_pd(1.0);
    __m256d mx = _mm256_set1_pd(-1.0);
    for (int q = 0; q < 1024; q += 4) {
      const __m256d a = _mm256_andnot_pd(sgn, _mm256_floor_pd(_mm256_mul_pd(_mm256_loadu_pd(dd + q), s8)));
      const __m256d z = _mm256_loadu_pd(nz + q);
      const __m256d s = _mm256_add_pd(_mm256_mul_pd(_mm256_mul_pd(a, _mm256_loadu_pd(iv + q)), z), _mm256_sub_pd(z, one));
      _mm256_storeu_pd(sq + q, s);
      mx = _mm256_max_pd(mx, s);
    }
    alignas(32) double m4[4];
    _mm256_store_pd(m4, mx);
    for (int i = 0; i < 4; i++) top = m4[i] > top ? m4[i] : top;
#else
    for (int q = 0; q < 1024; q++) {
      const double a = fabs(floor(dd[q] * (1.0 / 256)));  // (dot >> 8: exactly)
      sq[q] = a * iv[q] * nz[q] + (nz[q] - 1);
    }
    for (int q = 0; q < 1024; q++) top = sq[q] > top ? sq[q] : top;
#endif
    if (top == 0)  // (all of them 0: the first)
      for (int q = 0; q < 1024; q++)
        if (sq[q] == 0) return q;
  }
  const double lim = top - top / 1099511627776.0;
  int bq = -1;
  S128 bd = 0;  // (the best's x e, squared, and e e: as a ratio)
  S64 be = 1;
  auto take = [&](int q) {
    const S64 ee = hf_energies.e[k][q];
    if (ee <= 0) return;
    const S64 d = dot[q];  // (|x| < 2^40: 2^47 a product, 2^52 the sum)
    const S128 dd2 = (S128)(d >> 8) * (d >> 8);  // (2^88)
    if (bq < 0 || dd2 * be > bd * ee) bd = dd2, be = ee, bq = q;  // (2^88 times 2^19)
  };
  if (small) {  // (those not under lim, in order)
#ifdef DTSD_AVX2
    const __m256d vl = _mm256_set1_pd(lim);
    for (int q = 0; q < 1024; q += 4)
      for (U32 m = (U32)_mm256_movemask_pd(_mm256_cmp_pd(_mm256_loadu_pd(sq + q), vl, _CMP_NLT_UQ)); m; m &= m - 1)
        take(q + __builtin_ctz(m));
#else
    for (int q = 0; q < 1024; q++)
      if (!(sq[q] < lim)) take(q);
#endif
  } else
    for (int q = 0; q < 1024; q++) take(q);
  return bq < 0 ? 0 : bq;
}
// Each group's entry nearest the samples x (n of them; at most 32): its distance (hf_dist()'s),
// md[32], from their dot products with x (hf_dots()'s: dot; small, whether |x| < 2^40) and en,
// x's energy in those units: en - 2^21 sc x e + 2^32 sc^2 e e, the same sum (where small,
// en + 2^21 sc u, u = 2^11 sc e e - x e within 2^54)
static void hf_group_mins(const S64* dot, bool small, int n, S64 sc, S128 en, S128* md) {
  S32 et[1024];  // (the entries' energies over the n values)
  if (!(n & 7)) memcpy(et, hf_energies.e[n / 8 - 1], sizeof et);
  else
    for (int q = 0; q < 1024; q++) {
      et[q] = 0;
      for (int j = 0; j < n; j++) et[q] += HF_VQ[q][j] * HF_VQ[q][j];
    }
  for (int g = 0; g < 32; g++) {
    const S64* const dg = dot + g * 32;
    const S32* const eg = et + g * 32;
    if (small) {  // (sc < 2^23, e e < 2^19)
      S64 m = ~(U64)0 >> 1;
      for (int i = 0; i < 32; i++) {
        const S64 u = shl64((S64)(S32)sc * eg[i], 11) - dg[i];
        m = u < m ? u : m;
      }
      md[g] = en + (S128)m * shl64(sc, 21);
    } else {
      S128 m = 0;
      for (int i = 0; i < 32; i++) {
        const S128 d = en - shl128((S128)dg[i] * sc, 21) + shl128((S128)eg[i] * sc * sc, 32);
        m = !i || d < m ? d : m;
      }
      md[g] = m;
    }
  }
}

// Candidates in the order of their distance (k), ties by index (i): the rank of one, and the one of
// a rank (quickselect, reordering a)
struct KeyIx {
  S128 k;
  int i;
};
static INLINE bool key_less(const KeyIx& a, const KeyIx& b) { return a.k < b.k || (a.k == b.k && a.i < b.i); }
static int rank_of(const KeyIx* a, int n, int at) {
  int r = 0;
  for (int i = 0; i < n; i++) r += key_less(a[i], a[at]);
  return r;
}
static int select_rank(KeyIx* a, int n, int r) {
  if (r <= 1 && n > r) {  // (the first or the second: by one pass)
    int b = 0, s = -1;
    for (int i = 1; i < n; i++)
      if (key_less(a[i], a[b])) s = b, b = i;
      else if (s < 0 || key_less(a[i], a[s])) s = i;
    return a[r ? s : b].i;
  }
  int lo = 0, hi = n - 1;
  while (lo < hi) {
    const KeyIx p = a[lo + (hi - lo) / 2];
    int i = lo, j = hi;
    while (i <= j) {
      while (key_less(a[i], p)) i++;
      while (key_less(p, a[j])) j--;
      if (i <= j) {
        const KeyIx t = a[i];
        a[i++] = a[j];
        a[j--] = t;
      }
    }
    if (r <= j) hi = j;
    else if (r >= i) lo = i;
    else break;
  }
  return a[r].i;
}
// the same of distinct 64-bit keys: the one of rank r (quickselect, reordering a)
static U64 select_u64(U64* a, int n, int r) {
  int lo = 0, hi = n - 1;
  while (lo < hi) {
    const U64 p = a[lo + (hi - lo) / 2];
    int i = lo, j = hi;
    while (i <= j) {
      while (a[i] < p) i++;
      while (p < a[j]) j--;
      if (i <= j) {
        const U64 t = a[i];
        a[i++] = a[j];
        a[j--] = t;
      }
    }
    if (r <= j) hi = j;
    else if (r >= i) lo = i;
    else break;
  }
  return a[r];
}
// how far the best of n candidates beats the next: 0 (within 1/256), 1 (1/128) ... 11 (by 4 times
// or more), 12 by all (where the encoder's search misses, it is nearly always a near tie: 40% of
// `s.2.dts`'s HF VQ groups within 1/256 are the first, 83% from 1/32 to 1/16)
static int key_margin(const KeyIx* a, int n) {
  int b = 0, s = -1;  // (the first two)
  for (int i = 1; i < n; i++)
    if (key_less(a[i], a[b])) s = b, b = i;
    else if (s < 0 || key_less(a[i], a[s])) s = i;
  if (s < 0) return 0;
  const S128 d1 = a[b].k, d2 = a[s].k;
  if (d1 <= 0) return d2 > 0 ? 12 : 0;
  const S128 x = (d2 - d1) / ((d1 >> 8) + 1);  // (256ths of d1)
  int m = 0;
  while (m < 11 && x >= ((S128)1 << m)) m++;
  return m;
}
// the same against a unit u: floor(log2((d2 - d1) / u)) + k, 0 to top - 1 (top where the best is
// exact, 0 where there is no next)
static int key_margin_u(const KeyIx* a, int n, S128 u, int k = 23, int top = 12) {
  int b = 0, s = -1;
  for (int i = 1; i < n; i++)
    if (key_less(a[i], a[b])) s = b, b = i;
    else if (s < 0 || key_less(a[i], a[s])) s = i;
  if (s < 0) return 0;
  const S128 d1 = a[b].k, d2 = a[s].k;
  if (d1 <= 0) return d2 > 0 ? top : 0;
  return imin(near_bucket(d2 - d1, u, k), top - 1);
}

// --- the LFE (the encoder's decimation, as found; in integers) --------------------------------------

// The encoder's LFE samples (64x) are the WAV's LFE channel decimated by a filter near the
// decoder's interpolation prototype (LFE_FIR), then quantized by the scale factor nearest their
// RMS. The prototype's decimation, corrected by a short filter at the LFE's rate that a least
// squares fit learns as the stream goes (LfeFit), makes nearly all of them.
constexpr S64 LFE_G = 290844593;  // the prototype's decimation to the units of v times its scale (Q30; found)
constexpr int LFE_K = 6, LFE_NT = 2 * LFE_K + 1;  // the correction's taps: the decimated samples K before to K after
constexpr int LFE_NP = 7, LFE_NF = LFE_NT + LFE_NP;  // and the last LFE values (a decoded WAV's LFE is their interpolation)
constexpr int LFE_K2 = 2 * LFE_K + 1, LFE_N2 = 2 * LFE_K2 + 1;  // the decimations by 32 (twice the LFE's rate) around a value: 13 before to 13 after
#include "lfefilter.inc"   // (LFE_W2: the DTS-HD MA encoders' filter as a correction of those, Q30; tools/lfefilter.py)
#include "lfefilterd.inc"  // (LFE_W2D: FFmpeg's decodes' the same, its LFE interpolation undone; tools/lfefilter.py -d)

constexpr int LFE_Q = 23;  // the LFE's units: 2^-LFE_Q of those of a value times its step (its steps exact in them)

// the WAV channel c decimated by the prototype, the window ending at sample end, in the units
// of an LFE value times its scale (lfe_scale()); the WAV's samples to the core's up (2: every other
// sample, an LFE channel having nothing above the core's band to alias). (The samples within 2^23:
// |acc| < 2^60, in 64 bits; the result within 2^50)
static S64 lfe_decim(const WavIn& w, int c, S64 end, int up) {
  S32 buf[1024];
  const S32* const s = w.span(c, end - 512 * up, 512 * up, buf);
  S64 acc = 0;
  if (up == 1)
    for (int m = 0; m < 512; m++) acc += (S64)LFE_FIR[m] * s[m];
  else
    for (int m = 0; m < 512; m++) acc += (S64)LFE_FIR[m] * s[up * m];
  return (S64)(((S128)acc * LFE_G) >> (60 - LFE_Q));
}
// the LFE scale factor i's step (FFmpeg's: the 7-bit table's entry times the quantizer's 0.035,
// times 16, in 2^-LFE_Q, 0 or 2^22 to 2^46): an LFE value v is v times it. (Exactly: the encoder
// quantizes by the step itself, and a quiet LFE's, of an LFE value or a few, is no integer)
static INLINE S64 lfe_scale(int i) { return (S64)4697620 * SF7[i & 127]; }
// and as FFmpeg's decoder has it, rounded to an integer (a decoded WAV's LFE is its values times it)
static INLINE S64 lfe_scale_dec(int i) { return (((S64)4697620 * SF7[i & 127] + (1 << 22)) >> 23) << LFE_Q; }

// floor(128 log2(a)), a > 0 (the bit allocations' staircase's energies: model_smp.inc): the top 31
// bits' place, and the fraction by the thresholds 2^30 2^(k/128), k 1-127, rounded up (exactly)
static const U32 LOG2_128[127] = {
    1079572136, 1085434106, 1091327906, 1097253709, 1103211688, 1109202018, 1115224876, 1121280436,
    1127368878, 1133490380, 1139645120, 1145833281, 1152055042, 1158310587, 1164600099, 1170923762,
    1177281763, 1183674286, 1190101521, 1196563654, 1203060876, 1209593378, 1216161350, 1222764986,
    1229404479, 1236080024, 1242791816, 1249540053, 1256324932, 1263146652, 1270005413, 1276901417,
    1283834866, 1290805962, 1297814911, 1304861917, 1311947189, 1319070932, 1326233357, 1333434673,
    1340675091, 1347954824, 1355274086, 1362633090, 1370032053, 1377471192, 1384950724, 1392470869,
    1400031848, 1407633883, 1415277196, 1422962011, 1430688554, 1438457051, 1446267731, 1454120822,
    1462016554, 1469955159, 1477936870, 1485961921, 1494030548, 1502142986, 1510299474, 1518500250,
    1526745557, 1535035634, 1543370726, 1551751076, 1560176931, 1568648538, 1577166144, 1585730000,
    1594340357, 1602997468, 1611701585, 1620452966, 1629251865, 1638098542, 1646993255, 1655936265,
    1664927835, 1673968229, 1683057711, 1692196548, 1701385008, 1710623360, 1719911876, 1729250827,
    1738640488, 1748081134, 1757573042, 1767116489, 1776711757, 1786359126, 1796058880, 1805811302,
    1815616679, 1825475298, 1835387448, 1845353420, 1855373507, 1865448002, 1875577200, 1885761399,
    1896000896, 1906295994, 1916646993, 1927054196, 1937517910, 1948038441, 1958616097, 1969251188,
    1979944028, 1990694928, 2001504205, 2012372174, 2023299156, 2034285471, 2045331440, 2056437387,
    2067603639, 2078830522, 2090118366, 2101467502, 2112878263, 2124350983, 2135885999};
// A number m 2^e (m 0, or |m| in [2^61, 2^62)), as the fit computes: the same everywhere
struct Sf {
  S64 m;
  int e;
};
static INLINE S32 fine_log2(U128 a) {
  const int L = bitlen128(a) - 1;
  const U32 m = (U32)(L >= 30 ? a >> (L - 30) : a << (30 - L));
  int f = 0;
  for (int k = 64; k; k >>= 1)
    if (f + k <= 127 && m >= LOG2_128[f + k - 1]) f += k;
  return 128 * L + f;
}
static Sf sf_make(S128 m, int e) {
  Sf r = {0, 0};
  if (!m) return r;
  const int neg = m < 0;
  U128 a = neg ? (U128)0 - (U128)m : (U128)m;
  const int sh = bitlen128(a) - 62;
  if (sh > 0) a >>= sh;
  else a <<= -sh;
  r.m = neg ? -(S64)a : (S64)a;
  r.e = e + sh;
  return r;
}
static INLINE Sf sf_mul(Sf a, Sf b) { return sf_make((S128)a.m * b.m, a.e + b.e); }
static INLINE Sf sf_div(Sf a, Sf b) { return b.m ? sf_make(shl128(a.m, 64) / b.m, a.e - b.e - 64) : Sf{0, 0}; }
static Sf sf_add(Sf a, Sf b) {
  if (!a.m) return b;
  if (!b.m) return a;
  if (a.e < b.e) {
    const Sf t = a;
    a = b;
    b = t;
  }
  const int d = a.e - b.e;
  const S128 x = shl128(a.m, 62), y = d < 124 ? shl128(b.m, 62) >> d : 0;
  return sf_make(x + y, a.e - 62);
}
static INLINE Sf sf_sub(Sf a, Sf b) {
  b.m = -b.m;
  return sf_add(a, b);
}
// g less the sum of x[k] y[k], k < n: each product (124 bits) and g floored at 2^-7 of the
// largest's scale, summed, then rounded once (exact to 2^-117 of the largest term, each)
static Sf sf_dot_sub(Sf g, const Sf* x, const Sf* y, int n) {
  constexpr int NONE = -(1 << 30);
  int top = g.m ? g.e - 62 : NONE;
  for (int k = 0; k < n; k++)
    if (x[k].m && y[k].m && x[k].e + y[k].e > top) top = x[k].e + y[k].e;
  if (top == NONE) return Sf{0, 0};
  const int A = top + 7;
  auto down = [](S128 v, int d) { return d >= 127 ? (v < 0 ? (S128)-1 : 0) : v >> d; };
  S128 acc = g.m ? down(shl128(g.m, 62), A - (g.e - 62)) : 0;
  for (int k = 0; k < n; k++)  // (a product of 0: 0, past every shift)
    acc -= down((S128)x[k].m * y[k].m, A - (x[k].m && y[k].m ? x[k].e + y[k].e : NONE));
  return sf_make(acc, A);
}
// a times 2^q as an integer (floored), within +-2^62
static S64 sf_int(Sf a, int q) {
  const int e = a.e + q;
  if (!a.m) return 0;
  if (e >= 1) return a.m > 0 ? (S64)1 << 62 : -((S64)1 << 62);
  return e <= -63 ? (a.m < 0 ? -1 : 0) : a.m >> -e;
}

// The WAV past its end (wav.inc's at()): the core runs 512 samples ahead of its WAV (a 96 kHz WAV's
// 64-band analysis 1024), so the analysis of its last frame looks past the WAV's end, at what its
// encoder had there; 0s there miss three quarters of that frame's indexes. Each channel's
// continuation instead: the linear prediction of order WAV_LPC fitted to its last WAV_FIT samples
// (their autocorrelation under a half parabola, the newest weighted most, the windowed samples at 25
// bits, a 10000th of white noise in it; Levinson's recursion in the soft float), run on for WAV_EXT
// samples: each channel's fit where its continuation is first read, the continuation as far as it
// is read (where the model takes it or 0s: model_smp.inc's smp_subframe()).
constexpr int WAV_LPC = 64, WAV_FIT = 4096, WAV_EXT = 2048;
constexpr int WAV_PAST = 192;  // (and an index whose window lies past the end by more, without the audio: model_smp.inc's past_end())
static void wav_extend(WavIn& w) {
  if (w.ext || !w.ok || w.got != w.nsamp) return;
  w.ext = (S32*)xmalloc(sizeof(S32) * (size_t)w.nch * WAV_EXT);
  memset(w.ext, 0, sizeof(S32) * (size_t)w.nch * WAV_EXT);
  w.ext_n = WAV_EXT;
  for (int c = 0; c < WAV_CH; c++) w.ext_done[c] = -1;
}
// (each channel's fit as its continuation is first read, then the continuation as far as it is
// read, 256 samples at a time: as making all of it at once)
void WavIn::ext_more(int c, S64 kneed) const {
  const WavIn& w = *this;
  static_assert(WAV_FIT <= WAV_WIN, "the samples fitted are in the window");
  const S64 N = (S64)w.nsamp, n = N < WAV_FIT ? N : WAV_FIT;  // (those fitted: the ring has them)
  S64* const q = ext_q[c];  // (the predictor, Q30: x[t] by sum q_i x[t - i])
  S64& qa = ext_qa[c];      // (the sum of |q_i|, where under 2^39: the sums in 64 bits)
  if (ext_done[c] < 0) {
    ext_done[c] = 0;
    for (int i = 0; i <= WAV_LPC; i++) q[i] = 0;
    qa = 0;
    do {
      if (n <= 2 * WAV_LPC) break;
      // (not on the coroutine's stack: the windowed samples, within 2^48, shifted down to 25 bits
      // where longer, so the products' sums are within 2^62; after RP 0s, and 0s to a multiple
      // of 4 after them, so every lag's sum runs over the same k)
      constexpr int RP = 68;  // (the lags made, 4 at a time: those past WAV_LPC not taken)
      const int n4 = ((int)n + 3) & ~3;
      S64* const xb = (S64*)xmalloc(sizeof(S64) * (RP + WAV_FIT + 4));
      S64* const xs = xb + RP;
      for (int k = 0; k < RP; k++) xb[k] = 0;
      S64 vmax = 0;
      for (int k = 0; k < (int)n; k++) {
        xs[k] = (S64)w.at(c, N - n + k) * (k + 1) * (2 * n - k);  // (|x| <= 2^23: within 2^48)
        vmax |= xs[k] < 0 ? ~xs[k] : xs[k];
      }
      for (int k = (int)n; k < n4; k++) xs[k] = 0;
      const int sh = vmax ? imax(64 - __builtin_clzll((U64)vmax) - 25, 0) : 0;
      for (int k = 0; k < (int)n; k++) xs[k] >>= sh;
      S64 r[RP];
      for (int l = 0; l < RP; l += 4) {
#ifdef DTSD_AVX2  // (the low 32 bits of each lane signed: the samples, as they are within 2^25)
        __m256i a0 = _mm256_setzero_si256(), a1 = a0, a2 = a0, a3 = a0;
        for (int k = 0; k < n4; k += 4) {
          const __m256i x = _mm256_loadu_si256((const __m256i*)(xs + k));
          a0 = _mm256_add_epi64(a0, _mm256_mul_epi32(x, _mm256_loadu_si256((const __m256i*)(xs + k - l))));
          a1 = _mm256_add_epi64(a1, _mm256_mul_epi32(x, _mm256_loadu_si256((const __m256i*)(xs + k - l - 1))));
          a2 = _mm256_add_epi64(a2, _mm256_mul_epi32(x, _mm256_loadu_si256((const __m256i*)(xs + k - l - 2))));
          a3 = _mm256_add_epi64(a3, _mm256_mul_epi32(x, _mm256_loadu_si256((const __m256i*)(xs + k - l - 3))));
        }
        alignas(32) S64 t[16];
        _mm256_store_si256((__m256i*)t, a0), _mm256_store_si256((__m256i*)(t + 4), a1);
        _mm256_store_si256((__m256i*)(t + 8), a2), _mm256_store_si256((__m256i*)(t + 12), a3);
        for (int j = 0; j < 4; j++) r[l + j] = t[4 * j] + t[4 * j + 1] + t[4 * j + 2] + t[4 * j + 3];
#else
        for (int j = 0; j < 4; j++) {
          S64 a = 0;
          for (int k = 0; k < n4; k++) a += xs[k] * xs[k - l - j];
          r[l + j] = a;
        }
#endif
      }
      free(xb);
      if (r[0] <= 0) break;
      Sf R[WAV_LPC + 1], a[WAV_LPC + 1], t[WAV_LPC + 1];
      for (int l = 0; l <= WAV_LPC; l++) R[l] = sf_make(r[l], 0);
      R[0] = sf_add(R[0], sf_make(r[0] / 10000, 0));
      const Sf one = sf_make(1, 0);
      a[0] = one;
      Sf err = R[0];
      int m = 1;
      for (; m <= WAV_LPC; m++) {
        Sf acc = R[m];
        for (int i = 1; i < m; i++) acc = sf_add(acc, sf_mul(a[i], R[m - i]));
        Sf k = sf_div(acc, err);
        k.m = -k.m;
        for (int i = 1; i < m; i++) t[i] = sf_add(a[i], sf_mul(k, a[m - i]));
        for (int i = 1; i < m; i++) a[i] = t[i];
        a[m] = k;
        err = sf_mul(err, sf_sub(one, sf_mul(k, k)));
        if (err.m <= 0) break;
      }
      if (m <= WAV_LPC) break;  // (not a fit: 0s)
      for (int i = 1; i <= WAV_LPC; i++) {
        Sf u = a[i];
        u.m = -u.m;
        q[i] = sf_int(u, 30);
        qa = qa < (S64)1 << 39 ? qa + (q[i] < 0 ? -q[i] : q[i]) : qa;
      }
      q[0] = 1;  // (a fit)
    } while (0);
  }
  S32* const e = w.ext + (size_t)c * WAV_EXT;
  const int k0 = ext_done[c], k1 = (int)(kneed + 256 < WAV_EXT ? (kneed + 256) & ~255 : WAV_EXT);
  if (k1 <= k0) return;
  ext_done[c] = k1;
  if (!q[0]) return;  // (no fit: 0s, as made)
  S32 hb[WAV_LPC + WAV_EXT];  // (the samples before the first to make, then those made)
  for (int i = 0; i < WAV_LPC; i++) {
    const S64 s = N + k0 - WAV_LPC + i;
    hb[i] = s < N ? w.at(c, s) : e[s - N];
  }
  for (int k = k0; k < k1; k++) {
    const S32* const h = hb + WAV_LPC + (k - k0);
    S64 v;
    if (qa < (S64)1 << 39) {  // (|x| <= 2^23)
      S64 p = 0;
      for (int i = 1; i <= WAV_LPC; i++) p += q[i] * h[-i];
      v = (p + ((S64)1 << 29)) >> 30;
    } else {
      S128 p = 0;
      for (int i = 1; i <= WAV_LPC; i++) p += (S128)q[i] * h[-i];
      v = (S64)((p + ((S128)1 << 29)) >> 30);
    }
    e[k] = (S32)(v < -(1 << 23) ? -(1 << 23) : v > (1 << 23) - 1 ? (1 << 23) - 1 : v);
    hb[WAV_LPC + (k - k0)] = e[k];
  }
}

// The correction's least squares fit: the decimated samples around each LFE sample (f, NF of
// them: LFE_NT, or LFE_NF with the last LFE values, as a decoded WAV's LFE is the decoder's
// interpolation of the values, 8 of them in each of its samples, so with the last ones the fit
// sees the next alone) against the LFE value times its scale (y), each weighted by 1 over its
// scale squared (what the coder pays for is the error in steps), the older ones forgotten (by
// 2^-FS a subframe), ridged toward no correction. Plain data, zeroed (the model's memory): add()
// each sample, refit() each subframe. (K0: the tap the correction's identity is, ridged toward;
// -1 none, ridged toward 0.)
template <int NF, int K0 = LFE_K, int FS = 5>
struct LfeFit {
  S128 G[NF][NF], c[NF];  // the sums of f f' and f y
  S64 w[NF];              // the correction (Q30), less its identity (2^30 at K0): 0s the identity

  S64 predict(const S64* f) const {
    S128 p = K0 >= 0 ? shl128(f[K0 >= 0 ? K0 : 0], 30) : 0;
    for (int k = 0; k < NF; k++) p += (S128)w[k] * f[k];
    return (S64)(p >> 30);
  }
  // (st: y's step, 0 (nothing to fit) or 2^22 up: f and y divided by it (Q20, by its reciprocal
  // 2^82 / st, within 2^60) weight each by 1 / st^2; f within 2^52, so within 2^50 in steps, the
  // terms within 2^80)
  void add(const S64* f, S64 y, S64 st) {
    if (st <= 0) return;
    const S64 iv = (S64)(((S128)1 << 82) / (st > (S64)1 << 22 ? st : (S64)1 << 22));
    S64 fs[NF];
    for (int i = 0; i < NF; i++) fs[i] = (S64)((S128)f[i] * iv >> 62);
    const S64 ys = (S64)((S128)y * iv >> 62);
    for (int i = 0; i < NF; i++) {
      for (int j = 0; j <= i; j++) G[i][j] += (S128)fs[i] * fs[j] >> 20;
      c[i] += (S128)fs[i] * ys >> 20;
    }
  }
  // (G + r I) w = c + r e_K by LDL', r the mean diagonal / 2^23 (w as it was where that fails): L
  // and E = L D by rows, row i's k < i at i (i - 1) / 2 + k, each entry a dot product of the rows
  // before it (sf_dot_sub()), L's by D's reciprocal
  void refit() {
    S128 tr = 0;
    for (int i = 0; i < NF; i++) {
      for (int j = 0; j <= i; j++) G[i][j] -= G[i][j] >> FS;
      c[i] -= c[i] >> FS;
      tr += G[i][i];
    }
    if (tr <= 0) return;
    const Sf r = sf_make(tr / NF / ((S128)1 << 23) + 1, 0);
    constexpr int NT = NF * (NF - 1) / 2 + 1;
    Sf L[NT], E[NT], iD[NF], z[NF], t[NF];
    for (int j = 0; j < NF; j++) {
      const Sf* const Lj = L + j * (j - 1) / 2;
      const Sf d = sf_dot_sub(sf_add(sf_make(G[j][j], 0), r), E + j * (j - 1) / 2, Lj, j);
      if (d.m <= 0) return;
      iD[j] = sf_div(sf_make(1, 0), d);
      for (int i = j + 1; i < NF; i++) {
        Sf* const Ei = E + i * (i - 1) / 2;
        Ei[j] = sf_dot_sub(sf_make(G[i][j], 0), Ei, Lj, j);
        L[i * (i - 1) / 2 + j] = sf_mul(Ei[j], iD[j]);
      }
    }
    for (int i = 0; i < NF; i++) {  // L z = b, then D, then L' w = z
      Sf b = sf_make(c[i], 0);
      if (i == K0) b = sf_add(b, r);
      z[i] = sf_dot_sub(b, L + i * (i - 1) / 2, z, i);
    }
    for (int i = 0; i < NF; i++) z[i] = sf_mul(z[i], iD[i]);
    for (int i = NF - 1; i >= 0; i--) {
      for (int k = i + 1; k < NF; k++) t[k] = L[k * (k - 1) / 2 + i];
      z[i] = sf_dot_sub(z[i], t + i + 1, z + i + 1, NF - 1 - i);
    }
    for (int i = 0; i < NF; i++) w[i] = sf_int(z[i], 30) - (i == K0 ? (S64)1 << 30 : 0);
  }
};

// --- the alignment (c: found once, the header keeps it; in doubles) ---------------------------------

// A core's first frames, for the alignment: the decoder's plain subband samples of bands 0-7 (those
// without ADPCM) of a block of it, by channel
constexpr int PROBES_MAX = 64;  // (at most: call.inc's PROBES)
struct Probe {
  S64 t;  // the block
  S32 y[NCH][8];
  U8 ok[NCH][8];
};

// the double-precision analysis of bands 0-7 (as analyze_m()'s, unscaled), M bands, of the 16 M
// samples from s (the window's first: those before the block's place), by fir and the cosines cq
template <int M>
static void analyze_d(const double* s, const double* fir, const double (*cq)[M], double* x) {
  constexpr int L = 16 * M, H = M / 2;
  double acc[2 * M];
  for (int i = 0; i < 2 * M; i++) acc[i] = 0;
  for (int k = 0; k < L; k += 2 * M)  // (each acc[] by m in order: analyze_m()'s sums)
    for (int i = 0; i < 2 * M; i++) acc[i] += s[k + i] * fir[k + i];
  for (int k = H; k < M; k++) acc[k] -= acc[M - 1 - k];
  for (int k = M; k < M + H; k++) acc[k] += acc[3 * M - 1 - k];
  double r[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 0; i < M; i++)
    for (int b = 0; b < 8; b++) r[b] += acc[i + H] * cq[b][i];
  for (int b = 0; b < 8; b++) x[b] = r[b];
}

// The WAV's channel and offset of each of the core's nch channels, from the probe blocks pr (n of
// them), the WAV's samples to the core's up (its analysis's: analyze()): the offset and channel
// whose analysis correlates best with core channel 0 over offsets -1024..1536 (times up), then each
// other channel's best at that offset; a channel whose best correlation is under 0.9 is none. The
// WAV's samples the probes need must be in its window. (The decoders' usual delays first: where
// one correlates all but perfectly, it; where by 0.99 or more, the best of the offsets within 32
// of it, where that is inside them. Else the offsets by the 16 probes of most energy in core
// channel 0 first, the 64 places best by them then by all the probes: the best of those, as a
// search of all of them by all the probes finds it, where it is among them.)
static Bind find_bind(const WavIn& w, const Probe* pr, int n, int nch, int filts, int up, double* corr0) {
  Bind b;
  b.none();
  b.up = (S8)up;
  *corr0 = 0;
  if (!w.ok || n <= 0) return b;
  constexpr int NB1 = 16, NC = 64, SPAN = (2560 + 1) * 2 + 1024;  // (a probe's samples over the offsets, up 2)
  const int M = 32 * up, L = 16 * M;
  const S64 olo = -1024 * up, ohi = 1536 * up;
  const int span = (int)(ohi - olo) + L;
  static double fir[1024], cq32[8][32], cq64[8][64], seg[PROBES_MAX * SPAN];
  for (int m = 0; m < 512 * up; m++) fir[m] = (double)(up == 2 ? FIR_64 : filts ? FIR_PERFECT : FIR_NONPERFECT)[m];
  for (int k = 0; k < 8; k++) {
    for (int i = 0; i < 32; i++) cq32[k][i] = (double)COSQ[k][i];
    for (int i = 0; i < 64; i++) cq64[k][i] = (double)COSQ64[k][i];
  }
  if (n > PROBES_MAX) n = PROBES_MAX;
  auto fetch = [&](int wc, S64 o, int len, double* d) {  // (samples o.. of WAV channel wc: at()'s, those
    static S32 tmp[SPAN];                                  // read in a row from the ring)
    const int k0 = (int)clip(-o, 0, len), k1 = (int)clip((S64)w.got - o, k0, len), nk = k1 - k0;
    for (int k = 0; k < k0; k++) d[k] = 0;  // (before the WAV)
    const S32* const q = w.span(wc, o + k0, nk, tmp);
    double* const dk = d + k0;
    for (int k = 0; k < nk; k++) dk[k] = q[k];
    if (w.ext_on)  // (past what's read: its continuation, where made)
      for (int k = k1; k < len; k++) d[k] = w.at(wc, o + k);
    else
      for (int k = k1; k < len; k++) d[k] = 0;
  };
  int seg_wc = -1;
  auto load = [&](int wc) {  // (the probes' windows of WAV channel wc over all the offsets)
    if (seg_wc == wc) return;
    seg_wc = wc;
    for (int i = 0; i < n; i++) fetch(wc, (S64)M * pr[i].t + olo - L, span, seg + (size_t)i * span);
  };
  auto sums = [&](int ch, int i, const double* x, double* sm) {  // (probe i's to channel ch's sums)
    for (int k = 0; k < 8; k++)
      if (pr[i].ok[ch][k]) sm[0] += x[k] * pr[i].y[ch][k], sm[1] += x[k] * x[k], sm[2] += (double)pr[i].y[ch][k] * pr[i].y[ch][k];
  };
  auto anal = [&](const double* s, double* x) {
    if (up == 2) analyze_d<64>(s, fir, cq64, x);
    else analyze_d<32>(s, fir, cq32, x);
  };
  auto ratio = [](const double* sm) { return sm[1] > 0 && sm[2] > 0 ? sm[0] / sqrt(sm[1] * sm[2]) : 0; };
  auto corr = [&](int ch, S64 off, const int* pi, int np) {  // (by the loaded windows)
    double sm[3] = {0, 0, 0}, x[8];
    for (int j = 0; j < np; j++) {
      const int i = pi ? pi[j] : j;
      anal(seg + (size_t)i * span + (off - olo), x);
      sums(ch, i, x, sm);
    }
    return ratio(sm);
  };
  auto corr_at = [&](int wc, S64 off, int c0, int c1, double* r) {  // (channels c0..c1 - 1's at off,
    static double win[1024];                                        // by the probes' windows there)
    double sm[NCH][3], x[8];
    for (int ch = c0; ch < c1; ch++) sm[ch][0] = sm[ch][1] = sm[ch][2] = 0;
    for (int i = 0; i < n; i++) {
      fetch(wc, (S64)M * pr[i].t + off - L, L, win);
      anal(win, x);
      for (int ch = c0; ch < c1; ch++) sums(ch, i, x, sm[ch]);
    }
    for (int ch = c0; ch < c1; ch++) r[ch] = ratio(sm[ch]);
  };
  // the probes of most energy in channel 0, in their order
  int p1[NB1], n1 = 0;
  if (n > NB1) {
    double e[PROBES_MAX];
    for (int i = 0; i < n; i++) {
      e[i] = 0;
      for (int k = 0; k < 8; k++)
        if (pr[i].ok[0][k]) e[i] += (double)pr[i].y[0][k] * pr[i].y[0][k];
    }
    U8 pick[PROBES_MAX] = {0};
    for (int j = 0; j < NB1; j++) {
      int bi = -1;
      for (int i = 0; i < n; i++)
        if (!pick[i] && (bi < 0 || e[i] > e[bi])) bi = i;
      pick[bi] = 1;
    }
    for (int i = 0; i < n; i++)
      if (pick[i]) p1[n1++] = i;
  }
  double best = -2;
  int bw = 0;
  S64 bo = 0;
  // (the decoders' usual delays first, in the search's order: where one correlates all but
  // perfectly, it is the best)
  static const int USUAL[] = {0, 256, 512, 1024};
  for (int wc = 0; wc < w.nch; wc++)
    for (int u = 0; u < 4; u++) {
      double r;
      corr_at(wc, (S64)USUAL[u] * up, 0, 1, &r);
      if (r > best) best = r, bo = (S64)USUAL[u] * up, bw = wc;
    }
  if (best >= 0.999) n1 = -1;  // (no search)
  else if (best >= 0.99) {  // (the offsets within 32 (times up) of it, by all the probes: their best
    const S64 c = bo, r0 = 32 * up;  // where it is inside them; else the search)
    const int ln = L + 2 * (int)r0 + 1;  // (each probe's samples over them)
    static double loc[PROBES_MAX * (1024 + 2 * 64 + 1)];
    S64 lb = c;
    double lr = best;
    for (int i = 0; i < n; i++) fetch(bw, (S64)M * pr[i].t + c - r0 - L, ln, loc + (size_t)i * ln);
    auto lcorr = [&](S64 off, const int* pi, int np) {
      double sm[3] = {0, 0, 0}, x[8];
      for (int j = 0; j < np; j++) {
        const int i = pi ? pi[j] : j;
        anal(loc + (size_t)i * ln + (off - c + r0), x);
        sums(0, i, x, sm);
      }
      return ratio(sm);
    };
    // (the 3 best by the 16 probes of most energy, where there are more; of those and it, the best
    // by all of them, as of all the offsets in their order)
    S64 tk[3] = {c, c, c};
    double tr[3] = {-3, -3, -3};
    for (S64 off = c - r0; off <= c + r0 && n1 > 0; off++) {
      if (off == c) continue;
      const double r = lcorr(off, p1, n1);
      int k = 3;
      while (k > 0 && r > tr[k - 1]) k--;
      for (int z = 2; z > k; z--) tr[z] = tr[z - 1], tk[z] = tk[z - 1];
      if (k < 3) tr[k] = r, tk[k] = off;
    }
    for (S64 off = c - r0; off <= c + r0; off++) {
      if (off == c || (n1 > 0 && off != tk[0] && off != tk[1] && off != tk[2])) continue;
      const double r = lcorr(off, nullptr, n);
      if (r > lr) lr = r, lb = off;
    }
    if (lb > c - r0 && lb < c + r0) best = lr, bo = lb, n1 = -1;
    else best = -2;
  } else best = -2;
  struct Cand {
    double r;
    int wc;
    S64 off;
  } cd[NC];
  int nc = 0;
  for (int wc = 0; wc < w.nch && n1 >= 0; wc++) {
    load(wc);
    for (S64 off = olo; off <= ohi; off++) {
      if (!n1) {
        const double r = corr(0, off, nullptr, n);
        if (r > best) best = r, bo = off, bw = wc;
        continue;
      }
      const double r = corr(0, off, p1, n1);
      if (nc < NC) cd[nc++] = Cand{r, wc, off};
      else {
        int lo = 0;
        for (int k = 1; k < NC; k++)
          if (cd[k].r < cd[lo].r) lo = k;
        if (r > cd[lo].r) cd[lo] = Cand{r, wc, off};
      }
    }
  }
  if (n1 > 0) {  // (by all the probes, in the search's order: its first best)
    for (int k = 1; k < nc; k++)
      for (int z = k; z > 0 && (cd[z - 1].wc > cd[z].wc || (cd[z - 1].wc == cd[z].wc && cd[z - 1].off > cd[z].off)); z--) {
        const Cand t = cd[z];
        cd[z] = cd[z - 1], cd[z - 1] = t;
      }
    for (int k = 0; k < nc; k++) {
      load(cd[k].wc);
      const double r = corr(0, cd[k].off, nullptr, n);
      if (r > best) best = r, bo = cd[k].off, bw = cd[k].wc;
    }
  }
  *corr0 = best;
  if (best < 0.9) return b;
  b.off = (S32)bo;
  b.map[0] = (S8)bw;
  double bc[NCH], r[NCH];
  int bwc[NCH];
  for (int ch = 1; ch < nch; ch++) bc[ch] = -2, bwc[ch] = -1;
  for (int wc = 0; wc < w.nch && nch > 1; wc++) {
    corr_at(wc, bo, 1, nch, r);
    for (int ch = 1; ch < nch; ch++)
      if (r[ch] > bc[ch]) bc[ch] = r[ch], bwc[ch] = wc;
  }
  for (int ch = 1; ch < nch; ch++)
    if (bc[ch] >= 0.9) b.map[ch] = (S8)bwc[ch];
  return b;
}
