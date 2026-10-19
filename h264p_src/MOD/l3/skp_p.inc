/*  The template IDX/idx2inc.pl reads alongside skp.idx: the skp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l3/skp_p.inc with `H3_skp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l3/skp_h.inc.  Edit this file
    and skp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -3: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_skp_ND (or H_skp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H3_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int st = D.st, a3 = D.a3, b3 = D.b3, col = D.col, c9 = D.c9, cd = D.cd, run = D.run, cnt = D.cnt, eq = D.eq, lf = D.lf, af = D.af, td = D.td, en = D.en, sm = D.sm, tm = D.tm, tb = D.tb, bd = D.bd, qc = D.qc, c1 = D.c1, k9 = D.k9, tx = D.tx;
  (void) st;  (void) a3;  (void) b3;  (void) col;  (void) c9;  (void) cd;  (void) run;  (void) cnt;  (void) eq;  (void) lf;  (void) af;  (void) td;  (void) en;  (void) sm;  (void) tm;  (void) tb;  (void) bd;  (void) qc;  (void) c1;  (void) k9;  (void) tx;

  a = 0;
  a = a*10 + H3_skp_a_st[__min(9,__max(0,st-(1-1)))];
  a = a*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  a = a*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  a = a*10 + H3_skp_a_c9[__min(9,__max(0,c9-(1-1)))];
  a = a*2 + ((eq>0+(1-1)));
  a = a*3 + ((lf>0+(1-1))+(lf>1+(1-1)));
  a = a*3 + ((af>0+(1-1))+(af>1+(1-1)));

  b = 0;
  b = b*2 + ((st>4+(1-1)));
  b = b*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  b = b*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  b = b*2 + ((cd>0+(1-1)));
  b = b*7 + H3_skp_b_cnt[__min(15,__max(0,cnt-(1-1)))];

  c = 0;
  c = c*2 + ((st>1+(1-1)));
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*7 + H3_skp_c_c9[__min(9,__max(0,c9-(1-1)))];
  c = c*3 + ((run>1+(1-1))+(run>3+(1-1)));

  d = 0;
  d = d*10 + H3_skp_d_st[__min(9,__max(0,st-(1-1)))];
  d = d*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  d = d*3 + ((b3>0+(1-1))+(b3>1+(1-1)));

  e = 0;
  e = e*10 + H3_skp_e_st[__min(9,__max(0,st-(1-1)))];

  h = 0;
  h = h*10 + H3_skp_h_st[__min(9,__max(0,st-(1-1)))];

  i = 0;
  i = i*3 + ((a3>0+(1-1))+(a3>1+(1-1)));

  j = 0;
  j = j*10 + H3_skp_j_st[__min(9,__max(0,st-(1-1)))];
  j = j*10 + H3_skp_j_c9[__min(9,__max(0,c9-(1-1)))];
  j = j*18 + H3_skp_j_qc[__min(17,__max(0,qc-(1-1)))];

  k = 0;
  k = k*2 + ((b3>0+(1-1)));

  l = 0;
  l = l*10 + H3_skp_l_st[__min(9,__max(0,st-(1-1)))];
  l = l*2 + ((col>1+(1-1)));
  l = l*3 + ((cd>0+(1-1))+(cd>1+(1-1)));

  s = 0;
  s = s*10 + H3_skp_s_st[__min(9,__max(0,st-(1-1)))];

  m = 0;
  m = m*2 + ((col>1+(1-1)));

  w = 0;
  w = w*10 + H3_skp_w_st[__min(9,__max(0,st-(1-1)))];
  w = w*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  w = w*3 + ((b3>0+(1-1))+(b3>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

