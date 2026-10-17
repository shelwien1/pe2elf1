/*  The template IDX/idx2inc.pl reads alongside mbt.idx: the mbt
    family's index builder and the tables its indices address.  Copied
    through into MOD/mbt_p.inc with `H_mbt_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/mbt_h.inc.  Edit this file and
    mbt.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_mbt_ND (or H_mbt_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int st = D.st, ca = D.ca, cb = D.cb, cc = D.cc, nsk = D.nsk;
  (void) st;  (void) ca;  (void) cb;  (void) cc;  (void) nsk;

  a = 0;
  a = a*10 + H_mbt_a_st[__min(9,__max(0,st-(1-1)))];
  a = a*9 + H_mbt_a_ca[__min(8,__max(0,ca-(1-1)))];
  a = a*9 + H_mbt_a_cb[__min(8,__max(0,cb-(1-1)))];

  b = 0;
  b = b*10 + H_mbt_b_st[__min(9,__max(0,st-(1-1)))];
  b = b*9 + H_mbt_b_cc[__min(8,__max(0,cc-(1-1)))];
  b = b*5 + H_mbt_b_nsk[__min(4,__max(0,nsk-(1-1)))];

  c = 0;
  c = c*10 + H_mbt_c_st[__min(9,__max(0,st-(1-1)))];
  c = c*9 + H_mbt_c_ca[__min(8,__max(0,ca-(1-1)))];
  c = c*9 + H_mbt_c_cc[__min(8,__max(0,cc-(1-1)))];

  d = 0;
  d = d*10 + H_mbt_d_st[__min(9,__max(0,st-(1-1)))];

  s = 0;
  s = s*10 + H_mbt_s_st[__min(9,__max(0,st-(1-1)))];

  f = 0;
  f = f*10 + H_mbt_f_st[__min(9,__max(0,st-(1-1)))];
  f = f*3 + ((ca>5+(1-1))+(ca>6+(1-1)));

  m = 0;
  m = m*10 + H_mbt_m_st[__min(9,__max(0,st-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

