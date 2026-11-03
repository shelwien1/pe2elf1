/*  The template idx2inc.pl reads alongside tsvcomp-flr.idx: the flr family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/l3/tsvcomp-flr_p.inc with
    `L3_flr_` replaced by the level's prefix -- `L3_flr_` -- `MakeIndex <a,b,...>`
    replaced by the statements that build those indices, and `Table( type,
    name, size )` lifted out into the L3_flr_T struct in
    MOD/l3/tsvcomp-flr_h.inc.  The index builder is L3_flr_make, which the
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

void L3_flr_make( int fno, int col, int p1, int fy, int raw, int blk,
                  int o1, int prd, int od, int chn,
                  int room, int hl, int lov, int hiv, int pri, int ep, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) fno;  (void) col;  (void) p1;  (void) fy;  (void) raw;  (void) blk;
  (void) o1;   (void) prd;  (void) od;  (void) chn;
  (void) room; (void) hl;   (void) lov; (void) hiv; (void) pri;  (void) ep;

  a = 0;
  a = a*2 + ((fno>0+(1-1)));
  a = a*20 + L3_flr_a_col[__min(48,__max(0,col-(1-1)))];
  a = a*2 + ((prd>3+(1-1)));
  a = a*3 + ((hl>0+(1-1))+(hl>1+(1-1)));
  a = a*6 + L3_flr_a_pri[__min(32,__max(0,pri-(1-1)))];

  b = 0;
  b = b*4 + L3_flr_b_col[__min(7,__max(0,col-(1-1)))];
  b = b*8 + L3_flr_b_p1[__min(14,__max(0,p1-(1-1)))];
  b = b*2 + ((blk>0+(1-1)));
  b = b*2 + ((prd>2+(1-1)));
  b = b*4 + L3_flr_b_od[__min(9,__max(0,od-(-4-1)))];
  b = b*2 + ((chn>0+(1-1)));
  b = b*3 + ((lov>2+(1-1))+(lov>6+(1-1)));
  b = b*2 + ((hiv>0+(1-1)));
  b = b*6 + L3_flr_b_pri[__min(32,__max(0,pri-(1-1)))];
  b = b*3 + ((ep>7+(1-1))+(ep>9+(1-1)));

  c = 0;
  c = c*3 + ((col>4+(1-1))+(col>5+(1-1)));
  c = c*5 + L3_flr_c_o1[__min(7,__max(0,o1-(1-1)))];
  c = c*4 + L3_flr_c_od[__min(9,__max(0,od-(-4-1)))];
  c = c*2 + ((chn>0+(1-1)));
  c = c*2 + ((hiv>0+(1-1)));
  c = c*4 + L3_flr_c_pri[__min(32,__max(0,pri-(1-1)))];
  c = c*8 + L3_flr_c_ep[__min(11,__max(0,ep-(1-1)))];

  d = 0;
  d = d*2 + ((o1>5+(1-1)));
  d = d*3 + ((od>0+(-4-1))+(od>8+(-4-1)));
  d = d*3 + ((lov>0+(1-1))+(lov>1+(1-1)));
  d = d*5 + L3_flr_d_pri[__min(32,__max(0,pri-(1-1)))];
  d = d*6 + L3_flr_d_ep[__min(11,__max(0,ep-(1-1)))];

  s = 0;
  s = s*4 + L3_flr_s_col[__min(7,__max(0,col-(1-1)))];
  s = s*3 + ((fy>0+(1-1))+(fy>2+(1-1)));
  s = s*3 + ((od>3+(-4-1))+(od>7+(-4-1)));
  s = s*2 + ((hl>1+(1-1)));
  s = s*4 + L3_flr_s_lov[__min(8,__max(0,lov-(1-1)))];
  s = s*2 + ((hiv>7+(1-1)));
  s = s*13 + L3_flr_s_pri[__min(32,__max(0,pri-(1-1)))];
  s = s*3 + ((ep>5+(1-1))+(ep>9+(1-1)));

  f = 0;
  f = f*3 + ((col>3+(1-1))+(col>5+(1-1)));
  f = f*3 + ((fy>0+(1-1))+(fy>2+(1-1)));
  f = f*2 + ((o1>6+(1-1)));
  f = f*3 + ((prd>3+(1-1))+(prd>4+(1-1)));
  f = f*3 + ((od>0+(-4-1))+(od>8+(-4-1)));
  f = f*2 + ((hl>1+(1-1)));
  f = f*2 + ((lov>6+(1-1)));
  f = f*3 + ((hiv>1+(1-1))+(hiv>5+(1-1)));
  f = f*7 + L3_flr_f_pri[__min(32,__max(0,pri-(1-1)))];
  f = f*4 + L3_flr_f_ep[__min(11,__max(0,ep-(1-1)))];

  m = 0;
  m = m*8 + L3_flr_m_col[__min(15,__max(0,col-(1-1)))];
  m = m*2 + ((lov>0+(1-1)));
  m = m*2 + ((hiv>0+(1-1)));
  m = m*8 + L3_flr_m_pri[__min(32,__max(0,pri-(1-1)))];

  t = 0;
  t = t*14 + L3_flr_t_col[__min(48,__max(0,col-(1-1)))];
  t = t*2 + ((hl>1+(1-1)));
  t = t*5 + L3_flr_t_pri[__min(32,__max(0,pri-(1-1)))];
  t = t*2 + ((ep>10+(1-1)));

  n = 0;
  n = n*5 + L3_flr_n_col[__min(48,__max(0,col-(1-1)))];
  n = n*4 + L3_flr_n_p1[__min(3,__max(0,p1-(1-1)))];
  n = n*9 + L3_flr_n_ep[__min(11,__max(0,ep-(1-1)))];

  g = 0;
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

