/*  The template idx2inc.pl reads alongside tsvcomp-flr.idx: the flr family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-flr_p.inc with `TC_flr_`
    replaced by the prefix -- `TC_flr_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_flr_T struct in MOD/tsvcomp-flr_h.inc.

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

void tc_make_flr( int fno, int col, int p1, int fy, int raw, int blk,
                  int o1, int prd, int od, int chn,
                  int room, int hl, int lov, int hiv, int pri, int ep, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) fno;  (void) col;  (void) p1;  (void) fy;  (void) raw;  (void) blk;
  (void) o1;   (void) prd;  (void) od;  (void) chn;
  (void) room; (void) hl;   (void) lov; (void) hiv; (void) pri;  (void) ep;

  a = 0;
  TC_flr_a_fno.inc( a, fno );
  TC_flr_a_col.inc( a, col );
  TC_flr_a_p1.inc( a, p1 );
  TC_flr_a_fy.inc( a, fy );
  TC_flr_a_raw.inc( a, raw );
  TC_flr_a_blk.inc( a, blk );
  TC_flr_a_o1.inc( a, o1 );
  TC_flr_a_prd.inc( a, prd );
  TC_flr_a_od.inc( a, od );
  TC_flr_a_chn.inc( a, chn );
  TC_flr_a_room.inc( a, room );
  TC_flr_a_hl.inc( a, hl );
  TC_flr_a_lov.inc( a, lov );
  TC_flr_a_hiv.inc( a, hiv );
  TC_flr_a_pri.inc( a, pri );
  TC_flr_a_ep.inc( a, ep );

  b = 0;
  TC_flr_b_fno.inc( b, fno );
  TC_flr_b_col.inc( b, col );
  TC_flr_b_p1.inc( b, p1 );
  TC_flr_b_fy.inc( b, fy );
  TC_flr_b_raw.inc( b, raw );
  TC_flr_b_blk.inc( b, blk );
  TC_flr_b_o1.inc( b, o1 );
  TC_flr_b_prd.inc( b, prd );
  TC_flr_b_od.inc( b, od );
  TC_flr_b_chn.inc( b, chn );
  TC_flr_b_room.inc( b, room );
  TC_flr_b_hl.inc( b, hl );
  TC_flr_b_lov.inc( b, lov );
  TC_flr_b_hiv.inc( b, hiv );
  TC_flr_b_pri.inc( b, pri );
  TC_flr_b_ep.inc( b, ep );

  c = 0;
  TC_flr_c_col.inc( c, col );
  TC_flr_c_p1.inc( c, p1 );
  TC_flr_c_fy.inc( c, fy );
  TC_flr_c_o1.inc( c, o1 );
  TC_flr_c_prd.inc( c, prd );
  TC_flr_c_od.inc( c, od );
  TC_flr_c_chn.inc( c, chn );
  TC_flr_c_room.inc( c, room );
  TC_flr_c_hl.inc( c, hl );
  TC_flr_c_lov.inc( c, lov );
  TC_flr_c_hiv.inc( c, hiv );
  TC_flr_c_pri.inc( c, pri );
  TC_flr_c_ep.inc( c, ep );

  d = 0;
  TC_flr_d_col.inc( d, col );
  TC_flr_d_p1.inc( d, p1 );
  TC_flr_d_fy.inc( d, fy );
  TC_flr_d_o1.inc( d, o1 );
  TC_flr_d_prd.inc( d, prd );
  TC_flr_d_od.inc( d, od );
  TC_flr_d_chn.inc( d, chn );
  TC_flr_d_room.inc( d, room );
  TC_flr_d_hl.inc( d, hl );
  TC_flr_d_lov.inc( d, lov );
  TC_flr_d_hiv.inc( d, hiv );
  TC_flr_d_pri.inc( d, pri );
  TC_flr_d_ep.inc( d, ep );

  s = 0;
  TC_flr_s_col.inc( s, col );
  TC_flr_s_p1.inc( s, p1 );
  TC_flr_s_fy.inc( s, fy );
  TC_flr_s_o1.inc( s, o1 );
  TC_flr_s_prd.inc( s, prd );
  TC_flr_s_od.inc( s, od );
  TC_flr_s_chn.inc( s, chn );
  TC_flr_s_room.inc( s, room );
  TC_flr_s_hl.inc( s, hl );
  TC_flr_s_lov.inc( s, lov );
  TC_flr_s_hiv.inc( s, hiv );
  TC_flr_s_pri.inc( s, pri );
  TC_flr_s_ep.inc( s, ep );

  f = 0;
  TC_flr_f_col.inc( f, col );
  TC_flr_f_p1.inc( f, p1 );
  TC_flr_f_fy.inc( f, fy );
  TC_flr_f_o1.inc( f, o1 );
  TC_flr_f_prd.inc( f, prd );
  TC_flr_f_od.inc( f, od );
  TC_flr_f_chn.inc( f, chn );
  TC_flr_f_room.inc( f, room );
  TC_flr_f_hl.inc( f, hl );
  TC_flr_f_lov.inc( f, lov );
  TC_flr_f_hiv.inc( f, hiv );
  TC_flr_f_pri.inc( f, pri );
  TC_flr_f_ep.inc( f, ep );

  m = 0;
  TC_flr_m_col.inc( m, col );
  TC_flr_m_p1.inc( m, p1 );
  TC_flr_m_fy.inc( m, fy );
  TC_flr_m_o1.inc( m, o1 );
  TC_flr_m_prd.inc( m, prd );
  TC_flr_m_od.inc( m, od );
  TC_flr_m_chn.inc( m, chn );
  TC_flr_m_room.inc( m, room );
  TC_flr_m_hl.inc( m, hl );
  TC_flr_m_lov.inc( m, lov );
  TC_flr_m_hiv.inc( m, hiv );
  TC_flr_m_pri.inc( m, pri );
  TC_flr_m_ep.inc( m, ep );

  t = 0;
  TC_flr_t_col.inc( t, col );
  TC_flr_t_p1.inc( t, p1 );
  TC_flr_t_fy.inc( t, fy );
  TC_flr_t_o1.inc( t, o1 );
  TC_flr_t_prd.inc( t, prd );
  TC_flr_t_od.inc( t, od );
  TC_flr_t_chn.inc( t, chn );
  TC_flr_t_room.inc( t, room );
  TC_flr_t_hl.inc( t, hl );
  TC_flr_t_lov.inc( t, lov );
  TC_flr_t_hiv.inc( t, hiv );
  TC_flr_t_pri.inc( t, pri );
  TC_flr_t_ep.inc( t, ep );

  n = 0;
  TC_flr_n_col.inc( n, col );
  TC_flr_n_p1.inc( n, p1 );
  TC_flr_n_fy.inc( n, fy );
  TC_flr_n_o1.inc( n, o1 );
  TC_flr_n_prd.inc( n, prd );
  TC_flr_n_od.inc( n, od );
  TC_flr_n_chn.inc( n, chn );
  TC_flr_n_room.inc( n, room );
  TC_flr_n_hl.inc( n, hl );
  TC_flr_n_lov.inc( n, lov );
  TC_flr_n_hiv.inc( n, hiv );
  TC_flr_n_pri.inc( n, pri );
  TC_flr_n_ep.inc( n, ep );

  g = 0;
  TC_flr_g_col.inc( g, col );
  TC_flr_g_p1.inc( g, p1 );
  TC_flr_g_fy.inc( g, fy );
  TC_flr_g_o1.inc( g, o1 );
  TC_flr_g_prd.inc( g, prd );
  TC_flr_g_od.inc( g, od );
  TC_flr_g_chn.inc( g, chn );
  TC_flr_g_room.inc( g, room );
  TC_flr_g_hl.inc( g, hl );
  TC_flr_g_lov.inc( g, lov );
  TC_flr_g_hiv.inc( g, hiv );
  TC_flr_g_pri.inc( g, pri );
  TC_flr_g_ep.inc( g, ep );
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

