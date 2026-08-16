// -------------------------------------------------------------
// mrpc -- a lossless RGB/RGBA BMP compressor on MRP's scheme
//
// MRP (Matsuda et al., "Lossless coding using variable block-size adaptive
// prediction optimized for each image", mrp-0.5) is a two-pass coder: it
// segments the image into classes over a quadtree, fits one linear
// predictor per class by weighted least squares, indexes a family of
// generalized-Gaussian probability models by a quantised measure of local
// activity, and iterates all of that against measured code length,
// transmitting the class map, the coefficients and the quantiser.
//
// mrpc keeps that skeleton and extends every part of it to colour:
//
//   * one class map, shared by the components -- the segmentation is a
//     property of the image, not of a plane -- but its own coefficient
//     vector per (class, component);
//   * predictor taps that reach across components: PRD_ORDER causal
//     neighbours of the same component, XPRD_ORDER of every other, and the
//     components of the current pixel already coded, which is where most
//     of the colour correlation actually lives;
//   * activity, group quantiser and probability-model choice per
//     component, with this pixel's already-coded components folded into
//     the activity measure.
//
// It shares bmpc's BMP front end and rangecoder (sh_common.inc /
// sh_v2f.inc) but none of its model: no adaptive-parameter Counter here,
// nothing running a ParamUpdater per bit.  The probability models are
// static tables built once and indexed, so the cost of a pixel is a table
// lookup -- which is what makes two-pass optimisation affordable at all.
//
//   mrpc c in.bmp out.mrp
//   mrpc d out.mrp back.bmp
// -------------------------------------------------------------

#ifndef MRP_CLASS
#define MRP_CLASS    0   // predictor classes; 0 = pick from the image size
#endif
#ifndef MRP_MAXCLASS
#define MRP_MAXCLASS 63
#endif
#ifndef MRP_GROUP
#define MRP_GROUP    16  // activity groups, i.e. probability models in use
#endif
#ifndef PRD_ORDER
#define PRD_ORDER    20  // causal taps on the component being predicted
#endif
#ifndef XPRD_ORDER
#define XPRD_ORDER    6  // causal taps on each OTHER component
#endif
#ifndef XCUR
#define XCUR          1  // also tap the current pixel's earlier components
#endif
#ifndef COEF_PREC
#define COEF_PREC     6  // fixed-point bits of a predictor coefficient
#endif
#ifndef NUM_PMODEL
#define NUM_PMODEL   16  // generalized-Gaussian shapes to choose among
#endif
#ifndef NUM_SIGV
#define NUM_SIGV      3  // sigma variants per group, on top of the shape.
                         // The ladder steps by x1.38, so a group's sigma
                         // can be up to 16% off whatever the residuals in
                         // it actually want; NUM_SIGV=3 offers +-17.5%
                         // around the rung and codes the choice with the
                         // shape.  Variant 0 is the plain rung, so
                         // NUM_SIGV=1 is exactly the unextended model.
#endif
#ifndef PMFIX
#define PMFIX         1  // transmit a bucketed correction to each group's
                         // pmf, measured from the residuals the settled
                         // model actually produced.  Costs nc*MRP_GROUP*15
                         // small numbers of side information, so it is
                         // gated on measured gain and can come out to a
                         // single bit.
#endif
#ifndef PMFIX_ROUNDS
#define PMFIX_ROUNDS  1  // times to measure-and-apply the correction.  The
                         // second round sees a model that has already been
                         // corrected once and the thresholds re-chosen
                         // under it, so what it measures is the residual
                         // mismatch, and the levels compose.  Measured
                         // 0.04% for a second pass over the image: one
                         // round takes essentially all of it.
#endif
#ifndef SIGV_2STAGE
#define SIGV_2STAGE   1  // pick the shape first, then the sigma of that
                         // shape, instead of scanning the product.  The
                         // selection pass reads one cost table per
                         // candidate per (pixel, component) and is the
                         // whole price of NUM_SIGV; 0 = exhaustive, which
                         // measured 0.35% smaller for 3% more encode time
                         // -- inside the noise band BORDER_FIX mapped
                         // out, so it is not clearly a real 0.35%.
#endif
#ifndef PM_ACC
#define PM_ACC        3  // bits of the prediction's fraction the pmf sees
#endif
#ifndef MAX_UPARA
#define MAX_UPARA   512  // activity is capped here
#endif
#ifndef UPEL_DIST
#define UPEL_DIST     3  // radius of the activity neighbourhood
#endif
#ifndef XUPEL
#define XUPEL         1  // 1 = component c's activity also counts the errors
                         // already made on this pixel's earlier components:
                         // if red just missed by 30, green is about to too
#endif
#ifndef QT_DEPTH
#define QT_DEPTH      4  // quadtree levels below MAX_BSIZE
#endif
#ifndef MAX_BSIZE
#define MAX_BSIZE    32
#endif
#ifndef BASE_BSIZE
#define BASE_BSIZE    8
#endif
#ifndef MRP_EFFORT
#define MRP_EFFORT    1  // how long to keep iterating: 0 fast, 1 default,
                         // 2 maximum compression.  Sets MAX_ITER and
                         // EXTRA_ITER, either of which can still be given
                         // on its own.
                         //
                         // This is not a convergence tolerance.  The first
                         // loop alternates a weighted-least-squares fit
                         // with a class reassignment, and the fit
                         // minimises squared error rather than code
                         // length, so the sequence does not descend: on a
                         // 320x240 image it went 135K, 122K, 147K, 140K,
                         // 147K, 132K bytes.  Only the best iterate is
                         // kept and the second loop starts from it, so the
                         // whole file rides on which iterate happened to
                         // land low.  They are samples of a noisy process,
                         // not steps of a descent, and taking more of them
                         // is worth 4.7% of the corpus -- for about eight
                         // times the encode time, which is why it is a
                         // knob and not the default.  Decode is unaffected.
#endif
#ifndef MAX_ITER
#define MAX_ITER  (MRP_EFFORT<1 ? 12 : MRP_EFFORT<2 ? 20 : 60)
#endif
#ifndef EXTRA_ITER
#define EXTRA_ITER (MRP_EFFORT<1 ? 3 : MRP_EFFORT<2 ? 4 : 20)
#endif
#ifndef OPT_PRED
#define OPT_PRED      1  // run the coefficient search in the second loop
#endif
#ifndef MRP_ORD
#define MRP_ORD       2  // component coding order:
                         //  0 = as stored, 1 = component 1 first,
                         //  2 = trial: fit one global predictor per order
                         //      and keep the cheapest
#endif
#ifndef MRP_PROGRESS
#define MRP_PROGRESS  1  // report progress on stderr while encoding:
                         //  0 = never, 1 = when the image is big enough to
                         //  take a while (MRP_PROGMIN pixels), 2 = always.
                         // A two-pass coder that says nothing for an hour
                         // is indistinguishable from a hung one.
#endif
#ifndef MRP_PROGMIN
#define MRP_PROGMIN 200000
#endif
#ifndef COEF_MAX
#define COEF_MAX  4096   // pixels of a class the candidate sweep samples.
                         // It is choosing the argmin of 33 numbers; eight
                         // thousand samples decide that as well as thirty
                         // thousand do, and the sweep is the encoder's
                         // largest cost on a big image.  0 = all of them.
#endif
#ifndef MIN_GAIN
#define MIN_GAIN  4096   // stop a loop once an iteration improves by less
                         // than 1/MIN_GAIN of the cost.  MRP runs a fixed
                         // count; past a point the iterations are buying
                         // hundredths of a percent at a minute each.
#endif
#ifndef MRP_OLDSORT
#define MRP_OLDSORT 0
#endif
#ifndef INIT_METRIC
#define INIT_METRIC 0 // how InitClass ranks 8x8 blocks before the search:
                      //  0 = variance, as MRP does it
                      //  1 = gradient energy.  Variance is a weak proxy
                      //      for how hard a block is to PREDICT -- a smooth
                      //      ramp has large variance and no prediction
                      //      difficulty at all -- so this looked promising.
                      //      Measured: -0.01% on a 4096-wide crop, +0.9%
                      //      on a 320-wide one and +3.1% on another.  The
                      //      ranking is only a starting point for a
                      //      nonconvex search, and a better-motivated
                      //      starting point is not the same as a better
                      //      one.  Kept as a toggle, off.
#endif
#ifndef BORDER_FIX
#define BORDER_FIX 0 // refresh errB's borders after a full prediction pass.
                     // FillBorders runs at load time, when the errors do
                     // not exist yet, so during optimisation the margins
                     // hold zeros while CodeImage fills them from live
                     // errors -- pixels within UPEL_DIST of an edge are
                     // group-quantised against an activity that is not the
                     // one they are coded with.  This makes the two agree,
                     // and it is right: it is also 2.6% WORSE across the
                     // corpus.  What that measures is not the margin --
                     // 2*UPEL_DIST/W of the pixels cannot move a file 2.6%
                     // -- but the first loop's sensitivity to its starting
                     // point (see EXTRA_ITER).  Any change of the same
                     // size, correct or not, rolls the same dice.  Off
                     // until the search stops being a lottery; turn it on
                     // if you want the stored prd/grp to be exactly the
                     // coding-time values.
#endif
#ifndef DP_SHRINK
#define DP_SHRINK 1  // run the threshold trellis only up to the largest
                     // activity the image actually produced.  The DP is
                     // quadratic in that range and smooth images use a
                     // small fraction of MAX_UPARA.  It also lowers the
                     // last coded threshold, which is side information the
                     // stream no longer pays for -- so it is not purely a
                     // speed knob and the output moves.
#endif
#ifndef MRP_GATHER
#define MRP_GATHER  0    // 1 = AVX2 vgather in the coefficient search,
                         // 0 = SIMD-computed indices with scalar loads.
                         // Measured: the loads win, 6s against 8s.
#endif
#ifndef MRP_SEED
#define MRP_SEED  12345  // the coefficient search picks its tap pairs at
                         // random; this is the only thing that makes an
                         // encode non-deterministic across builds, and the
                         // knob that measures how much that matters
#endif
#ifndef MRP_VERBOSE
#define MRP_VERBOSE   0  // -DMRP_VERBOSE=1: optimisation trace on stderr
#endif

#define MAXC 4
#ifndef TRIAL_PIX
#define TRIAL_PIX 262144 // pixels the component-order trial fits on, spread
                         // over TRIAL_BANDS bands.  One contiguous band is
                         // not enough: it picks the order that suits that
                         // part of the picture, which on a 4096-wide image
                         // can be nothing like the rest of it.
#endif
#ifndef TRIAL_BANDS
#define TRIAL_BANDS  6
#endif
#ifndef MAX_TOTFREQ
#define MAX_TOTFREQ (1<<20)   // must stay under the rangecoder's range
                              // floor, or `range = (range/tot)*tot` throws
                              // away real bits: sh_rcm.inc renormalises by
                              // bytes, so that floor is 2^24
#endif
#define MIN_FREQ 1

#include "sh_common.inc"
#include "sh_rcm.inc"
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

// The progress report wants wall time.  clock() is CPU time, which is the
// same thing single-threaded and four times the truth on four threads.
static double tnow( void ) {
#ifdef _OPENMP
  return omp_get_wtime();
#elif defined(CLOCK_MONOTONIC)
  struct timespec ts;
  if( clock_gettime(CLOCK_MONOTONIC,&ts)==0 )
    return double(ts.tv_sec) + double(ts.tv_nsec)*1e-9;
  return double(clock())/double(CLOCKS_PER_SEC);
#else
  return double(clock())/double(CLOCKS_PER_SEC);
#endif
}
static double t_start = 0.0;
static int    g_prog  = 0;
#define PROG(...) do { if(g_prog){ fprintf(stderr,__VA_ARGS__); fflush(stderr);} } while(0)

static const int NUM_UPELS = UPEL_DIST*(UPEL_DIST+1);
static const int NUM_SUBPM = 1<<PM_ACC;
static const int MAXVAL    = 255;
static const int MAXPRD    = MAXVAL << COEF_PREC;
static const int MAX_COEF  = 2 << COEF_PREC;
static const int PMSIZE    = MAXVAL*2 + 1;      // the shifted pmf window
static const int MIN_BSIZE = MAX_BSIZE >> QT_DEPTH;
static const int NTMAX     = PRD_ORDER + (MAXC-1)*XPRD_ORDER + MAXC;
static const int PADL = UPEL_DIST+2, PADR = UPEL_DIST+2, PADT = UPEL_DIST+2;

// MRP's causal neighbourhood, in distance order (common.c, dyx[]).
static const int DYX[][2] = {
  {0,-1},{-1,0},
  {0,-2},{-1,-1},{-2,0},{-1,1},
  {0,-3},{-1,-2},{-2,-1},{-3,0},{-2,1},{-1,2},
  {0,-4},{-1,-3},{-2,-2},{-3,-1},{-4,0},{-3,1},{-2,2},{-1,3},
  {0,-5},{-1,-4},{-2,-3},{-3,-2},{-4,-1},{-5,0},{-4,1},{-3,2},{-2,3},{-1,4},
  {0,-6},{-1,-5},{-2,-4},{-3,-3},{-4,-2},{-5,-1},{-6,0},{-5,1},{-4,2},{-3,3},
  {-2,4},{-1,5}
};
static const int NDYX = int(sizeof(DYX)/sizeof(DYX[0]));

// The sigma ladder the groups are drawn from (common.c, sigma_a[]).
static const double SIGMA[16] = {
  0.15, 0.26, 0.38, 0.57, 0.83, 1.18, 1.65, 2.31,
  3.22, 4.47, 6.19, 8.55,11.80,16.27,22.42,30.89 };
static const double QTREE_PROB[7] = {0.05,0.2,0.35,0.5,0.65,0.8,0.95};

typedef double cost_t;

// -------------------------------------------------------------
// Rangecoder plumbing.  sh_v2f.inc already carries the cumulative-frequency
// entry points MRP's models want; rc_BProcess is only used by the header
// and fallback models.
// -------------------------------------------------------------
RangecoderM rc;
uint f_DEC = 0;

// A plain adaptive binary probability: no ParamUpdater, one shift.
struct BC {
  word p;
  void Init() { p = SCALE/2; }
  INLINE uint Code( uint bit ) {
    bit = rc.BProcess( p, bit );
    if( bit ) p = word(p - (p>>5)); else p = word(p + ((SCALE-p)>>5));
    if( p<32 ) p = 32; else if( p>SCALE-32 ) p = SCALE-32;
    return bit;
  }
};
static uint CodeBits( BC* t, uint v, uint n ) {          // n bits, MSB first
  uint cxt = 1;
  for( uint i=0; i<n; i++ ) {
    uint bit = (v>>(n-1-i))&1;
    bit = t[cxt].Code(bit);
    cxt += cxt+bit;
  }
  return cxt - (1u<<n);
}

// -------------------------------------------------------------
// Probability models
// -------------------------------------------------------------
struct PMod {
  int   size, id;
  uint* freq;
  uint* cumfreq;
  float* cost;      // -log2 freq[k]
  float* subcost;   // +log2 of the window total that starts at base
};

static double lngammaf( double x ) {
  static const double c[6] = { 76.18009172947146, -86.50532032941677,
    24.01409824083091, -1.231739572450155, 0.1208650973866179e-2,
    -0.5395239384953e-5 };
  double y=x, t=x+5.5, s=1.000000000190015;
  t -= (x+0.5)*log(t);
  for( int j=0; j<6; j++ ) s += c[j]/++y;
  return -t + log(2.5066282746310005*s/x);
}

