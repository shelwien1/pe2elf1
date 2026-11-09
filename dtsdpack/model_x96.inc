// model_x96.inc -- the model's X96 subbands (model.inc): each channel's, from X96's first subband to
// its last, the core's residuals below band 32 and the 96 kHz band above, against the 64-band
// analysis of a WAV at twice the core's rate (the core pass keeps it, and the core's decoded
// samples: model_smp.inc), the residuals that less the core's, as the encoder makes them (TS 102 114
// 6.2.2); each element coded as the core's are (model_smp.inc, model_side.inc), with histories of
// its own. Without such a WAV, against those alone.

template <int f_DEC, class LV>
struct ModelX96 : ModelSmp<f_DEC, LV> {
  typedef ModelSmp<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::checked;
  using B::xa;
  using B::yc;
  using B::cand;
  using B::side_code;
  using B::side_index;
  using B::abits_code;
  using B::code_cand;
  using B::code_index;
  static constexpr int NB = NSUB96;
  // the subframe's
  int nsmp9, bd9[NCH];          // its samples; whether the channel has the audio's (X9)
  S64 X9[NCH][NB][NSMP];        // the audio's subband samples (2^-AQ_F)
  int ac9[NCH][NB], es9[NCH][NB], ec9[NCH][NB], esmax9[NCH];
  S64 lpc9[NCH][NB][4];
  int lpg9[NCH][NB], lpg4_9[NCH][NB];
  int eab9[NCH][NB];            // the energy ABITS was coded against
  int psc9[NCH][NB];            // the scale factor's prediction (1 + it, 0 none)
  // across subframes and frames
  S64 xh9[NCH][NB][4];          // the audio's last 4 (xh9[.][.][0] the last)
  S32 yh9[NCH][NB][4];          // the decoder's
  S32 q1_9[NCH][NB], q2_9[NCH][NB];
  S8 qs9[NCH][NB];
  U8 miss9[NCH][NB];
  U8 lrank9[NCH][NB][VK_N];
  U8 ab_seen9[2][NB][65];       // (as ab_seen, at the 6-bit table's energies)
  int hm9[NCH];
  // the subband's last values (1 + them, 0 none): PMODE, predictor, ABITS, scale factor, its
  // prediction, VQ vector, joint scale factor (+ 129), energy
  S32 hpm[NCH][NB], hpv[NCH][NB], hab[NCH][NB], hsc[NCH][NB], hps[NCH][NB], hvq[NCH][NB], hjs[NCH][NB], hen[NCH][NB];
  U64 n_smp9, n_hit9, n_sf9, n_sfhit9;  // (-s: X96's samples and scale factors, those the audio predicted)
  // how often x96_scale()'s rules were the stream's, by the subband's kind (halved as they grow);
  // the search's tries (to 2048)
  U32 sfr9[3][3], ss9_n;

  // the step size times scale factor of channel ch's subband b (its ABITS 2 up: the quantizer's
  // ABITS + 1; no adjustment)
  static INLINE S64 x96_ss(const X96Sub& s, int ch, int b) {
    return (S64)LOSSY_QUANT[s.abits[ch][b] - 1] * SF6[s.scale[ch][b] & 63];
  }

  // the subframe sf starts (X96's pass): each bound channel's audio from the frame's analysis, less
  // the core's decoded samples below band 32; their LPC, autocorrelation, energy and how it goes
  void x96_subframe(const CoreRec& r, int sf) {
    const CoreSub& cs = r.sub[sf];
    B::sst = 0;
    nsmp9 = cs.nssc * 8;
    for (int ch = 0; ch < NCH; ch++) bd9[ch] = 0;
    if (!B::x96a) return;
    const int fb = r.nblk, n = nsmp9;
    for (int ch = 0; ch < r.npchs; ch++) {
      if (!B::bind.bound(ch, B::wav->nch)) continue;
      bd9[ch] = 1;
      esmax9[ch] = 0;
      for (int b = r.x96start; b < r.x96nsubs[ch]; b++) {
        S64* x = X9[ch][b];
        for (int j = 0; j < n; j++) x[j] = xa[ch][b][fb + j] - (b < NSUB ? shl64(yc[ch][b][fb + j], AQ_F) : 0);
        S64 h[4 + NSMP];
        for (int k = 0; k < 4; k++) h[3 - k] = xh9[ch][b][k];
        for (int j = 0; j < n; j++) h[4 + j] = x[j];
        lpc4(h, 4 + n, lpc9[ch][b], &lpg9[ch][b], &lpg4_9[ch][b]);
        S128 e = 0, cr = (S128)x[0] * xh9[ch][b][0], ee[NSSF] = {0, 0, 0, 0};
        for (int j = 0; j < n; j++) {
          e += (S128)x[j] * x[j];
          ee[j >> 3] += (S128)x[j] * x[j];
          if (j) cr += (S128)x[j] * x[j - 1];
        }
        ac9[ch][b] = ratio_bucket(cr, e, 8);
        int near;
        es9[ch][b] = 1 + scale_pred(x, n, SF6, SF6_N, &near);
        int up = 0;
        for (int k = 1; k < cs.nssc; k++) {
          const S128 a = ee[k - 1] + 1, z = ee[k];
          int hh = 0;
          while (hh < 7 && z > (a << (hh + 1))) hh++;
          if (z > a && hh + 1 > up) up = hh + 1;
        }
        ec9[ch][b] = cs.nssc > 1 ? 1 + up : 0;
        esmax9[ch] = imax(esmax9[ch], es9[ch][b]);
      }
    }
  }

