/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_raw_ND (or M_raw_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_raw_make( const M_raw_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_raw_VARS(M_LOCAL)

  a = 0;

  b = 0;
  b = (b<<5) + (((((c1)>>6)&1)<<4) + ((c1)&15)); // 01001111

  c = 0;
  c = (c<<5) + (((((c1)>>6)&3)<<3) + ((((c1)>>3)&1)<<2) + ((c1)&3)); // 11001011
  c = (c<<5) + (((((c2)>>6)&1)<<4) + ((c2)&15)); // 01001111

  d = 0;
  d = d*6 + M5_raw_d_code[__min(11,__max(0,(code)-(1-1)))];

  e = 0;
  e = e*4 + M5_raw_e_code[__min(11,__max(0,(code)-(1-1)))];
  e = e*7 + M5_raw_e_at[__min(11,__max(0,(at)-(1-1)))];

  h = 0;
  h = (h<<5) + (((c1)&31)); // 00011111
  h = h*4 + M5_raw_h_at[__min(11,__max(0,(at)-(1-1)))];

  i = 0;
  i = (i<<4) + (((((c1)>>7)&1)<<3) + ((c1)&7)); // 10000111
  i = i*3 + (((code)>0+(1-1))+((code)>1+(1-1)));

  j = 0;
  j = j*6 + M5_raw_j_at[__min(11,__max(0,(at)-(1-1)))];

  k = 0;
  k = (k<<6) + (((((c2)>>3)&15)<<2) + ((c2)&3)); // 01111011

  l = 0;
  l = (l<<6) + (((((c2)>>7)&1)<<5) + ((((c2)>>4)&3)<<3) + ((c2)&7)); // 10110111
  l = l*4 + M5_raw_l_code[__min(11,__max(0,(code)-(1-1)))];

  m = 0;
  m = m*8 + M5_raw_m_at[__min(11,__max(0,(at)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_raw
#define FC_PFX M5_raw_
#define FC_FAM raw
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
