// paq8-S0.inc -- template for the generated MOD/paq8-S0_p.inc.
//
// SparseModel declares no storage through IDX -- its tables are ContextMap and
// SmallStationaryContextMap members sized by their own template arguments -- so
// there is no Table() here and no MakeTables, and idx2inc.pl leaves the mapping
// declarations in paq8-S0_h.inc where the consumer needs them first.
//
// One Index, one function.  Like G0's builders this takes its inputs as
// parameters rather than reading the globals: the generated headers are
// included at the top of paq8hp.hpp, hundreds of lines before `words` and `w4`
// exist, and the call site passes the same expressions the hand-written
// packing was spelled with.

static inline int S0_MakeScm6( U32 words, U32 w4, U32 b1 ) {
  int Scm6;

  Scm6 = 0;
  Scm6 = (Scm6<<2) + ((((words)>>2)&3)); // 00001100
  Scm6 = (Scm6<<2) + ((((w4)>>2)&3)); // 00001100
  Scm6 = (Scm6<<4) + ((((b1)>>4)&15)); // 11110000
  return Scm6;
}
