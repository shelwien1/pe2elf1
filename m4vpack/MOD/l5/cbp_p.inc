/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_cbp_ND (or M_cbp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_cbp_make( const M_cbp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_cbp_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  a = a*9 + M5_cbp_a_lu[__min(8,__max(0,(lu)-(1-1)))];

  b = 0;
  b = b*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  b = b*7 + M5_cbp_b_mvc[__min(17,__max(0,(mvc)-(1-1)))];

  c = 0;
  c = c*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  c = c*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  c = c*6 + M5_cbp_c_cnt[__min(5,__max(0,(cnt)-(1-1)))];

  d = 0;
  d = d*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  d = d*5 + M5_cbp_d_frac[__min(4,__max(0,(frac)-(1-1)))];

  e = 0;
  e = e*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  e = e*8 + M5_cbp_e_qs[__min(32,__max(0,(qs)-(1-1)))];
  e = e*6 + M5_cbp_e_cnt[__min(5,__max(0,(cnt)-(1-1)))];

  h = 0;
  h = h*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  h = (h<<5) + (((pat)&31)); // 11111

  i = 0;
  i = i*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  i = i*9 + M5_cbp_i_lu[__min(8,__max(0,(lu)-(1-1)))];
  i = i*3 + (((lq)>0+(1-1))+((lq)>1+(1-1)));

  j = 0;
  j = j*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  j = j*9 + M5_cbp_j_lu[__min(8,__max(0,(lu)-(1-1)))];
  j = j*7 + M5_cbp_j_mvc[__min(17,__max(0,(mvc)-(1-1)))];

  k = 0;
  k = k*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  k = k*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  k = k*9 + M5_cbp_k_lu[__min(8,__max(0,(lu)-(1-1)))];

  l = 0;
  l = l*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));

  m = 0;
  m = m*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_cbp
#define FC_PFX M5_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
