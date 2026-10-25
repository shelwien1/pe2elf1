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
    the sign counters.  Each is H_esc_ND (or H_esc_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_esc_make( const H_esc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_esc_VARS(H_LOCAL)

  a = 0;
  a = a*20 + H9_esc_a_kq[__min(19,__max(0,(kq)-(1-1)))];
  a = a*11 + H9_esc_a_a1[__min(10,__max(0,(a1)-(1-1)))];

  b = 0;
  b = b*13 + H9_esc_b_pf[__min(12,__max(0,(pf)-(1-1)))];
  b = b*6 + H9_esc_b_a1[__min(5,__max(0,(a1)-(1-1)))];

  c = 0;
  c = c*13 + H9_esc_c_oc[__min(12,__max(0,(oc)-(1-1)))];
  c = c*2 + (((ws)>0+(1-1)));

  d = 0;
  d = d*13 + H9_esc_d_la[__min(12,__max(0,(la)-(1-1)))];
  d = d*6 + H9_esc_d_kq[__min(5,__max(0,(kq)-(1-1)))];

  e = 0;
  e = e*51 + H9_esc_e_sb[__min(50,__max(0,(sb)-(1-1)))];

  h = 0;
  h = h*13 + H9_esc_h_a1[__min(12,__max(0,(a1)-(1-1)))];
  h = h*6 + H9_esc_h_la[__min(5,__max(0,(la)-(1-1)))];
  h = h*40 + H9_esc_h_xp[__min(39,__max(0,(xp)-(1-1)))];

  i = 0;
  i = i*13 + H9_esc_i_pf[__min(12,__max(0,(pf)-(1-1)))];
  i = i*6 + H9_esc_i_oc[__min(5,__max(0,(oc)-(1-1)))];

  j = 0;
  j = j*20 + H9_esc_j_kq[__min(19,__max(0,(kq)-(1-1)))];
  j = j*6 + H9_esc_j_pf[__min(5,__max(0,(pf)-(1-1)))];

  k = 0;
  k = k*13 + H9_esc_k_a1[__min(12,__max(0,(a1)-(1-1)))];
  k = k*6 + H9_esc_k_oc[__min(5,__max(0,(oc)-(1-1)))];

  l = 0;
  l = l*13 + H9_esc_l_la[__min(12,__max(0,(la)-(1-1)))];
  l = l*6 + H9_esc_l_pf[__min(5,__max(0,(pf)-(1-1)))];

  s = 0;
  s = s*6 + H9_esc_s_a1[__min(5,__max(0,(a1)-(1-1)))];
  s = s*32 + H9_esc_s_av[__min(31,__max(0,(av)-(1-1)))];

  f = 0;
  f = f*6 + H9_esc_f_pf[__min(5,__max(0,(pf)-(1-1)))];
  f = f*17 + H9_esc_f_av[__min(31,__max(0,(av)-(1-1)))];

  m = 0;
  m = m*2 + (((ws)>0+(1-1)));

  w = 0;
  w = w*2 + (((ws)>0+(1-1)));

  t = 0;
  t = t*20 + H9_esc_t_kq[__min(19,__max(0,(kq)-(1-1)))];

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_esc
#define FC_PFX H9_esc_
#define FC_FAM esc
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
