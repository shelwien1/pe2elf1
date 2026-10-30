/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_esc_ND (or M_esc_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_esc_make( const M_esc_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_esc_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  b = 0;
  b = b*2 + (((dm)>0+(1-1)));

  c = 0;
  c = c*2 + (((dm)>0+(1-1)));

  d = 0;
  d = d*2 + (((dm)>0+(1-1)));

  e = 0;

  h = 0;

  m = 0;
  m = m*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_esc
#define FC_PFX M3_esc_
#define FC_FAM esc
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
