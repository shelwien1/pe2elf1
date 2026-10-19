/*  The template IDX/idx2inc.pl reads alongside mvg.idx: the mvg
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/mvg_p.inc with `H1_mvg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/mvg_h.inc.  Edit this file
    and mvg.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_mvg_ND (or H_mvg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_mvg_make( const H_mvg_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int comp = D.comp, lst = D.lst, ts = D.ts, zs = D.zs, ds = D.ds, sa = D.sa, sb = D.sb, mag = D.mag, tq = D.tq, ox = D.ox, ns = D.ns, es = D.es, as = D.as;
  (void) comp;  (void) lst;  (void) ts;  (void) zs;  (void) ds;  (void) sa;  (void) sb;  (void) mag;  (void) tq;  (void) ox;  (void) ns;  (void) es;  (void) as;

  a = 0;
  a = a*2 + ((comp>0+(1-1)));
  a = a*3 + ((ts>2+(1-1))+(ts>5+(1-1)));
  a = a*8 + H1_mvg_a_mag[__min(7,__max(0,mag-(1-1)))];

  b = 0;
  b = b*9 + H1_mvg_b_zs[__min(8,__max(0,zs-(1-1)))];
  b = b*8 + H1_mvg_b_mag[__min(7,__max(0,mag-(1-1)))];
  b = b*2 + ((comp>0+(1-1)));

  c = 0;
  c = c*10 + H1_mvg_c_ts[__min(9,__max(0,ts-(1-1)))];
  c = c*9 + H1_mvg_c_zs[__min(8,__max(0,zs-(1-1)))];
  c = c*10 + H1_mvg_c_as[__min(9,__max(0,as-(1-1)))];

  d = 0;
  d = d*10 + H1_mvg_d_ds[__min(9,__max(0,ds-(1-1)))];
  d = d*8 + H1_mvg_d_mag[__min(7,__max(0,mag-(1-1)))];

  m = 0;
  m = m*10 + H1_mvg_m_ts[__min(9,__max(0,ts-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

