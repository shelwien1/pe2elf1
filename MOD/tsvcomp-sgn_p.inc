/*  The template idx2inc.pl reads alongside tsvcomp-sgn.idx: the sgn family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-sgn_p.inc with `TC_sgn_`
    replaced by the prefix -- `TC_sgn_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_sgn_T struct in MOD/tsvcomp-sgn_h.inc.

    Edit this file, not MOD/.  ../mk.sh regenerates both.

    One function, building the family's ten indices together: the four stages
    of a decision (two counters, the APM, the mixer) each want a different
    grouping of the same variables, and the mantissa plane a fifth, which is
    what mp3c's separate index declarations per submodel are for.  The `(void)`
    lines are there because a variable at pattern zero generates no code, and a
    family lists every variable it has so the optimizer has somewhere to go.

    `_A` is the rich counter, `_B` the coarse one, `_S` the APM's buckets per
    context, `_W` the mixer's weights, `_T` the mantissa plane, `_G` the sign
    counters where a family codes signs under a context of their own.  Every
    one of them is TC_NODE (or TC_MNODE) rows deep per context, so where a bit
    sits in the cascade is context without having to be declared a variable.  */

void tc_make_sgn( const tc_dv & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax, mg = D.mg,
      pq = D.pq, sq = D.sq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm,
      x1 = D.x1, x1s = D.x1s;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) mg;    (void) pq;   (void) sq;
  (void) ps;   (void) chn;   (void) crun;  (void) cm;
  (void) x1;   (void) x1s;

  a = 0;
  a = a*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  a = a*6 + ((cls>0+(1-1))+(cls>3+(1-1))+(cls>5+(1-1))+(cls>6+(1-1))+(cls>8+(1-1)));
  a = a*7 + ((p0s>1+(-3-1))+(p0s>2+(-3-1))+(p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>5+(-3-1))+(p0s>6+(-3-1)));
  a = a*5 + ((mg>1+(1-1))+(mg>3+(1-1))+(mg>5+(1-1))+(mg>7+(1-1)));
  a = a*2 + ((band>4+(1-1)));
  a = a*5 + ((pq>4+(1-1))+(pq>12+(1-1))+(pq>20+(1-1))+(pq>28+(1-1)));
  a = a*9 + TC_sgn_a_sq[__min(32,__max(0,sq-(1-1)))];
  a = a*9 + TC_sgn_a_ps[__min(13,__max(0,ps-(-6-1)))];
  a = a*2 + ((chn>0+(1-1)));
  a = a*8 + ((x1s>3+(-6-1))+(x1s>4+(-6-1))+(x1s>5+(-6-1))+(x1s>6+(-6-1))+(x1s>7+(-6-1))+(x1s>8+(-6-1))+(x1s>9+(-6-1)));

  b = 0;
  b = b*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  b = b*2 + ((vpos>0+(1-1)));
  b = b*8 + ((p0s>0+(-3-1))+(p0s>1+(-3-1))+(p0s>2+(-3-1))+(p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>5+(-3-1))+(p0s>6+(-3-1)));
  b = b*5 + ((q1s>0+(-3-1))+(q1s>2+(-3-1))+(q1s>4+(-3-1))+(q1s>6+(-3-1)));
  b = b*4 + ((q2s>1+(-3-1))+(q2s>3+(-3-1))+(q2s>5+(-3-1)));
  b = b*4 + ((band>0+(1-1))+(band>3+(1-1))+(band>4+(1-1)));
  b = b*5 + ((pq>4+(1-1))+(pq>12+(1-1))+(pq>20+(1-1))+(pq>28+(1-1)));
  b = b*9 + TC_sgn_b_sq[__min(32,__max(0,sq-(1-1)))];
  b = b*6 + ((x1s>4+(-6-1))+(x1s>5+(-6-1))+(x1s>6+(-6-1))+(x1s>7+(-6-1))+(x1s>8+(-6-1)));

  c = 0;
  c = c*2 + ((pass>1+(1-1)));
  c = c*3 + ((p0s>0+(-3-1))+(p0s>6+(-3-1)));
  c = c*2 + ((q1s>4+(-3-1)));
  c = c*2 + ((q2s>4+(-3-1)));

  d = 0;
  d = d*3 + ((p0s>3+(-3-1))+(p0s>4+(-3-1)));
  d = d*2 + ((q1s>3+(-3-1)));
  d = d*2 + ((q2s>3+(-3-1)));
  d = d*3 + ((sq>8+(1-1))+(sq>24+(1-1)));
  d = d*4 + ((ps>2+(-6-1))+(ps>6+(-6-1))+(ps>10+(-6-1)));
  d = d*2 + ((chn>0+(1-1)));

  s = 0;
  s = s*2 + ((pass>1+(1-1)));
  s = s*4 + ((p0s>1+(-3-1))+(p0s>3+(-3-1))+(p0s>5+(-3-1)));
  s = s*5 + ((pq>4+(1-1))+(pq>12+(1-1))+(pq>20+(1-1))+(pq>28+(1-1)));
  s = s*3 + ((sq>8+(1-1))+(sq>24+(1-1)));
  s = s*4 + ((ps>2+(-6-1))+(ps>6+(-6-1))+(ps>10+(-6-1)));
  s = s*2 + ((chn>0+(1-1)));
  s = s*2 + ((crun>5+(1-1)));

  f = 0;
  f = f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  f = f*2 + ((vpos>0+(1-1)));
  f = f*7 + ((p0s>0+(-3-1))+(p0s>1+(-3-1))+(p0s>2+(-3-1))+(p0s>4+(-3-1))+(p0s>5+(-3-1))+(p0s>6+(-3-1)));
  f = f*3 + ((mg>0+(1-1))+(mg>2+(1-1)));
  f = f*2 + ((pq>16+(1-1)));
  f = f*9 + TC_sgn_f_sq[__min(32,__max(0,sq-(1-1)))];
  f = f*2 + ((ps>6+(-6-1)));
  f = f*2 + ((chn>0+(1-1)));
  f = f*2 + ((crun>4+(1-1)));
  f = f*2 + ((cm>0+(1-1)));

  m = 0;
  m = m*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  m = m*2 + ((mg>1+(1-1)));
  m = m*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  m = m*3 + ((sq>8+(1-1))+(sq>24+(1-1)));
  m = m*2 + ((chn>0+(1-1)));
  m = m*14 + TC_sgn_m_x1s[__min(13,__max(0,x1s-(-6-1)))];

  t = 0;

  n = 0;

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

