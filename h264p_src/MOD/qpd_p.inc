/*  The template IDX/idx2inc.pl reads alongside qpd.idx: the qpd
    family's index builder and the tables its indices address.  Copied
    through into MOD/qpd_p.inc with `H_qpd_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/qpd_h.inc.  Edit this file and
    qpd.idx, not MOD/; ../mk.sh regenerates both.

    `_A`-`_D` are the four counters, `_S` and `_F` the two APMs, `_W` the
    mixer, `_MT` and `_WM` the mantissa plane and its mixer, `_G` the sign
    counters.  Each is H_qpd_ND (or H_qpd_MND) rows deep per context: where a
    decision sits among the family's nodes is context without being a
    variable.  */

void H_qpd_make( const H_qpd_v & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int lnz = D.lnz, pc = D.pc, cc = D.cc, cls = D.cls, dc = D.dc, db = D.db, lz = D.lz, st = D.st;
  (void) lnz;  (void) pc;  (void) cc;  (void) cls;  (void) dc;  (void) db;  (void) lz;  (void) st;

  a = 0;
  a = a*2 + ((lnz>0+(1-1)));
  a = a*5 + H_qpd_a_pc[__min(4,__max(0,pc-(1-1)))];
  a = a*3 + ((cc>0+(1-1))+(cc>1+(1-1)));
  a = a*9 + H_qpd_a_cls[__min(8,__max(0,cls-(1-1)))];

  b = 0;
  b = b*2 + ((lnz>0+(1-1)));
  b = b*9 + H_qpd_b_dc[__min(8,__max(0,dc-(1-1)))];
  b = b*9 + H_qpd_b_db[__min(8,__max(0,db-(1-1)))];

  c = 0;
  c = c*7 + H_qpd_c_lz[__min(6,__max(0,lz-(1-1)))];
  c = c*10 + H_qpd_c_st[__min(9,__max(0,st-(1-1)))];

  d = 0;
  d = d*2 + ((lnz>0+(1-1)));

  s = 0;
  s = s*2 + ((lnz>0+(1-1)));

  f = 0;
  f = f*10 + H_qpd_f_st[__min(9,__max(0,st-(1-1)))];

  m = 0;
  m = m*2 + ((lnz>0+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  g = g*7 + H_qpd_g_lz[__min(6,__max(0,lz-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

