/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -9.  Copied through into
    MOD/l9/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and
    s) and `_WF` the one that mixes them (by node), `_MT` the mantissa
    plane, `_G` the sign counters.  Each is H_hdr_ND (or H_hdr_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H9_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, g;
  H_hdr_VARS(H_LOCAL)

  a = 0;
  a = (a<<8) + (((tag)&255)); // 11111111
  a = a*16 + H9_hdr_a_fld[__min(15,__max(0,(fld)-(1-1)))];

  b = 0;
  b = (b<<8) + (((tag)&255)); // 11111111
  b = (b<<8) + (((p1r)&255)); // 11111111

  c = 0;
  c = (c<<8) + (((tag)&255)); // 11111111
  c = c*11 + H9_hdr_c_p1[__min(10,__max(0,(p1)-(1-1)))];
  c = c*7 + H9_hdr_c_p2[__min(6,__max(0,(p2)-(1-1)))];

  d = 0;
  d = (d<<8) + (((tag)&255)); // 11111111
  d = d*8 + H9_hdr_d_fld[__min(7,__max(0,(fld)-(1-1)))];
  d = d*7 + H9_hdr_d_p1[__min(6,__max(0,(p1)-(1-1)))];
  d = d*61 + H9_hdr_d_dm[__min(60,__max(0,(dm)-(1-1)))];

  e = 0;
  e = (e<<8) + (((tag)&255)); // 11111111
  e = e*5 + H9_hdr_e_ws[__min(4,__max(0,(ws)-(1-1)))];
  e = e*8 + H9_hdr_e_fld[__min(7,__max(0,(fld)-(1-1)))];
  e = e*8 + H9_hdr_e_ml[__min(7,__max(0,(ml)-(1-1)))];

  h = 0;
  h = (h<<8) + (((tag)&255)); // 11111111
  h = h*3 + (((ch)>0+(1-1))+((ch)>1+(1-1)));
  h = h*16 + H9_hdr_h_fld[__min(15,__max(0,(fld)-(1-1)))];
  h = h*21 + H9_hdr_h_hx[__min(20,__max(0,(hx)-(1-1)))];

  i = 0;
  i = (i<<8) + (((tag)&255)); // 11111111
  i = i*9 + H9_hdr_i_pb[__min(8,__max(0,(pb)-(1-1)))];
  i = (i<<3) + (((p1r)&7)); // 00000111

  j = 0;
  j = (j<<8) + (((tag)&255)); // 11111111
  j = j*4 + H9_hdr_j_pos[__min(3,__max(0,(pos)-(1-1)))];
  j = j*5 + H9_hdr_j_p1[__min(4,__max(0,(p1)-(1-1)))];

  k = 0;
  k = (k<<8) + (((tag)&255)); // 11111111
  k = k*9 + H9_hdr_k_nb[__min(8,__max(0,(nb)-(1-1)))];
  k = (k<<3) + (((p1r)&7)); // 00000111

  l = 0;
  l = (l<<8) + (((tag)&255)); // 11111111
  l = (l<<4) + (((p1r)&15)); // 00001111
  l = l*5 + H9_hdr_l_fld[__min(4,__max(0,(fld)-(1-1)))];

  s = 0;
  s = (s<<8) + (((tag)&255)); // 11111111

  f = 0;
  f = (f<<8) + (((tag)&255)); // 11111111
  f = f*5 + H9_hdr_f_fld[__min(4,__max(0,(fld)-(1-1)))];

  m = 0;
  m = (m<<8) + (((tag)&255)); // 11111111

  w = 0;
  w = w*9 + H9_hdr_w_nb[__min(8,__max(0,(nb)-(1-1)))];

  t = 0;
  t = (t<<8) + (((tag)&255)); // 11111111

  g = 0;
  g = g*7 + H9_hdr_g_p1[__min(6,__max(0,(p1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_hdr
#define FC_PFX H9_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
