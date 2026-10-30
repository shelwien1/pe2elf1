/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_dcs_ND (or M_dcs_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_dcs_make( const M_dcs_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_dcs_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((lc)>0+(1-1)));
  a = a*7 + M3_dcs_a_lbn[__min(17,__max(0,(lbn)-(1-1)))];
  a = a*9 + M3_dcs_a_abn[__min(17,__max(0,(abn)-(1-1)))];

  b = 0;
  b = b*2 + (((lc)>0+(1-1)));
  b = b*10 + M3_dcs_b_ls[__min(14,__max(0,(ls)-(1-1)))];
  b = b*10 + M3_dcs_b_as[__min(14,__max(0,(as)-(1-1)))];

  c = 0;
  c = c*2 + (((lc)>0+(1-1)));
  c = c*3 + (((ls2)>0+(1-1))+((ls2)>1+(1-1)));
  c = c*3 + (((as2)>0+(1-1))+((as2)>1+(1-1)));
  c = c*3 + (((psg)>0+(1-1))+((psg)>1+(1-1)));

  d = 0;
  d = d*4 + M3_dcs_d_as[__min(14,__max(0,(as)-(1-1)))];
  d = d*6 + M3_dcs_d_prev[__min(13,__max(0,(prev)-(1-1)))];

  e = 0;
  e = e*2 + (((lc)>0+(1-1)));
  e = e*7 + M3_dcs_e_sz[__min(12,__max(0,(sz)-(1-1)))];
  e = e*2 + (((psg)>1+(1-1)));

  h = 0;
  h = h*2 + (((lc)>0+(1-1)));
  h = h*2 + (((coded)>0+(1-1)));
  h = h*6 + M3_dcs_h_qs[__min(32,__max(0,(qs)-(1-1)))];

  m = 0;
  m = m*2 + (((coded)>0+(1-1)));

  t = 0;
  t = t*2 + (((lc)>0+(1-1)));
  t = t*6 + M3_dcs_t_prev[__min(13,__max(0,(prev)-(1-1)))];

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_dcs
#define FC_PFX M3_dcs_
#define FC_FAM dcs
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
