/*  The template idx2inc.pl reads alongside tsvcomp.idx.  Everything here is
    copied through into MOD/tsvcomp_p.inc with `%M%` replaced by the prefix,
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
  MakeIndex dig_a,dig_b,dig_s,dig_m,dig_t
  X.a = dig_a;  X.b = dig_b;  X.s = dig_s;  X.m = dig_m;  X.t = dig_t;
}

void tc_make_flr( int fno, int col, int p1, int n1, int raw, int blk, tcx & X ) {
  int flr_a, flr_b, flr_s, flr_m, flr_t;
  (void) fno;  (void) col;  (void) p1;  (void) n1;  (void) raw;  (void) blk;
  MakeIndex flr_a,flr_b,flr_s,flr_m,flr_t
  X.a = flr_a;  X.b = flr_b;  X.s = flr_s;  X.m = flr_m;  X.t = flr_t;
}

void tc_make_cls( int rno, int band, int prev, int t1, int blk, tcx & X ) {
  int cls_a, cls_b, cls_s, cls_m, cls_t;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;
  MakeIndex cls_a,cls_b,cls_s,cls_m,cls_t
  X.a = cls_a;  X.b = cls_b;  X.s = cls_s;  X.m = cls_m;  X.t = cls_t;
}

void tc_make_aux( int fld, int p1, int p2, tcx & X ) {
  int aux_a, aux_b, aux_s, aux_m, aux_t;
  (void) fld;  (void) p1;  (void) p2;
  MakeIndex aux_a,aux_b,aux_s,aux_m,aux_t
  X.a = aux_a;  X.b = aux_b;  X.s = aux_s;  X.m = aux_m;  X.t = aux_t;
}

void tc_make_hdr( int tag, int p1, int p2, int pos, int p1r, tcx & X ) {
  int hdr_a, hdr_b, hdr_s, hdr_m, hdr_t;
  (void) tag;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;
  MakeIndex hdr_a,hdr_b,hdr_s,hdr_m,hdr_t
  X.a = hdr_a;  X.b = hdr_b;  X.s = hdr_s;  X.m = hdr_m;  X.t = hdr_t;
}

/*  The tables the indices address.  `_A` is the rich counter, `_B` the coarse
    one, `_S` the APM's 33 buckets per context, `_W` the mixer's two weights,
    `_T` the mantissa plane, `_G` the sign counters where a family codes signs
    under a context of their own.  Every one of them is TC_NODE (or TC_MNODE)
    rows deep per context, so where a bit sits in the cascade is context
    without having to be declared as a variable.  */

Table( cm_cnt, %M%dig_A, %M%dig_a_Volume * TC_NODE );
Table( cm_cnt, %M%dig_B, %M%dig_b_Volume * TC_NODE );
Table( cm_cnt, %M%dig_T, %M%dig_t_Volume * TC_MNODE );
Table( u16,    %M%dig_S, %M%dig_s_Volume * TC_NODE * 33 );
Table( i32,    %M%dig_W, %M%dig_m_Volume * TC_NODE * 2 );
Table( cm_cnt, %M%dig_G, TC_SGN_DIG );

Table( cm_cnt, %M%flr_A, %M%flr_a_Volume * TC_NODE );
Table( cm_cnt, %M%flr_B, %M%flr_b_Volume * TC_NODE );
Table( cm_cnt, %M%flr_T, %M%flr_t_Volume * TC_MNODE );
Table( u16,    %M%flr_S, %M%flr_s_Volume * TC_NODE * 33 );
Table( i32,    %M%flr_W, %M%flr_m_Volume * TC_NODE * 2 );
Table( cm_cnt, %M%flr_G, TC_SGN_FLR );

Table( cm_cnt, %M%cls_A, %M%cls_a_Volume * TC_NODE );
Table( cm_cnt, %M%cls_B, %M%cls_b_Volume * TC_NODE );
Table( cm_cnt, %M%cls_T, %M%cls_t_Volume * TC_MNODE );
Table( u16,    %M%cls_S, %M%cls_s_Volume * TC_NODE * 33 );
Table( i32,    %M%cls_W, %M%cls_m_Volume * TC_NODE * 2 );
Table( cm_cnt, %M%cls_G, TC_SGN_CLS );

Table( cm_cnt, %M%aux_A, %M%aux_a_Volume * TC_NODE );
Table( cm_cnt, %M%aux_B, %M%aux_b_Volume * TC_NODE );
Table( cm_cnt, %M%aux_T, %M%aux_t_Volume * TC_MNODE );
Table( u16,    %M%aux_S, %M%aux_s_Volume * TC_NODE * 33 );
Table( i32,    %M%aux_W, %M%aux_m_Volume * TC_NODE * 2 );
Table( cm_cnt, %M%aux_G, TC_SGN_AUX );

Table( cm_cnt, %M%hdr_A, %M%hdr_a_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_B, %M%hdr_b_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_T, %M%hdr_t_Volume * TC_MNODE );
Table( u16,    %M%hdr_S, %M%hdr_s_Volume * TC_NODE * 33 );
Table( i32,    %M%hdr_W, %M%hdr_m_Volume * TC_NODE * 2 );
Table( cm_cnt, %M%hdr_G, TC_SGN_HDR );
