
// C0: shared context quantizers.

static inline int MakeMl( int mlen ) { int Ml = 0;

  Ml = 0;
  Ml = Ml*17 + C0_l0[__min(24,__max(0,mlen-(1-1)))];
  return Ml; }
