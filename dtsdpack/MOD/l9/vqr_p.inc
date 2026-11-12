/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_vqr_ND (or M_vqr_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_vqr_make( const M_vqr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_vqr_VARS(M_LOCAL)

  a = 0;
  a = a*3 + (((mg)>8+(1-1))+((mg)>10+(1-1)));
  a = a*6 + D9_vqr_a_nc[__min(6,__max(0,(nc)-(1-1)))];
  a = a*8 + D9_vqr_a_ml[__min(15,__max(0,(ml)-(1-1)))];
  a = a*5 + D9_vqr_a_gr[__min(7,__max(0,(gr)-(1-1)))];

  b = 0;
  b = b*4 + D9_vqr_b_kd[__min(3,__max(0,(kd)-(1-1)))];
  b = b*7 + D9_vqr_b_mg[__min(12,__max(0,(mg)-(1-1)))];
  b = b*9 + D9_vqr_b_gn[__min(15,__max(0,(gn)-(1-1)))];

  c = 0;
  c = c*4 + D9_vqr_c_kd[__min(3,__max(0,(kd)-(1-1)))];
  c = c*13 + D9_vqr_c_bd[__min(31,__max(0,(bd)-(1-1)))];
  c = c*7 + D9_vqr_c_mg[__min(12,__max(0,(mg)-(1-1)))];

  d = 0;
  d = d*5 + D9_vqr_d_mg[__min(12,__max(0,(mg)-(1-1)))];
  d = d*4 + D9_vqr_d_gr[__min(7,__max(0,(gr)-(1-1)))];

  e = 0;
  e = e*4 + D9_vqr_e_kd[__min(3,__max(0,(kd)-(1-1)))];
  e = e*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  e = e*15 + D9_vqr_e_bd[__min(31,__max(0,(bd)-(1-1)))];

  h = 0;
  h = h*4 + D9_vqr_h_kd[__min(3,__max(0,(kd)-(1-1)))];
  h = h*8 + D9_vqr_h_gn[__min(15,__max(0,(gn)-(1-1)))];
  h = h*3 + (((lh)>0+(1-1))+((lh)>1+(1-1)));

  i = 0;
  i = i*2 + (((kd)>1+(1-1)));
  i = i*4 + D9_vqr_i_mg[__min(12,__max(0,(mg)-(1-1)))];
  i = i*3 + (((lh)>0+(1-1))+((lh)>1+(1-1)));
  i = i*7 + D9_vqr_i_gn[__min(15,__max(0,(gn)-(1-1)))];

  j = 0;
  j = j*3 + (((kd)>0+(1-1))+((kd)>2+(1-1)));
  j = j*5 + D9_vqr_j_bd[__min(31,__max(0,(bd)-(1-1)))];
  j = j*2 + (((lh)>0+(1-1)));

  k = 0;
  k = k*4 + D9_vqr_k_kd[__min(3,__max(0,(kd)-(1-1)))];
  k = k*2 + (((cc)>0+(1-1)));
  k = k*9 + D9_vqr_k_mg[__min(12,__max(0,(mg)-(1-1)))];

  l = 0;
  l = l*2 + (((kd)>2+(1-1)));
  l = l*5 + D9_vqr_l_gn[__min(15,__max(0,(gn)-(1-1)))];

  s = 0;
  s = s*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  s = s*5 + D9_vqr_s_mg[__min(12,__max(0,(mg)-(1-1)))];

  f = 0;
  f = f*4 + D9_vqr_f_kd[__min(3,__max(0,(kd)-(1-1)))];
  f = f*7 + D9_vqr_f_nc[__min(6,__max(0,(nc)-(1-1)))];
  f = f*16 + D9_vqr_f_ml[__min(15,__max(0,(ml)-(1-1)))];

  m = 0;

  w = 0;
  w = w*5 + D9_vqr_w_mg[__min(12,__max(0,(mg)-(1-1)))];
  w = w*9 + D9_vqr_w_ml[__min(15,__max(0,(ml)-(1-1)))];

  t = 0;
  t = t*4 + D9_vqr_t_kd[__min(3,__max(0,(kd)-(1-1)))];

  n = 0;
  n = n*4 + D9_vqr_n_kd[__min(3,__max(0,(kd)-(1-1)))];

  g = 0;
  g = g*4 + D9_vqr_g_kd[__min(3,__max(0,(kd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_vqr
#define FC_PFX D9_vqr_
#define FC_FAM vqr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