// One (sigma, shape) pair -> NUM_SUBPM shifted frequency tables, as
// common.c's set_freqtable: the pdf is sampled NUM_SUBPM times per integer
// so the table can be indexed by the fraction of the prediction.
static void set_freqtable( PMod* pm, double* pdf, int center,
                           int idx, double sigma ) {
  double shape = (idx<0) ? 2.0 : 3.2*(idx+1)/double(NUM_PMODEL);
  double beta  = exp(0.5*(lngammaf(3.0/shape)-lngammaf(1.0/shape))) / sigma;
  double sw    = 1.0/double(NUM_SUBPM);
  int    n     = pm->size * NUM_SUBPM;
  int    ctr   = center * NUM_SUBPM;
  for( int i=ctr; i<n; i++ ) {
    double x = (double(i)-double(ctr)+0.5)*sw;
    pdf[i+1] = exp(-pow(beta*x,shape));
  }
  for( int i=0; i<=ctr; i++ ) pdf[ctr-i] = pdf[ctr+i+1];
  for( int i=0; i<n; i++ )
    pdf[i] = (i==ctr) ? (2.0+pdf[i]+pdf[i+1])*0.5 : pdf[i]+pdf[i+1];
  for( int j=0; j<NUM_SUBPM; j++ ) {
    double norm = 0.0;
    for( int i=0; i<pm->size; i++ ) norm += pdf[i*NUM_SUBPM+j];
    norm = double(MAX_TOTFREQ - pm->size*MIN_FREQ)/norm + 1e-8;
    pm->cumfreq[0] = 0;
    for( int i=0; i<pm->size; i++ ) {
      pm->freq[i] = uint(norm*pdf[i*NUM_SUBPM+j]) + MIN_FREQ;
      pm->cumfreq[i+1] = pm->cumfreq[i] + pm->freq[i];
    }
    pm++;
  }
}

// The sigma multipliers a group may pick from.  The ladder's ratio is
// 1.38, so +-17.5% halves the worst-case distance to a rung: entry 0 is
// the rung itself, which keeps NUM_SIGV=1 bit-identical to the old model.
static const double SIGV[3] = { 1.0, 1.0/1.175, 1.175 };

// [group][variant][subposition], variant = sv*NUM_PMODEL + shape.  The
// decoder builds only the shapes the stream names, so num_pm is 1 there.
static PMod* alloc_pmodels( int num_pm, const int* pm_idx ) {
  int n = MRP_GROUP*num_pm*NUM_SUBPM;
  PMod* pm = new PMod[n];
  uint* fb = new uint[size_t(n)*(PMSIZE*2+1)];
  for( int i=0; i<n; i++ ) {
    pm[i].size    = PMSIZE;
    pm[i].freq    = fb + size_t(i)*(PMSIZE*2+1);
    pm[i].cumfreq = pm[i].freq + PMSIZE;
    pm[i].cost    = 0; pm[i].subcost = 0;
  }
  double* pdf = new double[size_t(PMSIZE)*NUM_SUBPM+1];
  for( int gr=0; gr<MRP_GROUP; gr++ )
    for( int i=0; i<num_pm; i++ ) {
      int v   = pm_idx ? pm_idx[gr] : i;
      int idx = v % NUM_PMODEL, sv = v / NUM_PMODEL;
      for( int j=0; j<NUM_SUBPM; j++ ) pm[(gr*num_pm+i)*NUM_SUBPM+j].id = i;
      set_freqtable( pm+size_t(gr*num_pm+i)*NUM_SUBPM, pdf, MAXVAL, idx,
                     SIGMA[gr]*SIGV[sv] );
    }
  delete[] pdf;
  return pm;
}

// -------------------------------------------------------------
// The generalized-Gaussian family is a prior, not a measurement: the
// residuals of a given (component, group) deviate from whichever member
// of it was picked, and always in the same direction for a given image.
// So after the loops settle, measure the deviation and transmit it.
//
// The window is [base, base+MAXVAL] and base is the prediction, so the
// table index carries the residual directly -- index i is residual
// i-MAXVAL whatever base happens to be.  That makes the correction a
// function of the index alone, and so something that can be baked into
// the frequency table once instead of evaluated per symbol.
//
// The buckets are signed log classes: sign matters because a predictor
// that is biased is biased one way.
static const int PMFIX_LVL = 8;                 // levels either side of 1.0
static const int PMFIX_BK  = 15;                // 1 + 2*7 signed log classes
// 2^(j/8) in 16.16, j = -8..8 -- a table, not pow(), because both sides
// have to build the same table (see the note on determinism in README)
static const uint PMFIX_MUL[2*PMFIX_LVL+1] = {
  32768, 35734, 38968, 42495, 46341, 50535,
  55109, 60097, 65536, 71468, 77936, 84990,
  92682, 101070, 110218, 120194, 131072,
};
// index -> bucket, and each bucket's index range: signing them keeps every
// bucket one contiguous run, which is what makes the mass of a bucket
// inside a moving window two cumfreq reads
static char pmfix_bk[PMSIZE];
static int  pmfix_lo[PMFIX_BK], pmfix_hi[PMFIX_BK];
static void pmfix_init() {
  static const int MAG[8] = { 1, 2, 4, 8, 16, 32, 64, MAXVAL+1 };
  for( int i=0; i<PMSIZE; i++ ) {
    int r = i - MAXVAL;
    if( r==0 ) { pmfix_bk[i] = 0; continue; }
    int a = (r<0) ? -r : r, c = 0;
    while( c<6 && a>=MAG[c+1] ) c++;
    pmfix_bk[i] = char( 1 + 2*c + (r<0) );
  }
  pmfix_lo[0] = MAXVAL; pmfix_hi[0] = MAXVAL+1;
  for( int c=0; c<7; c++ ) {
    int p = 1+2*c, n = p+1;
    pmfix_lo[p] = MAXVAL+MAG[c];      pmfix_hi[p] = MAXVAL+MAG[c+1];
    pmfix_lo[n] = MAXVAL-MAG[c+1]+1;  pmfix_hi[n] = MAXVAL-MAG[c]+1;
    if( pmfix_hi[p] > PMSIZE ) pmfix_hi[p] = PMSIZE;
    if( pmfix_lo[n] < 0 )      pmfix_lo[n] = 0;
  }
}

// The cost tables the optimiser reads instead of running the coder.
static const int CSTRIDE = PMSIZE + MAXVAL + 1;
static void set_costs( PMod* pm, int n ) {
  const double a = 1.0/log(2.0);
  // One allocation, fixed stride: the eight sub-positions of a group are
  // consecutive PMods, so lane s of a gather is cost[0] + s*CSTRIDE and the
  // whole 33-candidate sweep becomes two gathers per eight.
  float* blk = new float[size_t(n)*CSTRIDE];
  for( int i=0; i<n; i++ ) {
    PMod& p = pm[i];
    p.cost = blk + size_t(i)*CSTRIDE; p.subcost = p.cost+PMSIZE;
    for( int k=0; k<PMSIZE; k++ ) p.cost[k] = float(-a*log(double(p.freq[k])));
    for( int k=0; k<=MAXVAL; k++ )
      p.subcost[k] = float(a*log(double(p.cumfreq[k+MAXVAL+1]-p.cumfreq[k])));
  }
}

// MRP's set_spmodel: a geometric table for the side information.
static const int SPMAX = (MAX_UPARA+2 > MAX_COEF+1) ? MAX_UPARA+2 : MAX_COEF+1;
struct SPMod {
  uint freq[SPMAX+1], cumfreq[SPMAX+2];
  int size;
  void Set( int sz, int m ) {
    size = sz;
    if( m>=0 ) {
      double p = 1.0/double(1<<(m%8));
      uint sum = 0;
      for( int i=0; i<size; i++ ) {
        freq[i] = uint(exp(-p*i)*(1<<10)); if( !freq[i] ) freq[i]=1;
        sum += freq[i];
      }
      if( m&8 ) freq[0] = sum-freq[0];
    } else for( int i=0; i<size; i++ ) freq[i] = 1;
    cumfreq[0] = 0;
    for( int i=0; i<size; i++ ) cumfreq[i+1] = cumfreq[i]+freq[i];
  }
  cost_t Cost( int i ) const {
    return -log(double(freq[i])/double(cumfreq[size]))/log(2.0);
  }
};

static INLINE void EncSym( const PMod* pm, int lo, int hi, int s ) {
  uint off = pm->cumfreq[lo], tot = pm->cumfreq[hi]-off;
  rc.Encode( pm->cumfreq[s]-off, pm->freq[s], tot );
}
// Half the decoder's time is here: the window is always 256 symbols wide
// and the binary search over it is eight loads, each of which has to
// finish before the next address is known.  Counting instead of searching
// breaks that chain -- eight INDEPENDENT probes to pick a block of 32,
// then the block itself compared eight at a time.  Same answer, and no
// load waits on another load's result.
static INLINE int DecSym( const PMod* pm, int lo, int hi ) {
  const uint* c = pm->cumfreq + lo;
  uint off = c[0], tot = pm->cumfreq[hi]-off;
  uint v = rc.GetFreq( tot ) + off;         // compare unshifted
  int n = hi-lo, i;
#if defined(__AVX2__)
  if( n==256 ) {
    int b = 0;                              // c[32*(b+1)] > v <= c[32*b]
    b += (c[ 32]<=v) + (c[ 64]<=v) + (c[ 96]<=v) + (c[128]<=v)
       + (c[160]<=v) + (c[192]<=v) + (c[224]<=v);
    const uint* q = c + b*32 + 1;
    __m256i vv = _mm256_set1_epi32( int(v) );
    int cnt = 0;
    for( int t=0; t<32; t+=8 ) {
      __m256i x = _mm256_loadu_si256( (const __m256i*)(q+t) );
      // x <= v is the complement of x > v
      int m = _mm256_movemask_ps( _mm256_castsi256_ps(
                _mm256_cmpgt_epi32( x, vv ) ) );
      cnt += 8 - __builtin_popcount( uint(m) );
    }
    i = lo + b*32 + cnt;
    rc.Decode( pm->cumfreq[i]-off, pm->freq[i], tot );
    return i;
  }
#endif
  { int a=lo, j=hi-1;
    while( a<j ) { int k=(a+j)>>1; if( pm->cumfreq[k+1] <= v ) a=k+1; else j=k; }
    i = a; }
  rc.Decode( pm->cumfreq[i]-off, pm->freq[i], tot );
  return i;
}
static INLINE void EncSP( const SPMod& p, int s ) {
  rc.Encode( p.cumfreq[s], p.freq[s], p.cumfreq[p.size] );
}
static INLINE int DecSP( const SPMod& p ) {
  uint v = rc.GetFreq( p.cumfreq[p.size] );
  int i=0, j=p.size-1;
  while( i<j ) { int k=(i+j)>>1; if( p.cumfreq[k+1]<=v ) i=k+1; else j=k; }
  rc.Decode( p.cumfreq[i], p.freq[i], p.cumfreq[p.size] );
  return i;
}

// -------------------------------------------------------------
// The codec
// -------------------------------------------------------------
struct MRPC {
  uint W, H, nc, stride, BS;
  int  num_class;
  int  ord[MAXC];             // coding position -> component of the raster

  byte*  org;                 // bordered raster, nc bytes/pixel
  short* prd;                 // clamped fixed-point prediction
  short* errB;                // bordered |2e|, same layout as org
  short* upara;
  char*  grp;
  char*  cls;                 // one class per pixel, shared by components

  int  nt[MAXC];
  int  toff[MAXC][NTMAX];
  // The same taps in de-interleaved coordinates.  org is nc bytes per
  // pixel, so eight consecutive pixels of one component are a stride-nc
  // gather; out of a per-component plane they are one 8-byte load, which
  // is the difference between a gather and a vector.
  byte* plane[MAXC];
  int  tpl[MAXC][NTMAX];      // which plane each tap reads
  int  tof[MAXC][NTMAX];      // its offset, in plane elements
  int  uoff[NUM_UPELS], uw[NUM_UPELS];

  int  (*coef)[MAXC][NTMAX];  // [class][component][tap]
  int  th[MRP_MAXCLASS][MAXC][MRP_GROUP];
  char (*uq)[MAXC][MAX_UPARA+1];
  int  pm_idx[MAXC][MRP_GROUP];

  PMod*  pmodels;
  int    num_pm;
  PMod*  pml[MAXC][MRP_GROUP];
  // the same thing as a bare pointer: the inner loops were paying two
  // dependent loads (pml -> PMod -> cost) for every pixel and component
  const float* pmlc[MAXC][MRP_GROUP];

  cost_t class_cost[MRP_MAXCLASS];
  cost_t qtflag_cost[QT_DEPTH<<3];
  cost_t th_cost[MAX_UPARA+2];
  cost_t coef_cost[16][MAX_COEF+1];
  int    coef_m[MAXC][NTMAX];
  int    qtctx[QT_DEPTH<<3];
  int    qtree_code[QT_DEPTH<<2];
  char*  qtmap[QT_DEPTH];
  int    qtw[QT_DEPTH], qth[QT_DEPTH];
  int    mtfbuf[MRP_MAXCLASS];
  int    opt_loop;
  uint   rnd;
  uint   tb0[TRIAL_BANDS], tb1[TRIAL_BANDS];   // rows the trial looks at
  int    tnb;                                  // 0 = the whole image
  ALIGN(32) int cA[40], cB[40];   // n/SSR and n%SSR of the 33 candidates
  // Pixels grouped by class.  The coefficient search visits one class at a
  // time and used to find it by scanning the whole image -- once per
  // (class, component, tap pair), which on this corpus is ten thousand
  // full-image scans to reach a sixty-third of the pixels each time.
  uint*  cpo;                 // bordered offset (org, errB)
  uint*  cpq;                 // plain offset (prd, grp)
  size_t cbeg[MRP_MAXCLASS+1];

  INLINE uint Rand() { rnd = rnd*1664525u + 1013904223u; return rnd>>8; }
  INLINE size_t OI( uint y, uint x ) const {
    return (size_t(y+PADT)*BS + (x+PADL))*nc;
  }
  INLINE size_t PI( uint y, uint x ) const { return (size_t(y)*W + x)*nc; }

  // ---------------------------------------------------------------
  void BuildTaps() {
    for( uint k=0; k<nc; k++ ) {
      int n = 0;
      for( int i=0; i<PRD_ORDER && i<NDYX; i++ )
        toff[k][n++] = (DYX[i][0]*int(BS) + DYX[i][1])*int(nc) + int(k);
      for( uint j=0; j<nc; j++ ) {
        if( j==k ) continue;
        for( int i=0; i<XPRD_ORDER && i<NDYX; i++ )
          toff[k][n++] = (DYX[i][0]*int(BS) + DYX[i][1])*int(nc) + int(j);
      }
#if XCUR
      // the components of this pixel already coded: offset 0, and the one
      // thing a per-plane coder cannot say
      for( uint j=0; j<k; j++ ) toff[k][n++] = int(j);
#endif
      nt[k] = n;
    }
    for( uint k=0; k<nc; k++ ) {
      int n = 0;
      for( int i=0; i<PRD_ORDER && i<NDYX; i++, n++ ) {
        tpl[k][n] = int(k); tof[k][n] = DYX[i][0]*int(BS) + DYX[i][1];
      }
      for( uint j=0; j<nc; j++ ) {
        if( j==k ) continue;
        for( int i=0; i<XPRD_ORDER && i<NDYX; i++, n++ ) {
          tpl[k][n] = int(j); tof[k][n] = DYX[i][0]*int(BS) + DYX[i][1];
        }
      }
#if XCUR
      for( uint j=0; j<k; j++, n++ ) { tpl[k][n] = int(j); tof[k][n] = 0; }
#endif
    }
    for( int i=0; i<NUM_UPELS; i++ ) {
      uoff[i] = (DYX[i][0]*int(BS) + DYX[i][1])*int(nc);
      double d = sqrt(double(DYX[i][0]*DYX[i][0] + DYX[i][1]*DYX[i][1]));
      uw[i] = int(64.0/d + 0.5);
    }
  }

