// anal.inc -- what the WAV says of the core: the subband samples of its audio by the encoder's
// analysis filterbank (in integers: c and d compute them alike everywhere), the indexes they
// quantize to as the core's step sizes, scale factors and ADPCM predictions say, the scale factors
// their RMS makes, and how the core's channels and blocks line up with the WAV's (c finds it, the
// header keeps it).

constexpr int AQ_F = 16;  // the analysis's subband samples: in 2^-AQ_F of the decoder's units
constexpr int QB_N = 21;  // the buckets of how near the rounding boundary a sample lies
constexpr int SB_N = 8;   // and a scale factor's RMS the midpoint between two table entries

#include "tables.inc"

static INLINE S32 clip23(S64 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : (S32)a; }
static INLINE S64 rshift(S64 a, int bits) { return bits > 0 ? (a + (1LL << (bits - 1))) >> bits : a; }
typedef __int128 S128;

// How the core's channels and blocks line up with the WAV's: the analysis of block t (the core's
// t-th subband sample since the stream's first frame) ends before sample 32 t + off of the WAV, and
// map[ch] is the WAV's channel of core channel ch (-1 none, as for every channel without a WAV)
struct Bind {
  S32 off;
  S8 map[NCH];
  void none() {
    off = 0;
    for (int ch = 0; ch < NCH; ch++) map[ch] = -1;
  }
};

// The subband samples of the 512 samples of WAV channel c before sample o, as the encoder's
// analysis makes them (FFmpeg dcaenc's transform, by the prototype filter fir), in 2^-AQ_F of the
// decoder's units (its gain, 2, in): x[32]
static void analyze(const WavIn& w, int c, S64 o, const S64* fir, S64* x) {
  S128 acc[64];
  memset(acc, 0, sizeof acc);
  for (int m = 0; m < 512; m++) acc[m & 63] += (S128)w.at(c, o - 512 + m) * fir[m];
  for (int k = 16; k < 32; k++) acc[k] -= acc[31 - k];
  for (int k = 32; k < 48; k++) acc[k] += acc[95 - k];
  S64 a[32];
  for (int i = 0; i < 32; i++) a[i] = (S64)(acc[i + 16] >> 20);  // (2^-24 of a sample)
  for (int b = 0; b < 32; b++) {
    S128 r = 0;
    for (int i = 0; i < 32; i++) r += (S128)a[i] * COSQ[b][i];
    x[b] = (S64)((r + ((S128)1 << (84 - AQ_F))) >> (85 - AQ_F));  // (2 r: 2^-(24 + 62) to 2^-AQ_F)
  }
}

// --- the decoder's subband samples (FFmpeg dca_core.c's, bit-exact) ---------------------------------

