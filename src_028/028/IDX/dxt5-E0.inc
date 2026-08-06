
// E0: color endpoint literal contexts


static inline uint MakeE0W( uint w ) { int E0W = 0;
  MakeIndex E0W
  return E0W; }
static inline uint MakeE0N( uint n ) { int E0N = 0;
  MakeIndex E0N
  return E0N; }
static inline uint MakeE0P( uint p2 ) { int E0P = 0;
  MakeIndex E0P
  return E0P; }
static inline uint MakeE0Q( uint q ) { int E0Q = 0;
  MakeIndex E0Q
  return E0Q; }
static inline uint MakeE0J( uint w, uint n ) { int E0J = 0;
  MakeIndex E0J
  return E0J; }
static inline uint MakeE0E( uint eex ) { int E0E = 0;
  MakeIndex E0E
  return E0E; }

static inline uint MakeE1E( uint een ) { int E1E = 0;
  MakeIndex E1E
  return E1E; }

static inline uint MakeE0M( uint med ) { int E0M = 0;
  MakeIndex E0M
  return E0M; }
static inline uint MakeE1A( uint ea, uint lum ) { int E1A = 0;
  MakeIndex E1A
  return E1A; }
static inline uint MakeE1W( uint w ) { int E1W = 0;
  MakeIndex E1W
  return E1W; }
static inline uint MakeE1P( uint p2 ) { int E1P = 0;
  MakeIndex E1P
  return E1P; }
static inline uint MakeE1Q( uint q ) { int E1Q = 0;
  MakeIndex E1Q
  return E1Q; }
static inline uint MakeE1J( uint ea, uint w ) { int E1J = 0;
  MakeIndex E1J
  return E1J; }
static inline uint MakeE1M( uint med ) { int E1M = 0;
  MakeIndex E1M
  return E1M; }
static inline uint MakeEX1( uint e0r ) { int EX1 = 0;
  MakeIndex EX1
  return EX1; }
static inline uint MakeEX2( uint e0r, uint e0g ) { int EX2 = 0;
  MakeIndex EX2
  return EX2; }
static inline uint MakeE0H( uint h, uint hv ) { int E0H = 0;
  MakeIndex E0H
  return E0H; }
static inline uint MakeE1H( uint h, uint hv ) { int E1H = 0;
  MakeIndex E1H
  return E1H; }
static inline uint MakeE0PP( uint px, uint pn, uint qv ) { int E0PP = 0;
  MakeIndex E0PP
  return E0PP; }
