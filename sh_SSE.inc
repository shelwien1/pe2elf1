
// -------------------------------------------------------------
// sh_SSE.inc -- SSE / APM stage made of Counter<CP> cells.
//
//   row     : SSE<CP,NB>, one per row context (IDX Index Cx of
//             sh_model-S0.idx); the table of rows is the generated S0_T's
//   columns : NB buckets, uniform in stretch(p) over [-LIM, +LIM]
//   cell    : Counter<CP> -- the same n0/n1 + ParamUpdater(wr, mw, K)
//             machinery as the order-1 model, with its own bundle CP_S0;
//             identity-initialized with mass T0 (CP::SEEDS)
//
//   Predict(z):   s = clip(z, +-LIM);  x = (s+LIM)(NB-1)/(2 LIM)      (z = st(p_in))
//                 j = floor(x);  wt = x-j
//                 z_sse = z_j + wt*(z_j+1 - z_j)          (z_j = the cells' logits)
//                 (the caller squashes; p_sse = sq(z_sse))
//   Update(bit):  UPD 1: c[j].C_Update(bit); c[j+1].C_Update(bit)
//                 UPD 2: the same with weights 1-wt and wt (floored at
//                        UPMIN) -- the interpolated update, each cell
//                        runs its own Newton step on a fractional event
//
// Like Counter, a row holds model state only: Predict() returns what
// Update() needs in an SSE_Pred the caller keeps.  The final probability is
// the mixer's (sh_mix2.inc): p = mix(p, SSE(p)).
//
// NB is a knob and the row's template parameter.  In the shipping build it
// is folded and SSE<CP,NB> is the element type of the generated table; in
// the tuning build it is a load-time value, so the table is an SSE_Tbl<CP>:
// the row instantiated for every NB in [2, SSE_NB_MAX] (coder0.cpp) and
// dispatched on the value read at Init(), so opt.pl measures exactly the
// code that ships (the SSEj of freq_SSE1_dbg.inc, in one place).  Every
// other knob is clamped in the bundle (IDX-FORMAT.md sec.5), so opt.pl may
// sweep any bit pattern without crashing the coder.  Design notes and
// measurements: SSE-DESIGN.md.
// -------------------------------------------------------------

// What Predict() computes and Update() needs back.  It does not depend on
// NB, so every SSE<CP,NB> and the tuning build's dispatcher return one type.
template<class CP> struct SSE_Pred {
  typename Counter<CP>::Pred pr0, pr1;   // the two bracketing cells' predictions and intermediates
  int   j;                               // left bucket
  float wt;                              // interpolation weight of c[j+1]
  float z;                               // the output logit z0 + wt*(z1-z0)
  float dzdz;                            // d z_sse / d z_in: (NB-1)/(2 LIM) * (z1-z0), 0 where the input clip binds
  float sh0, sh1;                        // d z_sse / d z_cell: 1-wt and wt, or 0 where the cell's logit clip binds
};

