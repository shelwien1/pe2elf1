/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_lvl_ND (or M_lvl_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_lvl_make( const M_lvl_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_lvl_VARS(M_LOCAL)

  a = 0;
  a = a*7 + M3_lvl_a_nmb[__min(8,__max(0,(nmb)-(1-1)))];
  a = a*5 + M3_lvl_a_ngt[__min(8,__max(0,(ngt)-(1-1)))];
  a = a*28 + M3_lvl_a_pb[__min(64,__max(0,(pos)-(1-1)))];

  b = 0;
  b = (b<<3) + (((cls)&7)); // 111
  b = b*7 + M3_lvl_b_nb2[__min(8,__max(0,(nb2)-(1-1)))];

  c = 0;
  c = (c<<2) + (((cls)&3)); // 011
  c = c*6 + M3_lvl_c_pm[__min(8,__max(0,(pm)-(1-1)))];
  c = c*38 + M3_lvl_c_pb[__min(64,__max(0,(pos)-(1-1)))];
  c = c*2 + (((qs)>3+(1-1)));

  d = 0;
  d = (d<<2) + ((((cls)>>1)&3)); // 110
  d = d*19 + M3_lvl_d_mvc[__min(30,__max(0,(mvc)-(1-1)))];
  d = d*31 + M3_lvl_d_pb[__min(64,__max(0,(pos)-(1-1)))];

  e = 0;
  e = (e<<2) + ((((cls)>>1)&3)); // 110
  e = e*5 + M3_lvl_e_den[__min(4,__max(0,(den)-(1-1)))];
  e = e*37 + M3_lvl_e_pb[__min(64,__max(0,(pos)-(1-1)))];

  h = 0;
  h = (h<<2) + ((((cls)>>1)&3)); // 110
  h = (h<<4) + (((rb)&15)); // 1111
  h = (h<<6) + (((pos)&63)); // 111111

  m = 0;
  m = (m<<2) + ((((cls)>>1)&3)); // 110
  m = m*11 + M3_lvl_m_pb[__min(64,__max(0,(pos)-(1-1)))];

  t = 0;
  t = (t<<1) + (((cls)&1)); // 001

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_lvl
#define FC_PFX M3_lvl_
#define FC_FAM lvl
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
