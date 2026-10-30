/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_lvl_ND (or M_lvl_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_lvl_make( const M_lvl_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_lvl_VARS(M_LOCAL)

  a = 0;
  a = (a<<1) + ((((cls)>>2)&1)); // 100
  a = a*3 + (((gs)>0+(1-1))+((gs)>4+(1-1)));
  a = a*28 + M5_lvl_a_pb[__min(64,__max(0,(pos)-(1-1)))];
  a = a*5 + M5_lvl_a_qs[__min(32,__max(0,(qs)-(1-1)))];

  b = 0;
  b = (b<<1) + (((cls)&1)); // 001
  b = b*6 + M5_lvl_b_nmb[__min(8,__max(0,(nmb)-(1-1)))];
  b = b*5 + M5_lvl_b_ngt[__min(8,__max(0,(ngt)-(1-1)))];
  b = b*11 + M5_lvl_b_pb[__min(64,__max(0,(pos)-(1-1)))];

  c = 0;
  c = (c<<1) + ((((cls)>>2)&1)); // 100
  c = c*4 + M5_lvl_c_gs[__min(9,__max(0,(gs)-(1-1)))];
  c = c*14 + M5_lvl_c_kc[__min(32,__max(0,(kc)-(1-1)))];

  d = 0;
  d = (d<<1) + (((cls)&1)); // 001
  d = d*5 + M5_lvl_d_pm[__min(8,__max(0,(pm)-(1-1)))];
  d = d*5 + M5_lvl_d_qs[__min(32,__max(0,(qs)-(1-1)))];

  e = 0;
  e = (e<<2) + ((((cls)>>1)&3)); // 110
  e = e*7 + M5_lvl_e_nb2[__min(8,__max(0,(nb2)-(1-1)))];

  h = 0;
  h = (h<<3) + (((cls)&7)); // 111
  h = h*5 + M5_lvl_h_den[__min(4,__max(0,(den)-(1-1)))];
  h = h*4 + M5_lvl_h_pb[__min(64,__max(0,(pos)-(1-1)))];

  i = 0;
  i = (i<<2) + (((cls)&3)); // 011
  i = i*19 + M5_lvl_i_mvc[__min(30,__max(0,(mvc)-(1-1)))];
  i = i*23 + M5_lvl_i_pb[__min(64,__max(0,(pos)-(1-1)))];

  j = 0;
  j = (j<<1) + ((((cls)>>1)&1)); // 010
  j = j*6 + M5_lvl_j_qs[__min(32,__max(0,(qs)-(1-1)))];
  j = j*23 + M5_lvl_j_pb[__min(64,__max(0,(pos)-(1-1)))];

  k = 0;
  k = (k<<3) + (((cls)&7)); // 111
  k = (k<<6) + (((lh)&63)); // 111111
  k = (k<<6) + (((pos)&63)); // 111111

  l = 0;
  l = (l<<3) + (((cls)&7)); // 111
  l = (l<<4) + (((rb)&15)); // 1111
  l = (l<<6) + (((pos)&63)); // 111111

  m = 0;
  m = m*5 + M5_lvl_m_pb[__min(64,__max(0,(pos)-(1-1)))];
  m = m*3 + (((gs)>0+(1-1))+((gs)>1+(1-1)));

  t = 0;
  t = (t<<2) + (((((cls)>>2)&1)<<1) + ((cls)&1)); // 101
  t = t*4 + M5_lvl_t_qs[__min(32,__max(0,(qs)-(1-1)))];

  n = 0;
  n = (n<<3) + (((cls)&7)); // 111

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_lvl
#define FC_PFX M5_lvl_
#define FC_FAM lvl
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
