// model_mant.inc -- the model's mantissas (model.inc): each mantissa's magnitude and sign (the
// mag family; the sign a decision of it, by contexts of its own), against the coefficients its
// neighbours give, rescaled to its quantiser: the bins before, the last block's at the bin, the
// partner channel's; against its tile (whose largest coefficient its exponent was set by) and a
// tonal predictor's coefficient; and AHT's data (aht): the vector indices, the gains, the gain
// adaptive mantissas, against the bins before and the last frame's.

template <int f_DEC, class LV>
struct ModelMant : ModelExp<f_DEC, LV> {
  typedef ModelExp<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::hist;
  using B::h;
  using B::es_eff;
  using B::checked;
  using B::cat;
  M_FAM(mag)
  M_FAM(aht)
  int m_nzh;  // the channel's last mantissas in the block: a bit each, not 0 (mant_open())
  int a_prevbin, a_prevgb, a_pg;  // AHT: the bin before with gain adaptive mantissas, its bits; the gain before
  int a_prevbin2;                 // and the one before it
  // The tiles: a tile is a channel's exponent group (its bins of one exponent) over the blocks that
  // reuse its exponents. An encoder sets the exponent by the tile's largest coefficient, so that
  // one comes out big (|m| >= 1/2), unless the exponent was lowered (A/52's deltas reach +-2 at
  // most) or capped. Per channel, the strategy of its exponents; per tile (by its first bin),
  // whether one of its mantissas was big, how many were coded, whether one of its bins had none.
  S8 t_es[NCH];
  U8 t_big[NCH][NBIN], t_cnt[NCH][NBIN], t_hid[NCH][NBIN];

  void mant_open(void) { m_nzh = 0; }
  // the channel's exponents, of strategy es, start new tiles
  void tile_open(int ch, int es) {
    t_es[ch] = (S8)es;
    memset(t_big[ch], 0, NBIN);
    memset(t_cnt[ch], 0, NBIN);
    memset(t_hid[ch], 0, NBIN);
  }
  // the first bin of the tile of channel ch's bin, and its bins (*n)
  INLINE int tile_of(const Run& r, int ch, int bin, int* n) const {
    const int strt = r.strtmant[ch], gsz = t_es[ch] ? 1 << (t_es[ch] - 1) : 1, o = ch != CPL;  // (absexp's bin alone)
    if (o && bin == strt) return *n = 1, strt;
    return *n = gsz, strt + o + (bin - strt - o) / gsz * gsz;
  }
  // a block done: its bins without mantissas, in their tiles (the largest coefficient may be one)
  void tile_block(const Frame& f, const Run& r) {
    hist[h].tn_blk++;  // (the tonal predictor goes on in the bins that had a long block's mantissa)
    for (int ch = 0; ch <= f.nch; ch++) {
      const int off = (ch == CPL && !r.cplinu) || r.chahtinu[ch];
      if (off || !t_es[ch]) continue;
      for (int bin = r.strtmant[ch]; bin < r.endmant[ch]; bin++) {
        int n;
        if (!r.bap[ch][bin]) t_hid[ch][tile_of(r, ch, bin, &n)] = 1;
      }
    }
  }

