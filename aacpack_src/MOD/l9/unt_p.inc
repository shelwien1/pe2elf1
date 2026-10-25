/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -9.  Copied through into
    MOD/l9/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l9/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and
    s) and `_WF` the one that mixes them (by node), `_MT` the mantissa
    plane, `_G` the sign counters.  Each is H_unt_ND (or H_unt_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H9_unt_make( const H_unt_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, g;
  H_unt_VARS(H_LOCAL)

  a = 0;
  a = a*16 + H9_unt_a_fld[__min(15,__max(0,(fld)-(1-1)))];
  a = a*8 + H9_unt_a_ctx[__min(7,__max(0,(ctx)-(1-1)))];

  b = 0;
  b = b*16 + H9_unt_b_fld[__min(15,__max(0,(fld)-(1-1)))];
  b = b*7 + H9_unt_b_p1[__min(6,__max(0,(p1)-(1-1)))];

  c = 0;
  c = c*16 + H9_unt_c_fld[__min(15,__max(0,(fld)-(1-1)))];
  c = c*16 + H9_unt_c_ctx[__min(15,__max(0,(ctx)-(1-1)))];

  d = 0;
  d = d*16 + H9_unt_d_fld[__min(15,__max(0,(fld)-(1-1)))];

  e = 0;
  e = e*16 + H9_unt_e_fld[__min(15,__max(0,(fld)-(1-1)))];
  e = e*4 + H9_unt_e_p2[__min(3,__max(0,(p2)-(1-1)))];

  h = 0;
  h = h*16 + H9_unt_h_fld[__min(15,__max(0,(fld)-(1-1)))];
  h = h*4 + H9_unt_h_p1[__min(3,__max(0,(p1)-(1-1)))];
  h = h*4 + H9_unt_h_p2[__min(3,__max(0,(p2)-(1-1)))];

  i = 0;
  i = i*16 + H9_unt_i_ctx[__min(15,__max(0,(ctx)-(1-1)))];
  i = i*7 + H9_unt_i_p1[__min(6,__max(0,(p1)-(1-1)))];

  j = 0;
  j = j*16 + H9_unt_j_fld[__min(15,__max(0,(fld)-(1-1)))];
  j = j*11 + H9_unt_j_p1[__min(10,__max(0,(p1)-(1-1)))];

  k = 0;
  k = k*16 + H9_unt_k_fld[__min(15,__max(0,(fld)-(1-1)))];
  k = k*8 + H9_unt_k_ctx[__min(7,__max(0,(ctx)-(1-1)))];
  k = k*3 + (((p1)>0+(1-1))+((p1)>1+(1-1)));

  l = 0;
  l = l*7 + H9_unt_l_p1[__min(6,__max(0,(p1)-(1-1)))];
  l = l*4 + H9_unt_l_p2[__min(3,__max(0,(p2)-(1-1)))];

  s = 0;
  s = s*16 + H9_unt_s_fld[__min(15,__max(0,(fld)-(1-1)))];

  f = 0;
  f = f*16 + H9_unt_f_fld[__min(15,__max(0,(fld)-(1-1)))];

  m = 0;
  m = m*16 + H9_unt_m_fld[__min(15,__max(0,(fld)-(1-1)))];

  w = 0;
  w = w*8 + H9_unt_w_fld[__min(7,__max(0,(fld)-(1-1)))];

  t = 0;
  t = t*16 + H9_unt_t_fld[__min(15,__max(0,(fld)-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_unt
#define FC_PFX H9_unt_
#define FC_FAM unt
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
