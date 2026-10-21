/*  The template IDX/idx2inc.pl reads alongside lvl.idx: the lvl
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/lvl_p.inc with `H5_lvl_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/lvl_h.inc.  Edit this file
    and lvl.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_lvl_ND (or H_lvl_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H5_lvl_make( const H_lvl_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int cg = D.cg, g1 = D.g1, e1 = D.e1, nc = D.nc, pos = D.pos, sa = D.sa, sb = D.sb, q6 = D.q6, tot = D.tot, md = D.md, pm = D.pm, pg = D.pg, em = D.em, pe = D.pe, pl = D.pl, tz = D.tz, pq = D.pq, lh = D.lh, lt = D.lt, ls = D.ls, hv = D.hv, fq = D.fq;
  (void) cg;  (void) g1;  (void) e1;  (void) nc;  (void) pos;  (void) sa;  (void) sb;  (void) q6;  (void) tot;  (void) md;  (void) pm;  (void) pg;  (void) em;  (void) pe;  (void) pl;  (void) tz;  (void) pq;  (void) lh;  (void) lt;  (void) ls;  (void) hv;  (void) fq;

  a = 0;
  a = a*5 + H5_lvl_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*4 + H5_lvl_a_e1[__min(4,__max(0,(e1)-(1-1)))];
  a = a*2 + (((nc)>1+(1-1)));

  b = 0;
  b = b*6 + H5_lvl_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*4 + H5_lvl_b_pos[__min(5,__max(0,(pos)-(1-1)))];
  b = b*7 + H5_lvl_b_tot[__min(7,__max(0,(tot)-(1-1)))];
  b = b*6 + H5_lvl_b_hv[__min(7,__max(0,(hv)-(1-1)))];
  b = b*4 + H5_lvl_b_fq[__min(3,__max(0,(fq)-(1-1)))];

  c = 0;
  c = c*5 + H5_lvl_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*4 + H5_lvl_c_sa[__min(3,__max(0,(sa)-(1-1)))];
  c = c*4 + H5_lvl_c_sb[__min(3,__max(0,(sb)-(1-1)))];
  c = c*8 + H5_lvl_c_q6[__min(8,__max(0,(q6)-(1-1)))];
  c = c*62 + H5_lvl_c_lh[__min(64,__max(0,(lh)-(1-1)))];

  d = 0;
  d = d*5 + H5_lvl_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*7 + H5_lvl_d_md[__min(19,__max(0,(md)-(1-1)))];
  d = d*4 + H5_lvl_d_pg[__min(16,__max(0,(pg)-(1-1)))];
  d = d*7 + H5_lvl_d_lt[__min(7,__max(0,(lt)-(1-1)))];

  e = 0;
  e = e*6 + H5_lvl_e_cg[__min(13,__max(0,(cg)-(1-1)))];
  e = e*3 + (((g1)>0+(1-1))+((g1)>3+(1-1)));
  e = e*4 + H5_lvl_e_e1[__min(4,__max(0,(e1)-(1-1)))];
  e = e*9 + H5_lvl_e_tz[__min(8,__max(0,(tz)-(1-1)))];

  h = 0;
  h = h*3 + (((pos)>0+(1-1))+((pos)>4+(1-1)));
  h = h*8 + H5_lvl_h_tz[__min(8,__max(0,(tz)-(1-1)))];

  i = 0;
  i = i*3 + (((cg)>0+(1-1))+((cg)>2+(1-1)));
  i = i*2 + (((g1)>2+(1-1)));

  j = 0;
  j = j*8 + H5_lvl_j_lt[__min(7,__max(0,(lt)-(1-1)))];
  j = j*61 + H5_lvl_j_lh[__min(64,__max(0,(lh)-(1-1)))];
  j = j*6 + H5_lvl_j_pos[__min(5,__max(0,(pos)-(1-1)))];

  k = 0;
  k = k*6 + H5_lvl_k_cg[__min(13,__max(0,(cg)-(1-1)))];
  k = k*6 + H5_lvl_k_pos[__min(5,__max(0,(pos)-(1-1)))];
  k = k*4 + H5_lvl_k_g1[__min(4,__max(0,(g1)-(1-1)))];

  l = 0;
  l = l*2 + (((cg)>0+(1-1)));
  l = l*4 + H5_lvl_l_sa[__min(3,__max(0,(sa)-(1-1)))];
  l = l*4 + H5_lvl_l_sb[__min(3,__max(0,(sb)-(1-1)))];
  l = l*7 + H5_lvl_l_lh[__min(64,__max(0,(lh)-(1-1)))];

  m = 0;
  m = m*3 + (((cg)>0+(1-1))+((cg)>2+(1-1)));
  m = m*2 + (((g1)>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

