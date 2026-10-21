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
    mixer, `_G` the sign counters.  Each is H_cbf_ND (or H_cbf_MND) rows
    deep per context: where a decision sits among the family's nodes is
    context without being a variable.  */

void H1_cbf_make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  H_cbf_VARS(H_LOCAL)

  a = 0;
  a = a*6 + H1_cbf_a_cat[__min(13,__max(0,(cat)-(1-1)))];
  a = a*6 + H1_cbf_a_na[__min(9,__max(0,(na)-(1-1)))];
  a = a*6 + H1_cbf_a_nb[__min(9,__max(0,(nb)-(1-1)))];
  a = a*2 + (((intra)>0+(1-1)));
  a = a*5 + H1_cbf_a_ly[__min(10,__max(0,(ly)-(1-1)))];

  b = 0;
  b = b*6 + H1_cbf_b_cat[__min(13,__max(0,(cat)-(1-1)))];
  b = b*4 + H1_cbf_b_np[__min(3,__max(0,(np)-(1-1)))];
  b = b*3 + (((pos)>1+(1-1))+((pos)>2+(1-1)));
  b = b*3 + (((crcb)>0+(1-1))+((crcb)>1+(1-1)));
  b = b*4 + H1_cbf_b_dq[__min(8,__max(0,(dq)-(1-1)))];

  c = 0;
  c = c*5 + H1_cbf_c_cat[__min(13,__max(0,(cat)-(1-1)))];
  c = c*4 + H1_cbf_c_q6[__min(8,__max(0,(q6)-(1-1)))];
  c = c*3 + (((col)>0+(1-1))+((col)>1+(1-1)));
  c = c*8 + H1_cbf_c_ch[__min(15,__max(0,(ch)-(1-1)))];

  d = 0;
  d = d*5 + H1_cbf_d_cat[__min(13,__max(0,(cat)-(1-1)))];
  d = d*9 + H1_cbf_d_md[__min(19,__max(0,(md)-(1-1)))];
  d = d*4 + H1_cbf_d_q6[__min(8,__max(0,(q6)-(1-1)))];

  m = 0;
  m = m*5 + H1_cbf_m_cat[__min(13,__max(0,(cat)-(1-1)))];
  m = m*2 + (((kn)>0+(1-1)));

  t = 0;
  t = t*6 + H1_cbf_t_cat[__min(13,__max(0,(cat)-(1-1)))];

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_cbf
#define FC_PFX H1_cbf_
#define FC_FAM cbf
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
