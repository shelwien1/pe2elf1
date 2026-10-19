/*  The template IDX/idx2inc.pl reads alongside cbf.idx: the cbf
    family's index builder and the tables its indices address.  Copied
    through into MOD/l3/cbf_p.inc with `H3_cbf_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l3/cbf_h.inc.  Edit this file
    and cbf.idx, not MOD/; ../../mk.sh regenerates both.

    Level -3: `_A`-`_E`, `_H`-`_L` are the ten counters, `_W`, `_W2` and `_W3`
    the three mixers (selected by indices m, w and s) and `_WF` the one that
    mixes them (by node and the state of code()'s hint), `_MT` and `_WM`
    the mantissa plane and its mixer, `_G` the sign counters.  Each is H_cbf_ND (or H_cbf_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H3_cbf_make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, m, w, t, n, g;
  int cat = D.cat, na = D.na, nb = D.nb, intra = D.intra, q6 = D.q6, np = D.np, pos = D.pos, crcb = D.crcb, col = D.col, nc = D.nc, kn = D.kn, md = D.md, en = D.en, pg = D.pg, em = D.em, pz = D.pz, tn = D.tn, pq = D.pq, ch = D.ch, ds = D.ds, hv = D.hv, ly = D.ly, dq = D.dq;
  (void) cat;  (void) na;  (void) nb;  (void) intra;  (void) q6;  (void) np;  (void) pos;  (void) crcb;  (void) col;  (void) nc;  (void) kn;  (void) md;  (void) en;  (void) pg;  (void) em;  (void) pz;  (void) tn;  (void) pq;  (void) ch;  (void) ds;  (void) hv;  (void) ly;  (void) dq;

  a = 0;
  a = a*5 + H3_cbf_a_cat[__min(13,__max(0,cat-(1-1)))];
  a = a*6 + H3_cbf_a_na[__min(9,__max(0,na-(1-1)))];
  a = a*5 + H3_cbf_a_nb[__min(9,__max(0,nb-(1-1)))];
  a = a*2 + ((intra>0+(1-1)));
  a = a*5 + H3_cbf_a_ly[__min(10,__max(0,ly-(1-1)))];

  b = 0;
  b = b*3 + ((cat>0+(1-1))+(cat>2+(1-1)));
  b = b*4 + H3_cbf_b_np[__min(3,__max(0,np-(1-1)))];
  b = b*3 + ((pos>1+(1-1))+(pos>2+(1-1)));
  b = b*3 + ((crcb>0+(1-1))+(crcb>1+(1-1)));
  b = b*2 + ((kn>0+(1-1)));
  b = b*5 + H3_cbf_b_dq[__min(8,__max(0,dq-(1-1)))];

  c = 0;
  c = c*5 + H3_cbf_c_cat[__min(13,__max(0,cat-(1-1)))];
  c = c*6 + H3_cbf_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*2 + ((kn>0+(1-1)));
  c = c*14 + H3_cbf_c_ch[__min(15,__max(0,ch-(1-1)))];

  d = 0;
  d = d*5 + H3_cbf_d_cat[__min(13,__max(0,cat-(1-1)))];
  d = d*4 + H3_cbf_d_nc[__min(5,__max(0,nc-(1-1)))];
  d = d*13 + H3_cbf_d_md[__min(19,__max(0,md-(1-1)))];

  e = 0;
  e = e*2 + ((na>7+(1-1)));
  e = e*7 + H3_cbf_e_nb[__min(9,__max(0,nb-(1-1)))];

  h = 0;
  h = h*6 + H3_cbf_h_cat[__min(13,__max(0,cat-(1-1)))];
  h = h*12 + H3_cbf_h_md[__min(19,__max(0,md-(1-1)))];
  h = h*6 + H3_cbf_h_q6[__min(8,__max(0,q6-(1-1)))];

  i = 0;
  i = i*5 + H3_cbf_i_cat[__min(13,__max(0,cat-(1-1)))];
  i = i*2 + ((np>0+(1-1)));
  i = i*6 + H3_cbf_i_ds[__min(7,__max(0,ds-(1-1)))];

  j = 0;
  j = j*6 + H3_cbf_j_na[__min(9,__max(0,na-(1-1)))];

  k = 0;
  k = k*5 + H3_cbf_k_cat[__min(13,__max(0,cat-(1-1)))];

  l = 0;
  l = l*3 + ((cat>0+(1-1))+(cat>4+(1-1)));
  l = l*3 + ((col>0+(1-1))+(col>1+(1-1)));
  l = l*2 + ((md>9+(1-1)));

  s = 0;

  m = 0;
  m = m*5 + H3_cbf_m_cat[__min(13,__max(0,cat-(1-1)))];
  m = m*2 + ((kn>0+(1-1)));

  w = 0;
  w = w*5 + H3_cbf_w_cat[__min(13,__max(0,cat-(1-1)))];
  w = w*5 + H3_cbf_w_nc[__min(5,__max(0,nc-(1-1)))];
  w = w*8 + H3_cbf_w_md[__min(19,__max(0,md-(1-1)))];
  w = w*2 + ((kn>0+(1-1)));

  t = 0;
  t = t*6 + H3_cbf_t_cat[__min(13,__max(0,cat-(1-1)))];

  n = 0;
  n = n*2 + ((cat>3+(1-1)));

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.c[4] = e;  X.c[5] = h;  X.c[6] = i;  X.c[7] = j;  X.c[8] = k;  X.c[9] = l;  X.s = s;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

