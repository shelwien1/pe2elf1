// model_mb.inc -- the model's macroblocks (model.inc): a macroblock's kind, flags, coded block
// pattern and dquant, which it codes around its vectors and blocks (code_mb.inc); the skp, mbt, cbp
// and qpd families.

template <int f_DEC, class LV>
struct ModelMb : ModelMv<f_DEC, LV> {
  typedef ModelMv<f_DEC, LV> B;  // (what this element takes of those below)
  using B::ENC;
  using B::cat;
  using B::mbcur;
  using B::mbref;
  using B::ref_mbs;
  using B::rowb;
  using B::mo;
  using B::rec;
  using B::est;
  using B::est_err;
  using B::est_nb;
  using B::tmk;
  using B::tmx;
  using B::tmy;
  using B::rowb_clear;
  using B::checked;
  using B::sel;
  using B::est_trust;
  using B::hdr_val;
  using B::code_mv;
  using B::code_block;
  M_FAM(skp)
  M_FAM(mbt)
  M_FAM(cbp)
  M_FAM(qpd)

  // coded block pattern, block 0 in bit 5 (the cbp family, the block its node). Contexts: the left
  // and above 8x8 blocks (in this macroblock or a neighbour), the size of the motion vector
  // differences (mvc, kNone for none), which components of the vector are sub-pel (frac, 4 for none),
  // the co-located macroblock of the last anchor VOP, the blocks coded so far, the quantiser (qs:
  // before this macroblock's dquant, which comes after the pattern), and lq: whether the left and
  // above macroblocks agree on having coded blocks (0), else 1 + whether the co-located one has some.
  // (level -9: cie, the intra estimate's AC coefficients per block; tq its trust; ctx, cem an inter
  // block's prediction's texture and its edge miss)
  int code_cbp(int cls3, rp<const MBCtx> L, rp<const MBCtx> A, rp<const MBCtx> R, int mvc, int frac, int qs, int lq,
               int cbp, pcInt cie, int tq, pcInt ctx, pcInt cem) {
    M_cbp_v D;
    int b, v = 0, n = 0;
    D.tq = tq;
    D.c3 = cls3;
    D.mvc = mvc;
    D.frac = frac;
    D.qs = qs;
    D.lq = lq;
    for (b = 0; b < MB_BLOCKS; b++) {
      int l, u, bit;
      if (b < MB_LUMA) {  // (b & 1: the right column, b & 2: the lower row)
        l = (b & 1) ? cbp_bit(v, b - 1) : L ? cbp_bit(L->cbp, b + 1) : kTriNone;
        u = (b & 2) ? cbp_bit(v, b - 2) : A ? cbp_bit(A->cbp, b + 2) : kTriNone;
      } else {
        l = L ? cbp_bit(L->cbp, b) : kTriNone;
        u = A ? cbp_bit(A->cbp, b) : kTriNone;
      }
      D.lu = tri2(l, u);
      D.ie = cie[b];
      D.tx = ctx[b];
      D.em = cem[b];
      D.rbit = R ? cbp_bit(R->cbp, b) : kTriNone;
      D.cnt = n;
      D.pat = v >> (MB_BLOCKS - b) & kPatMask;
      sel(f_cbp, D);
      bit = f_cbp.bit(b, ENC && cbp_bit(cbp, b));
      v |= bit << (MB_BLOCKS - 1 - b);
      n += bit;
    }
    return v;
  }

  // neighbouring blocks' coefficient counts (CAVLC's nC)
  static int nz_ctx(const MBCtx& me, rp<const MBCtx> L, rp<const MBCtx> A, int b) {
    int na, nb, m;
    if (b < MB_LUMA) {
      na = (b & 1) ? me.nz[b - 1] : L ? L->nz[b + 1] : -1;
      nb = (b & 2) ? me.nz[b - 2] : A ? A->nz[b + 2] : -1;
    } else {
      na = L ? L->nz[b] : -1;
      nb = A ? A->nz[b] : -1;
    }
    if (na < 0 && nb < 0) return 0;
    m = na < 0 ? nb : nb < 0 ? na : (na + nb + 1) >> 1;
    return 1 + cap(m, kCap);  // (0 none)
  }

  // the macroblock's kind and flags: a decision of the mbt family, its node, with the variables of
  // the neighbours of the kind it asks of
  int mbt_bit(int node, M_mbt_v& D, int b) {
    sel(f_mbt, D);
    return f_mbt.bit(node, ENC && b);
  }
  // how many of the macroblocks are of the kind k
  static int count_of(int k, rp<const MBCtx> a, rp<const MBCtx> b) { return (a && a->cls == k) + (b && b->cls == k); }

  // the macroblocks around the one being coded: left, above, above left, above right (0: none, or
  // not coded), and the co-located one of the last anchor VOP (0: none)
  struct Around {
    rp<const MBCtx> L, A, AL, AR, R;  // (never the macroblock coded, which alone is written)
  };
  // a kind decision's variables of the macroblocks around (mbt's and skp's cl, c4, rt): how many of
  // the left and above, and of all four, are of kind k; how the co-located one is (rt_of(), tri_of())
  template <class V>
  static void around(V& D, const Around& n, int k, int rt) {
    D.cl = count_of(k, n.L, n.A);
    D.c4 = D.cl + count_of(k, n.AL, n.AR);
    D.rt = rt;
  }
  // the co-located macroblock R against kind k: RT_SAME, RT_NEXT (of kind next, the one beside k; k
  // where there is none), RT_OTHER, RT_NONE (none); or whether it is of kind k (kTriNone: none)
  static int rt_of(rp<const MBCtx> R, int k, int next) {
    return !R ? RT_NONE : R->cls == k ? RT_SAME : R->cls == next ? RT_NEXT : RT_OTHER;
  }
  static int tri_of(rp<const MBCtx> R, int k) { return R ? R->cls == k : kTriNone; }

  // (level -9) skipping's variables, zt and ze (0 none)
  void skip_vars(pInt zt, pInt ze) {
    int a, b;
    if (mo.pict_type == PT_B) mo.direct(0, 0);  // (what skip_feat() predicts by)
    rec.skip_feat(&a, &b);
    *zt = a < 0 ? 0 : qlog1(a);
    *ze = b < 0 ? 0 : qlog1(b);
  }

  // a macroblock with no vectors or blocks of its own ends (skipped; a B-VOP's direct one without a
  // vector): of class cls, its blocks none, its vectors the motion state's, its prediction in the picture
  void mb_bare(MBCtx& me, const MB& m, int cls, int mb_x) {
    me.cls = (U8)cls;
    rowb_clear(mb_x);
    mo.kind(m);
    if (m.mb == MODB_NONE) mo.direct(0, 0);
    mo.mb_end();
    if (LV::recon && rec.on) rec.mb_inter(), rec.mb_end();
  }

#include "code_mb.inc"  // code_mb(): one macroblock record; in decoding mode m is filled in
};
