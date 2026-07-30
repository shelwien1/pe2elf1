// paq8-G0.inc -- template for the generated MOD/paq8-G0_p.inc.
//
// G0 declares parameters and mixer context selectors, no storage, so there is
// no Table() here and no MakeTables: with MakeTables absent, idx2inc.pl leaves
// the mapping/mask/constant declarations in paq8-G0_h.inc, which is where the
// consumer wants them (it needs the values before anything that uses them).
//
// Each function below is the one m.set() index it is named for.  The parameter
// names are the .idx's variable names, because that is what MakeIndex writes
// into the body; the call sites in ContextModel::p pass the same expressions
// the arithmetic used to be spelled with.

static inline int G0_MakeMX0( U32 order, U32 w4, U32 c2 ) {
  int MX0;
  MakeIndex MX0
  return MX0;
}

static inline int G0_MakeMX1( U32 order, U32 w4, U32 words1 ) {
  int MX1;
  MakeIndex MX1
  return MX1;
}

static inline int G0_MakeMX2( U32 bpos, U32 w4 ) {
  int MX2;
  MakeIndex MX2
  return MX2;
}

static inline int G0_MakeMX4( U32 order, U32 c2, U32 bpos ) {
  int MX4;
  MakeIndex MX4
  return MX4;
}
