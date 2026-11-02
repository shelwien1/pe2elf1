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
    counters.  Each is H_sgn_ND (or H_sgn_MND) rows deep per context:
    where a decision sits among the family's nodes is context without
    being a variable.  */

void H3_sgn_make( const H_sgn_v & D, tcx & X ) {
  int a, b, c, d, e, h, m, t, g;
  H_sgn_VARS(H_LOCAL)

  a = 0;
  a = a*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  a = a*3 + (((s2)>0+(1-1))+((s2)>1+(1-1)));
  a = a*12 + H3_sgn_a_cb[__min(11,__max(0,(cb)-(1-1)))];

  b = 0;
  b = b*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  b = b*5 + H3_sgn_b_mg[__min(4,__max(0,(mg)-(1-1)))];

  c = 0;
  c = c*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  c = c*8 + H3_sgn_c_mg[__min(7,__max(0,(mg)-(1-1)))];

  d = 0;
  d = d*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  d = d*3 + (((s1)>0+(1-1))+((s1)>1+(1-1)));
  d = d*3 + (((pf)>0+(1-1))+((pf)>1+(1-1)));

  e = 0;
  e = e*12 + H3_sgn_e_cb[__min(11,__max(0,(cb)-(1-1)))];
  e = e*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  e = e*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));

  h = 0;
  h = h*3 + (((oc)>0+(1-1))+((oc)>1+(1-1)));
  h = h*3 + (((ms)>0+(1-1))+((ms)>1+(1-1)));
  h = h*3 + (((pf)>0+(1-1))+((pf)>1+(1-1)));

  m = 0;
  m = m*12 + H3_sgn_m_cb[__min(11,__max(0,(cb)-(1-1)))];

  t = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.m = m;  X.t = t;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C3_sgn
#define FC_PFX H3_sgn_
#define FC_FAM sgn
#define FC_NC 6
#define FC_NMIX 1
#define FC_NAPM 0
#include "../../famcfg.inc"
