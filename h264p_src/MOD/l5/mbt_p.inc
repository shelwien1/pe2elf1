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
    mixer, `_G` the sign counters.  Each is H_mbt_ND (or H_mbt_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_mbt_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((st)>3+(1-1)));
  a = a*2 + (((ca)>1+(1-1)));
  a = a*7 + H5_mbt_a_cb[__min(8,__max(0,(cb)-(1-1)))];
  a = a*6 + H5_mbt_a_en[__min(14,__max(0,(en)-(1-1)))];

  b = 0;
  b = b*2 + (((st)>2+(1-1)));
  b = b*7 + H5_mbt_b_cc[__min(8,__max(0,(cc)-(1-1)))];
  b = b*5 + H5_mbt_b_td[__min(16,__max(0,(td)-(1-1)))];
  b = b*6 + H5_mbt_b_c1[__min(9,__max(0,(c1)-(1-1)))];

  c = 0;
  c = c*5 + H5_mbt_c_ca[__min(8,__max(0,(ca)-(1-1)))];
  c = c*4 + H5_mbt_c_cc[__min(8,__max(0,(cc)-(1-1)))];

  d = 0;
  d = d*4 + H5_mbt_d_st[__min(9,__max(0,(st)-(1-1)))];

  e = 0;
  e = e*7 + H5_mbt_e_ca[__min(8,__max(0,(ca)-(1-1)))];
  e = e*5 + H5_mbt_e_td[__min(16,__max(0,(td)-(1-1)))];

  h = 0;
  h = h*2 + (((st)>1+(1-1)));

  i = 0;
  i = i*4 + H5_mbt_i_st[__min(9,__max(0,(st)-(1-1)))];
  i = i*6 + H5_mbt_i_en[__min(14,__max(0,(en)-(1-1)))];
  i = i*5 + H5_mbt_i_ca[__min(8,__max(0,(ca)-(1-1)))];

  j = 0;
  j = j*5 + H5_mbt_j_cb[__min(8,__max(0,(cb)-(1-1)))];

  k = 0;
  k = k*4 + H5_mbt_k_st[__min(9,__max(0,(st)-(1-1)))];
  k = k*4 + H5_mbt_k_nsk[__min(4,__max(0,(nsk)-(1-1)))];
  k = k*5 + H5_mbt_k_cc[__min(8,__max(0,(cc)-(1-1)))];

  l = 0;
  l = l*2 + (((st)>1+(1-1)));
  l = l*8 + H5_mbt_l_ca[__min(8,__max(0,(ca)-(1-1)))];
  l = l*8 + H5_mbt_l_cb[__min(8,__max(0,(cb)-(1-1)))];

  m = 0;
  m = m*2 + (((st)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_mbt
#define FC_PFX H5_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
