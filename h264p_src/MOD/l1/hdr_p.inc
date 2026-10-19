/*  The template IDX/idx2inc.pl reads alongside hdr.idx: the hdr
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/hdr_p.inc with `H1_hdr_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/hdr_h.inc.  Edit this file
    and hdr.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int tag = D.tag, fld = D.fld, p1 = D.p1, p2 = D.p2, pos = D.pos, p1r = D.p1r, sty = D.sty, pr = D.pr;
  (void) tag;  (void) fld;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;  (void) sty;  (void) pr;

  a = 0;
  a = (a<<6) + (((((tag)>>6)&1)<<5) + ((tag)&31)); // 01011111
  a = a*4 + H1_hdr_a_fld[__min(3,__max(0,fld-(1-1)))];
  a = a*8 + H1_hdr_a_p1[__min(7,__max(0,p1-(1-1)))];
  a = a*10 + H1_hdr_a_sty[__min(9,__max(0,sty-(1-1)))];
  a = a*4 + H1_hdr_a_pr[__min(3,__max(0,pr-(1-1)))];

  b = 0;
  b = (b<<5) + (((((tag)>>5)&1)<<4) + ((tag)&15)); // 00101111
  b = (b<<4) + (((p1r)&15)); // 00001111

  c = 0;
  c = (c<<8) + (((tag)&255)); // 11111111
  c = c*7 + H1_hdr_c_fld[__min(15,__max(0,fld-(1-1)))];

  d = 0;
  d = (d<<8) + (((tag)&255)); // 11111111
  d = d*9 + H1_hdr_d_p1[__min(15,__max(0,p1-(1-1)))];
  d = d*8 + H1_hdr_d_p2[__min(7,__max(0,p2-(1-1)))];

  m = 0;

  t = 0;
  t = (t<<8) + (((tag)&255)); // 11111111

  n = 0;
  n = (n<<8) + (((tag)&255)); // 11111111

  g = 0;
  g = g*8 + H1_hdr_g_p1[__min(7,__max(0,p1-(1-1)))];
  g = g*4 + H1_hdr_g_pos[__min(3,__max(0,pos-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

