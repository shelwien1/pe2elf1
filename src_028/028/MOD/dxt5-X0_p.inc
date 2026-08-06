
// X0: dxt3 nibbles + tail contexts


static inline uint MakeX1( uint xl ) { int X1 = 0;

  X1 = 0;
  X0_xa.inc( X1, xl );
  return X1; }
static inline uint MakeX2( uint xu ) { int X2 = 0;

  X2 = 0;
  X0_xb.inc( X2, xu );
  return X2; }
static inline uint MakeX3( uint p2 ) { int X3 = 0;

  X3 = 0;
  X0_xc.inc( X3, p2 );
  return X3; }
static inline uint MakeX4( uint xp, uint xpr, uint xv ) { int X4 = 0;

  X4 = 0;
  X0_xp.inc( X4, xp );
  X0_xr.inc( X4, xpr );
  X0_xv.inc( X4, xv );
  return X4; }
static inline uint MakeX5( uint xp, uint xl, uint xu ) { int X5 = 0;

  X5 = 0;
  X0_x5p.inc( X5, xp );
  X0_x5l.inc( X5, xl );
  X0_x5u.inc( X5, xu );
  return X5; }
static inline uint MakeTL( uint prev ) { int TL = 0;

  TL = 0;
  X0_tp.inc( TL, prev );
  return TL; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in X0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits X0_T and binds its
// CtrTab / Mixer / APM views to these.
