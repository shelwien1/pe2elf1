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
    mixer, `_G` the sign counters.  Each is H_mvs_ND (or H_mvs_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_mvs_make( const H_mvs_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_mvs_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  a = a*2 + (((comp)>0+(1-1)));
  a = a*4 + H5_mvs_a_ma[__min(7,__max(0,(ma)-(1-1)))];
  a = a*4 + H5_mvs_a_mb[__min(7,__max(0,(mb)-(1-1)))];
  a = a*5 + H5_mvs_a_oth[__min(5,__max(0,(oth)-(1-1)))];
  a = a*4 + H5_mvs_a_sp[__min(8,__max(0,(sp)-(1-1)))];
  a = a*5 + H5_mvs_a_rt[__min(14,__max(0,(rt)-(1-1)))];

  b = 0;
  b = b*2 + (((kind)>2+(1-1)));
  b = b*2 + (((comp)>0+(1-1)));
  b = b*2 + (((lst)>0+(1-1)));
  b = b*4 + H5_mvs_b_ps[__min(3,__max(0,(ps)-(1-1)))];
  b = b*4 + H5_mvs_b_rf[__min(3,__max(0,(rf)-(1-1)))];
  b = b*9 + H5_mvs_b_zd[__min(12,__max(0,(zd)-(1-1)))];

  c = 0;
  c = c*3 + (((kind)>0+(1-1))+((kind)>2+(1-1)));
  c = c*7 + H5_mvs_c_pd[__min(8,__max(0,(pd)-(1-1)))];
  c = c*5 + H5_mvs_c_sp[__min(8,__max(0,(sp)-(1-1)))];
  c = c*7 + H5_mvs_c_zd[__min(12,__max(0,(zd)-(1-1)))];

  d = 0;
  d = d*2 + (((kind)>2+(1-1)));
  d = d*2 + (((comp)>0+(1-1)));
  d = d*7 + H5_mvs_d_pd[__min(8,__max(0,(pd)-(1-1)))];
  d = d*4 + H5_mvs_d_nr[__min(3,__max(0,(nr)-(1-1)))];

  e = 0;
  e = e*2 + (((kind)>2+(1-1)));
  e = e*2 + (((sp)>0+(1-1)));
  e = e*10 + H5_mvs_e_rt[__min(14,__max(0,(rt)-(1-1)))];

  h = 0;
  h = h*3 + (((kind)>0+(1-1))+((kind)>2+(1-1)));
  h = h*5 + H5_mvs_h_oth[__min(5,__max(0,(oth)-(1-1)))];

  i = 0;
  i = i*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  i = i*2 + (((comp)>0+(1-1)));
  i = i*3 + (((zd)>4+(1-1))+((zd)>8+(1-1)));

  j = 0;
  j = j*4 + H5_mvs_j_kind[__min(3,__max(0,(kind)-(1-1)))];
  j = j*2 + (((comp)>0+(1-1)));
  j = j*7 + H5_mvs_j_ma[__min(7,__max(0,(ma)-(1-1)))];
  j = j*6 + H5_mvs_j_mb[__min(7,__max(0,(mb)-(1-1)))];
  j = j*5 + H5_mvs_j_cq[__min(9,__max(0,(cq)-(1-1)))];

  k = 0;
  k = k*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  k = k*7 + H5_mvs_k_pd[__min(8,__max(0,(pd)-(1-1)))];
  k = k*5 + H5_mvs_k_sp[__min(8,__max(0,(sp)-(1-1)))];
  k = k*4 + H5_mvs_k_ps[__min(3,__max(0,(ps)-(1-1)))];

  l = 0;
  l = l*4 + H5_mvs_l_kind[__min(3,__max(0,(kind)-(1-1)))];

  m = 0;
  m = m*4 + H5_mvs_m_kind[__min(3,__max(0,(kind)-(1-1)))];

  t = 0;
  t = t*2 + (((kind)>2+(1-1)));
  t = t*2 + (((comp)>0+(1-1)));

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_mvs
#define FC_PFX H5_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
