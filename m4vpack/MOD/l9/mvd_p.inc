/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_mvd_ND (or M_mvd_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_mvd_make( const M_mvd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_mvd_VARS(M_LOCAL)

  a = 0;
  a = a*5 + M9_mvd_a_ns[__min(16,__max(0,(ns)-(1-1)))];

  b = 0;
  b = b*3 + (((kind)>2+(1-1))+((kind)>3+(1-1)));
  b = b*2 + (((comp)>0+(1-1)));
  b = b*9 + M9_mvd_b_oth[__min(17,__max(0,(oth)-(1-1)))];

  c = 0;
  c = c*5 + M9_mvd_c_kind[__min(5,__max(0,(kind)-(1-1)))];
  c = c*6 + M9_mvd_c_rs[__min(17,__max(0,(rs)-(1-1)))];

  d = 0;
  d = d*5 + M9_mvd_d_kind[__min(5,__max(0,(kind)-(1-1)))];
  d = d*2 + (((comp)>0+(1-1)));
  d = d*9 + M9_mvd_d_dz[__min(17,__max(0,(dz)-(1-1)))];
  d = d*3 + (((ds)>0+(1-1))+((ds)>1+(1-1)));

  e = 0;
  e = e*11 + M9_mvd_e_sp[__min(17,__max(0,(sp)-(1-1)))];

  h = 0;
  h = h*4 + M9_mvd_h_kind[__min(5,__max(0,(kind)-(1-1)))];
  h = h*2 + (((comp)>0+(1-1)));
  h = h*10 + M9_mvd_h_lq[__min(17,__max(0,(lq)-(1-1)))];
  h = h*8 + M9_mvd_h_aq[__min(17,__max(0,(aq)-(1-1)))];

  i = 0;
  i = i*5 + M9_mvd_i_kind[__min(5,__max(0,(kind)-(1-1)))];
  i = i*2 + (((comp)>0+(1-1)));
  i = i*7 + M9_mvd_i_fc[__min(6,__max(0,(fc)-(1-1)))];

  j = 0;
  j = j*3 + (((kind)>1+(1-1))+((kind)>2+(1-1)));
  j = j*6 + M9_mvd_j_rbi[__min(5,__max(0,(rbi)-(1-1)))];
  j = j*3 + (((rsame)>0+(1-1))+((rsame)>1+(1-1)));

  k = 0;
  k = k*3 + (((kind)>0+(1-1))+((kind)>2+(1-1)));
  k = k*2 + (((comp)>0+(1-1)));
  k = k*5 + M9_mvd_k_ns[__min(16,__max(0,(ns)-(1-1)))];
  k = k*10 + M9_mvd_k_oth[__min(17,__max(0,(oth)-(1-1)))];

  l = 0;
  l = l*2 + (((comp)>0+(1-1)));
  l = l*8 + M9_mvd_l_tz[__min(17,__max(0,(tz)-(1-1)))];
  l = l*3 + (((ts)>0+(1-1))+((ts)>1+(1-1)));

  s = 0;
  s = s*3 + (((oth)>2+(1-1))+((oth)>15+(1-1)));

  f = 0;
  f = f*2 + (((kind)>4+(1-1)));
  f = f*2 + (((comp)>0+(1-1)));
  f = f*5 + M9_mvd_f_ns[__min(16,__max(0,(ns)-(1-1)))];

  m = 0;
  m = m*3 + (((kind)>1+(1-1))+((kind)>3+(1-1)));
  m = m*6 + M9_mvd_m_tz[__min(17,__max(0,(tz)-(1-1)))];

  w = 0;
  w = w*5 + M9_mvd_w_kind[__min(5,__max(0,(kind)-(1-1)))];
  w = w*2 + (((comp)>0+(1-1)));
  w = w*5 + M9_mvd_w_oth[__min(17,__max(0,(oth)-(1-1)))];

  t = 0;
  t = t*5 + M9_mvd_t_kind[__min(5,__max(0,(kind)-(1-1)))];

  n = 0;
  n = n*3 + (((kind)>0+(1-1))+((kind)>1+(1-1)));

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_mvd
#define FC_PFX M9_mvd_
#define FC_FAM mvd
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
