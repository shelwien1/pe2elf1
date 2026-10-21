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
    mixer, `_G` the sign counters.  Each is H_sig_ND (or H_sig_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_sig_VARS(H_LOCAL)

  a = 0;
  a = a*6 + H1_sig_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*54 + H1_sig_a_pos[__min(63,__max(0,(pos)-(1-1)))];
  a = a*7 + H1_sig_a_rem[__min(10,__max(0,(rem)-(1-1)))];

  b = 0;
  b = b*5 + H1_sig_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*55 + H1_sig_b_pos[__min(63,__max(0,(pos)-(1-1)))];
  b = b*16 + H1_sig_b_md[__min(19,__max(0,(md)-(1-1)))];
  b = b*283 + H1_sig_b_hb[__min(510,__max(0,(hb)-(1-1)))];

  c = 0;
  c = c*2 + (((cg)>0+(1-1)));
  c = c*38 + H1_sig_c_pos[__min(63,__max(0,(pos)-(1-1)))];

  d = 0;
  d = d*6 + H1_sig_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*41 + H1_sig_d_pos[__min(63,__max(0,(pos)-(1-1)))];
  d = d*6 + H1_sig_d_rem[__min(10,__max(0,(rem)-(1-1)))];
  d = d*13 + H1_sig_d_h3[__min(14,__max(0,(h3)-(1-1)))];

  m = 0;
  m = m*4 + H1_sig_m_cg[__min(13,__max(0,(cg)-(1-1)))];
  m = m*3 + (((pos)>0+(1-1))+((pos)>5+(1-1)));
  m = m*5 + H1_sig_m_rem[__min(10,__max(0,(rem)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_sig
#define FC_PFX H1_sig_
#define FC_FAM sig
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
