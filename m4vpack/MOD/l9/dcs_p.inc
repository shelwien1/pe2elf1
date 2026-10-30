/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_dcs_ND (or M_dcs_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_dcs_make( const M_dcs_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_dcs_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((lc)>0+(1-1)));
  a = a*3 + (((es)>0+(1-1))+((es)>1+(1-1)));

  b = 0;
  b = b*2 + (((lc)>0+(1-1)));
  b = b*8 + M9_dcs_b_ls[__min(14,__max(0,(ls)-(1-1)))];
  b = b*8 + M9_dcs_b_as[__min(14,__max(0,(as)-(1-1)))];

  c = 0;
  c = c*2 + (((coded)>0+(1-1)));
  c = c*3 + (((qs)>4+(1-1))+((qs)>10+(1-1)));

  d = 0;
  d = d*6 + M9_dcs_d_lbn[__min(17,__max(0,(lbn)-(1-1)))];
  d = d*5 + M9_dcs_d_abn[__min(17,__max(0,(abn)-(1-1)))];

  e = 0;
  e = e*2 + (((lc)>0+(1-1)));
  e = e*3 + (((ls2)>0+(1-1))+((ls2)>1+(1-1)));
  e = e*3 + (((as2)>0+(1-1))+((as2)>1+(1-1)));
  e = e*3 + (((psg)>0+(1-1))+((psg)>1+(1-1)));

  h = 0;
  h = h*2 + (((lc)>0+(1-1)));
  h = h*13 + M9_dcs_h_sz[__min(12,__max(0,(sz)-(1-1)))];
  h = h*3 + (((psg)>0+(1-1))+((psg)>1+(1-1)));

  i = 0;
  i = i*2 + (((lc)>0+(1-1)));
  i = i*8 + M9_dcs_i_prev[__min(13,__max(0,(prev)-(1-1)))];
  i = i*2 + (((coded)>0+(1-1)));

  j = 0;
  j = j*2 + (((lc)>0+(1-1)));
  j = j*6 + M9_dcs_j_ls[__min(14,__max(0,(ls)-(1-1)))];
  j = j*5 + M9_dcs_j_prev[__min(13,__max(0,(prev)-(1-1)))];

  k = 0;
  k = k*2 + (((lc)>0+(1-1)));
  k = k*6 + M9_dcs_k_as[__min(14,__max(0,(as)-(1-1)))];
  k = k*5 + M9_dcs_k_prev[__min(13,__max(0,(prev)-(1-1)))];

  l = 0;
  l = l*7 + M9_dcs_l_ie[__min(26,__max(0,(ie)-(1-1)))];
  l = l*6 + M9_dcs_l_tq[__min(64,__max(0,(tq)-(1-1)))];

  s = 0;
  s = s*2 + (((lc)>0+(1-1)));
  s = s*2 + (((prev)>4+(1-1)));

  f = 0;
  f = f*2 + (((lc)>0+(1-1)));

  m = 0;
  m = m*2 + (((coded)>0+(1-1)));

  w = 0;
  w = w*2 + (((lc)>0+(1-1)));
  w = w*3 + (((psg)>0+(1-1))+((psg)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_dcs
#define FC_PFX M9_dcs_
#define FC_FAM dcs
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
