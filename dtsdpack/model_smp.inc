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
  M_FAM(hit)
  M_FAM(smp)
  M_FAM(vqr)
  WavIn* wav;     // the WAV (none: ok 0)
  Bind bind;      // how the core lines up with it
  const S64* fir; // the prototype filter of the frame's FILTS
  S64 tnext;      // the blocks so far: the next frame's first (since the stream's first frame)
  S64 tframe;     // the frame's first
  S64 t0;         // the subframe's first
  int nsmp;       // the subframe's samples
  int ext_set;    // (the WAV's continuation past its end: 1 made, 2 taken or not: smp_subframe())
  int past_on;    // (and whether the audio far past it is the encoder's: past_end())
  int bound[NCH];  // whether the subframe's channel has the audio's subband samples (X)
  S64 X[NCH][NSUB][NSMP];  // the audio's subband samples in the subframe (2^-AQ_F)
  S64 xh[NCH][NSUB][4];    // the audio's last 4 before the subframe (xh[.][.][0] the last)
  S32 yh[NCH][NSUB][4];    // the decoder's last 4
  S32 q1[NCH][NSUB], q2[NCH][NSUB];  // the subband's last two indexes (1 + their length, 0 none; with their signs in qs)
  S8 qs[NCH][NSUB];
  U8 miss[NCH][NSUB];      // the subband's last 8 indexes, a bit each: not the audio's
  U32 chm[NCH];            // and the channel's last 32 (those coded against the audio)
  int ac[NCH][NSUB], es[NCH][NSUB], ec[NCH][NSUB];  // the subframe's subbands' autocorrelation, energy, its change
  int eq[NCH][NSUB];       // and where in its scale factor's bucket the energy lies (scale_quarter())
  int esmax[NCH];          // the channel's loudest subband's energy (es)
  S64 tlev[NCH][NSMP];     // the channel's level at each of the subframe's samples: the root of the sum of its subband
                           // samples' squares (2^-AQ_F)
  S64 tlvl[NCH];           // (and the root of their mean square)
  int eab[NCH][NSUB];      // the energy ABITS was coded against (es; an ADPCM subband's residual's)
  U8 ab_seen[2][NSUB][129];  // the ABITS each subband had the last time it had that energy (by pm; 1 + it, 0 none)
  // the bit allocations' staircases (stair_guess()), by pass (the core's, the extension's) and
  // subband (an ADPCM subband's, by its residual's energy, the same); each pass's level
  // (the energies in 128ths of a doubling)
  static constexpr int AL_ND = 512, AL_NK = 2 * AL_ND + 1;  // (the level's search: 4 doublings either side)
  static constexpr int AB_RC = 1024;  // (the rate control's: 8 doublings either side of the last level)
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
  U64 n_st, n_sthit;  // (-s: the ABITS a staircase said, those it said right)
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
  // how often the RMS rule's scale factor was the core's, scale_ceil()'s, and scale_search()'s (the
  // better of those two's where it isn't sure; halved as they grow); the search's tries (to 2048)
  U32 sfr[3], ss_n;
  // the same of the bit allocation search's ABITS (abits_search()) and the first guess's without
  // it, where it was tried; its tries; the ABITS (-s: those it said, those it said right)
  U32 abr[2], ab_n;
  U64 n_ab, n_abs, n_abhit;
  int abfit_a[NCH][NSUB];   // the subframe's subbands' search's ABITS (-1 none), and its fit's distance
  U64 abfit_d[NCH][NSUB];
  struct AbCache {          // a bit allocation search's, where PMODE's made it (its window, its result)
    int g, a0, a1, p, G, gc;
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
  int lfe_n;               // how many
  S64 lfe_f[4 * NSSF][LFE_NF];  // each one's decimated samples around it (the last values after them: code_lfe()'s)
  S64 lfe_f3[4 * NSSF][LFE_N2];  // and the prototype's decimations by 32 around it (lfit3's, LFE_W2's)
  S64 lfe_past[LFE_NP];    // the last LFE values (times their steps; [0] the last)
  S64 lfe_p[4 * NSSF];     // and its prediction (the value times its step)
  int lfe_scl;             // their scale factor (the walk has it after them: coded with the first)
  S64 lfe_st;              // its step
  int lfe_hm, lfe_lp;      // whether the last was the prediction (as hm), the last less its prediction (as lp)
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
  // fifth the same of FFmpeg's decodes, LFE_W2D (its LFE interpolation undone), the better from a
  // decoded WAV's first values, which the fits haven't learned yet; and the sixth the third's at
  // each place of a frame's values apart (lfit4, by lfe_pl): `Master Audio 7.1.dts`'s encoder
  // decimates its LFE by another filter at each (once learned, the fits at each place make 93% of
  // its values, the best of the rest 30%), refitted only where the first five missed more than a
  // quarter of the values since the last refit (lfe_poor(): refitted every time, they'd cost
  // nearly as much as all the others)
  static constexpr int LFE_PL = 16;  // (a frame's values: 8 a 512-sample frame's)
  LfeFit<LFE_N2, LFE_K2, 7> lfit3, lfit4[LFE_PL];
  U32 lfe_nsf;
  U8 lfe_pn[LFE_PL];        // (whether lfit4's has taken values since its last refit)
  int lfe_pl[4 * NSSF];     // (each value's place in its frame)
  U32 lfh[6];               // how often each one's prediction was the value (halved as they grow)
  U32 lfw[5], lfwn;         // (the first five's since the last refit, and the values)
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
  // FATE's samples 1.0-1.6% smaller, the 7-frame xll ones 0.3-0.7%, `s.2.dts`'s core 0.04%)
  static constexpr U32 HIT_ONE = 256, HIT_PRIOR = 1, HIT_LIMIT = 1u << 4, HIT_SLIMIT = 1u << 18, HIT_RLIMIT = 1u << 12;
  U32 hit_c[2][4][8][QB_N][QB_N][4], hit_r[2][4][8][QB_N][2];
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
  // 0.03% more). The core's subbands', then X96's
  static constexpr int HC_MAX = 32, HC_S0 = -10, HC_K = 8;
  struct HitCal {
    S32 s;   // (the scale: 2^(s/4))
    U32 m;   // (the misses)
    U64 e;   // (those expected: the probabilities coded, in CO_ONE)
    U32 k;   // (its steps so far, to HC_K)
  };
  HitCal hcal[NSUB + NSUB96];
  // unit u times hc's scale
  static S128 hc_unit(S128 u, int s) {
    static const int QM[4] = {128, 152, 181, 215};  // (2^(k/4), in 128ths)
    const int e = s >> 2;
    return (e >= 0 ? u << e : u >> -e) * QM[s & 3] >> 7;
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

  void smp_start(WavIn* w, const Bind& b) {
    wav = w;
    bind = b;
    tnext = 0;
    memcpy(hgw, HF_GW, sizeof hgw);
    memcpy(hge, HF_GE, sizeof hge);
    for (int g = 0; g < 32; g++) hg_dist(g);
    hg_n = 0;
    for (int k = 0; k < NSUB + NSUB96; k++) hcal[k].s = HC_S0;
    for (int i = 0; i < 128; i++) lfe_cn[i] = -1;
    lfe_nsf = 0;
    lfe_lp = 9;
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
  bool past_far(S64 t) const { return wav && 32 * bind.up * t + bind.off - (S64)wav->nsamp > WAV_PAST * bind.up; }
  bool past_end(S64 t) const { return past_on && past_far(t); }

  // --- the subframe's audio --------------------------------------------------------------------------

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
    for (int ch = 0; ch < NCH; ch++) bound[ch] = 0;
    lfe_on = 0;
    if (!wav || !wav->ok) return;
    const int up = bind.up;
    const S64 o0 = 32 * up * t0 + bind.off, hs = o0 + 32 * up * (S64)(nsmp - 1);
    S64 lo = o0 - 512 * up, hi = hs;
    // the LFE (64x only), its samples' windows and the correction's around them
    const S64 n0 = t0 / 2;
    if (r.lff == 2 && !r.xpass && bind.lfe >= 0 && bind.lfe < wav->nch) {
      lfe_on = 1;
      lfe_n = 2 * r.lff * s.nssc;
      lo = imin(lo, 64 * up * (n0 - LFE_K) + bind.off - 544 * up);
      hi = imax(hi, 64 * up * (n0 + lfe_n - 1 + LFE_K) + bind.off + 32 * up);
    }
    wav->need(lo, hi);
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
      for (int j = 0; j < nsmp; j++) {
        analyze_bound(*wav, bind, ch, o0 + 32 * up * j, fir, x, x96a ? NSUB96 : NSUB, !ss_rule() && !x96a);
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
      }
      tlvl[ch] = nsmp ? isqrt128(tl / (U128)nsmp) : 0;
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
      analyze_bound(*wav, bind, ch, o0 + 32 * bind.up * j, fir, x, x96a ? NSUB96 : NSUB, !ss_rule() && !x96a);
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
        for (int k = 0; k < 5; k++) lfw[k] = 0;
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
  // allocation table's ABITS (apl's c0t, within AB_RC of the last)
  // (at the subframe's first ABITS, at bit at of the frame)
  void level_centre(const CoreRec& r, const CoreSub& s, int p, U64 at) {
    AbLevel& L = abl[p];
    L.set = 1;
    if (!p) bg_used = 0, memset(bg_done, 0, sizeof bg_done);
    L.nobs = 0;
    memcpy(ab_snap[p], ab_st[p], sizeof ab_snap[p]);
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        fe_s[ch][b] = fe_a[ch][b] = FE_NONE;
        if (!bound[ch]) continue;
        S64* const x = bud_v[ch][b];
        const int n = scale_values(s, ch, b, 0, s.nssc, x);
        int near;
        fe_s[ch][b] = fine_energy(x, n);
        // (a transient's: the encoders' energy is the part from it on, where its second scale factor
        // is: ab_transient())
        fe_s[ch][b] = ab_transient(s, ch, b, x, n, fe_s[ch][b], &fe_a[ch][b]);
        bud_n[ch][b] = n;
        bud_si[ch][b] = scale_pred(x, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
        memset(bud_b[ch][b], 0xFF, sizeof bud_b[ch][b]);
      }
    L.avail = 4 * ((S64)r.fsize * 8 - (S64)at);
    if (!p && L.rn) {  // (the samples' bits fall as the level rises: the nearer either side)
      const S64 T = L.avail + L.rb;
      S32 lo = L.c0 - AB_RC, hi = L.c0 + AB_RC;
      S64 tl = bud_total(r, s, lo), th = bud_total(r, s, hi);
      if (tl <= T) hi = lo, th = tl;
      else if (th > T) lo = hi, tl = th;
      while (hi - lo > 1) {
        const S32 m = lo + (hi - lo) / 2;
        const S64 tm = bud_total(r, s, m);
        if (tm > T) lo = m, tl = tm;
        else hi = m, th = tm;
      }
      L.c0 = clip(tl - T < T - th ? lo : hi, -AL_C, AL_C);
      // (and the table's: where its ABITS take those bits)
      AbParLevel& A = apl[0];
      lo = (A.cset ? A.c0t : L.c0) - AB_RC, hi = (A.cset ? A.c0t : L.c0) + AB_RC;
      tl = bud_total_tab(r, lo), th = bud_total_tab(r, hi);
      if (tl <= T) hi = lo, th = tl;
      else if (th > T) lo = hi, tl = th;
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
  // gives the ABITS
  int bud_bits(const CoreRec& r, int ch, int b, int a) {
    if (a <= 0 || a > 26) return 0;
    S16& t = bud_b[ch][b][a];
    if (t >= 0) return t;
    const int how = sample_coding(r, ch, a), n = bud_n[ch][b];
    int sum = 0;
    if (how == 2) {
      sum = 4 * (a - 3) * n;
    } else if (how == 3) {
      sum = BLOCK_BITS[a - 1] * n;
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
  // before's or more, the largest such rise's; not an ADPCM subband's, not from 20 up: tmode_rule()):
  // the encoders seen allocate its bits by the mean square of its values from the transient on,
  // where they take one, the rise rise_thr() or more, else by the subframe's: the fine energy the
  // ABITS are guessed by (fe the subframe's; *fa those values' where they take one, else FE_NONE).
  // As the threshold is theirs, the other of the two mean squares is no second guess (`s.2.dts`'s
  // core 0.15% smaller without it, the other lossless WAVs' up to 0.45%), nor is a subband that
  // doesn't predict where its gain says it would a transient by that alone.
  S32 ab_transient(const CoreSub& s, int ch, int b, const S64* x, int n, S32 fe, S32* fa_) const {
    *fa_ = FE_NONE;
    if (s.pmode[ch][b] || b >= 20 || s.nssc < 2 || fe == FE_NONE) return fe;
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
  // of the shape's: within 10 of it in `Master Audio 7.1.dts`'s, a downmix's). Fitted afterwards, a
  // level per subframe, the staircases miss 0.6% of a lossless WAV's ABITS where their intervals as
  // they come miss 1.6%. So the stream's offset is the one most of the last ABITS of all the bands
  // (the newer the more) say at their subframes' levels, for each shape, the shape the one more of
  // them say (app: from a stream's first frames, where a band of its own has few; and far more
  // exactly, as every band's ABITS count, than each band's offset by its own ABITS, which the table
  // goes by until a stream's first ABITS above 1: `s.2.dts`'s core 2.2% smaller, the other lossless
  // WAVs' 2-6%); a subframe's level, the core's from the budget as the staircases' centre is
  // (level_centre(), at the table's ABITS: c0t), the extension's that centre, plus how far the last
  // subframes' levels lay from it, then the shift at which the table makes the most of the
  // subframe's ABITS so far (as the staircases' level). Its guesses are the first ones where they
  // have missed less than the staircases' (ap_rule()).
  static constexpr int AP_W = 1024;  // (a band's offsets: within AP_W of its first)
  static constexpr int AP_SH = 64;   // (the level: within AP_SH of the prior)
  static constexpr S64 AP_FAR = (S64)1 << 40;
  struct AbPar {
    S32 c, o;              // the scores' centre (the band's first ABITS above 1's), its offset
    U8 set, one, out;      // (whether c is set; ABITS 1 seen, with c unset; ABITS in a row the scores miss)
    U32 sc[2 * AP_W + 1];  // each offset's score (ap_inc's)
  } abp[2][NSUB];
  // band b's offset in shape k, less the stream's
  static S32 ap_shape(int k, int b) {
    static const S16 LOW[5] = {-768, -640, -512, -320, -192};
    return (b < 5 && !k ? LOW[b] : 0) + (b > 19 ? (S32)((256 * (b - 19) + 1) / 3) : 0);
  }
  struct AbPool {
    S32 c, best[2];          // the scores' centre (the stream's first ABITS above 1's); each shape's offset
    U8 set, k, out;          // (whether c is set; the shape taken; ABITS in a row the scores miss)
    U32 sc[2][2 * AP_W + 1]; // each shape's offsets' scores (ap_inc's)
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
      q.best[0] = q.c, q.best[1] = (S32)(m - ap_shape(1, b));
      memset(q.sc, 0, sizeof q.sc);
    }
    U32 mx[2];
    for (int k = 0; k < 2; k++) {
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
    if (mx[1 - q.k] > mx[q.k]) q.k = (U8)(1 - q.k);
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
        for (int k = 0; k < 2; k++)
          for (int j = 0; j <= 2 * AP_W; j++) app[p].sc[k][j] >>= 10;
        A.inc >>= 10;
      }
    }
    const S64 dd = (lv - A.cb) * 1024;
    A.drift = A.dset ? A.drift + (dd - A.drift) / 32 : dd;
    A.dset = 1;
  }
  // whether the table's guesses are the first ones: its misses, of the last 1024 or so where both
  // had one, fewer than the staircases' (by 2)
  bool ap_rule(int p) const { return apl[p].miss[1] + 1 < apl[p].miss[0]; }
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
  // b (below 20; they make no prediction there): 8 (19 + b) / 19 times the energy (to 0.1% in each
  // subband: `s.2.dts`'s 2441 subbands below 20 that would gain by prediction and rise by it or
  // more don't predict, of the 216367 that rise less 130 don't; its TMODEs follow it but for 9 of
  // the 2.27 million that don't predict, all within 2% of it; FFmpeg's master_audio_7.1_24bit.dts's
  // as well, but for its first subframe)
  static double rise_thr(int b) { return 8.0 * (19 + b) / 19; }
  // How subband b's energy rises against that, for its PMODE and TMODE (a sid context, tr:
  // model.inc): 0 from subband 20, or by less than 20 eighths of a doubling (the largest rise; or
  // one subsubframe), else by the rise in eighths of a doubling past rise_thr(), floored: 1 -4 or
  // less, 2 -3 or -2, 3 -1, 4 0, 5 more
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
    const double q = b < 20 ? rise_max(s, ch, b, &tk) : 0;
    if (q < rise8.t[20]) return 0;
    const double u = q / rise_thr(b);  // (2^(d/8) at least, d the largest)
    return u >= rise8.t[1] ? 5 : u >= 1 ? 4 : 2 * u >= rise8.t[7] ? 3 : 2 * u >= rise8.t[5] ? 2 : 1;
  }
  // A lossless WAV's transient (TMODE) as encoders make it: where a subsubframe's energy is 8 times
  // the one before's at least, the largest such rise's (*tg, 1 to nssc - 1, else 0); how likely, the
  // class returned: 0 no such rise; 1 the subband predicts, 2 it is 20 or above (the encoders seen
  // make no transient in either); else 3 + 2 times the rise's doublings past 3 (to 2) + whether
  // ABITS is 10 or more (the likelier, the more of both)
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
    if (b >= 20) return 2;
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
  int code_rank(int r, int nb, M_vqr_v& D, const KeyIx* c, int n) {
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
        B::sel(f_vqr, D);
      }
      if (!f_vqr.bit(VN_BKT + t, ENC && bk > t)) break;
      t++;
    }
    if (t < 2) return t;
    int m = 1;
    for (int p = 0; p < t - 1; p++) m = 2 * m + f_vqr.bit(VN_BIT + 3 * t + imin(p, 2), ENC && ((r >> (t - 2 - p)) & 1));
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
  // (mlu > 0: ml's unit)
  // (ord: where given, the ranks' order instead of the keys', which the margins go by: each
  // candidate's, by its index)
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn, S128 mu = 0, S128 mlu = 0, const S128* ord = nullptr) {
    return code_cand(n, nb, v, kd, ch, b, gn, lrank[ch][b][kd], mu, mlu, ord);
  }
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn, U8& lh, S128 mu = 0, S128 mlu = 0, const S128* ord = nullptr) {
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
    B::sel(f_vqr, D);
    int at = 0;
    if (ENC)
      while (cand[at].i != v) at++;
    const int r = code_rank(ENC ? rank_of(cand, n, at) : 0, nb, D, cand, n);
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
    S64 xr[NSMP];
    bool resc = false;
    if (bound[ch] && !ss_rule() && !x96a && nsmp == 16) {
      const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
      const int nt = sf_entries(r.shuff[ch]);
      if (si < nt) {
        S128 ms = 0, nz = 0;
        for (int j = 0; j < nsmp; j++) ms += (S128)x[j] * x[j];
        ms /= nsmp;
        if (b == NSUB - 1 && cpl_on(r, ch, b)) {
          S128 m0 = 0;
          for (int j = 0; j < nsmp; j++) m0 += (S128)X[ch][0][j] * X[ch][0][j];
          nz = m0 / nsmp >> 40;
        }
        const U32 V = tab[si];  // (the region: a run's first's below its value, its last's from it on)
        int r0 = si, r1 = si;
        while (r0 > 0 && tab[r0 - 1] == V) r0--;
        while (r1 + 1 < nt && tab[r1 + 1] == V) r1++;
        const S64 vv = (S64)V << AQ_F;
        const S64 lo = si != r0 ? vv : r0 > 0 ? ((S64)tab[r0 - 1] + V) << (AQ_F - 1) : 0;
        const S64 hi = si != r1 ? vv : r1 + 1 < nt ? ((S64)V + tab[r1 + 1]) << (AQ_F - 1) : -1;
        const S64 rr = isqrt128((U128)(ms + nz));
        if (ms > 0 && hi > lo && (rr < lo || rr >= hi)) {
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
    const bool ur = resc && hrs[1] > hrs[0] + 64 + hrs[0] / 32;
    S128 gm[32], go[32], md[32], en = 0;  // (the scores of the values taken, and of the others)
    int f0 = 0;
    for (int g = 0; g < 32; g++) {
      const S128 k0 = hf_group_score(hgw, hge, g, x, nsmp, sc), k1 = resc ? hf_group_score(hgw, hge, g, xr, nsmp, sc) : k0;
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
    // first half the time, 98% from 2 times it)
    S128 mu = bound[ch] && !ss_rule() && !x96a ? (S128)tlvl[ch] * sc << 24 : 0;
    // (the top subband's with its leak's noise in too: the root of its square and that of 2^46 times
    // the noise's RMS times the scale factor, 2^-2 of the RMS's unit, where it is the encoder's
    // values' noise more than the level's is: `s.2.dts`'s core 0.2% smaller)
    if (mu > 0 && b == NSUB - 1 && cpl_on(r, ch, b)) {
      S128 m0 = 0;
      for (int j = 0; j < nsmp; j++) m0 += (S128)X[ch][0][j] * X[ch][0][j];
      const double sl = sqrt((double)(m0 / nsmp >> 40)) * (double)sc * 70368744177664.0;  // (2^46)
      mu = (S128)sqrt((double)mu * (double)mu + sl * sl);
    }
    // (the boundary order, hf_bound_grp(): the learned scores' W to the power 1.5, as they have
    // been learned, rather than the boundary's 1: `s.2.dts`'s core 0.3% smaller; the centroids' to
    // the power 1)
    S128 og[32];
    if (!mn) {
      if (nsmp == 16) hf_bound_grp_d(gm, hgd, og);
      else hf_bound_grp(gm, &HF_GC[0][0], 32, nsmp, false, og);
    }
    const int g = code_cand(32, 5, (int)v >> 5, VK_HFG, ch, b, gn, 0, mu << 19, mn ? nullptr : og);
    if (resc && !mn) {  // (which ranked the encoder's group higher, in the boundary orders)
      int rc = 0, ro = 0;
      S128 oo[32];
      hf_bound_grp_d(go, hgd, oo);
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
    S64 xs[NSMP];
    bool pushed = false;
    if (!mn && nsmp == 16 && g != f0) {
      double g2 = 0, gx[16];
      for (int j = 0; j < 16; j++) gx[j] = 2.0 * (double)(hgw[f0][j] - hgw[g][j]) * (double)sc * 65536.0, g2 += gx[j] * gx[j];
      if (g2 > 0) {
        const double lam = (double)(gm[g] - gm[f0]) / g2 / 8192.0;  // (hf_group_score()'s units: x 8192)
        pushed = true;
        for (int j = 0; j < 16 && pushed; j++) {
          const double d = lam * gx[j];
          if (d > 1e12 || d < -1e12) pushed = false;  // (the values within 2^40: hf_dist()'s)
          else xs[j] = x[j] - (S64)llround(d);
        }
      }
    }
    const bool up = pushed && hpw[1] > hpw[0] + 16 + hpw[0] / 32;
    KeyIx oc[32], cc[32];  // (the other order's boundary keys; the order's, as code_cand() may reorder cand)
    S128 kv[32], ko[32], ov[32], oo[32];
    S128 d0[32], d1[32];
    hf_dists(g, x, nsmp, sc, d0);
    if (pushed) hf_dists(g, xs, nsmp, sc, d1);
    for (int i = 0; i < 32; i++) {
      const S128 k0 = d0[i], k1 = pushed ? d1[i] : k0;
      cand[i].k = kv[i] = up ? k1 : k0, cand[i].i = i;
      ko[i] = up ? k0 : k1;
    }
    hf_bound_vec(kv, g, nsmp, ov);
    if (pushed) {
      hf_bound_vec(ko, g, nsmp, oo);
      for (int i = 0; i < 32; i++) cc[i].k = ov[i], cc[i].i = i, oc[i].k = oo[i], oc[i].i = i;
    }
    // (a lossless WAV's vectors: how far the nearest beats the next against the channel's level in
    // the subframe times the scale factor, as the encoder's arithmetic errs by its level: `s.2.dts`'s
    // encoder took the nearest of subband 28's 97% of the time within 2^-19 of that, 85% within
    // 2^-20, 49% within 2^-24; by the nearest's distance, 63% within 1/256 of it, 99% by 2^4 times
    // it or more)
    v = g * 32 + code_cand(32, 5, (int)v & 31, VK_HFV, ch, b, gn, mu, mu, ov);
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
  // step and scale factor; so of the entries from p - 3 to p + 5 (a VQ subband's to p + 12), the one whose reconstruction lies
  // nearest them, where nearer by 4 times the next best's error at least (else -1; a lossless WAV's
  // values are the encoder's input: none is; a VQ subband's all the same, *conf -1); *conf the
  // log2 of that ratio less 2, to 7
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
        const int qs = hf_shape(v, n);
        for (int i = imax(p - 3, 0); i <= imin(p + 12, nt - 1); i++) {
          const S128 d = vq_err(v, n, qs, tab[i]);
          if (d < bd) bd = d, vq = qs;
        }
      }
    }
    S128 e1 = -1, e2 = -1;
    int best = -1;
    for (int i = imax(p - 3, 0); i <= imin(p + (hfb ? 12 : 5), nt - 1); i++) {
      S128 e;
      if (hfb) {
        e = vq_err(v, n, vq, tab[i]);
      } else {
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0) continue;
        e = quant_err(v, n, ss, lo, hi);
      }
      if (best < 0 || e < e1) e2 = e1, e1 = e, best = i;
      else if (e2 < 0 || e < e2) e2 = e;
    }
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
  bool ss_rule() const { return sfr[2] > sfr[sf_base()] + 64 + sfr[sf_base()] / 8; }
  bool search_on() const { return ss_n < 2048 || ss_rule() || !(n_sf & 63); }
  // the same of the bit allocation search, of the ABITS: in its first 512 (the 7-frame samples' the
  // same, 4-8% faster), and while it has been right clearly more often than the first guess without
  // it (a lossless WAV's search, which rarely finds one, as often: then it ran on, for nothing, its
  // streams the same and 7.6% faster)
  bool ab_rule() const { return abr[1] > abr[0] + 32 + abr[0] / 32; }
  bool ab_search_on() const { return ab_n < 512 || abr[1] > abr[0] + 32 + abr[0] / 64 || !(n_ab & 63); }
  bool hf_min_on() const { return hf_n < 2048 || hfr[0] * 8 > hfr[1] || !(n_hfvq & 63); }

  // The ABITS a decoded WAV says (the bit allocation search): its values v (n of them, the ADPCM
  // prediction off) are what the decoder made of indexes at the step of the ABITS and a scale factor;
  // so of ABITS a0..a1 (g the guess) and the scale factors p - 3 to p + 5, the pair whose lattice
  // lies nearest them in its steps (a finer one fits as nearly but in more of its steps, a coarser
  // one misses the values between its points), of pairs as near the coarser lattice, then the scale
  // factor nearer p (where the values all lie on both, as a 3-level quantizer's do on the 7-level
  // one's at the same scale factor: `ES 6.1 16bit.dts`'s search right 99.23% of the time from 97.90%,
  // its core 2.1% smaller): 1 + its ABITS, where an index it makes isn't 0 (else 0: the values say
  // nothing); *conf the log2 of the next pair's distance over its, 0-7 (a pair as near isn't
  // counted), *dbest its distance (norm_dist()'s), *ibest its scale factor. And where none fits
  // within a 64th of a step squared a value, a transient's: of the ABITS a0..a1, the one whose
  // lattice fits the subsubframes before some one and those from it on, each part at a scale factor
  // of its own (part_fit()), the nearest, where 4 times nearer than the pair at least (*dbest still
  // the pair's: `ES 6.1 16bit.dts`'s search right 99.71%, its core 12.8% smaller still, FATE's
  // `dts_es.dts`'s 9.2%, `open bitrate.dts`'s 10.0%). (Only the pairs under cut: 0 where none is, and
  // no transients.)
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
  int abits_search(const CoreRec& r, int ch, const S64* v, int n, int g, int a0, int a1, int p, int* conf, U64* dbest,
                   U64 cut = ~0ull, int* ibest = nullptr) {
    constexpr int NP = 27 * 9, NK = 2;
    const int nt = sf_entries(r.shuff[ch]), nk = imin(n, NK);
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
    if (cut == ~0ull) return abits_pass(r, ch, w, n, ga, a0, a1, p, io, ni, conf, dbest, ibest);
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
    if (cut == ~0ull && n >= 16 && d1 >= ((U64)n << 10) && ab_rule()) {  // (a transient's: two scale factors, where
                                                                             // the search's rule is taken: a lossless
                                                                             // WAV's fit none)
      const int ns = n / 8;
      U64 bk = ~0ull;
      int ba = -1;
      for (int a = a0; a <= a1; a++) {
        S64 lo, hi;
        quant_range(r, ch, a, lo, hi);
        for (int k = 1; k < ns; k++) {  // (the parts' fits under the best's, as all would find)
          const U64 dA = part_fit(r, ch, a, lo, hi, v, 8 * k, bk);
          if (dA >= bk) continue;
          const U64 dB = part_fit(r, ch, a, lo, hi, v + 8 * k, n - 8 * k, bk - dA);
          const U64 d = dA + dB < dA ? ~0ull : dA + dB;
          if (d < bk) bk = d, ba = a;
        }
      }
      if (ba >= 0 && bk != ~0ull && (d1 + eps) / (bk + eps) >= 4) {
        ab_tdist = bk;
        const U64 ratio = (d1 + eps) / (bk + eps);
        *conf = imin(bit_len(ratio) - 3, 7);
        *dbest = d1;
        return 1 + ba;
      }
    }
    const U64 ratio = d2 == ~0ull ? ~0ull : (d2 + eps) / (d1 + eps);
    *conf = imin(bit_len(ratio) - 1, 7);
    *dbest = d1;
    if (ibest) *ibest = pr[best].i;
    return 1 + pr[best].a;
  }
  U64 ab_tdist;  // (the transient's fit's distance, where abits_search() took one; the caller sets it to ~0)
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
    if (n >= 16 && d1 >= ((U64)n << 10) && ab_rule()) {  // (a transient's: abits_search()'s)
      const int ns = n / 8;
      const S64* v = ab_v;
      U64 bk = ~0ull;
      int bta = -1;
      for (int a = a0; a <= a1; a++) {
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
  const S64* ab_v;  // (abits_search()'s values, in their order: abits_pass()'s transients')
  // the nearest fit of the values v (m of them) to ABITS a's lattice (its range lo..hi) at a scale
  // factor from 3 below their RMS rule's to 5 above: its distance (norm_dist()'s)
  U64 part_fit(const CoreRec& r, int ch, int a, S64 lo, S64 hi, const S64* v, int m, U64 lim = ~0ull) const {
    const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
    const int nt = sf_entries(r.shuff[ch]);
    int near;
    const int p = scale_pred(v, m, tab, nt, &near);
    U64 bd = ~0ull;
    // (from p out, each one's distance only while it could be under the nearest's and lim: those
    // under lim as trying them all finds; ~0 where none is)
    const int i0 = imax(p - 3, 0), i1 = imin(p + 5, nt - 1);
    for (int ki = 0; ki <= 2 * 5; ki++) {
      const int i = ki & 1 ? p + (ki + 1) / 2 : p - ki / 2;
      if (i < i0 || i > i1) continue;
      const S64 ss = step_for(r, ch, a, i);
      if (ss <= 0) continue;
      QErr q;
      q.set(ss, lo, hi);
      const U64 dl = bd < lim ? bd : lim;
      S128 e = 0, el = ~(U128)0 >> 1;
      const double l = (double)dl * (double)ss * (double)ss / 268435456.0 * (1 + 1e-9);  // (a distance past el is
      if (l < 1e36) el = (l < 9e18 ? (S128)(S64)l : (S128)l) + 1;                        // past dl's)
      int nz = 0, k = 0;
      while (k < m && e <= el) e += q.err(v[k++], &nz);
      if (e > el) continue;
      const U64 d = norm_dist(e, ss);
      if (d < bd) bd = d;
    }
    return bd < lim ? bd : ~0ull;
  }
  // The TMODE a decoded WAV says (the transient search), its ABITS known: where the subband's values
  // fit one scale factor's lattice (to a 64th of a step squared a value), none (0, *conf 7); else
  // the subsubframe from which two scale factors' fit best, where 4 times better than one at least
  // (else -1), *conf the log2 of that ratio less 2, to 7. (Each part's scale factor the nearest of
  // those from 3 below its RMS rule's to 5 above; the whole's fit the bit allocation search's,
  // where that found this ABITS.)
  int tmode_search(const CoreRec& r, const CoreSub& s, int ch, int b, int* conf) {
    const int a = s.abits[ch][b], ns = s.nssc;
    if (!a || ns < 2) return -1;
    S64 x[NSMP];
    const int n = scale_values(s, ch, b, 0, ns, x);
    S64 lo, hi;
    quant_range(r, ch, a, lo, hi);
    auto part = [&](int j0, int j1) { return part_fit(r, ch, a, lo, hi, x + 8 * j0, 8 * (j1 - j0)); };  // (subsubframes j0..j1 - 1)
    const U64 d0 = abfit_a[ch][b] == a ? abfit_d[ch][b] : part(0, ns);
    if (d0 < ((U64)n << 10)) return *conf = 7, 0;
    U64 bk = ~0ull;
    int best = -1;
    for (int k = 1; k < ns; k++) {
      const U64 d1 = part(0, k), d2 = part(k, ns), d = d1 + d2 < d1 ? ~0ull : d1 + d2;
      if (d < bk) bk = d, best = k;
    }
    const U64 eps = 2 * (U64)n;
    if (best < 0 || bk == ~0ull || (d0 == ~0ull ? false : (d0 + eps) / (bk + eps) < 4)) return -1;
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
  int scale_near_lev(int ch, int b, const S64* v, int n, const U32* tab, int nt, int p, S128 add = 0) const {
    S128 ms = 0;
    for (int i = 0; i < n; i++) ms += (S128)v[i] * v[i];
    const S64 rr = isqrt128((U128)(ms / n + add)), lev = tlvl[ch], e = (S64)tab[p] << AQ_F;
    S64 bd = -1;
    if (rr > e && p + 1 < nt) bd = ((S64)tab[p] + tab[p + 1]) << (AQ_F - 1);
    else if (rr <= e && p > 0) bd = ((S64)tab[p - 1] + tab[p]) << (AQ_F - 1);
    if (bd < 0 || lev <= 0) return -1;
    int sm = 0;
    for (int k = 0; k < NSUB; k++) sm += hcal[k].s;
    sm = sm >= 0 ? (sm + NSUB / 2) / NSUB : -((-sm + NSUB / 2) / NSUB);
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
    tcx X_;
    int p = -1, pc = -1, ps = -1, near = SB_N - 1, conf = 0, tried = 0, nlev = -1;
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
        // they are)
        if (b == NSUB - 1 && cpl_on(r, ch, b)) {
          S128 ms = 0, m0 = 0;
          for (int i = 0; i < n; i++) ms += (S128)v_[i] * v_[i];
          for (int i = 0; i < nsmp; i++) m0 += (S128)X[ch][0][i] * X[ch][0][i];
          p = scale_pred_ms(ms / n + (m0 / nsmp >> 40), r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
          if (!ss_rule() && !x96a && p >= 0)
            nlev = scale_near_lev(ch, b, v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p, m0 / nsmp >> 40);
        } else {
          p = scale_pred(v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
          // (a lossless WAV's: pn's outside the VQ subbands, whose contexts want the RMS's nearness
          // as it was: `s.2.dts`'s core 1.1% smaller, `3-1.dts`'s 0.1%; and pl's)
          if (!ss_rule() && !x96a && p >= 0)
            nlev = scale_near_lev(ch, b, v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p);
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
    f_scl.make(D, X_);
    f_scl.select(X_);
    if (p >= 0) {
      if (f_scl.bit(CN_HIT, ENC && v == p)) v = p;
      else v = checked(p + f_scl.codes(X_, ENC ? v - p : 0, (u32)D.kd), 0, max - 1);
      hm[ch] = v == p ? 1 : 2;
      n_sfhit += v == p;
    } else {
      if (A && f_scl.bit(CN_SAME, ENC && v == A - 1)) v = A - 1;
      else v = checked((A ? A - 1 : 0) + f_scl.codes(X_, ENC ? v - (A ? A - 1 : 0) : 0, (u32)D.kd), 0, max - 1);
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
        sfr[2] += v == pss;
        ss_n += ss_n < 2048;
      }
      if (sfr[0] + sfr[1] >= 1u << 16) sfr[0] >>= 1, sfr[1] >>= 1, sfr[2] >>= 1;
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
    const int a = s.abits[ch][b], pm = s.pmode[ch][b];
    const S64 ss = step_scale(r, s, ch, b, j >> 3);
    const S32 pred = pm ? adpcm_pred(s.pvq[ch][b], yh[ch][b]) : 0;
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
    M_smp_v D;
    D.ab = a;
    D.hw = sample_coding(r, ch, a);
    D.pk = bnd;
    D.nr = bnd ? near_bucket(dist, (S128)ss << AQ_F, 17) : 0;
    // (the encoder's arithmetic errs by about its signal's level: the channel's loudest subband's)
    const S128 lvu = (S128)(r.shuff[ch] == 6 ? SF7 : SF6)[imax(esmax[ch] - 1, 0)] << 38;
    D.nl = bnd ? near_bucket(dist, lvu, 28) : 0;
    D.ql = bnd ? imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21) : 0;
    D.hm = imin(__builtin_popcount(miss[ch][b]), 3);
    D.pm = pm;
    D.bd = b;
    D.m1 = q1[ch][b] ? imin(q1[ch][b] - 1, 21) : 22;
    D.m2 = q2[ch][b] ? imin(q2[ch][b] - 1, 21) : 22;
    D.sq = bnd ? svar(qp) : 0;
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
    const bool je = bnd && x96a && ss_rule() && jx_low(b) && je_on(ch, b);
    if (je) {
      S128 d1;
      S64 o1;
      qe = requant(je_one(ch, b, j) - shl64(pred, AQ_F), ss, lo, hi, &d1, &o1);
      if (qe != qp && (U64)je_qn[b][1] * 4 > (U64)je_qn[b][0] * 3) jd = 1, alt = qe;
    }
    v = code_index(D, v, bnd, qp, alt, lo, hi, &how, &hcal[b], dist, !ss_rule() ? (S128)tlev[ch][j] << 22 : lvu,
                   (pc >= 8 ? 3 : pc >= 3 ? 2 : pc >= 1 ? 1 : 0) + 4 * jd, bnd ? (S128)ss << AQ_F : 0,
                   __builtin_popcount(miss[ch][b]));
    if (je) {
      je_qn[b][0]++, je_qn[b][1] += qe == v;
      if (je_qn[b][0] >= 1u << 16) je_qn[b][0] >>= 1, je_qn[b][1] >>= 1;
    }
    miss[ch][b] = (U8)(miss[ch][b] << 1 | (bnd && how != 0));
    if (bnd) chm[ch] = chm[ch] << 1 | (how != 0);
    n_smp++;
    n_hit += how == 0;
    q2[ch][b] = q1[ch][b];
    q1[ch][b] = 1 + bit_len((U64)(v < 0 ? -v : v));
    qs[ch][b] = (S8)svar(v);
    const S32 y = clip23((S64)dequantize((S32)v, ss) + pred);
    push_y(ch, b, y);
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
  S64 code_index(const M_smp_v& D, S64 v, int pk, S64 qp, S64 alt, S64 lo, S64 hi, int* how, HitCal* hc = nullptr,
                 S128 dist = 0, S128 unit = 0, int cb = 0, S128 su = 0, int hn = -1) {
    *how = 2;
    if (pk) {
      const int kd = D.ab == 27 ? 2 : D.pm;
      M_hit_v H;
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
        U32* const ce[4] = {hit_c[kd][hq][cb][j0][i0], hit_c[kd][hq][cb][j0][i1], hit_c[kd][hq][cb][j1][i0],
                            hit_c[kd][hq][cb][j1][i1]};
        U32* const ro[2] = {hit_r[kd][hq][cb][i0], hit_r[kd][hq][cb][i1]};
        const U32 cw[4] = {(U32)(v0 * w0), (U32)(v0 * w1), (U32)(v1 * w0), (U32)(v1 * w1)}, rw[2] = {16u * w0, 16u * w1};
        const U64 rd[2] = {2 * (U64)ro[0][1] + 2 * HIT_ONE, 2 * (U64)ro[1][1] + 2 * HIT_ONE},
                  rn[2] = {HIT_PRIOR * HIT_ONE * (2 * (U64)ro[0][0] + HIT_ONE), HIT_PRIOR * HIT_ONE * (2 * (U64)ro[1][0] + HIT_ONE)};
        S64 lp = 0;  // (the weights' sum of the rates' log2, 2^-16 units: each cell's two)
        for (int k = 0; k < 4; k++) {
          if (!cw[k]) continue;
          const U32* ck = ce[k];
          const U64 r = rd[k & 1];
          lp += (S64)cw[k] * (lg2tab.log2x((U64)ck[0] * r + rn[k & 1]) - lg2tab.log2x(((U64)ck[1] + HIT_PRIOR * HIT_ONE) * r));
          lp += (S64)cw[k] * (lg2tab.log2x((U64)ck[2] * r + rn[k & 1]) - lg2tab.log2x(((U64)ck[3] + HIT_PRIOR * HIT_ONE) * r));
        }
        const U32 pq = lg2tab.prob(lp >> 9);
        hit = c->code_bit(pq, ENC && v == qp);
        if (hk) hc_learn(*hc, hit, pq);
        for (int k = 0; k < 4; k++)
          if (cw[k]) {
            ce[k][0] += hit ? 0 : cw[k], ce[k][2] += hit ? 0 : cw[k];
            if ((ce[k][1] += cw[k]) >= HIT_ONE * HIT_LIMIT) ce[k][0] >>= 1, ce[k][1] >>= 1;
            if ((ce[k][3] += cw[k]) >= HIT_ONE * HIT_SLIMIT) ce[k][2] >>= 1, ce[k][3] >>= 1;
          }
        for (int k = 0; k < 2; k++)
          if (rw[k]) {
            ro[k][0] += hit ? 0 : rw[k];
            if ((ro[k][1] += rw[k]) >= HIT_ONE * HIT_RLIMIT) ro[k][0] >>= 1, ro[k][1] >>= 1;
          }
        if (hit) return *how = 0, qp;
        B::sel(f_hit, H);  // (the family's rows for a miss only)
      } else {
        B::sel(f_hit, H);
        hit = f_hit.bit(HT_HIT, ENC && v == qp);  // (the LFE's misses are the fit's: by the family)
      }
      if (hit) return *how = 0, qp;
      if (alt != qp && f_hit.bit(HT_ALT, ENC && v == alt)) return *how = 1, alt;
      // (neither, the audio's 4 or more: where within HIT_NEAR of it, its side, alt's or the other
      // (up or down where none is), and its distance in unary, from 2 on alt's side (1 is alt), else
      // from 1; else its magnitude, below. A miss next to a small index costs less as its
      // magnitude: 96-24.dts's, the X96 residuals' in its core's values)
      if ((qp < 0 ? -qp : qp) >= 4) {
        const int sa = alt > qp ? 1 : alt < qp ? -1 : 0;
        const S64 d = ENC ? v - qp : 0, ad = d < 0 ? -d : d;
        if (f_hit.bit(HT_NEAR, ENC && ad <= HIT_NEAR)) {
          const int same = f_hit.bit(HT_SIDE, ENC && (sa ? (d > 0) == (sa > 0) : d > 0));
          int k = same && sa ? 2 : 1;
          while (k < HIT_NEAR && f_hit.bit(HT_DIST + (same ? HIT_NEAR : 0) + k, ENC && ad > k)) k++;
          const S64 w = qp + (same ? (sa ? sa : 1) : (sa ? -sa : -1)) * k;
          return w < lo ? lo : w > hi ? hi : w;
        }
      }
    }
    B::sel(f_smp, D);
    const S64 mmax = -lo > hi ? -lo : hi;
    const int maxnb = imax(bit_len((U64)mmax), 1);
    const S64 m = checked(code_len(f_smp, QN_LEN, maxnb, LV::smp::mt, ENC ? (v < 0 ? -v : v) : 0, ENC), 0, mmax);
    int neg = 0;
    if (m && m <= hi && m <= -lo) neg = f_smp.bit(QN_SIGN, ENC && v < 0);
    else if (m > hi) neg = 1;
    return neg ? -m : m;
  }

  // --- the LFE -----------------------------------------------------------------------------------------

  // The subframe's LFE scale factor v (coded with its first sample, the walk's order notwithstanding:
  // the samples are coded by its step): against the one the predictions' RMS makes
  int code_lfe_scale(int v) {
    S64 x[4 * NSSF];
    for (int i = 0; i < lfe_n; i++) x[i] = lfe_p[i] >> (LFE_Q - 12);  // (RMS / 16, in 2^-AQ_F: scale_pred()'s)
    int near;
    const int p = scale_pred(x, lfe_n, SF7, SF7_N, &near);
    M_scl_v D;
    tcx X_;
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
    f_scl.make(D, X_);
    f_scl.select(X_);
    if (f_scl.bit(CN_HIT, ENC && v == p)) v = p;
    else v = (int)checked(p + f_scl.codes(X_, ENC ? v - p : 0, 3), 0, 127);
    lfe_hm = v == p ? 1 : 2;
    lfe_lp = dvar(v - p);
    n_sf++;
    n_sfhit += v == p;
    lfe_scl = v;
    lfe_st = ss_rule() ? lfe_scale_dec(v) : lfe_scale(v);
    return v;
  }
  // the LFE's prediction from its features f: the fit's that has predicted the values more often
  // (with the last values the better against a decoded WAV, and for some lossless ones)
  // (the fixed one's on a tie: so from the stream's start)
  int lfe_best() const {
    int k = 3;
    for (int j = 0; j < 3; j++)
      if (lfh[j] > lfh[k]) k = j;
    if (lfh[4] > lfh[k]) k = 4;
    if (lfh[5] > lfh[k]) k = 5;
    return k;
  }
  bool lfe_poor() const {
    U32 b = lfw[0];
    for (int k = 1; k < 5; k++) b = lfw[k] > b ? lfw[k] : b;
    return 4 * (U64)b < 3 * (U64)lfwn;
  }
  // the DTS-HD MA encoders' filter's (LFE_W2, in 2^-30), FFmpeg's decodes' (LFE_W2D, in 2^-24: d)
  S64 lfe_fixed(int i, bool d = false) const {
    const S32* const w = d ? LFE_W2D : LFE_W2;
    S128 p = 0;
    for (int k = 0; k < LFE_N2; k++) p += (S128)w[k] * lfe_f3[i][k];
    return (S64)(p >> (d ? 24 : 30));
  }
  S64 lfe_pred(int i) const {
    const int k = lfe_best();
    return k == 5 ? lfit4[lfe_pl[i]].predict(lfe_f3[i])
         : k >= 3 ? lfe_fixed(i, k == 4)
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
    const S64 p1 = lfit.predict(lfe_f[i]), p2 = lfit2.predict(lfe_f[i]), p3 = lfit3.predict(lfe_f3[i]), p4 = lfe_fixed(i),
              p5 = lfe_fixed(i, true), p6 = lfit4[lfe_pl[i]].predict(lfe_f3[i]);
    const int lb = lfe_best();
    lfe_p[i] = lb == 5 ? p6 : lb == 4 ? p5 : lb == 3 ? p4 : lb == 2 ? p3 : lb == 1 ? p2 : p1;
    const S64 qp = requant(lfe_p[i], lfe_st << 6, -128, 127, &dist, &alt);
    M_smp_v D;
    D.ab = 27;
    D.hw = 0;
    D.pk = 1;
    D.nr = near_bucket(dist, (S128)lfe_st << 22, 17);
    D.nl = 0;
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
      const S64 pk[6] = {p1, p2, p3, p4, p5, p6};
      for (int k = 0; k < 6; k++) {
        const U32 h = requant(pk[k], lfe_st << 6, -128, 127, &dd, &al) == v;
        lfh[k] += h;
        if (k < 5) lfw[k] += h;
      }
      lfwn++;
      if (lfh[0] + lfh[1] + lfh[2] + lfh[3] >= 4u << 11)  // (by the first four's sum, as before the fifth)
        for (int k = 0; k < 6; k++) lfh[k] >>= 1;
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