static inline uint MakeE1PP( uint px, uint pn, uint qv ) { int E1PP = 0;
  MakeIndex E1PP
  return E1PP; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
// channel c row j; widths 32/64/32 per r,g,b -- see chw[] in Codec::init
Table( Ctr, %M%tE09, %M%E0E_Volume * 32 );
Table( Ctr, %M%tE19, %M%E0E_Volume * 64 );
Table( Ctr, %M%tE29, %M%E0E_Volume * 32 );
Table( Ctr, %M%tE39, %M%E1E_Volume * 32 );
Table( Ctr, %M%tE49, %M%E1E_Volume * 64 );
Table( Ctr, %M%tE59, %M%E1E_Volume * 32 );
Table( Ctr, %M%tE00, %M%E0W_Volume * 32 );
Table( Ctr, %M%tE01, %M%E0N_Volume * 32 );
Table( Ctr, %M%tE02, %M%E0P_Volume * 32 );
Table( Ctr, %M%tE03, 1 * 32 );
Table( Ctr, %M%tE04, %M%E0Q_Volume * 32 );
Table( Ctr, %M%tE05, %M%E0J_Volume * 32 );
Table( Ctr, %M%tE06, %M%E0M_Volume * 32 );
Table( Ctr, %M%tE07, %M%E0H_Volume * 32 );
Table( Ctr, %M%tE08, %M%E0PP_Volume * 32 );
Table( int, %M%mxE0w, 11 * 32 );
Table( Ctr, %M%tE10, %M%E0W_Volume * 64 );
Table( Ctr, %M%tE11, %M%E0N_Volume * 64 );
Table( Ctr, %M%tE12, %M%E0P_Volume * 64 );
Table( Ctr, %M%tE13, %M%EX1_Volume * 64 );
Table( Ctr, %M%tE14, %M%E0Q_Volume * 64 );
Table( Ctr, %M%tE15, %M%E0J_Volume * 64 );
Table( Ctr, %M%tE16, %M%E0M_Volume * 64 );
Table( Ctr, %M%tE17, %M%E0H_Volume * 64 );
Table( Ctr, %M%tE18, %M%E0PP_Volume * 64 );
Table( int, %M%mxE1w, 11 * 64 );
Table( Ctr, %M%tE20, %M%E0W_Volume * 32 );
Table( Ctr, %M%tE21, %M%E0N_Volume * 32 );
Table( Ctr, %M%tE22, %M%E0P_Volume * 32 );
Table( Ctr, %M%tE23, %M%EX2_Volume * 32 );
Table( Ctr, %M%tE24, %M%E0Q_Volume * 32 );
Table( Ctr, %M%tE25, %M%E0J_Volume * 32 );
Table( Ctr, %M%tE26, %M%E0M_Volume * 32 );
Table( Ctr, %M%tE27, %M%E0H_Volume * 32 );
Table( Ctr, %M%tE28, %M%E0PP_Volume * 32 );
Table( int, %M%mxE2w, 11 * 32 );
Table( Ctr, %M%tE30, %M%E1A_Volume * 32 );
Table( Ctr, %M%tE31, %M%E1W_Volume * 32 );
Table( Ctr, %M%tE32, %M%E1P_Volume * 32 );
Table( Ctr, %M%tE33, 1 * 32 );
Table( Ctr, %M%tE34, %M%E1Q_Volume * 32 );
Table( Ctr, %M%tE35, %M%E1J_Volume * 32 );
Table( Ctr, %M%tE36, %M%E1M_Volume * 32 );
Table( Ctr, %M%tE37, %M%E1H_Volume * 32 );
Table( Ctr, %M%tE38, %M%E1PP_Volume * 32 );
Table( int, %M%mxE3w, 11 * 32 );
Table( Ctr, %M%tE40, %M%E1A_Volume * 64 );
Table( Ctr, %M%tE41, %M%E1W_Volume * 64 );
Table( Ctr, %M%tE42, %M%E1P_Volume * 64 );
Table( Ctr, %M%tE43, %M%EX1_Volume * 64 );
Table( Ctr, %M%tE44, %M%E1Q_Volume * 64 );
Table( Ctr, %M%tE45, %M%E1J_Volume * 64 );
Table( Ctr, %M%tE46, %M%E1M_Volume * 64 );
Table( Ctr, %M%tE47, %M%E1H_Volume * 64 );
Table( Ctr, %M%tE48, %M%E1PP_Volume * 64 );
Table( int, %M%mxE4w, 11 * 64 );
Table( Ctr, %M%tE50, %M%E1A_Volume * 32 );
Table( Ctr, %M%tE51, %M%E1W_Volume * 32 );
Table( Ctr, %M%tE52, %M%E1P_Volume * 32 );
Table( Ctr, %M%tE53, %M%EX2_Volume * 32 );
Table( Ctr, %M%tE54, %M%E1Q_Volume * 32 );
Table( Ctr, %M%tE55, %M%E1J_Volume * 32 );
Table( Ctr, %M%tE56, %M%E1M_Volume * 32 );
Table( Ctr, %M%tE57, %M%E1H_Volume * 32 );
Table( Ctr, %M%tE58, %M%E1PP_Volume * 32 );
Table( int, %M%mxE5w, 11 * 32 );
