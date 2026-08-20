
// M0: the MS iDelta quantizer.  Runs once per coded symbol.

static inline int MakeMd( int dlog ) { int Md = 0;

  Md = 0;
  M0_d0.inc( Md, dlog );
  return Md; }
