/*  The template IDX/idx2inc.pl reads alongside sig.idx: the sig
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/sig_p.inc with `H1_sig_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/sig_h.inc.  Edit this file
    and sig.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_sig_ND (or H_sig_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int cg = D.cg, pos = D.pos, rem = D.rem, zl = D.zl, md = D.md, nc = D.nc, fld = D.fld, q6 = D.q6, pg = D.pg, em = D.em, pe = D.pe, ps = D.ps, tz = D.tz, pq = D.pq, hb = D.hb, ft = D.ft, h3 = D.h3, pr = D.pr, sn = D.sn, hv = D.hv, pf = D.pf;
  (void) cg;  (void) pos;  (void) rem;  (void) zl;  (void) md;  (void) nc;  (void) fld;  (void) q6;  (void) pg;  (void) em;  (void) pe;  (void) ps;  (void) tz;  (void) pq;  (void) hb;  (void) ft;  (void) h3;  (void) pr;  (void) sn;  (void) hv;  (void) pf;

  a = 0;
  a = a*14 + H1_sig_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*64 + H1_sig_a_pos[__min(63,__max(0,pos-(1-1)))];
  a = a*7 + H1_sig_a_rem[__min(10,__max(0,rem-(1-1)))];

  b = 0;
  b = b*14 + H1_sig_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*39 + H1_sig_b_pos[__min(63,__max(0,pos-(1-1)))];
  b = b*16 + H1_sig_b_md[__min(19,__max(0,md-(1-1)))];
  b = b*511 + H1_sig_b_hb[__min(510,__max(0,hb-(1-1)))];

  c = 0;
  c = c*14 + H1_sig_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*64 + H1_sig_c_pos[__min(63,__max(0,pos-(1-1)))];
  c = c*9 + H1_sig_c_sn[__min(8,__max(0,sn-(1-1)))];

  d = 0;
  d = d*5 + H1_sig_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*64 + H1_sig_d_pos[__min(63,__max(0,pos-(1-1)))];
  d = d*9 + H1_sig_d_rem[__min(10,__max(0,rem-(1-1)))];
  d = d*15 + H1_sig_d_h3[__min(14,__max(0,h3-(1-1)))];

  m = 0;
  m = m*14 + H1_sig_m_cg[__min(13,__max(0,cg-(1-1)))];
  m = m*4 + H1_sig_m_pos[__min(9,__max(0,pos-(1-1)))];
  m = m*4 + H1_sig_m_rem[__min(10,__max(0,rem-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

