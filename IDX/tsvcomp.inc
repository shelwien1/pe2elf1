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

void tc_make_dig( const tc_dv & D, tcx & X ) {
  int dig_a, dig_b, dig_c, dig_d, dig_s, dig_f, dig_m, dig_t;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax,
      pq = D.pq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) pq;   (void) ps;   (void) chn;
  (void) crun; (void) cm;
  MakeIndex dig_a,dig_b,dig_c,dig_d,dig_s,dig_f,dig_m,dig_t
  X.a = dig_a;  X.b = dig_b;  X.c = dig_c;  X.d = dig_d;  X.s = dig_s;  X.f = dig_f;  X.m = dig_m;  X.t = dig_t;
}

void tc_make_sgn( const tc_dv & D, tcx & X ) {
  int sgn_a, sgn_b, sgn_c, sgn_d, sgn_s, sgn_f, sgn_m, sgn_t;
  int rno = D.rno, pass = D.pass, band = D.band, col = D.col, vpos = D.vpos,
      q1 = D.q1, q2 = D.q2, q1s = D.q1s, q2s = D.q2s, t1 = D.t1, t1s = D.t1s,
      t2 = D.t2, n1 = D.n1, w1 = D.w1, p0 = D.p0, p0s = D.p0s, pn = D.pn,
      cls = D.cls, zrun = D.zrun, blk = D.blk, bkq = D.bkq, ax = D.ax, mg = D.mg,
      pq = D.pq, sq = D.sq, ps = D.ps, chn = D.chn, crun = D.crun, cm = D.cm;
  (void) rno;  (void) pass;  (void) band;  (void) col;  (void) vpos;
  (void) q1;   (void) q2;    (void) q1s;   (void) q2s;  (void) t1;
  (void) t1s;  (void) t2;    (void) n1;    (void) w1;   (void) p0;
  (void) p0s;  (void) pn;    (void) cls;   (void) zrun; (void) blk;
  (void) bkq;  (void) ax;    (void) mg;    (void) pq;   (void) sq;
  (void) ps;   (void) chn;   (void) crun;  (void) cm;
  MakeIndex sgn_a,sgn_b,sgn_c,sgn_d,sgn_s,sgn_f,sgn_m,sgn_t
  X.a = sgn_a;  X.b = sgn_b;  X.c = sgn_c;  X.d = sgn_d;  X.s = sgn_s;  X.f = sgn_f;  X.m = sgn_m;  X.t = sgn_t;
}

void tc_make_flr( int fno, int col, int p1, int n1, int raw, int blk,
                  int o1, int pn, int od, int chn,
                  int room, int hl, int lov, int hiv, int pri, int ep, tcx & X ) {
  int flr_a, flr_b, flr_c, flr_d, flr_s, flr_f, flr_m, flr_t;
  (void) fno;  (void) col;  (void) p1;  (void) n1;  (void) raw;  (void) blk;
  (void) o1;   (void) pn;   (void) od;  (void) chn;
  (void) room; (void) hl;   (void) lov; (void) hiv; (void) pri;  (void) ep;
  MakeIndex flr_a,flr_b,flr_c,flr_d,flr_s,flr_f,flr_m,flr_t
  X.a = flr_a;  X.b = flr_b;  X.c = flr_c;  X.d = flr_d;  X.s = flr_s;  X.f = flr_f;  X.m = flr_m;  X.t = flr_t;
}

void tc_make_cls( int rno, int band, int prev, int t1, int blk, int tn, int prev2,
                  int pq, tcx & X ) {
  int cls_a, cls_b, cls_c, cls_d, cls_s, cls_f, cls_m, cls_t;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;  (void) tn;  (void) prev2;
  (void) pq;
  MakeIndex cls_a,cls_b,cls_c,cls_d,cls_s,cls_f,cls_m,cls_t
  X.a = cls_a;  X.b = cls_b;  X.c = cls_c;  X.d = cls_d;  X.s = cls_s;  X.f = cls_f;  X.m = cls_m;  X.t = cls_t;
}

void tc_make_aux( int fld, int p1, int p2, tcx & X ) {
  int aux_a, aux_b, aux_c, aux_d, aux_s, aux_f, aux_m, aux_t;
  (void) fld;  (void) p1;  (void) p2;
  MakeIndex aux_a,aux_b,aux_c,aux_d,aux_s,aux_f,aux_m,aux_t
  X.a = aux_a;  X.b = aux_b;  X.c = aux_c;  X.d = aux_d;  X.s = aux_s;  X.f = aux_f;  X.m = aux_m;  X.t = aux_t;
}