  // the audio's values a scale factor scales (into v: the subframe's samples of channel ch's
  // subband b, less the ADPCM prediction where it predicts, the open loop's)
  int x96_values(const X96Sub& s, int ch, int b, S64* v) {
    S64 h[4] = {xh9[ch][b][0], xh9[ch][b][1], xh9[ch][b][2], xh9[ch][b][3]};
    const int pm = s.pmode[ch][b], pvq = s.pvq[ch][b];
    for (int j = 0; j < nsmp9; j++) {
      const S64 x = X9[ch][b][j];
      v[j] = pm ? x - adpcm_pred_x(pvq, h) : x;
      h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = x;
    }
    return nsmp9;
  }
  // how the encoder would gain by predicting channel ch's subband b (as ModelSmp's pmode_gain())
  int x96_pmode_gain(int ch, int b) {
    const int k = lpg4_9[ch][b];
    if (k < 2 || k > 9) return k < 2 ? 0 : 9;
    int bq = 0;
    S64 bdist = pvq_dist(0, lpc9[ch][b]);
    for (int q = 1; q < 4096; q++) {
      const S64 d = pvq_dist(q, lpc9[ch][b]);
      if (d < bdist) bdist = d, bq = q;
    }
    return clip(pvq_gain4(bq, xh9[ch][b], X9[ch][b], nsmp9) - 2, 1, 8);
  }

