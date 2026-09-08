/*  The template idx2inc.pl reads alongside tsvcomp.idx.  Everything here is
    copied through into MOD/tsvcomp_p.inc with `TC_` replaced by the prefix,
    `MakeIndex <a,b,...>` replaced by the statements that build those indices,
    and `Table( type, name, size )` lifted out into the TC_T struct in
    MOD/tsvcomp_h.inc.

    Edit this file, not MOD/.  ../mk.sh regenerates both.

    One function per family, building that family's five indices together: the
    four stages of a decision (two counters, the APM, the mixer) each want a
    different grouping of the same variables, and the mantissa plane a fifth,
    which is what mp3c's separate index declarations per submodel are for.
    The `(void)` lines are there because a variable at pattern zero generates
    no code, and a family lists every variable it has so the optimizer has
    somewhere to go.  */

void tc_make_dig( const tc_dv & D, tcx & X ) {
  int dig_a, dig_b, dig_c, dig_d, dig_s, dig_f, dig_m, dig_t;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax,
      pq = D.pq;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;

  dig_a = 0;
  dig_a = dig_a*8 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1))+(pass>3+(1-1))+(pass>4+(1-1))+(pass>5+(1-1))+(pass>6+(1-1)));
  dig_a = dig_a*16 + TC_dig_a_band[__min(15,__max(0,band-(1-1)))];
  dig_a = dig_a*3 + ((col>1+(1-1))+(col>5+(1-1)));
  dig_a = dig_a*3 + ((bkq>1+(1-1))+(bkq>3+(1-1)));
  dig_a = dig_a*6 + ((cls>1+(1-1))+(cls>3+(1-1))+(cls>6+(1-1))+(cls>7+(1-1))+(cls>8+(1-1)));
  dig_a = dig_a*2 + ((blk>0+(1-1)));
  dig_a = dig_a*2 + ((ax>6+(1-1)));

  dig_b = 0;
  dig_b = dig_b*3 + ((pass>2+(1-1))+(pass>6+(1-1)));
  dig_b = dig_b*4 + ((band>0+(1-1))+(band>1+(1-1))+(band>8+(1-1)));
  dig_b = dig_b*4 + ((col>1+(1-1))+(col>4+(1-1))+(col>6+(1-1)));
  dig_b = dig_b*2 + ((q2>2+(1-1)));
  dig_b = dig_b*3 + ((bkq>1+(1-1))+(bkq>2+(1-1)));
  dig_b = dig_b*2 + ((zrun>2+(1-1)));
  dig_b = dig_b*2 + ((blk>0+(1-1)));
  dig_b = dig_b*2 + ((p0>0+(1-1)));
  dig_b = dig_b*2 + ((pn>0+(1-1)));
  dig_b = dig_b*9 + TC_dig_b_pq[__min(32,__max(0,pq-(1-1)))];

  dig_c = 0;
  dig_c = dig_c*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  dig_c = dig_c*3 + ((band>1+(1-1))+(band>2+(1-1)));
  dig_c = dig_c*3 + ((ax>0+(1-1))+(ax>9+(1-1)));
  dig_c = dig_c*2 + ((q2s>0+(-6-1)));

  dig_d = 0;
  dig_d = dig_d*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  dig_d = dig_d*2 + ((vpos>0+(1-1)));
  dig_d = dig_d*7 + ((ax>1+(1-1))+(ax>3+(1-1))+(ax>5+(1-1))+(ax>7+(1-1))+(ax>9+(1-1))+(ax>11+(1-1)));
  dig_d = dig_d*7 + ((q1s>1+(-6-1))+(q1s>3+(-6-1))+(q1s>5+(-6-1))+(q1s>7+(-6-1))+(q1s>9+(-6-1))+(q1s>11+(-6-1)));
  dig_d = dig_d*4 + ((q2s>1+(-6-1))+(q2s>3+(-6-1))+(q2s>11+(-6-1)));
  dig_d = dig_d*3 + ((cls>1+(1-1))+(cls>2+(1-1)));
  dig_d = dig_d*6 + ((p0s>4+(-6-1))+(p0s>5+(-6-1))+(p0s>6+(-6-1))+(p0s>7+(-6-1))+(p0s>8+(-6-1)));
  dig_d = dig_d*9 + TC_dig_d_pq[__min(32,__max(0,pq-(1-1)))];

  dig_s = 0;
  dig_s = dig_s*2 + ((pass>0+(1-1)));
  dig_s = dig_s*2 + ((t1>1+(1-1)));
  dig_s = dig_s*3 + ((bkq>1+(1-1))+(bkq>2+(1-1)));
  dig_s = dig_s*2 + ((w1>1+(1-1)));
  dig_s = dig_s*3 + ((p0>0+(1-1))+(p0>2+(1-1)));
  dig_s = dig_s*4 + ((p0s>1+(-6-1))+(p0s>2+(-6-1))+(p0s>10+(-6-1)));
  dig_s = dig_s*2 + ((pn>1+(1-1)));
  dig_s = dig_s*9 + TC_dig_s_ax[__min(12,__max(0,ax-(1-1)))];
  dig_s = dig_s*4 + ((pq>5+(1-1))+(pq>16+(1-1))+(pq>26+(1-1)));

  dig_f = 0;
  dig_f = dig_f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  dig_f = dig_f*2 + ((vpos>0+(1-1)));
  dig_f = dig_f*10 + TC_dig_f_cls[__min(9,__max(0,cls-(1-1)))];
  dig_f = dig_f*3 + ((q1s>3+(-6-1))+(q1s>9+(-6-1)));
  dig_f = dig_f*2 + ((t1s>0+(-6-1)));
  dig_f = dig_f*7 + ((ax>1+(1-1))+(ax>3+(1-1))+(ax>5+(1-1))+(ax>7+(1-1))+(ax>9+(1-1))+(ax>11+(1-1)));
  dig_f = dig_f*3 + ((band>1+(1-1))+(band>3+(1-1)));
  dig_f = dig_f*4 + ((pq>5+(1-1))+(pq>16+(1-1))+(pq>26+(1-1)));

  dig_m = 0;
  dig_m = dig_m*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  dig_m = dig_m*2 + ((q1>1+(1-1)));
  dig_m = dig_m*2 + ((bkq>2+(1-1)));
  dig_m = dig_m*9 + TC_dig_m_cls[__min(15,__max(0,cls-(1-1)))];
  dig_m = dig_m*8 + ((vpos>0+(1-1))+(vpos>1+(1-1))+(vpos>2+(1-1))+(vpos>3+(1-1))+(vpos>4+(1-1))+(vpos>5+(1-1))+(vpos>6+(1-1)));
  dig_m = dig_m*3 + ((p0>0+(1-1))+(p0>2+(1-1)));

  dig_t = 0;
  dig_t = dig_t*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  dig_t = dig_t*6 + ((cls>3+(1-1))+(cls>5+(1-1))+(cls>7+(1-1))+(cls>8+(1-1))+(cls>11+(1-1)));
  dig_t = dig_t*2 + ((p0>0+(1-1)));
  X.a = dig_a;  X.b = dig_b;  X.c = dig_c;  X.d = dig_d;  X.s = dig_s;  X.f = dig_f;  X.m = dig_m;  X.t = dig_t;
}