template<class CP, int NB> struct SSE {
  static_assert( NB>=2 && NB<=SSE_NB_MAX, "SSE bucket count out of range" );
  Counter<CP> c[NB];

  // every bucket at its own probability: the seeds are in the bundle
  void Init() { for( int j=0; j<NB; j++ ) c[j].InitN( CP::SEEDS.a[j], CP::SEEDS.b[j] ); }

  // z = logit of P(bit==0) from the primary model
  SSE_Pred<CP> Predict( float z ) const {
    SSE_Pred<CP> r;
    float s0 = clamp( z, -CP::ZMAX, CP::ZMAX );
    float s  = clamp( s0, -CP::LIM, CP::LIM );
    float x  = (s + CP::LIM) * CP::QSCALE;
    int   j  = int(x); if( j > NB-2 ) j = NB-2;
    r.j   = j;
    r.wt  = x - float(j);
    r.pr0 = c[j].PredictF();
    r.pr1 = c[j+1].PredictF();
    float z0 = clamp( r.pr0.z, -CP::ZMAX, CP::ZMAX ), z1 = clamp( r.pr1.z, -CP::ZMAX, CP::ZMAX );
    r.dzdz = ( s0 > -CP::LIM && s0 < CP::LIM ) ? CP::QSCALE * (z1 - z0) : 0.0f;
    r.sh0  = ( r.pr0.z > -CP::ZMAX && r.pr0.z < CP::ZMAX ) ? 1.0f - r.wt : 0.0f;
    r.sh1  = ( r.pr1.z > -CP::ZMAX && r.pr1.z < CP::ZMAX ) ? r.wt : 0.0f;
    r.z    = z0 + r.wt*(z1-z0);
    return r;
  }

  // ef = A2 end-to-end error at the SSE output, dL/dz_sse; each cell gets
  // it times the derivative of the output w.r.t. its logit (its
  // interpolation share, 0 where its clip binds).
  void Update( int bit, const SSE_Pred<CP>& pr, float ef ) {
    Counter<CP>& c0 = c[pr.j];
    Counter<CP>& c1 = c[pr.j+1];
    if( CP::UPD==1 ) { c0.C_Update(bit, pr.pr0, 1.0f, ef*pr.sh0); c1.C_Update(bit, pr.pr1, 1.0f, ef*pr.sh1); }
    else {
      float g0 = 1.0f-pr.wt, g1 = pr.wt;
      c0.C_Update( bit, pr.pr0, g0<CP::UPMIN ? CP::UPMIN : g0, ef*pr.sh0 );
      c1.C_Update( bit, pr.pr1, g1<CP::UPMIN ? CP::UPMIN : g1, ef*pr.sh1 );
    }
  }
};

// Tuning-build table: the row for every NB in [2, SSE_NB_MAX], one of them
// allocated at Init() for the NB the bundle read at load, every access
// dispatched on it.  tbl[i] yields a Ref -- the row's three operations on
// row i of the live instantiation -- and the table keeps no per-query
// state.  Declared by the S0 template's def_Data, allocated in its def_Init
// and freed in its def_Quit, i.e. by the generated S0_Init()/S0_Quit() like
// a Table() member: the one allocation of the coder that is not a Table()
// line, and it is in the same place.
template<class CP> struct SSE_Tbl {
  void* p = 0;   // SSE<CP,nb>[n]
  uint  n = 0;
  int   nb = 0;

  // f( (SSE<CP,nb>*)p ) for the live nb; every instantiation returns the same type
  template<int NB = 2, class F> auto dispatch( F&& f ) const -> decltype( f( (SSE<CP,2>*)0 ) ) {
    if constexpr( NB >= SSE_NB_MAX ) return f( (SSE<CP,NB>*)p );
    else return nb==NB ? f( (SSE<CP,NB>*)p ) : dispatch<NB+1>( (F&&)f );
  }
  template<class R> static R* newrows( R*, uint n ) { return new R[n]; }   // no (): Init() fills every row, as Table() (IDX-FORMAT.md sec.9)

  void Init( qword rows ) {
    nb = sse_nb_clamp( CP::NB );
    n  = uint(rows);
    dispatch( [&]( auto* r ) { p = newrows( r, n ); } );
  }
  void  Quit()        { dispatch( [&]( auto* r ) { delete[] r; } ); p = 0; n = 0; }
  qword Bytes() const { return dispatch( [&]( auto* r ) { return qword(n) * sizeof(*r); } ); }

  struct Ref {
    SSE_Tbl& t; uint i;
    void Init()                                              { t.dispatch( [&]( auto* r ) { r[i].Init(); } ); }
    SSE_Pred<CP> Predict( float z ) const                    { return t.dispatch( [&]( auto* r ) { return r[i].Predict(z); } ); }
    void Update( int bit, const SSE_Pred<CP>& pr, float ef ) { t.dispatch( [&]( auto* r ) { r[i].Update(bit, pr, ef); } ); }
  };
  Ref operator[]( uint i ) { return { *this, i }; }
};
