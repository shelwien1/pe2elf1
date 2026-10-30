/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_%F%_ND (or M_%F%_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void %M%make( const M_%F%_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_%F%_VARS(M_LOCAL)
  MakeIndex a,b,c,d,e,h,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2, %M%A, tc_rows(%M%a_Volume, %M%hA) * M_%F%_ND );
Table( cm_cnt2, %M%B, tc_rows(%M%b_Volume, %M%hB) * M_%F%_ND );
Table( cm_cnt2, %M%C, tc_rows(%M%c_Volume, %M%hC) * M_%F%_ND );
Table( cm_cnt2, %M%D, tc_rows(%M%d_Volume, %M%hD) * M_%F%_ND );
Table( cm_cnt2, %M%E, tc_rows(%M%e_Volume, %M%hE) * M_%F%_ND );
Table( cm_cnt2, %M%H, tc_rows(%M%h_Volume, %M%hH) * M_%F%_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * M_%F%_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * M_%F%_ND * tc_nin(6) );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * M_%F%_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * M_%F%_MND * TC_MIN );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * M_%F%_MND );
Table( cm_cnt, %M%G, %M%g_Volume * M_%F%_NG );

// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_%F%
#define FC_PFX %M%
#define FC_FAM %F%
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