  // A coefficient as its mantissa gives it (level q of bap b, at exponent e), in 2^-40 of full
  // scale, and a level's step there (bap 1-5's rounded down)
  static INLINE S64 tn_step(int b, int e) {
    return b >= 6 ? (S64)1 << (41 - quant_bits[b] - e) : (((S64)1 << 41) >> e) / quant_bits[b];
  }
  // The tonal predictor's variables at channel ch's bin (et, ts, tq: famvars.inc), a long block's
  // with bap b at exponent e: a steady tone makes each bin's coefficients over the blocks a sampled
  // sinusoid, x = a x1 - x2 (a = 2 cos of the tone's phase advance a block); its prediction (*p)
  void tn_vars(M_mag_v& D, int ch, int bin, int b, int e, int sw, S64* p) const {
    const Hist& H = hist[h];
    D.et = D.ts = D.tq = 0;
    *p = 0;
    if (sw || H.tn_t[ch][bin] != H.tn_blk || H.tn_n[ch][bin] < 3) return;
    const S64 x1 = H.tn_x1[ch][bin], st = tn_step(b, e), em = H.tn_e[ch][bin], mm = H.tn_m[ch][bin];
    *p = H.tn_a[ch][bin] * x1 / 16384 - H.tn_x2[ch][bin];
    const S64 ap = *p < 0 ? -*p : *p;
    D.et = est_var((U64)(4 * ap / st));
    D.ts = 2 * ap < st ? 0 : *p > 0 ? 1 : 2;
    D.tq = 1 + (4 * em < mm ? 0 : 2 * em < mm ? 1 : em < mm ? 2 : 3);
  }
  // the predictor after the bin's coefficient x (*p its prediction)
  void tn_update(int ch, int bin, S64 x, S64 p, S64 st) {
    Hist& H = hist[h];
    U8& n = H.tn_n[ch][bin];
    if (H.tn_t[ch][bin] != H.tn_blk) n = 0, H.tn_e[ch][bin] = H.tn_m[ch][bin] = 0;  // (none in the block before)
    H.tn_t[ch][bin] = H.tn_blk + 1;
    const S64 x1 = H.tn_x1[ch][bin], x2 = H.tn_x2[ch][bin];
    if (n >= 3) {
      const S64 e = x - p;
      H.tn_e[ch][bin] = (3 * H.tn_e[ch][bin] + (e < 0 ? -e : e)) / 4;
      H.tn_m[ch][bin] = (3 * H.tn_m[ch][bin] + (x < 0 ? -x : x)) / 4;
    }
    if (n >= 2 && (x1 > st || x1 < -st)) {  // (the step a tone takes: the three last make it)
      S64 ao = (x + x2) * 16384 / x1;
      ao = ao > 32768 ? 32768 : ao < -32768 ? -32768 : ao;
      H.tn_a[ch][bin] = (S32)(n >= 3 ? (3 * H.tn_a[ch][bin] + ao) / 4 : ao);
      n = 3;
    } else if (n < 2) {
      n++;
    }
    H.tn_x2[ch][bin] = x1;
    H.tn_x1[ch][bin] = x;
  }
  // whether level q of bap b is big: |m| >= 1/2, as the quantiser rounds it
  static INLINE int is_big(int q, int b) {
    static const U8 t[6] = {0, 1, 1, 2, 3, 4};
    const int a = q < 0 ? -q : q;
    return b < 6 ? a >= t[b] : a >= 1 << (quant_bits[b] - 2);
  }
  // The variables of the bin's tile (famvars.inc's tx, tb, tc, tr); its first bin
  int tile_vars(M_mag_v& D, const Frame& f, const Run& r, int blk, int ch, int bin) const {
    int n;
    const int g0 = tile_of(r, ch, bin, &n), g1 = imin(g0 + n, r.endmant[ch]), ec = r.exps[ch][bin];
    const int satl = g0 > r.strtmant[ch] && ec - r.exps[ch][g0 - 1] == 2;
    const int satr = g0 + n < r.endmant[ch] && r.exps[ch][g0 + n] - ec == -2;
    int hid = t_hid[ch][g0], nb = 0, rb = 0, fb = -1;
    for (int k = g0; k < g1; k++) {
      if (!r.bap[ch][k]) hid = 1;
      else nb++, rb += k > bin;
    }
    if (f.eac3 || blk == f.nblk - 1) fb = reuse_after(f, blk, ch);  // (known)
    D.tx = ec == 24 || (ch != CPL && bin == r.strtmant[ch] && ec == 15) ? 0 : satl && satr ? 1 : satl ? 2 : satr ? 3 : 4;
    D.tb = t_big[ch][g0] ? 2 : hid;
    D.tc = imin(t_cnt[ch][g0], 3);
    D.tr = fb >= 0 ? imin(rb + fb * nb, 3) : 4 + imin(rb, 3);
    return g0;
  }

  void aht_open(void) {
    a_prevbin = a_prevbin2 = -1;
    a_prevgb = 1;
    a_pg = 3;
  }

