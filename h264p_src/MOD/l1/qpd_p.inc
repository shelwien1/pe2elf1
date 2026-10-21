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
    mixer, `_G` the sign counters.  Each is H_qpd_ND (or H_qpd_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_qpd_make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_qpd_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((pc)>0+(1-1))+((pc)>1+(1-1)));
  a = a*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  a = a*3 + (((cls)>3+(1-1))+((cls)>5+(1-1)));
  a = a*8 + H1_qpd_a_sq[__min(13,__max(0,(sq)-(1-1)))];

  b = 0;
  b = b*3 + (((cls)>2+(1-1))+((cls)>6+(1-1)));
  b = b*8 + H1_qpd_b_sq[__min(13,__max(0,(sq)-(1-1)))];

  c = 0;
  c = c*4 + H1_qpd_c_lz[__min(6,__max(0,(lz)-(1-1)))];
  c = c*2 + (((st)>1+(1-1)));
  c = c*6 + H1_qpd_c_db[__min(8,__max(0,(db)-(1-1)))];
  c = c*7 + H1_qpd_c_bc[__min(13,__max(0,(bc)-(1-1)))];

  d = 0;
  d = d*4 + H1_qpd_d_st[__min(9,__max(0,(st)-(1-1)))];
  d = d*13 + H1_qpd_d_cl[__min(13,__max(0,(cl)-(1-1)))];

  m = 0;
  m = m*2 + (((lnz)>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_qpd
#define FC_PFX H1_qpd_
#define FC_FAM qpd
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
