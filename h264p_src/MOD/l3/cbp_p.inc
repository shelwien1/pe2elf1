/*  The template IDX/idx2inc.pl reads alongside cbp.idx: the cbp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l3/cbp_p.inc with `H3_cbp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l3/cbp_h.inc.  Edit this file
    and cbp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -3: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_cbp_ND (or H_cbp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H3_cbp_make( const H_cbp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int inc = D.inc, pc = D.pc, cls = D.cls, col = D.col, q6 = D.q6, st = D.st, en = D.en, td = D.td, pg = D.pg, tp = D.tp, em = D.em, pq = D.pq;
  (void) inc;  (void) pc;  (void) cls;  (void) col;  (void) q6;  (void) st;  (void) en;  (void) td;  (void) pg;  (void) tp;  (void) em;  (void) pq;

  a = 0;
  a = a*3 + ((inc>0+(1-1))+(inc>2+(1-1)));
  a = a*4 + H3_cbp_a_pc[__min(4,__max(0,pc-(1-1)))];
  a = a*9 + H3_cbp_a_cls[__min(8,__max(0,cls-(1-1)))];
  a = a*10 + H3_cbp_a_st[__min(9,__max(0,st-(1-1)))];

  b = 0;
  b = b*13 + H3_cbp_b_col[__min(12,__max(0,col-(1-1)))];
  b = b*4 + H3_cbp_b_q6[__min(8,__max(0,q6-(1-1)))];

  c = 0;
  c = c*9 + H3_cbp_c_cls[__min(8,__max(0,cls-(1-1)))];
  c = c*13 + H3_cbp_c_col[__min(12,__max(0,col-(1-1)))];
  c = c*3 + ((pc>0+(1-1))+(pc>2+(1-1)));

  d = 0;
  d = d*4 + H3_cbp_d_inc[__min(3,__max(0,inc-(1-1)))];
  d = d*9 + H3_cbp_d_cls[__min(8,__max(0,cls-(1-1)))];

  e = 0;
  e = e*3 + ((inc>0+(1-1))+(inc>2+(1-1)));
  e = e*9 + H3_cbp_e_cls[__min(8,__max(0,cls-(1-1)))];

  h = 0;
  h = h*3 + ((inc>0+(1-1))+(inc>2+(1-1)));
  h = h*9 + H3_cbp_h_q6[__min(8,__max(0,q6-(1-1)))];

  i = 0;
  i = i*4 + H3_cbp_i_inc[__min(3,__max(0,inc-(1-1)))];
  i = i*9 + H3_cbp_i_cls[__min(8,__max(0,cls-(1-1)))];
  i = i*9 + H3_cbp_i_q6[__min(8,__max(0,q6-(1-1)))];

  j = 0;
  j = j*4 + H3_cbp_j_inc[__min(3,__max(0,inc-(1-1)))];
  j = j*10 + H3_cbp_j_st[__min(9,__max(0,st-(1-1)))];

  k = 0;
  k = k*4 + H3_cbp_k_inc[__min(3,__max(0,inc-(1-1)))];
  k = k*13 + H3_cbp_k_col[__min(12,__max(0,col-(1-1)))];
  k = k*9 + H3_cbp_k_cls[__min(8,__max(0,cls-(1-1)))];

  l = 0;
  l = l*4 + H3_cbp_l_inc[__min(3,__max(0,inc-(1-1)))];
  l = l*3 + ((pc>0+(1-1))+(pc>1+(1-1)));
  l = l*9 + H3_cbp_l_q6[__min(8,__max(0,q6-(1-1)))];

  s = 0;
  s = s*4 + H3_cbp_s_inc[__min(3,__max(0,inc-(1-1)))];

  m = 0;

  w = 0;
  w = w*4 + H3_cbp_w_inc[__min(3,__max(0,inc-(1-1)))];
  w = w*3 + ((cls>2+(1-1))+(cls>5+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

