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
    the sign counters.  Each is H_mvs_ND (or H_mvs_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_mvs_make( const H_mvs_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_mvs_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((kind)>0+(1-1))+((kind)>2+(1-1)));
  a = a*2 + (((comp)>0+(1-1)));
  a = a*5 + H9_mvs_a_ma[__min(7,__max(0,(ma)-(1-1)))];
  a = a*4 + H9_mvs_a_mb[__min(7,__max(0,(mb)-(1-1)))];
  a = a*4 + H9_mvs_a_oth[__min(5,__max(0,(oth)-(1-1)))];
  a = a*5 + H9_mvs_a_sp[__min(8,__max(0,(sp)-(1-1)))];
  a = a*7 + H9_mvs_a_rt[__min(14,__max(0,(rt)-(1-1)))];

  b = 0;
  b = b*2 + (((kind)>2+(1-1)));
  b = b*2 + (((comp)>0+(1-1)));
  b = b*2 + (((lst)>0+(1-1)));
  b = b*4 + H9_mvs_b_ps[__min(3,__max(0,(ps)-(1-1)))];
  b = b*4 + H9_mvs_b_rf[__min(3,__max(0,(rf)-(1-1)))];
  b = b*9 + H9_mvs_b_zd[__min(12,__max(0,(zd)-(1-1)))];

  c = 0;
  c = c*4 + H9_mvs_c_kind[__min(3,__max(0,(kind)-(1-1)))];
  c = c*2 + (((comp)>0+(1-1)));
  c = c*7 + H9_mvs_c_pd[__min(8,__max(0,(pd)-(1-1)))];
  c = c*4 + H9_mvs_c_sp[__min(8,__max(0,(sp)-(1-1)))];
  c = c*9 + H9_mvs_c_zd[__min(12,__max(0,(zd)-(1-1)))];

  d = 0;
  d = d*2 + (((kind)>2+(1-1)));
  d = d*8 + H9_mvs_d_pd[__min(8,__max(0,(pd)-(1-1)))];
  d = d*4 + H9_mvs_d_nr[__min(3,__max(0,(nr)-(1-1)))];

  e = 0;
  e = e*3 + (((kind)>0+(1-1))+((kind)>2+(1-1)));
  e = e*8 + H9_mvs_e_tv[__min(13,__max(0,(tv)-(1-1)))];
  e = e*8 + H9_mvs_e_rt[__min(14,__max(0,(rt)-(1-1)))];

  h = 0;
  h = h*2 + (((kind)>1+(1-1)));
  h = h*2 + (((comp)>0+(1-1)));
  h = h*2 + (((tv)>1+(1-1)));
  h = h*6 + H9_mvs_h_oth[__min(5,__max(0,(oth)-(1-1)))];
  h = h*8 + H9_mvs_h_tq[__min(12,__max(0,(tq)-(1-1)))];
  h = h*4 + H9_mvs_h_ea[__min(6,__max(0,(ea)-(1-1)))];
  h = h*2 + (((eb)>0+(1-1)));

  i = 0;
  i = i*4 + H9_mvs_i_kind[__min(3,__max(0,(kind)-(1-1)))];
  i = i*2 + (((comp)>0+(1-1)));
  i = i*4 + H9_mvs_i_tv[__min(13,__max(0,(tv)-(1-1)))];
  i = i*4 + H9_mvs_i_tq[__min(12,__max(0,(tq)-(1-1)))];
  i = i*5 + H9_mvs_i_zd[__min(12,__max(0,(zd)-(1-1)))];
  i = i*7 + H9_mvs_i_ta[__min(13,__max(0,(ta)-(1-1)))];

  j = 0;
  j = j*4 + H9_mvs_j_kind[__min(3,__max(0,(kind)-(1-1)))];
  j = j*4 + H9_mvs_j_ma[__min(7,__max(0,(ma)-(1-1)))];
  j = j*6 + H9_mvs_j_mb[__min(7,__max(0,(mb)-(1-1)))];

  k = 0;
  k = k*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  k = k*7 + H9_mvs_k_pd[__min(8,__max(0,(pd)-(1-1)))];
  k = k*5 + H9_mvs_k_sp[__min(8,__max(0,(sp)-(1-1)))];
  k = k*4 + H9_mvs_k_ps[__min(3,__max(0,(ps)-(1-1)))];

  l = 0;
  l = l*4 + H9_mvs_l_kind[__min(3,__max(0,(kind)-(1-1)))];
  l = l*2 + (((comp)>0+(1-1)));
  l = l*3 + (((oth)>0+(1-1))+((oth)>4+(1-1)));

  s = 0;
  s = s*4 + H9_mvs_s_kind[__min(3,__max(0,(kind)-(1-1)))];
  s = s*2 + (((comp)>0+(1-1)));
  s = s*3 + (((tv)>9+(1-1))+((tv)>12+(1-1)));

  f = 0;
  f = f*4 + H9_mvs_f_kind[__min(3,__max(0,(kind)-(1-1)))];
  f = f*4 + H9_mvs_f_sp[__min(8,__max(0,(sp)-(1-1)))];
  f = f*6 + H9_mvs_f_cq[__min(9,__max(0,(cq)-(1-1)))];

  m = 0;
  m = m*4 + H9_mvs_m_pd[__min(5,__max(0,(pd)-(1-1)))];

  w = 0;
  w = w*4 + H9_mvs_w_kind[__min(3,__max(0,(kind)-(1-1)))];
  w = w*2 + (((pd)>7+(1-1)));
  w = w*4 + H9_mvs_w_ma[__min(7,__max(0,(ma)-(1-1)))];
  w = w*3 + (((nr)>1+(1-1))+((nr)>2+(1-1)));

  t = 0;
  t = t*2 + (((kind)>2+(1-1)));
  t = t*2 + (((comp)>0+(1-1)));

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_mvs
#define FC_PFX H9_mvs_
#define FC_FAM mvs
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
