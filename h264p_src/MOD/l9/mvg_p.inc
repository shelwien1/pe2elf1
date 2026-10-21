/*  The template IDX/idx2inc.pl reads alongside each .idx of IDX/l9: a
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
    the sign counters.  Each is H_mvg_ND (or H_mvg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_mvg_make( const H_mvg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  H_mvg_VARS(H_LOCAL)

  a = 0;
  a = a*2 + (((comp)>0+(1-1)));
  a = a*6 + H9_mvg_a_ts[__min(9,__max(0,(ts)-(1-1)))];
  a = a*4 + H9_mvg_a_mag[__min(7,__max(0,(mag)-(1-1)))];

  b = 0;
  b = b*7 + H9_mvg_b_zs[__min(8,__max(0,(zs)-(1-1)))];
  b = b*4 + H9_mvg_b_mag[__min(7,__max(0,(mag)-(1-1)))];
  b = b*2 + (((comp)>0+(1-1)));

  c = 0;
  c = c*6 + H9_mvg_c_ts[__min(9,__max(0,(ts)-(1-1)))];
  c = c*6 + H9_mvg_c_zs[__min(8,__max(0,(zs)-(1-1)))];
  c = c*8 + H9_mvg_c_as[__min(9,__max(0,(as)-(1-1)))];

  d = 0;
  d = d*8 + H9_mvg_d_ds[__min(9,__max(0,(ds)-(1-1)))];
  d = d*3 + (((mag)>3+(1-1))+((mag)>6+(1-1)));

  e = 0;
  e = e*3 + (((sa)>0+(1-1))+((sa)>1+(1-1)));
  e = e*3 + (((sb)>0+(1-1))+((sb)>1+(1-1)));
  e = e*2 + (((comp)>0+(1-1)));
  e = e*5 + H9_mvg_e_ns[__min(6,__max(0,(ns)-(1-1)))];

  h = 0;
  h = h*7 + H9_mvg_h_ts[__min(9,__max(0,(ts)-(1-1)))];
  h = h*9 + H9_mvg_h_tq[__min(12,__max(0,(tq)-(1-1)))];
  h = h*4 + H9_mvg_h_es[__min(3,__max(0,(es)-(1-1)))];

  i = 0;
  i = i*4 + H9_mvg_i_ts[__min(9,__max(0,(ts)-(1-1)))];
  i = i*4 + H9_mvg_i_ds[__min(9,__max(0,(ds)-(1-1)))];

  j = 0;
  j = j*2 + (((lst)>0+(1-1)));
  j = j*4 + H9_mvg_j_ox[__min(3,__max(0,(ox)-(1-1)))];
  j = j*5 + H9_mvg_j_ns[__min(6,__max(0,(ns)-(1-1)))];

  k = 0;
  k = k*3 + (((mag)>1+(1-1))+((mag)>4+(1-1)));
  k = k*7 + H9_mvg_k_ts[__min(9,__max(0,(ts)-(1-1)))];
  k = k*3 + (((sa)>0+(1-1))+((sa)>1+(1-1)));

  l = 0;
  l = l*7 + H9_mvg_l_zs[__min(8,__max(0,(zs)-(1-1)))];
  l = l*6 + H9_mvg_l_ds[__min(9,__max(0,(ds)-(1-1)))];

  s = 0;
  s = s*4 + H9_mvg_s_ts[__min(9,__max(0,(ts)-(1-1)))];
  s = s*4 + H9_mvg_s_mag[__min(7,__max(0,(mag)-(1-1)))];

  f = 0;
  f = f*2 + (((comp)>0+(1-1)));
  f = f*8 + H9_mvg_f_ts[__min(9,__max(0,(ts)-(1-1)))];

  m = 0;

  w = 0;
  w = w*7 + H9_mvg_w_zs[__min(8,__max(0,(zs)-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}


// the family's configuration class (famcfg.inc): the level's shape, as
// the tables above have it
#define FC_NAME C9_mvg
#define FC_PFX H9_mvg_
#define FC_FAM mvg
#define FC_NC 10
#define FC_NMIX 3
#define FC_NAPM 2
#include "../../famcfg.inc"
