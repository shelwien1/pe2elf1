/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_exp_ND (or M_exp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_exp_make( const M_exp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_exp_VARS(M_LOCAL)

  a = 0;
  a = a*10 + A1_exp_a_pd[__min(9,__max(0,(pd)-(1-1)))];
  a = a*6 + A1_exp_a_d1[__min(5,__max(0,(d1)-(1-1)))];
  a = a*4 + A1_exp_a_es[__min(3,__max(0,(es)-(1-1)))];

  b = 0;
  b = b*10 + A1_exp_b_pd[__min(9,__max(0,(pd)-(1-1)))];
  b = b*7 + A1_exp_b_po[__min(9,__max(0,(po)-(1-1)))];
  b = b*4 + A1_exp_b_es[__min(3,__max(0,(es)-(1-1)))];

  c = 0;
  c = c*10 + A1_exp_c_pd[__min(9,__max(0,(pd)-(1-1)))];
  c = c*6 + A1_exp_c_pl[__min(5,__max(0,(pl)-(1-1)))];
  c = c*6 + A1_exp_c_d1[__min(5,__max(0,(d1)-(1-1)))];

  d = 0;
  d = d*8 + A1_exp_d_ht[__min(7,__max(0,(ht)-(1-1)))];
  d = d*10 + A1_exp_d_pd[__min(9,__max(0,(pd)-(1-1)))];
  d = d*6 + A1_exp_d_po[__min(9,__max(0,(po)-(1-1)))];

  m = 0;
  m = m*4 + A1_exp_m_cc[__min(3,__max(0,(cc)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_exp
#define FC_PFX A1_exp_
#define FC_FAM exp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
