/*  The template idx2inc.pl reads alongside tsvcomp-cls.idx: the cls family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-cls_p.inc with `%M%`
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
Table( i32,    %M%W, %M%m_Volume * %M%ND * TC_MIXN );
Table( u8,     %M%WC, %M%m_Volume * %M%ND );
Table( i32,    %M%WM, %M%n_Volume * %M%MND * 3 );
Table( u8,     %M%WMC, %M%n_Volume * %M%MND );
Table( cm_cnt, %M%G, %M%g_Volume * TC_SGN_CLS );
