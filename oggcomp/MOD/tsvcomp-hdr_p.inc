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
  TC_hdr_a_tag.inc( a, tag );
  TC_hdr_a_p1.inc( a, p1 );
  TC_hdr_a_p2.inc( a, p2 );
  TC_hdr_a_pos.inc( a, pos );
  TC_hdr_a_p1r.inc( a, p1r );

  b = 0;
  TC_hdr_b_p1.inc( b, p1 );
  TC_hdr_b_pos.inc( b, pos );
  TC_hdr_b_p1r.inc( b, p1r );

  c = 0;
  TC_hdr_c_tag.inc( c, tag );

  d = 0;
  TC_hdr_d_tag.inc( d, tag );

  s = 0;
  TC_hdr_s_tag.inc( s, tag );

  f = 0;
  TC_hdr_f_tag.inc( f, tag );

  m = 0;
  TC_hdr_m_tag.inc( m, tag );

  t = 0;
  TC_hdr_t_tag.inc( t, tag );
  TC_hdr_t_p1r.inc( t, p1r );

  n = 0;
  TC_hdr_n_tag.inc( n, tag );
  TC_hdr_n_p1r.inc( n, p1r );

  g = 0;
  TC_hdr_g_p1.inc( g, p1 );
  TC_hdr_g_p2.inc( g, p2 );
  TC_hdr_g_pos.inc( g, pos );
  TC_hdr_g_p1r.inc( g, p1r );
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

