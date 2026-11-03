/*  The template idx2inc.pl reads alongside tsvcomp-aux.idx: the aux family's
    index builder and the tables its indices address.

    Everything here is copied through into MOD/l9/tsvcomp-aux_p.inc with
    `L9_aux_` replaced by the level's prefix -- `L9_aux_` -- `MakeIndex <a,b,...>`
    replaced by the statements that build those indices, and `Table( type,
    name, size )` lifted out into the L9_aux_T struct in
    MOD/l9/tsvcomp-aux_h.inc.  The index builder is L9_aux_make, which the
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

void L9_aux_make( int fld, int p1, int p2, tcx & X ) {
  int a, b, c, d, s, f, m, t, n, g;
  (void) fld;  (void) p1;  (void) p2;

  a = 0;
  a = a*13 + L9_aux_a_fld[__min(18,__max(0,fld-(1-1)))];
  a = a*6 + L9_aux_a_p1[__min(15,__max(0,p1-(1-1)))];
  a = a*3 + ((p2>0+(1-1))+(p2>6+(1-1)));

  b = 0;
  b = b*9 + L9_aux_b_fld[__min(18,__max(0,fld-(1-1)))];

  c = 0;
  c = c*8 + L9_aux_c_fld[__min(18,__max(0,fld-(1-1)))];

  d = 0;
  d = d*5 + L9_aux_d_fld[__min(18,__max(0,fld-(1-1)))];

  s = 0;
  s = s*19 + L9_aux_s_fld[__min(18,__max(0,fld-(1-1)))];

  f = 0;
  f = f*19 + L9_aux_f_fld[__min(18,__max(0,fld-(1-1)))];

  m = 0;
  m = m*13 + L9_aux_m_fld[__min(18,__max(0,fld-(1-1)))];

  t = 0;
  t = t*8 + L9_aux_t_fld[__min(18,__max(0,fld-(1-1)))];

  n = 0;
  n = n*19 + L9_aux_n_fld[__min(18,__max(0,fld-(1-1)))];

  g = 0;
  g = g*4 + L9_aux_g_p1[__min(15,__max(0,p1-(1-1)))];
  g = g*4 + L9_aux_g_p2[__min(13,__max(0,p2-(1-1)))];
  X.a = a;  X.b = b;  X.c = c;  X.d = d;  X.s = s;  X.f = f;  X.m = m;  X.t = t;
  X.n = n;  X.g = g;
}

