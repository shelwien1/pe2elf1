/*  The template IDX/idx2inc.pl reads alongside lvl.idx: the lvl
    family's index builder and the tables its indices address.  Copied
    through into MOD/lvl_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/lvl_h.inc.  Edit this file and
    lvl.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is %M%ND (or %M%MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const %M%v & D, tcx & X ) {
  int a, b, c, d, e, h, s, f, m, w, t, n, g;
  int cg = D.cg, g1 = D.g1, e1 = D.e1, nc = D.nc, pos = D.pos, sa = D.sa, sb = D.sb, q6 = D.q6, tot = D.tot, md = D.md, pm = D.pm, pg = D.pg, em = D.em, pe = D.pe, pl = D.pl, tz = D.tz, pq = D.pq, lh = D.lh, lt = D.lt, ls = D.ls;
  (void) cg;  (void) g1;  (void) e1;  (void) nc;  (void) pos;  (void) sa;  (void) sb;  (void) q6;  (void) tot;  (void) md;  (void) pm;  (void) pg;  (void) em;  (void) pe;  (void) pl;  (void) tz;  (void) pq;  (void) lh;  (void) lt;  (void) ls;
  MakeIndex a,b,c,d,e,h,s,f,m,w,t,n,g
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * %M%ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * %M%ND );
Table( cm_cnt2, %M%C, %M%c_Volume * %M%ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * %M%ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * %M%ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * %M%ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * %M%MND );
Table( i16,    %M%S, tc_rows(%M%s_Volume, %M%hS) * %M%ND * %M%qs );
Table( u8,     %M%SC, tc_rows(%M%s_Volume, %M%hS) * %M%ND * %M%qs );
Table( i16,    %M%F, tc_rows(%M%f_Volume, %M%hF) * %M%ND * %M%qf );
Table( u8,     %M%FC, tc_rows(%M%f_Volume, %M%hF) * %M%ND * %M%qf );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * %M%ND * TC_MIXN );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * %M%ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * %M%MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * %M%MND );
Table( i32,    %M%W2, %M%w_Volume * %M%ND * TC_MIXN );
Table( u8,     %M%W2C, %M%w_Volume * %M%ND );
Table( i32,    %M%W3, tc_rows(%M%s_Volume, %M%hS) * %M%ND * TC_MIXN );
Table( u8,     %M%W3C, tc_rows(%M%s_Volume, %M%hS) * %M%ND );
Table( i32,    %M%WF, %M%ND * 4 );
Table( u8,     %M%WFC, %M%ND );
Table( cm_cnt, %M%G, %M%g_Volume * %M%NG );