  INLINE int Predict( uint k, const byte* p, const int* cf ) const {
    int v = 0;
    const int* to = toff[k];
    for( int i=0, n=nt[k]; i<n; i++ ) v += int(p[to[i]]) * cf[i];
    return v;
  }
  static INLINE int Clip( int v ) {
    return (v<0) ? 0 : (v>MAXPRD ? MAXPRD : v);
  }
  // |2*org - prd_half| with MRP's econv twist, which is what the activity
  // measure and the group quantiser are calibrated on.
  static INLINE int Econv( int org_, int prdc ) {
    int j = prdc >> (COEF_PREC-1);
    int t = (org_<<1) - j;
    return (t>0) ? t-1 : -t;
  }
#if defined(__AVX2__)
  // All nc components at once: a pixel's error values are adjacent in
  // errB, so one 64-bit load per neighbour covers every component and the
  // twelve-tap weighted sum becomes twelve vector madds instead of 12*nc
  // scalar ones.  Returns the sums before the shift, as the scalar does.
  INLINE void ActivityN( const short* e, int* u ) const {
    __m128i acc = _mm_setzero_si128();
    for( int i=0; i<NUM_UPELS; i++ ) {
      __m128i v = _mm_cvtepi16_epi32(
                    _mm_loadl_epi64( (const __m128i*)(e+uoff[i]) ) );
      acc = _mm_add_epi32( acc, _mm_mullo_epi32( v, _mm_set1_epi32(uw[i]) ) );
    }
    _mm_storeu_si128( (__m128i*)u, acc );
  }
#endif
  INLINE int Activity( uint k, const short* e ) const {
    int u = 0;
    for( int i=0; i<NUM_UPELS; i++ ) u += int(e[uoff[i]+int(k)]) * uw[i];
#if XUPEL
    for( uint j=0; j<k; j++ ) u += int(e[j]) * 64;
#endif
    u >>= 6;
    return (u>MAX_UPARA) ? MAX_UPARA : u;
  }
  // base and subposition of the pmf window for a clamped prediction
  static INLINE int Qprd( int prdc ) {
    return (MAXPRD - prdc + (1<<(COEF_PREC-PM_ACC-1))) >> (COEF_PREC-PM_ACC);
  }

  // ---------------------------------------------------------------
  // Borders.  Every cell outside the image is a function of position
  // alone -- the optimiser recomputes predictions out of raster order, so
  // a cell may not mean one thing during row y and another after it -- and
  // every one of them is filled from a pixel that is already coded when
  // the first reader needs it:
  //   left  (y,-j)      := pixel (y-1, 0)          j = 1..PADL
  //   right (y,W-1+j)   := pixel (y,   W-1)
  //   top   (-j, x)     := pixel (0, x-PADR-1)     the earliest reader of
  //                        that cell is pixel (0, x-PADR)
  // which is edge replication with a lag, and reproduces what MRP gets by
  // clamping its per-pixel offset tables.
  void BorderLeft( uint y ) {
    byte*  p = org  + OI(y,0);
    short* e = errB + OI(y,0);
    byte   sv[MAXC]; short se[MAXC];
    for( uint k=0; k<nc; k++ ) {
      sv[k] = (y>0) ? p[-int(BS)*int(nc)+int(k)] : 128;
      se[k] = (y>0) ? e[-int(BS)*int(nc)+int(k)] : 0;
    }
    for( int j=1; j<=PADL; j++ )
      for( uint k=0; k<nc; k++ ) {
        p[-j*int(nc)+int(k)] = sv[k];
        e[-j*int(nc)+int(k)] = se[k];
      }
  }
  void BorderRight( uint y ) {
    byte*  p = org  + OI(y,W-1);
    short* e = errB + OI(y,W-1);
    for( int j=1; j<=PADR; j++ )
      for( uint k=0; k<nc; k++ ) {
        p[j*int(nc)+int(k)] = p[k];
        e[j*int(nc)+int(k)] = e[k];
      }
  }
  // the cell (-j, x) for every j, from pixel (0, src)
  void BorderTopCol( int x, int src ) {
    if( x < -PADL || x >= int(W)+PADR ) return;
    byte*  s = org  + OI(0,0) + size_t(src)*nc;
    short* q = errB + OI(0,0) + size_t(src)*nc;
    for( int j=1; j<=PADT; j++ ) {
      byte*  p = org  + OI(0,0) + (-j*int(BS) + x)*int(nc);
      short* e = errB + OI(0,0) + (-j*int(BS) + x)*int(nc);
      for( uint k=0; k<nc; k++ ) { p[k] = s[k]; e[k] = q[k]; }
    }
  }
  // encoder-side: the whole border at once, org already loaded
  void FillBorders() {
    for( uint y=0; y<H; y++ ) { BorderLeft(y); BorderRight(y); }
    for( int x=-PADL; x<int(W)+PADR; x++ ) {
      int s = x-PADR-1;
      if( s<0 ) continue;                 // stays at the 128/0 fill
      if( s>int(W)-1 ) s = int(W)-1;
      BorderTopCol( x, s );
    }
  }

  // org -> planes.  Encoder only: the decoder predicts one pixel at a
  // time and has nothing to batch.
  INLINE size_t PL( uint y, uint x ) const {
    return size_t(y+PADT)*BS + (x+PADL);
  }
  void BuildPlanes() {
    size_t bn = size_t(H+PADT)*BS;
    for( uint k=0; k<nc; k++ ) {
      if( !plane[k] ) plane[k] = new byte[bn];
      const byte* p = org + k;
      byte* d = plane[k];
      for( size_t i=0; i<bn; i++, p+=nc ) d[i] = *p;
    }
  }

#if defined(__AVX2__)
  // Eight consecutive pixels of component k under one coefficient vector.
  INLINE void Predict8( uint k, const int* cf, size_t o, int* out ) const {
    __m256i a0 = _mm256_setzero_si256(), a1 = _mm256_setzero_si256();
    const int* tp = tpl[k]; const int* to = tof[k];
    int n = nt[k], i = 0;
    for( ; i+2<=n; i+=2 ) {
      const byte* q0 = plane[tp[i]]   + o + to[i];
      const byte* q1 = plane[tp[i+1]] + o + to[i+1];
      __m256i v0 = _mm256_cvtepu8_epi32( _mm_loadl_epi64((const __m128i*)q0) );
      __m256i v1 = _mm256_cvtepu8_epi32( _mm_loadl_epi64((const __m128i*)q1) );
      a0 = _mm256_add_epi32( a0, _mm256_mullo_epi32( v0, _mm256_set1_epi32(cf[i]) ) );
      a1 = _mm256_add_epi32( a1, _mm256_mullo_epi32( v1, _mm256_set1_epi32(cf[i+1]) ) );
    }
    for( ; i<n; i++ ) {
      const byte* q = plane[tp[i]] + o + to[i];
      __m256i v = _mm256_cvtepu8_epi32( _mm_loadl_epi64((const __m128i*)q) );
      a0 = _mm256_add_epi32( a0, _mm256_mullo_epi32( v, _mm256_set1_epi32(cf[i]) ) );
    }
    _mm256_storeu_si256( (__m256i*)out, _mm256_add_epi32(a0,a1) );
  }
  // clamp to [0,MAXPRD] and the doubled absolute error, eight at a time
  INLINE void ClipErr8( const int* pv, const byte* org8, size_t ostep,
                        short* prdo, short* erro, size_t stepo ) const {
    for( int i=0; i<8; i++ ) {
      int v = Clip( pv[i] );
      prdo[i*stepo] = short(v);
      erro[i*stepo] = short( Econv( int(org8[i*ostep]), v ) );
    }
  }
#endif

  // ---------------------------------------------------------------
  void PredictRegion( uint y0, uint x0, uint y1, uint x1 ) {
    const int full = BORDER_FIX && (x0==0) && (x1==W) && (y0==0) && (y1==H);
    // Rows are independent here: the predictor reads `org`/`plane` and
    // writes `prd`/`errB`, and nothing in this pass reads what it writes.
    // Every pixel's result is a function of its own position, so the
    // output does not depend on the schedule -- no reduction, no order.
    // Only worth a team on the full-image calls; the class search runs
    // this on 32-row blocks from inside a parallel region already.
#ifdef _OPENMP
    #pragma omp parallel for schedule(static) if( y1-y0 >= 64 )
#endif
    for( uint y=y0; y<y1; y++ ) {
      const char* pc = cls + size_t(y)*W;
      uint x = x0;
#if defined(__AVX2__)
      // eight at a time as long as the class holds; the quadtree bottoms
      // out at MIN_BSIZE so runs of one class are at least that long
      ALIGN(32) int pv[8];
      while( x+8 <= x1 ) {
        int cl = int(pc[x]);
        uint r = x+1;
        while( r<x1 && int(pc[r])==cl ) r++;
        if( r-x < 8 ) {                      // short run: scalar
          const byte* p = org + OI(y,x);
          short* pr = prd + PI(y,x); short* pe = errB + OI(y,x);
          for( uint t=x; t<r; t++, p+=nc, pr+=nc, pe+=nc )
            for( uint k=0; k<nc; k++ ) {
              int v = Clip( Predict( k, p, coef[cl][k] ) );
              pr[k] = short(v); pe[k] = short( Econv( int(p[k]), v ) );
            }
          x = r; continue;
        }
        for( ; x+8<=r; x+=8 ) {
          size_t o = PL(y,x);
          for( uint k=0; k<nc; k++ ) {
            Predict8( k, coef[cl][k], o, pv );
            ClipErr8( pv, org+OI(y,x)+k, nc, prd+PI(y,x)+k, errB+OI(y,x)+k, nc );
          }
        }
      }
#endif
      {
        const byte* p = org + OI(y,x);
        short* pr = prd + PI(y,x);
        short* pe = errB + OI(y,x);
        for( ; x<x1; x++, p+=nc, pr+=nc, pe+=nc ) {
          int cl = int(pc[x]);
          for( uint k=0; k<nc; k++ ) {
            int v = Clip( Predict( k, p, coef[cl][k] ) );
            pr[k] = short(v);
            pe[k] = short( Econv( int(p[k]), v ) );
          }
        }
      }
    }
    // the margins now hold the same errors CodeImage will put there
    if( full ) FillBorders();
  }

  // Same as CalcCost, but gives up as soon as the running total passes
  // `bound`.  The class search asks 63 questions per block and only wants
  // the smallest answer, so most candidates need only be shown to be
  // worse than one that is already known.  `>` not `>=`, so a candidate
  // that would tie still completes and the tie-break is unchanged.
  cost_t CalcCostB( uint y0, uint x0, uint y1, uint x1, cost_t bound ) {
    cost_t cost = 0;
    for( uint y=y0; y<y1; y++ ) {
      cost += CalcCost( y, x0, y+1, x1 );
      if( cost > bound ) return cost;
    }
    return cost;
  }
  cost_t CalcCost( uint y0, uint x0, uint y1, uint x1 ) {
    cost_t cost = 0;
    for( uint y=y0; y<y1; y++ ) {
      const byte* p = org + OI(y,x0);
      const short* pr = prd + PI(y,x0);
      const short* pe = errB + OI(y,x0);
      short* pu = upara + PI(y,x0);
      char*  pg = grp + PI(y,x0);
      const char* pc = cls + size_t(y)*W + x0;
      for( uint x=x0; x<x1; x++ ) {
        int cl = int(*pc++);
#if defined(__AVX2__)
        ALIGN(16) int uv[4]; ActivityN( pe, uv );
#endif
        for( uint k=0; k<nc; k++ ) {
#if defined(__AVX2__)
          int u = uv[k];
          for( uint j=0; j<k*XUPEL; j++ ) u += int(pe[j])*64;
          u >>= 6; if( u>MAX_UPARA ) u = MAX_UPARA;
#else
          int u  = Activity( k, pe );
#endif
          pu[k]  = short(u);
          int gr = int(uq[cl][k][u]);
          pg[k]  = char(gr);
          int q  = Qprd( int(pr[k]) );
          const float* cq = pmlc[k][gr] + (q & (NUM_SUBPM-1))*CSTRIDE;
          int base = q >> PM_ACC;
          cost += cq[base + int(p[k])] + cq[PMSIZE + base];
        }
        p += nc; pr += nc; pe += nc; pu += nc; pg += nc;
      }
    }
    return cost;
  }

  // ---------------------------------------------------------------
  // Initial segmentation: 8x8 blocks ranked by variance, split evenly
  // across the classes (MRP's init_class, summed over components).
  void InitClass() {
    uint nbx = W/BASE_BSIZE, nby = H/BASE_BSIZE;
    if( nbx==0 || nby==0 ) { memset( cls, 0, size_t(W)*H ); return; }
    size_t nb = size_t(nbx)*nby;
    double* var = new double[nb];
    int* idx = new int[nb];
    for( size_t b=0; b<nb; b++ ) {
      uint by = uint(b/nbx)*BASE_BSIZE, bx = uint(b%nbx)*BASE_BSIZE;
#if INIT_METRIC
      double g = 0;
      for( uint i=0; i<BASE_BSIZE; i++ ) {
        const byte* p = org + OI(by+i,bx);
        const byte* q = org + OI(by+i-1,bx);
        for( uint j=0; j<BASE_BSIZE*nc; j++ ) {
          int dx = int(p[j]) - int(p[int(j)-int(nc)]);
          int dy = int(p[j]) - int(q[j]);
          g += double((dx<0?-dx:dx) + (dy<0?-dy:dy));
        }
      }
      var[b] = g;
#else
      double s=0, s2=0;
      for( uint i=0; i<BASE_BSIZE; i++ ) {
        const byte* p = org + OI(by+i,bx);
        for( uint j=0; j<BASE_BSIZE*nc; j++ ) { double v=p[j]; s+=v; s2+=v*v; }
      }
      var[b] = s2 - s*s/double(BASE_BSIZE*BASE_BSIZE*nc);
#endif
      idx[b] = int(b);
    }
    // Stable merge sort by (variance, block index).  The insertion sort
    // this replaces is O(nb^2) -- 262144 blocks at 16 MP is ~17G shifts,
    // once per encode -- and stability matters: the initial assignment
    // feeds the whole nonconvex search, so an unstable order moves the
    // stream.
#if MRP_OLDSORT
    for( size_t i=1; i<nb; i++ ) {
      int t = idx[i]; size_t j = i;
      while( j>0 && var[idx[j-1]] > var[t] ) { idx[j] = idx[j-1]; j--; }
      idx[j] = t;
    }
#else
    { int* tmp = new int[nb];
      for( size_t w=1; w<nb; w*=2 ) {
        for( size_t lo=0; lo<nb; lo+=2*w ) {
          size_t mid = (lo+w<nb) ? lo+w : nb, hi = (lo+2*w<nb) ? lo+2*w : nb;
          size_t a=lo, b=mid, o=lo;
          while( a<mid && b<hi )
            tmp[o++] = ( var[idx[b]] < var[idx[a]] ) ? idx[b++] : idx[a++];
          while( a<mid ) tmp[o++] = idx[a++];
          while( b<hi  ) tmp[o++] = idx[b++];
        }
        for( size_t i=0; i<nb; i++ ) idx[i] = tmp[i];
      }
      delete[] tmp; }
#endif
    memset( cls, 0, size_t(W)*H );
    for( size_t k=0; k<nb; k++ ) {
      int cl = int((k*size_t(num_class))/nb);
      uint by = uint(size_t(idx[k])/nbx)*BASE_BSIZE;
      uint bx = uint(size_t(idx[k])%nbx)*BASE_BSIZE;
      for( uint i=0; i<BASE_BSIZE; i++ )
        for( uint j=0; j<BASE_BSIZE; j++ )
          cls[size_t(by+i)*W + bx+j] = char(cl);
    }
    delete[] idx; delete[] var;
  }

