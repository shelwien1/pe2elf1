// paq8-R0.inc -- template for the generated MOD/paq8-R0_p.inc.
//
// RecordModel's tables are ContextMap members sized by their own template
// arguments, so no Table() and no MakeTables; the mapping declarations stay in
// paq8-R0_h.inc where the consumer needs them first.
//
// Both builders take the already-bucketed second field as a parameter rather
// than computing it: the gap is min()'d against an IDX cap and shifted by an
// IDX amount, and the age goes through llog(), none of which an Index can
// spell.  What the Index owns is the PACKING -- how many bits the byte gets and
// how many are left for what happened last time it was seen -- which is what
// the hand-written shift used to state without saying so.

static inline int R0_MakeCm0( U32 c, U32 gap ) {
  int Cm0;
  MakeIndex Cm0
  return Cm0;
}

static inline int R0_MakeCm1( U32 w, U32 age ) {
  int Cm1;
  MakeIndex Cm1
  return Cm1;
}
