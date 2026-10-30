/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_skp_ND (or M_skp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_skp_make( const M_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_skp_VARS(M_LOCAL)

  a = 0;
  a = a*4 + M5_skp_a_st[__min(3,__max(0,(st)-(1-1)))];
  a = a*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));
  a = a*3 + (((rt)>0+(1-1))+((rt)>1+(1-1)));

  b = 0;
  b = b*4 + M5_skp_b_st[__min(3,__max(0,(st)-(1-1)))];
  b = b*5 + M5_skp_b_c4[__min(4,__max(0,(c4)-(1-1)))];
  b = b*6 + M5_skp_b_run[__min(16,__max(0,(run)-(1-1)))];

  c = 0;
  c = c*4 + M5_skp_c_st[__min(3,__max(0,(st)-(1-1)))];
  c = c*11 + M5_skp_c_rc[__min(10,__max(0,(rc)-(1-1)))];
  c = c*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  d = 0;
  d = d*4 + M5_skp_d_st[__min(3,__max(0,(st)-(1-1)))];
  d = d*2 + (((pz)>0+(1-1)));
  d = d*3 + (((rz)>0+(1-1))+((rz)>1+(1-1)));

  e = 0;
  e = e*4 + M5_skp_e_st[__min(3,__max(0,(st)-(1-1)))];
  e = e*3 + (((bsk)>0+(1-1))+((bsk)>1+(1-1)));

  h = 0;
  h = h*4 + M5_skp_h_st[__min(3,__max(0,(st)-(1-1)))];
  h = h*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));
  h = h*3 + (((rr)>0+(1-1))+((rr)>1+(1-1)));

  i = 0;
  i = i*4 + M5_skp_i_st[__min(3,__max(0,(st)-(1-1)))];
  i = i*6 + M5_skp_i_run[__min(16,__max(0,(run)-(1-1)))];
  i = i*5 + M5_skp_i_c4[__min(4,__max(0,(c4)-(1-1)))];
  i = i*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  j = 0;
  j = j*4 + M5_skp_j_st[__min(3,__max(0,(st)-(1-1)))];
  j = j*11 + M5_skp_j_rc[__min(10,__max(0,(rc)-(1-1)))];
  j = j*6 + M5_skp_j_run[__min(16,__max(0,(run)-(1-1)))];

  k = 0;
  k = k*4 + M5_skp_k_st[__min(3,__max(0,(st)-(1-1)))];
  k = k*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));
  k = k*5 + M5_skp_k_c4[__min(4,__max(0,(c4)-(1-1)))];
  k = k*3 + (((rt)>0+(1-1))+((rt)>1+(1-1)));

  l = 0;
  l = l*4 + M5_skp_l_st[__min(3,__max(0,(st)-(1-1)))];

  m = 0;
  m = m*4 + M5_skp_m_st[__min(3,__max(0,(st)-(1-1)))];
  m = m*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_skp
#define FC_PFX M5_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
