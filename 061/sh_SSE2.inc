
// -------------------------------------------------------------
// sh_SSE2.inc -- SSE / APM stage made of Counter<CP> cells.
//
//   rows    : one per row context (IDX Index Cx x Cx3 of sh_model-S0.idx);
//             rows beyond 1<<HBITS or the SSE_MAXCELLS_LOG cap are hashed
//   columns : NB buckets, uniform in stretch(p) over [-LIM, +LIM]
//   cell    : Counter<CP> -- the same n0/n1 + ParamUpdater(wr, mw, K)
//             machinery as the order-1 model, with its own bundle CP_S0;
//             identity-initialized with mass T0
//
//   Predict(cx, z):  s = clip(z, +-LIM);  x = (s+LIM)(NB-1)/(2 LIM)      (z = st(p_in))
//                    j = floor(x);  wt = x-j
//                    z_sse = z_j + wt*(z_j+1 - z_j)          (z_j = the cells' logits)
//                    (the caller squashes; p_sse = sq(z_sse))
//   Update(bit):     UPD 1: c[j].C_Update(bit); c[j+1].C_Update(bit)
//                    UPD 2: the same with weights 1-wt and wt (floored at
//                           UPMIN) -- the interpolated update, each cell
//                           runs its own Newton step on a fractional event
//
// The final probability is the mixer's (sh_mix2.inc): p = mix(p, SSE(p)).
//
// Two versions, selected by USE_NEW from the generated IDX headers:
//   USE_NEW=0 (shipping) SSE_Ctr<CP, CP::NB>: NB is a plain template
//                        parameter, every knob a folded constant.
//   USE_NEW=1 (tuning)   SSE_Dyn<CP>: CP::NB is a patchable runtime value;
//                        the stage is instantiated for every NB in
//                        [2, SSE_NB_MAX] and dispatched once at Init(), so
//                        opt.pl measures exactly the code that ships (the
//                        SSEjQ switch of freq_SSE1_dbg.inc, in one place).
// Every other knob is clamped at the point of use (IDX-FORMAT.md sec.5), so
// opt.pl may sweep any bit pattern without crashing the coder.
// Design notes and measurements: SSE-DESIGN.md.
// -------------------------------------------------------------

#ifndef SSE_MAXCELLS_LOG
#define SSE_MAXCELLS_LOG 24    // hard cap: 2^24 cells whatever HBITS/NB say
#endif
#ifndef SSE_NB_MAX
#define SSE_NB_MAX 16          // bucket counts instantiated by the dispatcher
#endif

static constexpr int sse_nb_clamp( int nb ) { return nb<2 ? 2 : nb>SSE_NB_MAX ? SSE_NB_MAX : nb; }

// Rows of the table for a context volume: min(volume, 2^HBITS, cap/NB).
// A constant expression in the shipping build (it sizes the Table() array
// of the generated S0_T), a runtime value in the tuning build.
static constexpr qword sse_rows( qword vol, int hbits, int nb ) {
  hbits = hbits<8 ? 8 : hbits>30 ? 30 : hbits;
  qword ncx = vol < (qword(1)<<hbits) ? vol : (qword(1)<<hbits);
  qword maxcells = qword(1)<<SSE_MAXCELLS_LOG;
  if( ncx*qword(nb) > maxcells ) ncx = maxcells/qword(nb);
  return ncx<1 ? 1 : ncx;
}

