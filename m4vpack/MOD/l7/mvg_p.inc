/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7: a family's index builder,
    the tables its indices address and its configuration class, at level -7.  Copied through
    into MOD/l7/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and
    `_W3` the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node and the state of code()'s hint).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_mvg_ND (or M_mvg_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M7_mvg_make( const M_mvg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_mvg_VARS(M_LOCAL)

  a = 0;
  a = a*4 + M7_mvg_a_kind[__min(5,__max(0,(kind)-(1-1)))];
  a = a*2 + (((comp)>0+(1-1)));
  a = a*3 + (((ls)>0+(1-1))+((ls)>1+(1-1)));
  a = a*3 + (((as)>0+(1-1))+((as)>1+(1-1)));

  b = 0;
  b = b*4 + M7_mvg_b_kind[__min(5,__max(0,(kind)-(1-1)))];
  b = b*2 + (((comp)>0+(1-1)));
  b = b*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));
  b = b*11 + M7_mvg_b_dz[__min(17,__max(0,(dz)-(1-1)))];

  c = 0;
  c = c*3 + (((kind)>1+(1-1))+((kind)>3+(1-1)));
  c = c*2 + (((comp)>0+(1-1)));
  c = c*3 + (((mag)>1+(1-1))+((mag)>2+(1-1)));

  d = 0;
  d = d*4 + M7_mvg_d_kind[__min(5,__max(0,(kind)-(1-1)))];
  d = d*2 + (((hm)>0+(1-1)));

  e = 0;
  e = e*5 + M7_mvg_e_kind[__min(5,__max(0,(kind)-(1-1)))];
  e = e*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));
  e = e*4 + M7_mvg_e_hm[__min(3,__max(0,(hm)-(1-1)))];

  h = 0;
  h = h*3 + (((kind)>0+(1-1))+((kind)>1+(1-1)));
  h = h*3 + (((ls)>0+(1-1))+((ls)>1+(1-1)));

  i = 0;
  i = i*2 + (((comp)>0+(1-1)));
  i = i*3 + (((as)>0+(1-1))+((as)>1+(1-1)));

  j = 0;
  j = j*4 + M7_mvg_j_kind[__min(5,__max(0,(kind)-(1-1)))];
  j = j*2 + (((comp)>0+(1-1)));
  j = j*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));
  j = j*5 + M7_mvg_j_mag[__min(16,__max(0,(mag)-(1-1)))];

  k = 0;
  k = k*4 + M7_mvg_k_kind[__min(5,__max(0,(kind)-(1-1)))];
  k = k*3 + (((ls)>0+(1-1))+((ls)>1+(1-1)));
  k = k*3 + (((as)>0+(1-1))+((as)>1+(1-1)));
  k = k*2 + (((ds)>0+(1-1)));

  l = 0;
  l = l*3 + (((kind)>3+(1-1))+((kind)>4+(1-1)));
  l = l*2 + (((comp)>0+(1-1)));

  s = 0;
  s = s*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));

  f = 0;
  f = f*5 + M7_mvg_f_kind[__min(5,__max(0,(kind)-(1-1)))];

  m = 0;
  m = m*4 + M7_mvg_m_kind[__min(5,__max(0,(kind)-(1-1)))];

  w = 0;
  w = w*3 + (((kind)>1+(1-1))+((kind)>3+(1-1)));
  w = w*2 + (((ls)>0+(1-1)));
  w = w*2 + (((as)>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C7_mvg
#define FC_PFX M7_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
