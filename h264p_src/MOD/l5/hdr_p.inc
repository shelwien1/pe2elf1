/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a
    family's index builder, the tables its indices address and its
    configuration class, at level -5.  Copied through into
    MOD/l5/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_hdr_ND (or H_hdr_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_hdr_VARS(H_LOCAL)

  a = 0;
  a = (a<<6) + (((((tag)>>6)&1)<<5) + ((tag)&31)); // 01011111
  a = a*4 + H5_hdr_a_fld[__min(3,__max(0,(fld)-(1-1)))];
  a = a*5 + H5_hdr_a_p1[__min(7,__max(0,(p1)-(1-1)))];
  a = a*3 + (((sty)>1+(1-1))+((sty)>2+(1-1)));
  a = a*2 + (((pr)>0+(1-1)));

  b = 0;
  b = (b<<7) + (((tag)&127)); // 01111111
  b = (b<<8) + (((p1r)&255)); // 11111111

  c = 0;
  c = (c<<7) + (((tag)&127)); // 01111111
  c = c*6 + H5_hdr_c_fld[__min(15,__max(0,(fld)-(1-1)))];

  d = 0;
  d = (d<<6) + (((tag)&63)); // 00111111
  d = d*6 + H5_hdr_d_p1[__min(15,__max(0,(p1)-(1-1)))];
  d = d*7 + H5_hdr_d_p2[__min(7,__max(0,(p2)-(1-1)))];

  e = 0;
  e = (e<<3) + (((((tag)>>4)&1)<<2) + ((((tag)>>2)&1)<<1) + ((tag)&1)); // 00010101
  e = e*2 + (((pos)>1+(1-1)));

  h = 0;
  h = (h<<5) + ((((tag)>>1)&31)); // 00111110
  h = h*5 + H5_hdr_h_sty[__min(9,__max(0,(sty)-(1-1)))];

  i = 0;
  i = (i<<6) + (((((tag)>>3)&15)<<2) + ((tag)&3)); // 01111011

  j = 0;
  j = j*2 + (((fld)>0+(1-1)));
  j = j*4 + H5_hdr_j_p2[__min(7,__max(0,(p2)-(1-1)))];

  k = 0;
  k = (k<<7) + (((tag)&127)); // 01111111
  k = k*16 + H5_hdr_k_fld[__min(15,__max(0,(fld)-(1-1)))];

  l = 0;
  l = l*2 + (((pos)>1+(1-1)));

  m = 0;

  t = 0;
  t = (t<<4) + ((((tag)>>1)&15)); // 00011110

  n = 0;

  g = 0;
  g = g*7 + H5_hdr_g_p1[__min(7,__max(0,(p1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_hdr
#define FC_PFX H5_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
