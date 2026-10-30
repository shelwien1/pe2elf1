/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_skp_ND (or M_skp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M1_skp_make( const M_skp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_skp_VARS(M_LOCAL)

  a = 0;
  a = a*4 + M1_skp_a_st[__min(3,__max(0,(st)-(1-1)))];
  a = a*2 + (((pz)>0+(1-1)));
  a = a*3 + (((rz)>0+(1-1))+((rz)>1+(1-1)));

  b = 0;
  b = b*4 + M1_skp_b_st[__min(3,__max(0,(st)-(1-1)))];
  b = b*11 + M1_skp_b_rc[__min(10,__max(0,(rc)-(1-1)))];
  b = b*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  c = 0;
  c = c*4 + M1_skp_c_st[__min(3,__max(0,(st)-(1-1)))];
  c = c*11 + M1_skp_c_rc[__min(10,__max(0,(rc)-(1-1)))];
  c = c*6 + M1_skp_c_run[__min(16,__max(0,(run)-(1-1)))];

  d = 0;
  d = d*4 + M1_skp_d_st[__min(3,__max(0,(st)-(1-1)))];
  d = d*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));
  d = d*5 + M1_skp_d_c4[__min(4,__max(0,(c4)-(1-1)))];
  d = d*3 + (((rt)>0+(1-1))+((rt)>1+(1-1)));

  m = 0;
  m = m*4 + M1_skp_m_st[__min(3,__max(0,(st)-(1-1)))];
  m = m*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_skp
#define FC_PFX M1_skp_
#define FC_FAM skp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
