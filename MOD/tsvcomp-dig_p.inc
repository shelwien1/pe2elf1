/*  The template idx2inc.pl reads alongside tsvcomp-dig.idx: the dig family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-dig_p.inc with `TC_dig_`
    replaced by the prefix -- `TC_dig_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_dig_T struct in MOD/tsvcomp-dig_h.inc.

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

void tc_make_dig( const tc_dv & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax,
      pq = D.pq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm,
      x1 = D.x1, x1s = D.x1s;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;   (void) ps;   (void) chn;
  (void) crun; (void) cm;   (void) x1;    (void) x1s;

  a = 0;
  a = a*8 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1))+(pass>3+(1-1))+(pass>4+(1-1))+(pass>5+(1-1))+(pass>6+(1-1)));
  a = a*16 + TC_dig_a_band[__min(15,__max(0,band-(1-1)))];
  a = a*5 + ((col>1+(1-1))+(col>4+(1-1))+(col>5+(1-1))+(col>6+(1-1)));
  a = a*2 + ((bkq>2+(1-1)));
  a = a*6 + ((cls>1+(1-1))+(cls>3+(1-1))+(cls>6+(1-1))+(cls>7+(1-1))+(cls>8+(1-1)));
  a = a*2 + ((blk>0+(1-1)));
  a = a*2 + ((p0s>1+(-6-1)));
  a = a*2 + ((ax>6+(1-1)));
  a = a*3 + ((pq>8+(1-1))+(pq>24+(1-1)));

  b = 0;
  b = b*3 + ((pass>1+(1-1))+(pass>5+(1-1)));
  b = b*9 + TC_dig_b_band[__min(15,__max(0,band-(1-1)))];
  b = b*4 + ((q1>0+(1-1))+(q1>2+(1-1))+(q1>4+(1-1)));
  b = b*2 + ((bkq>3+(1-1)));
  b = b*2 + ((cls>1+(1-1)));
  b = b*3 + ((zrun>0+(1-1))+(zrun>2+(1-1)));
  b = b*2 + ((blk>0+(1-1)));
  b = b*4 + ((p0>0+(1-1))+(p0>1+(1-1))+(p0>2+(1-1)));
  b = b*3 + ((p0s>2+(-6-1))+(p0s>7+(-6-1)));
  b = b*10 + TC_dig_b_ax[__min(12,__max(0,ax-(1-1)))];
  b = b*9 + TC_dig_b_pq[__min(32,__max(0,pq-(1-1)))];
  b = b*2 + ((chn>0+(1-1)));

  c = 0;
  c = c*2 + ((pass>0+(1-1)));
  c = c*3 + ((band>1+(1-1))+(band>3+(1-1)));
  c = c*5 + ((ax>0+(1-1))+(ax>1+(1-1))+(ax>2+(1-1))+(ax>9+(1-1)));
  c = c*2 + ((q1s>1+(-6-1)));
  c = c*2 + ((p0s>6+(-6-1)));
  c = c*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  c = c*3 + ((ps>3+(-6-1))+(ps>9+(-6-1)));

  d = 0;
  d = d*2 + ((vpos>0+(1-1)));
  d = d*7 + ((ax>1+(1-1))+(ax>3+(1-1))+(ax>5+(1-1))+(ax>7+(1-1))+(ax>9+(1-1))+(ax>11+(1-1)));
  d = d*9 + TC_dig_d_q1s[__min(13,__max(0,q1s-(-6-1)))];
  d = d*7 + ((p0s>2+(-6-1))+(p0s>4+(-6-1))+(p0s>5+(-6-1))+(p0s>6+(-6-1))+(p0s>7+(-6-1))+(p0s>8+(-6-1)));
  d = d*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  d = d*4 + ((ps>2+(-6-1))+(ps>6+(-6-1))+(ps>10+(-6-1)));
  d = d*2 + ((chn>0+(1-1)));
  d = d*8 + ((x1s>3+(-6-1))+(x1s>4+(-6-1))+(x1s>5+(-6-1))+(x1s>6+(-6-1))+(x1s>7+(-6-1))+(x1s>8+(-6-1))+(x1s>9+(-6-1)));

  s = 0;
  s = s*2 + ((pass>0+(1-1)));
  s = s*2 + ((band>0+(1-1)));
  s = s*3 + ((t1>1+(1-1))+(t1>2+(1-1)));
  s = s*3 + ((bkq>0+(1-1))+(bkq>2+(1-1)));
  s = s*2 + ((t1s>0+(-6-1)));
  s = s*3 + ((p0>0+(1-1))+(p0>2+(1-1)));
  s = s*2 + ((p0s>1+(-6-1)));
  s = s*8 + ((ax>0+(1-1))+(ax>4+(1-1))+(ax>6+(1-1))+(ax>8+(1-1))+(ax>9+(1-1))+(ax>10+(1-1))+(ax>11+(1-1)));
  s = s*2 + ((ps>2+(-6-1)));
  s = s*2 + ((chn>0+(1-1)));
  s = s*2 + ((cm>0+(1-1)));

  f = 0;
  f = f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  f = f*9 + TC_dig_f_cls[__min(9,__max(0,cls-(1-1)))];
  f = f*4 + ((q1s>4+(-6-1))+(q1s>8+(-6-1))+(q1s>11+(-6-1)));
  f = f*6 + ((ax>3+(1-1))+(ax>5+(1-1))+(ax>7+(1-1))+(ax>9+(1-1))+(ax>11+(1-1)));
  f = f*4 + ((band>0+(1-1))+(band>2+(1-1))+(band>4+(1-1)));
  f = f*2 + ((chn>0+(1-1)));

  m = 0;
  m = m*2 + ((pass>1+(1-1)));
  m = m*2 + ((bkq>2+(1-1)));
  m = m*10 + TC_dig_m_cls[__min(15,__max(0,cls-(1-1)))];
  m = m*8 + ((vpos>0+(1-1))+(vpos>1+(1-1))+(vpos>2+(1-1))+(vpos>3+(1-1))+(vpos>4+(1-1))+(vpos>5+(1-1))+(vpos>6+(1-1)));
  m = m*2 + ((p0>0+(1-1)));
  m = m*3 + ((pn>0+(1-1))+(pn>2+(1-1)));
  m = m*14 + TC_dig_m_pq[__min(32,__max(0,pq-(1-1)))];
  m = m*5 + ((chn>0+(1-1))+(chn>2+(1-1))+(chn>4+(1-1))+(chn>6+(1-1)));

  t = 0;
  t = t*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  t = t*3 + ((band>0+(1-1))+(band>2+(1-1)));
  t = t*11 + TC_dig_t_cls[__min(15,__max(0,cls-(1-1)))];
  t = t*9 + TC_dig_t_q1s[__min(13,__max(0,q1s-(-6-1)))];
  t = t*2 + ((p0>0+(1-1)));
  t = t*3 + ((pq>19+(1-1))+(pq>23+(1-1)));
  t = t*5 + ((chn>0+(1-1))+(chn>2+(1-1))+(chn>4+(1-1))+(chn>6+(1-1)));

  n = 0;
  n = n*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  n = n*7 + ((cls>1+(1-1))+(cls>3+(1-1))+(cls>6+(1-1))+(cls>8+(1-1))+(cls>11+(1-1))+(cls>13+(1-1)));
  n = n*2 + ((p0>0+(1-1)));
  n = n*2 + ((chn>0+(1-1)));

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