  // ---------------------------------------------------------------
  // Least squares per (class, component).  MRP weights each sample by
  // 1/sigma[group]^2 once the rate model is in force, which is the
  // statement that a sample from a noisy neighbourhood should not drag the
  // predictor as hard as one from a smooth one.
  // The normal equations only, over rows [y0,y1): the component-order
  // trial fits on a band rather than the whole image, which is a choice
  // between n! candidates and does not need every pixel to make it.
  void FitPredictor( int mmse ) {
    double wg[MRP_GROUP];
    for( int g=0; g<MRP_GROUP; g++ )
      wg[g] = mmse ? 1.0 : 1.0/(SIGMA[g]*SIGMA[g]);
    int nck = num_class*int(nc);
    // one normal-equation system per (class, component), all independent;
    // the scratch that used to be `static` is per-task now
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,1) if(!tnb)
#endif
    for( int ii=0; ii<nck; ii++ ) {
      int cl = ii/int(nc); uint k = uint(ii%int(nc));
      static thread_local double mat[NTMAX][NTMAX+1];
      static thread_local int    piv[NTMAX];
      int n = nt[k];
      for( int i=0; i<n; i++ ) for( int j=0; j<=n; j++ ) mat[i][j] = 0.0;
      // The tap values go into a contiguous vector once, and the rank-1
      // update of the upper triangle is then FMAs down each row -- 861
      // scalar multiply-adds per pixel become 216 vector ones, with the
      // right-hand column carried in element n of the same sweep.
      ALIGN(32) double xv[NTMAX+8];
      const int* to = toff[k];
      // The pixels of one class, from the class list.  Scanning the whole
      // image per (class, component) to touch a sixty-third of it cost
      // C*K full passes over cls; the list costs one.
      // names deliberately unlike the caller's: FITPIX(p,pg) would expand
      // to `const byte* p = p;`, which is self-initialisation, not shadowing
      #define FITPIX(P,PG) do {                                          \
          const byte* fp_ = (P); const char* fg_ = (PG);                  \
          double w = wg[int(fg_[k])];                                     \
          for( int i=0; i<n; i++ ) xv[i] = double(fp_[to[i]]);            \
          xv[n] = double(fp_[k]);                                         \
          for( int i=0; i<n; i++ ) {                                      \
            double a = w*xv[i];                                           \
            double* m = mat[i];                                           \
            int j = i;                                                    \
            AVXFIT                                                        \
            for( ; j<=n; j++ ) m[j] += a*xv[j];                           \
          }                                                               \
        } while(0)
#if defined(__AVX2__) && defined(__FMA__)
      #define AVXFIT  { __m256d va = _mm256_set1_pd(a);                   \
          for( ; j+4<=n+1; j+=4 )                                         \
            _mm256_storeu_pd( m+j, _mm256_fmadd_pd( va,                   \
              _mm256_loadu_pd(xv+j), _mm256_loadu_pd(m+j) ) ); }
#else
      #define AVXFIT
