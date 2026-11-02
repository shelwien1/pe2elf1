/*  The template idx2inc.pl reads alongside tsvcomp-dig.idx: the dig family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-dig_p.inc with `TC_dig_`
    replaced by the prefix -- `TC_dig_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_dig_T struct in MOD/tsvcomp-dig_h.inc.

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

void tc_make_dig( const tc_dv & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax,
      pq = D.pq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm,
      x1 = D.x1, x1s = D.x1s, pmx = D.pmx, psm = D.psm, prm = D.prm;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;   (void) ps;   (void) chn;
  (void) crun; (void) cm;   (void) x1;    (void) x1s;
  (void) pmx;  (void) psm;  (void) prm;

  a = 0;
  TC_dig_a_rno.inc( a, rno );
  TC_dig_a_pass.inc( a, pass );
  TC_dig_a_band.inc( a, band );
  TC_dig_a_col.inc( a, col );
  TC_dig_a_q1.inc( a, q1 );
  TC_dig_a_q2.inc( a, q2 );
  TC_dig_a_t1.inc( a, t1 );
  TC_dig_a_bkq.inc( a, bkq );
  TC_dig_a_cls.inc( a, cls );
  TC_dig_a_zrun.inc( a, zrun );
  TC_dig_a_blk.inc( a, blk );
  TC_dig_a_vpos.inc( a, vpos );
  TC_dig_a_q1s.inc( a, q1s );
  TC_dig_a_q2s.inc( a, q2s );
  TC_dig_a_t1s.inc( a, t1s );
  TC_dig_a_t2.inc( a, t2 );
  TC_dig_a_n1.inc( a, n1 );
  TC_dig_a_w1.inc( a, w1 );
  TC_dig_a_p0.inc( a, p0 );
  TC_dig_a_p0s.inc( a, p0s );
  TC_dig_a_pn.inc( a, pn );
  TC_dig_a_ax.inc( a, ax );
  TC_dig_a_pq.inc( a, pq );
  TC_dig_a_ps.inc( a, ps );
  TC_dig_a_chn.inc( a, chn );
  TC_dig_a_crun.inc( a, crun );
  TC_dig_a_cm.inc( a, cm );
  TC_dig_a_x1.inc( a, x1 );
  TC_dig_a_x1s.inc( a, x1s );
  TC_dig_a_pmx.inc( a, pmx );
  TC_dig_a_psm.inc( a, psm );
  TC_dig_a_prm.inc( a, prm );

  b = 0;
  TC_dig_b_rno.inc( b, rno );
  TC_dig_b_pass.inc( b, pass );
  TC_dig_b_band.inc( b, band );
  TC_dig_b_col.inc( b, col );
  TC_dig_b_q1.inc( b, q1 );
  TC_dig_b_q2.inc( b, q2 );
  TC_dig_b_t1.inc( b, t1 );
  TC_dig_b_bkq.inc( b, bkq );
  TC_dig_b_cls.inc( b, cls );
  TC_dig_b_zrun.inc( b, zrun );
  TC_dig_b_blk.inc( b, blk );
  TC_dig_b_vpos.inc( b, vpos );
  TC_dig_b_q1s.inc( b, q1s );
  TC_dig_b_q2s.inc( b, q2s );
  TC_dig_b_t1s.inc( b, t1s );
  TC_dig_b_t2.inc( b, t2 );
  TC_dig_b_n1.inc( b, n1 );
  TC_dig_b_w1.inc( b, w1 );
  TC_dig_b_p0.inc( b, p0 );
  TC_dig_b_p0s.inc( b, p0s );
  TC_dig_b_pn.inc( b, pn );
  TC_dig_b_ax.inc( b, ax );
  TC_dig_b_pq.inc( b, pq );
  TC_dig_b_ps.inc( b, ps );
  TC_dig_b_chn.inc( b, chn );
  TC_dig_b_crun.inc( b, crun );
  TC_dig_b_cm.inc( b, cm );
  TC_dig_b_x1.inc( b, x1 );
  TC_dig_b_x1s.inc( b, x1s );
  TC_dig_b_pmx.inc( b, pmx );
  TC_dig_b_psm.inc( b, psm );
  TC_dig_b_prm.inc( b, prm );

  c = 0;
  TC_dig_c_pass.inc( c, pass );
  TC_dig_c_vpos.inc( c, vpos );
  TC_dig_c_band.inc( c, band );
  TC_dig_c_t1s.inc( c, t1s );
  TC_dig_c_t2.inc( c, t2 );
  TC_dig_c_n1.inc( c, n1 );
  TC_dig_c_w1.inc( c, w1 );
  TC_dig_c_ax.inc( c, ax );
  TC_dig_c_q1s.inc( c, q1s );
  TC_dig_c_q2s.inc( c, q2s );
  TC_dig_c_cls.inc( c, cls );
  TC_dig_c_p0s.inc( c, p0s );
  TC_dig_c_pq.inc( c, pq );
  TC_dig_c_ps.inc( c, ps );
  TC_dig_c_chn.inc( c, chn );
  TC_dig_c_crun.inc( c, crun );
  TC_dig_c_cm.inc( c, cm );
  TC_dig_c_x1.inc( c, x1 );
  TC_dig_c_x1s.inc( c, x1s );

  d = 0;
  TC_dig_d_pass.inc( d, pass );
  TC_dig_d_vpos.inc( d, vpos );
  TC_dig_d_band.inc( d, band );
  TC_dig_d_t1s.inc( d, t1s );
  TC_dig_d_t2.inc( d, t2 );
  TC_dig_d_n1.inc( d, n1 );
  TC_dig_d_w1.inc( d, w1 );
  TC_dig_d_ax.inc( d, ax );
  TC_dig_d_q1s.inc( d, q1s );
  TC_dig_d_q2s.inc( d, q2s );
  TC_dig_d_cls.inc( d, cls );
  TC_dig_d_p0s.inc( d, p0s );
  TC_dig_d_pq.inc( d, pq );
  TC_dig_d_ps.inc( d, ps );
  TC_dig_d_chn.inc( d, chn );
  TC_dig_d_crun.inc( d, crun );
  TC_dig_d_cm.inc( d, cm );
  TC_dig_d_x1.inc( d, x1 );
  TC_dig_d_x1s.inc( d, x1s );

  s = 0;
  TC_dig_s_pass.inc( s, pass );
  TC_dig_s_band.inc( s, band );
  TC_dig_s_q1.inc( s, q1 );
  TC_dig_s_t1.inc( s, t1 );
  TC_dig_s_bkq.inc( s, bkq );
  TC_dig_s_cls.inc( s, cls );
  TC_dig_s_vpos.inc( s, vpos );
  TC_dig_s_q1s.inc( s, q1s );
  TC_dig_s_q2s.inc( s, q2s );
  TC_dig_s_t1s.inc( s, t1s );
  TC_dig_s_t2.inc( s, t2 );
  TC_dig_s_n1.inc( s, n1 );
  TC_dig_s_w1.inc( s, w1 );
  TC_dig_s_p0.inc( s, p0 );
  TC_dig_s_p0s.inc( s, p0s );
  TC_dig_s_pn.inc( s, pn );
  TC_dig_s_ax.inc( s, ax );
  TC_dig_s_pq.inc( s, pq );
  TC_dig_s_ps.inc( s, ps );
  TC_dig_s_chn.inc( s, chn );
  TC_dig_s_crun.inc( s, crun );
  TC_dig_s_cm.inc( s, cm );
  TC_dig_s_x1.inc( s, x1 );
  TC_dig_s_x1s.inc( s, x1s );

  f = 0;
  TC_dig_f_pass.inc( f, pass );
  TC_dig_f_vpos.inc( f, vpos );
  TC_dig_f_cls.inc( f, cls );
  TC_dig_f_q1s.inc( f, q1s );
  TC_dig_f_p0s.inc( f, p0s );
  TC_dig_f_t1s.inc( f, t1s );
  TC_dig_f_ax.inc( f, ax );
  TC_dig_f_band.inc( f, band );
  TC_dig_f_pq.inc( f, pq );
  TC_dig_f_ps.inc( f, ps );
  TC_dig_f_chn.inc( f, chn );
  TC_dig_f_crun.inc( f, crun );
  TC_dig_f_cm.inc( f, cm );
  TC_dig_f_x1.inc( f, x1 );
  TC_dig_f_x1s.inc( f, x1s );

  m = 0;
  TC_dig_m_pass.inc( m, pass );
  TC_dig_m_band.inc( m, band );
  TC_dig_m_q1.inc( m, q1 );
  TC_dig_m_t1.inc( m, t1 );
  TC_dig_m_bkq.inc( m, bkq );
  TC_dig_m_cls.inc( m, cls );
  TC_dig_m_vpos.inc( m, vpos );
  TC_dig_m_q1s.inc( m, q1s );
  TC_dig_m_q2s.inc( m, q2s );
  TC_dig_m_t1s.inc( m, t1s );
  TC_dig_m_t2.inc( m, t2 );
  TC_dig_m_n1.inc( m, n1 );
  TC_dig_m_w1.inc( m, w1 );
  TC_dig_m_p0.inc( m, p0 );
  TC_dig_m_p0s.inc( m, p0s );
  TC_dig_m_pn.inc( m, pn );
  TC_dig_m_ax.inc( m, ax );
  TC_dig_m_pq.inc( m, pq );
  TC_dig_m_ps.inc( m, ps );
  TC_dig_m_chn.inc( m, chn );
  TC_dig_m_crun.inc( m, crun );
  TC_dig_m_cm.inc( m, cm );
  TC_dig_m_x1.inc( m, x1 );
  TC_dig_m_x1s.inc( m, x1s );

  t = 0;
  TC_dig_t_pass.inc( t, pass );
  TC_dig_t_band.inc( t, band );
  TC_dig_t_q1.inc( t, q1 );
  TC_dig_t_t1.inc( t, t1 );
  TC_dig_t_bkq.inc( t, bkq );
  TC_dig_t_cls.inc( t, cls );
  TC_dig_t_vpos.inc( t, vpos );
  TC_dig_t_q1s.inc( t, q1s );
  TC_dig_t_q2s.inc( t, q2s );
  TC_dig_t_t1s.inc( t, t1s );
  TC_dig_t_t2.inc( t, t2 );
  TC_dig_t_n1.inc( t, n1 );
  TC_dig_t_w1.inc( t, w1 );
  TC_dig_t_p0.inc( t, p0 );
  TC_dig_t_p0s.inc( t, p0s );
  TC_dig_t_pn.inc( t, pn );
  TC_dig_t_ax.inc( t, ax );
  TC_dig_t_pq.inc( t, pq );
  TC_dig_t_ps.inc( t, ps );
  TC_dig_t_chn.inc( t, chn );
  TC_dig_t_crun.inc( t, crun );
  TC_dig_t_cm.inc( t, cm );
  TC_dig_t_x1.inc( t, x1 );
  TC_dig_t_x1s.inc( t, x1s );

  n = 0;
  TC_dig_n_pass.inc( n, pass );
  TC_dig_n_band.inc( n, band );
  TC_dig_n_q1.inc( n, q1 );
  TC_dig_n_t1.inc( n, t1 );
  TC_dig_n_bkq.inc( n, bkq );
  TC_dig_n_cls.inc( n, cls );
  TC_dig_n_vpos.inc( n, vpos );
  TC_dig_n_q1s.inc( n, q1s );
  TC_dig_n_q2s.inc( n, q2s );
  TC_dig_n_t1s.inc( n, t1s );
  TC_dig_n_t2.inc( n, t2 );
  TC_dig_n_n1.inc( n, n1 );
  TC_dig_n_w1.inc( n, w1 );
  TC_dig_n_p0.inc( n, p0 );
  TC_dig_n_p0s.inc( n, p0s );
  TC_dig_n_pn.inc( n, pn );
  TC_dig_n_ax.inc( n, ax );
  TC_dig_n_pq.inc( n, pq );
  TC_dig_n_ps.inc( n, ps );
  TC_dig_n_chn.inc( n, chn );
  TC_dig_n_crun.inc( n, crun );
  TC_dig_n_cm.inc( n, cm );
  TC_dig_n_x1.inc( n, x1 );
  TC_dig_n_x1s.inc( n, x1s );

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

