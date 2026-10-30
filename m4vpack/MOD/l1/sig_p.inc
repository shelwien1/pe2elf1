/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_sig_ND (or M_sig_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M1_sig_make( const M_sig_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_sig_VARS(M_LOCAL)

  a = 0;
  a = (a<<3) + (((cls)&7)); // 111
  a = a*7 + M1_sig_a_rnz[__min(17,__max(0,(rnz)-(1-1)))];
  a = a*4 + M1_sig_a_xm[__min(9,__max(0,(xm)-(1-1)))];
  a = (a<<6) + (((pos)&63)); // 111111

  b = 0;
  b = (b<<3) + (((cls)&7)); // 111
  b = (b<<5) + (((fp)&31)); // 11111
  b = (b<<6) + (((pos)&63)); // 111111

  c = 0;
  c = (c<<3) + (((cls)&7)); // 111
  c = (c<<6) + (((pos)&63)); // 111111
  c = (c<<16) + (((h16)&65535)); // 1111111111111111

  d = 0;
  d = (d<<3) + (((cls)&7)); // 111
  d = (d<<8) + (((h8)&255)); // 11111111
  d = (d<<6) + (((pos)&63)); // 111111

  m = 0;
  m = (m<<3) + (((cls)&7)); // 111
  m = (m<<6) + (((pos)&63)); // 111111

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_sig
#define FC_PFX M1_sig_
#define FC_FAM sig
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
