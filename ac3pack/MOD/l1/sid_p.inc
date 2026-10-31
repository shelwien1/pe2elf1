/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_sid_ND (or M_sid_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_sid_make( const M_sid_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_sid_VARS(M_LOCAL)

  a = 0;

  b = 0;
  b = (b<<5) + (((((fld)>>5)&1)<<4) + ((fld)&15)); // 101111
  b = (b<<6) + (((lv)&63)); // 111111

  c = 0;
  c = (c<<5) + (((((fld)>>3)&7)<<2) + ((fld)&3)); // 111011
  c = (c<<5) + (((lv)&31)); // 011111
  c = c*3 + (((pr)>0+(1-1))+((pr)>1+(1-1)));
  c = c*3 + (((lb)>0+(1-1))+((lb)>1+(1-1)));

  d = 0;
  d = (d<<6) + (((fld)&63)); // 111111
  d = (d<<6) + (((pq)&63)); // 111111
  d = d*2 + (((lb)>1+(1-1)));

  m = 0;
  m = (m<<5) + (((((fld)>>4)&3)<<3) + ((fld)&7)); // 110111

  t = 0;
  t = (t<<5) + (((((fld)>>3)&7)<<2) + ((fld)&3)); // 111011

  n = 0;
  n = (n<<6) + (((fld)&63)); // 111111

  g = 0;
  g = (g<<6) + (((fld)&63)); // 111111
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_sid
#define FC_PFX A1_sid_
#define FC_FAM sid
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
