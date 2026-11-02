/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l7 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -7: the ten counters, the three mixers and the final one, no APMs.  Copied through into
    MOD/l7/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l7/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3` the three mixers
    (selected by indices m, w and s) and `_WF` the one that mixes them (by
    node), `_MT` the mantissa plane, `_G` the sign counters.  Each is
    H_hdr_ND (or H_hdr_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void H7_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, g;
  H_hdr_VARS(H_LOCAL)

  a = 0;
  a = (a<<8) + (((tag)&255)); // 11111111
  a = a*3 + (((fld)>13+(1-1))+((fld)>14+(1-1)));

  b = 0;
  b = (b<<4) + (((((tag)>>3)&3)<<2) + ((tag)&3)); // 00011011
  b = (b<<6) + (((((p1r)>>6)&3)<<4) + ((((p1r)>>2)&7)<<1) + ((p1r)&1)); // 11011101

  c = 0;
  c = (c<<8) + (((tag)&255)); // 11111111
  c = c*9 + H7_hdr_c_p1[__min(10,__max(0,(p1)-(1-1)))];
  c = c*7 + H7_hdr_c_p2[__min(6,__max(0,(p2)-(1-1)))];

  d = 0;
  d = (d<<8) + (((tag)&255)); // 11111111
  d = d*3 + (((fld)>3+(1-1))+((fld)>6+(1-1)));
  d = d*61 + H7_hdr_d_dm[__min(60,__max(0,(dm)-(1-1)))];

  e = 0;
  e = (e<<6) + (((tag)&63)); // 00111111
  e = e*5 + H7_hdr_e_ws[__min(4,__max(0,(ws)-(1-1)))];
  e = e*6 + H7_hdr_e_fld[__min(7,__max(0,(fld)-(1-1)))];
  e = e*5 + H7_hdr_e_ml[__min(7,__max(0,(ml)-(1-1)))];

  h = 0;
  h = (h<<7) + (((((tag)>>7)&1)<<6) + ((tag)&63)); // 10111111
  h = h*3 + (((ch)>0+(1-1))+((ch)>1+(1-1)));
  h = h*13 + H7_hdr_h_fld[__min(15,__max(0,(fld)-(1-1)))];
  h = h*10 + H7_hdr_h_hx[__min(20,__max(0,(hx)-(1-1)))];

  i = 0;
  i = (i<<8) + (((tag)&255)); // 11111111
  i = i*7 + H7_hdr_i_pb[__min(8,__max(0,(pb)-(1-1)))];
  i = (i<<6) + (((((p1r)>>6)&3)<<4) + ((p1r)&15)); // 11001111

  j = 0;
  j = (j<<8) + (((tag)&255)); // 11111111
  j = j*4 + H7_hdr_j_pos[__min(3,__max(0,(pos)-(1-1)))];

  k = 0;
  k = (k<<4) + (((((tag)>>4)&3)<<2) + ((tag)&3)); // 00110011
  k = k*6 + H7_hdr_k_nb[__min(8,__max(0,(nb)-(1-1)))];
  k = (k<<5) + (((((p1r)>>6)&3)<<3) + ((p1r)&7)); // 11000111

  l = 0;
  l = (l<<7) + (((((tag)>>4)&15)<<3) + ((tag)&7)); // 11110111
  l = (l<<5) + (((((p1r)>>7)&1)<<4) + ((((p1r)>>3)&3)<<2) + ((p1r)&3)); // 10011011
  l = l*4 + H7_hdr_l_fld[__min(4,__max(0,(fld)-(1-1)))];

  s = 0;

  m = 0;
  m = (m<<8) + (((tag)&255)); // 11111111

  w = 0;
  w = w*4 + H7_hdr_w_nb[__min(8,__max(0,(nb)-(1-1)))];

  t = 0;
  t = (t<<4) + (((((tag)>>6)&1)<<3) + ((tag)&7)); // 01000111

  g = 0;
  g = g*6 + H7_hdr_g_p1[__min(6,__max(0,(p1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C7_hdr
#define FC_PFX H7_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 0
#include "../../famcfg.inc"
