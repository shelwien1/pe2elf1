/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_aht_ND (or M_aht_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_aht_make( const M_aht_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_aht_VARS(M_LOCAL)

  a = 0;
  a = a*5 + A1_aht_a_kd[__min(4,__max(0,(kd)-(1-1)))];
  a = a*14 + A1_aht_a_hb[__min(19,__max(0,(hb)-(1-1)))];

  b = 0;
  b = b*5 + A1_aht_b_kd[__min(4,__max(0,(kd)-(1-1)))];
  b = b*20 + A1_aht_b_hb[__min(19,__max(0,(hb)-(1-1)))];
  b = b*3 + (((lg)>0+(1-1))+((lg)>1+(1-1)));
  b = b*6 + A1_aht_b_bk[__min(5,__max(0,(bk)-(1-1)))];

  c = 0;
  c = c*5 + A1_aht_c_kd[__min(4,__max(0,(kd)-(1-1)))];
  c = c*20 + A1_aht_c_hb[__min(19,__max(0,(hb)-(1-1)))];
  c = c*24 + A1_aht_c_q1[__min(23,__max(0,(q1)-(1-1)))];

  d = 0;
  d = d*5 + A1_aht_d_kd[__min(4,__max(0,(kd)-(1-1)))];
  d = d*6 + A1_aht_d_bk[__min(5,__max(0,(bk)-(1-1)))];
  d = d*24 + A1_aht_d_qa[__min(23,__max(0,(qa)-(1-1)))];
  d = d*3 + (((lg)>0+(1-1))+((lg)>1+(1-1)));

  m = 0;
  m = m*5 + A1_aht_m_kd[__min(4,__max(0,(kd)-(1-1)))];
  m = m*20 + A1_aht_m_hb[__min(19,__max(0,(hb)-(1-1)))];

  t = 0;
  t = t*5 + A1_aht_t_kd[__min(4,__max(0,(kd)-(1-1)))];
  t = t*20 + A1_aht_t_hb[__min(19,__max(0,(hb)-(1-1)))];

  n = 0;
  n = n*5 + A1_aht_n_kd[__min(4,__max(0,(kd)-(1-1)))];

  g = 0;
  g = g*20 + A1_aht_g_hb[__min(19,__max(0,(hb)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_aht
#define FC_PFX A1_aht_
#define FC_FAM aht
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
