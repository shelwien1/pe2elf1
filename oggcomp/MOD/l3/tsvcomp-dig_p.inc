/*  The template idx2inc.pl reads alongside tsvcomp-dig.idx: the dig family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/l3/tsvcomp-dig_p.inc with
    `L3_dig_` replaced by the level's prefix -- `L3_dig_` -- `MakeIndex <a,b,...>`
    replaced by the statements that build those indices, and `Table( type,
    name, size )` lifted out into the L3_dig_T struct in
    MOD/l3/tsvcomp-dig_h.inc.  The index builder is L3_dig_make, which the
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

void L3_dig_make( const tc_dv & D, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax,
      pq = D.pq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm,
      x1 = D.x1, x1s = D.x1s, pmx = D.pmx, psm = D.psm, prm = D.prm;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;   (void) ps;   (void) chn;
  (void) crun; (void) cm;   (void) x1;    (void) x1s;
  (void) pmx;  (void) psm;  (void) prm;

  a = 0;
  a = a*8 + L3_dig_a_pass[__min(7,__max(0,pass-(1-1)))];
  a = a*11 + L3_dig_a_band[__min(15,__max(0,band-(1-1)))];
  a = a*5 + L3_dig_a_col[__min(7,__max(0,col-(1-1)))];
  a = a*4 + L3_dig_a_bkq[__min(4,__max(0,bkq-(1-1)))];
  a = a*5 + L3_dig_a_cls[__min(15,__max(0,cls-(1-1)))];
  a = a*2 + ((zrun>3+(1-1)));
  a = a*2 + ((blk>0+(1-1)));
  a = a*2 + ((p0>0+(1-1)));
  a = a*3 + ((p0s>1+(-6-1))+(p0s>3+(-6-1)));
  a = a*2 + ((pn>1+(1-1)));
  a = a*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  a = a*2 + ((ps>3+(-6-1)));
  a = a*4 + L3_dig_a_chn[__min(7,__max(0,chn-(1-1)))];
  a = a*2 + ((x1>1+(1-1)));
  a = a*4 + L3_dig_a_x1s[__min(13,__max(0,x1s-(-6-1)))];
  a = a*3 + ((pmx>2+(1-1))+(pmx>3+(1-1)));
  a = a*2 + ((psm>0+(1-1)));
  a = a*2 + ((prm>0+(1-1)));

  b = 0;
  b = b*2 + ((rno>0+(1-1)));
  b = b*3 + ((pass>0+(1-1))+(pass>4+(1-1)));
  b = b*7 + L3_dig_b_band[__min(15,__max(0,band-(1-1)))];
  b = b*2 + ((col>6+(1-1)));
  b = b*3 + ((q1>0+(1-1))+(q1>2+(1-1)));
  b = b*4 + L3_dig_b_bkq[__min(4,__max(0,bkq-(1-1)))];
  b = b*2 + ((cls>2+(1-1)));
  b = b*2 + ((zrun>0+(1-1)));
  b = b*2 + ((blk>0+(1-1)));
  b = b*3 + ((q1s>0+(-6-1))+(q1s>12+(-6-1)));
  b = b*3 + ((p0>0+(1-1))+(p0>2+(1-1)));
  b = b*2 + ((pn>2+(1-1)));
  b = b*4 + L3_dig_b_ax[__min(12,__max(0,ax-(1-1)))];
  b = b*13 + L3_dig_b_pq[__min(32,__max(0,pq-(1-1)))];
  b = b*3 + ((ps>2+(-6-1))+(ps>10+(-6-1)));
  b = b*2 + ((chn>0+(1-1)));
  b = b*5 + L3_dig_b_x1s[__min(13,__max(0,x1s-(-6-1)))];
  b = b*4 + L3_dig_b_pmx[__min(4,__max(0,pmx-(1-1)))];
  b = b*2 + ((psm>1+(1-1)));
  b = b*2 + ((prm>2+(1-1)));

  c = 0;
  c = c*3 + ((band>2+(1-1))+(band>3+(1-1)));
  c = c*3 + ((t1s>0+(-6-1))+(t1s>12+(-6-1)));
  c = c*2 + ((ax>1+(1-1)));
  c = c*6 + L3_dig_c_p0s[__min(13,__max(0,p0s-(-6-1)))];
  c = c*3 + ((pq>8+(1-1))+(pq>24+(1-1)));
  c = c*2 + ((ps>1+(-6-1)));

  d = 0;
  d = d*3 + ((band>0+(1-1))+(band>1+(1-1)));
  d = d*4 + L3_dig_d_t1s[__min(13,__max(0,t1s-(-6-1)))];
  d = d*3 + ((w1>1+(1-1))+(w1>2+(1-1)));
  d = d*5 + L3_dig_d_ax[__min(12,__max(0,ax-(1-1)))];
  d = d*4 + L3_dig_d_q1s[__min(13,__max(0,q1s-(-6-1)))];
  d = d*2 + ((q2s>1+(-6-1)));
  d = d*4 + L3_dig_d_p0s[__min(13,__max(0,p0s-(-6-1)))];
  d = d*12 + L3_dig_d_pq[__min(32,__max(0,pq-(1-1)))];
  d = d*8 + L3_dig_d_ps[__min(13,__max(0,ps-(-6-1)))];
  d = d*5 + L3_dig_d_chn[__min(7,__max(0,chn-(1-1)))];
  d = d*3 + ((crun>2+(1-1))+(crun>5+(1-1)));
  d = d*2 + ((cm>0+(1-1)));
  d = d*8 + L3_dig_d_x1s[__min(13,__max(0,x1s-(-6-1)))];

  s = 0;
  s = s*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  s = s*3 + ((band>0+(1-1))+(band>2+(1-1)));
  s = s*2 + ((q1>1+(1-1)));
  s = s*3 + ((bkq>0+(1-1))+(bkq>2+(1-1)));
  s = s*3 + ((w1>0+(1-1))+(w1>1+(1-1)));
  s = s*2 + ((p0s>12+(-6-1)));
  s = s*2 + ((pn>1+(1-1)));
  s = s*5 + L3_dig_s_ax[__min(12,__max(0,ax-(1-1)))];
  s = s*7 + L3_dig_s_pq[__min(32,__max(0,pq-(1-1)))];
  s = s*3 + ((chn>0+(1-1))+(chn>2+(1-1)));
  s = s*2 + ((crun>8+(1-1)));
  s = s*2 + ((x1>0+(1-1)));

  f = 0;
  f = f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  f = f*8 + L3_dig_f_cls[__min(9,__max(0,cls-(1-1)))];
  f = f*4 + L3_dig_f_q1s[__min(13,__max(0,q1s-(-6-1)))];
  f = f*4 + L3_dig_f_ax[__min(12,__max(0,ax-(1-1)))];
  f = f*5 + L3_dig_f_band[__min(5,__max(0,band-(1-1)))];
  f = f*4 + L3_dig_f_pq[__min(32,__max(0,pq-(1-1)))];
  f = f*3 + ((ps>6+(-6-1))+(ps>7+(-6-1)));
  f = f*2 + ((chn>0+(1-1)));
  f = f*2 + ((x1>2+(1-1)));

  m = 0;
  m = m*2 + ((pass>1+(1-1)));
  m = m*2 + ((bkq>2+(1-1)));
  m = m*7 + L3_dig_m_cls[__min(15,__max(0,cls-(1-1)))];
  m = m*2 + ((vpos>3+(1-1)));
  m = m*4 + L3_dig_m_p0[__min(3,__max(0,p0-(1-1)))];
  m = m*3 + ((pn>0+(1-1))+(pn>2+(1-1)));
  m = m*2 + ((pq>20+(1-1)));
  m = m*5 + L3_dig_m_chn[__min(7,__max(0,chn-(1-1)))];
  m = m*2 + ((x1s>0+(-6-1)));

  t = 0;
  t = t*3 + ((pass>0+(1-1))+(pass>2+(1-1)));
  t = t*3 + ((band>1+(1-1))+(band>2+(1-1)));
  t = t*2 + ((q1>4+(1-1)));
  t = t*10 + L3_dig_t_cls[__min(15,__max(0,cls-(1-1)))];
  t = t*10 + L3_dig_t_q1s[__min(13,__max(0,q1s-(-6-1)))];
  t = t*2 + ((p0>0+(1-1)));
  t = t*2 + ((ax>9+(1-1)));
  t = t*7 + L3_dig_t_pq[__min(32,__max(0,pq-(1-1)))];
  t = t*2 + ((ps>2+(-6-1)));
  t = t*5 + L3_dig_t_chn[__min(7,__max(0,chn-(1-1)))];

  n = 0;
  n = n*4 + L3_dig_n_pass[__min(3,__max(0,pass-(1-1)))];
  n = n*2 + ((band>0+(1-1)));
  n = n*3 + ((cls>6+(1-1))+(cls>7+(1-1)));
  n = n*2 + ((p0>0+(1-1)));
  n = n*2 + ((p0s>0+(-6-1)));
  n = n*2 + ((ax>10+(1-1)));
  n = n*2 + ((chn>0+(1-1)));

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

