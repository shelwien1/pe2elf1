/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -7: the ten counters, the three mixers and the final one, no APMs.  Copied through into
    MOD/l7/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3` the three mixers
    (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node), `_MT` the mantissa plane, `_G` the sign counters.  Each is
    H_sbr_ND (or H_sbr_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H7_sbr_make( const H_sbr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, g;
  H_sbr_VARS(H_LOCAL)

  a = 0;
  a = a*9 + H7_sbr_a_kd[__min(14,__max(0,(kd)-(1-1)))];
  a = a*7 + H7_sbr_a_d1[__min(10,__max(0,(d1)-(1-1)))];
  a = a*4 + H7_sbr_a_pv[__min(5,__max(0,(pv)-(1-1)))];
  a = a*8 + H7_sbr_a_rs[__min(14,__max(0,(rs)-(1-1)))];

  b = 0;
  b = b*9 + H7_sbr_b_kd[__min(14,__max(0,(kd)-(1-1)))];
  b = b*9 + H7_sbr_b_pv[__min(13,__max(0,(pv)-(1-1)))];
  b = b*3 + (((bd)>0+(1-1))+((bd)>3+(1-1)));
  b = b*4 + H7_sbr_b_cl[__min(30,__max(0,(cl)-(1-1)))];

  c = 0;
  c = c*6 + H7_sbr_c_kd[__min(14,__max(0,(kd)-(1-1)))];
  c = c*14 + H7_sbr_c_bd[__min(30,__max(0,(bd)-(1-1)))];
  c = c*2 + (((ar)>0+(1-1)));

  d = 0;
  d = d*9 + H7_sbr_d_kd[__min(14,__max(0,(kd)-(1-1)))];
  d = d*7 + H7_sbr_d_d1[__min(10,__max(0,(d1)-(1-1)))];
  d = d*7 + H7_sbr_d_d2[__min(6,__max(0,(d2)-(1-1)))];
  d = d*6 + H7_sbr_d_rm[__min(14,__max(0,(rm)-(1-1)))];

  e = 0;
  e = e*3 + (((kd)>1+(1-1))+((kd)>13+(1-1)));
  e = e*7 + H7_sbr_e_lv[__min(11,__max(0,(lv)-(1-1)))];

  h = 0;
  h = h*7 + H7_sbr_h_kd[__min(14,__max(0,(kd)-(1-1)))];
  h = h*3 + (((en)>0+(1-1))+((en)>1+(1-1)));
  h = h*2 + (((fr)>0+(1-1)));
  h = h*3 + (((pv)>1+(1-1))+((pv)>2+(1-1)));
  h = h*61 + H7_sbr_h_dg[__min(60,__max(0,(dg)-(1-1)))];

  i = 0;
  i = i*5 + H7_sbr_i_kd[__min(14,__max(0,(kd)-(1-1)))];
  i = i*2 + (((nb)>4+(1-1)));
  i = i*2 + (((bd)>1+(1-1)));

  j = 0;
  j = j*4 + H7_sbr_j_kd[__min(14,__max(0,(kd)-(1-1)))];
  j = j*3 + (((d1)>0+(1-1))+((d1)>2+(1-1)));

  k = 0;
  k = k*9 + H7_sbr_k_kd[__min(14,__max(0,(kd)-(1-1)))];
  k = k*4 + H7_sbr_k_pv[__min(7,__max(0,(pv)-(1-1)))];
  k = k*2 + (((d1)>4+(1-1)));

  l = 0;
  l = l*10 + H7_sbr_l_kd[__min(14,__max(0,(kd)-(1-1)))];
  l = l*2 + (((ar)>0+(1-1)));
  l = l*2 + (((fr)>0+(1-1)));
  l = l*7 + H7_sbr_l_bd[__min(12,__max(0,(bd)-(1-1)))];

  s = 0;
  s = s*5 + H7_sbr_s_kd[__min(14,__max(0,(kd)-(1-1)))];

  m = 0;
  m = m*10 + H7_sbr_m_kd[__min(14,__max(0,(kd)-(1-1)))];

  w = 0;
  w = w*2 + (((ar)>0+(1-1)));

  t = 0;
  t = t*3 + (((kd)>0+(1-1))+((kd)>13+(1-1)));

  g = 0;
  g = g*6 + H7_sbr_g_d1[__min(5,__max(0,(d1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C7_sbr
#define FC_PFX H7_sbr_
#define FC_FAM sbr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