  // An element of X96's (fid: the core's names) of channel ch's subband b (j: the sample, or the VQ
  // vector's pair of subsubframes), its value v in lo..hi
  S64 x96_code(const CoreRec& r, int sf, int fid, int ch, int b, int j, S64 v, S64 lo, S64 hi) {
    const X96Sub& s = r.xs[sf];
    const int bd = bd9[ch], b0 = r.x96start;
    switch (fid) {
      case F_PMODE:
        return side_code(SF_PMODE, ch, b, v, 0, 1, 1, hpm[ch][b], b > b0 ? s.pmode[ch][b - 1] + 1 : 0, bd ? ac9[ch][b] : 8,
                         bd ? imin(lpg4_9[ch][b], 27) : 0, bd ? x96_pmode_gain(ch, b) : 9);
      case F_PVQ:
        if (bd) {
          for (int q = 0; q < 4096; q++) cand[q].k = pvq_dist(q, lpc9[ch][b]), cand[q].i = q;
          return code_cand(4096, 12, (int)v, VK_PVQ, ch, b, lpg9[ch][b], lrank9[ch][b][VK_PVQ]);
        }
        return side_index(SF_PVQ, ch, b, v, 12, hpv[ch][b], 8, 0);
      case F_ABITS: {  // (0 none, 1 VQ, else the quantizer's ABITS + 1: against the audio's energy, as ModelSmp's)
        int e = bd ? es9[ch][b] : 0;
        if (e && s.pmode[ch][b]) {
          S64 x[NSMP];
          int near;
          const int n = x96_values(s, ch, b, x);
          e = 1 + scale_pred(x, n, SF6, SF6_N, &near);
        }
        eab9[ch][b] = e;
        const int le = hen[ch][b];
        const int de = e && le ? dvar(e - le) : 9;
        const int ls = hsc[ch][b] && hps[ch][b] ? dvar(hsc[ch][b] - hps[ch][b]) : 9;
        const int lbd = b > b0 && hab[ch][b - 1] ? dvar(s.abits[ch][b - 1] - (hab[ch][b - 1] - 1)) : 9;
        int nb = 9, mx = 16;
        if (e) {
          const int n1 = b > b0 ? es9[ch][b - 1] : 0, n2 = b + 1 < r.x96nsubs[ch] ? es9[ch][b + 1] : 0;
          nb = clip((e - imax(n1, n2)) / 2, -4, 4) + 4;
          mx = imin((esmax9[ch] - e) / 4, 15);
        }
        U8* const sn = e ? &ab_seen9[s.pmode[ch][b]][b][imin(e, 64)] : nullptr;
        int nn = 15;
        if (sn && *sn) {
          const U8 *lo_ = sn - 1, *hi_ = e < 64 ? sn + 1 : nullptr;
          const int a = e > 1 && *lo_ ? clip(*lo_ - *sn, -1, 1) + 1 : 3, z = hi_ && *hi_ ? clip(*hi_ - *sn, -1, 1) + 1 : 3;
          nn = a * 4 + z;
        }
        int sv = sn ? *sn : 0, near_e = 0;
        for (int k = 1; sn && !sv && k <= 2; k++) {
          if (e - k >= 1 && sn[-k]) sv = sn[-k], near_e = 1;
          else if (e + k <= 64 && sn[k]) sv = sn[k], near_e = 1;
        }
        v = abits_code(ch, b, v, lo, hi, hab[ch][b], b > b0 ? s.abits[ch][b - 1] + 1 : 0, e, de, ls, s.pmode[ch][b], lbd, nb,
                       mx, sv, near_e ? 14 : nn, 0);
        if (sn) *sn = (U8)(checked(v, lo, hi) + 1);
        return v;
      }
      case F_SCALES:
        return x96_scale(r, s, ch, b, v);
      case F_JOIN_SHUFF:
        return side_code(SF_JSHUFF, ch, 0, v, lo, hi, 0, 0, 0, 8, 0, 9);
      case F_JOIN_SCALES: {  // (against the scale the audio's subband is of its source's, where the source has a level)
        const int src = r.x96joinx[ch] - 1;
        int p = -1, conf = 8;
        if (bd && src >= 0 && bd9[src]) p = x96_joint(r, ch, src, b, &conf);
        const S32 A = p >= 0 ? p + 65 : hjs[ch][b], P = p >= 0 ? hjs[ch][b] : 0;  // (coded 128 up: index p is p - 64)
        return side_code(SF_JSCALE, ch, b, v + 128, lo + 128, hi + 128, 0, A, P, conf, 0, 9) - 128;
      }
      case F_HFVQ:  // (a pair of subsubframes' vector: its 16 samples, at the subband's scale factor)
        if (bd && 16 * j < nsmp9) {
          const int si = s.scale[ch][b] & 63, n = imin(16, nsmp9 - 16 * j);
          const S64 sc = SF6[si];
          const S64* x = &X9[ch][b][16 * j];
          for (int g = 0; g < 32; g++) cand[g].k = hf_group_dist(g, x, n, sc), cand[g].i = g;
          const int g = code_cand(32, 5, (int)v >> 5, VK_HFG, ch, b, imin(si >> 3, 15), lrank9[ch][b][VK_HFG]);
          for (int i = 0; i < 32; i++) cand[i].k = hf_dist(g * 32 + i, x, n, sc), cand[i].i = i;
          return g * 32 + code_cand(32, 5, (int)v & 31, VK_HFV, ch, b, imin(si >> 3, 15), lrank9[ch][b][VK_HFV]);
        }
        return side_index(SF_HFVQ, ch, b, v, 10, hvq[ch][b], 8, 0);
      case F_q:
        return x96_q(r, s, ch, b, j, v, lo, hi);
      default:
        fatal("internal error: a field %s in X96", field_name[fid]);
    }
    return v;
  }

