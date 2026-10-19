/*  The template IDX/idx2inc.pl reads alongside cbf.idx: the cbf
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/cbf_p.inc with `H1_cbf_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/cbf_h.inc.  Edit this file
    and cbf.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_cbf_ND (or H_cbf_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_cbf_make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int cat = D.cat, na = D.na, nb = D.nb, intra = D.intra, q6 = D.q6, np = D.np, pos = D.pos, crcb = D.crcb, col = D.col, nc = D.nc, kn = D.kn, md = D.md, en = D.en, pg = D.pg, em = D.em, pz = D.pz, tn = D.tn, pq = D.pq, ch = D.ch, ds = D.ds, hv = D.hv, ly = D.ly, dq = D.dq;
  (void) cat;  (void) na;  (void) nb;  (void) intra;  (void) q6;  (void) np;  (void) pos;  (void) crcb;  (void) col;  (void) nc;  (void) kn;  (void) md;  (void) en;  (void) pg;  (void) em;  (void) pz;  (void) tn;  (void) pq;  (void) ch;  (void) ds;  (void) hv;  (void) ly;  (void) dq;

  a = 0;
  a = a*14 + H1_cbf_a_cat[__min(13,__max(0,cat-(1-1)))];
  a = a*6 + H1_cbf_a_na[__min(9,__max(0,na-(1-1)))];
  a = a*10 + H1_cbf_a_nb[__min(9,__max(0,nb-(1-1)))];
  a = a*2 + ((intra>0+(1-1)));
  a = a*11 + H1_cbf_a_ly[__min(10,__max(0,ly-(1-1)))];

  b = 0;
  b = b*14 + H1_cbf_b_cat[__min(13,__max(0,cat-(1-1)))];
  b = b*4 + H1_cbf_b_np[__min(3,__max(0,np-(1-1)))];
  b = b*4 + H1_cbf_b_pos[__min(3,__max(0,pos-(1-1)))];
  b = b*3 + ((crcb>0+(1-1))+(crcb>1+(1-1)));
  b = b*2 + ((kn>0+(1-1)));
  b = b*17 + H1_cbf_b_em[__min(16,__max(0,em-(1-1)))];
  b = b*9 + H1_cbf_b_dq[__min(8,__max(0,dq-(1-1)))];

  c = 0;
  c = c*2 + ((cat>0+(1-1)));
  c = c*9 + H1_cbf_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*2 + ((kn>0+(1-1)));
  c = c*6 + H1_cbf_c_en[__min(14,__max(0,en-(1-1)))];
  c = c*16 + H1_cbf_c_ch[__min(15,__max(0,ch-(1-1)))];

  d = 0;
  d = d*14 + H1_cbf_d_cat[__min(13,__max(0,cat-(1-1)))];
  d = d*6 + H1_cbf_d_nc[__min(5,__max(0,nc-(1-1)))];
  d = d*5 + H1_cbf_d_md[__min(19,__max(0,md-(1-1)))];
  d = d*2 + ((kn>0+(1-1)));
  d = d*17 + H1_cbf_d_pg[__min(16,__max(0,pg-(1-1)))];

  m = 0;
  m = m*5 + H1_cbf_m_cat[__min(13,__max(0,cat-(1-1)))];
  m = m*2 + ((kn>0+(1-1)));

  t = 0;
  t = t*14 + H1_cbf_t_cat[__min(13,__max(0,cat-(1-1)))];

  n = 0;
  n = n*14 + H1_cbf_n_cat[__min(13,__max(0,cat-(1-1)))];

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

