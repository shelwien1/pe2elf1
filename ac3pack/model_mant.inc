// model_mant.inc -- the model's mantissas (model.inc): each mantissa's magnitude and sign (the
// mag family), against the coefficients its neighbours give, rescaled to its quantiser: the bins
// before, the last block's at the bin, the partner channel's; and AHT's data (aht): the vector
// indices, the gains, the gain adaptive mantissas.

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

  void mant_open(void) { m_nzh = 0; }
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
  template <class F>
  INLINE int code_sign(F& f, u32 sc, int m, int lo, int hi, int v, int k) {
    int neg;
    if (!m) return 0;
    if (m > hi) neg = 1;
    else if (m > -lo) neg = 0;
    else {
      const int k0 = c->cat;
      if (k >= 0) cat(k);
      neg = f.sign(sc, ENC && v < 0);
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
    // the magnitude
    const int mmax = imax(-lo, hi);
    int m = 0;
    B::sel(f_mag, D);
    if (b < 6) {
      while (m < mmax && f_mag.bit(MN_U + imin(m, MAG_SMALL - 1), ENC && abs(v) > m)) m++;
    } else {
      m = (int)checked(code_len(f_mag, MN_LEN, quant_bits[b], LV::mag::mt, ENC ? abs(v) : 0, ENC), 0, mmax);
    }
    // its sign, where both are possible (lo..hi is one level longer on one side)
    v = code_sign(f_mag, (u32)b, m, lo, hi, v, CAT_SIGN);
    m_nzh = ((m_nzh << 1) | (v != 0)) & 15;
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
    v = code_sign(f_aht, big ? AG_BIG : AG_MANT, m, lo, hi, v, -1);
    if (!big && bk == NBLK - 1) {  // (the bin's six done)
      a_prevbin2 = a_prevbin;
      a_prevbin = bin;
      a_prevgb = nbits;
    }
    return v;
  }
};
