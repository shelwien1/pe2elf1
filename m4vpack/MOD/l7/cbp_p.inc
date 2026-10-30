/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7: a family's index builder,
    the tables its indices address and its configuration class, at level -7.  Copied through
    into MOD/l7/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and
    `_W3` the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node and the state of code()'s hint).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_cbp_ND (or M_cbp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M7_cbp_make( const M_cbp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_cbp_VARS(M_LOCAL)

  a = 0;
  a = a*7 + M7_cbp_a_lu[__min(8,__max(0,(lu)-(1-1)))];

  b = 0;
  b = b*2 + (((c3)>1+(1-1)));
  b = b*2 + (((mvc)>15+(1-1)));

  c = 0;
  c = c*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  c = c*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  c = c*6 + M7_cbp_c_cnt[__min(5,__max(0,(cnt)-(1-1)))];

  d = 0;
  d = d*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  d = d*5 + M7_cbp_d_frac[__min(4,__max(0,(frac)-(1-1)))];

  e = 0;
  e = e*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  e = e*4 + M7_cbp_e_qs[__min(32,__max(0,(qs)-(1-1)))];
  e = e*3 + (((cnt)>0+(1-1))+((cnt)>1+(1-1)));

  h = 0;
  h = h*2 + (((c3)>1+(1-1)));
  h = (h<<5) + (((pat)&31)); // 11111

  i = 0;
  i = i*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  i = i*7 + M7_cbp_i_lu[__min(8,__max(0,(lu)-(1-1)))];
  i = i*3 + (((lq)>0+(1-1))+((lq)>1+(1-1)));

  j = 0;
  j = j*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  j = j*9 + M7_cbp_j_lu[__min(8,__max(0,(lu)-(1-1)))];
  j = j*7 + M7_cbp_j_mvc[__min(17,__max(0,(mvc)-(1-1)))];

  k = 0;
  k = k*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  k = k*9 + M7_cbp_k_lu[__min(8,__max(0,(lu)-(1-1)))];

  l = 0;

  s = 0;
  s = s*2 + (((lu)>7+(1-1)));

  f = 0;
  f = f*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));

  m = 0;
  m = m*2 + (((c3)>0+(1-1)));

  w = 0;
  w = w*2 + (((c3)>1+(1-1)));
  w = w*7 + M7_cbp_w_lu[__min(8,__max(0,(lu)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C7_cbp
#define FC_PFX M7_cbp_
#define FC_FAM cbp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
