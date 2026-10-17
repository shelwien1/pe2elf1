/*  The template IDX/idx2inc.pl reads alongside cbp.idx: the cbp
    family's index builder and the tables its indices address.  Copied
    through into MOD/cbp_p.inc with `H_cbp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/cbp_h.inc.  Edit this file and
    cbp.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_cbp_ND (or H_cbp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_cbp_make( const H_cbp_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int inc = D.inc, pc = D.pc, cls = D.cls, col = D.col, q6 = D.q6, st = D.st, en = D.en, td = D.td, pg = D.pg;
  (void) inc;  (void) pc;  (void) cls;  (void) col;  (void) q6;  (void) st;  (void) en;  (void) td;  (void) pg;

  a = 0;
  a = a*3 + ((inc>0+(1-1))+(inc>2+(1-1)));
  a = a*5 + H_cbp_a_pc[__min(4,__max(0,pc-(1-1)))];
  a = a*9 + H_cbp_a_cls[__min(8,__max(0,cls-(1-1)))];
  a = a*10 + H_cbp_a_st[__min(9,__max(0,st-(1-1)))];

  b = 0;
  b = b*4 + H_cbp_b_inc[__min(3,__max(0,inc-(1-1)))];
  b = b*3 + ((col>0+(1-1))+(col>1+(1-1)));
  b = b*9 + H_cbp_b_q6[__min(8,__max(0,q6-(1-1)))];

  c = 0;
  c = c*9 + H_cbp_c_cls[__min(8,__max(0,cls-(1-1)))];
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*3 + ((pc>0+(1-1))+(pc>2+(1-1)));
  c = c*17 + H_cbp_c_pg[__min(16,__max(0,pg-(1-1)))];

  d = 0;
  d = d*4 + H_cbp_d_inc[__min(3,__max(0,inc-(1-1)))];
  d = d*15 + H_cbp_d_en[__min(14,__max(0,en-(1-1)))];
  d = d*9 + H_cbp_d_cls[__min(8,__max(0,cls-(1-1)))];

  s = 0;
  s = s*4 + H_cbp_s_inc[__min(3,__max(0,inc-(1-1)))];
  s = s*17 + H_cbp_s_td[__min(16,__max(0,td-(1-1)))];

  f = 0;
  f = f*3 + ((cls>2+(1-1))+(cls>5+(1-1)));

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

