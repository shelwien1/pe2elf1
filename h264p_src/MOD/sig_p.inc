/*  The template IDX/idx2inc.pl reads alongside sig.idx: the sig
    family's index builder and the tables its indices address.  Copied
    through into MOD/sig_p.inc with `H_sig_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/sig_h.inc.  Edit this file and
    sig.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` the
    mixer, `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_sig_ND (or H_sig_MND) rows deep per context: where a
    decision sits among the family's nodes is context without being a
    variable.  */

void H_sig_make( const H_sig_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int cg = D.cg, pos = D.pos, nc = D.nc, cnt = D.cnt, fld = D.fld, q6 = D.q6;
  (void) cg;  (void) pos;  (void) nc;  (void) cnt;  (void) fld;  (void) q6;

  a = 0;
  a = a*14 + H_sig_a_cg[__min(13,__max(0,cg-(1-1)))];
  a = a*64 + H_sig_a_pos[__min(63,__max(0,pos-(1-1)))];
  a = a*6 + H_sig_a_nc[__min(5,__max(0,nc-(1-1)))];
  a = a*4 + H_sig_a_cnt[__min(3,__max(0,cnt-(1-1)))];

  b = 0;
  b = b*14 + H_sig_b_cg[__min(13,__max(0,cg-(1-1)))];
  b = b*64 + H_sig_b_pos[__min(63,__max(0,pos-(1-1)))];
  b = b*2 + ((fld>0+(1-1)));

  c = 0;
  c = c*14 + H_sig_c_cg[__min(13,__max(0,cg-(1-1)))];
  c = c*64 + H_sig_c_pos[__min(63,__max(0,pos-(1-1)))];
  c = c*9 + H_sig_c_q6[__min(8,__max(0,q6-(1-1)))];

  d = 0;
  d = d*14 + H_sig_d_cg[__min(13,__max(0,cg-(1-1)))];
  d = d*8 + H_sig_d_cnt[__min(7,__max(0,cnt-(1-1)))];

  s = 0;
  s = s*14 + H_sig_s_cg[__min(13,__max(0,cg-(1-1)))];

  f = 0;
  f = f*14 + H_sig_f_cg[__min(13,__max(0,cg-(1-1)))];
  f = f*6 + H_sig_f_nc[__min(5,__max(0,nc-(1-1)))];

  m = 0;
  m = m*14 + H_sig_m_cg[__min(13,__max(0,cg-(1-1)))];
  m = m*4 + H_sig_m_pos[__min(9,__max(0,pos-(1-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

