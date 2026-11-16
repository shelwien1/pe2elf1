// model_side.inc -- the model's side information (model.inc): a subband's PMODE, PVQ, TMODE, joint
// scale factors and high-frequency VQ vector and the LFE samples (the sid family; the joint scale
// factors the jsc family), each against its last value and the place before's in the subframe; the
// bit allocations (abt), against the last ones, the subband below's and the audio's energy.

template <int f_DEC, class LV>
struct ModelSide : ModelHdr<f_DEC, LV> {
  typedef ModelHdr<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::hist;
  using B::checked;
  using B::code_val;
  M_FAM(sid)
  M_FAM(jsc)
  M_FAM(abt)
  int sst;  // sid: whether the value before in the subframe kept its last one (0 none, 1 kept, 2 changed)
  int ast;  // abt: the same of the subband below's ABITS

  // A value v of sid's field fld (in lo..hi; a flag where bit1) of channel ch's subband b: against
  // its last value A and the place before's, P (1 + them, 0 none); ac, ab, ec as famvars.inc says.
  // The joint scale factors go to the jsc family, with the same variables
  S64 side_code(int fld, int ch, int b, S64 v, S64 lo, S64 hi, int bit1, S32 A, S32 P, int ac, int ab, int ec, int tr = 0) {
    if (fld == SF_JSCALE) return side_val<M_jsc_v>(f_jsc, fld, ch, b, v, lo, hi, bit1, A, P, ac, ab, ec, tr);
    return side_val<M_sid_v>(f_sid, fld, ch, b, v, lo, hi, bit1, A, P, ac, ab, ec, tr);
  }
  template <class V, class F>
  S64 side_val(F& f, int fld, int ch, int b, S64 v, S64 lo, S64 hi, int bit1, S32 A, S32 P, int ac, int ab, int ec, int tr) {
    V D;
    tcx X;
    D.fld = fld;
    D.cc = chan_class(ch);
    D.bd = b;
    D.lv = vvar(A, kVal);
    D.pq = vvar(P, kVal);
    D.st = sst;
    D.ac = ac;
    D.ab = ab;
    D.ec = ec;
    D.tr = tr;
    f.make(D, X);
    f.select(X);
    v = code_val(f, X, bit1, v, A, P, A ? A - 1 : P ? P - 1 : 0, (u32)fld, lo, hi);
    sst = !A ? 0 : v == A - 1 ? 1 : 2;
    return v;
  }
  // An index v of nbits (PVQ, HFVQ) of sid's field fld: whether it is the last one A (1 + it, 0
  // none), else its top 4 bits as a tree, the bits below them by place
  S64 side_index(int fld, int ch, int b, S64 v, int nbits, S32 A, int ac, int ab) {
    M_sid_v D;
    D.fld = fld;
    D.cc = chan_class(ch);
    D.bd = b;
    D.lv = vvar(A, kVal);
    D.pq = 0;
    D.st = sst;
    D.ac = ac;
    D.ab = ab;
    D.ec = 9;
    D.tr = 0;
    B::sel(f_sid, D);
    if (A && f_sid.bit(SN_SAME, ENC && v == A - 1)) return sst = 1, A - 1;
    int x = 1;
    S64 u = 0;
    for (int i = nbits - 1; i >= 0; i--) {
      const int p = nbits - 1 - i;
      const int node = p < 4 ? SN_IX + x : SN_IXPOS + imin(p - 4, 15);
      const int bit = f_sid.bit(node, ENC && ((v >> i) & 1));
      if (p < 4) x = x * 2 + bit;
      u = u * 2 + bit;
    }
    sst = A ? 2 : 0;
    return u;
  }

  // ABITS v (in lo..hi) of channel ch's subband b: against its last value A, the subband below's in
  // the subframe Bw (1 + them, 0 none), and the audio's energy (es, de, ls, lbd as famvars.inc says)
  // (Sn: the first guess, 1 + it, 0 none; then Alt, the second (1 + it), else the last one; then the
  // difference from the first of them there is; pq, pq2: the outside predictions of the first two,
  // P(not it) in 4096ths, -1 none)
  S64 abits_code(int ch, int b, S64 v, S64 lo, S64 hi, S32 A, S32 Bw, int es, int de, int ls, int pm, int lbd,
                 int nb, int mx, S32 Sn, int nn, int sc, int eq, int sm, S32 Alt = 0, int bg = 0, int ag = 0, int sx = 0,
                 int pq = -1, int pq2 = -1) {
    M_abt_v D;
    tcx X;
    D.bd = b;
    D.cc = chan_class(ch);
    D.la = vvar(A, 27);
    D.lb = vvar(Bw, 27);
    D.es = es;
    D.de = de;
    D.ls = ls;
    D.pm = pm;
    D.st = lbd;
    D.nb = nb;
    D.mx = mx;
    D.sn = vvar(Sn, 27);
    D.sd = !Sn || !A ? 0 : Sn == A ? 1 : 2;
    D.nn = nn;
    D.sc = sc;
    D.eq = eq;
    D.sm = sm;
    D.bg = bg;
    D.ag = ag;
    D.sx = sx;
    f_abt.make(D, X);
    f_abt.select(X);
    const S32 c1 = Sn ? Sn : A, c2 = Alt ? Alt : Sn ? A : Bw;
    int pa[HN_N];
    if (pq >= 0 || pq2 >= 0) {
      for (int k = 0; k < HN_N; k++) pa[k] = -1;
      if (pq >= 0) pa[HN_SAME] = cm_pclip(pq);
      if (pq2 >= 0) pa[HN_SAME2] = cm_pclip(pq2);
      f_abt.prpa = pa;
    }
    v = code_val(f_abt, X, 0, v, c1, c2, c1 ? c1 - 1 : c2 ? c2 - 1 : 0, (u32)imin(b, M_abt_NG - 1), lo, hi);  // (X96's subbands past 31: 31's)
    f_abt.prpa = nullptr;
    ast = !A ? 0 : v == A - 1 ? 1 : 2;
    return v;
  }
};
