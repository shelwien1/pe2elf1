/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -1: four counters, which keep their probabilities stretched (cm_cnt2s), and one mixer, no APMs.  Copied through into
    MOD/l1/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one mixer (selected by
    index m, rows of 16 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_esc_ND (or H_esc_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H1_esc_make( const H_esc_v & D, tcx & X ) {
  int a, b, c, d, m, t, g;
  H_esc_VARS(H_LOCAL)

  a = 0;
  a = a*13 + H1_esc_a_a1[__min(12,__max(0,(a1)-(1-1)))];
  a = a*6 + H1_esc_a_la[__min(5,__max(0,(la)-(1-1)))];
  a = a*40 + H1_esc_a_xp[__min(39,__max(0,(xp)-(1-1)))];

  b = 0;
  b = b*51 + H1_esc_b_sb[__min(50,__max(0,(sb)-(1-1)))];

  c = 0;
  c = c*13 + H1_esc_c_la[__min(12,__max(0,(la)-(1-1)))];
  c = c*6 + H1_esc_c_kq[__min(5,__max(0,(kq)-(1-1)))];

  d = 0;
  d = d*13 + H1_esc_d_pf[__min(12,__max(0,(pf)-(1-1)))];
  d = d*6 + H1_esc_d_oc[__min(5,__max(0,(oc)-(1-1)))];

  m = 0;
  m = m*2 + (((ws)>0+(1-1)));

  t = 0;
  t = t*20 + H1_esc_t_kq[__min(19,__max(0,(kq)-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_esc
#define FC_PFX H1_esc_
#define FC_FAM esc
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
