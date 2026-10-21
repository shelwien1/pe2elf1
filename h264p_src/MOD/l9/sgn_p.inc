/*  The template IDX/idx2inc.pl reads alongside sgn.idx: the sgn
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/sgn_p.inc with `H9_sgn_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/sgn_h.inc.  Edit this file
    and sgn.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int cg = D.cg, pos = D.pos, sa = D.sa, sb = D.sb, scb = D.scb, mag = D.mag, md = D.md, pp = D.pp, mg = D.mg, ez = D.ez, ts = D.ts, pq = D.pq, gh = D.gh, se = D.se, dm = D.dm;
  (void) cg;  (void) pos;  (void) sa;  (void) sb;  (void) scb;  (void) mag;  (void) md;  (void) pp;  (void) mg;  (void) ez;  (void) ts;  (void) pq;  (void) gh;  (void) se;  (void) dm;

  a = 0;
  a = a*3 + (((cg)>0+(1-1))+((cg)>1+(1-1)));
  a = a*3 + (((pos)>0+(1-1))+((pos)>2+(1-1)));
  a = a*4 + H9_sgn_a_sa[__min(3,__max(0,(sa)-(1-1)))];
  a = a*4 + H9_sgn_a_sb[__min(3,__max(0,(sb)-(1-1)))];

  b = 0;
  b = b*4 + H9_sgn_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*4 + H9_sgn_b_scb[__min(3,__max(0,(scb)-(1-1)))];
  b = b*2 + (((pos)>0+(1-1)));

  c = 0;
  c = c*4 + H9_sgn_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*7 + H9_sgn_c_md[__min(19,__max(0,(md)-(1-1)))];

  d = 0;
  d = d*5 + H9_sgn_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*3 + (((pp)>0+(1-1))+((pp)>1+(1-1)));
  d = d*7 + H9_sgn_d_mg[__min(15,__max(0,(mg)-(1-1)))];

  e = 0;
  e = e*5 + H9_sgn_e_cg[__min(13,__max(0,(cg)-(1-1)))];
  e = e*10 + H9_sgn_e_mg[__min(15,__max(0,(mg)-(1-1)))];

  h = 0;
  h = h*5 + H9_sgn_h_cg[__min(13,__max(0,(cg)-(1-1)))];
  h = h*3 + (((pos)>0+(1-1))+((pos)>1+(1-1)));
  h = h*5 + H9_sgn_h_ez[__min(5,__max(0,(ez)-(1-1)))];
  h = h*3 + (((pp)>0+(1-1))+((pp)>1+(1-1)));
  h = h*6 + H9_sgn_h_ts[__min(6,__max(0,(ts)-(1-1)))];
  h = h*9 + H9_sgn_h_pq[__min(24,__max(0,(pq)-(1-1)))];
  h = h*6 + H9_sgn_h_dm[__min(6,__max(0,(dm)-(1-1)))];

  i = 0;
  i = i*3 + (((cg)>2+(1-1))+((cg)>4+(1-1)));
  i = i*5 + H9_sgn_i_ez[__min(5,__max(0,(ez)-(1-1)))];
  i = i*5 + H9_sgn_i_se[__min(7,__max(0,(se)-(1-1)))];

  j = 0;
  j = j*6 + H9_sgn_j_cg[__min(13,__max(0,(cg)-(1-1)))];
  j = j*4 + H9_sgn_j_sa[__min(3,__max(0,(sa)-(1-1)))];
  j = j*4 + H9_sgn_j_sb[__min(3,__max(0,(sb)-(1-1)))];
  j = j*7 + H9_sgn_j_gh[__min(8,__max(0,(gh)-(1-1)))];
  j = j*4 + H9_sgn_j_pos[__min(3,__max(0,(pos)-(1-1)))];

  k = 0;
  k = k*3 + (((cg)>2+(1-1))+((cg)>3+(1-1)));
  k = k*3 + (((pp)>0+(1-1))+((pp)>1+(1-1)));
  k = k*4 + H9_sgn_k_mg[__min(15,__max(0,(mg)-(1-1)))];
  k = k*6 + H9_sgn_k_ez[__min(5,__max(0,(ez)-(1-1)))];

  l = 0;
  l = l*5 + H9_sgn_l_cg[__min(13,__max(0,(cg)-(1-1)))];
  l = l*4 + H9_sgn_l_pos[__min(3,__max(0,(pos)-(1-1)))];
  l = l*7 + H9_sgn_l_ts[__min(6,__max(0,(ts)-(1-1)))];
  l = l*2 + (((gh)>0+(1-1)));

  s = 0;
  s = s*3 + (((pp)>0+(1-1))+((pp)>1+(1-1)));
  s = s*5 + H9_sgn_s_se[__min(7,__max(0,(se)-(1-1)))];

  f = 0;

  m = 0;
  m = m*3 + (((cg)>2+(1-1))+((cg)>4+(1-1)));

  w = 0;
  w = w*6 + H9_sgn_w_cg[__min(13,__max(0,(cg)-(1-1)))];
  w = w*7 + H9_sgn_w_mg[__min(15,__max(0,(mg)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

