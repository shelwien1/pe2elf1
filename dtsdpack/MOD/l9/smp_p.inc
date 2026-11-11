/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_smp_ND (or M_smp_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_smp_make( const M_smp_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_smp_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((pk)>0+(1-1)));
  a = a*9 + D9_smp_a_nr[__min(15,__max(0,(nr)-(1-1)))];
  a = a*7 + D9_smp_a_nl[__min(15,__max(0,(nl)-(1-1)))];

  b = 0;
  b = b*2 + (((pk)>0+(1-1)));
  b = b*10 + D9_smp_b_ab[__min(27,__max(0,(ab)-(1-1)))];
  b = b*9 + D9_smp_b_ql[__min(21,__max(0,(ql)-(1-1)))];

  c = 0;
  c = c*6 + D9_smp_c_nr[__min(15,__max(0,(nr)-(1-1)))];
  c = c*3 + (((hm)>0+(1-1))+((hm)>1+(1-1)));

  d = 0;
  d = d*2 + (((pk)>0+(1-1)));
  d = d*11 + D9_smp_d_ab[__min(27,__max(0,(ab)-(1-1)))];
  d = d*4 + D9_smp_d_m1[__min(22,__max(0,(m1)-(1-1)))];

  e = 0;
  e = e*2 + (((pk)>0+(1-1)));
  e = e*11 + D9_smp_e_ab[__min(27,__max(0,(ab)-(1-1)))];
  e = e*7 + D9_smp_e_m1[__min(22,__max(0,(m1)-(1-1)))];
  e = e*9 + D9_smp_e_m2[__min(22,__max(0,(m2)-(1-1)))];

  h = 0;
  h = h*13 + D9_smp_h_ab[__min(27,__max(0,(ab)-(1-1)))];

  i = 0;
  i = i*10 + D9_smp_i_ab[__min(27,__max(0,(ab)-(1-1)))];
  i = i*4 + D9_smp_i_hw[__min(3,__max(0,(hw)-(1-1)))];
  i = i*2 + (((pm)>0+(1-1)));

  j = 0;
  j = j*10 + D9_smp_j_ab[__min(27,__max(0,(ab)-(1-1)))];
  j = j*15 + D9_smp_j_bd[__min(31,__max(0,(bd)-(1-1)))];

  k = 0;
  k = k*5 + D9_smp_k_nr[__min(15,__max(0,(nr)-(1-1)))];
  k = k*2 + (((pm)>0+(1-1)));

  l = 0;
  l = l*2 + (((pk)>0+(1-1)));
  l = l*4 + D9_smp_l_nl[__min(15,__max(0,(nl)-(1-1)))];
  l = l*13 + D9_smp_l_ab[__min(27,__max(0,(ab)-(1-1)))];

  s = 0;
  s = s*2 + (((pk)>0+(1-1)));
  s = s*8 + D9_smp_s_nr[__min(15,__max(0,(nr)-(1-1)))];
  s = s*8 + D9_smp_s_nl[__min(15,__max(0,(nl)-(1-1)))];

  f = 0;
  f = f*2 + (((pk)>0+(1-1)));
  f = f*10 + D9_smp_f_nl[__min(15,__max(0,(nl)-(1-1)))];
  f = f*7 + D9_smp_f_nr[__min(15,__max(0,(nr)-(1-1)))];

  m = 0;
  m = m*2 + (((pk)>0+(1-1)));
  m = m*5 + D9_smp_m_nr[__min(15,__max(0,(nr)-(1-1)))];

  w = 0;
  w = w*3 + (((nl)>3+(1-1))+((nl)>10+(1-1)));

  t = 0;
  t = t*9 + D9_smp_t_ab[__min(27,__max(0,(ab)-(1-1)))];

  n = 0;
  n = n*28 + D9_smp_n_ab[__min(27,__max(0,(ab)-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_smp
#define FC_PFX D9_smp_
#define FC_FAM smp
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
