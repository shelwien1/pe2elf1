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
    the sign counters.  Each is H_scf_ND (or H_scf_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_scf_make( const H_scf_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_scf_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  a = a*15 + H9_scf_a_pa[__min(14,__max(0,(pa)-(1-1)))];
  a = a*6 + H9_scf_a_d1[__min(5,__max(0,(d1)-(1-1)))];

  b = 0;
  b = b*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  b = b*15 + H9_scf_b_pd[__min(14,__max(0,(pd)-(1-1)))];
  b = b*4 + H9_scf_b_d1[__min(3,__max(0,(d1)-(1-1)))];

  c = 0;
  c = c*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  c = c*51 + H9_scf_c_sb[__min(50,__max(0,(sb)-(1-1)))];
  c = c*2 + (((ws)>0+(1-1)));

  d = 0;
  d = d*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  d = d*14 + H9_scf_d_d1[__min(13,__max(0,(d1)-(1-1)))];
  d = d*6 + H9_scf_d_d2[__min(5,__max(0,(d2)-(1-1)))];
  d = d*61 + H9_scf_d_dm[__min(60,__max(0,(dm)-(1-1)))];

  e = 0;
  e = e*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  e = e*14 + H9_scf_e_oc[__min(13,__max(0,(oc)-(1-1)))];
  e = e*6 + H9_scf_e_pa[__min(5,__max(0,(pa)-(1-1)))];
  e = e*3 + (((mu)>0+(1-1))+((mu)>1+(1-1)));

  h = 0;
  h = h*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  h = h*16 + H9_scf_h_cb[__min(15,__max(0,(cb)-(1-1)))];
  h = h*4 + H9_scf_h_pa[__min(3,__max(0,(pa)-(1-1)))];

  i = 0;
  i = i*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  i = i*8 + H9_scf_i_gd[__min(7,__max(0,(gd)-(1-1)))];
  i = i*8 + H9_scf_i_pa[__min(7,__max(0,(pa)-(1-1)))];

  j = 0;
  j = j*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  j = j*8 + H9_scf_j_gr[__min(7,__max(0,(gr)-(1-1)))];
  j = j*2 + (((ws)>0+(1-1)));
  j = j*26 + H9_scf_j_pw[__min(25,__max(0,(pw)-(1-1)))];

  k = 0;
  k = k*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  k = k*11 + H9_scf_k_pv[__min(10,__max(0,(pv)-(1-1)))];
  k = k*6 + H9_scf_k_sb[__min(5,__max(0,(sb)-(1-1)))];
  k = k*16 + H9_scf_k_sk[__min(15,__max(0,(sk)-(1-1)))];

  l = 0;
  l = l*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  l = l*13 + H9_scf_l_pa[__min(12,__max(0,(pa)-(1-1)))];
  l = l*6 + H9_scf_l_pd[__min(5,__max(0,(pd)-(1-1)))];

  s = 0;
  s = s*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  s = s*4 + H9_scf_s_pa[__min(3,__max(0,(pa)-(1-1)))];
  s = s*3 + (((fb)>0+(1-1))+((fb)>1+(1-1)));

  f = 0;
  f = f*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  f = f*4 + H9_scf_f_d1[__min(3,__max(0,(d1)-(1-1)))];

  m = 0;
  m = m*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));
  m = m*3 + (((mu)>0+(1-1))+((mu)>1+(1-1)));

  w = 0;
  w = w*2 + (((ws)>0+(1-1)));

  t = 0;
  t = t*3 + (((kd)>0+(1-1))+((kd)>1+(1-1)));

  n = 0;

  g = 0;
  g = g*6 + H9_scf_g_d1[__min(5,__max(0,(d1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_scf
#define FC_PFX H9_scf_
#define FC_FAM scf
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
