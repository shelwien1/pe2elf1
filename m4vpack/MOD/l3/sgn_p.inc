/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_sgn_ND (or M_sgn_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_sgn_make( const M_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_sgn_VARS(M_LOCAL)

  a = 0;
  a = (a<<1) + (((cls)&1)); // 001
  a = a*3 + (((xs)>0+(1-1))+((xs)>1+(1-1)));

  b = 0;
  b = (b<<2) + ((((cls)>>1)&3)); // 110
  b = b*18 + M3_sgn_b_fdlu[__min(26,__max(0,(fdlu)-(1-1)))];
  b = b*2 + (((pos)>0+(1-1)));

  c = 0;
  c = (c<<3) + (((cls)&7)); // 111
  c = c*7 + M3_sgn_c_flu[__min(8,__max(0,(flu)-(1-1)))];
  c = c*9 + M3_sgn_c_flu2[__min(8,__max(0,(flu2)-(1-1)))];

  d = 0;
  d = (d<<1) + ((((cls)>>1)&1)); // 010
  d = d*4 + M3_sgn_d_vote[__min(19,__max(0,(vote)-(1-1)))];
  d = d*2 + (((pos)>0+(1-1)));

  e = 0;
  e = (e<<3) + (((cls)&7)); // 111
  e = e*9 + M3_sgn_e_lsa[__min(8,__max(0,(lsa)-(1-1)))];
  e = e*50 + M3_sgn_e_pb[__min(64,__max(0,(pos)-(1-1)))];

  h = 0;
  h = (h<<3) + (((cls)&7)); // 111
  h = h*27 + M3_sgn_h_gh[__min(26,__max(0,(gh)-(1-1)))];
  h = h*46 + M3_sgn_h_pb[__min(64,__max(0,(pos)-(1-1)))];

  m = 0;
  m = (m<<3) + (((cls)&7)); // 111
  m = m*5 + M3_sgn_m_pb[__min(64,__max(0,(pos)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_sgn
#define FC_PFX M3_sgn_
#define FC_FAM sgn
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
