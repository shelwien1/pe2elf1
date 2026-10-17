/*  The template IDX/idx2inc.pl reads alongside nal.idx: the nal
    family's index builder and the tables its indices address.  Copied
    through into MOD/nal_p.inc with `H_nal_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/nal_h.inc.  Edit this file and
    nal.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_nal_ND (or H_nal_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_nal_make( const H_nal_v & D, tcx & X ) {
  int a, b, c, d, e, h, s, f, m, w, t, n, g;
  int fld = D.fld, ctx = D.ctx, p1 = D.p1, p2 = D.p2;
  (void) fld;  (void) ctx;  (void) p1;  (void) p2;

  a = 0;
  a = a*6 + H_nal_a_fld[__min(31,__max(0,fld-(1-1)))];
  a = a*16 + H_nal_a_ctx[__min(15,__max(0,ctx-(1-1)))];

  b = 0;
  b = b*32 + H_nal_b_fld[__min(31,__max(0,fld-(1-1)))];
  b = b*7 + H_nal_b_p1[__min(6,__max(0,p1-(1-1)))];
  b = b*4 + H_nal_b_p2[__min(3,__max(0,p2-(1-1)))];

  c = 0;
  c = c*32 + H_nal_c_fld[__min(31,__max(0,fld-(1-1)))];
  c = c*3 + ((ctx>6+(1-1))+(ctx>8+(1-1)));

  d = 0;
  d = d*32 + H_nal_d_fld[__min(31,__max(0,fld-(1-1)))];

  e = 0;
  e = e*6 + H_nal_e_fld[__min(31,__max(0,fld-(1-1)))];
  e = e*4 + H_nal_e_p2[__min(3,__max(0,p2-(1-1)))];

  h = 0;
  h = h*6 + H_nal_h_fld[__min(31,__max(0,fld-(1-1)))];
  h = h*3 + ((ctx>6+(1-1))+(ctx>7+(1-1)));

  s = 0;
  s = s*5 + H_nal_s_fld[__min(31,__max(0,fld-(1-1)))];

  f = 0;
  f = f*32 + H_nal_f_fld[__min(31,__max(0,fld-(1-1)))];

  m = 0;
  m = m*4 + H_nal_m_fld[__min(31,__max(0,fld-(1-1)))];

  w = 0;
  w = w*32 + H_nal_w_fld[__min(31,__max(0,fld-(1-1)))];

  t = 0;
  t = t*32 + H_nal_t_fld[__min(31,__max(0,fld-(1-1)))];

  n = 0;
  n = n*32 + H_nal_n_fld[__min(31,__max(0,fld-(1-1)))];

  g = 0;
  g = g*32 + H_nal_g_fld[__min(31,__max(0,fld-(1-1)))];
  g = g*4 + H_nal_g_ctx[__min(3,__max(0,ctx-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

