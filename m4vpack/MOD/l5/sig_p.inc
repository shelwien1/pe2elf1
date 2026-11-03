/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_sig_ND (or M_sig_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M5_sig_make( const M_sig_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_sig_VARS(M_LOCAL)

  a = 0;
  a = a*13 + M5_sig_a_nc[__min(17,__max(0,(nc)-(1-1)))];
  a = a*3 + (((qs)>2+(1-1))+((qs)>4+(1-1)));
  a = (a<<1) + ((((pos)>>5)&1)); // 100000

  b = 0;
  b = b*2 + (((kb)>13+(1-1)));
  b = (b<<4) + (((((pos)>>4)&3)<<2) + (((pos)>>1)&3)); // 110110

  c = 0;
  c = (c<<1) + (((cls)&1)); // 001
  c = c*8 + M5_sig_c_nz[__min(8,__max(0,(nz)-(1-1)))];
  c = (c<<1) + ((((pos)>>3)&1)); // 001000

  d = 0;
  d = (d<<1) + ((((cls)>>1)&1)); // 010
  d = d*8 + M5_sig_d_past[__min(8,__max(0,(past)-(1-1)))];
  d = d*5 + M5_sig_d_qs[__min(32,__max(0,(qs)-(1-1)))];
  d = (d<<6) + (((pos)&63)); // 111111

  e = 0;
  e = (e<<3) + (((cls)&7)); // 111
  e = e*15 + M5_sig_e_run[__min(16,__max(0,(run)-(1-1)))];
  e = (e<<6) + (((pos)&63)); // 111111

  h = 0;
  h = (h<<2) + ((((cls)>>1)&3)); // 110
  h = (h<<5) + (((fp)&31)); // 11111
  h = (h<<6) + (((pos)&63)); // 111111

  i = 0;
  i = (i<<3) + (((cls)&7)); // 111
  i = i*12 + M5_sig_i_mvc[__min(30,__max(0,(mvc)-(1-1)))];
  i = (i<<6) + (((pos)&63)); // 111111

  j = 0;
  j = (j<<1) + (((cls)&1)); // 001
  j = j*7 + M5_sig_j_rnz[__min(17,__max(0,(rnz)-(1-1)))];
  j = j*5 + M5_sig_j_xm[__min(9,__max(0,(xm)-(1-1)))];
  j = (j<<3) + ((((pos)>>3)&7)); // 111000

  k = 0;
  k = (k<<1) + ((((cls)>>1)&1)); // 010
  k = (k<<6) + (((((h8)>>7)&1)<<5) + ((((h8)>>4)&3)<<3) + ((h8)&7)); // 10110111
  k = (k<<6) + (((pos)&63)); // 111111

  l = 0;
  l = (l<<2) + ((((cls)>>1)&3)); // 110
  l = (l<<6) + (((pos)&63)); // 111111
  l = (l<<13) + (((((h16)>>10)&63)<<7) + ((((h16)>>8)&1)<<6) + ((((h16)>>6)&1)<<5) + ((h16)&31)); // 1111110101011111

  m = 0;
  m = (m<<1) + (((cls)&1)); // 001
  m = (m<<5) + ((((pos)>>1)&31)); // 111110

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_sig
#define FC_PFX M5_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
