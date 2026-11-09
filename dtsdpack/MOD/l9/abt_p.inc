/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_abt_ND (or M_abt_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_abt_make( const M_abt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_abt_VARS(M_LOCAL)

  a = 0;
  a = a*27 + D9_abt_a_bd[__min(31,__max(0,(bd)-(1-1)))];
  a = (a<<8) + (((es)&255)); // 11111111

  b = 0;
  b = b*9 + D9_abt_b_la[__min(27,__max(0,(la)-(1-1)))];
  b = b*8 + D9_abt_b_lb[__min(27,__max(0,(lb)-(1-1)))];

  c = 0;
  c = c*23 + D9_abt_c_sn[__min(27,__max(0,(sn)-(1-1)))];
  c = c*24 + D9_abt_c_la[__min(27,__max(0,(la)-(1-1)))];

  d = 0;
  d = d*9 + D9_abt_d_sn[__min(27,__max(0,(sn)-(1-1)))];
  d = d*3 + (((sd)>0+(1-1))+((sd)>1+(1-1)));
  d = d*10 + D9_abt_d_de[__min(9,__max(0,(de)-(1-1)))];
  d = d*15 + D9_abt_d_sc[__min(14,__max(0,(sc)-(1-1)))];

  e = 0;
  e = e*25 + D9_abt_e_bd[__min(31,__max(0,(bd)-(1-1)))];
  e = e*25 + D9_abt_e_sn[__min(27,__max(0,(sn)-(1-1)))];

  h = 0;
  h = h*3 + (((lb)>20+(1-1))+((lb)>24+(1-1)));
  h = (h<<7) + (((es)&127)); // 01111111
  h = h*13 + D9_abt_h_bd[__min(31,__max(0,(bd)-(1-1)))];

  i = 0;
  i = (i<<8) + (((es)&255)); // 11111111
  i = i*21 + D9_abt_i_la[__min(27,__max(0,(la)-(1-1)))];

  j = 0;
  j = j*7 + D9_abt_j_de[__min(9,__max(0,(de)-(1-1)))];

  k = 0;
  k = k*2 + (((sn)>0+(1-1)));
  k = k*12 + D9_abt_k_nn[__min(15,__max(0,(nn)-(1-1)))];
  k = k*15 + D9_abt_k_sc[__min(14,__max(0,(sc)-(1-1)))];
  k = k*5 + D9_abt_k_eq[__min(4,__max(0,(eq)-(1-1)))];

  l = 0;
  l = l*20 + D9_abt_l_bd[__min(31,__max(0,(bd)-(1-1)))];
  l = l*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  l = l*22 + D9_abt_l_la[__min(27,__max(0,(la)-(1-1)))];

  s = 0;
  s = s*9 + D9_abt_s_sn[__min(27,__max(0,(sn)-(1-1)))];
  s = s*2 + (((sd)>1+(1-1)));
  s = s*7 + D9_abt_s_nn[__min(15,__max(0,(nn)-(1-1)))];
  s = s*4 + D9_abt_s_sc[__min(14,__max(0,(sc)-(1-1)))];

  f = 0;
  f = (f<<6) + (((((es)>>3)&15)<<2) + ((es)&3)); // 01111011

  m = 0;
  m = m*8 + D9_abt_m_nn[__min(15,__max(0,(nn)-(1-1)))];

  w = 0;
  w = w*2 + (((st)>8+(1-1)));

  t = 0;
  t = t*3 + (((bd)>14+(1-1))+((bd)>19+(1-1)));

  n = 0;
  n = n*28 + D9_abt_n_la[__min(27,__max(0,(la)-(1-1)))];

  g = 0;
  g = g*32 + D9_abt_g_bd[__min(31,__max(0,(bd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_abt
#define FC_PFX D9_abt_
#define FC_FAM abt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
