/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -7: the ten counters, the three mixers and the final one, no APMs.  Copied through into
    MOD/l7/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3` the three mixers
    (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node), `_MT` the mantissa plane, `_G` the sign counters.  Each is
    H_esc_ND (or H_esc_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H7_esc_make( const H_esc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, g;
  H_esc_VARS(H_LOCAL)

  a = 0;
  a = a*12 + H7_esc_a_kq[__min(19,__max(0,(kq)-(1-1)))];
  a = a*8 + H7_esc_a_a1[__min(10,__max(0,(a1)-(1-1)))];

  b = 0;
  b = b*3 + (((pf)>8+(1-1))+((pf)>10+(1-1)));

  c = 0;
  c = c*8 + H7_esc_c_oc[__min(12,__max(0,(oc)-(1-1)))];
  c = c*2 + (((ws)>0+(1-1)));

  d = 0;
  d = d*4 + H7_esc_d_la[__min(12,__max(0,(la)-(1-1)))];
  d = d*3 + (((kq)>0+(1-1))+((kq)>4+(1-1)));

  e = 0;
  e = e*23 + H7_esc_e_sb[__min(50,__max(0,(sb)-(1-1)))];

  h = 0;
  h = h*10 + H7_esc_h_a1[__min(12,__max(0,(a1)-(1-1)))];
  h = h*6 + H7_esc_h_la[__min(5,__max(0,(la)-(1-1)))];
  h = h*13 + H7_esc_h_xp[__min(39,__max(0,(xp)-(1-1)))];

  i = 0;
  i = i*9 + H7_esc_i_pf[__min(12,__max(0,(pf)-(1-1)))];
  i = i*5 + H7_esc_i_oc[__min(5,__max(0,(oc)-(1-1)))];

  j = 0;
  j = j*12 + H7_esc_j_kq[__min(19,__max(0,(kq)-(1-1)))];
  j = j*5 + H7_esc_j_pf[__min(5,__max(0,(pf)-(1-1)))];

  k = 0;
  k = k*3 + (((a1)>8+(1-1))+((a1)>11+(1-1)));

  l = 0;
  l = l*3 + (((la)>9+(1-1))+((la)>10+(1-1)));
  l = l*2 + (((pf)>4+(1-1)));

  s = 0;
  s = s*15 + H7_esc_s_av[__min(31,__max(0,(av)-(1-1)))];

  m = 0;
  m = m*2 + (((ws)>0+(1-1)));

  w = 0;
  w = w*2 + (((ws)>0+(1-1)));

  t = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C7_esc
#define FC_PFX H7_esc_
#define FC_FAM esc
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
