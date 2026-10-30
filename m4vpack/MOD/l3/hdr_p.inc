/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3: a family's index builder,
    the tables its indices address and its configuration class, at level -3.  Copied through
    into MOD/l3/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by index m, rows of
    16 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_hdr_ND (or M_hdr_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void M3_hdr_make( const M_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, n, g;
  M_hdr_VARS(M_LOCAL)

  a = 0;
  a = a*40 + M3_hdr_a_fld[__min(39,__max(0,(fld)-(1-1)))];
  a = a*5 + M3_hdr_a_ty[__min(4,__max(0,(ty)-(1-1)))];
  a = a*16 + M3_hdr_a_p1[__min(64,__max(0,(p1)-(1-1)))];

  b = 0;
  b = b*40 + M3_hdr_b_fld[__min(39,__max(0,(fld)-(1-1)))];
  b = b*5 + M3_hdr_b_ty[__min(4,__max(0,(ty)-(1-1)))];
  b = b*32 + M3_hdr_b_pos[__min(31,__max(0,(pos)-(1-1)))];

  c = 0;
  c = c*40 + M3_hdr_c_fld[__min(39,__max(0,(fld)-(1-1)))];
  c = c*16 + M3_hdr_c_p1[__min(64,__max(0,(p1)-(1-1)))];
  c = c*32 + M3_hdr_c_pos[__min(31,__max(0,(pos)-(1-1)))];

  d = 0;
  d = d*40 + M3_hdr_d_fld[__min(39,__max(0,(fld)-(1-1)))];
  d = d*5 + M3_hdr_d_ty[__min(4,__max(0,(ty)-(1-1)))];

  e = 0;
  e = e*40 + M3_hdr_e_fld[__min(39,__max(0,(fld)-(1-1)))];
  e = e*16 + M3_hdr_e_p1[__min(64,__max(0,(p1)-(1-1)))];

  h = 0;
  h = h*40 + M3_hdr_h_fld[__min(39,__max(0,(fld)-(1-1)))];

  m = 0;
  m = m*40 + M3_hdr_m_fld[__min(39,__max(0,(fld)-(1-1)))];

  t = 0;
  t = t*40 + M3_hdr_t_fld[__min(39,__max(0,(fld)-(1-1)))];

  n = 0;
  n = n*40 + M3_hdr_n_fld[__min(39,__max(0,(fld)-(1-1)))];

  g = 0;
  g = g*40 + M3_hdr_g_fld[__min(39,__max(0,(fld)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C3_hdr
#define FC_PFX M3_hdr_
#define FC_FAM hdr
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
