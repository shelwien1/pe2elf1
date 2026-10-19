/*  The template IDX/idx2inc.pl reads alongside qpg.idx: the qpg
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/qpg_p.inc with `H1_qpg_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/qpg_h.inc.  Edit this file
    and qpg.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_qpg_ND (or H_qpg_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_qpg_make( const H_qpg_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int as = D.as, cs = D.cs, bs = D.bs, ls = D.ls, rs = D.rs, mag = D.mag, cls = D.cls, st = D.st, ss = D.ss, xe = D.xe, lq = D.lq, bk = D.bk;
  (void) as;  (void) cs;  (void) bs;  (void) ls;  (void) rs;  (void) mag;  (void) cls;  (void) st;  (void) ss;  (void) xe;  (void) lq;  (void) bk;

  a = 0;
  a = a*7 + H1_qpg_a_mag[__min(7,__max(0,mag-(1-1)))];
  a = a*7 + H1_qpg_a_ss[__min(9,__max(0,ss-(1-1)))];

  b = 0;
  b = b*6 + H1_qpg_b_cls[__min(8,__max(0,cls-(1-1)))];
  b = b*3 + ((st>1+(1-1))+(st>4+(1-1)));
  b = b*3 + ((ls>0+(1-1))+(ls>1+(1-1)));

  c = 0;
  c = c*7 + H1_qpg_c_cs[__min(9,__max(0,cs-(1-1)))];

  d = 0;
  d = d*7 + H1_qpg_d_bs[__min(9,__max(0,bs-(1-1)))];
  d = d*4 + H1_qpg_d_mag[__min(7,__max(0,mag-(1-1)))];
  d = d*2 + ((bk>8+(1-1)));

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

