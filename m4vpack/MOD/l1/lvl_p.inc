/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_lvl_ND (or M_lvl_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M1_lvl_make( const M_lvl_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_lvl_VARS(M_LOCAL)

  a = 0;
  a = (a<<1) + (((cls)&1)); // 001
  a = a*7 + M1_lvl_a_nmb[__min(8,__max(0,(nmb)-(1-1)))];
  a = a*5 + M1_lvl_a_ngt[__min(8,__max(0,(ngt)-(1-1)))];
  a = a*26 + M1_lvl_a_pb[__min(64,__max(0,(pos)-(1-1)))];

  b = 0;
  b = (b<<2) + ((((cls)>>1)&3)); // 110
  b = b*8 + M1_lvl_b_nb2[__min(8,__max(0,(nb2)-(1-1)))];

  c = 0;
  c = (c<<2) + (((cls)&3)); // 011
  c = c*6 + M1_lvl_c_pm[__min(8,__max(0,(pm)-(1-1)))];
  c = c*45 + M1_lvl_c_pb[__min(64,__max(0,(pos)-(1-1)))];
  c = c*2 + (((qs)>3+(1-1)));

  d = 0;
  d = (d<<2) + ((((cls)>>1)&3)); // 110
  d = d*15 + M1_lvl_d_mvc[__min(30,__max(0,(mvc)-(1-1)))];
  d = d*33 + M1_lvl_d_pb[__min(64,__max(0,(pos)-(1-1)))];

  m = 0;
  m = (m<<2) + ((((cls)>>1)&3)); // 110
  m = m*9 + M1_lvl_m_pb[__min(64,__max(0,(pos)-(1-1)))];
  m = m*4 + M1_lvl_m_gs[__min(9,__max(0,(gs)-(1-1)))];

  t = 0;
  t = (t<<1) + (((cls)&1)); // 001

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_lvl
#define FC_PFX M1_lvl_
#define FC_FAM lvl
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
