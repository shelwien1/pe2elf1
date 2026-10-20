/*  The template IDX/idx2inc.pl reads alongside cbp.idx: the cbp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l7/cbp_p.inc with `H7_cbp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l7/cbp_h.inc.  Edit this file
    and cbp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -7: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_cbp_ND (or H_cbp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H7_cbp_make( const H_cbp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int inc = D.inc, pc = D.pc, cls = D.cls, col = D.col, q6 = D.q6, st = D.st, en = D.en, td = D.td, pg = D.pg, tp = D.tp, em = D.em, pq = D.pq;
  (void) inc;  (void) pc;  (void) cls;  (void) col;  (void) q6;  (void) st;  (void) en;  (void) td;  (void) pg;  (void) tp;  (void) em;  (void) pq;

  a = 0;
  a = a*4 + H7_cbp_a_pc[__min(4,__max(0,pc-(1-1)))];
  a = a*3 + ((cls>2+(1-1))+(cls>5+(1-1)));
  a = a*4 + H7_cbp_a_st[__min(9,__max(0,st-(1-1)))];

  b = 0;
  b = b*6 + H7_cbp_b_col[__min(12,__max(0,col-(1-1)))];
  b = b*2 + ((q6>0+(1-1)));
  b = b*4 + H7_cbp_b_pq[__min(6,__max(0,pq-(1-1)))];

  c = 0;
  c = c*4 + H7_cbp_c_cls[__min(8,__max(0,cls-(1-1)))];
  c = c*7 + H7_cbp_c_col[__min(12,__max(0,col-(1-1)))];
  c = c*3 + ((pc>0+(1-1))+(pc>2+(1-1)));
  c = c*8 + H7_cbp_c_pg[__min(16,__max(0,pg-(1-1)))];

  d = 0;
  d = d*9 + H7_cbp_d_em[__min(16,__max(0,em-(1-1)))];
  d = d*5 + H7_cbp_d_cls[__min(8,__max(0,cls-(1-1)))];

  e = 0;
  e = e*5 + H7_cbp_e_pg[__min(16,__max(0,pg-(1-1)))];
  e = e*2 + ((cls>2+(1-1)));
  e = e*8 + H7_cbp_e_tp[__min(16,__max(0,tp-(1-1)))];

  h = 0;
  h = h*4 + H7_cbp_h_td[__min(16,__max(0,td-(1-1)))];
  h = h*6 + H7_cbp_h_en[__min(14,__max(0,en-(1-1)))];
  h = h*4 + H7_cbp_h_q6[__min(8,__max(0,q6-(1-1)))];

  i = 0;
  i = i*4 + H7_cbp_i_inc[__min(3,__max(0,inc-(1-1)))];
  i = i*5 + H7_cbp_i_pq[__min(6,__max(0,pq-(1-1)))];
  i = i*6 + H7_cbp_i_q6[__min(8,__max(0,q6-(1-1)))];

  j = 0;
  j = j*3 + ((inc>1+(1-1))+(inc>2+(1-1)));
  j = j*6 + H7_cbp_j_tp[__min(16,__max(0,tp-(1-1)))];
  j = j*8 + H7_cbp_j_em[__min(16,__max(0,em-(1-1)))];
  j = j*4 + H7_cbp_j_st[__min(9,__max(0,st-(1-1)))];

  k = 0;
  k = k*5 + H7_cbp_k_pg[__min(16,__max(0,pg-(1-1)))];
  k = k*3 + ((col>1+(1-1))+(col>7+(1-1)));
  k = k*3 + ((cls>2+(1-1))+(cls>5+(1-1)));

  l = 0;
  l = l*3 + ((inc>0+(1-1))+(inc>2+(1-1)));
  l = l*5 + H7_cbp_l_pc[__min(4,__max(0,pc-(1-1)))];
  l = l*5 + H7_cbp_l_en[__min(14,__max(0,en-(1-1)))];
  l = l*4 + H7_cbp_l_q6[__min(8,__max(0,q6-(1-1)))];

  s = 0;
  s = s*2 + ((inc>0+(1-1)));
  s = s*6 + H7_cbp_s_tp[__min(16,__max(0,tp-(1-1)))];
  s = s*4 + H7_cbp_s_pq[__min(6,__max(0,pq-(1-1)))];

  m = 0;

  w = 0;
  w = w*2 + ((inc>2+(1-1)));
  w = w*7 + H7_cbp_w_em[__min(16,__max(0,em-(1-1)))];
  w = w*4 + H7_cbp_w_cls[__min(8,__max(0,cls-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

