// bmg_ctr.inc -- everything between a model and the coder: the adaptive
// counter, the logistic domain it is mixed in, the mixer itself and the
// secondary estimator.
//
// ALGORITHM_v2.md section 5.2 makes the point that this layer is the model and
// not the coder: rc is handed a probability and this is what computes one.
// BMF computes it from a pair of integer counts; bmg computes it from an EMA
// counter (bcdr5's, unchanged) and then mixes several of them, which is the
// one structural departure -- section 5.5 describes BMF training neighbouring
// strips by *writing* to them because nothing blends predictions at read time.
// Here something does, so the neighbour writes are a refinement rather than the
// only way context evidence can combine.

// ---------------------------------------------------------------- counter

struct Counter {
  word P;                                   // P(bit==0), 15-bit

  void init( uint p0 = hSCALE ) { P = p0; }

  // EMA toward the coded bit, bounded into [mw, SCALE-mw].
  void Update( int c, int wr, int mw ) {
    int dp = P + mw - SCALE + ((SCALE - 2*mw) & (-c));
    dp = (dp * wr) >> SCALElog;
    int q = P - dp + c;
    P = q & mSCALE;
  }
};

static void ctr_fill( Counter* p, qword n, uint v ) {
  for( qword i=0; i<n; i++ ) p[i].P = v;
}

// An EMA with a fixed rate throws away most of what the first few
// observations of a context are worth: the n-th of them should move the
// estimate by about 1/n, not by the steady-state rate.  Every model here
// carries one use count per context row, and the count picks the rate out of
// this table -- fast while the row is new, settling to the tuned rate.
// It is also how a row that has never been seen can say so: the mixer is fed
// a neutral input rather than the seed value, so a cold model does not vote.
static int wrtab[16];
static void init_wrtab( int base ) {
  for( int i=0; i<16; i++ ) {
    int w = (int)((2*SCALE) / (2*i + 3));      // ~ SCALE/(i+1.5)
    wrtab[i] = (w > base) ? w : base;
  }
}

// ---------------------------------------------------------------- logistic

// 12-bit probability domain, +-2047 stretched domain.  Same tables every
// context-mixing coder since paq6 uses; kept because the mixer needs a domain
// where "add" means "multiply the odds".
static short stretch_t[4096];

static int squash( int d ) {
  static const int t[33] = {
    1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,2047,
    2549,2994,3348,3607,3785,3901,3975,4024,4050,4068,4079,4085,4089,4092,4093,4094 };
  if( d >  2047 ) return 4095;
  if( d < -2047 ) return 0;
  int w = d & 127;
  d = (d >> 7) + 16;
  return (t[d]*(128-w) + t[d+1]*w + 64) >> 7;
}

static void init_logistic( void ) {
  int pi = 0;
  for( int x=-2047; x<=2047; x++ ) {
    int v = squash(x);
    for( int p=pi; p<=v; p++ ) stretch_t[p] = x;
    pi = v+1;
  }
  for( int p=pi; p<4096; p++ ) stretch_t[p] = 2047;
}

// Counter probability (15-bit) -> stretched.
static INLINE int stretch15( uint p15 ) { return stretch_t[ p15 >> 3 ]; }

// ---------------------------------------------------------------- mixer

// Round-to-nearest instead of an arithmetic shift.  Every gradient step here is
// a signed product scaled down by a shift, and >> rounds toward minus infinity:
// a step of -1 for every negative product too small to register, and none for
// the positive ones.  In this mixer that is worth 0.01% -- the error term
// shrinks as the prediction improves, so the weights stop moving before the
// drift can accumulate.  It is used anyway because the same expression in an
// NLMS filter, where the step does not self-limit, walked every weight to its
// clamp and cost 24% before it was found; a shift that rounds is the cheap way
// not to have to know which case one is in.
static INLINE int rsh( int x, int s ) { return (x + (1 << (s-1))) >> s; }
static INLINE int iabs( int x ) { return x<0 ? -x : x; }
static INLINE int rsh64( long long x, int s ) {
  return (int)((x + (1LL << (s-1))) >> s);
}


