/*  The template IDX/idx2inc.pl reads alongside skp.idx: the skp
    family's index builder and the tables its indices address.  Copied
    through into MOD/skp_p.inc with `H_skp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/skp_h.inc.  Edit this file and
    skp.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_skp_ND (or H_skp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, s, f, m, w, t, n, g;
  int st = D.st, a3 = D.a3, b3 = D.b3, col = D.col, c9 = D.c9, cd = D.cd, run = D.run, cnt = D.cnt, eq = D.eq, lf = D.lf, af = D.af, td = D.td, en = D.en, sm = D.sm, tm = D.tm, tb = D.tb, bd = D.bd, qc = D.qc;
  (void) st;  (void) a3;  (void) b3;  (void) col;  (void) c9;  (void) cd;  (void) run;  (void) cnt;  (void) eq;  (void) lf;  (void) af;  (void) td;  (void) en;  (void) sm;  (void) tm;  (void) tb;  (void) bd;  (void) qc;

  a = 0;
  a = a*10 + H_skp_a_st[__min(9,__max(0,st-(1-1)))];
  a = a*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  a = a*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  a = a*10 + H_skp_a_c9[__min(9,__max(0,c9-(1-1)))];
  a = a*2 + ((eq>0+(1-1)));
  a = a*3 + ((lf>0+(1-1))+(lf>1+(1-1)));
  a = a*3 + ((af>0+(1-1))+(af>1+(1-1)));

  b = 0;
  b = b*10 + H_skp_b_st[__min(9,__max(0,st-(1-1)))];
  b = b*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  b = b*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  b = b*2 + ((cd>0+(1-1)));
  b = b*16 + H_skp_b_cnt[__min(15,__max(0,cnt-(1-1)))];

  c = 0;
  c = c*2 + ((st>1+(1-1)));
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*7 + H_skp_c_c9[__min(9,__max(0,c9-(1-1)))];
  c = c*5 + H_skp_c_run[__min(9,__max(0,run-(1-1)))];
  c = c*15 + H_skp_c_en[__min(14,__max(0,en-(1-1)))];

  d = 0;
  d = d*10 + H_skp_d_st[__min(9,__max(0,st-(1-1)))];
  d = d*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  d = d*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  d = d*17 + H_skp_d_td[__min(16,__max(0,td-(1-1)))];

  e = 0;
  e = e*10 + H_skp_e_st[__min(9,__max(0,st-(1-1)))];
  e = e*17 + H_skp_e_td[__min(16,__max(0,td-(1-1)))];
  e = e*15 + H_skp_e_en[__min(14,__max(0,en-(1-1)))];
  e = e*17 + H_skp_e_sm[__min(16,__max(0,sm-(1-1)))];

  h = 0;
  h = h*10 + H_skp_h_st[__min(9,__max(0,st-(1-1)))];
  h = h*17 + H_skp_h_tm[__min(16,__max(0,tm-(1-1)))];
  h = h*17 + H_skp_h_sm[__min(16,__max(0,sm-(1-1)))];
  h = h*17 + H_skp_h_bd[__min(16,__max(0,bd-(1-1)))];

  s = 0;
  s = s*10 + H_skp_s_st[__min(9,__max(0,st-(1-1)))];
  s = s*17 + H_skp_s_tb[__min(16,__max(0,tb-(1-1)))];
  s = s*17 + H_skp_s_bd[__min(16,__max(0,bd-(1-1)))];

  f = 0;
  f = f*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  f = f*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  f = f*18 + H_skp_f_qc[__min(17,__max(0,qc-(1-1)))];

  m = 0;
  m = m*2 + ((st>1+(1-1)));
  m = m*3 + ((col>0+(1-1))+(col>1+(1-1)));
  m = m*5 + H_skp_m_td[__min(4,__max(0,td-(1-1)))];

  w = 0;
  w = w*10 + H_skp_w_st[__min(9,__max(0,st-(1-1)))];
  w = w*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  w = w*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  w = w*17 + H_skp_w_td[__min(16,__max(0,td-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

