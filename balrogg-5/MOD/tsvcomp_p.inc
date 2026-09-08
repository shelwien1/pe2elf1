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

void tc_make_dig( int rno, int pass, int band, int col, int q1, int q2,
                  int t1, int cls, int zrun, int blk, int bkq, tcx & X ) {
  int dig_a, dig_b, dig_s, dig_m, dig_t;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) q1;  (void) q2;
  (void) t1;   (void) cls;   (void) zrun;  (void) blk;  (void) bkq;

  dig_a = 0;
  dig_a = dig_a*8 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1))+(pass>3+(1-1))+(pass>4+(1-1))+(pass>5+(1-1))+(pass>6+(1-1)));
  dig_a = dig_a*6 + ((band>0+(1-1))+(band>3+(1-1))+(band>4+(1-1))+(band>7+(1-1))+(band>8+(1-1)));
  dig_a = dig_a*2 + ((col>5+(1-1)));
  dig_a = dig_a*2 + ((bkq>3+(1-1)));
  dig_a = dig_a*8 + ((cls>1+(1-1))+(cls>3+(1-1))+(cls>4+(1-1))+(cls>5+(1-1))+(cls>6+(1-1))+(cls>7+(1-1))+(cls>8+(1-1)));
  dig_a = dig_a*2 + ((zrun>3+(1-1)));
  dig_a = dig_a*2 + ((blk>0+(1-1)));

  dig_b = 0;
  dig_b = dig_b*8 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1))+(pass>3+(1-1))+(pass>4+(1-1))+(pass>5+(1-1))+(pass>6+(1-1)));
  dig_b = dig_b*4 + ((band>0+(1-1))+(band>1+(1-1))+(band>8+(1-1)));
  dig_b = dig_b*6 + ((col>1+(1-1))+(col>2+(1-1))+(col>3+(1-1))+(col>4+(1-1))+(col>6+(1-1)));
  dig_b = dig_b*6 + ((q1>0+(1-1))+(q1>1+(1-1))+(q1>2+(1-1))+(q1>3+(1-1))+(q1>4+(1-1)));
  dig_b = dig_b*3 + ((q2>0+(1-1))+(q2>3+(1-1)));
  dig_b = dig_b*2 + ((t1>0+(1-1)));
  dig_b = dig_b*3 + ((bkq>1+(1-1))+(bkq>3+(1-1)));
  dig_b = dig_b*3 + ((zrun>1+(1-1))+(zrun>2+(1-1)));

  dig_s = 0;
  dig_s = dig_s*2 + ((pass>1+(1-1)));
  dig_s = dig_s*3 + ((band>1+(1-1))+(band>2+(1-1)));
  dig_s = dig_s*4 + ((q1>0+(1-1))+(q1>1+(1-1))+(q1>2+(1-1)));
  dig_s = dig_s*4 + ((t1>0+(1-1))+(t1>1+(1-1))+(t1>2+(1-1)));
  dig_s = dig_s*2 + ((bkq>2+(1-1)));
  dig_s = dig_s*2 + ((cls>2+(1-1)));

  dig_m = 0;
  dig_m = dig_m*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  dig_m = dig_m*2 + ((bkq>2+(1-1)));

  dig_t = 0;
  dig_t = dig_t*4 + ((pass>0+(1-1))+(pass>1+(1-1))+(pass>2+(1-1)));
  dig_t = dig_t*11 + TC_dig_t_cls[__min(15,__max(0,cls-(1-1)))];
  X.a = dig_a;  X.b = dig_b;  X.s = dig_s;  X.m = dig_m;  X.t = dig_t;
}

void tc_make_flr( int fno, int col, int p1, int n1, int raw, int blk, tcx & X ) {
  int flr_a, flr_b, flr_s, flr_m, flr_t;
  (void) fno;  (void) col;  (void) p1;  (void) n1;  (void) raw;  (void) blk;

  flr_a = 0;
  flr_a = flr_a*2 + ((fno>0+(1-1)));
  flr_a = flr_a*49 + TC_flr_a_col[__min(48,__max(0,col-(1-1)))];
  flr_a = flr_a*4 + ((p1>4+(1-1))+(p1>5+(1-1))+(p1>6+(1-1)));
  flr_a = flr_a*3 + ((n1>1+(1-1))+(n1>2+(1-1)));

  flr_b = 0;
  flr_b = flr_b*12 + TC_flr_b_p1[__min(14,__max(0,p1-(1-1)))];

  flr_s = 0;
  flr_s = flr_s*8 + ((col>0+(1-1))+(col>1+(1-1))+(col>2+(1-1))+(col>3+(1-1))+(col>4+(1-1))+(col>5+(1-1))+(col>6+(1-1)));
  flr_s = flr_s*4 + ((p1>0+(1-1))+(p1>1+(1-1))+(p1>2+(1-1)));
  flr_s = flr_s*4 + ((n1>0+(1-1))+(n1>1+(1-1))+(n1>2+(1-1)));

  flr_m = 0;

  flr_t = 0;
  flr_t = flr_t*26 + TC_flr_t_col[__min(48,__max(0,col-(1-1)))];
  flr_t = flr_t*2 + ((n1>2+(1-1)));
  X.a = flr_a;  X.b = flr_b;  X.s = flr_s;  X.m = flr_m;  X.t = flr_t;
}

