/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_hit_ND (or M_hit_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_hit_make( const M_hit_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_hit_VARS(M_LOCAL)

  a = 0;
  a = a*9 + D9_hit_a_nr[__min(15,__max(0,(nr)-(1-1)))];
  a = a*9 + D9_hit_a_nl[__min(15,__max(0,(nl)-(1-1)))];

  b = 0;
  b = b*7 + D9_hit_b_nr[__min(15,__max(0,(nr)-(1-1)))];
  b = b*7 + D9_hit_b_nl[__min(15,__max(0,(nl)-(1-1)))];
  b = b*2 + (((pm)>0+(1-1)));

  c = 0;
  c = c*3 + (((nr)>13+(1-1))+((nr)>14+(1-1)));
  c = c*3 + (((hm)>0+(1-1))+((hm)>1+(1-1)));

  d = 0;
  d = d*2 + (((hm)>0+(1-1)));

  e = 0;
  e = e*4 + D9_hit_e_nr[__min(15,__max(0,(nr)-(1-1)))];
  e = e*9 + D9_hit_e_ab[__min(27,__max(0,(ab)-(1-1)))];

  h = 0;
  h = h*3 + (((nl)>7+(1-1))+((nl)>12+(1-1)));
  h = h*15 + D9_hit_h_ab[__min(27,__max(0,(ab)-(1-1)))];

  i = 0;
  i = i*3 + (((nr)>11+(1-1))+((nr)>13+(1-1)));
  i = i*7 + D9_hit_i_nl[__min(15,__max(0,(nl)-(1-1)))];
  i = i*3 + (((hm)>0+(1-1))+((hm)>1+(1-1)));

  j = 0;
  j = j*9 + D9_hit_j_nr[__min(15,__max(0,(nr)-(1-1)))];
  j = j*11 + D9_hit_j_ql[__min(21,__max(0,(ql)-(1-1)))];

  k = 0;
  k = k*2 + (((nl)>5+(1-1)));
  k = k*7 + D9_hit_k_ql[__min(21,__max(0,(ql)-(1-1)))];
  k = k*3 + (((pm)>0+(1-1))+((pm)>1+(1-1)));

  l = 0;
  l = l*8 + D9_hit_l_nr[__min(15,__max(0,(nr)-(1-1)))];
  l = l*10 + D9_hit_l_nl[__min(15,__max(0,(nl)-(1-1)))];
  l = l*10 + D9_hit_l_ab[__min(27,__max(0,(ab)-(1-1)))];

  s = 0;
  s = s*5 + D9_hit_s_nr[__min(15,__max(0,(nr)-(1-1)))];
  s = s*9 + D9_hit_s_nl[__min(15,__max(0,(nl)-(1-1)))];

  f = 0;
  f = f*2 + (((nr)>13+(1-1)));
  f = f*5 + D9_hit_f_nl[__min(15,__max(0,(nl)-(1-1)))];

  m = 0;
  m = m*4 + D9_hit_m_nl[__min(15,__max(0,(nl)-(1-1)))];

  w = 0;
  w = w*2 + (((nr)>9+(1-1)));

  t = 0;
  t = t*3 + (((pm)>0+(1-1))+((pm)>1+(1-1)));

  n = 0;
  n = n*3 + (((pm)>0+(1-1))+((pm)>1+(1-1)));

  g = 0;
  g = g*3 + (((pm)>0+(1-1))+((pm)>1+(1-1)));
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_hit
#define FC_PFX D9_hit_
#define FC_FAM hit
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
