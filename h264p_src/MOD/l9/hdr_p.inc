/*  The template IDX/idx2inc.pl reads alongside hdr.idx: the hdr
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/hdr_p.inc with `H9_hdr_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/hdr_h.inc.  Edit this file
    and hdr.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int tag = D.tag, fld = D.fld, p1 = D.p1, p2 = D.p2, pos = D.pos, p1r = D.p1r, sty = D.sty, pr = D.pr;
  (void) tag;  (void) fld;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;  (void) sty;  (void) pr;

  a = 0;
  a = (a<<5) + (((tag)&31)); // 00011111
  a = a*2 + ((fld>0+(1-1)));
  a = a*5 + H9_hdr_a_p1[__min(7,__max(0,p1-(1-1)))];
  a = a*4 + H9_hdr_a_sty[__min(9,__max(0,sty-(1-1)))];
  a = a*2 + ((pr>0+(1-1)));

  b = 0;
  b = (b<<5) + (((((tag)>>2)&15)<<1) + ((tag)&1)); // 00111101
  b = (b<<8) + (((p1r)&255)); // 11111111

  c = 0;
  c = (c<<6) + (((((tag)>>4)&7)<<3) + ((tag)&7)); // 01110111
  c = c*7 + H9_hdr_c_fld[__min(15,__max(0,fld-(1-1)))];

  d = 0;
  d = (d<<1) + ((((tag)>>5)&1)); // 00100000
  d = d*11 + H9_hdr_d_p1[__min(15,__max(0,p1-(1-1)))];
  d = d*8 + H9_hdr_d_p2[__min(7,__max(0,p2-(1-1)))];

  e = 0;
  e = (e<<1) + ((((tag)>>6)&1)); // 01000000
  e = e*2 + ((pos>1+(1-1)));

  h = 0;
  h = (h<<5) + (((((tag)>>4)&3)<<3) + ((tag)&7)); // 00110111
  h = h*4 + H9_hdr_h_sty[__min(9,__max(0,sty-(1-1)))];

  i = 0;
  i = (i<<5) + (((((tag)>>6)&1)<<4) + (((tag)>>1)&15)); // 01011110

  j = 0;
  j = j*2 + ((fld>0+(1-1)));
  j = j*4 + H9_hdr_j_p2[__min(7,__max(0,p2-(1-1)))];

  k = 0;
  k = (k<<7) + (((tag)&127)); // 01111111
  k = k*15 + H9_hdr_k_fld[__min(15,__max(0,fld-(1-1)))];

  l = 0;
  l = (l<<4) + (((((tag)>>6)&1)<<3) + ((((tag)>>4)&1)<<2) + ((((tag)>>2)&1)<<1) + ((tag)&1)); // 01010101
  l = l*3 + ((pos>0+(1-1))+(pos>2+(1-1)));

  s = 0;
  s = (s<<6) + (((((tag)>>3)&15)<<2) + ((tag)&3)); // 01111011

  f = 0;
  f = (f<<7) + (((tag)&127)); // 01111111

  m = 0;

  w = 0;
  w = (w<<6) + (((tag)&63)); // 00111111

  t = 0;
  t = (t<<4) + ((((tag)>>1)&15)); // 00011110

  n = 0;

  g = 0;
  g = g*7 + H9_hdr_g_p1[__min(7,__max(0,p1-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

