/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a family's index builder,
    the tables its indices address and its configuration class, at level -9.  Copied through
    into MOD/l9/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that mixes them (by node
    and the state of code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the
    sign counters.  Each is M_hdr_ND (or M_hdr_MND) rows deep per context: where a decision sits
    among the family's nodes is context without being a variable.  */

void M9_hdr_make( const M_hdr_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  M_hdr_VARS(M_LOCAL)

  a = 0;
  a = a*8 + M9_hdr_a_fld[__min(39,__max(0,(fld)-(1-1)))];
  a = a*4 + M9_hdr_a_ty[__min(4,__max(0,(ty)-(1-1)))];
  a = a*4 + M9_hdr_a_p1[__min(64,__max(0,(p1)-(1-1)))];

  b = 0;
  b = b*11 + M9_hdr_b_fld[__min(39,__max(0,(fld)-(1-1)))];
  b = b*4 + M9_hdr_b_p1[__min(64,__max(0,(p1)-(1-1)))];

  c = 0;
  c = c*9 + M9_hdr_c_fld[__min(39,__max(0,(fld)-(1-1)))];
  c = c*2 + (((ty)>1+(1-1)));

  d = 0;
  d = d*11 + M9_hdr_d_fld[__min(39,__max(0,(fld)-(1-1)))];

  e = 0;
  e = e*11 + M9_hdr_e_fld[__min(39,__max(0,(fld)-(1-1)))];

  h = 0;
  h = h*13 + M9_hdr_h_fld[__min(39,__max(0,(fld)-(1-1)))];
  h = h*3 + (((ty)>1+(1-1))+((ty)>3+(1-1)));
  h = h*2 + (((pos)>0+(1-1)));

  i = 0;
  i = i*2 + (((ty)>1+(1-1)));

  j = 0;
  j = j*6 + M9_hdr_j_fld[__min(39,__max(0,(fld)-(1-1)))];
  j = j*5 + M9_hdr_j_p1[__min(64,__max(0,(p1)-(1-1)))];
  j = j*5 + M9_hdr_j_pos[__min(31,__max(0,(pos)-(1-1)))];

  k = 0;

  l = 0;
  l = l*10 + M9_hdr_l_fld[__min(39,__max(0,(fld)-(1-1)))];
  l = l*3 + (((ty)>1+(1-1))+((ty)>3+(1-1)));
  l = l*2 + (((p1)>0+(1-1)));
  l = l*3 + (((pos)>0+(1-1))+((pos)>1+(1-1)));

  s = 0;
  s = s*16 + M9_hdr_s_fld[__min(39,__max(0,(fld)-(1-1)))];

  f = 0;
  f = f*13 + M9_hdr_f_fld[__min(39,__max(0,(fld)-(1-1)))];

  m = 0;
  m = m*11 + M9_hdr_m_fld[__min(39,__max(0,(fld)-(1-1)))];

  w = 0;
  w = w*5 + M9_hdr_w_ty[__min(4,__max(0,(ty)-(1-1)))];

  t = 0;
  t = t*5 + M9_hdr_t_fld[__min(39,__max(0,(fld)-(1-1)))];

  n = 0;
  n = n*40 + M9_hdr_n_fld[__min(39,__max(0,(fld)-(1-1)))];

  g = 0;
  g = g*40 + M9_hdr_g_fld[__min(39,__max(0,(fld)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_hdr
#define FC_PFX M9_hdr_
#define FC_FAM hdr
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
