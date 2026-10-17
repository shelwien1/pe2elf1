/*  The template IDX/idx2inc.pl reads alongside sgn.idx: the sgn
    family's index builder and the tables its indices address.  Copied
    through into MOD/sgn_p.inc with `H_sgn_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/sgn_h.inc.  Edit this file and
    sgn.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int cg = D.cg, pos = D.pos, sa = D.sa, sb = D.sb, scb = D.scb, mag = D.mag, md = D.md;
  (void) cg;  (void) pos;  (void) sa;  (void) sb;  (void) scb;  (void) mag;  (void) md;

  a = 0;
  a = a*14 + H_sgn_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*4 + H_sgn_a_pos[__min(3,__max(0,pos-(1-1)))];
  a = a*4 + H_sgn_a_sa[__min(3,__max(0,sa-(1-1)))];
  a = a*4 + H_sgn_a_sb[__min(3,__max(0,sb-(1-1)))];

  b = 0;
  b = b*14 + H_sgn_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*4 + H_sgn_b_scb[__min(3,__max(0,scb-(1-1)))];
  b = b*4 + H_sgn_b_pos[__min(3,__max(0,pos-(1-1)))];

  c = 0;
  c = c*14 + H_sgn_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*3 + ((mag>0+(1-1))+(mag>1+(1-1)));
  c = c*20 + H_sgn_c_md[__min(19,__max(0,md-(1-1)))];

  d = 0;
  d = d*14 + H_sgn_d_cg[__min(13,__max(0,cg-(1-1)))];

  s = 0;
  s = s*14 + H_sgn_s_cg[__min(13,__max(0,cg-(1-1)))];

  f = 0;
  f = f*14 + H_sgn_f_cg[__min(13,__max(0,cg-(1-1)))];

  m = 0;
  m = m*14 + H_sgn_m_cg[__min(13,__max(0,cg-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

