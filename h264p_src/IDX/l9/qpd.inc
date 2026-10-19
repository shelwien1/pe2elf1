/*  The template IDX/idx2inc.pl reads alongside qpd.idx: the qpd
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/qpd_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/qpd_h.inc.  Edit this file
    and qpd.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_qpd_ND (or H_qpd_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int lnz = D.lnz, pc = D.pc, cc = D.cc, cls = D.cls, dc = D.dc, db = D.db, lz = D.lz, st = D.st, ae = D.ae, aq = D.aq, ac = D.ac, td = D.td, sq = D.sq, xe = D.xe, lq = D.lq, bc = D.bc, cl = D.cl, kc = D.kc;
  (void) lnz;  (void) pc;  (void) cc;  (void) cls;  (void) dc;  (void) db;  (void) lz;  (void) st;  (void) ae;  (void) aq;  (void) ac;  (void) td;  (void) sq;  (void) xe;  (void) lq;  (void) bc;  (void) cl;  (void) kc;
  MakeIndex a,b,c,d,e,h,i,j,k,l,s,f,m,w,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_qpd_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_qpd_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_qpd_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_qpd_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * H_qpd_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * H_qpd_ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * H_qpd_ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * H_qpd_ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * H_qpd_ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * H_qpd_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_qpd_MND );
Table( i16,    %M%S, tc_rows(%M%s_Volume, %M%hS) * H_qpd_ND * %M%qs );
Table( u8,     %M%SC, tc_rows(%M%s_Volume, %M%hS) * H_qpd_ND * %M%qs );
Table( i16,    %M%F, tc_rows(%M%f_Volume, %M%hF) * H_qpd_ND * %M%qf );
Table( u8,     %M%FC, tc_rows(%M%f_Volume, %M%hF) * H_qpd_ND * %M%qf );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_qpd_ND * TC_MIXN );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_qpd_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_qpd_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_qpd_MND );
Table( i32,    %M%W2, %M%w_Volume * H_qpd_ND * TC_MIXN );
Table( u8,     %M%W2C, %M%w_Volume * H_qpd_ND );
Table( i32,    %M%W3, tc_rows(%M%s_Volume, %M%hS) * H_qpd_ND * TC_MIXN );
Table( u8,     %M%W3C, tc_rows(%M%s_Volume, %M%hS) * H_qpd_ND );
Table( i32,    %M%WF, H_qpd_ND * 5 * 4 );
Table( u8,     %M%WFC, H_qpd_ND * 5 );
Table( cm_cnt, %M%G, %M%g_Volume * H_qpd_NG );
