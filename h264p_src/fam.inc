// A model family (oggcomp's tc_fam, balrogg-P-hash tc_fam.inc): the integer
// coder every IDX family is. A value is a cascade of binary decisions --
// zero? one? two? then the length of the rest in unary and its bits below
// the top one -- and each decision is predicted from six counters (tables
// A-E, H under six contexts), an APM, two mixers and a second APM, all
// indexed by the decision's node in the cascade. Decisions outside the
// cascade use bit() with nodes of their own, and the sign of a value has its
// own counters (G). prp, when set, is one more prediction for the mixer.
//
// Changed from oggcomp: no codebook-derived priors (tc_pcur), and errors
// are exceptions.

namespace h264p {

constexpr int TC_MIXN = 9;
constexpr int TC_NODE = 29;
constexpr int TC_MNODE = 169;
constexpr int TC_NBMAX = 62;
static INLINE int tc_node_len(int k) { return 3 + (k < 25 ? k : 25); }
static INLINE int tc_node_man(int nb, int p) { return (nb > 12 ? 12 : nb) * 13 + (p > 12 ? 12 : p); }
static void tc_vfits(const char *what, sz vol, sz nodes) {
  if(!(vol > 0 && vol < ((sz)1 << 31) && nodes > 0))
    cm_fail("an IDX family gives the %s an index of %" PRIu64 " rows; a Volume must stay under 2^31", what, (u64)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  if(!(vol * mult < ((sz)1 << 31)))
    cm_fail("an IDX family gives the %s %" PRIu64 " rows of %" PRIu64 "; the product must stay under 2^31", what, (u64)vol, (u64)mult);
}
//  Where the bits went, by stage (-v)
static double tc_bits[32];
static u64 tc_syms[32];
static int tc_stage, tc_verbose;
//  The context indices of one value: what idx2inc.pl's MakeIndex builds
struct tcx {
  int a, b, c, d, e, h, s, f, m, w, t, n, g;
};

//  The direction is a template parameter, f_DEC as in Rangecoder: 0
//  codes, 1 decodes.  One bit through the one coder there is,
//  Model<f_DEC>::rc -- a static member reached by type, so a family needs
//  no pointer to the model it belongs to.  The model is defined after the
//  families it holds; the name is dependent, so it is looked up when this
//  is instantiated, by which time it is there.
template <int f_DEC> struct Model;
template <int f_DEC> static INLINE int tc_bit(int p, int bit) {
  uint b = (uint)bit;
  if(!f_DEC && tc_verbose)
    tc_bits[tc_stage] -= log2(bit ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
  Model<f_DEC>::rc.rc_Process((uint)(p << 3), b);
  return (int)b;
}
template <int f_DEC> struct tc_fam {
  static constexpr int ENC = (f_DEC == 0);
  int hA, hB, hD, hE, hH, hS, hF, hM, hT, hN;
  cm_cnt *A, *B, *C, *D, *E, *H, *G, *T;
  i16 *S, *S2;
  i32 *W;
  cm_apm ap, ap2;
  cm_mix<TC_MIXN> mx, mx2, mx3;
  cm_mix<4> mxf;
  cm_mix<3> mxm;
  int prp;          // an outside prediction of P(0) for the mixers, -1 for none
  const int *prpa;  // or one per node
  sz ba, bb, bc, bd, be, bh;
  u32 bs, bf, bm, bw2, bt, bn, bg;
  int rA, rB, rC, rD, rE, rH, rT, rG;
  int mwA, mwB, mwC, mwD, mwE, mwH, mwT, mwG;
  int rS1, rS2;
  int lr, mb, lrm, mbm;
  int bw;
  int ng;
  int nd, nm;
  void wire(cm_cnt *a, sz va, cm_cnt *b, sz vb, cm_cnt *c, sz vc, cm_cnt *d, sz vd, cm_cnt *e, sz ve, cm_cnt *h, sz vh, cm_cnt *t, sz vt, i16 *s, u8 *sc, sz vs, i16 *s2, u8 *sc2, sz vf, i32 *w, sz vm, cm_cnt *g, sz vg, sz vgi, i32 *wm, sz vn, u8 *wc, u8 *wmc, i32 *w2, u8 *w2c, sz vw, i32 *w3, u8 *w3c, i32 *wf, u8 *wfc, int ra, int rb, int rc_, int rd, int re, int rh, int rt, int rg, int mwa, int mwb, int mwc, int mwd, int mwe, int mwh, int mwtt, int mwg, int rs1, int rs2, int lrate, int mbt, int lrm2, int mbm2, int bwt, int qqs, int qqf, int ndp, int nmp, int ha, int hb, int hd, int he, int hh, int hs, int hf, int hm, int ht, int hn) {
    A = a;
    B = b;
    C = c;
    D = d;
    E = e;
    H = h;
    T = t;
    S = s;
    S2 = s2;
    W = w;
    G = g;
    ng = (int)vg;
    nd = ndp;
    nm = nmp;
    //  Hashed where the Volume outgrows 2^h rows, direct otherwise; the
    //  rows are what the arrays were declared with (tc_rows).
    hA = va > ((sz)1 << tc_hbits(ha)) ? tc_hbits(ha) : 0;
    hB = vb > ((sz)1 << tc_hbits(hb)) ? tc_hbits(hb) : 0;
    hD = vd > ((sz)1 << tc_hbits(hd)) ? tc_hbits(hd) : 0;
    hE = ve > ((sz)1 << tc_hbits(he)) ? tc_hbits(he) : 0;
    hH = vh > ((sz)1 << tc_hbits(hh)) ? tc_hbits(hh) : 0;
    hS = vs > ((sz)1 << tc_hbits(hs)) ? tc_hbits(hs) : 0;
    hF = vf > ((sz)1 << tc_hbits(hf)) ? tc_hbits(hf) : 0;
    hM = vm > ((sz)1 << tc_hbits(hm)) ? tc_hbits(hm) : 0;
    hT = vt > ((sz)1 << tc_hbits(ht)) ? tc_hbits(ht) : 0;
    hN = vn > ((sz)1 << tc_hbits(hn)) ? tc_hbits(hn) : 0;
    {
      sz ka = tc_rows((long long)va, ha), kb = tc_rows((long long)vb, hb), kd = tc_rows((long long)vd, hd);
      sz ke = tc_rows((long long)ve, he), kh = tc_rows((long long)vh, hh);
      sz ks = tc_rows((long long)vs, hs), kf = tc_rows((long long)vf, hf), km = tc_rows((long long)vm, hm);
      sz kt = tc_rows((long long)vt, ht), kn = tc_rows((long long)vn, hn);
      tc_vfits("counter A", ka, nd);
      tc_vfits("counter B", kb, nd);
      tc_vfits("counter C", vc, nd);
      tc_vfits("counter D", kd, nd);
      tc_vfits("counter E", ke, nd);
      tc_vfits("counter H", kh, nd);
      tc_vfits("counter T", kt, nm);
      if(G)
        tc_vfits("counter G", vg, vgi);
      tc_ifits("APM 1", ks, (sz)nd * qqs);
      tc_ifits("APM 2", kf, (sz)nd * qqf);
      tc_ifits("mixer", km, (sz)nd * TC_MIXN);
      tc_ifits("mantissa mixer", kn, (sz)nm * 3);
      ap.init(S, sc, ks * nd, qqs);
      ap2.init(S2, sc2, kf * nd, qqf);
      mx.init(W, wc, km * nd);
      tc_ifits("mixer 2", vw, (sz)nd * TC_MIXN);
      mx2.init(w2, w2c, vw * nd);
      mx3.init(w3, w3c, ks * nd);
      mxf.init(wf, wfc, nd);
      mxm.init(wm, wmc, kn * nm);
    }
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rE = tc_clamp(re, 1, CM_TMAX);
    rH = tc_clamp(rh, 1, CM_TMAX);
    rT = tc_clamp(rt, 1, CM_TMAX);
    rG = tc_clamp(rg, 1, CM_TMAX);
    mwA = tc_clamp(mwa * 64, 0, CM_CONE / 4);
    mwB = tc_clamp(mwb * 64, 0, CM_CONE / 4);
    mwC = tc_clamp(mwc * 64, 0, CM_CONE / 4);
    mwD = tc_clamp(mwd * 64, 0, CM_CONE / 4);
    mwE = tc_clamp(mwe * 64, 0, CM_CONE / 4);
    mwH = tc_clamp(mwh * 64, 0, CM_CONE / 4);
    mwT = tc_clamp(mwtt * 64, 0, CM_CONE / 4);
    mwG = tc_clamp(mwg * 64, 0, CM_CONE / 4);
    rS1 = cm_apm::limit(tc_clamp(rs1, 1, 15));
    rS2 = cm_apm::limit(tc_clamp(rs2, 1, 15));
    lr = tc_clamp(lrate, 1, 64);
    mb = tc_clamp(mbt, 0, 255);
    lrm = tc_clamp(lrm2, 1, 64);
    mbm = tc_clamp(mbm2, 0, 255);
    bw = tc_clamp(bwt, 0, 16);
    prp = -1;
    prpa = nullptr;
  }
  INLINE void select(const tcx &x) {
    ba = (sz)(hA ? tc_hrow((u32)x.a, hA) : (u32)x.a) * nd;
    bb = (sz)(hB ? tc_hrow((u32)x.b, hB) : (u32)x.b) * nd;
    bc = (sz)x.c * nd;
    bd = (sz)(hD ? tc_hrow((u32)x.d, hD) : (u32)x.d) * nd;
    be = (sz)(hE ? tc_hrow((u32)x.e, hE) : (u32)x.e) * nd;
    bh = (sz)(hH ? tc_hrow((u32)x.h, hH) : (u32)x.h) * nd;
    bs = (hS ? tc_hrow((u32)x.s, hS) : (u32)x.s) * nd;
    bf = (hF ? tc_hrow((u32)x.f, hF) : (u32)x.f) * nd;
    bm = (hM ? tc_hrow((u32)x.m, hM) : (u32)x.m) * nd;
    bw2 = (u32)x.w * nd;
    bt = (hT ? tc_hrow((u32)x.t, hT) : (u32)x.t) * nm;
    bn = (hN ? tc_hrow((u32)x.n, hN) : (u32)x.n) * nm;
    bg = (u32)x.g;
    __builtin_prefetch(A + ba);
    __builtin_prefetch(B + bb);
    __builtin_prefetch(C + bc);
    __builtin_prefetch(D + bd);
    __builtin_prefetch(E + be);
    __builtin_prefetch(H + bh);
    __builtin_prefetch(ap.t + (sz)bs * (u32)ap.q);
    __builtin_prefetch(ap.c + (sz)bs * (u32)ap.q);
    __builtin_prefetch(mx.w + (sz)bm * TC_MIXN);
    __builtin_prefetch(mx.c + bm);
    __builtin_prefetch(ap2.t + (sz)bf * (u32)ap2.q);
    __builtin_prefetch(ap2.c + (sz)bf * (u32)ap2.q);
  }
  INLINE int bit(int node, int b) {
    cm_cnt a = A[ba + node];
    cm_cnt c = B[bb + node];
    cm_cnt e = C[bc + node];
    cm_cnt f = D[bd + node];
    cm_cnt g = E[be + node];
    cm_cnt h = H[bh + node];
    int pa = a.P();
    int p1 = ap.pp(pa, bs + (u32)node);
    if(p1 > CM_PONE - 1)
      p1 = CM_PONE - 1;
    mx.set(0, cm_stretch(p1));
    mx.set(1, cm_stretch(c.P()));
    mx.set(2, cm_stretch(e.P()));
    mx.set(3, cm_stretch(f.P()));
    mx.set(4, cm_stretch(g.P()));
    mx.set(5, cm_stretch(h.P()));
    mx.set(6, cm_stretch(pa));
    int pp = prpa ? prpa[node] : prp;
    mx.set(7, pp >= 0 ? cm_stretch(pp) : 0);
    mx.set(8, 256);
    for(int i = 0; i < TC_MIXN; i++)
      mx2.set(i, mx.x[i]), mx3.set(i, mx.x[i]);
    {
      mxf.set(0, cm_stretch(mx.mix(bm + (u32)node)));
      mxf.set(1, cm_stretch(mx2.mix(bw2 + (u32)node)));
      mxf.set(2, cm_stretch(mx3.mix(bs + (u32)node)));
      mxf.set(3, 256);
      int pm = mxf.mix((u32)node);
      int pf = ap2.pp(pm, bf + (u32)node);
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit<f_DEC>(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b);
    }
    mx.upd(b, lr, mb);
    mx2.upd(b, lr, mb);
    mx3.upd(b, lr, mb);
    mxf.upd(b, lr, mb);
    ap.upd(b, rS1);
    ap2.upd(b, rS2);
    a.upd(b, rA, mwA);
    A[ba + node] = a;
    c.upd(b, rB, mwB);
    B[bb + node] = c;
    e.upd(b, rC, mwC);
    C[bc + node] = e;
    f.upd(b, rD, mwD);
    D[bd + node] = f;
    g.upd(b, rE, mwE);
    E[be + node] = g;
    h.upd(b, rH, mwH);
    H[bh + node] = h;
    return b;
  }
  INLINE int bitm(int node, int b) {
    cm_cnt t = T[bt + node];
    if(prp < 0 || prpa) {
      b = tc_bit<f_DEC>(t.P(), b);
      t.upd(b, rT, mwT);
      T[bt + node] = t;
      return b;
    }
    mxm.set(0, cm_stretch(t.P()));
    mxm.set(1, cm_stretch(prp));
    mxm.set(2, 256);
    {
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit<f_DEC>(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
    }
    mxm.upd(b, lrm, mbm);
    t.upd(b, rT, mwT);
    T[bt + node] = t;
    return b;
  }
  i64 code(const tcx &v, i64 x) {
    int k, p, nb = 0;
    u64 u;
    select(v);
    if(UNLIKELY(ENC && x < 0))
      cm_fail("internal: a negative magnitude");
    if(bit(0, ENC && x == 0))
      return 0;
    if(bit(1, ENC && x == 1))
      return 1;
    if(bit(2, ENC && x == 2))
      return 2;
    u = ENC ? (u64)(x - 3) + 1 : 1;
    if(ENC) {
      u64 t = u;
      while(t) {
        nb++;
        t >>= 1;
      }
    }
    for(k = 0;; k++) {
      if(bit(tc_node_len(k), ENC && nb == k + 1)) {
        nb = k + 1;
        break;
      }
      if(UNLIKELY(k + 2 > TC_NBMAX))
        cm_fail("%s a value of 2^%d or more", ENC ? "internal:" : "corrupt stream:", TC_NBMAX);
    }
    {
      u64 pref = 1;
      for(p = 0; p < nb - 1; p++) {
        int mb = bitm(tc_node_man(nb, p), ENC && ((u >> (nb - 2 - p)) & 1) != 0);
        pref = pref * 2 + (u64)mb;
      }
      if(!ENC)
        u = pref;
    }
    return (i64)(u - 1) + 3;
  }
  //  The sign of a nonzero value, under the g index of the last select()
  //  and sign context sc: 1 negative.
  int sign(u32 sc, int s) {
    if(!G)
      cm_fail("internal: a family with no sign counter coded a sign");
    cm_cnt g = G[bg * (u32)ng + sc];
    s = tc_bit<f_DEC>(g.P(), ENC && s);
    g.upd(s, rG, mwG);
    G[bg * (u32)ng + sc] = g;
    return s;
  }
  i64 codes(const tcx &v, i64 x, u32 sc) {
    i64 m = code(v, ENC ? (x < 0 ? -x : x) : 0);
    if(!m)
      return 0;
    return sign(sc, x < 0) ? -m : m;
  }
};

}  // namespace h264p
