/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -9.  Copied through into
    MOD/l9/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and
    s) and `_WF` the one that mixes them (by node and the state of
    code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G`
    the sign counters.  Each is H_sbr_ND (or H_sbr_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_sbr_make( const H_sbr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_sbr_VARS(H_LOCAL)

  a = 0;
  a = a*15 + H9_sbr_a_kd[__min(14,__max(0,(kd)-(1-1)))];
  a = a*11 + H9_sbr_a_d1[__min(10,__max(0,(d1)-(1-1)))];
  a = a*6 + H9_sbr_a_pv[__min(5,__max(0,(pv)-(1-1)))];

  b = 0;
  b = b*15 + H9_sbr_b_kd[__min(14,__max(0,(kd)-(1-1)))];
  b = b*14 + H9_sbr_b_pv[__min(13,__max(0,(pv)-(1-1)))];
  b = b*6 + H9_sbr_b_bd[__min(5,__max(0,(bd)-(1-1)))];

  c = 0;
  c = c*15 + H9_sbr_c_kd[__min(14,__max(0,(kd)-(1-1)))];
  c = c*31 + H9_sbr_c_bd[__min(30,__max(0,(bd)-(1-1)))];
  c = c*2 + (((ar)>0+(1-1)));

  d = 0;
  d = d*15 + H9_sbr_d_kd[__min(14,__max(0,(kd)-(1-1)))];
  d = d*11 + H9_sbr_d_d1[__min(10,__max(0,(d1)-(1-1)))];
  d = d*7 + H9_sbr_d_d2[__min(6,__max(0,(d2)-(1-1)))];

  e = 0;
  e = e*15 + H9_sbr_e_kd[__min(14,__max(0,(kd)-(1-1)))];
  e = e*12 + H9_sbr_e_lv[__min(11,__max(0,(lv)-(1-1)))];
  e = e*2 + (((ch)>0+(1-1)));

  h = 0;
  h = h*15 + H9_sbr_h_kd[__min(14,__max(0,(kd)-(1-1)))];
  h = h*8 + H9_sbr_h_en[__min(7,__max(0,(en)-(1-1)))];
  h = h*2 + (((fr)>0+(1-1)));
  h = h*4 + H9_sbr_h_pv[__min(3,__max(0,(pv)-(1-1)))];

  i = 0;
  i = i*15 + H9_sbr_i_kd[__min(14,__max(0,(kd)-(1-1)))];
  i = i*21 + H9_sbr_i_nb[__min(20,__max(0,(nb)-(1-1)))];
  i = i*4 + H9_sbr_i_bd[__min(3,__max(0,(bd)-(1-1)))];

  j = 0;
  j = j*15 + H9_sbr_j_kd[__min(14,__max(0,(kd)-(1-1)))];
  j = j*2 + (((q)>0+(1-1)));
  j = j*6 + H9_sbr_j_d1[__min(5,__max(0,(d1)-(1-1)))];

  k = 0;
  k = k*15 + H9_sbr_k_kd[__min(14,__max(0,(kd)-(1-1)))];
  k = k*8 + H9_sbr_k_pv[__min(7,__max(0,(pv)-(1-1)))];
  k = k*6 + H9_sbr_k_d1[__min(5,__max(0,(d1)-(1-1)))];

  l = 0;
  l = l*15 + H9_sbr_l_kd[__min(14,__max(0,(kd)-(1-1)))];
  l = l*2 + (((ar)>0+(1-1)));
  l = l*2 + (((fr)>0+(1-1)));
  l = l*13 + H9_sbr_l_bd[__min(12,__max(0,(bd)-(1-1)))];

  s = 0;
  s = s*15 + H9_sbr_s_kd[__min(14,__max(0,(kd)-(1-1)))];

  f = 0;
  f = f*15 + H9_sbr_f_kd[__min(14,__max(0,(kd)-(1-1)))];
  f = f*4 + H9_sbr_f_pv[__min(3,__max(0,(pv)-(1-1)))];

  m = 0;
  m = m*15 + H9_sbr_m_kd[__min(14,__max(0,(kd)-(1-1)))];

  w = 0;
  w = w*2 + (((ar)>0+(1-1)));

  t = 0;
  t = t*15 + H9_sbr_t_kd[__min(14,__max(0,(kd)-(1-1)))];

  n = 0;

  g = 0;
  g = g*6 + H9_sbr_g_d1[__min(5,__max(0,(d1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sbr
#define FC_PFX H9_sbr_
#define FC_FAM sbr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
