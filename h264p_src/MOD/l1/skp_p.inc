/*  The template IDX/idx2inc.pl reads alongside skp.idx: the skp
    family's index builder and the tables its indices address.  Copied
    through into MOD/l1/skp_p.inc with `H1_skp_` replaced by the prefix,
    `MakeIndex` by the statements that build the indices, and the Table()
    lines lifted out into the struct in MOD/l1/skp_h.inc.  Edit this file
    and skp.idx, not MOD/; ../../mk.sh regenerates both.

    Level -1: `_A`-`_D` are the four counters, `_W` the one mixer
    (selected by index m, rows of 16 inputs), `_MT` and `_WM` the mantissa
    plane and its mixer, `_G` the sign counters.  Each is H_skp_ND (or H_skp_MND) rows deep per
    context: where a decision sits among the family's nodes is context
    without being a variable.  */

void H1_skp_make( const H_skp_v & D, tcx & X ) {
  int a, b, c, d, m, t, n, g;
  int st = D.st, a3 = D.a3, b3 = D.b3, col = D.col, c9 = D.c9, cd = D.cd, run = D.run, cnt = D.cnt, eq = D.eq, lf = D.lf, af = D.af, td = D.td, en = D.en, sm = D.sm, tm = D.tm, tb = D.tb, bd = D.bd, qc = D.qc, c1 = D.c1, k9 = D.k9, tx = D.tx;
  (void) st;  (void) a3;  (void) b3;  (void) col;  (void) c9;  (void) cd;  (void) run;  (void) cnt;  (void) eq;  (void) lf;  (void) af;  (void) td;  (void) en;  (void) sm;  (void) tm;  (void) tb;  (void) bd;  (void) qc;  (void) c1;  (void) k9;  (void) tx;

  a = 0;
  a = a*3 + ((st>1+(1-1))+(st>2+(1-1)));
  a = a*2 + ((a3>0+(1-1)));
  a = a*3 + ((b3>0+(1-1))+(b3>1+(1-1)));
  a = a*2 + ((col>1+(1-1)));
  a = a*9 + H1_skp_a_c9[__min(9,__max(0,c9-(1-1)))];
  a = a*2 + ((eq>0+(1-1)));

  b = 0;
  b = b*2 + ((st>1+(1-1)));
  b = b*3 + ((a3>0+(1-1))+(a3>1+(1-1)));
  b = b*2 + ((b3>1+(1-1)));
  b = b*3 + ((cd>0+(1-1))+(cd>1+(1-1)));
  b = b*8 + H1_skp_b_cnt[__min(15,__max(0,cnt-(1-1)))];

  c = 0;
  c = c*2 + ((st>1+(1-1)));
  c = c*4 + H1_skp_c_c9[__min(9,__max(0,c9-(1-1)))];
  c = c*6 + H1_skp_c_run[__min(9,__max(0,run-(1-1)))];

  d = 0;
  d = d*3 + ((b3>0+(1-1))+(b3>1+(1-1)));

  m = 0;
  m = m*3 + ((st>1+(1-1))+(st>3+(1-1)));
  m = m*2 + ((col>1+(1-1)));

  t = 0;

  n = 0;

  g = 0;
  X.c[0] = a;  X.c[1] = b;  X.c[2] = c;  X.c[3] = d;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

