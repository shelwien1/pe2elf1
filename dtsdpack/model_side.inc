// model_side.inc -- the model's side information (model.inc): a subband's PMODE, PVQ, TMODE, joint
// scale factors and high-frequency VQ vector and the LFE samples (the sid family, sdd where the WAV
// is decoded: model_smp.inc's side_code(); the joint scale factors the jsc family), each against
// its last value and the place before's in the subframe; the bit allocations (abt; a decoded WAV's
// the abd family), against the last ones, the subband below's and the audio's energy.

template <int f_DEC, class LV>
struct ModelSide : ModelHdr<f_DEC, LV> {
  typedef ModelHdr<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::hist;
  using B::checked;
  using B::code_val;
  M_FAM(sid)
  M_FAM(sdd)
  M_FAM(sdn)
  M_FAM(jsc)
  M_FAM(abt)
  M_FAM(abd)
  M_FAM(abn)
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
    v = code_val(f, X, bit1, v, A, P, A ? A - 1 : P ? P - 1 : 0, lo, hi);
    sst = !A ? 0 : v == A - 1 ? 1 : 2;
    return v;
  }
  // An index v of nbits (PVQ, HFVQ) of sid's field fld: whether it is the last one A (1 + it, 0
  // none), else its top 4 bits as a tree, the bits below them by place
  S64 side_index(int fld, int ch, int b, S64 v, int nbits, S32 A, int ac, int ab) {
    return side_ix<M_sid_v>(f_sid, fld, ch, b, v, nbits, A, ac, ab);
  }
  template <class V, class F>
  S64 side_ix(F& f, int fld, int ch, int b, S64 v, int nbits, S32 A, int ac, int ab) {
    V D;
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
    B::sel(f, D);
    if (A && f.bit(SN_SAME, ENC && v == A - 1)) return sst = 1, A - 1;
    int x = 1;
    S64 u = 0;
    for (int i = nbits - 1; i >= 0; i--) {
      const int p = nbits - 1 - i;
      const int node = p < 4 ? SN_IX + x : SN_IXPOS + imin(p - 4, 15);
      const int bit = f.bit(node, ENC && ((v >> i) & 1));
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
  // P(not it) in 4096ths, -1 none; dec: a decoded WAV's, the scale factor search's counts or the bit
  // allocation search's rule say (the abd family's: its own parameters, since format 118); nq, an
  // unbound channel's: the counts of its ABITS, 0-26, by its last one, de and lbd (model.inc: every
  // decision's outside prediction past the first two, since format 175)
  S64 abits_code(int ch, int b, S64 v, S64 lo, S64 hi, S32 A, S32 Bw, int es, int de, int ls, int pm, int lbd,
                 int nb, int mx, S32 Sn, int nn, int sc, int eq, int sm, S32 Alt = 0, int bg = 0, int ag = 0, int sx = 0,
                 int pq = -1, int pq2 = -1, int dec = 0, int nw = 0, const double* nq = nullptr) {
    M_abt_v D;
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
    const S32 c1 = Sn ? Sn : A, c2 = Alt ? Alt : Sn ? A : Bw;
    if (nw) {  // (since format 166 a channel the WAV doesn't bind: a family of its own)
      M_abn_v E;
#define M_COPY(f) E.f = D.f;
      M_abt_VARS(M_COPY)
#undef M_COPY
      v = nq ? abits_val_n(f_abn, E, v, lo, hi, c1, c2, pq, pq2, nq) : abits_val(f_abn, E, v, lo, hi, c1, c2, pq, pq2);
    } else if (dec) {
      M_abd_v E;
#define M_COPY(f) E.f = D.f;
      M_abt_VARS(M_COPY)
#undef M_COPY
      v = abits_val(f_abd, E, v, lo, hi, c1, c2, pq, pq2);
    } else
      v = abits_val(f_abt, D, v, lo, hi, c1, c2, pq, pq2);
    ast = !A ? 0 : v == A - 1 ? 1 : 2;
    return v;
  }
  // (since format 175, an unbound channel's ABITS by the counts q of its values 0-26 (model.inc's
  // B::nwa_n, each from NWA_A) past the first two decisions too: where it is neither c1 - 1 nor c2 -
  // 1, the outside predictions of its difference's magnitude's decisions, code()'s first 8 nodes
  // (0, 1, 2 and the length 1-5 of the magnitude less 2), and of its sign, by the counts' shares of
  // the values lo..hi but those two: on 12 streams without their WAVs the magnitude's 0.13% smaller
  // than by abn alone, with the sign's 0.04% more. P(not y of r) in 4096ths, -1 where r is none)
  static int q_op(double y, double r) { return r > 1e-6 ? clip((int)(4096.0 * (1.0 - y / r) + 0.5), 64, 4096 - 64) : -1; }
  template <class F, class V>
  S64 abits_val_n(F& f, V& D, S64 v, S64 lo, S64 hi, S32 c1, S32 c2, int pq, int pq2, const double* q0) {
    tcx X;
    f.make(D, X);
    f.select(X);
    int pa[AN_N];
    for (int k = 0; k < AN_N; k++) pa[k] = -1;
    if (pq >= 0) pa[AN_SAME] = cm_pclip(pq);
    if (pq2 >= 0) pa[AN_BELOW] = cm_pclip(pq2);
    f.prpa = pa;
    S64 out;
    if (c1 && f.bit(AN_SAME, ENC && v == c1 - 1)) out = c1 - 1;
    else if (c2 && c2 != c1 && f.bit(AN_BELOW, ENC && v == c2 - 1)) out = c2 - 1;
    else {
      const int base = c1 ? c1 - 1 : c2 ? c2 - 1 : 0;
      double q[27], w[28] = {}, r = 0.0;
      for (int k = 0; k < 27; k++) {
        q[k] = k < lo || k > hi || (c1 && k == c1 - 1) || (c2 && k == c2 - 1) ? 0.0 : q0[k];
        w[k > base ? k - base : base - k] += q[k], r += q[k];
      }
      for (int j = 0; j < TC_SMALL; j++) pa[j] = q_op(w[j], r), r -= w[j];
      for (int j = 0; j < 5; j++) {  // (the length: magnitudes 2^j + 2 .. 2^(j+1) + 1)
        double g = 0.0;
        for (int m = (1 << j) + 2; m <= (2 << j) + 1 && m < 28; m++) g += w[m];
        pa[tc_node_len(j)] = q_op(g, r), r -= g;
      }
      const S64 d = ENC ? v - base : 0, m = f.code(X, d < 0 ? -d : d);
      out = base;
      if (m) {
        const S64 vn = base - m, vp = base + m;
        const bool cn = vn >= lo && vn != c1 - 1 && vn != c2 - 1, cp = vp <= hi && vp != c1 - 1 && vp != c2 - 1;
        if (cn != cp) out = cp ? vp : vn;
        else {
          const double qn = vn >= 0 && vn < 27 ? q[vn] : 0.0, qp = vp < 27 ? q[vp] : 0.0;
          pa[AN_SIGN] = q_op(qn, qn + qp);
          out = f.bit(AN_SIGN, ENC && d < 0) ? vn : vp;
        }
      }
    }
    f.prpa = nullptr;
    return checked(out, lo, hi);
  }
  template <class F, class V>
  S64 abits_val(F& f, V& D, S64 v, S64 lo, S64 hi, S32 c1, S32 c2, int pq, int pq2) {
    tcx X;
    f.make(D, X);
    f.select(X);
    int pa[HN_N];
    if (pq >= 0 || pq2 >= 0) {
      for (int k = 0; k < HN_N; k++) pa[k] = -1;
      if (pq >= 0) pa[HN_SAME] = cm_pclip(pq);
      if (pq2 >= 0) pa[HN_SAME2] = cm_pclip(pq2);
      f.prpa = pa;
    }
    v = code_val(f, X, 0, v, c1, c2, c1 ? c1 - 1 : c2 ? c2 - 1 : 0, lo, hi);
    f.prpa = nullptr;
    return v;
  }
};
