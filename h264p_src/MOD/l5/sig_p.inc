/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a
    family's index builder, the tables its indices address and its
    configuration class, at level -5.  Copied through into
    MOD/l5/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_W` the one mixer
    (selected by index m), `_MT` and `_WM` the mantissa plane and its
    mixer, `_G` the sign counters.  Each is H_sig_ND (or H_sig_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H5_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  H_sig_VARS(H_LOCAL)

  a = 0;
  a = a*5 + H5_sig_a_cg[__min(13,__max(0,(cg)-(1-1)))];
  a = a*28 + H5_sig_a_pos[__min(63,__max(0,(pos)-(1-1)))];
  a = a*8 + H5_sig_a_rem[__min(10,__max(0,(rem)-(1-1)))];

  b = 0;
  b = b*5 + H5_sig_b_cg[__min(13,__max(0,(cg)-(1-1)))];
  b = b*27 + H5_sig_b_pos[__min(63,__max(0,(pos)-(1-1)))];
  b = b*16 + H5_sig_b_md[__min(19,__max(0,(md)-(1-1)))];
  b = b*319 + H5_sig_b_hb[__min(510,__max(0,(hb)-(1-1)))];

  c = 0;
  c = c*6 + H5_sig_c_cg[__min(13,__max(0,(cg)-(1-1)))];
  c = c*25 + H5_sig_c_pos[__min(63,__max(0,(pos)-(1-1)))];
  c = c*6 + H5_sig_c_q6[__min(8,__max(0,(q6)-(1-1)))];
  c = c*11 + H5_sig_c_pg[__min(16,__max(0,(pg)-(1-1)))];
  c = c*9 + H5_sig_c_ft[__min(8,__max(0,(ft)-(1-1)))];

  d = 0;
  d = d*4 + H5_sig_d_cg[__min(13,__max(0,(cg)-(1-1)))];
  d = d*7 + H5_sig_d_rem[__min(10,__max(0,(rem)-(1-1)))];
  d = d*8 + H5_sig_d_zl[__min(10,__max(0,(zl)-(1-1)))];
  d = d*9 + H5_sig_d_em[__min(16,__max(0,(em)-(1-1)))];
  d = d*291 + H5_sig_d_hb[__min(510,__max(0,(hb)-(1-1)))];

  e = 0;
  e = e*6 + H5_sig_e_cg[__min(13,__max(0,(cg)-(1-1)))];
  e = e*36 + H5_sig_e_pos[__min(63,__max(0,(pos)-(1-1)))];

  h = 0;
  h = h*6 + H5_sig_h_cg[__min(13,__max(0,(cg)-(1-1)))];
  h = h*46 + H5_sig_h_pos[__min(63,__max(0,(pos)-(1-1)))];
  h = h*5 + H5_sig_h_rem[__min(10,__max(0,(rem)-(1-1)))];
  h = h*8 + H5_sig_h_tz[__min(8,__max(0,(tz)-(1-1)))];
  h = h*8 + H5_sig_h_h3[__min(14,__max(0,(h3)-(1-1)))];

  i = 0;
  i = i*5 + H5_sig_i_cg[__min(13,__max(0,(cg)-(1-1)))];
  i = i*46 + H5_sig_i_pos[__min(63,__max(0,(pos)-(1-1)))];
  i = i*6 + H5_sig_i_hv[__min(7,__max(0,(hv)-(1-1)))];

  j = 0;
  j = j*12 + H5_sig_j_pos[__min(63,__max(0,(pos)-(1-1)))];
  j = j*6 + H5_sig_j_ft[__min(8,__max(0,(ft)-(1-1)))];
  j = j*2 + (((h3)>0+(1-1)));

  k = 0;
  k = k*11 + H5_sig_k_cg[__min(13,__max(0,(cg)-(1-1)))];
  k = k*58 + H5_sig_k_pos[__min(63,__max(0,(pos)-(1-1)))];
  k = k*7 + H5_sig_k_rem[__min(10,__max(0,(rem)-(1-1)))];
  k = k*13 + H5_sig_k_h3[__min(14,__max(0,(h3)-(1-1)))];
  k = k*4 + H5_sig_k_sn[__min(8,__max(0,(sn)-(1-1)))];
  k = k*6 + H5_sig_k_hv[__min(7,__max(0,(hv)-(1-1)))];

  l = 0;
  l = l*6 + H5_sig_l_cg[__min(13,__max(0,(cg)-(1-1)))];
  l = l*36 + H5_sig_l_pos[__min(63,__max(0,(pos)-(1-1)))];
  l = l*4 + H5_sig_l_sn[__min(8,__max(0,(sn)-(1-1)))];
  l = l*8 + H5_sig_l_pf[__min(11,__max(0,(pf)-(1-1)))];

  m = 0;
  m = m*5 + H5_sig_m_cg[__min(13,__max(0,(cg)-(1-1)))];
  m = m*3 + (((pos)>0+(1-1))+((pos)>8+(1-1)));
  m = m*3 + (((rem)>2+(1-1))+((rem)>6+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_sig
#define FC_PFX H5_sig_
#define FC_FAM sig
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
