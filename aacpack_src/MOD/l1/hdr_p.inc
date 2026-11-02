/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l1 (as h264p's): a
    family's index builder, the tables its indices address and its
    configuration class, at level -1: four counters, which keep their probabilities stretched (cm_cnt2s), and one mixer, no APMs.  Copied through into
    MOD/l1/<family>_p.inc with the family's name and prefix in place of
    their markers and `MakeIndex` by the statements that build the
    indices, and the Table() lines lifted out into the struct in
    MOD/l1/<family>_h.inc.  Edit this file and the .idx, not MOD/;
    ../../mk.sh regenerates both.

    `_A`-`_D` are the four counters (their probabilities kept stretched) and `_W` the one mixer (selected by
    index m, rows of 16 inputs), `_MT` the mantissa plane, `_G` the sign
    counters.  Each is H_hdr_ND (or H_hdr_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H1_hdr_make( const H_hdr_v & D, tcx & X ) {
  int a, b, c, d, m, t, g;
  H_hdr_VARS(H_LOCAL)

  a = 0;
  a = (a<<8) + (((tag)&255)); // 11111111
  a = a*8 + H1_hdr_a_fld[__min(7,__max(0,(fld)-(1-1)))];
  a = a*7 + H1_hdr_a_p1[__min(6,__max(0,(p1)-(1-1)))];
  a = a*61 + H1_hdr_a_dm[__min(60,__max(0,(dm)-(1-1)))];

  b = 0;
  b = (b<<8) + (((tag)&255)); // 11111111
  b = b*3 + (((ch)>0+(1-1))+((ch)>1+(1-1)));
  b = b*16 + H1_hdr_b_fld[__min(15,__max(0,(fld)-(1-1)))];
  b = b*21 + H1_hdr_b_hx[__min(20,__max(0,(hx)-(1-1)))];

  c = 0;
  c = (c<<8) + (((tag)&255)); // 11111111
  c = c*9 + H1_hdr_c_pb[__min(8,__max(0,(pb)-(1-1)))];
  c = (c<<3) + (((p1r)&7)); // 00000111

  d = 0;
  d = (d<<8) + (((tag)&255)); // 11111111
  d = d*5 + H1_hdr_d_ws[__min(4,__max(0,(ws)-(1-1)))];
  d = d*8 + H1_hdr_d_fld[__min(7,__max(0,(fld)-(1-1)))];
  d = d*8 + H1_hdr_d_ml[__min(7,__max(0,(ml)-(1-1)))];

  m = 0;
  m = (m<<8) + (((tag)&255)); // 11111111

  t = 0;
  t = (t<<8) + (((tag)&255)); // 11111111

  g = 0;
  g = g*7 + H1_hdr_g_p1[__min(6,__max(0,(p1)-(1-1)))];
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C1_hdr
#define FC_PFX H1_hdr_
#define FC_FAM hdr
#define FC_NC 4
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
