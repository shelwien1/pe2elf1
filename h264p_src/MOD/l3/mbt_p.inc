/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a
    family's index builder, the tables its indices address and its
    configuration class, at level -3.  Copied through into
    MOD/l3/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3` the
    three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is
    H_mbt_ND (or H_mbt_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H3_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  H_mbt_VARS(H_LOCAL)

  a = 0;
  a = a*7 + H3_mbt_a_ca[__min(8,__max(0,(ca)-(1-1)))];
  a = a*7 + H3_mbt_a_cb[__min(8,__max(0,(cb)-(1-1)))];

  b = 0;
  b = b*2 + (((st)>2+(1-1)));
  b = b*7 + H3_mbt_b_cc[__min(8,__max(0,(cc)-(1-1)))];
  b = b*6 + H3_mbt_b_c1[__min(9,__max(0,(c1)-(1-1)))];

  c = 0;
  c = c*3 + (((st)>1+(1-1))+((st)>3+(1-1)));
  c = c*7 + H3_mbt_c_ca[__min(8,__max(0,(ca)-(1-1)))];
  c = c*5 + H3_mbt_c_cc[__min(8,__max(0,(cc)-(1-1)))];

  d = 0;
  d = d*2 + (((st)>4+(1-1)));

  e = 0;
  e = e*4 + H3_mbt_e_ca[__min(8,__max(0,(ca)-(1-1)))];

  h = 0;
  h = h*2 + (((st)>1+(1-1)));

  i = 0;
  i = i*3 + (((st)>2+(1-1))+((st)>3+(1-1)));
  i = i*2 + (((ca)>0+(1-1)));

  j = 0;
  j = j*6 + H3_mbt_j_cb[__min(8,__max(0,(cb)-(1-1)))];

  k = 0;
  k = k*4 + H3_mbt_k_st[__min(9,__max(0,(st)-(1-1)))];
  k = k*5 + H3_mbt_k_nsk[__min(4,__max(0,(nsk)-(1-1)))];
  k = k*5 + H3_mbt_k_cc[__min(8,__max(0,(cc)-(1-1)))];

  l = 0;
  l = l*3 + (((st)>1+(1-1))+((st)>3+(1-1)));
  l = l*7 + H3_mbt_l_ca[__min(8,__max(0,(ca)-(1-1)))];
  l = l*7 + H3_mbt_l_cb[__min(8,__max(0,(cb)-(1-1)))];

  s = 0;
  s = s*2 + (((st)>1+(1-1)));

  m = 0;
  m = m*4 + H3_mbt_m_st[__min(9,__max(0,(st)-(1-1)))];

  w = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_mbt
#define FC_PFX H3_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
