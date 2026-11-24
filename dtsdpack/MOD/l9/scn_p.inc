/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_scn_ND (or M_scn_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_scn_make( const M_scn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_scn_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((kd)>2+(1-1)));
  a = a*8 + D9_scn_a_dp[__min(7,__max(0,(dp)-(1-1)))];

  b = 0;
  b = b*5 + D9_scn_b_pn[__min(7,__max(0,(pn)-(1-1)))];
  b = b*6 + D9_scn_b_dp[__min(7,__max(0,(dp)-(1-1)))];
  b = b*6 + D9_scn_b_pl[__min(15,__max(0,(pl)-(1-1)))];

  c = 0;
  c = c*2 + (((pk)>0+(1-1)));
  c = c*8 + D9_scn_c_ab[__min(27,__max(0,(ab)-(1-1)))];

  d = 0;
  d = d*2 + (((pk)>0+(1-1)));
  d = d*10 + D9_scn_d_lp[__min(9,__max(0,(lp)-(1-1)))];

  e = 0;
  e = e*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  e = e*2 + (((hm)>0+(1-1)));

  h = 0;
  h = h*3 + (((kd)>0+(1-1))+((kd)>2+(1-1)));
  h = h*3 + (((pn)>5+(1-1))+((pn)>6+(1-1)));
  h = h*9 + D9_scn_h_ab[__min(27,__max(0,(ab)-(1-1)))];

  i = 0;
  i = i*2 + (((kd)>2+(1-1)));
  i = i*2 + (((pm)>0+(1-1)));
  i = i*2 + (((pn)>6+(1-1)));

  j = 0;
  j = j*11 + D9_scn_j_bd[__min(31,__max(0,(bd)-(1-1)))];
  j = j*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  j = j*5 + D9_scn_j_nb[__min(9,__max(0,(nb)-(1-1)))];

  k = 0;
  k = k*2 + (((pk)>0+(1-1)));
  k = k*6 + D9_scn_k_lp[__min(9,__max(0,(lp)-(1-1)))];
  k = k*4 + D9_scn_k_pn[__min(7,__max(0,(pn)-(1-1)))];

  l = 0;
  l = l*2 + (((kd)>0+(1-1)));
  l = l*3 + (((hm)>0+(1-1))+((hm)>1+(1-1)));
  l = l*7 + D9_scn_l_ab[__min(27,__max(0,(ab)-(1-1)))];

  s = 0;
  s = s*4 + D9_scn_s_pn[__min(7,__max(0,(pn)-(1-1)))];
  s = s*5 + D9_scn_s_dp[__min(7,__max(0,(dp)-(1-1)))];
  s = s*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));

  f = 0;
  f = f*3 + (((kd)>1+(1-1))+((kd)>2+(1-1)));
  f = f*6 + D9_scn_f_pl[__min(15,__max(0,(pl)-(1-1)))];

  m = 0;
  m = m*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  m = m*6 + D9_scn_m_dp[__min(7,__max(0,(dp)-(1-1)))];

  w = 0;
  w = w*2 + (((pl)>0+(1-1)));

  t = 0;
  t = t*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));

  n = 0;
  n = n*4 + D9_scn_n_kd[__min(3,__max(0,(kd)-(1-1)))];

  g = 0;
  g = g*4 + D9_scn_g_kd[__min(3,__max(0,(kd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_scn
#define FC_PFX D9_scn_
#define FC_FAM scn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
