/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -3: six counters and one mixer, no APMs.  Copied through into
    MOD/l3/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by
    index m, rows of 16 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_hdr_VARS(H_LOCAL)

  a = 0;
  a = (a<<3) + (((((tag)>>7)&1)<<2) + ((((tag)>>4)&1)<<1) + ((tag)&1)); // 10010001
  a = a*3 + (((fld)>0+(1-1))+((fld)>6+(1-1)));
  a = a*2 + (((p1)>3+(1-1)));
  a = a*61 + H3_hdr_a_dm[__min(60,__max(0,(dm)-(1-1)))];

  b = 0;
  b = (b<<8) + (((tag)&255)); // 11111111
  b = b*3 + (((ch)>0+(1-1))+((ch)>1+(1-1)));
  b = b*13 + H3_hdr_b_fld[__min(15,__max(0,(fld)-(1-1)))];
  b = b*9 + H3_hdr_b_hx[__min(20,__max(0,(hx)-(1-1)))];

  c = 0;
  c = (c<<7) + (((((tag)>>4)&15)<<3) + ((tag)&7)); // 11110111
  c = c*4 + H3_hdr_c_pb[__min(8,__max(0,(pb)-(1-1)))];
  c = (c<<5) + (((((p1r)>>6)&3)<<3) + ((((p1r)>>3)&1)<<2) + ((p1r)&3)); // 11001011

  d = 0;
  d = (d<<6) + (((tag)&63)); // 00111111
  d = d*5 + H3_hdr_d_ws[__min(4,__max(0,(ws)-(1-1)))];
  d = d*7 + H3_hdr_d_fld[__min(7,__max(0,(fld)-(1-1)))];
  d = d*5 + H3_hdr_d_ml[__min(7,__max(0,(ml)-(1-1)))];

  e = 0;
  e = (e<<8) + (((tag)&255)); // 11111111
  e = e*4 + H3_hdr_e_p1[__min(10,__max(0,(p1)-(1-1)))];
  e = e*3 + (((p2)>0+(1-1))+((p2)>1+(1-1)));

  h = 0;
  h = (h<<7) + ((((tag)>>1)&127)); // 11111110
  h = h*3 + (((nb)>1+(1-1))+((nb)>2+(1-1)));
  h = (h<<4) + (((((p1r)>>7)&1)<<3) + ((p1r)&7)); // 10000111

  m = 0;
  m = (m<<3) + (((((tag)>>7)&1)<<2) + (((tag)>>3)&3)); // 10011000

  t = 0;
  t = (t<<4) + (((((tag)>>6)&1)<<3) + ((tag)&7)); // 01000111

  g = 0;
  g = g*6 + H3_hdr_g_p1[__min(6,__max(0,(p1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_hdr
#define FC_PFX H3_hdr_
#define FC_FAM hdr
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
