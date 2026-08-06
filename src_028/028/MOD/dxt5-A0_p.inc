
// A0: alpha endpoint literal contexts


static inline uint MakeAE0W( uint Wa0 ) { int AE0W = 0;

  AE0W = 0;
  A0_w0.inc( AE0W, Wa0 );
  return AE0W; }
static inline uint MakeAE0N( uint Na0 ) { int AE0N = 0;

  AE0N = 0;
  A0_n0.inc( AE0N, Na0 );
  return AE0N; }
static inline uint MakeAE0P( uint p2 ) { int AE0P = 0;

  AE0P = 0;
  A0_p0.inc( AE0P, p2 );
  return AE0P; }
static inline uint MakeAE0M( uint med0 ) { int AE0M = 0;

  AE0M = 0;
  A0_m0.inc( AE0M, med0 );
  return AE0M; }
static inline uint MakeAE0E( uint emx ) { int AE0E = 0;

  AE0E = 0;
  A0_e0.inc( AE0E, emx );
  return AE0E; }

static inline uint MakeAE1E( uint emn ) { int AE1E = 0;

  AE1E = 0;
  A0_e1.inc( AE1E, emn );
  return AE1E; }

static inline uint MakeAE0L( uint lp0 ) { int AE0L = 0;

  AE0L = 0;
  A0_l0.inc( AE0L, lp0 );
  return AE0L; }

static inline uint MakeAE1L( uint lp1 ) { int AE1L = 0;

  AE1L = 0;
  A0_l1.inc( AE1L, lp1 );
  return AE1L; }

static inline uint MakeAE1W( uint Wa1 ) { int AE1W = 0;

  AE1W = 0;
  A0_w1.inc( AE1W, Wa1 );
  return AE1W; }
static inline uint MakeAE1A( uint a0c ) { int AE1A = 0;

  AE1A = 0;
  A0_a1.inc( AE1A, a0c );
  return AE1A; }
static inline uint MakeAE1P( uint p3 ) { int AE1P = 0;

  AE1P = 0;
  A0_p1.inc( AE1P, p3 );
  return AE1P; }
static inline uint MakeAE1M( uint med1 ) { int AE1M = 0;

  AE1M = 0;
  A0_m1.inc( AE1M, med1 );
  return AE1M; }
static inline uint MakeAE0H( uint hh, uint hv ) { int AE0H = 0;

  AE0H = 0;
  A0_h0.inc( AE0H, hh );
  A0_hv0.inc( AE0H, hv );
  return AE0H; }
static inline uint MakeAE1H( uint hh, uint hv ) { int AE1H = 0;

  AE1H = 0;
  A0_h1.inc( AE1H, hh );
  A0_hv1.inc( AE1H, hv );
  return AE1H; }
static inline uint MakeAE0P2( uint amx, uint amn ) { int AE0P2 = 0;

  AE0P2 = 0;
  A0_q0x.inc( AE0P2, amx );
  A0_q0n.inc( AE0P2, amn );
  return AE0P2; }
static inline uint MakeAE1P2( uint amx, uint amn ) { int AE1P2 = 0;

  AE1P2 = 0;
  A0_q1x.inc( AE1P2, amx );
  A0_q1n.inc( AE1P2, amn );
  return AE1P2; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in A0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits A0_T and binds its
// CtrTab / Mixer / APM views to these.
