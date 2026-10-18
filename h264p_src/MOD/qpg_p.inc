/*  The template IDX/idx2inc.pl reads alongside qpg.idx: the qpg
    family's index builder and the tables its indices address.  Copied
    through into MOD/qpg_p.inc with `H_qpg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/qpg_h.inc.  Edit this file and
    qpg.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_qpg_ND (or H_qpg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_qpg_make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int as = D.as, cs = D.cs, bs = D.bs, ls = D.ls, rs = D.rs, mag = D.mag, cls = D.cls, st = D.st;
  (void) as;  (void) cs;  (void) bs;  (void) ls;  (void) rs;  (void) mag;  (void) cls;  (void) st;

  a = 0;
  a = a*10 + H_qpg_a_as[__min(9,__max(0,as-(1-1)))];
  a = a*8 + H_qpg_a_mag[__min(7,__max(0,mag-(1-1)))];

  b = 0;
  b = b*10 + H_qpg_b_as[__min(9,__max(0,as-(1-1)))];

  c = 0;
  c = c*10 + H_qpg_c_as[__min(9,__max(0,as-(1-1)))];
  c = c*10 + H_qpg_c_cs[__min(9,__max(0,cs-(1-1)))];

  d = 0;
  d = d*10 + H_qpg_d_bs[__min(9,__max(0,bs-(1-1)))];
  d = d*8 + H_qpg_d_mag[__min(7,__max(0,mag-(1-1)))];

  e = 0;
  e = e*10 + H_qpg_e_cs[__min(9,__max(0,cs-(1-1)))];
  e = e*4 + H_qpg_e_rs[__min(3,__max(0,rs-(1-1)))];

  h = 0;
  h = h*10 + H_qpg_h_as[__min(9,__max(0,as-(1-1)))];
  h = h*4 + H_qpg_h_rs[__min(3,__max(0,rs-(1-1)))];
  h = h*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  i = 0;
  i = i*10 + H_qpg_i_as[__min(9,__max(0,as-(1-1)))];
  i = i*10 + H_qpg_i_cs[__min(9,__max(0,cs-(1-1)))];
  i = i*10 + H_qpg_i_bs[__min(9,__max(0,bs-(1-1)))];

  j = 0;
  j = j*9 + H_qpg_j_cls[__min(8,__max(0,cls-(1-1)))];
  j = j*10 + H_qpg_j_st[__min(9,__max(0,st-(1-1)))];
  j = j*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  k = 0;
  k = k*8 + H_qpg_k_mag[__min(7,__max(0,mag-(1-1)))];
  k = k*10 + H_qpg_k_as[__min(9,__max(0,as-(1-1)))];
  k = k*9 + H_qpg_k_cls[__min(8,__max(0,cls-(1-1)))];

  l = 0;
  l = l*10 + H_qpg_l_cs[__min(9,__max(0,cs-(1-1)))];
  l = l*10 + H_qpg_l_bs[__min(9,__max(0,bs-(1-1)))];
  l = l*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  s = 0;
  s = s*10 + H_qpg_s_as[__min(9,__max(0,as-(1-1)))];
  s = s*8 + H_qpg_s_mag[__min(7,__max(0,mag-(1-1)))];

  f = 0;
  f = f*10 + H_qpg_f_as[__min(9,__max(0,as-(1-1)))];
  f = f*10 + H_qpg_f_st[__min(9,__max(0,st-(1-1)))];

  m = 0;
  m = m*10 + H_qpg_m_as[__min(9,__max(0,as-(1-1)))];

  w = 0;
  w = w*10 + H_qpg_w_as[__min(9,__max(0,as-(1-1)))];
  w = w*10 + H_qpg_w_cs[__min(9,__max(0,cs-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

