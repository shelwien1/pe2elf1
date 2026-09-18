
// -------------------------------------------------------------
// sh_SSE2.inc -- SSE / APM stage made of Counter<CP> cells.
//
// A table of rows (one per context Cx, from IDX/sh_model-S0.idx) x nb
// columns (one per bucket of the quantized input probability).  Every cell
// is a Counter<CP>: the same n0/n1 + ParamUpdater(wr, mw, K) machinery as
// the order-1 model's cells, with its own parameter bundle (CP_S0).
//
//   Predict(cx, p):  s  = clip(stretch(p), +-LIM)
//                    x  = (s+LIM) * (nb-1)/(2 LIM);  j = floor(x);  wt = x-j
//                    p_sse = lerp( c[j].p, c[j+1].p, wt )   (prob. or stretch)
//                    p_out = p + (p_sse - p) * W             (prob. or stretch)
//   Update(bit):     C_Update(bit) on both cells, or on the nearer one.
//
// Rows beyond 1<<HBITS (or the SSE_MAXCELLS_LOG cap) are hashed.  Every
// tunable is clamped here, at the point of use (IDX-FORMAT.md sec.5), so
// opt.pl may sweep any bit pattern without crashing the coder.
// Design notes and measurements: SSE-DESIGN.md.
// -------------------------------------------------------------

#ifndef SSE_MAXCELLS_LOG
#define SSE_MAXCELLS_LOG 24    // hard cap: 2^24 cells whatever HBITS/NB say
#endif

template<class CP> struct SSE_Ctr {
  typedef Counter<CP> Cell;

  Cell* t;        // ncx rows of nb cells
  uint  ncx;      // rows allocated
  uint  nb;       // cells (buckets) per row
  qword vol;      // context volume asked for; rows are hashed when vol > ncx
  float lim;      // stretch clip
  float qscale;   // (nb-1) / (2*lim)
  float w;        // weight of p_sse in the output
  int   ilog;     // interpolate in the stretch domain
  int   blog;     // blend in the stretch domain
  int   upd;      // update both cells (1) or the nearer one (0)

  // Per-query state, valid between Predict() and Update()
  Cell* c;
  float wt;

  static float stf( float p ) {
    p = clamp( p, 1.0f/65536, 1.0f-1.0f/65536 );
    return logf( p/(1.0f-p) );
  }
  static float sqf( float x ) { return 1.0f/(1.0f+expf(-x)); }
  static int iclamp( int x, int lo, int hi ) { return x<lo ? lo : x>hi ? hi : x; }

  void Init( qword volume, int hbits, int nbk, int lim256, int t0_256, int w_sc,
             int ilog_, int blog_, int upd_ ) {
    vol   = volume;
    nb    = iclamp( nbk, 2, 64 );
    hbits = iclamp( hbits, 8, 30 );
    lim   = float( iclamp(lim256, 64, 16*256) ) / 256;
    qscale = float(nb-1) / (2.0f*lim);
    w     = clamp( float(w_sc)/SCALE, 0.0f, 1.0f );
    ilog  = ilog_&1; blog = blog_&1; upd = upd_&1;

    ncx = uint( vol < (qword(1)<<hbits) ? vol : (qword(1)<<hbits) );
    qword maxcells = qword(1)<<SSE_MAXCELLS_LOG;
    if( qword(ncx)*nb > maxcells ) ncx = uint(maxcells/nb);
    if( ncx<1 ) ncx = 1;

    t = new Cell[ qword(ncx)*nb ];

    // Identity init: column j of every row starts at its bucket's own
    // probability sq(s_j), inverted through the cell's output map
    // p = sq( K * st( q0*(1-mw) + mwP0*mw ) ) at the seed K and mw, with
    // total mass T0 so the first updates move it at a controlled rate.
    float T0 = float( iclamp(t0_256, 1, 1<<20) ) / 256;
    float a[64], b[64];
    for( uint j=0; j<nb; j++ ) {
      float s_j = -lim + float(j)*(2.0f*lim/float(nb-1));
      float pm  = sqf( s_j / CP::K );
      float q0  = (pm - CP::mwP0*CP::M) / (1.0f - CP::M);
      q0 = clamp( q0, 1.0f/4096, 1.0f-1.0f/4096 );
      a[j] = q0*T0; b[j] = (1.0f-q0)*T0;
    }
    // row-major pass: the table is far bigger than any cache
    for( uint r=0; r<ncx; r++ ) for( uint j=0; j<nb; j++ ) t[ qword(r)*nb + j ].InitN( a[j], b[j] );
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
    int   j  = int(x); if( j > int(nb)-2 ) j = nb-2;
    wt = x - float(j);
    c  = t + qword(Row(cx))*nb + j;
    float p0 = c[0].PredictF();
    float p1 = c[1].PredictF();
    float ps = ilog ? sqf( stf(p0) + wt*(stf(p1)-stf(p0)) ) : p0 + wt*(p1-p0);
    float pf = blog ? sqf( s0 + (stf(ps)-s0)*w ) : p + (ps-p)*w;
    return pf;
  }

  void Update( int bit ) {
    if( upd ) { c[0].C_Update(bit); c[1].C_Update(bit); }
    else      { (wt<0.5f ? c[0] : c[1]).C_Update(bit); }
  }
};
