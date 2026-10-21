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
    the sign counters.  Each is H_mbt_ND (or H_mbt_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_mbt_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((st)>4+(1-1)));
  a = a*2 + (((ca)>0+(1-1)));
  a = a*4 + H9_mbt_a_cb[__min(8,__max(0,(cb)-(1-1)))];
  a = a*6 + H9_mbt_a_en[__min(14,__max(0,(en)-(1-1)))];

  b = 0;
  b = b*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  b = b*4 + H9_mbt_b_cc[__min(8,__max(0,(cc)-(1-1)))];
  b = b*3 + (((td)>1+(1-1))+((td)>15+(1-1)));
  b = b*7 + H9_mbt_b_c1[__min(9,__max(0,(c1)-(1-1)))];

  c = 0;
  c = c*2 + (((ca)>0+(1-1)));
  c = c*4 + H9_mbt_c_im[__min(4,__max(0,(im)-(1-1)))];
  c = c*4 + H9_mbt_c_ig[__min(12,__max(0,(ig)-(1-1)))];

  d = 0;
  d = d*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  d = d*3 + (((sm)>13+(1-1))+((sm)>15+(1-1)));
  d = d*6 + H9_mbt_d_tm[__min(16,__max(0,(tm)-(1-1)))];

  e = 0;
  e = e*3 + (((st)>1+(1-1))+((st)>4+(1-1)));
  e = e*6 + H9_mbt_e_ca[__min(8,__max(0,(ca)-(1-1)))];
  e = e*2 + (((td)>8+(1-1)));
  e = e*8 + H9_mbt_e_sm[__min(16,__max(0,(sm)-(1-1)))];
  e = e*7 + H9_mbt_e_tx[__min(14,__max(0,(tx)-(1-1)))];

  h = 0;
  h = h*2 + (((st)>2+(1-1)));
  h = h*10 + H9_mbt_h_dh[__min(9,__max(0,(dh)-(1-1)))];
  h = h*5 + H9_mbt_h_tb[__min(16,__max(0,(tb)-(1-1)))];
  h = h*3 + (((nsk)>1+(1-1))+((nsk)>3+(1-1)));
  h = h*10 + H9_mbt_h_bd[__min(16,__max(0,(bd)-(1-1)))];

  i = 0;
  i = i*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  i = i*6 + H9_mbt_i_tb[__min(16,__max(0,(tb)-(1-1)))];
  i = i*8 + H9_mbt_i_en[__min(14,__max(0,(en)-(1-1)))];
  i = i*3 + (((ca)>2+(1-1))+((ca)>6+(1-1)));

  j = 0;
  j = j*8 + H9_mbt_j_sm[__min(16,__max(0,(sm)-(1-1)))];
  j = j*10 + H9_mbt_j_dh[__min(9,__max(0,(dh)-(1-1)))];
  j = j*7 + H9_mbt_j_cb[__min(8,__max(0,(cb)-(1-1)))];

  k = 0;
  k = k*8 + H9_mbt_k_tm[__min(16,__max(0,(tm)-(1-1)))];
  k = k*3 + (((nsk)>0+(1-1))+((nsk)>1+(1-1)));
  k = k*6 + H9_mbt_k_cc[__min(8,__max(0,(cc)-(1-1)))];

  l = 0;
  l = l*7 + H9_mbt_l_bd[__min(16,__max(0,(bd)-(1-1)))];
  l = l*6 + H9_mbt_l_ca[__min(8,__max(0,(ca)-(1-1)))];
  l = l*8 + H9_mbt_l_cb[__min(8,__max(0,(cb)-(1-1)))];

  s = 0;
  s = s*3 + (((tb)>5+(1-1))+((tb)>9+(1-1)));
  s = s*7 + H9_mbt_s_dh[__min(9,__max(0,(dh)-(1-1)))];

  f = 0;
  f = f*4 + H9_mbt_f_ca[__min(8,__max(0,(ca)-(1-1)))];
  f = f*9 + H9_mbt_f_qc[__min(17,__max(0,(qc)-(1-1)))];
  f = f*4 + H9_mbt_f_im[__min(4,__max(0,(im)-(1-1)))];

  m = 0;
  m = m*4 + H9_mbt_m_st[__min(9,__max(0,(st)-(1-1)))];
  m = m*6 + H9_mbt_m_bd[__min(16,__max(0,(bd)-(1-1)))];

  w = 0;
  w = w*10 + H9_mbt_w_sm[__min(16,__max(0,(sm)-(1-1)))];
  w = w*6 + H9_mbt_w_tm[__min(16,__max(0,(tm)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_mbt
#define FC_PFX H9_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