// A gated linear mixer in the stretched domain.  One weight set per selector
// context; weights are 16.16 and updated by the usual gradient step, which for
// a logistic output is just (input * error).
template<int NI>
struct Mixer {
  int* wt;          // WSETS * NI weights
  int  nsets;
  int  st[NI];      // stretched inputs of the decision in flight
  int* w;           // weight set of the decision in flight
  int  pr;          // mixed probability, 12-bit P(bit==0)

  void alloc( int sets, int w0 ) {
    nsets = sets;
    wt = new int[ (qword)sets * NI ];
    for( qword i=0; i<(qword)sets*NI; i++ ) wt[i] = w0;
  }
  void free_( void ) { delete[] wt; wt = 0; }

  INLINE void select( int set ) { w = wt + (qword)set * NI; }

  INLINE int mix( void ) {
    long long d = 0;                       // a weight at its clamp times a
    for( int i=0; i<NI; i++ ) d += (long long)w[i]*st[i];   // saturated input
    d = (d + 32768) >> 16;                 // overflows 32 bits on its own

    if( d >  2047 ) d =  2047;
    if( d < -2047 ) d = -2047;
    pr = squash((int)d);
    return pr;
  }

  // bit is the coded bit; the mixer tracks P(bit==0), so the target is 4096
  // when the bit was 0.
  INLINE void update( int bit, int lr ) {
    if( lr < 1 ) lr = 1; else if( lr > 64 ) lr = 64;   // section 5: clamp at the point of use
    int err = ((1-bit) << 12) - pr;
    for( int i=0; i<NI; i++ ) {
      int nw = w[i] + rsh( st[i]*err*lr, 16 );
      if( nw >  (1<<20) ) nw =  (1<<20);
      if( nw < -(1<<20) ) nw = -(1<<20);
      w[i] = nw;
    }
  }
};

// ---------------------------------------------------------------- SSE / APM

// Secondary estimation: refine a probability by looking up what actually
// happened last time this context produced a probability near this one.
// 33 buckets over the stretched domain, linearly interpolated, both ends of
// the interpolation updated.
struct APM {
  word* t;
  int   idx;

  void alloc( int n ) {
    t = new word[ (qword)n * 33 ];
    for( int c=0; c<n; c++ )
      for( int j=0; j<33; j++ )
        t[c*33+j] = word( squash((j-16)*128) * 16 );
    idx = 0;
  }
  void free_( void ) { delete[] t; t = 0; }

  INLINE int pp( int pr12, int cx ) {
    if( pr12 < 0 ) pr12 = 0; else if( pr12 > 4095 ) pr12 = 4095;
    int s = stretch_t[pr12] + 2048;          // 0..4095
    int w = s & 127;
    idx = cx*33 + (s>>7);
    return (t[idx]*(128-w) + t[idx+1]*w) >> 11;
  }

  INLINE void update( int bit, int rate ) {
    if( rate < 1 ) rate = 1; else if( rate > 16 ) rate = 16;   // it is a shift
    int y = 1-bit;                            // target for P(bit==0)
    int g = (y<<16) + (y<<rate) - y - y;
    t[idx  ] += (g - t[idx  ]) >> rate;
    t[idx+1] += (g - t[idx+1]) >> rate;
  }
};

// ---------------------------------------------------------------- helpers

static INLINE int ilog2i( uint x ) {
  int r = 0;
  while( x > 1 ) { x >>= 1; r++; }
  return r;
}

// Cost of coding n symbols with this histogram, in bits: N*H(p).  Section 5.6
// -- every encoder-side choice in bmg is settled by comparing two of these.
static double estimate_cost( const uint* hist, int n, qword total ) {
  if( total == 0 ) return 0;
  double s = (double)total * log((double)total);
  for( int i=0; i<n; i++ ) if( hist[i] ) s -= (double)hist[i] * log((double)hist[i]);
  return s * 1.4426950408889634;
}
