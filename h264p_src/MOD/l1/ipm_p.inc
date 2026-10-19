/*  The template IDX/idx2inc.pl reads alongside ipm.idx: the ipm
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/ipm_p.inc with `H1_ipm_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/ipm_h.inc.  Edit this file
    and ipm.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_ipm_ND (or H_ipm_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_ipm_make( const H_ipm_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int k8 = D.k8, ma = D.ma, mb = D.mb, mp = D.mp, blk = D.blk, pv = D.pv, col = D.col, dm = D.dm, ds = D.ds, pm = D.pm, pr = D.pr, dr = D.dr, pq = D.pq, mh = D.mh, mc = D.mc, mg = D.mg, rg = D.rg, p2 = D.p2;
  (void) k8;  (void) ma;  (void) mb;  (void) mp;  (void) blk;  (void) pv;  (void) col;  (void) dm;  (void) ds;  (void) pm;  (void) pr;  (void) dr;  (void) pq;  (void) mh;  (void) mc;  (void) mg;  (void) rg;  (void) p2;

  a = 0;
  a = a*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  a = a*11 + H1_ipm_a_ma[__min(10,__max(0,ma-(1-1)))];
  a = a*11 + H1_ipm_a_mb[__min(10,__max(0,mb-(1-1)))];
  a = a*11 + H1_ipm_a_mc[__min(10,__max(0,mc-(1-1)))];

  b = 0;
  b = b*2 + ((k8>0+(1-1)));
  b = b*10 + H1_ipm_b_mp[__min(9,__max(0,mp-(1-1)))];
  b = b*10 + H1_ipm_b_pv[__min(9,__max(0,pv-(1-1)))];
  b = b*17 + H1_ipm_b_mh[__min(16,__max(0,mh-(1-1)))];

  c = 0;
  c = c*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  c = c*10 + H1_ipm_c_mp[__min(9,__max(0,mp-(1-1)))];
  c = c*10 + H1_ipm_c_col[__min(9,__max(0,col-(1-1)))];

  d = 0;
  d = d*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  d = d*10 + H1_ipm_d_mp[__min(9,__max(0,mp-(1-1)))];
  d = d*10 + H1_ipm_d_dm[__min(9,__max(0,dm-(1-1)))];
  d = d*3 + ((ds>4+(1-1))+(ds>7+(1-1)));

  m = 0;
  m = m*13 + H1_ipm_m_mg[__min(12,__max(0,mg-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

