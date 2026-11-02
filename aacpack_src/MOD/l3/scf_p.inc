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
    counters.  Each is H_scf_ND (or H_scf_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_scf_make( const H_scf_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_scf_VARS(H_LOCAL)

  a = 0;
  a = a*7 + H3_scf_a_oc[__min(13,__max(0,(oc)-(1-1)))];
  a = a*3 + (((mu)>0+(1-1))+((mu)>1+(1-1)));

  b = 0;
  b = b*2 + (((kd)>1+(1-1)));
  b = b*38 + H3_scf_b_sb[__min(50,__max(0,(sb)-(1-1)))];
  b = b*2 + (((ws)>0+(1-1)));

  c = 0;
  c = c*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  c = c*15 + H3_scf_c_pa[__min(14,__max(0,(pa)-(1-1)))];
  c = c*6 + H3_scf_c_d1[__min(5,__max(0,(d1)-(1-1)))];

  d = 0;
  d = d*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  d = d*10 + H3_scf_d_pv[__min(10,__max(0,(pv)-(1-1)))];
  d = d*3 + (((sb)>1+(1-1))+((sb)>4+(1-1)));
  d = d*4 + H3_scf_d_sk[__min(15,__max(0,(sk)-(1-1)))];

  e = 0;
  e = e*9 + H3_scf_e_d1[__min(13,__max(0,(d1)-(1-1)))];
  e = e*2 + (((d2)>4+(1-1)));
  e = e*61 + H3_scf_e_dm[__min(60,__max(0,(dm)-(1-1)))];

  h = 0;
  h = h*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  h = h*11 + H3_scf_h_pa[__min(12,__max(0,(pa)-(1-1)))];
  h = h*5 + H3_scf_h_pd[__min(5,__max(0,(pd)-(1-1)))];

  m = 0;
  m = m*2 + (((kd)>1+(1-1)));
  m = m*2 + (((mu)>1+(1-1)));

  t = 0;
  t = t*2 + (((kd)>1+(1-1)));

  g = 0;
  g = g*6 + H3_scf_g_d1[__min(5,__max(0,(d1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_scf
#define FC_PFX H3_scf_
#define FC_FAM scf
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
