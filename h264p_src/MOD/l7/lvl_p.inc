/*  The template IDX/idx2inc.pl reads alongside lvl.idx: the lvl
    family's index builder and the tables its indices address.  Copied
    through into MOD/l7/lvl_p.inc with `H7_lvl_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l7/lvl_h.inc.  Edit this file
    and lvl.idx, not MOD/; ../../mk.sh regenerates both.

    Level -7: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_lvl_ND (or H_lvl_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H7_lvl_make( const H_lvl_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int cg = D.cg, g1 = D.g1, e1 = D.e1, nc = D.nc, pos = D.pos, sa = D.sa, sb = D.sb, q6 = D.q6, tot = D.tot, md = D.md, pm = D.pm, pg = D.pg, em = D.em, pe = D.pe, pl = D.pl, tz = D.tz, pq = D.pq, lh = D.lh, lt = D.lt, ls = D.ls, hv = D.hv, fq = D.fq;
  (void) cg;  (void) g1;  (void) e1;  (void) nc;  (void) pos;  (void) sa;  (void) sb;  (void) q6;  (void) tot;  (void) md;  (void) pm;  (void) pg;  (void) em;  (void) pe;  (void) pl;  (void) tz;  (void) pq;  (void) lh;  (void) lt;  (void) ls;  (void) hv;  (void) fq;

  a = 0;
  a = a*4 + H7_lvl_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*3 + (((g1)>1+(1-1))+((g1)>3+(1-1)));
  a = a*2 + (((e1)>0+(1-1)));

  b = 0;
  b = b*6 + H7_lvl_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*3 + (((pe)>4+(1-1))+((pe)>6+(1-1)));
  b = b*4 + H7_lvl_b_pos[__min(5,__max(0,(pos)-(1-1)))];
  b = b*7 + H7_lvl_b_tot[__min(7,__max(0,(tot)-(1-1)))];
  b = b*4 + H7_lvl_b_hv[__min(7,__max(0,(hv)-(1-1)))];
  b = b*4 + H7_lvl_b_fq[__min(3,__max(0,(fq)-(1-1)))];

  c = 0;
  c = c*5 + H7_lvl_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*4 + H7_lvl_c_sa[__min(3,__max(0,(sa)-(1-1)))];
  c = c*4 + H7_lvl_c_sb[__min(3,__max(0,(sb)-(1-1)))];
  c = c*7 + H7_lvl_c_q6[__min(8,__max(0,(q6)-(1-1)))];
  c = c*28 + H7_lvl_c_lh[__min(64,__max(0,(lh)-(1-1)))];

  d = 0;
  d = d*4 + H7_lvl_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*8 + H7_lvl_d_md[__min(19,__max(0,(md)-(1-1)))];
  d = d*2 + (((pg)>14+(1-1)));
  d = d*6 + H7_lvl_d_lt[__min(7,__max(0,(lt)-(1-1)))];

  e = 0;
  e = e*9 + H7_lvl_e_pl[__min(11,__max(0,(pl)-(1-1)))];
  e = e*8 + H7_lvl_e_tz[__min(8,__max(0,(tz)-(1-1)))];
  e = e*7 + H7_lvl_e_pq[__min(24,__max(0,(pq)-(1-1)))];

  h = 0;
  h = h*5 + H7_lvl_h_cg[__min(13,__max(0,(cg)-(1-1)))];
  h = h*4 + H7_lvl_h_pos[__min(5,__max(0,(pos)-(1-1)))];
  h = h*7 + H7_lvl_h_pl[__min(11,__max(0,(pl)-(1-1)))];
  h = h*5 + H7_lvl_h_pe[__min(15,__max(0,(pe)-(1-1)))];
  h = h*9 + H7_lvl_h_tz[__min(8,__max(0,(tz)-(1-1)))];

  i = 0;
  i = i*3 + (((cg)>3+(1-1))+((cg)>4+(1-1)));
  i = i*6 + H7_lvl_i_pl[__min(11,__max(0,(pl)-(1-1)))];

  j = 0;
  j = j*3 + (((cg)>0+(1-1))+((cg)>1+(1-1)));
  j = j*7 + H7_lvl_j_lt[__min(7,__max(0,(lt)-(1-1)))];
  j = j*14 + H7_lvl_j_lh[__min(64,__max(0,(lh)-(1-1)))];
  j = j*6 + H7_lvl_j_pos[__min(5,__max(0,(pos)-(1-1)))];

  k = 0;
  k = k*3 + (((cg)>2+(1-1))+((cg)>4+(1-1)));
  k = k*2 + (((pos)>0+(1-1)));
  k = k*6 + H7_lvl_k_pl[__min(11,__max(0,(pl)-(1-1)))];

  l = 0;
  l = l*4 + H7_lvl_l_sa[__min(3,__max(0,(sa)-(1-1)))];
  l = l*4 + H7_lvl_l_sb[__min(3,__max(0,(sb)-(1-1)))];
  l = l*38 + H7_lvl_l_lh[__min(64,__max(0,(lh)-(1-1)))];

  s = 0;
  s = s*4 + H7_lvl_s_cg[__min(13,__max(0,(cg)-(1-1)))];
  s = s*2 + (((g1)>1+(1-1)));
  s = s*2 + (((pm)>0+(1-1)));
  s = s*7 + H7_lvl_s_em[__min(16,__max(0,(em)-(1-1)))];
  s = s*5 + H7_lvl_s_ls[__min(13,__max(0,(ls)-(1-1)))];

  m = 0;

  w = 0;
  w = w*5 + H7_lvl_w_cg[__min(13,__max(0,(cg)-(1-1)))];
  w = w*3 + (((g1)>0+(1-1))+((g1)>3+(1-1)));
  w = w*5 + H7_lvl_w_md[__min(19,__max(0,(md)-(1-1)))];
  w = w*3 + (((pg)>11+(1-1))+((pg)>15+(1-1)));

  t = 0;
  t = t*2 + (((cg)>0+(1-1)));

  n = 0;
  n = n*4 + H7_lvl_n_cg[__min(13,__max(0,(cg)-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