#endif
      if( tnb ) {                        // order trial: bands, one class
        for( int b=0; b<tnb; b++ )
        for( uint y=tb0[b]; y<tb1[b]; y++ ) {
          const char* pc = cls + size_t(y)*W;
          const byte* p  = org + OI(y,0);
          const char* pg = grp + PI(y,0);
          for( uint x=0; x<W; x++, p+=nc, pg+=nc ) {
            if( int(pc[x])!=cl ) continue;
            FITPIX( p, pg );
          }
        }
      } else {
        for( size_t ci=cbeg[cl], ce=cbeg[cl+1]; ci<ce; ci++ )
          FITPIX( org + cpo[ci], grp + cpq[ci] );
      }
      #undef FITPIX
      #undef AVXFIT
      for( int i=0; i<n; i++ ) for( int j=0; j<i; j++ ) mat[i][j] = mat[j][i];
      // Gauss-Jordan with partial pivoting
      for( int i=0; i<n; i++ ) piv[i] = i;
      for( int i=0; i<n; i++ ) {
        double best = 0.0; int bp = i;
        for( int k2=i; k2<n; k2++ ) {
          double v = fabs(mat[piv[k2]][i]);
          if( v>best ) { best = v; bp = k2; }
        }
        int t = piv[i]; piv[i] = piv[bp]; piv[bp] = t;
        if( best <= 1e-10 ) continue;
        double d = mat[piv[i]][i];
        for( int j=i; j<=n; j++ ) mat[piv[i]][j] /= d;
        for( int k2=0; k2<n; k2++ ) {
          if( k2==i ) continue;
          double f = mat[piv[k2]][i];
          for( int j=i; j<=n; j++ ) mat[piv[k2]][j] -= f*mat[piv[i]][j];
        }
      }
      // quantise, then hand the accumulated rounding back to the taps that
      // lost the most of it -- a predictor whose coefficients sum low
      // darkens the whole class
      double e = 0.0;
      for( int i=0; i<n; i++ ) {
        double d = (fabs(mat[piv[i]][i])>1e-10)
                 ? mat[piv[i]][n]*double(1<<COEF_PREC) : 0.0;
        int q = int(d); if( double(q)>d ) q--;
        if( q < -MAX_COEF ) { q = -MAX_COEF; d = q; }
        else if( q > MAX_COEF ) { q = MAX_COEF; d = q; }
        coef[cl][k][i] = q;
        d -= q; e += d;
        mat[piv[i]][n] = d;
      }
      for( int r=int(e+0.5); r>0; r-- ) {
        double d = 0.0; int j = 0;
        for( int i=0; i<n; i++ ) if( mat[piv[i]][n] > d ) { d = mat[piv[i]][n]; j = i; }
        if( coef[cl][k][j] < MAX_COEF ) coef[cl][k][j]++;
        mat[piv[j]][n] = 0.0;
      }
      for( int i=n; i<NTMAX; i++ ) coef[cl][k][i] = 0;
    }
  }
  cost_t DesignPredictor( int mmse ) {
    if( !tnb ) BuildClassList();
    FitPredictor( mmse );
    PredictRegion( 0, 0, H, W );
    return CalcCost( 0, 0, H, W );
  }

  // ---------------------------------------------------------------
  // Group quantiser: the thresholds on the activity are chosen by dynamic
  // programming over the exact code length, per class and component.
  cost_t OptimizeGroup() {
    size_t nu = MAX_UPARA+2;
    // cost of putting activity u in group gr, per (class, component)
    static cost_t* cbuf = 0;
    size_t need = size_t(num_class)*nc*MRP_GROUP*nu;
    if( !cbuf ) cbuf = new cost_t[size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*nu];
    for( size_t i=0; i<need; i++ ) cbuf[i] = 0.0;
    #define CB(cl,k,gr) (cbuf + ((size_t(cl)*nc + (k))*MRP_GROUP + (gr))*nu)
    static short umx[MRP_MAXCLASS][MAXC];
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) umx[cl][k] = 0;
    // Component-outer, not pixel-outer: every bin this pass touches is
    // indexed by k, so the components own disjoint memory and each bin
    // still receives its pixels in raster order -- the sums come out to
    // the same last bit at any thread count.  The activity is recomputed
    // per component rather than once for all of them, which is nc times
    // the work spread over nc threads: the same wall time for that part,
    // and the sixteen-group table loop that dominates goes nc-wide.
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for( int kk=0; kk<int(nc); kk++ ) {
      const uint k = uint(kk);
      for( uint y=0; y<H; y++ ) {
        const byte* p = org + OI(y,0);
        const short* pr = prd + PI(y,0);
        const short* pe = errB + OI(y,0);
        short* pu = upara + PI(y,0);
        const char* pc = cls + size_t(y)*W;
        for( uint x=0; x<W; x++, p+=nc, pr+=nc, pe+=nc, pu+=nc ) {
          int cl = int(pc[x]);
#if defined(__AVX2__)
          ALIGN(16) int uv[4]; ActivityN( pe, uv );
          int u = uv[k];
          for( uint j=0; j<k*XUPEL; j++ ) u += int(pe[j])*64;
          u >>= 6; if( u>MAX_UPARA ) u = MAX_UPARA;
#else
          int u = Activity( k, pe );
#endif
          pu[k] = short(u);
          if( u > umx[cl][k] ) umx[cl][k] = short(u);
          int q = Qprd( int(pr[k]) ), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
          int v = base + int(p[k]);
          for( int gr=0; gr<MRP_GROUP; gr++ ) {
            const float* cq = pmlc[k][gr] + size_t(sub)*CSTRIDE;
            CB(cl,k,gr)[u+1] += cq[v] + cq[PMSIZE+base];
          }
        }
      }
    }
    // one trellis per (class, component), reading its own slice of cbuf
    // and writing its own th/uq rows: independent, so collapse them
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,1) collapse(2)
#endif
    for( int cl=0; cl<num_class; cl++ ) for( int kk=0; kk<int(nc); kk++ ) {
      const uint k = uint(kk);
      static thread_local cost_t dp[MAX_UPARA+2];
      static thread_local int    tre[MRP_GROUP][MAX_UPARA+2];
#if DP_SHRINK
      // nothing above the largest observed activity can carry cost, and
      // the last group covers everything above the last threshold anyway
      size_t nue = size_t(umx[cl][k]) + 2;
      if( nue > nu ) nue = nu;
      if( nue < size_t(MRP_GROUP)+1 ) nue = size_t(MRP_GROUP)+1;
      if( nue > nu ) nue = nu;
#else
      const size_t nue = nu;
#endif
      for( int gr=0; gr<MRP_GROUP; gr++ ) {
        cost_t* c = CB(cl,k,gr);
        for( size_t u=1; u<nue; u++ ) c[u] += c[u-1];
      }
      for( size_t u=0; u<nue; u++ ) { dp[u] = CB(cl,k,0)[u]; tre[0][u] = 0; }
      for( int gr=1; gr<MRP_GROUP; gr++ ) {
        const cost_t* c = CB(cl,k,gr);
        for( int th1=int(nue)-1; th1>=0; th1-- ) {
          int th0 = th1;
          cost_t mc = dp[th1] - c[th1] + th_cost[0]
                    + th_cost[th0 - tre[gr-1][th0]];
          for( int t=0; t<th1; t++ ) {
            cost_t cc = dp[t] - c[t] + th_cost[th1-t]
                      + th_cost[t - tre[gr-1][t]];
            if( cc<mc ) { mc = cc; th0 = t; }
          }
          dp[th1] = mc + c[th1];
          tre[gr][th1] = th0;
          if( gr==MRP_GROUP-1 ) break;
        }
      }
      int t = int(nue)-1;
      for( int gr=MRP_GROUP-1; gr>0; gr-- ) { t = tre[gr][t]; th[cl][k][gr-1] = t; }
      th[cl][k][MRP_GROUP-1] = MAX_UPARA+1;
      int u = 0;
      for( int gr=0; gr<MRP_GROUP; gr++ )
        for( ; u<th[cl][k][gr] && u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(gr);
      for( ; u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(MRP_GROUP-1);
    }
    #undef CB
    cost_t cost = CalcCost( 0, 0, H, W );

    // and which (shape, sigma) variant each group should use.  The pass
    // costs one table read per candidate per (pixel, component), so it is
    // run twice over small candidate sets rather than once over their
    // product: sixteen shapes at the group's own sigma, then the sigma
    // variants of the shape that won.  19 reads instead of 48, and the
    // stages disagree only where the argmin shape moves with sigma.
    if( opt_loop>1 && num_pm>1 ) {
      static cost_t pc2[MAXC][MRP_GROUP][NUM_PMODEL*NUM_SIGV];
      static int    cnd[MAXC][MRP_GROUP][NUM_PMODEL*NUM_SIGV];
      int ncand = SIGV_2STAGE ? NUM_PMODEL : num_pm;
      for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
        for( int i=0; i<ncand; i++ ) cnd[k][g][i] = i;
      for( int stage=0; stage<(SIGV_2STAGE && NUM_SIGV>1 ? 2 : 1); stage++ ) {
        for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
          for( int i=0; i<ncand; i++ ) pc2[k][g][i] = 0.0;
        // same component-outer split as the histogram, and for the same
        // reason: pc2[k] is private to k and each bin keeps raster order
#ifdef _OPENMP
        #pragma omp parallel for schedule(static)
#endif
        for( int kk=0; kk<int(nc); kk++ ) {
          const uint k = uint(kk);
          for( uint y=0; y<H; y++ ) {
            const byte* p = org + OI(y,0);
            const short* pr = prd + PI(y,0);
            const char* pg = grp + PI(y,0);
            for( uint x=0; x<W; x++, p+=nc, pr+=nc, pg+=nc ) {
              int gr = int(pg[k]);
              int q = Qprd( int(pr[k]) ), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
              int v = base + int(p[k]);
              const int* cd = cnd[k][gr];
              for( int i=0; i<ncand; i++ ) {
                const PMod* pm = pmodels + (size_t(gr)*num_pm+cd[i])*NUM_SUBPM + sub;
                pc2[k][gr][i] += pm->cost[v] + pm->subcost[base];
              }
            }
          }
        }
        cost = 0.0;
        for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ ) {
          int best = 0;
          for( int i=1; i<ncand; i++ )
            if( pc2[k][g][i] < pc2[k][g][best] ) best = i;
          pm_idx[k][g] = cnd[k][g][best];
          pml[k][g] = pmodels + (size_t(g)*num_pm+pm_idx[k][g])*NUM_SUBPM;
          pmlc[k][g] = pml[k][g]->cost;
          cost += pc2[k][g][best];
        }
        if( stage==0 ) {                       // now the sigmas of that shape
          for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
            for( int s=0; s<NUM_SIGV; s++ )
              cnd[k][g][s] = s*NUM_PMODEL + pm_idx[k][g];
          ncand = NUM_SIGV;
        }
      }
    }
    return cost;
  }

  // ---------------------------------------------------------------
  // Class map.  Blocks are tried against every class and, in the second
  // loop, split down a quadtree wherever splitting pays for its own flag.
  short* prdbuf; short* errbuf;

  void SetPrdBuf( uint tly, uint tlx, uint bufsize ) {
    uint brx = (tlx+bufsize<W) ? tlx+bufsize : W;
    uint bry = (tly+bufsize<H) ? tly+bufsize : H;
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for( int cl=0; cl<num_class; cl++ ) {
      size_t bp = size_t(cl)*bufsize*bufsize*nc;
      for( uint y=tly; y<bry; y++ ) {
        short* pb0 = prdbuf + bp + (size_t(y%bufsize)*bufsize + tlx%bufsize)*nc;
        short* eb0 = errbuf + bp + (size_t(y%bufsize)*bufsize + tlx%bufsize)*nc;
        uint x = tlx;
#if defined(__AVX2__)
        // predict the whole row under this class, then put back the pixels
        // that already have it -- branchless beats correct-but-scalar here
        ALIGN(32) int pv[8];
        for( ; x+8<=brx; x+=8 ) {
          size_t o = PL(y,x);
          short* pb = pb0 + size_t(x-tlx)*nc;
          short* eb = eb0 + size_t(x-tlx)*nc;
          for( uint k=0; k<nc; k++ ) {
            Predict8( k, coef[cl][k], o, pv );
            ClipErr8( pv, org+OI(y,x)+k, nc, pb+k, eb+k, nc );
          }
          for( uint t=0; t<8; t++ ) if( int(cls[size_t(y)*W+x+t])==cl ) {
            const short* pr = prd + PI(y,x+t);
            const short* pe = errB + OI(y,x+t);
            for( uint k=0; k<nc; k++ ) { pb[t*nc+k] = pr[k]; eb[t*nc+k] = pe[k]; }
          }
        }
#endif
        {
          const byte* p = org + OI(y,x);
          short* pb = pb0 + size_t(x-tlx)*nc;
          short* eb = eb0 + size_t(x-tlx)*nc;
          for( ; x<brx; x++, p+=nc, pb+=nc, eb+=nc ) {
            if( int(cls[size_t(y)*W+x])==cl ) {
              const short* pr = prd + PI(y,x);
              const short* pe = errB + OI(y,x);
              for( uint k=0; k<nc; k++ ) { pb[k] = pr[k]; eb[k] = pe[k]; }
            } else for( uint k=0; k<nc; k++ ) {
              int v = Clip( Predict( k, p, coef[cl][k] ) );
              pb[k] = short(v);
              eb[k] = short( Econv( int(p[k]), v ) );
            }
          }
        }
      }
    }
  }
  // The buffer belongs to the enclosing MAX_BSIZE block, so its index is
  // taken modulo bufsize -- a sub-block of the quadtree is not at its
  // origin.
  void PutBlock( int cl, uint tly, uint tlx, uint bry, uint brx, uint bufsize ) {
    size_t bp = (size_t(cl)*bufsize*bufsize + size_t(tlx%bufsize))*nc;
    for( uint y=tly; y<bry; y++ ) {
      const short* pb = prdbuf + bp + size_t(y%bufsize)*bufsize*nc;
      const short* eb = errbuf + bp + size_t(y%bufsize)*bufsize*nc;
      short* pr = prd + PI(y,tlx);
      short* pe = errB + OI(y,tlx);
      char*  pc = cls + size_t(y)*W + tlx;
      for( uint x=tlx; x<brx; x++, pb+=nc, eb+=nc, pr+=nc, pe+=nc ) {
        *pc++ = char(cl);
        for( uint k=0; k<nc; k++ ) { pr[k] = pb[k]; pe[k] = eb[k]; }
      }
    }
  }
  // Returns the winning class and, in `outc`, the cost it won with --
  // which VbsClass would otherwise recompute over the same block.
  int FindClass( uint tly, uint tlx, uint bry, uint brx, uint bufsize,
                 cost_t& outc ) {
    // The block's current class is almost always competitive, so costing
    // it first gives every other candidate a tight bound to fail against.
    int cur = int(cls[size_t(tly)*W+tlx]);
    PutBlock( cur, tly, tlx, bry, brx, bufsize );
    cost_t mc = class_cost[mtfbuf[cur]] + CalcCost( tly, tlx, bry, brx );
    int mcl = cur;
    for( int cl=0; cl<num_class; cl++ ) {
      if( cl==cur ) continue;
      cost_t base = class_cost[mtfbuf[cl]];
      if( base > mc ) continue;                 // side cost alone loses
      PutBlock( cl, tly, tlx, bry, brx, bufsize );
      cost_t c = base + CalcCostB( tly, tlx, bry, brx, mc-base );
      // strictly-better, or an exact tie with a lower index: the same
      // winner the plain first-minimum scan would have picked
      if( c<mc || (c==mc && cl<mcl) ) { mc = c; mcl = cl; }
    }
    PutBlock( mcl, tly, tlx, bry, brx, bufsize );
    outc = mc;
    return mcl;
  }
  // `width` is the right edge of the enclosing block, not of the image:
  // the bottom-right child of a split must not look at a cell that lies in
  // the next parent, which the decoder has not reached yet.
  void MtfClass( uint y, uint x, uint bsize, uint width ) {
    int ref[3];
    if( y==0 ) {
      if( x==0 ) ref[0]=ref[1]=ref[2]=0;
      else ref[0]=ref[1]=ref[2]=int(cls[size_t(y)*W+x-1]);
    } else {
      ref[0] = int(cls[size_t(y-1)*W+x]);
      ref[1] = (x==0) ? ref[0] : int(cls[size_t(y)*W+x-1]);
      ref[2] = (x+bsize>=width) ? int(cls[size_t(y-1)*W+x])
                                : int(cls[size_t(y-1)*W+x+bsize]);
      if( ref[1]==ref[2] ) { ref[2]=ref[0]; ref[0]=ref[1]; }
    }
    for( int k=2; k>=0; k-- ) {
      int j = mtfbuf[ref[k]];
      if( j==0 ) continue;
      for( int i=0; i<num_class; i++ ) if( mtfbuf[i]<j ) mtfbuf[i]++;
      mtfbuf[ref[k]] = 0;
    }
  }
  int QtCtx( int level, uint tly, uint tlx, uint blksize, uint width ) {
    int ctx = 0;
    const char* qm = qtmap[level-1];
    int qw = qtw[level-1];
    int y = int((tly/MIN_BSIZE)>>level), x = int((tlx/MIN_BSIZE)>>level);
    if( y>0 ) {
      if( qm[(y-1)*qw+x]==1 ) ctx++;
      if( tlx+blksize<width && qm[(y-1)*qw+x+1]==1 ) ctx++;
    }
    if( x>0 && qm[y*qw+x-1]==1 ) ctx++;
    return ((level-1)*4 + ctx) << 1;
  }
  cost_t VbsClass( uint tly, uint tlx, uint blksize, uint width, int level,
                   uint bufsize ) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx || tly>=bry ) return 0;
    int mtf_save[MRP_MAXCLASS];
    for( int k=0; k<num_class; k++ ) mtf_save[k] = mtfbuf[k];
    MtfClass( tly, tlx, blksize, width );
    cost_t won;
    int cl = FindClass( tly, tlx, bry, brx, bufsize, won );
    cost_t qtcost = class_cost[mtfbuf[cl]];
    if( level>0 ) {
      int ctx = QtCtx( level, tly, tlx, blksize, width );
      cost_t cost1 = won + qtflag_cost[ctx];    // FindClass already has it
      uint half = blksize>>1;
      for( int k=0; k<num_class; k++ ) mtfbuf[k] = mtf_save[k];
      qtcost = qtflag_cost[ctx+1];
      qtcost += VbsClass( tly,      tlx,      half, width, level-1, bufsize );
      qtcost += VbsClass( tly,      tlx+half, half, width, level-1, bufsize );
      qtcost += VbsClass( tly+half, tlx,      half, width, level-1, bufsize );
      qtcost += VbsClass( tly+half, tlx+half, half, brx,   level-1, bufsize );
      cost_t cost2 = CalcCost( tly, tlx, bry, brx ) + qtcost;
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( cost1 < cost2 ) {
        for( int k=0; k<num_class; k++ ) mtfbuf[k] = mtf_save[k];
        MtfClass( tly, tlx, blksize, width );
        qtcost = class_cost[mtfbuf[cl]] + qtflag_cost[ctx];
        PutBlock( cl, tly, tlx, bry, brx, bufsize );
        int ty=qy, tx=qx, by2=qy+1, bx2=qx+1;
        for( int l=level; l>0; l-- ) {
          char* qm = qtmap[l-1]; int qw = qtw[l-1], qh = qth[l-1];
          for( int y=ty; y<by2; y++ ) for( int x=tx; x<bx2; x++ )
            if( y<qh && x<qw ) qm[y*qw+x] = 0;
          ty<<=1; tx<<=1; by2<<=1; bx2<<=1;
        }
      } else {
        qtmap[level-1][qy*qtw[level-1]+qx] = 1;
      }
    }
    return qtcost;
  }
  cost_t OptimizeClass() {
    int  level   = (opt_loop>1) ? QT_DEPTH : 0;
    uint blksize = (opt_loop>1) ? MAX_BSIZE : BASE_BSIZE;
    for( int i=0; i<num_class; i++ ) mtfbuf[i] = i;
    // this is the long pole -- every pixel of every block predicted under
    // every class -- so it reports as it goes rather than at the end
    uint nrow = (H+blksize-1)/blksize, done = 0, mark = 0;
    for( uint y=0; y<H; y+=blksize ) {
      for( uint x=0; x<W; x+=blksize ) {
        SetPrdBuf( y, x, blksize );
        VbsClass( y, x, blksize, W, level, blksize );
      }
      done++;
      if( g_prog && nrow>=8 && done*8/nrow > mark ) { mark = done*8/nrow; PROG("."); }
    }
    return CalcCost( 0, 0, H, W );
  }

  // ---------------------------------------------------------------
  // Coefficient search: move weight between two taps of one predictor,
  // keeping their sum, and take the best of the 11x3 shifts.  Trading
  // between taps rather than moving one at a time is what keeps the DC
  // gain of the predictor intact while it searches.
  void OptimizeCoef( int cl, uint k, int p1, int p2 ) {
    static const int SR = 11, SSR = 3;
    cost_t cbuf[SR*SSR];
    int* cf = coef[cl][k];
    cost_t side[SR*SSR];
    for( int i=0, n=0; i<SR; i++ ) {
      int y = cf[p1] + i - (SR>>1); if( y<0 ) y = -y; if( y>MAX_COEF ) y = MAX_COEF;
      for( int j=0; j<SSR; j++ ) {
        int x = cf[p2] - (i-(SR>>1)) - (j-(SSR>>1));
        if( x<0 ) x = -x; if( x>MAX_COEF ) x = MAX_COEF;
        side[n] = coef_cost[coef_m[k][p1]][y] + coef_cost[coef_m[k][p2]][x];
        cbuf[n++] = 0.0;
      }
    }
    // stride over the class's pixels; the sampled sum is scaled back up
    // before the side cost, which is in absolute bits, is added to it
    size_t cnt = cbeg[cl+1]-cbeg[cl];
    size_t st  = 1;
    if( COEF_MAX && cnt > COEF_MAX ) st = cnt/COEF_MAX;
    int o1 = toff[k][p1], o2 = toff[k][p2];
#if defined(__AVX2__)
    // Five vectors cover the 11x3 candidate grid (the last seven lanes are
    // parked on candidate 0 so their gathers stay in range and their sums
    // are dropped).  Each pass is two gathers instead of eight dependent
    // loads into eight different sub-position tables.
    __m256 acc[5]; for( int g=0; g<5; g++ ) acc[g] = _mm256_setzero_ps();
    ALIGN(32) float flush[40];
    for( int g=0; g<5; g++ ) for( int t=0; t<8; t++ ) flush[g*8+t] = 0.0f;
    uint since = 0;
    const __m256i vMAX = _mm256_set1_epi32(MAXPRD);
    const __m256i vZ   = _mm256_setzero_si256();
    const __m256i vH   = _mm256_set1_epi32(1<<(COEF_PREC-PM_ACC-1));
    const __m256i vCS  = _mm256_set1_epi32(CSTRIDE);
    const __m256i vSUB = _mm256_set1_epi32(NUM_SUBPM-1);
    const __m256i vPS  = _mm256_set1_epi32(PMSIZE);
    for( size_t ci=cbeg[cl], ce=cbeg[cl+1]; ci<ce; ci+=st ) {
      {
        const byte* p = org + cpo[ci];
        const short* pr = prd + cpq[ci];
        const char* pg = grp + cpq[ci];
        int d1 = int(p[o1]), d2 = int(p[o2]);
        int pf0 = int(pr[k]) - (d1-d2)*(SR>>1) + d2*(SSR>>1);
        const float* cb0 = pmlc[k][int(pg[k])];
        int v0 = int(p[k]);
        // the next sampled pixel lands in one of sixteen tables and near
        // the same base; ask for it, and for its pixel data, now
        if( ci+st<ce ) {
          const char* pgn = grp + cpq[ci+st];
          _mm_prefetch( (const char*)(org + cpo[ci+st]), _MM_HINT_T0 );
          _mm_prefetch( (const char*)(pmlc[k][int(pgn[k])]
                                      + ((MAXPRD-Clip(pf0))>>COEF_PREC) + v0),
                        _MM_HINT_T0 );
        }
        __m256i vpf = _mm256_set1_epi32(pf0);
        __m256i vdd = _mm256_set1_epi32(d1-d2);
        __m256i vd2 = _mm256_set1_epi32(d2);
        __m256i vv0 = _mm256_set1_epi32(v0);
        for( int g=0; g<5; g++ ) {
          __m256i A = _mm256_load_si256( (const __m256i*)(cA+g*8) );
          __m256i B = _mm256_load_si256( (const __m256i*)(cB+g*8) );
          __m256i pf = _mm256_add_epi32( vpf,
                         _mm256_sub_epi32( _mm256_mullo_epi32(A,vdd),
                                           _mm256_mullo_epi32(B,vd2) ) );
          pf = _mm256_max_epi32( _mm256_min_epi32(pf,vMAX), vZ );
          __m256i q = _mm256_srai_epi32(
                        _mm256_add_epi32( _mm256_sub_epi32(vMAX,pf), vH ),
                        COEF_PREC-PM_ACC );
          __m256i base = _mm256_srai_epi32( q, PM_ACC );
          __m256i off  = _mm256_mullo_epi32( _mm256_and_si256(q,vSUB), vCS );
          __m256i i1 = _mm256_add_epi32( off, _mm256_add_epi32(base,vv0) );
          __m256i i2 = _mm256_add_epi32( off, _mm256_add_epi32(base,vPS) );
#if MRP_GATHER
          __m256 c1 = _mm256_i32gather_ps( cb0, i1, 4 );
          __m256 c2 = _mm256_i32gather_ps( cb0, i2, 4 );
#else
          // AVX2's gather is a microcoded loop; eight ordinary loads off
          // SIMD-computed indices are usually the faster way to say it
          ALIGN(32) int x1i[8], x2i[8]; ALIGN(32) float f1[8], f2[8];
          _mm256_store_si256( (__m256i*)x1i, i1 );
          _mm256_store_si256( (__m256i*)x2i, i2 );
          for( int t=0; t<8; t++ ) { f1[t] = cb0[x1i[t]]; f2[t] = cb0[x2i[t]]; }
          __m256 c1 = _mm256_load_ps(f1);
          __m256 c2 = _mm256_load_ps(f2);
#endif
          acc[g] = _mm256_add_ps( acc[g], _mm256_add_ps(c1,c2) );
        }
        // float carries 24 bits; drain into the doubles before it matters
        if( ++since >= 4096 ) {
          for( int g=0; g<5; g++ ) {
            _mm256_store_ps( flush+g*8, acc[g] );
            acc[g] = _mm256_setzero_ps();
            for( int t=0; t<8; t++ ) if( g*8+t<33 ) cbuf[g*8+t] += flush[g*8+t];
          }
          since = 0;
        }
      }
    }
    for( int g=0; g<5; g++ ) {
      _mm256_store_ps( flush+g*8, acc[g] );
      for( int t=0; t<8; t++ ) if( g*8+t<33 ) cbuf[g*8+t] += flush[g*8+t];
    }
#else
    for( size_t ci=cbeg[cl], ce=cbeg[cl+1]; ci<ce; ci+=st ) {
      {
        const byte* p = org + cpo[ci];
        const short* pr = prd + cpq[ci];
        const char* pg = grp + cpq[ci];
        int d1 = int(p[o1]), d2 = int(p[o2]);
        int pf = int(pr[k]) - (d1-d2)*(SR>>1) + d2*(SSR>>1);
        const PMod* pmg = pml[k][int(pg[k])];
        int v0 = int(p[k]);
        cost_t* cb = cbuf;
        for( int i=0; i<SR; i++ ) {
          for( int j=0; j<SSR; j++ ) {
            int q = Qprd( Clip(pf) );
            const PMod* pm = pmg + (q&(NUM_SUBPM-1));
            int base = q>>PM_ACC;
            (*cb++) += pm->cost[base+v0] + pm->subcost[base];
            pf -= d2;
          }
          pf += d1 + d2*(SSR-1);
        }
      }
    }
#endif
    for( int i=0; i<SR*SSR; i++ ) cbuf[i] = cbuf[i]*cost_t(st) + side[i];
    int b = (SR*SSR)>>1;
    for( int i=0; i<SR*SSR; i++ ) if( cbuf[i] < cbuf[b] ) b = i;
    int i = (b/SSR) - (SR>>1), j = (b%SSR) - (SSR>>1);
    int y = cf[p1] + i, x = cf[p2] - i - j;
    if( y<-MAX_COEF ) y = -MAX_COEF; else if( y>MAX_COEF ) y = MAX_COEF;
    if( x<-MAX_COEF ) x = -MAX_COEF; else if( x>MAX_COEF ) x = MAX_COEF;
    i = y - cf[p1]; j = x - cf[p2];
    if( i==0 && j==0 ) return;
    cf[p1] = y; cf[p2] = x;
    for( size_t ci=cbeg[cl], ce=cbeg[cl+1]; ci<ce; ci++ ) {
      const byte* p = org + cpo[ci];
      short* pr = prd + cpq[ci];
      short* pe = errB + cpo[ci];
      int v = Clip( int(pr[k]) + int(p[o1])*i + int(p[o2])*j );
      pr[k] = short(v);
      pe[k] = short( Econv( int(p[k]), v ) );
    }
  }
  // MRP draws the two taps at random.  Sweeping them instead -- every tap
  // paired with the one a fixed distance away, the distance rotating with
  // the iteration -- covers the same pairs, converges no worse, and makes
  // an encode reproducible: with the random draw the same file compresses
  // to sizes ~1% apart depending only on the seed, which is enough noise
  // to hide the parameter differences one is trying to measure.
  uint optpass;
  void BuildClassList() {
    size_t cnt[MRP_MAXCLASS+1];
    for( int i=0; i<=num_class; i++ ) cnt[i] = 0;
    for( size_t i=0; i<size_t(W)*H; i++ ) cnt[int(cls[i])+1]++;
    cbeg[0] = 0;
    for( int i=0; i<num_class; i++ ) cbeg[i+1] = cbeg[i] + cnt[i+1];
    for( int i=0; i<=num_class; i++ ) cnt[i] = cbeg[i];
    for( uint y=0; y<H; y++ ) for( uint x=0; x<W; x++ ) {
      int c = int(cls[size_t(y)*W+x]);
      size_t j = cnt[c]++;
      cpo[j] = uint(OI(y,x));
      cpq[j] = uint(PI(y,x));
    }
  }
  cost_t OptimizePredictor() {
    BuildClassList();
    // Every (class, component) is an independent chain: OptimizeCoef reads
    // immutable state and touches prd/errB only at slot k of that class's
    // own pixels.  Classes partition the pixels, components partition the
    // slots, so all C*K chains are disjoint -- and the result of each is
    // what it was single-threaded, so the stream does not move with -j.
    int nck = num_class*int(nc);
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,1)
#endif
    for( int i=0; i<nck; i++ ) {
      int cl = i/int(nc); uint k = uint(i%int(nc));
      int n = nt[k];
      if( n<2 ) continue;
      int d = 1 + int( optpass % uint(n-1) );
      for( int t=0; t<n; t++ ) OptimizeCoef( cl, k, t, (t+d)%n );
    }
    optpass++;
    PredictRegion( 0, 0, H, W );
    return CalcCost( 0, 0, H, W );
  }
};

