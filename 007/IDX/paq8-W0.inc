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
  MakeIndex Bb34
  return Bb34;
}

static inline int W0_MakeCca( U32 col, U32 c, U32 above ) {
  int Cca;
  MakeIndex Cca
  return Cca;
}

static inline int W0_MakeCc( U32 col, U32 c ) {
  int Cc;
  MakeIndex Cc
  return Cc;
}

static inline int W0_MakeCsp( U32 c, U32 sc ) {
  int Csp;
  MakeIndex Csp
  return Csp;
}

static inline int W0_MakeWsp( U32 wordcount, U32 spacecount ) {
  int Wsp;
  MakeIndex Wsp
  return Wsp;
}

static inline int W0_MakeBp5( U32 b5, U32 c ) {
  int Bp5;
  MakeIndex Bp5
  return Bp5;
}

static inline int W0_MakeBp6( U32 b6, U32 c ) {
  int Bp6;
  MakeIndex Bp6
  return Bp6;
}

static inline int W0_MakeBp8( U32 b8, U32 b4 ) {
  int Bp8;
  MakeIndex Bp8
  return Bp8;
}
