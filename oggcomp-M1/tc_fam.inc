static INLINE int tc_bit(int p, int bit) {
  uint b = (uint)bit;
  if(tc_verbose && tc_enc)
    tc_bits[tc_stage] -= log2(bit ? 1.0 - p / (double)CM_PONE : p / (double)CM_PONE);
  if(tc_enc)
    rce.rc_Process((uint)(p << 3), b);
  else
    rcd.rc_Process((uint)(p << 3), b);
  return (int)b;
}
struct tc_fam {
  cm_cnt *A, *B, *C, *D, *G, *T;
  i16 *S, *S2;
  i32 *W;
  cm_apm ap, ap2;
  cm_mix<TC_MIXN> mx;
  cm_mix<3> mxm;
  tc_pcur pc;
  int prp;
  sz ba, bb, bc, bd;
  u32 bs, bf, bm, bt, bn, bg;
  int rA, rB, rC, rD, rT, rG;
  int mwA, mwB, mwC, mwD, mwT, mwG;
  int rS1, rS2;
  int lr, mb, lrm, mbm;
  int bw;
  int ng;
  int nd, nm;
  void wire(cm_cnt *a, sz va, cm_cnt *b, sz vb, cm_cnt *c, sz vc, cm_cnt *d, sz vd, cm_cnt *t, sz vt, i16 *s, u8 *sc, sz vs, i16 *s2, u8 *sc2, sz vf, i32 *w, sz vm, cm_cnt *g, sz vg, sz vgi, i32 *wm, sz vn, u8 *wc, u8 *wmc, int ra, int rb, int rc_, int rd, int rt, int rg, int mwa, int mwb, int mwc, int mwd, int mwtt, int mwg, int rs1, int rs2, int lrate, int mbt, int lrm2, int mbm2, int bwt, int qqs, int qqf, int ndp, int nmp) {
    A = a;
    B = b;
    C = c;
    D = d;
    T = t;
    S = s;
    S2 = s2;
    W = w;
    G = g;
    ng = (int)vg;
    nd = ndp;
    nm = nmp;
    tc_vfits("counter A", va, nd);
    tc_vfits("counter B", vb, nd);
    tc_vfits("counter C", vc, nd);
    tc_vfits("counter D", vd, nd);
    tc_vfits("counter T", vt, nm);
    if(G)
      tc_vfits("counter G", vg, vgi);
    tc_ifits("APM 1", vs, (sz)nd * qqs);
    tc_ifits("APM 2", vf, (sz)nd * qqf);
    tc_ifits("mixer", vm, (sz)nd * TC_MIXN);
    tc_ifits("mantissa mixer", vn, (sz)nm * 3);
    (void)sc;
    (void)sc2;
    (void)wc;
    (void)wmc;
    ap.init(S, sc, vs * nd, qqs);
    ap2.init(S2, sc2, vf * nd, qqf);
    mx.init(W, wc, vm * nd);
    mxm.init(wm, wmc, vn * nm);
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rT = tc_clamp(rt, 1, CM_TMAX);
    rG = tc_clamp(rg, 1, CM_TMAX);
    mwA = tc_clamp(mwa * 64, 0, CM_CONE / 4);
    mwB = tc_clamp(mwb * 64, 0, CM_CONE / 4);
    mwC = tc_clamp(mwc * 64, 0, CM_CONE / 4);
    mwD = tc_clamp(mwd * 64, 0, CM_CONE / 4);
    mwT = tc_clamp(mwtt * 64, 0, CM_CONE / 4);
    mwG = tc_clamp(mwg * 64, 0, CM_CONE / 4);
    rS1 = cm_apm::limit(tc_clamp(rs1, 1, 15));
    rS2 = cm_apm::limit(tc_clamp(rs2, 1, 15));
    lr = tc_clamp(lrate, 1, 64);
    mb = tc_clamp(mbt, 0, 255);
    lrm = tc_clamp(lrm2, 1, 64);
    mbm = tc_clamp(mbm2, 0, 255);
    bw = tc_clamp(bwt, 0, 16);
    pc.off();
    prp = -1;
  }
  INLINE void select(const tcx &x) {
    ba = (sz)x.a * nd;
    bb = (sz)x.b * nd;
    bc = (sz)x.c * nd;
    bd = (sz)x.d * nd;
    bs = (u32)x.s * nd;
    bf = (u32)x.f * nd;
    bm = (u32)x.m * nd;
    bt = (u32)x.t * nm;
    bn = (u32)x.n * nm;
    bg = (u32)x.g;
    __builtin_prefetch(A + ba);
    __builtin_prefetch(B + bb);
    __builtin_prefetch(C + bc);
    __builtin_prefetch(D + bd);
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
    int pa = a.P();
    int p1 = ap.pp(pa, bs + (u32)node);
    if(p1 > CM_PONE - 1)
      p1 = CM_PONE - 1;
    mx.set(0, cm_stretch(p1));
    mx.set(1, cm_stretch(c.P()));
    mx.set(2, cm_stretch(e.P()));
    mx.set(3, cm_stretch(f.P()));
    mx.set(4, cm_stretch(pa));
    mx.set(5, prp >= 0 ? cm_stretch(prp) : 0);
    mx.set(6, 256);
    {
      int pm = mx.mix(bm + (u32)node);
      int pf = ap2.pp(pm, bf + (u32)node);
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b);
    }
    mx.upd(b, lr, mb);
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
    return b;
  }
  INLINE int bitm(int node, int b) {
    cm_cnt t = T[bt + node];
    if(prp < 0) {
      b = tc_bit(t.P(), b);
      t.upd(b, rT, mwT);
      T[bt + node] = t;
      return b;
    }
    mxm.set(0, cm_stretch(t.P()));
    mxm.set(1, cm_stretch(prp));
    mxm.set(2, 256);
    {
      int pm = mxm.mix(bn + (u32)node);
      b = tc_bit(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b);
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
    if(UNLIKELY(tc_enc && x < 0))
      FATAL("negative magnitude");
    prp = tcp_head(pc, 0);
    if(bit(0, tc_enc && x == 0)) {
      prp = -1;
      return 0;
    }
    prp = tcp_head(pc, 1);
    if(bit(1, tc_enc && x == 1)) {
      prp = -1;
      return 1;
    }
    prp = tcp_head(pc, 2);
    if(bit(2, tc_enc && x == 2)) {
      prp = -1;
      return 2;
    }
    u = tc_enc ? (u64)(x - 3) + 1 : 1;
    if(tc_enc) {
      u64 t = u;
      while(t) {
        nb++;
        t >>= 1;
      }
    }
    for(k = 0;; k++) {
      prp = tcp_len(pc, k);
      if(bit(tc_node_len(k), tc_enc && nb == k + 1)) {
        nb = k + 1;
        break;
      }
      if(UNLIKELY(k + 2 > TC_NBMAX))
        FATAL("%s: a value of 2^%d or more", tc_enc ? "this stream holds" : "coded stream", TC_NBMAX);
    }
    {
      u64 pref = 1;
      for(p = 0; p < nb - 1; p++) {
        int mb;
        prp = tcp_man(pc, nb, p, pref);
        mb = bitm(tc_node_man(nb, p), tc_enc && ((u >> (nb - 2 - p)) & 1) != 0);
        pref = pref * 2 + (u64)mb;
      }
      prp = -1;
      if(!tc_enc)
        u = pref;
    }
    return (i64)(u - 1) + 3;
  }
  i64 codes(const tcx &v, i64 x, u32 sc) {
    i64 m = code(v, tc_enc ? (x < 0 ? -x : x) : 0);
    int s;
    if(!m)
      return 0;
    if(G) {
      cm_cnt g = G[bg * (u32)ng + sc];
      s = tc_bit(g.P(), tc_enc && x < 0);
      g.upd(s, rG, mwG);
      G[bg * (u32)ng + sc] = g;
    } else {
      FATAL_CODE(OGC_EXIT_IO, "a family with no sign counter coded a sign");
    }
    return s ? -m : m;
  }
};
static tc_fam fam_dig, fam_sgn, fam_flr, fam_cls, fam_aux, fam_hdr;
static i64 tc_last[F_N], tc_last2[F_N];
static i64 tc_auxc(int fld, i32 c, i64 x, int sgn) {
  tcx v;
  i64 r;
  tc_make_aux(fld, c, tc_qlog(tc_last[fld]), v);
  r = sgn ? fam_aux.codes(v, x, (u32)fld) : fam_aux.code(v, x);
  tc_last2[fld] = tc_last[fld];
  tc_last[fld] = r;
  return r;
}
static i64 tc_aux(int fld, i64 x) { return tc_auxc(fld, tc_qlog(tc_last2[fld]), x, 0); }
