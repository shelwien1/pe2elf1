/*  The template IDX/idx2inc.pl reads alongside ipm.idx: the ipm
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/ipm_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/ipm_h.inc.  Edit this file
    and ipm.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_ipm_ND (or H_ipm_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_ipm_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int k8 = D.k8, ma = D.ma, mb = D.mb, mp = D.mp, blk = D.blk, pv = D.pv, col = D.col, dm = D.dm, ds = D.ds, pm = D.pm, pr = D.pr, dr = D.dr, pq = D.pq, mh = D.mh, mc = D.mc, mg = D.mg, rg = D.rg, p2 = D.p2;
  (void) k8;  (void) ma;  (void) mb;  (void) mp;  (void) blk;  (void) pv;  (void) col;  (void) dm;  (void) ds;  (void) pm;  (void) pr;  (void) dr;  (void) pq;  (void) mh;  (void) mc;  (void) mg;  (void) rg;  (void) p2;
  MakeIndex a,b,c,d,e,h,i,j,k,l,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_ipm_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_ipm_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_ipm_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_ipm_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * H_ipm_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * H_ipm_ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * H_ipm_ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * H_ipm_ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * H_ipm_ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * H_ipm_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_ipm_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_ipm_ND * 24 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_ipm_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_ipm_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_ipm_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_ipm_NG );
