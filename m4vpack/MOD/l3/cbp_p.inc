/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_cbp_ND (or M_cbp_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_cbp_make( const M_cbp_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_cbp_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((c3)>1+(1-1)));
  a = a*3 + (((rbit)>0+(1-1))+((rbit)>1+(1-1)));
  a = a*6 + M3_cbp_a_cnt[__min(5,__max(0,(cnt)-(1-1)))];

  b = 0;
  b = b*2 + (((c3)>0+(1-1)));
  b = b*2 + (((rbit)>0+(1-1)));
  b = b*8 + M3_cbp_b_lu[__min(8,__max(0,(lu)-(1-1)))];

  c = 0;
  c = c*2 + (((c3)>1+(1-1)));
  c = c*8 + M3_cbp_c_mvc[__min(17,__max(0,(mvc)-(1-1)))];

  d = 0;
  d = d*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));
  d = (d<<5) + (((pat)&31)); // 11111

  e = 0;
  e = e*2 + (((c3)>1+(1-1)));
  e = e*8 + M3_cbp_e_lu[__min(8,__max(0,(lu)-(1-1)))];
  e = e*3 + (((lq)>0+(1-1))+((lq)>1+(1-1)));

  h = 0;
  h = h*5 + M3_cbp_h_frac[__min(4,__max(0,(frac)-(1-1)))];

  m = 0;
  m = m*3 + (((c3)>0+(1-1))+((c3)>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_cbp
#define FC_PFX M3_cbp_
#define FC_FAM cbp
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
