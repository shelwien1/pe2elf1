/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1: a
    family's index builder, the tables its indices address and its
    configuration class, at level -1.  Copied through into
    MOD/l1/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_W` the one mixer (selected by index
    m, rows of 16 inputs), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_%F%_ND (or H_%F%_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void %M%make( const H_%F%_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_%F%_VARS(H_LOCAL)
  MakeIndex a,b,c,d,m,t,n,g
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

Table( cm_cnt2s, %M%A, tc_rows(%M%a_Volume, %M%hA) * H_%F%_ND );
Table( cm_cnt2s, %M%B, tc_rows(%M%b_Volume, %M%hB) * H_%F%_ND );
Table( cm_cnt2s, %M%C, tc_rows(%M%c_Volume, %M%hC) * H_%F%_ND );
Table( cm_cnt2s, %M%D, tc_rows(%M%d_Volume, %M%hD) * H_%F%_ND );
Table( cm_cnt, %M%MT, tc_rows(%M%t_Volume, %M%hT) * H_%F%_MND );
Table( i32,    %M%W, tc_rows(%M%m_Volume, %M%hM) * H_%F%_ND * 16 );
Table( u8,     %M%WC, tc_rows(%M%m_Volume, %M%hM) * H_%F%_ND );
Table( i32,    %M%WM, tc_rows(%M%n_Volume, %M%hN) * H_%F%_MND * 3 );
Table( u8,     %M%WMC, tc_rows(%M%n_Volume, %M%hN) * H_%F%_MND );
Table( cm_cnt, %M%G, %M%g_Volume * H_%F%_NG );

// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_%F%
#define FC_PFX %M%
#define FC_FAM %F%
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
