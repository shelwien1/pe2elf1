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
    mixer, `_G` the sign counters.  Each is H_cbp_ND (or H_cbp_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_cbp_make( const H_cbp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_cbp_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((inc)>0+(1-1))+((inc)>2+(1-1)));
  a = a*4 + H1_cbp_a_pc[__min(4,__max(0,(pc)-(1-1)))];
  a = a*4 + H1_cbp_a_cls[__min(8,__max(0,(cls)-(1-1)))];
  a = a*4 + H1_cbp_a_st[__min(9,__max(0,(st)-(1-1)))];

  b = 0;
  b = b*3 + (((col)>0+(1-1))+((col)>1+(1-1)));
  b = b*2 + (((q6)>2+(1-1)));

  c = 0;
  c = c*4 + H1_cbp_c_cls[__min(8,__max(0,(cls)-(1-1)))];
  c = c*2 + (((col)>1+(1-1)));
  c = c*3 + (((pc)>0+(1-1))+((pc)>2+(1-1)));

  d = 0;
  d = d*2 + (((inc)>1+(1-1)));
  d = d*2 + (((pc)>1+(1-1)));
  d = d*5 + H1_cbp_d_q6[__min(8,__max(0,(q6)-(1-1)))];

  m = 0;
  m = m*4 + H1_cbp_m_inc[__min(3,__max(0,(inc)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_cbp
#define FC_PFX H1_cbp_
#define FC_FAM cbp
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
