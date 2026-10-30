/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_cbp_ND (or M_cbp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M1_cbp_make( const M_cbp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_cbp_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  a = a*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  a = a*6 + M1_cbp_a_cnt[__min(5,__max(0,(cnt)-(1-1)))];

  b = 0;
  b = b*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  b = b*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  b = b*9 + M1_cbp_b_lu[__min(8,__max(0,(lu)-(1-1)))];

  c = 0;
  c = c*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  c = (c<<5) + (((pat)&31)); // 11111

  d = 0;
  d = d*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  d = d*5 + M1_cbp_d_frac[__min(4,__max(0,(frac)-(1-1)))];

  m = 0;
  m = m*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_cbp
#define FC_PFX M1_cbp_
#define FC_FAM cbp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
