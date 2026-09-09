/*  The template idx2inc.pl reads alongside tsvcomp-hdr.idx: the hdr family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-hdr_p.inc with `TC_hdr_`
    replaced by the prefix -- `TC_hdr_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_hdr_T struct in MOD/tsvcomp-hdr_h.inc.

    Edit this file, not MOD/.  ../mk.sh regenerates both.

    One function, building the family's ten indices together: the four stages
    of a decision (two counters, the APM, the mixer) each want a different
    grouping of the same variables, and the mantissa plane a fifth, which is
    what mp3c's separate index declarations per submodel are for.  The `(void)`
    lines are there because a variable at pattern zero generates no code, and a
    family lists every variable it has so the optimizer has somewhere to go.

    `_A` is the rich counter, `_B` the coarse one, `_S` the APM's buckets per
    context, `_W` the mixer's weights, `_T` the mantissa plane, `_G` the sign
    counters where a family codes signs under a context of their own.  Every
    one of them is TC_NODE (or TC_MNODE) rows deep per context, so where a bit
    sits in the cascade is context without having to be declared a variable.  */

void tc_make_hdr( int tag, int p1, int p2, int pos, int p1r, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) tag;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;

  a = 0;
  a = (a<<6) + (((tag)&63)); // 111111
  a = a*5 + ((p1>0+(1-1))+(p1>10+(1-1))+(p1>12+(1-1))+(p1>13+(1-1)));
  a = a*8 + ((p2>0+(1-1))+(p2>1+(1-1))+(p2>2+(1-1))+(p2>3+(1-1))+(p2>4+(1-1))+(p2>5+(1-1))+(p2>6+(1-1)));
  a = a*3 + ((pos>1+(1-1))+(pos>2+(1-1)));

  b = 0;
  b = b*3 + ((p1>0+(1-1))+(p1>2+(1-1)));
  b = (b<<5) + (((((p1r)>>6)&1)<<4) + (((p1r)>>1)&15)); // 01011110

  c = 0;
  c = (c<<6) + (((tag)&63)); // 111111

  d = 0;
  d = (d<<6) + (((tag)&63)); // 111111

  s = 0;
  s = (s<<6) + (((tag)&63)); // 111111

  f = 0;
  f = (f<<6) + (((tag)&63)); // 111111

  m = 0;
  m = (m<<6) + (((tag)&63)); // 111111

  t = 0;
  t = (t<<6) + (((tag)&63)); // 111111
  t = (t<<6) + (((((p1r)>>5)&3)<<4) + ((p1r)&15)); // 01101111

  n = 0;
  n = (n<<6) + (((tag)&63)); // 111111
  n = (n<<6) + (((((p1r)>>5)&3)<<4) + ((p1r)&15)); // 01101111

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

