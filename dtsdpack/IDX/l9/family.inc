/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_%F%_ND (or M_%F%_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void %M%make( const M_%F%_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_%F%_VARS(M_LOCAL)
  MakeIndex a,b,c,d,e,h,i,j,k,l,s,f,m,w,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * M_%F%_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * M_%F%_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * M_%F%_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * M_%F%_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * M_%F%_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * M_%F%_ND );
Table( cm_cnt2, %M%I, tc_rows(%M%i_Volume, %M%hI) * M_%F%_ND );
Table( cm_cnt2, %M%J, tc_rows(%M%j_Volume, %M%hJ) * M_%F%_ND );
Table( cm_cnt2, %M%K, tc_rows(%M%k_Volume, %M%hK) * M_%F%_ND );
Table( cm_cnt2, %M%L, tc_rows(%M%l_Volume, %M%hL) * M_%F%_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * M_%F%_MND );
Table( i16,    %M%S, tc_rows(%M%s_Volume, %M%hS) * M_%F%_ND * cm_apm_q(%M%qs) );
Table( u8,     %M%SC, tc_rows(%M%s_Volume, %M%hS) * M_%F%_ND * cm_apm_q(%M%qs) );
Table( i16,    %M%F, tc_rows(%M%f_Volume, %M%hF) * M_%F%_ND * cm_apm_q(%M%qf) );
Table( u8,     %M%FC, tc_rows(%M%f_Volume, %M%hF) * M_%F%_ND * cm_apm_q(%M%qf) );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * M_%F%_ND * tc_nin(10) );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * M_%F%_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * M_%F%_MND * TC_MIN );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * M_%F%_MND );
Table( i32,    %M%W2, %M%w_Volume * M_%F%_ND * tc_nin(10) );
Table( u8,     %M%W2C, %M%w_Volume * M_%F%_ND );
Table( i32,    %M%W3, tc_rows(%M%s_Volume, %M%hS) * M_%F%_ND * tc_nin(10) );
Table( u8,     %M%W3C, tc_rows(%M%s_Volume, %M%hS) * M_%F%_ND );
Table( i32,    %M%WF, M_%F%_ND * HS_N * TC_FIN );
Table( u8,     %M%WFC, M_%F%_ND * HS_N );
Table( cm_cnt, %M%G, %M%g_Volume * M_%F%_NG );

// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_%F%
#define FC_PFX %M%
#define FC_FAM %F%
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
