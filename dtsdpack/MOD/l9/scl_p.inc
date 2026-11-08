/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_scl_ND (or M_scl_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_scl_make( const M_scl_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_scl_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((kd)>2+(1-1)));
  a = a*2 + (((pk)>0+(1-1)));
  a = a*4 + D9_scl_a_dp[__min(3,__max(0,(dp)-(1-1)))];

  b = 0;
  b = b*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  b = b*2 + (((pk)>0+(1-1)));
  b = b*8 + D9_scl_b_pn[__min(7,__max(0,(pn)-(1-1)))];
  b = b*4 + D9_scl_b_dp[__min(3,__max(0,(dp)-(1-1)))];

  c = 0;
  c = c*2 + (((kd)>0+(1-1)));
  c = c*2 + (((pk)>0+(1-1)));
  c = c*10 + D9_scl_c_ab[__min(27,__max(0,(ab)-(1-1)))];

  d = 0;
  d = d*3 + (((kd)>0+(1-1))+((kd)>2+(1-1)));
  d = d*2 + (((pk)>0+(1-1)));
  d = d*7 + D9_scl_d_lp[__min(9,__max(0,(lp)-(1-1)))];

  e = 0;
  e = e*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  e = e*2 + (((pk)>0+(1-1)));
  e = e*2 + (((hm)>0+(1-1)));

  h = 0;
  h = h*2 + (((pk)>0+(1-1)));
  h = h*7 + D9_scl_h_pn[__min(7,__max(0,(pn)-(1-1)))];
  h = h*10 + D9_scl_h_ab[__min(27,__max(0,(ab)-(1-1)))];

  i = 0;
  i = i*3 + (((kd)>1+(1-1))+((kd)>2+(1-1)));
  i = i*2 + (((pk)>0+(1-1)));
  i = i*2 + (((pm)>0+(1-1)));
  i = i*6 + D9_scl_i_pn[__min(7,__max(0,(pn)-(1-1)))];

  j = 0;
  j = j*2 + (((pk)>0+(1-1)));
  j = j*9 + D9_scl_j_bd[__min(31,__max(0,(bd)-(1-1)))];

  k = 0;
  k = k*2 + (((pk)>0+(1-1)));
  k = k*6 + D9_scl_k_lp[__min(9,__max(0,(lp)-(1-1)))];
  k = k*3 + (((pn)>3+(1-1))+((pn)>4+(1-1)));

  l = 0;
  l = l*4 + D9_scl_l_kd[__min(3,__max(0,(kd)-(1-1)))];
  l = l*2 + (((pk)>0+(1-1)));
  l = l*9 + D9_scl_l_ab[__min(27,__max(0,(ab)-(1-1)))];

  s = 0;
  s = s*4 + D9_scl_s_kd[__min(3,__max(0,(kd)-(1-1)))];
  s = s*2 + (((pk)>0+(1-1)));
  s = s*4 + D9_scl_s_pn[__min(7,__max(0,(pn)-(1-1)))];
  s = s*4 + D9_scl_s_dp[__min(3,__max(0,(dp)-(1-1)))];

  f = 0;
  f = f*3 + (((kd)>0+(1-1))+((kd)>2+(1-1)));
  f = f*2 + (((pk)>0+(1-1)));
  f = f*8 + D9_scl_f_ab[__min(27,__max(0,(ab)-(1-1)))];

  m = 0;
  m = m*3 + (((kd)>1+(1-1))+((kd)>2+(1-1)));
  m = m*2 + (((pk)>0+(1-1)));

  w = 0;
  w = w*3 + (((pn)>0+(1-1))+((pn)>6+(1-1)));

  t = 0;
  t = t*2 + (((kd)>0+(1-1)));
  t = t*2 + (((pk)>0+(1-1)));

  n = 0;
  n = n*4 + D9_scl_n_kd[__min(3,__max(0,(kd)-(1-1)))];

  g = 0;
  g = g*4 + D9_scl_g_kd[__min(3,__max(0,(kd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_scl
#define FC_PFX D9_scl_
#define FC_FAM scl
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
