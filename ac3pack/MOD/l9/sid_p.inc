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

void A9_sid_make( const M_sid_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_sid_VARS(M_LOCAL)

  a = 0;

  b = 0;
  b = (b<<5) + (((((fld)>>5)&1)<<4) + ((fld)&15)); // 101111
  b = (b<<6) + (((lv)&63)); // 111111

  c = 0;
  c = (c<<6) + (((fld)&63)); // 111111
  c = (c<<5) + (((lv)&31)); // 011111
  c = c*14 + A9_sid_c_pos[__min(15,__max(0,(pos)-(1-1)))];

  d = 0;
  d = (d<<6) + (((fld)&63)); // 111111
  d = (d<<6) + (((pq)&63)); // 111111
  d = d*2 + (((lb)>1+(1-1)));

  e = 0;
  e = (e<<6) + (((fld)&63)); // 111111
  e = e*3 + (((st)>0+(1-1))+((st)>1+(1-1)));
  e = e*3 + (((nch)>0+(1-1))+((nch)>2+(1-1)));

  h = 0;
  h = (h<<5) + (((((fld)>>3)&7)<<2) + ((fld)&3)); // 111011
  h = (h<<5) + (((lv)&31)); // 011111
  h = h*3 + (((pr)>0+(1-1))+((pr)>1+(1-1)));
  h = h*3 + (((lb)>0+(1-1))+((lb)>1+(1-1)));

  i = 0;
  i = (i<<6) + (((fld)&63)); // 111111
  i = i*2 + (((blk)>0+(1-1)));
  i = (i<<4) + ((((lv)>>1)&15)); // 011110

  j = 0;
  j = (j<<5) + (((((fld)>>2)&15)<<1) + ((fld)&1)); // 111101
  j = j*5 + A9_sid_j_sk[__min(4,__max(0,(sk)-(1-1)))];
  j = (j<<1) + ((((lv)>>4)&1)); // 010000

  k = 0;
  k = (k<<6) + (((fld)&63)); // 111111
  k = k*18 + A9_sid_k_pos[__min(31,__max(0,(pos)-(1-1)))];
  k = (k<<5) + (((pq)&31)); // 011111

  l = 0;
  l = (l<<6) + (((fld)&63)); // 111111
  l = l*2 + (((st)>1+(1-1)));
  l = l*3 + (((pr)>0+(1-1))+((pr)>1+(1-1)));
  l = l*4 + A9_sid_l_blk[__min(5,__max(0,(blk)-(1-1)))];

  s = 0;
  s = (s<<6) + (((fld)&63)); // 111111
  s = s*3 + (((lb)>0+(1-1))+((lb)>1+(1-1)));

  f = 0;
  f = (f<<5) + (((((fld)>>3)&7)<<2) + ((fld)&3)); // 111011

  m = 0;
  m = (m<<5) + (((((fld)>>4)&3)<<3) + ((fld)&7)); // 110111

  w = 0;
  w = w*3 + (((st)>0+(1-1))+((st)>1+(1-1)));
  w = w*2 + (((pr)>1+(1-1)));

  t = 0;
  t = (t<<5) + (((((fld)>>3)&7)<<2) + ((fld)&3)); // 111011

  n = 0;
  n = (n<<6) + (((fld)&63)); // 111111

  g = 0;
  g = (g<<6) + (((fld)&63)); // 111111
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_sid
#define FC_PFX A9_sid_
#define FC_FAM sid
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
