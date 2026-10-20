/*  The template IDX/idx2inc.pl reads alongside mbt.idx: the mbt
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/mbt_p.inc with `H9_mbt_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/mbt_h.inc.  Edit this file
    and mbt.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_mbt_ND (or H_mbt_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int st = D.st, ca = D.ca, cb = D.cb, cc = D.cc, nsk = D.nsk, en = D.en, td = D.td, sm = D.sm, tm = D.tm, tb = D.tb, dh = D.dh, bd = D.bd, qc = D.qc, im = D.im, ig = D.ig, c1 = D.c1, tx = D.tx;
  (void) st;  (void) ca;  (void) cb;  (void) cc;  (void) nsk;  (void) en;  (void) td;  (void) sm;  (void) tm;  (void) tb;  (void) dh;  (void) bd;  (void) qc;  (void) im;  (void) ig;  (void) c1;  (void) tx;

  a = 0;
  a = a*2 + ((st>4+(1-1)));
  a = a*2 + ((ca>0+(1-1)));
  a = a*4 + H9_mbt_a_cb[__min(8,__max(0,cb-(1-1)))];
  a = a*6 + H9_mbt_a_en[__min(14,__max(0,en-(1-1)))];

  b = 0;
  b = b*3 + ((st>1+(1-1))+(st>2+(1-1)));
  b = b*4 + H9_mbt_b_cc[__min(8,__max(0,cc-(1-1)))];
  b = b*3 + ((td>1+(1-1))+(td>15+(1-1)));
  b = b*7 + H9_mbt_b_c1[__min(9,__max(0,c1-(1-1)))];

  c = 0;
  c = c*2 + ((ca>0+(1-1)));
  c = c*4 + H9_mbt_c_im[__min(4,__max(0,im-(1-1)))];
  c = c*4 + H9_mbt_c_ig[__min(12,__max(0,ig-(1-1)))];

  d = 0;
  d = d*3 + ((st>1+(1-1))+(st>2+(1-1)));
  d = d*3 + ((sm>13+(1-1))+(sm>15+(1-1)));
  d = d*6 + H9_mbt_d_tm[__min(16,__max(0,tm-(1-1)))];

  e = 0;
  e = e*3 + ((st>1+(1-1))+(st>4+(1-1)));
  e = e*6 + H9_mbt_e_ca[__min(8,__max(0,ca-(1-1)))];
  e = e*2 + ((td>8+(1-1)));
  e = e*8 + H9_mbt_e_sm[__min(16,__max(0,sm-(1-1)))];
  e = e*7 + H9_mbt_e_tx[__min(14,__max(0,tx-(1-1)))];

  h = 0;
  h = h*2 + ((st>2+(1-1)));
  h = h*10 + H9_mbt_h_dh[__min(9,__max(0,dh-(1-1)))];
  h = h*5 + H9_mbt_h_tb[__min(16,__max(0,tb-(1-1)))];
  h = h*3 + ((nsk>1+(1-1))+(nsk>3+(1-1)));
  h = h*10 + H9_mbt_h_bd[__min(16,__max(0,bd-(1-1)))];

  i = 0;
  i = i*3 + ((st>1+(1-1))+(st>2+(1-1)));
  i = i*6 + H9_mbt_i_tb[__min(16,__max(0,tb-(1-1)))];
  i = i*8 + H9_mbt_i_en[__min(14,__max(0,en-(1-1)))];
  i = i*3 + ((ca>2+(1-1))+(ca>6+(1-1)));

  j = 0;
  j = j*8 + H9_mbt_j_sm[__min(16,__max(0,sm-(1-1)))];
  j = j*10 + H9_mbt_j_dh[__min(9,__max(0,dh-(1-1)))];
  j = j*7 + H9_mbt_j_cb[__min(8,__max(0,cb-(1-1)))];

  k = 0;
  k = k*8 + H9_mbt_k_tm[__min(16,__max(0,tm-(1-1)))];
  k = k*3 + ((nsk>0+(1-1))+(nsk>1+(1-1)));
  k = k*6 + H9_mbt_k_cc[__min(8,__max(0,cc-(1-1)))];

  l = 0;
  l = l*7 + H9_mbt_l_bd[__min(16,__max(0,bd-(1-1)))];
  l = l*6 + H9_mbt_l_ca[__min(8,__max(0,ca-(1-1)))];
  l = l*8 + H9_mbt_l_cb[__min(8,__max(0,cb-(1-1)))];

  s = 0;
  s = s*3 + ((tb>5+(1-1))+(tb>9+(1-1)));
  s = s*7 + H9_mbt_s_dh[__min(9,__max(0,dh-(1-1)))];

  f = 0;
  f = f*4 + H9_mbt_f_ca[__min(8,__max(0,ca-(1-1)))];
  f = f*9 + H9_mbt_f_qc[__min(17,__max(0,qc-(1-1)))];
  f = f*4 + H9_mbt_f_im[__min(4,__max(0,im-(1-1)))];

  m = 0;
  m = m*4 + H9_mbt_m_st[__min(9,__max(0,st-(1-1)))];
  m = m*6 + H9_mbt_m_bd[__min(16,__max(0,bd-(1-1)))];

  w = 0;
  w = w*10 + H9_mbt_w_sm[__min(16,__max(0,sm-(1-1)))];
  w = w*6 + H9_mbt_w_tm[__min(16,__max(0,tm-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

