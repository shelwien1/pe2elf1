
// P0: mode-based block (BC7/BC6H) byte contexts

static inline uint MakeP1( uint pw, uint pk ) { int P1 = 0;
  MakeIndex P1
  return P1; }
static inline uint MakeP2( uint pn, uint pk ) { int P2 = 0;
  MakeIndex P2
  return P2; }
static inline uint MakeP3( uint pp, uint pk ) { int P3 = 0;
  MakeIndex P3
  return P3; }
static inline uint MakeP4( uint pg, uint pk ) { int P4 = 0;
  MakeIndex P4
  return P4; }
static inline uint MakeP5( uint pm, uint pk ) { int P5 = 0;
  MakeIndex P5
  return P5; }
static inline uint MakeP6( uint pw, uint pn, uint pm ) { int P6 = 0;
  MakeIndex P6
  return P6; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
Table( Ctr, %M%tP0, %M%P1_Volume * 256 );
Table( Ctr, %M%tP1, %M%P2_Volume * 256 );
Table( Ctr, %M%tP2, %M%P3_Volume * 256 );
Table( Ctr, %M%tP3, %M%P4_Volume * 256 );
Table( Ctr, %M%tP4, %M%P5_Volume * 256 );
Table( Ctr, %M%tP5, %M%P6_Volume * 256 );
Table( int, %M%mxPw, 7 * 256 );
