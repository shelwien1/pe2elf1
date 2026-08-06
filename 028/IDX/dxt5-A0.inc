
// A0: alpha endpoint literal contexts


static inline uint MakeAE0W( uint Wa0 ) { int AE0W = 0;
  MakeIndex AE0W
  return AE0W; }
static inline uint MakeAE0N( uint Na0 ) { int AE0N = 0;
  MakeIndex AE0N
  return AE0N; }
static inline uint MakeAE0P( uint p2 ) { int AE0P = 0;
  MakeIndex AE0P
  return AE0P; }
static inline uint MakeAE0M( uint med0 ) { int AE0M = 0;
  MakeIndex AE0M
  return AE0M; }
static inline uint MakeAE0E( uint emx ) { int AE0E = 0;
  MakeIndex AE0E
  return AE0E; }

static inline uint MakeAE1E( uint emn ) { int AE1E = 0;
  MakeIndex AE1E
  return AE1E; }

static inline uint MakeAE0L( uint lp0 ) { int AE0L = 0;
  MakeIndex AE0L
  return AE0L; }

static inline uint MakeAE1L( uint lp1 ) { int AE1L = 0;
  MakeIndex AE1L
  return AE1L; }

static inline uint MakeAE1W( uint Wa1 ) { int AE1W = 0;
  MakeIndex AE1W
  return AE1W; }
static inline uint MakeAE1A( uint a0c ) { int AE1A = 0;
  MakeIndex AE1A
  return AE1A; }
static inline uint MakeAE1P( uint p3 ) { int AE1P = 0;
  MakeIndex AE1P
  return AE1P; }
static inline uint MakeAE1M( uint med1 ) { int AE1M = 0;
  MakeIndex AE1M
  return AE1M; }
static inline uint MakeAE0H( uint hh, uint hv ) { int AE0H = 0;
  MakeIndex AE0H
  return AE0H; }
static inline uint MakeAE1H( uint hh, uint hv ) { int AE1H = 0;
  MakeIndex AE1H
  return AE1H; }
static inline uint MakeAE0P2( uint amx, uint amn ) { int AE0P2 = 0;
  MakeIndex AE0P2
  return AE0P2; }
static inline uint MakeAE1P2( uint amx, uint amn ) { int AE1P2 = 0;
  MakeIndex AE1P2
  return AE1P2; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
Table( Ctr, %M%tA0W, %M%AE0W_Volume * 256 );
Table( Ctr, %M%tA0N, %M%AE0N_Volume * 256 );
Table( Ctr, %M%tA0P, %M%AE0P_Volume * 256 );
Table( Ctr, %M%tA0o, 256 );
Table( Ctr, %M%tA0M, %M%AE0M_Volume * 256 );
Table( Ctr, %M%tA0E, %M%AE0E_Volume * 256 );
Table( Ctr, %M%tA0L, %M%AE0L_Volume * 256 );
Table( Ctr, %M%tA1L, %M%AE1L_Volume * 256 );
Table( Ctr, %M%tA1E, %M%AE1E_Volume * 256 );
Table( Ctr, %M%tA0H, %M%AE0H_Volume * 256 );
Table( Ctr, %M%tA0P2, %M%AE0P2_Volume * 256 );
Table( Ctr, %M%tA1W, %M%AE1W_Volume * 256 );
Table( Ctr, %M%tA1A, %M%AE1A_Volume * 256 );
Table( Ctr, %M%tA1P, %M%AE1P_Volume * 256 );
Table( Ctr, %M%tA1o, 256 );
Table( Ctr, %M%tA1M, %M%AE1M_Volume * 256 );
Table( Ctr, %M%tA1H, %M%AE1H_Volume * 256 );
Table( Ctr, %M%tA1P2, %M%AE1P2_Volume * 256 );
Table( int, %M%mxA0w, 16 * 256 );   // + raster match/activity rows   // D6 added a tree row: 8 counters + bias
Table( int, %M%mxA1w, 10 * 256 );
