/*  The template IDX/idx2inc.pl reads alongside cbf.idx: the cbf
    family's index builder and the tables its indices address.  Copied
    through into MOD/cbf_p.inc with `H_cbf_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/cbf_h.inc.  Edit this file and
    cbf.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` the
    mixer, `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_cbf_ND (or H_cbf_MND) rows deep per context: where a
    decision sits among the family's nodes is context without being a
    variable.  */

void H_cbf_make( const H_cbf_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int cat = D.cat, na = D.na, nb = D.nb, intra = D.intra, q6 = D.q6, np = D.np, pos = D.pos, crcb = D.crcb, col = D.col, nc = D.nc, kn = D.kn, md = D.md;
  (void) cat;  (void) na;  (void) nb;  (void) intra;  (void) q6;  (void) np;  (void) pos;  (void) crcb;  (void) col;  (void) nc;  (void) kn;  (void) md;

  a = 0;
  a = a*14 + H_cbf_a_cat[__min(13,__max(0,cat-(1-1)))];
  a = a*10 + H_cbf_a_na[__min(9,__max(0,na-(1-1)))];
  a = a*10 + H_cbf_a_nb[__min(9,__max(0,nb-(1-1)))];
  a = a*2 + ((intra>0+(1-1)));

  b = 0;
  b = b*14 + H_cbf_b_cat[__min(13,__max(0,cat-(1-1)))];
  b = b*4 + H_cbf_b_np[__min(3,__max(0,np-(1-1)))];
  b = b*4 + H_cbf_b_pos[__min(3,__max(0,pos-(1-1)))];
  b = b*3 + ((crcb>0+(1-1))+(crcb>1+(1-1)));
  b = b*2 + ((kn>0+(1-1)));

  c = 0;
  c = c*14 + H_cbf_c_cat[__min(13,__max(0,cat-(1-1)))];
  c = c*9 + H_cbf_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*3 + ((col>0+(1-1))+(col>1+(1-1)));
  c = c*2 + ((kn>0+(1-1)));

  d = 0;
  d = d*14 + H_cbf_d_cat[__min(13,__max(0,cat-(1-1)))];
  d = d*6 + H_cbf_d_nc[__min(5,__max(0,nc-(1-1)))];
  d = d*20 + H_cbf_d_md[__min(19,__max(0,md-(1-1)))];
  d = d*2 + ((kn>0+(1-1)));

  s = 0;
  s = s*14 + H_cbf_s_cat[__min(13,__max(0,cat-(1-1)))];
  s = s*2 + ((kn>0+(1-1)));

  f = 0;
  f = f*14 + H_cbf_f_cat[__min(13,__max(0,cat-(1-1)))];
  f = f*2 + ((intra>0+(1-1)));

  m = 0;
  m = m*14 + H_cbf_m_cat[__min(13,__max(0,cat-(1-1)))];
  m = m*2 + ((kn>0+(1-1)));

  t = 0;
  t = t*14 + H_cbf_t_cat[__min(13,__max(0,cat-(1-1)))];

  n = 0;
  n = n*14 + H_cbf_n_cat[__min(13,__max(0,cat-(1-1)))];

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

