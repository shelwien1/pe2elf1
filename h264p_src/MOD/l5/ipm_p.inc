/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a
    family's index builder, the tables its indices address and its
    configuration class, at level -5.  Copied through into
    MOD/l5/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_ipm_ND (or H_ipm_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_ipm_make( const H_ipm_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_ipm_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((k8)>0+(1-1)));
  a = a*11 + H5_ipm_a_ma[__min(10,__max(0,(ma)-(1-1)))];
  a = a*11 + H5_ipm_a_mb[__min(10,__max(0,(mb)-(1-1)))];
  a = a*10 + H5_ipm_a_mc[__min(10,__max(0,(mc)-(1-1)))];

  b = 0;
  b = b*2 + (((k8)>0+(1-1)));
  b = b*6 + H5_ipm_b_mp[__min(9,__max(0,(mp)-(1-1)))];
  b = b*9 + H5_ipm_b_pv[__min(9,__max(0,(pv)-(1-1)))];
  b = b*17 + H5_ipm_b_mh[__min(16,__max(0,(mh)-(1-1)))];

  c = 0;
  c = c*8 + H5_ipm_c_mp[__min(9,__max(0,(mp)-(1-1)))];
  c = c*10 + H5_ipm_c_col[__min(9,__max(0,(col)-(1-1)))];

  d = 0;
  d = d*2 + (((k8)>0+(1-1)));
  d = d*8 + H5_ipm_d_mp[__min(9,__max(0,(mp)-(1-1)))];
  d = d*8 + H5_ipm_d_dm[__min(9,__max(0,(dm)-(1-1)))];
  d = d*4 + H5_ipm_d_ds[__min(9,__max(0,(ds)-(1-1)))];

  e = 0;
  e = e*2 + (((mp)>4+(1-1)));

  h = 0;
  h = h*9 + H5_ipm_h_dr[__min(8,__max(0,(dr)-(1-1)))];
  h = h*4 + H5_ipm_h_ds[__min(9,__max(0,(ds)-(1-1)))];

  i = 0;
  i = i*4 + H5_ipm_i_dm[__min(9,__max(0,(dm)-(1-1)))];
  i = i*9 + H5_ipm_i_ma[__min(10,__max(0,(ma)-(1-1)))];

  j = 0;
  j = j*2 + (((k8)>0+(1-1)));
  j = j*17 + H5_ipm_j_mh[__min(16,__max(0,(mh)-(1-1)))];
  j = j*10 + H5_ipm_j_mb[__min(10,__max(0,(mb)-(1-1)))];

  k = 0;
  k = k*2 + (((mp)>5+(1-1)));
  k = k*6 + H5_ipm_k_pv[__min(9,__max(0,(pv)-(1-1)))];

  l = 0;
  l = l*10 + H5_ipm_l_mc[__min(10,__max(0,(mc)-(1-1)))];
  l = l*9 + H5_ipm_l_ma[__min(10,__max(0,(ma)-(1-1)))];
  l = l*8 + H5_ipm_l_dm[__min(9,__max(0,(dm)-(1-1)))];

  m = 0;
  m = m*2 + (((k8)>0+(1-1)));
  m = m*2 + (((ma)>9+(1-1)));
  m = m*2 + (((mb)>9+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_ipm
#define FC_PFX H5_ipm_
#define FC_FAM ipm
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
