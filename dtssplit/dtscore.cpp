// dtscore - what the core frames of a DTS stream are made of, and what a model of them could take
//
// Usage: dtscore input.dts
//
// Every core frame is parsed down to the quantization indexes of its subband samples (dtscore.inc),
// written back and compared with the original; then the statistics of those that come back the
// same: what each part of the frames takes, how the samples are coded (Huffman, block or plain
// codes) by bit allocation (ABITS), and the empirical entropy of the fields under a few simple
// models, an estimate of what a coder of the core's own fields could make of them. A quantization
// index v is costed as its magnitude class (the bit length of |v|), entropy-coded in a context,
// plus its sign and its low bits as they are; scale factors and bit allocations as deltas to the
// subband's previous frame.
//
// Build: g++ -O2 -o dtscore dtscore.cpp

#include <math.h>
#include <map>
#define DTSSPLIT_LIB
#include "dtssplit.cpp"

static const char* const PART[DC_NCAT] = {
    "headers",       "PMODE",           "PVQ (ADPCM predictors)", "ABITS",      "TMODE", "scale factors",
    "joint intensity", "other side info", "HF VQ",                "LFE",        "samples: Huffman codes",
    "samples: block codes", "samples: plain", "DSYNC",           "after the subframes"};

struct Counts {  // symbols in contexts: their empirical entropy
  std::map<U64, std::map<S64, U64> > c;
  void add(U64 ctx, S64 sym) { c[ctx][sym]++; }
  double bits() const {
    double h = 0;
    for (std::map<U64, std::map<S64, U64> >::const_iterator i = c.begin(); i != c.end(); ++i) {
      U64 n = 0;
      for (std::map<S64, U64>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) n += j->second;
      for (std::map<S64, U64>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
        h -= j->second * log2((double)j->second / n);
    }
    return h;
  }
};

static int blen(S64 v) {
  int e = 0;
  for (v = v < 0 ? -v : v; v; v >>= 1) e++;
  return e;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fputs("Usage: dtscore input.dts\n", stderr);
    return EX_USAGE;
  }
  crc_init();
  dtscore_init();
  FILE* fi = fopen(argv[1], "rb");
  if (!fi) die(EX_READ, "can't open %s", argv[1]);
  std::vector<U8> b;
  {
    U8 t[1 << 16];
    size_t k;
    while ((k = fread(t, 1, sizeof t, fi))) b.insert(b.end(), t, t + k);
    fclose(fi);
  }
  b.resize(b.size() + PAD);
  const size_t n = b.size() - PAD;
  static CoreFrame f;
  static S32 prev_last[DC_CH][DC_SUB];
  static int prev_scale[DC_CH][DC_SUB], prev_abits[DC_CH][DC_SUB];
  U64 bits[DC_NCAT] = {0}, frames = 0, same = 0, bytes = 0;
  std::map<std::string, U64> fail;
  U64 abits_n[27] = {0};
  double abits_coded[27] = {0}, raw_bits = 0, sc_coded = 0, ab_coded = 0;
  Counts mag0, mag1, sc_d, ab_d;
  U64 how_n[4] = {0}, pmode = 0, bands = 0, tm = 0, tm_bands = 0;
  BitWriter w;
  for (size_t i = 0; i + 16 <= n;) {
    const int fs = core_frame(&b[i], n - i);
    if (!fs) {
      i++;
      continue;
    }
    frames++;
    const char* why = core_parse(&b[i], fs, f);
    if (!why && (!core_write(f, &b[i], f.data_end, w) || w.v.size() != (size_t)fs || memcmp(&w.v[0], &b[i], fs)))
      why = "doesn't write back the same";
    if (why) {
      fail[why]++;
      i += fs;
      continue;
    }
    same++;
    bytes += fs;
    for (int c = 0; c < DC_NCAT; c++) bits[c] += f.bits[c];
    for (int sf = 0; sf < f.nsubfs; sf++) {
      const CoreSub& s = f.sub[sf];
      for (int ch = 0; ch < f.npchs; ch++) {
        for (int bd = 0; bd < f.nsubs[ch]; bd++, bands++) pmode += s.pmode[ch][bd];
        for (int bd = 0; bd < f.vqsub[ch]; bd++) {
          const int a = s.abits[ch][bd];
          ab_coded += f.bhuff[ch] == 6 ? 5 : f.bhuff[ch] == 5 ? 4 : 0;
          ab_d.add(bd, a - prev_abits[ch][bd]);
          prev_abits[ch][bd] = a;
          if (a) {
            tm_bands++;
            tm += s.tmode[ch][bd] > 0;
            for (int j = 0; j < 1 + (s.tmode[ch][bd] > 0); j++) {
              sc_coded += f.shuff[ch] == 6 ? 7 : f.shuff[ch] == 5 ? 6 : 0;
              sc_d.add(0, s.scale[ch][bd][j] - prev_scale[ch][bd]);
              prev_scale[ch][bd] = s.scale[ch][bd][j];
            }
          }
          const int how = sample_coding(f, ch, a);
          how_n[how] += s.nssc;
          abits_n[a] += s.nssc;
          if (!how) continue;
          abits_coded[a] += s.nssc * 8 * (how == 2 ? a - 3.0 : how == 3 ? BLOCK_BITS[a - 1] / 4.0 : 0);
          for (int t = 0; t < s.nssc * 8; t++) {
            const S32 v = s.q[ch][bd][t], pv = t ? s.q[ch][bd][t - 1] : prev_last[ch][bd];
            const int e = blen(v);
            raw_bits += e;  // its sign and e-1 low bits
            mag0.add(a, e);
            mag1.add((U64)a << 8 | blen(pv), e);
          }
          prev_last[ch][bd] = s.q[ch][bd][s.nssc * 8 - 1];
        }
      }
    }
    i += fs;
  }
  printf("%s: %llu core frames, %llu parsed and written back the same (%llu bytes)\n", argv[1],
         (unsigned long long)frames, (unsigned long long)same, (unsigned long long)bytes);
  for (std::map<std::string, U64>::iterator j = fail.begin(); j != fail.end(); ++j)
    printf("  not: %s: %llu\n", j->first.c_str(), (unsigned long long)j->second);
  U64 all = 0;
  for (int c = 0; c < DC_NCAT; c++) all += bits[c];
  if (!all) return EX_OK;
  printf("\nwhat they take:\n");
  for (int c = 0; c < DC_NCAT; c++)
    printf("  %-26s %12.0f bytes  %6.2f%%\n", PART[c], bits[c] / 8.0, 100.0 * bits[c] / all);
  printf("\nsubbands with ADPCM %.1f%%, allocated subbands with a transient %.2f%%\n", 100.0 * pmode / (bands ? bands : 1),
         100.0 * tm / (tm_bands ? tm_bands : 1));
  printf("subband-subsubframes: no bits %llu, Huffman %llu, plain %llu, block codes %llu\n", (unsigned long long)how_n[0],
         (unsigned long long)how_n[1], (unsigned long long)how_n[2], (unsigned long long)how_n[3]);
  static const int levels[27] = {0,    3,     5,     7,     9,     13,     17,     25,     32,
                                 64,   128,   256,   512,   1024,  2048,   4096,   8192,   16384,
                                 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};
  printf("\nABITS  levels  subband-subsubframes  share  bits a sample as coded\n");
  const U64 ssfs = how_n[0] + how_n[1] + how_n[2] + how_n[3];
  for (int a = 0; a <= 26; a++)
    if (abits_n[a])
      printf("  %2d  %8d  %12llu  %6.2f%%  %6.3f\n", a, levels[a], (unsigned long long)abits_n[a],
             100.0 * abits_n[a] / ssfs, abits_coded[a] / (abits_n[a] * 8.0));
  const double samp = (double)bits[DC_HUFF] + bits[DC_BLOCK] + bits[DC_NFE];
  const double m0 = mag0.bits() + raw_bits, m1 = mag1.bits() + raw_bits;
  const double scb = sc_d.bits(), abb = ab_d.bits();
  printf("\nestimates (empirical entropy):\n");
  printf("  samples, %.0f bytes as coded: magnitude class by ABITS %.0f (%.1f%%), and by the sample before %.0f (%.1f%%)\n",
         samp / 8, m0 / 8, 100 * m0 / samp, m1 / 8, 100 * m1 / samp);
  printf("  scale factors, %.0f bytes as coded: deltas to the frame before %.0f (%.1f%%)\n", sc_coded / 8, scb / 8,
         100 * scb / (sc_coded ? sc_coded : 1));
  printf("  ABITS, %.0f bytes as coded: deltas to the frame before %.0f (%.1f%%)\n", ab_coded / 8, abb / 8,
         100 * abb / (ab_coded ? ab_coded : 1));
  const double est = m1 + scb + abb + (all - samp - sc_coded - ab_coded);
  printf("  the core: %.0f bytes -> %.0f (%.1f%%), the rest as it is\n", all / 8.0, est / 8, 100 * est / all);
  return EX_OK;
}
