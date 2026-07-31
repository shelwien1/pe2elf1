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

  MX0 = 0;
  MX0 = MX0*(9) + (order);
  MX0 = (MX0<<4) + ((((w4)>>4)&15)); // 11110000
  MX0 = (MX0<<4) + ((((c2)>>4)&15)); // 11110000
  return MX0;
}

static inline int G0_MakeMX1( U32 order, U32 w4, U32 words1 ) {
  int MX1;

  MX1 = 0;
  MX1 = MX1*(9) + (order);
  MX1 = (MX1<<2) + (((w4)&3)); // 00000011
  MX1 = (MX1<<6) + (((words1)&63)); // 00111111
  return MX1;
}

static inline int G0_MakeMX2( U32 bpos, U32 w4 ) {
  int MX2;

  MX2 = 0;
  MX2 = MX2*(8) + (bpos);
  MX2 = (MX2<<8) + (((w4)&255)); // 11111111
  return MX2;
}

static inline int G0_MakeMX4( U32 order, U32 c2, U32 bpos ) {
  int MX4;

  MX4 = 0;
  MX4 = MX4*(9) + (order);
  MX4 = (MX4<<5) + ((((c2)>>3)&31)); // 11111000
  MX4 = MX4*(8) + (bpos);
  return MX4;
}
