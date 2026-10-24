/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9 (as h264p's): a
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
    the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_sgn_VARS(H_LOCAL)

  a = 0;
  a = a*11 + H9_sgn_a_cb[__min(10,__max(0,(cb)-(1-1)))];
  a = a*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  a = a*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));

  b = 0;
  b = b*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  b = b*3 + (((s2)>0+(1-1))+((s2)>1+(1-1)));
  b = b*11 + H9_sgn_b_cb[__min(10,__max(0,(cb)-(1-1)))];

  c = 0;
  c = c*3 + (((pf)>0+(1-1))+((pf)>1+(1-1)));
  c = c*8 + H9_sgn_c_mg[__min(7,__max(0,(mg)-(1-1)))];

  d = 0;
  d = d*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  d = d*5 + H9_sgn_d_mg[__min(4,__max(0,(mg)-(1-1)))];

  e = 0;
  e = e*20 + H9_sgn_e_kq[__min(19,__max(0,(kq)-(1-1)))];

  h = 0;
  h = h*11 + H9_sgn_h_cb[__min(10,__max(0,(cb)-(1-1)))];
  h = h*8 + H9_sgn_h_mg[__min(7,__max(0,(mg)-(1-1)))];
  h = h*7 + H9_sgn_h_kq[__min(6,__max(0,(kq)-(1-1)))];

  i = 0;
  i = i*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  i = i*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  i = i*3 + (((pf)>0+(1-1))+((pf)>1+(1-1)));

  j = 0;
  j = j*2 + (((ws)>0+(1-1)));
  j = j*11 + H9_sgn_j_cb[__min(10,__max(0,(cb)-(1-1)))];

  k = 0;
  k = k*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  k = k*8 + H9_sgn_k_mg[__min(7,__max(0,(mg)-(1-1)))];

  l = 0;
  l = l*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  l = l*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));
  l = l*3 + (((pf)>0+(1-1))+((pf)>1+(1-1)));

  s = 0;
  s = s*11 + H9_sgn_s_cb[__min(10,__max(0,(cb)-(1-1)))];

  f = 0;
  f = f*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));

  m = 0;
  m = m*11 + H9_sgn_m_cb[__min(10,__max(0,(cb)-(1-1)))];

  w = 0;
  w = w*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sgn
#define FC_PFX H9_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
