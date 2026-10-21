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
    mixer, `_G` the sign counters.  Each is H_ipm_ND (or H_ipm_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_ipm_make( const H_ipm_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_ipm_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((k8)>0+(1-1)));
  a = a*11 + H1_ipm_a_ma[__min(10,__max(0,(ma)-(1-1)))];
  a = a*11 + H1_ipm_a_mb[__min(10,__max(0,(mb)-(1-1)))];
  a = a*10 + H1_ipm_a_mc[__min(10,__max(0,(mc)-(1-1)))];

  b = 0;
  b = b*2 + (((k8)>0+(1-1)));
  b = b*16 + H1_ipm_b_mh[__min(16,__max(0,(mh)-(1-1)))];
  b = b*11 + H1_ipm_b_mb[__min(10,__max(0,(mb)-(1-1)))];

  c = 0;
  c = c*8 + H1_ipm_c_mp[__min(9,__max(0,(mp)-(1-1)))];
  c = c*10 + H1_ipm_c_col[__min(9,__max(0,(col)-(1-1)))];

  d = 0;

  m = 0;
  m = m*2 + (((k8)>0+(1-1)));
  m = m*2 + (((ma)>9+(1-1)));
  m = m*2 + (((mb)>9+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_ipm
#define FC_PFX H1_ipm_
#define FC_FAM ipm
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
