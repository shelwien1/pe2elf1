// model_exp.inc -- the model's exponents (model.inc): each delta of a channel's exponents in a
// block (the exp family), against the last block's exponents there, the partner channel's in the
// block, and the deltas before.

template <int f_DEC, class LV>
struct ModelExp : ModelHdr<f_DEC, LV> {
  typedef ModelHdr<f_DEC, LV> B;
  using B::ENC;
  using B::hist;
  using B::h;
  M_FAM(exp)
  int e_d1, e_d2, e_ht, e_gi;  // the channel's deltas so far in the block (exp_open())
  int es_eff[NCH];             // each channel's exponents in effect: their strategy (0 none)

  void exp_start(void) { memset(es_eff, 0, sizeof es_eff); }
  // a channel's exponents start, of strategy es
  void exp_open(int ch, int es) {
    e_d1 = e_d2 = 5;
    e_ht = e_gi = 0;
    es_eff[ch] = es;
  }

  // the least of the exponents exps[lo, hi) (99 none)
  static INLINE int least(const S8* exps, int lo, int hi) {
    int m = 99;
    for (int b = lo; b < hi; b++) m = exps[b] < m ? exps[b] : m;
    return m;
  }

  // A delta of channel ch's exponents in block blk (v: 0-4, the delta + 2), its strategy es, the
  // first of the bins it sets bin, the exponent before it ex
  int code_dexp(const Frame& f, const Run& r, int blk, int ch, int es, int bin, int ex, int v) {
    const int gsz = 1 << (es - 1);
    const Hist& H = hist[h];
    M_exp_v D;
    D.es = es;
    D.cc = ch == CPL ? 0 : ch == f.lfech ? 3 : ch == 1 ? 1 : 2;
    D.pd = 9;
    D.pl = 5;
    if (H.has[ch]) {
      const int lo = bin > H.strt[ch] ? bin : H.strt[ch], hi = bin + gsz < H.end[ch] ? bin + gsz : H.end[ch];
      if (lo < hi) {
        D.pd = clip(least(H.exps[ch], lo, hi) - ex, -4, 4) + 4;
        if (bin - gsz >= H.strt[ch] && bin < H.end[ch]) D.pl = clip(H.exps[ch][bin] - H.exps[ch][bin - gsz], -2, 2) + 2;
      }
    }
    D.po = 9;
    const int pc = partner(f.b.acmod, f.nfch, ch);
    if (pc > 0 && bin >= r.strtmant[pc] && bin < r.endmant[pc]) {
      const int hi = bin + gsz < r.endmant[pc] ? bin + gsz : r.endmant[pc];
      D.po = clip(least(r.exps[pc], bin, hi) - ex, -4, 4) + 4;
    }
    D.d1 = e_d1;
    D.d2 = e_d2;
    D.bin = bin_to_band_tab[bin < NBIN - 3 ? bin : NBIN - 4];
    D.ex = cap(ex, 31);
    D.bs = ch >= 1 && ch <= f.nfch ? f.k[blk].blksw[ch] : 0;
    D.ht = e_ht;
    D.gi = e_gi;
    D.bk = blk;
    B::sel(f_exp, D);
    if (f_exp.bit(EXN_ZERO, ENC && v == 2)) v = 2;
    else if (f_exp.bit(EXN_DOWN, ENC && v < 2)) v = f_exp.bit(EXN_DOWN2, ENC && v == 0) ? 0 : 1;
    else v = f_exp.bit(EXN_UP2, ENC && v == 4) ? 4 : 3;
    e_ht = ((e_ht << 1) | (D.pd != 9 && clip(D.pd - 4, -2, 2) == v - 2)) & 7;
    e_d2 = e_d1;
    e_d1 = v;
    e_gi = e_gi == 2 ? 0 : e_gi + 1;
    return v;
  }
};
