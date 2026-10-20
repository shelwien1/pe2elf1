/*  The template IDX/idx2inc.pl reads alongside cbp.idx: the cbp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/cbp_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/cbp_h.inc.  Edit this file
    and cbp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_cbp_ND (or H_cbp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_cbp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int inc = D.inc, pc = D.pc, cls = D.cls, col = D.col, q6 = D.q6, st = D.st, en = D.en, td = D.td, pg = D.pg, tp = D.tp, em = D.em, pq = D.pq;
  (void) inc;  (void) pc;  (void) cls;  (void) col;  (void) q6;  (void) st;  (void) en;  (void) td;  (void) pg;  (void) tp;  (void) em;  (void) pq;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2s, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_cbp_ND );
Table( cm_cnt2s, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_cbp_ND );
Table( cm_cnt2s, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_cbp_ND );
Table( cm_cnt2s, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_cbp_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_cbp_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_cbp_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_cbp_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_cbp_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_cbp_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_cbp_NG );
