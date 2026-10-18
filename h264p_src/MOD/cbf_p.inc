/*  The template IDX/idx2inc.pl reads alongside cbf.idx: the cbf
    family's index builder and the tables its indices address.  Copied
    through into MOD/cbf_p.inc with `H_cbf_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/cbf_h.inc.  Edit this file and
    cbf.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H`-`_L` are the ten counters, `_S` and `_F` the two APMs,
    `_W`, `_W2` and `_W3` the three mixers (selected by indices m, w and s)
    and `_WF` the one that mixes them (by node and the state of code()'s
    hint), `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_cbf_ND (or H_cbf_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_cbf_make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, e, h, i, j, k, l, s, f, m, w, t, n, g;
  int cat = D.cat, na = D.na, nb = D.nb, intra = D.intra, q6 = D.q6, np = D.np, pos = D.pos, crcb = D.crcb, col = D.col, nc = D.nc, kn = D.kn, md = D.md, en = D.en, pg = D.pg, em = D.em, pz = D.pz, tn = D.tn, pq = D.pq, ch = D.ch, ds = D.ds, hv = D.hv;
  (void) cat;  (void) na;  (void) nb;  (void) intra;  (void) q6;  (void) np;  (void) pos;  (void) crcb;  (void) col;  (void) nc;  (void) kn;  (void) md;  (void) en;  (void) pg;  (void) em;  (void) pz;  (void) tn;  (void) pq;  (void) ch;  (void) ds;  (void) hv;

  a = 0;
  a = a*14 + H_cbf_a_cat[__min(13,__max(0,cat-(1-1)))];
  a = a*6 + H_cbf_a_na[__min(9,__max(0,na-(1-1)))];
  a = a*10 + H_cbf_a_nb[__min(9,__max(0,nb-(1-1)))];
  a = a*2 + ((intra>0+(1-1)));

  b = 0;
  b = b*14 + H_cbf_b_cat[__min(13,__max(0,cat-(1-1)))];
  b = b*4 + H_cbf_b_np[__min(3,__max(0,np-(1-1)))];
  b = b*4 + H_cbf_b_pos[__min(3,__max(0,pos-(1-1)))];
  b = b*3 + ((crcb>0+(1-1))+(crcb>1+(1-1)));
  b = b*2 + ((kn>0+(1-1)));
  b = b*17 + H_cbf_b_em[__min(16,__max(0,em-(1-1)))];

  c = 0;
  c = c*2 + ((cat>0+(1-1)));
  c = c*9 + H_cbf_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*2 + ((kn>0+(1-1)));
  c = c*15 + H_cbf_c_en[__min(14,__max(0,en-(1-1)))];
  c = c*16 + H_cbf_c_ch[__min(15,__max(0,ch-(1-1)))];

  d = 0;
  d = d*14 + H_cbf_d_cat[__min(13,__max(0,cat-(1-1)))];
  d = d*6 + H_cbf_d_nc[__min(5,__max(0,nc-(1-1)))];
  d = d*5 + H_cbf_d_md[__min(19,__max(0,md-(1-1)))];
  d = d*2 + ((kn>0+(1-1)));
  d = d*17 + H_cbf_d_pg[__min(16,__max(0,pg-(1-1)))];

  e = 0;
  e = e*14 + H_cbf_e_cat[__min(13,__max(0,cat-(1-1)))];
  e = e*5 + H_cbf_e_pz[__min(16,__max(0,pz-(1-1)))];
  e = e*10 + H_cbf_e_na[__min(9,__max(0,na-(1-1)))];
  e = e*10 + H_cbf_e_nb[__min(9,__max(0,nb-(1-1)))];
  e = e*11 + H_cbf_e_tn[__min(10,__max(0,tn-(1-1)))];

  h = 0;
  h = h*14 + H_cbf_h_cat[__min(13,__max(0,cat-(1-1)))];
  h = h*17 + H_cbf_h_pg[__min(16,__max(0,pg-(1-1)))];
  h = h*10 + H_cbf_h_md[__min(19,__max(0,md-(1-1)))];
  h = h*9 + H_cbf_h_q6[__min(8,__max(0,q6-(1-1)))];
  h = h*8 + H_cbf_h_hv[__min(7,__max(0,hv-(1-1)))];

  i = 0;
  i = i*14 + H_cbf_i_cat[__min(13,__max(0,cat-(1-1)))];
  i = i*17 + H_cbf_i_pz[__min(16,__max(0,pz-(1-1)))];
  i = i*5 + H_cbf_i_pq[__min(24,__max(0,pq-(1-1)))];
  i = i*4 + H_cbf_i_np[__min(3,__max(0,np-(1-1)))];
  i = i*8 + H_cbf_i_ds[__min(7,__max(0,ds-(1-1)))];

  j = 0;
  j = j*14 + H_cbf_j_cat[__min(13,__max(0,cat-(1-1)))];
  j = j*17 + H_cbf_j_em[__min(16,__max(0,em-(1-1)))];
  j = j*17 + H_cbf_j_pg[__min(16,__max(0,pg-(1-1)))];
  j = j*10 + H_cbf_j_na[__min(9,__max(0,na-(1-1)))];

  k = 0;
  k = k*14 + H_cbf_k_cat[__min(13,__max(0,cat-(1-1)))];
  k = k*10 + H_cbf_k_na[__min(9,__max(0,na-(1-1)))];
  k = k*10 + H_cbf_k_nb[__min(9,__max(0,nb-(1-1)))];
  k = k*17 + H_cbf_k_pz[__min(16,__max(0,pz-(1-1)))];

  l = 0;
  l = l*14 + H_cbf_l_cat[__min(13,__max(0,cat-(1-1)))];
  l = l*2 + ((col>1+(1-1)));
  l = l*20 + H_cbf_l_md[__min(19,__max(0,md-(1-1)))];

  s = 0;
  s = s*14 + H_cbf_s_cat[__min(13,__max(0,cat-(1-1)))];
  s = s*2 + ((kn>0+(1-1)));
  s = s*8 + H_cbf_s_pq[__min(24,__max(0,pq-(1-1)))];

  f = 0;
  f = f*14 + H_cbf_f_cat[__min(13,__max(0,cat-(1-1)))];
  f = f*2 + ((intra>0+(1-1)));

  m = 0;
  m = m*5 + H_cbf_m_cat[__min(13,__max(0,cat-(1-1)))];
  m = m*2 + ((kn>0+(1-1)));

  w = 0;
  w = w*14 + H_cbf_w_cat[__min(13,__max(0,cat-(1-1)))];
  w = w*5 + H_cbf_w_nc[__min(5,__max(0,nc-(1-1)))];
  w = w*5 + H_cbf_w_md[__min(19,__max(0,md-(1-1)))];
  w = w*2 + ((kn>0+(1-1)));

  t = 0;
  t = t*14 + H_cbf_t_cat[__min(13,__max(0,cat-(1-1)))];

  n = 0;
  n = n*14 + H_cbf_n_cat[__min(13,__max(0,cat-(1-1)))];

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.i = i;  X.j = j;  X.k = k;  X.l = l;  X.s = s;  X.f = f;  X.m = m;  X.w = w;  X.t = t;
  X.n = n;  X.g = g;
}