  // The estimate the bin b2 of channel ch (its mantissa q2, this block's) gives the bin with bap b
  // and exponent ec (mant_est(): 0 none, 1 no mantissa there)
  static INLINE int est_bin(const Run& r, int ch, int b2, S16 q2, int b, int ec) {
    if (b2 < r.strtmant[ch] || b2 >= r.endmant[ch]) return 0;
    return est_of(r.bap[ch][b2], q2, r.exps[ch][b2], b, ec);
  }

  // The sign of the magnitude m of a value v in lo..hi, where both are possible (lo..hi is one level
  // longer on one side): by family f's sign context sc (as category k where k >= 0)
  // (or, sc < 0, as decision node -sc of the contexts selected)
  template <class F>
  INLINE int code_sign(F& f, int sc, int m, int lo, int hi, int v, int k) {
    int neg;
    if (!m) return 0;
    if (m > hi) neg = 1;
    else if (m > -lo) neg = 0;
    else {
      const int k0 = c->cat;
      if (k >= 0) cat(k);
      neg = sc < 0 ? f.bit(-sc, ENC && v < 0) : f.sign((u32)sc, ENC && v < 0);
      cat(k0);
    }
    return neg ? -m : m;
  }

  // A mantissa (the level v, in lo..hi) of channel ch at bin, bap b, in block blk: its magnitude,
  // its sign where both are possible
  int code_q(const Frame& f, const Run& r, int blk, int ch, int bin, int b, int lo, int hi, int v) {
    const Hist& H = hist[h];
    const S16* q = f.k[blk].q[ch];
    const int ec = r.exps[ch][bin], strt = r.strtmant[ch];
    M_mag_v D;
    D.bap = b;
    D.gr = b == 1 || b == 2 || b == 4 ? (hi > -lo ? 0 : 1) : b < 6 ? 2 : 3;
    D.ex = ec;
    D.dx = bin > strt ? dvar(ec - r.exps[ch][bin - 1]) : 9;
    D.dn = bin + 1 < r.endmant[ch] ? dvar(r.exps[ch][bin + 1] - ec) : 9;
    D.gs = es_eff[ch];
    D.bs = blksw_of(f, blk, ch);
    D.cc = chan_class(f, ch);
    D.bin = bin_to_band_tab[bin];
    D.e1 = est_bin(r, ch, bin - 1, bin > strt ? q[bin - 1] : 0, b, ec);
    D.e2 = est_bin(r, ch, bin - 2, bin - 1 > strt ? q[bin - 2] : 0, b, ec);
    {  // the mean of the bins before's
      EstMean sm;
      for (int k = 1; k <= 4; k++)
        if (bin - k >= strt) sm.add(est_bin(r, ch, bin - k, q[bin - k], b, ec));
      D.sm = sm.get(0);
    }
    D.ep = D.ew = 0;
    D.sp = 0;
    if (H.has[ch] >= 2 && bin >= H.strt[ch] && bin < H.end[ch]) {
      EstMean ew;
      D.ep = est_of(H.bap[ch][bin], H.q[ch][bin], H.exps[ch][bin], b, ec);
      D.sp = svar(H.q[ch][bin]);
      for (int k = bin - 1; k <= bin + 1; k++)
        if (k >= H.strt[ch] && k < H.end[ch]) ew.add(est_of(H.bap[ch][k], H.q[ch][k], H.exps[ch][k], b, ec));
      D.ew = ew.get(1);
    }
    D.eo = D.ow = 0;
    D.so = 0;
    const int pc = partner(f.b.acmod, f.nfch, ch);
    if (pc > 0 && !r.chahtinu[pc]) {
      const S16* qp = f.k[blk].q[pc];
      EstMean ow;
      D.eo = est_bin(r, pc, bin, qp[bin], b, ec);
      if (D.eo) D.so = svar(qp[bin]);
      for (int k = bin - 1; k <= bin + 1; k++)
        if (k >= 0) ow.add(est_bin(r, pc, k, qp[k], b, ec));
      D.ow = ow.get(D.eo ? 1 : 0);
    }
    D.m1 = bin > strt ? (r.bap[ch][bin - 1] ? imin(abs(q[bin - 1]), kCap) : kCap + 1) : kCap + 2;
    D.nz = __builtin_popcount((unsigned)m_nzh & 15);
    D.s1 = bin > strt ? svar(q[bin - 1]) : 0;
    D.s2 = bin - 1 > strt ? svar(q[bin - 2]) : 0;
    D.rm = 0;  // (2/0: whether the bin's rematrixing band is rematrixed)
    if (f.b.acmod == 2 && ch >= 1)
      for (int k = 0; k < r.nrematbd; k++)
        if (bin >= rematrix_band_tab[k] && bin < rematrix_band_tab[k + 1]) D.rm = 1 + (r.rematflg[k] != 0);
    const int g0 = tile_vars(D, f, r, blk, ch, bin);
    S64 tp;
    tn_vars(D, ch, bin, b, ec, ch >= 1 && ch <= f.nfch && f.k[blk].blksw[ch], &tp);
    // the magnitude, by its contexts without the signs'
    const int mmax = imax(-lo, hi);
    int m = 0;
    M_mag_v DS = D;
    D.s1 = D.s2 = D.sp = D.so = D.ts = 0;
    B::sel(f_mag, D);
    if (b < 6) {
      while (m < mmax && f_mag.bit(MN_U + imin(m, MAG_SMALL - 1), ENC && abs(v) > m)) m++;
    } else {
      m = (int)checked(code_len(f_mag, MN_LEN, quant_bits[b], LV::mag::mt, ENC ? abs(v) : 0, ENC), 0, mmax);
    }
    // its sign, where both are possible (lo..hi is one level longer on one side)
    if (m && m <= hi && m <= -lo) B::sel(f_mag, DS);  // (the sign's, where it is coded: a decision node)
    v = code_sign(f_mag, -MN_SIGN, m, lo, hi, v, CAT_SIGN);
    m_nzh = ((m_nzh << 1) | (v != 0)) & 15;
    if (t_cnt[ch][g0] < UINT8_MAX) t_cnt[ch][g0]++;
    if (is_big(v, b)) t_big[ch][g0] = 1;
    if (!(ch >= 1 && ch <= f.nfch && f.k[blk].blksw[ch])) {  // (a short block's: none)
      const S64 st = tn_step(b, ec);
      tn_update(ch, bin, v * st, tp, st);
    }
    return v;
  }

