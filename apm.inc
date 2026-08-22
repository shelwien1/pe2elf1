// apm.inc -- the adaptive probability map (SSE) shared by the models
// Included from bmf.cpp; not a standalone translation unit.

// An adaptive probability map: a probability in, a better probability out.
//
// Nothing else in the codec does this.  Each model reaches its symbol through
// an adaptive frequency cell and codes it straight out of that cell, with no
// stage that asks whether the cell's estimate is systematically off in some
// direction the cell itself cannot see.  Wherever a model has a decision that
// carries a large share of its bits, that decision is split out as a binary one
// -- an exact rewrite of the same code by the chain rule, free on its own --
// and its probability comes through here first.
//
// The map is indexed by a fractional position between two stretch(p) buckets
// and blends the two entries, updating both in proportion.  Hard bucket
// boundaries would put a step discontinuity in the correction exactly where the
// estimate is least certain.
const int32_t kApmBuckets = 33;
const double kApmSpan = 8.0;
int32_t apm_pos[4096];
uint16_t apm_squash[kApmBuckets];
bool apm_tables_ready = false;

void apm_tables_init() {
  if( apm_tables_ready )
    return;
  for( int32_t i = 0; i<4096; ++i ) {
    const double p = ((double)i+0.5)/4096.0;
    double u = (log(p/(1.0-p))+kApmSpan)/(2.0*kApmSpan)*(double)(kApmBuckets-1);
    if( u<0.0 )
      u = 0.0;
    if( u>(double)(kApmBuckets-1)-1e-9 )
      u = (double)(kApmBuckets-1)-1e-9;
    apm_pos[i] = (int32_t)(u*4096.0);
  }
  for( int32_t b = 0; b<kApmBuckets; ++b ) {
    const double s = (double)b/(double)(kApmBuckets-1)*2.0*kApmSpan-kApmSpan;
    int32_t v = (int32_t)(65536.0/(1.0+exp(-s)));
    if( v<32 )
      v = 32;
    if( v>65504 )
      v = 65504;
    apm_squash[b] = (uint16_t)v;
  }
  apm_tables_ready = true;
}

struct Apm {
  static const int32_t kCtx = 256;
  static const int32_t kRate = 5;
  uint16_t t[kCtx*kApmBuckets];
  int32_t at;
  int32_t frac;

  void Init() {
    apm_tables_init();
    for( int32_t c = 0; c<kCtx; ++c )
      for( int32_t b = 0; b<kApmBuckets; ++b )
        t[c*kApmBuckets+b] = apm_squash[b];
    at = 0;
    frac = 0;
  }

  // p16 is the cell's own probability that the class is 0, in 1/65536.
  int32_t refine(int32_t p16, int32_t ctx) {
    const int32_t pos = apm_pos[p16>>4];
    at = ctx*kApmBuckets+(pos>>12);
    frac = pos&0xFFF;
    const int32_t lo = t[at], hi = t[at+1];
    const int32_t mapped = lo+(((hi-lo)*frac)>>12);
    // Half the map's opinion, half the model's: the map has fewer samples per
    // entry than the cell has, so it is the less certain of the two.
    const int32_t out = (3*mapped+p16)>>2;
    return out<32 ? 32 : out>65504 ? 65504 : out;
  }

  void update(int32_t bit) {
    const int32_t target = bit ? 65535 : 0;
    int32_t lo = t[at], hi = t[at+1];
    lo += ((target-lo)*(4096-frac))>>(12+kRate);
    hi += ((target-hi)*frac)>>(12+kRate);
    t[at] = (uint16_t)lo;
    t[at+1] = (uint16_t)hi;
  }
};


// Logistic mixing: probabilities combined in the stretch domain with weights
// learned per context by gradient descent on coding loss.  This is the piece
// IMPROVEMENTS.md section 2 asks for, in its cheapest form -- two inputs, one
// weight pair per context -- and it is used where two estimates of the same bit
// already exist side by side.
int32_t mix_stretch[4096];
uint16_t mix_squash[4096];
bool mix_tables_ready = false;

void mix_tables_init() {
  if( mix_tables_ready )
    return;
  for( int32_t i = 0; i<4096; ++i ) {
    const double p = ((double)i+0.5)/4096.0;
    double s = 256.0*log(p/(1.0-p));
    if( s<-2047.0 )
      s = -2047.0;
    if( s>2047.0 )
      s = 2047.0;
    mix_stretch[i] = (int32_t)s;
  }
  for( int32_t x = 0; x<4096; ++x ) {
    int32_t v = (int32_t)(65536.0/(1.0+exp(-(double)(x-2048)/256.0)));
    if( v<32 )
      v = 32;
    if( v>65504 )
      v = 65504;
    mix_squash[x] = (uint16_t)v;
  }
  mix_tables_ready = true;
}

template <int32_t N> struct MixN {
  static const int32_t kSets = 256;
  static const int32_t kShift = 18;
  int32_t w[kSets][N];
  int32_t in[N];
  int32_t set;

  void Init() {
    mix_tables_init();
    for( int32_t k = 0; k<kSets; ++k )
      for( int32_t j = 0; j<N; ++j )
        w[k][j] = (1<<16)/N;
    for( int32_t j = 0; j<N; ++j )
      in[j] = 0;
    set = 0;
  }

  void seed(int32_t k, const int32_t* weights) {
    for( int32_t j = 0; j<N; ++j )
      w[k&(kSets-1)][j] = weights[j];
  }

  int32_t mix(const int32_t* p, int32_t ctx) {
    set = ctx&(kSets-1);
    int32_t dot = 0;
    for( int32_t j = 0; j<N; ++j ) {
      in[j] = mix_stretch[p[j]>>4];
      dot += w[set][j]*in[j];
    }
    dot >>= 16;
    if( dot<-2047 )
      dot = -2047;
    if( dot>2047 )
      dot = 2047;
    return mix_squash[dot+2048];
  }

  void update(int32_t bit, int32_t p) {
    const int32_t err = (bit ? 65535 : 0)-p;
    for( int32_t j = 0; j<N; ++j )
      w[set][j] += (in[j]*err)>>kShift;
  }
};

using Mix2 = MixN<2>;
using Mix3 = MixN<3>;
using Mix4 = MixN<4>;
using Mix5 = MixN<5>;
