/*  The template idx2inc.pl reads alongside tsvcomp-cls.idx: the cls family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-cls_p.inc with `TC_cls_`
    replaced by the prefix -- `TC_cls_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_cls_T struct in MOD/tsvcomp-cls_h.inc.

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

void tc_make_cls( int rno, int band, int prev, int t1, int blk, int tn, int prev2,
                  int pq, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;  (void) tn;  (void) prev2;
  (void) pq;

  a = 0;
  TC_cls_a_rno.inc( a, rno );
  TC_cls_a_band.inc( a, band );
  TC_cls_a_prev.inc( a, prev );
  TC_cls_a_t1.inc( a, t1 );
  TC_cls_a_blk.inc( a, blk );
  TC_cls_a_tn.inc( a, tn );
  TC_cls_a_prev2.inc( a, prev2 );
  TC_cls_a_pq.inc( a, pq );

  b = 0;
  TC_cls_b_rno.inc( b, rno );
  TC_cls_b_band.inc( b, band );
  TC_cls_b_prev.inc( b, prev );
  TC_cls_b_t1.inc( b, t1 );
  TC_cls_b_blk.inc( b, blk );
  TC_cls_b_tn.inc( b, tn );
  TC_cls_b_prev2.inc( b, prev2 );
  TC_cls_b_pq.inc( b, pq );

  c = 0;
  TC_cls_c_band.inc( c, band );
  TC_cls_c_prev.inc( c, prev );
  TC_cls_c_t1.inc( c, t1 );
  TC_cls_c_tn.inc( c, tn );
  TC_cls_c_prev2.inc( c, prev2 );
  TC_cls_c_pq.inc( c, pq );

  d = 0;
  TC_cls_d_band.inc( d, band );
  TC_cls_d_prev.inc( d, prev );
  TC_cls_d_t1.inc( d, t1 );
  TC_cls_d_tn.inc( d, tn );
  TC_cls_d_prev2.inc( d, prev2 );
  TC_cls_d_pq.inc( d, pq );

  s = 0;
  TC_cls_s_band.inc( s, band );
  TC_cls_s_prev.inc( s, prev );
  TC_cls_s_t1.inc( s, t1 );
  TC_cls_s_tn.inc( s, tn );
  TC_cls_s_prev2.inc( s, prev2 );
  TC_cls_s_pq.inc( s, pq );

  f = 0;
  TC_cls_f_band.inc( f, band );
  TC_cls_f_prev.inc( f, prev );
  TC_cls_f_t1.inc( f, t1 );
  TC_cls_f_tn.inc( f, tn );
  TC_cls_f_prev2.inc( f, prev2 );
  TC_cls_f_pq.inc( f, pq );

  m = 0;
  TC_cls_m_band.inc( m, band );
  TC_cls_m_prev.inc( m, prev );
  TC_cls_m_t1.inc( m, t1 );
  TC_cls_m_tn.inc( m, tn );
  TC_cls_m_prev2.inc( m, prev2 );
  TC_cls_m_pq.inc( m, pq );

  t = 0;
  TC_cls_t_band.inc( t, band );
  TC_cls_t_prev.inc( t, prev );
  TC_cls_t_t1.inc( t, t1 );
  TC_cls_t_tn.inc( t, tn );
  TC_cls_t_prev2.inc( t, prev2 );
  TC_cls_t_pq.inc( t, pq );

  n = 0;
  TC_cls_n_band.inc( n, band );
  TC_cls_n_prev.inc( n, prev );
  TC_cls_n_t1.inc( n, t1 );
  TC_cls_n_tn.inc( n, tn );
  TC_cls_n_prev2.inc( n, prev2 );
  TC_cls_n_pq.inc( n, pq );

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