void tc_make_sgn( const tc_dv & D, tcx & X ) {
  int sgn_a, sgn_b, sgn_c, sgn_d, sgn_s, sgn_f, sgn_m, sgn_t;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax, mg = D.mg,
      pq = D.pq, sq = D.sq;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) mg;    (void) pq;   (void) sq;

  sgn_a = 0;
  sgn_a = sgn_a*5 + ((cls>5+(1-1))+(cls>6+(1-1))+(cls>7+(1-1))+(cls>8+(1-1)));
  sgn_a = sgn_a*6 + ((p0s>0+(-3-1))+(p0s>2+(-3-1))+(p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>5+(-3-1)));
  sgn_a = sgn_a*2 + ((mg>2+(1-1)));
  sgn_a = sgn_a*3 + ((band>2+(1-1))+(band>4+(1-1)));
  sgn_a = sgn_a*9 + TC_sgn_a_sq[__min(32,__max(0,sq-(1-1)))];

  sgn_b = 0;
  sgn_b = sgn_b*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  sgn_b = sgn_b*2 + ((vpos>0+(1-1)));
  sgn_b = sgn_b*6 + ((p0s>1+(-3-1))+(p0s>2+(-3-1))+(p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>5+(-3-1)));
  sgn_b = sgn_b*3 + ((q1s>2+(-3-1))+(q1s>4+(-3-1)));
  sgn_b = sgn_b*3 + ((t1s>4+(-3-1))+(t1s>6+(-3-1)));
  sgn_b = sgn_b*3 + ((q2s>2+(-3-1))+(q2s>4+(-3-1)));
  sgn_b = sgn_b*3 + ((band>0+(1-1))+(band>3+(1-1)));

  sgn_c = 0;
  sgn_c = sgn_c*2 + ((pass>1+(1-1)));
  sgn_c = sgn_c*3 + ((p0s>0+(-3-1))+(p0s>6+(-3-1)));
  sgn_c = sgn_c*2 + ((q1s>4+(-3-1)));
  sgn_c = sgn_c*2 + ((q2s>4+(-3-1)));

  sgn_d = 0;
  sgn_d = sgn_d*4 + ((p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>6+(-3-1)));
  sgn_d = sgn_d*2 + ((q1s>3+(-3-1)));
  sgn_d = sgn_d*2 + ((q2s>3+(-3-1)));

  sgn_s = 0;
  sgn_s = sgn_s*3 + ((p0s>2+(-3-1))+(p0s>5+(-3-1)));

  sgn_f = 0;
  sgn_f = sgn_f*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  sgn_f = sgn_f*2 + ((vpos>0+(1-1)));
  sgn_f = sgn_f*6 + ((p0s>0+(-3-1))+(p0s>2+(-3-1))+(p0s>3+(-3-1))+(p0s>4+(-3-1))+(p0s>6+(-3-1)));
  sgn_f = sgn_f*3 + ((mg>1+(1-1))+(mg>2+(1-1)));
  sgn_f = sgn_f*9 + TC_sgn_f_sq[__min(32,__max(0,sq-(1-1)))];

  sgn_m = 0;
  sgn_m = sgn_m*3 + ((pass>0+(1-1))+(pass>1+(1-1)));
  sgn_m = sgn_m*8 + ((vpos>0+(1-1))+(vpos>1+(1-1))+(vpos>2+(1-1))+(vpos>3+(1-1))+(vpos>4+(1-1))+(vpos>5+(1-1))+(vpos>6+(1-1)));
  sgn_m = sgn_m*2 + ((mg>1+(1-1)));
  sgn_m = sgn_m*9 + TC_sgn_m_sq[__min(32,__max(0,sq-(1-1)))];

  sgn_t = 0;
  X.a = sgn_a;  X.b = sgn_b;  X.c = sgn_c;  X.d = sgn_d;  X.s = sgn_s;  X.f = sgn_f;  X.m = sgn_m;  X.t = sgn_t;
}

