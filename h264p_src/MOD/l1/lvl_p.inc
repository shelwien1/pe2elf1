/*  The template IDX/idx2inc.pl reads alongside lvl.idx: the lvl
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/lvl_p.inc with `H1_lvl_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/lvl_h.inc.  Edit this file
    and lvl.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_lvl_ND (or H_lvl_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_lvl_make( const H_lvl_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int cg = D.cg, g1 = D.g1, e1 = D.e1, nc = D.nc, pos = D.pos, sa = D.sa, sb = D.sb, q6 = D.q6, tot = D.tot, md = D.md, pm = D.pm, pg = D.pg, em = D.em, pe = D.pe, pl = D.pl, tz = D.tz, pq = D.pq, lh = D.lh, lt = D.lt, ls = D.ls, hv = D.hv, fq = D.fq;
  (void) cg;  (void) g1;  (void) e1;  (void) nc;  (void) pos;  (void) sa;  (void) sb;  (void) q6;  (void) tot;  (void) md;  (void) pm;  (void) pg;  (void) em;  (void) pe;  (void) pl;  (void) tz;  (void) pq;  (void) lh;  (void) lt;  (void) ls;  (void) hv;  (void) fq;

  a = 0;
  a = a*8 + H1_lvl_a_lt[__min(7,__max(0,lt-(1-1)))];
  a = a*65 + H1_lvl_a_lh[__min(64,__max(0,lh-(1-1)))];
  a = a*6 + H1_lvl_a_pos[__min(5,__max(0,pos-(1-1)))];

  b = 0;
  b = b*14 + H1_lvl_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*6 + H1_lvl_b_pos[__min(5,__max(0,pos-(1-1)))];
  b = b*8 + H1_lvl_b_tot[__min(7,__max(0,tot-(1-1)))];
  b = b*4 + H1_lvl_b_fq[__min(3,__max(0,fq-(1-1)))];

  c = 0;
  c = c*5 + H1_lvl_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*4 + H1_lvl_c_sa[__min(3,__max(0,sa-(1-1)))];
  c = c*4 + H1_lvl_c_sb[__min(3,__max(0,sb-(1-1)))];
  c = c*6 + H1_lvl_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*65 + H1_lvl_c_lh[__min(64,__max(0,lh-(1-1)))];

  d = 0;
  d = d*14 + H1_lvl_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*20 + H1_lvl_d_md[__min(19,__max(0,md-(1-1)))];
  d = d*6 + H1_lvl_d_lt[__min(7,__max(0,lt-(1-1)))];

  m = 0;
  m = m*2 + ((g1>0+(1-1)));

  t = 0;
  t = t*14 + H1_lvl_t_cg[__min(13,__max(0,cg-(1-1)))];

  n = 0;
  n = n*14 + H1_lvl_n_cg[__min(13,__max(0,cg-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

