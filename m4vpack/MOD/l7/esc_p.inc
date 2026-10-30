/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7: a family's index builder,
    the tables its indices address and its configuration class, at level -7.  Copied through
    into MOD/l7/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and
    `_W3` the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node and the state of code()'s hint).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_esc_ND (or M_esc_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M7_esc_make( const M_esc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_esc_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  b = 0;

  c = 0;
  c = c*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  d = 0;

  e = 0;
  e = e*2 + (((dm)>0+(1-1)));

  h = 0;

  i = 0;
  i = i*2 + (((dm)>1+(1-1)));

  j = 0;

  k = 0;
  k = k*4 + M7_esc_k_dm[__min(3,__max(0,(dm)-(1-1)))];

  l = 0;

  s = 0;
  s = s*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  f = 0;

  m = 0;
  m = m*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  w = 0;
  w = w*3 + (((dm)>0+(1-1))+((dm)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C7_esc
#define FC_PFX M7_esc_
#define FC_FAM esc
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