void tc_make_flr( int fno, int col, int p1, int n1, int raw, int blk,
                  int o1, int pn, int od, int chn,
                  int room, int hl, int lov, int hiv, int pri, int ep, tcx & X ) {
  int flr_a, flr_b, flr_c, flr_d, flr_s, flr_f, flr_m, flr_t;
  (void) fno;  (void) col;  (void) p1;  (void) n1;  (void) raw;  (void) blk;
  (void) o1;   (void) pn;   (void) od;  (void) chn;
  (void) room; (void) hl;   (void) lov; (void) hiv; (void) pri;  (void) ep;

  flr_a = 0;
  flr_a = flr_a*2 + ((fno>0+(1-1)));
  flr_a = flr_a*34 + TC_flr_a_col[__min(48,__max(0,col-(1-1)))];
  flr_a = flr_a*2 + ((n1>0+(1-1)));
  flr_a = flr_a*3 + ((hl>0+(1-1))+(hl>1+(1-1)));

  flr_b = 0;
  flr_b = flr_b*2 + ((col>6+(1-1)));
  flr_b = flr_b*13 + TC_flr_b_p1[__min(14,__max(0,p1-(1-1)))];
  flr_b = flr_b*2 + ((n1>6+(1-1)));
  flr_b = flr_b*2 + ((blk>0+(1-1)));
  flr_b = flr_b*9 + TC_flr_b_lov[__min(8,__max(0,lov-(1-1)))];
  flr_b = flr_b*12 + TC_flr_b_ep[__min(11,__max(0,ep-(1-1)))];

  flr_c = 0;
  flr_c = flr_c*8 + ((col>0+(1-1))+(col>1+(1-1))+(col>2+(1-1))+(col>3+(1-1))+(col>4+(1-1))+(col>5+(1-1))+(col>6+(1-1)));
  flr_c = flr_c*8 + ((o1>0+(1-1))+(o1>1+(1-1))+(o1>2+(1-1))+(o1>3+(1-1))+(o1>4+(1-1))+(o1>5+(1-1))+(o1>6+(1-1)));
  flr_c = flr_c*12 + TC_flr_c_ep[__min(11,__max(0,ep-(1-1)))];

  flr_d = 0;

  flr_s = 0;
  flr_s = flr_s*6 + ((col>0+(1-1))+(col>2+(1-1))+(col>3+(1-1))+(col>4+(1-1))+(col>6+(1-1)));
  flr_s = flr_s*3 + ((n1>0+(1-1))+(n1>2+(1-1)));

  flr_f = 0;

  flr_m = 0;
  flr_m = flr_m*16 + TC_flr_m_col[__min(15,__max(0,col-(1-1)))];
  flr_m = flr_m*2 + ((p1>2+(1-1)));
  flr_m = flr_m*9 + TC_flr_m_pri[__min(32,__max(0,pri-(1-1)))];

  flr_t = 0;
  flr_t = flr_t*27 + TC_flr_t_col[__min(48,__max(0,col-(1-1)))];
  X.a = flr_a;  X.b = flr_b;  X.c = flr_c;  X.d = flr_d;  X.s = flr_s;  X.f = flr_f;  X.m = flr_m;  X.t = flr_t;
}

