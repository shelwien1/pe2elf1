/*  The template IDX/idx2inc.pl reads alongside mvs.idx: the mvs
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/mvs_p.inc with `H1_mvs_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/mvs_h.inc.  Edit this file
    and mvs.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_mvs_ND (or H_mvs_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_mvs_make( const H_mvs_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int kind = D.kind, comp = D.comp, lst = D.lst, ma = D.ma, mb = D.mb, oth = D.oth, ps = D.ps, rf = D.rf, pi = D.pi, nr = D.nr, pd = D.pd, sp = D.sp, tv = D.tv, zd = D.zd, cq = D.cq, tq = D.tq, rt = D.rt, ea = D.ea, eb = D.eb, ta = D.ta;
  (void) kind;  (void) comp;  (void) lst;  (void) ma;  (void) mb;  (void) oth;  (void) ps;  (void) rf;  (void) pi;  (void) nr;  (void) pd;  (void) sp;  (void) tv;  (void) zd;  (void) cq;  (void) tq;  (void) rt;  (void) ea;  (void) eb;  (void) ta;

  a = 0;
  a = a*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  a = a*2 + (((comp)>0+(1-1)));
  a = a*7 + H1_mvs_a_ma[__min(7,__max(0,(ma)-(1-1)))];
  a = a*7 + H1_mvs_a_mb[__min(7,__max(0,(mb)-(1-1)))];
  a = a*5 + H1_mvs_a_oth[__min(5,__max(0,(oth)-(1-1)))];

  b = 0;
  b = b*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  b = b*2 + (((comp)>0+(1-1)));
  b = b*2 + (((lst)>0+(1-1)));
  b = b*4 + H1_mvs_b_ps[__min(3,__max(0,(ps)-(1-1)))];
  b = b*4 + H1_mvs_b_rf[__min(3,__max(0,(rf)-(1-1)))];

  c = 0;
  c = c*4 + H1_mvs_c_kind[__min(3,__max(0,(kind)-(1-1)))];
  c = c*2 + (((comp)>0+(1-1)));

  d = 0;
  d = d*2 + (((kind)>2+(1-1)));
  d = d*4 + H1_mvs_d_nr[__min(3,__max(0,(nr)-(1-1)))];

  m = 0;
  m = m*4 + H1_mvs_m_kind[__min(3,__max(0,(kind)-(1-1)))];

  t = 0;
  t = t*2 + (((kind)>2+(1-1)));
  t = t*2 + (((comp)>0+(1-1)));

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

