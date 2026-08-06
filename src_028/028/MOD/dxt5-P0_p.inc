
// P0: mode-based block (BC7/BC6H) byte contexts

static inline uint MakeP1( uint pw, uint pk ) { int P1 = 0;

  P1 = 0;
  P1 = (P1<<8) + (((pw)&255)); // 11111111
  P1 = (P1<<4) + (((pk)&15)); // 1111
  return P1; }
static inline uint MakeP2( uint pn, uint pk ) { int P2 = 0;

  P2 = 0;
  P2 = (P2<<8) + (((pn)&255)); // 11111111
  P2 = (P2<<4) + (((pk)&15)); // 1111
  return P2; }
static inline uint MakeP3( uint pp, uint pk ) { int P3 = 0;

  P3 = 0;
  P3 = (P3<<8) + (((pp)&255)); // 11111111
  P3 = (P3<<4) + (((pk)&15)); // 1111
  return P3; }
static inline uint MakeP4( uint pg, uint pk ) { int P4 = 0;

  P4 = 0;
  P4 = (P4<<1) + (((pg)&1)); // 00000001
  P4 = (P4<<4) + (((pk)&15)); // 1111
  return P4; }
static inline uint MakeP5( uint pm, uint pk ) { int P5 = 0;

  P5 = 0;
  P5 = (P5<<4) + (((pm)&15)); // 1111
  P5 = (P5<<4) + (((pk)&15)); // 1111
  return P5; }
static inline uint MakeP6( uint pw, uint pn, uint pm ) { int P6 = 0;

  P6 = 0;
  P6 = (P6<<6) + ((((pw)>>1)&63)); // 01111110
  P6 = (P6<<6) + ((((pn)>>2)&63)); // 11111100
  P6 = (P6<<3) + (((pm)&7)); // 0111
  return P6; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in P0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits P0_T and binds its
// CtrTab / Mixer / APM views to these.
