/*  The template IDX/idx2inc.pl reads alongside lvl.idx: the lvl
    family's index builder and the tables its indices address.  Copied
    through into MOD/lvl_p.inc with `H_lvl_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/lvl_h.inc.  Edit this file and
    lvl.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_lvl_ND (or H_lvl_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_lvl_make( const H_lvl_v & D, tcx & X ) {
  int a, b, c, d, e, h, s, f, m, w, t, n, g;
  int cg = D.cg, g1 = D.g1, e1 = D.e1, nc = D.nc, pos = D.pos, sa = D.sa, sb = D.sb, q6 = D.q6, tot = D.tot, md = D.md, pm = D.pm, pg = D.pg, em = D.em, pe = D.pe, pl = D.pl, tz = D.tz, pq = D.pq, lh = D.lh, lt = D.lt;
  (void) cg;  (void) g1;  (void) e1;  (void) nc;  (void) pos;  (void) sa;  (void) sb;  (void) q6;  (void) tot;  (void) md;  (void) pm;  (void) pg;  (void) em;  (void) pe;  (void) pl;  (void) tz;  (void) pq;  (void) lh;  (void) lt;

  a = 0;
  a = a*14 + H_lvl_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*5 + H_lvl_a_g1[__min(4,__max(0,g1-(1-1)))];
  a = a*5 + H_lvl_a_e1[__min(4,__max(0,e1-(1-1)))];
  a = a*5 + H_lvl_a_nc[__min(5,__max(0,nc-(1-1)))];

  b = 0;
  b = b*14 + H_lvl_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*4 + H_lvl_b_pe[__min(15,__max(0,pe-(1-1)))];
  b = b*6 + H_lvl_b_pos[__min(5,__max(0,pos-(1-1)))];
  b = b*8 + H_lvl_b_tot[__min(7,__max(0,tot-(1-1)))];

  c = 0;
  c = c*14 + H_lvl_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*4 + H_lvl_c_sa[__min(3,__max(0,sa-(1-1)))];
  c = c*4 + H_lvl_c_sb[__min(3,__max(0,sb-(1-1)))];
  c = c*6 + H_lvl_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*65 + H_lvl_c_lh[__min(64,__max(0,lh-(1-1)))];

  d = 0;
  d = d*14 + H_lvl_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*5 + H_lvl_d_g1[__min(4,__max(0,g1-(1-1)))];
  d = d*20 + H_lvl_d_md[__min(19,__max(0,md-(1-1)))];
  d = d*3 + ((pg>3+(1-1))+(pg>15+(1-1)));
  d = d*8 + H_lvl_d_lt[__min(7,__max(0,lt-(1-1)))];

  e = 0;
  e = e*5 + H_lvl_e_g1[__min(4,__max(0,g1-(1-1)))];
  e = e*5 + H_lvl_e_e1[__min(4,__max(0,e1-(1-1)))];
  e = e*12 + H_lvl_e_pl[__min(11,__max(0,pl-(1-1)))];
  e = e*5 + H_lvl_e_tz[__min(4,__max(0,tz-(1-1)))];
  e = e*25 + H_lvl_e_pq[__min(24,__max(0,pq-(1-1)))];

  h = 0;
  h = h*14 + H_lvl_h_cg[__min(13,__max(0,cg-(1-1)))];
  h = h*6 + H_lvl_h_pos[__min(5,__max(0,pos-(1-1)))];
  h = h*12 + H_lvl_h_pl[__min(11,__max(0,pl-(1-1)))];
  h = h*16 + H_lvl_h_pe[__min(15,__max(0,pe-(1-1)))];
  h = h*5 + H_lvl_h_tz[__min(4,__max(0,tz-(1-1)))];

  s = 0;
  s = s*2 + ((cg>3+(1-1)));
  s = s*5 + H_lvl_s_pm[__min(4,__max(0,pm-(1-1)))];
  s = s*17 + H_lvl_s_em[__min(16,__max(0,em-(1-1)))];

  f = 0;

  m = 0;
  m = m*2 + ((g1>0+(1-1)));

  w = 0;
  w = w*14 + H_lvl_w_cg[__min(13,__max(0,cg-(1-1)))];
  w = w*5 + H_lvl_w_g1[__min(4,__max(0,g1-(1-1)))];
  w = w*20 + H_lvl_w_md[__min(19,__max(0,md-(1-1)))];
  w = w*3 + ((pg>3+(1-1))+(pg>15+(1-1)));

  t = 0;
  t = t*14 + H_lvl_t_cg[__min(13,__max(0,cg-(1-1)))];

  n = 0;
  n = n*14 + H_lvl_n_cg[__min(13,__max(0,cg-(1-1)))];

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

