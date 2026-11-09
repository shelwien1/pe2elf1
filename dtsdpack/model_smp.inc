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
  int bound[NCH];  // whether the subframe's channel has the audio's subband samples (X)
  S64 X[NCH][NSUB][NSMP];  // the audio's subband samples in the subframe (2^-AQ_F)
  S64 xh[NCH][NSUB][4];    // the audio's last 4 before the subframe (xh[.][.][0] the last)
  S32 yh[NCH][NSUB][4];    // the decoder's last 4
  S32 q1[NCH][NSUB], q2[NCH][NSUB];  // the subband's last two indexes (1 + their length, 0 none; with their signs in qs)
  S8 qs[NCH][NSUB];
  U8 miss[NCH][NSUB];      // the subband's last 8 indexes, a bit each: not the audio's
  int ac[NCH][NSUB], es[NCH][NSUB], ec[NCH][NSUB];  // the subframe's subbands' autocorrelation, energy, its change
  int eq[NCH][NSUB];       // and where in its scale factor's bucket the energy lies (scale_quarter())
  int esmax[NCH];          // the channel's loudest subband's energy (es)
  int eab[NCH][NSUB];      // the energy ABITS was coded against (es; an ADPCM subband's residual's)
  U8 ab_seen[2][NSUB][129];  // the ABITS each subband had the last time it had that energy (by pm; 1 + it, 0 none)
  // the bit allocations' staircases (stair_guess()), by pass (the core's, the extension's), pm and
  // subband; each pass's level
  static constexpr int AL_ND = 128, AL_NK = 2 * AL_ND + 1;  // (the level's search: 4 doublings either side)
  static constexpr int AB_RC = 256;  // (the rate control's: 8 doublings either side of the last level)
  static constexpr S32 FE_NONE = -(1 << 30), AL_C = 1 << 13;
  struct AbStair {
    S32 lo[32], hi[32];  // each ABITS's energies, less the level
    U32 has;             // (a bit per ABITS)
  } ab_st[2][2][NSUB];
  struct AbLevel {
    S32 c0, d;      // the subframe's centre and the shift from it
    S32 sc[AL_NK];  // how many of the subframe's ABITS each shift (less AL_ND) made the staircases'
    S64 cost, tgt;  // their cost (ab_bits()), and what the next subframe's aims at (the last ones' average)
    int n, set;     // (how many; whether the centre is set: at the subframe's first ABITS)
  } abl[2];
  S32 fe_s[NCH][NSUB];  // the subframe's subbands' fine energies (FE_NONE: none)
  U64 n_st, n_sthit;  // (-s: the ABITS a staircase said, those it said right)
  int pscale[NCH][NSUB];   // their first scale factor's prediction (1 + it, 0 none)
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
  // the same of the HF VQ groups' nearest entries (code_hfvq()): how often one was near, of how
  // many tries; the tries (to 2048); the vectors
  U32 hfr[2], hf_n;
  U64 n_hfvq;
  S64 lpc[NCH][NSUB][4];   // the subframe's subbands' LPC (lpc4(): the encoder's ADPCM predictor is the entry nearest)
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
  S64 lfe_past[LFE_NP];    // the last LFE values (times their steps; [0] the last)
  S64 lfe_p[4 * NSSF];     // and its prediction (the value times its step)
  int lfe_scl;             // their scale factor (the walk has it after them: coded with the first)
  S64 lfe_st;              // its step
  int lfe_hm, lfe_lp;      // whether the last was the prediction (as hm), the last less its prediction (as lp)
  S64 lfe_cn[64], lfe_cv[64];  // the decimated samples by place (n mod 64: n, the value)
  S32 lq1, lq2;            // the last two LFE indexes (as q1, q2, qs, miss)
  S8 lqs;
  U8 lmiss;
  LfeFit<LFE_NT> lfit;      // the correction without the last values
  LfeFit<LFE_NF> lfit2;     // and with them
  U32 lfh[2];               // how often each one's prediction was the value (halved as they grow)
  // X96's, where the frame has it and the WAV is at twice the core's rate: each bound channel's
  // 64-band analysis by block of the frame, and the core's decoded samples (X96's residuals are the
  // one less the other: model_x96.inc)
  int x96a;  // the frame's core pass keeps them
  int fb0;   // the subframe's first block in the frame
  S64 xa[NCH][NSUB96][128];
  S32 yc[NCH][NSUB][128];
  U64 n_smp, n_hit, n_sf, n_sfhit;  // (-s: the samples, those the audio predicted; the same of the scale factors)
  U64 n_vq[VK_N], n_vqhit[VK_N];    // (and the ranks, those 0)
  // how often a subband sample's index wasn't the audio's, of how many (code_index()), by pm, hm and
  // nearness: in each cell (nr, nl), and in each row (nl)
  static constexpr U32 HIT_PRIOR = 16, HIT_LIMIT = 1u << 18;
  U32 hit_c[2][4][QB_N][QB_N][2], hit_r[2][4][QB_N][2];

  void smp_start(WavIn* w, const Bind& b) {
    wav = w;
    bind = b;
    tnext = 0;
    lfit.init();
    lfit2.init();
    for (int i = 0; i < 64; i++) lfe_cn[i] = -1;
    lfe_lp = 9;
  }
  // the LFE's decimated sample n (64x: its window ends at 64 n + the offset)
  S64 lfe_d(S64 n) {
    const int i = (int)(n & 63);
    if (lfe_cn[i] != n) lfe_cn[i] = n, lfe_cv[i] = lfe_decim(*wav, bind.lfe, 64 * bind.up * n + bind.off, bind.up);
    return lfe_cv[i];
  }
  // a core frame not coded as records (kept as bytes): its blocks go by
  void skip_blocks(int n) { tnext += n; }

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
    fir = r.filts ? FIR_PERFECT : FIR_NONPERFECT;
    for (int ch = 0; ch < NCH; ch++) bound[ch] = 0;
    lfe_on = 0;
    if (!wav || !wav->ok) return;
    const int up = bind.up;
    const S64 o0 = 32 * up * t0 + bind.off;
    S64 lo = o0 - 512 * up, hi = o0 + 32 * up * (S64)(nsmp - 1);
    // the LFE (64x only), its samples' windows and the correction's around them
    const S64 n0 = t0 / 2;
    if (r.lff == 2 && !r.xpass && bind.lfe >= 0 && bind.lfe < wav->nch) {
      lfe_on = 1;
      lfe_n = 2 * r.lff * s.nssc;
      lo = imin(lo, 64 * up * (n0 - LFE_K) + bind.off - 512 * up);
      hi = imax(hi, 64 * up * (n0 + lfe_n - 1 + LFE_K) + bind.off);
    }
    wav->need(lo, hi);
    S64 past[LFE_NP];  // (for the scale factor's prediction: the subframe's values as predicted)
    for (int k = 0; k < LFE_NP; k++) past[k] = lfe_past[k];
    for (int i = 0; lfe_on && i < lfe_n; i++) {
      for (int k = 0; k < LFE_NT; k++) lfe_f[i][k] = lfe_d(n0 + i + k - LFE_K);
      for (int k = 0; k < LFE_NP; k++) lfe_f[i][LFE_NT + k] = past[k];
      lfe_p[i] = lfe_pred(lfe_f[i]);
      for (int k = LFE_NP - 1; k > 0; k--) past[k] = past[k - 1];
      past[0] = lfe_p[i];
    }
    for (int ch = r.ch0(); ch < r.ch1(); ch++) {
      if (!bind.bound(ch, wav->nch)) continue;
      bound[ch] = 1;
      S64 x[64];
      for (int j = 0; j < nsmp; j++) {
        analyze_bound(*wav, bind, ch, o0 + 32 * up * j, fir, x, x96a ? NSUB96 : NSUB);
        for (int b = 0; b < NSUB; b++) X[ch][b][j] = x[b];
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
    }
  }
  // the subframe done: the audio's last subband samples, as the next one's history; the LFE's
  // correction fit again
  void smp_subframe_end(const CoreRec& r) {
    if (lfe_on) lfit.refit(), lfit2.refit();
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
  // The pass's level: from the subframe's centre, the rate control's (level_centre(): where the
  // staircases' ABITS would cost what the last subframes' did), the shift at which the staircases
  // made the most of the subframe's ABITS so far (within AL_ND; of those, the nearest).
  // the fine energy of the n values v: 32 times the log2 of their mean square (FE_NONE: all 0)
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
  // cost what the last subframes' did (the nearer of the two either side; within AB_RC)
  void level_centre(const CoreRec& r, const CoreSub& s, int p) {
    AbLevel& L = abl[p];
    L.set = 1;
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      for (int b = 0; b < r.nvqsub[ch]; b++) {
        fe_s[ch][b] = FE_NONE;
        if (!bound[ch]) continue;
        S64 x[NSMP];
        const int n = scale_values(s, ch, b, 0, s.nssc, x);
        fe_s[ch][b] = fine_energy(x, n);
      }
    if (L.tgt <= 0) return;
    auto cost = [&](S32 l) {
      S64 c = 0;
      for (int ch = r.ch0(); ch < r.ch1(); ch++)
        for (int b = 0; b < r.nvqsub[ch]; b++)
          if (fe_s[ch][b] != FE_NONE) c += ab_bits(stair_guess(ab_st[p][s.pmode[ch][b]][b], fe_s[ch][b] - l));
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
  // pass p's guess on the staircase t at the energy x
  int level_guess(int p, const AbStair& t, S32 x) const { return stair_guess(t, x - abl[p].c0 - abl[p].d); }
  // and how sure it is, its guess g: how near x lies to the energies where the guess changes, the
  // nearer's side (0 none; 1-6 below, 7-12 above: within 0, 1, 3, 7, 15 32nds of a doubling or
  // farther)
  int level_margin(int p, const AbStair& t, S32 x, int g) const {
    S64 a, z;
    if (g < 0 || !stair_region(t, g, &a, &z)) return 0;
    const S64 y = (S64)x - abl[p].c0 - abl[p].d, lo = y - a, hi = z - y, m = lo < hi ? lo : hi;
    return 1 + 6 * (lo >= hi) + (m < 1 ? 0 : m < 2 ? 1 : m < 4 ? 2 : m < 8 ? 3 : m < 16 ? 4 : 5);
  }
  // and its ABITS was v: the shifts at which the staircase would have said it count, then the
  // staircase has it
  void level_add(int p, AbStair& t, S32 x, int v) {
    AbLevel& L = abl[p];
    S64 a, z;
    if (stair_region(t, v, &a, &z)) {  // (x - c0 - (k - AL_ND) in a..z)
      const S64 k0 = clip((S64)x - L.c0 - z + AL_ND, 0, AL_NK), k1 = clip((S64)x - L.c0 - a + AL_ND, -1, AL_NK - 1);
      for (S64 k = k0; k <= k1; k++) L.sc[k]++;
    }
    stair_add(t, x - L.c0 - L.d, v);
    L.cost += ab_bits(v), L.n++;
    int bk = AL_ND;  // (a quarter of an ABITS's score off for each unit from the centre)
    S32 bs = 4 * L.sc[AL_ND];
    for (int k = 1; k <= AL_ND; k++) {
      const S32 s1 = 4 * L.sc[AL_ND - k] - k, s2 = 4 * L.sc[AL_ND + k] - k;
      if (s1 > bs) bs = s1, bk = AL_ND - k;
      if (s2 > bs) bs = s2, bk = AL_ND + k;
    }
    L.d = bk - AL_ND;
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

  // A rank r in 0..2^nb - 1 (nb <= RANK_BITS), vqr's contexts selected: its bucket (0; 1; 2-3;
  // 4-7; ...) in unary, then its bits below the bucket's top
  int code_rank(int r, int nb) {
    const int bk = ENC ? bit_len((U64)r) : 0;
    int t = 0;
    while (t < nb && f_vqr.bit(VN_BKT + t, ENC && bk > t)) t++;
    if (t < 2) return t;
    int m = 1;
    for (int p = 0; p < t - 1; p++) m = 2 * m + f_vqr.bit(VN_BIT + 3 * t + imin(p, 2), ENC && ((r >> (t - 2 - p)) & 1));
    return m;
  }
  // the rank of index v among the n candidates (c), or the index of the rank coded (d); kind kd, the
  // subband's last rank of the kind lh (lrank's, or X96's)
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn) { return code_cand(n, nb, v, kd, ch, b, gn, lrank[ch][b][kd]); }
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn, U8& lh) {
    M_vqr_v D;
    D.kd = kd;
    D.cc = chan_class(ch);
    D.bd = b;
    D.mg = key_margin(cand, n);
    D.gn = gn;
    D.lh = lh;
    B::sel(f_vqr, D);
    int at = 0;
    if (ENC)
      while (cand[at].i != v) at++;
    const int r = code_rank(ENC ? rank_of(cand, n, at) : 0, nb);
    if (!ENC) v = select_rank(cand, n, r);
    lh = r ? 2 : 1;
    n_vq[kd]++;
    n_vqhit[kd] += !r;
    return v;
  }
  // How the encoder would gain by predicting channel ch's subband b: 0 by less than 2^(1/2) (the
  // LPC's gain), 9 by more than 2^(9/4), else by the gain of the entry nearest the LPC over the
  // subframe, in quarters of halvings less 2 (1-8)
  int pmode_gain(int ch, int b) {
    const int k = lpg4[ch][b];
    if (k < 2 || k > 9) return k < 2 ? 0 : 9;
    int bq = 0;
    S64* const d = this->pvqd;
    pvq_dists(lpc[ch][b], d);
    for (int q = 1; q < 4096; q++)
      if (d[q] < d[bq]) bq = q;
    S64 hd[4];
    hist4(ch, b, hd);
    return clip(pvq_gain4(bq, hd, X[ch][b], nsmp) - 2, 1, 8);
  }
  // Channel ch's subband b's ADPCM predictor v: its rank by the distance from the subband's LPC.
  // (Where the scale factor search's rule is taken, a decoded WAV's: of the first 16 in that order,
  // the first whose prediction leaves values that fit a quantizer's lattice, by the bit allocation
  // search, to a 64th of a step squared a value, first: a decoded WAV's values less the decoder's
  // prediction are its dequantized indexes.)
  S64 code_pvq(const CoreRec& r, const CoreSub& s, int ch, int b, S64 v) {
    S64* const d = this->pvqd;
    pvq_dists(lpc[ch][b], d);
    for (int q = 0; q < 4096; q++) cand[q].k = d[q], cand[q].i = q;
    if (ss_rule()) {
      const int K = pv_none * 4 > pv_n ? 4 : 16;  // (fewer where few fit: a 96/24 stream's core's)
      int top[16], nk = 0;  // (the order's first K, the first first)
      for (int q = 0; q < 4096; q++) {
        int k = nk;
        while (k > 0 && key_less(cand[q], cand[top[k - 1]])) k--;
        if (k >= K) continue;
        for (int z = imin(nk, K - 1); z > k; z--) top[z] = top[z - 1];
        top[k] = q;
        nk = imin(nk + 1, K);
      }
      for (int k = 0; k < nk; k++) {
        S64 x[NSMP];
        S64 h[4] = {xh[ch][b][0], xh[ch][b][1], xh[ch][b][2], xh[ch][b][3]};
        const int n = s.nssc * 8;
        for (int j = 0; j < n; j++) {
          const S64 y = X[ch][b][j];
          x[j] = y - adpcm_pred_x(top[k], h);
          h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
        }
        int near;
        const int e = 1 + scale_pred(x, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
        const U8 sv = ab_seen[1][b][imin(e, 128)];
        const S32 og = sv ? sv : hist.abits[ch][b];
        if (!og) break;
        const int g = og - 1, a0 = imax(1, g - 3), a1 = imin(26, g + 3);
        int conf;
        U64 d = ~0ull;
        if (abits_search(r, ch, x, n, g, a0, a1, e - 1, &conf, &d, (U64)n << 10)) {
          cand[top[k]].k = -1;
          break;
        }
        pv_none += k == nk - 1;
      }
      if (++pv_n >= 1u << 16) pv_n >>= 1, pv_none >>= 1;
    }
    return code_cand(4096, 12, (int)v, VK_PVQ, ch, b, lpg[ch][b]);
  }
  // Channel ch's subband b's HF VQ vector v (its scale factor coded): the rank of its group by the
  // groups' centroids (anal.inc's HF_GC), then its rank in the group
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
    S128 gm[32], md[32], en = 0;
    int f0 = 0;
    for (int g = 0; g < 32; g++) gm[g] = hf_group_dist(g, x, nsmp, sc), f0 = gm[g] < gm[f0] ? g : f0;
    int mn = 0;
    if (tried) {
      U8 jo[NSMP];  // (the samples, the largest first)
      auto mag = [](S64 a) { return a < 0 ? -a : a; };
      for (int j = 0; j < nsmp; j++) {
        int k = j;
        while (k > 0 && mag(x[jo[k - 1]]) < mag(x[j])) jo[k] = jo[k - 1], k--;
        jo[k] = (U8)j;
        en += (S128)x[j] * 16 * ((S128)x[j] * 16);  // (hf_dist()'s units)
      }
      S128 near = hf_group_min(f0, x, jo, nsmp, sc, en + 1);  // (the nearest of all: the centroids' first group's first)
      for (int g = 0; g < 32; g++)
        if (g != f0) near = hf_group_min(g, x, jo, nsmp, sc, near);
      const S128 cap = 16 * near + 1;
      for (int g = 0; g < 32; g++) md[g] = hf_group_min(g, x, jo, nsmp, sc, cap);
      mn = near * 64 < en;
    }
    for (int g = 0; g < 32; g++) cand[g].k = mn ? md[g] : gm[g], cand[g].i = g;
    const int g = code_cand(32, 5, (int)v >> 5, VK_HFG, ch, b, imin(si >> 3, 15));
    if (tried) {
      hfr[0] += mn, hfr[1]++;
      if (hfr[1] >= 1u << 16) hfr[0] >>= 1, hfr[1] >>= 1;
      hf_n += hf_n < 2048;
    }
    n_hfvq++;
    for (int i = 0; i < 32; i++) cand[i].k = hf_dist(g * 32 + i, x, nsmp, sc), cand[i].i = i;
    return g * 32 + code_cand(32, 5, (int)v & 31, VK_HFV, ch, b, imin(si >> 3, 15));
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
        for (int g = 0; g < 32; g++) cand[g].k = hf_group_dist(g, v, n, sc), cand[g].i = g;
        const int g = select_rank(cand, 32, 0);
        for (int e = 0; e < 32; e++) cand[e].k = hf_dist(g * 32 + e, v, n, sc), cand[e].i = e;
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
  // the same of the bit allocation search, of the ABITS
  bool ab_rule() const { return abr[1] > abr[0] + 32 + abr[0] / 32; }
  bool ab_search_on() const { return ab_n < 2048 || abr[1] * 4 > abr[0] * 3 || !(n_ab & 63); }
  bool hf_min_on() const { return hf_n < 2048 || hfr[0] * 8 > hfr[1] || !(n_hfvq & 63); }

  // The ABITS a decoded WAV says (the bit allocation search): its values v (n of them, the ADPCM
  // prediction off) are what the decoder made of indexes at the step of the ABITS and a scale
  // factor; so of ABITS a0..a1 (g the guess) and the scale factors p - 3 to p + 5, the pair whose
  // lattice lies nearest them in its steps (a finer one fits as nearly but in more of its steps, a
  // coarser one misses the values between its points): 1 + its ABITS, where an index it makes isn't
  // 0 (else 0: the values say nothing); *conf the log2 of the next pair's distance over its, 0-7,
  // *dbest its distance (norm_dist()'s). (Only the pairs under cut: 0 where none is.)
  // (The pairs by their distance at the 2 largest values, each one's whole distance, the largest
  // values first, while it could still be less than the second best's: as trying them all does.)
  int abits_search(const CoreRec& r, int ch, const S64* v, int n, int g, int a0, int a1, int p, int* conf, U64* dbest,
                   U64 cut = ~0ull) {
    constexpr int NP = 27 * 9, NK = 2;
    const int nt = sf_entries(r.shuff[ch]), nk = imin(n, NK);
    auto mag = [](S64 x) { return x < 0 ? -x : x; };
    S64 w[NSMP];  // (the values, the largest first)
    for (int j = 0; j < n; j++) {
      int k = j;
      while (k > 0 && mag(w[k - 1]) < mag(v[j])) w[k] = w[k - 1], k--;
      w[k] = v[j];
    }
    struct Pair {
      U64 d;  // (its distance at the largest values: at most its whole one's)
      S128 e;
      int nz;
      S8 a, i;
    } pr[NP];
    QErr qe[NP];
    int np = 0;
    const int i0 = imax(p - 3, 0), i1 = imin(p + 5, nt - 1);
    // (from the guesses out, g's and p: of equal distances the nearer pair)
    const int ga = clip(g, a0, a1);
    for (int ka = 0; ka <= 2 * (a1 - a0); ka++) {
      const int a = ka & 1 ? ga + (ka + 1) / 2 : ga - ka / 2;
      if (a < a0 || a > a1) continue;
      S64 lo, hi;
      quant_range(r, ch, a, lo, hi);
      for (int ki = 0; ki <= 2 * (i1 - i0); ki++) {
        const int i = ki & 1 ? p + (ki + 1) / 2 : p - ki / 2;
        if (i < i0 || i > i1) continue;
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
    int ord[NP], no = 0;  // (by that distance, stably; those under cut)
    for (int k = 0; k < np; k++) {
      if (pr[k].d >= cut) continue;
      int m = no++;
      while (m > 0 && pr[ord[m - 1]].d > pr[k].d) ord[m] = ord[m - 1], m--;
      ord[m] = k;
    }
    if (!no) return 0;
    U64 d1 = ~0ull, d2 = cut;
    int best = -1, bnz = 0;
    for (int k = 0; k < no && pr[ord[k]].d < d2; k++) {
      const int at = ord[k];
      const QErr& q = qe[at];
      S128 e = pr[at].e, lim = ~(U128)0 >> 1;
      int nz = pr[at].nz, j = nk;
      if (d2 != ~0ull) {  // (a distance past lim is past d2's: in doubles, made larger)
        const double l = (double)d2 * (double)q.ss * (double)q.ss / 268435456.0 * (1 + 1e-9);
        if (l < 1e36) lim = (S128)l + 1;
      }
      while (j < n && e <= lim) e += q.err(w[j++], &nz);
      if (e > lim) continue;
      const U64 d = norm_dist(e, q.ss);
      if (d < d1) d2 = d1, d1 = d, best = at, bnz = nz;
      else if (d < d2) d2 = d;
    }
    if (best < 0 || !bnz) return 0;
    const U64 eps = 2 * (U64)n;  // (2^-15 steps squared a value)
    const U64 ratio = d2 == ~0ull ? ~0ull : (d2 + eps) / (d1 + eps);
    *conf = imin(bit_len(ratio) - 1, 7);
    *dbest = d1;
    return 1 + pr[best].a;
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
    const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
    const int nt = sf_entries(r.shuff[ch]);
    S64 lo, hi;
    quant_range(r, ch, a, lo, hi);
    auto part = [&](int j0, int j1) {  // (subsubframes j0..j1 - 1: their fit)
      const S64* v = x + 8 * j0;
      const int m = 8 * (j1 - j0);
      int near;
      const int p = scale_pred(v, m, tab, nt, &near);
      U64 bd = ~0ull;
      for (int i = imax(p - 3, 0); i <= imin(p + 5, nt - 1); i++) {
        const S64 ss = step_for(r, ch, a, i);
        if (ss <= 0) continue;
        QErr q;
        q.set(ss, lo, hi);
        S128 e = 0;
        int nz = 0;
        for (int k = 0; k < m; k++) e += q.err(v[k], &nz);
        const U64 d = norm_dist(e, ss);
        if (d < bd) bd = d;
      }
      return bd;
    };
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
    const U128 q = num / den;
    return q >> 64 ? ~0ull : (U64)q;
  }

  // Scale factor j (0 the first, 1 the second, after a transient) of channel ch's subband b, v in
  // 0..max-1: against the one the audio makes, where the channel is bound, else the last. Three
  // rules make it from the audio's values, the one right clearly more often taken: the RMS rule; the
  // first entry above 0.99 times the RMS (a 96/24 encoder's); the scale factor search (a decoded
  // WAV's; where it isn't sure, the better of the others'), its sureness then the context the RMS
  // rule's nearness is
  S64 code_scale(const CoreRec& r, const CoreSub& s, int ch, int b, int j, S64 v) {
    const int hfb = b >= r.nvqsub[ch], tm = hfb ? 0 : s.tmode[ch][b];
    const int max = r.shuff[ch] == 6 ? 128 : 64;
    M_scl_v D;
    tcx X_;
    int p = -1, pc = -1, ps = -1, near = SB_N - 1, conf = 0, tried = 0;
    if (bound[ch]) {
      S64 v_[NSMP];
      const int n = tm ? (j ? scale_values(s, ch, b, tm, s.nssc, v_) : scale_values(s, ch, b, 0, tm, v_))
                       : scale_values(s, ch, b, 0, s.nssc, v_);
      if (n) {
        p = scale_pred(v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
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
    const S32 A = hist.scale[ch][b], P = hist.pscale[ch][b];
    D.dp = p >= 0 ? sf_dup(r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), p) : 0;
    D.kd = hfb ? 2 : j;
    D.ab = hfb ? 0 : vvar(s.abits[ch][b] + 1, 27);
    D.pm = s.pmode[ch][b];
    D.pk = p >= 0;
    D.pn = near;
    D.lp = A && P ? dvar(A - P) : 9;
    D.hm = hm[ch];
    D.bd = b;
    D.cc = chan_class(ch);
    D.nb = bound[ch] && b ? clip((es[ch][b - 1] - es[ch][b]) / 2, -4, 4) + 4 : 9;
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
    if (!j) pscale[ch][b] = p + 1;
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
    if (bound[ch]) qp = requant(X[ch][b][j] - shl64(pred, AQ_F), ss, lo, hi, &dist, &alt);
    M_smp_v D;
    D.ab = a;
    D.hw = sample_coding(r, ch, a);
    D.pk = bound[ch];
    D.nr = bound[ch] ? near_bucket(dist, (S128)ss << AQ_F, 17) : 0;
    // (the encoder's arithmetic errs by about its signal's level: the channel's loudest subband's)
    D.nl = bound[ch] ? near_bucket(dist, ((S128)(r.shuff[ch] == 6 ? SF7 : SF6)[imax(esmax[ch] - 1, 0)] << 38), 28) : 0;
    D.ql = bound[ch] ? imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21) : 0;
    D.hm = imin(__builtin_popcount(miss[ch][b]), 3);
    D.pm = pm;
    D.bd = b;
    D.m1 = q1[ch][b] ? imin(q1[ch][b] - 1, 21) : 22;
    D.m2 = q2[ch][b] ? imin(q2[ch][b] - 1, 21) : 22;
    D.sq = bound[ch] ? svar(qp) : 0;
    D.s1 = qs[ch][b];
    int how;
    v = code_index(D, v, bound[ch], qp, alt, lo, hi, &how);
    miss[ch][b] = (U8)(miss[ch][b] << 1 | (bound[ch] && how != 0));
    n_smp++;
    n_hit += how == 0;
    q2[ch][b] = q1[ch][b];
    q1[ch][b] = 1 + bit_len((U64)(v < 0 ? -v : v));
    qs[ch][b] = (S8)svar(v);
    const S32 y = clip23((S64)dequantize((S32)v, ss) + pred);
    push_y(ch, b, y);
    if (x96a) yc[ch][b][fb0 + j] = y;
    return v;
  }
  // An index v (in lo..hi), smp's variables D: whether it is the audio's qp (where pk), whether the
  // one on the other side of the rounding boundary (alt), else its magnitude and sign; *how which
  // (0 the audio's, 1 the other side's, 2 coded)
  S64 code_index(const M_smp_v& D, S64 v, int pk, S64 qp, S64 alt, S64 lo, S64 hi, int* how) {
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
      B::sel(f_hit, H);
      int hit;
      if (kd < 2) {
        // a subband sample's index is the audio's but where the encoder's arithmetic and this
        // analysis's differ, by how near the boundary it lies: by how often an index this near
        // wasn't (the cell's counts, from its row's where it has few), straight to the coder (its
        // probabilities are finer than the families')
        U32 *cell = hit_c[kd][D.hm][D.nr][D.nl], *row = hit_r[kd][D.hm][D.nl];
        const U64 pr = ((U64)(2 * row[0] + 1) << CO_PBITS) / (2 * (U64)row[1] + 2);
        const U64 pc = (((U64)cell[0] << CO_PBITS) + HIT_PRIOR * pr) / (cell[1] + HIT_PRIOR);
        hit = c->code_bit((U32)(pc < 1 ? 1 : pc > CO_ONE - 1 ? CO_ONE - 1 : pc), ENC && v == qp);
        U32* const ts[2] = {cell, row};
        for (int k = 0; k < 2; k++) {
          ts[k][0] += !hit;
          if (++ts[k][1] >= HIT_LIMIT) ts[k][0] >>= 1, ts[k][1] >>= 1;
        }
      } else {
        hit = f_hit.bit(HT_HIT, ENC && v == qp);  // (the LFE's misses are the fit's: by the family)
      }
      if (hit) return *how = 0, qp;
      if (alt != qp && f_hit.bit(HT_ALT, ENC && v == alt)) return *how = 1, alt;
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
    for (int i = 0; i < lfe_n; i++) x[i] = shl64(lfe_p[i], 12);  // (RMS / 16, in 2^-AQ_F: scale_pred()'s)
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
    D.hm = lfe_hm;
    D.bd = 0;
    D.dp = sf_dup(SF7, SF7_N, p);
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
    lfe_st = lfe_scale(v);
    return v;
  }
  // the LFE's prediction from its features f: the fit's that has predicted the values more often
  // (with the last values the better against a decoded WAV, and for some lossless ones)
  S64 lfe_pred(const S64* f) const { return lfh[1] > lfh[0] ? lfit2.predict(f) : lfit.predict(f); }
  // LFE sample i of the subframe (its value v): against its prediction at the scale factor's step
  S64 code_lfe(int i, S64 v) {
    S64 alt;
    S128 dist;
    int how;
    for (int k = 0; k < LFE_NP; k++) lfe_f[i][LFE_NT + k] = lfe_past[k];  // (the values decoded)
    const S64 p1 = lfit.predict(lfe_f[i]), p2 = lfit2.predict(lfe_f[i]);
    lfe_p[i] = lfh[1] > lfh[0] ? p2 : p1;
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
    lfit.add(lfe_f[i], v * lfe_st);
    lfit2.add(lfe_f[i], v * lfe_st);
    {  // (whether each prediction's index was the value)
      S128 dd;
      S64 al;
      lfh[0] += requant(p1, lfe_st << 6, -128, 127, &dd, &al) == v, lfh[1] += requant(p2, lfe_st << 6, -128, 127, &dd, &al) == v;
      if (lfh[0] + lfh[1] >= 1u << 12) lfh[0] >>= 1, lfh[1] >>= 1;
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