// -------------------------------------------------------------
// Side information: cost tables first, because the optimiser above spends
// them, then the coders that make them true.
// -------------------------------------------------------------
static const int PMCLASS_LEVEL = 32;
static const int PMCLASS_MAX   = 16;

struct MRPCIO : MRPC {
  SPMod sp;

  void SetCoefCost() {
    for( int m=0; m<16; m++ ) {
      sp.Set( MAX_COEF+1, m );
      for( int c=0; c<=MAX_COEF; c++ )
        coef_cost[m][c] = sp.Cost(c) + (c!=0 ? 1.0 : 0.0);
    }
    for( uint k=0; k<nc; k++ ) for( int i=0; i<NTMAX; i++ ) coef_m[k][i] = 8;
  }
  void DefaultSideCosts() {
    double lc = log(double(num_class))/log(2.0);
    for( int i=0; i<MRP_MAXCLASS; i++ ) class_cost[i] = lc;
    for( int i=0; i<(QT_DEPTH<<3); i++ ) qtflag_cost[i] = 1.0;
    sp.Set( MAX_UPARA+2, 8 );
    for( int i=0; i<MAX_UPARA+2; i++ ) th_cost[i] = sp.Cost(i);
  }

  // --- predictors ---------------------------------------------------
  cost_t CodePredictor( int measure ) {
    cost_t total = 0;
    for( uint k=0; k<nc; k++ ) for( int i=0; i<nt[k]; i++ ) {
      cost_t mc = 1e30; int mm = 0;
      for( int m=0; m<16; m++ ) {
        cost_t c = 0;
        for( int cl=0; cl<num_class; cl++ ) {
          int v = coef[cl][k][i]; if( v<0 ) v = -v;
          c += coef_cost[m][v];
        }
        if( c<mc ) { mc = c; mm = m; }
      }
      coef_m[k][i] = mm; total += mc;
      if( !measure ) {
        SPMod q; q.Set( 16, -1 );
        EncSP( q, mm );
        sp.Set( MAX_COEF+1, mm );
        for( int cl=0; cl<num_class; cl++ ) {
          int v = coef[cl][k][i], s = (v<0);
          if( v<0 ) v = -v;
          EncSP( sp, v );
          if( v>0 ) rc.Encode( uint(s), 1, 2 );
        }
      }
    }
    return total;
  }
  void DecodePredictor() {
    for( uint k=0; k<nc; k++ ) {
      for( int i=0; i<nt[k]; i++ ) {
        SPMod q; q.Set( 16, -1 );
        int mm = DecSP( q );
        sp.Set( MAX_COEF+1, mm );
        for( int cl=0; cl<num_class; cl++ ) {
          int v = DecSP( sp );
          if( v>0 ) {
            uint s = rc.GetFreq(2);
            rc.Decode( s, 1, 2 );
            if( s ) v = -v;
          }
          coef[cl][k][i] = v;
        }
      }
      for( int i=nt[k]; i<NTMAX; i++ )
        for( int cl=0; cl<num_class; cl++ ) coef[cl][k][i] = 0;
    }
  }

  // --- group thresholds and the shape of each group -----------------
  cost_t CodeThreshold( int measure ) {
    cost_t mc = 1e30; int mm = 0;
    for( int m=0; m<16; m++ ) {
      sp.Set( MAX_UPARA+2, m );
      cost_t c = 0;
      for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
        int prev = 0;
        for( int gr=0; gr<MRP_GROUP-1; gr++ ) {
          if( prev>MAX_UPARA ) break;
          c += sp.Cost( th[cl][k][gr]-prev );
          prev = th[cl][k][gr];
        }
      }
      if( c<mc ) { mc = c; mm = m; }
    }
    sp.Set( MAX_UPARA+2, mm );
    for( int i=0; i<MAX_UPARA+2; i++ ) th_cost[i] = sp.Cost(i);
    cost_t total = mc;
    if( num_pm>1 ) total += cost_t(nc)*MRP_GROUP*(log(double(num_pm))/log(2.0));
    if( !measure ) {
      SPMod q; q.Set( 16, -1 );
      EncSP( q, mm );
      for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
        int prev = 0;
        for( int gr=0; gr<MRP_GROUP-1; gr++ ) {
          if( prev>MAX_UPARA ) break;
          EncSP( sp, th[cl][k][gr]-prev );
          prev = th[cl][k][gr];
        }
      }
      SPMod u; u.Set( num_pm, -1 );
      for( uint k=0; k<nc; k++ ) for( int gr=0; gr<MRP_GROUP; gr++ )
        EncSP( u, pm_idx[k][gr] );
    }
    return total;
  }
  void DecodeThreshold() {
    SPMod q; q.Set( 16, -1 );
    int mm = DecSP( q );
    sp.Set( MAX_UPARA+2, mm );
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
      int prev = 0;
      for( int gr=0; gr<MRP_GROUP-1; gr++ ) {
        if( prev>MAX_UPARA ) { th[cl][k][gr] = prev; continue; }
        prev += DecSP( sp );
        th[cl][k][gr] = prev;
      }
      th[cl][k][MRP_GROUP-1] = MAX_UPARA+1;
      int u2 = 0;
      for( int gr=0; gr<MRP_GROUP; gr++ )
        for( ; u2<th[cl][k][gr] && u2<=MAX_UPARA; u2++ ) uq[cl][k][u2] = char(gr);
      for( ; u2<=MAX_UPARA; u2++ ) uq[cl][k][u2] = char(MRP_GROUP-1);
    }
    SPMod u; u.Set( num_pm, -1 );
    for( uint k=0; k<nc; k++ ) for( int gr=0; gr<MRP_GROUP; gr++ )
      pm_idx[k][gr] = DecSP( u );
  }

  // --- class map ----------------------------------------------------
  int*  qindex; uint* qhist; int qn;

  void QtIndex( uint tly, uint tlx, uint blksize, uint width, int level ) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx || tly>=bry ) return;
    if( level>0 ) {
      int ctx = QtCtx( level, tly, tlx, blksize, width );
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( qtmap[level-1][qy*qtw[level-1]+qx]==1 ) {
        qindex[qn++] = -(ctx+2); qtctx[ctx+1]++;
        uint half = blksize>>1;
        QtIndex( tly,      tlx,      half, width, level-1 );
        QtIndex( tly,      tlx+half, half, width, level-1 );
        QtIndex( tly+half, tlx,      half, width, level-1 );
        QtIndex( tly+half, tlx+half, half, brx,   level-1 );
        return;
      }
      qindex[qn++] = -(ctx+1); qtctx[ctx]++;
    }
    MtfClass( tly, tlx, blksize, width );
    int i = mtfbuf[int(cls[size_t(tly)*W+tlx])];
    qindex[qn++] = i; qhist[i]++;
  }

  cost_t CodeClass( int measure ) {
    int  level   = (opt_loop>1) ? QT_DEPTH : 0;
    uint blksize = (opt_loop>1) ? MAX_BSIZE : BASE_BSIZE;
    for( int k=0; k<(QT_DEPTH<<3); k++ ) qtctx[k] = 0;
    for( int i=0; i<num_class; i++ ) { qhist[i] = 0; mtfbuf[i] = i; }
    qn = 0;
    for( uint y=0; y<H; y+=blksize ) for( uint x=0; x<W; x+=blksize )
      QtIndex( y, x, blksize, W, level );

    // one of seven probabilities per quadtree context, and a quantised
    // log-probability per class label: both transmitted, so the cost model
    // the optimiser used and the coder that follows agree exactly
    if( level>0 ) for( int ctx=0; ctx<(QT_DEPTH<<2); ctx++ ) {
      cost_t best = 1e30; int bi = 0;
      for( int i=0; i<7; i++ ) {
        double p = QTREE_PROB[i];
        cost_t c = -log(p)*cost_t(qtctx[(ctx<<1)+1]) - log(1.0-p)*cost_t(qtctx[ctx<<1]);
        if( c<best ) { best = c; bi = i; }
      }
      qtree_code[ctx] = bi;
      double p = QTREE_PROB[bi];
      qtflag_cost[(ctx<<1)+1] = -log(p)/log(2.0);
      qtflag_cost[ctx<<1]     = -log(1.0-p)/log(2.0);
    }
    int mtf_code[MRP_MAXCLASS];
    double tot = 0.0;
    for( int i=0; i<num_class; i++ ) tot += double(qhist[i]);
    if( tot<=0.0 ) tot = 1.0;
    for( int i=0; i<num_class; i++ ) {
      double p = double(qhist[i])/tot;
      int m;
      if( p>0.0 ) {
        m = int(-log(p)/log(2.0)*(PMCLASS_LEVEL/PMCLASS_MAX));
        if( m>=PMCLASS_LEVEL ) m = PMCLASS_LEVEL-1;
      } else m = PMCLASS_LEVEL-1;
      mtf_code[i] = m;
      p = exp(-log(2.0)*((double)m+0.5)*PMCLASS_MAX/PMCLASS_LEVEL);
      class_cost[i] = -log(p)/log(2.0);
      qhist[i] = uint(p*(1<<10)); if( qhist[i]==0 ) qhist[i] = 1;
    }
    cost_t cost = 0;
    for( int j=0; j<qn; j++ ) {
      int i = qindex[j];
      cost += (i<0) ? qtflag_cost[-(i+1)] : class_cost[i];
    }
    if( measure ) return cost;

    SPMod q;
    if( level>0 ) {
      q.Set( 7, -1 );
      for( int ctx=0; ctx<(QT_DEPTH<<2); ctx++ ) EncSP( q, qtree_code[ctx] );
    }
    q.Set( PMCLASS_LEVEL, -1 );
    for( int i=0; i<num_class; i++ ) EncSP( q, mtf_code[i] );
    ClassCode( 0 );
    return cost;
  }

  // Walk qindex and code it, or (decode==1) rebuild the class map.
  SPMod cpm; SPMod qtp[QT_DEPTH<<2];
  void BuildClassPM() {
    cpm.size = num_class;
    cpm.cumfreq[0] = 0;
    for( int i=0; i<num_class; i++ ) {
      cpm.freq[i] = qhist[i];
      cpm.cumfreq[i+1] = cpm.cumfreq[i] + cpm.freq[i];
    }
    for( int ctx=0; ctx<(QT_DEPTH<<2); ctx++ ) {
      double p = QTREE_PROB[qtree_code[ctx]];
      qtp[ctx].size = 2;
      qtp[ctx].freq[0] = uint((1.0-p)*(1<<10)) + 1;
      qtp[ctx].freq[1] = uint(p*(1<<10)) + 1;
      qtp[ctx].cumfreq[0] = 0;
      qtp[ctx].cumfreq[1] = qtp[ctx].freq[0];
      qtp[ctx].cumfreq[2] = qtp[ctx].freq[0]+qtp[ctx].freq[1];
    }
  }
  void ClassCode( int ) {
    BuildClassPM();
    for( int j=0; j<qn; j++ ) {
      int i = qindex[j];
      if( i<0 ) { int ctx = -(i+1); EncSP( qtp[ctx>>1], ctx&1 ); }
      else EncSP( cpm, i );
    }
  }
  void DecodeClassMap() {
    int  level   = QT_DEPTH;
    uint blksize = MAX_BSIZE;
    SPMod q;
    if( level>0 ) {
      q.Set( 7, -1 );
      for( int ctx=0; ctx<(QT_DEPTH<<2); ctx++ ) qtree_code[ctx] = DecSP( q );
    }
    q.Set( PMCLASS_LEVEL, -1 );
    for( int i=0; i<num_class; i++ ) {
      int m = DecSP( q );
      double p = exp(-log(2.0)*((double)m+0.5)*PMCLASS_MAX/PMCLASS_LEVEL);
      qhist[i] = uint(p*(1<<10)); if( qhist[i]==0 ) qhist[i] = 1;
    }
    BuildClassPM();
    for( int i=0; i<num_class; i++ ) mtfbuf[i] = i;
    for( int l=0; l<QT_DEPTH; l++ )
      memset( qtmap[l], 0, size_t(qtw[l])*qth[l] );
    for( uint y=0; y<H; y+=blksize ) for( uint x=0; x<W; x+=blksize )
      QtDecode( y, x, blksize, W, level );
  }
  void QtDecode( uint tly, uint tlx, uint blksize, uint width, int level ) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx || tly>=bry ) return;
    if( level>0 ) {
      int ctx = QtCtx( level, tly, tlx, blksize, width );
      int bit = DecSP( qtp[ctx>>1] );
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( bit ) {
        qtmap[level-1][qy*qtw[level-1]+qx] = 1;
        uint half = blksize>>1;
        QtDecode( tly,      tlx,      half, width, level-1 );
        QtDecode( tly,      tlx+half, half, width, level-1 );
        QtDecode( tly+half, tlx,      half, width, level-1 );
        QtDecode( tly+half, tlx+half, half, brx,   level-1 );
        return;
      }
      qtmap[level-1][qy*qtw[level-1]+qx] = 0;
    }
    MtfClass( tly, tlx, blksize, width );
    int i = DecSP( cpm );
    int cl = 0;
    for( int j=0; j<num_class; j++ ) if( mtfbuf[j]==i ) { cl = j; break; }
    for( uint y=tly; y<bry; y++ )
      for( uint x=tlx; x<brx; x++ ) cls[size_t(y)*W+x] = char(cl);
  }

  // --- the image ----------------------------------------------------
  // Encoder and decoder run the identical loop: prediction, activity and
  // group all come out of what is already coded, so the only difference is
  // which way the symbol crosses the coder.
  void ResetBorders() {
    for( uint y=0; y<PADT; y++ ) {
      byte* p = org + size_t(y)*BS*nc;
      short* e = errB + size_t(y)*BS*nc;
      for( size_t i=0; i<size_t(BS)*nc; i++ ) { p[i] = 128; e[i] = 0; }
    }
    for( uint y=0; y<H; y++ ) {
      byte* p = org + OI(y,0); short* e = errB + OI(y,0);
      for( int j=1; j<=PADL; j++ ) for( uint k=0; k<nc; k++ ) {
        p[-j*int(nc)+int(k)] = 128; e[-j*int(nc)+int(k)] = 0;
      }
      p = org + OI(y,W-1); e = errB + OI(y,W-1);
      for( int j=1; j<=PADR; j++ ) for( uint k=0; k<nc; k++ ) {
        p[j*int(nc)+int(k)] = 128; e[j*int(nc)+int(k)] = 0;
      }
    }
  }
  // ---------------------------------------------------------------
  // 7.2: the transmitted pmf correction.
  //
  // MeasurePmFix walks the settled model once and, per (component,
  // group), compares how many symbols landed in each residual bucket
  // with how many the model expected to.  The ratio, quantised to
  // eighths of an octave, is the correction; BuildPmFix bakes it into a
  // private copy of that group's frequency table.
  //
  // The rebuild is integer throughout and reads only the transmitted
  // levels and the table the decoder builds anyway, so both sides land
  // on the same frequencies.
  PMod* pmfix;                     // [k][gr][sub], nc*MRP_GROUP*NUM_SUBPM
  int   pmfl[MAXC][MRP_GROUP][PMFIX_BK];
  int   pmfix_on;

  void AllocPmFix() {
    if( pmfix ) return;
    int n = MAXC*MRP_GROUP*NUM_SUBPM;
    pmfix = new PMod[n];
    uint*  fb  = new uint[size_t(n)*(PMSIZE*2+1)];
    float* blk = new float[size_t(n)*CSTRIDE];
    for( int i=0; i<n; i++ ) {
      pmfix[i].size    = PMSIZE;
      pmfix[i].freq    = fb + size_t(i)*(PMSIZE*2+1);
      pmfix[i].cumfreq = pmfix[i].freq + PMSIZE;
      pmfix[i].cost    = blk + size_t(i)*CSTRIDE;
      pmfix[i].subcost = pmfix[i].cost + PMSIZE;
      pmfix[i].id      = 0;
    }
  }
  void BuildPmFix() {
    AllocPmFix();
    const double a = 1.0/log(2.0);
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ ) {
      const PMod* src = pmodels + (size_t(g)*num_pm + pm_idx[k][g])*NUM_SUBPM;
      PMod* dst = pmfix + (size_t(k)*MRP_GROUP + g)*NUM_SUBPM;
      uint mul[PMFIX_BK];
      for( int b=0; b<PMFIX_BK; b++ ) mul[b] = PMFIX_MUL[pmfl[k][g][b]+PMFIX_LVL];
      for( int s=0; s<NUM_SUBPM; s++ ) {
        uint tot = 0;
        for( int i=0; i<PMSIZE; i++ ) {
          uint e = src[s].freq[i] - MIN_FREQ;
          uint f = MIN_FREQ + uint( (qword(e)*mul[int(pmfix_bk[i])]) >> 16 );
          dst[s].freq[i] = f; tot += f;
        }
        // the coder's total is a window of this table, so the whole table
        // staying under MAX_TOTFREQ is enough -- but scaling up can push
        // it over, and then everything comes back down by one ratio
        if( tot > MAX_TOTFREQ ) {
          uint room = MAX_TOTFREQ - PMSIZE*MIN_FREQ, ex = tot - PMSIZE*MIN_FREQ;
          for( int i=0; i<PMSIZE; i++ ) {
            uint e = dst[s].freq[i] - MIN_FREQ;
            dst[s].freq[i] = MIN_FREQ + uint( qword(e)*room/ex );
          }
        }
        dst[s].cumfreq[0] = 0;
        for( int i=0; i<PMSIZE; i++ )
          dst[s].cumfreq[i+1] = dst[s].cumfreq[i] + dst[s].freq[i];
        for( int i=0; i<PMSIZE; i++ )
          dst[s].cost[i] = float(-a*log(double(dst[s].freq[i])));
        for( int i=0; i<=MAXVAL; i++ )
          dst[s].subcost[i] =
            float(a*log(double(dst[s].cumfreq[i+MAXVAL+1]-dst[s].cumfreq[i])));
      }
      pml[k][g]  = dst;
      pmlc[k][g] = dst->cost;
    }
    pmfix_on = 1;
  }
  void MeasurePmFix() {
    static double obs[MAXC][MRP_GROUP][PMFIX_BK];
    static double exp_[MAXC][MRP_GROUP][PMFIX_BK];
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      for( int b=0; b<PMFIX_BK; b++ ) obs[k][g][b] = exp_[k][g][b] = 0.0;
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for( int kk=0; kk<int(nc); kk++ ) {
      const uint k = uint(kk);
      for( uint y=0; y<H; y++ ) {
        const byte* p = org + OI(y,0);
        const short* pr = prd + PI(y,0);
        const char* pg = grp + PI(y,0);
        for( uint x=0; x<W; x++, p+=nc, pr+=nc, pg+=nc ) {
          int gr = int(pg[k]);
          int q = Qprd( int(pr[k]) ), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
          int v = base + int(p[k]);
          obs[k][gr][int(pmfix_bk[v])] += 1.0;
          const PMod* pm = pml[k][gr] + sub;
          int hiw = base+MAXVAL+1;
          double tot = double(pm->cumfreq[hiw] - pm->cumfreq[base]);
          for( int b=0; b<PMFIX_BK; b++ ) {
            int lo = pmfix_lo[b] > base ? pmfix_lo[b] : base;
            int hi = pmfix_hi[b] < hiw  ? pmfix_hi[b] : hiw;
            if( hi>lo )
              exp_[k][gr][b] += double(pm->cumfreq[hi]-pm->cumfreq[lo]) / tot;
          }
        }
      }
    }
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      for( int b=0; b<PMFIX_BK; b++ ) {
        double o = obs[k][g][b], e = exp_[k][g][b];
        int lv = 0;
        if( e>0.5 && o>0.5 ) {
          double r = log(o/e)/log(2.0)*double(PMFIX_LVL);
          lv = int( r<0 ? r-0.5 : r+0.5 );
        } else if( e>0.5 ) lv = -PMFIX_LVL;      // nothing landed here
        // the ratio is measured against whatever pml currently is, so
        // on a second round it is the residual mismatch and composes
        lv += pmfl[k][g][b];
        if( lv >  PMFIX_LVL ) lv =  PMFIX_LVL;
        if( lv < -PMFIX_LVL ) lv = -PMFIX_LVL;
        pmfl[k][g][b] = lv;
      }
  }
  // one flag, then the levels zigzagged so that "no correction" is the
  // symbol the geometric model spends least on
  cost_t CodePmFix( int dec, int measure ) {
    SPMod f; f.Set( 2, -1 );
    if( measure ) {
      if( !pmfix_on ) return f.Cost( 0 );
      cost_t c = f.Cost( 1 );
      SPMod u; u.Set( 2*PMFIX_LVL+1, 4 );
      for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
        for( int b=0; b<PMFIX_BK; b++ ) {
          int lv = pmfl[k][g][b];
          c += u.Cost( lv<=0 ? -2*lv : 2*lv-1 );
        }
      return c;
    }
    if( dec ) pmfix_on = int(DecSP(f)); else EncSP( f, uint(pmfix_on) );
    if( !pmfix_on ) return 0;
    SPMod u; u.Set( 2*PMFIX_LVL+1, 4 );
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      for( int b=0; b<PMFIX_BK; b++ ) {
        if( dec ) {
          int z = int(DecSP(u));
          pmfl[k][g][b] = (z&1) ? (z+1)/2 : -(z/2);
        } else {
          int lv = pmfl[k][g][b];
          EncSP( u, uint(lv<=0 ? -2*lv : 2*lv-1) );
        }
      }
    if( dec ) BuildPmFix();
    return 0;
  }

  void CodeImage( int dec ) {
    ResetBorders();
    for( uint y=0; y<H; y++ ) {
      BorderLeft( y );
      byte* p = org + OI(y,0);
      short* pe = errB + OI(y,0);
      const char* pc = cls + size_t(y)*W;
      for( uint x=0; x<W; x++, p+=nc, pe+=nc ) {
        int cl = int(pc[x]);
        for( uint k=0; k<nc; k++ ) {
          int v  = Clip( Predict( k, p, coef[cl][k] ) );
          int u  = Activity( k, pe );
          int gr = int(uq[cl][k][u]);
          int q  = Qprd( v ), base = q>>PM_ACC;
          const PMod* pm = pml[k][gr] + (q&(NUM_SUBPM-1));
          int s;
          if( dec ) { s = DecSym( pm, base, base+MAXVAL+1 ); p[k] = byte(s-base); }
          else      { s = base + int(p[k]); EncSym( pm, base, base+MAXVAL+1, s ); }
          pe[k] = short( Econv( int(p[k]), v ) );
        }
        if( y==0 && int(x)+PADR+1 < int(W)+PADR ) BorderTopCol( int(x)+PADR+1, int(x) );
      }
      BorderRight( y );
    }
  }
};

