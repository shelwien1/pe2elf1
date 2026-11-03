/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_sig_ND (or M_sig_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_sig_make( const M_sig_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_sig_VARS(M_LOCAL)

  a = 0;
  a = a*10 + M9_sig_a_tx[__min(24,__max(0,(tx)-(1-1)))];
  a = a*2 + (((em)>0+(1-1)));
  a = a*37 + M9_sig_a_pb[__min(64,__max(0,(pos)-(1-1)))];
  a = a*8 + M9_sig_a_pf[__min(30,__max(0,(pf)-(1-1)))];
  a = a*3 + (((td)>0+(1-1))+((td)>1+(1-1)));

  b = 0;
  b = (b<<2) + (((((cls)>>2)&1)<<1) + ((cls)&1)); // 101
  b = (b<<3) + (((((pos)>>4)&3)<<1) + (((pos)>>2)&1)); // 110100

  c = 0;
  c = (c<<1) + ((((cls)>>1)&1)); // 010
  c = c*8 + M9_sig_c_nz[__min(8,__max(0,(nz)-(1-1)))];

  d = 0;
  d = (d<<2) + ((((cls)>>1)&3)); // 110
  d = d*6 + M9_sig_d_past[__min(8,__max(0,(past)-(1-1)))];
  d = d*6 + M9_sig_d_qs[__min(32,__max(0,(qs)-(1-1)))];
  d = (d<<6) + (((pos)&63)); // 111111
  d = d*4 + M9_sig_d_ie[__min(18,__max(0,(ie)-(1-1)))];
  d = d*9 + M9_sig_d_pe[__min(18,__max(0,(pe)-(1-1)))];
  d = d*13 + M9_sig_d_tq[__min(64,__max(0,(tq)-(1-1)))];

  e = 0;
  e = (e<<1) + ((((cls)>>1)&1)); // 010
  e = e*14 + M9_sig_e_run[__min(16,__max(0,(run)-(1-1)))];
  e = (e<<5) + ((((pos)>>1)&31)); // 111110

  h = 0;
  h = (h<<5) + (((fp)&31)); // 11111
  h = (h<<6) + (((pos)&63)); // 111111

  i = 0;
  i = (i<<2) + (((cls)&3)); // 011
  i = i*15 + M9_sig_i_mvc[__min(30,__max(0,(mvc)-(1-1)))];
  i = (i<<6) + (((pos)&63)); // 111111

  j = 0;
  j = (j<<1) + (((cls)&1)); // 001
  j = j*8 + M9_sig_j_rnz[__min(17,__max(0,(rnz)-(1-1)))];
  j = j*5 + M9_sig_j_xm[__min(9,__max(0,(xm)-(1-1)))];

  k = 0;
  k = (k<<2) + ((((cls)>>1)&3)); // 110
  k = (k<<7) + (((((h8)>>7)&1)<<6) + ((h8)&63)); // 10111111
  k = (k<<6) + (((pos)&63)); // 111111

  l = 0;
  l = (l<<3) + (((cls)&7)); // 111
  l = (l<<6) + (((pos)&63)); // 111111
  l = (l<<15) + (((((h16)>>8)&255)<<7) + ((h16)&127)); // 1111111101111111

  s = 0;
  s = (s<<2) + (((((cls)>>2)&1)<<1) + ((cls)&1)); // 101
  s = s*5 + M9_sig_s_sl[__min(17,__max(0,(sl)-(1-1)))];
  s = s*5 + M9_sig_s_sa[__min(17,__max(0,(sa)-(1-1)))];
  s = s*4 + M9_sig_s_pb[__min(64,__max(0,(pos)-(1-1)))];

  f = 0;
  f = (f<<3) + (((cls)&7)); // 111
  f = (f<<6) + (((pos)&63)); // 111111

  m = 0;
  m = (m<<3) + (((cls)&7)); // 111
  m = (m<<5) + ((((pos)>>1)&31)); // 111110
  m = m*3 + (((ie)>4+(1-1))+((ie)>13+(1-1)));

  w = 0;
  w = (w<<1) + ((((cls)>>2)&1)); // 100
  w = w*4 + M9_sig_w_kb[__min(16,__max(0,(kb)-(1-1)))];
  w = w*9 + M9_sig_w_qs[__min(32,__max(0,(qs)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sig
#define FC_PFX M9_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
