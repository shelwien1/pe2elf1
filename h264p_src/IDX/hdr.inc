/*  The template IDX/idx2inc.pl reads alongside hdr.idx: the hdr
    family's index builder and the tables its indices address.  Copied
    through into MOD/hdr_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/hdr_h.inc.  Edit this file and
    hdr.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index w) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is %M%ND (or %M%MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const %M%v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int tag = D.tag, fld = D.fld, p1 = D.p1, p2 = D.p2, pos = D.pos, p1r = D.p1r, sty = D.sty;
  (void) tag;  (void) fld;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;  (void) sty;
  MakeIndex a,b,c,d,e,h,i,j,k,l,s,f,m,w,t,n,g
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * %M%ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * %M%ND );
Table( cm_cnt2, %M%C, %M%c_Volume * %M%ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * %M%ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * %M%ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * %M%ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * %M%ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * %M%ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * %M%ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * %M%ND );
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