template<class CP, int NB> struct SSE_Ctr {
  typedef Counter<CP> Cell;
  static_assert( NB>=2 && NB<=SSE_NB_MAX, "SSE bucket count out of range" );

  Cell* t;        // ncx rows of NB cells: the Table() of the generated S0_T
  uint  ncx;      // rows
  qword vol;      // context volume asked for; rows are hashed when vol > ncx
  float lim;      // stretch clip (clamped CP::LIM)
  float zmax;     // |logit| bound, = stf's probability clip [1/65536, 1-1/65536]
  float qscale;   // (NB-1) / (2*lim)
  float upmin;    // floor of the proportional update weights
  int   upd;      // 1: both cells fully, 2: proportional

  // Per-query state, valid between Predict() and Update()
  Cell* c;
  float wt;
  typename Cell::Pred pr0, pr1;   // the two cells' predictions and intermediates, for C_Update()
  float dzdz;       // d z_sse / d z_in: (NB-1)/(2 LIM) * (z1-z0), 0 when the input clip binds
  float sh0, sh1;   // d z_sse / d z_cell: 1-wt and wt, or 0 where the cell's logit clip binds

  static float stf( float p ) {
    p = clamp( p, 1.0f/65536, 1.0f-1.0f/65536 );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }
  static int iclamp( int x, int lo, int hi ) { return x<lo ? lo : x>hi ? hi : x; }

  // table: sse_rows(volume, HBITS, NB) * NB cells, owned by the caller
  void Init( Cell* table, qword volume ) {
    t = table; ncx = 0;
    if( !CP::ON ) return;
    vol   = volume;
    lim   = clamp( CP::LIM, 0.25f, 16.0f );
    zmax  = rt_logf( 65535.0f );
    qscale = float(NB-1) / (2.0f*lim);
    upmin = clamp( CP::UPMIN, 0.0f, 1.0f );
    upd   = iclamp( CP::UPD, 1, 2 );
    ncx   = uint( sse_rows( vol, CP::HBITS, NB ) );

    // Identity init: column j of every row starts at its bucket's own
    // probability sq(s_j), inverted through the cell's output map
    // p = sq( K * st( q0*(1-mw) + mwP0*mw ) ) at the seed K and mw, with
    // total mass T0 so the first updates move it at a controlled rate.
    // (rt_expf: not constant-folded, see coder0.cpp)
    float T0 = clamp( CP::T0, 1.0f/256, 4096.0f );
    float a[NB], b[NB];
    for( int j=0; j<NB; j++ ) {
      float s_j = -lim + float(j)*(2.0f*lim/float(NB-1));
      float pm  = 1.0f/(1.0f+rt_expf( -(s_j / CP::K) ));
      float q0  = (pm - CP::mwP0*CP::M) / (1.0f - CP::M);
      q0 = clamp( q0, 1.0f/4096, 1.0f-1.0f/4096 );
      a[j] = q0*T0; b[j] = (1.0f-q0)*T0;
    }
    // row-major pass: the table is far bigger than any cache
    for( uint r=0; r<ncx; r++ ) for( int j=0; j<NB; j++ ) t[ qword(r)*NB + j ].InitN( a[j], b[j] );
  }

  uint Row( qword cx ) const {
    if( vol<=ncx ) return uint(cx);
    uint h = uint( (cx * 0x9E3779B97F4A7C15ull) >> 32 );
    return uint( (qword(h) * ncx) >> 32 );
  }

  // z = logit of P(bit==0) from the primary model; returns the SSE's logit
  float Predict( qword cx, float z ) {
    if( !CP::ON ) return z;
    float s0 = clamp( z, -zmax, zmax );
    float s  = clamp( s0, -lim, lim );
    float x  = (s + lim) * qscale;
    int   j  = int(x); if( j > NB-2 ) j = NB-2;
    wt = x - float(j);
    c  = t + qword(Row(cx))*NB + j;
    pr0 = c[0].PredictF();
    pr1 = c[1].PredictF();
    float z0 = clamp( pr0.z, -zmax, zmax ), z1 = clamp( pr1.z, -zmax, zmax );
    dzdz = ( s0 > -lim && s0 < lim ) ? qscale * (z1 - z0) : 0.0f;
    sh0  = ( pr0.z > -zmax && pr0.z < zmax ) ? 1.0f - wt : 0.0f;
    sh1  = ( pr1.z > -zmax && pr1.z < zmax ) ? wt : 0.0f;
    return z0 + wt*(z1-z0);
  }

  // d st(p_sse) / d st(p_in) of the last Predict() (1 when the stage is off)
  float dz2_dz1() const { return CP::ON ? dzdz : 1.0f; }

  // ef = A2 end-to-end error at the SSE output, dL/dz_sse; each cell gets
  // it times the derivative of the output w.r.t. its logit (its
  // interpolation share, 0 where its clip binds).
  void Update( int bit, float ef = 0.0f ) {
    if( !CP::ON ) return;
    if( upd==1 ) { c[0].C_Update(bit, pr0, 1.0f, ef*sh0); c[1].C_Update(bit, pr1, 1.0f, ef*sh1); }
    else {
      float g0 = 1.0f-wt, g1 = wt;
      c[0].C_Update( bit, pr0, g0<upmin ? upmin : g0, ef*sh0 );
      c[1].C_Update( bit, pr1, g1<upmin ? upmin : g1, ef*sh1 );
    }
  }
};

// Tuning-build front end: CP::NB is read once at Init() and the matching
// SSE_Ctr<CP,NB> instantiation is used from then on.
template<class CP> struct SSE_Dyn {
  typedef Counter<CP> Cell;
  struct If {
    virtual void  Init( Cell* table, qword volume ) = 0;
    virtual float Predict( qword cx, float p ) = 0;
    virtual void  Update( int bit, float ef ) = 0;
    virtual float dz2_dz1() const = 0;
    virtual ~If() {}
  };
  template<int NB> struct Impl : If {
    SSE_Ctr<CP,NB> s;
    void  Init( Cell* table, qword volume ) { s.Init(table, volume); }
    float Predict( qword cx, float p ) { return s.Predict(cx,p); }
    void  Update( int bit, float ef ) { s.Update(bit, ef); }
    float dz2_dz1() const { return s.dz2_dz1(); }
  };
  template<int NB> static If* make( int nb ) {
    if constexpr( NB > SSE_NB_MAX ) { (void)nb; return 0; }
    else return nb==NB ? (If*)new Impl<NB> : make<NB+1>(nb);
  }

  If* p;
  void  Init( Cell* table, qword volume ) { p = make<2>( sse_nb_clamp(CP::NB) ); p->Init(table, volume); }
  float Predict( qword cx, float x ) { return p->Predict(cx,x); }
  void  Update( int bit, float ef = 0.0f ) { p->Update(bit, ef); }
  float dz2_dz1() const { return p->dz2_dz1(); }
};
