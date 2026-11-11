/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_sid_ND (or M_sid_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_sid_make( const M_sid_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_sid_VARS(M_LOCAL)

  a = 0;
  a = a*2 + (((fld)>3+(1-1)));
  a = (a<<2) + (((lv)&3)); // 000011

  b = 0;
  b = b*2 + (((fld)>2+(1-1)));
  b = (b<<2) + (((((lv)>>2)&1)<<1) + ((lv)&1)); // 000101
  b = (b<<6) + (((pq)&63)); // 111111

  c = 0;
  c = c*3 + (((fld)>2+(1-1))+((fld)>5+(1-1)));
  c = c*22 + D9_sid_c_bd[__min(31,__max(0,(bd)-(1-1)))];
  c = (c<<2) + (((lv)&3)); // 000011

  d = 0;
  d = d*3 + (((fld)>2+(1-1))+((fld)>6+(1-1)));
  d = d*8 + D9_sid_d_ac[__min(8,__max(0,(ac)-(1-1)))];
  d = (d<<2) + (((lv)&3)); // 000011

  e = 0;
  e = e*2 + (((fld)>4+(1-1)));
  e = e*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  e = e*23 + D9_sid_e_bd[__min(31,__max(0,(bd)-(1-1)))];

  h = 0;
  h = h*3 + (((fld)>2+(1-1))+((fld)>3+(1-1)));
  h = h*12 + D9_sid_h_ab[__min(27,__max(0,(ab)-(1-1)))];
  h = (h<<3) + (((lv)&7)); // 000111

  i = 0;
  i = i*9 + D9_sid_i_fld[__min(8,__max(0,(fld)-(1-1)))];
  i = i*7 + D9_sid_i_ec[__min(9,__max(0,(ec)-(1-1)))];
  i = (i<<1) + (((lv)&1)); // 000001

  j = 0;
  j = j*3 + (((fld)>2+(1-1))+((fld)>4+(1-1)));
  j = j*2 + (((st)>0+(1-1)));

  k = 0;
  k = k*2 + (((fld)>6+(1-1)));
  k = k*10 + D9_sid_k_bd[__min(31,__max(0,(bd)-(1-1)))];
  k = k*5 + D9_sid_k_ac[__min(8,__max(0,(ac)-(1-1)))];

  l = 0;
  l = l*9 + D9_sid_l_fld[__min(8,__max(0,(fld)-(1-1)))];
  l = (l<<2) + (((lv)&3)); // 000011
  l = l*16 + D9_sid_l_ab[__min(27,__max(0,(ab)-(1-1)))];
  l = l*8 + D9_sid_l_ac[__min(8,__max(0,(ac)-(1-1)))];

  s = 0;
  s = s*9 + D9_sid_s_fld[__min(8,__max(0,(fld)-(1-1)))];
  s = (s<<1) + (((lv)&1)); // 000001

  f = 0;
  f = f*3 + (((fld)>3+(1-1))+((fld)>6+(1-1)));
  f = f*8 + D9_sid_f_ac[__min(8,__max(0,(ac)-(1-1)))];

  m = 0;
  m = m*4 + D9_sid_m_fld[__min(8,__max(0,(fld)-(1-1)))];

  w = 0;
  w = w*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  t = 0;
  t = t*9 + D9_sid_t_fld[__min(8,__max(0,(fld)-(1-1)))];

  n = 0;
  n = n*9 + D9_sid_n_fld[__min(8,__max(0,(fld)-(1-1)))];

  g = 0;
  g = g*9 + D9_sid_g_fld[__min(8,__max(0,(fld)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sid
#define FC_PFX D9_sid_
#define FC_FAM sid
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