// step size times scale factor of channel ch's subband b in subsubframe ssf (its transient's
// second scale factor from its subsubframe on; adjusted where SEL picks a Huffman code)
static INLINE S64 step_scale(const CoreRec& c, const CoreSub& s, int ch, int b, int ssf) {
  const int a = s.abits[ch][b], tm = s.tmode[ch][b];
  const int si = s.scale[ch][b][tm && ssf >= tm ? 1 : 0];
  S64 scale = c.shuff[ch] == 6 ? SF7[si] : SF6[si & 63];
  if (sample_coding(c, ch, a) == 1) scale = clip23((S64)SF_ADJ[c.adj[ch][a - 1]] * scale >> 22);
  return (S64)LOSSY_QUANT[a] * scale;
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
// *alt the index on the other side of the rounding boundary nearest the value, *near how near it
// lies: QB_N - 1 far, 0 within 1/400 of a step
static INLINE S64 requant(S64 v, S64 ss, S64 lo, S64 hi, int* near, S64* alt) {
  const S128 num = (S128)v << 22, den = (S128)ss << AQ_F;
  const int neg = num < 0;
  const S128 a = neg ? -num : num;
  const S128 qa = (2 * a + den) / (2 * den);  // floor(a / den + 1/2)
  const S128 r = a - qa * den;                 // -den/2 .. den/2
  const S128 d = den / 2 - (r < 0 ? -r : r);   // to the boundary
  *near = d * 20 >= den ? QB_N - 1 : (int)(d * 400 / den);
  S64 q = (S64)qa, o = r >= 0 ? q + 1 : q - 1;
  if (neg) q = -q, o = -o;
  *alt = o < lo ? lo : o > hi ? hi : o;
  return q < lo ? lo : q > hi ? hi : q;
}

// The scale factor of table tab (nt entries) nearest (linearly) to the RMS of the n values v
// (2^-AQ_F), as dtsreq found the encoder's; *near how near the RMS lies to the midpoint between it
// and the next entry the other way (0 there, SB_N - 1 far)
static int scale_pred(const S64* v, int n, const U32* tab, int nt, int* near) {
  S128 ss = 0;
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

// a bucket of x / y, |x| <= y, as -1..1 in n steps (0..n - 1); n / 2 where y is 0
static INLINE int ratio_bucket(S128 x, S128 y, int n) {
  if (y <= 0) return n / 2;
  const S128 k = (x + y) * n / (2 * y);
  return k < 0 ? 0 : k >= n ? n - 1 : (int)k;
}

// --- the alignment (c: found once, the header keeps it; in doubles) ---------------------------------

// A core's first frames, for the alignment: the decoder's plain subband samples of bands 0-7 (those
// without ADPCM) of a block of it, by channel
struct Probe {
  S64 t;  // the block
  S32 y[NCH][8];
  U8 ok[NCH][8];
};

// the double-precision analysis of bands 0-7 (as analyze()'s, unscaled)
static void analyze_d(const WavIn& w, int c, S64 o, const double* fir, double* x) {
  double acc[64];
  memset(acc, 0, sizeof acc);
  for (int m = 0; m < 512; m++) acc[m & 63] += w.at(c, o - 512 + m) * fir[m];
  for (int k = 16; k < 32; k++) acc[k] -= acc[31 - k];
  for (int k = 32; k < 48; k++) acc[k] += acc[95 - k];
  for (int b = 0; b < 8; b++) {
    double r = 0;
    for (int i = 0; i < 32; i++) r += acc[i + 16] * (double)COSQ[b][i];
    x[b] = r;
  }
}

// The WAV's channel and offset of each of the core's nch channels, from the probe blocks pr (n of
// them): the offset and channel whose analysis correlates best with core channel 0 over offsets
// -1024..1536, then each other channel's best at that offset; a channel whose best correlation is
// under 0.9 is none. The WAV's samples the probes need must be in its window.
static Bind find_bind(const WavIn& w, const Probe* pr, int n, int nch, int filts, double* corr0) {
  Bind b;
  b.none();
  *corr0 = 0;
  if (!w.ok || n <= 0) return b;
  double fir[512];
  for (int m = 0; m < 512; m++) fir[m] = (double)(filts ? FIR_PERFECT : FIR_NONPERFECT)[m];
  auto corr = [&](int ch, int wc, S64 off) {
    double sxy = 0, sxx = 0, syy = 0, x[8];
    for (int i = 0; i < n; i++) {
      analyze_d(w, wc, 32 * pr[i].t + off, fir, x);
      for (int k = 0; k < 8; k++)
        if (pr[i].ok[ch][k]) sxy += x[k] * pr[i].y[ch][k], sxx += x[k] * x[k], syy += (double)pr[i].y[ch][k] * pr[i].y[ch][k];
    }
    return sxx > 0 && syy > 0 ? sxy / sqrt(sxx * syy) : 0;
  };
  double best = -2;
  int bw = 0;
  S64 bo = 0;
  for (int wc = 0; wc < w.nch; wc++)
    for (S64 off = -1024; off <= 1536; off++) {
      const double r = corr(0, wc, off);
      if (r > best) best = r, bo = off, bw = wc;
    }
  *corr0 = best;
  if (best < 0.9) return b;
  b.off = (S32)bo;
  b.map[0] = (S8)bw;
  for (int ch = 1; ch < nch; ch++) {
    double bc = -2;
    int bwc = -1;
    for (int wc = 0; wc < w.nch; wc++) {
      const double r = corr(ch, wc, bo);
      if (r > bc) bc = r, bwc = wc;
    }
    if (bc >= 0.9) b.map[ch] = (S8)bwc;
  }
  return b;
}
