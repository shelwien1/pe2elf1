/*  The template IDX/idx2inc.pl reads alongside sgn.idx: the sgn
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/sgn_p.inc with `H5_sgn_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/sgn_h.inc.  Edit this file
    and sgn.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H5_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int cg = D.cg, pos = D.pos, sa = D.sa, sb = D.sb, scb = D.scb, mag = D.mag, md = D.md, pp = D.pp, mg = D.mg, ez = D.ez, ts = D.ts, pq = D.pq, gh = D.gh, se = D.se, dm = D.dm;
  (void) cg;  (void) pos;  (void) sa;  (void) sb;  (void) scb;  (void) mag;  (void) md;  (void) pp;  (void) mg;  (void) ez;  (void) ts;  (void) pq;  (void) gh;  (void) se;  (void) dm;

  a = 0;
  a = a*14 + H5_sgn_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*4 + H5_sgn_a_pos[__min(3,__max(0,pos-(1-1)))];
  a = a*4 + H5_sgn_a_sa[__min(3,__max(0,sa-(1-1)))];
  a = a*4 + H5_sgn_a_sb[__min(3,__max(0,sb-(1-1)))];
  a = a*9 + H5_sgn_a_gh[__min(8,__max(0,gh-(1-1)))];

  b = 0;
  b = b*14 + H5_sgn_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*4 + H5_sgn_b_scb[__min(3,__max(0,scb-(1-1)))];
  b = b*4 + H5_sgn_b_pos[__min(3,__max(0,pos-(1-1)))];

  c = 0;
  c = c*14 + H5_sgn_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*2 + ((mag>0+(1-1)));
  c = c*9 + H5_sgn_c_md[__min(19,__max(0,md-(1-1)))];

  d = 0;
  d = d*14 + H5_sgn_d_cg[__min(13,__max(0,cg-(1-1)))];

  e = 0;
  e = e*4 + H5_sgn_e_cg[__min(13,__max(0,cg-(1-1)))];

  h = 0;
  h = h*14 + H5_sgn_h_cg[__min(13,__max(0,cg-(1-1)))];
  h = h*4 + H5_sgn_h_pos[__min(3,__max(0,pos-(1-1)))];
  h = h*7 + H5_sgn_h_ts[__min(6,__max(0,ts-(1-1)))];
  h = h*7 + H5_sgn_h_dm[__min(6,__max(0,dm-(1-1)))];

  i = 0;
  i = i*14 + H5_sgn_i_cg[__min(13,__max(0,cg-(1-1)))];

  j = 0;
  j = j*14 + H5_sgn_j_cg[__min(13,__max(0,cg-(1-1)))];
  j = j*4 + H5_sgn_j_sa[__min(3,__max(0,sa-(1-1)))];
  j = j*4 + H5_sgn_j_sb[__min(3,__max(0,sb-(1-1)))];
  j = j*9 + H5_sgn_j_gh[__min(8,__max(0,gh-(1-1)))];
  j = j*2 + ((pos>0+(1-1)));

  k = 0;
  k = k*14 + H5_sgn_k_cg[__min(13,__max(0,cg-(1-1)))];

  l = 0;
  l = l*14 + H5_sgn_l_cg[__min(13,__max(0,cg-(1-1)))];
  l = l*4 + H5_sgn_l_pos[__min(3,__max(0,pos-(1-1)))];
  l = l*6 + H5_sgn_l_ts[__min(6,__max(0,ts-(1-1)))];
  l = l*9 + H5_sgn_l_gh[__min(8,__max(0,gh-(1-1)))];

  m = 0;
  m = m*14 + H5_sgn_m_cg[__min(13,__max(0,cg-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

