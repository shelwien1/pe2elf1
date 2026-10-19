/*  The template IDX/idx2inc.pl reads alongside mvg.idx: the mvg
    family's index builder and the tables its indices address.  Copied
    through into MOD/l3/mvg_p.inc with `H3_mvg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l3/mvg_h.inc.  Edit this file
    and mvg.idx, not MOD/; ../../mk.sh regenerates both.

    Level -3: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_mvg_ND (or H_mvg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H3_mvg_make( const H_mvg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int comp = D.comp, lst = D.lst, ts = D.ts, zs = D.zs, ds = D.ds, sa = D.sa, sb = D.sb, mag = D.mag, tq = D.tq, ox = D.ox, ns = D.ns, es = D.es, as = D.as;
  (void) comp;  (void) lst;  (void) ts;  (void) zs;  (void) ds;  (void) sa;  (void) sb;  (void) mag;  (void) tq;  (void) ox;  (void) ns;  (void) es;  (void) as;

  a = 0;
  a = a*2 + ((comp>0+(1-1)));
  a = a*8 + H3_mvg_a_mag[__min(7,__max(0,mag-(1-1)))];

  b = 0;
  b = b*9 + H3_mvg_b_zs[__min(8,__max(0,zs-(1-1)))];
  b = b*8 + H3_mvg_b_mag[__min(7,__max(0,mag-(1-1)))];
  b = b*2 + ((comp>0+(1-1)));

  c = 0;
  c = c*9 + H3_mvg_c_zs[__min(8,__max(0,zs-(1-1)))];

  d = 0;
  d = d*10 + H3_mvg_d_ds[__min(9,__max(0,ds-(1-1)))];
  d = d*8 + H3_mvg_d_mag[__min(7,__max(0,mag-(1-1)))];

  e = 0;
  e = e*3 + ((sa>0+(1-1))+(sa>1+(1-1)));
  e = e*3 + ((sb>0+(1-1))+(sb>1+(1-1)));
  e = e*2 + ((comp>0+(1-1)));
  e = e*7 + H3_mvg_e_ns[__min(6,__max(0,ns-(1-1)))];

  h = 0;

  i = 0;
  i = i*9 + H3_mvg_i_zs[__min(8,__max(0,zs-(1-1)))];
  i = i*10 + H3_mvg_i_ds[__min(9,__max(0,ds-(1-1)))];

  j = 0;
  j = j*2 + ((comp>0+(1-1)));
  j = j*2 + ((lst>0+(1-1)));
  j = j*4 + H3_mvg_j_ox[__min(3,__max(0,ox-(1-1)))];
  j = j*7 + H3_mvg_j_ns[__min(6,__max(0,ns-(1-1)))];

  k = 0;
  k = k*8 + H3_mvg_k_mag[__min(7,__max(0,mag-(1-1)))];
  k = k*3 + ((sa>0+(1-1))+(sa>1+(1-1)));

  l = 0;
  l = l*9 + H3_mvg_l_zs[__min(8,__max(0,zs-(1-1)))];
  l = l*10 + H3_mvg_l_ds[__min(9,__max(0,ds-(1-1)))];
  l = l*3 + ((sb>0+(1-1))+(sb>1+(1-1)));

  s = 0;
  s = s*8 + H3_mvg_s_mag[__min(7,__max(0,mag-(1-1)))];

  m = 0;

  w = 0;
  w = w*9 + H3_mvg_w_zs[__min(8,__max(0,zs-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

