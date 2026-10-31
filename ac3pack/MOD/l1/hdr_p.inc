/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a family's index builder,
    the tables its indices address and its configuration class, at level -1.  Copied through
    into MOD/l1/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one
    mixer (selected by index m, rows of 16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_hdr_ND (or M_hdr_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A1_hdr_make( const M_hdr_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  M_hdr_VARS(M_LOCAL)

  a = 0;
  a = (a<<1) + ((((fld)>>6)&1)); // 1000000

  b = 0;
  b = (b<<7) + (((fld)&127)); // 1111111
  b = (b<<4) + ((((lv)>>1)&15)); // 011110

  c = 0;
  c = (c<<2) + ((((fld)>>5)&3)); // 1100000
  c = (c<<4) + ((((lv)>>1)&15)); // 011110

  d = 0;
  d = (d<<7) + (((fld)&127)); // 1111111
  d = (d<<5) + (((((lv)>>2)&15)<<1) + ((lv)&1)); // 111101
  d = d*9 + A1_hdr_d_pos[__min(31,__max(0,(pos)-(1-1)))];

  m = 0;
  m = (m<<5) + ((((fld)>>2)&31)); // 1111100

  t = 0;
  t = (t<<4) + ((((fld)>>2)&15)); // 0111100

  n = 0;
  n = (n<<7) + (((fld)&127)); // 1111111

  g = 0;
  g = (g<<7) + (((fld)&127)); // 1111111
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C1_hdr
#define FC_PFX A1_hdr_
#define FC_FAM hdr
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
