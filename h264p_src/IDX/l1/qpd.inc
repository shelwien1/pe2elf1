/*  The template IDX/idx2inc.pl reads alongside qpd.idx: the qpd
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/qpd_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/qpd_h.inc.  Edit this file
    and qpd.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_qpd_ND (or H_qpd_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int lnz = D.lnz, pc = D.pc, cc = D.cc, cls = D.cls, dc = D.dc, db = D.db, lz = D.lz, st = D.st, ae = D.ae, aq = D.aq, ac = D.ac, td = D.td, sq = D.sq, xe = D.xe, lq = D.lq, bc = D.bc, cl = D.cl, kc = D.kc;
  (void) lnz;  (void) pc;  (void) cc;  (void) cls;  (void) dc;  (void) db;  (void) lz;  (void) st;  (void) ae;  (void) aq;  (void) ac;  (void) td;  (void) sq;  (void) xe;  (void) lq;  (void) bc;  (void) cl;  (void) kc;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2s, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_qpd_ND );
Table( cm_cnt2s, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_qpd_ND );
Table( cm_cnt2s, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_qpd_ND );
Table( cm_cnt2s, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_qpd_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_qpd_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_qpd_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_qpd_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_qpd_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_qpd_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_qpd_NG );
