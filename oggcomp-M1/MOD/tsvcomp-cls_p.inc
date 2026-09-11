/*  The template idx2inc.pl reads alongside tsvcomp-cls.idx: the cls family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/tsvcomp-cls_p.inc with `TC_cls_`
    replaced by the prefix -- `TC_cls_` -- `MakeIndex <a,b,...>` replaced by the
    statements that build those indices, and `Table( type, name, size )` lifted
    out into the TC_cls_T struct in MOD/tsvcomp-cls_h.inc.

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

void tc_make_cls( int rno, int band, int prev, int t1, int blk, int tn, int prev2,
                  int pq, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) rno;  (void) band;  (void) prev;  (void) t1;  (void) blk;  (void) tn;  (void) prev2;
  (void) pq;

  a = 0;
  a = a*2 + ((rno>0+(1-1)));
  a = a*4 + TC_cls_a_band[__min(15,__max(0,band-(1-1)))];
  a = a*3 + ((prev>1+(1-1))+(prev>6+(1-1)));
  a = a*5 + TC_cls_a_t1[__min(7,__max(0,t1-(1-1)))];
  a = a*2 + ((blk>0+(1-1)));
  a = a*3 + ((prev2>0+(1-1))+(prev2>1+(1-1)));

  b = 0;
  b = b*10 + TC_cls_b_band[__min(15,__max(0,band-(1-1)))];
  b = b*5 + TC_cls_b_prev[__min(15,__max(0,prev-(1-1)))];
  b = b*3 + ((t1>1+(1-1))+(t1>2+(1-1)));
  b = b*2 + ((blk>0+(1-1)));
  b = b*2 + ((tn>2+(1-1)));
  b = b*2 + ((prev2>2+(1-1)));
  b = b*2 + ((pq>23+(1-1)));

  c = 0;
  c = c*3 + ((band>0+(1-1))+(band>2+(1-1)));
  c = c*3 + ((prev>0+(1-1))+(prev>2+(1-1)));
  c = c*4 + TC_cls_c_t1[__min(3,__max(0,t1-(1-1)))];
  c = c*2 + ((tn>0+(1-1)));
  c = c*3 + ((prev2>0+(1-1))+(prev2>1+(1-1)));
  c = c*7 + TC_cls_c_pq[__min(32,__max(0,pq-(1-1)))];

  d = 0;
  d = d*2 + ((band>1+(1-1)));
  d = d*4 + TC_cls_d_prev[__min(3,__max(0,prev-(1-1)))];
  d = d*3 + ((t1>0+(1-1))+(t1>2+(1-1)));
  d = d*2 + ((tn>0+(1-1)));
  d = d*3 + ((pq>16+(1-1))+(pq>19+(1-1)));

  s = 0;
  s = s*4 + TC_cls_s_band[__min(3,__max(0,band-(1-1)))];
  s = s*4 + TC_cls_s_tn[__min(3,__max(0,tn-(1-1)))];
  s = s*3 + ((prev2>0+(1-1))+(prev2>2+(1-1)));
  s = s*5 + TC_cls_s_pq[__min(32,__max(0,pq-(1-1)))];

  f = 0;

  m = 0;
  m = m*2 + ((band>2+(1-1)));
  m = m*3 + ((prev>0+(1-1))+(prev>2+(1-1)));
  m = m*2 + ((t1>0+(1-1)));
  m = m*6 + TC_cls_m_pq[__min(32,__max(0,pq-(1-1)))];

  t = 0;
  t = t*2 + ((band>0+(1-1)));
  t = t*3 + ((prev>1+(1-1))+(prev>2+(1-1)));
  t = t*3 + ((t1>0+(1-1))+(t1>2+(1-1)));
  t = t*3 + ((tn>1+(1-1))+(tn>2+(1-1)));
  t = t*3 + ((pq>13+(1-1))+(pq>14+(1-1)));

  n = 0;
  n = n*4 + TC_cls_n_band[__min(3,__max(0,band-(1-1)))];
  n = n*2 + ((t1>2+(1-1)));
  n = n*2 + ((tn>2+(1-1)));
  n = n*9 + TC_cls_n_pq[__min(32,__max(0,pq-(1-1)))];

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

