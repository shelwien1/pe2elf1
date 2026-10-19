/*  The template IDX/idx2inc.pl reads alongside pcm.idx: the pcm
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/pcm_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/pcm_h.inc.  Edit this file
    and pcm.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_pcm_ND (or H_pcm_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_pcm_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int pl = D.pl, dp = D.dp, pd = D.pd, pd2 = D.pd2;
  (void) pl;  (void) dp;  (void) pd;  (void) pd2;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_pcm_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_pcm_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_pcm_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_pcm_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_pcm_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_pcm_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_pcm_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_pcm_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_pcm_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_pcm_NG );
