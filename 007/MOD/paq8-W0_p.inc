// paq8-W0.inc -- template for the generated MOD/paq8-W0_p.inc.
//
// WordModel's storage is its own ContextMap member plus two plain arrays, none
// of them sized from a Volume, so there is no Table() here and no MakeTables;
// idx2inc.pl leaves the mapping declarations in paq8-W0_h.inc, which is where
// the consumer needs them first.
//
// Eight builders, one per tight packing.  All of them take their inputs as
// parameters: these headers are included at the top of paq8hp.hpp, long before
// `col` and `spacecount` exist, and two of the inputs (`above`, and the
// already-shifted space count) are local to WordModel::mix and could not be
// read from here at all.

static inline int W0_MakeBb34( U32 b4, U32 b3 ) {
  int Bb34;

  Bb34 = 0;
  Bb34 = (Bb34<<8) + (((b4)&255)); // 11111111
  Bb34 = (Bb34<<8) + (((b3)&255)); // 11111111
  return Bb34;
}

static inline int W0_MakeCca( U32 col, U32 c, U32 above ) {
  int Cca;

  Cca = 0;
  Cca = (Cca<<6) + (((col)&63)); // 00111111
  Cca = (Cca<<8) + (((c)&255)); // 11111111
  Cca = (Cca<<8) + (((above)&255)); // 11111111
  return Cca;
}

static inline int W0_MakeCc( U32 col, U32 c ) {
  int Cc;

  Cc = 0;
  Cc = (Cc<<6) + (((col)&63)); // 00111111
  Cc = (Cc<<8) + (((c)&255)); // 11111111
  return Cc;
}

static inline int W0_MakeCsp( U32 c, U32 sc ) {
  int Csp;

  Csp = 0;
  Csp = (Csp<<8) + (((c)&255)); // 11111111
  Csp = Csp*(64) + (sc);
  return Csp;
}

static inline int W0_MakeWsp( U32 wordcount, U32 spacecount ) {
  int Wsp;

  Wsp = 0;
  Wsp = Wsp*(1) + (wordcount);
  Wsp = Wsp*(64) + (spacecount);
  return Wsp;
}

static inline int W0_MakeBp5( U32 b5, U32 c ) {
  int Bp5;

  Bp5 = 0;
  Bp5 = (Bp5<<8) + (((b5)&255)); // 11111111
  Bp5 = (Bp5<<8) + (((c)&255)); // 11111111
  return Bp5;
}

static inline int W0_MakeBp6( U32 b6, U32 c ) {
  int Bp6;

  Bp6 = 0;
  Bp6 = (Bp6<<8) + (((b6)&255)); // 11111111
  Bp6 = (Bp6<<8) + (((c)&255)); // 11111111
  return Bp6;
}

static inline int W0_MakeBp8( U32 b8, U32 b4 ) {
  int Bp8;

  Bp8 = 0;
  Bp8 = (Bp8<<8) + (((b8)&255)); // 11111111
  Bp8 = (Bp8<<8) + (((b4)&255)); // 11111111
  return Bp8;
}
