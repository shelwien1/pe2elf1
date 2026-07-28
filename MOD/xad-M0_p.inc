
// M0: the MS iDelta quantizer.  Runs once per coded symbol.

static inline int MakeMd( int dlog ) { int Md = 0;

  Md = 0;
  Md = Md*16 + M0_d0[__min(72,__max(0,dlog-(1-1)))];
  return Md; }
