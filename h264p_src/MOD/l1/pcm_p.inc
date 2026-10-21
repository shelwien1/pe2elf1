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
    mixer, `_G` the sign counters.  Each is H_pcm_ND (or H_pcm_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_pcm_make( const H_pcm_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_pcm_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));
  a = a*7 + H1_pcm_a_dp[__min(6,__max(0,(dp)-(1-1)))];
  a = a*16 + H1_pcm_a_pd[__min(15,__max(0,(pd)-(1-1)))];

  b = 0;
  b = b*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));
  b = b*8 + H1_pcm_b_pd[__min(7,__max(0,(pd)-(1-1)))];
  b = b*8 + H1_pcm_b_pd2[__min(7,__max(0,(pd2)-(1-1)))];

  c = 0;
  c = c*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));

  d = 0;
  d = d*7 + H1_pcm_d_dp[__min(6,__max(0,(dp)-(1-1)))];

  m = 0;
  m = m*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));

  t = 0;
  t = t*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));
  t = t*7 + H1_pcm_t_dp[__min(6,__max(0,(dp)-(1-1)))];

  n = 0;
  n = n*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));

  g = 0;
  g = g*3 + (((pl)>0+(1-1))+((pl)>1+(1-1)));
  g = g*4 + H1_pcm_g_pd[__min(3,__max(0,(pd)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_pcm
#define FC_PFX H1_pcm_
#define FC_FAM pcm
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
