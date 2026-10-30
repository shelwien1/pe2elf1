/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_mbt_ND (or M_mbt_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_mbt_make( const M_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_mbt_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((cl)>0+(1-1)));
  a = a*2 + (((rt)>0+(1-1)));

  b = 0;
  b = b*3 + (((st)>0+(1-1))+((st)>1+(1-1)));
  b = b*2 + (((c4)>3+(1-1)));
  b = b*4 + M9_mbt_b_rt[__min(3,__max(0,(rt)-(1-1)))];

  c = 0;
  c = c*4 + M9_mbt_c_st[__min(3,__max(0,(st)-(1-1)))];
  c = c*3 + (((rc)>2+(1-1))+((rc)>4+(1-1)));

  d = 0;
  d = d*2 + (((st)>0+(1-1)));
  d = d*3 + (((cd)>0+(1-1))+((cd)>1+(1-1)));

  e = 0;
  e = e*3 + (((st)>0+(1-1))+((st)>2+(1-1)));
  e = e*2 + (((intra)>0+(1-1)));
  e = e*2 + (((cl)>0+(1-1)));

  h = 0;
  h = h*2 + (((st)>0+(1-1)));
  h = h*4 + M9_mbt_h_fsn[__min(3,__max(0,(fsn)-(1-1)))];

  i = 0;
  i = i*4 + M9_mbt_i_st[__min(3,__max(0,(st)-(1-1)))];
  i = i*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));
  i = i*5 + M9_mbt_i_c4[__min(4,__max(0,(c4)-(1-1)))];

  j = 0;
  j = j*2 + (((st)>0+(1-1)));
  j = j*11 + M9_mbt_j_rc[__min(10,__max(0,(rc)-(1-1)))];
  j = j*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  k = 0;
  k = k*2 + (((st)>1+(1-1)));
  k = k*3 + (((rt)>0+(1-1))+((rt)>2+(1-1)));
  k = k*4 + M9_mbt_k_c4[__min(4,__max(0,(c4)-(1-1)))];

  l = 0;
  l = l*6 + M9_mbt_l_zt[__min(24,__max(0,(zt)-(1-1)))];
  l = l*9 + M9_mbt_l_ze[__min(24,__max(0,(ze)-(1-1)))];

  s = 0;
  s = s*4 + M9_mbt_s_st[__min(3,__max(0,(st)-(1-1)))];
  s = s*3 + (((cl)>0+(1-1))+((cl)>1+(1-1)));

  f = 0;
  f = f*4 + M9_mbt_f_st[__min(3,__max(0,(st)-(1-1)))];

  m = 0;
  m = m*2 + (((st)>0+(1-1)));

  w = 0;
  w = w*4 + M9_mbt_w_rt[__min(3,__max(0,(rt)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_mbt
#define FC_PFX M9_mbt_
#define FC_FAM mbt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