  // The estimate a gain adaptive mantissa v2 (hebap hb2, exponent e2) gives one of hebap hb at
  // exponent e (as mant_est()'s: 2 + quarter levels, to 2 + kEst): their steps are 2^(1 - bits),
  // whatever the gain
  static INLINE int gaq_est(int v2, int hb2, int e2, int hb, int e) {
    return est_scaled((U64)abs(v2) * 4, 1, bits_vs_hebap[hb] - bits_vs_hebap[hb2] + e - e2);
  }
  // AHT's variables at bin (hebap hb) of channel ch
  void aht_vars(M_aht_v& D, const Frame& f, const Run& r, int ch, int bin, int hb, int kd) {
    D.kd = kd;
    D.hb = hb;
    D.lg = 0;
    D.gm = f.aht[ch].gaqmod;
    D.bk = 0;
    D.pm = D.pb = 0;
    D.pg = a_pg;
    D.ex = bin >= r.strtmant[ch] && bin < r.endmant[ch] ? r.exps[ch][bin] : 0;
    D.bin = band_of(bin);
    D.q1 = D.q2 = D.q0 = D.qa = 0;
    D.pf = 0;
  }
  // the frame's AHT data into the history, as its block 0 ends (where it all comes)
  void aht_commit(const Frame& f, const Run& r) {
    Hist& H = hist[h];
    for (int ch = 0; ch <= f.nch; ch++) {
      H.ah_has[ch] = (U8)(r.chahtinu[ch] != 0);
      if (!r.chahtinu[ch]) continue;
      for (int bin = 0; bin < NBIN; bin++) {
        const int in = bin >= r.strtmant[ch] && bin < r.endmant[ch];
        H.ah_hb[ch][bin] = in ? r.bap[ch][bin] : 0;
        H.ah_ex[ch][bin] = in ? r.exps[ch][bin] : 0;
        memcpy(H.ah_m[ch][bin], f.aht[ch].m[bin], sizeof H.ah_m[ch][bin]);
      }
    }
  }
  // a gain of gaqmod 1-2 (a bit)
  int code_gaqgain(const Frame& f, const Run& r, int ch, int bin, int hb, int v) {
    M_aht_v D;
    aht_vars(D, f, r, ch, bin, hb, 1);
    B::sel(f_aht, D);
    v = f_aht.bit(AN_GAIN, ENC && v);
    a_pg = v;
    return v;
  }
  // a gain of gaqmod 3 (0-2)
  int code_gain3(const Frame& f, const Run& r, int ch, int bin, int hb, int v) {
    M_aht_v D;
    aht_vars(D, f, r, ch, bin, hb, 2);
    B::sel(f_aht, D);
    if (f_aht.bit(AN_G3A, ENC && v == 0)) v = 0;
    else v = f_aht.bit(AN_G3B, ENC && v == 1) ? 1 : 2;
    a_pg = v;
    return v;
  }
  // a vector index of nbits (hebap hb): its top bits as a tree, the rest by place
  int code_vq(const Frame& f, const Run& r, int ch, int bin, int hb, int nbits, int v) {
    M_aht_v D;
    int x = 1, u = 0;
    aht_vars(D, f, r, ch, bin, hb, 0);
    B::sel(f_aht, D);
    for (int i = nbits - 1; i >= 0; i--) {
      const int p = nbits - 1 - i;
      const int node = p < AHT_VQTREE ? AN_VQ + x : AN_VQPOS + imin(p - AHT_VQTREE, 15);
      const int bit = f_aht.bit(node, ENC && ((v >> i) & 1));
      if (p < AHT_VQTREE) x = x * 2 + bit;
      u = u * 2 + bit;
    }
    return u;
  }
  // a gain adaptive mantissa (big 0) of channel ch at bin in block bk, of nbits (gain log_gain,
  // hebap hb), or an escape's large mantissa (big 1): its magnitude, its sign where both are
  // possible
  int code_gaq(const Frame& f, const Run& r, int ch, int bin, int hb, int lg, int bk, int nbits, int big, int v) {
    const Aht& a = f.aht[ch];
    M_aht_v D;
    aht_vars(D, f, r, ch, bin, hb, 3 + big);
    D.lg = lg;
    D.bk = bk;
    if (!big) {
      const int e = r.exps[ch][bin];
      const Hist& H = hist[h];
      if (H.ah_has[ch] && H.ah_hb[ch][bin] >= 8)  // (the last frame's mantissa there)
        D.pf = gaq_est(H.ah_m[ch][bin][bk], H.ah_hb[ch][bin], H.ah_ex[ch][bin], hb, e);
      if (bk) {
        int s = 0;
        D.pm = 1 + imin((abs(a.m[bin][bk - 1]) << 3) >> (nbits - 1), 8);
        D.q0 = est_var((U64)(4 * abs(a.m[bin][0])));
        for (int k = 0; k < bk; k++) s += abs(a.m[bin][k]);
        D.qa = est_var((U64)(4 * s / bk));
      }
      if (a_prevbin >= 0) {
        D.pb = 1 + imin((abs(a.m[a_prevbin][bk]) << 3) >> (a_prevgb - 1), 8);
        D.q1 = gaq_est(a.m[a_prevbin][bk], r.bap[ch][a_prevbin], r.exps[ch][a_prevbin], hb, e);
      }
      if (a_prevbin2 >= 0) D.q2 = gaq_est(a.m[a_prevbin2][bk], r.bap[ch][a_prevbin2], r.exps[ch][a_prevbin2], hb, e);
    }
    const int lo = -(1 << (nbits - 1)), hi = (1 << (nbits - 1)) - 1;
    B::sel(f_aht, D);
    const int m = (int)checked(code_len(f_aht, AN_LEN, nbits, LV::aht::mt, ENC ? abs(v) : 0, ENC), 0, -lo);
    v = code_sign(f_aht, big ? AG_BIG : AG_MANT, m, lo, hi, v, -1);  // (by its own counters)
    if (!big && bk == NBLK - 1) {  // (the bin's six done)
      a_prevbin2 = a_prevbin;
      a_prevbin = bin;
      a_prevgb = nbits;
    }
    return v;
  }
};
