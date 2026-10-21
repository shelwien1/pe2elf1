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
    mixer, `_G` the sign counters.  Each is H_qpd_ND (or H_qpd_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_qpd_make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_qpd_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((pc)>0+(1-1))+((pc)>1+(1-1)));
  a = a*2 + (((cc)>1+(1-1)));
  a = a*2 + (((cls)>5+(1-1)));
  a = a*9 + H5_qpd_a_sq[__min(13,__max(0,(sq)-(1-1)))];

  b = 0;
  b = b*5 + H5_qpd_b_dc[__min(8,__max(0,(dc)-(1-1)))];
  b = b*3 + (((aq)>0+(1-1))+((aq)>7+(1-1)));
  b = b*9 + H5_qpd_b_ae[__min(13,__max(0,(ae)-(1-1)))];
  b = b*6 + H5_qpd_b_lq[__min(13,__max(0,(lq)-(1-1)))];

  c = 0;
  c = c*5 + H5_qpd_c_lz[__min(6,__max(0,(lz)-(1-1)))];
  c = c*3 + (((st)>1+(1-1))+((st)>4+(1-1)));
  c = c*6 + H5_qpd_c_db[__min(8,__max(0,(db)-(1-1)))];
  c = c*7 + H5_qpd_c_bc[__min(13,__max(0,(bc)-(1-1)))];

  d = 0;
  d = d*4 + H5_qpd_d_st[__min(9,__max(0,(st)-(1-1)))];
  d = d*8 + H5_qpd_d_aq[__min(13,__max(0,(aq)-(1-1)))];
  d = d*5 + H5_qpd_d_ac[__min(13,__max(0,(ac)-(1-1)))];

  e = 0;
  e = e*8 + H5_qpd_e_aq[__min(13,__max(0,(aq)-(1-1)))];
  e = e*4 + H5_qpd_e_cls[__min(8,__max(0,(cls)-(1-1)))];
  e = e*7 + H5_qpd_e_sq[__min(13,__max(0,(sq)-(1-1)))];
  e = e*6 + H5_qpd_e_xe[__min(18,__max(0,(xe)-(1-1)))];

  h = 0;
  h = h*9 + H5_qpd_h_ae[__min(13,__max(0,(ae)-(1-1)))];

  i = 0;
  i = i*10 + H5_qpd_i_aq[__min(13,__max(0,(aq)-(1-1)))];
  i = i*5 + H5_qpd_i_ac[__min(13,__max(0,(ac)-(1-1)))];
  i = i*3 + (((cls)>5+(1-1))+((cls)>6+(1-1)));
  i = i*5 + H5_qpd_i_td[__min(16,__max(0,(td)-(1-1)))];

  j = 0;
  j = j*3 + (((st)>1+(1-1))+((st)>4+(1-1)));
  j = j*4 + H5_qpd_j_db[__min(8,__max(0,(db)-(1-1)))];
  j = j*2 + (((lnz)>0+(1-1)));
  j = j*3 + (((td)>3+(1-1))+((td)>13+(1-1)));
  j = j*10 + H5_qpd_j_lq[__min(13,__max(0,(lq)-(1-1)))];

  k = 0;
  k = k*7 + H5_qpd_k_aq[__min(13,__max(0,(aq)-(1-1)))];
  k = k*2 + (((lnz)>0+(1-1)));
  k = k*3 + (((pc)>0+(1-1))+((pc)>3+(1-1)));
  k = k*9 + H5_qpd_k_kc[__min(13,__max(0,(kc)-(1-1)))];

  l = 0;
  l = l*3 + (((ac)>0+(1-1))+((ac)>8+(1-1)));
  l = l*5 + H5_qpd_l_db[__min(8,__max(0,(db)-(1-1)))];
  l = l*3 + (((st)>1+(1-1))+((st)>3+(1-1)));
  l = l*9 + H5_qpd_l_cl[__min(13,__max(0,(cl)-(1-1)))];

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_qpd
#define FC_PFX H5_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
