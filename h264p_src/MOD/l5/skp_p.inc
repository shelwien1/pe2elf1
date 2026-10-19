/*  The template IDX/idx2inc.pl reads alongside skp.idx: the skp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/skp_p.inc with `H5_skp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/skp_h.inc.  Edit this file
    and skp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_skp_ND (or H_skp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H5_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int st = D.st, a3 = D.a3, b3 = D.b3, col = D.col, c9 = D.c9, cd = D.cd, run = D.run, cnt = D.cnt, eq = D.eq, lf = D.lf, af = D.af, td = D.td, en = D.en, sm = D.sm, tm = D.tm, tb = D.tb, bd = D.bd, qc = D.qc, c1 = D.c1, k9 = D.k9, tx = D.tx;
  (void) st;  (void) a3;  (void) b3;  (void) col;  (void) c9;  (void) cd;  (void) run;  (void) cnt;  (void) eq;  (void) lf;  (void) af;  (void) td;  (void) en;  (void) sm;  (void) tm;  (void) tb;  (void) bd;  (void) qc;  (void) c1;  (void) k9;  (void) tx;

  a = 0;
  a = a*10 + H5_skp_a_st[__min(9,__max(0,st-(1-1)))];
  a = a*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  a = a*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  a = a*10 + H5_skp_a_c9[__min(9,__max(0,c9-(1-1)))];
  a = a*2 + ((eq>0+(1-1)));
  a = a*3 + ((lf>0+(1-1))+(lf>1+(1-1)));
  a = a*3 + ((af>0+(1-1))+(af>1+(1-1)));

  b = 0;
  b = b*2 + ((st>4+(1-1)));
  b = b*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  b = b*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  b = b*2 + ((cd>0+(1-1)));
  b = b*7 + H5_skp_b_cnt[__min(15,__max(0,cnt-(1-1)))];
  b = b*3 + ((c1>0+(1-1))+(c1>1+(1-1)));

  c = 0;
  c = c*2 + ((st>1+(1-1)));
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*7 + H5_skp_c_c9[__min(9,__max(0,c9-(1-1)))];
  c = c*3 + ((run>1+(1-1))+(run>3+(1-1)));
  c = c*15 + H5_skp_c_en[__min(14,__max(0,en-(1-1)))];

  d = 0;
  d = d*10 + H5_skp_d_st[__min(9,__max(0,st-(1-1)))];
  d = d*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  d = d*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  d = d*17 + H5_skp_d_td[__min(16,__max(0,td-(1-1)))];
  d = d*3 + ((c1>0+(1-1))+(c1>1+(1-1)));
  d = d*11 + H5_skp_d_k9[__min(10,__max(0,k9-(1-1)))];

  e = 0;
  e = e*10 + H5_skp_e_st[__min(9,__max(0,st-(1-1)))];
  e = e*17 + H5_skp_e_td[__min(16,__max(0,td-(1-1)))];
  e = e*5 + H5_skp_e_en[__min(14,__max(0,en-(1-1)))];

  h = 0;
  h = h*10 + H5_skp_h_st[__min(9,__max(0,st-(1-1)))];

  i = 0;
  i = i*15 + H5_skp_i_en[__min(14,__max(0,en-(1-1)))];
  i = i*3 + ((a3>0+(1-1))+(a3>1+(1-1)));

  j = 0;
  j = j*10 + H5_skp_j_st[__min(9,__max(0,st-(1-1)))];
  j = j*10 + H5_skp_j_c9[__min(9,__max(0,c9-(1-1)))];
  j = j*18 + H5_skp_j_qc[__min(17,__max(0,qc-(1-1)))];

  k = 0;
  k = k*17 + H5_skp_k_td[__min(16,__max(0,td-(1-1)))];
  k = k*2 + ((b3>0+(1-1)));

  l = 0;
  l = l*10 + H5_skp_l_st[__min(9,__max(0,st-(1-1)))];
  l = l*2 + ((col>1+(1-1)));
  l = l*3 + ((cd>0+(1-1))+(cd>1+(1-1)));

  m = 0;
  m = m*2 + ((col>1+(1-1)));
  m = m*5 + H5_skp_m_td[__min(4,__max(0,td-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

