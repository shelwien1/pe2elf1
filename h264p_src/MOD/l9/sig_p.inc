/*  The template IDX/idx2inc.pl reads alongside sig.idx: the sig
    family's index builder and the tables its indices address.  Copied
    through into MOD/sig_p.inc with `H9_sig_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/sig_h.inc.  Edit this file and
    sig.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_sig_ND (or H_sig_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H9_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int cg = D.cg, pos = D.pos, rem = D.rem, zl = D.zl, md = D.md, nc = D.nc, fld = D.fld, q6 = D.q6, pg = D.pg, em = D.em, pe = D.pe, ps = D.ps, tz = D.tz, pq = D.pq, hb = D.hb, ft = D.ft, h3 = D.h3, pr = D.pr, sn = D.sn, hv = D.hv, pf = D.pf;
  (void) cg;  (void) pos;  (void) rem;  (void) zl;  (void) md;  (void) nc;  (void) fld;  (void) q6;  (void) pg;  (void) em;  (void) pe;  (void) ps;  (void) tz;  (void) pq;  (void) hb;  (void) ft;  (void) h3;  (void) pr;  (void) sn;  (void) hv;  (void) pf;

  a = 0;
  a = a*14 + H9_sig_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*64 + H9_sig_a_pos[__min(63,__max(0,pos-(1-1)))];
  a = a*7 + H9_sig_a_rem[__min(10,__max(0,rem-(1-1)))];

  b = 0;
  b = b*14 + H9_sig_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*39 + H9_sig_b_pos[__min(63,__max(0,pos-(1-1)))];
  b = b*16 + H9_sig_b_md[__min(19,__max(0,md-(1-1)))];
  b = b*511 + H9_sig_b_hb[__min(510,__max(0,hb-(1-1)))];

  c = 0;
  c = c*5 + H9_sig_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*64 + H9_sig_c_pos[__min(63,__max(0,pos-(1-1)))];
  c = c*9 + H9_sig_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*17 + H9_sig_c_pg[__min(16,__max(0,pg-(1-1)))];
  c = c*9 + H9_sig_c_ft[__min(8,__max(0,ft-(1-1)))];

  d = 0;
  d = d*4 + H9_sig_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*6 + H9_sig_d_rem[__min(10,__max(0,rem-(1-1)))];
  d = d*6 + H9_sig_d_zl[__min(10,__max(0,zl-(1-1)))];
  d = d*17 + H9_sig_d_em[__min(16,__max(0,em-(1-1)))];
  d = d*511 + H9_sig_d_hb[__min(510,__max(0,hb-(1-1)))];

  e = 0;
  e = e*14 + H9_sig_e_cg[__min(13,__max(0,cg-(1-1)))];
  e = e*64 + H9_sig_e_pos[__min(63,__max(0,pos-(1-1)))];
  e = e*8 + H9_sig_e_ps[__min(7,__max(0,ps-(1-1)))];
  e = e*9 + H9_sig_e_tz[__min(8,__max(0,tz-(1-1)))];
  e = e*25 + H9_sig_e_pq[__min(24,__max(0,pq-(1-1)))];
  e = e*3 + ((pr>3+(1-1))+(pr>6+(1-1)));

  h = 0;
  h = h*5 + H9_sig_h_cg[__min(13,__max(0,cg-(1-1)))];
  h = h*64 + H9_sig_h_pos[__min(63,__max(0,pos-(1-1)))];
  h = h*5 + H9_sig_h_ps[__min(7,__max(0,ps-(1-1)))];
  h = h*9 + H9_sig_h_rem[__min(10,__max(0,rem-(1-1)))];
  h = h*9 + H9_sig_h_tz[__min(8,__max(0,tz-(1-1)))];
  h = h*15 + H9_sig_h_h3[__min(14,__max(0,h3-(1-1)))];

  i = 0;
  i = i*14 + H9_sig_i_cg[__min(13,__max(0,cg-(1-1)))];
  i = i*64 + H9_sig_i_pos[__min(63,__max(0,pos-(1-1)))];
  i = i*8 + H9_sig_i_pr[__min(7,__max(0,pr-(1-1)))];
  i = i*3 + ((ps>2+(1-1))+(ps>3+(1-1)));
  i = i*8 + H9_sig_i_hv[__min(7,__max(0,hv-(1-1)))];

  j = 0;
  j = j*14 + H9_sig_j_cg[__min(13,__max(0,cg-(1-1)))];
  j = j*28 + H9_sig_j_pos[__min(63,__max(0,pos-(1-1)))];
  j = j*9 + H9_sig_j_ft[__min(8,__max(0,ft-(1-1)))];
  j = j*3 + ((h3>0+(1-1))+(h3>7+(1-1)));

  k = 0;
  k = k*14 + H9_sig_k_cg[__min(13,__max(0,cg-(1-1)))];
  k = k*64 + H9_sig_k_pos[__min(63,__max(0,pos-(1-1)))];
  k = k*11 + H9_sig_k_rem[__min(10,__max(0,rem-(1-1)))];
  k = k*15 + H9_sig_k_h3[__min(14,__max(0,h3-(1-1)))];
  k = k*6 + H9_sig_k_hv[__min(7,__max(0,hv-(1-1)))];

  l = 0;
  l = l*14 + H9_sig_l_cg[__min(13,__max(0,cg-(1-1)))];
  l = l*64 + H9_sig_l_pos[__min(63,__max(0,pos-(1-1)))];
  l = l*16 + H9_sig_l_pe[__min(15,__max(0,pe-(1-1)))];
  l = l*8 + H9_sig_l_pr[__min(7,__max(0,pr-(1-1)))];
  l = l*9 + H9_sig_l_sn[__min(8,__max(0,sn-(1-1)))];
  l = l*12 + H9_sig_l_pf[__min(11,__max(0,pf-(1-1)))];

  s = 0;
  s = s*5 + H9_sig_s_pe[__min(15,__max(0,pe-(1-1)))];
  s = s*8 + H9_sig_s_pr[__min(7,__max(0,pr-(1-1)))];

  f = 0;
  f = f*3 + ((cg>0+(1-1))+(cg>1+(1-1)));
  f = f*6 + H9_sig_f_nc[__min(5,__max(0,nc-(1-1)))];

  m = 0;
  m = m*14 + H9_sig_m_cg[__min(13,__max(0,cg-(1-1)))];
  m = m*4 + H9_sig_m_pos[__min(9,__max(0,pos-(1-1)))];
  m = m*4 + H9_sig_m_rem[__min(10,__max(0,rem-(1-1)))];

  w = 0;
  w = w*4 + H9_sig_w_cg[__min(13,__max(0,cg-(1-1)))];
  w = w*6 + H9_sig_w_rem[__min(10,__max(0,rem-(1-1)))];
  w = w*7 + H9_sig_w_zl[__min(10,__max(0,zl-(1-1)))];
  w = w*17 + H9_sig_w_em[__min(16,__max(0,em-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

