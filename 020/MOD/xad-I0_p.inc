
// I0: the IMA step-index quantizer.  Runs once per coded symbol.

static inline int MakeIb( int sidx ) { int Ib = 0;

  Ib = 0;
  Ib = Ib*89 + I0_s0[__min(88,__max(0,sidx-(1-1)))];
  return Ib; }
