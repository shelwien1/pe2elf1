/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7: a
    family's index builder, the tables its indices address and its
    configuration class, at level -7.  Copied through into
    MOD/l7/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3` the
    three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is
    H_skp_ND (or H_skp_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H7_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  H_skp_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((a3)>0+(1-1)));
  a = a*2 + (((b3)>0+(1-1)));
  a = a*2 + (((col)>1+(1-1)));
  a = a*6 + H7_skp_a_c9[__min(9,__max(0,(c9)-(1-1)))];
  a = a*2 + (((eq)>0+(1-1)));

  b = 0;
  b = b*2 + (((st)>2+(1-1)));
  b = b*3 + (((a3)>0+(1-1))+((a3)>1+(1-1)));
  b = b*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  b = b*3 + (((cd)>0+(1-1))+((cd)>1+(1-1)));
  b = b*6 + H7_skp_b_cnt[__min(15,__max(0,(cnt)-(1-1)))];
  b = b*3 + (((c1)>0+(1-1))+((c1)>1+(1-1)));

  c = 0;
  c = c*2 + (((st)>1+(1-1)));
  c = c*3 + (((col)>0+(1-1))+((col)>1+(1-1)));
  c = c*9 + H7_skp_c_c9[__min(9,__max(0,(c9)-(1-1)))];
  c = c*3 + (((run)>1+(1-1))+((run)>6+(1-1)));
  c = c*8 + H7_skp_c_en[__min(14,__max(0,(en)-(1-1)))];

  d = 0;
  d = d*2 + (((a3)>0+(1-1)));
  d = d*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  d = d*2 + (((td)>0+(1-1)));
  d = d*2 + (((c1)>1+(1-1)));
  d = d*10 + H7_skp_d_k9[__min(10,__max(0,(k9)-(1-1)))];

  e = 0;
  e = e*2 + (((st)>1+(1-1)));
  e = e*7 + H7_skp_e_td[__min(16,__max(0,(td)-(1-1)))];
  e = e*5 + H7_skp_e_en[__min(14,__max(0,(en)-(1-1)))];
  e = e*6 + H7_skp_e_sm[__min(16,__max(0,(sm)-(1-1)))];
  e = e*8 + H7_skp_e_tx[__min(14,__max(0,(tx)-(1-1)))];

  h = 0;
  h = h*2 + (((st)>1+(1-1)));
  h = h*7 + H7_skp_h_tm[__min(16,__max(0,(tm)-(1-1)))];
  h = h*8 + H7_skp_h_sm[__min(16,__max(0,(sm)-(1-1)))];
  h = h*9 + H7_skp_h_bd[__min(16,__max(0,(bd)-(1-1)))];

  i = 0;
  i = i*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  i = i*5 + H7_skp_i_en[__min(14,__max(0,(en)-(1-1)))];

  j = 0;
  j = j*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  j = j*3 + (((c9)>0+(1-1))+((c9)>2+(1-1)));
  j = j*10 + H7_skp_j_qc[__min(17,__max(0,(qc)-(1-1)))];

  k = 0;
  k = k*4 + H7_skp_k_td[__min(16,__max(0,(td)-(1-1)))];
  k = k*5 + H7_skp_k_tm[__min(16,__max(0,(tm)-(1-1)))];
  k = k*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));

  l = 0;
  l = l*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  l = l*8 + H7_skp_l_bd[__min(16,__max(0,(bd)-(1-1)))];

  s = 0;
  s = s*2 + (((st)>1+(1-1)));
  s = s*5 + H7_skp_s_bd[__min(16,__max(0,(bd)-(1-1)))];

  m = 0;
  m = m*2 + (((st)>1+(1-1)));
  m = m*2 + (((col)>1+(1-1)));
  m = m*2 + (((td)>1+(1-1)));

  w = 0;
  w = w*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  w = w*3 + (((a3)>0+(1-1))+((a3)>1+(1-1)));
  w = w*3 + (((b3)>0+(1-1))+((b3)>1+(1-1)));
  w = w*5 + H7_skp_w_td[__min(16,__max(0,(td)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C7_skp
#define FC_PFX H7_skp_
#define FC_FAM skp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
