/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a
    family's index builder, the tables its indices address and its
    configuration class, at level -5.  Copied through into
    MOD/l5/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_sgn_VARS(H_LOCAL)

  a = 0;
  a = a*6 + H5_sgn_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*4 + H5_sgn_a_pos[__min(3,__max(0,(pos)-(1-1)))];
  a = a*4 + H5_sgn_a_sa[__min(3,__max(0,(sa)-(1-1)))];
  a = a*4 + H5_sgn_a_sb[__min(3,__max(0,(sb)-(1-1)))];
  a = a*7 + H5_sgn_a_gh[__min(8,__max(0,(gh)-(1-1)))];

  b = 0;
  b = b*5 + H5_sgn_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*4 + H5_sgn_b_scb[__min(3,__max(0,(scb)-(1-1)))];
  b = b*2 + (((pos)>0+(1-1)));

  c = 0;
  c = c*4 + H5_sgn_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*3 + (((mag)>0+(1-1))+((mag)>1+(1-1)));
  c = c*12 + H5_sgn_c_md[__min(19,__max(0,(md)-(1-1)))];

  d = 0;
  d = d*5 + H5_sgn_d_cg[__min(13,__max(0,(cg)-(1-1)))];

  e = 0;
  e = e*5 + H5_sgn_e_cg[__min(13,__max(0,(cg)-(1-1)))];

  h = 0;
  h = h*4 + H5_sgn_h_cg[__min(13,__max(0,(cg)-(1-1)))];
  h = h*2 + (((pos)>0+(1-1)));
  h = h*7 + H5_sgn_h_ts[__min(6,__max(0,(ts)-(1-1)))];
  h = h*6 + H5_sgn_h_dm[__min(6,__max(0,(dm)-(1-1)))];

  i = 0;
  i = i*6 + H5_sgn_i_cg[__min(13,__max(0,(cg)-(1-1)))];

  j = 0;
  j = j*3 + (((cg)>0+(1-1))+((cg)>1+(1-1)));
  j = j*4 + H5_sgn_j_sa[__min(3,__max(0,(sa)-(1-1)))];
  j = j*4 + H5_sgn_j_sb[__min(3,__max(0,(sb)-(1-1)))];
  j = j*4 + H5_sgn_j_pos[__min(3,__max(0,(pos)-(1-1)))];

  k = 0;

  l = 0;
  l = l*5 + H5_sgn_l_cg[__min(13,__max(0,(cg)-(1-1)))];
  l = l*2 + (((pos)>0+(1-1)));
  l = l*7 + H5_sgn_l_ts[__min(6,__max(0,(ts)-(1-1)))];
  l = l*7 + H5_sgn_l_gh[__min(8,__max(0,(gh)-(1-1)))];

  m = 0;
  m = m*5 + H5_sgn_m_cg[__min(13,__max(0,(cg)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_sgn
#define FC_PFX H5_sgn_
#define FC_FAM sgn
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
