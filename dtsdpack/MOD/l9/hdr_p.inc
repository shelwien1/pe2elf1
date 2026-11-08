/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_hdr_ND (or M_hdr_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void D9_hdr_make( const M_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_hdr_VARS(M_LOCAL)

  a = 0;
  a = (a<<6) + (((fld)&63)); // 111111

  b = 0;
  b = (b<<6) + (((fld)&63)); // 111111
  b = (b<<6) + (((lv)&63)); // 111111

  c = 0;
  c = (c<<6) + (((fld)&63)); // 111111
  c = (c<<6) + (((lv)&63)); // 111111
  c = c*3 + (((l2)>0+(1-1))+((l2)>1+(1-1)));

  d = 0;
  d = (d<<6) + (((fld)&63)); // 111111
  d = d*32 + D9_hdr_d_pos[__min(31,__max(0,(pos)-(1-1)))];

  e = 0;
  e = (e<<6) + (((fld)&63)); // 111111
  e = e*3 + (((st)>0+(1-1))+((st)>1+(1-1)));
  e = e*4 + D9_hdr_e_nch[__min(3,__max(0,(nch)-(1-1)))];

  h = 0;
  h = (h<<6) + (((fld)&63)); // 111111
  h = (h<<6) + (((lv)&63)); // 111111
  h = h*32 + D9_hdr_h_pos[__min(31,__max(0,(pos)-(1-1)))];

  i = 0;
  i = (i<<6) + (((fld)&63)); // 111111
  i = i*3 + (((l2)>0+(1-1))+((l2)>1+(1-1)));
  i = i*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  j = 0;
  j = (j<<6) + (((fld)&63)); // 111111
  j = (j<<6) + (((lv)&63)); // 111111
  j = j*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  k = 0;
  k = (k<<6) + (((fld)&63)); // 111111
  k = k*32 + D9_hdr_k_pos[__min(31,__max(0,(pos)-(1-1)))];
  k = k*3 + (((l2)>0+(1-1))+((l2)>1+(1-1)));

  l = 0;
  l = (l<<6) + (((fld)&63)); // 111111
  l = l*4 + D9_hdr_l_nch[__min(3,__max(0,(nch)-(1-1)))];

  s = 0;
  s = (s<<6) + (((fld)&63)); // 111111
  s = s*3 + (((l2)>0+(1-1))+((l2)>1+(1-1)));

  f = 0;
  f = (f<<6) + (((fld)&63)); // 111111

  m = 0;
  m = (m<<6) + (((fld)&63)); // 111111

  w = 0;
  w = w*3 + (((l2)>0+(1-1))+((l2)>1+(1-1)));
  w = w*3 + (((st)>0+(1-1))+((st)>1+(1-1)));

  t = 0;
  t = (t<<6) + (((fld)&63)); // 111111

  n = 0;
  n = (n<<6) + (((fld)&63)); // 111111

  g = 0;
  g = (g<<6) + (((fld)&63)); // 111111
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_hdr
#define FC_PFX D9_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
