/*  The template idx2inc.pl reads alongside tsvcomp-dig.idx: the dig family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-dig_p.inc with `%M%`
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
      x1 = D.x1, x1s = D.x1s;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;   (void) ps;   (void) chn;
  (void) crun; (void) cm;   (void) x1;    (void) x1s;
  MakeIndex a,b,c,d,s,f,m,t,n,g
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt, %M%A, %M%a_Volume * %M%ND );
Table( cm_cnt, %M%B, %M%b_Volume * %M%ND );
Table( cm_cnt, %M%C, %M%c_Volume * %M%ND );
Table( cm_cnt, %M%D, %M%d_Volume * %M%ND );
Table( cm_cnt, %M%MT, %M%t_Volume * %M%MND );
Table( i16,    %M%S, %M%s_Volume * %M%ND * %M%qs );
Table( u8,     %M%SC, %M%s_Volume * %M%ND * %M%qs );
Table( i16,    %M%F, %M%f_Volume * %M%ND * %M%qf );
Table( u8,     %M%FC, %M%f_Volume * %M%ND * %M%qf );
Table( i32,    %M%W, %M%m_Volume * %M%ND * 7 );
Table( u8,     %M%WC, %M%m_Volume * %M%ND );
Table( i32,    %M%WM, %M%n_Volume * %M%MND * 3 );
Table( u8,     %M%WMC, %M%n_Volume * %M%MND );
