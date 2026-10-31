/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_raw_ND (or M_raw_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_raw_make( const M_raw_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_raw_VARS(M_LOCAL)

  a = 0;
  a = a*7 + A1_raw_a_code[__min(6,__max(0,(code)-(1-1)))];
  a = (a<<8) + (((c1)&255)); // 11111111

  b = 0;
  b = b*7 + A1_raw_b_code[__min(6,__max(0,(code)-(1-1)))];
  b = (b<<8) + (((c1)&255)); // 11111111
  b = (b<<4) + ((((c2)>>4)&15)); // 11110000

  c = 0;
  c = c*7 + A1_raw_c_code[__min(6,__max(0,(code)-(1-1)))];
  c = c*257 + A1_raw_c_cl[__min(256,__max(0,(cl)-(1-1)))];

  d = 0;
  d = (d<<8) + (((c1)&255)); // 11111111
  d = (d<<8) + (((c2)&255)); // 11111111

  m = 0;
  m = m*7 + A1_raw_m_code[__min(6,__max(0,(code)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_raw
#define FC_PFX A1_raw_
#define FC_FAM raw
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
