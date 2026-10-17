/*  The template IDX/idx2inc.pl reads alongside hdr.idx: the hdr
    family's index builder and the tables its indices address.  Copied
    through into MOD/hdr_p.inc with `H_hdr_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/hdr_h.inc.  Edit this file and
    hdr.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int tag = D.tag, fld = D.fld, p1 = D.p1, p2 = D.p2, pos = D.pos, p1r = D.p1r, sty = D.sty;
  (void) tag;  (void) fld;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;  (void) sty;

  a = 0;
  a = (a<<8) + (((tag)&255)); // 11111111
  a = a*4 + H_hdr_a_fld[__min(3,__max(0,fld-(1-1)))];
  a = a*8 + H_hdr_a_p1[__min(7,__max(0,p1-(1-1)))];
  a = a*10 + H_hdr_a_sty[__min(9,__max(0,sty-(1-1)))];

  b = 0;
  b = (b<<8) + (((tag)&255)); // 11111111
  b = (b<<4) + (((p1r)&15)); // 00001111

  c = 0;
  c = (c<<8) + (((tag)&255)); // 11111111
  c = c*16 + H_hdr_c_fld[__min(15,__max(0,fld-(1-1)))];

  d = 0;
  d = (d<<8) + (((tag)&255)); // 11111111
  d = d*16 + H_hdr_d_p1[__min(15,__max(0,p1-(1-1)))];
  d = d*8 + H_hdr_d_p2[__min(7,__max(0,p2-(1-1)))];

  s = 0;
  s = (s<<8) + (((tag)&255)); // 11111111

  f = 0;
  f = (f<<8) + (((tag)&255)); // 11111111

  m = 0;
  m = (m<<8) + (((tag)&255)); // 11111111

  t = 0;
  t = (t<<8) + (((tag)&255)); // 11111111

  n = 0;
  n = (n<<8) + (((tag)&255)); // 11111111

  g = 0;
  g = g*8 + H_hdr_g_p1[__min(7,__max(0,p1-(1-1)))];
  g = g*4 + H_hdr_g_pos[__min(3,__max(0,pos-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

