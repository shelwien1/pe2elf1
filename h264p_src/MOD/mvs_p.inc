/*  The template IDX/idx2inc.pl reads alongside mvs.idx: the mvs
    family's index builder and the tables its indices address.  Copied
    through into MOD/mvs_p.inc with `H_mvs_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/mvs_h.inc.  Edit this file and
    mvs.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_mvs_ND (or H_mvs_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_mvs_make( const H_mvs_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int kind = D.kind, comp = D.comp, lst = D.lst, ma = D.ma, mb = D.mb, oth = D.oth, ps = D.ps, rf = D.rf, pi = D.pi, nr = D.nr, pd = D.pd, sp = D.sp, tv = D.tv, zd = D.zd, cq = D.cq, tq = D.tq, rt = D.rt;
  (void) kind;  (void) comp;  (void) lst;  (void) ma;  (void) mb;  (void) oth;  (void) ps;  (void) rf;  (void) pi;  (void) nr;  (void) pd;  (void) sp;  (void) tv;  (void) zd;  (void) cq;  (void) tq;  (void) rt;

  a = 0;
  a = a*3 + ((kind>1+(1-1))+(kind>2+(1-1)));
  a = a*2 + ((comp>0+(1-1)));
  a = a*5 + H_mvs_a_ma[__min(7,__max(0,ma-(1-1)))];
  a = a*6 + H_mvs_a_mb[__min(7,__max(0,mb-(1-1)))];
  a = a*6 + H_mvs_a_oth[__min(5,__max(0,oth-(1-1)))];
  a = a*7 + H_mvs_a_sp[__min(8,__max(0,sp-(1-1)))];
  a = a*15 + H_mvs_a_rt[__min(14,__max(0,rt-(1-1)))];

  b = 0;
  b = b*4 + H_mvs_b_kind[__min(3,__max(0,kind-(1-1)))];
  b = b*2 + ((comp>0+(1-1)));
  b = b*2 + ((lst>0+(1-1)));
  b = b*4 + H_mvs_b_ps[__min(3,__max(0,ps-(1-1)))];
  b = b*4 + H_mvs_b_rf[__min(3,__max(0,rf-(1-1)))];
  b = b*13 + H_mvs_b_zd[__min(12,__max(0,zd-(1-1)))];

  c = 0;
  c = c*4 + H_mvs_c_kind[__min(3,__max(0,kind-(1-1)))];
  c = c*2 + ((comp>0+(1-1)));
  c = c*7 + H_mvs_c_pd[__min(8,__max(0,pd-(1-1)))];
  c = c*9 + H_mvs_c_sp[__min(8,__max(0,sp-(1-1)))];
  c = c*13 + H_mvs_c_zd[__min(12,__max(0,zd-(1-1)))];

  d = 0;
  d = d*4 + H_mvs_d_kind[__min(3,__max(0,kind-(1-1)))];
  d = d*2 + ((comp>0+(1-1)));
  d = d*9 + H_mvs_d_pd[__min(8,__max(0,pd-(1-1)))];
  d = d*8 + H_mvs_d_ma[__min(7,__max(0,ma-(1-1)))];
  d = d*4 + H_mvs_d_nr[__min(3,__max(0,nr-(1-1)))];

  e = 0;
  e = e*3 + ((kind>1+(1-1))+(kind>2+(1-1)));
  e = e*14 + H_mvs_e_tv[__min(13,__max(0,tv-(1-1)))];
  e = e*9 + H_mvs_e_sp[__min(8,__max(0,sp-(1-1)))];
  e = e*15 + H_mvs_e_rt[__min(14,__max(0,rt-(1-1)))];

  h = 0;
  h = h*2 + ((kind>1+(1-1)));
  h = h*2 + ((comp>0+(1-1)));
  h = h*14 + H_mvs_h_tv[__min(13,__max(0,tv-(1-1)))];
  h = h*6 + H_mvs_h_oth[__min(5,__max(0,oth-(1-1)))];
  h = h*13 + H_mvs_h_tq[__min(12,__max(0,tq-(1-1)))];

  i = 0;
  i = i*4 + H_mvs_i_kind[__min(3,__max(0,kind-(1-1)))];
  i = i*2 + ((comp>0+(1-1)));
  i = i*14 + H_mvs_i_tv[__min(13,__max(0,tv-(1-1)))];
  i = i*13 + H_mvs_i_tq[__min(12,__max(0,tq-(1-1)))];
  i = i*13 + H_mvs_i_zd[__min(12,__max(0,zd-(1-1)))];

  j = 0;
  j = j*3 + ((kind>0+(1-1))+(kind>2+(1-1)));
  j = j*2 + ((comp>0+(1-1)));
  j = j*8 + H_mvs_j_ma[__min(7,__max(0,ma-(1-1)))];
  j = j*3 + ((mb>1+(1-1))+(mb>4+(1-1)));
  j = j*10 + H_mvs_j_cq[__min(9,__max(0,cq-(1-1)))];

  k = 0;
  k = k*4 + H_mvs_k_kind[__min(3,__max(0,kind-(1-1)))];
  k = k*2 + ((comp>0+(1-1)));
  k = k*7 + H_mvs_k_pd[__min(8,__max(0,pd-(1-1)))];
  k = k*4 + H_mvs_k_sp[__min(8,__max(0,sp-(1-1)))];
  k = k*4 + H_mvs_k_ps[__min(3,__max(0,ps-(1-1)))];

  l = 0;
  l = l*4 + H_mvs_l_kind[__min(3,__max(0,kind-(1-1)))];
  l = l*2 + ((comp>0+(1-1)));
  l = l*6 + H_mvs_l_oth[__min(5,__max(0,oth-(1-1)))];
  l = l*3 + ((tv>5+(1-1))+(tv>11+(1-1)));
  l = l*4 + H_mvs_l_rf[__min(3,__max(0,rf-(1-1)))];

  s = 0;
  s = s*4 + H_mvs_s_kind[__min(3,__max(0,kind-(1-1)))];
  s = s*2 + ((comp>0+(1-1)));
  s = s*14 + H_mvs_s_tv[__min(13,__max(0,tv-(1-1)))];

  f = 0;
  f = f*4 + H_mvs_f_kind[__min(3,__max(0,kind-(1-1)))];
  f = f*2 + ((comp>0+(1-1)));
  f = f*9 + H_mvs_f_sp[__min(8,__max(0,sp-(1-1)))];
  f = f*10 + H_mvs_f_cq[__min(9,__max(0,cq-(1-1)))];

  m = 0;
  m = m*4 + H_mvs_m_kind[__min(3,__max(0,kind-(1-1)))];
  m = m*6 + H_mvs_m_pd[__min(5,__max(0,pd-(1-1)))];

  w = 0;
  w = w*4 + H_mvs_w_kind[__min(3,__max(0,kind-(1-1)))];
  w = w*4 + H_mvs_w_pd[__min(8,__max(0,pd-(1-1)))];
  w = w*8 + H_mvs_w_ma[__min(7,__max(0,ma-(1-1)))];
  w = w*4 + H_mvs_w_nr[__min(3,__max(0,nr-(1-1)))];

  t = 0;
  t = t*4 + H_mvs_t_kind[__min(3,__max(0,kind-(1-1)))];
  t = t*2 + ((comp>0+(1-1)));

  n = 0;
  n = n*4 + H_mvs_n_kind[__min(3,__max(0,kind-(1-1)))];

  g = 0;
  g = g*4 + H_mvs_g_kind[__min(3,__max(0,kind-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

