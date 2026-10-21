/*  The template IDX/idx2inc.pl reads alongside mbt.idx: the mbt
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/mbt_p.inc with `H1_mbt_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/mbt_h.inc.  Edit this file
    and mbt.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_mbt_ND (or H_mbt_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_mbt_make( const H_mbt_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int st = D.st, ca = D.ca, cb = D.cb, cc = D.cc, nsk = D.nsk, en = D.en, td = D.td, sm = D.sm, tm = D.tm, tb = D.tb, dh = D.dh, bd = D.bd, qc = D.qc, im = D.im, ig = D.ig, c1 = D.c1, tx = D.tx;
  (void) st;  (void) ca;  (void) cb;  (void) cc;  (void) nsk;  (void) en;  (void) td;  (void) sm;  (void) tm;  (void) tb;  (void) dh;  (void) bd;  (void) qc;  (void) im;  (void) ig;  (void) c1;  (void) tx;

  a = 0;
  a = a*3 + (((st)>1+(1-1))+((st)>4+(1-1)));
  a = a*7 + H1_mbt_a_ca[__min(8,__max(0,(ca)-(1-1)))];
  a = a*7 + H1_mbt_a_cb[__min(8,__max(0,(cb)-(1-1)))];

  b = 0;
  b = b*4 + H1_mbt_b_st[__min(9,__max(0,(st)-(1-1)))];
  b = b*6 + H1_mbt_b_cc[__min(8,__max(0,(cc)-(1-1)))];
  b = b*2 + (((nsk)>3+(1-1)));

  c = 0;
  c = c*2 + (((ca)>0+(1-1)));
  c = c*3 + (((cb)>0+(1-1))+((cb)>5+(1-1)));

  d = 0;
  d = d*3 + (((st)>1+(1-1))+((st)>2+(1-1)));

  m = 0;

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

