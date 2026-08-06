
// X0: dxt3 nibbles + tail contexts


static inline uint MakeX1( uint xl ) { int X1 = 0;
  MakeIndex X1
  return X1; }
static inline uint MakeX2( uint xu ) { int X2 = 0;
  MakeIndex X2
  return X2; }
static inline uint MakeX3( uint p2 ) { int X3 = 0;
  MakeIndex X3
  return X3; }
static inline uint MakeX4( uint xp, uint xpr, uint xv ) { int X4 = 0;
  MakeIndex X4
  return X4; }
static inline uint MakeX5( uint xp, uint xl, uint xu ) { int X5 = 0;
  MakeIndex X5
  return X5; }
static inline uint MakeTL( uint prev ) { int TL = 0;
  MakeIndex TL
  return TL; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
Table( Ctr, %M%tX0, %M%X1_Volume * 16 );
Table( Ctr, %M%tX1, %M%X2_Volume * 16 );
Table( Ctr, %M%tX2, %M%X3_Volume * 16 );
Table( Ctr, %M%tX3, 16 );
Table( Ctr, %M%tX4, %M%X4_Volume * 16 );
Table( Ctr, %M%tX5, %M%X5_Volume * 16 );
Table( int, %M%mxXw, 7 * 16 );
Table( Ctr, %M%tTail, %M%TL_Volume * 256 );
