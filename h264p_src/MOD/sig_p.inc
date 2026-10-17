/*  The template IDX/idx2inc.pl reads alongside sig.idx: the sig
    family's index builder and the tables its indices address.  Copied
    through into MOD/sig_p.inc with `H_sig_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/sig_h.inc.  Edit this file and
    sig.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_E`, `_H` are the six counters, `_S` and `_F` the two APMs, `_W` and
    `_W2` the two mixers (selected by index m and by index d) and `_WF` the
    one that mixes them, `_MT` and `_WM` the mantissa plane and its mixer,
    `_G` the sign counters.  Each is H_sig_ND (or H_sig_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, e, h, s, f, m, t, n, g;
  int cg = D.cg, pos = D.pos, rem = D.rem, zl = D.zl, md = D.md, nc = D.nc, fld = D.fld, q6 = D.q6, pg = D.pg, em = D.em, pe = D.pe, ps = D.ps;
  (void) cg;  (void) pos;  (void) rem;  (void) zl;  (void) md;  (void) nc;  (void) fld;  (void) q6;  (void) pg;  (void) em;  (void) pe;  (void) ps;

  a = 0;
  a = a*14 + H_sig_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*64 + H_sig_a_pos[__min(63,__max(0,pos-(1-1)))];
  a = a*9 + H_sig_a_rem[__min(10,__max(0,rem-(1-1)))];

  b = 0;
  b = b*14 + H_sig_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*39 + H_sig_b_pos[__min(63,__max(0,pos-(1-1)))];
  b = b*16 + H_sig_b_md[__min(19,__max(0,md-(1-1)))];
  b = b*2 + ((fld>0+(1-1)));

  c = 0;
  c = c*5 + H_sig_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*64 + H_sig_c_pos[__min(63,__max(0,pos-(1-1)))];
  c = c*9 + H_sig_c_q6[__min(8,__max(0,q6-(1-1)))];
  c = c*17 + H_sig_c_pg[__min(16,__max(0,pg-(1-1)))];

  d = 0;
  d = d*4 + H_sig_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*6 + H_sig_d_rem[__min(10,__max(0,rem-(1-1)))];
  d = d*7 + H_sig_d_zl[__min(10,__max(0,zl-(1-1)))];
  d = d*17 + H_sig_d_em[__min(16,__max(0,em-(1-1)))];

  e = 0;
  e = e*14 + H_sig_e_cg[__min(13,__max(0,cg-(1-1)))];
  e = e*64 + H_sig_e_pos[__min(63,__max(0,pos-(1-1)))];
  e = e*4 + H_sig_e_ps[__min(3,__max(0,ps-(1-1)))];

  h = 0;
  h = h*5 + H_sig_h_cg[__min(13,__max(0,cg-(1-1)))];
  h = h*64 + H_sig_h_pos[__min(63,__max(0,pos-(1-1)))];
  h = h*4 + H_sig_h_ps[__min(3,__max(0,ps-(1-1)))];
  h = h*9 + H_sig_h_rem[__min(10,__max(0,rem-(1-1)))];

  s = 0;
  s = s*14 + H_sig_s_cg[__min(13,__max(0,cg-(1-1)))];
  s = s*5 + H_sig_s_pe[__min(15,__max(0,pe-(1-1)))];

  f = 0;
  f = f*3 + ((cg>0+(1-1))+(cg>1+(1-1)));
  f = f*6 + H_sig_f_nc[__min(5,__max(0,nc-(1-1)))];

  m = 0;
  m = m*14 + H_sig_m_cg[__min(13,__max(0,cg-(1-1)))];
  m = m*4 + H_sig_m_pos[__min(9,__max(0,pos-(1-1)))];
  m = m*4 + H_sig_m_rem[__min(10,__max(0,rem-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.e = e;  X.h = h;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

