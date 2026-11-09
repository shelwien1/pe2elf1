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
// whether scale factor i of table tab (nt entries) has the value of the one below (1), above (2),
// both (3), or neither (0): the encoder's choice among such is not the RMS rule's
static INLINE int sf_dup(const U32* tab, int nt, int i) {
  return (i > 0 && tab[i - 1] == tab[i]) | (i + 1 < nt && tab[i + 1] == tab[i]) << 1;
}

static INLINE S32 clip23(S64 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : (S32)a; }
static INLINE S64 rshift(S64 a, int bits) { return bits > 0 ? (a + (1LL << (bits - 1))) >> bits : a; }
typedef __int128 S128;
typedef unsigned __int128 U128;
// x times 2^k (C++11 leaves the left shift of a negative value undefined)
static INLINE S64 shl64(S64 x, int k) { return (S64)((U64)x << k); }
static INLINE S128 shl128(S128 x, int k) { return (S128)((U128)x << k); }

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
  S8 up;
  S8 map[NCH];   // each core channel's WAV channel (-1 none)
  S8 map2[NCH];  // the second of its mix (-1 none)
  S32 g1[NCH], g2[NCH];  // their gains (Q23)
  S8 lfe;        // the LFE's (-1 none)
  void none() {
    off = 0;
    up = 1;
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

template <int M>
static void analyze_m(const WavIn& w, int c, S64 o, const S64* fir, const S64 (*cq)[M], S64* x, int nb) {
  constexpr int L = 16 * M, H = M / 2;
  S32 buf[L];
  const S32* sp = w.span(c, o - L, L, buf);
  S32 mx = 0;
  for (int m = 0; m < L; m++) mx |= sp[m] < 0 ? ~sp[m] : sp[m];
  S128 acc[2 * M];
  S64 a[M];
  if (mx < (1 << 24)) {  // (samples of 25 bits at most: every sum in 64 bits, the halves' products, the same)
    const int t = fir == FIR_64 ? 2 : fir == FIR_PERFECT ? 1 : 0;
    const S32 *fh = split_tabs.fh[t], *fl = split_tabs.fl[t];
    S64 sh[2 * M], sl[2 * M];
    for (int i = 0; i < 2 * M; i++) sh[i] = sl[i] = 0;
    for (int k = 0; k < L; k += 2 * M)
      for (int i = 0; i < 2 * M; i++) sh[i] += (S64)sp[k + i] * fh[k + i], sl[i] += (S64)sp[k + i] * fl[k + i];
    for (int i = 0; i < 2 * M; i++) acc[i] = (S128)sh[i] * (1 << 20) + sl[i];
  } else {
    for (int i = 0; i < 2 * M; i++) acc[i] = 0;
    for (int m = 0; m < L; m++) acc[m & (2 * M - 1)] += (S128)sp[m] * fir[m];
  }
  for (int k = H; k < M; k++) acc[k] -= acc[M - 1 - k];
  for (int k = M; k < M + H; k++) acc[k] += acc[3 * M - 1 - k];
  S64 amax = 0;
  for (int i = 0; i < M; i++) a[i] = (S64)(acc[i + H] >> 20), amax |= a[i] < 0 ? ~a[i] : a[i];  // (2^-24 of a sample)
  if (amax < ((S64)1 << 46)) {  // (a = h 2^23 + l: the products in 56 bits, 64 of them in 62)
    const S32(*chp)[M] = (const S32(*)[M])(M == 64 ? &split_tabs.ch64[0][0] : &split_tabs.ch32[0][0]);
    const S32(*clp)[M] = (const S32(*)[M])(M == 64 ? &split_tabs.cl64[0][0] : &split_tabs.cl32[0][0]);
    S32 ah[M], al[M];
    for (int i = 0; i < M; i++) ah[i] = (S32)(a[i] >> 23), al[i] = (S32)(a[i] & 0x7FFFFF);
    for (int b = 0; b < nb; b++) {
      S64 hh = 0, hl = 0, lh = 0, ll = 0;
      for (int i = 0; i < M; i++) {
        hh += (S64)ah[i] * chp[b][i], hl += (S64)ah[i] * clp[b][i];
        lh += (S64)al[i] * chp[b][i], ll += (S64)al[i] * clp[b][i];
      }
      const S128 r = (S128)hh * ((S64)1 << 54) + (S128)hl * (1 << 23) + (S128)lh * ((S64)1 << 31) + ll;
      x[b] = (S64)((r + ((S128)1 << (84 - AQ_F))) >> (85 - AQ_F));  // (2 r: 2^-(24 + 62) to 2^-AQ_F)
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
static void analyze(const WavIn& w, int c, S64 o, int up, const S64* fir, S64* x, int nb = 32) {
  if (up == 2) analyze_m<64>(w, c, o, FIR_64, COSQ64, x, nb);
  else analyze_m<32>(w, c, o, fir, COSQ, x, nb);
}

// the first nb subband samples of core channel ch as the binding b makes them from the WAV
// (analyze()'s): its channel's, or the mix's
static void analyze_bound(const WavIn& w, const Bind& b, int ch, S64 o, const S64* fir, S64* x, int nb = 32) {
  analyze(w, b.map[ch], o, b.up, fir, x, nb);
  if (b.map2[ch] < 0 && b.g1[ch] == BIND_ONE) return;
  S64 x2[64];
  if (b.map2[ch] >= 0) analyze(w, b.map2[ch], o, b.up, fir, x2, nb);
  for (int k = 0; k < nb; k++) {
    const S128 v = (S128)x[k] * b.g1[ch] + (b.map2[ch] >= 0 ? (S128)x2[k] * b.g2[ch] : 0);
    x[k] = (S64)((v + (1 << 22)) >> 23);
  }
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
  const S128 qa = (2 * a + den) / (2 * den);  // floor(a / den + 1/2)
  const S128 r = a - qa * den;                 // -den/2 .. den/2
  *dist = den / 2 - (r < 0 ? -r : r);
  S64 q = (S64)qa, o = r >= 0 ? q + 1 : q - 1;
  if (neg) q = -q, o = -o;
  *alt = o < lo ? lo : o > hi ? hi : o;
  return q < lo ? lo : q > hi ? hi : q;
}
// how near a distance d lies to 0, against a unit u (both in the same units): floor(log2(d / u)) +
// k, in 0..QB_N - 1
static INLINE int near_bucket(S128 d, S128 u, int k) {
  if (u <= 0) return QB_N - 1;
  int e = 0;  // (floor(log2(d / u)), from -k up)
  if (d <= 0) return 0;
  while (e > -k && d < u) d <<= 1, e--;
  while (e < QB_N - 1 - k && d >= 2 * u) u <<= 1, e++;
  return clip(e + k, 0, QB_N - 1);
}

// The scale factor of table tab (nt entries) nearest (linearly) to the RMS of the n values v
// (2^-AQ_F), as dtsreq found the encoder's; *near how near the RMS lies to the midpoint between it
// and the next entry the other way (0 there, SB_N - 1 far)
static int scale_pred(const S64* v, int n, const U32* tab, int nt, int* near) {
  S128 ss = 0;
  if (n <= 0) return *near = SB_N - 1, 0;
  for (int j = 0; j < n; j++) ss += (S128)v[j] * v[j];
  const S128 ms = ss / n;  // 2^-2AQ_F
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

// How far a decoded WAV's values v (n of them, 2^-AQ_F; the ADPCM prediction off) lie from what the
// decoder makes of them at step and scale ss: the indexes nearest them, in lo..hi, dequantized as
// it does (the squared distances' sum, 2^-2AQ_F; by one 64-bit division a value: |v| < 2^40)
static S128 quant_err(const S64* v, int n, S64 ss, S64 lo, S64 hi) {
  S128 e = 0;
  for (int j = 0; j < n; j++) {
    const S64 u = v[j] * 64;  // (Q22, as ss)
    S64 q = (u < 0 ? -((-2 * u + ss) / (2 * ss)) : (2 * u + ss) / (2 * ss));
    q = q < lo ? lo : q > hi ? hi : q;
    const S64 d = v[j] - shl64(dequantize((S32)q, ss), AQ_F);
    e += (S128)d * d;
  }
  return e;
}
// quant_err()'s at one step and scale, value by value, for the searches that try many: the index
// by a reciprocal, then made exact (the same indexes as its divisions)
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
    const S64 u = v * 64, a = u < 0 ? -u : u, t = 2 * a + ss;  // (the index: floor(t / 2ss))
    S64 q = (S64)((double)a * inv + 0.5);
    while (q > 0 && 2 * q * ss > t) q--;
    while (2 * (q + 1) * ss <= t) q++;
    if (u < 0) q = -q;
    q = q < lo ? lo : q > hi ? hi : q;
    *nz |= q != 0;
    const S64 d = v - shl64(clip23(rshift(q * ssh, 22 - shift)), AQ_F);
    return (S128)d * d;
  }
};
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

// The HF VQ code book as the encoder searches it: groups of 32 entries, the group whose mean is
// nearest first, then the entry of the group nearest; the samples x (n, 2^-AQ_F) as 16 / sc times
// them (sc the subband's scale factor), against the entries as they are
struct HfGroups {
  S16 s[32][32];  // each group's sum
};
static HfGroups hf_groups_make(void) {
  HfGroups t;
  for (int g = 0; g < 32; g++)
    for (int j = 0; j < 32; j++) {
      int s = 0;
      for (int i = 0; i < 32; i++) s += HF_VQ[g * 32 + i][j];
      t.s[g][j] = (S16)s;
    }
  return t;
}
static const HfGroups hf_groups = hf_groups_make();
// group g's distance (in 2^(AQ_F + 9) sc units)
static INLINE S128 hf_group_dist(int g, const S64* x, int n, S64 sc) {
  S128 d = 0;
  for (int j = 0; j < n; j++) {
    const S128 e = (S128)x[j] * 512 - shl128((S128)hf_groups.s[g][j] * sc, AQ_F);
    d += e * e;
  }
  return d;
}
// entry q's distance (in 2^(AQ_F + 4) sc units)
static INLINE S128 hf_dist(int q, const S64* x, int n, S64 sc) {
  S128 d = 0;
  for (int j = 0; j < n; j++) {
    const S128 e = (S128)x[j] * 16 - shl128((S128)HF_VQ[q][j] * sc, AQ_F);
    d += e * e;
  }
  return d;
}

// The distance of group g's entry nearest the samples x (as hf_dist()'s), or cap where that is
// more: the samples by jo (n of them, the largest first), each entry's sum left where it reaches
// the nearest's so far (the same)
static S128 hf_group_min(int g, const S64* x, const U8* jo, int n, S64 sc, S128 cap) {
  S128 best = cap;
  for (int i = 0; i < 32; i++) {
    const S8* const e8 = HF_VQ[g * 32 + i];
    S128 d = 0;
    for (int k = 0; k < n && d < best; k++) {
      const int j = jo[k];
      const S128 e = (S128)x[j] * 16 - shl128((S128)e8[j] * sc, AQ_F);
      d += e * e;
    }
    if (d < best) best = d;
  }
  return best;
}

// the entry most nearly a multiple of the samples x (n of them, at most 32): the one whose
// projection leaves the least, (x e)^2 / (e e) the most (the first of equals); its scale is free
struct HfEnergies {
  S32 e[1024][4];  // each entry's energy over its first 8, 16, 24, 32 values
  HfEnergies() {
    for (int q = 0; q < 1024; q++) {
      S32 s = 0;
      for (int j = 0; j < 32; j++) {
        s += HF_VQ[q][j] * HF_VQ[q][j];
        if ((j & 7) == 7) e[q][j >> 3] = s;
      }
    }
  }
};
static const HfEnergies hf_energies;
static int hf_shape(const S64* x, int n) {
  const int k = (n + 7) / 8 - 1;
  int bq = -1;
  S128 bd = 0;  // (the best's x e, squared, and e e: as a ratio)
  S64 be = 1;
  for (int q = 0; q < 1024; q++) {
    const S64 ee = hf_energies.e[q][k];
    if (ee <= 0) continue;
    S64 d = 0;
    for (int j = 0; j < n; j++) d += x[j] * HF_VQ[q][j];  // (|x| < 2^40: 2^47 a product, 2^52 the sum)
    const S128 dd = (S128)(d >> 8) * (d >> 8);  // (2^88)
    if (bq < 0 || dd * be > bd * ee) bd = dd, be = ee, bq = q;  // (2^88 times 2^19)
  }
  return bq < 0 ? 0 : bq;
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
// how far the best of n candidates beats the next: 0 (within 1/16) to 7 (8 times), 8 by all
static int key_margin(const KeyIx* a, int n) {
  int b = 0, s = -1;  // (the first two)
  for (int i = 1; i < n; i++)
    if (key_less(a[i], a[b])) s = b, b = i;
    else if (s < 0 || key_less(a[i], a[s])) s = i;
  if (s < 0) return 0;
  const S128 d1 = a[b].k, d2 = a[s].k;
  if (d1 <= 0) return d2 > 0 ? 8 : 0;
  const S128 x = (d2 - d1) / ((d1 >> 4) + 1);  // (16ths of d1)
  int m = 0;
  while (m < 8 && x >= ((S128)1 << m)) m++;
  return m;
}

// --- the LFE (the encoder's decimation, as found; in integers) --------------------------------------

// The encoder's LFE samples (64x) are the WAV's LFE channel decimated by a filter near the
// decoder's interpolation prototype (LFE_FIR), then quantized by the scale factor nearest their
// RMS. The prototype's decimation, corrected by a short filter at the LFE's rate that a least
// squares fit learns as the stream goes (LfeFit), makes nearly all of them.
constexpr S64 LFE_G = 290844593;  // the prototype's decimation to the units of v times its scale (Q30; found)
constexpr int LFE_K = 6, LFE_NT = 2 * LFE_K + 1;  // the correction's taps: the decimated samples K before to K after
constexpr int LFE_NP = 7, LFE_NF = LFE_NT + LFE_NP;  // and the last LFE values (a decoded WAV's LFE is their interpolation)

// the WAV channel c decimated by the prototype, the window ending at sample end, in the units
// of an LFE value times its scale (lfe_scale()); the WAV's samples to the core's up (2: every other
// sample, an LFE channel having nothing above the core's band to alias)
static S64 lfe_decim(const WavIn& w, int c, S64 end, int up) {
  S128 acc = 0;
  for (int m = 0; m < 512; m++) acc += (S128)LFE_FIR[m] * w.at(c, end - 512 * up + up * m);
  return (S64)((acc * LFE_G) >> 60);
}
// the LFE scale factor i's step (FFmpeg's: the 7-bit table's entry times the quantizer's 0.035,
// times 16): an LFE value v is v times it
static INLINE S64 lfe_scale(int i) { return ((S64)4697620 * SF7[i & 127] + (1 << 22)) >> 23; }

// A number m 2^e (m 0, or |m| in [2^61, 2^62)), as the fit computes: the same everywhere
struct Sf {
  S64 m;
  int e;
};
static INLINE int bitlen128(U128 a) {
  const U64 hi = (U64)(a >> 64), lo = (U64)a;
  return hi ? 128 - __builtin_clzll(hi) : lo ? 64 - __builtin_clzll(lo) : 0;
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
// a times 2^q as an integer (floored), within +-2^62
static S64 sf_int(Sf a, int q) {
  const int e = a.e + q;
  if (!a.m) return 0;
  if (e >= 1) return a.m > 0 ? (S64)1 << 62 : -((S64)1 << 62);
  return e <= -63 ? (a.m < 0 ? -1 : 0) : a.m >> -e;
}

// The correction's least squares fit: the decimated samples around each LFE sample (f, NF of
// them: LFE_NT, or LFE_NF with the last LFE values, as a decoded WAV's LFE is the decoder's
// interpolation of the values, 8 of them in each of its samples, so with the last ones the fit
// sees the next alone) against the LFE value times its scale (y), the older ones forgotten (by
// 1/128 a frame), ridged toward no correction. Plain data: init(), then add() each sample,
// refit() each frame.
template <int NF>
struct LfeFit {
  S128 G[NF][NF], c[NF];  // the sums of f f' and f y
  S64 w[NF];              // the correction (Q30)

  void init() {
    memset((void*)this, 0, sizeof *this);
    w[LFE_K] = (S64)1 << 30;
  }
  S64 predict(const S64* f) const {
    S128 p = 0;
    for (int k = 0; k < NF; k++) p += (S128)w[k] * f[k];
    return (S64)(p >> 30);
  }
  void add(const S64* f, S64 y) {
    for (int i = 0; i < NF; i++) {
      for (int j = 0; j <= i; j++) G[i][j] += (S128)f[i] * f[j];
      c[i] += (S128)f[i] * y;
    }
  }
  // (G + r I) w = c + r e_K by LDL', r the mean diagonal / 2^23 (w as it was where that fails)
  void refit() {
    S128 tr = 0;
    for (int i = 0; i < NF; i++) {
      for (int j = 0; j <= i; j++) G[i][j] -= G[i][j] >> 7;
      c[i] -= c[i] >> 7;
      tr += G[i][i];
    }
    if (tr <= 0) return;
    const Sf r = sf_make(tr / NF / ((S128)1 << 23) + 1, 0);
    Sf L[NF][NF], D[NF], z[NF];
    for (int j = 0; j < NF; j++) {
      Sf d = sf_add(sf_make(G[j][j], 0), r);
      for (int k = 0; k < j; k++) d = sf_sub(d, sf_mul(sf_mul(L[j][k], L[j][k]), D[k]));
      if (d.m <= 0) return;
      D[j] = d;
      for (int i = j + 1; i < NF; i++) {
        Sf a = sf_make(G[i][j], 0);
        for (int k = 0; k < j; k++) a = sf_sub(a, sf_mul(sf_mul(L[i][k], L[j][k]), D[k]));
        L[i][j] = sf_div(a, d);
      }
    }
    for (int i = 0; i < NF; i++) {  // L z = b, then D, then L' w = z
      Sf b = sf_make(c[i], 0);
      if (i == LFE_K) b = sf_add(b, r);
      for (int k = 0; k < i; k++) b = sf_sub(b, sf_mul(L[i][k], z[k]));
      z[i] = b;
    }
    for (int i = 0; i < NF; i++) z[i] = sf_div(z[i], D[i]);
    for (int i = NF - 1; i >= 0; i--) {
      Sf x = z[i];
      for (int k = i + 1; k < NF; k++) x = sf_sub(x, sf_mul(L[k][i], z[k]));
      z[i] = x;
    }
    for (int i = 0; i < NF; i++) w[i] = sf_int(z[i], 30);
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
// WAV's samples the probes need must be in its window. (The offsets by the 16 probes of most
// energy in core channel 0 first, the 64 places best by them then by all the probes: the best of
// those, as a search of all of them by all the probes finds it, where it is among them.)
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
  int seg_wc = -1;
  auto load = [&](int wc) {  // (the probes' windows of WAV channel wc)
    if (seg_wc == wc) return;
    seg_wc = wc;
    for (int i = 0; i < n; i++) {
      const S64 o = (S64)M * pr[i].t + olo - L;
      for (int k = 0; k < span; k++) seg[(size_t)i * span + k] = w.at(wc, o + k);
    }
  };
  auto corr = [&](int ch, S64 off, const int* pi, int np) {
    double sxy = 0, sxx = 0, syy = 0, x[8];
    for (int j = 0; j < np; j++) {
      const int i = pi ? pi[j] : j;
      const double* s = seg + (size_t)i * span + (off - olo);
      if (up == 2) analyze_d<64>(s, fir, cq64, x);
      else analyze_d<32>(s, fir, cq32, x);
      for (int k = 0; k < 8; k++)
        if (pr[i].ok[ch][k]) sxy += x[k] * pr[i].y[ch][k], sxx += x[k] * x[k], syy += (double)pr[i].y[ch][k] * pr[i].y[ch][k];
    }
    return sxx > 0 && syy > 0 ? sxy / sqrt(sxx * syy) : 0;
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
  for (int wc = 0; wc < w.nch; wc++) {
    load(wc);
    for (int u = 0; u < 4; u++) {
      const double r = corr(0, (S64)USUAL[u] * up, nullptr, n);
      if (r > best) best = r, bo = (S64)USUAL[u] * up, bw = wc;
    }
  }
  if (best >= 0.999) n1 = -1;  // (no search)
  else best = -2;
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
  for (int ch = 1; ch < nch; ch++) {
    double bc = -2;
    int bwc = -1;
    for (int wc = 0; wc < w.nch; wc++) {
      load(wc);
      const double r = corr(ch, bo, nullptr, n);
      if (r > bc) bc = r, bwc = wc;
    }
    if (bc >= 0.9) b.map[ch] = (S8)bwc;
  }
  return b;
}
