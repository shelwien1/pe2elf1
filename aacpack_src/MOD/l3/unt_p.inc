/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -3: six counters and one mixer, no APMs.  Copied through into
    MOD/l3/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by
    index m, rows of 16 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_unt_ND (or H_unt_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_unt_make( const H_unt_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_unt_VARS(H_LOCAL)

  a = 0;
  a = a*16 + H3_unt_a_fld[__min(15,__max(0,(fld)-(1-1)))];

  b = 0;
  b = b*16 + H3_unt_b_fld[__min(15,__max(0,(fld)-(1-1)))];
  b = b*16 + H3_unt_b_ctx[__min(15,__max(0,(ctx)-(1-1)))];

  c = 0;
  c = c*16 + H3_unt_c_fld[__min(15,__max(0,(fld)-(1-1)))];
  c = c*7 + H3_unt_c_p1[__min(6,__max(0,(p1)-(1-1)))];

  d = 0;
  d = d*16 + H3_unt_d_fld[__min(15,__max(0,(fld)-(1-1)))];
  d = d*8 + H3_unt_d_ctx[__min(7,__max(0,(ctx)-(1-1)))];
  d = d*3 + (((p1)>0+(1-1))+((p1)>1+(1-1)));

  e = 0;
  e = e*16 + H3_unt_e_fld[__min(15,__max(0,(fld)-(1-1)))];
  e = e*11 + H3_unt_e_p1[__min(10,__max(0,(p1)-(1-1)))];

  h = 0;
  h = h*16 + H3_unt_h_fld[__min(15,__max(0,(fld)-(1-1)))];
  h = h*8 + H3_unt_h_ctx[__min(7,__max(0,(ctx)-(1-1)))];

  m = 0;
  m = m*16 + H3_unt_m_fld[__min(15,__max(0,(fld)-(1-1)))];

  t = 0;
  t = t*16 + H3_unt_t_fld[__min(15,__max(0,(fld)-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_unt
#define FC_PFX H3_unt_
#define FC_FAM unt
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
