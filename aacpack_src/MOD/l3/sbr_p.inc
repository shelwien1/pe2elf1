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
    counters.  Each is H_sbr_ND (or H_sbr_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_sbr_make( const H_sbr_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_sbr_VARS(H_LOCAL)

  a = 0;
  a = a*9 + H3_sbr_a_kd[__min(14,__max(0,(kd)-(1-1)))];
  a = a*12 + H3_sbr_a_bd[__min(30,__max(0,(bd)-(1-1)))];
  a = a*2 + (((ar)>0+(1-1)));

  b = 0;
  b = b*4 + H3_sbr_b_kd[__min(14,__max(0,(kd)-(1-1)))];
  b = b*6 + H3_sbr_b_lv[__min(11,__max(0,(lv)-(1-1)))];

  c = 0;
  c = c*7 + H3_sbr_c_kd[__min(14,__max(0,(kd)-(1-1)))];
  c = c*5 + H3_sbr_c_d1[__min(10,__max(0,(d1)-(1-1)))];
  c = c*7 + H3_sbr_c_d2[__min(6,__max(0,(d2)-(1-1)))];
  c = c*6 + H3_sbr_c_rm[__min(14,__max(0,(rm)-(1-1)))];

  d = 0;
  d = d*7 + H3_sbr_d_kd[__min(14,__max(0,(kd)-(1-1)))];
  d = d*2 + (((ar)>0+(1-1)));
  d = d*2 + (((fr)>0+(1-1)));
  d = d*6 + H3_sbr_d_bd[__min(12,__max(0,(bd)-(1-1)))];

  e = 0;
  e = e*7 + H3_sbr_e_kd[__min(14,__max(0,(kd)-(1-1)))];
  e = e*6 + H3_sbr_e_d1[__min(10,__max(0,(d1)-(1-1)))];
  e = e*6 + H3_sbr_e_pv[__min(5,__max(0,(pv)-(1-1)))];
  e = e*7 + H3_sbr_e_rs[__min(14,__max(0,(rs)-(1-1)))];

  h = 0;
  h = h*4 + H3_sbr_h_kd[__min(14,__max(0,(kd)-(1-1)))];
  h = h*21 + H3_sbr_h_nb[__min(20,__max(0,(nb)-(1-1)))];
  h = h*4 + H3_sbr_h_bd[__min(3,__max(0,(bd)-(1-1)))];

  m = 0;
  m = m*6 + H3_sbr_m_kd[__min(14,__max(0,(kd)-(1-1)))];

  t = 0;
  t = t*3 + (((kd)>0+(1-1))+((kd)>12+(1-1)));

  g = 0;
  g = g*6 + H3_sbr_g_d1[__min(5,__max(0,(d1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_sbr
#define FC_PFX H3_sbr_
#define FC_FAM sbr
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
