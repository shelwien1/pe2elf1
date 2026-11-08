// anal.inc -- what the WAV says of the core: the subband samples of its audio by the encoder's
// analysis filterbank (in integers: c and d compute them alike everywhere), the indexes they
// quantize to as the core's step sizes, scale factors and ADPCM predictions say, the scale factors
// their RMS makes, and how the core's channels and blocks line up with the WAV's (c finds it, the
// header keeps it).

constexpr int AQ_F = 16;  // the analysis's subband samples: in 2^-AQ_F of the decoder's units
constexpr int QB_N = 16;  // the buckets of how near the rounding boundary a sample lies (near_bucket())
constexpr int SB_N = 8;   // and a scale factor's RMS the midpoint between two table entries

#include "tables.inc"

static INLINE S32 clip23(S64 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : (S32)a; }
static INLINE S64 rshift(S64 a, int bits) { return bits > 0 ? (a + (1LL << (bits - 1))) >> bits : a; }
typedef __int128 S128;
typedef unsigned __int128 U128;

// How the core's channels and blocks line up with the WAV's: the analysis of block t (the core's
// t-th subband sample since the stream's first frame) ends before sample 32 t + off of the WAV, and
// map[ch] is the WAV's channel of core channel ch (-1 none, as for every channel without a WAV)
struct Bind {
  S32 off;
  S8 map[NCH];  // each core channel's WAV channel (-1 none)
  S8 lfe;       // the LFE's (-1 none)
  void none() {
    off = 0;
    for (int ch = 0; ch < NCH; ch++) map[ch] = -1;
    lfe = -1;
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
// *alt the index on the other side of the rounding boundary nearest the value, *dist how far the
// value lies from that boundary (in 2^-(AQ_F + 22))
static INLINE S64 requant(S64 v, S64 ss, S64 lo, S64 hi, S128* dist, S64* alt) {
  const S128 num = (S128)v << 22, den = (S128)ss << AQ_F;
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

// --- the encoder's codebook searches (as found; in integers) ---------------------------------------

// The 4 ADPCM coefficients the n samples x (2^-AQ_F) predict themselves by best: the normal
// equations of their autocorrelation (no window), solved by Levinson-Durbin in Q28, into alpha in
// Q24 (alpha[k] multiplies the sample k + 1 before, as the codebook's entries do); *gain the
// prediction gain in halvings (0-15). The encoder's predictor is then the codebook entry nearest
// alpha (pvq_dist()), over the subframe's samples and the 4 before them.
static void lpc4(const S64* x, int n, S64* alpha, int* gain) {
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
    S128 acc = (S128)R[i] << 28;
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
}
// the ADPCM codebook entry q's distance from the coefficients alpha (Q24)
static INLINE S64 pvq_dist(int q, const S64* alpha) {
  S64 d = 0;
  for (int m = 0; m < 4; m++) {
    const S64 e = alpha[m] - ((S64)ADPCM_VB[q][m] << 11);
    d += e * e;
  }
  return d;
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
    const S128 e = (S128)x[j] * 512 - (((S128)hf_groups.s[g][j] * sc) << AQ_F);
    d += e * e;
  }
  return d;
}
// entry q's distance (in 2^(AQ_F + 4) sc units)
static INLINE S128 hf_dist(int q, const S64* x, int n, S64 sc) {
  S128 d = 0;
  for (int j = 0; j < n; j++) {
    const S128 e = (S128)x[j] * 16 - (((S128)HF_VQ[q][j] * sc) << AQ_F);
    d += e * e;
  }
  return d;
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
  int b = 0;
  for (int i = 1; i < n; i++)
    if (key_less(a[i], a[b])) b = i;
  int s = -1;
  for (int i = 0; i < n; i++)
    if (i != b && (s < 0 || key_less(a[i], a[s]))) s = i;
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

// the WAV channel c decimated by the prototype, the window ending at sample end, in the units
// of an LFE value times its scale (lfe_scale())
static S64 lfe_decim(const WavIn& w, int c, S64 end) {
  S128 acc = 0;
  for (int m = 0; m < 512; m++) acc += (S128)LFE_FIR[m] * w.at(c, end - 512 + m);
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
static INLINE Sf sf_div(Sf a, Sf b) { return b.m ? sf_make(((S128)a.m << 64) / b.m, a.e - b.e - 64) : Sf{0, 0}; }
static Sf sf_add(Sf a, Sf b) {
  if (!a.m) return b;
  if (!b.m) return a;
  if (a.e < b.e) {
    const Sf t = a;
    a = b;
    b = t;
  }
  const int d = a.e - b.e;
  const S128 x = (S128)a.m << 62, y = d < 124 ? ((S128)b.m << 62) >> d : 0;
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

// The correction's least squares fit: the decimated samples around each LFE sample (f, LFE_NT of
// them) against the LFE value times its scale (y), the older ones forgotten (by 1/128 a frame),
// ridged toward no correction. Plain data: init(), then add() each sample, refit() each frame.
struct LfeFit {
  S128 G[LFE_NT][LFE_NT], c[LFE_NT];  // the sums of f f' and f y
  S64 w[LFE_NT];                      // the correction (Q30)

  void init() {
    memset((void*)this, 0, sizeof *this);
    w[LFE_K] = (S64)1 << 30;
  }
  S64 predict(const S64* f) const {
    S128 p = 0;
    for (int k = 0; k < LFE_NT; k++) p += (S128)w[k] * f[k];
    return (S64)(p >> 30);
  }
  void add(const S64* f, S64 y) {
    for (int i = 0; i < LFE_NT; i++) {
      for (int j = 0; j <= i; j++) G[i][j] += (S128)f[i] * f[j];
      c[i] += (S128)f[i] * y;
    }
  }
  // (G + r I) w = c + r e_K by LDL', r the mean diagonal / 2^23 (w as it was where that fails)
  void refit() {
    S128 tr = 0;
    for (int i = 0; i < LFE_NT; i++) {
      for (int j = 0; j <= i; j++) G[i][j] -= G[i][j] >> 7;
      c[i] -= c[i] >> 7;
      tr += G[i][i];
    }
    if (tr <= 0) return;
    const Sf r = sf_make(tr / LFE_NT / ((S128)1 << 23) + 1, 0);
    Sf L[LFE_NT][LFE_NT], D[LFE_NT], z[LFE_NT];
    for (int j = 0; j < LFE_NT; j++) {
      Sf d = sf_add(sf_make(G[j][j], 0), r);
      for (int k = 0; k < j; k++) d = sf_sub(d, sf_mul(sf_mul(L[j][k], L[j][k]), D[k]));
      if (d.m <= 0) return;
      D[j] = d;
      for (int i = j + 1; i < LFE_NT; i++) {
        Sf a = sf_make(G[i][j], 0);
        for (int k = 0; k < j; k++) a = sf_sub(a, sf_mul(sf_mul(L[i][k], L[j][k]), D[k]));
        L[i][j] = sf_div(a, d);
      }
    }
    for (int i = 0; i < LFE_NT; i++) {  // L z = b, then D, then L' w = z
      Sf b = sf_make(c[i], 0);
      if (i == LFE_K) b = sf_add(b, r);
      for (int k = 0; k < i; k++) b = sf_sub(b, sf_mul(L[i][k], z[k]));
      z[i] = b;
    }
    for (int i = 0; i < LFE_NT; i++) z[i] = sf_div(z[i], D[i]);
    for (int i = LFE_NT - 1; i >= 0; i--) {
      Sf x = z[i];
      for (int k = i + 1; k < LFE_NT; k++) x = sf_sub(x, sf_mul(L[k][i], z[k]));
      z[i] = x;
    }
    for (int i = 0; i < LFE_NT; i++) w[i] = sf_int(z[i], 30);
  }
};

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
