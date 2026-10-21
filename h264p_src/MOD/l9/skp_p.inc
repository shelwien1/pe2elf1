/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a
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
    the sign counters.  Each is H_skp_ND (or H_skp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_skp_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  a = a*2 + (((a3)>0+(1-1)));
  a = a*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  a = a*2 + (((col)>1+(1-1)));
  a = a*9 + H9_skp_a_c9[__min(9,__max(0,(c9)-(1-1)))];
  a = a*2 + (((eq)>0+(1-1)));

  b = 0;
  b = b*3 + (((a3)>0+(1-1))+((a3)>1+(1-1)));
  b = b*2 + (((b3)>1+(1-1)));
  b = b*3 + (((cd)>0+(1-1))+((cd)>1+(1-1)));
  b = b*8 + H9_skp_b_cnt[__min(15,__max(0,(cnt)-(1-1)))];
  b = b*3 + (((c1)>0+(1-1))+((c1)>1+(1-1)));

  c = 0;
  c = c*2 + (((st)>1+(1-1)));
  c = c*3 + (((col)>0+(1-1))+((col)>1+(1-1)));
  c = c*9 + H9_skp_c_c9[__min(9,__max(0,(c9)-(1-1)))];
  c = c*2 + (((run)>3+(1-1)));
  c = c*7 + H9_skp_c_en[__min(14,__max(0,(en)-(1-1)))];

  d = 0;
  d = d*2 + (((a3)>0+(1-1)));
  d = d*2 + (((b3)>0+(1-1)));
  d = d*2 + (((td)>14+(1-1)));
  d = d*3 + (((c1)>0+(1-1))+((c1)>1+(1-1)));
  d = d*8 + H9_skp_d_k9[__min(10,__max(0,(k9)-(1-1)))];

  e = 0;
  e = e*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  e = e*7 + H9_skp_e_td[__min(16,__max(0,(td)-(1-1)))];
  e = e*5 + H9_skp_e_en[__min(14,__max(0,(en)-(1-1)))];
  e = e*5 + H9_skp_e_sm[__min(16,__max(0,(sm)-(1-1)))];
  e = e*10 + H9_skp_e_tx[__min(14,__max(0,(tx)-(1-1)))];

  h = 0;
  h = h*2 + (((st)>1+(1-1)));
  h = h*6 + H9_skp_h_tm[__min(16,__max(0,(tm)-(1-1)))];
  h = h*8 + H9_skp_h_sm[__min(16,__max(0,(sm)-(1-1)))];
  h = h*10 + H9_skp_h_bd[__min(16,__max(0,(bd)-(1-1)))];

  i = 0;
  i = i*11 + H9_skp_i_sm[__min(16,__max(0,(sm)-(1-1)))];
  i = i*8 + H9_skp_i_en[__min(14,__max(0,(en)-(1-1)))];
  i = i*2 + (((a3)>0+(1-1)));

  j = 0;
  j = j*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  j = j*9 + H9_skp_j_tb[__min(16,__max(0,(tb)-(1-1)))];
  j = j*10 + H9_skp_j_qc[__min(17,__max(0,(qc)-(1-1)))];

  k = 0;
  k = k*5 + H9_skp_k_td[__min(16,__max(0,(td)-(1-1)))];
  k = k*8 + H9_skp_k_tm[__min(16,__max(0,(tm)-(1-1)))];

  l = 0;
  l = l*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  l = l*7 + H9_skp_l_bd[__min(16,__max(0,(bd)-(1-1)))];
  l = l*2 + (((col)>1+(1-1)));
  l = l*2 + (((cd)>1+(1-1)));

  s = 0;
  s = s*2 + (((st)>1+(1-1)));
  s = s*4 + H9_skp_s_tb[__min(16,__max(0,(tb)-(1-1)))];
  s = s*6 + H9_skp_s_bd[__min(16,__max(0,(bd)-(1-1)))];

  f = 0;
  f = f*3 + (((a3)>0+(1-1))+((a3)>1+(1-1)));
  f = f*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  f = f*5 + H9_skp_f_qc[__min(17,__max(0,(qc)-(1-1)))];

  m = 0;
  m = m*2 + (((st)>1+(1-1)));
  m = m*2 + (((col)>1+(1-1)));
  m = m*2 + (((td)>1+(1-1)));

  w = 0;
  w = w*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  w = w*3 + (((a3)>0+(1-1))+((a3)>1+(1-1)));
  w = w*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  w = w*6 + H9_skp_w_td[__min(16,__max(0,(td)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_skp
#define FC_PFX H9_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
