/*  The template idx2inc.pl reads alongside tsvcomp-aux.idx: the aux family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-aux_p.inc with `TC_aux_`
    replaced by the prefix -- `TC_aux_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_aux_T struct in MOD/tsvcomp-aux_h.inc.

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

void tc_make_aux( int fld, int p1, int p2, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) fld;  (void) p1;  (void) p2;

  a = 0;
  TC_aux_a_fld.inc( a, fld );
  TC_aux_a_p1.inc( a, p1 );
  TC_aux_a_p2.inc( a, p2 );

  b = 0;
  TC_aux_b_fld.inc( b, fld );
  TC_aux_b_p1.inc( b, p1 );
  TC_aux_b_p2.inc( b, p2 );

  c = 0;
  TC_aux_c_fld.inc( c, fld );

  d = 0;
  TC_aux_d_fld.inc( d, fld );

  s = 0;
  TC_aux_s_fld.inc( s, fld );

  f = 0;
  TC_aux_f_fld.inc( f, fld );

  m = 0;
  TC_aux_m_fld.inc( m, fld );

  t = 0;
  TC_aux_t_fld.inc( t, fld );

  n = 0;
  TC_aux_n_fld.inc( n, fld );

  g = 0;
  TC_aux_g_p1.inc( g, p1 );
  TC_aux_g_p2.inc( g, p2 );
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