void tc_make_cls( int rno, int band, int prev, int t1, int blk, int tn, int prev2,
                  int pq, tcx & X ) {
  int cls_a, cls_b, cls_c, cls_d, cls_s, cls_f, cls_m, cls_t;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;  (void) tn;  (void) prev2;
  (void) pq;

  cls_a = 0;
  cls_a = cls_a*2 + ((rno>0+(1-1)));
  cls_a = cls_a*8 + ((band>1+(1-1))+(band>2+(1-1))+(band>4+(1-1))+(band>6+(1-1))+(band>8+(1-1))+(band>9+(1-1))+(band>10+(1-1)));
  cls_a = cls_a*5 + ((prev>1+(1-1))+(prev>3+(1-1))+(prev>5+(1-1))+(prev>6+(1-1)));
  cls_a = cls_a*5 + ((t1>3+(1-1))+(t1>4+(1-1))+(t1>5+(1-1))+(t1>6+(1-1)));
  cls_a = cls_a*2 + ((blk>0+(1-1)));

  cls_b = 0;
  cls_b = cls_b*8 + ((band>0+(1-1))+(band>5+(1-1))+(band>6+(1-1))+(band>7+(1-1))+(band>8+(1-1))+(band>9+(1-1))+(band>10+(1-1)));
  cls_b = cls_b*6 + ((prev>0+(1-1))+(prev>1+(1-1))+(prev>2+(1-1))+(prev>4+(1-1))+(prev>6+(1-1)));
  cls_b = cls_b*4 + ((t1>0+(1-1))+(t1>1+(1-1))+(t1>2+(1-1)));
  cls_b = cls_b*2 + ((blk>0+(1-1)));

  cls_c = 0;
  cls_c = cls_c*4 + ((prev>0+(1-1))+(prev>1+(1-1))+(prev>2+(1-1)));
  cls_c = cls_c*4 + ((t1>0+(1-1))+(t1>1+(1-1))+(t1>2+(1-1)));
  cls_c = cls_c*4 + ((tn>0+(1-1))+(tn>1+(1-1))+(tn>2+(1-1)));

  cls_d = 0;
  cls_d = cls_d*4 + ((band>0+(1-1))+(band>1+(1-1))+(band>2+(1-1)));
  cls_d = cls_d*4 + ((prev>0+(1-1))+(prev>1+(1-1))+(prev>2+(1-1)));
  cls_d = cls_d*4 + ((prev2>0+(1-1))+(prev2>1+(1-1))+(prev2>2+(1-1)));

  cls_s = 0;

  cls_f = 0;

  cls_m = 0;
  cls_m = cls_m*2 + ((band>2+(1-1)));
  cls_m = cls_m*3 + ((prev>0+(1-1))+(prev>2+(1-1)));
  cls_m = cls_m*2 + ((t1>0+(1-1)));

  cls_t = 0;
  cls_t = cls_t*3 + ((band>0+(1-1))+(band>2+(1-1)));
  cls_t = cls_t*2 + ((prev>2+(1-1)));
  cls_t = cls_t*3 + ((t1>0+(1-1))+(t1>2+(1-1)));
  X.a = cls_a;  X.b = cls_b;  X.c = cls_c;  X.d = cls_d;  X.s = cls_s;  X.f = cls_f;  X.m = cls_m;  X.t = cls_t;
}

