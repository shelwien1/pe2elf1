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
// Clamped, and ordered: a ceiling swept to zero leaves the range coder a
// zero-width interval, and a floor above the ceiling inverts every clamp.
const int32_t kProbFloor = idx_clamp(AP_p_floor, 1, 1<<14);
const int32_t kProbCeil = idx_clamp(AP_p_ceil, kProbFloor+1, 65535);

const int32_t kApmBuckets = 33;    // 32 spans, plus the endpoint refine() interpolates to
// The span is carried as 1/256 of a stretch unit so it can be an integer
// parameter; 2048 is the 8.0 it replaces.
const double kApmSpan = idx_d(AP_span, 8);
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
    if( v<kProbFloor )
      v = kProbFloor;
    if( v>kProbCeil )
      v = kProbCeil;
    apm_squash[b] = (uint16_t)v;
  }
  apm_tables_ready = true;
}

struct Apm {
  static const int32_t kCtx = 256;   // contexts the map is sized for
  // Declared here, defined below the class.  In the tuning build an IDX value
  // is a read from a patchable object, and an in-class initializer has to be a
  // constant expression; an out-of-class definition may be dynamic.  The
  // shipping build initializes the same names from literals and folds them.
  static const int32_t kRate;
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
  //
  // The context is bounded here rather than at each call site.  Every caller
  // builds it from an Index or a mask whose width is an IDX parameter, so a
  // sweep can widen one past kCtx -- IDX/sweep.py caught exactly that on
  // CN_mslot, whose mask normally keeps four bits of a fourteen-bit context
  // and at its extreme keeps all fourteen, indexing a 256-context table with
  // 16383 and taking the encoder down with SIGSEGV.  Guarding inside refine
  // covers all six call sites at once and cannot be forgotten by a seventh;
  // update() is covered too, since it works from the `at` set here.
  int32_t refine(int32_t p16, int32_t ctx) {
    const int32_t pos = apm_pos[p16>>4];
    at = (int32_t)idx_bound((uint32_t)ctx, (uint32_t)kCtx)*kApmBuckets+(pos>>12);
    frac = pos&0xFFF;
    const int32_t lo = t[at], hi = t[at+1];
    const int32_t mapped = lo+(((hi-lo)*frac)>>12);
    // Half the map's opinion, half the model's: the map has fewer samples per
    // entry than the cell has, so it is the less certain of the two.
    const int32_t w = idx_clamp(AP_map_weight, 1, 15);
    const int32_t out = (w*mapped+p16)/(w+1);
    return out<kProbFloor ? kProbFloor : out>kProbCeil ? kProbCeil : out;
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

const int32_t Apm::kRate = idx_clamp(AP_rate, 1, 12);


// Logistic mixing: probabilities combined in the stretch domain with weights
// learned per context by gradient descent on coding loss.  This is the piece
// IMPROVEMENTS.md section 2 asks for, in its cheapest form -- two inputs, one
// weight pair per context -- and it is used where two estimates of the same bit
// already exist side by side.
// The mixer's stretch table spans [-kMixCentre, kMixCentre).
const int32_t kMixCentre = 2048;
int32_t mix_stretch[4096];
uint16_t mix_squash[4096];
bool mix_tables_ready = false;

void mix_tables_init() {
  if( mix_tables_ready )
    return;
  for( int32_t i = 0; i<4096; ++i ) {
    const double p = ((double)i+0.5)/4096.0;
    double s = (double)AP_stretch_scale*log(p/(1.0-p));
    // The clip is the table's own half-width, not a number of its own:
    // mix() indexes mix_squash by dot+kMixCentre.
    if( s<-(double)(kMixCentre-1) )
      s = -(double)(kMixCentre-1);
    if( s>(double)(kMixCentre-1) )
      s = (double)(kMixCentre-1);
    mix_stretch[i] = (int32_t)s;
  }
  for( int32_t x = 0; x<4096; ++x ) {
    int32_t v = (int32_t)(65536.0/(1.0+exp(-(double)(x-kMixCentre)/(double)AP_stretch_scale)));
    if( v<kProbFloor )
      v = kProbFloor;
    if( v>kProbCeil )
      v = kProbCeil;
    mix_squash[x] = (uint16_t)v;
  }
  mix_tables_ready = true;
}

// The learning rate sits at namespace scope, not as a static member of the
// template.  A static data member of a class template is *unordered* dynamic
// initialization: it may run before the mapping objects the tuning build reads
// its value from, in which case the shift comes out as the clamp floor and the
// tuning build silently learns at a different rate from the shipping build --
// the corpus caught it as fourteen streams differing between the two.
// Non-template statics defined after the MOD/ includes are ordered within the
// translation unit and do not have this problem.
static const int32_t kMixShift = idx_clamp(AP_mix_shift, 8, 24);

template <int32_t N> struct MixN {
  static const int32_t kSets = 256;  // weight sets the mixer is sized for
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
    if( dot<-(kMixCentre-1) )
      dot = -(kMixCentre-1);
    if( dot>kMixCentre-1 )
      dot = kMixCentre-1;
    return mix_squash[dot+kMixCentre];
  }

  void update(int32_t bit, int32_t p) {
    const int32_t err = (bit ? 65535 : 0)-p;
    for( int32_t j = 0; j<N; ++j )
      w[set][j] += (in[j]*err)>>kMixShift;
  }
};

using Mix2 = MixN<2>;
using Mix3 = MixN<3>;
using Mix4 = MixN<4>;
using Mix5 = MixN<5>;

// Cost of coding a bit at a 16-bit probability, in 1/256 bits.  Used to price a
// model's choices without running the coder.
int32_t bit_cost_tab[4096];
bool bit_cost_ready = false;

void bit_cost_init() {
  if( bit_cost_ready )
    return;
  for( int32_t i = 0; i<4096; ++i ) {
    const double p = ((double)i+0.5)/4096.0;
    bit_cost_tab[i] = (int32_t)(-256.0*log2(p));
  }
  bit_cost_ready = true;
}

// bit==1 costs -log2(p), bit==0 costs -log2(1-p).
int32_t bit_cost(int32_t p16, int32_t bit) {
  const int32_t q = bit ? p16 : 65536-p16;
  return bit_cost_tab[(q>>4)&4095];
}

// A probability with its own update count, adapting at 1/(n+2) until the count
// anneals.  Packed into one word: probability high, count low.  A context that
// sees tens of samples needs this; a fixed rate is far too slow while it is new.
const int32_t kAdaptCapMax = 255;  // the cap is a byte
const int32_t kAdaptCap = idx_clamp(AP_adapt_cap, 1, kAdaptCapMax);
int32_t adapt_recip[kAdaptCapMax+1];
bool adapt_tables_ready = false;

void adapt_tables_init() {
  if( adapt_tables_ready )
    return;
  for( int32_t k = 0; k<=kAdaptCapMax; ++k )
    adapt_recip[k] = 65536/(k+idx_clamp(AP_adapt_offset, 1, 64));
  adapt_tables_ready = true;
}

uint32_t adapt_init() {
  return (uint32_t)AP_cell_seed<<16;
}

int32_t adapt_p(uint32_t cell) {
  return (int32_t)(cell>>16);
}

int32_t adapt_n(uint32_t cell) {
  return (int32_t)(cell&0xFFFFu);
}

void adapt_bump(uint32_t &cell, int32_t target) {
  int32_t p = adapt_p(cell);
  int32_t n = adapt_n(cell);
  p += (int32_t)(((int64_t)(target-p)*adapt_recip[n])>>16);
  n += n<kAdaptCap;
  cell = ((uint32_t)p<<16)|(uint32_t)n;
}
