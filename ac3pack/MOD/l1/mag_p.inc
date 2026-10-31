/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_mag_ND (or M_mag_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_mag_make( const M_mag_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_mag_VARS(M_LOCAL)

  a = 0;
  a = a*4 + A1_mag_a_bap[__min(15,__max(0,(bap)-(1-1)))];
  a = a*24 + A1_mag_a_e1[__min(23,__max(0,(e1)-(1-1)))];

  b = 0;
  b = b*16 + A1_mag_b_bap[__min(15,__max(0,(bap)-(1-1)))];
  b = b*24 + A1_mag_b_ep[__min(23,__max(0,(ep)-(1-1)))];

  c = 0;
  c = c*16 + A1_mag_c_bap[__min(15,__max(0,(bap)-(1-1)))];
  c = c*10 + A1_mag_c_e1[__min(16,__max(0,(e1)-(1-1)))];
  c = c*10 + A1_mag_c_e2[__min(16,__max(0,(e2)-(1-1)))];

  d = 0;
  d = d*16 + A1_mag_d_bap[__min(15,__max(0,(bap)-(1-1)))];
  d = d*10 + A1_mag_d_e1[__min(16,__max(0,(e1)-(1-1)))];
  d = d*10 + A1_mag_d_ew[__min(16,__max(0,(ew)-(1-1)))];
  d = d*10 + A1_mag_d_ow[__min(16,__max(0,(ow)-(1-1)))];

  m = 0;
  m = m*16 + A1_mag_m_bap[__min(15,__max(0,(bap)-(1-1)))];

  t = 0;
  t = t*16 + A1_mag_t_bap[__min(15,__max(0,(bap)-(1-1)))];

  n = 0;
  n = n*16 + A1_mag_n_bap[__min(15,__max(0,(bap)-(1-1)))];

  g = 0;
  g = g*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  g = g*3 + (((sp)>0+(1-1))+((sp)>1+(1-1)));
  g = g*3 + (((so)>0+(1-1))+((so)>1+(1-1)));
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_mag
#define FC_PFX A1_mag_
#define FC_FAM mag
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
