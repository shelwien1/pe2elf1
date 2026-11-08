// dtsreq - how much of a DTS core follows from the lossless audio it was coded from
//
// Usage: dtsreq input.dts full.wav
//
// full.wav is the lossless decode of input.dts (ffmpeg -i input.dts -c:a pcm_s24le full.wav, which
// decodes XLL bit-exactly): the audio the encoder coded the core from. Its subband samples by the
// encoder's 32-band QMF analysis (FFmpeg dcaenc's transform, with the prototype filter of the core's
// FILTS, in double precision), aligned with the core's by the offset and channels that correlate
// best, are quantized as the core's own side information says: its step sizes (ABITS), scale
// factors, transient modes, and the ADPCM predictions from the decoded history where it predicts.
// Then compared with the core's quantization indexes. Each scale factor is compared with the table
// entry nearest to the RMS of what it scales (the ADPCM residuals where the band predicts).
//
// Printed: the shares that come out exact, and the empirical entropy of the differences (what a
// coder of the core that has the audio spends on them) against the entropy of the values
// themselves and their size as coded.
//
// Build: g++ -O2 -o dtsreq dtsreq.cpp
// (its tables are FFmpeg's, so it is under the LGPL 2.1 or later, as dtssplit.cpp is)

#include <math.h>
#include <map>
#define DTSSPLIT_LIB
#include "dtssplit.cpp"
#include "dtsreq_tables.inc"

static inline S32 clip23s(S64 a) { return a < -(1 << 23) ? -(1 << 23) : a > (1 << 23) - 1 ? (1 << 23) - 1 : (S32)a; }
static inline S64 rshift(S64 a, int bits) { return bits > 0 ? (a + (1LL << (bits - 1))) >> bits : a; }