// -------------------------------------------------------------
// Allocation and the two drivers
// -------------------------------------------------------------
struct Codec : MRPCIO {
  byte* img;                  // the BMP raster, as stored
  uint  pixbytes;

  void Alloc( uint W_, uint H_, uint nc_, uint stride_ ) {
    W = W_; H = H_; nc = nc_; stride = stride_;
    BS = W + PADL + PADR;
    num_class = MRP_CLASS ? MRP_CLASS
              : int(10.4e-5*double(W)*double(H) + 13.8);
    if( num_class > MRP_MAXCLASS ) num_class = MRP_MAXCLASS;
    if( num_class < 2 ) num_class = 2;
    size_t bn = size_t(H+PADT)*BS*nc;
    pmfix = 0; pmfix_on = 0; pmfix_init();
    memset( pmfl, 0, sizeof(pmfl) );
    org  = new byte[bn];   memset( org, 128, bn );
    errB = new short[bn];  memset( errB, 0, bn*sizeof(short) );
    prd   = new short[size_t(W)*H*nc];
    upara = new short[size_t(W)*H*nc];
    grp   = new char[size_t(W)*H*nc];  memset( grp, 0, size_t(W)*H*nc );
    cls   = new char[size_t(W)*H];     memset( cls, 0, size_t(W)*H );
    coef  = new int[MRP_MAXCLASS][MAXC][NTMAX];
    memset( coef, 0, size_t(MRP_MAXCLASS)*MAXC*NTMAX*sizeof(int) );
    uq    = new char[MRP_MAXCLASS][MAXC][MAX_UPARA+1];
    memset( uq, 0, size_t(MRP_MAXCLASS)*MAXC*(MAX_UPARA+1) );
    for( int l=0; l<QT_DEPTH; l++ ) {
      qtw[l] = int(((W+MIN_BSIZE-1)/MIN_BSIZE) >> (l+1)) + 2;
      qth[l] = int(((H+MIN_BSIZE-1)/MIN_BSIZE) >> (l+1)) + 2;
      qtmap[l] = new char[size_t(qtw[l])*qth[l]];
      memset( qtmap[l], 0, size_t(qtw[l])*qth[l] );
    }
    size_t nidx = (size_t(H/MIN_BSIZE+2))*(W/MIN_BSIZE+2)*2 + 256;
    cpo = new uint[size_t(W)*H];
    cpq = new uint[size_t(W)*H];
    qindex = new int[nidx];
    qhist  = new uint[MRP_MAXCLASS];
    prdbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    errbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    for( uint k=0; k<nc; k++ ) { ord[k] = int(k); plane[k] = 0; }
    rnd = MRP_SEED; optpass = 0;
    for( int i=0; i<40; i++ ) { cA[i] = (i<33)? i/3 : 0; cB[i] = (i<33)? i%3 : 0; }
    opt_loop = 1;
    BuildTaps();
  }
  void Free() {
    delete[] org; delete[] errB; delete[] prd; delete[] upara;
    delete[] grp; delete[] cls; delete[] coef; delete[] uq;
    for( int l=0; l<QT_DEPTH; l++ ) delete[] qtmap[l];
    delete[] cpo; delete[] cpq;
    delete[] qindex; delete[] qhist; delete[] prdbuf; delete[] errbuf;
    for( uint k=0; k<nc; k++ ) delete[] plane[k];
  }

  // the stored raster -> the bordered buffer, in coding order
  void LoadOrg() {
    for( uint y=0; y<H; y++ ) {
      const byte* s = img + size_t(y)*stride;
      byte* d = org + OI(y,0);
      for( uint x=0; x<W; x++, s+=nc, d+=nc )
        for( uint k=0; k<nc; k++ ) d[k] = s[ord[k]];
    }
    FillBorders();
    BuildPlanes();
  }
  void StoreOrg() {
    memset( img, 0, pixbytes );
    for( uint y=0; y<H; y++ ) {
      byte* d = img + size_t(y)*stride;
      const byte* s = org + OI(y,0);
      for( uint x=0; x<W; x++, s+=nc, d+=nc )
        for( uint k=0; k<nc; k++ ) d[ord[k]] = s[k];
    }
  }

