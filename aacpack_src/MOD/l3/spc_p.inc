/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l3 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -3: six counters and one mixer, no APMs.  Copied through into
    MOD/l3/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l3/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters and `_W` the one mixer (selected by
    index m, rows of 16 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_spc_ND (or H_spc_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_spc_make( const H_spc_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_spc_VARS(H_LOCAL)

  a = 0;
  a = a*12 + H3_spc_a_cb[__min(11,__max(0,(cb)-(1-1)))];
  a = a*2 + (((a1)>14+(1-1)));
  a = a*8 + H3_spc_a_a2[__min(7,__max(0,(a2)-(1-1)))];

  b = 0;
  b = b*12 + H3_spc_b_cb[__min(11,__max(0,(cb)-(1-1)))];
  b = b*36 + H3_spc_b_sb[__min(50,__max(0,(sb)-(1-1)))];
  b = b*2 + (((ws)>0+(1-1)));

  c = 0;
  c = c*12 + H3_spc_c_cb[__min(11,__max(0,(cb)-(1-1)))];
  c = c*10 + H3_spc_c_pn[__min(12,__max(0,(pn)-(1-1)))];
  c = c*6 + H3_spc_c_pf[__min(5,__max(0,(pf)-(1-1)))];

  d = 0;
  d = d*12 + H3_spc_d_cb[__min(11,__max(0,(cb)-(1-1)))];
  d = d*16 + H3_spc_d_pf[__min(15,__max(0,(pf)-(1-1)))];
  d = d*5 + H3_spc_d_a1[__min(7,__max(0,(a1)-(1-1)))];
  d = d*70 + H3_spc_d_dg[__min(69,__max(0,(dg)-(1-1)))];

  e = 0;
  e = e*12 + H3_spc_e_cb[__min(11,__max(0,(cb)-(1-1)))];
  e = e*22 + H3_spc_e_av[__min(31,__max(0,(av)-(1-1)))];
  e = e*3 + (((a1)>1+(1-1))+((a1)>2+(1-1)));

  h = 0;
  h = h*12 + H3_spc_h_cb[__min(11,__max(0,(cb)-(1-1)))];
  h = h*11 + H3_spc_h_a3[__min(10,__max(0,(a3)-(1-1)))];
  h = h*8 + H3_spc_h_nz[__min(7,__max(0,(nz)-(1-1)))];
  h = h*4 + H3_spc_h_bp[__min(3,__max(0,(bp)-(1-1)))];
  h = h*5 + H3_spc_h_pl[__min(4,__max(0,(pl)-(1-1)))];
  h = h*10 + H3_spc_h_zr[__min(13,__max(0,(zr)-(1-1)))];

  m = 0;
  m = m*12 + H3_spc_m_cb[__min(11,__max(0,(cb)-(1-1)))];
  m = m*2 + (((ws)>0+(1-1)));
  m = m*7 + H3_spc_m_av[__min(28,__max(0,(av)-(1-1)))];

  t = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_spc
#define FC_PFX H3_spc_
#define FC_FAM spc
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