void tc_make_hdr( int tag, int p1, int p2, int pos, int p1r, tcx & X ) {
  int hdr_a, hdr_b, hdr_c, hdr_d, hdr_s, hdr_f, hdr_m, hdr_t;
  (void) tag;  (void) p1;  (void) p2;  (void) pos;  (void) p1r;
  MakeIndex hdr_a,hdr_b,hdr_c,hdr_d,hdr_s,hdr_f,hdr_m,hdr_t
  X.a = hdr_a;  X.b = hdr_b;  X.c = hdr_c;  X.d = hdr_d;  X.s = hdr_s;  X.f = hdr_f;  X.m = hdr_m;  X.t = hdr_t;
}

/*  The tables the indices address.  `_A` is the rich counter, `_B` the coarse
    one, `_S` the APM's 33 buckets per context, `_W` the mixer's two weights,
    `_T` the mantissa plane, `_G` the sign counters where a family codes signs
    under a context of their own.  Every one of them is TC_NODE (or TC_MNODE)
    rows deep per context, so where a bit sits in the cascade is context
    without having to be declared as a variable.  */

Table( cm_cnt, %M%dig_A, %M%dig_a_Volume * TC_NODE );
Table( cm_cnt, %M%dig_B, %M%dig_b_Volume * TC_NODE );
Table( cm_cnt, %M%dig_C, %M%dig_c_Volume * TC_NODE );
Table( cm_cnt, %M%dig_D, %M%dig_d_Volume * TC_NODE );
Table( cm_cnt, %M%dig_T, %M%dig_t_Volume * TC_MNODE );
Table( u16,    %M%dig_S, %M%dig_s_Volume * TC_NODE * 33 );
Table( u8,     %M%dig_SC, %M%dig_s_Volume * TC_NODE * 33 );
Table( u16,    %M%dig_F, %M%dig_f_Volume * TC_NODE * 33 );
Table( u8,     %M%dig_FC, %M%dig_f_Volume * TC_NODE * 33 );
Table( i32,    %M%dig_W, %M%dig_m_Volume * TC_NODE * 7 );
Table( u8,     %M%dig_WC, %M%dig_m_Volume * TC_NODE );
Table( i32,    %M%dig_WM, %M%dig_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%dig_WMC, %M%dig_t_Volume * TC_MNODE );
Table( cm_cnt, %M%dig_G, TC_SGN_DIG );

Table( cm_cnt, %M%sgn_A, %M%sgn_a_Volume * TC_NODE );
Table( cm_cnt, %M%sgn_B, %M%sgn_b_Volume * TC_NODE );
Table( cm_cnt, %M%sgn_C, %M%sgn_c_Volume * TC_NODE );
Table( cm_cnt, %M%sgn_D, %M%sgn_d_Volume * TC_NODE );
Table( cm_cnt, %M%sgn_T, %M%sgn_t_Volume * TC_MNODE );
Table( u16,    %M%sgn_S, %M%sgn_s_Volume * TC_NODE * 33 );
Table( u8,     %M%sgn_SC, %M%sgn_s_Volume * TC_NODE * 33 );
Table( u16,    %M%sgn_F, %M%sgn_f_Volume * TC_NODE * 33 );
Table( u8,     %M%sgn_FC, %M%sgn_f_Volume * TC_NODE * 33 );
Table( i32,    %M%sgn_W, %M%sgn_m_Volume * TC_NODE * 7 );
Table( u8,     %M%sgn_WC, %M%sgn_m_Volume * TC_NODE );
Table( i32,    %M%sgn_WM, %M%sgn_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%sgn_WMC, %M%sgn_t_Volume * TC_MNODE );