  // The joint scale factor (its index, 0-128) of channel ch's subband b nearest the projection of
  // the audio's subband on its source's over the subframe (a decoded WAV's: the decoder's scale),
  // where the source's mean square is 2 or more (in the decoder's units; -1 where not); *conf its
  // log2 less 1 (to 7): how sure
  int x96_joint(const CoreRec& r, int ch, int src, int b, int* conf) {
    const int fb = r.nblk;
    S128 es = 0, ex = 0;
    for (int j = 0; j < nsmp9; j++) {
      const S64 xs = xa[src][b][fb + j], xc = xa[ch][b][fb + j];
      es += (S128)xs * xs;
      ex += (S128)xc * xs;
    }
    const int lev = bitlen128((U128)(es / nsmp9)) - 1 - 2 * AQ_F;
    if (lev < 1 || ex <= 0) return -1;
    *conf = imin(lev - 1, 7);
    const S128 g = shl128(ex, 17) / es;  // (Q17)
    int lo = 0, hi = 128;  // (the last entry at most g)
    while (lo < hi) {
      const int m = (lo + hi + 1) >> 1;
      if ((S128)JOINT_SCALE[m] <= g) lo = m;
      else hi = m - 1;
    }
    if (lo < 128 && 2 * g >= (S128)JOINT_SCALE[lo] + JOINT_SCALE[lo + 1]) lo++;
    return lo;
  }

  // The X96 scale factor search (ModelSmp's scale_search()'s): at X96's steps (no adjustment), its
  // quantizer's range; a VQ'd subband's vectors one each pair of subsubframes (16 samples), each the
  // two-stage search's at the RMS's scale factor and 2 above, the nearer
  int x96_search(const CoreRec& r, const X96Sub& s, int ch, int b, const S64* v, int n, int p, int* conf) {
    const int ba = s.abits[ch][b], np = (n + 15) / 16;
    S64 lo = 0, hi = 0;
    int vq[2] = {0, 0};
    if (ba >= 2) {
      const int a = ba - 1, how = x96_coding(r, ch, a);
      if (how == 3) {
        hi = (BLOCK_LEVELS[a - 1] - 1) / 2, lo = -hi;
      } else if (how == 2) {
        hi = (1LL << (a - 4)) - 1, lo = -(1LL << (a - 4));
      } else {
        const HuffBook& hb = QB[a - 1][r.x96sel[ch][a - 1]];
        lo = hb.minlev, hi = hb.minlev + hb.nlev - 1;
      }
    } else {
      for (int pp = 0; pp < np && pp < 2; pp++) {
        const S64* x = v + 16 * pp;
        const int m = imin(16, n - 16 * pp);
        S128 bd = -1;
        for (int kk = 0; kk < 2; kk++) {
          const S64 sc = SF6[clip(p + 2 * kk, 0, SF6_N - 1)];
          for (int g = 0; g < 32; g++) cand[g].k = hf_group_dist(g, x, m, sc), cand[g].i = g;
          const int g = select_rank(cand, 32, 0);
          for (int e = 0; e < 32; e++) cand[e].k = hf_dist(g * 32 + e, x, m, sc), cand[e].i = e;
          const int q = g * 32 + select_rank(cand, 32, 0);
          const S128 d = vq_err(x, m, q, sc);
          if (bd < 0 || d < bd) bd = d, vq[pp] = q;
        }
      }
    }
    S128 e1 = -1, e2 = -1;
    int best = -1;
    for (int i = imax(p - 3, 0); i <= imin(p + 5, SF6_N - 1); i++) {
      S128 e = 0;
      if (ba >= 2) {
        e = quant_err(v, n, (S64)LOSSY_QUANT[ba - 1] * SF6[i], lo, hi);
      } else {
        for (int pp = 0; pp < np && pp < 2; pp++) e += vq_err(v + 16 * pp, imin(16, n - 16 * pp), vq[pp], SF6[i]);
      }
      if (best < 0 || e < e1) e2 = e1, e1 = e, best = i;
      else if (e2 < 0 || e < e2) e2 = e;
    }
    if (best < 0 || e2 < 0) return -1;
    const S128 eps = (S128)n << (2 * AQ_F - 6);
    const S128 ratio = (e2 + eps) / (e1 + eps);
    if (ratio < 4) return -1;
    *conf = imin(bitlen128((U128)ratio) - 3, SB_N - 1);
    return best;
  }
  // (as search_on(): kind k's rule, the search's where it is right clearly more often than the
  // better of the others)
  bool x96_rule(int k) const {
    const U32 lead = imax(sfr9[k][0], sfr9[k][1]);
    return sfr9[k][2] > lead + 64 + lead / 8;
  }
  bool x96_search_on(int k) const { return ss9_n < 2048 || x96_rule(k) || !(n_sf9 & 63); }

