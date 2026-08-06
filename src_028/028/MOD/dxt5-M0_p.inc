
// M0: match history contexts


static inline uint MakeM1( uint hist, uint eq, uint mt ) {
  int M1 = 0;

  M1 = 0;
  M0_h1.inc( M1, hist );
  M0_h1t.inc( M1, mt );
  M1 = M1*(2) + (eq);
  return M1;
}

static inline uint MakeM2( uint hist ) {
  int M2 = 0;

  M2 = 0;
  M0_h2.inc( M2, hist );
  return M2;
}

static inline uint MakeMM( uint eq, uint hist, uint mt ) {
  int MM = 0;

  MM = 0;
  M0_me.inc( MM, eq );
  M0_mh.inc( MM, hist );
  M0_mmt.inc( MM, mt );
  return MM;
}


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in M0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits M0_T and binds its
// CtrTab / Mixer / APM views to these.
// 15 tables on the M1 context, 8 on M2 (see cap_vol)

// every mixer keyed on the match-model context

// pair / word match hashes; the *v arrays hold the cached value byte
