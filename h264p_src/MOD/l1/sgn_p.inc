/*  The template IDX/idx2inc.pl reads alongside sgn.idx: the sgn
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/sgn_p.inc with `H1_sgn_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/sgn_h.inc.  Edit this file
    and sgn.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int cg = D.cg, pos = D.pos, sa = D.sa, sb = D.sb, scb = D.scb, mag = D.mag, md = D.md, pp = D.pp, mg = D.mg, ez = D.ez, ts = D.ts, pq = D.pq, gh = D.gh, se = D.se, dm = D.dm;
  (void) cg;  (void) pos;  (void) sa;  (void) sb;  (void) scb;  (void) mag;  (void) md;  (void) pp;  (void) mg;  (void) ez;  (void) ts;  (void) pq;  (void) gh;  (void) se;  (void) dm;

  a = 0;
  a = a*6 + H1_sgn_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*4 + H1_sgn_a_pos[__min(3,__max(0,(pos)-(1-1)))];
  a = a*4 + H1_sgn_a_sa[__min(3,__max(0,(sa)-(1-1)))];
  a = a*4 + H1_sgn_a_sb[__min(3,__max(0,(sb)-(1-1)))];
  a = a*7 + H1_sgn_a_gh[__min(8,__max(0,(gh)-(1-1)))];

  b = 0;
  b = b*4 + H1_sgn_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*4 + H1_sgn_b_scb[__min(3,__max(0,(scb)-(1-1)))];
  b = b*4 + H1_sgn_b_pos[__min(3,__max(0,(pos)-(1-1)))];

  c = 0;
  c = c*6 + H1_sgn_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*2 + (((pos)>0+(1-1)));
  c = c*7 + H1_sgn_c_gh[__min(8,__max(0,(gh)-(1-1)))];

  d = 0;
  d = d*5 + H1_sgn_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*4 + H1_sgn_d_sa[__min(3,__max(0,(sa)-(1-1)))];
  d = d*4 + H1_sgn_d_sb[__min(3,__max(0,(sb)-(1-1)))];
  d = d*4 + H1_sgn_d_pos[__min(3,__max(0,(pos)-(1-1)))];

  m = 0;
  m = m*5 + H1_sgn_m_cg[__min(13,__max(0,(cg)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