  // Channel ch's subband b's scale factor v (0-63): against the one the audio makes, where the
  // channel is bound, else the last. By the subband's kind (k: its bits, its VQ, neither) the rules,
  // the one that has been right more often: the RMS rule (as the core's); the kind's own, a decoded
  // WAV's (where the subband has no bits, 2.1 times the peak: the decoder's noise is uniform to half
  // the scale factor; VQ'd, 1.4 times the RMS); the scale factor search (x96_search(): a decoded
  // WAV's too, where the subband has bits or a VQ vector; where it isn't sure, the rule before's)
  S64 x96_scale(const CoreRec& r, const X96Sub& s, int ch, int b, S64 v) {
    const int ba = s.abits[ch][b], k = ba >= 2 ? 0 : ba == 1 ? 1 : 2;
    M_scl_v D;
    tcx X_;
    int p = -1, p0 = -1, p1 = -1, ps = -1, pq = -1, conf = 0, tried = 0, near = SB_N - 1;
    if (bd9[ch]) {
      S64 x[NSMP];
      const int n = x96_values(s, ch, b, x);
      if (n) {
        p0 = p = scale_pred(x, n, SF6, SF6_N, &near);
        if (k == 1) {
          for (int i = 0; i < n; i++) x[i] = x[i] * 7 / 5;
          int nr;
          p1 = scale_pred(x, n, SF6, SF6_N, &nr);
        } else if (k == 2) {
          S64 pk = 0;
          for (int i = 0; i < n; i++) pk = imax(pk, x[i] < 0 ? -x[i] : x[i]);
          p1 = scale_near(pk * 21 / 10, SF6, SF6_N);
        }
        if (p1 >= 0 && sfr9[k][1] + 64 >= sfr9[k][0]) p = p1, near = SB_N - 1;
        if (k < 2 && x96_search_on(k)) {
          S64 y[NSMP];
          x96_values(s, ch, b, y);
          ps = x96_search(r, s, ch, b, y, n, p0, &conf), tried = 1;
        }
        pq = ps >= 0 ? ps : p;
        if (tried && x96_rule(k)) {
          p = pq;
          if (ps >= 0) near = conf;
        }
      }
    }
    const S32 A = hsc[ch][b], P = hps[ch][b];
    D.dp = p >= 0 ? sf_dup(SF6, SF6_N, p) : 0;
    D.kd = ba >= 2 ? 0 : 2;
    D.ab = ba >= 2 ? vvar(ba, 27) : 0;
    D.pm = s.pmode[ch][b];
    D.pk = p >= 0;
    D.pn = near;
    D.lp = A && P ? dvar(A - P) : 9;
    D.hm = hm9[ch];
    D.bd = b;
    this->f_scl.make(D, X_);
    this->f_scl.select(X_);
    if (p >= 0) {
      if (this->f_scl.bit(CN_HIT, ENC && v == p)) v = p;
      else v = checked(p + this->f_scl.codes(X_, ENC ? v - p : 0, (u32)D.kd), 0, 63);
      hm9[ch] = v == p ? 1 : 2;
      n_sfhit9 += v == p;
    } else {
      if (A && this->f_scl.bit(CN_SAME, ENC && v == A - 1)) v = A - 1;
      else v = checked((A ? A - 1 : 0) + this->f_scl.codes(X_, ENC ? v - (A ? A - 1 : 0) : 0, (u32)D.kd), 0, 63);
    }
    psc9[ch][b] = p + 1;
    n_sf9++;
    if (p0 >= 0) {
      sfr9[k][0] += v == p0, sfr9[k][1] += p1 >= 0 && v == p1;
      if (tried) {
        sfr9[k][2] += v == pq;
        ss9_n += ss9_n < 2048;
      }
      if (sfr9[k][0] + sfr9[k][1] + sfr9[k][2] >= 1u << 16) sfr9[k][0] >>= 1, sfr9[k][1] >>= 1, sfr9[k][2] >>= 1;
    }
    return v;
  }

