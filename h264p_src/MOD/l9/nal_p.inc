/*  The template IDX/idx2inc.pl reads alongside nal.idx: the nal
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/nal_p.inc with `H9_nal_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/nal_h.inc.  Edit this file
    and nal.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_nal_ND (or H_nal_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_nal_make( const H_nal_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int fld = D.fld, ctx = D.ctx, p1 = D.p1, p2 = D.p2;
  (void) fld;  (void) ctx;  (void) p1;  (void) p2;

  a = 0;
  a = a*6 + H9_nal_a_fld[__min(31,__max(0,(fld)-(1-1)))];
  a = a*3 + (((ctx)>2+(1-1))+((ctx)>8+(1-1)));
  a = a*3 + (((p1)>4+(1-1))+((p1)>5+(1-1)));

  b = 0;
  b = b*6 + H9_nal_b_fld[__min(31,__max(0,(fld)-(1-1)))];
  b = b*3 + (((p1)>0+(1-1))+((p1)>5+(1-1)));
  b = b*4 + H9_nal_b_p2[__min(3,__max(0,(p2)-(1-1)))];

  c = 0;
  c = c*10 + H9_nal_c_fld[__min(31,__max(0,(fld)-(1-1)))];
  c = c*4 + H9_nal_c_ctx[__min(31,__max(0,(ctx)-(1-1)))];

  d = 0;
  d = d*8 + H9_nal_d_fld[__min(31,__max(0,(fld)-(1-1)))];

  e = 0;
  e = e*6 + H9_nal_e_fld[__min(31,__max(0,(fld)-(1-1)))];
  e = e*2 + (((p2)>0+(1-1)));

  h = 0;
  h = h*6 + H9_nal_h_fld[__min(31,__max(0,(fld)-(1-1)))];
  h = h*3 + (((ctx)>1+(1-1))+((ctx)>5+(1-1)));

  i = 0;
  i = i*6 + H9_nal_i_ctx[__min(31,__max(0,(ctx)-(1-1)))];
  i = i*5 + H9_nal_i_p1[__min(15,__max(0,(p1)-(1-1)))];

  j = 0;
  j = j*5 + H9_nal_j_fld[__min(31,__max(0,(fld)-(1-1)))];
  j = j*3 + (((p2)>1+(1-1))+((p2)>2+(1-1)));
  j = j*5 + H9_nal_j_p1[__min(15,__max(0,(p1)-(1-1)))];

  k = 0;
  k = k*8 + H9_nal_k_fld[__min(31,__max(0,(fld)-(1-1)))];
  k = k*4 + H9_nal_k_ctx[__min(31,__max(0,(ctx)-(1-1)))];

  l = 0;
  l = l*6 + H9_nal_l_p1[__min(15,__max(0,(p1)-(1-1)))];
  l = l*3 + (((p2)>1+(1-1))+((p2)>2+(1-1)));

  s = 0;
  s = s*5 + H9_nal_s_fld[__min(31,__max(0,(fld)-(1-1)))];

  f = 0;
  f = f*8 + H9_nal_f_fld[__min(31,__max(0,(fld)-(1-1)))];

  m = 0;
  m = m*7 + H9_nal_m_fld[__min(31,__max(0,(fld)-(1-1)))];

  w = 0;
  w = w*8 + H9_nal_w_fld[__min(31,__max(0,(fld)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

