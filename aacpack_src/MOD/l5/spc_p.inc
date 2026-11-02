/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l5 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -5: the ten counters and one mixer, no APMs.  Copied through into
    MOD/l5/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l5/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters and `_W` the one mixer (selected by
    index m, rows of 24 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_spc_ND (or H_spc_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H5_spc_make( const H_spc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, m, t, g;
  H_spc_VARS(H_LOCAL)

  a = 0;
  a = a*12 + H5_spc_a_cb[__min(11,__max(0,(cb)-(1-1)))];
  a = a*6 + H5_spc_a_a1[__min(15,__max(0,(a1)-(1-1)))];
  a = a*8 + H5_spc_a_a2[__min(7,__max(0,(a2)-(1-1)))];

  b = 0;
  b = b*12 + H5_spc_b_cb[__min(11,__max(0,(cb)-(1-1)))];
  b = b*24 + H5_spc_b_av[__min(31,__max(0,(av)-(1-1)))];

  c = 0;
  c = c*12 + H5_spc_c_cb[__min(11,__max(0,(cb)-(1-1)))];
  c = c*16 + H5_spc_c_pf[__min(15,__max(0,(pf)-(1-1)))];
  c = c*5 + H5_spc_c_a1[__min(7,__max(0,(a1)-(1-1)))];
  c = c*70 + H5_spc_c_dg[__min(69,__max(0,(dg)-(1-1)))];

  d = 0;
  d = d*12 + H5_spc_d_cb[__min(11,__max(0,(cb)-(1-1)))];
  d = d*12 + H5_spc_d_ex[__min(14,__max(0,(ex)-(1-1)))];
  d = d*4 + H5_spc_d_a1[__min(3,__max(0,(a1)-(1-1)))];

  e = 0;
  e = e*12 + H5_spc_e_cb[__min(11,__max(0,(cb)-(1-1)))];
  e = e*10 + H5_spc_e_oc[__min(15,__max(0,(oc)-(1-1)))];
  e = e*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));
  e = e*3 + (((a1)>1+(1-1))+((a1)>2+(1-1)));

  h = 0;
  h = h*12 + H5_spc_h_cb[__min(11,__max(0,(cb)-(1-1)))];
  h = h*11 + H5_spc_h_a3[__min(10,__max(0,(a3)-(1-1)))];
  h = h*8 + H5_spc_h_nz[__min(7,__max(0,(nz)-(1-1)))];
  h = h*4 + H5_spc_h_bp[__min(3,__max(0,(bp)-(1-1)))];
  h = h*5 + H5_spc_h_pl[__min(4,__max(0,(pl)-(1-1)))];
  h = h*9 + H5_spc_h_zr[__min(13,__max(0,(zr)-(1-1)))];

  i = 0;
  i = i*12 + H5_spc_i_cb[__min(11,__max(0,(cb)-(1-1)))];
  i = i*10 + H5_spc_i_pn[__min(12,__max(0,(pn)-(1-1)))];
  i = i*6 + H5_spc_i_pf[__min(5,__max(0,(pf)-(1-1)))];

  j = 0;
  j = j*12 + H5_spc_j_cb[__min(11,__max(0,(cb)-(1-1)))];
  j = j*4 + H5_spc_j_tp[__min(3,__max(0,(tp)-(1-1)))];
  j = j*6 + H5_spc_j_a1[__min(8,__max(0,(a1)-(1-1)))];
  j = j*2 + (((ws)>0+(1-1)));

  k = 0;
  k = k*12 + H5_spc_k_cb[__min(11,__max(0,(cb)-(1-1)))];
  k = k*6 + H5_spc_k_qn[__min(10,__max(0,(qn)-(1-1)))];
  k = k*7 + H5_spc_k_ds[__min(7,__max(0,(ds)-(1-1)))];
  k = k*17 + H5_spc_k_sc[__min(16,__max(0,(sc)-(1-1)))];

  l = 0;
  l = l*12 + H5_spc_l_cb[__min(11,__max(0,(cb)-(1-1)))];
  l = l*41 + H5_spc_l_sb[__min(50,__max(0,(sb)-(1-1)))];
  l = l*2 + (((ws)>0+(1-1)));

  m = 0;
  m = m*12 + H5_spc_m_cb[__min(11,__max(0,(cb)-(1-1)))];
  m = m*2 + (((ws)>0+(1-1)));
  m = m*6 + H5_spc_m_av[__min(28,__max(0,(av)-(1-1)))];

  t = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C5_spc
#define FC_PFX H5_spc_
#define FC_FAM spc
#define FC_NC 10
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
