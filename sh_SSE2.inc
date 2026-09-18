
// -------------------------------------------------------------
// sh_SSE2.inc -- SSE / APM stage made of Counter<CP> cells.
//
// A table of rows (one per row context Cx, from IDX/sh_model-S0.idx) x NB
// columns (one per bucket of the quantized input probability).  Every cell
// is a Counter<CP>: the same n0/n1 + ParamUpdater(wr, mw, K) machinery as
// the order-1 model's cells, with its own parameter bundle (CP_S0).
//
//   Predict(cx, p):  s  = clip(stretch(p), +-LIM)
//                    x  = (s+LIM) * (NB-1)/(2 LIM);  j = floor(x);  wt = x-j
//                    p_sse = lerp( c[j].p, c[j+1].p, wt )   (prob. or stretch)
//                    p_out = p + (p_sse - p) * W             (prob. or stretch)
//   Update(bit):     C_Update(bit) on both cells, or on the nearer one.
//
// Rows beyond 1<<HBITS (or the SSE_MAXCELLS_LOG cap) are hashed.  Every
// knob comes from the bundle (CP::NB, CP::LIM, ...; config.hpp with CP_SSE).
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

template<class CP, int NB> struct SSE_Ctr {
  typedef Counter<CP> Cell;
  static_assert( NB>=2 && NB<=SSE_NB_MAX, "SSE bucket count out of range" );

  Cell* t;        // ncx rows of NB cells
  uint  ncx;      // rows allocated
  qword vol;      // context volume asked for; rows are hashed when vol > ncx
  float lim;      // stretch clip (clamped CP::LIM)
  float qscale;   // (NB-1) / (2*lim)
  float w;        // weight of p_sse in the output (clamped CP::W)

  // Per-query state, valid between Predict() and Update()
  Cell* c;
  float wt;

  static float stf( float p ) {
    p = clamp( p, 1.0f/65536, 1.0f-1.0f/65536 );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }
  static int iclamp( int x, int lo, int hi ) { return x<lo ? lo : x>hi ? hi : x; }

  void Init( qword volume ) {
    vol   = volume;
    int hbits = iclamp( CP::HBITS, 8, 30 );
    lim   = clamp( CP::LIM, 0.25f, 16.0f );
    qscale = float(NB-1) / (2.0f*lim);
    w     = clamp( CP::W, 0.0f, 1.0f );

    ncx = uint( vol < (qword(1)<<hbits) ? vol : (qword(1)<<hbits) );
    qword maxcells = qword(1)<<SSE_MAXCELLS_LOG;
    if( qword(ncx)*NB > maxcells ) ncx = uint(maxcells/NB);
    if( ncx<1 ) ncx = 1;

    t = new Cell[ qword(ncx)*NB ];

    // Identity init: column j of every row starts at its bucket's own
    // probability sq(s_j), inverted through the cell's output map
    // p = sq( K * st( q0*(1-mw) + mwP0*mw ) ) at the seed K and mw, with
    // total mass T0 so the first updates move it at a controlled rate.
    float T0 = clamp( CP::T0, 1.0f/256, 4096.0f );
    float a[NB], b[NB];
    for( int j=0; j<NB; j++ ) {
      float s_j = -lim + float(j)*(2.0f*lim/float(NB-1));
      float pm  = sqf( s_j / CP::K );
      float q0  = (pm - CP::mwP0*CP::M) / (1.0f - CP::M);
      q0 = clamp( q0, 1.0f/4096, 1.0f-1.0f/4096 );
      a[j] = q0*T0; b[j] = (1.0f-q0)*T0;
    }
    // row-major pass: the table is far bigger than any cache
    for( uint r=0; r<ncx; r++ ) for( int j=0; j<NB; j++ ) t[ qword(r)*NB + j ].InitN( a[j], b[j] );
  }

  void Quit() { delete[] t; t=0; }

  uint Row( qword cx ) const {
    if( vol<=ncx ) return uint(cx);
    uint h = uint( (cx * 0x9E3779B97F4A7C15ull) >> 32 );
    return uint( (qword(h) * ncx) >> 32 );
  }

  // p = P(bit==0) from the primary model; returns the refined P(bit==0)
  float Predict( qword cx, float p ) {
    float s0 = stf(p);
    float s  = clamp( s0, -lim, lim );
    float x  = (s + lim) * qscale;
    int   j  = int(x); if( j > NB-2 ) j = NB-2;
    wt = x - float(j);
    c  = t + qword(Row(cx))*NB + j;
    float p0 = c[0].PredictF();
    float p1 = c[1].PredictF();
    float ps = CP::ILOG ? sqf( stf(p0) + wt*(stf(p1)-stf(p0)) ) : p0 + wt*(p1-p0);
    float pf = CP::BLOG ? sqf( s0 + (stf(ps)-s0)*w ) : p + (ps-p)*w;
    return pf;
  }

  void Update( int bit ) {
    if( CP::UPD ) { c[0].C_Update(bit); c[1].C_Update(bit); }
    else          { (wt<0.5f ? c[0] : c[1]).C_Update(bit); }
  }
};

// Tuning-build front end: CP::NB is read once at Init() and the matching
// SSE_Ctr<CP,NB> instantiation is used from then on.
template<class CP> struct SSE_Dyn {
  struct If {
    virtual void  Init( qword volume ) = 0;
    virtual float Predict( qword cx, float p ) = 0;
    virtual void  Update( int bit ) = 0;
    virtual void  Quit() = 0;
    virtual ~If() {}
  };
  template<int NB> struct Impl : If {
    SSE_Ctr<CP,NB> s;
    void  Init( qword volume ) { s.Init(volume); }
    float Predict( qword cx, float p ) { return s.Predict(cx,p); }
    void  Update( int bit ) { s.Update(bit); }
    void  Quit() { s.Quit(); }
  };
  template<int NB> static If* make( int nb ) {
    if constexpr( NB > SSE_NB_MAX ) { (void)nb; return 0; }
    else return nb==NB ? (If*)new Impl<NB> : make<NB+1>(nb);
  }

  If* p;
  int nb;
  void  Init( qword volume ) { nb = sse_nb_clamp(CP::NB); p = make<2>(nb); p->Init(volume); }
  float Predict( qword cx, float x ) { return p->Predict(cx,x); }
  void  Update( int bit ) { p->Update(bit); }
  void  Quit() { p->Quit(); delete p; p=0; }
};
