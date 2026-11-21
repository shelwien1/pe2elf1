// model_smp.inc -- the model's elements against the audio (model.inc): the WAV's subband samples by
// the encoder's analysis (anal.inc), which the core's own step sizes, scale factors and ADPCM
// predictions quantize to nearly every index the core has (the hit and smp families), whose RMS
// makes nearly every scale factor (scl), and whose code book searches as the encoder's make nearly
// every ADPCM predictor and HF VQ vector (vqr); the WAV's LFE channel decimated, nearly every LFE
// sample. Where the WAV isn't bound to a channel, against the last values and the neighbours'.

template <int f_DEC, class LV>
struct ModelSmp : ModelSide<f_DEC, LV> {
  typedef ModelSide<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::hist;
  using B::checked;
  using B::cat;
  M_FAM(scl)
  M_FAM(scd)
  M_FAM(hit)
  M_FAM(hid)
  M_FAM(smp)
  M_FAM(smd)
  M_FAM(vqr)
  M_FAM(vqp)
  M_FAM(vpd)
  WavIn* wav;     // the WAV (none: ok 0)
  Bind bind;      // how the core lines up with it
  const S64* fir; // the prototype filter of the frame's FILTS
  S64 tnext;      // the blocks so far: the next frame's first (since the stream's first frame)
  S64 tframe;     // the frame's first
  S64 t0;         // the subframe's first
  int nsmp;       // the subframe's samples
  int ext_set;    // (the WAV's continuation past its end: 1 made, 2 taken or not: smp_subframe())
  int past_on;    // (and whether the audio far past it is the encoder's: past_end())
  // (a cut stream's WAV whose last frame a decoder made from that frame alone, vend_swap(): whether
  // that's been decided, whether that frame's samples are the continuation from its start now,
  // where it starts, the WAV's own there, the analyses' reach as it was)
  int vend_set, vend_on, vend_past;
  S64 vend_e, vend_ag;
  S32* vend_buf;
  int bound[NCH];  // whether the subframe's channel has the audio's subband samples (X)
  S64 X[NCH][NSUB][NSMP];  // the audio's subband samples in the subframe (2^-AQ_F)
  // (a mixed channel with a second pair of gains before the mix (Bind's alt): the binding with every
  // such channel's second pair, bind2; which one a channel's subframe takes, gsel; the audio's by
  // the other, XO; of the subframe's samples so far those whose index each makes, the taken's and
  // the other's, gh; and by how much the other has made more of them lately, gsc: gain_pick())
  Bind bind2;
  U8 gsel[NCH];
  S32 gsc[NCH];
  U32 gh[NCH][2];
  S64 XO[NCH][NSUB][NSMP];
  S64 xh[NCH][NSUB][4];    // the audio's last 4 before the subframe (xh[.][.][0] the last)
  S32 yh[NCH][NSUB][4];    // the decoder's last 4
  S32 q1[NCH][NSUB], q2[NCH][NSUB];  // the subband's last two indexes (1 + their length, 0 none; with their signs in qs)
  S8 qs[NCH][NSUB];
  U8 miss[NCH][NSUB];      // the subband's last 8 indexes, a bit each: not the audio's
  // the decoded values less the audio's, by subband: their running mean (halved each sample: in
  // 2^-AQ_F), and how often the audio's index was the value, and the audio's with that mean added
  // (but for half a step of it), halved at 256: where the encoder's input had more than the WAV (the
  // Cymatics DTS:X stream's surround channels: its height channels, it seems, not in the WAV's
  // 7.1), slow, the last values tell it where it is more than the step (their bottom subband,
  // finely quantized, missed 9% of the time, 70% where it's loud; 20% there with the mean)
  S64 qcar[NCH][NSUB];
  U32 qcn[NCH][NSUB][2];
  U32 chm[NCH];            // and the channel's last 32 (those coded against the audio)
  int ac[NCH][NSUB], es[NCH][NSUB], ec[NCH][NSUB];  // the subframe's subbands' autocorrelation, energy, its change
  int eq[NCH][NSUB];       // and where in its scale factor's bucket the energy lies (scale_quarter())
  int esmax[NCH];          // the channel's loudest subband's energy (es)
  S64 tlev[NCH][NSMP];     // the channel's level at each of the subframe's samples: the root of the sum of its subband
                           // samples' squares (2^-AQ_F)
  S64 tlvl[NCH];           // (and the root of their mean square)
  // a lossless WAV's at the core's rate, by subband: the level along the subband's path through the
  // encoder's FFT (path_levels()), at each sample and over the subframe (the root of their mean square)
  S64 plev[NCH][NSUB][NSMP], plvl[NCH][NSUB];
  int eab[NCH][NSUB];      // the energy ABITS was coded against (es; an ADPCM subband's residual's)
  U8 ab_seen[2][NSUB][129];  // the ABITS each subband had the last time it had that energy (by pm; 1 + it, 0 none)
  // the bit allocations' staircases (stair_guess()), by pass (the core's, the extension's) and
  // subband (an ADPCM subband's, by its residual's energy, the same); each pass's level
  // (the energies in 128ths of a doubling)
  static constexpr int AL_ND = 512, AL_NK = 2 * AL_ND + 1;  // (the level's search: 4 doublings either side)
  static constexpr int AB_RC = 1024;  // (the rate control's: 8 doublings either side of the last level)
  static constexpr int AB_RB = 4096;  // (the core's budget's, where the bits meet the frame's only further: 32)
  static constexpr S32 FE_NONE = -(1 << 30), AL_C = 1 << 15;
  struct AbStair {
    S32 lo[32], hi[32];  // each ABITS's energies, less the level
    U32 has;             // (a bit per ABITS)
  } ab_st[2][NSUB], ab_snap[2][NSUB];  // (ab_snap: as they were at the subframe's first ABITS)
  struct AbLevel {
    S32 c0, d;      // the subframe's centre and the shift from it
    S32 sc[AL_NK];  // how many of the subframe's ABITS each shift (less AL_ND) made the staircases'
    S64 cost, tgt;  // their cost (ab_bits()), and what the next subframe's aims at (the last ones' average)
    int n, set;     // (how many; whether the centre is set: at the subframe's first ABITS)
    S64 avail, rb;  // the frame's bits from the subframe's first ABITS on, and what the samples' bits
                    // (bud_bits()) come to past them, the last subframes' average (quarter bits)
    int rn;         // (how many subframes rb is of, to 1000)
    int nobs;       // the subframe's ABITS so far: each one's subband, ABITS and fine energy
    struct {
      S32 x;
      U8 b, v;
    } obs[NCH * NSUB];
  } abl[2];
  S32 fe_s[NCH][NSUB];  // the subframe's subbands' fine energies (FE_NONE: none)
  S32 fe_a[NCH][NSUB];  // (a transient's, where the encoders take one: its values' from it on; else FE_NONE)
  // the core's subframe's bit budget as its ABITS come (ab_budget()): the subbands coded, the bits
  // their ABITS' samples take
  U8 bg_done[NCH][NSUB];
  S64 bg_used;
  S16 bud_b[NCH][NSUB][27];    // the bits (in quarters) their samples would take at each ABITS (bud_bits(); -1 not yet)
  S64 bud_v[NCH][NSUB][NSMP];  // their values (an ADPCM subband's residual)
  int bud_n[NCH][NSUB], bud_si[NCH][NSUB];  // (how many; the RMS rule's scale factor)
  int bud_xs[NCH][NSUB];  // (a transient's second scale factor's and TMODE's bits, in quarters: format 125)
  U64 n_st, n_sthit;  // (-s: the ABITS a staircase said, those it said right)
  // FFmpeg's encoder's bit allocations made again from the WAV (ffenc.inc), where the core's code
  // books have been its (sel_ff): ffe, allocated then; ffe_ok, whether the frame's are made; how
  // often its ABITS was the core's, by its margin (FfEnc::margin()), the side of its range that is
  // nearer, whether under 8 and the lattice the values fit (FfEnc::lattice(): none, its, one above,
  // one below), and the other where not (the lattice's, else the one across the nearer end) (-s:
  // those it said, those it said right)
  FfEnc* ffe;
  int ffe_ok;
  // (since format 143 its transform of the WAV rounded to 16 bits too, where it has more than 16:
  // a decode of an encode of a 16-bit WAV gives the encoder's input back so, wherever the encode's
  // noise lies under half its LSB (FFmpeg's decode as 24 bits of its encode of the 16-bit WAV
  // below: 94.6% of the samples, all of them in its quiet blocks); ffr, allocated then; ffr_ok,
  // whether the frame's is made; ffr_c, how often the WAV's quantization as it is and as rounded
  // was the core's index where the two differ, halved at 4096 (it is the encoder's input where
  // the signal is quiet, not where loud: at 2^16, that decode 10% bigger; by level instead, 0.16%
  // smaller), the indexes by the rounded where it has been twice as often and 64 more (by a 16th
  // and 4, `s.2.dts`'s first 3 MB as decoded 2.1% bigger, by a quarter and 64 1.1%: in its first
  // 16 frames the rounded was right a third of the time, ahead for a while, then all but never), it
  // made no more where clearly less; ffr_l the same halved at 2^16, and where it says the rounded
  // as a rule (a 16-bit WAV's decode), the one not taken is the other candidate, the alternative,
  // where the two differ (since format 144: that decode 3.5% smaller; by the quick counts alone,
  // FFmpeg's decodes of 24-bit WAVs' encodes up to 19 bytes bigger))
  FfEnc* ffr;
  int ffr_ok;
  U32 ffr_c[2], ffr_l[2];
  U16 ffn[2][FfEnc::NMG][4][2][2], ffd[FfEnc::NMG][4][2];
  U64 n_ff, n_ffhit;
  // and its indexes: its encoder's quantization of the values it makes from the WAV, how often the
  // core's where it isn't the model's prediction (that one, it) and in all (all, it); where it has
  // been both more often and all but always, the WAV is the encoder's input
  U32 qff_c[2], qff_n[2];
  bool qff_on() const { return qff_n[0] >= 256 && (U64)qff_n[1] * 1000 >= (U64)qff_n[0] * 995 && jx_more(qff_c, 4, 1); }
  // and its LFE: its 8 values (lfe_downsample()) from each of two WAV channels no core channel
  // takes, with the history FFmpeg's encoder has (its input's last channel's, a sample late:
  // shift_history() fills the LFE's from the slot before its input's first channel), or the
  // channel's own; how often each one's quantization at the core's scale factor was the core's
  // value; where the best's all but always, the values and their scale factor by it
  static constexpr int FFL_N = 4;
  S32 ffl[FFL_N][8];
  U8 ffl_v[FFL_N];          // (whether made: the frame's channels)
  int ffl_ok;               // (whether any)
  int ffl_use;              // (whether the subframe's LFE goes by them)
  U32 ffl_h[FFL_N], ffl_n;  // (the values each made, of those tallied)
  int ffl_best() const {
    int k = 0;
    for (int i = 1; i < FFL_N; i++)
      if (ffl_h[i] > ffl_h[k]) k = i;
    return k;
  }
  bool ffl_on() const { return ffl_ok && ffl_v[ffl_best()] && ffl_n >= 8 && (U64)ffl_h[ffl_best()] * 200 >= (U64)ffl_n * 199; }
  // and its scale factors: by the ABITS (to 12), how often each of four was the core's: the
  // model's prediction, FFmpeg's encoder's rule on the decode's peak (calc_one_scale()) and the
  // least whose step puts that peak within 3/32 and 1/4 of the quantizer's top level (FfEnc::
  // scale_within()); since format 137 by the second's scale factor too (in fours, to 40) and the
  // first's difference from it (to 2)
  U16 ffs_w[13][11][5][4];
  int pscale[NCH][NSUB];   // their first scale factor's prediction (1 + it, 0 none)
  // A lossless WAV's top subbands, VQ ones, as its encoder has them: subband NSUB - 1 - c with m
  // 2^-23 of subband c added, c 0 to CPL_N - 1, m one of cpl_m(c, k), k 0 to CPL_K - 1 (CPL_D[c]
  // the first): how often each has made the RMS rule's scale factor right, and the HF VQ search's
  // group (code_scale(), code_hfvq()); the subframe's subbands as the analysis has them (a WAV at
  // the core's rate: a 64-band analysis's subbands 30 and 31 aren't the top ones)
  static constexpr int CPL_N = 2, CPL_K = 9;
  static int cpl_m(int c, int k) {
    static const S8 M[CPL_N][CPL_K] = {{0, -2, -4, -6, -8, -10, -12, -14, -16}, {-2, -1, 0, 1, 2, 3, 4, 5, 6}};
    return M[c][k];
  }
  static int cpl_d(int c) { return c ? 5 : 4; }  // (-8: 2^-20 of the bottom subband from the top one; +3)
  U32 cpl_hit[CPL_N][CPL_K], cpl_n[CPL_N];
  // The HF VQ groups' scores as this stream's encoder has them (code_hfvq()): hfgroups.inc's as it
  // starts, learned on where a lossless WAV's 16 samples' first group wasn't the encoder's -- a
  // perceptron's step of the vector (1/512 of it, then as the steps double from 1024, half as much
  // each time, to 1/16384) to the encoder's group's W and from the first's, the same of 64 to their
  // bias (anal.inc's hf_group_score(): e = -2 bias - |W|^2, kept from 0); the steps so far. (Not
  // from a vector whose peak is over 8 times the code book's: where the WAV isn't the encoder's
  // input, as at a splice, a few such moved the scores so far that `s.2.dts`'s first 1.2 MB twice
  // over had 18% of its second copy's groups wrong, from 6%)
  static constexpr int HF_VQ_PEAK = 89;  // (HF_VQ's largest magnitude)
  S32 hgw[32][16];
  S64 hge[32];
  U32 hg_n;
  // (the entries' distances in each group, hf_pair_dists()'s, a group's made where first asked for)
  double hpd[32][32][32];
  U32 hpd_on;
  // (the distances between their W to the power 1.5, d sqrt(d), as hf_bound_grp() computes them:
  // group g's with each other's, again where g's W moves)
  double hgd[32][32];
  void hg_dist(int g) {
    for (int f = 0; f < 32; f++) {
      double s2 = 0;
      for (int j = 0; j < 16; j++) {
        const double e = (double)hgw[g][j] - (double)hgw[f][j];
        s2 += e * e;
      }
      const double d = sqrt(s2);
      hgd[g][f] = hgd[f][g] = d * sqrt(d);
    }
  }
  void hg_learn(const S64* x, S64 sc, int t, int f) {
    if (sc <= 0) return;  // (a scale factor of 0, the tables' padding: no encoder's, but it parses)
    S64 mx = 0;
    for (int j = 0; j < 16; j++) mx = x[j] > mx ? x[j] : -x[j] > mx ? -x[j] : mx;
    if (mx > ((S64)8 * HF_VQ_PEAK * sc << (AQ_F - 4))) return;  // (x: HF_VQ's values times sc / 16)
    const int sh = 9 + imin(bit_len((U64)(hg_n >> 10)), 5);
    hg_n += hg_n < (1u << 30);
    const S64 den = sc << (sh + 3);  // (x / den: the vector u times 2^-sh, in 512ths)
    for (int k = 0; k < 2; k++) {
      const int g = k ? f : t;
      S64 n0 = 0, n1 = 0;
      for (int j = 0; j < 16; j++) {
        const S64 a = x[j] < 0 ? -((-x[j] + den / 2) / den) : (x[j] + den / 2) / den;
        n0 += (S64)hgw[g][j] * hgw[g][j];
        hgw[g][j] = (S32)clip((S64)hgw[g][j] + (k ? -a : a), -65535, 65535);
        n1 += (S64)hgw[g][j] * hgw[g][j];
      }
      hge[g] += (k ? 1 : -1) * ((S64)1 << (25 - sh)) - (n1 - n0);
      hg_dist(g);
    }
    S64 m = hge[0];
    for (int g = 1; g < 32; g++) m = hge[g] < m ? hge[g] : m;
    for (int g = 0; g < 32; g++) hge[g] = clip(hge[g] - m, 0, (S64)1 << 40);
  }
#ifdef DTSD_PVQDUMP
  // (./mk.sh pvqdump: a lossless WAV's ADPCM predictor, appended to $DTSD_PVQDUMP: its index as a
  // 32-bit integer and 32 bits of 0, the subband's LPC (lpc4()'s, 2^-24) as 64-bit ones;
  // tools/pvqblocks.py reads them)
  static void pvq_dump(const S64* alpha, int v) {
    static FILE* f = getenv("DTSD_PVQDUMP") ? fopen(getenv("DTSD_PVQDUMP"), "ab") : nullptr;
    if (!f) return;
    const S32 h[2] = {v, 0};
    fwrite(h, 4, 2, f);
    fwrite(alpha, 8, 4, f);
  }
#endif
#ifdef DTSD_LFEDUMP
  // (./mk.sh lfedump: an LFE value, appended to $DTSD_LFEDUMP: the value as a 32-bit
  // integer and 32 bits of 0, its step and the prototype's decimations by 32 around it (lfe_f3's)
  // as 64-bit ones; tools/lfefilter.py reads them)
  static void lfe_dump(const S64* f, S64 st, int v) {
    static FILE* fp = getenv("DTSD_LFEDUMP") ? fopen(getenv("DTSD_LFEDUMP"), "ab") : nullptr;
    if (!fp) return;
    const S32 h[2] = {v, 0};
    fwrite(h, 4, 2, fp);
    fwrite(&st, 8, 1, fp);
    fwrite(f, 8, LFE_N2, fp);
  }
#endif
#ifdef DTSD_HFDUMP
  // (./mk.sh hfdump: a lossless WAV's vector of 16 samples, appended to $DTSD_HFDUMP: its index and
  // scale factor as 32-bit integers, its samples as 64-bit ones; tools/hfgroups.py reads them)
  static void hf_dump(const S64* x, S64 sc, int v) {
    static FILE* f = getenv("DTSD_HFDUMP") ? fopen(getenv("DTSD_HFDUMP"), "ab") : nullptr;
    if (!f) return;
    const S32 h[2] = {v, (S32)sc};
    fwrite(h, 4, 2, f);
    fwrite(x, 8, 16, f);
  }
#endif
  S64 xcpl[CPL_N][NCH][NSMP];
  // the k taken: the first's, but where another's has been right clearly more often
  int cpl_k(int c) const {
    int k = 0;
    const U32* h = cpl_hit[c];
    for (int i = 1; i < CPL_K; i++)
      if (h[i] > h[k]) k = i;
    return h[k] > h[cpl_d(c)] + 16 + h[cpl_d(c)] / 32 ? k : cpl_d(c);
  }
  // channel ch's subband b (one of them: on) as k's: n values into y
  void cpl_values(int c, int ch, int k, S64* y, int n) const {
    for (int i = 0; i < n; i++) y[i] = xcpl[c][ch][i] + rshift(X[ch][c][i] * cpl_m(c, k), 23);
  }
  bool cpl_on(const CoreRec& r, int ch, int b) const {
    return b >= NSUB - CPL_N && b >= r.nvqsub[ch] && bound[ch] && !x96a && !ss_rule() && bind.up == 1;
  }
  // subband c's power over subband b's (NSUB - 1 - c's, one of them: on), in doublings, less o,
  // halved: in 0..hi (how much of the leak is left in it, the contexts of its scale factor and VQ
  // vector, where the others' want them as they were: -1)
  int cpl_leak(const CoreRec& r, int ch, int b, int o, int hi) const {
    if (!cpl_on(r, ch, b)) return -1;
    const int c = NSUB - 1 - b;
    S128 p0 = 0, p1 = 0;
    for (int i = 0; i < nsmp; i++) p0 += (S128)X[ch][c][i] * X[ch][c][i], p1 += (S128)X[ch][b][i] * X[ch][b][i];
    return clip((bitlen128((U128)(p0 + 1)) - bitlen128((U128)(p1 + 1)) - o) / 2, 0, hi);
  }
  int hm[NCH];             // whether the channel's scale factor before was the prediction (0 none, 1 it was, 2 not)
  // how often the RMS rule's scale factor was the core's and scale_ceil()'s (halved as they grow);
  // where the search was tried, found one and it wasn't rule k's, how often rule k's was the core's
  // and the search's (sfq[k], halved the same); the search's tries (to 2048)
  U32 sfr[2], sfq[2][2], ss_n;
  // how often a scale factor the search was tried on lay below its window (3 under the RMS rule's), of
  // how many (low_on())
  U32 sfl[2];
  // the same of the bit allocation search's ABITS (abits_search()) and the first guess's without
  // it, where it was tried; its tries; the ABITS (-s: those it said, those it said right)
  U32 abr[2], ab_n;
  // where the search's ABITS lie 2 or more from the staircase's (the first guess without it, or the
  // table's), how often the staircase's was right and the search's, by the search's sureness (0-3);
  // where they differ at all and the search's missed, how often the staircase's was right and the
  // last ABITS (ab2); each halved at 2^12
  U32 abq[4][2], ab2[2];
  bool abq_stair(int k) const { return abq[k][0] > abq[k][1] + 8 + abq[k][1] / 16; }
  void abq_add(int k, bool s, bool g) {
    abq[k][0] += s, abq[k][1] += g;
    if (abq[k][0] + abq[k][1] >= 1u << 12) abq[k][0] >>= 1, abq[k][1] >>= 1;
  }
  U64 n_ab, n_abs, n_abhit;
  int abfit_a[NCH][NSUB];   // the subframe's subbands' search's ABITS (-1 none), and its fit's distance
  U64 abfit_d[NCH][NSUB];
  struct AbCache {          // a bit allocation search's, where PMODE's made it (its window, its result,
    int g, a0, a1, p, G, gc, G1, Gc;  // its ab_g1 and ab_gc)
    U64 gd;
  } abc[NCH][NSUB];
  U32 pv_n, pv_none;        // (code_pvq(): the predictors tried, those of which none fit)
  // (one where none is tried: X96 codes the subband too, where none would fit; a 96/24 stream's
  // other subbands fit seldom past the first 4, and trying more there takes long for little)
  // (code_pvq()'s predictors tried where few fit: `96-24.dts`'s core 1.0% smaller than by 4, about
  // 3% slower)
  static constexpr int PV_KLOW = 32;
  void pv_nofit() {
    pv_none++;
    if (++pv_n >= 1u << 16) pv_n >>= 1, pv_none >>= 1;
  }
  U32 pvr[2];               // (code_pvq2(): how often the two-stage search's first was the predictor, the nearest of all's)
  U64 n_pv, n_pvhit;        // (-s: the predictors, those first)
  // the same of the HF VQ groups' nearest entries (code_hfvq()): how often one was near, of how
  // many tries; the tries (to 2048); the vectors
  U32 hfr[2], hf_n;
  U64 n_hfvq;
  S64 lpc[NCH][NSUB][4];   // the subframe's subbands' LPC (lpc4(): the encoder's ADPCM predictor is its search's, pvq_first())
  int lpg[NCH][NSUB];      // and its prediction gain in halvings
  int lpg4[NCH][NSUB];     // and in quarters of them
  U8 lrank[NCH][NSUB][VK_N];  // the subband's last rank of each kind (vqr's lh)
  KeyIx cand[4096];        // the candidates of a rank
  S64 pvqd[4096];          // (the ADPCM code book's distances: pvq_dists(); not on the stack, which a
                           // coroutine's yield copies, 64 KB at most: Lib3/coro3b.inc)
  // the LFE (64x) against the WAV's LFE channel (bind.lfe)
  int lfe_on;              // the subframe's LFE samples are coded against the audio's
  S64 lfe_g;               // the fixed filters' gain, Q15 (smp_start(): a DTS:X encoder's downmix's where so bound)
  int lfe_n;               // how many
  S64 lfe_n0;              // the first's place (its interpolation's 64 samples from 64 times it)
  S64 lfe_f[4 * NSSF][LFE_NF];  // each one's decimated samples around it (the last values after them: code_lfe()'s)
  S64 lfe_f3[4 * NSSF][LFE_N2];  // and the prototype's decimations by 32 around it (lfit3's, LFE_W2's)
  S64 lfe_past[LFE_NP];    // the last LFE values (times their steps; [0] the last)
  S64 lfe_p[4 * NSSF];     // and its prediction (the value times its step)
  int lfe_scl;             // their scale factor (the walk has it after them: coded with the first)
  S64 lfe_st;              // its step
  int lfe_hm, lfe_lp;      // whether the last was the prediction (as hm), the last less its prediction (as lp)
  // (where a decoded WAV's: how often each of five was the LFE's scale factor, the predictions' RMS
  // rule's, FFmpeg's encoder's rule on their peak (its LFE's, calc_one_scale() at ABITS 11, of the
  // peak in its units, 128 times the decoder's), the least whose step puts the peak within 3/32 of
  // 127 (FfEnc::scale_within_peak()), the RMS rule's on LFE_W2D's predictions, and the one whose
  // lattice the WAV's LFE fits (lfe_lattice()), halved as they grow: before the scale factor the
  // fifth's predictions after the subframe's first have only predictions for the values before,
  // and its inversion of FFmpeg's interpolation makes their errors grow (by 2 to 4 times a value),
  // where LFE_W2D's don't)
  U16 lfs_w[5];
  S64 lfe_cn[128], lfe_cv[128];  // the decimations by 32 by place (m mod 128: m, the value)
  S32 lq1, lq2;            // the last two LFE indexes (as q1, q2, qs, miss)
  S8 lqs;
  U8 lmiss;
  LfeFit<LFE_NT> lfit;      // the correction without the last values
  LfeFit<LFE_NF> lfit2;     // and with them
  // the first by the decimations by 32 (those halfway between too, where the LFE's rate aliases
  // the filter's band), forgetting 1/2048 a subframe, not 1/32, refitted every 16th subframe
  // (lfe_nsf): the encoders' filter doesn't change, so over a long stream that one learns it far
  // better, over a short one the first, learning faster, is often the better (`s.2.dts`'s LFE
  // values 99.3% the prediction from 96.7%, its core 2.6% smaller); and the DTS-HD MA encoders'
  // filter as fitted to `s.2.dts`'s values, LFE_W2 (lfe_fixed()), the fourth, the better of all
  // where it is their filter, from a stream's start (`s.2.dts`'s LFE 3184 bytes to 223, its core
  // 3.0% smaller in all; FFmpeg's master_audio_7.1_24bit.dts's LFE 262 to 44, its core 5.9%, its
  // filter the same; `Master Audio 7.1.dts`'s, no linear filter of its WAV's, 0.4% bigger); and the
  // fifth the same of FFmpeg's decodes, its LFE interpolation undone (since format 135 exactly,
  // lfe_dec(), where the WAV is at the core's rate; else, and before, LFE_W2D, a fit of it), the
  // better from a decoded WAV's first values, which the fits haven't learned yet; and the sixth the
  // third's at each place of a frame's values apart (lfit4, by lfe_pl): `Master Audio 7.1.dts`'s
  // encoder decimates its LFE by another filter at each (once learned, the fits at each place make
  // 93% of its values, the best of the rest 30%), refitted only where the first five missed more
  // than a quarter of the values since the last refit (lfe_poor(): refitted every time, they'd cost
  // nearly as much as all the others); the third leaves out the values far from its prediction
  // (LfeFit's RJ: the Cymatics stream's LFE 659 bytes smaller, where the others gained nothing)
  static constexpr int LFE_PL = 16;  // (a frame's values: 8 a 512-sample frame's)
  LfeFit<LFE_N2, LFE_K2, 7, true> lfit3;
  LfeFit<LFE_N2, LFE_K2, 7> lfit4[LFE_PL];
  U32 lfe_nsf;
  U8 lfe_pn[LFE_PL];        // (whether lfit4's has taken values since its last refit)
  int lfe_pl[4 * NSSF];     // (each value's place in its frame)
  // (and each one's half a step up, rounded: a DTS:X stream's encoder rounded its LFE values up, to
  // the integer at or above, so that their prediction, the nearest, was half a step low: the third
  // fit's 50% of its loud LFE values, half a step up 81%)
  U32 lfh[24];              // how often each one's prediction was the value (halved as they grow)
  U32 lfw[10], lfwn;        // (the first five's since the last refit, and the values)
  // (and since format 123 the same by the class of the best's prediction, its sign and how many
  // steps it is from 0, under 4, 16, 32 or more: lfc, 16 a value, halved at LFC_N values of the
  // class, lfcn; and how often the best's and its class's best's have been the value lately, lsw,
  // halved at LSW_N values, lswn: the class's taken where it has been LSW_M more often. The
  // Cymatics stream's fits err by sign and size differently: its best was the value 85.2% of the
  // time, the classes' 88.6%; `Master Audio 7.1.dts`'s 89.2% and 89.3%, the others' the same)
  static constexpr int LFC_N = 32, LSW_N = 256, LSW_M = 4;
  U32 lfc[8][24], lfcn[8];
  U32 lsw[2], lswn;
  // (and since format 127 each of the twelve again, less its error as a linear function of it, a +
  // b x / 64 in steps, for each sign of it (lfl, a and b in 2^-16): least mean squares, normalized,
  // learning at 1/LFL_R from the values within 2 steps of it; the 24 taken as the twelve were,
  // those as they are first. The Cymatics stream's fits err by sign and size: their values are a
  // little more than 0.7% bigger than the predictions, those near 0 a sixth of a step less)
  static constexpr int LFL_R = 32, LFE_NC = 24;
  S64 lfl[2][12][2];
  // X96's, where the frame has it and the WAV is at twice the core's rate: each bound channel's
  // 64-band analysis by block of the frame, and the core's decoded samples (X96's residuals are the
  // one less the other: model_x96.inc)
  int x96a;  // the frame's core pass keeps them
  int fb0;   // the subframe's first block in the frame
  S64 xa[NCH][NSUB96][128];
  S32 yc[NCH][NSUB][128];
  // Where X96 codes the core's subband too (residuals: from its X96START), the decode's value is the
  // core's plus X96's, each on its own lattice, and X96's step is like the core's (`96-24.dts`'s 0.3
  // to 0.7 of it in subbands 27-29, 0.9-2.1 in 30, 35-85 in 31, where its own decimation to 48 kHz
  // left the core little: its values there are 0.4 and 0.02 of the decode's): there the core's index
  // is the one whose value leaves the rest on X96's lattice (jx_search(), jx_fit()). From the frames
  // before: the lowest subband X96 codes (NSUB none yet); by subband, how often X96's step was
  // 2^(i/16 - 8) of the core's (jx_h, and their sum), and its vector's scale factor where it codes
  // the subband by its HF VQ (jx_hv); the core's values against the decode's (their products' sums:
  // the gain); the core subbands' steps, by subframe (0 none); the subframe's fit: 1 + X96's step's
  // place in jx_steps (0 none), else by each 16 samples, 1 + X96's vector's entry and its scale factor
  static constexpr S64 JX_LOW = 2458;  // (a gain under 0.6, in 2^-12: the core's values aren't the decode's)
  // Each counted as the stream goes, where the two differ (halved as they grow), by subband: how
  // often the decode's nearest index was the core's, and the fit's (jx_r); where the gain is under
  // JX_LOW, how often the scale factor was the audio's prediction, and the last one (jx_sr), and the
  // same of the ADPCM predictor, its search's first (jx_pr): the second taken where it has been
  // right more often, by a margin (jx_more())
  U32 jx_r[NSUB][2], jx_sr[NSUB][2], jx_pr[NSUB][2];
  // the same of the scale factor the fit's lattice says (jx_jr, against the prediction), and whether
  // X96 coded the subband by its vectors the last time
  U32 jx_jr[NSUB][2];
  U8 jx_wv[NCH][NSUB];
  int jx_lo;
  U32 jx_h[NSUB][256], jx_hn[NSUB], jx_hv[NSUB][256], jx_hvn[NSUB];
  U64 jx_hb[NSUB][4];  // (jx_h's bins that aren't 0, by bit)
  S128 jx_yw[NSUB], jx_ww[NSUB];
  S64 jx_cs[NSF][NCH][NSUB];
  int jx_k[NCH][NSUB], jx_vq[NCH][NSUB][NSMP / 16], jx_vs[NCH][NSUB][NSMP / 16];
  // Some decoded WAVs' bottom subband isn't the core's values there, but a filter of them (as if the
  // decode had been high-passed: `96-24.dts`'s, `ES 6.1 16bit.dts`'s): the decode less the core's
  // value, as the core's last 16 values, the decode's present one, its last 8 and its next 12 in the
  // subframe (0 past it) say it (least squares, LfeFit's: every channel's together, refitted each 64
  // samples; on `96-24.dts`'s, R^2 0.97; the next ones `ES 6.1 16bit.dts`'s core 2.1% smaller,
  // `96-24.dts`'s 0.3%, 4 of them 0.1%, 16 as 12); the decode less the fit's is the audio's value
  // where its index has been the core's more often than the decode's own, by a margin (b0r:
  // jx_tally()'s)
  static constexpr int B0_F = 12;
  static constexpr int B0_N = 25 + B0_F;
  LfeFit<B0_N, -1> b0fit;
  // Where X96 codes the core's subbands too, the core's values against the decode's, by channel and
  // subband from JE_B0 up: a least squares fit of the core's decoded values on the 64-band
  // analysis's of the subband about each, 3 either side (je_feat()), the older forgotten. The
  // core's input was the 96 kHz audio decimated, so near its top the core has little of the
  // decode's values and those shaped: the fit's are within half a step of the core's 83.5% of the
  // time in `96-24.dts`'s subband 30, the decode's 62.5%, and its predictors' median rank by their
  // LPC is 7, by the decode's 1147. The subframes it has had (to 255); the last one's last 3
  // values and the block after them (je_hx, je_ht: the values before the subframe); how often the
  // decode's LPC's first was the predictor, and the fit's values' (je_pr)
  static constexpr int JE_B0 = NSUB - 8, JE_W = 3, JE_N = 2 * JE_W + 1, JE_MIN = 4;
  LfeFit<JE_N, -1> je_fit[NCH][NSUB - JE_B0];
  U8 je_n[NCH][NSUB - JE_B0];
  S64 je_hx[NCH][NSUB - JE_B0][JE_W], je_ht[NCH][NSUB - JE_B0];
  U32 je_pr[NSUB][2], je_qn[NSUB][2];  // (and of the samples, how many, and how many the fit's index was: halved as they grow)
  // subband b's features at block t of the frame, the subframe's blocks t0 to t1 - 1 (0 past them,
  // and before them where the last subframe wasn't the one before)
  void je_feat(int ch, int b, int t, int t0, int t1, S64* f) const {
    const int k = b - JE_B0;
    const bool hv = je_ht[ch][k] == tframe + t0;
    for (int o = -JE_W; o <= JE_W; o++) {
      const int u = t + o;
      f[o + JE_W] = u >= t0 && u < t1 ? xa[ch][b][u] : u < t0 && u >= t0 - JE_W && hv ? je_hx[ch][k][JE_W + u - t0] : 0;
    }
  }
  // whether the fit has had JE_MIN subframes of channel ch's subband b (where X96 is there)
  bool je_on(int ch, int b) const { return x96a && b >= JE_B0 && je_n[ch][b - JE_B0] >= JE_MIN; }
  // the fit's value of the subframe's sample j
  S64 je_one(int ch, int b, int j) const {
    S64 f[JE_N];
    je_feat(ch, b, fb0 + j, fb0, fb0 + nsmp, f);
    return je_fit[ch][b - JE_B0].predict(f);
  }
  // the LPC (lpc4()) of channel ch's subband b's last 4 decoded values and the fit's of the
  // subframe's (into al, its gain in halvings *g and quarters *g4; the fit's values into y)
  void je_lpc(int ch, int b, S64* al, int* g, int* g4, S64* y) const {
    S64 x[4 + NSMP];
    for (int k = 0; k < 4; k++) x[3 - k] = shl64(yh[ch][b][k], AQ_F);
    for (int j = 0; j < nsmp; j++) x[4 + j] = y[j] = je_one(ch, b, j);
    lpc4(x, 4 + nsmp, al, g, g4);
  }
  // the gains PMODE goes by (lpg4's, pmode_gain()'s) of the fit's values
  void je_gains(int ch, int b, int* g4, int* pg) {
    S64 y[NSMP], al[4], hd[4];
    int g;
    je_lpc(ch, b, al, &g, g4, y);
    const int k = *g4;
    if (k < 2 || k > 9) {
      *pg = k < 2 ? 0 : 9;
      return;
    }
    for (int i = 0; i < 4; i++) hd[i] = shl64(yh[ch][b][i], AQ_F);
    S64* const d = this->pvqd;
    pvq_dists(al, d);
    *pg = clip(pvq_gain4(pvq_first(al, d), hd, y, nsmp) - 2, 1, 8);
  }
  // the subframe's core values decoded (yc): each X96-coded subband's fit again
  void je_learn(const CoreRec& r) {
    if (!x96a) return;
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bound[ch]) continue;
      for (int b = imax(jx_lo, JE_B0); b < r.nsubs[ch]; b++) {
        const int k = b - JE_B0;
        for (int j = 0; j < nsmp; j++) {
          S64 f[JE_N];
          je_feat(ch, b, fb0 + j, fb0, fb0 + nsmp, f);
          je_fit[ch][k].add(f, shl64(yc[ch][b][fb0 + j], AQ_F), (S64)1 << 22);
        }
        je_fit[ch][k].refit();
        je_n[ch][k] += je_n[ch][k] < 255;
        if (nsmp >= JE_W) {
          for (int i = 0; i < JE_W; i++) je_hx[ch][k][i] = xa[ch][b][fb0 + nsmp - JE_W + i];
          je_ht[ch][k] = tframe + fb0 + nsmp;
        }
      }
    }
  }
  S64 b0y[NCH][16], b0w[NCH][8];  // (2^-AQ_F; [0] the last)
  U32 b0r[2], b0n;
  U64 n_smp, n_hit, n_sf, n_sfhit;  // (-s: the samples, those the audio predicted; the same of the scale factors)
  U64 n_vq[VK_N], n_vqhit[VK_N];    // (and the ranks, those 0)
  // how often a subband sample's index wasn't the audio's, of how many (code_index()), by pm, hm, cb
  // (4 more where the fit of the core's values says another: code_q()) and nearness: in each cell
  // (nr, nl), and in each row (nl); in 256ths (HIT_ONE), a flag counted in the cells either side of
  // where its nearnesses lie. Halved as they pass their limits: a row's at 2^12 flags; a cell's
  // counts twice over, at 16 flags and at 2^18, its rate the geometric mean of the two, as the
  // encoder's arithmetic errs by more or less than the level says as the signal goes (format 71's
  // one count at 128: `s.2.dts`'s core 0.4% smaller than at 2^18, `96-24.dts`'s 0.1% bigger; with
  // both, at 64, those two 0.08% and 0.11% smaller still; at 16, with HIT_PRIOR 1 rather than 2,
  // FATE's samples 1.0-1.6% smaller, the 7-frame xll ones 0.3-0.7%, `s.2.dts`'s core 0.04%; since
  // format 106, with the calibration below, HIT_PRIOR 2 again: `s.2.dts`'s core 42 bytes smaller,
  // the short streams 8 bigger; at 3, 7 bytes bigger in all; the fast counts at 8 or 32 flags, or
  // the rows at 2^14, the short streams' and `s.2.dts`'s gains each the other's loss)
  static constexpr U32 HIT_ONE = 256, HIT_PRIOR = 2, HIT_LIMIT = 1u << 4, HIT_SLIMIT = 1u << 18, HIT_RLIMIT = 1u << 12;
  U32 hit_c[2][4][16][QB_N][QB_N][4], hit_r[2][4][16][QB_N][2];
  // (format 104) And then calibrated: by its miss probability's log2 (HAP_N bins, in halves until
  // format 107, one between two weighing on both by how near), the misses each bin's flags were
  // expected to make and those they made (2^26 a miss), its probability times their ratio ((made +
  // 2) / (expected + 2)); in two tables, one halved at 4 misses (expected and made; at 32 until
  // format 106: with HIT_PRIOR 2, at 16 the short streams 19 bytes smaller, at 8 45, at 4 54, at 2
  // the same), one at 4096, and of the fast one's, the slow one's and their mean (of the logs) the
  // one whose flags have cost the least so far (hap_k, 2^-16 bits): the cells' rates' geometric
  // mean, from counts that start empty, is not how often their flags miss (with the cap's fourth
  // bin, tc_fam.inc's, 12 bytes of it: the regression set 0.16% smaller, FATE's samples 0.4-1.8%,
  // the 7-frame xll ones 0.4-1.0%, `s.2.dts`'s core 0.10%, `dts_es.dts`'s 0.17% bigger; one table,
  // halved at 1024 misses, 0.08% smaller, by the cells' class of recent misses too 0.07%, by
  // quarters of a log2 0.07%). Since format 107 the bins are whole log2s, the fast table's prior 1
  // miss, and the tables the same for subbands with ADPCM prediction and without (by PMODE until
  // then): the short streams 22 bytes smaller, `s.2.dts`'s core 5; each alone 9-12 of it, the bins
  // by two log2s 17 bytes bigger than by one, the fast table's prior 1 alongside the rest 21
  // smaller
  static constexpr int HAP_N = 20;
  static constexpr U64 HAP_A = (U64)2 << 26, HAP_FAST = (U64)1 << 28, HAP_SLOW = (U64)1 << 38;
  U64 hap_e[2][HAP_N], hap_m[2][HAP_N];  // (the fast table and the slow one)
  S64 hap_k[3];
  // (format 108) And what is calibrated is the cells' rates mixed, not their mean: its log2 the sum
  // of the fast counts' rates' log2 (their mean's over the cells, by how near), the slow ones', the
  // rows' (their rates', by how near) and 1, each times a weight learned as the flags go (from 1/2,
  // 1/2, 0 and 0, by the gradient of the flag's cost, HMX_R of it), each weight the sum of one for
  // the mean's whole log2 and one for that in the subband (hmx_d, hmx_b; the X96 subbands' own),
  // no surer of a miss than 2^-0.05 nor of a hit than 2^-19: the regression set's short streams
  // 0.13% smaller, `s.2.dts`'s core 3 bytes bigger (with the weights for the log2 alone, 23 bytes
  // smaller in all, mixing the rates' logits 2, learning at 1/1000 or 1/500 21 and 12; with the
  // subband's alone 14 more, with both 37; the rows' rates 15 more; by the cells' class of recent
  // misses, 14 bigger; the cells where a channel coded before missed as an input too, 1 smaller;
  // at 1/1024, the short streams 13 smaller and `s.2.dts` 13 bigger; faster at a stream's start,
  // or slower, bigger; the calibration's tables' probabilities mixed in place of the cheaper's,
  // 171 bigger)
  static constexpr double HMX_R = 1.0 / 2048;
  double hmx_d[HAP_N][4], hmx_b[NSUB + NSUB96 + 1][HAP_N][4];
  // (format 109) A miss neither within HIT_NEAR of the audio's index nor alt (the audio's 4 or
  // more): by its distance past HIT_NEAR and its side, where so far such misses' distances would
  // have taken fewer bits than their magnitudes (fm_c: 2 for each bit of each one's length and 2
  // more, either way, halved past 2^16), else by its magnitude, as all were until then. Where a cut
  // stream's WAV ends, its last frame decoded alone (format 101), its far misses lie nearer the
  // audio's index than 0: `Master Audio 7.1.dts`'s core 2.0% smaller, `Master Audio 5.0
  // 96khz.dts`'s 1.8%, `mid.dtsc` 1.6%, `s2h.dtsc` 1.2%, the regression set's short streams 0.6%
  // (`s.2.dts`'s, uncut, the same); FATE's XXCH samples', where they don't, 18-76% bigger by
  // their distances alone, `open bitrate.dts`'s 11%. (The switch by core, X96 and the LFE apart,
  // the side as alt's or not, the last far miss's length as the length's context: within 2 bytes)
  U64 fm_c[2];
  // (format 80) An index's miss in the channels coded before, at the same subband's same sample:
  // where the WAV isn't what the encoder had, or a decoder's values aren't the core's, the channels
  // miss together (in 3 MB from `s.2.dts`'s middle, cut from it, a sample missed 0.11% of the time
  // where none had, 20% where one had, 85% where two; in a lossless WAV as it goes, the channels'
  // near ties, which the nearness has, 0.09% and 0.5%). So such a sample's hit flag has cells of its
  // own (the class cb + 8), counted always, and taken while they have coded those flags in fewer
  // bits than the others, by 4 bits and a 32nd (xm_loss); while they are, the others don't count
  // those flags: the short streams 0.2% smaller, `master_audio_7.1_24bit.dts`'s core 1.8%, the
  // 7-frame XLL samples with XCH and X96 2-3%, `s.2.dts`'s the same
  S64 xm_id[NSUB][NSMP];  // (the subframe each count is of)
  U8 xm_cnt[NSUB][NSMP];  // how many channels' indexes missed at subband b's sample j of the subframe so far
  S64 xm_loss[2];         // (those flags' costs, 2^-16 bits: by the others' cells, by their own; halved as they grow)
  // The encoder's arithmetic errs more in some subbands than in others, the same ones through a
  // stream (`s.2.dts`'s encoder's: those next to the multiples of 3 kHz, 15 and 16 most, then 23,
  // 24, 7 and 8, an index this near the boundary missing there 3 to 15 times as often as in
  // subband 5's; no direction to it): the nearness those counts are by (nl, its distance against
  // the level) is against the level times the subband's error's scale, which is learned as the
  // stream goes, in quarter doublings (from HC_S0, -9 before format 73: most streams' end up at -3
  // to -15, a decoded WAV's near -3; from 0, a short stream's were still on their way down at its
  // end, `s.2.dts`'s first 1.2 MB 0.3% bigger): a step up where the subband's misses since its
  // last step are more than those its cells expected by 1.25 times the root of those (once 2 or
  // more were), down where fewer by as much (to HC_MAX either way); both halved as those expected
  // pass 1024. After its first HC_K steps, by 2 times the root: a long stream's scales then stray
  // less (`s.2.dts`'s core 0.05% smaller after 16, the short streams the same; after 8, from -10,
  // 0.03% more). The core's subbands', then X96's. Where the WAV is at the core's rate, the core's
  // start where the lossless WAVs' end, on average (HC_B: `s.2.dts`'s, `Master Audio 7.1.dts`'s,
  // `master_audio_7.1_24bit.dts`'s and `3-1.dts`'s, three encoders': -4 to -8 next to the multiples
  // of 6 kHz, 7-8, 15-16 and 23-24, -12 to -16 elsewhere), and once the scale factor search's rule
  // holds (a decoded WAV's), from HC_S0 at the least (hc_dec): the short streams 0.1% smaller,
  // `master_audio_7.1_24bit.dts`'s core 0.7%, `3-1.dts`'s 0.5%, the 7-frame FATE samples, too short
  // for the rule, 2-4 bytes bigger
  static constexpr int HC_MAX = 32, HC_S0 = -10, HC_K = 8;
  // A decoded WAV's indexes miss by its own rounding, the same through the stream, more than by its
  // decoder's arithmetic, which errs by the level: their hit cells' nearness is against 2^DW_HC of
  // its LSBs (16-bit: 256 of the core's units) rather than against the level, under which its quiet
  // subbands' error scales ran down to -HC_MAX and stayed (a 16-bit decode's subbands 18-24, 2-3),
  // and the family's nearness (nl) against the level floored at 2^DW_NL (format 142: the 16-bit
  // decodes of FFmpeg's encodes 1.2-2.8% smaller, the 24-bit ones up to 4%; the scales then end
  // at -16 to 1)
  static constexpr int DW_HC = 21, DW_NL = 19;
  struct HitCal {
    S32 s;   // (the scale: 2^(s/4))
    U32 m;   // (the misses)
    U64 e;   // (those expected: the probabilities coded, in CO_ONE)
    U32 k;   // (its steps so far, to HC_K)
  };
  HitCal hcal[NSUB + NSUB96];
  int hc_dec;  // (whether the scales have been raised to HC_S0 at the least since ss_rule() first held)
  // unit u times hc's scale
  static S128 hc_unit(S128 u, int s) {
    static const int QM[4] = {128, 152, 181, 215};  // (2^(k/4), in 128ths)
    const int e = s >> 2;
    return (e >= 0 ? u << e : u >> -e) * QM[s & 3] >> 7;
  }
  // (the core's subbands' scales' mean, rounded)
  int hc_mean() const {
    int sm = 0;
    for (int k = 0; k < NSUB; k++) sm += hcal[k].s;
    return sm >= 0 ? (sm + NSUB / 2) / NSUB : -((-sm + NSUB / 2) / NSUB);
  }
  // (after a hit flag: hit, at probability pq of a miss)
  static void hc_learn(HitCal& hc, int hit, U32 pq) {
    hc.m += !hit, hc.e += pq;
    if (hc.e < 2 * (U64)CO_ONE) return;
    const S64 d = (S64)hc.m * CO_ONE - (S64)hc.e;  // ((d / CO_ONE)^2 > 1.25^2 e / CO_ONE)
    if (16 * (U128)((S128)d * d) > (hc.k < HC_K ? 25u : 64u) * (U128)hc.e * CO_ONE) {
      if (d > 0) hc.s += hc.s < HC_MAX;
      else hc.s -= hc.s > -HC_MAX;
      hc.m = 0, hc.e = 0;
      hc.k += hc.k < HC_K;
    } else if (hc.e > 1024 * (U64)CO_ONE) {
      hc.m >>= 1, hc.e >>= 1;
    }
  }

  // A lossless WAV's subband samples as the encoder's arithmetic makes them: its cosine modulation
  // runs through a 16-point FFT (subbands 2k and 31 - 2k its bin k), whose rounding errs by the
  // values on a bin's path, so most where the bin shares the most stages with bin 0 (subbands 0 and
  // 31, where the signal mostly is): bin 8 (subbands 15 and 16), then 4 and 12 (7, 8, 23, 24), then
  // 2, 6, 10 and 14 (3, 4, 11, 12, 19, 20, 27, 28); 72% of `s.2.dts`'s indexes' bytes are theirs.
  // Where the bottom subband has 90% of the level, the misses lie within a few 2^-23 of its value of
  // their boundary (subbands 1 to 4 within 1, 7 and 8 3 to 4, 23 and 24 6, 15 and 16 12), and part
  // of that is a bias: subband 7's lie 0 to 3 of them below it, 8's 0 to 4 above (none from the
  // other subbands, nor at other times: the noise is the rounding's). So a subband's value is taken
  // with m 2^-25 of the bottom subband's value added, m from -12 to 12: the m that has made the
  // audio's indexes clearly more often (by LK_H, the counts halved each LK_T samples whose index an m
  // could change), from where the encoders' are, the WAV at the core's rate (`s.2.dts`'s,
  // `Master Audio 7.1.dts`'s, `master_audio_7.1_24bit.dts`'s, `3-1.dts`'s, the same signs: subband 7's
  // -6 to -9, 8's 3 to 6, 15 and 16 -7 to 9), else from 0 (`s.2.dts`'s core 0.7% smaller, the
  // other lossless WAVs' 0.1-0.3%)
  static constexpr int LK_M = 12, LK_N = 2 * LK_M + 1, LK_H = 8, LK_T = 16384;
  S8 lk_c[NSUB];                     // (each subband's m)
  U32 lk_s[NSUB][LK_N], lk_t[NSUB];  // (each m's count of the indexes it made the audio's; how many counted)
  // (after an index v of subband b: x its value without the bias, x0 the bottom subband's, p its
  // prediction, ss, lo, hi requant()'s; qp, alt, dist requant()'s of the value with it)
  void bias_learn(int b, S64 x, S64 x0, S64 p, S64 ss, S64 lo, S64 hi, S64 v, S64 qp, S64 alt, S128 dist) {
    if (v != qp && v != alt) return;
    const S64 a0 = x0 < 0 ? -x0 : x0;
    if ((((S128)a0 * (2 * LK_M) << 22) >> 25) + ((S128)1 << 22) < dist) return;  // (no m makes another index)
    for (int m = -LK_M; m <= LK_M; m++) {
      S128 d;
      S64 o;
      lk_s[b][m + LK_M] += requant(x + rshift(x0 * m, 25) - p, ss, lo, hi, &d, &o) == v;
    }
    if (++lk_t[b] >= LK_T) {
      lk_t[b] >>= 1;
      for (int k = 0; k < LK_N; k++) lk_s[b][k] >>= 1;
    }
    int best = lk_c[b] + LK_M;
    for (int k = 0; k < LK_N; k++)
      if (lk_s[b][k] > lk_s[b][best] + LK_H || (lk_s[b][k] > lk_s[b][best] && k == LK_M)) best = k;
    lk_c[b] = (S8)(best - LK_M);
  }
  // The noise goes by the values on the subband's path: at sample j of channel ch, of the 16 bins'
  // energies (the squares of subbands 2k and 31 - 2k), the root of a fifth of the sum of all of them,
  // of those that share the bin's first FFT stage, its first two, its first three, and its own, into
  // plev (for the indexes' nearness, against the level of all of them: `s.2.dts`'s core 0.14%
  // smaller; weighted 1, 2, 4, 8 by stage 0.1% bigger, 1, 4, 16, 64 0.6%)
  // (since format 87 bin 0's in the stages it shares with the other even bins as hf_stair2() has
  // its bottom subband's value, the binade's: those stages' sums are its value's halves, quarters
  // and eighths, their rounding in units of its last place, where the first stage's, the FFT's
  // inputs, is the twiddled values': `s.2.dts`'s core 0.13% smaller, its even bins' subbands'
  // indexes; in the first stage too 0.1% less, the odd bins' bigger; each bin's own last stage by
  // its own binades, or the first stage's weight doubled or halved, bigger)
  void path_levels(int ch, int j) {
    U128 E[16], S1[2] = {0, 0}, S2[4] = {0, 0, 0, 0}, S3[8] = {0, 0, 0, 0, 0, 0, 0, 0}, T = 0;
    for (int k = 0; k < 16; k++) {
      const S64 a = X[ch][2 * k][j], c = X[ch][31 - 2 * k][j];
      E[k] = (U128)((S128)a * a) + (U128)((S128)c * c);
      T += E[k];
      if (!k) E[0] = hf_stair2(a) + (U128)((S128)c * c);
      S1[k & 1] += E[k], S2[k & 3] += E[k], S3[k & 7] += E[k];
    }
    for (int b = 0; b < NSUB; b++) {
      const int k = b & 1 ? (31 - b) / 2 : b / 2;
      plev[ch][b][j] = isqrt128((T + S1[k & 1] + S2[k & 3] + S3[k & 7] + E[k]) / 5);
    }
  }

  // A twin: a core channel bound to the same WAV channels by the same gains as an earlier one, or,
  // for a subframe, one whose analysis there is an earlier one's and not all 0 (dtwin_find()). The
  // encoder's input is the same for both, and so, nearly always, are its choices: each side field of
  // a twin's coded first as whether it's the earlier channel's (twin_flag()), an index as the earlier
  // one's where its step and prediction are the same (code_q()). (The 9 short samples with twins by
  // their binding 22% smaller, 12272 bytes to 9573: `cx_xbr_xxch_71_24_48_3840.dts` 4035 to 2734;
  // `master_audio_7.1_24bit.dts`, 4 of whose channels are the same for 8% of it, 1.7% smaller; a
  // silent subframe's channels as twins, `Master Audio 7.1.dts` 0.4% bigger.)
  int twin[NCH];   // (the earlier channel: -1 none)
  int dtwin[NCH];  // (else the subframe's: an earlier one of the pass whose analysis is this one's)
  U16 tw_n[6][2];  // (by field, twin_field()'s k: how often the twin's value wasn't (0) and was (1))
  // A lossless WAV's scale factors (code_scale()): the encoder's RMS rule (scale_pred_ms()) on the
  // values with the FFT's noise in them, its power at each sample SP_K of the bottom subband's
  // binade's (hf_stair2()) in the top subband, else SP_K2 (the HF VQ subbands 28-30) or SP_K3 (the
  // rest, by 2^((s - m) / 8), s the subband's index error scale and m the subbands' mean, hc_mean():
  // the scale says how far the WAV's path is from the encoder's there, and so its noise; the
  // regression set's 18 streams and `s.2.dts` 0.12% smaller, `s.2.dts`'s core 0.19%; at 2^-42,
  // SP_K3's best without it, by 2^((s - m) / 4) 0.15% bigger than by 2^((s - m) / 8), by
  // 2^(3 (s - m) / 16), 2^((s - m) / 12) or 2^((s - m) / 16) 0.01-0.03%; the HF VQ subbands by their
  // FFT partner's (31 - b) too 0.14%, the top subband by subband 0's 0.25%) of the square of the
  // subband's FFT path's level (path_levels()); their mean square taken as a normal variable, the
  // mean and variance theirs: each entry's cell's probability
  // (sf_post()). Where the likeliest's is under SP_PT, coded by rank, each "this one?" at its share
  // of what those before left, past SP_R as f_scl's difference from the likeliest (sf_post_code());
  // the share calibrated (since format 105): by rank (the first, the second, the rest) and SP_NB
  // buckets of its logit, -8 to 8 (24 until format 107, 6 bytes bigger; 8 as 12), its odds times
  // the bucket's hits made against those expected (the shares' sum) over its misses so (each with a
  // prior of 2), between its bucket's and the nearer neighbour's by how far it lies from its
  // bucket's middle, the counts halved past SP_LIM (where they had been mixed with the share at 16,
  // halved past 1023, which leaves the bucket's rate as its counts grow, not the share's own odds
  // within it: `s.2.dts`'s core 0.13% smaller, its scale factors 0.7%, the other streams within 3
  // bytes; without the neighbours 7 bytes less of it, halved past 1023 4 less, past 255 25 bytes
  // bigger; a prior of 1 or 4 as 2); surer than that, the RMS rule's hit flag
  // as before. And only where the stream's indexes have been the audio's at least SP_HPM thousandths
  // of the time, after SP_HN of them (sp_on()): the noise is the analysis's where the WAV is the
  // encoder's input to the bit, its indexes then the audio's 99.66-99.92% of the time (the lossless
  // WAVs' streams at hand), and a WAV that isn't (FATE's clipped decodes, 79-83%) or another
  // encoder's (FFmpeg's 7-frame DTS-HD MA samples, 97.5-99.2%) has noise of its own: without the
  // switch, their cores 1-7% bigger (the counts learning from nothing what f_scl's tuned hit flag
  // knows), FATE's XXCH samples 3.0-5.3%. `s.2.dts`'s core 5.7% smaller, its scale factors 24% (the
  // top subband's 12188 bytes to 9409, subbands 28-30's 4539 to 3275, the rest's 4042 to 3157; the
  // top subband's alone 2.9%; at the RMS rule's own noise there, 3/4 of 2^-40, `s.2.dts`'s first 3
  // MB and 3 MB of its middle 0.7% bigger than at 2.4 2^-40); the other lossless WAVs' 0.1-2.9%
  // (`mid.dtsc`'s most), the short samples' up to 2.3% bigger. Every scale factor this way, the
  // lossless WAVs' short streams 1.2-1.8% bigger; behind a switch by whether the likeliest or the
  // RMS rule's guess has been the encoder's more often, `s.2.dts`'s core 2.3% bigger; the noise's
  // power adapted to each stream (a step toward the likelier of 2^(1/4) times it and 2^(-1/4), by
  // how much), 12 bytes less in all; the switch at 99% the same in all, at 98% 23 bytes more
  static constexpr int SP_R = 6, SP_NB = 12, SP_W = 8, SP_LIM = 4095;
  static constexpr double SP_K = 2.4 / 1099511627776.0;   // (2.4 2^-40; 3 2^-40 0.4% bigger)
  static constexpr double SP_K2 = 1.0 / 8796093022208.0;  // (2^-43)
  static constexpr double SP_K3 = 1.0 / 17592186044416.0;  // (2^-44; 2^-43 0.01% bigger, 2^-45 0.05%)
  static constexpr double SP_PT = 0.999;
  // (the top subband's, subbands 28-30's, the rest's: by rank and bucket, the hits, the misses, the
  // hits expected and the misses so)
  double sp_x[3][3][SP_NB][4];
  // (a bucket's calibration: its hits made against expected over its misses so, with a prior of 2)
  static double sp_k(const double* x) { return (x[1] + 2.0) * (x[2] + 2.0) / ((x[0] + 2.0) * (x[3] + 2.0)); }
  static constexpr U64 SP_HN = 2560, SP_HPM = 995;  // (a 5.1 frame's indexes)
  bool sp_on() const { return n_smp >= SP_HN && n_hit * 1000 >= n_smp * SP_HPM; }
  double sp_t[SP_NB - 1];    // (the buckets' edges)
  S32 tw_pr[NCH][NSUB][8];  // (each channel's ADPCM predictions in the subsubframe, and its decoded
  S32 tw_y[NCH][NSUB][8];   // values: its twins' after it)
  U32 tw_t[2];  // (how often the audio's index was a twin's where the earlier channel's decoded value's
                // wasn't, and the other way around)
  int twin_of(int ch) const { return ch < NCH ? (twin[ch] >= 0 ? twin[ch] : dtwin[ch]) : -1; }
  // whether v is the twin's value tv, at the field k's odds so far (v then tv)
  bool twin_flag(int k, S64& v, S64 tv) {
    U16* const n = tw_n[k];
    U32 p = (U32)((U64)(2 * n[0] + 1) * CO_ONE / (2 * ((U64)n[0] + n[1]) + 2));
    p = p < 1 ? 1 : p > CO_ONE - 1 ? CO_ONE - 1 : p;
    const int hit = c->code_bit(p, ENC && v == tv);
    if (++n[hit] >= 1024) n[0] >>= 1, n[1] >>= 1;
    if (hit) v = tv;
    return hit;
  }

  // FFmpeg's encoder's bit allocations for the core's frame r, its subframe s (ffenc.inc): where the
  // core's code books have been its, the frame as its are (one subframe of 16 blocks, at 32, 44.1 or
  // 48 kHz; to 6 channels, each a WAV channel as it is), from the WAV's 512 samples from o (where
  // its first block's analysis ends), and the 512 before them (FFmpeg's s32: 24 bits shifted up)
  bool ff_frame(const CoreRec& r, const CoreSub& s, S64 o) {
    ffl_ok = 0, ffr_ok = 0;
    if (!B::sel_ff || r.nsubfs != 1 || r.nblks != 15 || s.nssc != 2 || r.npchs > FfEnc::MAXC || bind.up != 1 || wav->dec != 1)
      return false;
    const int ri = r.sfreq == 13 ? 2 : r.sfreq == 8 ? 1 : r.sfreq == 3 ? 0 : -1;
    if (ri < 0) return false;
    for (int ch = 0; ch < r.npchs; ch++)
      if (bind.map[ch] < 0 || bind.map2[ch] >= 0 || bind.map[ch] >= wav->nch) return false;
    if (!ffe) {
      ffe = (FfEnc*)xmalloc(sizeof(FfEnc));
      ffe->init();
    }
    ffe->set(r.npchs, ri, 8 * (r.fsize + 1), r.lff != 0);
    const bool rd = wav->bps >= 3 && !(ffr_c[0] >= 64 && ffr_c[0] > 4 * (U64)ffr_c[1]);
    if (rd && !ffr) {
      ffr = (FfEnc*)xmalloc(sizeof(FfEnc));
      ffr->init();
    }
    if (rd) ffr->set(r.npchs, ri, 8 * (r.fsize + 1), r.lff != 0);
    wav->need(o - 512, o + 512);
    for (int ch = 0; ch < r.npchs; ch++) {
      S32 buf[1024];
      const S32* x = wav->span(bind.map[ch], o - 512, 1024, buf);
      for (int i = 0; i < 512; i++) ffe->hist[ch][i] = (S32)((U32)x[i] << 8), ffe->in[ch][i] = (S32)((U32)x[512 + i] << 8);
      for (int i = 0; rd && i < 1024; i++) {
        const S32 v = imin((S32)((((S64)x[i] + 128) >> 8) * 256), 0x7FFF00);  // (as 16 bits, clipped)
        (i < 512 ? ffr->hist[ch][i] : ffr->in[ch][i - 512]) = (S32)((U32)v << 8);
      }
    }
    ffe->frame();
    if (rd) ffr->transform(), ffr_ok = 1;
    ffl_ok = 0;
    if (r.lff == 2) {
      int nc = 0;
      for (int c = 0; c < wav->nch && nc < 2; c++) {
        bool used = false;
        for (int ch = 0; ch < r.npchs; ch++) used |= bind.map[ch] == c;
        if (used) continue;
        S32 b1[512], b2[512], b3[512], hl[512], lx[512];
        const S32* const x = wav->span(c, o, 512, b1);
        const S32* const h0 = wav->span(wav->nch - 1, o - 513, 512, b2);
        const S32* const h1 = wav->span(c, o - 512, 512, b3);
        for (int i = 0; i < 512; i++) lx[i] = (S32)((U32)x[i] << 8);
        for (int k = 0; k < 2; k++) {
          for (int i = 0; i < 512; i++) hl[i] = (S32)((U32)(k ? h1 : h0)[i] << 8);
          if (!k) hl[0] = 0;  // (input[-1]: the memory before the frame's buffer, as 0)
          FfEnc::lfe_down(hl, lx, ffl[2 * nc + k]);
          ffl_v[2 * nc + k] = 1;
        }
        nc++;
      }
      for (int k = 2 * nc; k < FFL_N; k++) ffl_v[k] = 0;
      ffl_ok = nc > 0;
    }
    return true;
  }
  // the quantization of FFmpeg's LFE values at scale factor sf (its LFE's ABITS 11's step): q, or
  // false where its exponent is out of range
  static bool ffl_quant(int sf, FfSoft& q) {
    const int ns = sf & 127, e = FE_SF_INV[ns].e + FE_STEP_INV[11].e - 17;
    if (e < 1 || e > 30) return false;
    q = FfSoft{fe_mul32(FE_SF_INV[ns].m, FE_STEP_INV[11].m), e};
    return true;
  }
  // (each candidate's values against the subframe's s, at its scale factor sf: after the walk has
  // them all)
  void ffl_tally(const CoreSub& s, int sf) {
    FfSoft q;
    if (!ffl_ok || !ffl_quant(sf, q)) return;
    for (int i = 0; i < 8; i++) {
      for (int k = 0; k < FFL_N; k++) ffl_h[k] += ffl_v[k] && (S8)(fe_quant(ffl[k][i], q) & 0xff) == s.lfe[i];
      ffl_n++;
    }
    if (ffl_n >= 1u << 16) {
      ffl_n >>= 1;
      for (int k = 0; k < FFL_N; k++) ffl_h[k] >>= 1;
    }
  }
  // channel ch's subband b's ABITS v (in lo..hi) against FFmpeg's encoder's made again: whether it
  // is that, by how near its SNR lies the nearer end of its range, else whether the ABITS across
  // that end; false where neither (the family codes it then, as any)
  bool ff_abits(int ch, int b, S64& v, S64 lo, S64 hi) {
    int dir;
    const int ga = ffe->abits[ch][b], mb = ffe->margin(ch, b, &dir), al = ffe->lattice(ch, b);
    const int lc = !al ? 0 : al == ga ? 1 : al == ga + 1 ? 2 : al == ga - 1 ? 3 : 0;
    if (ga < lo || ga > hi) return false;
    n_ff++;
    if (ff_flag(ffn[ga >= 8][mb][lc][dir > 0], v, ga)) return n_ffhit++, true;
    const int g2 = lc == 2 ? ga + 1 : lc == 3 ? ga - 1 : ga + dir;
    return g2 >= lo && g2 <= hi && ff_flag(ffd[mb][lc], v, g2);
  }
  // whether v is tv, by the counts n (of not, of so), v then tv
  bool ff_flag(U16* n, S64& v, S64 tv) {
    U32 p = (U32)((U64)(2 * n[0] + 1) * CO_ONE / (2 * ((U64)n[0] + n[1]) + 2));
    p = p < 1 ? 1 : p > CO_ONE - 1 ? CO_ONE - 1 : p;
    const int hit = c->code_bit(p, ENC && v == tv);
    if (++n[hit] >= 4096) n[0] >>= 1, n[1] >>= 1;
    if (hit) v = tv;
    return hit;
  }

  void smp_start(WavIn* w, const Bind& b) {
    wav = w;
    bind = bind2 = b;
    for (int ch = 0; ch < NCH; ch++) {
      if (b.alt[ch]) bind2.p1[ch] = b.pa1[ch], bind2.p2[ch] = b.pa2[ch];
      gsel[ch] = 0, gsc[ch] = 0, gh[ch][0] = gh[ch][1] = 0;
      twin[ch] = -1;
      for (int e = 0; e < ch && twin[ch] < 0 && b.map[ch] >= 0 && !b.alt[ch]; e++)
        if (b.map[e] == b.map[ch] && b.map2[e] == b.map2[ch] && b.g1[e] == b.g1[ch] && b.g2[e] == b.g2[ch] && b.p1[e] == b.p1[ch] &&
            b.p2[e] == b.p2[ch] && !b.alt[e])
          twin[ch] = e;
    }
    for (int k = 0; k < 6; k++) tw_n[k][0] = 1, tw_n[k][1] = 7;
    ffe_ok = 0, n_ff = n_ffhit = 0;
    ffr_ok = 0, ffr_c[0] = ffr_c[1] = 0, ffr_l[0] = ffr_l[1] = 0;
    abk[0] = abk[1] = ab_ex[0] = ab_ex[1] = 0, ab_g1 = ab_gc = 0;
    for (int k = 0; k < 4; k++) abq[k][0] = abq[k][1] = 0;
    ab2[0] = ab2[1] = 0;
    qff_c[0] = qff_c[1] = qff_n[0] = qff_n[1] = 0;
    ffl_ok = ffl_use = 0, ffl_n = 0;
    // (since format 140 the two fixed filters' predictions, LFE_W2's and LFE_W2D's, times the DTS:X
    // encoders' downmix gain, 23170/32768, where the binding mixes two WAV channels with a gain before
    // the mix, as Call::pick_level() takes for their sign: the Cymatics stream's LFE runs at 0.69-0.70
    // of LFE_W2's where it is loud, as the core's channels do at 0.707092 of the WAV's, and those two
    // predictions were the value 43% of the time, in its silences; its LFE 1.4% smaller, its core
    // 0.19%, at 0.695 a little less. Its filter is not quite theirs, though: that gain falls to
    // 0.50-0.61 over frames 7000-13000. The fits' errors learnt from larger misses too, up to a
    // quarter of the prediction, so that theirs would learn the gain: its LFE 3.6% bigger)
    lfe_g = BIND_Q15;
    for (int ch = 0; ch < NCH; ch++)
      if (b.map[ch] >= 0 && b.map2[ch] >= 0 && (b.p1[ch] != BIND_Q15 || b.p2[ch] != BIND_Q15 || b.alt[ch])) lfe_g = 23170;
    for (int k = 0; k < FFL_N; k++) ffl_h[k] = 0, ffl_v[k] = 0;
    for (int m = 0; m < FfEnc::NMG; m++)
      for (int c = 0; c < 4; c++) {
        for (int g = 0; g < 2; g++)
          for (int d = 0; d < 2; d++) ffn[g][m][c][d][0] = 1, ffn[g][m][c][d][1] = 7;
        ffd[m][c][0] = 1, ffd[m][c][1] = 7;
      }
    memset(ffs_w, 0, sizeof ffs_w);
    tw_t[0] = tw_t[1] = 0;
    memset(sp_x, 0, sizeof sp_x);
    {  // (MC's samples: the quantiles in an order of each value's own, by an LCG's Fisher-Yates)
      U64 rs = 0x243F6A8885A308D3ull;
      for (int j = 0; j < 16; j++) {
        int pm[MC_N];
        for (int t = 0; t < MC_N; t++) pm[t] = t;
        for (int t = MC_N - 1; t > 0; t--) {
          rs = rs * 6364136223846793005ull + 1442695040888963407ull;
          const int u = (int)((rs >> 33) % (U64)(t + 1)), w = pm[t];
          pm[t] = pm[u], pm[u] = w;
        }
        for (int t = 0; t < MC_N; t++) mcz[t][j] = MC_Q[pm[t]];
        for (int t = 0; t < MC_N + 4; t++) mczt[j][t] = mcz[t & (MC_N - 1)][j];
      }
    }
    for (int k = 1; k < SP_NB; k++) {  // (det_exp_neg(): the same everywhere)
      const double z = -8.0 + 16.0 * k / SP_NB;
      sp_t[k - 1] = z < 0 ? det_exp_neg(z) / (1.0 + det_exp_neg(z)) : 1.0 / (1.0 + det_exp_neg(-z));
    }
    tnext = 0;
    memcpy(hgw, HF_GW, sizeof hgw);
    memcpy(hge, HF_GE, sizeof hge);
    for (int g = 0; g < 32; g++) hg_dist(g);
    hg_n = 0;
    static const S8 HC_B[NSUB] = {-12, -14, -15, -14, -13, -16, -15, -8, -7, -14, -15, -12, -12, -14, -13, -4,
                                  -4,  -12, -13, -9,  -11, -14, -12, -6, -7, -12, -13, -9,  -10, -10, -10, -10};
    for (int k = 0; k < NSUB + NSUB96; k++) hcal[k].s = k < NSUB && b.up == 1 ? HC_B[k] : HC_S0;
    hc_dec = 0;
    hpd_on = 0;
    static const S8 LK0[NSUB] = {0, 0, 0, -1, 1, 0, 0, -7, 5, 0, 0, 0, 0, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0, -1, 2};
    for (int k = 0; k < NSUB; k++) lk_c[k] = b.up == 1 ? LK0[k] : 0;
    memset(lk_s, 0, sizeof lk_s), memset(lk_t, 0, sizeof lk_t);
    for (int i = 0; i < 128; i++) lfe_cn[i] = -1;
    lfe_nsf = 0;
    lfe_lp = 9;
    lfs_w[0] = lfs_w[1] = lfs_w[2] = lfs_w[3] = lfs_w[4] = 0;
    jx_lo = NSUB;
    for (int ch = 0; ch < NCH; ch++)
      for (int k = 0; k < NSUB - JE_B0; k++) je_n[ch][k] = 0, je_ht[ch][k] = -1;
    memset(je_pr, 0, sizeof je_pr);
    memset(je_qn, 0, sizeof je_qn);
  }
  // the LFE's decimated sample n (64x: its window ends at 64 n + the offset)
  S64 lfe_d(S64 n) { return lfe_d2(2 * n); }
  // the prototype's decimation by 32 whose window ends at 32 m + the offset (m even: the LFE's own)
  S64 lfe_d2(S64 m) {
    const int i = (int)(m & 127);
    if (lfe_cn[i] != m) lfe_cn[i] = m, lfe_cv[i] = lfe_decim(*wav, bind.lfe, 32 * bind.up * m + bind.off, bind.up);
    return lfe_cv[i];
  }
  // a core frame not coded as records (kept as bytes): its blocks go by
  void skip_blocks(int n) { tnext += n; }
  // whether block t's analysis window lies past the WAV's end by more than WAV_PAST samples (at the
  // core's rate), and the continuation there isn't the encoder's input: a stream cut from a longer
  // one ends with its WAV, but its last frame, 512 samples ahead, has the audio that went on, which
  // no continuation makes; there an index is coded without the audio (past_on, a bit, as smp_subframe()
  // finds the continuation making fewer than half of those indexes)
  // (past vend_e, while the frames before a cut stream's last take the continuation from there:
  // VEND_PAST, the one that makes the frames before most of their indexes; 128 to 384 tried)
  static constexpr int VEND_PAST = 256;
  bool past_far(S64 t) const {
    return wav && 32 * bind.up * t + bind.off - (vend_on ? vend_e : (S64)wav->nsamp) > (vend_on ? VEND_PAST : WAV_PAST) * bind.up;
  }
  bool past_end(S64 t) const { return (vend_on ? vend_past : past_on) && past_far(t); }
  // every channel but the LFE's samples from vend_e to the WAV's end: the continuation from vend_e
  // (on), or their own again (the continuation of the WAV's end fitted before, on its own samples)
  void vend_swap(bool on) {
    WavIn& w = *wav;
    const S64 n = (S64)w.nsamp - vend_e;
    if (on) {
      for (int c = 0; w.ext && c < w.nch; c++)
        if (w.ext_done[c] < 0) w.ext_more(c, 0);
      vend_buf = (S32*)xmalloc(sizeof(S32) * (size_t)(w.nch * n));
      S32* const run = (S32*)xmalloc(sizeof(S32) * (size_t)n);
      for (int c = 0; c < w.nch; c++) {
        S32* const sv = vend_buf + (size_t)c * n;
        for (S64 k = 0; k < n; k++) sv[k] = w.ring[c][(vend_e + k) & (WAV_WIN - 1)];
        if (c == bind.lfe) continue;
        S64 q[WAV_LPC + 1], qa;
        wav_fit(w, c, vend_e, q, qa);
        wav_run(w, c, vend_e, q, qa, run, (int)n);
        for (S64 k = 0; k < n; k++) w.ring[c][(vend_e + k) & (WAV_WIN - 1)] = run[k];
      }
      free(run);
      vend_ag = w.ana_got, w.ana_got = imin64(w.ana_got, vend_e);
    } else {
      for (int c = 0; c < w.nch; c++)
        for (S64 k = 0; k < n; k++) w.ring[c][(vend_e + k) & (WAV_WIN - 1)] = vend_buf[(size_t)c * n + k];
      free(vend_buf);
      vend_buf = nullptr;
      w.ana_got = vend_ag;
    }
    vend_on = on;
  }
  // (c) of the subframe's samples j0 on, those of its bound channels' subbands that neither predict
  // nor are VQ ones whose index the analysis makes (as ext_hits())
  int vend_hits(const CoreRec& r, const CoreSub& s, S64 o0, int j0, int* hp = nullptr, int* np = nullptr) {
    int h = 0;
    S64 x[64];
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bind.bound(ch, wav->nch)) continue;
      for (int j = j0; j < nsmp; j++) {
        analyze_bound(*wav, gsel[ch] ? bind2 : bind, ch, o0 + 32 * bind.up * j, fir, x, x96a ? NSUB96 : NSUB, !ss_rule() && !x96a);
        for (int b = 0; b < r.nvqsub[ch]; b++) {
          const int a = s.abits[ch][b];
          if (!a || s.pmode[ch][b]) continue;
          S64 qlo, qhi, alt;
          S128 dist;
          quant_range(r, ch, a, qlo, qhi);
          const int hit = requant(x[b], step_scale(r, s, ch, b, j >> 3), qlo, qhi, &dist, &alt) == s.q[ch][b][j];
          h += hit;
          if (hp && past_far(t0 + j)) *hp += hit, ++*np;
        }
      }
    }
    return h;
  }

  // --- the subframe's audio --------------------------------------------------------------------------

  // which of its pairs of gains before the mix mixed channel ch's subframe takes: the other, where
  // it has made more of the channel's indexes lately (the difference, a quarter of it gone each
  // subframe, past 1). (A DTS:X encoder put its sides into the core's surrounds times 32767/32768,
  // rounded: the Cymatics stream's. Where the sides are under 2^14 that is them as they are, and
  // a binding by a stretch of those can't tell the two apart: the stream's frames 3000-5999,
  // bound by their quiet start, take 1, and this the other from their 22nd frame: 10611 bytes,
  // 21449 in format 121)
  void gain_pick(int ch) {
    gsc[ch] += (S32)gh[ch][1] - (S32)gh[ch][0] - gsc[ch] / 4;
    if (gsc[ch] > 1) gsel[ch] ^= 1, gsc[ch] = 0;
    gh[ch][0] = gh[ch][1] = 0;
  }
  // the subframe sf of the frame c starts (its SSC known): the audio's subband samples of its bound
  // channels, and their subbands' energy and how it goes
  void smp_subframe(const CoreRec& r, int sf) {
    const CoreSub& s = r.sub[sf];
    level_start(r.xpass ? 1 : 0);
    t0 = tframe + r.nblk;
    fb0 = r.nblk;
    nsmp = s.nssc * 8;
    // (as the header says: whether X96 is there comes after the core's subframes)
    if (!r.xpass) x96a = r.ext && r.ext_id == EXT_X96 && bind.up == 2 && wav && wav->ok;
    if (x96a) jx_steps.make();
    if (x96a) memset(jx_cs[sf], 0, sizeof jx_cs[sf]);
    fir = r.filts ? FIR_PERFECT : FIR_NONPERFECT;
    for (int ch = 0; ch < NCH; ch++) bound[ch] = 0, dtwin[ch] = -1;
    lfe_on = 0;
    if (!r.xpass) ffl_ok = ffl_use = 0;
    if (!wav || !wav->ok) return;
    const int up = bind.up;
    const S64 o0 = 32 * up * t0 + bind.off, hs = o0 + 32 * up * (S64)(nsmp - 1);
    S64 lo = o0 - 512 * up, hi = hs;
    // the LFE (64x only), its samples' windows and the correction's around them
    const S64 n0 = t0 / 2;
    lfe_n0 = n0;
    if (r.lff == 2 && !r.xpass && bind.lfe >= 0 && bind.lfe < wav->nch) {
      lfe_on = 1;
      lfe_n = 2 * r.lff * s.nssc;
      lo = imin(lo, 64 * up * (n0 - LFE_K) + bind.off - 544 * up);
      hi = imax(hi, 64 * up * (n0 + lfe_n - 1 + LFE_K) + bind.off + 32 * up);
    }
    wav->need(lo, hi);
    // (lfe_dec()'s, to its last value's, whether it is taken or not: the WAV read the same at both
    // ends whatever the rules (the binding's first pass reads on at the coding end))
    if (lfe_on && up == 1) wav->need(lo, 64 * (n0 + lfe_n + 7) + bind.off);
    if (!r.xpass && !sf) ffe_ok = ff_frame(r, s, 32 * (S64)t0 + bind.off);
    // (where the analysis first reaches into the WAV's last frame (of the core's frame's length), but
    // not past its end: a cut stream's WAV whose last frame a decoder made from that frame's own
    // subband samples alone, its synthesis without the frames before (FFmpeg's, where the lossless
    // extension ends before the core does), lacks what those frames' samples add there, and nothing
    // there says what that was; so the frames before take the continuation from that frame's start
    // instead (each channel but the LFE, whose decoding has no such gap) where it makes more of the
    // subframe's indexes, a bit, and code without the audio their samples whose windows lie past
    // that by more than VEND_PAST where the continuation makes fewer than half of those, a second
    // bit; the WAV's own again where the analysis reaches past its end. Since format 101: 3 MB of
    // `s.2.dts`'s middle 8.4% smaller, FATE's `Master Audio 5.0 96khz.dts` 5.0%, `Master Audio
    // 7.1.dts` 2.9%; with that frame as the decoder made it going on (a bound) 28%; the last frame
    // taking the continuation too, bigger)
    const S64 fr = 32 * (S64)up * (r.nblks + 1), ve = (S64)wav->nsamp - fr;
    if (!vend_set && hs > ve && hs <= (S64)wav->nsamp && ve >= fr && fr <= WAV_EXT) {
      vend_set = 1;
      wav->need(lo, (S64)wav->nsamp);
      if (wav->got == wav->nsamp) {
        vend_e = ve;
        int m = 0, hp = 0, np = 0;
        if (ENC) {
          const int j0 = (int)clip((ve - o0) / (32 * (S64)up) + 1, 0, nsmp);
          const int h0 = vend_hits(r, s, o0, j0);
          vend_swap(true);
          m = vend_hits(r, s, o0, j0, &hp, &np) > h0;
          if (!m) vend_swap(false);
        }
        if (c->code_bit(CO_ONE / 2, m) && !vend_on) vend_swap(true);
        if (vend_on) vend_past = c->code_bit(CO_ONE / 2, ENC && 2 * hp < np);
      }
    }
    if (vend_on && hs > (S64)wav->nsamp) vend_swap(false);
    // (past the WAV's end, its continuation: from where the subbands' analysis first reaches past
    // it, or 0s, whichever makes more of that subframe's indexes, a bit: the encoder's input went
    // on, or ended; the LFE's windows, reaching on further, have it before)
    if (ext_set < 2 && hi > (S64)wav->nsamp && wav->got == wav->nsamp) {
      if (!ext_set) ext_set = 1, wav_extend(*wav), wav->ext_on = 1;
      if (hs > (S64)wav->nsamp) {
        ext_set = 2;
        int m = 0, hp[2] = {0, 0}, np = 0;
        if (ENC) m = ext_choose(r, s, o0, hp, &np);
        wav->ext_on = c->code_bit(CO_ONE / 2, m);
        past_on = c->code_bit(CO_ONE / 2, ENC && 2 * hp[wav->ext_on] < np);
      }
    }
    S64 past[LFE_NP];  // (for the scale factor's prediction: the subframe's values as predicted)
    for (int k = 0; k < LFE_NP; k++) past[k] = lfe_past[k];
    for (int i = 0; lfe_on && i < lfe_n; i++) {
      for (int k = 0; k < LFE_NT; k++) lfe_f[i][k] = lfe_d(n0 + i + k - LFE_K);
      for (int k = 0; k < LFE_N2; k++) lfe_f3[i][k] = lfe_d2(2 * (n0 + i) + k - LFE_K2);
      lfe_pl[i] = (int)clip(n0 + i - tframe / 2, 0, LFE_PL - 1);
      for (int k = 0; k < LFE_NP; k++) lfe_f[i][LFE_NT + k] = past[k];
      lfe_p[i] = lfe_pred(i);
      for (int k = LFE_NP - 1; k > 0; k--) past[k] = past[k - 1];
      past[0] = lfe_p[i];
    }
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bind.bound(ch, wav->nch)) continue;
      bound[ch] = 1;
      S64 x[64];
      if (bind.alt[ch]) {
        gain_pick(ch);
        for (int j = 0; j < nsmp; j++) {
          analyze_bound(*wav, gsel[ch] ? bind : bind2, ch, o0 + 32 * up * j, fir, x, NSUB, !ss_rule() && !x96a);
          for (int b = 0; b < NSUB; b++) XO[ch][b][j] = x[b];
        }
      }
      for (int j = 0; j < nsmp; j++) {
        analyze_bound(*wav, gsel[ch] ? bind2 : bind, ch, o0 + 32 * up * j, fir, x, x96a ? NSUB96 : NSUB, !ss_rule() && !x96a);
        for (int b = 0; b < NSUB; b++) X[ch][b][j] = x[b];
        for (int c = 0; c < CPL_N; c++) {
          const int tb = NSUB - 1 - c;
          xcpl[c][ch][j] = x[tb];
          if (tb >= r.nvqsub[ch] && !x96a && !ss_rule() && up == 1) X[ch][tb][j] += rshift(x[c] * cpl_m(c, cpl_k(c)), 23);
        }
        for (int b = 0; x96a && b < NSUB96; b++) xa[ch][b][fb0 + j] = x[b];
      }
      const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
      const int nt = sf_entries(r.shuff[ch]);
      for (int b = 0; b < r.nsubs[ch]; b++) {  // (the LPC over the subframe and the 4 before it: hist4())
        S64 x[4 + NSMP];
        S64 h[4];
        hist4(ch, b, h);
        for (int k = 0; k < 4; k++) x[3 - k] = h[k];
        for (int j = 0; j < nsmp; j++) x[4 + j] = X[ch][b][j];
        lpc4(x, 4 + nsmp, lpc[ch][b], &lpg[ch][b], &lpg4[ch][b]);
      }
      for (int b = 0; b < NSUB; b++) {
        const S64* v = X[ch][b];
        S128 e = 0, cr = (S128)v[0] * xh[ch][b][0], ee[NSSF] = {0, 0, 0, 0};
        for (int j = 0; j < nsmp; j++) {
          e += (S128)v[j] * v[j];
          ee[j >> 3] += (S128)v[j] * v[j];
          if (j) cr += (S128)v[j] * v[j - 1];
        }
        ac[ch][b] = ratio_bucket(cr, e, 8);
        int near;
        es[ch][b] = 1 + scale_pred(v, nsmp, tab, nt, &near);
        eq[ch][b] = scale_quarter(v, nsmp, tab, nt, es[ch][b] - 1);
        // how the energy changes: the largest rise from a subsubframe to the next, in halvings
        // (0 none: one subsubframe; 1 none to 8 more than 2^7 times)
        int up = 0;
        for (int k = 1; k < s.nssc; k++) {
          const S128 a = ee[k - 1] + 1, z = ee[k];
          int h = 0;
          while (h < 7 && z > (a << (h + 1))) h++;
          if (z > a && h + 1 > up) up = h + 1;
        }
        ec[ch][b] = s.nssc > 1 ? 1 + up : 0;
      }
      esmax[ch] = 0;
      for (int b = 0; b < NSUB; b++) esmax[ch] = imax(esmax[ch], es[ch][b]);
      U128 tl = 0;
      for (int j = 0; j < nsmp; j++) {
        U128 t = 0;
        for (int b = 0; b < NSUB; b++) t += (U128)((S128)X[ch][b][j] * X[ch][b][j]);
        tlev[ch][j] = isqrt128(t);
        tl += (U128)((S128)tlev[ch][j] * tlev[ch][j]);
        if (!ss_rule() && up == 1) path_levels(ch, j);
      }
      tlvl[ch] = nsmp ? isqrt128(tl / (U128)nsmp) : 0;
      for (int b = 0; !ss_rule() && up == 1 && b < NSUB; b++) {
        U128 pl = 0;
        for (int j = 0; j < nsmp; j++) pl += (U128)((S128)plev[ch][b][j] * plev[ch][b][j]);
        plvl[ch][b] = nsmp ? isqrt128(pl / (U128)nsmp) : 0;
      }
    }
    dtwin_find(r);
  }
  // each bound channel's twin for the subframe, without one by its binding: the first bound channel
  // before it in the pass whose subband samples in it are all this one's, not all 0
  void dtwin_find(const CoreRec& r) {
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bound[ch] || twin[ch] >= 0) continue;
      for (int e = r.ch0(); e < ch && dtwin[ch] < 0; e++) {
        if (!bound[e]) continue;
        bool same = true, live = false;
        for (int b = 0; same && b < NSUB; b++) {
          same = !memcmp(X[e][b], X[ch][b], nsmp * sizeof X[ch][b][0]);
          for (int j = 0; same && !live && j < nsmp; j++) live = X[ch][b][j] != 0;
        }
        if (same && live) dtwin[ch] = e;
      }
    }
  }
  // (c) the subframe's indexes the analysis makes with the WAV's continuation past its end (on) or
  // with 0s there, of channel ch's samples j0..j1 - 1: those of its bound channels' subbands that
  // neither predict nor are VQ ones (hp: those of the samples whose windows lie past the WAV's end by
  // WAV_PAST, and of them, np)
  int ext_hits(const CoreRec& r, const CoreSub& s, S64 o0, int on, int ch, int j0, int j1, int* hp, int* np) {
    wav->ext_on = on;
    int h = 0;
    S64 x[64];
    for (int j = j0; j < j1; j++) {
      analyze_bound(*wav, gsel[ch] ? bind2 : bind, ch, o0 + 32 * bind.up * j, fir, x, x96a ? NSUB96 : NSUB, !ss_rule() && !x96a);
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        const int a = s.abits[ch][b];
        if (!a || s.pmode[ch][b]) continue;
        S64 qlo, qhi, alt;
        S128 dist;
        quant_range(r, ch, a, qlo, qhi);
        const int hit = requant(x[b], step_scale(r, s, ch, b, j >> 3), qlo, qhi, &dist, &alt) == s.q[ch][b][j];
        h += hit;
        if (past_far(t0 + j)) *hp += hit, ++*np;
      }
    }
    return h;
  }
  // (c) whether the continuation makes more of them than 0s do (ext_hits()), hp[on] and np theirs: of
  // the samples whose windows reach past the WAV's end (those that end in it are the same either way,
  // and none of them far past it); the continuation's channel by channel only while its count could
  // still go past the 0s' (or where it has, for hp[1])
  int ext_choose(const CoreRec& r, const CoreSub& s, S64 o0, int* hp, int* np) {
    const S64 st = 32 * (S64)bind.up;
    const int jin = (int)clip(((S64)wav->nsamp - o0) / st + 1, 0, nsmp);  // (the samples whose windows end in the WAV)
    int h0 = 0, room = 0, chs[NCH], nc = 0, rest[NCH];
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bind.bound(ch, wav->nch)) continue;
      int k = 0;
      for (int b = 0; b < r.nvqsub[ch]; b++) k += s.abits[ch][b] && !s.pmode[ch][b];
      chs[nc] = ch, rest[nc++] = k * (nsmp - jin), room += k * (nsmp - jin);
      h0 += ext_hits(r, s, o0, 0, ch, jin, nsmp, hp, np);
    }
    int h1 = 0, m = -1, n1 = 0;
    for (int i = 0; i < nc; i++) {
      h1 += ext_hits(r, s, o0, 1, chs[i], jin, nsmp, hp + 1, &n1);
      room -= rest[i];
      if (m < 0 && h1 > h0) m = 1;
      if (m < 0 && h1 + room <= h0) {
        m = 0;
        break;
      }
    }
    return m > 0;
  }
  // the subframe done: the audio's last subband samples, as the next one's history; the LFE's
  // correction fit again
  void smp_subframe_end(const CoreRec& r) {
    if (lfe_on) {
      lfit.refit(), lfit2.refit();
      if (!(++lfe_nsf & 15)) {
        lfit3.refit();
        if (lfe_poor())
          for (int k = 0; k < LFE_PL; k++)
            if (lfe_pn[k]) lfit4[k].refit(), lfe_pn[k] = 0;
        for (int k = 0; k < 10; k++) lfw[k] = 0;
        lfwn = 0;
      }
    }
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bound[ch]) continue;
      for (int b = 0; b < NSUB; b++)
        for (int k = 0; k < 4; k++) xh[ch][b][k] = X[ch][b][nsmp - 1 - k];
    }
  }

  // --- the bit allocations' staircases ---------------------------------------------------------------

  // An encoder's ABITS rise with a subband's energy, at thresholds its rate control moves from frame
  // to frame. A subband's staircase keeps, for each ABITS, the fine energies (fine_energy(), less the
  // level) it was last seen at, as intervals monotone in the energy, the newest winning; its guess
  // at an energy, the ABITS of the interval it lies in, or of the nearer of the two either side.
  // The pass's level: from the subframe's centre, the rate control's (level_centre()), the shift at
  // which the staircases made the most of the subframe's ABITS so far (within AL_ND; of those, the
  // nearest). The encoders seen fill their frames to within 16 bits, and the bits a subband's
  // samples take at an ABITS are the audio's (bud_bits(): its values quantized at the RMS rule's
  // scale factor, by the frame's codes, are the stream's to the bit): so the core's centre is where
  // the staircases' ABITS would take the bits the frame has for them (its bits from the subframe's
  // first ABITS on, less what those of the last subframes' ABITS left of theirs); until a subframe
  // has been seen, and in the extension's pass, where they would cost (ab_bits()) what the last
  // subframes' did. The subframe's ABITS all known, the staircases have them at its last level.
  // the fine energy of the n values v: 128 times the log2 of their mean square (FE_NONE: all 0)
  static S32 fine_energy(const S64* v, int n) {
    U128 e = 0;
    for (int j = 0; j < n; j++) e += (U128)((S128)v[j] * v[j]);
    return e ? fine_log2(e) - fine_log2((U128)n) : FE_NONE;
  }
  // the same in 2^-AP_FB of a log2 (AP_FB 9 before format 95: quarters of fine_energy()'s units)
  static constexpr int AP_FB = 18;
  static constexpr S64 AP_XU = (S64)1 << (AP_FB - 7), AP_XQ = (S64)1 << (AP_FB - 9);  // (a 128th, a quarter of one, so)
  static S32 fine_energy_x(const S64* v, int n) {
    U128 e = 0;
    for (int j = 0; j < n; j++) e += (U128)((S128)v[j] * v[j]);
    return e ? (S32)(log2_bits(e, AP_FB) - log2_bits((U128)n, AP_FB)) : FE_NONE;
  }
  S32 fe_sx[NCH][NSUB];  // (fe_s's energies so: fine_energy_x()'s)
  // the staircase t's ABITS at the energy x (-1 none)
  static int stair_guess(const AbStair& t, S32 x) {
    int kb = -1;
    for (U32 m = t.has; m; m &= m - 1) {
      const int k = __builtin_ctz(m);
      if (x < t.lo[k]) return kb < 0 || t.lo[k] - x < x - t.hi[kb] ? k : kb;
      if (x <= t.hi[k]) return k;
      kb = k;
    }
    return kb;
  }
  // the energies at which it is v: a to z (false: v isn't on it)
  static bool stair_region(const AbStair& t, int v, S64* a, S64* z) {
    if (!(t.has >> v & 1)) return false;
    const U32 below = t.has & ((1u << v) - 1), above = t.has & ~((2u << v) - 1);
    *a = -((S64)1 << 40), *z = (S64)1 << 40;
    if (below) {
      const S64 h = (S64)t.hi[31 - __builtin_clz(below)] + t.lo[v];
      *a = (h - (h & 1)) / 2 + 1;
    }
    if (above) {
      const S64 h = (S64)t.hi[v] + t.lo[__builtin_ctz(above)];
      *z = (h - (h & 1)) / 2;
    }
    return true;
  }
  // it is v at x: the intervals either side give way
  static void stair_add(AbStair& t, S32 x, int v) {
    const U32 bit = 1u << v;
    if (t.has & bit) t.lo[v] = imin(t.lo[v], x), t.hi[v] = imax(t.hi[v], x);
    else t.has |= bit, t.lo[v] = t.hi[v] = x;
    for (U32 m = t.has & ~(bit | (bit - 1)); m; m &= m - 1) {
      const int k = __builtin_ctz(m);
      if (t.hi[k] <= x) t.has &= ~(1u << k);
      else if (t.lo[k] <= x) t.lo[k] = x + 1;
    }
    for (U32 m = t.has & (bit - 1); m; m &= m - 1) {
      const int k = __builtin_ctz(m);
      if (t.lo[k] >= x) t.has &= ~(1u << k);
      else if (t.hi[k] >= x) t.hi[k] = x - 1;
    }
  }
  // a subframe of pass p starts: the last one's level, its cost into the target
  void level_start(int p) {
    AbLevel& L = abl[p];
    L.c0 = clip(L.c0 + L.d, -AL_C, AL_C), L.d = 0;
    memset(L.sc, 0, sizeof L.sc);
    if (L.n) L.tgt = L.tgt ? (L.tgt + L.cost) >> 1 : L.cost;
    L.cost = 0, L.n = 0, L.set = 0;
  }
  // the bits a sample at each ABITS takes, in 64ths (log2 of its levels)
  static int ab_bits(int a) {
    static const S16 t[27] = {0,   101, 149, 180, 203, 237, 262,  297,  320,  384,  448,  512,  576, 640,
                              704, 768, 832, 896, 960, 1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472};
    return a >= 0 && a < 27 ? t[a] : 0;
  }
  // the subframe's first ABITS (all its PMODE and PVQ known): its subbands' fine energies, and the
  // centre as the encoder's rate control would make it: the level at which the staircases' ABITS
  // cost what the last subframes' did (the nearer of the two either side; within AB_RC); the
  // core's, where their samples would take the frame's bits (above), and the same of the
  // allocation table's ABITS (apl's c0t), each within AB_RC of the last or, where only further,
  // within AB_RB: since format 124, as a DTS:X encoder's level rises 8-23 doublings from a
  // subframe to the next at a loud onset (37 times in the Cymatics stream, whose bit allocations
  // came to 14.1 KB from 16.0; within 64 doublings, `Master Audio 7.1.dts`'s core 1.4% smaller,
  // but two of FATE's 7-frame XXCH samples' 1.3% and 1.7% bigger; anywhere, more of them bigger)
  // (at the subframe's first ABITS, at bit at of the frame)
  void level_centre(const CoreRec& r, const CoreSub& s, int p, U64 at) {
    AbLevel& L = abl[p];
    L.set = 1;
    if (!p) bg_used = 0, memset(bg_done, 0, sizeof bg_done);
    L.nobs = 0;
    memcpy(ab_snap[p], ab_st[p], sizeof ab_snap[p]);
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        fe_s[ch][b] = fe_a[ch][b] = fe_sx[ch][b] = FE_NONE;
        if (!bound[ch]) continue;
        S64* const x = bud_v[ch][b];
        const int n = scale_values(s, ch, b, 0, s.nssc, x);
        int near;
        fe_s[ch][b] = fine_energy(x, n);
        // (a transient's: the encoders' energy is the part from it on, where its second scale factor
        // is: ab_transient())
        int tk_ = 0;
        fe_s[ch][b] = ab_transient(s, ch, b, x, n, fe_s[ch][b], &fe_a[ch][b], &tk_);
        fe_sx[ch][b] = fe_s[ch][b] == FE_NONE   ? FE_NONE
                       : fe_a[ch][b] != FE_NONE ? fine_energy_x(x + 8 * tk_, n - 8 * tk_)
                                                : fine_energy_x(x, n);
        bud_n[ch][b] = n;
        bud_si[ch][b] = scale_pred(x, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
        // (a transient's: the bits of its second scale factor, where the code book codes differences
        // the RMS rule's for its values from the transient on less the one for those before, and of
        // its TMODE's code above a 0's)
        bud_xs[ch][b] = 0;
        if (fe_a[ch][b] != FE_NONE) {
          const int sh = r.shuff[ch];
          int xb = sh + 1;
          if (sh < 5) {
            const int ne = sf_entries(sh);
            xb = SB[sh].bits(scale_pred(x + 8 * tk_, n - 8 * tk_, SF6, ne, &near) - scale_pred(x, 8 * tk_, SF6, ne, &near));
          }
          if (s.nssc > 1) xb += TB[r.thuff[ch]].bits(tk_) - TB[r.thuff[ch]].bits(0);
          bud_xs[ch][b] = 4 * xb;
        }
        memset(bud_b[ch][b], 0xFF, sizeof bud_b[ch][b]);
      }
    L.avail = 4 * ((S64)r.fsize * 8 - (S64)at);
    if (!p && L.rn) {  // (the samples' bits fall as the level rises: the nearer either side)
      const S64 T = L.avail + L.rb;
      S32 lo = L.c0 - AB_RC, hi = L.c0 + AB_RC;
      S64 tl = bud_total(r, s, lo), th = bud_total(r, s, hi);
      if (tl <= T) {  // (lower: to AB_RB, where the bits take the frame's there; else the edge)
        const S32 l2 = L.c0 - AB_RB;
        const S64 t2 = bud_total(r, s, l2);
        if (t2 > T) hi = lo, th = tl, lo = l2, tl = t2;
        else hi = lo, th = tl;
      } else if (th > T) {
        const S32 h2 = L.c0 + AB_RB;
        const S64 t2 = bud_total(r, s, h2);
        if (t2 <= T) lo = hi, tl = th, hi = h2, th = t2;
        else lo = hi, tl = th;
      }
      while (hi - lo > 1) {
        const S32 m = lo + (hi - lo) / 2;
        const S64 tm = bud_total(r, s, m);
        if (tm > T) lo = m, tl = tm;
        else hi = m, th = tm;
      }
      L.c0 = clip(tl - T < T - th ? lo : hi, -AL_C, AL_C);
      // (and the table's: where its ABITS take those bits)
      AbParLevel& A = apl[0];
      const S32 ct = A.cset ? A.c0t : L.c0;
      lo = ct - AB_RC, hi = ct + AB_RC;
      tl = bud_total_tab(r, lo), th = bud_total_tab(r, hi);
      if (tl <= T) {
        const S32 l2 = ct - AB_RB;
        const S64 t2 = bud_total_tab(r, l2);
        if (t2 > T) hi = lo, th = tl, lo = l2, tl = t2;
        else hi = lo, th = tl;
      } else if (th > T) {
        const S32 h2 = ct + AB_RB;
        const S64 t2 = bud_total_tab(r, h2);
        if (t2 <= T) lo = hi, tl = th, hi = h2, th = t2;
        else lo = hi, tl = th;
      }
      while (hi - lo > 1) {
        const S32 m = lo + (hi - lo) / 2;
        const S64 tm = bud_total_tab(r, m);
        if (tm > T) lo = m, tl = tm;
        else hi = m, th = tm;
      }
      A.c0t = clip(tl - T < T - th ? lo : hi, -AL_C, AL_C), A.cset = 1;
      return;
    }
    if (L.tgt <= 0) return;
    auto cost = [&](S32 l) {
      S64 c = 0;
      for (int ch = r.ch0(); ch < r.ch1(); ch++)
        for (int b = 0; b < r.nvqsub[ch]; b++)
          if (fe_s[ch][b] != FE_NONE) c += ab_bits(stair_guess(ab_st[p][b], fe_s[ch][b] - l));
      return c;
    };
    S32 lo = L.c0 - AB_RC, hi = L.c0 + AB_RC;  // (the cost falls as the level rises)
    S64 clo = cost(lo), chi = cost(hi);
    if (clo <= L.tgt) hi = lo, chi = clo;
    else if (chi > L.tgt) lo = hi, clo = chi;
    while (hi - lo > 1) {
      const S32 m = lo + (hi - lo) / 2;
      const S64 cm = cost(m);
      if (cm > L.tgt) lo = m, clo = cm;
      else hi = m, chi = cm;
    }
    L.c0 = clip(clo - L.tgt < L.tgt - chi ? lo : hi, -AL_C, AL_C);
  }
  // The bits (in quarters) the core's subframe's samples may still take as channel ch's subband b's
  // ABITS comes, less what the guesses for it and the subbands after it would take at the level as
  // it is (the table's where they are the first ones, else the staircases'; the encoders fill their
  // frames: abt's bg): what they take from the subframe's first ABITS on (abl's avail + rb), less
  // the ABITS coded's, less those guesses'
  S64 ab_budget(const CoreRec& r) {
    const AbLevel& L = abl[0];
    S64 rest = 0;
    for (int c = r.ch0(); c < r.ch1(); c++)
      if (bound[c])
        for (int k = 0; k < r.nvqsub[c]; k++) {
          if (bg_done[c][k]) continue;
          int g = -1, m;
          if (fe_s[c][k] != FE_NONE && ap_rule(0)) g = ap_guess(0, k, fe_s[c][k], &m);
          if (g < 0) g = fe_s[c][k] == FE_NONE ? -1 : level_guess(0, ab_st[0][k], fe_s[c][k]);
          if (g < 0) g = hist.abits[c][k] ? hist.abits[c][k] - 1 : 0;
          rest += bud_bits(r, c, k, g);
        }
    return L.avail + L.rb - bg_used - rest;
  }
  // and channel ch's subband b's ABITS is v
  void ab_budget_add(const CoreRec& r, int ch, int b, int v) {
    if (bg_done[ch][b]) return;
    bg_done[ch][b] = 1;
    bg_used += bud_bits(r, ch, b, v);
  }
  // the bits (in quarters) channel ch's subband b's samples would take at ABITS a: its values
  // quantized at the RMS rule's scale factor (as requant() does), by the code the frame's header
  // gives the ABITS; and where the encoders take a transient (ab_transient()), the side
  // information's that it adds, a second scale factor and the TMODE's code (since format 125:
  // without them the budget's level lay low where a subframe has more transients than the last
  // ones, 31 128ths of a doubling with 20-49 of them, 69 with 50 or more, in the Cymatics stream;
  // its values quantized at each part's scale factor besides, the same)
  int bud_bits(const CoreRec& r, int ch, int b, int a) {
    if (a <= 0 || a > 26) return 0;
    S16& t = bud_b[ch][b][a];
    if (t >= 0) return t;
    const int how = sample_coding(r, ch, a), n = bud_n[ch][b];
    int sum = bud_xs[ch][b];
    if (how == 2) {
      sum += 4 * (a - 3) * n;
    } else if (how == 3) {
      sum += BLOCK_BITS[a - 1] * n;
    } else {
      const HuffBook& hb = QB[a - 1][r.sel[ch][a - 1]];
      const S64 ss = step_for(r, ch, a, bud_si[ch][b]), lo = hb.minlev, hi = hb.minlev + hb.nlev - 1;
      for (int j = 0; j < n; j++) {
        const S64 v = bud_v[ch][b][j];  // (|v| < 2^41)
        S64 q = ss > 0 ? ((v < 0 ? -v : v) * 128 + ss) / (2 * ss) : 0;
        if (v < 0) q = -q;
        sum += 4 * hb.elen[(q < lo ? lo : q > hi ? hi : q) - lo];
      }
    }
    return t = (S16)sum;
  }
  // the bits the core's bound subbands' samples would take at level l (a subband the staircase says
  // nothing of, at its last ABITS)
  S64 bud_total(const CoreRec& r, const CoreSub& s, S32 l) {
    S64 t = 0;
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      if (bound[ch])
        for (int b = 0; b < r.nvqsub[ch]; b++) {
          int g = fe_s[ch][b] == FE_NONE ? -1 : stair_guess(ab_st[0][b], fe_s[ch][b] - l);
          if (g < 0) g = hist.abits[ch][b] ? hist.abits[ch][b] - 1 : 0;
          t += bud_bits(r, ch, b, g);
        }
    return t;
  }
  // A lossless WAV's subband with a transient (where a subsubframe's mean square is 8 times the one
  // before's or more, the largest such rise's; not an ADPCM subband's: tmode_rule()):
  // the encoders seen allocate its bits by the mean square of its values from the transient on,
  // where they take one, the rise rise_thr() or more, else by the subframe's: the fine energy the
  // ABITS are guessed by (fe the subframe's; *fa those values' where they take one, else FE_NONE).
  // As the threshold is theirs, the other of the two mean squares is no second guess (`s.2.dts`'s
  // core 0.15% smaller without it, the other lossless WAVs' up to 0.45%), nor is a subband that
  // doesn't predict where its gain says it would a transient by that alone.
  S32 ab_transient(const CoreSub& s, int ch, int b, const S64* x, int n, S32 fe, S32* fa_, int* tko = nullptr) const {
    *fa_ = FE_NONE;
    if (s.pmode[ch][b] || s.nssc < 2 || fe == FE_NONE) return fe;
    int tk = 0;
    S32 tr = 0;
    for (int k = 1; k < s.nssc; k++) {
      const S32 e0 = fine_energy(x + 8 * (k - 1), 8), e1 = fine_energy(x + 8 * k, 8);
      if (e1 == FE_NONE) continue;
      const S32 rr = e0 == FE_NONE ? (S32)1 << 20 : e1 - e0;
      if (rr > tr) tr = rr, tk = k;
    }
    if (!tk || tr < 3 * 128) return fe;
    int k;
    if (rise_max(s, ch, b, &k) < rise_thr(b)) return fe;
    if (tko) *tko = tk;
    return *fa_ = fine_energy(x + 8 * tk, n - 8 * tk);
  }
  // the bits the core's bound subbands' samples would take at the table's ABITS at level l (where it
  // has none, the staircases', else the last ABITS)
  S64 bud_total_tab(const CoreRec& r, S32 l) {
    S64 t = 0;
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      if (bound[ch])
        for (int b = 0; b < r.nvqsub[ch]; b++) {
          int g = -1;
          if (fe_s[ch][b] != FE_NONE) {
            bool ok;
            const S32 o = ap_off(0, b, &ok);
            g = ok ? ap_abits((S64)fe_s[ch][b] - l - o) : abp[0][b].one ? 1 : stair_guess(ab_st[0][b], fe_s[ch][b] - l);
          }
          if (g < 0) g = hist.abits[ch][b] ? hist.abits[ch][b] - 1 : 0;
          t += bud_bits(r, ch, b, g);
        }
    return t;
  }
  // pass p's subframe's ABITS all known: the staircases as they were at its first, each of its ABITS
  // then added at its last level; and (the core's) what its samples' bits came to past the frame's
  // bits from its first ABITS on
  void level_commit(const CoreRec& r, const CoreSub& s, int p) {
    AbLevel& L = abl[p];
    if (!L.set) return;
    ap_commit(p);
    memcpy(ab_st[p], ab_snap[p], sizeof ab_snap[p]);
    for (int i = 0; i < L.nobs; i++) stair_add(ab_st[p][L.obs[i].b], L.obs[i].x - L.c0 - L.d, L.obs[i].v);
    if (p) return;
    S64 e = 0;
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      if (bound[ch])
        for (int b = 0; b < r.nvqsub[ch]; b++) e += bud_bits(r, ch, b, s.abits[ch][b]);
    L.rb = L.rn ? L.rb + (e - L.avail - L.rb) / 2 : e - L.avail;
    L.rn += L.rn < 1000;
  }
  // pass p's guess on the staircase t at the energy x
  int level_guess(int p, const AbStair& t, S32 x) const { return stair_guess(t, x - abl[p].c0 - abl[p].d); }
  // and how sure it is, its guess g: how near x lies to the energies where the guess changes, the
  // nearer's side (0 none; 1-8 below, 9-16 above: within 0, 1, 3, 7, 15, 31, 63 128ths of a
  // doubling or farther)
  int level_margin(int p, const AbStair& t, S32 x, int g) const {
    S64 a, z;
    if (g < 0 || !stair_region(t, g, &a, &z)) return 0;
    const S64 y = (S64)x - abl[p].c0 - abl[p].d, lo = y - a, hi = z - y, m = lo < hi ? lo : hi;
    int k = 0;
    while (k < 7 && m >= ((S64)1 << k)) k++;
    return 1 + 8 * (lo >= hi) + k;
  }
  // and its ABITS was v: the shifts at which the staircase would have said it count, then the
  // staircase has it
  void level_add(int p, AbStair& t, S32 x, int v) {
    AbLevel& L = abl[p];
    S64 a, z;
    // (the shift the most say, a 16th of an ABITS's score off for each unit from the centre, of
    // equals the nearer the centre, then the lower: as a scan from the centre out finds it. Only
    // the shifts k0..k1 gain, by as much each: where the best is among them, it stays; else the
    // best of them, where better)
    int bk = L.d + AL_ND;
    if (stair_region(t, v, &a, &z)) {  // (x - c0 - (k - AL_ND) in a..z)
      const S64 k0 = clip((S64)x - L.c0 - z + AL_ND, 0, AL_NK), k1 = clip((S64)x - L.c0 - a + AL_ND, -1, AL_NK - 1);
      for (S64 k = k0; k <= k1; k++) L.sc[k]++;
      if (k0 <= k1 && (bk < k0 || bk > k1)) {
        auto score = [&](int k) { return 16 * L.sc[k] - (k > AL_ND ? k - AL_ND : AL_ND - k); };
        auto rank = [](int k) { return k > AL_ND ? 2 * (k - AL_ND) : k < AL_ND ? 2 * (AL_ND - k) - 1 : 0; };
        S32 bs = score(bk);
        for (int k = (int)k0; k <= (int)k1; k++) {
          const S32 s = score(k);
          if (s > bs || (s == bs && rank(k) < rank(bk))) bs = s, bk = k;
        }
      }
    }
    stair_add(t, x - L.c0 - L.d, v);
    L.obs[L.nobs].x = x, L.obs[L.nobs].b = (U8)(&t - ab_st[p]), L.obs[L.nobs].v = (U8)v, L.nobs++;
    L.cost += ab_bits(v), L.n++;
    L.d = bk - AL_ND;
  }

  // --- the lossless WAVs' encoders' allocation table ----------------------------------------------

  // The DTS-HD MA encoders seen allocate by one table: a subband's ABITS is a (2 to 26; else 1)
  // where its fine energy, less the level and its band's offset, is ap_edge(a) or more (in 1.5 dB
  // steps: 64 units), the next's less; the bands' offsets are one offset for the stream and a shape
  // (ap_shape()): bands 5-19 the same, 20-27 2 dB more each, and the bottom five the same or, the
  // shape of `s.2.dts`'s encoder and FFmpeg's `master_audio_7.1_24bit.dts`'s, 18, 15, 12, 7.5 and
  // 4.5 dB lower (the offsets each band's own ABITS make, at the end of a stream, lie within 2 units
  // of the shape's: within 10 of it in `Master Audio 7.1.dts`'s, a downmix's), or, a DTS:X
  // encoder's (the Cymatics stream's; format 124), 18, 15, 12, 9 and 6 dB lower: with a level for
  // each subframe fitted afterwards, it makes all but 0.10% of that stream's ABITS (the first
  // shape is 1.5 dB off in its bands 3 and 4, so that the staircases' guesses were taken). Fitted afterwards, a
  // level per subframe, the staircases miss 0.6% of a lossless WAV's ABITS where their intervals as
  // they come miss 1.6%. So the stream's offset is the one most of the last ABITS of all the bands
  // (the newer the more) say at their subframes' levels, for each shape, the shape the one most of
  // them say (app: from a stream's first frames, where a band of its own has few; and far more
  // exactly, as every band's ABITS count, than each band's offset by its own ABITS, which the table
  // goes by until a stream's first ABITS above 1: `s.2.dts`'s core 2.2% smaller, the other lossless
  // WAVs' 2-6%); a subframe's level, the core's from the budget as the staircases' centre is
  // (level_centre(), at the table's ABITS: c0t), the extension's that centre, plus how far the last
  // subframes' levels lay from it, then the shift at which the table makes the most of the
  // subframe's ABITS so far (as the staircases' level). Its guesses are the first ones but where
  // they have missed far more than the staircases' (ap_rule()). That level is a whole 128th, the
  // encoders' lies between them: with the energies exact, a level for each subframe makes all but
  // 0.033% of `s.2.dts`'s ABITS, the nearest 128th 0.21% wrong. So how the table's guess stands
  // against the levels at which it makes all of the subframe's ABITS so far, from the energies in
  // quarters of a 128th (apl's ilo, ihi: ap_range()), is abt's context sx (ap_strad()): where
  // every one of them makes the guess it misses 0.025% of the time, where some don't 5-24%.
  static constexpr int AP_W = 1024;  // (a band's offsets: within AP_W of its first)
  static constexpr int AP_SH = 96;   // (the level: within AP_SH of the prior; 64 before format 78)
  static constexpr S64 AP_FAR = (S64)1 << 40;
  struct AbPar {
    S32 c, o;              // the scores' centre (the band's first ABITS above 1's), its offset
    U8 set, one, out;      // (whether c is set; ABITS 1 seen, with c unset; ABITS in a row the scores miss)
    U32 sc[2 * AP_W + 1];  // each offset's score (ap_inc's)
  } abp[2][NSUB];
  // band b's offset in shape k, less the stream's
  static constexpr int AP_NK = 3;  // (the shapes: `s.2.dts`'s encoder's, the flat one, the DTS:X encoder's)
  static S32 ap_shape(int k, int b) {
    static const S16 LOW[2][5] = {{-768, -640, -512, -320, -192}, {-768, -640, -512, -384, -256}};
    return (b < 5 && k != 1 ? LOW[k >> 1][b] : 0) + (b > 19 ? (S32)((256 * (b - 19) + 1) / 3) : 0);
  }
  struct AbPool {
    S32 c, best[AP_NK];      // the scores' centre (the stream's first ABITS above 1's); each shape's offset
    U8 set, k, out;          // (whether c is set; the shape taken; ABITS in a row the scores miss)
    U32 sc[AP_NK][2 * AP_W + 1];  // each shape's offsets' scores (ap_inc's)
  } app[2];
  // band b's offset (*ok: whether it has one): the stream's in its shape, or where none is set yet,
  // the band's own
  S32 ap_off(int p, int b, bool* ok) const {
    const AbPool& q = app[p];
    *ok = true;
    if (q.set) return q.best[q.k] + ap_shape(q.k, b);
    *ok = abp[p][b].set;
    return abp[p][b].o;
  }
  struct AbParLevel {
    U32 inc;          // (the scores' step: it grows, the older ones count less)
    S32 pri, d;       // the subframe's prior level, the shift from it
    S64 drift;        // the last subframes' levels less their centres (2^-10 units, an average)
    int dset;         // (whether drift is)
    S32 sc[AL_NK];    // how many of the subframe's ABITS each shift (less AL_ND) made the table's
    int anch;         // whether the centre is the budget's (the core's) or the cost's (the extension's)
    S32 c0t, cb;      // (the core's) the centre the budget makes of the table's ABITS; the one the prior is from
    int cset;         // (whether c0t is)
    U32 miss[2];      // the staircases' first guesses wrong, the table's (where both have one)
    S64 ilo, ihi;     // (the levels, in fe_sx's units, at which the table makes all of the subframe's ABITS so far: ilo < l <= ihi)
  } apl[2];
  U64 n_tb, n_tbhit;  // (-s: the ABITS the table said, those it said right)
  // ABITS a's lower edge (a 2-27: 27 none)
  static S64 ap_edge(int a) {
    static const S16 K[11] = {0, 0, 0, 3, 5, 6, 8, 10, 12, 13, 17};
    return a > 26 ? AP_FAR : a < 2 ? -AP_FAR : 64 * (S64)(a <= 10 ? K[a] : 17 + 4 * (a - 10));
  }
  // the table's ABITS at y
  static int ap_abits(S64 y) {
    if (y < 0) return 1;
    if (y >= ap_edge(10)) return (int)imin(10 + (int)((y - ap_edge(10)) >> 8), 26);
    int a = 2;
    while (y >= ap_edge(a + 1)) a++;
    return a;
  }
  // the subframe of pass p starts, its centre known
  void ap_start(int p) {
    AbParLevel& A = apl[p];
    A.cb = !p && A.cset && abl[p].rn ? A.c0t : abl[p].c0;
    A.pri = (S32)clip((S64)A.cb + (A.drift >= 0 ? (A.drift + 512) >> 10 : -((-A.drift + 512) >> 10)), -AL_C, AL_C);
    A.d = 0;
    A.anch = p ? abl[p].tgt > 0 : abl[p].rn > 0;
    memset(A.sc, 0, sizeof A.sc);
    A.ilo = -AP_INF, A.ihi = AP_INF;
  }
  static constexpr S64 AP_INF = (S64)1 << 50;
  static S64 imax64(S64 a, S64 b) { return a > b ? a : b; }
  static S64 imin64(S64 a, S64 b) { return a < b ? a : b; }
  // band b's offset in quarters (ap_off()'s, its shape's slope exact)
  S64 ap_off4(int p, int b, bool* ok) const {
    const AbPool& q = app[p];
    *ok = true;
    if (q.set)
      return 4 * (S64)q.best[q.k] + (b < 5 ? 4 * ap_shape(q.k, b) : 0) + (b > 19 ? (1024 * (S64)(b - 19) + 1) / 3 : 0);
    *ok = abp[p][b].set;
    return 4 * (S64)abp[p][b].o;
  }
  // The share of the levels lo..hi of those the subframe's ABITS so far allow (apl's ilo..ihi): of
  // a lossless WAV's, under a normal prior about the subframe's level, the table's (pri + d), at a
  // deviation of AP_SD 128ths, 1/AP_UM of it the share's as it is; of a decoded WAV's (whose first
  // guess is the search's) the share as it is. The subframe's level lies within about half a
  // 128th of that one half the time (3 MB of `s.2.dts`'s middle: the share's first decision 82
  // bytes where the range is straddled, 524 as it was), but the margin (abt's sm) already has much
  // of it: since format 100 `s.2.dts`'s core 0.09% smaller, the lossless WAVs' short streams 0.05%;
  // at 0.35, 0.5 or 0.7 of a 128th, 0.08%, 0.05% and 0.03%; at an eighth, 0.06%; a decoded WAV's
  // too, its streams 0.04-0.8% bigger
  static constexpr double AP_SD = 0.25;
  static constexpr int AP_UM = 100;
  // (the normal's mass between a and b, about mu at deviation s; det_q()'s tails)
  static double ap_mass(double a, double b, double mu, double s) {
    const double za = (a - mu) / s, zb = (b - mu) / s;
    return za >= 0 ? det_q(za) - det_q(zb) : zb <= 0 ? det_q(-zb) - det_q(-za) : 1.0 - det_q(-za) - det_q(zb);
  }
  double ap_pshare(int p, S64 lo, S64 hi) const {
    const AbParLevel& A = apl[p];
    const S64 w = A.ihi - A.ilo, a = imax64(A.ilo, lo), z = imin64(A.ihi, hi);
    const double u = z > a ? (double)(z - a) / (double)w : 0.0;
    if (ss_rule()) return u;
    const double mu = (double)((S64)(A.pri + A.d) * AP_XU), s = AP_SD * (double)AP_XU;
    const double t = ap_mass((double)A.ilo, (double)A.ihi, mu, s);
    if (!(t > 1e-9)) return u;
    const double m = z > a ? ap_mass((double)a, (double)z, mu, s) : 0.0;
    return (1.0 - 1.0 / AP_UM) * (m / t) + u / AP_UM;
  }
  // how the table's guess g for channel ch's band b stands against the levels at which it makes the
  // subframe's ABITS so far: 0 they don't bound it, 1 all of them say g; else by its share of them
  // (ap_pshare()'s since format 100), 2 over nine tenths, 3 over half, 4 over a tenth, 5 less
  // (since format 95 the levels and the energies in 2^-18 of a log2, fe_sx's, rather than in
  // quarters of fine_energy()'s 128ths: the table's misses are 71% of `s.2.dts`'s ABITS' bytes, 82%
  // of them within a 128th of an edge, and there the energy's quarters still put a third of them on
  // the wrong side; the levels the ABITS so far allow, so measured, are narrower: `s.2.dts`'s core
  // 0.29% smaller, the short streams 8 bytes (in 32nds 0.24%, in 256ths 0.28%). The guess itself by
  // the quarters, 0.04%, or by these 0.03%, with the level clamped into the range the short streams
  // 0.2% smaller but `s.2.dts`'s core 0.05% bigger; the range's middle as the level, far bigger)
  // (*sh: the share of those levels at which the table says g, in 4096ths, -1 where they're
  // unbounded: abits_code()'s outside prediction since format 97)
  int ap_strad(int p, int ch, int b, int g, int* sh = nullptr) const {
    const AbParLevel& A = apl[p];
    bool ok;
    const S64 o = ap_off4(p, b, &ok);
    if (sh) *sh = -1;
    if (!ok || fe_sx[ch][b] == FE_NONE || g < 1 || g > 26) return 0;
    const S64 y = fe_sx[ch][b] - o * AP_XQ, glo = y - ap_edge(g + 1) * AP_XU, ghi = y - ap_edge(g) * AP_XU;
    if (glo <= A.ilo && A.ihi <= ghi) {
      if (sh) *sh = 4096;
      return 1;
    }
    if (A.ilo <= -AP_INF || A.ihi >= AP_INF) return 0;
    const double ps = ap_pshare(p, glo, ghi);
    if (sh) *sh = (int)(ps * 4096.0 + 0.5);
    return ps > 0.9 ? 2 : ps > 0.5 ? 3 : ps > 0.1 ? 4 : 5;
  }
  // (the same share for ABITS a)
  int ap_share(int p, int ch, int b, int a) const {
    const AbParLevel& A = apl[p];
    bool ok;
    const S64 o = ap_off4(p, b, &ok);
    if (!ok || fe_sx[ch][b] == FE_NONE || a < 1 || a > 26 || A.ilo <= -AP_INF || A.ihi >= AP_INF) return -1;
    const S64 y = fe_sx[ch][b] - o * AP_XQ, alo = y - ap_edge(a + 1) * AP_XU, ahi = y - ap_edge(a) * AP_XU;
    return (int)(ap_pshare(p, alo, ahi) * 4096.0 + 0.5);
  }
  // and its ABITS was v
  void ap_range(int p, int ch, int b, int v) {
    AbParLevel& A = apl[p];
    bool ok;
    const S64 o = ap_off4(p, b, &ok);
    if (!ok || fe_sx[ch][b] == FE_NONE || v < 1 || v > 26) return;
    const S64 y = fe_sx[ch][b] - o * AP_XQ, lo = v < 26 ? y - ap_edge(v + 1) * AP_XU : -AP_INF, hi = v > 1 ? y - ap_edge(v) * AP_XU : AP_INF;
    A.ilo = imax64(A.ilo, lo), A.ihi = imin64(A.ihi, hi);
    if (A.ilo >= A.ihi) A.ilo = lo, A.ihi = hi;  // (none: from this one on)
  }
  // the table's ABITS for band b at the fine energy x (-1 none), and how sure it is (*m, as
  // level_margin()'s)
  int ap_guess(int p, int b, S32 x, int* m) const {
    *m = 0;
    bool ok;
    const S32 o = ap_off(p, b, &ok);
    if (!ok) return abp[p][b].one ? 1 : -1;
    const S64 y = (S64)x - apl[p].pri - apl[p].d - o;
    const int g = ap_abits(y);
    const S64 lo = y - ap_edge(g), hi = ap_edge(g + 1) - 1 - y, mm = lo < hi ? lo : hi;
    int k = 0;
    while (k < 7 && mm >= ((S64)1 << k)) k++;
    *m = 1 + 8 * (lo >= hi) + k;
    return g;
  }
  // and its ABITS was v: the shifts at which the table would have said it count
  void ap_add(int p, int b, S32 x, int v) {
    AbParLevel& A = apl[p];
    bool ok;
    const S32 o = ap_off(p, b, &ok);
    if (!ok || v < 1 || v > 26) return;
    // (edge(v) <= x - pri - s - o < edge(v + 1): x - pri - o - edge(v + 1) < s <= x - pri - o - edge(v))
    const S64 y = (S64)x - A.pri - o;
    const S64 k0 = clip(y - ap_edge(v + 1) + 1 + AL_ND, AL_ND - AP_SH, AL_ND + AP_SH + 1),
              k1 = clip(y - ap_edge(v) + AL_ND, AL_ND - AP_SH - 1, AL_ND + AP_SH);
    // (as the staircases': a 16th of an ABITS's score off for each unit from the prior; the best,
    // the last one or one of those this ABITS raised; of equal ones the nearer the prior, then the
    // lower)
    auto score = [&](S64 k) { return 16 * (S64)A.sc[k] - (k > AL_ND ? k - AL_ND : AL_ND - k); };
    auto better = [&](S64 k, S64 j) {
      const S64 a = score(k), z = score(j);
      const S64 dk = k > AL_ND ? k - AL_ND : AL_ND - k, dj = j > AL_ND ? j - AL_ND : AL_ND - j;
      return a > z || (a == z && (dk < dj || (dk == dj && k < j)));
    };
    S64 bk = A.d + AL_ND;
    for (S64 k = k0; k <= k1; k++) {
      A.sc[k]++;
      if (k != bk && better(k, bk)) bk = k;
    }
    A.d = (S32)(bk - AL_ND);
  }
  // sc[i0..i1] each raised by inc: the most of them (8 at a time where AVX2 is)
  static U32 ap_raise(U32* sc, int i0, int i1, U32 inc) {
    U32 m = 0;
    int k = i0;
#ifdef DTSD_AVX2
    const __m256i vi = _mm256_set1_epi32((int)inc);
    __m256i vm = _mm256_setzero_si256();
    for (; k + 8 <= i1 + 1; k += 8) {
      const __m256i v = _mm256_add_epi32(_mm256_loadu_si256((const __m256i*)(sc + k)), vi);
      _mm256_storeu_si256((__m256i*)(sc + k), v);
      vm = _mm256_max_epu32(vm, v);
    }
    alignas(32) U32 m8[8];
    _mm256_store_si256((__m256i*)m8, vm);
    for (int i = 0; i < 8; i++) m = m8[i] > m ? m8[i] : m;
#endif
    for (; k <= i1; k++) {
      sc[k] += inc;
      m = sc[k] > m ? sc[k] : m;
    }
    return m;
  }
  // the stream's offset in each shape: an ABITS v of band b at whose offset lo to hi (as ap_commit()
  // has them) say it is, as a band's own scores go (from its first ABITS above 1; again where two in
  // a row lie past the scores, the level having moved)
  void ap_pool(int p, int b, int v, S64 lo, S64 hi) {
    AbPool& q = app[p];
    const U32 inc = apl[p].inc;
    if (q.set) {
      const S64 sh = ap_shape(q.k, b);
      if (hi - sh < (S64)q.c - AP_W || lo - sh > (S64)q.c + AP_W) {
        if (++q.out >= 2) q.set = 0;
      } else
        q.out = 0;
    }
    if (!q.set) {
      if (v < 2) return;
      const S64 m = ((lo > hi - 255 ? lo : hi - 255) + hi) >> 1;
      q.set = 1, q.out = 0, q.k = 0, q.c = (S32)(m - ap_shape(0, b));
      for (int k = 1; k < AP_NK; k++) q.best[k] = (S32)(m - ap_shape(k, b));
      memset(q.sc, 0, sizeof q.sc);
    }
    U32 mx[AP_NK];
    for (int k = 0; k < AP_NK; k++) {
      const S64 sh = ap_shape(k, b), cur = clip((S64)q.best[k] - q.c, -AP_W, AP_W) + AP_W;
      const S64 j0 = clip(lo - sh - q.c, -AP_W, AP_W + 1) + AP_W, j1 = clip(hi - sh - q.c, -AP_W - 1, AP_W) + AP_W;
      U32 best = q.sc[k][cur];
      S64 bo = cur;
      if (j0 <= j1) {  // (the best: the last one, or of those this raised the nearest it)
        const U32 m = ap_raise(q.sc[k], (int)j0, (int)j1, inc);
        best = q.sc[k][cur];
        if (m > best) {
          best = m, bo = -1;
          for (S64 j = j0; j <= j1; j++)
            if (q.sc[k][j] == best && (bo < 0 || (j > cur ? j - cur : cur - j) < (bo > cur ? bo - cur : cur - bo))) bo = j;
        }
      }
      q.best[k] = (S32)(bo - AP_W + q.c);
      mx[k] = best;
    }
    int kb = q.k;  // (another where it says more)
    for (int k = 0; k < AP_NK; k++)
      if (mx[k] > mx[kb]) kb = k;
    q.k = (U8)kb;
  }
  // pass p's subframe's ABITS all known (abl[p]'s obs): each band's offsets scored at its level, the
  // level's distance from the centre into the average
  void ap_commit(int p) {
    AbParLevel& A = apl[p];
    const AbLevel& L = abl[p];
    const S64 lv = (S64)A.pri + A.d;
    if (!A.anch) return;  // (a level of nothing yet: the stream's first subframe)
    if (!A.inc) A.inc = 1 << 10;
    for (int i = 0; i < L.nobs; i++) {
      AbPar& t = abp[p][L.obs[i].b];
      const int v = L.obs[i].v;
      if (v < 1 || v > 26) continue;
      const S64 x = (S64)L.obs[i].x - lv;
      const S64 lo = v < 26 ? x - ap_edge(v + 1) + 1 : -AP_FAR, hi = v > 1 ? x - ap_edge(v) : AP_FAR;
      ap_pool(p, L.obs[i].b, v, lo, hi);
      // (a band's scores start at its first ABITS above 1; again where two in a row lie past them:
      // the level has moved, as from a silence)
      if (t.set && (hi < (S64)t.c - AP_W || lo > (S64)t.c + AP_W) && ++t.out >= 2) t.set = 0;
      if (!t.set) {
        if (v < 2) {
          t.one = 1;
          continue;
        }
        t.set = 1, t.out = 0, t.c = t.o = (S32)(((lo > hi - 255 ? lo : hi - 255) + hi) >> 1);
        memset(t.sc, 0, sizeof t.sc);
      }
      const S64 i0 = clip(lo - t.c, -AP_W, AP_W + 1) + AP_W, i1 = clip(hi - t.c, -AP_W - 1, AP_W) + AP_W;
      if (i0 > i1) continue;
      t.out = 0;
      // (the best: the last one, or one of those the ABITS raised; of equal ones, the nearer it, then
      // the lower: the most of them, then of those the nearest)
      const S64 cur = t.o - t.c + AP_W;
      U32 best = ap_raise(t.sc, (int)i0, (int)i1, A.inc);
      if (t.sc[cur] > best) best = t.sc[cur];
      S64 bo = -1;
      if (t.sc[cur] == best) bo = cur;
      else
        for (S64 k = i0; k <= i1; k++)
          if (t.sc[k] == best && (bo < 0 || (k > cur ? k - cur : cur - k) < (bo > cur ? bo - cur : cur - bo))) bo = k;
      t.o = (S32)(bo - AP_W + t.c);
      A.inc += A.inc >> 10;  // (each ABITS: the older ones' scores 1023/1024 of the newer's)
      if (A.inc >= 1u << 20) {
        for (int c = 0; c < NSUB; c++)
          for (int k = 0; k <= 2 * AP_W; k++) abp[p][c].sc[k] >>= 10;
        for (int k = 0; k < AP_NK; k++)
          for (int j = 0; j <= 2 * AP_W; j++) app[p].sc[k][j] >>= 10;
        A.inc >>= 10;
      }
    }
    const S64 dd = (lv - A.cb) * 1024;
    A.drift = A.dset ? A.drift + (dd - A.drift) / 32 : dd;
    A.dset = 1;
  }
  // whether the table's guesses are the first ones: but where its misses, of the last 1024 or so
  // where both had one, are the staircases' and 128 more (format 77's: where they were fewer by
  // 2, from a stream's start on the staircases)
  bool ap_rule(int p) const { return apl[p].miss[1] < apl[p].miss[0] + 128; }
  // pass p's ABITS v: the staircases' guess sg, the table's tg
  void ap_score(int p, int sg, int tg, int v) {
    if (sg < 0 || tg < 0) return;
    U32* m = apl[p].miss;
    m[0] += v != sg, m[1] += v != tg;
    if (m[0] + m[1] >= 1024) m[0] >>= 1, m[1] >>= 1;
  }

  // the largest rise of subband b's energy from a subsubframe to the next (the ratio of the two,
  // each 1 more; 0 with one subsubframe), where it is (*tk, 1 to nssc - 1; 0 none)
  double rise_max(const CoreSub& s, int ch, int b, int* tk) const {
    double qm = 0;
    *tk = 0;
    for (int k = 1; k < s.nssc; k++) {
      S128 e0 = 1, e1 = 1;
      for (int j = 0; j < 8; j++) {
        const S64 x0 = X[ch][b][8 * (k - 1) + j], x1 = X[ch][b][8 * k + j];
        e0 += (S128)x0 * x0, e1 += (S128)x1 * x1;
      }
      const double q = (double)e1 / (double)e0;
      if (q > qm) qm = q, *tk = k;
    }
    return qm;
  }
  // The rise from a subsubframe to the next at which the encoders seen take a transient in subband
  // b: below 20 (where they predict), 8 (19 + b) / 19 times the energy (to 0.1% in each subband:
  // `s.2.dts`'s 2441 subbands below 20 that would gain by prediction and rise by it or more don't
  // predict, of the 216367 that rise less 130 don't; its TMODEs follow it but for 9 of the 2.27
  // million that don't predict, all within 2% of it; FFmpeg's master_audio_7.1_24bit.dts's as well,
  // but for its first subframe); from 20 up, 2000 times (since format 125: the Cymatics stream's
  // encoder takes 2719 of its 2721 transients there where the rise reaches it, and 88 more subbands
  // reach it; `s.2.dts`'s its one, the only subband there that rises 1500 times)
  static double rise_thr(int b) { return b < 20 ? 8.0 * (19 + b) / 19 : 2000.0; }
  // How subband b's energy rises against that, for its PMODE and TMODE (a sid context, tr:
  // model.inc): 0 by less than 20 eighths of a doubling (the largest rise; or one subsubframe), else
  // by the rise in eighths of a doubling past rise_thr(), floored: 1 -4 or less, 2 -3 or -2, 3 -1,
  // 4 0, 5 more (0 from subband 20 before format 125)
  struct Rise8 {  // (2^(r/8), r 0 to 48)
    double t[49];
    Rise8() {
      static const double R8[8] = {1.0, 1.0905077326652577, 1.189207115002721, 1.2968395546510096,
                                   1.4142135623730951, 1.5422108254079407, 1.681792830507429, 1.8340080864093424};
      for (int r = 0; r <= 48; r++) t[r] = ldexp(R8[r & 7], r >> 3);
    }
  };
  int pm_rise(const CoreSub& s, int ch, int b) const {
    static const Rise8 rise8;
    int tk;
    const double q = rise_max(s, ch, b, &tk);
    if (q < rise8.t[20]) return 0;
    const double u = q / rise_thr(b);  // (2^(d/8) at least, d the largest)
    return u >= rise8.t[1] ? 5 : u >= 1 ? 4 : 2 * u >= rise8.t[7] ? 3 : 2 * u >= rise8.t[5] ? 2 : 1;
  }
  // A lossless WAV's transient (TMODE) as encoders make it: where a subsubframe's energy is 8 times
  // the one before's at least, the largest such rise's (*tg, 1 to nssc - 1, else 0); how likely, the
  // class returned: 0 no such rise; 1 the subband predicts (the encoders seen make no transient
  // there); else 3 + 2 times the rise's doublings past 3 (to 2) + whether ABITS is 10 or more (the
  // likelier, the more of both; 2 from subband 20 up, before format 125)
  int tmode_rule(const CoreSub& s, int ch, int b, int* tg) {
    S128 e[NSSF];
    for (int k = 0; k < s.nssc; k++) {
      e[k] = 1;
      for (int j = 8 * k; j < 8 * k + 8; j++) e[k] += (S128)X[ch][b][j] * X[ch][b][j];
    }
    int best = 0, bh = -1;
    for (int k = 1; k < s.nssc; k++) {
      int h = 0;  // (the rise in doublings, to 6)
      while (h < 6 && e[k] >= (e[k - 1] << (h + 1))) h++;
      if (h > bh) bh = h, best = k;
    }
    *tg = 0;
    if (bh < 3) return 0;
    *tg = best;
    if (s.pmode[ch][b]) return 1;
    return 3 + 2 * imin(bh - 3, 2) + (s.abits[ch][b] >= 10);
  }
  // the 4 samples before the subframe's of channel ch's subband b (h[0] the last) as the encoder's
  // ADPCM had them: the decoder's (an encoder runs its ADPCM closed, so a lossless WAV's predictor,
  // PMODE and residual start from them), the audio's own where the scale factor search's rule is
  // taken (a decoded WAV's: its values are the decoder's, the decoder's history theirs before)
  void hist4(int ch, int b, S64* h) {
    const int closed = !ss_rule();
    for (int k = 0; k < 4; k++) h[k] = closed ? shl64(yh[ch][b][k], AQ_F) : xh[ch][b][k];
  }
  // the audio's values a scale factor scales (n of them into v): channel ch's subband b over its
  // subsubframes j0 to j1 (8 samples each), less the ADPCM prediction where the subband predicts:
  // from the 4 samples before the subframe as the encoder had them (hist4()), then from its own
  // (the subframe's decoder's samples aren't decoded yet)
  int scale_values(const CoreSub& s, int ch, int b, int j0, int j1, S64* v) {
    S64 h[4];
    hist4(ch, b, h);
    const int pm = s.pmode[ch][b], pvq = s.pvq[ch][b];
    int n = 0;
    for (int j = 0; j < j1 * 8; j++) {
      const S64 x = X[ch][b][j];
      if (j >= j0 * 8) v[n++] = pm ? x - adpcm_pred_x(pvq, h) : x;
      h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = x;
    }
    return n;
  }

  // --- the code books' indexes ----------------------------------------------------------------------

  // A rank r in 0..2^nb - 1 (nb <= RANK_BITS), vqr's contexts selected (from D): its bucket (0; 1;
  // 2-3; 4-7; ...) in unary, then its bits below the bucket's top. Past the first, whether it is
  // past bucket t's top (r >= 2^t) goes by how far the next bucket's first candidate (of the n, c,
  // by their keys) lies from the best against this one's (gr, the contexts selected again): where
  // the encoder's arithmetic makes another its choice, the nearer ones are likelier, and how many
  // lie near goes by where the keys jump (`s.2.dts`'s core 0.7% smaller)
  template <class F, class V>
  int code_rank(F& f, int r, int nb, V& D, const KeyIx* c, int n, const double* qr = nullptr) {
    // (qr: each rank's probability, the outside prediction of each decision, its stretch doubled)
    auto setp = [&](int lo, int mid, int hi) {
      if (!qr) return;
      double a = 0, t = 0;
      for (int i = lo; i < hi && i < n; i++) t += qr[i], a += i >= mid ? qr[i] : 0;
      const int p0 = t > 0 ? clip((int)((1.0 - a / t) * CM_PONE + 0.5), 1, CM_PONE - 1) : CM_PHALF;
      f.prp = cm_pclip(cm_squash(2 * cm_stretch(p0)));
    };
    const int bk = ENC ? bit_len((U64)r) : 0;
    int t = 0;
    S128 sk[64];  // (the keys, sorted, once past the first bucket)
    const int ns = n < 64 ? n : 64;
    while (t < nb) {
      if (t == 1)
        for (int i = 0; i < ns; i++) {
          int j = i;
          for (; j > 0 && sk[j - 1] > c[i].k; j--) sk[j] = sk[j - 1];
          sk[j] = c[i].k;
        }
      if (t >= 1) {
        D.gr = 7;
        if ((1 << t) < ns) {  // (the gaps' ratio in doublings: 0 within 2, 6 by 64 or more)
          const S128 a = sk[1 << t] - sk[0], g = sk[1 << (t - 1)] - sk[0];
          D.gr = !g ? (a ? 6 : 0) : 0;
          if (g)
            while (D.gr < 6 && a >= g << (D.gr + 1)) D.gr++;
        }
        B::sel(f, D);
      }
      setp(t ? 1 << (t - 1) : 0, 1 << t, n);
      if (!f.bit(VN_BKT + t, ENC && bk > t)) break;
      t++;
    }
    if (t < 2) {
      f.prp = -1;
      return t;
    }
    int m = 1;
    for (int p = 0; p < t - 1; p++) {
      const int sh = t - 2 - p;
      setp(m << (sh + 1), (2 * m + 1) << sh, (m + 1) << (sh + 1));
      m = 2 * m + f.bit(VN_BIT + 3 * t + imin(p, 2), ENC && ((r >> sh) & 1));
    }
    f.prp = -1;
    return m;
  }
  // the rank of index v among the n candidates (c), or the index of the rank coded (d); kind kd, the
  // subband's last rank of the kind lh (lrank's, or X96's)
  // (mu > 0: how far the best beats the next against mu, not against the best, while that has been
  // wrong where sure (8 or more) no more than 2 times more often than the other: a decoded WAV's
  // vectors are nearly its own, and miss at near ties by the best's distance)
  U32 hvs[2];  // (the misses where each was sure: against the best, against mu)
  U32 hrs[2];  // (code_hfvq(): which ranked the encoder's group higher, the values as they are or scaled into its scale factor's region)
  U32 hpw[2];  // (code_hfvq(): where a group wasn't the first, the vectors' order that ranked the encoder's higher, the values as they are or moved)
  // A lossless WAV's HF VQ subbands at the core's rate (code_hfvq()): the groups and the entries by
  // a Monte Carlo posterior too, the encoder's choice made on samples of its noisy values. The noise
  // is drawn about each value at its deviation (in the top subband 3 2^-40 of the bottom subband's
  // binade's square, hf_stair2()'s, in subbands 28-30 2^-41 of the square of the level along the
  // subband's FFT path, plev's, 2^-40 before format 98), the samples whose mean square the scale
  // factor's region (the RMS rule's, code_scale()'s) doesn't hold left out; each sample's group the
  // scores' rule's (hf_group_score()'s, the scores as the model has learned them), its entry the
  // group's nearest.
  // The counts, with a prior of MC_A to each per MC_N kept, give each candidate's probability, and
  // each decision of its rank (code_rank()) the probability of its outcome as vqr's outside
  // prediction, its stretch doubled; the entry's probabilities are those of the samples that took
  // the encoder's group, or where fewer than MC_NG did, the group's nearest entries of all the
  // samples. Taken alone, the posterior codes about as the model does (3 MB of `s.2.dts`'s middle:
  // its top subband's groups 407 bytes against the model's 420, the entries 457 against 459), and
  // the two differ: mixed at a fixed weight, 6% less than either. As the outside prediction,
  // `s.2.dts`'s core 0.72% smaller (the top subband's alone, the stretch as it is, the prior 1/4);
  // doubled, the prior 1/20, 0.25% more (by 3 0.01% less, by 4 0.04%); more passes where the region
  // leaves few, 0.18% (format 94's to 256 kept, at most 512, 0.19% more; since format 96 to MC_T, at
  // most MC_K, 0.20% more again, to MC_P passes rather than 8 0.10%, the prior 1/80 0.02%; 64 passes,
  // 0.01% more; the top subband's noise at 2, 3.25, 3.5 or 4 2^-40, 0.15%, 0.02%, 0.05% and 0.10%
  // bigger); subbands 28-30 too, 0.27% (since format 98 their noise at 2^-41, 0.05% smaller; at
  // 2^-42 0.02%, 2^-43 or 2^-39 bigger; their scale factors' noise at half of SP_K2 bigger, at twice
  // the same; their noise at 2^-38 0.11% less; at format 88's 2^-44, the short streams 0.17%
  // bigger): 81703 bytes to 80550
  // in format 93, 3 MB of its middle 0.9% smaller, its first 3 MB 0.2%, the rest as they were. The
  // top subband's noise at 2 or 4 2^-40, 0.14% and 0.02% bigger; with heavier tails (Laplace's,
  // Student's t with 5 or 3 degrees of freedom, at the same variance), the short streams 1-5 bytes
  // bigger; the scores not learning from a miss the samples gave 1/2 or more, bigger (from 1/20, 15
  // bytes in the short streams); the samples' groups by the scores weighed
  // as the model's are (gw), the same; the posterior's first as the near-tie count's context, bigger.
  // A stop once MC_B samples agree on both, 5 bytes more; with it and the scores' sums in AVX2,
  // `s.2.dts`'s core compresses in 61 seconds where format 92's took 38 (100 without them). Since
  // format 96 the samples are made four at a time, the scores' least found in AVX2 and the entries'
  // nearest by 64-bit keys (hf_near()), the same in a third less time (3 MB of `s.2.dts`'s middle in
  // 1.27 seconds where format 95's took 1.93): four times the samples, about as fast. The samples:
  // MC_Q's 256 quantiles in an order of each value's own, a pass taking them in another pairing (all
  // in IEEE's basic arithmetic, the scores' products and sums in the same order in the AVX2 build and
  // the plain one)
  static constexpr int MC_N = 256, MC_K = 2048, MC_B = 32, MC_P = 32, MC_T = 1024, MC_NG = 16;
  static constexpr double MC_A = 0.0125;
  double mcz[MC_N][16];      // (the noise's samples)
  double mczt[16][MC_N + 4];  // (the same by value, the first 4 again at the end)
  S64 mcy[MC_K][16];         // (the samples kept, the encoder's group and entry for each)
  int mcg[MC_K], mce[MC_K];
  // (the entry of group g nearest y, hf_dists()'s least, the first of equals: as each distance is
  // 256 y y + 2^21 sc (2^11 sc e - y v), e the entry's energy, by the latter in 64 bits where
  // 0 < sc < 2^31 and |y| < 2^40 (sm: known to be), 2^11 sc e within 2^60 and y v 2^51)
  static int hf_near(int g, const S64* y, S64 sc, bool sm = false) {
    if (!sm) {
      S64 mx = 0;
      for (int j = 0; j < 16; j++) mx |= y[j] < 0 ? -y[j] : y[j];
      sm = mx < (S64)1 << 40;
    }
    if (sc < 1 || sc >= (S64)1 << 31 || !sm) {
      S128 dd[32];
      hf_dists(g, y, 16, sc, dd);
      int be = 0;
      for (int i = 1; i < 32; i++) be = dd[i] < dd[be] ? i : be;
      return be;
    }
    alignas(32) double a[32];
    hf_gdots(g, y, 16, a);
    const S32* const ee = hf_energies.e[1] + g * 32;
#ifdef DTSD_AVX2  // (the keys in 64 bits, the dot products' by the magic number 1.5 2^52 as |y v| < 2^51; the least's first)
    const __m256d mg = _mm256_set1_pd(6755399441055744.0);
    const __m256i mgi = _mm256_castpd_si256(mg), sv = _mm256_set1_epi64x(sc);
    __m256i k[8];
    for (int i = 0; i < 8; i++)
      k[i] = _mm256_sub_epi64(_mm256_slli_epi64(_mm256_mul_epu32(_mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(ee + 4 * i))), sv), 11),
                              _mm256_sub_epi64(_mm256_castpd_si256(_mm256_add_pd(_mm256_load_pd(a + 4 * i), mg)), mgi));
    __m256i m = k[0];
    for (int i = 1; i < 8; i++) m = _mm256_blendv_epi8(m, k[i], _mm256_cmpgt_epi64(m, k[i]));
    __m256i w = _mm256_permute4x64_epi64(m, 0x4E);
    m = _mm256_blendv_epi8(m, w, _mm256_cmpgt_epi64(m, w));
    w = _mm256_permute4x64_epi64(m, 0xB1);
    m = _mm256_blendv_epi8(m, w, _mm256_cmpgt_epi64(m, w));
    U32 eq = 0;
    for (int i = 0; i < 8; i++) eq |= (U32)_mm256_movemask_pd(_mm256_castsi256_pd(_mm256_cmpeq_epi64(k[i], m))) << (4 * i);
    return __builtin_ctz(eq);
#else
    int be = 0;
    S64 bk = ((S64)ee[0] * sc << 11) - (S64)a[0];
    for (int i = 1; i < 32; i++) {
      const S64 k = ((S64)ee[i] * sc << 11) - (S64)a[i];
      if (k < bk) be = i, bk = k;
    }
    return be;
#endif
  }
  // hf_mc(): the encoder's values sampled about x, the noise's deviation sg at each, those whose mean
  // square lies in lo2..hi2 kept (to MC_K), each one's group by the scores' rule (hf_group_score()'s,
  // in doubles) and its entry there; a block of MC_B at a time, of MC_N in a pass, a pass taking the
  // values' samples in another pairing, until MC_B kept agree, or after a pass MC_T are kept, or MC_P
  // passes, or 8 with none kept; how many were kept
  int hf_mc(const S64* x, const double* sg, double lo2, double hi2, S64 sc) {
    alignas(32) double kq[32], wt[16][32];
    for (int q = 0; q < 32; q++) {
      double w2 = 0;
      for (int j = 0; j < 16; j++) w2 += (double)hgw[q][j] * (double)hgw[q][j], wt[j][q] = (double)hgw[q][j];
      kq[q] = (double)sc * (ldexp(w2, AQ_F) + ldexp((double)hge[q], 32 - AQ_F)) / 16384.0;
    }
    int mk = 0;
    bool same = true;
#ifdef DTSD_AVX2  // (four samples at a time, where each value lies within 2^51: x within 2^50, the noise 2^49)
    S64 mx = 0;
    double sm = 0;
    for (int j = 0; j < 16; j++) mx |= x[j] < 0 ? -x[j] : x[j], sm = sm < fabs(sg[j]) ? fabs(sg[j]) : sm;
    const bool v4 = mx < (S64)1 << 50 && sm < 70368744177664.0;  // (|MC_Q| < 3: 2^46)
    const bool ys = mx < (S64)1 << 39 && sm < 68719476736.0;    // (and the samples within 2^40: 2^36)
    alignas(32) S64 y4[16][4];
    alignas(32) double ms4[4];
#else
    const bool ys = false;
#endif
    for (int pass = 0; pass < MC_P; pass++) {
      for (int t0 = 0; t0 < MC_N; t0 += MC_B) {
        for (int t = t0; t < t0 + MC_B && mk < MC_K; t++) {
          S64* y = mcy[mk];
          double ms = 0;
#ifdef DTSD_AVX2
          if (v4) {
            if (!(t & 3)) {  // (as below, the rounding by truncation, to 64 bits and back by the magic number 1.5 2^52)
              const __m256d mg = _mm256_set1_pd(6755399441055744.0), hp = _mm256_set1_pd(0.5), hn = _mm256_set1_pd(-0.5);
              const __m256i mgi = _mm256_castpd_si256(mg);
              __m256d s = _mm256_setzero_pd();
              for (int j = 0; j < 16; j++) {
                const __m256d e = _mm256_mul_pd(_mm256_set1_pd(sg[j]), _mm256_loadu_pd(mczt[j] + ((t + pass * (53 * j + 1)) & (MC_N - 1))));
                const __m256d r = _mm256_round_pd(_mm256_add_pd(e, _mm256_blendv_pd(hn, hp, _mm256_cmp_pd(e, _mm256_setzero_pd(), _CMP_GE_OQ))),
                                                  _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
                const __m256i v = _mm256_add_epi64(_mm256_set1_epi64x(x[j]), _mm256_sub_epi64(_mm256_castpd_si256(_mm256_add_pd(r, mg)), mgi));
                _mm256_store_si256((__m256i*)y4[j], v);
                const __m256d vd = _mm256_sub_pd(_mm256_castsi256_pd(_mm256_add_epi64(v, mgi)), mg);
                s = _mm256_add_pd(s, _mm256_mul_pd(vd, vd));
              }
              _mm256_store_pd(ms4, s);
            }
            for (int j = 0; j < 16; j++) y[j] = y4[j][t & 3];
            ms = ms4[t & 3];
          } else
#endif
          for (int j = 0; j < 16; j++) {
            const double e = sg[j] * mcz[(t + pass * (53 * j + 1)) & (MC_N - 1)][j];
            y[j] = x[j] + (S64)(e + (e >= 0 ? 0.5 : -0.5));
            ms += (double)y[j] * (double)y[j];
          }
          ms /= 16;
          if (ms < lo2 || ms >= hi2) continue;
          int bq = 0;  // (the least of kq - y.W, the first of equals: the products and sums in order, as the plain C++'s)
#ifdef DTSD_AVX2
          __m256d a0 = _mm256_setzero_pd(), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0;
          for (int j = 0; j < 16; j++) {
            const __m256d v = _mm256_set1_pd((double)y[j]);
            const double* const w = wt[j];
            a0 = _mm256_add_pd(a0, _mm256_mul_pd(v, _mm256_load_pd(w))), a1 = _mm256_add_pd(a1, _mm256_mul_pd(v, _mm256_load_pd(w + 4)));
            a2 = _mm256_add_pd(a2, _mm256_mul_pd(v, _mm256_load_pd(w + 8))), a3 = _mm256_add_pd(a3, _mm256_mul_pd(v, _mm256_load_pd(w + 12)));
            a4 = _mm256_add_pd(a4, _mm256_mul_pd(v, _mm256_load_pd(w + 16))), a5 = _mm256_add_pd(a5, _mm256_mul_pd(v, _mm256_load_pd(w + 20)));
            a6 = _mm256_add_pd(a6, _mm256_mul_pd(v, _mm256_load_pd(w + 24))), a7 = _mm256_add_pd(a7, _mm256_mul_pd(v, _mm256_load_pd(w + 28)));
          }
          a0 = _mm256_sub_pd(_mm256_load_pd(kq), a0), a1 = _mm256_sub_pd(_mm256_load_pd(kq + 4), a1);
          a2 = _mm256_sub_pd(_mm256_load_pd(kq + 8), a2), a3 = _mm256_sub_pd(_mm256_load_pd(kq + 12), a3);
          a4 = _mm256_sub_pd(_mm256_load_pd(kq + 16), a4), a5 = _mm256_sub_pd(_mm256_load_pd(kq + 20), a5);
          a6 = _mm256_sub_pd(_mm256_load_pd(kq + 24), a6), a7 = _mm256_sub_pd(_mm256_load_pd(kq + 28), a7);
          __m256d m = _mm256_min_pd(_mm256_min_pd(_mm256_min_pd(a0, a1), _mm256_min_pd(a2, a3)), _mm256_min_pd(_mm256_min_pd(a4, a5), _mm256_min_pd(a6, a7)));
          m = _mm256_min_pd(m, _mm256_permute2f128_pd(m, m, 1));
          m = _mm256_min_pd(m, _mm256_permute_pd(m, 5));  // (the least in each lane; the first lane equal to it)
          bq = __builtin_ctz((U32)_mm256_movemask_pd(_mm256_cmp_pd(a0, m, _CMP_EQ_OQ)) | (U32)_mm256_movemask_pd(_mm256_cmp_pd(a1, m, _CMP_EQ_OQ)) << 4 |
                             (U32)_mm256_movemask_pd(_mm256_cmp_pd(a2, m, _CMP_EQ_OQ)) << 8 | (U32)_mm256_movemask_pd(_mm256_cmp_pd(a3, m, _CMP_EQ_OQ)) << 12 |
                             (U32)_mm256_movemask_pd(_mm256_cmp_pd(a4, m, _CMP_EQ_OQ)) << 16 | (U32)_mm256_movemask_pd(_mm256_cmp_pd(a5, m, _CMP_EQ_OQ)) << 20 |
                             (U32)_mm256_movemask_pd(_mm256_cmp_pd(a6, m, _CMP_EQ_OQ)) << 24 | (U32)_mm256_movemask_pd(_mm256_cmp_pd(a7, m, _CMP_EQ_OQ)) << 28);
#else
          double dt[32];
          for (int q = 0; q < 32; q++) {
            double a = 0;
            for (int j = 0; j < 16; j++) a += (double)y[j] * wt[j][q];
            dt[q] = kq[q] - a;
          }
          for (int q = 1; q < 32; q++)
            if (dt[q] < dt[bq]) bq = q;
#endif
          const int be = hf_near(bq, y, sc, ys);
          mcg[mk] = bq, mce[mk] = be;
          same = same && (!mk || (bq == mcg[0] && be == mce[0]));
          mk++;
        }
        if (mk >= MC_K || (same && mk >= MC_B)) return mk;
      }
      if (mk >= MC_T || (!mk && pass >= 7)) return mk;  // (none kept in 8 passes: none to be had)
    }
    return mk;
  }
  // (mlu > 0: ml's unit)
  // (ord: where given, the ranks' order instead of the keys', which the margins go by: each
  // candidate's, by its index)
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn, S128 mu = 0, S128 mlu = 0, const S128* ord = nullptr,
                const double* pq = nullptr) {
    return code_cand(n, nb, v, kd, ch, b, gn, lrank[ch][b][kd], mu, mlu, ord, pq);
  }
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn, U8& lh, S128 mu = 0, S128 mlu = 0, const S128* ord = nullptr,
                const double* pq = nullptr) {
    M_vqr_v D;
    D.kd = kd;
    D.cc = chan_class(ch);
    D.bd = b;
    const int mr = key_margin(cand, n), ml = mu > 0 ? key_margin_u(cand, n, mu) : 0;
    D.mg = mu > 0 && hvs[1] <= hvs[0] + 2 ? ml : mr;
    D.ml = mlu > 0 ? key_margin_u(cand, n, mlu, 25, 14) : 15;
    D.nc = 6;
    if (mlu > 0 && (kd == VK_HFV || kd == VK_HFG)) {  // (how many are near ties with the nearest: where
                                                      // the encoder's arithmetic makes another its choice,
                                                      // which one; the groups' too, in their unit's terms:
                                                      // `s.2.dts`'s core 0.06% smaller; within 2^-20 of
                                                      // the unit for the groups and 2^-19 for the
                                                      // vectors, from 2^-18 for both, 0.18% more)
      S128 d1 = cand[0].k;
      for (int i = 1; i < n; i++) d1 = cand[i].k < d1 ? cand[i].k : d1;
      int c = -1;
      const S128 tie = mlu >> (kd == VK_HFG ? 20 : 19);
      for (int i = 0; i < n; i++) c += cand[i].k - d1 <= tie;
      D.nc = c <= 2 ? c : c <= 4 ? 3 : c <= 8 ? 4 : 5;
    }
    D.gn = gn;
    D.lh = lh;
    D.gr = 7;
    if (ord)
      for (int i = 0; i < n; i++) cand[i].k = ord[cand[i].i];
    if (ord && mlu > 0) {  // (and ml in the order's terms: the second's boundary key against the unit,
                           // the groups' 2^-22 of it, the vectors' 2^-5, as their W, in 512ths of the
                           // vectors' units, run to the power 1.5: `s.2.dts`'s core 0.5% smaller; 0
                           // where the rest tie with the first)
      S128 k2 = -1;
      for (int i = 0; i < n; i++)
        if (cand[i].k >= 0 && (k2 < 0 || cand[i].k < k2)) k2 = cand[i].k;
      S128 u = mlu >> (kd == VK_HFG ? 22 : 5);
      if (u < 1) u = 1;
      D.ml = k2 < 0 ? 0 : imin(near_bucket(k2, u, 25), 13);
    }
    // (pq, each candidate's probability, by rank; and how sure that is, mq, the likeliest one's
    // probability, which selects the first mixer's weights and a table's rows: how far to trust the
    // posterior goes with how sure it is. Since format 99: `s.2.dts`'s core 0.07% smaller, 0.11%
    // with vqr tuned again, the short streams 0.03%; the likeliest one's against the first in the
    // order too, or 5 or 12 classes rather than 8, the same; in the other mixers' indexes, bigger)
    double qr[64];
    D.mq = 0;
    if (pq) {
      int ix[64];
      for (int a = 0; a < n; a++) {
        int k = a;
        for (; k > 0 && key_less(cand[a], cand[ix[k - 1]]); k--) ix[k] = ix[k - 1];
        ix[k] = a;
      }
      for (int k = 0; k < n; k++) qr[k] = pq[cand[ix[k]].i];
      double pt = qr[0];
      for (int k = 1; k < n; k++) pt = qr[k] > pt ? qr[k] : pt;
      D.mq = 1 + (pt < 0.3 ? 0 : pt < 0.45 ? 1 : pt < 0.6 ? 2 : pt < 0.75 ? 3 : pt < 0.9 ? 4 : pt < 0.97 ? 5 : pt < 0.995 ? 6 : 7);
    }
    int at = 0;
    if (ENC)
      while (cand[at].i != v) at++;
    const int r0 = ENC ? rank_of(cand, n, at) : 0;
    int r;
    if (kd == VK_PVQ || kd == VK_PVE) {  // (the predictors' ranks, by the vqp family: format 116; vpd's where the WAV is decoded)
      if (dw_rule()) {
        M_vpd_v E;
#define M_COPY(f) E.f = D.f;
        M_vqr_VARS(M_COPY)
#undef M_COPY
        B::sel(f_vpd, E);
        r = code_rank(f_vpd, r0, nb, E, cand, n, pq ? qr : nullptr);
      } else {
        M_vqp_v E;
#define M_COPY(f) E.f = D.f;
        M_vqr_VARS(M_COPY)
#undef M_COPY
        B::sel(f_vqp, E);
        r = code_rank(f_vqp, r0, nb, E, cand, n, pq ? qr : nullptr);
      }
    } else {
      B::sel(f_vqr, D);
      r = code_rank(f_vqr, r0, nb, D, cand, n, pq ? qr : nullptr);
    }
    if (!ENC) v = select_rank(cand, n, r);
    if (mu > 0 && r) {
      hvs[0] += mr >= 8, hvs[1] += ml >= 8;
      if (hvs[0] + hvs[1] > 4096) hvs[0] >>= 1, hvs[1] >>= 1;
    }
    lh = r ? 2 : 1;
    n_vq[kd]++;
    n_vqhit[kd] += !r;
    return v;
  }
  // The ADPCM predictor's search, as the encoder's: the two stages (anal.inc's pvq_block_dist()),
  // but where the nearest of all has been the predictor clearly more often (another encoder's)
  bool pv_nearest() const { return pvr[1] > pvr[0] + 64 + pvr[0] / 8; }
  // the predictor it would choose for the coefficients alpha, the entries' distances d
  int pvq_first(const S64* alpha, const S64* d) const {
    int g = 0;
    if (pv_nearest()) {
      for (int q = 1; q < 4096; q++)
        if (d[q] < d[g]) g = q;
      return g;
    }
    S64 bd = pvq_block_dist(0, alpha);
    for (int k = 1; k < 64; k++) {
      const S64 x = pvq_block_dist(k, alpha);
      if (x < bd) bd = x, g = k;
    }
    int e = g * 64;
    for (int q = e + 1; q < g * 64 + 64; q++)
      if (d[q] < d[e]) e = q;
    return e;
  }
  // Predictor v (index of the code book) for the coefficients alpha, the entries' distances d: its
  // block's rank (by the codewords, or by each one's nearest entry: pv_nearest()) and its rank in
  // the block (by d); fit, where >= 0, first in both (a decoded WAV's: code_pvq()); gn vqr's, the
  // subband's last ranks of the two lb, le. The rule's tallies kept.
  int code_pvq2(const S64* alpha, const S64* d, int fit, int v, int ch, int b, int gn, U8& lb, U8& le, const int* more = nullptr,
                int nmore = 0) {
    const bool nr = pv_nearest();
    int g0 = 0, q0 = 0;  // (the two stages' first, the nearest of all)
    S64 bd = -1;
    for (int k = 0; k < 64; k++) {
      S64 m = d[k * 64];
      int mq = k * 64;
      for (int q = k * 64 + 1; q < k * 64 + 64; q++)
        if (d[q] < m) m = d[q], mq = q;
      if (d[mq] < d[q0]) q0 = mq;
      const S64 x = pvq_block_dist(k, alpha);
      if (bd < 0 || x < bd) bd = x, g0 = k;
      cand[k].k = nr ? m : x, cand[k].i = k;
    }
    // (more: other fits after it, each first after those before it)
    const int nf = fit >= 0 ? 1 + nmore : 0;
    for (int i = 0; i < nf; i++) {
      const int q = i ? more[i - 1] : fit;
      if (cand[q >> 6].k > -(S64)(nf - i)) cand[q >> 6].k = -(S64)(nf - i);
    }
    const S64 h0 = n_vqhit[VK_PVQ] + n_vqhit[VK_PVE];
    const int g = code_cand(64, 6, ENC ? v >> 6 : 0, VK_PVQ, ch, b, gn, lb);
    for (int i = 0; i < 64; i++) cand[i].k = d[g * 64 + i], cand[i].i = i;
    for (int i = 0; i < nf; i++) {
      const int q = i ? more[i - 1] : fit;
      if (q >> 6 == g && cand[q & 63].k > -(S64)(nf - i)) cand[q & 63].k = -(S64)(nf - i);
    }
    v = g * 64 + code_cand(64, 6, ENC ? v & 63 : 0, VK_PVE, ch, b, gn, le);
    n_pv++;
    n_pvhit += (S64)(n_vqhit[VK_PVQ] + n_vqhit[VK_PVE]) == h0 + 2;
    int e0 = g0 * 64;
    for (int q = e0 + 1; q < g0 * 64 + 64; q++)
      if (d[q] < d[e0]) e0 = q;
    pvr[0] += e0 == v, pvr[1] += q0 == v;
    if (pvr[0] + pvr[1] >= 1u << 16) pvr[0] >>= 1, pvr[1] >>= 1;
    return v;
  }
  // Whether channel ch's subband b is quiet as the encoders seen take it, where they make no
  // prediction: its values' RMS over the subframe under 100 (`s.2.dts`'s subbands that would gain
  // by it from 99.5 to 100 predict 2.4% of the time, from 100 to 100.5 97%; where the RMS rule's
  // scale factor under 100 was taken, an RMS under 99.5, 78 more of them the other way: its core
  // 0.05% smaller, `master_audio_7.1_24bit.dts`'s 0.5%)
  bool pm_quiet(int ch, int b) const {
    S128 e = 0;
    for (int j = 0; j < nsmp; j++) e += (S128)X[ch][b][j] * X[ch][b][j];
    return e < (S128)nsmp * (((S128)100 << AQ_F) * ((S128)100 << AQ_F));
  }
  // How the encoder would gain by predicting channel ch's subband b: 0 by less than 2^(1/2) (the
  // LPC's gain), 9 by more than 2^(9/4), else by the gain of the entry its search would choose for
  // the LPC (pvq_first()) over the subframe, in quarters of halvings less 2 (1-8)
  int pmode_gain(int ch, int b) {
    const int k = lpg4[ch][b];
    if (k < 2 || k > 9) return k < 2 ? 0 : 9;
    S64* const d = this->pvqd;
    pvq_dists(lpc[ch][b], d);
    const int bq = pvq_first(lpc[ch][b], d);
    S64 hd[4];
    hist4(ch, b, hd);
    return clip(pvq_gain4(bq, hd, X[ch][b], nsmp) - 2, 1, 8);
  }
  // Channel ch's subband b's ADPCM predictor v: its ranks as the encoder's search would make them
  // from the subband's LPC (code_pvq2()). (Where the scale factor search's rule is taken, a decoded
  // WAV's: of the first 1024 by the distance from the LPC, or 4 where few have fitted, the first
  // whose prediction leaves values that fit a quantizer's lattice, by the bit allocation search, to
  // a 64th of a step squared a value, first: a decoded WAV's values less the decoder's prediction
  // are its dequantized indexes. Others fit too, nearly, where their predictions are near the
  // encoder's: where that fit isn't within a 1024th of a step squared a value (as the encoder's
  // nearly always is), the rest of the 1024 on that fit's lattice, the nearest taken, the first of
  // equals: `ES 6.1 16bit.dts`'s predictors 99.38% the audio's from 90.60%, its core 21.3% smaller,
  // `open bitrate.dts`'s 16.2%, FATE's `dts_es.dts`'s 3.6%.)
  KeyIx pvsel[4096];  // (code_pvq()'s selection, and its candidates' heap)
  int pvtop[4096];
  U64 pvpk[4096], pvpks[4096];  // (and its keys packed, d 2^12 + q, and their selection)
  S64 code_pvq(const CoreRec& r, const CoreSub& s, int ch, int b, S64 v) {
    S64* const d = this->pvqd;
    pvq_dists(lpc[ch][b], d);
    int fit = -1;
    U64 fd = ~0ull;  // (the fit's distance; its lattice, and the error sum fd makes on it)
    QErr fq;
    S128 flim = 0;
    if (ss_rule()) {
      // (fewer, PV_KLOW, where few of the first 4 fit: a 96/24 stream's core's; there, whether few
      // do goes by the first 4 however many are tried)
      const int K = pv_none * 4 > pv_n ? PV_KLOW : 1024, kn = K < 1024 ? 4 : K;
      // (the order's first K: the K-th by selection, those up to it on a heap, taken in order. By
      // keys d 2^12 + q, the order of d and q, where the K-th's d is under 2^52 - 1: those of it or
      // more held at that come after the first K as they do; else by the 128-bit keys)
      int nk = 0, hn;
      int* const top = pvtop;
      const U64* const pk = pvpk;
      constexpr S64 PK_SAT = ((S64)1 << 52) - 1;
      for (int q = 0; q < 4096; q++) pvpk[q] = (U64)(d[q] < PK_SAT ? d[q] : PK_SAT) << 12 | (U64)q;
      memcpy(pvpks, pvpk, sizeof pvpks);
      const U64 kp = select_u64(pvpks, 4096, K - 1);
      const bool packed = (S64)(kp >> 12) < PK_SAT;
      if (packed) {
        for (int q = 0; q < 4096; q++)
          if (pk[q] <= kp) top[nk++] = q;
      } else {
        for (int q = 0; q < 4096; q++) cand[q].k = d[q], cand[q].i = q;
        memcpy(pvsel, cand, sizeof pvsel);
        const KeyIx kk = cand[select_rank(pvsel, 4096, K - 1)];
        for (int q = 0; q < 4096; q++)
          if (!key_less(kk, cand[q])) top[nk++] = q;
      }
      auto before = [&](int a, int b) { return packed ? pk[a] < pk[b] : key_less(cand[a], cand[b]); };
      auto sift = [&](int i, int n) {
        for (;;) {
          const int l = 2 * i + 1, rt = l + 1;
          int m = i;
          if (l < n && before(top[l], top[m])) m = l;
          if (rt < n && before(top[rt], top[m])) m = rt;
          if (m == i) return;
          const int t = top[i];
          top[i] = top[m], top[m] = t;
          i = m;
        }
      };
      for (int i = nk / 2 - 1; i >= 0; i--) sift(i, nk);
      hn = nk;
      for (int k = 0; k < nk; k++) {
        const int qk = top[0];  // (the next in the order)
        top[0] = top[--hn];
        sift(0, hn);
        S64 x[NSMP];
        S64 h[4] = {xh[ch][b][0], xh[ch][b][1], xh[ch][b][2], xh[ch][b][3]};
        const int n = s.nssc * 8;
        for (int j = 0; j < n; j++) {
          const S64 y = X[ch][b][j];
          x[j] = y - adpcm_pred_x(qk, h);
          h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
        }
        if (fit >= 0) {  // (past the fit: on its lattice)
          S128 er = 0;
          int nz = 0;
          for (int j = 0; j < n && er < flim; j++) er += fq.err(x[j], &nz);
          if (er < flim) {
            fit = qk, fd = norm_dist(er, fq.ss);
            flim = (S128)((double)fd * (double)fq.ss * (double)fq.ss / 268435456.0 * (1 + 1e-9)) + 1;
          }
          continue;
        }
        int near;
        const int e = 1 + scale_pred(x, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
        const U8 sv = ab_seen[1][b][imin(e, 128)];
        const S32 og = sv ? sv : hist.abits[ch][b];
        if (!og) break;
        const int g = og - 1, a0 = imax(1, g - 3), a1 = imin(26, g + 3);
        int conf, fi = -1;
        U64 d = ~0ull;
        const int G = abits_search(r, ch, x, n, g, a0, a1, e - 1, &conf, &d, (U64)n << 10, &fi);
        if (G) {
          fit = qk, fd = d;
          if (fd < (U64)n << 6) break;
          S64 lo, hi;
          quant_range(r, ch, G - 1, lo, hi);
          fq.set(step_for(r, ch, G - 1, fi), lo, hi);
          flim = (S128)((double)fd * (double)fq.ss * (double)fq.ss / 268435456.0 * (1 + 1e-9)) + 1;
          continue;
        }
        pv_none += k == imin(nk, kn) - 1;
      }
      if (++pv_n >= 1u << 16) pv_n >>= 1, pv_none >>= 1;
    }
#ifdef DTSD_PVQDUMP
    if (ENC && !ss_rule()) pvq_dump(lpc[ch][b], (int)v);
#endif
    return code_pvq2(lpc[ch][b], d, fit, (int)v, ch, b, lpg[ch][b], lrank[ch][b][VK_PVQ], lrank[ch][b][VK_PVE]);
  }
  // Channel ch's subband b's HF VQ vector v (its scale factor coded): the rank of its group by the
  // groups' scores (anal.inc's hf_group_score(), as hgw learns them), then its rank in the group
  // (The groups by their nearest entries instead, where the nearest of all lies within 1/64 of the
  // values' energy: a decoded WAV's values are the vector's, whatever the encoder's search; a
  // lossless WAV's aren't, and where the encoder's order misses, the group is near its first. A
  // group's nearest entry counts at 16 times the nearest of all at most. They are found in the
  // first 2048 vectors, while that has been so in 1 of 8, and in 1 vector in 64.)
  S64 code_hfvq(const CoreRec& r, const CoreSub& s, int ch, int b, S64 v) {
    const int si = s.scale[ch][b][0];
    const S64 sc = r.shuff[ch] == 6 ? SF7[si] : SF6[si & 63];
    const S64* x = X[ch][b];
    const int tried = hf_min_on();
    // (a lossless WAV's, where the RMS rule's region of the encoder's scale factor (code_scale()'s
    // rule: the top subband's with the leak's noise in it) doesn't hold the values' RMS: the
    // encoder's values were louder or quieter than these, so the groups and the vectors go by these
    // scaled into the region, where that has ranked the encoder's group higher clearly more often
    // than the values as they are: `s.2.dts`'s top subband's group the first 64.3% of the time there
    // from 62.7%, its vector the nearest 59.8% from 58.3%, its core 0.5% smaller, a quarter of the way
    // in; a sixteenth of the way, just past the edge, 0.1% more)
    // (since format 87 the top subband's, at the core's rate, go by the mean of the encoder's values
    // given that their RMS lies in that region, the noise's mean there: the RMS's square errs by
    // D = 2/16 x n (and n n / 16, its mean the rule's noise), a normal variable of variance
    // 4/256 sum x_j^2 s_j (s_j the noise power at sample j, hf_stair2()'s at twice the RMS rule's,
    // 1.5 2^-40), so given D in the region, the noise at sample j has the mean 2/16 x_j s_j / that
    // variance times D's mean there (det_trunc()), and the values as they are are never taken:
    // `s.2.dts`'s core 0.66% smaller; at the RMS rule's power 0.04% less, 4 times it 0.02%; where
    // the RMS lies outside the region alone 0.07% less. And the noise's variance left along x is
    // less, its share D's given the region: the boundary orders' spreads less that much of it,
    // hf_less_along() (0.02%). Subbands 28-30's the same, the noise 2^-44 of the power along their
    // FFT path, 0.03%, but the short streams 0.06% bigger; subband 28's by the bottom subband's
    // binade nothing)
    // (since format 88 subbands 28-30's the same, their noise 2^-44 of the power along their FFT
    // path, plev's, where the switch below, which the top subband's comparisons keep, takes the
    // values moved, the scores of the values taken alone: 0.04%; taken always, 0.03% with the short
    // streams 0.06% bigger; by their own comparisons 0.01% less, and slower)
    S64 xr[NSMP];
    bool resc = false, tg = false, tgo = false;  // (tgo: the values moved by the noise's mean)
    double tgv = 1;  // (D's variance's share left)
    if (bound[ch] && !ss_rule() && !x96a && nsmp == 16) {
      const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
      const int nt = sf_entries(r.shuff[ch]);
      if (si < nt) {
        S128 ms = 0, nz = 0;
        for (int j = 0; j < nsmp; j++) ms += (S128)x[j] * x[j];
        ms /= nsmp;
        if (b == NSUB - 1 && cpl_on(r, ch, b)) {
          S128 m0 = 0;
          for (int j = 0; j < nsmp; j++) m0 += (S128)hf_stair2(X[ch][0][j]);
          nz = m0 / nsmp * 3 >> 42;  // (3/4 of 2^-40 of its power, as that rule's: 0.03% smaller than 2^-40)
        }
        const U32 V = tab[si];  // (the region: a run's first's below its value, its last's from it on)
        int r0 = si, r1 = si;
        while (r0 > 0 && tab[r0 - 1] == V) r0--;
        while (r1 + 1 < nt && tab[r1 + 1] == V) r1++;
        const S64 vv = (S64)V << AQ_F;
        const S64 lo = si != r0 ? vv : r0 > 0 ? ((S64)tab[r0 - 1] + V) << (AQ_F - 1) : 0;
        const S64 hi = si != r1 ? vv : r1 + 1 < nt ? ((S64)V + tab[r1 + 1]) << (AQ_F - 1) : -1;
        const S64 rr = isqrt128((U128)(ms + nz));
        double sp[16], sv = 0;  // (in doubles, IEEE's arithmetic, in the samples' order)
        const bool t31 = b == NSUB - 1 && cpl_on(r, ch, b);
        if ((t31 || (b >= 28 && b < NSUB - 1)) && bind.up == 1 && ms > 0 && (hi > lo || (hi < 0 && lo > 0)))
          for (int j = 0; j < nsmp; j++) {
            if (t31) {
              sp[j] = ldexp((double)hf_stair2(X[ch][0][j]), -40) * 1.5;
            } else {
              const double l = ldexp((double)plev[ch][b][j], -22);
              sp[j] = l * l;
            }
            sv += (double)x[j] * (double)x[j] * sp[j];
          }
        if (sv > 0) {
          const double vd = sv * (4.0 / 256), sd = sqrt(vd), m = (double)(ms + nz);
          const double a = ((double)lo * (double)lo - m) / sd, e = hi < 0 ? 1e30 : ((double)hi * (double)hi - m) / sd;
          const double md = det_trunc(a, e, &tgv) * sd;
          resc = tgo = true, tg = t31;
          for (int j = 0; j < nsmp && resc; j++) {
            const double y = (double)x[j] * (1.0 + (2.0 / 16) * sp[j] * md / vd);
            if (y > 1e12 || y < -1e12) resc = false;  // (within 2^40: hf_dist()'s)
            else xr[j] = (S64)llround(y);
          }
          if (!resc) tg = tgo = false;
          if (!tg) tgv = 1;
        } else if (ms > 0 && hi > lo && (rr < lo || rr >= hi)) {
          const S64 t = rr < lo ? lo + (hi - lo) / 16 : hi - (hi - lo) / 16;
          const double t2 = (double)t * (double)t - (double)nz;
          if (t2 > 0) {
            const double a = sqrt(t2 / (double)ms);
            resc = true;
            for (int j = 0; j < nsmp && resc; j++) {
              const double y = (double)x[j] * a;
              if (y > 1e12 || y < -1e12) resc = false;  // (within 2^40: hf_dist()'s)
              else xr[j] = (S64)llround(y);
            }
          }
        }
      }
    }
    const bool ur = resc && (tg || hrs[1] > hrs[0] + 64 + hrs[0] / 32);
    S128 gm[32], go[32], md[32], en = 0;  // (the scores of the values taken, and of the others)
    int f0 = 0;
    // (a lossless WAV's top subband at the core's rate: the scores' squares weighed as its vectors'
    // distances are below, by T 2^27 times a quarter of the code book's unit, at a mean of 1:
    // `s.2.dts`'s core 0.18% smaller at 2^26; 2^25 0.4% bigger; 2^27 with the noise below, 2^26
    // 0.04% bigger, 2^28 0.03%, 2^25 0.7%)
    U32 gw[16], gs = 0;
    const bool gwo = b == NSUB - 1 && bound[ch] && !ss_rule() && !x96a && bind.up == 1 && nsmp == 16;
    // (and since format 87 subband 28's boundary orders, FFT bin 14's, by that noise too, the bottom
    // subband's binade's at a quarter of the top one's power (T twice as large): bin 14 shares the
    // FFT's first stage with bin 0, whose sums go by that value: `s.2.dts`'s core 0.04% smaller; at
    // the top one's power or 1/16 of it 0.02% less; the distances' and scores' weights nothing;
    // subbands 29 and 30's, odd bins', by the bottom subband's power nothing)
    const bool gwn = gwo || (b == 28 && bound[ch] && !ss_rule() && !x96a && bind.up == 1 && nsmp == 16);
    const int gt = gwo ? 0 : 2;  // (T^2's exponent over the top subband's)
    if (gwo) hf_weights(X[ch][0], sc, 74, gw, &gs);
    auto gscore = [&](int g, const S64* xx) {
      return gwo && gs ? hf_group_score_w(hgw, hge, g, xx, sc, gw, gs) : hf_group_score(hgw, hge, g, xx, nsmp, sc);
    };
    for (int g = 0; g < 32; g++) {
      const bool one = tgo && !tg;  // (subbands 28-30: the scores of the values taken alone)
      const S128 k0 = one && ur ? 0 : gscore(g, x), k1 = resc && !(one && !ur) ? gscore(g, xr) : k0;
      gm[g] = ur ? k1 : k0, go[g] = ur ? k0 : k1;
      f0 = gm[g] < gm[f0] ? g : f0;
    }
    if (ur) x = xr;
    int mn = 0;
    if (tried) {
      const S64* dot;
      const double* dd;
      const bool small = hf_dots(x, nsmp, &dot, &dd);
      for (int j = 0; j < nsmp; j++) en += (S128)x[j] * 16 * ((S128)x[j] * 16);  // (hf_dist()'s units)
      hf_group_mins(dot, small, nsmp, sc, en, md);
      S128 near = en + 1;  // (the nearest of all)
      for (int g = 0; g < 32; g++) near = md[g] < near ? md[g] : near;
      const S128 cap = 16 * near + 1;
      for (int g = 0; g < 32; g++) md[g] = md[g] < cap ? md[g] : cap;
      mn = near * 64 < en;
    }
    for (int g = 0; g < 32; g++) cand[g].k = mn ? md[g] : gm[g], cand[g].i = g;
    const int lk = cpl_leak(r, ch, b, 26, 15), gn = lk >= 0 ? lk : imin(si >> 3, 15);
    // (a lossless WAV's: the unit of the margins against the level, below; the groups' scores run
    // 2^19 times the vectors' distances: `s.2.dts`'s groups' within 2^-4 of that unit were the
    // first half the time, 98% from 2 times it; at the core's rate the level along the subband's
    // FFT path, plvl, rather than the channel's: `s.2.dts`'s core 0.1% smaller)
    S128 mu = bound[ch] && !ss_rule() && !x96a ? (S128)(bind.up == 1 ? plvl[ch][b] : tlvl[ch]) * sc << 24 : 0;
    // (the top subband's with its leak's noise in too: the root of its square and that of 2^46 times
    // the noise's RMS times the scale factor, 2^-2 of the RMS's unit, where it is the encoder's
    // values' noise more than the level's is: `s.2.dts`'s core 0.2% smaller; the bottom one's power
    // as hf_stair2() has it, 0.03% more)
    if (mu > 0 && b == NSUB - 1 && cpl_on(r, ch, b)) {
      S128 m0 = 0;
      for (int j = 0; j < nsmp; j++) m0 += (S128)hf_stair2(X[ch][0][j]);
      const double sl = sqrt((double)(m0 / nsmp >> 40)) * (double)sc * 70368744177664.0;  // (2^46)
      mu = (S128)sqrt((double)mu * (double)mu + sl * sl);
    }
    // (the boundary order, hf_bound_grp(): the learned scores' W to the power 1.5, as they have
    // been learned, rather than the boundary's 1: `s.2.dts`'s core 0.3% smaller; the centroids' to
    // the power 1)
    // (there too, the boundary order's distance between two groups' W the noise's spread across
    // their boundary, each value's squared difference weighed by its noise power, hf_noise()'s at T
    // 2^23 times a quarter of the code book's unit (2^23.5 before format 85, of x0 as it is), as the
    // vectors' below: `s.2.dts`'s core 0.3%
    // smaller; 2^24.5 0.12% bigger than that, 2^22.5 0.05%; to the power 1.5 as the W's, 1.25 or
    // 1.75 0.5% and 0.3% bigger)
    double rg[16];
    if (gwn) hf_noise(X[ch][0], sc, 66 + gt, rg);
    // (each group's boundary key the largest against the 8 nearest of those nearer, hf_bound_grp_m(),
    // the vectors' against 4; in every subband of 16 values, the others' by the distances unweighed:
    // `s.2.dts`'s core 0.03% smaller than in its top subband alone, the short streams 14 bytes)
    double ug[16], kg = 0;  // (the noise's variance left along x, above)
    if (gwo && tg && tgv < 1) {
      double sx = 0;
      for (int j = 0; j < 16; j++) ug[j] = (double)X[ch][b][j] * rg[j], sx += (double)X[ch][b][j] * ug[j];
      kg = sx > 0 ? (1 - tgv) / sx : 0;
    }
    auto bgrp = [&](const S128* k, S128* o, int* am) { hf_bound_grp_m(k, hgw, gwn ? rg : nullptr, hgd, 8, o, am, kg > 0 ? ug : nullptr, kg); };
    S128 og[32];
    int am[32];
    for (int i = 0; i < 32; i++) am[i] = f0;
    if (!mn) {
      if (nsmp == 16) bgrp(gm, og, am);
      else hf_bound_grp(gm, &HF_GC[0][0], 32, nsmp, false, og);
    }
    double mpg[32];  // (a lossless WAV's: the groups' probabilities by hf_mc(), above)
    int mk = 0;
    const bool m28 = b >= 28 && b < NSUB - 1 && bound[ch] && !ss_rule() && !x96a && bind.up == 1 && nsmp == 16;
    if (!mn && ((gwo && cpl_on(r, ch, b)) || m28) && sp_on()) {
      const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
      const int nt = sf_entries(r.shuff[ch]);
      if (si < nt) {
        const U32 V = tab[si];
        int r0 = si, r1 = si;
        while (r0 > 0 && tab[r0 - 1] == V) r0--;
        while (r1 + 1 < nt && tab[r1 + 1] == V) r1++;
        const S64 vv = (S64)V << AQ_F;
        const S64 lo = si != r0 ? vv : r0 > 0 ? ((S64)tab[r0 - 1] + V) << (AQ_F - 1) : 0;
        const S64 hi = si != r1 ? vv : r1 + 1 < nt ? ((S64)V + tab[r1 + 1]) << (AQ_F - 1) : -1;
        double sg[16];
        for (int j = 0; j < 16; j++)
          sg[j] = m28 ? ldexp((double)plev[ch][b][j], -20) * 0.7071067811865476 : sqrt(ldexp((double)hf_stair2(X[ch][0][j]), -40) * 3.0);
        mk = hf_mc(X[ch][b], sg, (double)lo * (double)lo, hi < 0 ? 1e300 : (double)hi * (double)hi, sc);
        int cg[32] = {0};
        for (int t = 0; t < mk; t++) cg[mcg[t]]++;
        const double al = MC_A * mk / MC_N;
        for (int q = 0; q < 32; q++) mpg[q] = (cg[q] + al) / (mk + 32 * al);
      }
    }
    const int g = code_cand(32, 5, (int)v >> 5, VK_HFG, ch, b, gn, 0, mu << 19, mn ? nullptr : og, mk ? mpg : nullptr);
    if (resc && !mn && !(tgo && !tg)) {  // (which ranked the encoder's group higher, in the boundary orders)
      int rc = 0, ro = 0;
      S128 oo[32];
      bgrp(go, oo, nullptr);
      for (int k = 0; k < 32; k++) rc += og[k] < og[g], ro += oo[k] < oo[g];
      const int rs = ur ? rc : ro, rp = ur ? ro : rc;
      if (rs != rp) hrs[rs < rp]++;
      if (hrs[0] + hrs[1] >= 1u << 16) hrs[0] >>= 1, hrs[1] >>= 1;
    }
    if (tried) {
      hfr[0] += mn, hfr[1]++;
      if (hfr[1] >= 1u << 16) hfr[0] >>= 1, hfr[1] >>= 1;
      hf_n += hf_n < 2048;
    }
    n_hfvq++;
    // (a lossless WAV's, where the group isn't the scores' first: the encoder's values lay past the
    // scores' boundary between the two, so the vectors go by the values moved onto it along the
    // scores' gradient where that has ranked the encoder's vector higher clearly more often than the
    // values as they are: `s.2.dts`'s top subband's vector the nearest 63.1% of the time there from
    // 62.2%, its core 0.1% smaller; FFmpeg's `master_audio_7.1_24bit.dts`'s and `3-1.dts`'s less)
    // (the top subband's, as above, onto the boundary's point the likeliest under the noise: each
    // value moved by its noise power times the gradient's, hf_noise()'s at T 2^19.5 times a quarter
    // of the code book's unit, `s.2.dts`'s core 0.17% smaller; from 2^17 to 2^22 within 0.02%. Since
    // format 85 onto its boundary with the group its largest key is against, hf_bound_grp_m()'s,
    // the one that binds, rather than with the first: 0.02%)
    // (since format 87 the top subband's, at the core's rate, by the mean of the noise given that
    // the encoder's group won: a normal variable along the gradient past the boundary, as above (the
    // values' noise sc 2^10 in these units), its mean there, det_trunc(), rather than the boundary
    // itself: `s.2.dts`'s core 0.11% smaller; sc 2^8.5 or 2^11.5 0.04-0.06% less. And where the
    // group was the scores' first too, given it won against the next: 0.06%)
    S64 xs[NSMP];
    bool pushed = false;
    int f1 = -1;  // (the next, where the group was the first)
    if (gwo && !mn && nsmp == 16 && g == f0)
      for (int q = 0; q < 32; q++)
        if (q != g && (f1 < 0 || gm[q] < gm[f1])) f1 = q;
    if (!mn && nsmp == 16 && (g != f0 || f1 >= 0)) {
      const int fk = g == f0 ? f1 : am[g] != g ? am[g] : f0;
      double g2 = 0, gx[16], rp[16];
      for (int j = 0; j < 16; j++) rp[j] = 1;
      if (gwn) hf_noise(X[ch][0], sc, 59 + gt, rp);
      for (int j = 0; j < 16; j++) gx[j] = 2.0 * (double)(hgw[fk][j] - hgw[g][j]) * (double)sc * 65536.0, g2 += gx[j] * gx[j] * rp[j];
      if (g2 > 0) {
        double lam = (double)(gm[g] - gm[fk]) / g2 / 8192.0;  // (hf_group_score()'s units: x 8192)
        if (gwo) {
          double vq;
          const double tau = ldexp((double)sc, 10), sg = sqrt(g2);
          lam = det_trunc(lam * sg / tau, 1e30, &vq) * tau / sg;
        }
        pushed = true;
        for (int j = 0; j < 16 && pushed; j++) {
          const double d = lam * gx[j] * rp[j];
          if (d > 1e12 || d < -1e12) pushed = false;  // (the values within 2^40: hf_dist()'s)
          else xs[j] = x[j] - (S64)llround(d);
        }
        // (since format 88 the top subband's, then given that the group won against the one the
        // scores of the values pushed rank first of the rest too, the same way: 0.05%; against
        // two more, or the first again, less)
        if (gwo && pushed) {
          S128 s2[32];
          int f2 = -1;
          for (int q = 0; q < 32; q++) {
            s2[q] = gscore(q, xs);
            if (q != g && q != fk && (f2 < 0 || s2[q] < s2[f2])) f2 = q;
          }
          double h2 = 0, hx[16];
          for (int j = 0; j < 16; j++) hx[j] = 2.0 * (double)(hgw[f2][j] - hgw[g][j]) * (double)sc * 65536.0, h2 += hx[j] * hx[j] * rp[j];
          if (h2 > 0) {
            double vq;
            const double tau = ldexp((double)sc, 10), sh = sqrt(h2);
            const double mu = det_trunc((double)(s2[g] - s2[f2]) / h2 / 8192.0 * sh / tau, 1e30, &vq) * tau / sh;
            S64 xt[16];
            bool ok = true;
            for (int j = 0; j < 16 && ok; j++) {
              const double dd = mu * hx[j] * rp[j];
              if (dd > 1e12 || dd < -1e12) ok = false;
              else xt[j] = xs[j] - (S64)llround(dd);
            }
            if (ok)
              for (int j = 0; j < 16; j++) xs[j] = xt[j];
          }
        }
      }
    }
    const bool up = pushed && hpw[1] > hpw[0] + 16 + hpw[0] / 32;
    KeyIx oc[32], cc[32];  // (the other order's boundary keys; the order's, as code_cand() may reorder cand)
    S128 kv[32], ko[32], ov[32], oo[32];
    S128 d0[32], d1[32];
    // (a lossless WAV's top subband at the core's rate: each value's distance weighed against the
    // encoder's noise there, its FFT's bin 0's, which goes with the bottom subband's value at the
    // sample: by T^2 / (T^2 + x0^2), T 2^26.5 times a quarter of the code book's unit (an entry's
    // step times the scale factor): `s.2.dts`'s core 0.28% smaller at 2^25.5; by the total level or
    // the path's about the same; 2^25 0.03% smaller, 2^24 2.4% bigger; 2^26.5 with the noise in
    // the boundary order below, 2^25.5 0.06% bigger, 2^24.5 1.4%, 2^27.5 0.07%. And the boundary
    // order's distance between two entries the noise's spread across their boundary: the encoder's
    // values are these and the noise, so its choice flips from the first to another where the noise
    // along the two entries' difference crosses their boundary, its spread the root of their squared
    // differences weighed by each value's noise power, hf_noise()'s at T 2^22.5 times that unit:
    // `s.2.dts`'s core 0.2% smaller; 2^24 0.05% bigger than that, 2^22 0.02%; the distances
    // between the entries weighed by T^2 / (T^2 + x0^2) instead 0.07% bigger than none; to the
    // power 1.25 or 0.75 0.1% bigger; the boundary keys of the distances unweighed, from their own
    // first, 0.2% bigger)
    double rv[16];
    if (gwn) hf_noise(X[ch][0], sc, 65 + gt, rv);
    if (gwo) {
      hf_dists_w(g, x, sc, X[ch][0], d0);
      if (pushed) hf_dists_w(g, xs, sc, X[ch][0], d1);
    } else {
      hf_dists(g, x, nsmp, sc, d0);
      if (pushed) hf_dists(g, xs, nsmp, sc, d1);
    }
    for (int i = 0; i < 32; i++) {
      const S128 k0 = d0[i], k1 = pushed ? d1[i] : k0;
      cand[i].k = kv[i] = up ? k1 : k0, cand[i].i = i;
      ko[i] = up ? k0 : k1;
    }
    if (nsmp == 16 && !gwn && !(hpd_on >> g & 1)) hf_pair_dists(g, hpd[g]), hpd_on |= 1u << g;
    double uv[16], kq = 0;  // (the noise's variance left along x, above)
    if (gwo && tg && tgv < 1) {
      double sx = 0;
      for (int j = 0; j < 16; j++) uv[j] = (double)X[ch][b][j] * rv[j], sx += (double)X[ch][b][j] * uv[j];
      kq = sx > 0 ? (1 - tgv) / sx : 0;
    }
    if (nsmp == 16) hf_bound_vec_m(kv, g, gwn ? rv : nullptr, hpd[g], 4, ov, kq > 0 ? uv : nullptr, kq);
    else hf_bound_vec(kv, g, nsmp, ov);
    if (pushed) {
      if (nsmp == 16) hf_bound_vec_m(ko, g, gwn ? rv : nullptr, hpd[g], 4, oo, kq > 0 ? uv : nullptr, kq);
      else hf_bound_vec(ko, g, nsmp, oo);
      for (int i = 0; i < 32; i++) cc[i].k = ov[i], cc[i].i = i, oc[i].k = oo[i], oc[i].i = i;
    }
    // (a lossless WAV's vectors: how far the nearest beats the next against the channel's level in
    // the subframe times the scale factor, as the encoder's arithmetic errs by its level: `s.2.dts`'s
    // encoder took the nearest of subband 28's 97% of the time within 2^-19 of that, 85% within
    // 2^-20, 49% within 2^-24; by the nearest's distance, 63% within 1/256 of it, 99% by 2^4 times
    // it or more)
    double mpv[32];
    if (mk) {  // (the entry's: of the samples of the encoder's group, or where fewer than MC_NG, of all)
      int ce[32] = {0}, ng = 0;
      for (int t = 0; t < mk; t++)
        if (mcg[t] == g) ce[mce[t]]++, ng++;
      if (ng < MC_NG) {
        ng = mk;
        for (int t = 0; t < mk; t++)
          if (mcg[t] != g) ce[hf_near(g, mcy[t], sc)]++;
      }
      const double al = MC_A * ng / MC_N;
      for (int i = 0; i < 32; i++) mpv[i] = (ce[i] + al) / (ng + 32 * al);
    }
    v = g * 32 + code_cand(32, 5, (int)v & 31, VK_HFV, ch, b, gn, mu, mu, ov, mk ? mpv : nullptr);
    if (pushed) {  // (which order ranked the encoder's vector higher, in the boundary orders)
      const int at = (int)v & 31, rc = rank_of(cc, 32, at), ro = rank_of(oc, 32, at);
      const int rp = up ? rc : ro, r0 = up ? ro : rc;
      if (rp != r0) hpw[rp < r0]++;
      if (hpw[0] + hpw[1] >= 1u << 16) hpw[0] >>= 1, hpw[1] >>= 1;
    }
    if (cpl_on(r, ch, b)) {  // (each m's group by the centroids)
      const int c = NSUB - 1 - b;
      for (int k = 0; k < CPL_K; k++) {
        S64 y[NSMP];
        cpl_values(c, ch, k, y, nsmp);
        int gb = 0;
        S128 db = hf_group_score(hgw, hge, 0, y, nsmp, sc);
        for (int gg = 1; gg < 32; gg++) {
          const S128 d = hf_group_score(hgw, hge, gg, y, nsmp, sc);
          if (d < db) db = d, gb = gg;
        }
        cpl_hit[c][k] += gb == (int)(v >> 5);
      }
    }
    if (!mn && nsmp == 16) {  // (the scores, where they ranked the groups)
#ifdef DTSD_HFDUMP
      if (ENC) hf_dump(x, sc, (int)v);
#endif
      // (not where the bottom subband's leak, 2^32 times as loud or more, swamps the top ones': their
      // groups go by its noise as much as by the encoder's scores; `s.2.dts`'s core 0.3% smaller)
      if (f0 != (int)(v >> 5) && lk < 3) hg_learn(x, sc, (int)(v >> 5), f0);
    }
    return v;
  }

  // --- the scale factors -----------------------------------------------------------------------------

  // The scale factor a decoded WAV says (the scale factor search): its values v (n of them, the
  // ADPCM prediction off) are what the decoder made of the indexes, or of a VQ vector (hfb), at the
  // step and scale factor; so of the entries from p - 3 to p + 5 (a VQ subband's to p + 12; where
  // low_on(), from p - 16), and past the window's top the two either side of the peak's
  // (peak_scale()) or the three about the vector's least-squares scale (vq_scale(); since format
  // 146: the 7-bit table's are 13-25 entries above the RMS rule's, `dtswavsample16.wav`'s a
  // quarter of the time, its core 69 KB to 18 KB), the one whose reconstruction lies nearest them,
  // where nearer by 4 times the next best's error at least (else -1; a lossless WAV's values are the
  // encoder's input: none is; a VQ subband's all the same, *conf -1); *conf the log2 of that ratio
  // less 2, to 7
  int scale_search(const CoreRec& r, const CoreSub& s, int ch, int b, int hfb, const S64* v, int n, int p, int* conf) {
    const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
    const int nt = sf_entries(r.shuff[ch]), a = s.abits[ch][b];
    S64 lo = 0, hi = 0;
    int vq = 0;
    if (!hfb) {
      quant_range(r, ch, a, lo, hi);
    } else {  // (the vector: the two-stage search's at the RMS's scale factor and 3 above, and the
              // entry most nearly a multiple of the values at its best scale factor, the nearest of
              // those: a decoded WAV's is the vector, its scale factor 1-12 above the RMS rule's)
      S128 bd = -1;
      for (int k = 0; k < 2; k++) {
        const S64 sc = tab[clip(p + 3 * k, 0, nt - 1)];
        S128 dd[32];
        hf_group_dists(v, n, sc, dd);
        for (int g = 0; g < 32; g++) cand[g].k = dd[g], cand[g].i = g;
        const int g = select_rank(cand, 32, 0);
        hf_dists(g, v, n, sc, dd);
        for (int e = 0; e < 32; e++) cand[e].k = dd[e], cand[e].i = e;
        const int q2 = g * 32 + select_rank(cand, 32, 0);
        const S128 d = vq_err(v, n, q2, sc);
        if (bd < 0 || d < bd) bd = d, vq = q2;
      }
      {  // (the entry most nearly a multiple of the values, at its best scale factor)
        const int qs = hf_shape(v, n), c = vq_scale(v, n, qs, tab, nt);
        for (int i = imax(p - 3, 0); i <= imin(p + 12, nt - 1); i++) {
          const S128 d = vq_err(v, n, qs, tab[i]);
          if (d < bd) bd = d, vq = qs;
        }
        for (int i = c - 1; c >= 0 && i <= c + 1; i++)  // (and about its least-squares scale, past them)
          if (i > p + 12 && i < nt) {
            const S128 d = vq_err(v, n, qs, tab[i]);
            if (d < bd) bd = d, vq = qs;
          }
      }
    }
    S128 e1 = -1, e2 = -1;
    int best = -1;
    auto cand = [&](int i) {
      S128 e;
      if (hfb) {
        e = vq_err(v, n, vq, tab[i]);
      } else {
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0) return;
        e = quant_err(v, n, ss, lo, hi);
      }
      if (best < 0 || e < e1) e2 = e1, e1 = e, best = i;
      else if (e2 < 0 || e < e2) e2 = e;
    };
    const int top = imin(p + (hfb ? 12 : 5), nt - 1);
    for (int i = imax(p - (hfb || !low_on() ? 3 : 16), 0); i <= top; i++) cand(i);
    const int c = hfb ? vq_scale(v, n, vq, tab, nt) : peak_scale(r, ch, a, lo, hi, v, n, top);
    for (int i = c - 1; c >= 0 && i <= c + hfb; i++)  // (past the window: the vector's three, the peak's two)
      if (i > top && i < nt) cand(i);
    if (best < 0 || e2 < 0) return -1;
    const S128 eps = (S128)n << (2 * AQ_F - 6);  // (an eighth of a unit a value)
    const S128 ratio = (e2 + eps) / (e1 + eps);
    if (ratio < 4) return hfb ? (*conf = -1, best) : -1;  // (the RMS's is rarely a decoded VQ subband's)
    *conf = imin(bitlen128((U128)ratio) - 3, SB_N - 1);
    return best;
  }
  // whether to try the search: in its first 2048 tries, while its rule is the one taken, and in 1
  // of 64 scale factors (it costs, and a lossless WAV's gains nothing); the rule: the search's
  // where it is right clearly more often than the better of the others
  int sf_base() const { return sfr[1] > sfr[0] + 64 + sfr[0] / 8 ? 1 : 0; }
  // (by a 32nd, the search's: on a lossless WAV it tracks the better rule, on a decoded one it is
  // ahead by a fifth or more within two frames. Since format 119, and the decoded WAVs' families
  // (abd, scd) with it, by a 16th in format 118; before, by 64 more and an 8th, which FATE's
  // 7-frame decoded samples never reached: the short streams 0.47% smaller, the decoded WAVs'
  // 0.69%, the lossless ones' the same; by any lead, FATE's x96_xch sample, whose search is no
  // better than the RMS rule, 32 bytes bigger). Until format 149 the search's count grew where it was
  // tried and the rules' on every scale factor: once off past its first 2048 tries, tried on 1 in
  // 64, it never came on again; and where it found none (no scale factor 4 times nearer than the
  // next) it was counted as the rule's. Since then they are counted where it found one, and only where the
  // two differ: `padded.dts` (FFmpeg's samples; 5.1 at 1509 kbps, under -66 dBFS for its first 340
  // frames and -40 after), whose search, unsure on its quiet start, was counted there as the RMS
  // rule's against scale_ceil()'s, which won, was taken from its 47th frame on, from never: its core
  // 418846 bytes to 167352, 60% smaller (counted where tried, 186653; where it differs but its none
  // as the rule's, 181522)
  bool ss_rule() const {
    const int k = sf_base();
    return sfq[k][1] > sfq[k][0] + sfq[k][0] / 32;
  }
  // whether the search goes down to 16 under the RMS rule's: where after 256 tries more than an
  // eighth of the scale factors have lain under its window. FFmpeg's encoder takes, at its fine
  // steps, the least scale factor whose range holds the subband's peak (dcaenc.c's
  // calc_one_scale()), 3-14 entries under the RMS rule's: its streams' scale factors then 76-99%
  // the prediction from 33-71%, their cores 18-51% smaller; every stream's so, the DTS encoders'
  // decodes found finer lattices than theirs as near (`ES 6.1 16bit.dts`'s core 11% bigger)
  bool low_on() const { return sfl[1] >= 256 && sfl[0] * 8 > sfl[1]; }
  bool search_on() const { return ss_n < 2048 || ss_rule() || !(n_sf & 63); }
  // the same of the bit allocation search, of the ABITS: in its first 512 (the 7-frame samples' the
  // same, 4-8% faster), and while it has been right clearly more often than the first guess without
  // it (a lossless WAV's search, which rarely finds one, as often: then it ran on, for nothing, its
  // streams the same and 7.6% faster); since format 148, of those first 512 only every 8th where it
  // has been right 24 times fewer than the first guess (FATE's 7-frame samples', whose first
  // guesses win from their first frames, and the lossless WAVs': the decoded ones' searches lead
  // from their first tries; FATE's XCH sample 4% faster, the sets' sizes the same to a few bytes);
  // since format 150 wherever the scale factor search's rule holds too, its ABITS the first guess
  // or the second wherever it runs (`padded.dts`'s, not clearly more often right than the first
  // guess it is counted against, ran on 36% of its ABITS: its core 10.5% smaller, as fast)
  // (the rule: by 16 and a 32nd since format 119, by 32 before: the decoded WAVs' streams but
  // FATE's 1-7 bytes smaller; by 8 and a 64th, a lossless WAV's too, `s.2.dts`'s first 3 MB's 5
  // bytes bigger)
  bool ab_rule() const { return abr[1] > abr[0] + 16 + abr[0] / 32; }
  // whether the families of a decoded WAV's (sdd, hid, smd, vpd) take the decisions: where either
  // search has been right clearly more often (abd's rule), and at -9 where after a 5.1 frame's
  // indexes under 90% of them have been the audio's (the decodes of FATE's XBR and XXCH samples,
  // whose searches fail too: 82.9-85% the audio's; `x96_xch_61_24_96_3840.dtsc` 23 bytes bigger with
  // the others' families, at 95% the lossless WAVs' noisy first frames theirs; at -8 the 8-frame cut
  // of the Cymatics stream, 75.5%, 20 bytes bigger)
  bool dw_rule() const {
    return ss_rule() || ab_rule() || (LV::id == 9 && n_smp >= SP_HN && n_hit * 100 < n_smp * 90);
  }
  // sid's side information, sdd's where the WAV is decoded (the joint scale factors jsc's either way)
  S64 side_code(int fld, int ch, int b, S64 v, S64 lo, S64 hi, int bit1, S32 A, S32 P, int ac, int ab, int ec, int tr = 0) {
    if (fld == SF_JSCALE || !dw_rule()) return B::side_code(fld, ch, b, v, lo, hi, bit1, A, P, ac, ab, ec, tr);
    return B::template side_val<M_sdd_v>(B::f_sdd, fld, ch, b, v, lo, hi, bit1, A, P, ac, ab, ec, tr);
  }
  S64 side_index(int fld, int ch, int b, S64 v, int nbits, S32 A, int ac, int ab) {
    if (!dw_rule()) return B::side_index(fld, ch, b, v, nbits, A, ac, ab);
    return B::template side_ix<M_sdd_v>(B::f_sdd, fld, ch, b, v, nbits, A, ac, ab);
  }
  bool ab_search_on() const {
    if (ss_rule() || abr[1] > abr[0] + 32 + abr[0] / 64) return true;
    if (ab_n < 512) return abr[0] < abr[1] + 24 || !(n_ab & 7);
    return !(n_ab & 63);
  }
  bool hf_min_on() const { return hf_n < 2048 || hfr[0] * 8 > hfr[1] || !(n_hfvq & 63); }

  // The ABITS a decoded WAV says (the bit allocation search): its values v (n of them, the ADPCM
  // prediction off) are what the decoder made of indexes at the step of the ABITS and a scale
  // factor; so of ABITS a0..a1 (g the guess) and the scale factors p - 3 to p + 5, the pair whose
  // lattice lies nearest them in its steps (a finer one fits as nearly but in more of its steps, a
  // coarser one misses the values between its points), of pairs as near the coarser lattice, then
  // the scale factor nearer p (where the values all lie on both, as a 3-level quantizer's do on the
  // 7-level one's at the same scale factor: `ES 6.1 16bit.dts`'s search right 99.23% of the time
  // from 97.90%, its core 2.1% smaller): 1 + its ABITS, where an index it makes isn't 0 (else 0:
  // the values say nothing); *conf the log2 of the next pair's distance over its, 0-7 (a pair as
  // near isn't counted), *dbest its distance (norm_dist()'s), *ibest its scale factor. And where
  // none fits within 3 eps (but where FFmpeg's encoder is made again: it takes no transients), a
  // transient's: of the ABITS a0..a1, the one whose lattice fits the subsubframes before some one
  // and those from it on, each part at a scale factor of its own (part_fit()), the nearest, where 4
  // times nearer than the pair at least (*dbest still the pair's: `ES 6.1 16bit.dts`'s search right
  // 99.71%, its core 12.8% smaller still, FATE's `dts_es.dts`'s 9.2%, `open bitrate.dts`'s 10.0%).
  // Until format 147 that only where none fit within a 64th of a step squared a value, as the
  // transient search did until format 146: but a transient's quiet first part lies near 0 on the
  // second's lattice, and one lattice fits its values that well (the DTS CD's sample's search right
  // in 97.8% of its ABITS from 90.6%, its bit allocations 2.2 KB to 0.4; `dtswavsample14.wav`'s,
  // right too seldom before to be taken, 15.3 KB to 2.1, its PMODEs 1.3 KB to 0.5). And a split
  // where the pair fits to that 64th yields to a coarse lattice's one scale factor (below) as near
  // as it, by the cut at its distance and eps (`dtswavsample16.wav`'s encoder takes no transients:
  // without it, its bit allocations 3.7% bigger). (Only the pairs under cut: 0 where none is, and
  // no transients.) Since format 146, but where FFmpeg's encoder is made again (its ABITS its own),
  // and not after another transient's: the coarse lattices under a0..a1 by themselves, ABITS 1 to
  // AB_COARSE, by the cut at the range pair's distance and eps (as near as it, or nearer): theirs
  // taken where 4 times nearer, and where as near, the one of the two that has been the ABITS more
  // often (abk, by an eighth and 2: a 3-level quantizer's values, 0 and a step either side, lie as
  // well on the lattices of more levels at a finer step, and which of them an encoder takes is its
  // own: `dtswavsample16.wav`'s the coarse one, `ES 6.1 16bit.dts`'s the range's). Since format 148,
  // where the range's pair is within 3 eps and no coarse lattice as near, the ABITS between those
  // and the range the same way, their ties learned with the coarse ones' (a subband whose signal
  // falls takes the quantizer of fewer levels its values lie on, under the window as often as in
  // it: 72% of `dtswavsample16.wav`'s misses, its core 14% smaller). Then where the pair isn't
  // within 3 eps, the other ABITS of af0..af1 by the cut at 3 eps, theirs taken where 4 times
  // nearer (a subband's ABITS leaps from about its last where its signal does:
  // `dtswavsample16.wav`'s bit allocations 98 KB to 15 KB, the DTS CD's 2.6 KB to 2.2; every ABITS
  // in the first pass instead, its window's guess lost, the DTS CD's 4.3 KB); since format 148
  // while a quarter of the range's pairs at least have been within 3 eps lately (ab_ex: FATE's
  // 7-frame samples' decodes, noise near full scale and clipped, 12% of theirs, and the pass, which
  // found none of theirs, cost them 4% of the time; `dtswavsample16.wav` 8 bytes bigger).
  // (The pairs by their distance at the 2 largest values, each one's whole distance, the largest
  // values first, while it could still be less than the second best's: as trying them all does.)
  // a pair of abits_pass()'s: its step and range, its ABITS and scale factor, and a bound under its
  // distance (norm_dist()'s of its exact squared distance), lb (ab_bounds())
  struct AbPair {
    S64 ss, lo, hi;
    double lb;
    S8 a, i;
  };
  // The pairs pr's (np of them) distances at the values w (n of them) in doubles: each value's index
  // as QErr::err() finds it, floor(|64 w| / ss + 1/2), exact in doubles but within 1e-7 of a half
  // (where the estimate's rounding could decide: then the pair's indexes by QErr's exact rule), its
  // point as dequantize() makes it (exact), the squares summed (each within 2^-53 of it) and normed:
  // so within 2^-47 of the exact distance before norm_dist()'s floor, lb 1e-12 under that less 2.
  static void ab_bounds(const S64* w, int n, AbPair* pr, int np) {
    alignas(32) double ua[NSMP + 4], sg[NSMP + 4], vd[NSMP + 4];
    const int n4 = (n + 3) & ~3;
    for (int j = 0; j < n4; j++) {
      const S64 v = j < n ? w[j] : 0;
      ua[j] = (double)(v < 0 ? -v : v) * 64, sg[j] = v < 0 ? -0.0 : 0.0, vd[j] = (double)v;
    }
    for (int k = 0; k < np; k++) {
      AbPair& x = pr[k];
      int shift = 0;
      S64 ssh = x.ss;
      if (x.ss > (1 << 23)) shift = ilog2((U64)(x.ss >> 23)) + 1, ssh = x.ss >> shift;
      const int sh = 22 - shift;
      const double inv = 1.0 / (double)x.ss, dh = sh > 0 ? (double)((U64)1 << (sh - 1)) : 0.0,
                   dsc = sh > 0 ? 1.0 / (double)((U64)1 << sh) : 1.0, dssh = (double)ssh, dlo = (double)x.lo, dhi = (double)x.hi;
      double e = 0;
      bool tie = false;
#ifdef DTSD_AVX2
      const __m256d vinv = _mm256_set1_pd(inv), half = _mm256_set1_pd(0.5), tl = _mm256_set1_pd(1e-7), th = _mm256_set1_pd(1 - 1e-7),
                    big = _mm256_set1_pd(16777216.0), vlo = _mm256_set1_pd(dlo), vhi = _mm256_set1_pd(dhi),
                    vssh = _mm256_set1_pd(dssh), vdh = _mm256_set1_pd(dh), vdsc = _mm256_set1_pd(dsc),
                    c23lo = _mm256_set1_pd(-8388608.0), c23hi = _mm256_set1_pd(8388607.0), f16 = _mm256_set1_pd(65536.0);
      __m256d acc = _mm256_setzero_pd(), tm = _mm256_setzero_pd();
      for (int j = 0; j < n4; j += 4) {
        const __m256d tq = _mm256_add_pd(_mm256_mul_pd(_mm256_load_pd(ua + j), vinv), half);
        const __m256d qa = _mm256_floor_pd(tq), fr = _mm256_sub_pd(tq, qa);
        tm = _mm256_or_pd(tm, _mm256_and_pd(_mm256_cmp_pd(tq, big, _CMP_LT_OQ),
                                            _mm256_or_pd(_mm256_cmp_pd(fr, tl, _CMP_LT_OQ), _mm256_cmp_pd(fr, th, _CMP_GT_OQ))));
        const __m256d qv = _mm256_min_pd(_mm256_max_pd(_mm256_xor_pd(qa, _mm256_load_pd(sg + j)), vlo), vhi);
        __m256d y = _mm256_mul_pd(qv, vssh);
        if (sh > 0) y = _mm256_floor_pd(_mm256_mul_pd(_mm256_add_pd(y, vdh), vdsc));
        y = _mm256_min_pd(_mm256_max_pd(y, c23lo), c23hi);
        const __m256d d = _mm256_sub_pd(_mm256_load_pd(vd + j), _mm256_mul_pd(y, f16));
        acc = _mm256_add_pd(acc, _mm256_mul_pd(d, d));
      }
      alignas(32) double a4[4];
      _mm256_store_pd(a4, acc);
      e = (a4[0] + a4[1]) + (a4[2] + a4[3]);
      tie = _mm256_movemask_pd(tm) != 0;
#else
      for (int j = 0; j < n; j++) {
        const double tq = ua[j] * inv + 0.5, qa = floor(tq), fr = tq - qa;
        if (tq < 16777216.0 && (fr < 1e-7 || fr > 1 - 1e-7)) tie = true;
        double qv = sg[j] != 0 || signbit(sg[j]) ? -qa : qa;
        qv = qv < dlo ? dlo : qv > dhi ? dhi : qv;
        double y = qv * dssh;
        if (sh > 0) y = floor((y + dh) * dsc);
        y = y < -8388608.0 ? -8388608.0 : y > 8388607.0 ? 8388607.0 : y;
        const double d = vd[j] - y * 65536.0;
        e += d * d;
      }
#endif
      if (tie) {  // (the indexes exact, as QErr::err() finds them; the squares in doubles still)
        QErr q;
        int nz = 0;
        q.set(x.ss, x.lo, x.hi);
        e = 0;
        for (int j = 0; j < n; j++) {
          const double d = (double)q.dev(w[j], &nz);
          e += d * d;
        }
      }
      x.lb = e * 268435456.0 / ((double)x.ss * (double)x.ss) * (1 - 1e-12) - 2;
    }
  }
  // The cut's search, where nearly always no pair fits (a lossless WAV's predictors, and a decoded
  // one's but the decoder's): whether one could, its error sum within lim (the heap's, d2 cut, while
  // none has fitted: those popped then are all of them, each failing at its lim or fitting), its
  // values' squared distances in doubles, each index as ab_bounds() finds it, 4 pairs at a time;
  // false only where each pair's sum is past its lim by more than the doubles' rounding ((n + 1)
  // 2^-53 of it at most; lim at most l + 1) before any index of it lies within 1e-7 of a rounding
  // boundary (there the estimate could be off by 1: then true, as the exact search finds)
  bool ab_cut_any(const CoreRec& r, int ch, const S64* v, int n, int a0, int a1, int p, U64 cut) const {
    constexpr int NP = 27 * 9;
    alignas(32) double inv[NP + 3], ssh[NP + 3], dh[NP + 3], dsc[NP + 3], plo[NP + 3], phi[NP + 3], lim[NP + 3];
    const int nt = sf_entries(r.shuff[ch]), i0 = imax(p - 3, 0), i1 = imin(p + 5, nt - 1);
    int m = 0;
    for (int a = a0; a <= a1; a++) {
      S64 lo, hi;
      quant_range(r, ch, a, lo, hi);
      for (int i = i0; i <= i1; i++) {
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0) continue;
        const double l = (double)cut * (double)ss * (double)ss / 268435456.0 * (1 + 1e-9);
        if (!(l < 1e36) || ss >= (S64)1 << 48) return true;
        int shift = 0;
        S64 sh_ss = ss;
        if (ss > (1 << 23)) shift = ilog2((U64)(ss >> 23)) + 1, sh_ss = ss >> shift;
        const int sh = 22 - shift;
        inv[m] = 1.0 / (double)ss, ssh[m] = (double)sh_ss, dh[m] = sh > 0 ? (double)((U64)1 << (sh - 1)) : 0.0;
        dsc[m] = sh > 0 ? 1.0 / (double)((U64)1 << sh) : 1.0, plo[m] = (double)lo, phi[m] = (double)hi;
        lim[m] = (l + 1) * (1 + 1e-12);
        m++;
      }
    }
    alignas(32) double ua[NSMP], sg[NSMP], vd[NSMP];
    for (int j = 0; j < n; j++) {
      const S64 x = v[j], ax = x < 0 ? -x : x;
      if (ax >= (S64)1 << 44) return true;  // (the doubles exact within 2^53)
      ua[j] = (double)ax * 64, sg[j] = x < 0 ? -0.0 : 0.0, vd[j] = (double)x;
    }
#ifdef DTSD_AVX2
    for (; m & 3; m++) inv[m] = 0, ssh[m] = 0, dh[m] = 0, dsc[m] = 1, plo[m] = 0, phi[m] = 0, lim[m] = -1;  // (past at once)
    const __m256d half = _mm256_set1_pd(0.5), tl = _mm256_set1_pd(1e-7), th = _mm256_set1_pd(1 - 1e-7),
                  big = _mm256_set1_pd(16777216.0), c23lo = _mm256_set1_pd(-8388608.0), c23hi = _mm256_set1_pd(8388607.0),
                  f16 = _mm256_set1_pd(65536.0);
    for (int k = 0; k < m; k += 4) {
      const __m256d vinv = _mm256_load_pd(inv + k), vssh = _mm256_load_pd(ssh + k), vdh = _mm256_load_pd(dh + k),
                    vdsc = _mm256_load_pd(dsc + k), vlo = _mm256_load_pd(plo + k), vhi = _mm256_load_pd(phi + k),
                    vlim = _mm256_load_pd(lim + k);
      __m256d acc = _mm256_setzero_pd();
      int past = _mm256_movemask_pd(_mm256_cmp_pd(acc, vlim, _CMP_GT_OQ));
      for (int j = 0; j < n && past != 15; j++) {
        const __m256d tq = _mm256_add_pd(_mm256_mul_pd(_mm256_broadcast_sd(ua + j), vinv), half);
        const __m256d qa = _mm256_floor_pd(tq), fr = _mm256_sub_pd(tq, qa);
        const __m256d tie = _mm256_and_pd(_mm256_cmp_pd(tq, big, _CMP_LT_OQ),
                                          _mm256_or_pd(_mm256_cmp_pd(fr, tl, _CMP_LT_OQ), _mm256_cmp_pd(fr, th, _CMP_GT_OQ)));
        if (_mm256_movemask_pd(tie) & ~past) return true;
        const __m256d qv = _mm256_min_pd(_mm256_max_pd(_mm256_xor_pd(qa, _mm256_broadcast_sd(sg + j)), vlo), vhi);
        __m256d y = _mm256_floor_pd(_mm256_mul_pd(_mm256_add_pd(_mm256_mul_pd(qv, vssh), vdh), vdsc));
        y = _mm256_min_pd(_mm256_max_pd(y, c23lo), c23hi);
        const __m256d d = _mm256_sub_pd(_mm256_broadcast_sd(vd + j), _mm256_mul_pd(y, f16));
        acc = _mm256_add_pd(acc, _mm256_mul_pd(d, d));
        past = _mm256_movemask_pd(_mm256_cmp_pd(acc, vlim, _CMP_GT_OQ));
      }
      if (past != 15) return true;
    }
#else
    for (int k = 0; k < m; k++) {
      double e = 0;
      int j = 0;
      for (; j < n && !(e > lim[k]); j++) {
        const double tq = ua[j] * inv[k] + 0.5, qa = floor(tq), fr = tq - qa;
        if (tq < 16777216.0 && (fr < 1e-7 || fr > 1 - 1e-7)) return true;
        double qv = signbit(sg[j]) ? -qa : qa;
        qv = qv < plo[k] ? plo[k] : qv > phi[k] ? phi[k] : qv;
        double y = floor((qv * ssh[k] + dh[k]) * dsc[k]);
        y = y < -8388608.0 ? -8388608.0 : y > 8388607.0 ? 8388607.0 : y;
        const double d = vd[j] - y * 65536.0;
        e += d * d;
      }
      if (!(e > lim[k])) return true;
    }
#endif
    return false;
  }
  int abits_search(const CoreRec& r, int ch, const S64* v, int n, int g, int a0, int a1, int p, int* conf, U64* dbest,
                   U64 cut = ~0ull, int* ibest = nullptr, int af0 = 0, int af1 = -1) {
    constexpr int NP = 27 * 9, NK = 2;
    const int nt = sf_entries(r.shuff[ch]), nk = imin(n, NK);
    if (cut != ~0ull && !ab_cut_any(r, ch, v, n, a0, a1, p, cut)) return ab_v = v, 0;
    auto mag = [](S64 x) { return x < 0 ? -x : x; };
    S64 w[NSMP];  // (the values, the largest first)
    for (int j = 0; j < n; j++) {
      int k = j;
      while (k > 0 && mag(w[k - 1]) < mag(v[j])) w[k] = w[k - 1], k--;
      w[k] = v[j];
    }
    if (!n || !w[0]) return 0;  // (all 0s: every pair's indexes are 0s)
    ab_v = v;
    struct Pair {
      U64 d;  // (its distance at the largest values: at most its whole one's)
      S128 e;
      int nz;
      S8 a, i;
    } pr[NP];
    QErr qe[NP];
    int np = 0;
    const int i0 = imax(p - 3, 0), i1 = imin(p + 5, nt - 1);
    // (from the guesses out, g's and p: of equal distances the nearer pair; the scale factors' order
    // made once)
    int io[17], ni = 0;
    for (int ki = 0; ki <= 2 * (i1 - i0); ki++) {
      const int i = ki & 1 ? p + (ki + 1) / 2 : p - ki / 2;
      if (i >= i0 && i <= i1) io[ni++] = i;
    }
    const int ga = clip(g, a0, a1);
    if (cut == ~0ull) {
      const U64 t0 = ab_tdist, eps = 2 * (U64)n;
      ab_tdist = ~0ull;
      int G = abits_pass(r, ch, w, n, ga, a0, a1, p, io, ni, conf, dbest, ibest);
      const bool tr = ab_tdist != ~0ull;  // (a transient's)
      if (!tr) ab_tdist = t0;
      ab_g1 = ab_gc = 0;
      if (ffe_ok || (tr && *dbest >= (U64)n << 10)) return G;
      if (tr) {  // (a split where one lattice fits to a 64th: a coarse lattice's one scale factor as near, rather)
        if (a0 > 1) {
          int cc = 0, ic = 0;
          U64 dc = ~0ull;
          const U64 td = ab_tdist, cutt = td > ~0ull - 2 * eps ? ~0ull - 1 : td + eps + 1;
          const int Gc = abits_search(r, ch, w, n, 1, 1, imin(AB_COARSE, a0 - 1), p, &cc, &dc, cutt, &ic);
          if (Gc) {
            G = Gc, *conf = cc, *dbest = dc, ab_tdist = t0;
            if (ibest) *ibest = ic;
          }
        }
        return G;
      }
      if (a0 > 1) {  // (the coarse lattices)
        int cc = 0, ic = 0;
        U64 dc = ~0ull;
        const U64 cutc = !G ? (U64)n << 20 : *dbest > ~0ull - 2 * eps ? ~0ull - 1 : *dbest + eps + 1;
        const int Gc = abits_search(r, ch, w, n, 1, 1, imin(AB_COARSE, a0 - 1), p, &cc, &dc, cutc, &ic);
        if (Gc) {
          const bool clear = !G || (*dbest + eps) / (dc + eps) >= 4, tie = !clear && dc <= *dbest + eps;
          if (tie) ab_g1 = G, ab_gc = Gc;
          if (clear || (tie && abk[1] > abk[0] + abk[0] / 8 + 2)) {
            G = Gc, *conf = cc, *dbest = dc;
            if (ibest) *ibest = ic;
          }
        }
      }
      if (G - 1 >= a0 && *dbest < 3 * eps && !ab_g1 && af0 <= a0 - 1 && a0 - 1 > AB_COARSE) {
        // (the range's pair within 3 eps, no coarse lattice as near: the ABITS between the coarse ones
        // and the range by the cut at its distance and eps, as near as it, the one of the two that has
        // been the ABITS more often, as the coarse pass's)
        const int l0 = imax(af0, AB_COARSE + 1), l1 = a0 - 1;
        int c2 = 0, i2 = 0;
        U64 d2 = ~0ull;
        const int G2 = abits_search(r, ch, w, n, l1, l0, l1, p, &c2, &d2, *dbest + eps + 1, &i2);
        if (G2) {
          ab_g1 = G, ab_gc = G2;
          if (abk[1] > abk[0] + abk[0] / 8 + 2) {
            G = G2, *conf = c2, *dbest = d2;
            if (ibest) *ibest = i2;
          }
        }
      }
      ab_ex[0] += G && *dbest < 3 * eps, ab_ex[1]++;
      if (ab_ex[1] >= 1u << 12) ab_ex[0] >>= 1, ab_ex[1] >>= 1;
      if (af1 >= af0 && (af0 < a0 || af1 > a1) && (!G || *dbest >= 3 * eps) && (ab_ex[1] < 64 || 4 * ab_ex[0] >= ab_ex[1])) {
        // (the rest of af0..af1, under and over: no pair of the range's or the coarse pass's lies within 3 eps)
        const int l0 = imax(af0, a0 > 1 ? imin(AB_COARSE, a0 - 1) + 1 : af0), l1 = imin(af1, a0 - 1),
                  u0 = imax(af0, a1 + 1), u1 = af1;
        int G2 = 0, c2 = 0, i2 = 0;
        U64 d2 = ~0ull;
        if (l0 <= l1) G2 = abits_search(r, ch, w, n, l1, l0, l1, p, &c2, &d2, 3 * eps, &i2);
        if (u0 <= u1) {
          int c3 = 0, i3 = 0;
          U64 d3 = ~0ull;
          const int G3 = abits_search(r, ch, w, n, u0, u0, u1, p, &c3, &d3, G2 && d2 < 3 * eps ? d2 + 1 : 3 * eps, &i3);
          if (G3 && (!G2 || d3 < d2)) G2 = G3, c2 = c3, d2 = d3, i2 = i3;
        }
        if (G2 && d2 < 3 * eps && (!G || (*dbest + eps) / (d2 + eps) >= 4)) {
          G = G2, *conf = c2, *dbest = d2;
          if (ibest) *ibest = i2;
        }
      }
      return G;
    }
    for (int ka = 0; ka <= 2 * (a1 - a0); ka++) {
      const int a = ka & 1 ? ga + (ka + 1) / 2 : ga - ka / 2;
      if (a < a0 || a > a1) continue;
      S64 lo, hi;
      quant_range(r, ch, a, lo, hi);
      for (int z = 0; z < ni; z++) {
        const int i = io[z];
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0 || np >= NP) continue;
        QErr& q = qe[np];
        Pair& x = pr[np];
        q.set(ss, lo, hi);
        x.e = 0, x.nz = 0;
        for (int k = 0; k < nk; k++) x.e += q.err(w[k], &x.nz);
        x.d = norm_dist(x.e, ss), x.a = (S8)a, x.i = (S8)i;
        np++;
      }
    }
    U128 ord[NP];  // (those under cut, by that distance, the first of equals first: a heap of d 2^8 + k)
    int no = 0;
    for (int k = 0; k < np; k++)
      if (pr[k].d < cut) ord[no++] = (U128)pr[k].d << 8 | (unsigned)k;
    if (!no) return 0;
    auto down = [&](int i) {
      for (int c; (c = 2 * i + 1) < no; i = c) {
        if (c + 1 < no && ord[c + 1] < ord[c]) c++;
        if (!(ord[c] < ord[i])) break;
        const U128 t = ord[c];
        ord[c] = ord[i], ord[i] = t;
      }
    };
    for (int i = no / 2 - 1; i >= 0; i--) down(i);
    U64 d1 = ~0ull, d2 = cut;
    int best = -1, bnz = 0;
    while (no > 0 && ((U64)(ord[0] >> 8) < d2 || (best >= 0 && (U64)(ord[0] >> 8) <= d1))) {
      const int at = (int)(ord[0] & 255);
      ord[0] = ord[--no];
      down(0);
      const QErr& q = qe[at];
      S128 e = pr[at].e, lim = ~(U128)0 >> 1;
      int nz = pr[at].nz, j = nk;
      if (d2 != ~0ull) {  // (a distance past lim is past d2's: in doubles, made larger)
        const double l = (double)d2 * (double)q.ss * (double)q.ss / 268435456.0 * (1 + 1e-9);
        if (l < 1e36) lim = (l < 9e18 ? (S128)(S64)l : (S128)l) + 1;  // (in 64 bits where it fits: the same)
      }
      while (j < n && e <= lim) e += q.err(w[j++], &nz);
      if (e > lim) continue;
      const U64 d = norm_dist(e, q.ss);
      if (d < d1) {
        d2 = d1, d1 = d, best = at, bnz = nz;
      } else if (d == d1) {  // (of equals, the coarser lattice, then the scale factor nearer p's)
        if (q.ss > qe[best].ss || (q.ss == qe[best].ss && abs(pr[at].i - p) < abs(pr[best].i - p))) best = at, bnz = nz;
      } else if (d < d2) {
        d2 = d;
      }
    }
    if (best < 0 || !bnz) return 0;
    const U64 eps = 2 * (U64)n;  // (2^-15 steps squared a value)
    const U64 ratio = d2 == ~0ull ? ~0ull : (d2 + eps) / (d1 + eps);
    *conf = imin(bit_len(ratio) - 1, 7);
    *dbest = d1;
    if (ibest) *ibest = pr[best].i;
    return 1 + pr[best].a;
  }
  // (abits_search()'s coarse lattices, ABITS 1 to AB_COARSE; abk, how often the range's pair and the
  // coarse one were the ABITS where they fit as near and differ, halved at 4096 (and the ties under
  // the range's, which go the same way); ab_g1, ab_gc, those two of the last search, else 0; ab_ex,
  // of how many searches the range's pair, after them, has been within 3 eps, halved at 4096)
  static constexpr int AB_COARSE = 3;
  U32 abk[2], ab_ex[2];
  int ab_g1, ab_gc;
  U64 ab_tdist;  // (the transient's fit's distance, where abits_search() took one; the caller sets it to ~0)
  // The distances under which a split's is 4 times nearer than a whole's, dw ((dw + eps) / (d + eps)
  // >= 4 for d under it, as the searches take one: 0 where none is), eps 2 n
  static U64 split_bound(U64 dw, U64 eps) {
    const U64 q4 = (dw + eps) / 4;
    return q4 >= eps ? q4 - eps + 1 : 0;
  }
  // abits_search() without a cut: of the pairs (in their order, from the guesses out), the nearest
  // and the second nearest under 128 times its distance and 127 eps (where *conf is 7), as the heap
  // finds them: of pairs as near, the coarser lattice, then the scale factor nearer p, then the
  // nearer at the 2 largest values, then the first. Each pair's distance in doubles first (ab_dists()),
  // a bound under its exact one; then the exact ones, by the bounds from the least, while one could
  // still be under the second nearest's (or the bound past which *conf is 7).
  int abits_pass(const CoreRec& r, int ch, const S64* w, int n, int ga, int a0, int a1, int p, const int* io, int ni,
                 int* conf, U64* dbest, int* ibest) {
    constexpr int NP = 27 * 9, NK = 2;
    const int nk = imin(n, NK);
    const U64 eps = 2 * (U64)n;  // (2^-15 steps squared a value)
    AbPair pr[NP];
    int np = 0;
    for (int ka = 0; ka <= 2 * (a1 - a0); ka++) {
      const int a = ka & 1 ? ga + (ka + 1) / 2 : ga - ka / 2;
      if (a < a0 || a > a1) continue;
      S64 lo, hi;
      quant_range(r, ch, a, lo, hi);
      for (int z = 0; z < ni; z++) {
        const int i = io[z];
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0 || np >= NP) continue;
        AbPair& x = pr[np++];
        x.ss = ss, x.lo = lo, x.hi = hi, x.a = (S8)a, x.i = (S8)i;
      }
    }
    ab_bounds(w, n, pr, np);
    U64 d1 = ~0ull, d2 = ~0ull, t = ~0ull;
    int ba = -1, bi = 0, bnz = 0, bk = -1;
    S64 bss = 0;
    S128 bpe = 0;  // (the best's distance at the 2 largest values, unnormed; bk its place among the pairs)
    U64 done[(NP + 63) / 64] = {0};
    int lo4[4], n4 = 0;  // (the 4 least bounds' pairs, the least first: of equal bounds, the first)
    for (int m = 0; m < np; m++) {
      if (n4 == 4 && !(pr[m].lb < pr[lo4[3]].lb)) continue;
      int z = n4 < 4 ? n4++ : 3;
      while (z > 0 && pr[m].lb < pr[lo4[z - 1]].lb) lo4[z] = lo4[z - 1], z--;
      lo4[z] = m;
    }
    QErr q;
    for (int step = 0;; step++) {
      int k = -1;
      double lb = 0;
      if (step < n4) k = lo4[step], lb = pr[k].lb;
      else if (n4 == 4)
        for (int m = 0; m < np; m++)
          if (!(done[m >> 6] >> (m & 63) & 1) && (k < 0 || pr[m].lb < lb)) k = m, lb = pr[m].lb;
      if (k < 0) break;
      const U64 B = d2 < t ? d2 : t;
      if (B != ~0ull && lb > (double)B * (1 + 1e-15) + 2) break;  // (the rest are all past B)
      done[k >> 6] |= 1ull << (k & 63);
      const AbPair& x = pr[k];
      q.set(x.ss, x.lo, x.hi);
      S128 e = 0, lim = ~(U128)0 >> 1, pe = 0;
      if (B != ~0ull) {  // (a distance past lim is B's at least: in doubles, made larger)
        const double l = (double)B * (double)x.ss * (double)x.ss / 268435456.0 * (1 + 1e-9);
        if (l < 1e36) lim = (l < 9e18 ? (S128)(S64)l : (S128)l) + 1;
      }
      int nz = 0, j = 0;
      for (; j < nk; j++) e += q.err(w[j], &nz);
      pe = e;
      while (j < n && e <= lim) e += q.err(w[j++], &nz);
      if (e > lim) continue;
      const U64 d = norm_dist(e, x.ss);
      if (d == ~0ull) continue;
      const S64 ss = x.ss;
      const int a = x.a, i = x.i;
      if (d < d1) {
        d2 = d1, d1 = d, ba = a, bi = i, bnz = nz, bss = ss, bpe = pe, bk = k;
        t = d1 > (~0ull - 127 * eps) / 128 ? ~0ull : 128 * d1 + 127 * eps;
      } else if (d == d1) {
        bool take = ss > bss || (ss == bss && abs(i - p) < abs(bi - p));
        if (!take && ss == bss && abs(i - p) == abs(bi - p)) {
          const U64 u = norm_dist(pe, ss), ub = norm_dist(bpe, bss);
          take = u < ub || (u == ub && k < bk);
        }
        if (take) ba = a, bi = i, bnz = nz, bss = ss, bpe = pe, bk = k;
      } else if (d < d2) {
        d2 = d;
      }
    }
    if (ba < 0 || !bnz) return 0;
    if (n >= 16 && d1 >= 3 * eps && !ffe_ok && ab_rule()) {  // (a transient's: abits_search()'s)
      const int ns = n / 8;
      const S64* v = ab_v;
      U64 bk = split_bound(d1, eps);  // (only a split 4 times nearer is taken: the parts' fits under that alone)
      int bta = -1;
      for (int a = a0; bk && a <= a1; a++) {
        S64 lo, hi;
        quant_range(r, ch, a, lo, hi);
        for (int k = 1; k < ns; k++) {
          const U64 dA = part_fit(r, ch, a, lo, hi, v, 8 * k, bk);
          if (dA >= bk) continue;
          const U64 dB = part_fit(r, ch, a, lo, hi, v + 8 * k, n - 8 * k, bk - dA);
          const U64 d = dA + dB < dA ? ~0ull : dA + dB;
          if (d < bk) bk = d, bta = a;
        }
      }
      if (bta >= 0 && bk != ~0ull && (d1 + eps) / (bk + eps) >= 4) {
        ab_tdist = bk;
        const U64 ratio = (d1 + eps) / (bk + eps);
        *conf = imin(bit_len(ratio) - 3, 7);
        *dbest = d1;
        return 1 + bta;
      }
    }
    const U64 ratio = d2 == ~0ull ? ~0ull : (d2 + eps) / (d1 + eps);
    *conf = imin(bit_len(ratio) - 1, 7);
    *dbest = d1;
    if (ibest) *ibest = bi;
    return 1 + ba;
  }
  const S64* ab_v;  // (abits_search()'s values, in their order: abits_pass()'s transients'; its own passes
                    // after the first get them the largest first, as their cuts are past soonest so)
  // Where the largest of the m values v lies past the top of ABITS a's lattice (its range lo..hi) at
  // scale factor i1, the first entry above i1 whose step puts it at the top or under (the table's
  // end where none does), else -1: a lone index's RMS among 16 lies 12 dB under its peak, its
  // encoder's scale factor 6 entries above the RMS rule's at 2.2 dB an entry (11 at 1.1), out of
  // the searches' windows (`dtswavsample16.wav`'s quiet subbands, and the DTS CD's)
  int peak_scale(const CoreRec& r, int ch, int a, S64 lo, S64 hi, const S64* v, int m, int i1) const {
    S64 pk = 0;
    for (int k = 0; k < m; k++) pk = imax64(pk, v[k] < 0 ? -v[k] : v[k]);
    const S128 top = hi > -lo ? hi : -lo, pv = (S128)pk * 64;
    if (top <= 0 || (S128)step_for(r, ch, a, i1) * top >= pv) return -1;
    int l = i1 + 1, h = sf_entries(r.shuff[ch]);
    while (l < h) {
      const int m2 = (l + h) >> 1;
      if ((S128)step_for(r, ch, a, m2) * top >= pv) h = m2;
      else l = m2 + 1;
    }
    return l;
  }
  // the nearest fit of the values v (m of them) to ABITS a's lattice (its range lo..hi) at a scale
  // factor from 3 below their RMS rule's to 5 above, and the two either side of the peak's past
  // that (peak_scale(): since format 146): its distance (norm_dist()'s)
  U64 part_fit(const CoreRec& r, int ch, int a, S64 lo, S64 hi, const S64* v, int m, U64 lim = ~0ull) const {
    const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
    const int nt = sf_entries(r.shuff[ch]);
    int near;
    const int p = scale_pred(v, m, tab, nt, &near);
    U64 bd = ~0ull;
    // (from p out, each one's distance only while it could be under the nearest's and lim: those
    // under lim as trying them all finds; ~0 where none is)
    const int i0 = imax(p - 3, 0), i1 = imin(p + 5, nt - 1);
    auto fit = [&](int i) {
      const S64 ss = step_for(r, ch, a, i);
      if (ss <= 0) return;
      QErr q;
      q.set(ss, lo, hi);
      const U64 dl = bd < lim ? bd : lim;
      S128 e = 0, el = ~(U128)0 >> 1;
      const double l = (double)dl * (double)ss * (double)ss / 268435456.0 * (1 + 1e-9);  // (a distance past el is
      if (l < 1e36) el = (l < 9e18 ? (S128)(S64)l : (S128)l) + 1;                        // past dl's)
      int nz = 0, k = 0;
      while (k < m && e <= el) e += q.err(v[k++], &nz);
      if (e > el) return;
      const U64 d = norm_dist(e, ss);
      if (d < bd) bd = d;
    };
    for (int ki = 0; ki <= 2 * 5; ki++) {
      const int i = ki & 1 ? p + (ki + 1) / 2 : p - ki / 2;
      if (i >= i0 && i <= i1) fit(i);
    }
    const int pi = peak_scale(r, ch, a, lo, hi, v, m, i1);
    for (int i = pi - 1; pi >= 0 && i <= pi; i++)
      if (i > i1 && i < nt) fit(i);
    return bd < lim ? bd : ~0ull;
  }
  // The TMODE a decoded WAV says (the transient search), its ABITS known: the subsubframe from which
  // two scale factors' fit best, where 4 times better than one at least, *conf the log2 of that ratio
  // less 2, to 7; else where the subband's values fit one scale factor's lattice to a 64th of a step
  // squared a value, none (0, *conf 7), and -1 where not. (Each part's scale factor the nearest of
  // those from 3 below its RMS rule's to 5 above, and the peak's (part_fit()); the whole's fit the
  // bit allocation search's, where that found this ABITS, or its own where nearer.) Until format 146
  // a fit to that 64th was none, 4 times better or not: but where a transient's first part is
  // quiet, its scale factor far under the second's, its values lie near 0 on the second's lattice,
  // and the DTS CD's TMODEs were 18905 such of 53430 transients (its core 16.8 KB to 11.6; the
  // peak's scale factors, which a lone index's subband fits only at, 3.6% smaller again). Since
  // then none only where within 3 eps, as no split's can be 4 times nearer, or where FFmpeg's
  // encoder is made again, which takes no transients.
  int tmode_search(const CoreRec& r, const CoreSub& s, int ch, int b, int* conf) {
    const int a = s.abits[ch][b], ns = s.nssc;
    if (!a || ns < 2) return -1;
    S64 x[NSMP];
    const int n = scale_values(s, ch, b, 0, ns, x);
    S64 lo, hi;
    quant_range(r, ch, a, lo, hi);
    const U64 eps = 2 * (U64)n;
    U64 d0 = abfit_a[ch][b] == a ? abfit_d[ch][b] : part_fit(r, ch, a, lo, hi, x, n);
    if (d0 < 3 * eps || (ffe_ok && d0 < ((U64)n << 10))) return *conf = 7, 0;
    if (abfit_a[ch][b] == a) {  // (the whole's own fit too, the peak's scale factors among its)
      const U64 dw = part_fit(r, ch, a, lo, hi, x, n);
      if (dw < d0) d0 = dw;
      if (d0 < 3 * eps) return *conf = 7, 0;
    }
    const bool one = d0 < ((U64)n << 10);
    U64 bk = d0 == ~0ull ? ~0ull : split_bound(d0, eps);  // (only a split 4 times nearer is taken)
    int best = -1;
    for (int k = 1; bk && k < ns; k++) {  // (each part's fit only while under the best's, as all would find)
      const U64 d1 = part_fit(r, ch, a, lo, hi, x, 8 * k, bk);
      if (d1 >= bk) continue;
      const U64 d2 = part_fit(r, ch, a, lo, hi, x + 8 * k, 8 * (ns - k), bk - d1), d = d1 + d2 < d1 ? ~0ull : d1 + d2;
      if (d < bk) bk = d, best = k;
    }
    if (best < 0 || bk == ~0ull || (d0 == ~0ull ? false : (d0 + eps) / (bk + eps) < 4)) return one ? (*conf = 7, 0) : -1;
    const U64 ratio = d0 == ~0ull ? ~0ull : (d0 + eps) / (bk + eps);
    *conf = imin(bit_len(ratio) - 3, 7);
    return best;
  }
  // a squared distance e (2^-2AQ_F) in the step ss's (2^-22) squared, times 2^16 (to 2^64 - 1)
  static U64 norm_dist(S128 e, S64 ss) {
    const U128 num = (U128)e << 28, den = (U128)((S128)ss * ss);
    if (!(num >> 64) && !(den >> 64)) return (U64)num / (U64)den;  // (the same, in 64 bits)
    if (!(num >> 120) && den) {  // (the same, from an estimate in doubles within 1 of it where under 10^15: then
                                 // q den is under num + den, under 2^127)
      auto dbl = [](U128 x) { return (double)(U64)(x >> 64) * 18446744073709551616.0 + (double)(U64)x; };
      const double qd = dbl(num) / dbl(den);
      if (qd < 1e15) {
        U64 q = (U64)qd;
        const U128 p = (U128)q * den;
        if (p > num) q--;
        else if (num - p >= den) q++;
        return q;
      }
    }
    const U128 q = num / den;
    return q >> 64 ? ~0ull : (U64)q;
  }

  // A lossless WAV's subband: how near the RMS of its n values v lies to the boundary
  // between tab's entry p (nt entries) and the one beside it on the RMS's side, as the encoder's
  // arithmetic errs, for the RMS rule's nearness (code_scale()): the nearer of the distance times
  // the root of n against the channel's level in the subframe times the subband's error's scale
  // (hcal's, less their mean), from 2^-25 (2^-24 before format 74: `s.2.dts`'s core 0.24% smaller),
  // and against the RMS itself, from 2^-16, in doublings
  // (0-15; -1 where there is no boundary). Where p ends a run of entries with its value, the value
  // is a boundary too, the one between the run's two ends (anal.inc's sf_run()): the nearer counts
  // (`s.2.dts`'s core 0.6% smaller; pn's nearness, the relative one, stays the midpoint's: which side
  // it lies on says which way a miss goes, to the run's other end or past the midpoint)
  // (add: a power the RMS's square has in it too, the top subband's leak's noise: the nearness then
  // against the root of the level's unit squared and 2^48 times it, the noise's as the level's is,
  // `s.2.dts`'s core 0.13% smaller)
  // (lv: the level instead of the channel's, where 0 or more)
  int scale_near_lev(int ch, int b, const S64* v, int n, const U32* tab, int nt, int p, S128 add = 0, S64 lv = -1) const {
    S128 ms = 0;
    for (int i = 0; i < n; i++) ms += (S128)v[i] * v[i];
    const S64 rr = isqrt128((U128)(ms / n + add)), lev = lv >= 0 ? lv : tlvl[ch], e = (S64)tab[p] << AQ_F;
    S64 bd = -1;
    if (rr > e && p + 1 < nt) bd = ((S64)tab[p] + tab[p + 1]) << (AQ_F - 1);
    else if (rr <= e && p > 0) bd = ((S64)tab[p - 1] + tab[p]) << (AQ_F - 1);
    if (bd < 0 || lev <= 0) return -1;
    const int sm = hc_mean();
    S64 dd = rr > bd ? rr - bd : bd - rr;
    if (sf_run(tab, nt, p)) {
      const S64 de = rr > e ? rr - e : e - rr;
      if (de < dd) dd = de;
    }
    const S128 ul = hc_unit((S128)lev, hcal[b].s - sm);
    const S128 u = add > 0 ? (S128)isqrt128((U128)(ul * ul + shl128(add, 48))) : ul;
    const int a = near_bucket((S128)dd * isqrt128((U128)n << 8) >> 4, u, 25);
    return imin(a, near_bucket((S128)dd, (S128)rr, 16));
  }

  // (sf_post(): the n values v (2^-AQ_F) with noise of power s2 at each: the entries of tab (nt)
  // within SP_W of c0, the probability of each one's cell of the RMS (the run of equal entries'
  // first takes up to its value, its last from it on, those between none: scale_pred_ms()'s), into
  // ci and pv, the likelier first (the lower where equal): their number, 0 where there's no noise;
  // in doubles, IEEE's arithmetic, in the samples' order)
  static int sf_post(const S64* v, const double* s2, int n, const U32* tab, int nt, int c0, int* ci, double* pv) {
    double sx = 0, sn = 0, sxn = 0, snn = 0;
    for (int i = 0; i < n; i++) {
      const double a = (double)v[i] * (double)v[i];
      sx += a, sn += s2[i], sxn += a * s2[i], snn += s2[i] * s2[i];
    }
    const double vr = (4 * sxn + 2 * snn) / ((double)n * n);
    if (!(vr > 0)) return 0;
    const double m = (sx + sn) / n, sd = sqrt(vr), U = 65536.0;
    int k = 0;
    for (int i = imax(c0 - SP_W, 0); i <= imin(c0 + SP_W, nt - 1); i++) {
      const bool rl = i > 0 && tab[i - 1] == tab[i], rh = i + 1 < nt && tab[i + 1] == tab[i];
      if (rl && rh) continue;
      const double lo = rl ? (double)tab[i] * U : i ? ((double)tab[i - 1] + tab[i]) * (U / 2) : 0.0;
      const bool top = i + 1 >= nt;
      const double hi = rh ? (double)tab[i] * U : top ? 0.0 : ((double)tab[i] + tab[i + 1]) * (U / 2);
      const double za = (lo * lo - m) / sd, zb = top ? 1e30 : (hi * hi - m) / sd;
      const double qb = top ? 0.0 : zb >= 0 ? det_q(zb) : 0.0;
      double pp = za >= 0 ? det_q(za) - qb : zb <= 0 ? det_q(-zb) - det_q(-za) : 1.0 - det_q(-za) - qb;
      if (!(pp > 0)) pp = 0;
      int at = k++;
      while (at > 0 && pv[at - 1] < pp) ci[at] = ci[at - 1], pv[at] = pv[at - 1], at--;
      ci[at] = i, pv[at] = pp;
    }
    return k;
  }
  // (sf_mc(): sf_post()'s by sampling, n at most 16: the values with the noise drawn as hf_mc()'s
  // (SF_P passes of MC_N samples in its pairings, MC_A of a sample to each cell), each sample's mean
  // square counted in its cell (since format 96 16 passes, 0.07% smaller than 1; 4, 0.05%).
  // The top subband's since format 94: where it is weak against the noise, its mean square's
  // distribution is skewed, where sf_post()'s normal one isn't: `s.2.dts`'s core 0.13% smaller,
  // the short streams 6 bytes; subbands 28-30's and the rest's, by any power of their noise from
  // 1/2 to 8 of theirs, 0.1-1.4% bigger, with 4096 samples the same)
  static constexpr int SF_P = 16;  // (sf_mc()'s passes of MC_N samples)
  int sf_mc(const S64* v, const double* s2, int n, const U32* tab, int nt, int c0, int* ci, double* pv) {
    double clo[2 * SP_W + 1], chi[2 * SP_W + 1], sd[16];
    int cix[2 * SP_W + 1], k = 0;
    const double U = 65536.0;
    for (int i = imax(c0 - SP_W, 0); i <= imin(c0 + SP_W, nt - 1); i++) {
      const bool rl = i > 0 && tab[i - 1] == tab[i], rh = i + 1 < nt && tab[i + 1] == tab[i];
      if (rl && rh) continue;
      const double lo = rl ? (double)tab[i] * U : i ? ((double)tab[i - 1] + tab[i]) * (U / 2) : 0.0;
      const bool top = i + 1 >= nt;
      const double hi = rh ? (double)tab[i] * U : top ? 0.0 : ((double)tab[i] + tab[i + 1]) * (U / 2);
      clo[k] = lo * lo, chi[k] = top ? 1e300 : hi * hi, cix[k++] = i;
    }
    bool any = false;
    for (int i = 0; i < n; i++) sd[i] = sqrt(s2[i]), any = any || sd[i] > 0;
    if (!any || !k) return 0;
    int cnt[2 * SP_W + 1] = {0};
    double vd[16];
    for (int i = 0; i < n; i++) vd[i] = (double)v[i];
    for (int pass = 0; pass < SF_P; pass++)
      for (int t = 0; t < MC_N; t += 4) {  // (four samples at a time, each one's sums in order, as the plain C++'s)
        alignas(32) double ms[4];
#ifdef DTSD_AVX2
        __m256d a = _mm256_setzero_pd();
        for (int i = 0; i < n; i++) {
          const __m256d z = _mm256_loadu_pd(mczt[i] + ((t + pass * (53 * i + 1)) & (MC_N - 1)));
          const __m256d y = _mm256_add_pd(_mm256_set1_pd(vd[i]), _mm256_mul_pd(_mm256_set1_pd(sd[i]), z));
          a = _mm256_add_pd(a, _mm256_mul_pd(y, y));
        }
        _mm256_store_pd(ms, _mm256_div_pd(a, _mm256_set1_pd((double)n)));
#else
        for (int u = 0; u < 4; u++) {
          double a = 0;
          for (int i = 0; i < n; i++) {
            const double y = vd[i] + sd[i] * mczt[i][((t + pass * (53 * i + 1)) & (MC_N - 1)) + u];
            a += y * y;
          }
          ms[u] = a / n;
        }
#endif
        for (int u = 0; u < 4; u++)
          for (int c = 0; c < k; c++)
            if (ms[u] >= clo[c] && ms[u] < chi[c]) {
              cnt[c]++;
              break;
            }
      }
    int m = 0;
    for (int c = 0; c < k; c++) {
      const double pp = (cnt[c] + MC_A) / (MC_N * SF_P + k * MC_A);
      int at = m++;
      while (at > 0 && pv[at - 1] < pp) ci[at] = ci[at - 1], pv[at] = pv[at - 1], at--;
      ci[at] = cix[c], pv[at] = pp;
    }
    return m;
  }
  // (the scale factor v by sf_post()'s k entries ci, pv, of class g (the top subband, 28-30, the
  // rest); past the first SP_R, f_scl's difference from the likeliest)
  template <class F>
  int sf_post_code(F& f, int g, const int* ci, const double* pv, int k, S64 v, tcx& X_, u32 kd, int max) {
    double rem = 1.0;
    for (int r = 0; r < k && r < SP_R; r++) {
      double pc = rem > 1e-12 ? pv[r] / rem : 0.0;
      pc = pc < 1e-4 ? 1e-4 : pc > 1 - 1e-4 ? 1 - 1e-4 : pc;
      int bk = 0;
      while (bk < SP_NB - 1 && pc >= sp_t[bk]) bk++;
      double(*const xr)[4] = sp_x[g][r < 2 ? r : 2];
      // (between the bucket's calibration and the nearer neighbour's, by the odds' place from the
      // bucket's middle toward its edge, half of the neighbour's there; the outer buckets alone)
      const double od = pc / (1.0 - pc);
      double kk = sp_k(xr[bk]);
      if (bk > 0 && bk < SP_NB - 1) {
        const double olo = sp_t[bk - 1] / (1.0 - sp_t[bk - 1]), ohi = sp_t[bk] / (1.0 - sp_t[bk]);
        const double om = sqrt(olo * ohi), ed = od < om ? olo : ohi;
        double wn = 0.5 * (od - om) / (ed - om);
        wn = wn < 0.0 ? 0.0 : wn > 0.5 ? 0.5 : wn;
        kk = kk * (1.0 - wn) + sp_k(xr[od < om ? bk - 1 : bk + 1]) * wn;
      }
      const double ph = 1.0 / (1.0 + (1.0 - pc) / pc * kk);
      U32 q = (U32)((1.0 - ph) * CO_ONE);
      const U32 qe = CO_ONE >> 14;  // (no surer than 2^-14: 2^-11 until format 103's 18-bit coder)
      q = q < qe ? qe : q > CO_ONE - qe ? CO_ONE - qe : q;
      const int hit = c->code_bit(q, ENC && v == ci[r]);
      double* const xx = xr[bk];
      xx[hit ? 0 : 1] += 1.0, xx[2] += pc, xx[3] += 1.0 - pc;
      if (xx[0] + xx[1] > SP_LIM) xx[0] *= 0.5, xx[1] *= 0.5, xx[2] *= 0.5, xx[3] *= 0.5;
      if (hit) return ci[r];
      rem -= pv[r];
    }
    return (int)checked(ci[0] + f.codes(X_, ENC ? v - ci[0] : 0, kd), 0, max - 1);
  }
  // A scale factor v in 0..max-1 by the family f (scl's; a decoded WAV's scd's, since format 118:
  // scale()), its variables D: by sf_post()'s k entries where spu (sf_post_code()), else against
  // p (>= 0: whether it is p, else its difference from it) or the last one A (1 + it, 0 none)
  template <class F, class V>
  S64 scale_val(F& f, V& D, S64 v, int p, S32 A, int max, int spu = 0, int sg = 0, const int* spc = nullptr,
                const double* spp = nullptr, int spk = 0) {
    tcx X;
    f.make(D, X);
    f.select(X);
    if (spu) return sf_post_code(f, sg, spc, spp, spk, v, X, (u32)D.kd, max);
    if (p >= 0) return f.bit(CN_HIT, ENC && v == p) ? p : checked(p + f.codes(X, ENC ? v - p : 0, (u32)D.kd), 0, max - 1);
    if (A && f.bit(CN_SAME, ENC && v == A - 1)) return A - 1;
    return checked((A ? A - 1 : 0) + f.codes(X, ENC ? v - (A ? A - 1 : 0) : 0, (u32)D.kd), 0, max - 1);
  }
  // (scl's, or scd's where the scale factor search shows the WAV decoded: ss_rule())
  S64 scale(M_scl_v& D, S64 v, int p, S32 A, int max, int spu = 0, int sg = 0, const int* spc = nullptr,
            const double* spp = nullptr, int spk = 0) {
    if (!ss_rule()) return scale_val(f_scl, D, v, p, A, max, spu, sg, spc, spp, spk);
    M_scd_v E;
#define M_COPY(f) E.f = D.f;
    M_scl_VARS(M_COPY)
#undef M_COPY
    return scale_val(f_scd, E, v, p, A, max, spu, sg, spc, spp, spk);
  }

  // Scale factor j (0 the first, 1 the second, after a transient) of channel ch's subband b, v in
  // 0..max-1: against the one the audio makes, where the channel is bound, else the last. Three
  // rules make it from the audio's values, the one right clearly more often taken: the RMS rule; the
  // first entry above 0.99 times the RMS (a 96/24 encoder's); the scale factor search (a decoded
  // WAV's; where it isn't sure, the better of the others'), its sureness then the context the RMS
  // rule's nearness is
  S64 code_scale(const CoreRec& r, const CoreSub& s, int ch, int b, int j, S64 v, int pex = -1) {
    const int hfb = b >= r.nvqsub[ch], tm = hfb ? 0 : s.tmode[ch][b];
    const int max = r.shuff[ch] == 6 ? 128 : 64;
    M_scl_v D;
    int p = -1, pc = -1, ps = -1, near = SB_N - 1, conf = 0, tried = 0, nlev = -1;
    const int sg = b == NSUB - 1 ? 0 : hfb ? 1 : 2;  // (the posterior's: sf_post(), its class)
    int spc[2 * SP_W + 1], spk = 0;
    double spp[2 * SP_W + 1];
    if (bound[ch]) {
      S64 v_[NSMP];
      int n = tm ? (j ? scale_values(s, ch, b, tm, s.nssc, v_) : scale_values(s, ch, b, 0, tm, v_))
                 : scale_values(s, ch, b, 0, s.nssc, v_);
      if (past_on && n) {  // (without the values whose windows lie far past the WAV's end, where some are left)
        const int j0 = tm && j ? tm * 8 : 0;
        int m = 0;
        for (int i = 0; i < n; i++)
          if (!past_end(t0 + j0 + i)) v_[m++] = v_[i];
        if (m) n = m;
      }
      if (n) {
        // (a lossless WAV's top subband: its RMS with the bottom one's leak's noise in it, 2^-40 of
        // that one's power, as the encoder's scale factor goes: 81.6% of `s.2.dts`'s, from 79.2%; and
        // pl's, the nearness as that rule's: `s.2.dts`'s core 0.02% smaller than on the values as
        // they are. Since format 84 3/4 of 2^-40, with the HF VQ's noise by the value: 0.03%
        // smaller; 1/2 or 3/2 of it bigger. The nearness's noise there the bottom subband's power
        // at each sample weighed by the top one's, as the encoder's RMS goes: it errs by twice the
        // value times the noise, a sample at a time: 0.05% smaller than the power as it is; 1/2 or
        // 2 times it 0.12% and 0.06% bigger. Since format 85 the bottom subband's power as
        // hf_stair2() has it, and the nearness's noise with the noise's own square in it too, the
        // sum of the noise powers' squares over the top subband's power, at twice the Gaussian's
        // half: 0.02%; at the Gaussian's, or 4 times it, 0.01% less)
        if (b == NSUB - 1 && cpl_on(r, ch, b)) {
          S128 ms = 0, m0 = 0;
          for (int i = 0; i < n; i++) ms += (S128)v_[i] * v_[i];
          for (int i = 0; i < nsmp; i++) m0 += (S128)hf_stair2(X[ch][0][i]);
          p = scale_pred_ms(ms / n + (m0 / nsmp * 3 >> 42), r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
          if (!ss_rule() && !x96a && p >= 0) {
            S128 nz = m0 / nsmp * 3 >> 42;
            if (n == nsmp) {  // (in doubles, IEEE's arithmetic, in the samples' order)
              double nu = 0, de = 0, q4 = 0;
              for (int i = 0; i < n; i++) {
                const double a = (double)v_[i] * (double)v_[i], c = (double)hf_stair2(X[ch][0][i]);
                nu += a * c, de += a, q4 += c * c;
              }
              if (de > 0) nz = (S128)(ldexp(nu / de * 3, -42) + q4 / de * ldexp(9.0, -84));
            }
            nlev = scale_near_lev(ch, b, v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, nz);
            if (n == nsmp) {
              double s2[NSMP];
              for (int i = 0; i < n; i++) s2[i] = (double)hf_stair2(X[ch][0][i]) * SP_K;
              spk = n <= 16 ? sf_mc(v_, s2, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, spc, spp) : sf_post(v_, s2, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, spc, spp);
              if (spk && (spp[0] >= SP_PT || !sp_on())) spk = 0;
            }
          }
        } else {
          p = scale_pred(v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
          // (a lossless WAV's: pn's outside the VQ subbands, whose contexts want the RMS's nearness
          // as it was: `s.2.dts`'s core 1.1% smaller, `3-1.dts`'s 0.1%; and pl's)
          // (since format 87 at the core's rate against the level along the subband's FFT path,
          // the indexes' (path_levels()), at each sample weighed by the value's power there, as
          // the encoder's RMS errs by twice the value times the noise, times 2^(3/4): `s.2.dts`'s
          // core 0.16% smaller, the subbands 15-30's; as it is 0.16% less, times 2^(1/2) or 2 0.03%
          // and 0.04% less, the power's mean bigger than the channel's level)
          S64 lv = -1;
          if (!ss_rule() && !x96a && bind.up == 1 && p >= 0) {  // (in doubles, IEEE's arithmetic, in the samples' order)
            const int j0 = tm && j ? tm * 8 : 0;
            double nu = 0, de = 0, pm = 0;
            int c = 0;
            for (int i = 0; i < n && j0 + i < nsmp; i++, c++) {
              const double a = (double)v_[i] * (double)v_[i], l = (double)plev[ch][b][j0 + i] * (double)plev[ch][b][j0 + i];
              nu += a * l, de += a, pm += l;
            }
            if (c) lv = (S64)hc_unit((S128)sqrt(de > 0 ? nu / de : pm / c), 3);
          }
          if (!ss_rule() && !x96a && p >= 0)
            nlev = scale_near_lev(ch, b, v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, 0, lv);
          if ((!hfb || (b >= NSUB - 4 && n == nsmp)) && !ss_rule() && !x96a && bind.up == 1 && p >= 0) {
            const int j0 = tm && j ? tm * 8 : 0;
            if (j0 + n <= nsmp) {
              double s2[NSMP];
              double kb = hfb ? SP_K2 : SP_K3;
              if (!hfb) {  // (SP_K3 by 2^((s - m) / 8), s the subband's error scale, m their mean)
                const int d = (hcal[b].s - hc_mean()) / 2, q = d >= 0 ? d / 2 : -((-d + 1) / 2);
                kb = ldexp(d - 2 * q ? 1.4142135623730951 * kb : kb, q);
              }
              for (int i = 0; i < n; i++) s2[i] = (double)plev[ch][b][j0 + i] * (double)plev[ch][b][j0 + i] * kb;
              spk = sf_post(v_, s2, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, spc, spp);
              if (spk && (spp[0] >= SP_PT || !sp_on())) spk = 0;
            }
          }
        }
        pc = scale_ceil(v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]));
        if (search_on()) ps = scale_search(r, s, ch, b, hfb, v_, n, p, &conf), tried = 1;
      }
    }
    // (the better of the two RMS rules, and the search where it is better still: where it isn't
    // sure, that rule's)
    const int pr = p, pb = sf_base() ? pc : p;
    const int pss = ps >= 0 ? ps : pb;
    if (p >= 0) {
      if (tried && ss_rule()) {
        p = pss;
        if (ps >= 0) near = imax(conf, 0);
      } else {
        p = pb;
      }
    }
    // (where X96 codes the subband too, a scale factor alone in its subframe: of the prediction and
    // the 2 either side of it, the first whose step the fit finds, by their distance from it (jx_try();
    // X96's vectors tried only where it had them the last time), or where the core has little of the
    // decode's values, which say little of the scale factor, of the last one and the 8 either side of
    // it; where that has been right more often than the prediction, by a margin: `96-24.dts`'s scale
    // factors 98.1% the audio's from 89.5%, its core 10.5% smaller (5.8% of that the last ones'), 26%
    // slower)
    int pj = -1;
    if (bound[ch] && x96a && b >= jx_lo && !hfb && !tm && s.abits[ch][b] && p >= 0 && (jx_hn[b] | jx_hvn[b])) {
      const int a = s.abits[ch][b], nt = sf_entries(r.shuff[ch]);
      S64 lo, hi;
      quant_range(r, ch, a, lo, hi);
      const bool lg = jx_low(b) && hist.scale[ch][b];
      const int c0 = lg ? hist.scale[ch][b] - 1 : p, no = lg ? 17 : 5;
      // (those, in order from the RMS rule's of the core's values as the decode's fit has them, less
      // the prediction (je_one()): where the core's quantizer has 3 levels, a few scale factors' steps
      // fit the values, and the one nearest the last isn't the encoder's often; `96-24.dts`'s
      // subband 30's 88.8% right from 75.7%, its core 1.4% smaller)
      int ce = -1;
      if (lg && ss_rule() && je_on(ch, b)) {
        S64 ve[NSMP], h[4];
        for (int k = 0; k < 4; k++) h[k] = shl64(yh[ch][b][k], AQ_F);
        for (int i = 0; i < nsmp; i++) {
          const S64 y = je_one(ch, b, i);
          ve[i] = s.pmode[ch][b] ? y - adpcm_pred_x(s.pvq[ch][b], h) : y;
          h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
        }
        int ne;
        ce = scale_pred(ve, nsmp, r.shuff[ch] == 6 ? SF7 : SF6, nt, &ne);
      }
      const int cc = ce >= 0 ? ce : c0, nw = ce >= 0 ? 2 * (8 + (ce > c0 ? ce - c0 : c0 - ce)) + 1 : no;
      for (int o = 0; o < nw && pj < 0; o++) {
        const int i = cc + (o & 1 ? (o + 1) / 2 : -(o / 2));
        if (ce >= 0 && (i < c0 - 8 || i > c0 + 8)) continue;
        const S64 st = i >= 0 && i < nt ? step_for(r, ch, a, i) : 0;
        if (st <= 0) continue;
        S64 ss[NSMP];
        for (int k = 0; k < nsmp; k++) ss[k] = st;
        int k9, vq[NSMP / 16], vs[NSMP / 16];
        if (jx_try(s, ch, b, ss, lo, hi, &k9, vq, vs, !jx_wv[ch][b])) pj = i;
      }
    }
    const int pp = p;
    if (pj >= 0 && jx_more(jx_jr[b], 4, 1)) p = pj, near = SB_N - 1;
    if (pex >= 0 && !hfb && !tm) p = pex, near = SB_N - 1;  // (the core's values': jx_core_scale())
    const S32 A = hist.scale[ch][b], P = hist.pscale[ch][b];
    // (where X96 codes the subband too and the core has little of the decode's values, the last
    // one where it has been right more often, by a margin: `96-24.dts`'s core 0.3% smaller)
    const int lw = bound[ch] && jx_low(b), pa = p;
    if (lw && jx_more(jx_sr[b], 4, 1)) p = -1, near = SB_N - 1;
    // (where FFmpeg's encoder's bit allocations are made again, the first guess the one of four that
    // has been the scale factor most often at the subband's ABITS (to 12): the model's prediction,
    // FFmpeg's encoder's rule on the decode's peak (calc_one_scale()), and the least scale factor
    // whose step puts that peak within 3/32 or 8/32 of the quantizer's top level (FfEnc::
    // scale_within(): the decode's peak is an index times the step, the index at most that level).
    // FFmpeg's decode of its encode of `s.2.dts`'s first 3 MB: the model's prediction right in
    // 76.5% of its scale factors, the rule in 74.0% (the decode's peak 3 under at ABITS 1-3), the
    // two within 95.9% and 81.9%, the four so chosen 96.7%; its core 26% smaller, `open
    // bitrate.dts`'s encode's 7%, the two others' 37% and 58%. The nearness the model's own, the
    // run its choice's: set to the surest where the choice isn't the model's, 0.3% bigger. Since
    // format 137 most often there and at the rule's scale factor (in fours, to 40) and the model's
    // prediction's difference from it (to 2): which one is right goes by both, the rule and the
    // two within where the steps are under the WAV's unit, the model's prediction more often
    // where they aren't. The five FFmpeg encodes' first guesses wrong 1545 times in their
    // 1301664 scale factors, from 8263 (by the ABITS and the rule's scale factor in twos, 4578):
    // their scale factors 8.4 KB to 1.8, the five 34892 bytes to 28360 (31715 so, 32533 by the
    // rule's in eights; by the difference to 1 or 3 the first guesses wrong 2572 and 1639 times))
    int fc[4] = {-1, -1, -1, -1}, p0 = p, fb = 0;
    if (ffe_ok && !r.xpass && bound[ch] && !hfb && !tm && !s.pmode[ch][b] && p >= 0 && r.shuff[ch] == 6) {
      FfSoft qq;
      const int a = s.abits[ch][b];
      fc[0] = p, fc[1] = FfEnc::one_scale(ffe->peak[ch][b], a, &qq);
      fc[2] = ffe->scale_within(ch, b, a, 3), fc[3] = ffe->scale_within(ch, b, a, 8);
      fb = (imin(fc[1], 40) >> 2) * 5 + clip(fc[0] - fc[1], -2, 2) + 2;
      const U16* const w = ffs_w[imin(a, 12)][fb / 5][fb % 5];
      int k = 0;
      for (int i = 1; i < 4; i++)
        if (w[i] > w[k]) k = i;
      if (qff_on()) k = 1;  // (since format 138 the rule's where the WAV is the encoder's input)
      if (k) p = fc[k];
    }
    D.dp = p >= 0 ? sf_run(r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p) : 0;
    D.kd = hfb ? 2 : j;
    D.ab = hfb ? 0 : vvar(s.abits[ch][b] + 1, 27);
    D.pm = s.pmode[ch][b];
    D.pk = p >= 0;
    D.pn = nlev >= 0 && p == pr && !hfb ? imin(nlev, SB_N - 1) : near;
    D.pl = nlev >= 0 && p == pr ? imin(nlev, 14) : 15;
    D.lp = A && P ? dvar(A - P) : 9;
    D.hm = hm[ch];
    D.bd = b;
    D.cc = chan_class(ch);
    D.nb = bound[ch] && b ? clip((es[ch][b - 1] - es[ch][b]) / 2, -4, 4) + 4 : 9;
    if (cpl_leak(r, ch, b, 24, 8) >= 0) D.nb = cpl_leak(r, ch, b, 24, 8);
    const int spu = spk && p >= 0 && p == pr;
    v = scale(D, v, p, A, max, spu && p == p0, sg, spc, spp, spk);
    if (fc[0] >= 0) {
      U16* const w = ffs_w[imin(s.abits[ch][b], 12)][fb / 5][fb % 5];
      for (int i = 0; i < 4; i++) w[i] += v == fc[i];
      if (w[0] + w[1] + w[2] + w[3] >= 4096)
        for (int i = 0; i < 4; i++) w[i] >>= 1;
    }
    if (spu) p = spc[0];
    if (p >= 0) {
      hm[ch] = v == p ? 1 : 2;
      n_sfhit += v == p;
    }
    if (lw) jx_tally(jx_sr[b], v, pa, A - 1);
    if (pj >= 0) jx_tally(jx_jr[b], v, pp, pj);
    if (!j) pscale[ch][b] = p + 1;
    if (!j && cpl_on(r, ch, b)) {  // (each m's RMS rule)
      const int c = NSUB - 1 - b;
      for (int k = 0; k < CPL_K; k++) {
        S64 y[NSMP];
        int nr;
        cpl_values(c, ch, k, y, nsmp);
        cpl_hit[c][k] += scale_pred(y, nsmp, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &nr) == v;
      }
      if (++cpl_n[c] >= 1u << 16) {
        cpl_n[c] >>= 1;
        for (int k = 0; k < CPL_K; k++) cpl_hit[c][k] >>= 1;
      }
    }
    n_sf++;
    if (pr >= 0) {
      sfr[0] += v == pr, sfr[1] += v == pc;
      if (tried) {
        if (ps >= 0 && ps != pr) sfq[0][0] += v == pr, sfq[0][1] += v == ps;
        if (ps >= 0 && ps != pc) sfq[1][0] += v == pc, sfq[1][1] += v == ps;
        for (int k = 0; k < 2; k++)
          if (sfq[k][0] + sfq[k][1] >= 1u << 16) sfq[k][0] >>= 1, sfq[k][1] >>= 1;
        ss_n += ss_n < 2048;
        sfl[0] += v < pr - 3;
        if (++sfl[1] >= 1u << 16) sfl[0] >>= 1, sfl[1] >>= 1;
      }
      if (sfr[0] + sfr[1] >= 1u << 16) sfr[0] >>= 1, sfr[1] >>= 1;
    }
    return v;
  }

  // --- the subband samples ---------------------------------------------------------------------------

  // whether e (2^-AQ_F) is within half a unit
  static INLINE bool jx_near(S64 e) { return e < (S64)1 << (AQ_F - 1) && e > -((S64)1 << (AQ_F - 1)); }
  // a histogram's count of bin i (jx_h's, jx_hv's; its sum n), halved as they grow
  static void jx_count(U32* h, U32& n, int i, U64* hb = nullptr) {
    if (i < 0 || i > 255) return;
    h[i]++;
    if (hb) hb[i >> 6] |= 1ull << (i & 63);
    if (++n < 1u << 16) return;
    for (int k = 0; k < 256; k++) h[k] >>= 1;
    n >>= 1;
    if (hb)
      for (int k = 0; k < 256; k++)
        if (!h[k]) hb[k >> 6] &= ~(1ull << (k & 63));
  }
  // whether bin i or one beside it has been seen
  static INLINE bool jx_seen(const U32* h, int i) { return i >= 1 && i <= 254 && (h[i - 1] | h[i] | h[i + 1]); }
  // the core's gain over the decode in subband b (in 2^-12, to 2; 1 where unknown)
  S64 jx_gain(int b) const {
    if (jx_ww[b] <= 0) return 4096;
    const S128 g = shl128(jx_yw[b], 12) / jx_ww[b];
    return g < 0 ? 0 : g > 8192 ? 8192 : (S64)g;
  }
  // whether X96 codes the core's subband b too and the core has little of the decode's values there
  bool jx_low(int b) const { return x96a && b >= jx_lo && jx_gain(b) < JX_LOW; }
  // whether a tally's second guess has been right more often than its first by k and m 2^-4 of that
  static bool jx_more(const U32* c, U32 k, int m) { return c[1] > c[0] + k + (c[0] * m >> 4); }
  // a tally c of v against two guesses: c[0] where it is a, c[1] where it is l (where both are, -1
  // none, and differ)
  static void jx_tally(U32* c, S64 v, S64 a, S64 l) {
    if (a < 0 || l < 0 || a == l) return;
    if (v == a) c[0]++;
    else if (v == l) c[1]++;
    if (c[0] + c[1] >= 1u << 16) c[0] >>= 1, c[1] >>= 1;
  }
  // The core's index at a sample whose decode's value is w, at its step ss and ADPCM prediction
  // pred, that leaves the rest of w on X96's lattice of step s9 (within half a unit): of the indexes
  // within 2 of the gain al's (2^-12) and the nearest w's, the one leaving X96 the least index, then
  // the nearest the gain's (*rr: 64 times that index's magnitude plus its distance from the gain's;
  // *ok whether any did)
  static S64 jx_fit(S64 w, S32 pred, S64 ss, S64 lo, S64 hi, S64 s9, S64 al, S64* rr, int* ok) {
    S128 d;
    S64 o, cb = 0, sb = -1;
    const S64 ca = requant(((w >> 4) * al >> 8) - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
    for (int k = 0; k <= 5; k++) {
      const S64 c = k < 5 ? ca + k - 2 : requant(w - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
      if (c < lo || c > hi || (k == 5 && c >= ca - 2 && c <= ca + 2)) continue;
      const S64 res = w - shl64(clip23((S64)dequantize((S32)c, ss) + pred), AQ_F);
      const S64 r = requant(res, s9, -4095, 4095, &d, &o);
      if (!jx_near(res - shl64(dequantize((S32)r, s9), AQ_F))) continue;
      const S64 sc = (r < 0 ? -r : r) * 64 + (c < ca ? ca - c : c - ca);
      if (sb < 0 || sc < sb) sb = sc, cb = c, *rr = sc;
    }
    *ok = sb >= 0;
    return cb;
  }
  // (jx_fit()'s candidates at a sample, but for X96's step: the core's indexes it tries, the rests
  // they leave and the decoder's samples they make)
  struct JxCand {
    S64 res;
    S32 c, dc, y;  // (the index, its distance from the gain's, the decoder's sample)
  };
  struct JxTest {  // (jx_pick()'s test's parts, each candidate's: res in the decoder's units, its bound but
                   // for the step's part, the steps under which the test holds)
    double x[8], cb[8], lim[8];
  };
  static INLINE int jx_cands(S64 w, S32 pred, S64 ss, S64 lo, S64 hi, S64 al, JxCand* cd, JxTest& ct) {
    S128 d;
    S64 o;
    const S64 ca = requant(((w >> 4) * al >> 8) - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
    int n = 0;
    for (int k = 0; k <= 5; k++) {
      const S64 c = k < 5 ? ca + k - 2 : requant(w - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
      if (c < lo || c > hi || (k == 5 && c >= ca - 2 && c <= ca + 2)) continue;
      const S32 y = clip23((S64)dequantize((S32)c, ss) + pred);
      JxCand& e = cd[n];
      e.res = w - shl64(y, AQ_F), e.c = (S32)c, e.dc = (S32)(c < ca ? ca - c : c - ca), e.y = y;
      const double x = (double)e.res * (1.0 / 65536), ax = x < 0 ? -x : x;
      ct.x[n] = x, ct.cb[n] = 1.0 + 1e-6 + 4e-7 * ax, ct.lim[n] = 4194304.0 - ax;
      n++;
    }
    for (int i = n; i < 8; i++) ct.x[i] = 0, ct.cb[i] = 0, ct.lim[i] = 0;  // (the rest, which jx_pick() reads 4 at a time)
    return n;
  }
  // of the candidates cd (n of them), the one jx_fit() takes at X96's step k: its place in cd (-1
  // none), *rr its score. (A test in doubles first, whether the rest x can lie on X96's lattice of
  // step t, in the decoder's units: where it does, x lies within 1 + |q| t 2^-22 of q t, its index
  // q's point, by dequantize()'s rounding and shift, and |q| t is at most |x| + t / 2: so where it
  // lies 1 + (|x| + t) 4e-7 + 1e-6 away or more, it doesn't. Only at a step of 3 or more can x lie
  // that far; past 4095, and where clip23() could decide, the exact test's.)
  static INLINE int jx_pick(const JxCand* cd, const JxTest& ct, int n, int k, S64* rr) {
    const S64 s9 = jx_steps.v[k];
    const double it = jx_steps.it[k], t = jx_steps.t[k], tb = jx_steps.tb[k];
    U32 off = 0;  // (the candidates the test rules out, by bit: 4 at a time where AVX2 is, as the loop below)
    if (t >= 3) {
#ifdef DTSD_AVX2
      const __m256d vit = _mm256_set1_pd(it), vt = _mm256_set1_pd(t), vtb = _mm256_set1_pd(tb), half = _mm256_set1_pd(0.5),
                    qlo = _mm256_set1_pd(-4095.0), qhi = _mm256_set1_pd(4095.0), sgn = _mm256_set1_pd(-0.0);
      for (int h = 0; h < 8; h += 4) {
        const __m256d x = _mm256_loadu_pd(ct.x + h);
        const __m256d q = _mm256_floor_pd(_mm256_add_pd(_mm256_mul_pd(x, vit), half));
        const __m256d ad = _mm256_andnot_pd(sgn, _mm256_sub_pd(x, _mm256_mul_pd(q, vt)));
        const __m256d m = _mm256_and_pd(_mm256_and_pd(_mm256_cmp_pd(vt, _mm256_loadu_pd(ct.lim + h), _CMP_LT_OQ), _mm256_cmp_pd(q, qlo, _CMP_GE_OQ)),
                                        _mm256_and_pd(_mm256_cmp_pd(q, qhi, _CMP_LE_OQ), _mm256_cmp_pd(ad, _mm256_add_pd(_mm256_loadu_pd(ct.cb + h), vtb), _CMP_GE_OQ)));
        off |= (U32)_mm256_movemask_pd(m) << h;
      }
#else
      for (int i = 0; i < n; i++) {
        if (!(t < ct.lim[i])) continue;
        const double q = floor(ct.x[i] * it + 0.5);
        if (!(q >= -4095 && q <= 4095)) continue;
        const double d = ct.x[i] - q * t;
        if ((d < 0 ? -d : d) >= ct.cb[i] + tb) off |= 1u << i;
      }
#endif
    }
    S64 sb = -1;
    int ib = -1;
    for (int i = 0; i < n; i++) {
      if (off >> i & 1) continue;
      S128 d;
      S64 o;
      const S64 r = requant(cd[i].res, s9, -4095, 4095, &d, &o);
      if (!jx_near(cd[i].res - shl64(dequantize((S32)r, s9), AQ_F))) continue;
      const S64 sc = (r < 0 ? -r : r) * 64 + cd[i].dc;
      if (sb < 0 || sc < sb) sb = sc, ib = i;
    }
    *rr = sb;
    return ib;
  }
  // Channel ch's subband b's fit (jx_k, jx_vq) in the subframe, where its values aren't all on the
  // core's lattice (the subframe decoded with the indexes each one makes, its ADPCM predictions
  // from them): X96's step, of those whose ratio to the core's (to 1/16 octave or beside) has been
  // X96's in the subband, that leaves each sample's rest on its lattice with the least sum of
  // jx_fit()'s scores (the largest step of equals); else X96's vectors (jx_search_vq()). (The
  // scores' indexes' distances count: X96's step plus the core's makes the same values, the core's
  // index off by X96's (`96-24.dts`'s subband 30, a quarter of its subframes' steps, its core 5.6%
  // bigger by the X96 indexes alone).)
  void jx_search(const CoreRec& r, const CoreSub& s, int ch, int b, S64 lo, S64 hi) {
    S64 ss[NSMP];
    for (int j = 0; j < nsmp; j++) ss[j] = step_scale(r, s, ch, b, j >> 3);
    jx_try(s, ch, b, ss, lo, hi, &jx_k[ch][b], jx_vq[ch][b], jx_vs[ch][b]);
  }
  // (the fit at the core's steps ss, into *k9, vq and vs: 0 none, 1 all on the core's lattice, 2
  // X96's step, 3 its vectors, unless novq)
  int jx_try(const CoreSub& s, int ch, int b, const S64* ss, S64 lo, S64 hi, int* k9, int* vq, int* vs, bool novq = false) {
    const int pm = s.pmode[ch][b], n = nsmp;
    const S64* w = X[ch][b];
    *k9 = 0;
    for (int q = 0; q < NSMP / 16; q++) vq[q] = 0;
    if (ss[0] <= 0) return 0;
    S32 h[4];
    memcpy(h, yh[ch][b], sizeof h);
    bool plain = true;
    for (int j = 0; j < n && plain; j++) {
      const S32 pred = pm ? adpcm_pred(s.pvq[ch][b], h) : 0;
      S128 d;
      S64 o;
      const S64 c = requant(w[j] - shl64(pred, AQ_F), ss[j], lo, hi, &d, &o);
      const S32 y = clip23((S64)dequantize((S32)c, ss[j]) + pred);
      plain = jx_near(w[j] - shl64(y, AQ_F));
      h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
    }
    if (plain) return 1;  // (X96 adds nothing there)
    const int l0 = lg16((U64)ss[0]);
    const S64 al = jx_gain(b);
    S64 best = -1, bs9 = 0;
    // (each sample's candidates made once: where there's no ADPCM prediction, they are the same at
    // every step; else where the samples before were decoded the same, as the first one's always
    // are)
    JxCand cd[NSMP][6];
    JxTest ct[NSMP];
    S32 ck[NSMP][4];
    int nc[NSMP];
    for (int j = 0; j < n; j++) nc[j] = -1;
    // (the steps by their lg16(), ascending: those a seen bin or one beside it holds; the same
    // fit as all of them in order would make, the scores' sums only growing)
    // (the bins i, 1 to 254, where i - 1, i or i + 1 isn't 0: jx_hb's bits and those beside them)
    U64 sb[4];
    for (int x = 0; x < 4; x++) {
      const U64 m = jx_hb[b][x];
      sb[x] = m | m << 1 | m >> 1 | (x ? jx_hb[b][x - 1] >> 63 : 0) | (x < 3 ? jx_hb[b][x + 1] << 63 : 0);
    }
    sb[0] &= ~1ull, sb[3] &= ~(1ull << 63);
    for (int x = 0; x < 4 && jx_hn[b]; x++)
     for (U64 bits = sb[x]; bits; bits &= bits - 1) {
      const int i = 64 * x + __builtin_ctzll(bits);
      const int l = i + l0 - 128 - jx_steps.lg0;
      if (l < 0 || l >= jx_steps.nl) continue;
      for (int z = jx_steps.at[l]; z < jx_steps.at[l + 1]; z++) {
        const int k = jx_steps.by[z];
        const S64 s9 = jx_steps.v[k];
        memcpy(h, yh[ch][b], sizeof h);
        S64 tot = 0;
        int j = 0;
        for (; j < n; j++) {
          if (nc[j] < 0 || (pm && memcmp(ck[j], h, sizeof h))) {
            nc[j] = jx_cands(w[j], pm ? adpcm_pred(s.pvq[ch][b], h) : 0, ss[j], lo, hi, al, cd[j], ct[j]);
            memcpy(ck[j], h, sizeof h);
          }
          S64 rr;
          const int ib = jx_pick(cd[j], ct[j], nc[j], k, &rr);
          if (ib < 0) break;
          tot += rr;
          if (best >= 0 && tot > best) break;
          const S32 y = cd[j][ib].y;
          h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
        }
        if (j == n && (best < 0 || tot < best || s9 > bs9)) best = tot, bs9 = s9, *k9 = k + 1;
      }
    }
    if (*k9) return 2;
    return !novq && jx_hvn[b] && jx_search_vq(s, ch, b, lo, hi, ss, l0, vq, vs) ? 3 : 0;
  }
  // X96's vector of each 16 samples (an entry of its code book at a scale factor whose ratio to the
  // core's step has been X96's in the subband, or beside) whose values leave each sample's rest on
  // the core's lattice: the first such, by the first sample's entry values first (into vq and vs;
  // whether each has one, none after one none fits)
  bool jx_search_vq(const CoreSub& s, int ch, int b, S64 lo, S64 hi, const S64* ss, int l0, int* vq, int* vs) {
    const int pm = s.pmode[ch][b], n = nsmp;
    const S64* w = X[ch][b];
    S32 h[4];
    memcpy(h, yh[ch][b], sizeof h);
    for (int q = 0; q < n / 16; q++) {
      const int j0 = 16 * q;
      for (int si = 0; si < 64 && !vq[q]; si++) {
        if (!jx_seen(jx_hv[b], lg16((U64)SF6[si] << 22) - l0 + 128)) continue;
        if (!pm) {  // (without ADPCM prediction, each sample's test by its entry's value there alone:
                    // of the entries whose values pass at every place, the first, as trying them in
                    // order finds; each value tried where an entry still in has it)
          if (!hf_places.made) hf_places.make();
          U64 in[16];
          for (int x = 0; x < 16; x++) in[x] = ~0ull;
          bool any = true;
          for (int j = 0; j < 16 && any; j++) {
            alignas(32) U64 mj[16] = {0};
#ifdef DTSD_AVX2  // (the masks 4 words at a time)
            const __m256i i0 = _mm256_loadu_si256((const __m256i*)in), i1 = _mm256_loadu_si256((const __m256i*)(in + 4)),
                          i2 = _mm256_loadu_si256((const __m256i*)(in + 8)), i3 = _mm256_loadu_si256((const __m256i*)(in + 12));
            __m256i m0 = _mm256_setzero_si256(), m1 = m0, m2 = m0, m3 = m0;
#endif
            for (int e = 0; e < hf_places.nv[j]; e++) {
              const U64* const me = hf_places.m[j][e];
#ifdef DTSD_AVX2
              const __m256i e0 = _mm256_loadu_si256((const __m256i*)me), e1 = _mm256_loadu_si256((const __m256i*)(me + 4)),
                            e2 = _mm256_loadu_si256((const __m256i*)(me + 8)), e3 = _mm256_loadu_si256((const __m256i*)(me + 12));
              const __m256i hv = _mm256_or_si256(_mm256_or_si256(_mm256_and_si256(e0, i0), _mm256_and_si256(e1, i1)),
                                                 _mm256_or_si256(_mm256_and_si256(e2, i2), _mm256_and_si256(e3, i3)));
              if (_mm256_testz_si256(hv, hv)) continue;
#else
              U64 hit = 0;
              for (int x = 0; x < 16; x++) hit |= me[x] & in[x];
              if (!hit) continue;
#endif
              const S64 u = w[j0 + j] - shl64(clip23(((S64)hf_places.v[j][e] * SF6[si] + 8) >> 4), AQ_F);
              S128 d;
              S64 o;
              const S64 c = requant(u, ss[j0 + j], lo, hi, &d, &o);
              if (!jx_near(u - shl64(clip23((S64)dequantize((S32)c, ss[j0 + j])), AQ_F))) continue;
#ifdef DTSD_AVX2
              m0 = _mm256_or_si256(m0, e0), m1 = _mm256_or_si256(m1, e1), m2 = _mm256_or_si256(m2, e2), m3 = _mm256_or_si256(m3, e3);
#else
              for (int x = 0; x < 16; x++) mj[x] |= me[x];
#endif
            }
#ifdef DTSD_AVX2
            _mm256_store_si256((__m256i*)mj, m0), _mm256_store_si256((__m256i*)(mj + 4), m1);
            _mm256_store_si256((__m256i*)(mj + 8), m2), _mm256_store_si256((__m256i*)(mj + 12), m3);
#endif
            U64 left = 0;
            for (int x = 0; x < 16; x++) in[x] &= mj[x], left |= in[x];
            any = left != 0;
          }
          for (int x = 0; x < 16 && any && !vq[q]; x++)
            if (in[x]) vq[q] = 64 * x + __builtin_ctzll(in[x]) + 1, vs[q] = si;
          continue;
        }
        const S32 p0 = pm ? adpcm_pred(s.pvq[ch][b], h) : 0;
        // (the entries whose first value leaves the first sample's rest on the lattice, in order)
        U64 mk[16] = {0};
        for (int e = 0; e < hf_first.nv; e++) {
          const int e8 = hf_first.v[e];
          const S64 u = w[j0] - shl64(clip23(((S64)(e8 - 128) * SF6[si] + 8) >> 4), AQ_F);
          S128 d;
          S64 o;
          const S64 c = requant(u - shl64(p0, AQ_F), ss[j0], lo, hi, &d, &o);
          if (jx_near(u - shl64(clip23((S64)dequantize((S32)c, ss[j0]) + p0), AQ_F)))
            for (int x = 0; x < 16; x++) mk[x] |= hf_first.m[e][x];
        }
        for (int x = 0; x < 16 && !vq[q]; x++)
          for (U64 bits = mk[x]; bits && !vq[q]; bits &= bits - 1) {
            const int k = 64 * x + __builtin_ctzll(bits);
            S32 g[4];
            memcpy(g, h, sizeof g);
            int j = j0;
            for (; j < j0 + 16; j++) {
              const S32 pred = pm ? adpcm_pred(s.pvq[ch][b], g) : 0;
              const S64 u = w[j] - shl64(clip23(((S64)HF_VQ[k][j - j0] * SF6[si] + 8) >> 4), AQ_F);
              S128 d;
              S64 o;
              const S64 c = requant(u - shl64(pred, AQ_F), ss[j], lo, hi, &d, &o);
              const S32 y = clip23((S64)dequantize((S32)c, ss[j]) + pred);
              if (!jx_near(u - shl64(y, AQ_F))) break;
              g[3] = g[2], g[2] = g[1], g[1] = g[0], g[0] = y;
            }
            if (j == j0 + 16) vq[q] = k + 1, vs[q] = si, memcpy(h, g, sizeof h);
          }
      }
      if (!vq[q]) return false;
    }
    return n >= 16;
  }

  // Sample j of channel ch's subband b (its index v in lo..hi): whether it is the index the audio's
  // subband sample quantizes to, whether the one on the other side of the rounding boundary, else
  // its magnitude and sign by the subband's indexes before; then the decoder's sample, for the
  // ADPCM predictions after it
  S64 code_q(const CoreRec& r, const CoreSub& s, int ch, int b, int j, S64 v, S64 lo, S64 hi) {
    if (!hc_dec && ss_rule()) {
      hc_dec = 1;
      for (int k = 0; k < NSUB; k++) hcal[k].s = hcal[k].s < HC_S0 ? HC_S0 : hcal[k].s;
    }
    const int a = s.abits[ch][b], pm = s.pmode[ch][b];
    const S64 ss = step_scale(r, s, ch, b, j >> 3);
    const S32 pred = pm ? adpcm_pred(s.pvq[ch][b], yh[ch][b]) : 0;
    tw_pr[ch][b][j & 7] = pred;
    S64 qp = 0, alt = 0;
    S128 dist = 0;
    const int bnd = bound[ch] && !past_end(t0 + j);  // (`s.2.dts`'s first 3 MB 5.0% smaller, cut streams 0.5-5.7%)
    // (where X96 codes the subband too, the audio's value as the core's: the decode's less X96's
    // part as its fit has it, where that has been the core's index clearly more often than the
    // nearest: `96-24.dts`'s core 26% smaller)
    S64 xv = X[ch][b][j], qf = 0;
    int fx = 0;  // (whether a fit says an index: qf)
    if (x96a && j == 0) {
      jx_cs[&s - r.sub][ch][b] = ss;  // (s: r.sub's)
      jx_k[ch][b] = 0;
      for (int q = 0; q < NSMP / 16; q++) jx_vq[ch][b][q] = 0;
      if (bnd && b >= jx_lo) jx_search(r, s, ch, b, lo, hi);
    }
    if (x96a && bnd && jx_k[ch][b]) {
      S64 rr;
      int ok;
      const S64 c = jx_fit(xv, pred, ss, lo, hi, jx_steps.v[jx_k[ch][b] - 1], jx_gain(b), &rr, &ok);
      if (ok) fx = 1, qf = c;
    } else if (x96a && bnd && j < 16 * (NSMP / 16) && jx_vq[ch][b][j >> 4]) {
      const int k = jx_vq[ch][b][j >> 4] - 1;
      const S64 u = xv - shl64(clip23(((S64)HF_VQ[k][j & 15] * SF6[jx_vs[ch][b][j >> 4]] + 8) >> 4), AQ_F);
      S128 d;
      S64 o;
      const S64 c = requant(u - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
      if (jx_near(u - shl64(clip23((S64)dequantize((S32)c, ss) + pred), AQ_F))) fx = 1, qf = c;
    }
    S64 q0 = 0, qb = 0, f0[B0_N];
    // (a decoded WAV's bottom subband: the fit's value; `96-24.dts`'s indexes there 99.9% the audio's
    // from 94.5%, its core 2.3% smaller, `ES 6.1 16bit.dts`'s 3.4%)
    const int b0 = (x96a || ss_rule()) && bnd && b == 0;
    if (b0) {
      for (int k = 0; k < 16; k++) f0[k] = b0y[ch][k];
      f0[16] = xv;
      for (int k = 0; k < 8; k++) f0[17 + k] = b0w[ch][k];
      for (int k = 0; k < B0_F; k++) f0[25 + k] = j + 1 + k < s.nssc * 8 ? X[ch][0][j + 1 + k] : 0;
      const S64 xc = xv - b0fit.predict(f0);
      S128 d;
      S64 o;
      qb = requant(xc - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
      if (jx_more(b0r, 4, 1)) xv = xc;
    }
    // (a lossless WAV's: with its bias, m 2^-25 of the bottom subband's value, bias_learn()'s)
    const bool lk = bnd && b > 0 && !ss_rule() && !x96a && !fx;
    const S64 xu = xv;
    if (lk) xv += rshift(X[ch][0][j] * lk_c[b], 25);
    if (bnd) q0 = qp = requant(xv - shl64(pred, AQ_F), ss, lo, hi, &dist, &alt);
    if (b0) {  // (the decode's own index, for the tally)
      S128 d;
      S64 o;
      q0 = requant(X[ch][b][j] - shl64(pred, AQ_F), ss, lo, hi, &d, &o);
    }
    if (fx && jx_more(jx_r[b], 4, 1)) {  // (the fit's index, as the audio's: on its lattice point)
      xv = shl64(clip23((S64)dequantize((S32)qf, ss) + pred), AQ_F);
      qp = requant(xv - shl64(pred, AQ_F), ss, lo, hi, &dist, &alt);
    }
    // (a twin's index as its earlier channel's, far from a boundary, where the ABITS, the step and
    // the prediction are the same: the same value quantized the same way; in a pass the earlier
    // channel is in, bound[]'s, which has coded the subsubframe's samples of the subband. Else,
    // where the earlier channel's step is no coarser, its decoded value as the audio's, where that
    // has been the index clearly more often: `cx_xbr_xxch_71_24_48_3840.dts` 2770 bytes to 2734,
    // the twins of lossless WAVs' streams as they were)
    const int tw = twin_of(ch);
    S64 qa = 0, qt = 0;
    int tq = 0;
    if (bnd && tw >= 0 && bound[tw] && s.abits[tw][b] == a && step_scale(r, s, tw, b, j >> 3) == ss && tw_pr[tw][b][j & 7] == pred) {
      qp = s.q[tw][b][j];
      alt = qp + (qp >= 0 ? 1 : -1);
      dist = (S128)1 << 100;
    } else if (bnd && tw >= 0 && bound[tw] && b < r.nvqsub[tw] && s.abits[tw][b] && step_scale(r, s, tw, b, j >> 3) <= ss) {
      S128 dt;
      S64 at;
      qa = qp;
      qt = requant(shl64(tw_y[tw][b][j & 7], AQ_F) - shl64(pred, AQ_F), ss, lo, hi, &dt, &at);
      tq = 1;
      if (jx_more(tw_t, 4, 1)) qp = qt, dist = dt, alt = at;
    }
    // (since format 138 an FFmpeg encode's index as its encoder quantizes the values it makes from
    // the WAV, at the core's scale factor and ABITS (ffenc.inc's transform, its quantize_value()):
    // where the WAV is the encoder's input, the core's every time; taken, sure, where it has been
    // all but always and more often than the audio's, as it is not for the encode's own decodes;
    // else where it has been more often where the two differ, taken with the one next to it toward
    // the audio's as the other (a 16-bit decode's of its encode of a 16-bit WAV, whose ABITS are its
    // again: 445344 bytes to 381290))
    S64 qff = 0, qfa = 0, qfr = 0;
    const S64 qm = qp;
    int hq = 0, hr = 0;
    if (ffe_ok && !r.xpass && bnd && !pm && a > 0 && a <= 26 && r.shuff[ch] == 6 && j < FfEnc::NS) {
      const int ns = s.scale[ch][b][0] & 127, e = FE_SF_INV[ns].e + FE_STEP_INV[a].e - 17;
      if (e >= 1 && e <= 30) {
        const FfSoft qq{fe_mul32(FE_SF_INV[ns].m, FE_STEP_INV[a].m), e};
        qff = fe_quant(ffe->sub[ch][b][j], qq);
        qff = qff < lo ? lo : qff > hi ? hi : qff;
        hq = 1;
        if (ffr_ok) {
          qfa = qff, qfr = fe_quant(ffr->sub[ch][b][j], qq), hr = 1;
          qfr = qfr < lo ? lo : qfr > hi ? hi : qfr;
          if (jx_more(ffr_c, 64, 16)) qff = qfr;
        }
        if (qff_on()) qp = qff, alt = qff + (qff >= 0 ? 1 : -1), dist = (S128)1 << 100;
        else if (qff != qp && jx_more(qff_c, 4, 1)) alt = qff + (qp > qff ? 1 : -1), qp = qff;
        if (hr && qfa != qfr && qp == qff && jx_more(ffr_l, 64, 16)) alt = qff == qfr ? qfa : qfr;
      }
    }
    // (a sample past the WAV's end coded without it: its continuation's index as context)
    const int pe = bound[ch] && !bnd;
    S64 qc = 0;
    if (pe) {
      S128 dc;
      S64 ac;
      qc = requant(xv - shl64(pred, AQ_F), ss, lo, hi, &dc, &ac);
    }
    // (the audio's with the decoded values' running difference from it added, but for half a step
    // of it (a step is ss / 64 in 2^-AQ_F), where that has been the index more often by 16 (by 0,
    // the short streams 92 bytes bigger, the noise at `3-1.dts`'s start enough to switch; by 16,
    // as they were, the Cymatics stream 1455 bytes smaller): not a twin's sure one)
    const int cq = bnd && dist != (S128)1 << 100;
    S64 qk = 0;
    const S64 qp0 = qp;
    if (cq) {
      S128 dk;
      S64 ak;
      const S64 u = ss >> 7, c = qcar[ch][b], cc = c > u ? c - u : c < -u ? c + u : 0;
      qk = cc ? requant(xv + cc - shl64(pred, AQ_F), ss, lo, hi, &dk, &ak) : qp;
      if (cc && qcn[ch][b][1] > qcn[ch][b][0] + 16) qp = qk, dist = dk, alt = ak;
    }
    M_smp_v D;
    D.ab = a;
    D.hw = sample_coding(r, ch, a);
    D.pk = bnd;
    D.nr = bnd ? near_bucket(dist, (S128)ss << AQ_F, 17) : 0;
    // (the encoder's arithmetic errs by about its signal's level: the channel's loudest subband's;
    // a decoded WAV's nearness against that level floored at 2^DW_NL, its hit cells' (code_index())
    // against its rounding's unit, DW_HC doublings above its LSB)
    const S128 lvu = (S128)(r.shuff[ch] == 6 ? SF7 : SF6)[imax(esmax[ch] - 1, 0)] << 38;
    const bool dw = ss_rule();
    D.nl = bnd ? near_bucket(dist, dw ? lvu + ((S128)1 << (38 + DW_NL)) : lvu, 28) : 0;
    D.ql = bnd ? imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21) : pe ? imin(bit_len((U64)(qc < 0 ? -qc : qc)), 21) : 0;
    D.hm = imin(__builtin_popcount(miss[ch][b]), 3);
    D.pm = pm;
    D.bd = b;
    D.m1 = q1[ch][b] ? imin(q1[ch][b] - 1, 21) : 22;
    D.m2 = q2[ch][b] ? imin(q2[ch][b] - 1, 21) : 22;
    D.sq = bnd ? svar(qp) : pe ? svar(qc) : 0;
    D.s1 = qs[ch][b];
    int how;
    // (cb: how many of the channel's last 32 missed, 0, 1-2, 3-7, more (0, 1, 2-3, more before format
    // 74): where the WAV isn't what the encoder had, as in the frames after a cut stream's start (its
    // decoder's state not the encoder's), nearly all of a channel's indexes miss by a step or two
    // for a while; its subbands' own last 8 tell it 32 times later)
    const int pc = __builtin_popcount(chm[ch]);
    // (where the core has little of the decode's values, the index of the fit's value (je_one()),
    // where it isn't the audio's and has been the core's more than 3/4 of the time, the second guess,
    // and the hit's cell by whether it is: `96-24.dts`'s subband 30's misses 0.6% where it is, 9.2%
    // where not, and then that one 99.5% of the time; its core 1.1% smaller)
    int jd = 0;
    S64 qe = 0;
    const S64 xid = tframe * 8 + (&s - r.sub);
    const int xm = bnd && xm_id[b][j] == xid ? xm_cnt[b][j] : 0;
    const bool je = bnd && x96a && ss_rule() && jx_low(b) && je_on(ch, b);
    if (je) {
      S128 d1;
      S64 o1;
      qe = requant(je_one(ch, b, j) - shl64(pred, AQ_F), ss, lo, hi, &d1, &o1);
      if (qe != qp && (U64)je_qn[b][1] * 4 > (U64)je_qn[b][0] * 3) jd = 1, alt = qe;
    }
    // (a lossless WAV's nearness against the level at the sample, at the core's rate the level along
    // the subband's FFT path)
    const S64 lv = !dw ? bind.up == 1 ? plev[ch][b][j] : tlev[ch][j] : 0;
    const S128 hu = !dw ? (S128)lv << 22 : (S128)(wav->bps == 2 ? 256 : 1) << (38 + DW_HC);
    v = code_index(D, v, bnd, qp, alt, lo, hi, &how, &hcal[b], dist, hu,
                   (pc >= 8 ? 3 : pc >= 3 ? 2 : pc >= 1 ? 1 : 0) + 4 * jd, bnd ? (S128)ss << AQ_F : 0,
                   __builtin_popcount(miss[ch][b]), xm > 0);
    if (lk) bias_learn(b, xu, X[ch][0][j], shl64(pred, AQ_F), ss, lo, hi, v, qp, alt, dist);
    if (bnd && how != 0) {
      if (xm_id[b][j] != xid) xm_id[b][j] = xid, xm_cnt[b][j] = 0;
      xm_cnt[b][j] += xm_cnt[b][j] < 255;
    }
    if (je) {
      je_qn[b][0]++, je_qn[b][1] += qe == v;
      if (je_qn[b][0] >= 1u << 16) je_qn[b][0] >>= 1, je_qn[b][1] >>= 1;
    }
    if (hq) {
      if (qff != qm) qff_c[0] += v == qm, qff_c[1] += v == qff;
      qff_n[0]++, qff_n[1] += v == qff;
      if (qff_c[0] + qff_c[1] >= 1u << 16) qff_c[0] >>= 1, qff_c[1] >>= 1;
      if (qff_n[0] >= 1u << 16) qff_n[0] >>= 1, qff_n[1] >>= 1;
    }
    if (hr) {
      if (qfa != qfr) ffr_c[0] += v == qfa, ffr_c[1] += v == qfr;
      if (ffr_c[0] + ffr_c[1] >= 1u << 12) ffr_c[0] >>= 1, ffr_c[1] >>= 1;
      if (qfa != qfr) ffr_l[0] += v == qfa, ffr_l[1] += v == qfr;
      if (ffr_l[0] + ffr_l[1] >= 1u << 16) ffr_l[0] >>= 1, ffr_l[1] >>= 1;
    }
    miss[ch][b] = (U8)(miss[ch][b] << 1 | (bnd && how != 0));
    if (bnd) chm[ch] = chm[ch] << 1 | (how != 0);
    if (bnd && bind.alt[ch]) {  // (gain_pick()'s counts: the index each pair's audio makes as it is)
      S128 dg;
      S64 ag;
      gh[ch][0] += requant(X[ch][b][j] - shl64(pred, AQ_F), ss, lo, hi, &dg, &ag) == v;
      gh[ch][1] += requant(XO[ch][b][j] - shl64(pred, AQ_F), ss, lo, hi, &dg, &ag) == v;
    }
    n_smp++;
    n_hit += how == 0;
    q2[ch][b] = q1[ch][b];
    q1[ch][b] = 1 + bit_len((U64)(v < 0 ? -v : v));
    qs[ch][b] = (S8)svar(v);
    const S32 y = clip23((S64)dequantize((S32)v, ss) + pred);
    if (cq) {
      U32* const n = qcn[ch][b];
      n[0] += qp0 == v, n[1] += qk == v;
      if (n[0] + n[1] >= 1u << 8) n[0] >>= 1, n[1] >>= 1;
      qcar[ch][b] += (shl64(y, AQ_F) - xv - qcar[ch][b]) >> 1;
    }
    push_y(ch, b, y);
    tw_y[ch][b][j & 7] = y;
    if (tq && qa != qt) {
      if (v == qa) tw_t[0]++;
      else if (v == qt) tw_t[1]++;
      if (tw_t[0] + tw_t[1] >= 1u << 16) tw_t[0] >>= 1, tw_t[1] >>= 1;
    }
    if (x96a) yc[ch][b][fb0 + j] = y;
    if (fx) jx_tally(jx_r[b], v, q0, qf);
    if (b0) {  // (the fit, and the last values)
      jx_tally(b0r, v, q0, qb);
      b0fit.add(f0, X[ch][0][j] - shl64(y, AQ_F), ss >> 6 > (S64)1 << 22 ? ss >> 6 : (S64)1 << 22);
      if (++b0n >= 64) b0fit.refit(), b0n = 0;
      for (int k = 15; k > 0; k--) b0y[ch][k] = b0y[ch][k - 1];
      for (int k = 7; k > 0; k--) b0w[ch][k] = b0w[ch][k - 1];
      b0y[ch][0] = shl64(y, AQ_F), b0w[ch][0] = X[ch][0][j];
    }
    if (x96a && bnd) {  // (the gain, forgetting as it grows)
      const S64 w = X[ch][b][j];
      jx_yw[b] += (S128)y * w, jx_ww[b] += (S128)w * (w >> AQ_F);
      if (jx_ww[b] > (S128)1 << 100) jx_yw[b] /= 2, jx_ww[b] /= 2;
    }
    return v;
  }
  // An index v (in lo..hi), smp's variables D: whether it is the audio's qp (where pk), whether the
  // one on the other side of the rounding boundary (alt), else its magnitude and sign; *how which
  // (0 the audio's, 1 the other side's, 2 coded)
  // (nt: how near the boundary against the channel's level at the sample, a lossless WAV's: the
  // encoder's arithmetic errs by about the level of all its signal there, its loudest subband's
  // the lesser guess; -1 none: against that, D.nl; su, the unit of D.nr's nearness against the
  // step, 0 none)
  // (hit's and smp's, or hid's and smd's where the WAV is decoded: dw_rule())
  S64 code_index(const M_smp_v& D, S64 v, int pk, S64 qp, S64 alt, S64 lo, S64 hi, int* how, HitCal* hc = nullptr,
                 S128 dist = 0, S128 unit = 0, int cb = 0, S128 su = 0, int hn = -1, bool xm = false) {
    if (!dw_rule()) return code_ix<M_hit_v>(f_hit, f_smp, LV::smp::mt, D, v, pk, qp, alt, lo, hi, how, hc, dist, unit, cb, su, hn, xm);
    M_smd_v E;
#define M_COPY(f) E.f = D.f;
    M_smp_VARS(M_COPY)
#undef M_COPY
    return code_ix<M_hid_v>(f_hid, f_smd, LV::smd::mt, E, v, pk, qp, alt, lo, hi, how, hc, dist, unit, cb, su, hn, xm);
  }
  template <class VH, class FH, class FS, class VS>
  S64 code_ix(FH& fh, FS& fs, int mt, const VS& D, S64 v, int pk, S64 qp, S64 alt, S64 lo, S64 hi, int* how, HitCal* hc,
              S128 dist, S128 unit, int cb, S128 su, int hn, bool xm) {
    *how = 2;
    bool far = false;  // (a miss past HIT_NEAR: by distance or by magnitude, fm_c's)
    if (pk) {
      const int kd = D.ab == 27 ? 2 : D.pm;
      VH H;
      H.pm = kd;
      H.nr = D.nr;
      H.nl = D.nl;
      H.ab = D.ab;
      H.ql = D.ql;
      H.hm = D.hm;
      int hit;
      if (kd < 2) {
        // a subband sample's index is the audio's but where the encoder's arithmetic and this
        // analysis's differ, by how near the boundary it lies: by how often an index this near
        // wasn't (the cell's counts, from its row's where it has few), straight to the coder (its
        // probabilities are finer than the families'; the nearness against the level, unit, times
        // the subband's error's scale, hc's, where the channel's last 32 missed 3 times or fewer:
        // the encoder's arithmetic's misses, not those of a WAV that isn't what it had)
        // (the nearnesses with their fractions, near_frac()'s: the cells at their floors and the
        // next ones, each by how near; the probability their rates' geometric mean, each cell's
        // (misses + HIT_PRIOR times its row's (misses + 1/2) / (all + 1)) / (all + HIT_PRIOR), the
        // row's by the nearness against the level alone; each counts the flag by its weight, in
        // 256ths. Within a doubling the rate changes several times over: `s.2.dts`'s core 0.7%
        // smaller, FATE's samples' 1.3-2.6%; the subbands' error scales, which the scale factors'
        // nearness goes by too, learn from these probabilities)
        const bool hk = hc && (cb & 3) < 3;
        int i0 = D.nl, w1 = 0, j0 = D.nr, v1 = 0;
        const int ld = lg16n(dist);
        if (hc) near_frac(ld, lg16n(hc_unit(unit, hk ? hc->s : 0)), 28, &i0, &w1);
        if (su > 0) near_frac(ld, lg16n(su), 16, &j0, &v1);
        const int i1 = i0 + (w1 > 0), j1 = j0 + (v1 > 0), w0 = 16 - w1, v0 = 16 - v1;
        // (the cells' classes of the subband's last 8: 0, 1-3, 4-6, more, where hn says how many
        // missed; D.hm's 0, 1, 2, more where not, X96's: the core's 0.04-0.2% smaller, FATE's
        // samples 0.4-1.6%)
        const int hq = hn < 0 ? D.hm : hn >= 7 ? 3 : hn >= 4 ? 2 : hn >= 1 ? 1 : 0;
        const U32 cw[4] = {(U32)(v0 * w0), (U32)(v0 * w1), (U32)(v1 * w0), (U32)(v1 * w1)}, rw[2] = {16u * w0, 16u * w1};
        // (a class's cells' probability of a miss, its fast and slow counts' and its rows' log2s,
        // and their counts after the flag)
        S64 cl_f = 0, cl_s = 0, cl_r = 0;
        auto cprob = [&](int cl) {
          U32* const ce[4] = {hit_c[kd][hq][cl][j0][i0], hit_c[kd][hq][cl][j0][i1], hit_c[kd][hq][cl][j1][i0],
                              hit_c[kd][hq][cl][j1][i1]};
          U32* const ro[2] = {hit_r[kd][hq][cl][i0], hit_r[kd][hq][cl][i1]};
          const U64 rd[2] = {2 * (U64)ro[0][1] + 2 * HIT_ONE, 2 * (U64)ro[1][1] + 2 * HIT_ONE},
                    rn[2] = {HIT_PRIOR * HIT_ONE * (2 * (U64)ro[0][0] + HIT_ONE), HIT_PRIOR * HIT_ONE * (2 * (U64)ro[1][0] + HIT_ONE)};
          S64 lf = 0, ls = 0, lr = 0;  // (the weights' sums of the rates' log2, 2^-16 units)
          for (int k = 0; k < 4; k++) {
            if (!cw[k]) continue;
            const U32* ck = ce[k];
            const U64 r = rd[k & 1];
            lf += (S64)cw[k] * (lg2tab.log2x((U64)ck[0] * r + rn[k & 1]) - lg2tab.log2x(((U64)ck[1] + HIT_PRIOR * HIT_ONE) * r));
            ls += (S64)cw[k] * (lg2tab.log2x((U64)ck[2] * r + rn[k & 1]) - lg2tab.log2x(((U64)ck[3] + HIT_PRIOR * HIT_ONE) * r));
          }
          for (int k = 0; k < 2; k++)
            if (rw[k]) lr += (S64)rw[k] * (lg2tab.log2x(2 * (U64)ro[k][0] + 1) - lg2tab.log2x(2 * (U64)ro[k][1] + 2));
          cl_f = lf >> 8, cl_s = ls >> 8, cl_r = lr >> 8;
          return lg2tab.prob((lf + ls) >> 9);
        };
        auto cupd = [&](int cl, int ht) {
          U32* const ce[4] = {hit_c[kd][hq][cl][j0][i0], hit_c[kd][hq][cl][j0][i1], hit_c[kd][hq][cl][j1][i0],
                              hit_c[kd][hq][cl][j1][i1]};
          U32* const ro[2] = {hit_r[kd][hq][cl][i0], hit_r[kd][hq][cl][i1]};
          for (int k = 0; k < 4; k++)
            if (cw[k]) {
              ce[k][0] += ht ? 0 : cw[k], ce[k][2] += ht ? 0 : cw[k];
              if ((ce[k][1] += cw[k]) >= HIT_ONE * HIT_LIMIT) ce[k][0] >>= 1, ce[k][1] >>= 1;
              if ((ce[k][3] += cw[k]) >= HIT_ONE * HIT_SLIMIT) ce[k][2] >>= 1, ce[k][3] >>= 1;
            }
          for (int k = 0; k < 2; k++)
            if (rw[k]) {
              ro[k][0] += ht ? 0 : rw[k];
              if ((ro[k][1] += rw[k]) >= HIT_ONE * HIT_RLIMIT) ro[k][0] >>= 1, ro[k][1] >>= 1;
            }
        };
        // (where a channel coded before missed at this sample, the cells of their own, cb + 8, while
        // they have cost less: xm_loss)
        const U32 p0 = cprob(cb);
        S64 mf = cl_f, ms = cl_s, mr = cl_r;
        const U32 p1 = xm ? cprob(cb + 8) : 0;
        const bool ux = xm && xm_loss[1] + ((S64)4 << 16) + xm_loss[0] / 32 < xm_loss[0];
        if (ux) mf = cl_f, ms = cl_s, mr = cl_r;
        const U32 pm = ux ? p1 : p0;
        // (the rates mixed by the weights for the mean's log2, and the subband's)
        const double mx[4] = {(double)mf / 65536.0, (double)ms / 65536.0, (double)mr / 65536.0, 1.0};
        int mxb = (int)(((S64)lg2tab.log2x(CO_ONE) - (S64)lg2tab.log2x(pm)) >> 16);
        mxb = mxb > HAP_N - 1 ? HAP_N - 1 : mxb;
        double* const md = hmx_d[mxb];
        double* const mb = hmx_b[hc ? (int)(hc - hcal) : NSUB + NSUB96][mxb];
        double xs = (0.5 + md[0] + mb[0]) * mx[0] + (0.5 + md[1] + mb[1]) * mx[1] + (md[2] + mb[2]) * mx[2] + (md[3] + mb[3]) * mx[3];
        xs = xs > -0.05 ? -0.05 : xs < -19.0 ? -19.0 : xs;
        const U32 pq = lg2tab.prob((S64)(xs * 65536.0));
        const S64 hap_l = (S64)lg2tab.log2x(pq) - (S64)lg2tab.log2x(CO_ONE);  // (its log2, 2^-16)
        int hap_b = (int)(-hap_l >> 16), hap_f = (int)(-hap_l >> 8) & 255;  // (its bin, 256ths past it)
        if (hap_b >= HAP_N - 1) hap_b = HAP_N - 2, hap_f = 255;
        S64 hap_c[2];
        for (int t = 0; t < 2; t++) {
          const U64* const he = hap_e[t];
          const U64* const hm2 = hap_m[t];
          const U64 ha = t ? HAP_A : HAP_A / 2;  // (the fast table's prior a miss, the slow one's 2)
          const S64 c0 = (S64)lg2tab.log2x(hm2[hap_b] + ha) - (S64)lg2tab.log2x(he[hap_b] + ha);
          const S64 c1 = (S64)lg2tab.log2x(hm2[hap_b + 1] + ha) - (S64)lg2tab.log2x(he[hap_b + 1] + ha);
          hap_c[t] = ((256 - hap_f) * c0 + hap_f * c1) >> 8;
        }
        const U32 hap_p[3] = {lg2tab.prob(hap_l + hap_c[0]), lg2tab.prob(hap_l + hap_c[1]),
                              lg2tab.prob(hap_l + (hap_c[0] + hap_c[1]) / 2)};
        S64* const hap_s = hap_k;
        int hap_i = 2;
        if (hap_s[0] < hap_s[hap_i]) hap_i = 0;
        if (hap_s[1] < hap_s[hap_i]) hap_i = 1;
        hit = c->code_bit(hap_p[hap_i], ENC && v == qp);
        {
          const S64 one = lg2tab.log2x(CO_ONE);
          for (int t = 0; t < 3; t++) hap_s[t] += one - lg2tab.log2x(hit ? CO_ONE - hap_p[t] : hap_p[t]);
          if (hap_s[0] + hap_s[1] + hap_s[2] >= (S64)1 << 40) hap_s[0] >>= 1, hap_s[1] >>= 1, hap_s[2] >>= 1;
        }
        for (int t = 0; t < 2; t++) {
          U64* const he = hap_e[t];
          U64* const hm2 = hap_m[t];
          const U64 lim = t ? HAP_SLOW : HAP_FAST;
          for (int k = 0; k < 2; k++) {
            const U64 w = k ? (U64)hap_f : (U64)(256 - hap_f);
            he[hap_b + k] += (U64)pq * w;
            if (!hit) hm2[hap_b + k] += w << CO_PBITS;
            if (he[hap_b + k] + hm2[hap_b + k] >= lim) he[hap_b + k] >>= 1, hm2[hap_b + k] >>= 1;
          }
        }
        {
          const double pd = (double)pq / (double)CO_ONE, er = (hit ? (pd > 0.8 ? -4.0 : -pd / (1.0 - pd)) : 1.0) * HMX_R;
          for (int k = 0; k < 4; k++) md[k] += er * mx[k], mb[k] += er * mx[k];
        }
        if (hk) hc_learn(*hc, hit, pm);
        if (!ux) cupd(cb, hit);
        if (xm) {
          cupd(cb + 8, hit);
          const S64 one = lg2tab.log2x(CO_ONE);
          xm_loss[0] += one - lg2tab.log2x(hit ? CO_ONE - p0 : p0);
          xm_loss[1] += one - lg2tab.log2x(hit ? CO_ONE - p1 : p1);
          if (xm_loss[0] + xm_loss[1] >= (S64)1 << 40) xm_loss[0] >>= 1, xm_loss[1] >>= 1;
        }
        if (hit) return *how = 0, qp;
        B::sel(fh, H);  // (the family's rows for a miss only)
      } else {
        B::sel(fh, H);
        hit = fh.bit(HT_HIT, ENC && v == qp);  // (the LFE's misses are the fit's: by the family)
      }
      if (hit) return *how = 0, qp;
      if (alt != qp && fh.bit(HT_ALT, ENC && v == alt)) return *how = 1, alt;
      far = (qp < 0 ? -qp : qp) >= 4;
      // (neither, the audio's 4 or more: where within HIT_NEAR of it, its side, alt's or the other
      // (up or down where none is), and its distance in unary, from 2 on alt's side (1 is alt), else
      // from 1; else its distance past HIT_NEAR or its magnitude, below. A miss next to a small
      // index costs less as its magnitude: 96-24.dts's, the X96 residuals' in its core's values)
      if ((qp < 0 ? -qp : qp) >= 4) {
        const int sa = alt > qp ? 1 : alt < qp ? -1 : 0;
        const S64 d = ENC ? v - qp : 0, ad = d < 0 ? -d : d;
        if (fh.bit(HT_NEAR, ENC && ad <= HIT_NEAR)) {
          const int same = fh.bit(HT_SIDE, ENC && (sa ? (d > 0) == (sa > 0) : d > 0));
          int k = same && sa && alt == qp + sa ? 2 : 1;  // (alt one away: 1 is alt)
          while (k < HIT_NEAR && fh.bit(HT_DIST + (same ? HIT_NEAR : 0) + k, ENC && ad > k)) k++;
          const S64 w = qp + (same ? (sa ? sa : 1) : (sa ? -sa : -1)) * k;
          return w < lo ? lo : w > hi ? hi : w;
        }
      }
    }
    B::sel(fs, D);
    S64 w;
    if (far && fm_c[1] < fm_c[0]) {
      // (its distance past HIT_NEAR, as far as either side allows, then which side where both do)
      const S64 d = ENC ? v - qp : 0, ad = d < 0 ? -d : d;
      const S64 up = hi - qp - HIT_NEAR - 1, dn = qp - lo - HIT_NEAR - 1, fmax = up > dn ? up : dn;
      const int fnb = imax(bit_len((U64)(fmax > 0 ? fmax : 0)), 1);
      const S64 m = checked(code_len(fs, QN_LEN, fnb, mt, ENC ? ad - HIT_NEAR - 1 : 0, ENC), 0, fmax);
      int neg = 0;
      if (m <= up && m <= dn) neg = fs.bit(QN_SIGN, ENC && d < 0);
      else if (m > up) neg = 1;
      w = neg ? qp - (m + HIT_NEAR + 1) : qp + (m + HIT_NEAR + 1);
      w = w < lo ? lo : w > hi ? hi : w;
    } else {
      const S64 mmax = -lo > hi ? -lo : hi;
      const int maxnb = imax(bit_len((U64)mmax), 1);
      const S64 m = checked(code_len(fs, QN_LEN, maxnb, mt, ENC ? (v < 0 ? -v : v) : 0, ENC), 0, mmax);
      int neg = 0;
      if (m && m <= hi && m <= -lo) neg = fs.bit(QN_SIGN, ENC && v < 0);
      else if (m > hi) neg = 1;
      w = neg ? -m : m;
    }
    if (far) {
      const S64 aw = w < 0 ? -w : w, dw = w - qp, af = (dw < 0 ? -dw : dw) - HIT_NEAR - 1;
      fm_c[0] += 2 * (U64)bit_len((U64)aw) + 2, fm_c[1] += 2 * (U64)bit_len((U64)(af > 0 ? af : 0)) + 2;
      if (fm_c[0] + fm_c[1] >= (U64)1 << 16) fm_c[0] >>= 1, fm_c[1] >>= 1;
    }
    return w;
  }

  // --- the LFE -----------------------------------------------------------------------------------------

  // The subframe's LFE scale factor v (coded with its first sample, the walk's order notwithstanding:
  // the samples are coded by its step): against the one the predictions' RMS makes
  int code_lfe_scale(int v) {
    S64 x[4 * NSSF];
    for (int i = 0; i < lfe_n; i++) x[i] = lfe_p[i] >> (LFE_Q - 12);  // (RMS / 16, in 2^-AQ_F: scale_pred()'s)
    int near;
    int p = scale_pred(x, lfe_n, SF7, SF7_N, &near);
    int lc[5] = {p, -1, -1, -1, -1};
    if (ss_rule() && bind.up == 1) {
      S64 m = 0;
      for (int i = 0; i < lfe_n; i++) m = imax64(m, lfe_p[i] < 0 ? -lfe_p[i] : lfe_p[i]);
      const S32 pk = (S32)(m >> 20 < 0x7fffffff ? m >> 20 : 0x7fffffff);  // (the encoder's units: 2^-20 of the model's)
      FfSoft qq;
      lc[1] = FfEnc::one_scale(fe_get_cb(pk), 11, &qq), lc[2] = FfEnc::scale_within_peak(pk, 11, 3);
      S64 x2[4 * NSSF];
      for (int i = 0; i < lfe_n; i++) x2[i] = lfe_fixed(i, true) >> (LFE_Q - 12);
      int near2;
      lc[3] = scale_pred(x2, lfe_n, SF7, SF7_N, &near2);
      lc[4] = lfe_lattice(imin(lc[0], lc[1]) - 8, imax(lc[0], lc[1]) + 8);
      int k = 0;
      for (int i = 1; i < 5; i++)
        if (lc[i] >= 0 && lfs_w[i] > lfs_w[k]) k = i;
      p = lc[k];
      if (k == 3) near = near2;
    }
    M_scl_v D;
    D.kd = 3;
    D.ab = 0;
    D.pm = 0;
    D.pk = 1;
    D.pn = near;
    D.lp = lfe_lp;
    D.pl = 15;
    D.hm = lfe_hm;
    D.bd = 0;
    D.dp = sf_run(SF7, SF7_N, p);
    D.cc = 0;
    D.nb = 9;
    v = (int)scale(D, v, p, 0, 128);
    if (lc[1] >= 0) {
      for (int i = 0; i < 5; i++) lfs_w[i] += v == lc[i];
      if (lfs_w[0] + lfs_w[1] + lfs_w[2] + lfs_w[3] + lfs_w[4] >= 1024)
        for (int i = 0; i < 5; i++) lfs_w[i] >>= 1;
    }
    lfe_hm = v == p ? 1 : 2;
    lfe_lp = dvar(v - p);
    n_sf++;
    n_sfhit += v == p;
    lfe_scl = v;
    lfe_st = ss_rule() ? lfe_scale_dec(v) : lfe_scale(v);
    return v;
  }
  // (where FFmpeg's encoder's LFE values have been the core's (ffl_on()): the scale factor v by its
  // rule on the best's peak (calc_one_scale())
  int code_lfe_scale_ff(int v) {
    const S32* const x = ffl[ffl_best()];
    S32 m = 0;
    for (int i = 0; i < 8; i++) {
      const S32 a = x[i] < 0 ? (S32)(0u - (U32)x[i]) : x[i];  // (as abs(): -2^31 stays so, below any)
      if (a > m) m = a;
    }
    FfSoft qq;
    const int p = FfEnc::one_scale(ffe->get_cb(m), 11, &qq);
    M_scl_v D;
    D.kd = 3;
    D.ab = 0;
    D.pm = 0;
    D.pk = 1;
    D.pn = SB_N - 1;
    D.lp = lfe_lp;
    D.pl = 15;
    D.hm = lfe_hm;
    D.bd = 0;
    D.dp = sf_run(SF7, SF7_N, p);
    D.cc = 0;
    D.nb = 9;
    v = (int)scale(D, v, p, 0, 128);
    lfe_hm = v == p ? 1 : 2;
    lfe_lp = dvar(v - p);
    n_sf++;
    n_sfhit += v == p;
    lfe_scl = v;
    lfe_st = ss_rule() ? lfe_scale_dec(v) : lfe_scale(v);
    return v;
  }
  // and value i, v, by the best's quantization at that scale factor, sure
  S64 code_lfe_ff(int i, S64 v) {
    FfSoft q;
    S64 qp = 0, alt = 1;
    S128 dist = 0;
    if (ffl_quant(lfe_scl, q)) {
      qp = (S8)(fe_quant(ffl[ffl_best()][i], q) & 0xff);
      alt = qp + (qp >= 0 ? 1 : -1);
      alt = alt > 127 ? 126 : alt < -128 ? -127 : alt;
      dist = (S128)1 << 100;
    }
    M_smp_v D;
    D.ab = 27;
    D.hw = 0;
    D.pk = 1;
    D.nr = near_bucket(dist, (S128)1 << 22, 17);
    D.nl = 0;
    D.ql = imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21);
    D.hm = imin(__builtin_popcount(lmiss), 3);
    D.pm = 0;
    D.bd = 0;
    D.m1 = lq1 ? imin(lq1 - 1, 21) : 22;
    D.m2 = lq2 ? imin(lq2 - 1, 21) : 22;
    D.sq = svar(qp);
    D.s1 = lqs;
    int how;
    v = code_index(D, v, 1, qp, alt, -128, 127, &how);
    lmiss = (U8)(lmiss << 1 | (how != 0));
    lq2 = lq1;
    lq1 = 1 + bit_len((U64)(v < 0 ? -v : v));
    lqs = (S8)svar(v);
    n_smp++;
    n_hit += how == 0;
    for (int k = LFE_NP - 1; k > 0; k--) lfe_past[k] = lfe_past[k - 1];
    lfe_past[0] = v * lfe_st;
    return v;
  }
  // the LFE's prediction from its features f: the fit's that has predicted the values more often
  // (with the last values the better against a decoded WAV, and for some lossless ones)
  // (the fixed one's on a tie: so from the stream's start)
  // (6-11: those half a step up, where better)
  int lfe_best() const {
    int k = 3;
    for (int j = 0; j < 3; j++)
      if (lfh[j] > lfh[k]) k = j;
    if (lfh[4] > lfh[k]) k = 4;
    if (lfh[5] > lfh[k]) k = 5;
    for (int j = 6; j < LFE_NC; j++)
      if (lfh[j] > lfh[k]) k = j;
    return k;
  }
  bool lfe_poor() const {
    U32 b = lfw[0];
    for (int k = 1; k < 10; k++) b = lfw[k] > b ? lfw[k] : b;
    return 4 * (U64)b < 3 * (U64)lfwn;
  }
  // half a step less a unit: a prediction half a step up rounds up (an exact one, to itself)
  S64 lfe_half() const { return lfe_st > 0 ? lfe_st / 2 - 1 : 0; }
  // the DTS-HD MA encoders' filter's (LFE_W2, in 2^-30), FFmpeg's decodes' (LFE_W2D, in 2^-24: d)
  S64 lfe_fixed(int i, bool d = false) const {
    const S32* const w = d ? LFE_W2D : LFE_W2;
    S128 p = 0;
    for (int k = 0; k < LFE_N2; k++) p += (S128)w[k] * lfe_f3[i][k];
    return (S64)(p >> (d ? 24 : 30));
  }
  // value i's by FFmpeg's decoder's LFE interpolation undone, where a decoded WAV's (ss_rule()):
  // its 1024 samples from value i's through LFE_LSH, less the 7 values before (lfe_f[i]'s past ones,
  // times their steps) as FFmpeg dequantized them (the value times the step's 16 times, floored)
  // through LFE_LSU; the WAV's samples at 24 bits or more half a unit up (FFmpeg's floats floored
  // there). On FFmpeg's decode of its encode of `s.2.dts`'s first 3 MB 5058 of its 5072 LFE
  // values within 0.04 of a step, all but the last 14, whose samples pass the WAV's end (the
  // noise's power 3.7 times the WAV's: a 16-bit WAV's, 256 times as far, makes only the louder
  // ones; where a step is a few of FFmpeg's integer LFE units, those blur it), where LFE_W2D's was
  // a fit of the decimations
  S64 lfe_dec(int i) const {
    if (!ss_rule() || bind.up != 1) return lfe_fixed(i, true);
    S32 buf[1024];
    const S32* const s = wav->span(bind.lfe, 64 * (lfe_n0 + i) + bind.off - 512, 1024, buf);
    const S64 h = wav->bps >= 3;
    S128 acc = 0;
    for (int k = 0; k < 1024; k++) acc += (S128)LFE_LSH[k] * (2 * (S64)s[k] + h);
    for (int k = 0; k < 7; k++) acc -= (S128)LFE_LSU[k] * (2 * clip23(lfe_f[i][LFE_NT + k] >> 27));
    // (acc 2^41 times the value as dequantized, x: the value times its step, (16 x + 15/2) 2^23)
    return (S64)((acc * ((S128)1 << 26) + ((S128)15 << 62)) >> 40);
  }
  // (a decoded WAV's at the core's rate: the scale factor from s0 to s1 whose lattice the WAV's LFE
  // fits best: at each, the subframe's values as FFmpeg's decoder would have them at it, each the
  // inversion's (lfe_dec()'s) from the WAV and the values before it so decoded, rounded to its step,
  // and their squared distances from those lattice points against its step's square; where the
  // step is right, the values before each are the decoder's and the inversion exact (within 0.04
  // of a step), where not, they drift. The best where its values come within 0.06 of a step at the
  // root of their mean square, some not 0, else -1: on FFmpeg's encodes of `s.2.dts`'s first 3 MB
  // and of the noise, 99.5% and 100% of their LFE scale factors so, all right; `ES 6.1
  // 16bit.dts`'s 87.7%, all right, the rest where its LFE is quiet)
  static constexpr double LFE_LAT_T = 0.0036;  // (0.06 squared)
  int lfe_lattice(int s0, int s1) const {
    S128 base[4 * NSSF];  // (the samples' part of each value's inversion, the same at any step)
    const S64 h = wav->bps >= 3;
    for (int i = 0; i < lfe_n; i++) {
      S32 buf[1024];
      const S32* const s = wav->span(bind.lfe, 64 * (lfe_n0 + i) + bind.off - 512, 1024, buf);
      S128 acc = 0;
      for (int k = 0; k < 1024; k++) acc += (S128)LFE_LSH[k] * (2 * (S64)s[k] + h);
      base[i] = acc;
    }
    int best = -1;
    double br = 0;
    for (int sf = imin(s1, SF7_N - 1); sf >= imax(s0, 0); sf--) {
      const S64 st = lfe_scale_dec(sf);
      if (st <= 0) continue;
      S64 pi[7];  // (the 7 values before, as FFmpeg dequantized them; [0] the last)
      for (int k = 0; k < 7; k++) pi[k] = clip23(lfe_past[k] >> 27);
      S128 res = 0;
      int nz = 0;
      for (int i = 0; i < lfe_n; i++) {
        S128 acc = base[i];
        for (int k = 0; k < 7; k++) acc -= (S128)LFE_LSU[k] * (2 * pi[k]);
        const S64 e = (S64)((acc * ((S128)1 << 26) + ((S128)15 << 62)) >> 40);
        S64 q = e >= 0 ? (e + st / 2) / st : -((-e + st / 2) / st);
        q = q < -128 ? -128 : q > 127 ? 127 : q;
        const S64 d = e - q * st;
        res += (S128)d * d;
        nz += q != 0;
        for (int k = 6; k > 0; k--) pi[k] = pi[k - 1];
        pi[0] = clip23((q * st) >> 27);
      }
      const double rel = (double)res / ((double)st * st);
      if (nz && (best < 0 || rel < br)) best = sf, br = rel;
    }
    return best >= 0 && br <= LFE_LAT_T * lfe_n ? best : -1;
  }
  // a fixed filter's prediction p at their gain (lfe_g)
  S64 lfe_gain(S64 p) const { return lfe_g == BIND_Q15 ? p : (S64)((S128)p * lfe_g / BIND_Q15); }
  S64 lfe_pred(int i) const {
    const int k = lfe_best() % 6;  // (the step not yet known: as is)
    return k == 5 ? lfit4[lfe_pl[i]].predict(lfe_f3[i])
         : k == 4 ? lfe_gain(lfe_dec(i))
         : k >= 3 ? lfe_gain(lfe_fixed(i))
         : k == 2 ? lfit3.predict(lfe_f3[i])
         : k == 1 ? lfit2.predict(lfe_f[i])
                  : lfit.predict(lfe_f[i]);
  }
  // LFE sample i of the subframe (its value v): against its prediction at the scale factor's step
  S64 code_lfe(int i, S64 v) {
    S64 alt;
    S128 dist;
    int how;
    for (int k = 0; k < LFE_NP; k++) lfe_f[i][LFE_NT + k] = lfe_past[k];  // (the values decoded)
    const S64 p1 = lfit.predict(lfe_f[i]), p2 = lfit2.predict(lfe_f[i]), p3 = lfit3.predict(lfe_f3[i]),
              p4 = lfe_gain(lfe_fixed(i)), p5 = lfe_gain(lfe_dec(i)), p6 = lfit4[lfe_pl[i]].predict(lfe_f3[i]);
    const S64 pk[6] = {p1, p2, p3, p4, p5, p6};
    const int lb = lfe_best();
    const S64 pg = pk[lb % 6] + (lb % 12 >= 6 ? lfe_half() : 0), ag = pg < 0 ? -pg : pg;
    const int cx = 2 * (ag < 4 * lfe_st ? 0 : ag < 16 * lfe_st ? 1 : ag < 32 * lfe_st ? 2 : 3) + (pg < 0);
    S64 pc[LFE_NC], lu[12], lc[12];  // (the twelve, and less their error's fit; each x / 64 and the fit's
    for (int k = 0; k < 12; k++) {    // value, in 2^-16 of a step)
      pc[k] = pk[k % 6] + (k >= 6 ? lfe_half() : 0);
      lu[k] = lfe_st > 0 ? (S64)((S128)pc[k] * 1024 / lfe_st) : 0;
      lu[k] = lu[k] > 1 << 20 ? 1 << 20 : lu[k] < -(1 << 20) ? -(1 << 20) : lu[k];
      const S64* const w = lfl[pc[k] < 0][k];
      lc[k] = w[0] + w[1] * lu[k] / 65536;
      pc[12 + k] = pc[k] - (S64)((S128)lc[k] * lfe_st / 65536);
    }
    int kc = lb;  // (the class's best, the best where none is better there)
    for (int k = 0; k < LFE_NC; k++)
      if (lfc[cx][k] > lfc[cx][kc]) kc = k;
    const int kk = lsw[1] > lsw[0] + LSW_M ? kc : lb;
    lfe_p[i] = pc[kk];
    const S64 qp = requant(lfe_p[i], lfe_st << 6, -128, 127, &dist, &alt);
    M_smp_v D;
    D.ab = 27;
    D.hw = 0;
    D.pk = 1;
    D.nr = near_bucket(dist, (S128)lfe_st << 22, 17);
    // (since format 139 nl, a subband sample's loudness nearness, which an LFE value has no use for,
    // how many of the 24 predictions make qp too, each weighed by how often it has been the value
    // lately (lfh's counts, which the choice above goes by, plus 1), in 0-15: where all 24 agree the
    // value is the prediction (the Cymatics stream's 42% of its values, every one of them), where 8
    // or fewer do 72% of the time or less. Its core 0.4% smaller, `ES 6.1 16bit.dts`'s 1.2%, the
    // others within 3 bytes; the predictions unweighed, 0.01% less; the alternative's count for the
    // flag after a miss, nothing more, as the predictions' means or medians, which miss more often)
    {
      U64 wa = 0, wt = 0;
      for (int k = 0; k < LFE_NC; k++) {
        S128 dd;
        S64 al;
        const U64 w = (U64)lfh[k] + 1;
        wt += w;
        if (requant(pc[k], lfe_st << 6, -128, 127, &dd, &al) == qp) wa += w;
      }
      D.nl = (int)(wa * 15 / wt);
    }
    D.ql = imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21);
    D.hm = imin(__builtin_popcount(lmiss), 3);
    D.pm = 0;
    D.bd = 0;
    D.m1 = lq1 ? imin(lq1 - 1, 21) : 22;
    D.m2 = lq2 ? imin(lq2 - 1, 21) : 22;
    D.sq = svar(qp);
    D.s1 = lqs;
    v = code_index(D, v, 1, qp, alt, -128, 127, &how);
    lmiss = (U8)(lmiss << 1 | (how != 0));
    lq2 = lq1;
    lq1 = 1 + bit_len((U64)(v < 0 ? -v : v));
    lqs = (S8)svar(v);
    n_smp++;
    n_hit += how == 0;
    lfit.add(lfe_f[i], v * lfe_st, lfe_st);
    lfit2.add(lfe_f[i], v * lfe_st, lfe_st);
    lfit3.add(lfe_f3[i], v * lfe_st, lfe_st);
    lfit4[lfe_pl[i]].add(lfe_f3[i], v * lfe_st, lfe_st), lfe_pn[lfe_pl[i]] = 1;
#ifdef DTSD_LFEDUMP
    if (ENC) lfe_dump(lfe_f3[i], lfe_st, (int)v);
#endif
    {  // (whether each prediction's index was the value)
      S128 dd;
      S64 al;
      U32 hk[LFE_NC];
      for (int k = 0; k < LFE_NC; k++) {
        hk[k] = requant(pc[k], lfe_st << 6, -128, 127, &dd, &al) == v;
        lfh[k] += hk[k];
      }
      for (int k = 0; k < 5; k++) lfw[k] += hk[k], lfw[5 + k] += hk[6 + k];
      lfwn++;
      bool hv = false;  // (by the first four's sums, as before the fifth; or theirs half a step up, or
      for (int g = 0; g < LFE_NC; g += 6)  // less their error's fit)
        hv |= lfh[g] + lfh[g + 1] + lfh[g + 2] + lfh[g + 3] >= 4u << 11;
      if (hv)
        for (int k = 0; k < LFE_NC; k++) lfh[k] >>= 1;
      for (int k = 0; k < LFE_NC; k++) lfc[cx][k] += hk[k] << 4;
      if (++lfcn[cx] >= LFC_N) {
        for (int k = 0; k < LFE_NC; k++) lfc[cx][k] >>= 1;
        lfcn[cx] >>= 1;
      }
      if (lfe_st > 0)
        for (int k = 0; k < 12; k++) {
          const S128 d = (S128)pc[k] - (S128)v * lfe_st;
          if ((d < 0 ? -d : d) >= 2 * (S128)lfe_st) continue;
          S64* const w = lfl[pc[k] < 0][k];
          const S64 er = (S64)(d * 65536 / lfe_st) - lc[k], nm = 65536 + lu[k] * lu[k] / 65536;
          w[0] += er * 65536 / nm / LFL_R, w[1] += er * lu[k] / nm / LFL_R;
        }
      lsw[0] += hk[lb], lsw[1] += hk[kc];
      if (++lswn >= LSW_N) lsw[0] >>= 1, lsw[1] >>= 1, lswn >>= 1;
    }
    for (int k = LFE_NP - 1; k > 0; k--) lfe_past[k] = lfe_past[k - 1];
    lfe_past[0] = v * lfe_st;
    return v;
  }

  // the decoder's next sample of channel ch's subband b
  INLINE void push_y(int ch, int b, S32 y) {
    S32* h = yh[ch][b];
    h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
  }
  // the decoder's samples of the subframe's subbands without indexes: none where ABITS is 0 but the
  // ADPCM prediction, the VQ vector's (and the prediction) past VQSUB, none past SUBS
  void smp_fill(const CoreRec& r, const CoreSub& s) {
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      for (int b = 0; b < NSUB; b++) {
        if (b < r.nvqsub[ch] && s.abits[ch][b]) continue;  // (pushed as decoded)
        const int pm = b < r.nsubs[ch] && s.pmode[ch][b];
        for (int j = 0; j < nsmp; j++) {
          S64 y = 0;
          if (b >= r.nvqsub[ch] && b < r.nsubs[ch]) {
            const S64 sc = r.shuff[ch] == 6 ? SF7[s.scale[ch][b][0]] : SF6[s.scale[ch][b][0] & 63];
            y = clip23((HF_VQ[s.hfvq[ch][b]][j] * sc + 8) >> 4);
          }
          if (pm) y = clip23(y + adpcm_pred(s.pvq[ch][b], yh[ch][b]));
          push_y(ch, b, (S32)y);
          if (x96a) yc[ch][b][fb0 + j] = (S32)y;
        }
        if (b >= r.nsubs[ch] || (b < r.nvqsub[ch] && !s.abits[ch][b])) q1[ch][b] = q2[ch][b] = 0, qs[ch][b] = 0;
      }
  }
};
