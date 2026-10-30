/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_sgn_ND (or M_sgn_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_sgn_make( const M_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_sgn_VARS(M_LOCAL)

  a = 0;
  a = (a<<2) + (((((cls)>>2)&1)<<1) + ((cls)&1)); // 101
  a = a*3 + (((pp)>0+(1-1))+((pp)>1+(1-1)));
  a = a*4 + M9_sgn_a_mg[__min(42,__max(0,(mg)-(1-1)))];
  a = a*2 + (((pp2)>1+(1-1)));

  b = 0;
  b = (b<<3) + (((cls)&7)); // 111
  b = b*9 + M9_sgn_b_lsa[__min(8,__max(0,(lsa)-(1-1)))];
  b = b*4 + M9_sgn_b_pb[__min(64,__max(0,(pos)-(1-1)))];

  c = 0;
  c = (c<<3) + (((cls)&7)); // 111
  c = c*15 + M9_sgn_c_gh[__min(26,__max(0,(gh)-(1-1)))];
  c = c*4 + M9_sgn_c_pb[__min(64,__max(0,(pos)-(1-1)))];

  d = 0;
  d = (d<<1) + ((((cls)>>1)&1)); // 010
  d = d*2 + (((pp2)>1+(1-1)));
  d = d*8 + M9_sgn_d_mg2[__min(42,__max(0,(mg2)-(1-1)))];
  d = d*2 + (((rk)>3+(1-1)));

  e = 0;
  e = (e<<1) + ((((cls)>>1)&1)); // 010
  e = e*2 + (((flu)>5+(1-1)));
  e = e*3 + (((es)>0+(1-1))+((es)>1+(1-1)));
  e = e*3 + (((pj)>0+(1-1))+((pj)>1+(1-1)));

  h = 0;
  h = (h<<1) + (((cls)&1)); // 001
  h = h*9 + M9_sgn_h_flu[__min(8,__max(0,(flu)-(1-1)))];
  h = h*9 + M9_sgn_h_flu2[__min(8,__max(0,(flu2)-(1-1)))];

  i = 0;
  i = (i<<1) + (((cls)&1)); // 001
  i = i*2 + (((pp)>1+(1-1)));
  i = i*9 + M9_sgn_i_mg[__min(42,__max(0,(mg)-(1-1)))];
  i = i*3 + (((rk)>1+(1-1))+((rk)>4+(1-1)));

  j = 0;
  j = (j<<2) + ((((cls)>>1)&3)); // 110
  j = j*18 + M9_sgn_j_fdlu[__min(26,__max(0,(fdlu)-(1-1)))];
  j = j*2 + (((pos)>0+(1-1)));

  k = 0;
  k = (k<<1) + (((cls)&1)); // 001
  k = k*3 + (((xs)>0+(1-1))+((xs)>1+(1-1)));

  l = 0;
  l = (l<<1) + (((cls)&1)); // 001
  l = l*20 + M9_sgn_l_lam[__min(24,__max(0,(lam)-(1-1)))];
  l = l*3 + (((pos)>0+(1-1))+((pos)>20+(1-1)));

  s = 0;
  s = (s<<1) + (((cls)&1)); // 001
  s = s*13 + M9_sgn_s_vote[__min(19,__max(0,(vote)-(1-1)))];
  s = s*4 + M9_sgn_s_pb[__min(64,__max(0,(pos)-(1-1)))];

  f = 0;
  f = (f<<3) + (((cls)&7)); // 111
  f = f*6 + M9_sgn_f_pb[__min(64,__max(0,(pos)-(1-1)))];

  m = 0;
  m = (m<<2) + (((((cls)>>2)&1)<<1) + ((cls)&1)); // 101
  m = m*2 + (((pos)>20+(1-1)));
  m = m*2 + (((pp)>0+(1-1)));

  w = 0;
  w = w*2 + (((pp2)>0+(1-1)));
  w = w*5 + M9_sgn_w_mg2[__min(42,__max(0,(mg2)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sgn
#define FC_PFX M9_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
