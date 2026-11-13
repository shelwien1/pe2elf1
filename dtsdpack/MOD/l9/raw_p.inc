/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_raw_ND (or M_raw_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_raw_make( const M_raw_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_raw_VARS(M_LOCAL)

  a = 0;
  a = a*4 + D9_raw_a_code[__min(3,__max(0,(code)-(1-1)))];

  b = 0;
  b = b*4 + D9_raw_b_code[__min(3,__max(0,(code)-(1-1)))];
  b = (b<<7) + (((((c1)>>7)&1)<<6) + ((c1)&63)); // 10111111

  c = 0;
  c = c*4 + D9_raw_c_code[__min(3,__max(0,(code)-(1-1)))];
  c = (c<<5) + (((((c1)>>5)&7)<<2) + ((c1)&3)); // 11100011
  c = (c<<4) + (((((c2)>>3)&7)<<1) + ((c2)&1)); // 00111001

  d = 0;
  d = d*4 + D9_raw_d_code[__min(3,__max(0,(code)-(1-1)))];
  d = d*10 + D9_raw_d_cl[__min(256,__max(0,(cl)-(1-1)))];

  e = 0;
  e = e*4 + D9_raw_e_code[__min(3,__max(0,(code)-(1-1)))];
  e = e*6 + D9_raw_e_at[__min(39,__max(0,(at)-(1-1)))];

  h = 0;
  h = h*4 + D9_raw_h_code[__min(3,__max(0,(code)-(1-1)))];
  h = h*7 + D9_raw_h_cl[__min(256,__max(0,(cl)-(1-1)))];
  h = (h<<5) + (((((c1)>>7)&1)<<4) + ((((c1)>>3)&7)<<1) + ((c1)&1)); // 10111001

  i = 0;
  i = i*4 + D9_raw_i_code[__min(3,__max(0,(code)-(1-1)))];
  i = (i<<5) + (((((c1)>>7)&1)<<4) + ((((c1)>>5)&1)<<3) + ((((c1)>>2)&3)<<1) + ((c1)&1)); // 10101101
  i = i*5 + D9_raw_i_at[__min(39,__max(0,(at)-(1-1)))];

  j = 0;
  j = j*4 + D9_raw_j_code[__min(3,__max(0,(code)-(1-1)))];
  j = j*10 + D9_raw_j_cl[__min(256,__max(0,(cl)-(1-1)))];
  j = j*5 + D9_raw_j_at[__min(39,__max(0,(at)-(1-1)))];

  k = 0;
  k = (k<<4) + (((((c1)>>6)&1)<<3) + ((((c1)>>3)&3)<<1) + ((c1)&1)); // 01011001
  k = (k<<2) + (((((c2)>>4)&1)<<1) + (((c2)>>2)&1)); // 00010100

  l = 0;
  l = l*4 + D9_raw_l_code[__min(3,__max(0,(code)-(1-1)))];
  l = (l<<5) + (((((c1)>>3)&15)<<1) + ((c1)&1)); // 01111001

  s = 0;
  s = s*4 + D9_raw_s_code[__min(3,__max(0,(code)-(1-1)))];
  s = s*8 + D9_raw_s_at[__min(39,__max(0,(at)-(1-1)))];

  f = 0;
  f = f*4 + D9_raw_f_code[__min(3,__max(0,(code)-(1-1)))];
  f = (f<<7) + (((((c1)>>4)&15)<<3) + ((c1)&7)); // 11110111

  m = 0;
  m = m*4 + D9_raw_m_code[__min(3,__max(0,(code)-(1-1)))];

  w = 0;
  w = w*7 + D9_raw_w_at[__min(39,__max(0,(at)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_raw
#define FC_PFX D9_raw_
#define FC_FAM raw
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
