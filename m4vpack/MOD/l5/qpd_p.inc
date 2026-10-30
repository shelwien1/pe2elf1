/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_qpd_ND (or M_qpd_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_qpd_make( const M_qpd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_qpd_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  a = a*2 + (((pdq)>0+(1-1)));

  b = 0;
  b = b*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  b = b*4 + M5_qpd_b_qs[__min(32,__max(0,(qs)-(1-1)))];

  c = 0;
  c = c*2 + (((c3)>0+(1-1)));

  d = 0;
  d = d*2 + (((pdq)>0+(1-1)));
  d = d*6 + M5_qpd_d_qs[__min(32,__max(0,(qs)-(1-1)))];

  e = 0;
  e = e*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  e = e*2 + (((pdq)>0+(1-1)));
  e = e*5 + M5_qpd_e_qs[__min(32,__max(0,(qs)-(1-1)))];

  h = 0;
  h = h*2 + (((pdq)>0+(1-1)));

  i = 0;
  i = i*7 + M5_qpd_i_qs[__min(32,__max(0,(qs)-(1-1)))];

  j = 0;

  k = 0;
  k = k*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  k = k*2 + (((pdq)>0+(1-1)));

  l = 0;
  l = l*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  l = l*5 + M5_qpd_l_qs[__min(32,__max(0,(qs)-(1-1)))];

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_qpd
#define FC_PFX M5_qpd_
#define FC_FAM qpd
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
