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
    mixer, `_G` the sign counters.  Each is H_qpg_ND (or H_qpg_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_qpg_make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_qpg_VARS(H_LOCAL)

  a = 0;
  a = a*5 + H5_qpg_a_mag[__min(7,__max(0,(mag)-(1-1)))];
  a = a*6 + H5_qpg_a_ss[__min(9,__max(0,(ss)-(1-1)))];
  a = a*5 + H5_qpg_a_xe[__min(18,__max(0,(xe)-(1-1)))];
  a = a*4 + H5_qpg_a_lq[__min(9,__max(0,(lq)-(1-1)))];

  b = 0;
  b = b*3 + (((as)>2+(1-1))+((as)>5+(1-1)));

  c = 0;
  c = c*3 + (((as)>5+(1-1))+((as)>7+(1-1)));
  c = c*6 + H5_qpg_c_cs[__min(9,__max(0,(cs)-(1-1)))];

  d = 0;
  d = d*7 + H5_qpg_d_bs[__min(9,__max(0,(bs)-(1-1)))];
  d = d*3 + (((mag)>2+(1-1))+((mag)>4+(1-1)));
  d = d*2 + (((bk)>8+(1-1)));

  e = 0;
  e = e*4 + H5_qpg_e_cs[__min(9,__max(0,(cs)-(1-1)))];
  e = e*2 + (((rs)>2+(1-1)));
  e = e*6 + H5_qpg_e_ss[__min(9,__max(0,(ss)-(1-1)))];

  h = 0;
  h = h*4 + H5_qpg_h_as[__min(9,__max(0,(as)-(1-1)))];
  h = h*4 + H5_qpg_h_rs[__min(3,__max(0,(rs)-(1-1)))];
  h = h*2 + (((ls)>0+(1-1)));

  i = 0;
  i = i*6 + H5_qpg_i_as[__min(9,__max(0,(as)-(1-1)))];
  i = i*5 + H5_qpg_i_cs[__min(9,__max(0,(cs)-(1-1)))];
  i = i*7 + H5_qpg_i_bs[__min(9,__max(0,(bs)-(1-1)))];

  j = 0;
  j = j*4 + H5_qpg_j_cls[__min(8,__max(0,(cls)-(1-1)))];
  j = j*4 + H5_qpg_j_st[__min(9,__max(0,(st)-(1-1)))];
  j = j*3 + (((ls)>0+(1-1))+((ls)>1+(1-1)));

  k = 0;
  k = k*4 + H5_qpg_k_mag[__min(7,__max(0,(mag)-(1-1)))];
  k = k*8 + H5_qpg_k_as[__min(9,__max(0,(as)-(1-1)))];
  k = k*3 + (((cls)>5+(1-1))+((cls)>6+(1-1)));

  l = 0;
  l = l*7 + H5_qpg_l_cs[__min(9,__max(0,(cs)-(1-1)))];
  l = l*6 + H5_qpg_l_bs[__min(9,__max(0,(bs)-(1-1)))];

  m = 0;
  m = m*3 + (((as)>2+(1-1))+((as)>5+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_qpg
#define FC_PFX H5_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