void tc_make_cls( int rno, int band, int prev, int t1, int blk, tcx & X ) {
  int cls_a, cls_b, cls_s, cls_m, cls_t;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;

  cls_a = 0;
  cls_a = cls_a*2 + ((rno>0+(1-1)));
  cls_a = cls_a*5 + ((band>3+(1-1))+(band>4+(1-1))+(band>7+(1-1))+(band>8+(1-1)));
  cls_a = cls_a*4 + ((t1>1+(1-1))+(t1>3+(1-1))+(t1>5+(1-1)));
  cls_a = cls_a*2 + ((blk>0+(1-1)));

  cls_b = 0;
  cls_b = cls_b*3 + ((band>0+(1-1))+(band>10+(1-1)));
  cls_b = cls_b*9 + TC_cls_b_prev[__min(15,__max(0,prev-(1-1)))];
  cls_b = cls_b*3 + ((t1>0+(1-1))+(t1>2+(1-1)));
  cls_b = cls_b*2 + ((blk>0+(1-1)));

  cls_s = 0;
  cls_s = cls_s*4 + ((band>0+(1-1))+(band>1+(1-1))+(band>2+(1-1)));
  cls_s = cls_s*4 + ((prev>0+(1-1))+(prev>1+(1-1))+(prev>2+(1-1)));

  cls_m = 0;

  cls_t = 0;
  cls_t = cls_t*4 + ((band>0+(1-1))+(band>1+(1-1))+(band>2+(1-1)));
  cls_t = cls_t*3 + ((t1>1+(1-1))+(t1>2+(1-1)));
  X.a = cls_a;  X.b = cls_b;  X.s = cls_s;  X.m = cls_m;  X.t = cls_t;
}

void tc_make_aux( int fld, int p1, int p2, tcx & X ) {
  int aux_a, aux_b, aux_s, aux_m, aux_t;
  (void) fld;  (void) p1;  (void) p2;

  aux_a = 0;
  aux_a = aux_a*(32) + (fld);
  aux_a = aux_a*7 + ((p1>0+(1-1))+(p1>3+(1-1))+(p1>8+(1-1))+(p1>12+(1-1))+(p1>13+(1-1))+(p1>14+(1-1)));
  aux_a = aux_a*2 + ((p2>0+(1-1)));

  aux_b = 0;
  aux_b = aux_b*(32) + (fld);

  aux_s = 0;
  aux_s = aux_s*(32) + (fld);

  aux_m = 0;
  aux_m = aux_m*(32) + (fld);

  aux_t = 0;
  aux_t = aux_t*(32) + (fld);
  X.a = aux_a;  X.b = aux_b;  X.s = aux_s;  X.m = aux_m;  X.t = aux_t;
}

void tc_make_hdr( int tag, int p1, int p2, int pos, int p1r, tcx & X ) {
  int hdr_a, hdr_b, hdr_s, hdr_m, hdr_t;
  (void) tag;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;

  hdr_a = 0;
  hdr_a = hdr_a*(64) + (tag);
  hdr_a = hdr_a*4 + ((p1>1+(1-1))+(p1>11+(1-1))+(p1>13+(1-1)));
  hdr_a = hdr_a*8 + ((p2>0+(1-1))+(p2>1+(1-1))+(p2>2+(1-1))+(p2>3+(1-1))+(p2>4+(1-1))+(p2>5+(1-1))+(p2>6+(1-1)));
  hdr_a = hdr_a*4 + ((pos>0+(1-1))+(pos>1+(1-1))+(pos>2+(1-1)));

  hdr_b = 0;
  hdr_b = hdr_b*3 + ((p1>1+(1-1))+(p1>2+(1-1)));
  hdr_b = hdr_b*2 + ((pos>0+(1-1)));
  hdr_b = (hdr_b<<7) + (((((p1r)>>6)&3)<<5) + ((p1r)&31)); // 11011111

  hdr_s = 0;
  hdr_s = hdr_s*(64) + (tag);

  hdr_m = 0;
  hdr_m = hdr_m*(64) + (tag);

  hdr_t = 0;
  hdr_t = hdr_t*(64) + (tag);
  hdr_t = (hdr_t<<6) + (((((p1r)>>7)&1)<<5) + ((p1r)&31)); // 10011111
  X.a = hdr_a;  X.b = hdr_b;  X.s = hdr_s;  X.m = hdr_m;  X.t = hdr_t;
}

/*  The tables the indices address.  `_A` is the rich counter, `_B` the coarse
    one, `_S` the APM's 33 buckets per context, `_W` the mixer's two weights,
    `_T` the mantissa plane, `_G` the sign counters where a family codes signs
    under a context of their own.  Every one of them is TC_NODE (or TC_MNODE)
    rows deep per context, so where a bit sits in the cascade is context
    without having to be declared as a variable.  */