Table( cm_cnt, %M%flr_A, %M%flr_a_Volume * TC_NODE );
Table( cm_cnt, %M%flr_B, %M%flr_b_Volume * TC_NODE );
Table( cm_cnt, %M%flr_C, %M%flr_c_Volume * TC_NODE );
Table( cm_cnt, %M%flr_D, %M%flr_d_Volume * TC_NODE );
Table( cm_cnt, %M%flr_T, %M%flr_t_Volume * TC_MNODE );
Table( u16,    %M%flr_S, %M%flr_s_Volume * TC_NODE * 33 );
Table( u8,     %M%flr_SC, %M%flr_s_Volume * TC_NODE * 33 );
Table( u16,    %M%flr_F, %M%flr_f_Volume * TC_NODE * 33 );
Table( u8,     %M%flr_FC, %M%flr_f_Volume * TC_NODE * 33 );
Table( i32,    %M%flr_W, %M%flr_m_Volume * TC_NODE * 7 );
Table( u8,     %M%flr_WC, %M%flr_m_Volume * TC_NODE );
Table( i32,    %M%flr_WM, %M%flr_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%flr_WMC, %M%flr_t_Volume * TC_MNODE );
Table( cm_cnt, %M%flr_G, TC_SGN_FLR );

Table( cm_cnt, %M%cls_A, %M%cls_a_Volume * TC_NODE );
Table( cm_cnt, %M%cls_B, %M%cls_b_Volume * TC_NODE );
Table( cm_cnt, %M%cls_C, %M%cls_c_Volume * TC_NODE );
Table( cm_cnt, %M%cls_D, %M%cls_d_Volume * TC_NODE );
Table( cm_cnt, %M%cls_T, %M%cls_t_Volume * TC_MNODE );
Table( u16,    %M%cls_S, %M%cls_s_Volume * TC_NODE * 33 );
Table( u8,     %M%cls_SC, %M%cls_s_Volume * TC_NODE * 33 );
Table( u16,    %M%cls_F, %M%cls_f_Volume * TC_NODE * 33 );
Table( u8,     %M%cls_FC, %M%cls_f_Volume * TC_NODE * 33 );
Table( i32,    %M%cls_W, %M%cls_m_Volume * TC_NODE * 7 );
Table( u8,     %M%cls_WC, %M%cls_m_Volume * TC_NODE );
Table( i32,    %M%cls_WM, %M%cls_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%cls_WMC, %M%cls_t_Volume * TC_MNODE );
Table( cm_cnt, %M%cls_G, TC_SGN_CLS );

Table( cm_cnt, %M%aux_A, %M%aux_a_Volume * TC_NODE );
Table( cm_cnt, %M%aux_B, %M%aux_b_Volume * TC_NODE );
Table( cm_cnt, %M%aux_C, %M%aux_c_Volume * TC_NODE );
Table( cm_cnt, %M%aux_D, %M%aux_d_Volume * TC_NODE );
Table( cm_cnt, %M%aux_T, %M%aux_t_Volume * TC_MNODE );
Table( u16,    %M%aux_S, %M%aux_s_Volume * TC_NODE * 33 );
Table( u8,     %M%aux_SC, %M%aux_s_Volume * TC_NODE * 33 );
Table( u16,    %M%aux_F, %M%aux_f_Volume * TC_NODE * 33 );
Table( u8,     %M%aux_FC, %M%aux_f_Volume * TC_NODE * 33 );
Table( i32,    %M%aux_W, %M%aux_m_Volume * TC_NODE * 7 );
Table( u8,     %M%aux_WC, %M%aux_m_Volume * TC_NODE );
Table( i32,    %M%aux_WM, %M%aux_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%aux_WMC, %M%aux_t_Volume * TC_MNODE );
Table( cm_cnt, %M%aux_G, TC_SGN_AUX );

Table( cm_cnt, %M%hdr_A, %M%hdr_a_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_B, %M%hdr_b_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_C, %M%hdr_c_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_D, %M%hdr_d_Volume * TC_NODE );
Table( cm_cnt, %M%hdr_T, %M%hdr_t_Volume * TC_MNODE );
Table( u16,    %M%hdr_S, %M%hdr_s_Volume * TC_NODE * 33 );
Table( u8,     %M%hdr_SC, %M%hdr_s_Volume * TC_NODE * 33 );
Table( u16,    %M%hdr_F, %M%hdr_f_Volume * TC_NODE * 33 );
Table( u8,     %M%hdr_FC, %M%hdr_f_Volume * TC_NODE * 33 );
Table( i32,    %M%hdr_W, %M%hdr_m_Volume * TC_NODE * 7 );
Table( u8,     %M%hdr_WC, %M%hdr_m_Volume * TC_NODE );
Table( i32,    %M%hdr_WM, %M%hdr_t_Volume * TC_MNODE * 3 );
Table( u8,     %M%hdr_WMC, %M%hdr_t_Volume * TC_MNODE );
Table( cm_cnt, %M%hdr_G, TC_SGN_HDR );
