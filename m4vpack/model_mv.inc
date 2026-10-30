// model_mv.inc -- the model's vectors (model.inc): each one's difference as coded, against the
// prediction the motion state makes (motion.inc); the mvd and mvg families.

template <int f_DEC, class LV>
struct ModelMv : ModelRes<f_DEC, LV> {
  typedef ModelRes<f_DEC, LV> B;  // (what this element takes of those below)
  using B::ENC;
  using B::tmk;
  using B::tmx;
  using B::tmy;
  using B::checked;
  using B::sel;
  M_FAM(mvd)
  M_FAM(mvg)

  // A motion vector difference as coded, like CABAC's mvd: its magnitude in units of the VLC code
  // (so f_code doesn't change its statistics; the mvd family), its sign (mvg), the f_code residual
  // bits (mvd). Contexts: the neighbours' magnitudes (their sum, and each), the other component
  // (oth: of this vector for y, of the previous one for x, kNone for none), the co-located macroblock
  // of the last anchor VOP, and d: the difference that would give this vector the co-located one's
  // value (dok 0: none), whose magnitude is the magnitude's hint; with the decoder, td the difference
  // that would give it the vector the template matches best (tok 0: none), the hint then.
  int code_mvd(int kind, int comp, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, int oth, int dok, int d,
               int sp, int v, int* q, int tok, int td) {
    const int shift = fcode - 1, a = v < 0 ? -v : v;
    const int ad = (d < 0 ? -d : d) - 1, dq = d ? (ad >> shift) + 1 : 0;
    const int ds = !dok || !d ? 0 : d > 0 ? 1 : 2;
    const int tq = td ? (((td < 0 ? -td : td) - 1) >> shift) + 1 : 0, ts = !tok || !td ? 0 : td > 0 ? 1 : 2;
    M_mvd_v D;
    tcx X;
    int neg, r, i;
    i64 x;
    D.kind = kind;
    D.comp = comp;
    D.ns = mvq((L ? L->mva[comp] : 0) + (A ? A->mva[comp] : 0));
    D.oth = oth;
    D.rs = R ? mvq(R->mva[comp]) : kNone;
    D.dz = dok ? mvq(dq) : kNone;
    D.sp = sp;
    D.lq = L ? mvq(L->mva[comp]) : kNone;
    D.aq = A ? mvq(A->mva[comp]) : kNone;
    D.ds = ds;
    D.fc = shift;
    D.rbi = D.rsame = 0;
    D.tz = tok ? mvq(tq) : kNone;
    D.ts = ts;
    f_mvd.make(D, X);
    f_mvd.hint = ds ? dq : tok ? tq : -1;  // (the magnitude that repeats the co-located vector, or the template's)
    x = checked(f_mvd.code(X, ENC ? (a ? ((a - 1) >> shift) + 1 : 0) : 0), 0, MV_VLC_MAX);
    f_mvd.hint = -1;
    *q = (int)x;
    if (!x) return 0;
    {
      M_mvg_v G;
      G.kind = kind;
      G.comp = comp;
      G.ls = L ? L->mvs[comp] : 0;
      G.as = A ? A->mvs[comp] : 0;
      G.ds = ds;
      G.dz = D.dz;
      G.mag = mvq((int)x);
      G.hm = !ds ? HM_NONE : x < dq ? HM_BELOW : x == dq ? HM_EQUAL : HM_ABOVE;
      G.ts = ts;
      G.thm = !ts ? HM_NONE : x < tq ? HM_BELOW : x == tq ? HM_EQUAL : HM_ABOVE;
      sel(f_mvg, G);
      neg = f_mvg.bit(0, ENC && v < 0);
    }
    for (i = shift - 1, r = 0; i >= 0; i--) {  // residual bits, predicted by d's while they agree so far
      const int same = ds == 1 + neg && x == dq && ad >> (i + 1) == ((int)(x - 1) << (shift - i - 1) | r);
      D.rbi = i;
      D.rsame = same ? 1 + ((ad >> i) & 1) : 0;
      sel(f_mvd, D);
      r |= f_mvd.bit(TC_NODE + i, ENC && ((a - 1) >> i) & 1) << i;
    }
    x = ((x - 1) << shift | r) + 1;
    return neg ? -(int)x : (int)x;
  }

  // one vector (x, y) of kind kind, predicted by (px, py) from candidates with spreads (sx, sy) (-1:
  // none), with the co-located vector (tx, ty) if tok; tm: the template's vector (tmk[], tmx[], tmy[])
  // of that direction, or -1
  void code_mv(MB* m, int* mvi, int kind, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R, MBCtx* me, int* sumq,
               int px, int py, int sx, int sy, int tok, int tx, int ty, int* mx, int* my, int tm = -1) {
    const int first = *mvi == 0, mk = tm >= 0 && tmk[tm];
    int v, q;
    v = code_mvd(kind, 0, fcode, L, A, R, first ? kNone : mvq(me->mva[0]), tok, mv_wrap(tx - px, fcode),
                 mv_spread(sx, fcode), m->mv[*mvi], &q, mk, mk ? mv_wrap(tmx[tm] - px, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *mx = Motion::vector(px, v, fcode);
    if (first) { me->mva[0] = (U8)q; me->mvs[0] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
    v = code_mvd(kind, 1, fcode, L, A, R, mvq(q), tok, mv_wrap(ty - py, fcode), mv_spread(sy, fcode),
                 m->mv[*mvi], &q, mk, mk ? mv_wrap(tmy[tm] - py, fcode) : 0);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    *my = Motion::vector(py, v, fcode);
    if (first) { me->mva[1] = (U8)q; me->mvs[1] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
  }
};