  // sample j of channel ch's subband b (its index v in lo..hi): as ModelSmp's code_q(), at X96's
  // step and scale; then the decoder's sample, for the ADPCM predictions after it
  S64 x96_q(const CoreRec& r, const X96Sub& s, int ch, int b, int j, S64 v, S64 lo, S64 hi) {
    const int a = s.abits[ch][b] - 1, pm = s.pmode[ch][b], bd = bd9[ch];
    const S64 ss = x96_ss(s, ch, b);
    const S32 pred = pm ? adpcm_pred(s.pvq[ch][b], yh9[ch][b]) : 0;
    S64 qp = 0, alt = 0;
    S128 dist = 0;
    if (bd) qp = requant(X9[ch][b][j] - shl64(pred, AQ_F), ss, lo, hi, &dist, &alt);
    M_smp_v D;
    D.ab = a;
    D.hw = x96_coding(r, ch, a);
    D.pk = bd;
    D.nr = bd ? near_bucket(dist, (S128)ss << AQ_F, 17) : 0;
    D.nl = bd ? near_bucket(dist, ((S128)SF6[imax(esmax9[ch] - 1, 0)] << 38), 28) : 0;
    D.ql = bd ? imin(bit_len((U64)(qp < 0 ? -qp : qp)), 21) : 0;
    D.hm = imin(__builtin_popcount(miss9[ch][b]), 3);
    D.pm = pm;
    D.bd = b;
    D.m1 = q1_9[ch][b] ? imin(q1_9[ch][b] - 1, 21) : 22;
    D.m2 = q2_9[ch][b] ? imin(q2_9[ch][b] - 1, 21) : 22;
    D.sq = bd ? svar(qp) : 0;
    D.s1 = qs9[ch][b];
    int how;
    v = code_index(D, v, bd, qp, alt, lo, hi, &how);
    miss9[ch][b] = (U8)(miss9[ch][b] << 1 | (bd && how != 0));
    n_smp9++;
    n_hit9 += bd && how == 0;
    q2_9[ch][b] = q1_9[ch][b];
    q1_9[ch][b] = 1 + bit_len((U64)(v < 0 ? -v : v));
    qs9[ch][b] = (S8)svar(v);
    push9(ch, b, clip23((S64)dequantize((S32)v, ss) + pred));
    return v;
  }
  INLINE void push9(int ch, int b, S32 y) {
    S32* h = yh9[ch][b];
    h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = y;
  }

  // the subframe done: the decoder's samples of its subbands without indexes (a VQ'd one's vectors,
  // none of one without bits: its noise isn't the model's), the predictions added; the audio's last
  // samples; its values as the last ones
  void x96_subframe_end(const CoreRec& r, int sf) {
    const X96Sub& s = r.xs[sf];
    for (int ch = 0; ch < r.npchs; ch++)
      for (int b = 0; b < NB; b++) {
        const int in = b >= r.x96start && b < r.x96nsubs[ch], ba = in ? s.abits[ch][b] : 0;
        if (!in) {  // (FFmpeg's: an inactive subband's history cleared)
          yh9[ch][b][0] = yh9[ch][b][1] = yh9[ch][b][2] = yh9[ch][b][3] = 0;
          hpm[ch][b] = hab[ch][b] = hsc[ch][b] = hps[ch][b] = hen[ch][b] = 0;
        } else {
          if (ba < 2) {
            for (int j = 0; j < nsmp9; j++) {
              S64 y = 0;
              if (ba == 1) y = clip23((HF_VQ[s.vq[ch][b][j >> 4]][j & 15] * (S64)SF6[s.scale[ch][b] & 63] + 8) >> 4);
              if (s.pmode[ch][b]) y = clip23(y + adpcm_pred(s.pvq[ch][b], yh9[ch][b]));
              push9(ch, b, (S32)y);
            }
            q1_9[ch][b] = q2_9[ch][b] = 0, qs9[ch][b] = 0;
          }
          hpm[ch][b] = s.pmode[ch][b] + 1;
          if (s.pmode[ch][b]) hpv[ch][b] = s.pvq[ch][b] + 1;
          hab[ch][b] = ba + 1;
          hsc[ch][b] = s.scale[ch][b] + 1;
          hps[ch][b] = psc9[ch][b];
          if (ba == 1) hvq[ch][b] = s.vq[ch][b][0] + 1;
          hen[ch][b] = bd9[ch] ? eab9[ch][b] : 0;
        }
        const int src = r.x96joinx[ch] - 1;
        if (src >= 0 && b >= r.x96nsubs[ch] && b < r.x96nsubs[src]) hjs[ch][b] = s.jscale[ch][b] + 129;
        psc9[ch][b] = 0;
        if (bd9[ch] && in)
          for (int k = 0; k < 4; k++) xh9[ch][b][k] = X9[ch][b][nsmp9 - 1 - k];
      }
  }
};