void tc_make_aux( int fld, int p1, int p2, tcx & X ) {
  int aux_a, aux_b, aux_c, aux_d, aux_s, aux_f, aux_m, aux_t;
  (void) fld;  (void) p1;  (void) p2;

  aux_a = 0;
  aux_a = aux_a*(32) + (fld);
  aux_a = aux_a*6 + ((p1>0+(1-1))+(p1>8+(1-1))+(p1>12+(1-1))+(p1>13+(1-1))+(p1>14+(1-1)));
  aux_a = aux_a*3 + ((p2>0+(1-1))+(p2>6+(1-1)));

  aux_b = 0;
  aux_b = aux_b*(32) + (fld);

  aux_c = 0;
  aux_c = aux_c*(32) + (fld);

  aux_d = 0;
  aux_d = aux_d*(32) + (fld);

  aux_s = 0;
  aux_s = aux_s*(32) + (fld);

  aux_f = 0;
  aux_f = aux_f*(32) + (fld);

  aux_m = 0;
  aux_m = aux_m*(32) + (fld);

  aux_t = 0;
  aux_t = aux_t*(32) + (fld);
  X.a = aux_a;  X.b = aux_b;  X.c = aux_c;  X.d = aux_d;  X.s = aux_s;  X.f = aux_f;  X.m = aux_m;  X.t = aux_t;
}

void tc_make_hdr( int tag, int p1, int p2, int pos, int p1r, tcx & X ) {
  int hdr_a, hdr_b, hdr_c, hdr_d, hdr_s, hdr_f, hdr_m, hdr_t;
  (void) tag;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;

  hdr_a = 0;
  hdr_a = hdr_a*(64) + (tag);
  hdr_a = hdr_a*5 + ((p1>0+(1-1))+(p1>10+(1-1))+(p1>12+(1-1))+(p1>13+(1-1)));
  hdr_a = hdr_a*8 + ((p2>0+(1-1))+(p2>1+(1-1))+(p2>2+(1-1))+(p2>3+(1-1))+(p2>4+(1-1))+(p2>5+(1-1))+(p2>6+(1-1)));
  hdr_a = hdr_a*3 + ((pos>1+(1-1))+(pos>2+(1-1)));

  hdr_b = 0;
  hdr_b = hdr_b*3 + ((p1>0+(1-1))+(p1>2+(1-1)));
  hdr_b = (hdr_b<<5) + (((((p1r)>>6)&1)<<4) + (((p1r)>>1)&15)); // 01011110

  hdr_c = 0;
  hdr_c = hdr_c*(64) + (tag);

  hdr_d = 0;
  hdr_d = hdr_d*(64) + (tag);

  hdr_s = 0;
  hdr_s = hdr_s*(64) + (tag);

  hdr_f = 0;
  hdr_f = hdr_f*(64) + (tag);

  hdr_m = 0;
  hdr_m = hdr_m*(64) + (tag);

  hdr_t = 0;
  hdr_t = hdr_t*(64) + (tag);
  hdr_t = (hdr_t<<6) + (((((p1r)>>5)&3)<<4) + ((p1r)&15)); // 01101111
  X.a = hdr_a;  X.b = hdr_b;  X.c = hdr_c;  X.d = hdr_d;  X.s = hdr_s;  X.f = hdr_f;  X.m = hdr_m;  X.t = hdr_t;
}

/*  The tables the indices address.  `_A` is the rich counter, `_B` the coarse
    one, `_S` the APM's 33 buckets per context, `_W` the mixer's two weights,
    `_T` the mantissa plane, `_G` the sign counters where a family codes signs
    under a context of their own.  Every one of them is TC_NODE (or TC_MNODE)
    rows deep per context, so where a bit sits in the cascade is context
    without having to be declared as a variable.  */






