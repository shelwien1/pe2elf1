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
    the sign counters.  Each is H_sec_ND (or H_sec_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_sec_make( const H_sec_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_sec_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((kd)>0+(1-1)));
  a = a*17 + H9_sec_a_pc[__min(16,__max(0,(pc)-(1-1)))];
  a = a*17 + H9_sec_a_fc[__min(16,__max(0,(fc)-(1-1)))];

  b = 0;
  b = b*2 + (((kd)>0+(1-1)));
  b = b*17 + H9_sec_b_fc[__min(16,__max(0,(fc)-(1-1)))];
  b = b*8 + H9_sec_b_sb[__min(7,__max(0,(sb)-(1-1)))];

  c = 0;
  c = c*2 + (((kd)>0+(1-1)));
  c = c*17 + H9_sec_c_cb[__min(16,__max(0,(cb)-(1-1)))];
  c = c*17 + H9_sec_c_fn[__min(16,__max(0,(fn)-(1-1)))];
  c = c*3 + (((ob)>0+(1-1))+((ob)>1+(1-1)));

  d = 0;
  d = d*2 + (((kd)>0+(1-1)));
  d = d*17 + H9_sec_d_gc[__min(16,__max(0,(gc)-(1-1)))];
  d = d*4 + H9_sec_d_ws[__min(3,__max(0,(ws)-(1-1)))];

  e = 0;
  e = e*2 + (((kd)>0+(1-1)));
  e = e*17 + H9_sec_e_oc[__min(16,__max(0,(oc)-(1-1)))];
  e = e*4 + H9_sec_e_pc[__min(3,__max(0,(pc)-(1-1)))];
  e = e*3 + (((mu)>0+(1-1))+((mu)>1+(1-1)));

  h = 0;
  h = h*2 + (((kd)>0+(1-1)));
  h = h*51 + H9_sec_h_sb[__min(50,__max(0,(sb)-(1-1)))];
  h = h*2 + (((ws)>0+(1-1)));
  h = h*17 + H9_sec_h_cb[__min(16,__max(0,(cb)-(1-1)))];

  i = 0;
  i = i*2 + (((kd)>0+(1-1)));
  i = i*17 + H9_sec_i_run[__min(16,__max(0,(run)-(1-1)))];
  i = i*17 + H9_sec_i_cb[__min(16,__max(0,(cb)-(1-1)))];

  j = 0;
  j = j*2 + (((kd)>0+(1-1)));
  j = j*9 + H9_sec_j_rem[__min(8,__max(0,(rem)-(1-1)))];
  j = j*17 + H9_sec_j_fn[__min(16,__max(0,(fn)-(1-1)))];

  k = 0;
  k = k*2 + (((kd)>0+(1-1)));
  k = k*8 + H9_sec_k_gr[__min(7,__max(0,(gr)-(1-1)))];
  k = k*17 + H9_sec_k_pc[__min(16,__max(0,(pc)-(1-1)))];
  k = k*25 + H9_sec_k_sb[__min(49,__max(0,(sb)-(1-1)))];

  l = 0;
  l = l*2 + (((kd)>0+(1-1)));
  l = l*17 + H9_sec_l_cb[__min(16,__max(0,(cb)-(1-1)))];
  l = l*17 + H9_sec_l_fc[__min(16,__max(0,(fc)-(1-1)))];
  l = l*4 + H9_sec_l_run[__min(3,__max(0,(run)-(1-1)))];

  s = 0;
  s = s*2 + (((kd)>0+(1-1)));
  s = s*17 + H9_sec_s_cb[__min(16,__max(0,(cb)-(1-1)))];

  f = 0;
  f = f*2 + (((kd)>0+(1-1)));
  f = f*17 + H9_sec_f_fc[__min(16,__max(0,(fc)-(1-1)))];

  m = 0;
  m = m*2 + (((kd)>0+(1-1)));

  w = 0;
  w = w*2 + (((kd)>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sec
#define FC_PFX H9_sec_
#define FC_FAM sec
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
