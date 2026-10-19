/*  The template IDX/idx2inc.pl reads alongside nal.idx: the nal
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/nal_p.inc with `H5_nal_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/nal_h.inc.  Edit this file
    and nal.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_nal_ND (or H_nal_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H5_nal_make( const H_nal_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int fld = D.fld, ctx = D.ctx, p1 = D.p1, p2 = D.p2;
  (void) fld;  (void) ctx;  (void) p1;  (void) p2;

  a = 0;
  a = a*7 + H5_nal_a_fld[__min(31,__max(0,fld-(1-1)))];
  a = a*5 + H5_nal_a_ctx[__min(15,__max(0,ctx-(1-1)))];

  b = 0;
  b = b*6 + H5_nal_b_fld[__min(31,__max(0,fld-(1-1)))];
  b = b*4 + H5_nal_b_p1[__min(6,__max(0,p1-(1-1)))];
  b = b*4 + H5_nal_b_p2[__min(3,__max(0,p2-(1-1)))];

  c = 0;
  c = c*6 + H5_nal_c_fld[__min(31,__max(0,fld-(1-1)))];
  c = c*5 + H5_nal_c_ctx[__min(31,__max(0,ctx-(1-1)))];

  d = 0;
  d = d*9 + H5_nal_d_fld[__min(31,__max(0,fld-(1-1)))];

  e = 0;
  e = e*5 + H5_nal_e_fld[__min(31,__max(0,fld-(1-1)))];

  h = 0;
  h = h*6 + H5_nal_h_fld[__min(31,__max(0,fld-(1-1)))];
  h = h*3 + ((ctx>1+(1-1))+(ctx>8+(1-1)));

  i = 0;
  i = i*4 + H5_nal_i_ctx[__min(31,__max(0,ctx-(1-1)))];
  i = i*5 + H5_nal_i_p1[__min(15,__max(0,p1-(1-1)))];

  j = 0;
  j = j*6 + H5_nal_j_fld[__min(31,__max(0,fld-(1-1)))];
  j = j*3 + ((p1>4+(1-1))+(p1>5+(1-1)));

  k = 0;
  k = k*6 + H5_nal_k_fld[__min(31,__max(0,fld-(1-1)))];
  k = k*6 + H5_nal_k_ctx[__min(31,__max(0,ctx-(1-1)))];

  l = 0;
  l = l*5 + H5_nal_l_p1[__min(15,__max(0,p1-(1-1)))];

  m = 0;
  m = m*4 + H5_nal_m_fld[__min(31,__max(0,fld-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

