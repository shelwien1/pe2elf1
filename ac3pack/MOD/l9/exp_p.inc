/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_exp_ND (or M_exp_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void A9_exp_make( const M_exp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_exp_VARS(M_LOCAL)

  a = 0;
  a = a*4 + A9_exp_a_es[__min(3,__max(0,(es)-(1-1)))];
  a = a*4 + A9_exp_a_cc[__min(3,__max(0,(cc)-(1-1)))];
  a = a*10 + A9_exp_a_pd[__min(9,__max(0,(pd)-(1-1)))];

  b = 0;
  b = b*10 + A9_exp_b_pd[__min(9,__max(0,(pd)-(1-1)))];
  b = b*6 + A9_exp_b_d1[__min(5,__max(0,(d1)-(1-1)))];
  b = b*4 + A9_exp_b_es[__min(3,__max(0,(es)-(1-1)))];

  c = 0;
  c = c*10 + A9_exp_c_pd[__min(9,__max(0,(pd)-(1-1)))];
  c = c*7 + A9_exp_c_po[__min(9,__max(0,(po)-(1-1)))];
  c = c*4 + A9_exp_c_es[__min(3,__max(0,(es)-(1-1)))];

  d = 0;
  d = d*7 + A9_exp_d_po[__min(9,__max(0,(po)-(1-1)))];
  d = d*2 + (((d1)>4+(1-1)));
  d = d*4 + A9_exp_d_cc[__min(3,__max(0,(cc)-(1-1)))];

  e = 0;
  e = e*6 + A9_exp_e_d1[__min(5,__max(0,(d1)-(1-1)))];
  e = e*6 + A9_exp_e_d2[__min(5,__max(0,(d2)-(1-1)))];
  e = e*4 + A9_exp_e_es[__min(3,__max(0,(es)-(1-1)))];
  e = e*3 + (((cc)>0+(1-1))+((cc)>2+(1-1)));

  h = 0;
  h = h*10 + A9_exp_h_pd[__min(9,__max(0,(pd)-(1-1)))];
  h = h*6 + A9_exp_h_pl[__min(5,__max(0,(pl)-(1-1)))];
  h = h*6 + A9_exp_h_d1[__min(5,__max(0,(d1)-(1-1)))];

  i = 0;
  i = i*15 + A9_exp_i_bin[__min(45,__max(0,(bin)-(1-1)))];
  i = i*10 + A9_exp_i_pd[__min(9,__max(0,(pd)-(1-1)))];
  i = i*4 + A9_exp_i_es[__min(3,__max(0,(es)-(1-1)))];

  j = 0;
  j = j*20 + A9_exp_j_ex[__min(24,__max(0,(ex)-(1-1)))];
  j = j*6 + A9_exp_j_d1[__min(5,__max(0,(d1)-(1-1)))];
  j = j*10 + A9_exp_j_pd[__min(9,__max(0,(pd)-(1-1)))];

  k = 0;
  k = k*8 + A9_exp_k_ht[__min(7,__max(0,(ht)-(1-1)))];
  k = k*10 + A9_exp_k_pd[__min(9,__max(0,(pd)-(1-1)))];
  k = k*6 + A9_exp_k_po[__min(9,__max(0,(po)-(1-1)))];

  l = 0;
  l = l*2 + (((bs)>0+(1-1)));
  l = l*6 + A9_exp_l_bk[__min(5,__max(0,(bk)-(1-1)))];
  l = l*10 + A9_exp_l_pd[__min(9,__max(0,(pd)-(1-1)))];
  l = l*6 + A9_exp_l_d1[__min(5,__max(0,(d1)-(1-1)))];
  l = l*3 + (((gi)>0+(1-1))+((gi)>1+(1-1)));

  s = 0;
  s = s*7 + A9_exp_s_pd[__min(9,__max(0,(pd)-(1-1)))];
  s = s*4 + A9_exp_s_es[__min(3,__max(0,(es)-(1-1)))];

  f = 0;
  f = f*8 + A9_exp_f_po[__min(9,__max(0,(po)-(1-1)))];
  f = f*6 + A9_exp_f_d1[__min(5,__max(0,(d1)-(1-1)))];

  m = 0;
  m = m*4 + A9_exp_m_cc[__min(3,__max(0,(cc)-(1-1)))];

  w = 0;
  w = w*8 + A9_exp_w_ht[__min(7,__max(0,(ht)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_exp
#define FC_PFX A9_exp_
#define FC_FAM exp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
