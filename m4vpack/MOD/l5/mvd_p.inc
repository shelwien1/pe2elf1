/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_mvd_ND (or M_mvd_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_mvd_make( const M_mvd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_mvd_VARS(M_LOCAL)

  a = 0;
  a = a*10 + M5_mvd_a_ns[__min(16,__max(0,(ns)-(1-1)))];

  b = 0;
  b = b*3 + (((kind)>2+(1-1))+((kind)>3+(1-1)));
  b = b*2 + (((comp)>0+(1-1)));
  b = b*8 + M5_mvd_b_oth[__min(17,__max(0,(oth)-(1-1)))];

  c = 0;
  c = c*4 + M5_mvd_c_kind[__min(5,__max(0,(kind)-(1-1)))];
  c = c*7 + M5_mvd_c_rs[__min(17,__max(0,(rs)-(1-1)))];

  d = 0;
  d = d*5 + M5_mvd_d_kind[__min(5,__max(0,(kind)-(1-1)))];
  d = d*2 + (((comp)>0+(1-1)));
  d = d*7 + M5_mvd_d_dz[__min(17,__max(0,(dz)-(1-1)))];
  d = d*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));

  e = 0;
  e = e*5 + M5_mvd_e_kind[__min(5,__max(0,(kind)-(1-1)))];
  e = e*11 + M5_mvd_e_sp[__min(17,__max(0,(sp)-(1-1)))];

  h = 0;
  h = h*5 + M5_mvd_h_kind[__min(5,__max(0,(kind)-(1-1)))];
  h = h*2 + (((comp)>0+(1-1)));
  h = h*10 + M5_mvd_h_lq[__min(17,__max(0,(lq)-(1-1)))];
  h = h*7 + M5_mvd_h_aq[__min(17,__max(0,(aq)-(1-1)))];

  i = 0;
  i = i*4 + M5_mvd_i_kind[__min(5,__max(0,(kind)-(1-1)))];
  i = i*2 + (((comp)>0+(1-1)));

  j = 0;
  j = j*2 + (((kind)>2+(1-1)));
  j = j*6 + M5_mvd_j_rbi[__min(5,__max(0,(rbi)-(1-1)))];
  j = j*3 + (((rsame)>0+(1-1))+((rsame)>1+(1-1)));

  k = 0;
  k = k*2 + (((kind)>0+(1-1)));
  k = k*2 + (((comp)>0+(1-1)));
  k = k*5 + M5_mvd_k_ns[__min(16,__max(0,(ns)-(1-1)))];
  k = k*6 + M5_mvd_k_oth[__min(17,__max(0,(oth)-(1-1)))];

  l = 0;

  m = 0;
  m = m*2 + (((kind)>4+(1-1)));

  t = 0;
  t = t*6 + M5_mvd_t_kind[__min(5,__max(0,(kind)-(1-1)))];
  t = t*2 + (((comp)>0+(1-1)));

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_mvd
#define FC_PFX M5_mvd_
#define FC_FAM mvd
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
