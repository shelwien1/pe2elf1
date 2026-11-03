/*  The template idx2inc.pl reads alongside tsvcomp-dig.idx: the dig family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/l1/tsvcomp-dig_p.inc with
    `L1_dig_` replaced by the level's prefix -- `L1_dig_` -- `MakeIndex <a,b,...>`
    replaced by the statements that build those indices, and `Table( type,
    name, size )` lifted out into the L1_dig_T struct in
    MOD/l1/tsvcomp-dig_h.inc.  The index builder is L1_dig_make, which the
    level's class calls (levels.inc).

    Edit this file, not MOD/.  ../../mk.sh regenerates both.

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

void L1_dig_make( const tc_dv & D, tcx & X ) {
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
  a = a*8 + L1_dig_a_pass[__min(7,__max(0,pass-(1-1)))];
  a = a*10 + L1_dig_a_band[__min(15,__max(0,band-(1-1)))];
  a = a*5 + L1_dig_a_col[__min(7,__max(0,col-(1-1)))];
  a = a*4 + L1_dig_a_bkq[__min(4,__max(0,bkq-(1-1)))];
  a = a*9 + L1_dig_a_cls[__min(15,__max(0,cls-(1-1)))];
  a = a*2 + ((zrun>3+(1-1)));
  a = a*2 + ((blk>0+(1-1)));
  a = a*2 + ((w1>2+(1-1)));
  a = a*2 + ((p0>0+(1-1)));
  a = a*2 + ((p0s>1+(-6-1)));
  a = a*2 + ((pn>1+(1-1)));
  a = a*3 + ((ax>0+(1-1))+(ax>11+(1-1)));
  a = a*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  a = a*3 + ((chn>0+(1-1))+(chn>5+(1-1)));
  a = a*2 + ((crun>8+(1-1)));
  a = a*3 + ((x1s>1+(-6-1))+(x1s>12+(-6-1)));

  b = 0;
  b = b*2 + ((rno>0+(1-1)));
  b = b*2 + ((pass>0+(1-1)));
  b = b*12 + L1_dig_b_band[__min(15,__max(0,band-(1-1)))];
  b = b*5 + L1_dig_b_col[__min(7,__max(0,col-(1-1)))];
  b = b*6 + L1_dig_b_q1[__min(5,__max(0,q1-(1-1)))];
  b = b*2 + ((bkq>3+(1-1)));
  b = b*2 + ((cls>1+(1-1)));
  b = b*3 + ((zrun>0+(1-1))+(zrun>1+(1-1)));
  b = b*2 + ((blk>0+(1-1)));
  b = b*2 + ((p0>0+(1-1)));
  b = b*2 + ((p0s>7+(-6-1)));
  b = b*2 + ((pn>1+(1-1)));
  b = b*4 + L1_dig_b_ax[__min(12,__max(0,ax-(1-1)))];
  b = b*15 + L1_dig_b_pq[__min(32,__max(0,pq-(1-1)))];
  b = b*7 + L1_dig_b_ps[__min(13,__max(0,ps-(-6-1)))];
  b = b*4 + L1_dig_b_chn[__min(7,__max(0,chn-(1-1)))];

  c = 0;
  c = c*2 + ((pass>0+(1-1)));
  c = c*5 + L1_dig_c_ax[__min(12,__max(0,ax-(1-1)))];
  c = c*3 + ((q2s>1+(-6-1))+(q2s>11+(-6-1)));
  c = c*4 + L1_dig_c_p0s[__min(13,__max(0,p0s-(-6-1)))];
  c = c*2 + ((pq>21+(1-1)));
  c = c*5 + L1_dig_c_ps[__min(13,__max(0,ps-(-6-1)))];

  d = 0;
  d = d*4 + L1_dig_d_t1s[__min(13,__max(0,t1s-(-6-1)))];
  d = d*2 + ((t2>2+(1-1)));
  d = d*6 + L1_dig_d_ax[__min(12,__max(0,ax-(1-1)))];
  d = d*3 + ((q1s>0+(-6-1))+(q1s>11+(-6-1)));
  d = d*4 + L1_dig_d_p0s[__min(13,__max(0,p0s-(-6-1)))];
  d = d*2 + ((pq>11+(1-1)));
  d = d*2 + ((ps>6+(-6-1)));
  d = d*3 + ((chn>0+(1-1))+(chn>4+(1-1)));
  d = d*2 + ((x1>2+(1-1)));
  d = d*4 + L1_dig_d_x1s[__min(13,__max(0,x1s-(-6-1)))];

  s = 0;
  s = s*3 + ((band>0+(1-1))+(band>2+(1-1)));
  s = s*3 + ((q1>0+(1-1))+(q1>2+(1-1)));
  s = s*3 + ((bkq>1+(1-1))+(bkq>2+(1-1)));
  s = s*2 + ((vpos>0+(1-1)));
  s = s*2 + ((w1>0+(1-1)));
  s = s*2 + ((p0s>3+(-6-1)));
  s = s*3 + ((pn>0+(1-1))+(pn>1+(1-1)));
  s = s*2 + ((ax>5+(1-1)));
  s = s*7 + L1_dig_s_pq[__min(32,__max(0,pq-(1-1)))];
  s = s*2 + ((ps>3+(-6-1)));
  s = s*2 + ((chn>0+(1-1)));
  s = s*2 + ((cm>0+(1-1)));
  s = s*2 + ((x1>1+(1-1)));
  s = s*7 + L1_dig_s_x1s[__min(13,__max(0,x1s-(-6-1)))];

  f = 0;
  f = f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  f = f*7 + L1_dig_f_cls[__min(9,__max(0,cls-(1-1)))];
  f = f*3 + ((q1s>4+(-6-1))+(q1s>10+(-6-1)));
  f = f*2 + ((p0s>1+(-6-1)));
  f = f*7 + L1_dig_f_ax[__min(12,__max(0,ax-(1-1)))];
  f = f*6 + L1_dig_f_band[__min(5,__max(0,band-(1-1)))];
  f = f*9 + L1_dig_f_pq[__min(32,__max(0,pq-(1-1)))];
  f = f*3 + ((chn>0+(1-1))+(chn>3+(1-1)));
  f = f*3 + ((crun>2+(1-1))+(crun>8+(1-1)));
  f = f*2 + ((x1>1+(1-1)));
  f = f*4 + L1_dig_f_x1s[__min(13,__max(0,x1s-(-6-1)))];

  m = 0;
  m = m*2 + ((pass>1+(1-1)));
  m = m*2 + ((bkq>1+(1-1)));
  m = m*8 + L1_dig_m_cls[__min(15,__max(0,cls-(1-1)))];
  m = m*2 + ((vpos>5+(1-1)));
  m = m*4 + L1_dig_m_p0[__min(3,__max(0,p0-(1-1)))];
  m = m*3 + ((pn>0+(1-1))+(pn>2+(1-1)));
  m = m*5 + L1_dig_m_pq[__min(32,__max(0,pq-(1-1)))];
  m = m*5 + L1_dig_m_chn[__min(7,__max(0,chn-(1-1)))];

  t = 0;
  t = t*3 + ((pass>0+(1-1))+(pass>2+(1-1)));
  t = t*2 + ((band>0+(1-1)));
  t = t*3 + ((q1>1+(1-1))+(q1>3+(1-1)));
  t = t*7 + L1_dig_t_cls[__min(15,__max(0,cls-(1-1)))];
  t = t*2 + ((q1s>1+(-6-1)));
  t = t*2 + ((q2s>11+(-6-1)));
  t = t*2 + ((p0>0+(1-1)));
  t = t*5 + L1_dig_t_p0s[__min(13,__max(0,p0s-(-6-1)))];
  t = t*6 + L1_dig_t_ax[__min(12,__max(0,ax-(1-1)))];
  t = t*7 + L1_dig_t_pq[__min(32,__max(0,pq-(1-1)))];
  t = t*5 + L1_dig_t_chn[__min(7,__max(0,chn-(1-1)))];

  n = 0;
  n = n*4 + L1_dig_n_pass[__min(3,__max(0,pass-(1-1)))];
  n = n*2 + ((band>1+(1-1)));
  n = n*7 + L1_dig_n_cls[__min(15,__max(0,cls-(1-1)))];
  n = n*2 + ((ax>7+(1-1)));
  n = n*2 + ((ps>12+(-6-1)));
  n = n*2 + ((chn>0+(1-1)));

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

