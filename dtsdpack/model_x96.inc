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
  int fb9;                      // its first block in the frame
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
  S32 jem[NCH][NB];  // and the joint scale factor's running mean (in 256ths; each new one 1/8 of it)
  // A subband without bits is the decoder's noise: FFmpeg's, its generator's draws times the scale
  // factor (FFmpeg's rand_x96(): a linear congruential generator from 1 at the decoder's start, each
  // subframe's subbands without bits drawing their samples in turn, where the scale factor is above
  // 1), so where that is the decoder, the scale factor is the entry whose draws lie nearest the
  // audio's values (`96-24.dts`'s 99.7% of those). The generator's state xor 1 (its seed at 0), and
  // within the subframe at the next subband; how often the rule before was right, and the noise's
  // (where they differ), the noise's taken where it has been right more often, by a margin
  U32 lcg_, lcg_run;
  U32 lcg_ab;  // (and as the ABITS guess it: x96_ab_search())
  U32 lcg_r[2];
  static S32 lcg_next(U32& st) {
    st = 1103515245u * st + 12345u;
    return (S32)(st & 0x7fffffff) - 0x40000000;
  }
  U64 n_smp9, n_hit9, n_sf9, n_sfhit9;  // (-s: X96's samples and scale factors, those the audio predicted)
  // how often x96_scale()'s rules were the stream's, by the subband's kind (halved as they grow);
  // the search's tries (to 2048)
  U32 sfr9[3][3], ss9_n;
  // how often the ABITS' first guess was right, and the bit allocation search's (halved as they
  // grow); the search's tries (to 2048); the ABITS (-s: and those the search said, those right)
  U32 ab9r[2], ab9_n;
  U64 n_ab9, n_abs9, n_abhit9;

  // the step size times scale factor of channel ch's subband b (its ABITS 2 up: the quantizer's
  // ABITS + 1; no adjustment)
  static INLINE S64 x96_ss(const X96Sub& s, int ch, int b) {
    return (S64)LOSSY_QUANT[s.abits[ch][b] - 1] * SF6[s.scale[ch][b] & 63];
  }

  // the subframe sf starts (X96's pass): each bound channel's audio from the frame's analysis, less
  // the core's decoded samples below band 32; their LPC, autocorrelation, energy and how it goes
  void x96_subframe(const CoreRec& r, int sf) {
    const CoreSub& cs = r.sub[sf];
    lcg_run = lcg_ab = lcg_;
    fb9 = r.nblk;
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
  // The core's values in channel ch's subband b over the subframe, where X96 codes it too and the
  // core has little of the decode's (jx_low(): a 96/24 stream's top subbands, where its own
  // decimation to 48 kHz left the core little and X96's step is many times the core's, 35-85 in
  // `96-24.dts`'s subband 31), before the core's ABITS and scale factors say X96's lattice: the
  // decode less X96's values, on the lattice of its last ABITS there (its 3 levels where it was a
  // vector) at its last scale factor or within JC_W9 of it, the coarsest that leaves each value within
  // an eighth of its step (into y, 2^-AQ_F); false where none does
  static constexpr int JC_W9 = 3;
  bool jx_core_values(const CoreRec& r, int ch, int b, S64* y) {
    const int n = B::nsmp, i9 = hsc[ch][b] - 1;
    if (hab[ch][b] < 2 || i9 < 0) return false;
    const int a9 = hab[ch][b] - 1 >= 2 ? hab[ch][b] - 1 : 2;  // (as coded: the quantizer's ABITS + 1)
    const S64* w = B::X[ch][b];
    S64 lo, hi, best = -1, yy[NSMP];
    x96_range(r, ch, a9 - 1, lo, hi);
    for (int i = imax(0, i9 - JC_W9); i <= imin(SF6_N - 1, i9 + JC_W9); i++) {
      const S64 s9 = (S64)LOSSY_QUANT[a9 - 1] * SF6[i], u = shl64(dequantize(1, s9), AQ_F);
      if (s9 <= best || u <= 0) continue;
      bool ok = true;
      for (int j = 0; j < n && ok; j++) {
        S128 dd;
        S64 o;
        yy[j] = w[j] - shl64(dequantize((S32)requant(w[j], s9, lo, hi, &dd, &o), s9), AQ_F);
        ok = 8 * (yy[j] < 0 ? -yy[j] : yy[j]) <= u;
      }
      if (ok) best = s9, memcpy(y, yy, sizeof yy);
    }
    return best > 0;
  }
  // Of the K ADPCM predictors nearest by their distances d (the first of equals the lower), in turn,
  // from a heap of all 4096 (the one that fits is mostly near the start), the first that fits(); -1
  // none
  template <class F>
  int pv_first_fit(const S64* d, int K, F fits) {
    int* const top = B::pvtop;
    auto less = [&](int p, int q) { return d[p] < d[q] || (d[p] == d[q] && p < q); };
    auto sift = [&](int i, int m) {
      const int t = top[i];
      for (;;) {
        const int l = 2 * i + 1;
        if (l >= m) break;
        const int c = l + 1 < m && less(top[l + 1], top[l]) ? l + 1 : l;
        if (!less(top[c], t)) break;
        top[i] = top[c], i = c;
      }
      top[i] = t;
    };
    for (int q = 0; q < 4096; q++) top[q] = q;
    for (int i = 4096 / 2 - 1; i >= 0; i--) sift(i, 4096);
    for (int k = 0, hn = 4096; k < K; k++) {
      const int qk = top[0];
      top[0] = top[--hn];
      sift(0, hn);
      if (fits(qk)) return qk;
    }
    return -1;
  }
  // The first ADPCM predictor of the K nearest by d (the core's values' own LPC's distances) whose
  // prediction, the decoder's from its last 4 samples and the values y (each to its nearest unit),
  // leaves every value exactly on a lattice of the core's last ABITS there, at a scale factor within
  // JC_WC of its last (the decoder's dequantized indexes: an encoder's ADPCM had them so); -1 none
  static constexpr int JC_WC = 12;
  int jx_core_fit(const CoreRec& r, int ch, int b, const S64* y, const S64* d, int K) {
    const int n = B::nsmp, a = B::hist.abits[ch][b] - 1, si = B::hist.scale[ch][b] - 1;
    if (a < 1 || si < 0) return -1;
    S64 lo, hi, ss[2 * JC_WC + 1];
    S32 unit[2 * JC_WC + 1], yi[NSMP];
    int nl = 0;
    quant_range(r, ch, a, lo, hi);
    for (int i = imax(0, si - JC_WC); i <= imin(sf_entries(r.shuff[ch]) - 1, si + JC_WC); i++) {
      const S64 st = step_for(r, ch, a, i);
      if (st > 0) ss[nl] = st, unit[nl++] = dequantize(1, st);
    }
    if (!nl) return -1;
    const bool tri = lo == -1 && hi == 1;  // (3 levels: every value not 0 is the unit, one lattice's)
    for (int j = 0; j < n; j++) yi[j] = (S32)((y[j] + ((S64)1 << (AQ_F - 1))) >> AQ_F);
    return pv_first_fit(d, K, [&](int qk) {
      S32 h[4];
      memcpy(h, B::yh[ch][b], sizeof h);
      U32 alive = (1u << nl) - 1;  // (the lattices it may be on)
      for (int j = 0; j < n && alive; j++) {
        const S32 x = yi[j] - adpcm_pred(qk, h);
        h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = yi[j];
        if (!x) continue;
        if (tri) {
          const S32 ax = x < 0 ? -x : x;
          U32 m = 0;
          for (int t = 0; t < nl; t++) m |= (U32)(unit[t] == ax) << t;
          alive &= m;
          continue;
        }
        for (int t = 0; t < nl; t++) {
          S128 dd;
          S64 o;
          if (alive >> t & 1 && dequantize((S32)requant(shl64(x, AQ_F), ss[t], lo, hi, &dd, &o), ss[t]) != x)
            alive &= ~(1u << t);
        }
      }
      return alive != 0;
    });
  }
  // Every vector X96's decoder makes of the code book's entries' first 16 values at each scale
  // factor from 1 to 62 (FFmpeg's (e sc + 8) >> 4), in a hash table of their values (chained, made
  // the first time it is needed), so that a vector's makers are found at once
  struct VqMade {
    static constexpr int N = 62 * 1024, NB = 1 << 17;
    U32 hh[N];        // (each's hash)
    S32 head[NB], next[N];
    static U32 hash(const S32* z) {
      U64 h = 1469598103934665603ull;
      for (int j = 0; j < 16; j++) h = (h ^ (U32)z[j]) * 1099511628211ull;
      return (U32)(h ^ h >> 32);
    }
    static void make(int m, S32* z) {  // (made m: scale factor 1 + m / 1024, entry m % 1024)
      const S64 sc = SF6[1 + m / 1024];
      const S8* q = HF_VQ[m % 1024];
      for (int j = 0; j < 16; j++) z[j] = (S32)((q[j] * sc + 8) >> 4);
    }
    VqMade() {
      for (int b = 0; b < NB; b++) head[b] = -1;
      for (int m = N - 1; m >= 0; m--) {  // (each chain in order)
        S32 z[16];
        make(m, z);
        hh[m] = hash(z);
        next[m] = head[hh[m] & (NB - 1)], head[hh[m] & (NB - 1)] = m;
      }
    }
    // whether the vector z (16 values) is one of an entry's at a scale factor in i0..i1
    bool made(const S32* z, int i0, int i1) const {
      const U32 h = hash(z);
      for (int m = head[h & (NB - 1)]; m >= 0; m = next[m]) {
        const int i = 1 + m / 1024;
        if (hh[m] != h || i < i0 || i > i1) continue;
        S32 y[16];
        make(m, y);
        if (!memcmp(y, z, sizeof y)) return true;
      }
      return false;
    }
  };
  // X96's ADPCM predictor in channel ch's subband b, a decoded WAV's: the first of the K nearest by
  // d (its LPC's) whose prediction, the decoder's from the decode's own last 4 values (its history,
  // yh9, isn't the decoder's after a subband of FFmpeg's noise), leaves every one of the decode's
  // values (to the nearest unit) exactly a vector of the code book's at a scale factor within XP_W of
  // its last there (VqMade's), or on the lattice of its last ABITS, where a quantizer's, at those
  // scale factors; -1 none
  static constexpr int XP_W = 6;
  int x96_pvq_fit(const CoreRec& r, int ch, int b, const S64* d, int K) {
    static const VqMade vm;
    const int n = nsmp9, i9 = hsc[ch][b] - 1, a9 = hab[ch][b] - 1;
    if (i9 < 0 || n != 16) return -1;
    const S64 half = (S64)1 << (AQ_F - 1);
    S32 yi[NSMP], h0[4];
    for (int j = 0; j < n; j++) yi[j] = (S32)((X9[ch][b][j] + half) >> AQ_F);
    for (int k = 0; k < 4; k++) h0[k] = (S32)((xh9[ch][b][k] + half) >> AQ_F);
    int ni = 0, si[2 * XP_W + 1];
    for (int i = imax(1, i9 - XP_W); i <= imin(SF6_N - 2, i9 + XP_W); i++) si[ni++] = i;
    S64 lo = 0, hi = 0, ss[2 * XP_W + 1];
    int nl = 0;
    if (a9 >= 2) {
      x96_range(r, ch, a9 - 1, lo, hi);
      for (int t = 0; t < ni; t++) ss[nl++] = (S64)LOSSY_QUANT[a9 - 1] * SF6[si[t]];
    }
    return pv_first_fit(d, K, [&](int qk) {
      S32 x[NSMP], h[4] = {h0[0], h0[1], h0[2], h0[3]};
      for (int j = 0; j < n; j++) {
        x[j] = yi[j] - adpcm_pred(qk, h);
        h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = yi[j];
      }
      if (ni && vm.made(x, si[0], si[ni - 1])) return true;
      for (int t = 0; t < nl; t++) {
        int j = 0;
        for (S128 dd; j < n; j++) {
          S64 o;
          if (dequantize((S32)requant(shl64(x[j], AQ_F), ss[t], lo, hi, &dd, &o), ss[t]) != x[j]) break;
        }
        if (j == n) return true;
      }
      return false;
    });
  }
  // Whether the core's values of channel ch's subband b, the decode less X96's (jx_core_values()),
  // lie exactly on a lattice of the core's last ABITS there, at a scale factor within JC_WC of its
  // last, as they are (a subband that doesn't predict: 1); -1 where they aren't known
  int jx_core_plain(const CoreRec& r, int ch, int b) {
    S64 y[NSMP];
    if (!jx_core_values(r, ch, b, y)) return -1;
    const int n = B::nsmp, a = B::hist.abits[ch][b] - 1, si = B::hist.scale[ch][b] - 1;
    if (a < 1 || si < 0) return -1;
    S64 lo, hi;
    quant_range(r, ch, a, lo, hi);
    S32 yi[NSMP];
    bool any = false;
    for (int j = 0; j < n; j++) yi[j] = (S32)((y[j] + ((S64)1 << (AQ_F - 1))) >> AQ_F), any |= yi[j] != 0;
    if (!any) return 1;
    for (int i = imax(0, si - JC_WC); i <= imin(sf_entries(r.shuff[ch]) - 1, si + JC_WC); i++)
      if (jx_on_lattice(yi, n, step_for(r, ch, a, i), lo, hi)) return 1;
    return 0;
  }
  // whether the values x (n) are each exactly an index's at step and scale st (in lo..hi)
  static bool jx_on_lattice(const S32* x, int n, S64 st, S64 lo, S64 hi) {
    if (st <= 0) return false;
    for (int j = 0; j < n; j++) {
      S128 dd;
      S64 o;
      if (dequantize((S32)requant(shl64(x[j], AQ_F), st, lo, hi, &dd, &o), st) != x[j]) return false;
    }
    return true;
  }
  // The core's scale factor of channel ch's subband b (its ABITS and predictor coded) where the
  // core's values are known (jx_core_values()): the first whose lattice they lie on exactly, less
  // the decoder's prediction; -1 none (or where they are all predicted, saying no scale factor)
  int jx_core_scale(const CoreRec& r, const CoreSub& s, int ch, int b) {
    S64 y[NSMP];
    if (!jx_core_values(r, ch, b, y)) return -1;
    const int n = B::nsmp, a = s.abits[ch][b], nt = sf_entries(r.shuff[ch]);
    S64 lo, hi;
    quant_range(r, ch, a, lo, hi);
    S32 x[NSMP], h[4];
    memcpy(h, B::yh[ch][b], sizeof h);
    bool any = false;
    for (int j = 0; j < n; j++) {
      const S32 yi = (S32)((y[j] + ((S64)1 << (AQ_F - 1))) >> AQ_F);
      x[j] = yi - (s.pmode[ch][b] ? adpcm_pred(s.pvq[ch][b], h) : 0);
      h[3] = h[2], h[2] = h[1], h[1] = h[0], h[0] = yi;
      any |= x[j] != 0;
    }
    if (!any) return -1;
    for (int i = 0; i < nt; i++)
      if (jx_on_lattice(x, n, step_for(r, ch, a, i), lo, hi)) return i;
    return -1;
  }
  // Whether X96's decode of channel ch's subband b is, as it is, a vector of the code book's at a
  // scale factor within XP_W of its last there (VqMade's) or on the lattice of its last ABITS at
  // one (a subband that doesn't predict: 1); -1 where its last isn't known
  int x96_plain(const CoreRec& r, int ch, int b) {
    static const VqMade vm;
    const int n = nsmp9, i9 = hsc[ch][b] - 1, a9 = hab[ch][b] - 1;
    if (i9 < 0 || n != 16 || a9 < 0) return -1;
    S32 yi[NSMP];
    bool any = false;
    for (int j = 0; j < n; j++) yi[j] = (S32)((X9[ch][b][j] + ((S64)1 << (AQ_F - 1))) >> AQ_F), any |= yi[j] != 0;
    if (!any) return 1;
    const int i0 = imax(1, i9 - XP_W), i1 = imin(SF6_N - 2, i9 + XP_W);
    if (vm.made(yi, i0, i1)) return 1;
    if (a9 >= 2) {
      S64 lo, hi;
      x96_range(r, ch, a9 - 1, lo, hi);
      for (int i = i0; i <= i1; i++)
        if (jx_on_lattice(yi, n, (S64)LOSSY_QUANT[a9 - 1] * SF6[i], lo, hi)) return 1;
    }
    return 0;
  }
  // Channel ch's subband b's ADPCM predictor v where jx_core_values() said the core's values y: by
  // their own LPC, the first of jx_core_fit()'s 1024 first (code_pvq2())
  S64 code_pvq_core(const CoreRec& r, int ch, int b, const S64* y, S64 v) {
    const int n = B::nsmp;
    S64 x[4 + NSMP], al[4];
    int g, g4;
    for (int k = 0; k < 4; k++) x[3 - k] = shl64(B::yh[ch][b][k], AQ_F);
    for (int j = 0; j < n; j++) x[4 + j] = y[j];
    lpc4(x, 4 + n, al, &g, &g4);
    S64* const d = B::pvqd;
    pvq_dists(al, d);
    return B::code_pvq2(al, d, jx_core_fit(r, ch, b, y, d, 1024), (int)v, ch, b, g, B::lrank[ch][b][VK_PVQ],
                        B::lrank[ch][b][VK_PVE]);
  }

  // how the encoder would gain by predicting channel ch's subband b (as ModelSmp's pmode_gain())
  int x96_pmode_gain(int ch, int b) {
    const int k = lpg4_9[ch][b];
    if (k < 2 || k > 9) return k < 2 ? 0 : 9;
    S64* const d = this->pvqd;
    pvq_dists(lpc9[ch][b], d);
    const int bq = this->pvq_first(lpc9[ch][b], d);
    return clip(pvq_gain4(bq, xh9[ch][b], X9[ch][b], nsmp9) - 2, 1, 8);
  }

  // An element of X96's (fid: the core's names) of channel ch's subband b (j: the sample, or the VQ
  // vector's pair of subsubframes), its value v in lo..hi
  S64 x96_code(const CoreRec& r, int sf, int fid, int ch, int b, int j, S64 v, S64 lo, S64 hi) {
    const X96Sub& s = r.xs[sf];
    const int bd = bd9[ch], b0 = r.x96start;
    switch (fid) {
      case F_PMODE:  // (a decoded WAV's where the decode is, as it is, a vector or on a lattice X96's last ABITS
                     // there makes, x96_plain(), as the core's that fit one: `96-24.dts`'s core 0.2% smaller)
        if (bd && B::ss_rule() && hab[ch][b] >= 2 && x96_plain(r, ch, b) > 0)
          return side_code(SF_PMSR, ch, b, v, 0, 1, 1, hpm[ch][b], b > b0 ? s.pmode[ch][b - 1] + 1 : 0, 0,
                           imin(lpg4_9[ch][b], 27), x96_pmode_gain(ch, b));
        return side_code(SF_PMODE, ch, b, v, 0, 1, 1, hpm[ch][b], b > b0 ? s.pmode[ch][b - 1] + 1 : 0, bd ? ac9[ch][b] : 8,
                         bd ? imin(lpg4_9[ch][b], 27) : 0, bd ? x96_pmode_gain(ch, b) : 9);
      case F_PVQ:
        if (bd) {
          S64* const d = this->pvqd;
          pvq_dists(lpc9[ch][b], d);
          // (a decoded WAV's: the first by its LPC that fits, x96_pvq_fit(), first: `96-24.dts`'s X96
          // predictors 1078 bytes to 483, its core 1.6% smaller)
          const int fit = B::ss_rule() ? x96_pvq_fit(r, ch, b, d, 1024) : -1;
          return this->code_pvq2(lpc9[ch][b], d, fit, (int)v, ch, b, lpg9[ch][b], lrank9[ch][b][VK_PVQ],
                                 lrank9[ch][b][VK_PVE]);
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
        // (the bit allocation search's, x96_ab_search(), where it has been right more often than the
        // first guess without it, by a margin: as sure; `96-24.dts`'s X96 ABITS 1388 bytes to 62, its
        // core 3.2% smaller)
        const S32 og = sv ? sv : hab[ch][b];
        const bool srch = bd && x96_ab_on();
        const int G = srch ? x96_ab_search(r, s, ch, b, (int)hi) : -1;
        S32 Sn = sv;
        int sc = 0;
        if (G >= 0 && x96_ab_rule()) Sn = G + 1, sc = 13 + (G + 1 == og), n_abs9++;
        v = abits_code(ch, b, v, lo, hi, hab[ch][b], b > b0 ? s.abits[ch][b - 1] + 1 : 0, e, de, ls, s.pmode[ch][b], lbd, nb,
                       mx, Sn, near_e ? 14 : nn, sc, 4, 0);
        v = checked(v, lo, hi);
        if (sn) *sn = (U8)(v + 1);
        if (srch) {
          const int ro = og && v == og - 1, rs = G >= 0 ? v == G : ro;
          ab9r[0] += ro, ab9r[1] += rs;
          if (ab9r[0] + ab9r[1] >= 1u << 16) ab9r[0] >>= 1, ab9r[1] >>= 1;
          ab9_n += ab9_n < 2048;
        }
        n_ab9++;
        n_abhit9 += sc && v == G;
        if (bd && !v) {  // (the generator's draws, where the values aren't all within half a unit of 0)
          bool z = true;
          for (int j = 0; j < nsmp9 && z; j++) z = B::jx_near(X9[ch][b][j]);
          if (!z) {
            U32 st = lcg_ab ^ 1;
            for (int j = 0; j < nsmp9; j++) lcg_next(st);
            lcg_ab = st ^ 1;
          }
        }
        return v;
      }
      case F_SCALES:
        return x96_scale(r, s, ch, b, v);
      case F_JOIN_SHUFF:
        return side_code(SF_JSHUFF, ch, 0, v, lo, hi, 0, 0, 0, 8, 0, 9);
      case F_JOIN_SCALES: {  // (against the scale the audio's subband is of its source's, where the source has a level)
        const int src = r.x96joinx[ch] - 1;
        int p = -1, conf = 8, pr = -1;
        if (bd && src >= 0 && bd9[src]) p = x96_joint(r, ch, src, b, &conf, &pr);
        S32 A = p + 65, P = hjs[ch][b];  // (coded 128 up: index p is p - 64)
        if (p < 0) {  // (else, the source too quiet to say, 13/16 of the subband's running mean and 3/16
                      // of the subband below's (0 where none): their noise's ratio drifts, `96-24.dts`'s
                      // core 0.9% smaller than by a quarter of the last one and the one below; and where
                      // the two have any power, 4/5 of that and 1/5 of their powers' ratio's, 0.5%)
          const int u = 13 * jem[ch][b] + 768 * (b > r.x96nsubs[ch] ? s.jscale[ch][b - 1] : 0);
          A = 129 + (pr < 0 ? (u + 2048) >> 12 : (13 * u + 13107 * (pr - 64) + 32768) >> 16), P = 0;
        }
        return side_code(SF_JSCALE, ch, b, v + 128, lo + 128, hi + 128, 0, A, P, conf, 0, 9) - 128;
      }
      case F_HFVQ:  // (a pair of subsubframes' vector: its 16 samples, at the subband's scale factor)
        if (bd && 16 * j < nsmp9) {
          const int si = s.scale[ch][b] & 63, n = imin(16, nsmp9 - 16 * j);
          const S64 sc = SF6[si];
          // (a predicting subband's values less the open loop's prediction, as the scale factor
          // search's: the decoder adds it to the vector's; `96-24.dts`'s groups 98.2% the first from
          // 95.0%, its core 1.4% smaller)
          S64 xp[NSMP];
          if (s.pmode[ch][b]) x96_values(s, ch, b, xp);
          const S64* x = s.pmode[ch][b] ? &xp[16 * j] : &X9[ch][b][16 * j];
          // (the groups by their nearest entries where the nearest of all lies within 1/64 of the
          // values' energy, as the core's: a decoded WAV's values are the vector's, `96-24.dts`'s
          // groups 95.0% the first from 86.4%, its core 1.4% smaller; else by their centroids)
          S128 md[32], en = 0;
          const S64* dot;
          const double* dd;
          const bool small = hf_dots(x, n, &dot, &dd);
          for (int jj = 0; jj < n; jj++) en += (S128)x[jj] * 16 * ((S128)x[jj] * 16);
          hf_group_mins(dot, small, n, sc, en, md);
          S128 near = en + 1;
          for (int g = 0; g < 32; g++) near = md[g] < near ? md[g] : near;
          const bool mn = near * 64 < en;
          for (int g = 0; g < 32; g++) cand[g].k = mn ? md[g] : hf_group_dist(g, x, n, sc), cand[g].i = g;
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
  // where the source's mean square is 2 or more (in the decoder's units); *conf its log2 less 1 (to
  // 7): how sure. Where it is 1 to 2 (or the projection isn't positive), the one nearest the root of
  // their powers' ratio (by their logs: 3.2 bits a scale there from 4.9; conf 0), and where less,
  // -1 (*pr that one, where either has power: else -1)
  int x96_joint(const CoreRec& r, int ch, int src, int b, int* conf, int* pr) {
    const int fb = r.nblk;
    S128 es = 0, ex = 0, ec = 0;
    for (int j = 0; j < nsmp9; j++) {
      const S64 xs = xa[src][b][fb + j], xc = xa[ch][b][fb + j];
      es += (S128)xs * xs;
      ex += (S128)xc * xs;
      ec += (S128)xc * xc;
    }
    const int lev = bitlen128((U128)(es / nsmp9)) - 1 - 2 * AQ_F;
    if (es > 0 && ec > 0) {
      const S128 q = shl128(ec, 34) / es;  // (Q34, as the entries' squares)
      int lo = 0, hi = 128;
      while (lo < hi) {
        const int m = (lo + hi + 1) >> 1;
        if ((S128)JOINT_SCALE[m] * JOINT_SCALE[m] <= q) lo = m;
        else hi = m - 1;
      }
      if (lo < 128 && q >= (S128)JOINT_SCALE[lo] * JOINT_SCALE[lo + 1]) lo++;
      *pr = lo;
    }
    if (lev < 0) return -1;
    if (lev < 1 || ex <= 0) return *pr >= 0 ? (*conf = 0, *pr) : -1;
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

  // X96's quantizer range of ABITS a (its bit allocation less 1) in channel ch
  void x96_range(const CoreRec& r, int ch, int a, S64& lo, S64& hi) const {
    const int how = x96_coding(r, ch, a);
    if (how == 3) {
      hi = (BLOCK_LEVELS[a - 1] - 1) / 2, lo = -hi;
    } else if (how == 2) {
      hi = (1LL << (a - 4)) - 1, lo = -(1LL << (a - 4));
    } else {
      const HuffBook& hb = QB[a - 1][r.x96sel[ch][a - 1]];
      lo = hb.minlev, hi = hb.minlev + hb.nlev - 1;
    }
  }
  // X96's bit allocation search: the ABITS (as coded: 0 none, 1 VQ, 2 up a quantizer's + 1) whose
  // decode channel ch's subband b's values are, a decoded WAV's: FFmpeg's noise where
  // x96_noise_scale() is sure of it, from the generator's state as the ABITS before say (lcg_ab:
  // their subbands without bits drawing where their values aren't all within half a unit of 0; the
  // state's 99.9% of `96-24.dts`'s); else the coarsest lattice of a quantizer (to vhi) at a scale
  // factor p - 3 to p + 5 (the RMS rule's) they all lie on within half a unit, an index of it not 0
  // and its step 1.5 units or more (a finer one fits nearly anything: 763 of `96-24.dts`'s 113529
  // wrong, 5 with it); else VQ, where each 16 of them are the shape entry's at a scale factor within
  // half a unit; -1 where none is, or they're all within half a unit of 0 (`96-24.dts`'s: of its
  // 7694 ABITS that differ from the last ones, 94% right and 2 wrong)
  int x96_ab_search(const CoreRec& r, const X96Sub& s, int ch, int b, int vhi) {
    S64 x[NSMP];
    const int n = x96_values(s, ch, b, x);
    const S128 half2 = (S128)1 << (2 * AQ_F - 2);
    bool zero = true;
    for (int j = 0; j < n && zero; j++) zero = B::jx_near(x[j]);
    if (zero) return -1;
    if (r.x96shuff[ch] != 6 && !s.pmode[ch][b]) {
      int sure = 0;
      x96_noise_scale(lcg_ab, ch, b, &sure);
      if (sure) return 0;
    }
    int near;
    const int p = scale_pred(x, n, SF6, SF6_N, &near);
    int best = -1;
    S64 bss = 0;
    for (int a = 1; a + 1 <= vhi && a <= 26; a++) {
      S64 lo, hi;
      x96_range(r, ch, a, lo, hi);
      for (int i = imax(p - 3, 0); i <= imin(p + 5, SF6_N - 1); i++) {
        const S64 ss = (S64)LOSSY_QUANT[a] * SF6[i];
        if (ss <= bss || ss < (S64)3 << 21) continue;
        QErr q;
        q.set(ss, lo, hi);
        int nz = 0, j = 0;
        for (; j < n; j++)
          if (q.err(x[j], &nz) >= half2) break;
        if (j == n && nz) bss = ss, best = a + 1;
      }
    }
    if (best >= 0) return best;
    bool vq = vhi >= 1;
    for (int pp = 0; pp < (n + 15) / 16 && vq; pp++) {
      const S64* xv = x + 16 * pp;
      const int m = imin(16, n - 16 * pp), qs = hf_shape(xv, m);
      bool ok = false;
      for (int i = 0; i < SF6_N && !ok; i++) {
        ok = true;
        for (int j = 0; j < m && ok; j++) {
          const S64 e = xv[j] - shl64(clip23((HF_VQ[qs][j] * (S64)SF6[i] + 8) >> 4), AQ_F);
          ok = (S128)e * e < half2;
        }
      }
      vq = ok;
    }
    return vq ? 1 : -1;
  }
  // (as ModelSmp's ab_rule(): the search's rule, where it has been right more often than the first
  // guess without it, by a margin (it is rarely wrong); tried in its first 2048 values, while its rule
  // is taken, and in 1 of 64)
  bool x96_ab_rule() const { return ab9r[1] > ab9r[0] + 16; }
  bool x96_ab_on() const { return ab9_n < 2048 || x96_ab_rule() || !(n_ab9 & 63); }

  // The X96 scale factor search (ModelSmp's scale_search()'s): at X96's steps (no adjustment), its
  // quantizer's range; a VQ'd subband's vectors one each pair of subsubframes (16 samples), each the
  // nearest of the two-stage search's at the RMS's scale factor and 2 above and the entry most nearly
  // a multiple of the values at its best scale factor (as the core's), its scale factors to 12 above
  // the RMS's, and where it isn't sure the nearest still (*conf 0: the RMS's is rarely a decoded VQ
  // subband's; `96-24.dts`'s core 3.3% smaller)
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
        const int qs = hf_shape(x, m);
        for (int i = imax(p - 3, 0); i <= imin(p + 12, SF6_N - 1); i++) {
          const S128 d = vq_err(x, m, qs, SF6[i]);
          if (d < bd) bd = d, vq[pp] = qs;
        }
      }
    }
    S128 e1 = -1, e2 = -1;
    int best = -1;
    for (int i = imax(p - 3, 0); i <= imin(p + (ba >= 2 ? 5 : 12), SF6_N - 1); i++) {
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
    if (ratio < 4) return ba < 2 ? (*conf = 0, best) : -1;
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

  // The entry whose noise, from the generator's state st (xor 1), lies nearest channel ch's subband
  // b's values in the subframe (*sure: where its distance is under a unit a value and a quarter of
  // the next's); of entries the same (1 and 2, 3 and 4), the first, as the encoders take them
  // (`96-24.dts`'s X96 scale factors 99.4% the audio's from 94.5%, its core 1.5% smaller)
  int x96_noise_scale(U32 st, int ch, int b, int* sure) const {
    S32 rv[NSMP];
    st ^= 1;
    for (int j = 0; j < nsmp9; j++) rv[j] = lcg_next(st);
    int bi = -1;
    S128 be = 0, e2 = -1;
    for (int i = 0; i < SF6_N; i++) {
      if (i > 0 && SF6[i] == SF6[i - 1]) continue;
      S128 e = 0;
      for (int j = 0; j < nsmp9; j++) {
        const S64 y = SF6[i] > 1 ? ((S64)rv[j] * (S32)SF6[i] + ((S64)1 << 30)) >> 31 : 0;  // (FFmpeg's mul31())
        const S64 d = X9[ch][b][j] - shl64(y, AQ_F);
        e += (S128)d * d;
      }
      if (bi < 0 || e < be) e2 = bi < 0 ? -1 : be, be = e, bi = i;
      else if (e2 < 0 || e < e2) e2 = e;
    }
    *sure = be < (S128)nsmp9 << (2 * AQ_F) && (e2 < 0 || 4 * be < e2);
    return bi;
  }
  // Channel ch's subband b's scale factor v (0-63): against the one the audio makes, where the
  // channel is bound, else the last. By the subband's kind (k: its bits, its VQ, neither) the rules,
  // the one that has been right more often: the RMS rule (as the core's); the kind's own, a decoded
  // WAV's (where the subband has no bits, 2.1 times the peak: the decoder's noise is uniform to half
  // the scale factor; VQ'd, 1.4 times the RMS); the scale factor search (x96_search(): a decoded
  // WAV's too, where the subband has bits or a VQ vector; where it isn't sure, the rule before's);
  // and where the subband has no bits, FFmpeg's noise (x96_noise_scale(), where it is sure), where
  // that has been right more often than the rest, by a margin (`96-24.dts`'s core 3.7% smaller).
  // Then the noise's draws, where the subband has no bits and the scale factor is above 1
  S64 x96_scale(const CoreRec& r, const X96Sub& s, int ch, int b, S64 v) {
    v = x96_scale0(r, s, ch, b, v);
    if (s.abits[ch][b] == 0 && (r.x96shuff[ch] == 6 ? SF7[v & 127] : SF6[v & 63]) > 1) {  // (the noise's draws)
      U32 st = lcg_run ^ 1;
      for (int j = 0; j < nsmp9; j++) lcg_next(st);
      lcg_run = st ^ 1;
    }
    return v;
  }
  S64 x96_scale0(const CoreRec& r, const X96Sub& s, int ch, int b, S64 v) {
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
    int pl = -1, sure = 0;
    if (bd9[ch] && k == 2 && r.x96shuff[ch] != 6) {
      pl = x96_noise_scale(lcg_run, ch, b, &sure);
      if (!sure) pl = -1;
    }
    const int pr = p;
    if (pl >= 0 && B::jx_more(lcg_r, 4, 1)) p = pl, near = SB_N - 1;
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
    D.cc = chan_class(ch);
    D.nb = 9;
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
    if (pl >= 0) B::jx_tally(lcg_r, v, pr >= 0 ? pr : A - 1, pl);
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
    const int a = s.abits[ch][b] - 1, pm = s.pmode[ch][b], bd = bd9[ch] && !B::past_end(B::tframe + fb9 + j);
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
    lcg_ = lcg_run;
    if (B::x96a) {  // (X96's steps against the core's where it codes the core's subbands too: jx_search())
      B::jx_lo = r.x96start;
      for (int ch = 0; ch < r.npchs; ch++)
        for (int b = r.x96start; b < r.x96nsubs[ch] && b < NSUB; b++) {
          B::jx_wv[ch][b] = s.abits[ch][b] == 1;
          const S64 cs = B::jx_cs[sf][ch][b];
          if (cs <= 0) continue;
          if (s.abits[ch][b] >= 2) B::jx_count(B::jx_h[b], B::jx_hn[b], lg16((U64)x96_ss(s, ch, b)) - lg16((U64)cs) + 128);
          else if (s.abits[ch][b] == 1)
            B::jx_count(B::jx_hv[b], B::jx_hvn[b], lg16((U64)SF6[s.scale[ch][b] & 63] << 22) - lg16((U64)cs) + 128);
        }
    }
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
        if (src >= 0 && b >= r.x96nsubs[ch] && b < r.x96nsubs[src]) {
          hjs[ch][b] = s.jscale[ch][b] + 129;
          jem[ch][b] += (256 * s.jscale[ch][b] - jem[ch][b]) >> 3;
        }
        psc9[ch][b] = 0;
        if (bd9[ch] && in)
          for (int k = 0; k < 4; k++) xh9[ch][b][k] = X9[ch][b][nsmp9 - 1 - k];
      }
  }
};
