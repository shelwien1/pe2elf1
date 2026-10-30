/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_sgn_ND (or M_sgn_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_sgn_make( const M_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_sgn_VARS(M_LOCAL)

  a = 0;
  a = (a<<3) + (((cls)&7)); // 111
  a = a*2 + (((first)>0+(1-1)));

  b = 0;
  b = (b<<3) + (((cls)&7)); // 111
  b = b*9 + M5_sgn_b_lsa[__min(8,__max(0,(lsa)-(1-1)))];
  b = b*6 + M5_sgn_b_pb[__min(64,__max(0,(pos)-(1-1)))];

  c = 0;
  c = (c<<3) + (((cls)&7)); // 111
  c = c*27 + M5_sgn_c_gh[__min(26,__max(0,(gh)-(1-1)))];
  c = c*6 + M5_sgn_c_pb[__min(64,__max(0,(pos)-(1-1)))];

  d = 0;
  d = (d<<3) + (((cls)&7)); // 111
  d = (d<<6) + (((pos)&63)); // 111111
  d = d*2 + (((ps)>0+(1-1)));

  e = 0;
  e = (e<<3) + (((cls)&7)); // 111
  e = e*9 + M5_sgn_e_flu[__min(8,__max(0,(flu)-(1-1)))];
  e = (e<<6) + (((pos)&63)); // 111111

  h = 0;
  h = (h<<3) + (((cls)&7)); // 111
  h = h*9 + M5_sgn_h_flu[__min(8,__max(0,(flu)-(1-1)))];
  h = h*9 + M5_sgn_h_flu2[__min(8,__max(0,(flu2)-(1-1)))];

  i = 0;
  i = (i<<3) + (((cls)&7)); // 111
  i = i*15 + M5_sgn_i_vote[__min(19,__max(0,(vote)-(1-1)))];
  i = i*6 + M5_sgn_i_pb[__min(64,__max(0,(pos)-(1-1)))];

  j = 0;
  j = (j<<3) + (((cls)&7)); // 111
  j = j*27 + M5_sgn_j_fdlu[__min(26,__max(0,(fdlu)-(1-1)))];
  j = j*6 + M5_sgn_j_pb[__min(64,__max(0,(pos)-(1-1)))];

  k = 0;
  k = (k<<3) + (((cls)&7)); // 111
  k = k*3 + (((xs)>0+(1-1))+((xs)>1+(1-1)));
  k = (k<<6) + (((pos)&63)); // 111111

  l = 0;
  l = (l<<3) + (((cls)&7)); // 111
  l = l*25 + M5_sgn_l_lam[__min(24,__max(0,(lam)-(1-1)))];
  l = l*6 + M5_sgn_l_pb[__min(64,__max(0,(pos)-(1-1)))];

  m = 0;
  m = (m<<3) + (((cls)&7)); // 111
  m = m*6 + M5_sgn_m_pb[__min(64,__max(0,(pos)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_sgn
#define FC_PFX M5_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
