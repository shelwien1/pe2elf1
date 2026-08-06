
// E0: color endpoint literal contexts


static inline uint MakeE0W( uint w ) { int E0W = 0;

  E0W = 0;
  E0_ew.inc( E0W, w );
  return E0W; }
static inline uint MakeE0N( uint n ) { int E0N = 0;

  E0N = 0;
  E0_en.inc( E0N, n );
  return E0N; }
static inline uint MakeE0P( uint p2 ) { int E0P = 0;

  E0P = 0;
  E0_ep.inc( E0P, p2 );
  return E0P; }
static inline uint MakeE0Q( uint q ) { int E0Q = 0;

  E0Q = 0;
  E0_eq.inc( E0Q, q );
  return E0Q; }
static inline uint MakeE0J( uint w, uint n ) { int E0J = 0;

  E0J = 0;
  E0_ejw.inc( E0J, w );
  E0_ejn.inc( E0J, n );
  return E0J; }
static inline uint MakeE0E( uint eex ) { int E0E = 0;

  E0E = 0;
  E0_ee0.inc( E0E, eex );
  return E0E; }

static inline uint MakeE1E( uint een ) { int E1E = 0;

  E1E = 0;
  E0_ee1.inc( E1E, een );
  return E1E; }

static inline uint MakeE0M( uint med ) { int E0M = 0;

  E0M = 0;
  E0_em.inc( E0M, med );
  return E0M; }
static inline uint MakeE1A( uint ea, uint lum ) { int E1A = 0;

  E1A = 0;
  E0_fa.inc( E1A, ea );
  E0_fl.inc( E1A, lum );
  return E1A; }
static inline uint MakeE1W( uint w ) { int E1W = 0;

  E1W = 0;
  E0_fw.inc( E1W, w );
  return E1W; }
static inline uint MakeE1P( uint p2 ) { int E1P = 0;

  E1P = 0;
  E0_fp.inc( E1P, p2 );
  return E1P; }
static inline uint MakeE1Q( uint q ) { int E1Q = 0;

  E1Q = 0;
  E0_fq.inc( E1Q, q );
  return E1Q; }
static inline uint MakeE1J( uint ea, uint w ) { int E1J = 0;

  E1J = 0;
  E0_fja.inc( E1J, ea );
  E0_fjw.inc( E1J, w );
  return E1J; }
static inline uint MakeE1M( uint med ) { int E1M = 0;

  E1M = 0;
  E0_fm.inc( E1M, med );
  return E1M; }
static inline uint MakeEX1( uint e0r ) { int EX1 = 0;

  EX1 = 0;
  E0_x1r.inc( EX1, e0r );
  return EX1; }
static inline uint MakeEX2( uint e0r, uint e0g ) { int EX2 = 0;

  EX2 = 0;
  E0_x2r.inc( EX2, e0r );
  E0_x2g.inc( EX2, e0g );
  return EX2; }
static inline uint MakeE0H( uint h, uint hv ) { int E0H = 0;

  E0H = 0;
  E0_eh.inc( E0H, h );
  E0_ehv.inc( E0H, hv );
  return E0H; }
static inline uint MakeE1H( uint h, uint hv ) { int E1H = 0;

  E1H = 0;
  E0_fh.inc( E1H, h );
  E0_fhv.inc( E1H, hv );
  return E1H; }
static inline uint MakeE0PP( uint px, uint pn, uint qv ) { int E0PP = 0;

  E0PP = 0;
  E0_ep0.inc( E0PP, px );
  E0_ep0n.inc( E0PP, pn );
  E0_epv.inc( E0PP, qv );
  return E0PP; }
static inline uint MakeE1PP( uint px, uint pn, uint qv ) { int E1PP = 0;

  E1PP = 0;
  E0_ep1.inc( E1PP, px );
  E0_ep1n.inc( E1PP, pn );
  E0_epw.inc( E1PP, qv );
  return E1PP; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in E0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits E0_T and binds its
// CtrTab / Mixer / APM views to these.
// channel c row j; widths 32/64/32 per r,g,b -- see chw[] in Codec::init
