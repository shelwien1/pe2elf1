/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_esc_ND (or M_esc_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_esc_make( const M_esc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_esc_VARS(M_LOCAL)

  a = 0;
  a = a*4 + M5_esc_a_dm[__min(3,__max(0,(dm)-(1-1)))];

  b = 0;

  c = 0;
  c = c*4 + M5_esc_c_dm[__min(3,__max(0,(dm)-(1-1)))];

  d = 0;

  e = 0;
  e = e*4 + M5_esc_e_dm[__min(3,__max(0,(dm)-(1-1)))];

  h = 0;

  i = 0;
  i = i*4 + M5_esc_i_dm[__min(3,__max(0,(dm)-(1-1)))];

  j = 0;

  k = 0;
  k = k*4 + M5_esc_k_dm[__min(3,__max(0,(dm)-(1-1)))];

  l = 0;

  m = 0;
  m = m*4 + M5_esc_m_dm[__min(3,__max(0,(dm)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_esc
#define FC_PFX M5_esc_
#define FC_FAM esc
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
