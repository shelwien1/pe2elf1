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
  int esmax[NCH];          // the channel's loudest subband's energy (es)
  int eab[NCH][NSUB];      // the energy ABITS was coded against (es; an ADPCM subband's residual's)
  U8 ab_seen[2][NSUB][129];  // the ABITS each subband had the last time it had that energy (by pm; 1 + it, 0 none)
  int pscale[NCH][NSUB];   // their first scale factor's prediction (1 + it, 0 none)
  int hm[NCH];             // whether the channel's scale factor before was the prediction (0 none, 1 it was, 2 not)
  S64 lpc[NCH][NSUB][4];   // the subframe's subbands' LPC (lpc4(): the encoder's ADPCM predictor is the entry nearest)
  int lpg[NCH][NSUB];      // and its prediction gain in halvings
  int lpg4[NCH][NSUB];     // and in quarters of them
  U8 lrank[NCH][NSUB][VK_N];  // the subband's last rank of each kind (vqr's lh)
  KeyIx cand[4096];        // the candidates of a rank
  // the LFE (64x) against the WAV's LFE channel (bind.lfe)
  int lfe_on;              // the subframe's LFE samples are coded against the audio's
  int lfe_n;               // how many
  S64 lfe_f[4 * NSSF][LFE_NT];  // each one's decimated samples around it
  S64 lfe_p[4 * NSSF];     // and its prediction (the value times its step)
  int lfe_scl;             // their scale factor (the walk has it after them: coded with the first)
  S64 lfe_st;              // its step
  int lfe_hm, lfe_lp;      // whether the last was the prediction (as hm), the last less its prediction (as lp)
  S64 lfe_cn[64], lfe_cv[64];  // the decimated samples by place (n mod 64: n, the value)
  S32 lq1, lq2;            // the last two LFE indexes (as q1, q2, qs, miss)
  S8 lqs;
  U8 lmiss;
  LfeFit lfit;
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
    for (int i = 0; i < 64; i++) lfe_cn[i] = -1;
    lfe_lp = 9;
  }
  // the LFE's decimated sample n (64x: its window ends at 64 n + the offset)
  S64 lfe_d(S64 n) {
    const int i = (int)(n & 63);
    if (lfe_cn[i] != n) lfe_cn[i] = n, lfe_cv[i] = lfe_decim(*wav, bind.lfe, 64 * n + bind.off);
    return lfe_cv[i];
  }
  // a core frame not coded as records (kept as bytes): its blocks go by
  void skip_blocks(int n) { tnext += n; }

  // --- the subframe's audio --------------------------------------------------------------------------

  // the subframe sf of the frame c starts (its SSC known): the audio's subband samples of its bound
  // channels, and their subbands' energy and how it goes
  void smp_subframe(const CoreRec& r, int sf) {
    const CoreSub& s = r.sub[sf];
    t0 = tframe + r.nblk;
    nsmp = s.nssc * 8;
    fir = r.filts ? FIR_PERFECT : FIR_NONPERFECT;
    for (int ch = 0; ch < NCH; ch++) bound[ch] = 0;
    lfe_on = 0;
    if (!wav || !wav->ok) return;
    const S64 o0 = 32 * t0 + bind.off;
    S64 lo = o0 - 512, hi = o0 + 32 * (S64)(nsmp - 1);
    // the LFE (64x only), its samples' windows and the correction's around them
    const S64 n0 = t0 / 2;
    if (r.lff == 2 && bind.lfe >= 0 && bind.lfe < wav->nch) {
      lfe_on = 1;
      lfe_n = 2 * r.lff * s.nssc;
      lo = imin(lo, 64 * (n0 - LFE_K) + bind.off - 512);
      hi = imax(hi, 64 * (n0 + lfe_n - 1 + LFE_K) + bind.off);
    }
    wav->need(lo, hi);
    for (int i = 0; lfe_on && i < lfe_n; i++) {
      for (int k = 0; k < LFE_NT; k++) lfe_f[i][k] = lfe_d(n0 + i + k - LFE_K);
      lfe_p[i] = lfit.predict(lfe_f[i]);
    }
    for (int ch = 0; ch < r.npchs; ch++) {
      if (!bind.bound(ch, wav->nch)) continue;
      bound[ch] = 1;
      S64 x[32];
      for (int j = 0; j < nsmp; j++) {
        analyze_bound(*wav, bind, ch, o0 + 32 * j, fir, x);
        for (int b = 0; b < NSUB; b++) X[ch][b][j] = x[b];
      }
      const U32* tab = r.shuff[ch] == 6 ? SF7 : SF6;
      const int nt = sf_entries(r.shuff[ch]);
      for (int b = 0; b < r.nsubs[ch]; b++) {  // (the LPC over the subframe and the 4 before it)
        S64 x[4 + NSMP];
        for (int k = 0; k < 4; k++) x[3 - k] = xh[ch][b][k];
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
    if (lfe_on) lfit.refit();
    for (int ch = 0; ch < r.npchs; ch++) {
      if (!bound[ch]) continue;
      for (int b = 0; b < NSUB; b++)
        for (int k = 0; k < 4; k++) xh[ch][b][k] = X[ch][b][nsmp - 1 - k];
    }
  }

  // the audio's values a scale factor scales (n of them into v): channel ch's subband b over its
  // subsubframes j0 to j1 (8 samples each), less the ADPCM prediction from its own samples where
  // the subband predicts (the open loop's: the decoder's samples aren't decoded yet)
  int scale_values(const CoreSub& s, int ch, int b, int j0, int j1, S64* v) {
    S64 h[4] = {xh[ch][b][0], xh[ch][b][1], xh[ch][b][2], xh[ch][b][3]};
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
  // the rank of index v among the n candidates (c), or the index of the rank coded (d); kind kd
  int code_cand(int n, int nb, int v, int kd, int ch, int b, int gn) {
    M_vqr_v D;
    D.kd = kd;
    D.cc = chan_class(ch);
    D.bd = b;
    D.mg = key_margin(cand, n);
    D.gn = gn;
    D.lh = lrank[ch][b][kd];
    B::sel(f_vqr, D);
    int at = 0;
    if (ENC)
      while (cand[at].i != v) at++;
    const int r = code_rank(ENC ? rank_of(cand, n, at) : 0, nb);
    if (!ENC) v = select_rank(cand, n, r);
    lrank[ch][b][kd] = r ? 2 : 1;
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
    S64 bd = pvq_dist(0, lpc[ch][b]);
    for (int q = 1; q < 4096; q++) {
      const S64 d = pvq_dist(q, lpc[ch][b]);
      if (d < bd) bd = d, bq = q;
    }
    return clip(pvq_gain4(bq, xh[ch][b], X[ch][b], nsmp) - 2, 1, 8);
  }
  // Channel ch's subband b's ADPCM predictor v: its rank by the distance from the subband's LPC
  S64 code_pvq(int ch, int b, S64 v) {
    for (int q = 0; q < 4096; q++) cand[q].k = pvq_dist(q, lpc[ch][b]), cand[q].i = q;
    return code_cand(4096, 12, (int)v, VK_PVQ, ch, b, lpg[ch][b]);
  }
  // Channel ch's subband b's HF VQ vector v (its scale factor coded): the rank of its group by the
  // groups' means, then its rank in the group
  S64 code_hfvq(const CoreRec& r, const CoreSub& s, int ch, int b, S64 v) {
    const int si = s.scale[ch][b][0];
    const S64 sc = r.shuff[ch] == 6 ? SF7[si] : SF6[si & 63];
    const S64* x = X[ch][b];
    for (int g = 0; g < 32; g++) cand[g].k = hf_group_dist(g, x, nsmp, sc), cand[g].i = g;
    const int g = code_cand(32, 5, (int)v >> 5, VK_HFG, ch, b, imin(si >> 3, 15));
    for (int i = 0; i < 32; i++) cand[i].k = hf_dist(g * 32 + i, x, nsmp, sc), cand[i].i = i;
    return g * 32 + code_cand(32, 5, (int)v & 31, VK_HFV, ch, b, imin(si >> 3, 15));
  }

  // --- the scale factors -----------------------------------------------------------------------------

  // Scale factor j (0 the first, 1 the second, after a transient) of channel ch's subband b, v in
  // 0..max-1: against the one the audio's RMS makes, where the channel is bound, else the last
  S64 code_scale(const CoreRec& r, const CoreSub& s, int ch, int b, int j, S64 v) {
    const int hfb = b >= r.nvqsub[ch], tm = hfb ? 0 : s.tmode[ch][b];
    const int max = r.shuff[ch] == 6 ? 128 : 64;
    M_scl_v D;
    tcx X_;
    int p = -1, near = SB_N - 1;
    if (bound[ch]) {
      S64 v_[NSMP];
      const int n = tm ? (j ? scale_values(s, ch, b, tm, s.nssc, v_) : scale_values(s, ch, b, 0, tm, v_))
                       : scale_values(s, ch, b, 0, s.nssc, v_);
      if (n) p = scale_pred(v_, n, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
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
    push_y(ch, b, clip23((S64)dequantize((S32)v, ss) + pred));
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
  // LFE sample i of the subframe (its value v): against its prediction at the scale factor's step
  S64 code_lfe(int i, S64 v) {
    S64 alt;
    S128 dist;
    int how;
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
    for (int ch = 0; ch < r.npchs; ch++)
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
        }
        if (b >= r.nsubs[ch] || (b < r.nvqsub[ch] && !s.abits[ch][b])) q1[ch][b] = q2[ch][b] = 0, qs[ch][b] = 0;
      }
  }
};
