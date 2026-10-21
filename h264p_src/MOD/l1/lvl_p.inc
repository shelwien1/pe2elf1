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
    mixer, `_G` the sign counters.  Each is H_lvl_ND (or H_lvl_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_lvl_make( const H_lvl_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_lvl_VARS(H_LOCAL)

  a = 0;
  a = a*7 + H1_lvl_a_lt[__min(7,__max(0,(lt)-(1-1)))];
  a = a*61 + H1_lvl_a_lh[__min(64,__max(0,(lh)-(1-1)))];
  a = a*6 + H1_lvl_a_pos[__min(5,__max(0,(pos)-(1-1)))];

  b = 0;
  b = b*6 + H1_lvl_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*5 + H1_lvl_b_pos[__min(5,__max(0,(pos)-(1-1)))];
  b = b*6 + H1_lvl_b_tot[__min(7,__max(0,(tot)-(1-1)))];
  b = b*3 + (((fq)>1+(1-1))+((fq)>2+(1-1)));

  c = 0;
  c = c*5 + H1_lvl_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*4 + H1_lvl_c_sa[__min(3,__max(0,(sa)-(1-1)))];
  c = c*4 + H1_lvl_c_sb[__min(3,__max(0,(sb)-(1-1)))];
  c = c*6 + H1_lvl_c_q6[__min(8,__max(0,(q6)-(1-1)))];
  c = c*60 + H1_lvl_c_lh[__min(64,__max(0,(lh)-(1-1)))];

  d = 0;
  d = d*5 + H1_lvl_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*2 + (((g1)>3+(1-1)));
  d = d*7 + H1_lvl_d_md[__min(19,__max(0,(md)-(1-1)))];
  d = d*6 + H1_lvl_d_lt[__min(7,__max(0,(lt)-(1-1)))];

  m = 0;
  m = m*2 + (((cg)>0+(1-1)));
  m = m*2 + (((g1)>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_lvl
#define FC_PFX H1_lvl_
#define FC_FAM lvl
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