  void SetPmodels() {
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ ) {
      pml[k][g] = pmodels + (size_t(g)*num_pm + pm_idx[k][g])*NUM_SUBPM;
      pmlc[k][g] = pml[k][g]->cost;
    }
  }

  // --- component order ------------------------------------------
  // Only the taps that read the current pixel make the order matter, and
  // what they are worth is exactly what one global predictor per order
  // measures.  Rows are sampled: this is a choice between n! candidates,
  // not a number that has to be right to the byte.
  cost_t TrialOrder() {
    LoadOrg();
    memset( cls, 0, size_t(W)*H );
    int save = num_class; num_class = 1;
    FitPredictor( 1 );
    cost_t c = 0;
    for( int b=0; b<tnb; b++ ) {
      PredictRegion( tb0[b], 0, tb1[b], W );
      c += CalcCost( tb0[b], 0, tb1[b], W );
    }
    num_class = save;
    return c;
  }
  // Bands for the trial: a fixed pixel budget spread evenly down the
  // image, so a wide picture is sampled in more places rather than in one
  // deeper stripe.
  void SetTrialBands() {
    uint rows = uint(TRIAL_PIX / (W?W:1));
    if( rows < 16 ) rows = 16;
    if( rows >= H ) { tnb = 1; tb0[0] = 0; tb1[0] = H; return; }
    uint nb = TRIAL_BANDS, bh = rows/nb;
    if( bh < 8 ) { bh = 8; nb = rows/bh; if( nb<1 ) nb = 1; }
    tnb = int(nb);
    for( uint b=0; b<nb; b++ ) {
      uint y0 = uint((double(H-bh)*double(b))/double(nb>1?nb-1:1));
      tb0[b] = y0; tb1[b] = y0+bh;
    }
  }
  void ChooseOrder() {
    SetTrialBands();
#if MRP_ORD==1
    if( nc>=2 ) { ord[0] = 1; ord[1] = 0; }
#elif MRP_ORD==2
    int perm[MAXC], best[MAXC];
    for( uint k=0; k<nc; k++ ) perm[k] = int(k), best[k] = int(k);
    cost_t bc = 1e30;
    int idx[MAXC]; for( uint k=0; k<nc; k++ ) idx[k] = 0;
    // straightforward permutation walk; nc is 3 or 4
    int n = int(nc), c_[MAXC] = {0,0,0,0}, i = 0;
    for(;;) {
      for( int k=0; k<n; k++ ) ord[k] = perm[k];
      cost_t cc = TrialOrder();
      PROG( "." );
      if( cc<bc ) { bc = cc; for( int k=0; k<n; k++ ) best[k] = perm[k]; }
      while( i<n ) {                                   // Heap's algorithm
        if( c_[i]<i ) {
          int a = (i&1) ? c_[i] : 0;
          int t = perm[a]; perm[a] = perm[i]; perm[i] = t;
          c_[i]++; i = 0; break;
        } else { c_[i] = 0; i++; }
      }
      if( i>=n ) break;
    }
    (void)idx;
    for( int k=0; k<n; k++ ) ord[k] = best[k];
#endif
    tnb = 0;                                   // back to the whole image
    LoadOrg();
  }

  // --- encoder ---------------------------------------------------
  void CodeParams( int dec ) {
    SPMod u;
    u.Set( MRP_MAXCLASS+1, -1 );
    if( dec ) num_class = DecSP( u ); else EncSP( u, num_class );
    u.Set( int(nc), -1 );
    for( uint k=0; k<nc; k++ ) {
      if( dec ) ord[k] = DecSP( u ); else EncSP( u, ord[k] );
    }
  }

  void Encode() {
    num_pm  = NUM_PMODEL*NUM_SIGV;
    pmodels = alloc_pmodels( num_pm, 0 );
    set_costs( pmodels, MRP_GROUP*num_pm*NUM_SUBPM );
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      pm_idx[k][g] = NUM_PMODEL>>1;
    SetPmodels();
    SetCoefCost();
    DefaultSideCosts();

    PROG( "mrpc: %ux%ux%u  %d classes  taps %d+%d*%d%s\n", W, H, nc,
          num_class, PRD_ORDER, int(nc)-1, XPRD_ORDER, XCUR?"+cur":"" );
    double t0 = tnow();
    PROG( "mrpc: order trial " );
    ChooseOrder();
    PROG( " order" );
    for( uint k=0; k<nc; k++ ) PROG( " %d", ord[k] );
    PROG( "  (%.1fs)\n", tnow()-t0 );
    PROG( "mrpc: -DMRP_CLASS=n and -DMAX_ITER=n are the time dials;"
          " both searches are linear in the class count\n" );
    CodeParams( 0 );          // after ChooseOrder: it is what it picked
    InitClass();

    // saved best of each loop
    char* cls_s = new char[size_t(W)*H];
    int (*coef_s)[MAXC][NTMAX] = new int[MRP_MAXCLASS][MAXC][NTMAX];
    static int th_s[MRP_MAXCLASS][MAXC][MRP_GROUP];
    int pm_s[MAXC][MRP_GROUP];
    // the quadtree travels with the class map it describes: restoring one
    // without the other emits flags for a segmentation that is no longer
    // there, and the decoder fills blocks the encoder never coded
    char* qt_s[QT_DEPTH];
    for( int l=0; l<QT_DEPTH; l++ ) qt_s[l] = new char[size_t(qtw[l])*qth[l]];
    #define SAVE() do { memcpy(cls_s,cls,size_t(W)*H); \
        memcpy(coef_s,coef,size_t(num_class)*MAXC*NTMAX*sizeof(int)); \
        memcpy(th_s,th,sizeof(th)); memcpy(pm_s,pm_idx,sizeof(pm_idx)); \
        for( int l=0; l<QT_DEPTH; l++ ) \
          memcpy(qt_s[l],qtmap[l],size_t(qtw[l])*qth[l]); } while(0)
    #define LOAD() do { memcpy(cls,cls_s,size_t(W)*H); \
        memcpy(coef,coef_s,size_t(num_class)*MAXC*NTMAX*sizeof(int)); \
        memcpy(th,th_s,sizeof(th)); memcpy(pm_idx,pm_s,sizeof(pm_idx)); \
        for( int l=0; l<QT_DEPTH; l++ ) \
          memcpy(qtmap[l],qt_s[l],size_t(qtw[l])*qth[l]); \
        SetPmodels(); RebuildUq(); } while(0)

    cost_t best = 1e30; int last = 0;
    opt_loop = 1;
    for( int it=0; it<MAX_ITER; it++ ) {
      double ta = tnow();
      PROG( "mrpc: [1:%2d] fit", it );
      cost_t c = DesignPredictor( it==0 );
      PROG( " %.0fs group", tnow()-ta ); double tb = tnow();
      c = OptimizeGroup();
      PROG( " %.0fs class", tnow()-tb ); tb = tnow();
      c = OptimizeClass();
      PROG( " %.0fs = %.0f B  [%.0fs]%s\n", tnow()-tb, c/8.0, tnow()-t_start,
            (c<best) ? " *" : "" );
      // an iteration only resets the patience counter if it bought
      // something worth the minute it cost
      if( c < best*(1.0 - 1.0/MIN_GAIN) ) last = it;
      if( c<best ) { best = c; SAVE(); }
      if( it-last >= EXTRA_ITER ) break;
    }
    LOAD();
    PredictRegion( 0, 0, H, W );
    CalcCost( 0, 0, H, W );

    opt_loop = 2;
    best = 1e30; last = 0;
    for( int it=0; it<MAX_ITER; it++ ) {
      cost_t c = 0;
      double tb = tnow();
      PROG( "mrpc: [2:%2d] coef", it );
#if OPT_PRED
      c = OptimizePredictor();
#endif
      PROG( " %.0fs group", tnow()-tb ); tb = tnow();
      cost_t side = CodePredictor( 1 );
      c = OptimizeGroup();
      side += CodeThreshold( 1 );
      PROG( " %.0fs class", tnow()-tb ); tb = tnow();
      c = OptimizeClass();
      side += CodeClass( 1 );
      c += side;
      PROG( " %.0fs = %.0f + %.0f side B  [%.0fs]%s\n", tnow()-tb,
            (c-side)/8.0, side/8.0, tnow()-t_start, (c<best) ? " *" : "" );
      if( c < best*(1.0 - 1.0/MIN_GAIN) ) last = it;
      if( c<best ) { best = c; SAVE(); }
#if OPT_PRED
      if( it-last >= EXTRA_ITER ) break;
#else
      if( it>last ) break;
#endif
    }
    LOAD();
    PredictRegion( 0, 0, H, W );
    CalcCost( 0, 0, H, W );
    CodeClass( 1 );                       // refresh qindex and the costs
    #undef SAVE
    #undef LOAD
    delete[] cls_s; delete[] coef_s;
    for( int l=0; l<QT_DEPTH; l++ ) delete[] qt_s[l];

#if MRP_VERBOSE
    Dump( "enc" );
#endif
#if PMFIX
    // The model is settled; now measure how far the family it was chosen
    // from is from the residuals it produced, and send the difference.
    // The thresholds are re-run against the corrected costs so the group
    // boundaries agree with what will be coded -- and if the whole thing
    // does not pay for its own side information, it comes back out and
    // costs one bit.
    {
      cost_t c0 = CalcCost( 0, 0, H, W );
      int sv = opt_loop; opt_loop = 1;      // thresholds only, not shapes
      cost_t c1 = 0;
      for( int r=0; r<PMFIX_ROUNDS; r++ ) {
        MeasurePmFix();                     // deltas, against the current pml
        BuildPmFix();
        c1 = OptimizeGroup();
      }
      cost_t side = CodePmFix( 0, 1 );
      if( c1 + side >= c0 ) {
        pmfix_on = 0; SetPmodels(); OptimizeGroup();
        PROG( " (pmfix declined)" );
      } else {
        PROG( " (pmfix %.0f B)", (c0-c1-side)/8.0 );
      }
      opt_loop = sv;
      CodeClass( 1 );
    }
#endif
    // --- write it out
    PROG( "mrpc: writing" );
    CodeClass( 0 );
    CodePredictor( 0 );
    CodeThreshold( 0 );
#if PMFIX
    CodePmFix( 0, 0 );
#endif
    CodeImage( 0 );
    PROG( " ... done  [%.0fs]\n", tnow()-t_start );
  }
#if MRP_VERBOSE
  void Dump( const char* tag ) {
    unsigned long hc=0, hp=0, ht=0, hm=0;
    for( size_t i=0; i<size_t(W)*H; i++ ) hc = hc*1000003 + (unsigned char)cls[i];
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
      for( int i=0; i<NTMAX; i++ ) hp = hp*1000003 + (unsigned)(coef[cl][k][i]+1000);
      for( int g=0; g<MRP_GROUP; g++ ) ht = ht*1000003 + (unsigned)th[cl][k][g];
    }
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      hm = hm*1000003 + (unsigned)pm_idx[k][g];
    fprintf( stderr, "%s: nc=%u ncls=%d ord=%d%d%d%d cls=%lx prd=%lx th=%lx pm=%lx\n",
             tag, nc, num_class, ord[0], ord[1], ord[2], nc>3?ord[3]:0,
             hc&0xffffffff, hp&0xffffffff, ht&0xffffffff, hm&0xffffffff );
  }
#endif
  void RebuildUq() {
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
      int u = 0;
      for( int gr=0; gr<MRP_GROUP; gr++ )
        for( ; u<th[cl][k][gr] && u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(gr);
      for( ; u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(MRP_GROUP-1);
    }
  }

  // --- decoder ---------------------------------------------------
  void Decode() {
    num_pm  = NUM_PMODEL*NUM_SIGV;
    pmodels = alloc_pmodels( num_pm, 0 );
    opt_loop = 2;
    CodeParams( 1 );
    DecodeClassMap();
    DecodePredictor();
    DecodeThreshold();
    SetPmodels();
#if PMFIX
    CodePmFix( 1, 0 );        // rebuilds the tables if the flag is set
#endif
#if MRP_VERBOSE
    Dump( "dec" );
#endif
    CodeImage( 1 );
  }
};

// -------------------------------------------------------------
// BMP front end.  Same shape as bmpc's: the header and anything that is
// not a 24/32bpp raster go through a plain order-1 byte model -- adaptive
// binary probabilities with a shift, not a Counter with an optimiser
// attached to it.
// -------------------------------------------------------------
static uint get16( const byte* p ) { return p[0] | (p[1]<<8); }
static uint get32( const byte* p ) {
  return p[0] | (p[1]<<8) | (p[2]<<16) | (uint(p[3])<<24);
}
static const uint HDR0 = 54;   // BITMAPFILEHEADER + BITMAPINFOHEADER

struct BmpInfo { uint ok, off, W, H, nc, stride, pixbytes; };

static BmpInfo ParseBMP( const byte* d, uint flen_ ) {
  BmpInfo b; memset( &b, 0, sizeof(b) );
  if( flen_<HDR0 ) return b;
  if( d[0]!='B' || d[1]!='M' ) return b;
  uint off  = get32(d+10);
  uint isz  = get32(d+14);
  int  w    = int(get32(d+18));
  int  h    = int(get32(d+22));
  uint pl   = get16(d+26);
  uint bpp  = get16(d+28);
  uint comp = get32(d+30);
  if( isz<40 || pl!=1 || comp!=0 ) return b;
  if( bpp!=24 && bpp!=32 ) return b;             // RGB / RGBA only
  if( w<=0 || h==0 ) return b;
  uint H_ = uint( h<0 ? -h : h ), W_ = uint(w);
  if( off<HDR0 || off>flen_ ) return b;
  if( off < 14+isz ) return b;
  uint st = ((W_*bpp+31)/32)*4;
  if( st==0 || H_ > (0xFFFFFFFFu/st) ) return b;
  uint pix = st*H_;
  if( pix > flen_-off ) return b;
  b.ok = 1; b.off = off; b.W = W_; b.H = H_; b.nc = bpp/8;
  b.stride = st; b.pixbytes = pix;
  return b;
}

static BC (*o1)[256];
static INLINE uint CodeByte( BC* t, uint c ) { return CodeBits( t, c, 8 ); }

int main( int argc, char** argv ) {
  uint c, f_len, f_pos;
  FILE *f, *g;

#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

  if( argc<4 ) {
    printf(
      "mrpc - lossless RGB/RGBA BMP compressor on MRP's scheme\n"
      "\n"
      "Usage: %s <mode> <input> <output>\n"
      "\n"
      "  <mode>    'c' compress, 'd' decompress\n"
      "\n"
      "The image is segmented into %d-odd classes over a quadtree, one\n"
      "linear predictor is fitted per (class, colour component) over\n"
      "%d causal taps of its own component and %d of every other, and the\n"
      "residual is coded with a generalized-Gaussian model picked by the\n"
      "local activity.  Class map, coefficients and quantiser are\n"
      "optimized against measured code length and transmitted.\n"
      "Anything that is not a 24/32bpp raster falls back to order 1.\n",
      argv[0], MRP_MAXCLASS, PRD_ORDER, XPRD_ORDER );
    return 1;
  }

  f_DEC = (argv[1][0]=='d');
  f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  byte hdr[HDR0];
  BmpInfo b; memset(&b,0,sizeof(b));

  if( f_DEC==0 ) {
    f_len = flen(f);
    uint n = f_len<HDR0 ? f_len : HDR0;
    if( n ) { if( fread(hdr,1,n,f)!=n ) return 4; }
    b = ParseBMP( hdr, f_len );
    fwrite( &f_len, 1, sizeof(f_len), g );
    putc( int(b.ok), g );
    fseek( f, 0, SEEK_SET );
    rc.StartEncode(g);
  } else {
    f_len = 0;
    if( fread(&f_len,1,sizeof(f_len),f)!=sizeof(f_len) ) return 4;
    b.ok = uint(getc(f))&1;
    rc.StartDecode(f);
  }

  o1 = (BC(*)[256]) new BC[256*256];
  for( int i=0; i<256*256; i++ ) ((BC*)o1)[i].Init();

  uint last_c = 0;
  f_pos = 0;
  uint nhdr = b.ok ? (f_DEC ? HDR0 : b.off) : f_len;
  for( ; f_pos<nhdr; f_pos++ ) {
    c = (f_DEC==0) ? uint(getc(f)) : 0;
    c = CodeByte( o1[last_c], c );
    if( f_DEC ) putc( int(c), g );
    if( f_pos<HDR0 ) hdr[f_pos] = byte(c);
    last_c = c;
    if( b.ok && f_pos==HDR0-1 && f_DEC ) {
      b = ParseBMP( hdr, f_len );
      if( b.ok==0 ) return 5;
      nhdr = b.off;
    }
  }

  if( b.ok ) {
    static Codec cd;
    cd.Alloc( b.W, b.H, b.nc, b.stride );
    cd.pixbytes = b.pixbytes;
    cd.img = new byte[b.pixbytes];
    if( f_DEC==0 ) { if( fread(cd.img,1,b.pixbytes,f)!=b.pixbytes ) return 6; }

#if MRP_PROGRESS
    g_prog = (f_DEC==0) &&
             (MRP_PROGRESS>1 || double(b.W)*double(b.H) >= MRP_PROGMIN);
#endif
    t_start = tnow();
    if( f_DEC==0 ) cd.Encode();
    else { cd.Decode(); cd.StoreOrg(); }

    // row padding is not part of the raster and is rarely zero
    if( b.stride > b.W*b.nc ) {
      for( uint y=0; y<b.H; y++ )
        for( uint x=b.W*b.nc; x<b.stride; x++ ) {
          byte* q = cd.img + size_t(y)*b.stride + x;
          uint v = CodeByte( o1[2], f_DEC ? 0 : uint(*q) );
          if( f_DEC ) *q = byte(v);
        }
    }
    if( f_DEC ) fwrite( cd.img, 1, b.pixbytes, g );

    // and whatever trails the raster
    uint tail = f_len - b.off - b.pixbytes;
    for( uint i=0; i<tail; i++ ) {
      c = (f_DEC==0) ? uint(getc(f)) : 0;
      c = CodeByte( o1[last_c], c );
      if( f_DEC ) putc( int(c), g );
      last_c = c;
    }
  }

  if( f_DEC==0 ) rc.FinishEncode();
  fclose(g); fclose(f);
  return 0;
}
