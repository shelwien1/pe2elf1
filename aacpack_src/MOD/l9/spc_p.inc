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
    s) and `_WF` the one that mixes them (by node and the state of
    code()'s hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G`
    the sign counters.  Each is H_spc_ND (or H_spc_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_spc_make( const H_spc_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_spc_VARS(H_LOCAL)

  a = 0;
  a = a*11 + H9_spc_a_cb[__min(10,__max(0,(cb)-(1-1)))];
  a = a*16 + H9_spc_a_a1[__min(15,__max(0,(a1)-(1-1)))];
  a = a*8 + H9_spc_a_a2[__min(7,__max(0,(a2)-(1-1)))];

  b = 0;
  b = b*11 + H9_spc_b_cb[__min(10,__max(0,(cb)-(1-1)))];
  b = b*32 + H9_spc_b_av[__min(31,__max(0,(av)-(1-1)))];
  b = b*4 + H9_spc_b_a1[__min(3,__max(0,(a1)-(1-1)))];

  c = 0;
  c = c*11 + H9_spc_c_cb[__min(10,__max(0,(cb)-(1-1)))];
  c = c*16 + H9_spc_c_pf[__min(15,__max(0,(pf)-(1-1)))];
  c = c*8 + H9_spc_c_a1[__min(7,__max(0,(a1)-(1-1)))];

  d = 0;
  d = d*11 + H9_spc_d_cb[__min(10,__max(0,(cb)-(1-1)))];
  d = d*15 + H9_spc_d_ex[__min(14,__max(0,(ex)-(1-1)))];
  d = d*4 + H9_spc_d_a1[__min(3,__max(0,(a1)-(1-1)))];

  e = 0;
  e = e*11 + H9_spc_e_cb[__min(10,__max(0,(cb)-(1-1)))];
  e = e*16 + H9_spc_e_oc[__min(15,__max(0,(oc)-(1-1)))];
  e = e*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));
  e = e*4 + H9_spc_e_a1[__min(3,__max(0,(a1)-(1-1)))];

  h = 0;
  h = h*11 + H9_spc_h_cb[__min(10,__max(0,(cb)-(1-1)))];
  h = h*11 + H9_spc_h_a3[__min(10,__max(0,(a3)-(1-1)))];
  h = h*8 + H9_spc_h_nz[__min(7,__max(0,(nz)-(1-1)))];
  h = h*4 + H9_spc_h_bp[__min(3,__max(0,(bp)-(1-1)))];

  i = 0;
  i = i*11 + H9_spc_i_cb[__min(10,__max(0,(cb)-(1-1)))];
  i = i*13 + H9_spc_i_pn[__min(12,__max(0,(pn)-(1-1)))];
  i = i*6 + H9_spc_i_pf[__min(5,__max(0,(pf)-(1-1)))];

  j = 0;
  j = j*11 + H9_spc_j_cb[__min(10,__max(0,(cb)-(1-1)))];
  j = j*4 + H9_spc_j_tp[__min(3,__max(0,(tp)-(1-1)))];
  j = j*9 + H9_spc_j_a1[__min(8,__max(0,(a1)-(1-1)))];
  j = j*2 + (((ws)>0+(1-1)));

  k = 0;
  k = k*11 + H9_spc_k_cb[__min(10,__max(0,(cb)-(1-1)))];
  k = k*11 + H9_spc_k_qn[__min(10,__max(0,(qn)-(1-1)))];
  k = k*8 + H9_spc_k_ds[__min(7,__max(0,(ds)-(1-1)))];

  l = 0;
  l = l*11 + H9_spc_l_cb[__min(10,__max(0,(cb)-(1-1)))];
  l = l*51 + H9_spc_l_sb[__min(50,__max(0,(sb)-(1-1)))];
  l = l*2 + (((ws)>0+(1-1)));

  s = 0;
  s = s*11 + H9_spc_s_cb[__min(10,__max(0,(cb)-(1-1)))];
  s = s*32 + H9_spc_s_av[__min(31,__max(0,(av)-(1-1)))];
  s = s*4 + H9_spc_s_a1[__min(3,__max(0,(a1)-(1-1)))];

  f = 0;
  f = f*11 + H9_spc_f_cb[__min(10,__max(0,(cb)-(1-1)))];
  f = f*40 + H9_spc_f_xp[__min(39,__max(0,(xp)-(1-1)))];

  m = 0;
  m = m*11 + H9_spc_m_cb[__min(10,__max(0,(cb)-(1-1)))];
  m = m*2 + (((ws)>0+(1-1)));
  m = m*8 + H9_spc_m_av[__min(28,__max(0,(av)-(1-1)))];

  w = 0;
  w = w*8 + H9_spc_w_a1[__min(7,__max(0,(a1)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_spc
#define FC_PFX H9_spc_
#define FC_FAM spc
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
