/*  The template IDX/idx2inc.pl reads alongside qpg.idx: the qpg
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/qpg_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/qpg_h.inc.  Edit this file
    and qpg.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_qpg_ND (or H_qpg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int as = D.as, cs = D.cs, bs = D.bs, ls = D.ls, rs = D.rs, mag = D.mag, cls = D.cls, st = D.st, ss = D.ss, xe = D.xe, lq = D.lq, bk = D.bk;
  (void) as;  (void) cs;  (void) bs;  (void) ls;  (void) rs;  (void) mag;  (void) cls;  (void) st;  (void) ss;  (void) xe;  (void) lq;  (void) bk;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2s, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_qpg_ND );
Table( cm_cnt2s, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_qpg_ND );
Table( cm_cnt2s, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_qpg_ND );
Table( cm_cnt2s, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_qpg_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_qpg_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_qpg_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_qpg_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_qpg_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_qpg_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_qpg_NG );
