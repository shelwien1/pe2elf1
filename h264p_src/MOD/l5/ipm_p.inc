/*  The template IDX/idx2inc.pl reads alongside ipm.idx: the ipm
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/ipm_p.inc with `H5_ipm_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/ipm_h.inc.  Edit this file
    and ipm.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_ipm_ND (or H_ipm_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H5_ipm_make( const H_ipm_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int k8 = D.k8, ma = D.ma, mb = D.mb, mp = D.mp, blk = D.blk, pv = D.pv, col = D.col, dm = D.dm, ds = D.ds, pm = D.pm, pr = D.pr, dr = D.dr, pq = D.pq, mh = D.mh, mc = D.mc, mg = D.mg, rg = D.rg, p2 = D.p2;
  (void) k8;  (void) ma;  (void) mb;  (void) mp;  (void) blk;  (void) pv;  (void) col;  (void) dm;  (void) ds;  (void) pm;  (void) pr;  (void) dr;  (void) pq;  (void) mh;  (void) mc;  (void) mg;  (void) rg;  (void) p2;

  a = 0;
  a = a*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  a = a*11 + H5_ipm_a_ma[__min(10,__max(0,ma-(1-1)))];
  a = a*11 + H5_ipm_a_mb[__min(10,__max(0,mb-(1-1)))];
  a = a*11 + H5_ipm_a_mc[__min(10,__max(0,mc-(1-1)))];

  b = 0;
  b = b*2 + ((k8>0+(1-1)));
  b = b*10 + H5_ipm_b_mp[__min(9,__max(0,mp-(1-1)))];
  b = b*10 + H5_ipm_b_pv[__min(9,__max(0,pv-(1-1)))];
  b = b*17 + H5_ipm_b_mh[__min(16,__max(0,mh-(1-1)))];

  c = 0;
  c = c*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  c = c*10 + H5_ipm_c_mp[__min(9,__max(0,mp-(1-1)))];
  c = c*10 + H5_ipm_c_col[__min(9,__max(0,col-(1-1)))];

  d = 0;
  d = d*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  d = d*10 + H5_ipm_d_mp[__min(9,__max(0,mp-(1-1)))];
  d = d*10 + H5_ipm_d_dm[__min(9,__max(0,dm-(1-1)))];
  d = d*3 + ((ds>4+(1-1))+(ds>7+(1-1)));

  e = 0;
  e = e*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  e = e*10 + H5_ipm_e_mp[__min(9,__max(0,mp-(1-1)))];

  h = 0;
  h = h*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  h = h*10 + H5_ipm_h_ds[__min(9,__max(0,ds-(1-1)))];

  i = 0;
  i = i*4 + H5_ipm_i_dm[__min(9,__max(0,dm-(1-1)))];
  i = i*11 + H5_ipm_i_ma[__min(10,__max(0,ma-(1-1)))];

  j = 0;
  j = j*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  j = j*17 + H5_ipm_j_mh[__min(16,__max(0,mh-(1-1)))];
  j = j*11 + H5_ipm_j_mb[__min(10,__max(0,mb-(1-1)))];
  j = j*9 + H5_ipm_j_p2[__min(8,__max(0,p2-(1-1)))];

  k = 0;
  k = k*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  k = k*10 + H5_ipm_k_mp[__min(9,__max(0,mp-(1-1)))];
  k = k*10 + H5_ipm_k_pv[__min(9,__max(0,pv-(1-1)))];

  l = 0;
  l = l*3 + ((k8>0+(1-1))+(k8>1+(1-1)));
  l = l*11 + H5_ipm_l_mc[__min(10,__max(0,mc-(1-1)))];
  l = l*11 + H5_ipm_l_ma[__min(10,__max(0,ma-(1-1)))];
  l = l*8 + H5_ipm_l_dm[__min(9,__max(0,dm-(1-1)))];

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

