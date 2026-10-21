/*  The template IDX/idx2inc.pl reads alongside nal.idx: the nal
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/nal_p.inc with `H1_nal_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/nal_h.inc.  Edit this file
    and nal.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_nal_ND (or H_nal_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_nal_make( const H_nal_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int fld = D.fld, ctx = D.ctx, p1 = D.p1, p2 = D.p2;
  (void) fld;  (void) ctx;  (void) p1;  (void) p2;

  a = 0;
  a = a*7 + H1_nal_a_fld[__min(31,__max(0,(fld)-(1-1)))];
  a = a*6 + H1_nal_a_ctx[__min(31,__max(0,(ctx)-(1-1)))];

  b = 0;
  b = b*5 + H1_nal_b_fld[__min(31,__max(0,(fld)-(1-1)))];
  b = b*3 + (((p1)>0+(1-1))+((p1)>5+(1-1)));
  b = b*4 + H1_nal_b_p2[__min(3,__max(0,(p2)-(1-1)))];

  c = 0;
  c = c*5 + H1_nal_c_fld[__min(31,__max(0,(fld)-(1-1)))];
  c = c*4 + H1_nal_c_ctx[__min(31,__max(0,(ctx)-(1-1)))];

  d = 0;
  d = d*8 + H1_nal_d_fld[__min(31,__max(0,(fld)-(1-1)))];

  m = 0;
  m = m*4 + H1_nal_m_fld[__min(31,__max(0,(fld)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

