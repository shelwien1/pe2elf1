/*  The template IDX/idx2inc.pl reads alongside mvs.idx: the mvs
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/mvs_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/mvs_h.inc.  Edit this file
    and mvs.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_mvs_ND (or H_mvs_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const H_mvs_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int kind = D.kind, comp = D.comp, lst = D.lst, ma = D.ma, mb = D.mb, oth = D.oth, ps = D.ps, rf = D.rf, pi = D.pi, nr = D.nr, pd = D.pd, sp = D.sp, tv = D.tv, zd = D.zd, cq = D.cq, tq = D.tq, rt = D.rt, ea = D.ea, eb = D.eb, ta = D.ta;
  (void) kind;  (void) comp;  (void) lst;  (void) ma;  (void) mb;  (void) oth;  (void) ps;  (void) rf;  (void) pi;  (void) nr;  (void) pd;  (void) sp;  (void) tv;  (void) zd;  (void) cq;  (void) tq;  (void) rt;  (void) ea;  (void) eb;  (void) ta;
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2s, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_mvs_ND );
Table( cm_cnt2s, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_mvs_ND );
Table( cm_cnt2s, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_mvs_ND );
Table( cm_cnt2s, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_mvs_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_mvs_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_mvs_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_mvs_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_mvs_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_mvs_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_mvs_NG );
