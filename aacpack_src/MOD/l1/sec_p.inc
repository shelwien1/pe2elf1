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
    counters.  Each is H_sec_ND (or H_sec_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H1_sec_make( const H_sec_v & D, tcx & X ) {
  int a, b, c, d, m, t, g;
  H_sec_VARS(H_LOCAL)

  a = 0;
  a = a*4 + H1_sec_a_kd[__min(3,__max(0,(kd)-(1-1)))];
  a = a*17 + H1_sec_a_oc[__min(16,__max(0,(oc)-(1-1)))];
  a = a*4 + H1_sec_a_pc[__min(3,__max(0,(pc)-(1-1)))];
  a = a*3 + (((mu)>0+(1-1))+((mu)>1+(1-1)));
  a = a*17 + H1_sec_a_pr[__min(16,__max(0,(pr)-(1-1)))];

  b = 0;
  b = b*4 + H1_sec_b_kd[__min(3,__max(0,(kd)-(1-1)))];
  b = b*17 + H1_sec_b_gc[__min(16,__max(0,(gc)-(1-1)))];
  b = b*4 + H1_sec_b_ws[__min(3,__max(0,(ws)-(1-1)))];
  b = b*13 + H1_sec_b_ch[__min(12,__max(0,(ch)-(1-1)))];

  c = 0;
  c = c*4 + H1_sec_c_kd[__min(3,__max(0,(kd)-(1-1)))];
  c = c*17 + H1_sec_c_run[__min(16,__max(0,(run)-(1-1)))];
  c = c*17 + H1_sec_c_cb[__min(16,__max(0,(cb)-(1-1)))];
  c = c*13 + H1_sec_c_ch[__min(12,__max(0,(ch)-(1-1)))];

  d = 0;
  d = d*4 + H1_sec_d_kd[__min(3,__max(0,(kd)-(1-1)))];
  d = d*17 + H1_sec_d_pc[__min(16,__max(0,(pc)-(1-1)))];
  d = d*17 + H1_sec_d_fc[__min(16,__max(0,(fc)-(1-1)))];
  d = d*17 + H1_sec_d_pr[__min(16,__max(0,(pr)-(1-1)))];

  m = 0;
  m = m*4 + H1_sec_m_kd[__min(3,__max(0,(kd)-(1-1)))];
  m = m*5 + H1_sec_m_bq[__min(4,__max(0,(bq)-(1-1)))];
  m = m*7 + H1_sec_m_pr[__min(6,__max(0,(pr)-(1-1)))];

  t = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_sec
#define FC_PFX H1_sec_
#define FC_FAM sec
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
