/*  The template IDX/idx2inc.pl reads alongside qpd.idx: the qpd
    family's index builder and the tables its indices address.  Copied
    through into MOD/l9/qpd_p.inc with `H9_qpd_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l9/qpd_h.inc.  Edit this file
    and qpd.idx, not MOD/; ../../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_qpd_ND (or H_qpd_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_qpd_make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int lnz = D.lnz, pc = D.pc, cc = D.cc, cls = D.cls, dc = D.dc, db = D.db, lz = D.lz, st = D.st, ae = D.ae, aq = D.aq, ac = D.ac, td = D.td, sq = D.sq, xe = D.xe, lq = D.lq, bc = D.bc, cl = D.cl, kc = D.kc;
  (void) lnz;  (void) pc;  (void) cc;  (void) cls;  (void) dc;  (void) db;  (void) lz;  (void) st;  (void) ae;  (void) aq;  (void) ac;  (void) td;  (void) sq;  (void) xe;  (void) lq;  (void) bc;  (void) cl;  (void) kc;

  a = 0;
  a = a*3 + (((pc)>0+(1-1))+((pc)>1+(1-1)));
  a = a*3 + (((cc)>0+(1-1))+((cc)>1+(1-1)));
  a = a*2 + (((cls)>5+(1-1)));
  a = a*9 + H9_qpd_a_sq[__min(13,__max(0,(sq)-(1-1)))];

  b = 0;
  b = b*3 + (((aq)>0+(1-1))+((aq)>12+(1-1)));
  b = b*7 + H9_qpd_b_ae[__min(13,__max(0,(ae)-(1-1)))];
  b = b*10 + H9_qpd_b_lq[__min(13,__max(0,(lq)-(1-1)))];

  c = 0;
  c = c*4 + H9_qpd_c_lz[__min(6,__max(0,(lz)-(1-1)))];
  c = c*3 + (((st)>1+(1-1))+((st)>3+(1-1)));
  c = c*6 + H9_qpd_c_db[__min(8,__max(0,(db)-(1-1)))];
  c = c*9 + H9_qpd_c_bc[__min(13,__max(0,(bc)-(1-1)))];

  d = 0;
  d = d*4 + H9_qpd_d_st[__min(9,__max(0,(st)-(1-1)))];
  d = d*7 + H9_qpd_d_aq[__min(13,__max(0,(aq)-(1-1)))];
  d = d*6 + H9_qpd_d_ac[__min(13,__max(0,(ac)-(1-1)))];

  e = 0;
  e = e*7 + H9_qpd_e_aq[__min(13,__max(0,(aq)-(1-1)))];
  e = e*3 + (((cls)>4+(1-1))+((cls)>6+(1-1)));
  e = e*4 + H9_qpd_e_sq[__min(13,__max(0,(sq)-(1-1)))];
  e = e*9 + H9_qpd_e_xe[__min(18,__max(0,(xe)-(1-1)))];

  h = 0;
  h = h*7 + H9_qpd_h_ae[__min(13,__max(0,(ae)-(1-1)))];
  h = h*7 + H9_qpd_h_dc[__min(8,__max(0,(dc)-(1-1)))];
  h = h*3 + (((pc)>0+(1-1))+((pc)>3+(1-1)));

  i = 0;
  i = i*8 + H9_qpd_i_aq[__min(13,__max(0,(aq)-(1-1)))];
  i = i*5 + H9_qpd_i_ac[__min(13,__max(0,(ac)-(1-1)))];
  i = i*4 + H9_qpd_i_cls[__min(8,__max(0,(cls)-(1-1)))];
  i = i*6 + H9_qpd_i_td[__min(16,__max(0,(td)-(1-1)))];

  j = 0;
  j = j*3 + (((st)>1+(1-1))+((st)>2+(1-1)));
  j = j*5 + H9_qpd_j_dc[__min(8,__max(0,(dc)-(1-1)))];
  j = j*3 + (((db)>0+(1-1))+((db)>6+(1-1)));
  j = j*4 + H9_qpd_j_td[__min(16,__max(0,(td)-(1-1)))];
  j = j*8 + H9_qpd_j_lq[__min(13,__max(0,(lq)-(1-1)))];

  k = 0;
  k = k*6 + H9_qpd_k_aq[__min(13,__max(0,(aq)-(1-1)))];
  k = k*3 + (((pc)>0+(1-1))+((pc)>1+(1-1)));
  k = k*10 + H9_qpd_k_kc[__min(13,__max(0,(kc)-(1-1)))];

  l = 0;
  l = l*3 + (((ac)>0+(1-1))+((ac)>5+(1-1)));
  l = l*6 + H9_qpd_l_db[__min(8,__max(0,(db)-(1-1)))];
  l = l*2 + (((st)>1+(1-1)));
  l = l*13 + H9_qpd_l_cl[__min(13,__max(0,(cl)-(1-1)))];

  s = 0;
  s = s*2 + (((lnz)>0+(1-1)));
  s = s*4 + H9_qpd_s_aq[__min(13,__max(0,(aq)-(1-1)))];

  f = 0;
  f = f*4 + H9_qpd_f_st[__min(9,__max(0,(st)-(1-1)))];

  m = 0;

  w = 0;
  w = w*3 + (((st)>1+(1-1))+((st)>4+(1-1)));
  w = w*6 + H9_qpd_w_aq[__min(13,__max(0,(aq)-(1-1)))];
  w = w*3 + (((ac)>0+(1-1))+((ac)>9+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

