/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a
    family's index builder, the tables its indices address and its
    configuration class, at level -1.  Copied through into
    MOD/l1/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_W` the one mixer (selected by index
    m, rows of 16 inputs), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_sgn_ND (or H_sgn_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_sgn_VARS(H_LOCAL)

  a = 0;
  a = a*6 + H1_sgn_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*4 + H1_sgn_a_pos[__min(3,__max(0,(pos)-(1-1)))];
  a = a*4 + H1_sgn_a_sa[__min(3,__max(0,(sa)-(1-1)))];
  a = a*4 + H1_sgn_a_sb[__min(3,__max(0,(sb)-(1-1)))];
  a = a*7 + H1_sgn_a_gh[__min(8,__max(0,(gh)-(1-1)))];

  b = 0;
  b = b*4 + H1_sgn_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*4 + H1_sgn_b_scb[__min(3,__max(0,(scb)-(1-1)))];
  b = b*4 + H1_sgn_b_pos[__min(3,__max(0,(pos)-(1-1)))];

  c = 0;
  c = c*6 + H1_sgn_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*2 + (((pos)>0+(1-1)));
  c = c*7 + H1_sgn_c_gh[__min(8,__max(0,(gh)-(1-1)))];

  d = 0;
  d = d*5 + H1_sgn_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*4 + H1_sgn_d_sa[__min(3,__max(0,(sa)-(1-1)))];
  d = d*4 + H1_sgn_d_sb[__min(3,__max(0,(sb)-(1-1)))];
  d = d*4 + H1_sgn_d_pos[__min(3,__max(0,(pos)-(1-1)))];

  m = 0;
  m = m*5 + H1_sgn_m_cg[__min(13,__max(0,(cg)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_sgn
#define FC_PFX H1_sgn_
#define FC_FAM sgn
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
