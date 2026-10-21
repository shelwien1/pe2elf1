/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a
    family's index builder, the tables its indices address and its
    configuration class, at level -1.  Copied through into
    MOD/l1/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_W` the one mixer (selected by index
    m, rows of 16 inputs), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_mvg_ND (or H_mvg_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_mvg_make( const H_mvg_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_mvg_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((comp)>0+(1-1)));
  a = a*2 + (((lst)>0+(1-1)));
  a = a*2 + (((ox)>1+(1-1)));

  b = 0;
  b = b*5 + H1_mvg_b_mag[__min(7,__max(0,(mag)-(1-1)))];
  b = b*2 + (((comp)>0+(1-1)));

  c = 0;

  d = 0;
  d = d*3 + (((mag)>2+(1-1))+((mag)>4+(1-1)));

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_mvg
#define FC_PFX H1_mvg_
#define FC_FAM mvg
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
