/*  The template IDX/idx2inc.pl reads alongside mvg.idx: the mvg
    family's index builder and the tables its indices address.  Copied
    through into MOD/mvg_p.inc with `H_mvg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/mvg_h.inc.  Edit this file and
    mvg.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_mvg_ND (or H_mvg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_mvg_make( const H_mvg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int comp = D.comp, lst = D.lst, ts = D.ts, zs = D.zs, ds = D.ds, sa = D.sa, sb = D.sb, mag = D.mag, tq = D.tq, ox = D.ox, ns = D.ns;
  (void) comp;  (void) lst;  (void) ts;  (void) zs;  (void) ds;  (void) sa;  (void) sb;  (void) mag;  (void) tq;  (void) ox;  (void) ns;

  a = 0;
  a = a*2 + ((comp>0+(1-1)));
  a = a*10 + H_mvg_a_ts[__min(9,__max(0,ts-(1-1)))];
  a = a*8 + H_mvg_a_mag[__min(7,__max(0,mag-(1-1)))];

  b = 0;
  b = b*9 + H_mvg_b_zs[__min(8,__max(0,zs-(1-1)))];
  b = b*8 + H_mvg_b_mag[__min(7,__max(0,mag-(1-1)))];
  b = b*2 + ((comp>0+(1-1)));

  c = 0;
  c = c*10 + H_mvg_c_ts[__min(9,__max(0,ts-(1-1)))];
  c = c*9 + H_mvg_c_zs[__min(8,__max(0,zs-(1-1)))];

  d = 0;
  d = d*10 + H_mvg_d_ds[__min(9,__max(0,ds-(1-1)))];
  d = d*8 + H_mvg_d_mag[__min(7,__max(0,mag-(1-1)))];

  e = 0;
  e = e*3 + ((sa>0+(1-1))+(sa>1+(1-1)));
  e = e*3 + ((sb>0+(1-1))+(sb>1+(1-1)));
  e = e*2 + ((comp>0+(1-1)));
  e = e*7 + H_mvg_e_ns[__min(6,__max(0,ns-(1-1)))];

  h = 0;
  h = h*10 + H_mvg_h_ts[__min(9,__max(0,ts-(1-1)))];
  h = h*13 + H_mvg_h_tq[__min(12,__max(0,tq-(1-1)))];

  i = 0;
  i = i*10 + H_mvg_i_ts[__min(9,__max(0,ts-(1-1)))];
  i = i*9 + H_mvg_i_zs[__min(8,__max(0,zs-(1-1)))];
  i = i*10 + H_mvg_i_ds[__min(9,__max(0,ds-(1-1)))];

  j = 0;
  j = j*2 + ((comp>0+(1-1)));
  j = j*2 + ((lst>0+(1-1)));
  j = j*4 + H_mvg_j_ox[__min(3,__max(0,ox-(1-1)))];
  j = j*7 + H_mvg_j_ns[__min(6,__max(0,ns-(1-1)))];

  k = 0;
  k = k*8 + H_mvg_k_mag[__min(7,__max(0,mag-(1-1)))];
  k = k*10 + H_mvg_k_ts[__min(9,__max(0,ts-(1-1)))];
  k = k*3 + ((sa>0+(1-1))+(sa>1+(1-1)));

  l = 0;
  l = l*9 + H_mvg_l_zs[__min(8,__max(0,zs-(1-1)))];
  l = l*10 + H_mvg_l_ds[__min(9,__max(0,ds-(1-1)))];
  l = l*3 + ((sb>0+(1-1))+(sb>1+(1-1)));

  s = 0;
  s = s*10 + H_mvg_s_ts[__min(9,__max(0,ts-(1-1)))];
  s = s*8 + H_mvg_s_mag[__min(7,__max(0,mag-(1-1)))];

  f = 0;
  f = f*2 + ((comp>0+(1-1)));
  f = f*10 + H_mvg_f_ts[__min(9,__max(0,ts-(1-1)))];

  m = 0;
  m = m*10 + H_mvg_m_ts[__min(9,__max(0,ts-(1-1)))];

  w = 0;
  w = w*9 + H_mvg_w_zs[__min(8,__max(0,zs-(1-1)))];
  w = w*10 + H_mvg_w_ts[__min(9,__max(0,ts-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

