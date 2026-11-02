/*  The template idx2inc.pl reads alongside tsvcomp-sgn.idx: the sgn family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-sgn_p.inc with `TC_sgn_`
    replaced by the prefix -- `TC_sgn_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_sgn_T struct in MOD/tsvcomp-sgn_h.inc.

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

void tc_make_sgn( const tc_dv & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax, mg = D.mg,
      pq = D.pq, sq = D.sq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm,
      x1 = D.x1, x1s = D.x1s;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) mg;    (void) pq;   (void) sq;
  (void) ps;   (void) chn;   (void) crun;  (void) cm;
  (void) x1;   (void) x1s;

  a = 0;
  TC_sgn_a_pass.inc( a, pass );
  TC_sgn_a_vpos.inc( a, vpos );
  TC_sgn_a_cls.inc( a, cls );
  TC_sgn_a_p0s.inc( a, p0s );
  TC_sgn_a_q1s.inc( a, q1s );
  TC_sgn_a_mg.inc( a, mg );
  TC_sgn_a_t1s.inc( a, t1s );
  TC_sgn_a_q2s.inc( a, q2s );
  TC_sgn_a_band.inc( a, band );
  TC_sgn_a_pq.inc( a, pq );
  TC_sgn_a_sq.inc( a, sq );
  TC_sgn_a_ps.inc( a, ps );
  TC_sgn_a_chn.inc( a, chn );
  TC_sgn_a_crun.inc( a, crun );
  TC_sgn_a_cm.inc( a, cm );
  TC_sgn_a_x1.inc( a, x1 );
  TC_sgn_a_x1s.inc( a, x1s );

  b = 0;
  TC_sgn_b_pass.inc( b, pass );
  TC_sgn_b_vpos.inc( b, vpos );
  TC_sgn_b_cls.inc( b, cls );
  TC_sgn_b_p0s.inc( b, p0s );
  TC_sgn_b_q1s.inc( b, q1s );
  TC_sgn_b_mg.inc( b, mg );
  TC_sgn_b_t1s.inc( b, t1s );
  TC_sgn_b_q2s.inc( b, q2s );
  TC_sgn_b_band.inc( b, band );
  TC_sgn_b_pq.inc( b, pq );
  TC_sgn_b_sq.inc( b, sq );
  TC_sgn_b_ps.inc( b, ps );
  TC_sgn_b_chn.inc( b, chn );
  TC_sgn_b_crun.inc( b, crun );
  TC_sgn_b_cm.inc( b, cm );
  TC_sgn_b_x1.inc( b, x1 );
  TC_sgn_b_x1s.inc( b, x1s );

  c = 0;
  TC_sgn_c_pass.inc( c, pass );
  TC_sgn_c_vpos.inc( c, vpos );
  TC_sgn_c_p0s.inc( c, p0s );
  TC_sgn_c_q1s.inc( c, q1s );
  TC_sgn_c_t1s.inc( c, t1s );
  TC_sgn_c_q2s.inc( c, q2s );
  TC_sgn_c_mg.inc( c, mg );
  TC_sgn_c_pq.inc( c, pq );
  TC_sgn_c_sq.inc( c, sq );
  TC_sgn_c_ps.inc( c, ps );
  TC_sgn_c_chn.inc( c, chn );
  TC_sgn_c_crun.inc( c, crun );
  TC_sgn_c_cm.inc( c, cm );
  TC_sgn_c_x1.inc( c, x1 );
  TC_sgn_c_x1s.inc( c, x1s );

  d = 0;
  TC_sgn_d_pass.inc( d, pass );
  TC_sgn_d_vpos.inc( d, vpos );
  TC_sgn_d_p0s.inc( d, p0s );
  TC_sgn_d_q1s.inc( d, q1s );
  TC_sgn_d_t1s.inc( d, t1s );
  TC_sgn_d_q2s.inc( d, q2s );
  TC_sgn_d_mg.inc( d, mg );
  TC_sgn_d_pq.inc( d, pq );
  TC_sgn_d_sq.inc( d, sq );
  TC_sgn_d_ps.inc( d, ps );
  TC_sgn_d_chn.inc( d, chn );
  TC_sgn_d_crun.inc( d, crun );
  TC_sgn_d_cm.inc( d, cm );
  TC_sgn_d_x1.inc( d, x1 );
  TC_sgn_d_x1s.inc( d, x1s );

  s = 0;
  TC_sgn_s_pass.inc( s, pass );
  TC_sgn_s_vpos.inc( s, vpos );
  TC_sgn_s_p0s.inc( s, p0s );
  TC_sgn_s_mg.inc( s, mg );
  TC_sgn_s_pq.inc( s, pq );
  TC_sgn_s_sq.inc( s, sq );
  TC_sgn_s_ps.inc( s, ps );
  TC_sgn_s_chn.inc( s, chn );
  TC_sgn_s_crun.inc( s, crun );
  TC_sgn_s_cm.inc( s, cm );
  TC_sgn_s_x1.inc( s, x1 );
  TC_sgn_s_x1s.inc( s, x1s );

  f = 0;
  TC_sgn_f_pass.inc( f, pass );
  TC_sgn_f_vpos.inc( f, vpos );
  TC_sgn_f_p0s.inc( f, p0s );
  TC_sgn_f_mg.inc( f, mg );
  TC_sgn_f_pq.inc( f, pq );
  TC_sgn_f_sq.inc( f, sq );
  TC_sgn_f_ps.inc( f, ps );
  TC_sgn_f_chn.inc( f, chn );
  TC_sgn_f_crun.inc( f, crun );
  TC_sgn_f_cm.inc( f, cm );
  TC_sgn_f_x1.inc( f, x1 );
  TC_sgn_f_x1s.inc( f, x1s );

  m = 0;
  TC_sgn_m_pass.inc( m, pass );
  TC_sgn_m_vpos.inc( m, vpos );
  TC_sgn_m_p0s.inc( m, p0s );
  TC_sgn_m_mg.inc( m, mg );
  TC_sgn_m_pq.inc( m, pq );
  TC_sgn_m_sq.inc( m, sq );
  TC_sgn_m_ps.inc( m, ps );
  TC_sgn_m_chn.inc( m, chn );
  TC_sgn_m_crun.inc( m, crun );
  TC_sgn_m_cm.inc( m, cm );
  TC_sgn_m_x1.inc( m, x1 );
  TC_sgn_m_x1s.inc( m, x1s );

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

