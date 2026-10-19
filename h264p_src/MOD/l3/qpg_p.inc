/*  The template IDX/idx2inc.pl reads alongside qpg.idx: the qpg
    family's index builder and the tables its indices address.  Copied
    through into MOD/l3/qpg_p.inc with `H3_qpg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l3/qpg_h.inc.  Edit this file
    and qpg.idx, not MOD/; ../../mk.sh regenerates both.

    Level -3: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_qpg_ND (or H_qpg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H3_qpg_make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int as = D.as, cs = D.cs, bs = D.bs, ls = D.ls, rs = D.rs, mag = D.mag, cls = D.cls, st = D.st, ss = D.ss, xe = D.xe, lq = D.lq, bk = D.bk;
  (void) as;  (void) cs;  (void) bs;  (void) ls;  (void) rs;  (void) mag;  (void) cls;  (void) st;  (void) ss;  (void) xe;  (void) lq;  (void) bk;

  a = 0;
  a = a*8 + H3_qpg_a_mag[__min(7,__max(0,mag-(1-1)))];
  a = a*2 + ((ss>5+(1-1)));

  b = 0;

  c = 0;
  c = c*10 + H3_qpg_c_cs[__min(9,__max(0,cs-(1-1)))];

  d = 0;
  d = d*10 + H3_qpg_d_bs[__min(9,__max(0,bs-(1-1)))];
  d = d*8 + H3_qpg_d_mag[__min(7,__max(0,mag-(1-1)))];
  d = d*10 + H3_qpg_d_bk[__min(9,__max(0,bk-(1-1)))];

  e = 0;
  e = e*3 + ((cs>5+(1-1))+(cs>8+(1-1)));
  e = e*10 + H3_qpg_e_ss[__min(9,__max(0,ss-(1-1)))];

  h = 0;
  h = h*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  i = 0;
  i = i*10 + H3_qpg_i_cs[__min(9,__max(0,cs-(1-1)))];
  i = i*10 + H3_qpg_i_bs[__min(9,__max(0,bs-(1-1)))];

  j = 0;
  j = j*9 + H3_qpg_j_cls[__min(8,__max(0,cls-(1-1)))];
  j = j*10 + H3_qpg_j_st[__min(9,__max(0,st-(1-1)))];
  j = j*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  k = 0;
  k = k*8 + H3_qpg_k_mag[__min(7,__max(0,mag-(1-1)))];
  k = k*9 + H3_qpg_k_cls[__min(8,__max(0,cls-(1-1)))];

  l = 0;
  l = l*10 + H3_qpg_l_cs[__min(9,__max(0,cs-(1-1)))];
  l = l*10 + H3_qpg_l_bs[__min(9,__max(0,bs-(1-1)))];
  l = l*2 + ((ls>0+(1-1)));

  s = 0;
  s = s*8 + H3_qpg_s_mag[__min(7,__max(0,mag-(1-1)))];

  m = 0;

  w = 0;
  w = w*10 + H3_qpg_w_cs[__min(9,__max(0,cs-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

