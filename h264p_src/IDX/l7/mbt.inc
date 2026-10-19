/*  The template IDX/idx2inc.pl reads alongside mbt.idx: the mbt
    family's index builder and the tables its indices address.  Copied
    through into MOD/l7/mbt_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l7/mbt_h.inc.  Edit this file
    and mbt.idx, not MOD/; ../../mk.sh regenerates both.

    Level -7: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_mbt_ND (or H_mbt_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int st = D.st, ca = D.ca, cb = D.cb, cc = D.cc, nsk = D.nsk, en = D.en, td = D.td, sm = D.sm, tm = D.tm, tb = D.tb, dh = D.dh, bd = D.bd, qc = D.qc, im = D.im, ig = D.ig, c1 = D.c1, tx = D.tx;
  (void) st;  (void) ca;  (void) cb;  (void) cc;  (void) nsk;  (void) en;  (void) td;  (void) sm;  (void) tm;  (void) tb;  (void) dh;  (void) bd;  (void) qc;  (void) im;  (void) ig;  (void) c1;  (void) tx;
  MakeIndex a,b,c,d,e,h,i,j,k,l,s,m,w,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_mbt_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_mbt_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_mbt_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_mbt_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * H_mbt_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * H_mbt_ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * H_mbt_ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * H_mbt_ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * H_mbt_ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * H_mbt_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_mbt_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_mbt_ND * 24 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_mbt_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_mbt_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_mbt_MND );
Table( i32,    %M%W2, %M%w_Volume * H_mbt_ND * 24 );
Table( u8,     %M%W2C, %M%w_Volume * H_mbt_ND );
Table( i32,    %M%W3, tc_rows(%M%s_Volume, %M%hS) * H_mbt_ND * 24 );
Table( u8,     %M%W3C, tc_rows(%M%s_Volume, %M%hS) * H_mbt_ND );
Table( i32,    %M%WF, H_mbt_ND * 5 * 4 );
Table( u8,     %M%WFC, H_mbt_ND * 5 );
Table( cm_cnt, %M%G, %M%g_Volume * H_mbt_NG );
