/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_abd_ND (or M_abd_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_abd_make( const M_abd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_abd_VARS(M_LOCAL)

  a = 0;
  a = a*8 + D9_abd_a_bd[__min(31,__max(0,(bd)-(1-1)))];
  a = (a<<2) + ((((es)>>4)&3)); // 00110000

  b = 0;
  b = b*12 + D9_abd_b_la[__min(27,__max(0,(la)-(1-1)))];
  b = b*9 + D9_abd_b_lb[__min(27,__max(0,(lb)-(1-1)))];

  c = 0;
  c = c*18 + D9_abd_c_sn[__min(27,__max(0,(sn)-(1-1)))];
  c = c*15 + D9_abd_c_la[__min(27,__max(0,(la)-(1-1)))];

  d = 0;
  d = d*5 + D9_abd_d_sn[__min(27,__max(0,(sn)-(1-1)))];
  d = d*3 + (((sd)>0+(1-1))+((sd)>1+(1-1)));
  d = d*4 + D9_abd_d_de[__min(9,__max(0,(de)-(1-1)))];
  d = d*4 + D9_abd_d_sc[__min(14,__max(0,(sc)-(1-1)))];
  d = d*2 + (((sm)>14+(1-1)));
  d = d*3 + (((ag)>1+(1-1))+((ag)>2+(1-1)));
  d = d*2 + (((sx)>3+(1-1)));

  e = 0;
  e = e*9 + D9_abd_e_bd[__min(31,__max(0,(bd)-(1-1)))];
  e = e*15 + D9_abd_e_sn[__min(27,__max(0,(sn)-(1-1)))];

  h = 0;
  h = h*15 + D9_abd_h_lb[__min(27,__max(0,(lb)-(1-1)))];
  h = (h<<6) + (((((es)>>6)&1)<<5) + ((es)&31)); // 01011111
  h = h*15 + D9_abd_h_bd[__min(31,__max(0,(bd)-(1-1)))];

  i = 0;
  i = (i<<5) + (((((es)>>6)&1)<<4) + ((((es)>>4)&1)<<3) + ((es)&7)); // 01010111
  i = i*3 + (((nb)>3+(1-1))+((nb)>6+(1-1)));
  i = i*16 + D9_abd_i_la[__min(27,__max(0,(la)-(1-1)))];

  j = 0;

  k = 0;
  k = k*18 + D9_abd_k_sn[__min(27,__max(0,(sn)-(1-1)))];
  k = k*2 + (((nn)>14+(1-1)));
  k = k*5 + D9_abd_k_sc[__min(14,__max(0,(sc)-(1-1)))];
  k = k*3 + (((sm)>4+(1-1))+((sm)>9+(1-1)));
  k = k*6 + D9_abd_k_sx[__min(5,__max(0,(sx)-(1-1)))];

  l = 0;
  l = l*12 + D9_abd_l_bd[__min(31,__max(0,(bd)-(1-1)))];
  l = l*2 + (((cc)>0+(1-1)));
  l = l*12 + D9_abd_l_la[__min(27,__max(0,(la)-(1-1)))];

  s = 0;
  s = s*12 + D9_abd_s_sn[__min(27,__max(0,(sn)-(1-1)))];
  s = s*2 + (((sd)>0+(1-1)));
  s = s*10 + D9_abd_s_sc[__min(14,__max(0,(sc)-(1-1)))];
  s = s*8 + D9_abd_s_sm[__min(16,__max(0,(sm)-(1-1)))];
  s = s*2 + (((ag)>0+(1-1)));

  f = 0;
  f = (f<<2) + ((((es)>>5)&3)); // 01100000

  m = 0;
  m = m*4 + D9_abd_m_nn[__min(15,__max(0,(nn)-(1-1)))];

  w = 0;
  w = w*3 + (((st)>0+(1-1))+((st)>8+(1-1)));

  t = 0;
  t = t*10 + D9_abd_t_bd[__min(31,__max(0,(bd)-(1-1)))];

  n = 0;
  n = n*28 + D9_abd_n_la[__min(27,__max(0,(la)-(1-1)))];

  g = 0;
  g = g*32 + D9_abd_g_bd[__min(31,__max(0,(bd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_abd
#define FC_PFX D9_abd_
#define FC_FAM abd
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
