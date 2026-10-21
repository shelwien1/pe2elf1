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
    H_qpg_ND (or H_qpg_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H7_qpg_make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  H_qpg_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((mag)>4+(1-1)));
  a = a*3 + (((ss)>5+(1-1))+((ss)>7+(1-1)));
  a = a*8 + H7_qpg_a_xe[__min(18,__max(0,(xe)-(1-1)))];
  a = a*6 + H7_qpg_a_lq[__min(9,__max(0,(lq)-(1-1)))];

  b = 0;
  b = b*4 + H7_qpg_b_as[__min(9,__max(0,(as)-(1-1)))];
  b = b*2 + (((ls)>0+(1-1)));

  c = 0;
  c = c*4 + H7_qpg_c_as[__min(9,__max(0,(as)-(1-1)))];
  c = c*6 + H7_qpg_c_cs[__min(9,__max(0,(cs)-(1-1)))];

  d = 0;
  d = d*7 + H7_qpg_d_bs[__min(9,__max(0,(bs)-(1-1)))];
  d = d*4 + H7_qpg_d_mag[__min(7,__max(0,(mag)-(1-1)))];
  d = d*2 + (((bk)>8+(1-1)));

  e = 0;
  e = e*4 + H7_qpg_e_cs[__min(9,__max(0,(cs)-(1-1)))];
  e = e*2 + (((rs)>2+(1-1)));
  e = e*7 + H7_qpg_e_ss[__min(9,__max(0,(ss)-(1-1)))];

  h = 0;
  h = h*4 + H7_qpg_h_as[__min(9,__max(0,(as)-(1-1)))];
  h = h*4 + H7_qpg_h_rs[__min(3,__max(0,(rs)-(1-1)))];
  h = h*3 + (((ls)>0+(1-1))+((ls)>1+(1-1)));

  i = 0;
  i = i*5 + H7_qpg_i_as[__min(9,__max(0,(as)-(1-1)))];
  i = i*6 + H7_qpg_i_cs[__min(9,__max(0,(cs)-(1-1)))];
  i = i*8 + H7_qpg_i_bs[__min(9,__max(0,(bs)-(1-1)))];

  j = 0;
  j = j*6 + H7_qpg_j_cls[__min(8,__max(0,(cls)-(1-1)))];
  j = j*3 + (((st)>1+(1-1))+((st)>2+(1-1)));

  k = 0;
  k = k*3 + (((mag)>1+(1-1))+((mag)>2+(1-1)));
  k = k*6 + H7_qpg_k_as[__min(9,__max(0,(as)-(1-1)))];
  k = k*3 + (((cls)>5+(1-1))+((cls)>6+(1-1)));

  l = 0;
  l = l*7 + H7_qpg_l_cs[__min(9,__max(0,(cs)-(1-1)))];
  l = l*6 + H7_qpg_l_bs[__min(9,__max(0,(bs)-(1-1)))];
  l = l*2 + (((ls)>1+(1-1)));

  s = 0;
  s = s*7 + H7_qpg_s_as[__min(9,__max(0,(as)-(1-1)))];
  s = s*4 + H7_qpg_s_mag[__min(7,__max(0,(mag)-(1-1)))];

  m = 0;

  w = 0;
  w = w*3 + (((as)>4+(1-1))+((as)>5+(1-1)));
  w = w*6 + H7_qpg_w_cs[__min(9,__max(0,(cs)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C7_qpg
#define FC_PFX H7_qpg_
#define FC_FAM qpg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
