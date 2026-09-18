
// -------------------------------------------------------------
// sh_SSE2.inc -- SSE / APM stage made of Counter<CP> cells.
//
// A table of rows (one per row context Cx, from IDX/sh_model-S0.idx) x NB
// columns (one per bucket of the quantized input probability).  Every cell
// is a Counter<CP>: the same n0/n1 + ParamUpdater(wr, mw, K) machinery as
// the order-1 model's cells, with its own parameter bundle (CP_S0).
//
//   Predict(cx, p):  s  = clip(stretch(p), +-LIM)
//                    x  = (s+LIM) * (NB-1)/(2 LIM)   (QLIN: x = p*(NB-1))
//                    j = floor(x);  wt = x-j;  sub-row from the row's history
//                    p_sse = sum_k w_k c[k].p over the DEG+1 cells around x,
//                    w_k = uniform B-spline basis of degree DEG (1: linear,
//                    2: quadratic over 3 cells, 3: cubic over 4), in the
//                    probability or the stretch domain (ILOG)
//                    p_out = p + (p_sse - p) * W             (prob. or stretch)
//   Update(bit):     C_Update(bit, g): UPD 0 the heaviest cell, 1 every
//                    contributing cell with g=1, 2 proportional (g = w_k,
//                    floored at UPMIN -- the B-spline weights are >= 0 and
//                    sum to 1, and are exactly dp_sse/dcell);
//                    then the row history shifts in the bit (HMODE 0) or
//                    whether the SSE gave the bit p > 0.5 (HMODE 1).
//
// A row has 2^HW sub-rows selected by its own HW-bit history (the "width"
// of sh_SSE1.inc).  Rows beyond 1<<HBITS (or the SSE_MAXCELLS_LOG cap) are
// hashed.  Every knob comes from the bundle (CP::NB, CP::LIM, ...;
// config.hpp with CP_SSE).
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

  Cell* t;        // ncx rows x (1<<hw) sub-rows x stride cells
  byte* hist;     // per-row history (hw bits used)
  uint  ncx;      // rows allocated
  qword vol;      // context volume asked for; rows are hashed when vol > ncx
  int   hw;       // history bits (clamped CP::HW)
  int   deg;      // kernel degree (clamped CP::DEG)
  int   pad;      // padding cells at each end of a sub-row (1 for deg > 1)
  int   stride;   // cells per sub-row: NB + 2*pad
  float lim;      // stretch clip (clamped CP::LIM)
  float qscale;   // (NB-1) / (2*lim)
  float w;        // weight of p_sse in the output (clamped CP::W)
  float upmin;    // floor of the proportional update weights
  int   upd;      // clamped CP::UPD

  // Per-query state, valid between Predict() and Update()
  Cell* c;        // first contributing cell
  int   n;        // contributing cells (deg+1)
  float wk[4];    // their kernel weights
  float wt;
  float ps;       // SSE output before blending
  uint  row;

  static float stf( float p ) {
    p = clamp( p, 1.0f/65536, 1.0f-1.0f/65536 );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }
  static int iclamp( int x, int lo, int hi ) { return x<lo ? lo : x>hi ? hi : x; }

  void Init( qword volume ) {
    vol   = volume;
    int hbits = iclamp( CP::HBITS, 8, 30 );
    hw    = iclamp( CP::HW, 0, 6 );
    lim   = clamp( CP::LIM, 0.25f, 16.0f );
    qscale = float(NB-1) / (2.0f*lim);
    w     = clamp( CP::W, 0.0f, 1.0f );
    upmin = clamp( CP::UPMIN, 0.0f, 1.0f );
    upd   = iclamp( CP::UPD, 0, 2 );
    deg   = iclamp( CP::DEG, 1, 3 );
    pad   = deg>1;
    stride = NB + 2*pad;

    ncx = uint( vol < (qword(1)<<hbits) ? vol : (qword(1)<<hbits) );
    qword maxcells = qword(1)<<SSE_MAXCELLS_LOG;
    qword percx = qword(stride)<<hw;
    if( qword(ncx)*percx > maxcells ) ncx = uint(maxcells/percx);
    if( ncx<1 ) ncx = 1;

    t = new Cell[ qword(ncx)*percx ];
    hist = new byte[ ncx ];
    memset( hist, 0, ncx );

    // Identity init: column j of every row starts at its bucket's own
    // probability sq(s_j), inverted through the cell's output map
    // p = sq( K * st( q0*(1-mw) + mwP0*mw ) ) at the seed K and mw, with
    // total mass T0 so the first updates move it at a controlled rate.
    // (padding cells continue the identity line beyond the ends).  The
    // values are computed for all NB+2 slots with a constant loop bound so
    // that both builds (stride runtime vs. folded) evaluate the same float
    // expressions the same way and stay byte-identical.
    float T0 = clamp( CP::T0, 1.0f/256, 4096.0f );
    float a[NB+2], b[NB+2];
    for( int k=0; k<NB+2; k++ ) {
      int   j   = k-1;
      float s_j = CP::QLIN ? stf( float(j)/float(NB-1) ) : -lim + float(j)*(2.0f*lim/float(NB-1));
      float pm  = sqf( s_j / CP::K );
      float q0  = (pm - CP::mwP0*CP::M) / (1.0f - CP::M);
      q0 = clamp( q0, 1.0f/4096, 1.0f-1.0f/4096 );
      a[k] = q0*T0; b[k] = (1.0f-q0)*T0;
    }
    // row-major pass: the table is far bigger than any cache
    for( qword r=0; r<(qword(ncx)<<hw); r++ ) for( int k=0; k<stride; k++ ) t[ r*stride + k ].InitN( a[k+1-pad], b[k+1-pad] );
  }

  void Quit() { delete[] t; t=0; delete[] hist; hist=0; }

  uint Row( qword cx ) const {
    if( vol<=ncx ) return uint(cx);
    uint h = uint( (cx * 0x9E3779B97F4A7C15ull) >> 32 );
    return uint( (qword(h) * ncx) >> 32 );
  }

  // p = P(bit==0) from the primary model; returns the refined P(bit==0)
  float Predict( qword cx, float p ) {
    float s0 = stf(p);
    float s  = clamp( s0, -lim, lim );
    float x  = CP::QLIN ? p * float(NB-1) : (s + lim) * qscale;
    row = Row(cx);
    uint sub = (row << hw) + (hist[row] & ((1u<<hw)-1));
    Cell* base = t + qword(sub)*stride + pad;       // base[0..NB-1] = the buckets
    if( deg==1 ) {
      // linear: the two neighbours (kept as the exact lerp)
      int j = int(x); if( j > NB-2 ) j = NB-2;
      wt = x - float(j);
      c = base + j; n = 2; wk[0] = 1.0f-wt; wk[1] = wt;
      float p0 = c[0].PredictF();
      float p1 = c[1].PredictF();
      ps = CP::ILOG ? sqf( stf(p0) + wt*(stf(p1)-stf(p0)) ) : p0 + wt*(p1-p0);
    } else {
      if( deg==2 ) {
        // quadratic B-spline: nearest knot j and its two neighbours, t in [-1/2, 1/2]
        int j = int(x + 0.5f); if( j > NB-1 ) j = NB-1;
        float u = x - float(j);
        c = base + j - 1; n = 3;
        wk[0] = 0.5f*(0.5f-u)*(0.5f-u);
        wk[1] = 0.75f - u*u;
        wk[2] = 0.5f*(0.5f+u)*(0.5f+u);
        wt = 0.5f+u;
      } else {
        // cubic B-spline: cells j-1..j+2, t in [0,1]
        int j = int(x); if( j > NB-2 ) j = NB-2;
        float u = x - float(j), u2 = u*u, u3 = u2*u, v = 1.0f-u;
        c = base + j - 1; n = 4;
        wk[0] = v*v*v/6.0f;
        wk[1] = (3.0f*u3 - 6.0f*u2 + 4.0f)/6.0f;
        wk[2] = (-3.0f*u3 + 3.0f*u2 + 3.0f*u + 1.0f)/6.0f;
        wk[3] = u3/6.0f;
        wt = u;
      }
      float acc = 0.0f;
      for( int k=0; k<n; k++ ) {
        float pk = c[k].PredictF();
        acc += wk[k] * (CP::ILOG ? stf(pk) : pk);
      }
      ps = CP::ILOG ? sqf(acc) : acc;
    }
    float pf = CP::BLOG ? sqf( s0 + (stf(ps)-s0)*w ) : p + (ps-p)*w;
    return pf;
  }

  void Update( int bit ) {
    if( upd==1 )      { for( int k=0; k<n; k++ ) c[k].C_Update(bit); }
    else if( upd==2 ) {
      for( int k=0; k<n; k++ ) c[k].C_Update(bit, wk[k]<upmin ? upmin : wk[k]);
    }
    else {
      int kb = 0; for( int k=1; k<n; k++ ) if( wk[k]>wk[kb] ) kb = k;
      c[kb].C_Update(bit);
    }
    if( hw ) {
      int h = CP::HMODE ? ((ps>=0.5f) == (bit==0)) : bit;
      hist[row] = byte((hist[row]<<1) | h);
    }
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
