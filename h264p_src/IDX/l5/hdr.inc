/*  The template IDX/idx2inc.pl reads alongside hdr.idx: the hdr
    family's index builder and the tables its indices address.  Copied
    through into MOD/l5/hdr_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l5/hdr_h.inc.  Edit this file
    and hdr.idx, not MOD/; ../../mk.sh regenerates both.

    Level -5: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  int tag = D.tag, fld = D.fld, p1 = D.p1, p2 = D.p2, pos = D.pos, p1r = D.p1r, sty = D.sty, pr = D.pr;
  (void) tag;  (void) fld;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;  (void) sty;  (void) pr;
  MakeIndex a,b,c,d,e,h,i,j,k,l,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_hdr_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_hdr_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_hdr_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_hdr_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * H_hdr_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * H_hdr_ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * H_hdr_ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * H_hdr_ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * H_hdr_ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * H_hdr_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_hdr_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_hdr_ND * 24 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_hdr_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_hdr_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_hdr_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_hdr_NG );
