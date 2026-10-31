/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_aht_ND (or M_aht_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void A9_aht_make( const M_aht_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_aht_VARS(M_LOCAL)

  a = 0;
  a = a*5 + A9_aht_a_kd[__min(4,__max(0,(kd)-(1-1)))];
  a = a*14 + A9_aht_a_hb[__min(19,__max(0,(hb)-(1-1)))];

  b = 0;
  b = b*5 + A9_aht_b_kd[__min(4,__max(0,(kd)-(1-1)))];
  b = b*20 + A9_aht_b_hb[__min(19,__max(0,(hb)-(1-1)))];
  b = b*3 + (((lg)>0+(1-1))+((lg)>1+(1-1)));
  b = b*6 + A9_aht_b_bk[__min(5,__max(0,(bk)-(1-1)))];

  c = 0;
  c = c*5 + A9_aht_c_kd[__min(4,__max(0,(kd)-(1-1)))];
  c = c*20 + A9_aht_c_hb[__min(19,__max(0,(hb)-(1-1)))];
  c = c*24 + A9_aht_c_q1[__min(23,__max(0,(q1)-(1-1)))];

  d = 0;
  d = d*5 + A9_aht_d_kd[__min(4,__max(0,(kd)-(1-1)))];
  d = d*20 + A9_aht_d_hb[__min(19,__max(0,(hb)-(1-1)))];
  d = d*14 + A9_aht_d_q0[__min(23,__max(0,(q0)-(1-1)))];

  e = 0;
  e = e*5 + A9_aht_e_kd[__min(4,__max(0,(kd)-(1-1)))];
  e = e*20 + A9_aht_e_hb[__min(19,__max(0,(hb)-(1-1)))];
  e = e*10 + A9_aht_e_q1[__min(16,__max(0,(q1)-(1-1)))];
  e = e*10 + A9_aht_e_q2[__min(16,__max(0,(q2)-(1-1)))];

  h = 0;
  h = h*5 + A9_aht_h_kd[__min(4,__max(0,(kd)-(1-1)))];
  h = h*4 + A9_aht_h_gm[__min(3,__max(0,(gm)-(1-1)))];
  h = h*4 + A9_aht_h_pg[__min(3,__max(0,(pg)-(1-1)))];
  h = h*20 + A9_aht_h_hb[__min(19,__max(0,(hb)-(1-1)))];

  i = 0;
  i = i*5 + A9_aht_i_kd[__min(4,__max(0,(kd)-(1-1)))];
  i = i*8 + A9_aht_i_ex[__min(21,__max(0,(ex)-(1-1)))];
  i = i*6 + A9_aht_i_bk[__min(5,__max(0,(bk)-(1-1)))];

  j = 0;
  j = j*5 + A9_aht_j_kd[__min(4,__max(0,(kd)-(1-1)))];
  j = j*15 + A9_aht_j_bin[__min(45,__max(0,(bin)-(1-1)))];
  j = j*20 + A9_aht_j_hb[__min(19,__max(0,(hb)-(1-1)))];

  k = 0;
  k = k*5 + A9_aht_k_kd[__min(4,__max(0,(kd)-(1-1)))];
  k = k*6 + A9_aht_k_bk[__min(5,__max(0,(bk)-(1-1)))];
  k = k*24 + A9_aht_k_qa[__min(23,__max(0,(qa)-(1-1)))];
  k = k*3 + (((lg)>0+(1-1))+((lg)>1+(1-1)));

  l = 0;
  l = l*5 + A9_aht_l_kd[__min(4,__max(0,(kd)-(1-1)))];
  l = l*20 + A9_aht_l_hb[__min(19,__max(0,(hb)-(1-1)))];
  l = l*6 + A9_aht_l_bk[__min(5,__max(0,(bk)-(1-1)))];
  l = l*10 + A9_aht_l_pm[__min(9,__max(0,(pm)-(1-1)))];
  l = l*10 + A9_aht_l_pb[__min(9,__max(0,(pb)-(1-1)))];

  s = 0;
  s = s*5 + A9_aht_s_kd[__min(4,__max(0,(kd)-(1-1)))];
  s = s*7 + A9_aht_s_hb[__min(19,__max(0,(hb)-(1-1)))];

  f = 0;
  f = f*5 + A9_aht_f_kd[__min(4,__max(0,(kd)-(1-1)))];
  f = f*6 + A9_aht_f_bk[__min(5,__max(0,(bk)-(1-1)))];
  f = f*10 + A9_aht_f_q1[__min(16,__max(0,(q1)-(1-1)))];

  m = 0;
  m = m*5 + A9_aht_m_kd[__min(4,__max(0,(kd)-(1-1)))];
  m = m*20 + A9_aht_m_hb[__min(19,__max(0,(hb)-(1-1)))];

  w = 0;

  t = 0;
  t = t*5 + A9_aht_t_kd[__min(4,__max(0,(kd)-(1-1)))];
  t = t*20 + A9_aht_t_hb[__min(19,__max(0,(hb)-(1-1)))];

  n = 0;
  n = n*5 + A9_aht_n_kd[__min(4,__max(0,(kd)-(1-1)))];

  g = 0;
  g = g*20 + A9_aht_g_hb[__min(19,__max(0,(hb)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_aht
#define FC_PFX A9_aht_
#define FC_FAM aht
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
