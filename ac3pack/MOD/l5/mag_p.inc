/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5: a family's index builder,
    the tables its indices address and its configuration class, at level -5.  Copied through
    into MOD/l5/<family>_p.inc with the family's name and prefix in place of their markers and
    `MakeIndex` by the statements that build the indices, and the Table() lines lifted out into
    the struct in MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/; ../../mk.sh
    regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by index m,
    rows of 24 inputs).  `_MT` and `_WM` are the mantissa plane and its mixer, `_G` the sign counters.  Each
    is M_mag_ND (or M_mag_MND) rows deep per context: where a decision sits among the family's
    nodes is context without being a variable.  */

void A5_mag_make( const M_mag_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, n, g;
  M_mag_VARS(M_LOCAL)

  a = 0;
  a = a*16 + A5_mag_a_bap[__min(15,__max(0,(bap)-(1-1)))];
  a = a*4 + A5_mag_a_gr[__min(3,__max(0,(gr)-(1-1)))];

  b = 0;
  b = b*4 + A5_mag_b_bap[__min(15,__max(0,(bap)-(1-1)))];
  b = b*24 + A5_mag_b_e1[__min(23,__max(0,(e1)-(1-1)))];

  c = 0;
  c = c*16 + A5_mag_c_bap[__min(15,__max(0,(bap)-(1-1)))];
  c = c*24 + A5_mag_c_ep[__min(23,__max(0,(ep)-(1-1)))];

  d = 0;
  d = d*16 + A5_mag_d_bap[__min(15,__max(0,(bap)-(1-1)))];
  d = d*10 + A5_mag_d_e1[__min(16,__max(0,(e1)-(1-1)))];
  d = d*10 + A5_mag_d_e2[__min(16,__max(0,(e2)-(1-1)))];

  e = 0;
  e = e*16 + A5_mag_e_bap[__min(15,__max(0,(bap)-(1-1)))];
  e = e*24 + A5_mag_e_eo[__min(23,__max(0,(eo)-(1-1)))];

  h = 0;
  h = h*16 + A5_mag_h_bap[__min(15,__max(0,(bap)-(1-1)))];
  h = h*24 + A5_mag_h_sm[__min(23,__max(0,(sm)-(1-1)))];
  h = h*18 + A5_mag_h_m1[__min(17,__max(0,(m1)-(1-1)))];

  i = 0;
  i = i*16 + A5_mag_i_bap[__min(15,__max(0,(bap)-(1-1)))];
  i = i*10 + A5_mag_i_dx[__min(9,__max(0,(dx)-(1-1)))];
  i = i*10 + A5_mag_i_dn[__min(9,__max(0,(dn)-(1-1)))];
  i = i*4 + A5_mag_i_gs[__min(3,__max(0,(gs)-(1-1)))];

  j = 0;
  j = j*7 + A5_mag_j_bap[__min(15,__max(0,(bap)-(1-1)))];
  j = j*8 + A5_mag_j_ex[__min(21,__max(0,(ex)-(1-1)))];
  j = j*15 + A5_mag_j_bin[__min(45,__max(0,(bin)-(1-1)))];
  j = j*3 + (((cc)>0+(1-1))+((cc)>2+(1-1)));

  k = 0;
  k = k*16 + A5_mag_k_bap[__min(15,__max(0,(bap)-(1-1)))];
  k = k*10 + A5_mag_k_e1[__min(16,__max(0,(e1)-(1-1)))];
  k = k*10 + A5_mag_k_ew[__min(16,__max(0,(ew)-(1-1)))];
  k = k*10 + A5_mag_k_ow[__min(16,__max(0,(ow)-(1-1)))];

  l = 0;
  l = l*16 + A5_mag_l_bap[__min(15,__max(0,(bap)-(1-1)))];
  l = l*5 + A5_mag_l_nz[__min(4,__max(0,(nz)-(1-1)))];
  l = l*2 + (((bs)>0+(1-1)));
  l = l*4 + A5_mag_l_gr[__min(3,__max(0,(gr)-(1-1)))];
  l = l*4 + A5_mag_l_gs[__min(3,__max(0,(gs)-(1-1)))];

  m = 0;
  m = m*16 + A5_mag_m_bap[__min(15,__max(0,(bap)-(1-1)))];

  t = 0;
  t = t*16 + A5_mag_t_bap[__min(15,__max(0,(bap)-(1-1)))];

  n = 0;
  n = n*16 + A5_mag_n_bap[__min(15,__max(0,(bap)-(1-1)))];

  g = 0;
  g = g*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  g = g*3 + (((sp)>0+(1-1))+((sp)>1+(1-1)));
  g = g*3 + (((so)>0+(1-1))+((so)>1+(1-1)));
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as the tables above have it
#define FC_NAME C5_mag
#define FC_PFX A5_mag_
#define FC_FAM mag
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
