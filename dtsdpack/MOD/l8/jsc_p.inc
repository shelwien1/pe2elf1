/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l8: a family's index builder,
    the tables its indices address and its configuration class, at level -8.  Copied through
    into MOD/l8/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l8/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_jsc_ND (or M_jsc_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D8_jsc_make( const M_jsc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_jsc_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((fld)>3+(1-1)));
  a = (a<<1) + (((lv)&1)); // 000001

  b = 0;
  b = b*2 + (((fld)>2+(1-1)));
  b = (b<<1) + ((((lv)>>3)&1)); // 001000
  b = (b<<6) + (((pq)&63)); // 111111

  c = 0;
  c = c*2 + (((fld)>3+(1-1)));

  d = 0;
  d = d*2 + (((fld)>3+(1-1)));
  d = d*4 + D8_jsc_d_ac[__min(8,__max(0,(ac)-(1-1)))];
  d = (d<<3) + (((lv)&7)); // 000111

  e = 0;
  e = e*2 + (((fld)>3+(1-1)));
  e = e*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));

  h = 0;
  h = h*2 + (((fld)>3+(1-1)));
  h = h*7 + D8_jsc_h_ab[__min(27,__max(0,(ab)-(1-1)))];
  h = (h<<1) + ((((lv)>>2)&1)); // 000100
  h = h*4 + D8_jsc_h_tr[__min(5,__max(0,(tr)-(1-1)))];

  i = 0;
  i = i*2 + (((fld)>7+(1-1)));
  i = i*7 + D8_jsc_i_ec[__min(9,__max(0,(ec)-(1-1)))];
  i = (i<<1) + ((((lv)>>1)&1)); // 000010
  i = i*4 + D8_jsc_i_tr[__min(5,__max(0,(tr)-(1-1)))];

  j = 0;
  j = j*3 + (((fld)>3+(1-1))+((fld)>4+(1-1)));
  j = (j<<1) + (((pq)&1)); // 000001
  j = j*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  k = 0;
  k = k*3 + (((fld)>2+(1-1))+((fld)>6+(1-1)));
  k = k*3 + (((bd)>18+(1-1))+((bd)>24+(1-1)));
  k = k*4 + D8_jsc_k_ac[__min(8,__max(0,(ac)-(1-1)))];

  l = 0;
  l = l*2 + (((fld)>3+(1-1)));
  l = (l<<1) + ((((lv)>>3)&1)); // 001000
  l = l*6 + D8_jsc_l_ab[__min(27,__max(0,(ab)-(1-1)))];
  l = l*8 + D8_jsc_l_ac[__min(8,__max(0,(ac)-(1-1)))];

  s = 0;
  s = s*3 + (((fld)>2+(1-1))+((fld)>3+(1-1)));
  s = (s<<3) + (((lv)&7)); // 000111

  f = 0;
  f = f*3 + (((fld)>3+(1-1))+((fld)>6+(1-1)));
  f = f*8 + D8_jsc_f_ac[__min(8,__max(0,(ac)-(1-1)))];

  m = 0;
  m = m*4 + D8_jsc_m_fld[__min(8,__max(0,(fld)-(1-1)))];

  w = 0;
  w = w*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  t = 0;

  n = 0;
  n = n*9 + D8_jsc_n_fld[__min(8,__max(0,(fld)-(1-1)))];

  g = 0;
  g = g*9 + D8_jsc_g_fld[__min(8,__max(0,(fld)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C8_jsc
#define FC_PFX D8_jsc_
#define FC_FAM jsc
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