// empirical entropy of symbols in contexts
struct Counts {
  std::map<S64, std::map<S64, U64> > c;
  void add(S64 ctx, S64 sym) { c[ctx][sym]++; }
  double bytes() const {
    double h = 0;
    for (std::map<S64, std::map<S64, U64> >::const_iterator i = c.begin(); i != c.end(); ++i) {
      U64 n = 0;
      for (std::map<S64, U64>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) n += j->second;
      for (std::map<S64, U64>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
        h -= j->second * log2((double)j->second / n);
    }
    return h / 8;
  }
};

static std::vector<S32> pcm[XWAV];
static int wch;
static const float* fir;
static double cosm[32][32];

// the subband samples of the analysis of the 512 samples of WAV channel c before sample o
static void analyze(int c, long o, double* out) {
  double acc[64];
  memset(acc, 0, sizeof acc);
  const long n = (long)pcm[c].size();
  for (int m = 0; m < 512; m++) {
    const long s = o - 512 + m;
    acc[m & 63] += (s >= 0 && s < n ? pcm[c][s] : 0) * (double)fir[m];
  }
  for (int k = 16; k < 32; k++) acc[k] = acc[k] - acc[31 - k];
  for (int k = 32; k < 48; k++) acc[k] = acc[k] + acc[95 - k];
  for (int b = 0; b < 32; b++) {
    double r = 0;
    for (int i = 0; i < 32; i++) r += acc[i + 16] * cosm[b][i];
    out[b] = 2 * r;  // (the gain of the decoder's synthesis)
  }
}

// a subband sample's step and scale factor (as the decoder takes them)
static S64 step_scale(const CoreFrame& f, const CoreSub& s, int ch, int b, int ssf) {
  const int a = s.abits[ch][b], tm = s.tmode[ch][b];
  const int si = s.scale[ch][b][tm && ssf >= tm ? 1 : 0];
  S64 scale = f.shuff[ch] == 6 ? SF7[si] : SF6[si];
  if (sample_coding(f, ch, a) == 1) scale = clip23s((S64)SF_ADJ[f.adj[ch][a - 1]] * scale >> 22);
  return (S64)LOSSY_QUANT[a] * scale;
}
static S32 dequantize(S32 q, S64 ss) {
  int shift = 0;
  if (ss > (1 << 23)) {
    shift = 63 - __builtin_clzll(ss >> 23) + 1;
    ss >>= shift;
  }
  return clip23s(rshift((S64)q * ss, 22 - shift));
}
static S32 predict(int pvq, const S32* h) {  // h[0] the sample before, h[3] four before
  S64 p = 0;
  for (int k = 0; k < 4; k++) p += (S64)h[k] * ADPCM_VB[pvq][k];
  return clip23s(rshift(p, 13));
}
static void quant_range(const CoreFrame& f, int ch, int a, S64& lo, S64& hi) {
  const int how = sample_coding(f, ch, a);
  if (how == 3) {
    hi = (BLOCK_LEVELS[a - 1] - 1) / 2, lo = -hi;
  } else if (how == 2) {
    hi = (1LL << (a - 4)) - 1, lo = -(1LL << (a - 4));
  } else {
    const HuffBook& hb = QB[a - 1][f.sel[ch][a - 1]];
    lo = hb.minlev, hi = hb.minlev + hb.nlev - 1;
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fputs("Usage: dtsreq input.dts full.wav\n", stderr);
    return EX_USAGE;
  }
  crc_init();
  dtscore_init();
  std::vector<U8> b;
  {
    FILE* fi = fopen(argv[1], "rb");
    if (!fi) die(EX_READ, "can't open %s", argv[1]);
    U8 t[1 << 16];
    size_t k;
    while ((k = fread(t, 1, sizeof t, fi))) b.insert(b.end(), t, t + k);
    fclose(fi);
  }
  const size_t n = b.size();
  b.resize(n + PAD);
  {  // the WAV
    FILE* fw = fopen(argv[2], "rb");
    if (!fw) die(EX_READ, "can't open %s", argv[2]);
    std::vector<U8> w;
    U8 t[1 << 16];
    size_t k;
    while ((k = fread(t, 1, sizeof t, fw))) w.insert(w.end(), t, t + k);
    fclose(fw);
    size_t i = 12, data = 0, dn = 0;
    int bits = 0;
    while (w.size() >= 12 && i + 8 <= w.size()) {
      const U32 id = be32(&w[i]), sz = get32(&w[i + 4]);
      if (id == 0x666d7420 && i + 24 <= w.size()) wch = get16(&w[i + 10]), bits = get16(&w[i + 22]);
      if (id == 0x64617461) {
        data = i + 8;
        dn = sz == 0xFFFFFFFF || data + sz > w.size() ? w.size() - data : sz;
        break;
      }
      i += 8 + sz + (sz & 1);
    }
    if (!data || !wch || wch > XWAV || (bits != 16 && bits != 24 && bits != 32)) die(EX_DATA, "%s: not a PCM WAV", argv[2]);
    const int bps = bits / 8;
    const size_t ns = dn / ((size_t)bps * wch);
    for (int c = 0; c < wch; c++) pcm[c].resize(ns);
    for (size_t s = 0; s < ns; s++)
      for (int c = 0; c < wch; c++) {
        const U8* p = &w[data + (s * wch + c) * bps];
        pcm[c][s] = bps == 2 ? (S16)get16(p) : bps == 3 ? (S32)((U32)p[0] << 8 | (U32)p[1] << 16 | (U32)p[2] << 24) >> 8
                                                        : (S32)get32(p) >> 8;  // (32-bit: a 24-bit decode's)
      }
  }
  // the core frames
  std::vector<std::pair<size_t, int> > frames;
  static CoreFrame f;
  int filts = -1, nch = 0;
  for (size_t i = 0; i + 16 <= n;) {
    const int fs = core_frame(&b[i], n - i);
    if (!fs) {
      i++;
      continue;
    }
    if (!core_parse(&b[i], fs, f)) {
      if (filts < 0) filts = f.filts, nch = f.npchs;
      if (f.npchs == nch && f.filts == filts && !f.sumf && !f.sums) frames.push_back(std::make_pair(i, fs));
    }
    i += fs;
  }
  if (frames.empty()) die(EX_DATA, "%s: no core frames it takes (sum/difference coded channels aren't)", argv[1]);
  fir = filts ? FIR_PERFECT : FIR_NONPERFECT;
  for (int bd = 0; bd < 32; bd++)
    for (int i = 0; i < 32; i++)
      cosm[bd][i] = cos(M_PI * (2 * bd + 1) * (2 * (i + 32) + 1) / 128.0) * (((bd + 1) & 2) ? -1 : 1);

  // the alignment: from frames a quarter into the stream (where a decode of a cut stream is right),
  // the decoded plain subband samples of bands 0-7 against the analysis at offsets O (block t's
  // analysis ending at sample 32 t + O) of each WAV channel
  struct Blk {
    long t;  // the block
    S32 y[32];
    U8 ok[32];
  };
  std::vector<Blk> probe[DC_CH];
  {
    long t = 0;
    const size_t k0 = frames.size() / 4;
    for (size_t k = 0; k < frames.size() && probe[0].size() < 64; k++) {
      core_parse(&b[frames[k].first], frames[k].second, f);
      for (int sf = 0; sf < f.nsubfs; sf++) {
        const CoreSub& s = f.sub[sf];
        for (int ssf = 0; ssf < s.nssc; ssf++)
          for (int m = 0; m < 8; m++, t++) {
            if (k < k0) continue;
            for (int ch = 0; ch < nch; ch++) {
              Blk bl;
              bl.t = t;
              for (int bd = 0; bd < 32; bd++) {
                bl.ok[bd] = bd < 8 && bd < f.vqsub[ch] && s.abits[ch][bd] && !s.pmode[ch][bd];
                bl.y[bd] = bl.ok[bd] ? dequantize(s.q[ch][bd][ssf * 8 + m], step_scale(f, s, ch, bd, ssf)) : 0;
              }
              if (probe[ch].size() < 64) probe[ch].push_back(bl);
            }
          }
      }
    }
  }
  auto corr = [&](int ch, int w, long O) {
    double sxy = 0, sxx = 0, syy = 0, xs[32];
    for (size_t i = 0; i < probe[ch].size(); i++) {
      const Blk& bl = probe[ch][i];
      analyze(w, 32 * bl.t + O, xs);
      for (int bd = 0; bd < 8; bd++)
        if (bl.ok[bd]) sxy += xs[bd] * bl.y[bd], sxx += xs[bd] * xs[bd], syy += (double)bl.y[bd] * bl.y[bd];
    }
    return sxy / sqrt(sxx * syy + 1e-30);
  };
  long O = 0;
  int map[DC_CH];
  double best = -2;
  for (int w = 0; w < wch; w++)
    for (long o = -1024; o <= 1536; o++) {
      const double r = corr(0, w, o);
      if (r > best) best = r, O = o, map[0] = w;
    }
  double worst = best;
  for (int ch = 1; ch < nch; ch++) {
    double bc = -2;
    for (int w = 0; w < wch; w++) {
      const double r = corr(ch, w, O);
      if (r > bc) bc = r, map[ch] = w;
    }
    if (bc < worst) worst = bc;
  }
  printf("%s: %zu core frames, %d channels, FILTS %d; the analysis %ld samples ahead, channels", argv[1], frames.size(), nch, filts, O);
  for (int ch = 0; ch < nch; ch++) printf(" %d<-%d", ch, map[ch]);
  printf(" of the WAV (correlation %.6f at the least)\n", worst);

  // the core, frame by frame: its subband samples decoded, the audio's quantized, compared
  static S32 hist[DC_CH][32][4];  // the decoded samples before, of each subband (for ADPCM)
  memset(hist, 0, sizeof hist);
  U64 cnt[27] = {0}, exact[27] = {0}, off1[27] = {0}, coded = 0, coded_sf = 0, nsf = 0, sf_exact = 0, skipped = 0;
  Counts qv, dq, dqb, sv, ds;
  long t = 0;
  for (size_t k = 0; k < frames.size(); k++) {
    core_parse(&b[frames[k].first], frames[k].second, f);
    coded += f.bits[DC_HUFF] + f.bits[DC_BLOCK] + f.bits[DC_NFE];
    coded_sf += f.bits[DC_SCALE];
    for (int sf = 0; sf < f.nsubfs; sf++) {
      const CoreSub& s = f.sub[sf];
      const int ns = s.nssc * 8;
      // the subframe's analysis
      static double X[DC_CH][32][DC_SSF * 8];
      bool full = 32 * (t + ns - 1) + O <= (long)pcm[0].size();
      for (int ch = 0; ch < nch; ch++)
        for (int j = 0; j < ns; j++) {
          double xs[32];
          analyze(map[ch], 32 * (t + j) + O, xs);
          for (int bd = 0; bd < 32; bd++) X[ch][bd][j] = xs[bd];
        }
      for (int ch = 0; ch < nch; ch++)
        for (int bd = 0; bd < f.vqsub[ch]; bd++) {
          const int a = s.abits[ch][bd];
          S32* h = hist[ch][bd];
          if (!a) {
            memset(h, 0, sizeof hist[ch][bd]);
            continue;
          }
          S64 lo, hi;
          quant_range(f, ch, a, lo, hi);
          double v[DC_SSF * 8];  // what is quantized: the sample, or its ADPCM residual
          for (int j = 0; j < ns; j++) {
            const int ssf = j / 8;
            const S64 ss = step_scale(f, s, ch, bd, ssf);
            const S32 q = s.q[ch][bd][j];
            const S32 pred = s.pmode[ch][bd] ? predict(s.pvq[ch][bd], h) : 0;
            v[j] = X[ch][bd][j] - pred;
            if (full) {
              const double qf = v[j] * 4194304.0 / (double)ss;
              S64 qp = (S64)llround(qf);
              qp = qp < lo ? lo : qp > hi ? hi : qp;
              const S64 d = q - qp;
              const double fr = fabs(qf - floor(qf) - 0.5);
              cnt[a]++;
              exact[a] += !d;
              off1[a] += d == 1 || d == -1;
              qv.add(a, q);
              dq.add(a, d);
              dqb.add(a * 32 + (fr < 0.05 ? (int)(fr * 400) : 20), d);
            } else {
              skipped++;
            }
            const S32 y = clip23s((S64)dequantize(q, ss) + pred);
            memmove(h + 1, h, 3 * sizeof *h);
            h[0] = y;
          }
          // its scale factors
          if (!full) continue;
          const int tm = s.tmode[ch][bd];
          for (int g = 0; g < 1 + (tm > 0); g++) {
            const int j0 = g ? tm * 8 : 0, j1 = g || !tm ? ns : tm * 8;
            double rs = 0;
            for (int j = j0; j < j1; j++) rs += v[j] * v[j];
            const double rms = sqrt(rs / (j1 - j0));
            const U32* tab = f.shuff[ch] == 6 ? SF7 : SF6;
            const int nt = f.shuff[ch] == 6 ? 128 : 64;
            int p = 0;
            for (int i = 1; i < nt; i++)
              if (fabs((double)tab[i] - rms) < fabs((double)tab[p] - rms)) p = i;
            const int si = s.scale[ch][bd][g];
            nsf++;
            sf_exact += si == p;
            sv.add(a, si);
            ds.add(a, si - p);
          }
        }
      t += ns;
    }
  }
  U64 all = 0, ex = 0, o1 = 0;
  printf("\nABITS  samples   exact  off by 1\n");
  for (int a = 1; a <= 26; a++)
    if (cnt[a]) {
      printf("  %2d %9llu %7.2f%% %7.2f%%\n", a, (unsigned long long)cnt[a], 100.0 * exact[a] / cnt[a], 100.0 * off1[a] / cnt[a]);
      all += cnt[a], ex += exact[a], o1 += off1[a];
    }
  if (!all) return EX_OK;
  printf("  all %8llu %7.2f%% %7.2f%%", (unsigned long long)all, 100.0 * ex / all, 100.0 * o1 / all);
  if (skipped) printf("  (and %llu at the end, past the audio)", (unsigned long long)skipped);
  printf("\n\nthe quantization indexes: %.0f bytes as coded, entropy (by ABITS) %.0f;\n", coded / 8.0, qv.bytes());
  printf("  of their differences to the audio's %.0f, and by the distance to the rounding boundary %.0f\n", dq.bytes(), dqb.bytes());
  printf("the scale factors: %.0f bytes as coded, entropy (by ABITS) %.0f; %.2f%% the nearest to the RMS,\n", coded_sf / 8.0,
         sv.bytes(), 100.0 * sf_exact / (nsf ? nsf : 1));
  printf("  the entropy of their differences to it %.0f\n", ds.bytes());
  return EX_OK;
}
