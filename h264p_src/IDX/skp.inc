/*  The template IDX/idx2inc.pl reads alongside skp.idx: the skp
    family's index builder and the tables its indices address.  Copied
    through into MOD/skp_p.inc with `%M%` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/skp_h.inc.  Edit this file and
    skp.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is %M%ND (or %M%MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void %M%make( const %M%v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int st = D.st, a3 = D.a3, b3 = D.b3, col = D.col, c9 = D.c9, cd = D.cd, run = D.run, cnt = D.cnt, eq = D.eq, lf = D.lf, af = D.af, td = D.td, en = D.en;
  (void) st;  (void) a3;  (void) b3;  (void) col;  (void) c9;  (void) cd;  (void) run;  (void) cnt;  (void) eq;  (void) lf;  (void) af;  (void) td;  (void) en;
  MakeIndex a,b,c,d,s,f,m,t,n,g
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt, %M%A, tc_rows(%M%a_Volume, %M%hA) * %M%ND );
Table( cm_cnt, %M%B, tc_rows(%M%b_Volume, %M%hB) * %M%ND );
Table( cm_cnt, %M%C, %M%c_Volume * %M%ND );
Table( cm_cnt, %M%D, tc_rows(%M%d_Volume, %M%hD) * %M%ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * %M%MND );
Table( i16,    %M%S, tc_rows(%M%s_Volume, %M%hS) * %M%ND * %M%qs );
Table( u8,     %M%SC, tc_rows(%M%s_Volume, %M%hS) * %M%ND * %M%qs );
Table( i16,    %M%F, tc_rows(%M%f_Volume, %M%hF) * %M%ND * %M%qf );
Table( u8,     %M%FC, tc_rows(%M%f_Volume, %M%hF) * %M%ND * %M%qf );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * %M%ND * TC_MIXN );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * %M%ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * %M%MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * %M%MND );
Table( i32,    %M%W2, tc_rows(%M%d_Volume, %M%hD) * %M%ND * TC_MIXN );
Table( u8,     %M%W2C, tc_rows(%M%d_Volume, %M%hD) * %M%ND );
Table( i32,    %M%WF, %M%ND * 3 );
Table( u8,     %M%WFC, %M%ND );
Table( cm_cnt, %M%G, %M%g_Volume * %M%NG );
