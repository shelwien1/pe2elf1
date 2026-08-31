// -------------------------------------------------------------
// mrpc_lib.cpp -- the codec, behind the C API in mrpc_lib.h.
//
// Nothing in here reads or writes a file, knows what a BMP is, or looks
// at argv: a raster comes in and a blob goes out.  The frontend in
// mrpc.cpp does the rest.
//
// mrpc -- lossless RGB/RGBA image compression on MRP's scheme
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

#ifndef CLASS_TRIAL
#define CLASS_TRIAL  0   // 1 = measure the class count instead of predicting
                         // it: one pass of each loop per candidate
#endif
#ifndef MRP_CLASS
#define MRP_CLASS    0   // predictor classes; 0 = pick from the image size
#endif
#ifndef MRP_MAXCLASS
#define MRP_MAXCLASS 63  // MRP's own cap.  Nothing in the codec needs it to be
                         // this: raise it and every array that depends on it
                         // grows linearly, up to 255 where the class map's
                         // byte runs out.  Measured, raising it is not worth
                         // it -- see TUNING.md.  It is part of the format:
                         // the class count is coded over MRP_MAXCLASS+1
                         // symbols, so a stream written at one setting will
                         // not decode under another.
#endif
#ifndef MRP_GROUP
#define MRP_GROUP    16  // activity groups, i.e. probability models in use
#endif
#ifndef SIGMA_LO
#define SIGMA_LO   0.15  // the scale ladder those groups draw from
#endif
#ifndef SIGMA_HI
#define SIGMA_HI  30.89
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
#ifndef NUM_SHAPE
#define NUM_SHAPE    16  // generalized-Gaussian shapes to choose among
#endif
#ifndef NUM_TAIL
#define NUM_TAIL      5  // ... times this many tail weights.  A generalized
                         // Gaussian's tail is tied to its peak -- one shape
                         // parameter sets both -- and real residuals do not
                         // oblige.  Mixing a little uniform mass in unties
                         // them, which is the one degree of freedom the
                         // family is missing; see TUNING.md.  1 is the codec
                         // without it, byte for byte.  Five costs nothing
                         // measurable in time -- the model search is under a
                         // percent of the encode -- and 42 MB of tables.
#endif
#define NUM_PMODEL (NUM_SHAPE*NUM_TAIL)
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
#ifndef MAX_ITER
#define MAX_ITER     20  // iterations of each optimisation loop
#endif
#ifndef EXTRA_ITER
#define EXTRA_ITER    4  // ... after the last improvement
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
// Which of the three searches the device is allowed to run.  All three by
// default; the switches are here so that one of them can be taken out of
// a comparison without taking the device out of the build.
#ifndef MRP_CL_CLASS
#define MRP_CL_CLASS  1
#endif
#ifndef MRP_CL_COEF
#define MRP_CL_COEF   1
#endif
#ifndef MRP_CL_GROUP
#define MRP_CL_GROUP  1
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
#ifndef MIN_SAMP
#define MIN_SAMP     0   // least-squares samples per coefficient a class has
                         // to hold to stay alive.  0 disables the check and
                         // is the codec that chooses its class count up
                         // front; see MergeStarved.
#endif
#define MAX_TOTFREQ (1<<20)
#define MIN_FREQ 1

#ifndef MRP_SSE
#define MRP_SSE    0x11  // Secondary estimation on the residual: the static
                         // pmf is kept and one multiplicative correction per
                         // band of the residual is learned as the image is
                         // coded -- a couple of dozen numbers a context
                         // rather than 511, and decodable, because
                         // everything it depends on is already in the
                         // stream.  Three fields:
                         //
                         //   [3:0]  what the correction is keyed on
                         //     0 = off; the codec without it, byte for byte
                         //     1 = (component, activity group)
                         //     2 = ... and the subpixel position
                         //     3 = ... and how big the residual to the left
                         //         was, in four steps
                         //     4 = ... to the left and above, three each
                         //   [7:4]  how the residual is banded
                         //     0 = log2 of the magnitude         18 bands
                         //     1 = exact to 7, log2 beyond       28 bands
                         //     2 = exact to 15, log2 beyond      42 bands
                         //   [11:8] how the correction is estimated
                         //     0 = a gradient step at a fixed rate
                         //     1 = observed over expected, with a prior
                         //     2 = a gradient step on a count-scheduled rate
                         //
                         // This is only the default the trial falls back on:
                         // ChooseSse picks one per image and the stream says
                         // which.  MRPC_SSE pins it for a measurement.
#endif
#ifndef SSE_RATE
#define SSE_RATE      5  // log2 of the reciprocal adaptation rate
#endif


#include "mrpc_lib.h"
#include "sh_common.inc"
#include <time.h>

// Wall clock for the progress report.  This was clock(), CPU time, which
// is the same number for a single-threaded encoder; with the
// optimisation loop on a device it is several times the number the user
// is actually waiting on.
#if defined(_WIN32)||defined(_MSC_VER)
static double tnow(void) {
  return double(clock())/double(CLOCKS_PER_SEC);
}
#else
static double tnow(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return double(t.tv_sec)+double(t.tv_nsec)*1e-9;
}
#endif

static double t_start = 0.0;
static int g_prog = 0;
#define PROG(...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
        do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          if( g_prog ) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
            fprintf(stderr, __VA_ARGS__);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
            fflush(stderr);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
          }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
        } while( 0 )

static const int NUM_UPELS = UPEL_DIST*(UPEL_DIST+1);
static const int NUM_SUBPM = 1<<PM_ACC;
static const int MAXVAL = 255;
static const int MAXPRD = MAXVAL<<COEF_PREC;
// The clamp on a predictor coefficient, in units of 1.0.  It bounds the
// alphabet the coefficient coder works over, so it travels in the stream:
// the encoder picks one of these from the fit it just did.
static const int CRANGE[] = {2, 4, 8};
static const int NCRANGE = int(sizeof(CRANGE)/sizeof(CRANGE[0]));
static const int MAX_COEF = 8<<COEF_PREC;
static const int PMSIZE = MAXVAL*2+1;     // the shifted pmf window
static const int MIN_BSIZE = MAX_BSIZE>>QT_DEPTH;
static const int NTMAX = PRD_ORDER+(MAXC-1)*XPRD_ORDER+MAXC;
static const int PADL = UPEL_DIST+2, PADR = UPEL_DIST+2, PADT = UPEL_DIST+2;

// MRP's causal neighbourhood, in distance order (common.c, dyx[]).
static constexpr int DYX[][2] = {{0, -1}, {-1, 0}, {0, -2}, {-1, -1}, {-2, 0}, {-1, 1}, {0, -3}, {-1, -2}, {-2, -1}, {-3, 0}, {-2, 1}, {-1, 2}, {0, -4}, {-1, -3}, {-2, -2}, {-3, -1}, {-4, 0}, {-3, 1}, {-2, 2}, {-1, 3}, {0, -5}, {-1, -4}, {-2, -3}, {-3, -2}, {-4, -1}, {-5, 0}, {-4, 1}, {-3, 2}, {-2, 3}, {-1, 4}, {0, -6}, {-1, -5}, {-2, -4}, {-3, -3}, {-4, -2}, {-5, -1}, {-6, 0}, {-5, 1}, {-4, 2}, {-3, 3}, {-2, 4}, {-1, 5}};
static const int NDYX = int(sizeof(DYX)/sizeof(DYX[0]));

// The tap tables must reach no further than the padding.  DYX is in
// distance order, so a PRD_ORDER of n uses its first n entries, and the
// furthest of those in each direction is what has to fit.  Without this
// -DPRD_ORDER=42 builds and then reads outside the plane: DYX[30] is
// (0,-6), which wants PADL >= 6, and DYX[36] is (-6,0), which wants
// PADT >= 6, where both are UPEL_DIST+2 = 5.
enum { RCH_UP = 0, RCH_LEFT = 1, RCH_RIGHT = 2 };
static constexpr int DyxReach(int n, int which) {
  int m = 0;
  for( int i = 0; i<n&&i<NDYX; i++ ) {
    int v = (which==RCH_UP) ? -DYX[i][0] : (which==RCH_LEFT) ? -DYX[i][1] : DYX[i][1];
    if( v>m )
      m = v;
  }
  return m;
}
static const int TAPS_MAX = (PRD_ORDER>XPRD_ORDER) ? PRD_ORDER : XPRD_ORDER;
static_assert(DyxReach(TAPS_MAX, RCH_UP)<=PADT,
              "PRD_ORDER/XPRD_ORDER reach above the top padding; raise UPEL_DIST");
static_assert(DyxReach(TAPS_MAX, RCH_LEFT)<=PADL,
              "PRD_ORDER/XPRD_ORDER reach left of the padding; raise UPEL_DIST");
static_assert(DyxReach(TAPS_MAX, RCH_RIGHT)<=PADR,
              "PRD_ORDER/XPRD_ORDER reach right of the padding; raise UPEL_DIST");

// The sigma ladder the groups are drawn from (common.c, sigma_a[]).
// The scale ladder the groups are drawn from.  MRP's own (common.c,
// sigma_a[]) is sixteen steps from 0.15 to 30.89, very nearly geometric
// with a ratio of 1.378, and it is the *only* place the model's scale
// comes from: the threshold DP chooses which activities land in which
// group, but not what sigma that group has.  So a group whose residuals
// want a scale between two rungs, or above the top one, gets the nearest
// rung and pays for it.  SIGMA_LO/SIGMA_HI generate the ladder instead,
// so both can be moved.
struct SigmaLadder {
  double v[MRP_GROUP];
  void Init() {
    if( MRP_GROUP==16&&SIGMA_LO==0.15&&SIGMA_HI==30.89 ) {
      // MRP's own numbers, to the digit, so the default is unchanged
      static const double a[16] = {0.15, 0.26, 0.38, 0.57, 0.83, 1.18, 1.65, 2.31,
                                   3.22, 4.47, 6.19, 8.55, 11.80, 16.27, 22.42, 30.89};
      for( int i = 0; i<16; i++ )
        v[i] = a[i];
      return;
    }
    double r = pow(double(SIGMA_HI)/double(SIGMA_LO), 1.0/double(MRP_GROUP-1));
    double x = SIGMA_LO;
    for( int i = 0; i<MRP_GROUP; i++, x *= r )
      v[i] = x;
  }
};
static SigmaLadder g_sigma;
#define SIGMA (g_sigma.v)
static const double QTREE_PROB[7] = {0.05, 0.2, 0.35, 0.5, 0.65, 0.8, 0.95};

typedef double cost_t;

// -------------------------------------------------------------
// Rangecoder plumbing.  sh_v2f.inc already carries the cumulative-frequency
// entry points MRP's models want; rc_BProcess is only used by the header
// and fallback models.
// -------------------------------------------------------------
Rangecoder rc;
uint f_DEC = 0;

// A plain adaptive binary probability: no ParamUpdater, one shift.
struct BC {
  word p;
  void Init() {
    p = SCALE/2;
  }

  INLINE uint Code(uint bit) {
    bit = rc.rc_BProcess(p, bit);
    if( bit )
      p = word(p-(p>>5));
    else
      p = word(p+((SCALE-p)>>5));
    if( p<32 )
      p = 32;
    else if( p>SCALE-32 )
      p = SCALE-32;
    return bit;
  }
};
static uint CodeBits(BC* t, uint v, uint n) { // n bits, MSB first
  uint cxt = 1;
  for( uint i = 0; i<n; i++ ) {
    uint bit = (v>>(n-1-i))&1;
    bit = t[cxt].Code(bit);
    cxt += cxt+bit;
  }
  return cxt-(1u<<n);
}

// -------------------------------------------------------------
// Probability models
// -------------------------------------------------------------
struct PMod {
  int size, id;
  uint* freq;
  uint* cumfreq;
  float* cost;    // -log2 freq[k]
  float* subcost; // +log2 of the window total that starts at base
};

static double lngammaf(double x) {
  static const double c[6] = {76.18009172947146, -86.50532032941677, 24.01409824083091, -1.231739572450155, 0.1208650973866179e-2, -0.5395239384953e-5};
  double y = x, t = x+5.5, s = 1.000000000190015;
  t -= (x+0.5)*log(t);
  for( int j = 0; j<6; j++ )
    s += c[j]/++y;
  return -t+log(2.5066282746310005*s/x);
}

// One (sigma, shape) pair -> NUM_SUBPM shifted frequency tables, as
// common.c's set_freqtable: the pdf is sampled NUM_SUBPM times per integer
// so the table can be indexed by the fraction of the prediction.
// How much of the mass each tail weight spreads flat under the curve.
static const double TAILW[8] = {0.0, 1.0/1024, 1.0/256, 1.0/64,
                                1.0/32, 1.0/16, 1.0/8, 1.0/4};

static void set_freqtable(PMod* pm, double* pdf, int center, int idx, double sigma) {
  int shp = (idx<0) ? -1 : idx%NUM_SHAPE;
  double tw = (idx<0) ? 0.0 : TAILW[(idx/NUM_SHAPE)&7];
  double shape = (idx<0) ? 2.0 : 3.2*(shp+1)/double(NUM_SHAPE);
  double beta = exp(0.5*(lngammaf(3.0/shape)-lngammaf(1.0/shape)))/sigma;
  double sw = 1.0/double(NUM_SUBPM);
  int n = pm->size*NUM_SUBPM;
  int ctr = center*NUM_SUBPM;
  for( int i = ctr; i<n; i++ ) {
    double x = (double(i)-double(ctr)+0.5)*sw;
    pdf[i+1] = exp(-pow(beta*x, shape));
  }
  for( int i = 0; i<=ctr; i++ )
    pdf[ctr-i] = pdf[ctr+i+1];
  for( int i = 0; i<n; i++ )
    pdf[i] = (i==ctr) ? (2.0+pdf[i]+pdf[i+1])*0.5 : pdf[i]+pdf[i+1];
  for( int j = 0; j<NUM_SUBPM; j++ ) {
    double norm = 0.0;
    for( int i = 0; i<pm->size; i++ )
      norm += pdf[i*NUM_SUBPM+j];
    const double budget = double(MAX_TOTFREQ-pm->size*MIN_FREQ);
    const double flat = tw*budget/double(pm->size);
    norm = (1.0-tw)*budget/norm+1e-8;
    pm->cumfreq[0] = 0;
    for( int i = 0; i<pm->size; i++ ) {
      pm->freq[i] = uint(norm*pdf[i*NUM_SUBPM+j]+flat)+MIN_FREQ;
      pm->cumfreq[i+1] = pm->cumfreq[i]+pm->freq[i];
    }
    pm++;
  }
}

// [group][shape][subposition], and the two blocks the tables live in.
// The decoder builds only the shapes the stream names, so num_pm is 1
// there.  Init builds, Quit gives back: the codec makes one of these per
// operation and a library that leaked it would leak per call.
struct PModSet {
  PMod* pm;      // [group][shape][subposition]
  uint* freqblk; // the frequency and cumulative tables, one block
  float* costblk;// and the code lengths the optimiser reads
  int n;

  void Init(int num_pm, const int* pm_idx);
  void Costs();

  void Quit() {
    delete[] pm;
    delete[] freqblk;
    delete[] costblk;
    pm = 0;
    freqblk = 0;
    costblk = 0;
    n = 0;
  }
};

// The cost tables the optimiser reads instead of running the coder.
static const int CSTRIDE = PMSIZE+MAXVAL+1;

void PModSet::Init(int num_pm, const int* pm_idx) {
  Quit();
  n = MRP_GROUP*num_pm*NUM_SUBPM;
  pm = new PMod[n];
  freqblk = new uint[size_t(n)*(PMSIZE*2+1)];
  for( int i = 0; i<n; i++ ) {
    pm[i].size = PMSIZE;
    pm[i].freq = freqblk+size_t(i)*(PMSIZE*2+1);
    pm[i].cumfreq = pm[i].freq+PMSIZE;
    pm[i].cost = 0;
    pm[i].subcost = 0;
  }
  double* pdf = new double[size_t(PMSIZE)*NUM_SUBPM+1];
  for( int gr = 0; gr<MRP_GROUP; gr++ )
    for( int i = 0; i<num_pm; i++ ) {
      int idx = pm_idx ? pm_idx[gr] : i;
      for( int j = 0; j<NUM_SUBPM; j++ )
        pm[(gr*num_pm+i)*NUM_SUBPM+j].id = i;
      set_freqtable(pm+size_t(gr*num_pm+i)*NUM_SUBPM, pdf, MAXVAL, idx, SIGMA[gr]);
    }
  delete[] pdf;
}

// One allocation, fixed stride: the eight sub-positions of a group are
// consecutive PMods, so lane s of a gather is cost[0] + s*CSTRIDE and the
// whole 33-candidate sweep becomes two gathers per eight.
void PModSet::Costs() {
  const double a = 1.0/log(2.0);
  delete[] costblk;
  costblk = new float[size_t(n)*CSTRIDE];
  for( int i = 0; i<n; i++ ) {
    PMod &p = pm[i];
    p.cost = costblk+size_t(i)*CSTRIDE;
    p.subcost = p.cost+PMSIZE;
    for( int k = 0; k<PMSIZE; k++ )
      p.cost[k] = float(-a*log(double(p.freq[k])));
    for( int k = 0; k<=MAXVAL; k++ )
      p.subcost[k] = float(a*log(double(p.cumfreq[k+MAXVAL+1]-p.cumfreq[k])));
  }
}

// MRP's set_spmodel: a geometric table for the side information.
static const int SPMAX = (MAX_UPARA+2>MAX_COEF+1) ? MAX_UPARA+2 : MAX_COEF+1;
struct SPMod {
  uint freq[SPMAX+1], cumfreq[SPMAX+2];
  int size;
  void Set(int sz, int m) {
    size = sz;
    if( m>=0 ) {
      double p = 1.0/double(1<<(m%8));
      uint sum = 0;
      for( int i = 0; i<size; i++ ) {
        freq[i] = uint(exp(-p*i)*(1<<10));
        if( !freq[i] )
          freq[i] = 1;
        sum += freq[i];
      }
      if( m&8 )
        freq[0] = sum-freq[0];
    } else
      for( int i = 0; i<size; i++ )
        freq[i] = 1;
    cumfreq[0] = 0;
    for( int i = 0; i<size; i++ )
      cumfreq[i+1] = cumfreq[i]+freq[i];
  }

  cost_t Cost(int i) const {
    return -log(double(freq[i])/double(cumfreq[size]))/log(2.0);
  }
};

static INLINE void EncSym(const PMod* pm, int lo, int hi, int s) {
  uint off = pm->cumfreq[lo], tot = pm->cumfreq[hi]-off;
  rc.rc_Process(pm->cumfreq[s]-off, pm->freq[s], tot);
}

static INLINE int DecSym(const PMod* pm, int lo, int hi) {
  uint off = pm->cumfreq[lo], tot = pm->cumfreq[hi]-off;
  uint v = rc.rc_GetFreq(tot);
  int i = lo, j = hi-1;
  while( i<j ) {
    int k = (i+j)>>1;
    if( pm->cumfreq[k+1]-off<=v )
      i = k+1;
    else
      j = k;
  }
  rc.rc_Process(pm->cumfreq[i]-off, pm->freq[i], tot);
  return i;
}

static INLINE void EncSP(const SPMod &p, int s) {
  rc.rc_Process(p.cumfreq[s], p.freq[s], p.cumfreq[p.size]);
}

static INLINE int DecSP(const SPMod &p) {
  uint v = rc.rc_GetFreq(p.cumfreq[p.size]);
  int i = 0, j = p.size-1;
  while( i<j ) {
    int k = (i+j)>>1;
    if( p.cumfreq[k+1]<=v )
      i = k+1;
    else
      j = k;
  }
  rc.rc_Process(p.cumfreq[i], p.freq[i], p.cumfreq[p.size]);
  return i;
}

// -------------------------------------------------------------
// Secondary estimation
// -------------------------------------------------------------
// Everything above this point is static: a table built before the first
// bit is written, indexed by the transmitted class, threshold and shape.
// The search that chose those tables is exact precisely because a
// symbol's cost does not depend on when it is coded, and that is also
// the ceiling on what the search can find -- the best static model.
//
// This is the one piece that adapts.  It keeps the static pmf and learns
// a multiplicative correction per band of the residual:
//
//     p'(s)  proportional to  p(s) * r[ctx][band(s)]
//
// which factors, because SseBand is monotone in the residual and so each
// band is one contiguous run of symbols:
//
//     p'(s) = P(band) * p(s | band)
//
// The second factor is the static table unchanged -- EncSym over the
// band's own sub-range, the same cumfreq array, no new state.  Only the
// first factor is corrected, and it is a couple of dozen numbers.  With
// r == 1 the two
// stages cost exactly what the one stage cost, so this is a strict
// generalisation rather than a replacement.
//
// r is learned by a gradient step on the log loss, in the log domain and
// therefore multiplicatively:
//
//     r[b] *= 1 + rate*([b == observed] - P(b))
//
// whose fixed point is the r that makes the model's band distribution
// match the empirical one.  It runs on integers, off the same quantised
// P the coder used, so the encoder and the decoder walk it identically.
static const int SSEB = 48;         // the most any rule below uses
static const int SSE_TOT = 1<<16;   // the band stage codes against this
static const uint SSE_ONE = 1<<16;  // r == 1
static const uint SSE_MIN = 1<<8;
static const uint SSE_MAX = 1<<24;
// the counting estimator's prior, in units of one symbol's worth of mass
static const uint SSE_A = 8u*uint(SSE_TOT);
static const uint SSE_HALF = 1u<<29; // halve the counts before they overflow

// How the residual is banded.  Coarse enough that a context sees each band
// often, fine enough that a band is a place the static family can be wrong
// in one direction.  All three rules are sign-and-magnitude and monotone in
// the residual, which is what makes a band one contiguous run of symbols
// and the second stage a plain sub-range of the table already there.
//
//   0  log2 of the magnitude                            18 bands
//   1  exact to 7, log2 beyond                          28 bands
//   2  exact to 15, log2 beyond                         42 bands
//
// Rule 1 is both finer and smaller than rule 0: it spends its resolution
// where the residual actually lives instead of spreading it evenly over
// eight octaves that between them hold a percent of the mass.
static INLINE int SseIlog(int a) {
  int l = 0;
  while( a>0&&l<15 ) {
    a >>= 1;
    l++;
  }
  return l;
}

static INLINE int SseHalf(int rule, int a) {
  switch( rule ) {
    case 1:  return (a<8) ? a : 5+SseIlog(a);
    case 2:  return (a<16) ? a : 12+SseIlog(a);
    default: return SseIlog(a);
  }
}

static INLINE int SseNHalf(int rule) {
  return SseHalf(rule, 255)+1;
}

static INLINE int SseBand(int rule, int r) {
  int n = SseNHalf(rule);
  int a = (r<0) ? -r : r;
  int q = SseHalf(rule, a);
  return (r<0) ? n-1-q : n+q;
}

// The starved-class floor is an encoder decision -- the class count it
// settles on is transmitted -- so it can be swept without touching the
// stream format or rebuilding.
static int g_minsamp = -1;
static INLINE int MinSamp() {
  if( g_minsamp<0 ) {
    const char* e = getenv("MRPC_MINSAMP");
    g_minsamp = e ? atoi(e) : MIN_SAMP;
    if( g_minsamp<0 )
      g_minsamp = 0;
  }
  return g_minsamp;
}

// the build default, overridable for a measurement without a rebuild
static int SseDefault() {
  const char* e = getenv("MRPC_SSE");
  int v = e ? int(strtol(e, 0, 0)) : MRP_SSE;
  if( v<0 )
    v = 0;
  if( (v&15)>4 )
    v = (v&~15)|4;
  if( ((v>>4)&15)>2 )
    v = (v&~0xF0)|(2<<4);
  if( ((v>>8)&15)>2 )
    v = (v&~0xF00)|(2<<8);
  return v;
}

struct SseMod {
  uint* r;        // [ctx][band], 16.16 -- the correction itself
  uint* e;        // [ctx][band] -- and, for the counting estimator, how
  uint* o;        //   much mass the static model put there against how
                  //   much actually turned up
  uint* n;        // [ctx] symbols seen, for the count-scheduled rate
  int nctx;
  int nb;         // bands this rule uses
  int est;        // 0 gradient step, 1 counts, 2 scheduled gradient
  int bs[SSEB+1]; // band boundaries in symbol coordinates
  byte bmap[PMSIZE];

  void Init(int nc_) {
    nctx = nc_;
    r = new uint[size_t(nc_)*SSEB];
    e = new uint[size_t(nc_)*SSEB];
    o = new uint[size_t(nc_)*SSEB];
    n = new uint[size_t(nc_)];
    est = 0;
    SetRule(0);
  }

  void SetRule(int rule) {
    nb = 2*SseNHalf(rule);
    // SseBand is monotone, so the band of a symbol is a range of symbols
    for( int b = 0; b<=nb; b++ )
      bs[b] = -1;
    for( int sym = 0; sym<PMSIZE; sym++ ) {
      int b = SseBand(rule, sym-MAXVAL);
      bmap[sym] = byte(b);
      if( bs[b]<0 )
        bs[b] = sym;
    }
    bs[nb] = PMSIZE;
    for( int b = nb-1; b>=0; b-- )
      if( bs[b]<0 )
        bs[b] = bs[b+1];
    Reset();
  }

  void Reset() {
    if( !r )
      return;
    for( size_t i = 0; i<size_t(nctx)*SSEB; i++ ) {
      r[i] = SSE_ONE;
      e[i] = SSE_A;
      o[i] = SSE_A;
    }
    for( int i = 0; i<nctx; i++ )
      n[i] = 0;
  }

  void Quit() {
    delete[] r;
    delete[] e;
    delete[] o;
    delete[] n;
    r = 0;
    e = 0;
    o = 0;
    n = 0;
  }

  // The band distribution for one symbol: static mass times correction,
  // renormalised to SSE_TOT exactly.  Bands the window does not reach get
  // frequency 0 and cannot be coded; every band it does reach gets at
  // least 1.  `f0` comes back with the same thing uncorrected, which is
  // what the counting estimator has to accumulate.
  INLINE void Bands(int ctx, const PMod* pm, int lo, int hi,
                    uint* f, uint* f0, uint* cum, int* a0, int* a1) const {
    const uint* rc_ = r+size_t(ctx)*SSEB;
    const uint* ec = e+size_t(ctx)*SSEB;
    const uint* oc = o+size_t(ctx)*SSEB;
    qword w[SSEB], m[SSEB], Z = 0;
    for( int b = 0; b<nb; b++ ) {
      int a = (bs[b]<lo) ? lo : bs[b];
      int z = (bs[b+1]>hi) ? hi : bs[b+1];
      a0[b] = a;
      a1[b] = z;
      m[b] = (z>a) ? qword(pm->cumfreq[z]-pm->cumfreq[a]) : 0;
      // the correction, either as the running gradient estimate or as the
      // ratio of observed to expected, folded straight into the weight
      w[b] = (est==1) ? (m[b]*oc[b])/qword(ec[b] ? ec[b] : 1) : ((m[b]*rc_[b])>>16);
      Z += w[b];
    }
    // one reciprocal, then a multiply a band: no division in the loop
    qword inv = (qword(SSE_TOT)<<32)/(Z ? Z : 1);
    uint T = 0;
    int mx = 0;
    for( int b = 0; b<nb; b++ ) {
      uint v = 0;
      if( a1[b]>a0[b] ) {
        v = uint((w[b]*inv)>>32);
        if( v==0 )
          v = 1;
      }
      f[b] = v;
      T += v;
      if( v>f[mx] )
        mx = b;
    }
    // The rounding slack goes to the largest band.  T can be up to nb over
    // SSE_TOT, from the floor of 1 on bands the window reaches; the largest
    // band holds at least SSE_TOT/nb, which is two orders of magnitude more
    // than that, so this never empties it.
    f[mx] = (T<=uint(SSE_TOT)) ? f[mx]+(uint(SSE_TOT)-T) : f[mx]-(T-uint(SSE_TOT));
    cum[0] = 0;
    for( int b = 0; b<nb; b++ )
      cum[b+1] = cum[b]+f[b];
    if( est==1 ) {
      qword tot = qword(pm->cumfreq[hi]-pm->cumfreq[lo]);
      qword iv = (qword(SSE_TOT)<<32)/(tot ? tot : 1);
      for( int b = 0; b<nb; b++ )
        f0[b] = uint((m[b]*iv)>>32);
    }
  }

  // Estimator 0: one gradient step on the log loss, off the quantised
  // probabilities the coder just used.  Its fixed point is the r that
  // makes the model's band distribution match the empirical one, and its
  // rate is a fixed shift -- so it stays near no correction on a context
  // it has seen too little of, which is what the images the static family
  // already fits want.
  //
  // Estimator 1: the ratio of what turned up to what the static model
  // expected, with a prior.  Sharper early -- a context here sees on the
  // order of a thousand symbols and has a couple of dozen bands to learn
  // -- at the cost of a divide a band, and with no shrinkage beyond the
  // prior.
  INLINE void Update(int ctx, const uint* f, const uint* f0, int bb) {
    if( est!=1 ) {
      uint* rc_ = r+size_t(ctx)*SSEB;
      // Estimator 2 schedules the rate off how much of this context has
      // been seen: 1/4 on the first few symbols, settling to 1/2^SSE_RATE.
      // A context here sees on the order of a thousand symbols and has a
      // couple of dozen bands to learn, so most of what a fixed rate can
      // ever learn it learns in the first tenth of the image.
      int sh = SSE_RATE;
      if( est==2 ) {
        uint m2 = ++n[ctx];
        sh = 2;
        while( sh<SSE_RATE&&(m2>>sh) )
          sh++;
      }
      for( int b = 0; b<nb; b++ ) {
        int g = int((b==bb) ? uint(SSE_TOT) : 0u)-int(f[b]);
        qword d = (qword(rc_[b])*qword(g<0 ? -g : g))>>(16+sh);
        qword v = (g<0) ? (qword(rc_[b])>d ? qword(rc_[b])-d : 0) : qword(rc_[b])+d;
        rc_[b] = uint(v<qword(SSE_MIN) ? qword(SSE_MIN) : (v>qword(SSE_MAX) ? qword(SSE_MAX) : v));
      }
      return;
    }
    uint* ec = e+size_t(ctx)*SSEB;
    uint* oc = o+size_t(ctx)*SSEB;
    qword sum = 0;
    for( int b = 0; b<nb; b++ ) {
      ec[b] += f0[b];
      sum += ec[b];
    }
    oc[bb] += uint(SSE_TOT);
    if( sum>qword(SSE_HALF) )
      for( int b = 0; b<nb; b++ ) {
        ec[b] = (ec[b]>>1)+1;
        oc[b] = (oc[b]>>1)+1;
      }
  }
};

// -------------------------------------------------------------
// The OpenCL layer sits between the tables above and the codec below:
// it needs the strides of the cost tables and the geometry constants,
// and the codec needs to be able to call it.
// -------------------------------------------------------------
#ifdef MRP_OPENCL
#include "mrpc_cl.inc"
#else
// so that the options mean the same in both builds
struct CLOpts {
  int use, plat, dev, type, verbose, cache;
};

static CLOpts g_clopt = {0, -1, -1, 0, 0, 0};
#endif

// -------------------------------------------------------------
// The codec
// -------------------------------------------------------------
struct MRPC {
  uint W, H, nc, stride, BS;
  uint iw, ih;   // the raster's own width and height.  They are W and H
                 // unless the image is being coded transposed, in which
                 // case W and H are the other way round and only LoadOrg,
                 // StoreOrg and CodePad still speak the raster's geometry.
  int tflip;
  // Per raster component, the bijection the values go through on the way
  // in.  See BuildValueMaps: it is what stops a plane that only uses one
  // value in sixteen from paying for the fifteen it does not.
  byte vmap[MAXC][256];  // raster value -> what the codec codes
  byte vinv[MAXC][256];  // and back
  int vkind[MAXC];       // 0 identity, 1 affine, 2 the used set
  int vlo[MAXC], vgs[MAXC];
  int vmode;             // what the encoder may use: 0 none, 1 the scale
                         // only, 2 also the used set
  byte vused[MAXC][256];
  byte* plotbuf;     // where CI_PLOT writes, in the caller's raster
  double plot_bits;  // and what it came to before the rounding
  byte* qmapbuf;     // and where the class map goes, one byte a pixel
  int num_class;
  int ord[MAXC]; // coding position -> component of the raster

  byte* org;   // bordered raster, nc bytes/pixel
  short* prd;  // clamped fixed-point prediction
  short* errB; // bordered |2e|, same layout as org
  short* upara;
  char* grp;
  byte* cls; // one class per pixel, shared by components.  Unsigned on
             // purpose: signed char put a ceiling of 127 on
             // MRP_MAXCLASS that had nothing to do with the codec.

  int nt[MAXC];
  int toff[MAXC][NTMAX];
  // The same taps in de-interleaved coordinates.  org is nc bytes per
  // pixel, so eight consecutive pixels of one component are a stride-nc
  // gather; out of a per-component plane they are one 8-byte load, which
  // is the difference between a gather and a vector.
  byte* plane[MAXC];
  int tpl[MAXC][NTMAX]; // which plane each tap reads
  int tof[MAXC][NTMAX]; // its offset, in plane elements
  int uoff[NUM_UPELS], uw[NUM_UPELS];

  int (*coef)[MAXC][NTMAX]; // [class][component][tap]
  int th[MRP_MAXCLASS][MAXC][MRP_GROUP];
  char (*uq)[MAXC][MAX_UPARA+1];
  int pm_idx[MAXC][MRP_GROUP];

  PModSet pmset; // owns pmodels
  SseMod sse;    // the one adaptive part of the model
  // Which model this stream carries, in the fields MRP_SSE describes.  It
  // travels in `params` rather than being compiled in, so that one build
  // decodes any stream rather than quietly producing garbage, and so that
  // the encoder can choose per image -- see ChooseSse.
  int sse_on;
  // Where the correction is keyed.  Beyond (component, activity group) the
  // useful axis turns out not to be a finer description of the prediction
  // but a coarse description of the neighbourhood's *last* residual: the
  // static model has no notion of "nothing has been happening here", and a
  // flat region is exactly where its tails are most wrong.  Which of them
  // wins depends on how many symbols the image has, so all of them are on
  // the menu and none of them is the rule.
  static INLINE int BC4(int r) {
    int a = (r<0) ? -r : r;
    return (a>3) ? 3 : ((a>1) ? 2 : a);
  }
  static INLINE int BC3(int r) {
    int a = (r<0) ? -r : r;
    return (a>1) ? 2 : a;
  }
  INLINE int SseCtx(uint k, int gr, int q, int lf, int ab) const {
    int c = int(k*MRP_GROUP)+gr;
    switch( sse_on&15 ) {
      case 2:  return c*NUM_SUBPM+(q&(NUM_SUBPM-1));
      case 3:  return c*4+BC4(lf);
      case 4:  return (c*3+BC3(lf))*3+BC3(ab);
      default: return c;
    }
  }
  static const int SSE_NCTX = MAXC*MRP_GROUP*(NUM_SUBPM>9 ? NUM_SUBPM : 9);
  PMod* pmodels;
  int num_pm;
  PMod* pml[MAXC][MRP_GROUP];
  // the same thing as a bare pointer: the inner loops were paying two
  // dependent loads (pml -> PMod -> cost) for every pixel and component
  const float* pmlc[MAXC][MRP_GROUP];

  cost_t class_cost[MRP_MAXCLASS];
  cost_t qtflag_cost[QT_DEPTH<<3];
  cost_t th_cost[MAX_UPARA+2];
  cost_t coef_cost[16][MAX_COEF+1];
  int mxc;       // the clamp this image settled on, CRANGE[crange]<<COEF_PREC
  int crange;
  int coef_m[MAXC][NTMAX];
  int qtctx[QT_DEPTH<<3];
  int qtree_code[QT_DEPTH<<2];
  char* qtmap[QT_DEPTH];
  int qtw[QT_DEPTH], qth[QT_DEPTH];
  int mtfbuf[MRP_MAXCLASS];
  int opt_loop;
  uint rnd;
  uint tb0[TRIAL_BANDS], tb1[TRIAL_BANDS]; // rows the trial looks at
  int tnb;                                 // 0 = the whole image
  ALIGN(32) int cA[40], cB[40];            // n/SSR and n%SSR of the 33 candidates
  // Pixels grouped by class.  The coefficient search visits one class at a
  // time and used to find it by scanning the whole image -- once per
  // (class, component, tap pair), which on this corpus is ten thousand
  // full-image scans to reach a sixty-third of the pixels each time.
  uint* cpo; // bordered offset (org, errB)
  uint* cpq; // plain offset (prd, grp)
  size_t cbeg[MRP_MAXCLASS+1];
  // Scratch the searches reuse.  These were function statics, which is
  // one process-wide copy each -- fine for a program that compresses one
  // image and exits, a leak per call and a race between contexts for a
  // library.  They belong to the codec, and Quit gives them back.
  cost_t* s_cbuf; // the group histogram
  uint* s_cb32;   // the class list, as the kernels index it
  double* s_scan; // one coefficient sweep's 33 candidates per class
  double* s_pc2;  // the shape choice

  INLINE uint Rand() {
    rnd = rnd*1664525u+1013904223u;
    return rnd>>8;
  }

  INLINE size_t OI(uint y, uint x) const {
    return (size_t(y+PADT)*BS+(x+PADL))*nc;
  }

  INLINE size_t PI(uint y, uint x) const {
    return (size_t(y)*W+x)*nc;
  }

  // ---------------------------------------------------------------
  void BuildTaps() {
    for( uint k = 0; k<nc; k++ ) {
      int n = 0;
      for( int i = 0; i<PRD_ORDER&&i<NDYX; i++ )
        toff[k][n++] = (DYX[i][0]*int(BS)+DYX[i][1])*int(nc)+int(k);
      for( uint j = 0; j<nc; j++ ) {
        if( j==k )
          continue;
        for( int i = 0; i<XPRD_ORDER&&i<NDYX; i++ )
          toff[k][n++] = (DYX[i][0]*int(BS)+DYX[i][1])*int(nc)+int(j);
      }
#if XCUR
      // the components of this pixel already coded: offset 0, and the one
      // thing a per-plane coder cannot say
      for( uint j = 0; j<k; j++ )
        toff[k][n++] = int(j);
#endif
      nt[k] = n;
    }
    for( uint k = 0; k<nc; k++ ) {
      int n = 0;
      for( int i = 0; i<PRD_ORDER&&i<NDYX; i++, n++ ) {
        tpl[k][n] = int(k);
        tof[k][n] = DYX[i][0]*int(BS)+DYX[i][1];
      }
      for( uint j = 0; j<nc; j++ ) {
        if( j==k )
          continue;
        for( int i = 0; i<XPRD_ORDER&&i<NDYX; i++, n++ ) {
          tpl[k][n] = int(j);
          tof[k][n] = DYX[i][0]*int(BS)+DYX[i][1];
        }
      }
#if XCUR
      for( uint j = 0; j<k; j++, n++ ) {
        tpl[k][n] = int(j);
        tof[k][n] = 0;
      }
#endif
    }
    for( int i = 0; i<NUM_UPELS; i++ ) {
      uoff[i] = (DYX[i][0]*int(BS)+DYX[i][1])*int(nc);
      double d = sqrt(double(DYX[i][0]*DYX[i][0]+DYX[i][1]*DYX[i][1]));
      uw[i] = int(64.0/d+0.5);
    }
  }

  INLINE int Predict(uint k, const byte* p, const int* cf) const {
    int v = 0;
    const int* to = toff[k];
    for( int i = 0, n = nt[k]; i<n; i++ )
      v += int(p[to[i]])*cf[i];
    return v;
  }

  static INLINE int Clip(int v) {
    return (v<0) ? 0 : (v>MAXPRD ? MAXPRD : v);
  }

  // |2*org - prd_half| with MRP's econv twist, which is what the activity
  // measure and the group quantiser are calibrated on.
  static INLINE int Econv(int org_, int prdc) {
    int j = prdc>>(COEF_PREC-1);
    int t = (org_<<1)-j;
    return (t>0) ? t-1 : -t;
  }

#if defined(__AVX2__)
  // All nc components at once: a pixel's error values are adjacent in
  // errB, so one 64-bit load per neighbour covers every component and the
  // twelve-tap weighted sum becomes twelve vector madds instead of 12*nc
  // scalar ones.  Returns the sums before the shift, as the scalar does.
  INLINE void ActivityN(const short* e, int* u) const {
    __m128i acc = _mm_setzero_si128();
    for( int i = 0; i<NUM_UPELS; i++ ) {
      __m128i v = _mm_cvtepi16_epi32(_mm_loadl_epi64((const __m128i*)(e+uoff[i])));
      acc = _mm_add_epi32(acc, _mm_mullo_epi32(v, _mm_set1_epi32(uw[i])));
    }
    _mm_storeu_si128((__m128i*)u, acc);
  }

#endif
  INLINE int Activity(uint k, const short* e) const {
    int u = 0;
    for( int i = 0; i<NUM_UPELS; i++ )
      u += int(e[uoff[i]+int(k)])*uw[i];
#if XUPEL
    for( uint j = 0; j<k; j++ )
      u += int(e[j])*64;
#endif
    u >>= 6;
    return (u>MAX_UPARA) ? MAX_UPARA : u;
  }

  // base and subposition of the pmf window for a clamped prediction
  static INLINE int Qprd(int prdc) {
    return (MAXPRD-prdc+(1<<(COEF_PREC-PM_ACC-1)))>>(COEF_PREC-PM_ACC);
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
  void BorderLeft(uint y) {
    byte* p = org+OI(y, 0);
    short* e = errB+OI(y, 0);
    byte sv[MAXC];
    short se[MAXC];
    for( uint k = 0; k<nc; k++ ) {
      sv[k] = (y>0) ? p[-int(BS)*int(nc)+int(k)] : 128;
      se[k] = (y>0) ? e[-int(BS)*int(nc)+int(k)] : 0;
    }
    for( int j = 1; j<=PADL; j++ )
      for( uint k = 0; k<nc; k++ ) {
        p[-j*int(nc)+int(k)] = sv[k];
        e[-j*int(nc)+int(k)] = se[k];
      }
  }

  void BorderRight(uint y) {
    byte* p = org+OI(y, W-1);
    short* e = errB+OI(y, W-1);
    for( int j = 1; j<=PADR; j++ )
      for( uint k = 0; k<nc; k++ ) {
        p[j*int(nc)+int(k)] = p[k];
        e[j*int(nc)+int(k)] = e[k];
      }
  }

  // the cell (-j, x) for every j, from pixel (0, src)
  void BorderTopCol(int x, int src) {
    if( x<-PADL||x>=int(W)+PADR )
      return;
    byte* s = org+OI(0, 0)+size_t(src)*nc;
    short* q = errB+OI(0, 0)+size_t(src)*nc;
    for( int j = 1; j<=PADT; j++ ) {
      byte* p = org+OI(0, 0)+(-j*int(BS)+x)*int(nc);
      short* e = errB+OI(0, 0)+(-j*int(BS)+x)*int(nc);
      for( uint k = 0; k<nc; k++ ) {
        p[k] = s[k];
        e[k] = q[k];
      }
    }
  }

  // encoder-side: the whole border at once, org already loaded
  void FillBorders() {
    for( uint y = 0; y<H; y++ ) {
      BorderLeft(y);
      BorderRight(y);
    }
    for( int x = -PADL; x<int(W)+PADR; x++ ) {
      int s = x-PADR-1;
      if( s<0 )
        continue; // stays at the 128/0 fill
      if( s>int(W)-1 )
        s = int(W)-1;
      BorderTopCol(x, s);
    }
  }

  // org -> planes.  Encoder only: the decoder predicts one pixel at a
  // time and has nothing to batch.
  INLINE size_t PL(uint y, uint x) const {
    return size_t(y+PADT)*BS+(x+PADL);
  }

  void BuildPlanes() {
    size_t bn = size_t(H+PADT)*BS;
    for( uint k = 0; k<nc; k++ ) {
      if( !plane[k] )
        plane[k] = new byte[bn];
      const byte* p = org+k;
      byte* d = plane[k];
      for( size_t i = 0; i<bn; i++, p += nc )
        d[i] = *p;
    }
  }

#if defined(__AVX2__)
  // Eight consecutive pixels of component k under one coefficient vector.
  INLINE void Predict8(uint k, const int* cf, size_t o, int* out) const {
    __m256i a0 = _mm256_setzero_si256(), a1 = _mm256_setzero_si256();
    const int* tp = tpl[k];
    const int* to = tof[k];
    int n = nt[k], i = 0;
    for(; i+2<=n; i += 2 ) {
      const byte* q0 = plane[tp[i]]+o+to[i];
      const byte* q1 = plane[tp[i+1]]+o+to[i+1];
      __m256i v0 = _mm256_cvtepu8_epi32(_mm_loadl_epi64((const __m128i*)q0));
      __m256i v1 = _mm256_cvtepu8_epi32(_mm_loadl_epi64((const __m128i*)q1));
      a0 = _mm256_add_epi32(a0, _mm256_mullo_epi32(v0, _mm256_set1_epi32(cf[i])));
      a1 = _mm256_add_epi32(a1, _mm256_mullo_epi32(v1, _mm256_set1_epi32(cf[i+1])));
    }
    for(; i<n; i++ ) {
      const byte* q = plane[tp[i]]+o+to[i];
      __m256i v = _mm256_cvtepu8_epi32(_mm_loadl_epi64((const __m128i*)q));
      a0 = _mm256_add_epi32(a0, _mm256_mullo_epi32(v, _mm256_set1_epi32(cf[i])));
    }
    _mm256_storeu_si256((__m256i*)out, _mm256_add_epi32(a0, a1));
  }

  // clamp to [0,MAXPRD] and the doubled absolute error, eight at a time
  INLINE void ClipErr8(const int* pv, const byte* org8, size_t ostep, short* prdo, short* erro, size_t stepo) const {
    for( int i = 0; i<8; i++ ) {
      int v = Clip(pv[i]);
      prdo[i*stepo] = short(v);
      erro[i*stepo] = short(Econv(int(org8[i*ostep]), v));
    }
  }

#endif

  // ---------------------------------------------------------------
  void PredictRegion(uint y0, uint x0, uint y1, uint x1) {
    for( uint y = y0; y<y1; y++ ) {
      const byte* pc = cls+size_t(y)*W;
      uint x = x0;
#if defined(__AVX2__)
      // eight at a time as long as the class holds; the quadtree bottoms
      // out at MIN_BSIZE so runs of one class are at least that long
      ALIGN(32) int pv[8];
      while( x+8<=x1 ) {
        int cl = int(pc[x]);
        uint r = x+1;
        while( r<x1&&int(pc[r])==cl )
          r++;
        if( r-x<8 ) {   // short run: scalar
          const byte* p = org+OI(y, x);
          short* pr = prd+PI(y, x);
          short* pe = errB+OI(y, x);
          for( uint t = x; t<r; t++, p += nc, pr += nc, pe += nc )
            for( uint k = 0; k<nc; k++ ) {
              int v = Clip(Predict(k, p, coef[cl][k]));
              pr[k] = short(v);
              pe[k] = short(Econv(int(p[k]), v));
            }
          x = r;
          continue;
        }
        for(; x+8<=r; x += 8 ) {
          size_t o = PL(y, x);
          for( uint k = 0; k<nc; k++ ) {
            Predict8(k, coef[cl][k], o, pv);
            ClipErr8(pv, org+OI(y, x)+k, nc, prd+PI(y, x)+k, errB+OI(y, x)+k, nc);
          }
        }
      }
#endif
      {
        const byte* p = org+OI(y, x);
        short* pr = prd+PI(y, x);
        short* pe = errB+OI(y, x);
        for(; x<x1; x++, p += nc, pr += nc, pe += nc ) {
          int cl = int(pc[x]);
          for( uint k = 0; k<nc; k++ ) {
            int v = Clip(Predict(k, p, coef[cl][k]));
            pr[k] = short(v);
            pe[k] = short(Econv(int(p[k]), v));
          }
        }
      }
    }
  }

  cost_t CalcCost(uint y0, uint x0, uint y1, uint x1) {
    cost_t cost = 0;
    for( uint y = y0; y<y1; y++ ) {
      const byte* p = org+OI(y, x0);
      const short* pr = prd+PI(y, x0);
      const short* pe = errB+OI(y, x0);
      short* pu = upara+PI(y, x0);
      char* pg = grp+PI(y, x0);
      const byte* pc = cls+size_t(y)*W+x0;
      for( uint x = x0; x<x1; x++ ) {
        int cl = int(*pc++);
#if defined(__AVX2__)
        ALIGN(16) int uv[4];
        ActivityN(pe, uv);
#endif
        for( uint k = 0; k<nc; k++ ) {
#if defined(__AVX2__)
          int u = uv[k];
          for( uint j = 0; j<k*XUPEL; j++ )
            u += int(pe[j])*64;
          u >>= 6;
          if( u>MAX_UPARA )
            u = MAX_UPARA;
#else
          int u = Activity(k, pe);
#endif
          pu[k] = short(u);
          int gr = int(uq[cl][k][u]);
          pg[k] = char(gr);
          int q = Qprd(int(pr[k]));
          const float* cq = pmlc[k][gr]+(q&(NUM_SUBPM-1))*CSTRIDE;
          int base = q>>PM_ACC;
          cost += cq[base+int(p[k])]+cq[PMSIZE+base];
        }
        p += nc;
        pr += nc;
        pe += nc;
        pu += nc;
        pg += nc;
      }
    }
    return cost;
  }

  // ---------------------------------------------------------------
  // Initial segmentation: 8x8 blocks ranked by variance, split evenly
  // across the classes (MRP's init_class, summed over components).
  void InitClass() {
    uint nbx = W/BASE_BSIZE, nby = H/BASE_BSIZE;
    if( nbx==0||nby==0 ) {
      memset(cls, 0, size_t(W)*H);
      return;
    }
    size_t nb = size_t(nbx)*nby;
    double* var = new double[nb];
    int* idx = new int[nb];
    for( size_t b = 0; b<nb; b++ ) {
      uint by = uint(b/nbx)*BASE_BSIZE, bx = uint(b%nbx)*BASE_BSIZE;
      double s = 0, s2 = 0;
      for( uint i = 0; i<BASE_BSIZE; i++ ) {
        const byte* p = org+OI(by+i, bx);
        for( uint j = 0; j<BASE_BSIZE*nc; j++ ) {
          double v = p[j];
          s += v;
          s2 += v*v;
        }
      }
      var[b] = s2-s*s/double(BASE_BSIZE*BASE_BSIZE*nc);
      idx[b] = int(b);
    }
    for( size_t i = 1; i<nb; i++ ) { // insertion sort by variance
      int t = idx[i];
      size_t j = i;
      while( j>0&&var[idx[j-1]]>var[t] ) {
        idx[j] = idx[j-1];
        j--;
      }
      idx[j] = t;
    }
    memset(cls, 0, size_t(W)*H);
    for( size_t k = 0; k<nb; k++ ) {
      int cl = int((k*size_t(num_class))/nb);
      uint by = uint(size_t(idx[k])/nbx)*BASE_BSIZE;
      uint bx = uint(size_t(idx[k])%nbx)*BASE_BSIZE;
      for( uint i = 0; i<BASE_BSIZE; i++ )
        for( uint j = 0; j<BASE_BSIZE; j++ )
          cls[size_t(by+i)*W+bx+j] = byte(cl);
    }
    delete[] idx;
    delete[] var;
  }

  // ---------------------------------------------------------------
  // Least squares per (class, component).  MRP weights each sample by
  // 1/sigma[group]^2 once the rate model is in force, which is the
  // statement that a sample from a noisy neighbourhood should not drag the
  // predictor as hard as one from a smooth one.
  // The normal equations only, over rows [y0,y1): the component-order
  // trial fits on a band rather than the whole image, which is a choice
  // between n! candidates and does not need every pixel to make it.
  void FitPredictor(int mmse) {
    static double mat[NTMAX][NTMAX+1];
    static double wg[MRP_GROUP];
    static int piv[NTMAX];
    for( int g = 0; g<MRP_GROUP; g++ )
      wg[g] = mmse ? 1.0 : 1.0/(SIGMA[g]*SIGMA[g]);
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        int n = nt[k];
        for( int i = 0; i<n; i++ )
          for( int j = 0; j<=n; j++ )
            mat[i][j] = 0.0;
        // The tap values go into a contiguous vector once, and the rank-1
        // update of the upper triangle is then FMAs down each row -- 861
        // scalar multiply-adds per pixel become 216 vector ones, with the
        // right-hand column carried in element n of the same sweep.
        ALIGN(32) double xv[NTMAX+8];
        const int* to = toff[k];
        for( int b = 0; b<(tnb ? tnb : 1); b++ )
          for( uint y = (tnb ? tb0[b] : 0); y<(tnb ? tb1[b] : H); y++ ) {
            const byte* pc = cls+size_t(y)*W;
            const byte* p = org+OI(y, 0);
            const char* pg = grp+PI(y, 0);
            for( uint x = 0; x<W; x++, p += nc, pg += nc ) {
              if( int(pc[x])!=cl )
                continue;
              double w = wg[int(pg[k])];
              for( int i = 0; i<n; i++ )
                xv[i] = double(p[to[i]]);
              xv[n] = double(p[k]);
              for( int i = 0; i<n; i++ ) {
                double a = w*xv[i];
                double* m = mat[i];
                int j = i;
#if defined(__AVX2__)&&defined(__FMA__)
                __m256d va = _mm256_set1_pd(a);
                for(; j+4<=n+1; j += 4 )
                  _mm256_storeu_pd(m+j, _mm256_fmadd_pd(va, _mm256_loadu_pd(xv+j), _mm256_loadu_pd(m+j)));
#endif
                for(; j<=n; j++ )
                  m[j] += a*xv[j];
              }
            }
          }
        for( int i = 0; i<n; i++ )
          for( int j = 0; j<i; j++ )
            mat[i][j] = mat[j][i];
        // Gauss-Jordan with partial pivoting
        for( int i = 0; i<n; i++ )
          piv[i] = i;
        for( int i = 0; i<n; i++ ) {
          double best = 0.0;
          int bp = i;
          for( int k2 = i; k2<n; k2++ ) {
            double v = fabs(mat[piv[k2]][i]);
            if( v>best ) {
              best = v;
              bp = k2;
            }
          }
          int t = piv[i];
          piv[i] = piv[bp];
          piv[bp] = t;
          if( best<=1e-10 )
            continue;
          double d = mat[piv[i]][i];
          for( int j = i; j<=n; j++ )
            mat[piv[i]][j] /= d;
          for( int k2 = 0; k2<n; k2++ ) {
            if( k2==i )
              continue;
            double f = mat[piv[k2]][i];
            for( int j = i; j<=n; j++ )
              mat[piv[k2]][j] -= f*mat[piv[i]][j];
          }
        }
        // quantise, then hand the accumulated rounding back to the taps that
        // lost the most of it -- a predictor whose coefficients sum low
        // darkens the whole class
        double e = 0.0;
        for( int i = 0; i<n; i++ ) {
          double d = (fabs(mat[piv[i]][i])>1e-10) ? mat[piv[i]][n]*double(1<<COEF_PREC) : 0.0;
          int q = int(d);
          if( double(q)>d )
            q--;
          if( q<-mxc ) {
            q = -mxc;
            d = q;
          } else if( q>mxc ) {
            q = mxc;
            d = q;
          }
          coef[cl][k][i] = q;
          d -= q;
          e += d;
          mat[piv[i]][n] = d;
        }
        for( int r = int(e+0.5); r>0; r-- ) {
          double d = 0.0;
          int j = 0;
          for( int i = 0; i<n; i++ )
            if( mat[piv[i]][n]>d ) {
              d = mat[piv[i]][n];
              j = i;
            }
          if( coef[cl][k][j]<mxc )
            coef[cl][k][j]++;
          mat[piv[j]][n] = 0.0;
        }
        for( int i = n; i<NTMAX; i++ )
          coef[cl][k][i] = 0;
      }
  }

  cost_t DesignPredictor(int mmse) {
    FitPredictor(mmse);
    PredictRegion(0, 0, H, W);
    return CalcCost(0, 0, H, W);
  }

  // ---------------------------------------------------------------
  // Group quantiser: the thresholds on the activity are chosen by dynamic
  // programming over the exact code length, per class and component.
  // The dynamic program over the activity thresholds, and the group
  // table it fills in.  It reads the histogram and nothing else, so it
  // does not care which of the two paths measured it.
  void GroupDP(cost_t* cbuf, size_t nu) {
#define CB(cl, k, gr) (cbuf+((size_t(cl)*nc+(k))*MRP_GROUP+(gr))*nu)
    static cost_t dp[MAX_UPARA+2];
    static int tre[MRP_GROUP][MAX_UPARA+2];
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        for( int gr = 0; gr<MRP_GROUP; gr++ ) {
          cost_t* c = CB(cl, k, gr);
          for( size_t u = 1; u<nu; u++ )
            c[u] += c[u-1];
        }
        for( size_t u = 0; u<nu; u++ ) {
          dp[u] = CB(cl, k, 0)[u];
          tre[0][u] = 0;
        }
        for( int gr = 1; gr<MRP_GROUP; gr++ ) {
          const cost_t* c = CB(cl, k, gr);
          for( int th1 = int(nu)-1; th1>=0; th1-- ) {
            int th0 = th1;
            cost_t mc = dp[th1]-c[th1]+th_cost[0]+th_cost[th0-tre[gr-1][th0]];
            for( int t = 0; t<th1; t++ ) {
              cost_t cc = dp[t]-c[t]+th_cost[th1-t]+th_cost[t-tre[gr-1][t]];
              if( cc<mc ) {
                mc = cc;
                th0 = t;
              }
            }
            dp[th1] = mc+c[th1];
            tre[gr][th1] = th0;
            if( gr==MRP_GROUP-1 )
              break;
          }
        }
        int t = int(nu)-1;
        for( int gr = MRP_GROUP-1; gr>0; gr-- ) {
          t = tre[gr][t];
          th[cl][k][gr-1] = t;
        }
        th[cl][k][MRP_GROUP-1] = MAX_UPARA+1;
        int u = 0;
        for( int gr = 0; gr<MRP_GROUP; gr++ )
          for(; u<th[cl][k][gr]&&u<=MAX_UPARA; u++ )
            uq[cl][k][u] = char(gr);
        for(; u<=MAX_UPARA; u++ )
          uq[cl][k][u] = char(MRP_GROUP-1);
      }
#undef CB
  }

  cost_t OptimizeGroupHost() {
    size_t nu = MAX_UPARA+2;
    // cost of putting activity u in group gr, per (class, component)
    if( !s_cbuf )
      s_cbuf = new cost_t[size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*nu];
    cost_t* cbuf = s_cbuf;
    size_t need = size_t(num_class)*nc*MRP_GROUP*nu;
    for( size_t i = 0; i<need; i++ )
      cbuf[i] = 0.0;
#define CB(cl, k, gr) (cbuf+((size_t(cl)*nc+(k))*MRP_GROUP+(gr))*nu)
    for( uint y = 0; y<H; y++ ) {
      const byte* p = org+OI(y, 0);
      const short* pr = prd+PI(y, 0);
      const short* pe = errB+OI(y, 0);
      short* pu = upara+PI(y, 0);
      const byte* pc = cls+size_t(y)*W;
      for( uint x = 0; x<W; x++, p += nc, pr += nc, pe += nc, pu += nc ) {
        int cl = int(pc[x]);
#if defined(__AVX2__)
        ALIGN(16) int uv[4];
        ActivityN(pe, uv);
#endif
        for( uint k = 0; k<nc; k++ ) {
#if defined(__AVX2__)
          int u = uv[k];
          for( uint j = 0; j<k*XUPEL; j++ )
            u += int(pe[j])*64;
          u >>= 6;
          if( u>MAX_UPARA )
            u = MAX_UPARA;
#else
          int u = Activity(k, pe);
#endif
          pu[k] = short(u);
          int q = Qprd(int(pr[k])), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
          int v = base+int(p[k]);
          for( int gr = 0; gr<MRP_GROUP; gr++ ) {
            const float* cq = pmlc[k][gr]+size_t(sub)*CSTRIDE;
            CB(cl, k, gr)[u+1] += cq[v]+cq[PMSIZE+base];
          }
        }
      }
    }
    GroupDP(cbuf, nu);
    cost_t cost = CalcCost(0, 0, H, W);

    // and which generalized-Gaussian shape each group should use
    if( opt_loop>1&&num_pm>1 ) {
      static cost_t pc2[MAXC][MRP_GROUP][NUM_PMODEL];
      for( uint k = 0; k<nc; k++ )
        for( int g = 0; g<MRP_GROUP; g++ )
          for( int i = 0; i<NUM_PMODEL; i++ )
            pc2[k][g][i] = 0.0;
      for( uint y = 0; y<H; y++ ) {
        const byte* p = org+OI(y, 0);
        const short* pr = prd+PI(y, 0);
        const char* pg = grp+PI(y, 0);
        for( uint x = 0; x<W; x++, p += nc, pr += nc, pg += nc )
          for( uint k = 0; k<nc; k++ ) {
            int gr = int(pg[k]);
            int q = Qprd(int(pr[k])), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
            int v = base+int(p[k]);
            for( int i = 0; i<NUM_PMODEL; i++ ) {
              const PMod* pm = pmodels+(size_t(gr)*num_pm+i)*NUM_SUBPM+sub;
              pc2[k][gr][i] += pm->cost[v]+pm->subcost[base];
            }
          }
      }
      cost = 0.0;
      for( uint k = 0; k<nc; k++ )
        for( int g = 0; g<MRP_GROUP; g++ ) {
          int best = 0;
          for( int i = 1; i<NUM_PMODEL; i++ )
            if( pc2[k][g][i]<pc2[k][g][best] )
              best = i;
          pm_idx[k][g] = best;
          pml[k][g] = pmodels+(size_t(g)*num_pm+best)*NUM_SUBPM;
          pmlc[k][g] = pml[k][g]->cost;
          cost += pc2[k][g][best];
        }
    }
    return cost;
  }

  // ---------------------------------------------------------------
  // Class map.  Blocks are tried against every class and, in the second
  // loop, split down a quadtree wherever splitting pays for its own flag.
  short* prdbuf;
  short* errbuf;

  void SetPrdBuf(uint tly, uint tlx, uint bufsize) {
    uint brx = (tlx+bufsize<W) ? tlx+bufsize : W;
    uint bry = (tly+bufsize<H) ? tly+bufsize : H;
    for( int cl = 0; cl<num_class; cl++ ) {
      size_t bp = size_t(cl)*bufsize*bufsize*nc;
      for( uint y = tly; y<bry; y++ ) {
        short* pb0 = prdbuf+bp+(size_t(y%bufsize)*bufsize+tlx%bufsize)*nc;
        short* eb0 = errbuf+bp+(size_t(y%bufsize)*bufsize+tlx%bufsize)*nc;
        uint x = tlx;
#if defined(__AVX2__)
        // predict the whole row under this class, then put back the pixels
        // that already have it -- branchless beats correct-but-scalar here
        ALIGN(32) int pv[8];
        for(; x+8<=brx; x += 8 ) {
          size_t o = PL(y, x);
          short* pb = pb0+size_t(x-tlx)*nc;
          short* eb = eb0+size_t(x-tlx)*nc;
          for( uint k = 0; k<nc; k++ ) {
            Predict8(k, coef[cl][k], o, pv);
            ClipErr8(pv, org+OI(y, x)+k, nc, pb+k, eb+k, nc);
          }
          for( uint t = 0; t<8; t++ )
            if( int(cls[size_t(y)*W+x+t])==cl ) {
              const short* pr = prd+PI(y, x+t);
              const short* pe = errB+OI(y, x+t);
              for( uint k = 0; k<nc; k++ ) {
                pb[t*nc+k] = pr[k];
                eb[t*nc+k] = pe[k];
              }
            }
        }
#endif
        {
          const byte* p = org+OI(y, x);
          short* pb = pb0+size_t(x-tlx)*nc;
          short* eb = eb0+size_t(x-tlx)*nc;
          for(; x<brx; x++, p += nc, pb += nc, eb += nc ) {
            if( int(cls[size_t(y)*W+x])==cl ) {
              const short* pr = prd+PI(y, x);
              const short* pe = errB+OI(y, x);
              for( uint k = 0; k<nc; k++ ) {
                pb[k] = pr[k];
                eb[k] = pe[k];
              }
            } else
              for( uint k = 0; k<nc; k++ ) {
                int v = Clip(Predict(k, p, coef[cl][k]));
                pb[k] = short(v);
                eb[k] = short(Econv(int(p[k]), v));
              }
          }
        }
      }
    }
  }

  // The buffer belongs to the enclosing MAX_BSIZE block, so its index is
  // taken modulo bufsize -- a sub-block of the quadtree is not at its
  // origin.
  void PutBlock(int cl, uint tly, uint tlx, uint bry, uint brx, uint bufsize) {
    size_t bp = (size_t(cl)*bufsize*bufsize+size_t(tlx%bufsize))*nc;
    for( uint y = tly; y<bry; y++ ) {
      const short* pb = prdbuf+bp+size_t(y%bufsize)*bufsize*nc;
      const short* eb = errbuf+bp+size_t(y%bufsize)*bufsize*nc;
      short* pr = prd+PI(y, tlx);
      short* pe = errB+OI(y, tlx);
      byte* pc = cls+size_t(y)*W+tlx;
      for( uint x = tlx; x<brx; x++, pb += nc, eb += nc, pr += nc, pe += nc ) {
        *pc++ = char(cl);
        for( uint k = 0; k<nc; k++ ) {
          pr[k] = pb[k];
          pe[k] = eb[k];
        }
      }
    }
  }

  int FindClass(uint tly, uint tlx, uint bry, uint brx, uint bufsize) {
    cost_t mc = 1e30;
    int mcl = 0;
    for( int cl = 0; cl<num_class; cl++ ) {
      PutBlock(cl, tly, tlx, bry, brx, bufsize);
      cost_t c = class_cost[mtfbuf[cl]]+CalcCost(tly, tlx, bry, brx);
      if( c<mc ) {
        mc = c;
        mcl = cl;
      }
    }
    PutBlock(mcl, tly, tlx, bry, brx, bufsize);
    return mcl;
  }

  // `width` is the right edge of the enclosing block, not of the image:
  // the bottom-right child of a split must not look at a cell that lies in
  // the next parent, which the decoder has not reached yet.
  void MtfClass(uint y, uint x, uint bsize, uint width) {
    int ref[3];
    if( y==0 ) {
      if( x==0 )
        ref[0] = ref[1] = ref[2] = 0;
      else
        ref[0] = ref[1] = ref[2] = int(cls[size_t(y)*W+x-1]);
    } else {
      ref[0] = int(cls[size_t(y-1)*W+x]);
      ref[1] = (x==0) ? ref[0] : int(cls[size_t(y)*W+x-1]);
      ref[2] = (x+bsize>=width) ? int(cls[size_t(y-1)*W+x]) : int(cls[size_t(y-1)*W+x+bsize]);
      if( ref[1]==ref[2] ) {
        ref[2] = ref[0];
        ref[0] = ref[1];
      }
    }
    for( int k = 2; k>=0; k-- ) {
      int j = mtfbuf[ref[k]];
      if( j==0 )
        continue;
      for( int i = 0; i<num_class; i++ )
        if( mtfbuf[i]<j )
          mtfbuf[i]++;
      mtfbuf[ref[k]] = 0;
    }
  }

  int QtCtx(int level, uint tly, uint tlx, uint blksize, uint width) {
    int ctx = 0;
    const char* qm = qtmap[level-1];
    int qw = qtw[level-1];
    int y = int((tly/MIN_BSIZE)>>level), x = int((tlx/MIN_BSIZE)>>level);
    if( y>0 ) {
      if( qm[(y-1)*qw+x]==1 )
        ctx++;
      if( tlx+blksize<width&&qm[(y-1)*qw+x+1]==1 )
        ctx++;
    }
    if( x>0&&qm[y*qw+x-1]==1 )
      ctx++;
    return ((level-1)*4+ctx)<<1;
  }

  cost_t VbsClass(uint tly, uint tlx, uint blksize, uint width, int level, uint bufsize) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx||tly>=bry )
      return 0;
    int mtf_save[MRP_MAXCLASS];
    for( int k = 0; k<num_class; k++ )
      mtf_save[k] = mtfbuf[k];
    MtfClass(tly, tlx, blksize, width);
    int cl = FindClass(tly, tlx, bry, brx, bufsize);
    cost_t qtcost = class_cost[mtfbuf[cl]];
    if( level>0 ) {
      int ctx = QtCtx(level, tly, tlx, blksize, width);
      cost_t cost1 = CalcCost(tly, tlx, bry, brx)+class_cost[mtfbuf[cl]]+qtflag_cost[ctx];
      uint half = blksize>>1;
      for( int k = 0; k<num_class; k++ )
        mtfbuf[k] = mtf_save[k];
      qtcost = qtflag_cost[ctx+1];
      qtcost += VbsClass(tly, tlx, half, width, level-1, bufsize);
      qtcost += VbsClass(tly, tlx+half, half, width, level-1, bufsize);
      qtcost += VbsClass(tly+half, tlx, half, width, level-1, bufsize);
      qtcost += VbsClass(tly+half, tlx+half, half, brx, level-1, bufsize);
      cost_t cost2 = CalcCost(tly, tlx, bry, brx)+qtcost;
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( cost1<cost2 ) {
        for( int k = 0; k<num_class; k++ )
          mtfbuf[k] = mtf_save[k];
        MtfClass(tly, tlx, blksize, width);
        qtcost = class_cost[mtfbuf[cl]]+qtflag_cost[ctx];
        PutBlock(cl, tly, tlx, bry, brx, bufsize);
        int ty = qy, tx = qx, by2 = qy+1, bx2 = qx+1;
        for( int l = level; l>0; l-- ) {
          char* qm = qtmap[l-1];
          int qw = qtw[l-1], qh = qth[l-1];
          for( int y = ty; y<by2; y++ )
            for( int x = tx; x<bx2; x++ )
              if( y<qh&&x<qw )
                qm[y*qw+x] = 0;
          ty <<= 1;
          tx <<= 1;
          by2 <<= 1;
          bx2 <<= 1;
        }
      } else {
        qtmap[level-1][qy*qtw[level-1]+qx] = 1;
      }
    }
    return qtcost;
  }

  cost_t OptimizeClassHost() {
    int level = (opt_loop>1) ? QT_DEPTH : 0;
    uint blksize = (opt_loop>1) ? MAX_BSIZE : BASE_BSIZE;
    for( int i = 0; i<num_class; i++ )
      mtfbuf[i] = i;
    // this is the long pole -- every pixel of every block predicted under
    // every class -- so it reports as it goes rather than at the end
    uint nrow = (H+blksize-1)/blksize, done = 0, mark = 0;
    for( uint y = 0; y<H; y += blksize ) {
      for( uint x = 0; x<W; x += blksize ) {
        SetPrdBuf(y, x, blksize);
        VbsClass(y, x, blksize, W, level, blksize);
      }
      done++;
      if( g_prog&&nrow>=8&&done*8/nrow>mark ) {
        mark = done*8/nrow;
        PROG(".");
      }
    }
    return CalcCost(0, 0, H, W);
  }


  // ---------------------------------------------------------------
  // The same search, with the per-pixel arithmetic on an OpenCL device.
  //
  // The host version asks "what does this block cost under class c?"
  // 341 times per 32x32 block -- once per quadtree node per class -- and
  // each answer is a fresh pass over the block's pixels.  Every one of
  // those passes computes the same per-pixel numbers; only the rectangle
  // they are summed over differs.  So the device computes the cube of
  // per-pixel costs once, for every class at once, and the quadtree above
  // reads rectangle sums out of a prefix sum of it.  That is a fifth of
  // the arithmetic before any of it is made parallel.
  //
  // One thing changes with it.  A pixel's activity, which picks its
  // probability model, is a weighted sum of the errors of its causal
  // neighbours -- and inside a block those neighbours belong to whichever
  // sub-block the recursion has just decided on.  The cube has to fix a
  // class for the whole block to compute anything, so it answers as if
  // the neighbours had the class being tried.  At the top of the quadtree
  // that is exactly what the host does; further down it differs wherever
  // two adjacent sub-blocks end up with different classes, which is a
  // decision made on a slightly wrong cost.  The class map is still a
  // class map -- the file decodes, and the cost accounting after the
  // search is exact -- and measured on its own it is worth a few tens of
  // bytes either way; what it is not is bit-identical.  -C turns it off.
#ifdef MRP_OPENCL
  int cl_up;                        // what has been uploaded
  int cub_y, cub_x, cub_h, cub_w;   // the block the cube describes

  // Bring up the device and upload what never changes: the taps as one
  // offset into the plane array each, zero-padded to the constant the
  // kernel's loop count is rounded to, and the cost tables.
  int CLInit() {
    if( !g_clopt.use||!g_cl.ctx )
      return 0;
    // The device and the program belong to the context and outlive the
    // image; only the buffers are sized from it, so a second image
    // through the same codec opens nothing and compiles nothing.
    if( !g_cl.Program() )
      return 0;
    size_t bn = size_t(H+PADT)*BS;
    if( !g_cl.Buffers(W, H, nc, BS, bn, num_class, MRP_GROUP*num_pm*NUM_SUBPM) )
      return 0;

    static int tapoff[MAXC*NTMAX];
    const int np = NTMAX; // the kernels' tap loop bound, zero-padded
    for( int k = 0; k<MAXC; k++ )
      for( int i = 0; i<np; i++ )
        tapoff[k*np+i] = (uint(k)<nc&&i<nt[k]) ? int(tpl[k][i]*bn)+tof[k][i] : 0;
    static int udy[NUM_UPELS], udx[NUM_UPELS], uwv[NUM_UPELS];
    for( int i = 0; i<NUM_UPELS; i++ ) {
      udy[i] = DYX[i][0];
      udx[i] = DYX[i][1];
      uwv[i] = uw[i];
    }
    g_cl.Write(g_cl.d_tapoff, tapoff, size_t(MAXC)*np*sizeof(int), "tapoff");
    g_cl.Write(g_cl.d_udy, udy, sizeof(udy), "udy");
    g_cl.Write(g_cl.d_udx, udx, sizeof(udx), "udx");
    g_cl.Write(g_cl.d_uw, uwv, sizeof(uwv), "uw");
    g_cl.Write(g_cl.d_cA, cA, CL_NCAND*sizeof(int), "cA");
    g_cl.Write(g_cl.d_cB, cB, CL_NCAND*sizeof(int), "cB");
    g_cl.Write(g_cl.d_costtab, pmodels[0].cost,
               size_t(MRP_GROUP)*num_pm*NUM_SUBPM*CSTRIDE*sizeof(float), "costtab");
    cl_up = 0;
    return g_cl.active;
  }

  // the planes change when the component order does, the rest every pass
  int CLSync(int planes_too) {
    if( !g_cl.active )
      return 0;
    size_t bn = size_t(H+PADT)*BS;
    if( planes_too ) {
      for( uint k = 0; k<nc; k++ )
        if( !g_cl.Write(g_cl.d_planes, plane[k], bn, "planes", size_t(k)*bn) )
          return 0;
    }
    static int pmoff[MAXC*MRP_GROUP];
    for( int k = 0; k<MAXC; k++ )
      for( int g = 0; g<MRP_GROUP; g++ )
        pmoff[k*MRP_GROUP+g] = int(size_t((g*num_pm+pm_idx[k<int(nc) ? k : 0][g])*NUM_SUBPM)*CSTRIDE);
    if( !g_cl.Write(g_cl.d_coef, coef, size_t(MRP_MAXCLASS)*MAXC*NTMAX*sizeof(int), "coef") )
      return 0;
    if( !g_cl.Write(g_cl.d_uq, uq, size_t(MRP_MAXCLASS)*MAXC*(MAX_UPARA+1), "uq") )
      return 0;
    if( !g_cl.Write(g_cl.d_pmoff, pmoff, sizeof(pmoff), "pmoff") )
      return 0;
    return 1;
  }

  // the activity halo of a block: the committed errors of everything it
  // can see outside itself.  The kernel fills the rest of the tile.
  void BuildHalo(int b, int tly, int tlx) {
    const int tw = CL_TW[b], th = CL_TH[b];
    const int rows = int(H)+PADT;
    for( uint k = 0; k<nc; k++ ) {
      short* d = g_cl.halo+size_t(k)*th*tw;
      for( int ty = 0; ty<th; ty++ ) {
        const int y = tly+ty-HT_T+PADT;
        short* dr = d+size_t(ty)*tw;
        if( y>=rows ) { // past the bottom of the raster: never read
          for( int tx = 0; tx<tw; tx++ )
            dr[tx] = 0;
          continue;
        }
        const int x0 = tlx-HT_L+PADL;
        const short* s = errB+(size_t(y)*BS+size_t(x0))*nc+k;
        int n = int(BS)-x0;
        if( n>tw )
          n = tw;
        for( int tx = 0; tx<n; tx++, s += nc )
          dr[tx] = *s;
        for( int tx = n; tx<tw; tx++ )
          dr[tx] = 0;
      }
    }
  }

  INLINE cost_t CubeC(int cl, uint tly, uint tlx, uint bry, uint brx) const {
    return g_cl.CubeCost(cl, int(tly)-cub_y, int(tlx)-cub_x, int(bry)-cub_y, int(brx)-cub_x);
  }

  void SetCls(int cl, uint tly, uint tlx, uint bry, uint brx) {
    for( uint y = tly; y<bry; y++ )
      memset(cls+size_t(y)*W+tlx, cl, brx-tlx);
  }

  int FindClassCL(uint tly, uint tlx, uint bry, uint brx) {
    cost_t mc = 1e30;
    int mcl = 0;
    for( int cl = 0; cl<num_class; cl++ ) {
      cost_t c = class_cost[mtfbuf[cl]]+CubeC(cl, tly, tlx, bry, brx);
      if( c<mc ) {
        mc = c;
        mcl = cl;
      }
    }
    SetCls(mcl, tly, tlx, bry, brx);
    return mcl;
  }

  // as VbsClass, but the pixel cost of the region comes back through
  // *pcost instead of being measured again after the fact
  cost_t VbsClassCL(uint tly, uint tlx, uint blksize, uint width, int level, cost_t* pcost) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx||tly>=bry ) {
      *pcost = 0;
      return 0;
    }
    int mtf_save[MRP_MAXCLASS];
    for( int k = 0; k<num_class; k++ )
      mtf_save[k] = mtfbuf[k];
    MtfClass(tly, tlx, blksize, width);
    int cl = FindClassCL(tly, tlx, bry, brx);
    cost_t qtcost = class_cost[mtfbuf[cl]];
    cost_t mycost = CubeC(cl, tly, tlx, bry, brx);
    if( level>0 ) {
      int ctx = QtCtx(level, tly, tlx, blksize, width);
      cost_t cost1 = mycost+class_cost[mtfbuf[cl]]+qtflag_cost[ctx];
      uint half = blksize>>1;
      for( int k = 0; k<num_class; k++ )
        mtfbuf[k] = mtf_save[k];
      cost_t qc = qtflag_cost[ctx+1], sub = 0, cc = 0;
      qc += VbsClassCL(tly, tlx, half, width, level-1, &cc);
      sub += cc;
      qc += VbsClassCL(tly, tlx+half, half, width, level-1, &cc);
      sub += cc;
      qc += VbsClassCL(tly+half, tlx, half, width, level-1, &cc);
      sub += cc;
      qc += VbsClassCL(tly+half, tlx+half, half, brx, level-1, &cc);
      sub += cc;
      cost_t cost2 = sub+qc;
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( cost1<cost2 ) {
        for( int k = 0; k<num_class; k++ )
          mtfbuf[k] = mtf_save[k];
        MtfClass(tly, tlx, blksize, width);
        qtcost = class_cost[mtfbuf[cl]]+qtflag_cost[ctx];
        SetCls(cl, tly, tlx, bry, brx);
        mycost = CubeC(cl, tly, tlx, bry, brx);
        int ty = qy, tx = qx, by2 = qy+1, bx2 = qx+1;
        for( int l = level; l>0; l-- ) {
          char* qm = qtmap[l-1];
          int qw = qtw[l-1], qh = qth[l-1];
          for( int y = ty; y<by2; y++ )
            for( int x = tx; x<bx2; x++ )
              if( y<qh&&x<qw )
                qm[y*qw+x] = 0;
          ty <<= 1;
          tx <<= 1;
          by2 <<= 1;
          bx2 <<= 1;
        }
      } else {
        qtmap[level-1][qy*qtw[level-1]+qx] = 1;
        qtcost = qc;
        mycost = sub;
      }
    }
    *pcost = mycost;
    return qtcost;
  }


  // The first loop's class search, with the measuring batched onto the
  // device and the deciding left here.  Each chunk of blocks comes back
  // as a cost per (block, class); the loop below then walks them in
  // raster order exactly as the host path does, so MtfClass sees the
  // same neighbours and the class costs are the ones that will be paid.
  // What is approximated is the activity halo: a block is measured
  // against the error planes as the pass began rather than against its
  // neighbours' decisions, which is the same trade the quadtree cube
  // already makes inside a block.
  cost_t OptimizeClassBatch() {
    const uint bs = BASE_BSIZE;
    const int nbx = int((W+bs-1)/bs), nby = int((H+bs-1)/bs);
    for( int i = 0; i<num_class; i++ )
      mtfbuf[i] = i;
    uint mark = 0;
    for( int by = 0; by<nby; by++ ) {
      const uint y = uint(by)*bs;
      const uint bry = (y+bs<H) ? y+bs : H;
      for( int x0 = 0; x0<nbx; x0 += g_cl.bchunk ) {
        int n = nbx-x0;
        if( n>g_cl.bchunk )
          n = g_cl.bchunk;
        if( !g_cl.BlockBatch(nbx, by*nbx+x0, n, num_class) )
          return OptimizeClassHost();
        for( int bi = 0; bi<n; bi++ ) {
          const uint x = uint(x0+bi)*bs;
          const uint brx = (x+bs<W) ? x+bs : W;
          MtfClass(y, x, bs, W);
          const float* bc = g_cl.blockcost+size_t(bi)*num_class;
          cost_t mc = 1e30;
          int mcl = 0;
          for( int cl = 0; cl<num_class; cl++ ) {
            cost_t c = class_cost[mtfbuf[cl]]+cost_t(bc[cl]);
            if( c<mc ) {
              mc = c;
              mcl = cl;
            }
          }
          SetCls(mcl, y, x, bry, brx);
        }
      }
      // the row is decided: predict it for real, and let the device have
      // it before the next row measures against it
      PredictRegion(y, 0, bry, W);
      if( !CLClassState(y, bry) )
        return OptimizeClassHost();
      if( g_prog&&nby>=8&&uint(by+1)*8/uint(nby)>mark ) {
        mark = uint(by+1)*8/uint(nby);
        PROG(".");
      }
    }
    return CalcCost(0, 0, H, W);
  }

  cost_t OptimizeClassCL() {
    int level = (opt_loop>1) ? QT_DEPTH : 0;
    uint blksize = (opt_loop>1) ? MAX_BSIZE : BASE_BSIZE;
    int b = (opt_loop>1) ? 0 : 1; // which of the two cube programs
    for( int i = 0; i<num_class; i++ )
      mtfbuf[i] = i;
    uint nrow = (H+blksize-1)/blksize, done = 0, mark = 0;
    for( uint y = 0; y<H; y += blksize ) {
      for( uint x = 0; x<W; x += blksize ) {
        uint bry = (y+blksize<H) ? y+blksize : H;
        uint brx = (x+blksize<W) ? x+blksize : W;
        cub_y = int(y);
        cub_x = int(x);
        cub_h = int(bry-y);
        cub_w = int(brx-x);
        BuildHalo(b, cub_y, cub_x);
        if( g_cl.active&&g_cl.BlockCube(b, cub_y, cub_x, cub_h, cub_w, num_class) ) {
          cost_t pc = 0;
          VbsClassCL(y, x, blksize, W, level, &pc);
          PredictRegion(y, x, bry, brx); // commit what the search chose
        } else {
          // the device dropped out; the host path picks up from a state
          // that is committed at every block boundary
          SetPrdBuf(y, x, blksize);
          VbsClass(y, x, blksize, W, level, blksize);
        }
      }
      done++;
      if( g_prog&&nrow>=8&&done*8/nrow>mark ) {
        mark = done*8/nrow;
        PROG(".");
      }
    }
    return CalcCost(0, 0, H, W);
  }
#endif

  cost_t OptimizeClass() {
#ifdef MRP_OPENCL
    if( MRP_CL_CLASS&&g_cl.active&&CLSync(cl_up==0) ) {
      cl_up = 1;
      if( opt_loop>1 )
        return OptimizeClassCL();
      // The first loop's blocks are BASE_BSIZE and have no quadtree
      // under them: the cube is then one node's work for one node, and
      // 8x8 pixels does not pay for a launch.  Those go through the
      // batched path instead -- a chunk of blocks and every class in one
      // launch -- and to the host if the device has not got it.
      if( g_cl.k_bblk&&CLClassState() )
        return OptimizeClassBatch();
    }
#endif
    return OptimizeClassHost();
  }

  // ---------------------------------------------------------------
  // Starved classes
  // ---------------------------------------------------------------
  // The class count is chosen up front from the pixel count alone, and on
  // a small image that is how the search falls apart: 24 classes x 34 taps
  // over 20k pixels is 25 samples a coefficient, the per-class fits chase
  // noise, the class search cannot tell the classes apart, and the
  // segmentation collapses -- 2 of 24 classes ever used and a quadtree of
  // two leaves.  Output against class count is then bimodal rather than
  // smooth, which is why every attempt to fix this by *moving* the count
  // was worse on average: a rule that moves it sometimes moves it across
  // the boundary.
  //
  // This removes classes that have starved instead.  After each class
  // search, any class holding fewer than MIN_SAMP pixels per coefficient
  // it has to fit is folded into the surviving class whose predictor is
  // closest to it, and the labels are compacted.  The count only ever
  // falls, it falls to something the image can actually fit, and no
  // decision about it is made before the segmentation has been seen.
  //
  // Measured on the codec as it was, a floor of 16 samples a coefficient
  // was worth -0.246% mean and -0.297% total on the corpus.  Measured
  // again with the residual correction and the per-image coefficient
  // clamp in place it is +0.241% mean and -0.125% total, and the +-5%
  // per-image spread never went away: four tiles pay 2.1-4.7% while the
  // other twenty gain.  So it is off by default -- MRPC_MINSAMP turns it
  // on -- because the correction was already buying most of what it buys,
  // and because a change that costs 5% on a sixth of the corpus to gain
  // 0.1% on the total is not one to make on this evidence.
  //
  // Returns 1 if anything was merged, so the caller knows to refresh the
  // side costs, the predictions and the cost.
  int MergeStarved() {
    const int ms = MinSamp();
    if( !ms||num_class<2 )
      return 0;
    int ntmax = 0;
    for( uint k = 0; k<nc; k++ )
      if( nt[k]>ntmax )
        ntmax = nt[k];
    const size_t floor_ = size_t(ms)*size_t(ntmax);
    size_t cnt[MRP_MAXCLASS];
    for( int cl = 0; cl<num_class; cl++ )
      cnt[cl] = 0;
    for( size_t i = 0; i<size_t(W)*H; i++ )
      cnt[cls[i]]++;
    // never empty the image: if everything is starved, the largest class
    // survives on its own
    int live[MRP_MAXCLASS], nlive = 0, big = 0;
    for( int cl = 0; cl<num_class; cl++ ) {
      if( cnt[cl]>cnt[big] )
        big = cl;
      if( cnt[cl]>=floor_ )
        live[nlive++] = cl;
    }
    if( nlive==num_class )
      return 0;
    if( nlive==0 ) {
      live[0] = big;
      nlive = 1;
    }
    // each dead class goes to the live one it already resembles, measured
    // on the coefficients rather than on position: two regions with the
    // same filter code the same whether or not they touch
    int map[MRP_MAXCLASS];
    for( int cl = 0; cl<num_class; cl++ )
      map[cl] = -1;
    for( int i = 0; i<nlive; i++ )
      map[live[i]] = i;
    for( int cl = 0; cl<num_class; cl++ ) {
      if( map[cl]>=0 )
        continue;
      double bd = 1e300;
      int bi = 0;
      for( int i = 0; i<nlive; i++ ) {
        double d = 0;
        for( uint k = 0; k<nc; k++ )
          for( int t = 0; t<nt[k]; t++ ) {
            double e = double(coef[cl][k][t]-coef[live[i]][k][t]);
            d += e*e;
          }
        if( d<bd ) {
          bd = d;
          bi = i;
        }
      }
      map[cl] = bi;
    }
    for( size_t i = 0; i<size_t(W)*H; i++ )
      cls[i] = byte(map[cls[i]]);
    // compact the per-class tables the same way, so that a merge does not
    // throw away the fit the survivors already have
    for( int i = 0; i<nlive; i++ ) {
      if( live[i]==i )
        continue;
      memcpy(coef[i], coef[live[i]], sizeof(coef[0]));
      memcpy(th[i], th[live[i]], sizeof(th[0]));
      memcpy(uq[i], uq[live[i]], sizeof(uq[0]));
    }
    num_class = nlive;
#ifdef MRP_OPENCL
    cl_up = 0; // the device's copy of all of that is now wrong
#endif
    return 1;
  }

  // ---------------------------------------------------------------
  // Coefficient search: move weight between two taps of one predictor,
  // keeping their sum, and take the best of the 11x3 shifts.  Trading
  // between taps rather than moving one at a time is what keeps the DC
  // gain of the predictor intact while it searches.
  void OptimizeCoef(int cl, uint k, int p1, int p2) {
    static const int SR = 11, SSR = 3;
    cost_t cbuf[SR*SSR];
    int* cf = coef[cl][k];
    cost_t side[SR*SSR];
    for( int i = 0, n = 0; i<SR; i++ ) {
      int y = cf[p1]+i-(SR>>1);
      if( y<0 )
        y = -y;
      if( y>mxc )
        y = mxc;
      for( int j = 0; j<SSR; j++ ) {
        int x = cf[p2]-(i-(SR>>1))-(j-(SSR>>1));
        if( x<0 )
          x = -x;
        if( x>mxc )
          x = mxc;
        side[n] = coef_cost[coef_m[k][p1]][y]+coef_cost[coef_m[k][p2]][x];
        cbuf[n++] = 0.0;
      }
    }
    // stride over the class's pixels; the sampled sum is scaled back up
    // before the side cost, which is in absolute bits, is added to it
    size_t cnt = cbeg[cl+1]-cbeg[cl];
    size_t st = 1;
    if( COEF_MAX&&cnt>COEF_MAX )
      st = cnt/COEF_MAX;
    int o1 = toff[k][p1], o2 = toff[k][p2];
#if defined(__AVX2__)
    // Five vectors cover the 11x3 candidate grid (the last seven lanes are
    // parked on candidate 0 so their gathers stay in range and their sums
    // are dropped).  Each pass is two gathers instead of eight dependent
    // loads into eight different sub-position tables.
    __m256 acc[5];
    for( int g = 0; g<5; g++ )
      acc[g] = _mm256_setzero_ps();
    ALIGN(32) float flush[40];
    for( int g = 0; g<5; g++ )
      for( int t = 0; t<8; t++ )
        flush[g*8+t] = 0.0f;
    uint since = 0;
    const __m256i vMAX = _mm256_set1_epi32(MAXPRD);
    const __m256i vZ = _mm256_setzero_si256();
    const __m256i vH = _mm256_set1_epi32(1<<(COEF_PREC-PM_ACC-1));
    const __m256i vCS = _mm256_set1_epi32(CSTRIDE);
    const __m256i vSUB = _mm256_set1_epi32(NUM_SUBPM-1);
    const __m256i vPS = _mm256_set1_epi32(PMSIZE);
    for( size_t ci = cbeg[cl], ce = cbeg[cl+1]; ci<ce; ci += st ) {
      {
        const byte* p = org+cpo[ci];
        const short* pr = prd+cpq[ci];
        const char* pg = grp+cpq[ci];
        int d1 = int(p[o1]), d2 = int(p[o2]);
        int pf0 = int(pr[k])-(d1-d2)*(SR>>1)+d2*(SSR>>1);
        const float* cb0 = pmlc[k][int(pg[k])];
        int v0 = int(p[k]);
        // the next sampled pixel lands in one of sixteen tables and near
        // the same base; ask for it, and for its pixel data, now
        if( ci+st<ce ) {
          const char* pgn = grp+cpq[ci+st];
          _mm_prefetch((const char*)(org+cpo[ci+st]), _MM_HINT_T0);
          _mm_prefetch((const char*)(pmlc[k][int(pgn[k])]+((MAXPRD-Clip(pf0))>>COEF_PREC)+v0), _MM_HINT_T0);
        }
        __m256i vpf = _mm256_set1_epi32(pf0);
        __m256i vdd = _mm256_set1_epi32(d1-d2);
        __m256i vd2 = _mm256_set1_epi32(d2);
        __m256i vv0 = _mm256_set1_epi32(v0);
        for( int g = 0; g<5; g++ ) {
          __m256i A = _mm256_load_si256((const __m256i*)(cA+g*8));
          __m256i B = _mm256_load_si256((const __m256i*)(cB+g*8));
          __m256i pf = _mm256_add_epi32(vpf, _mm256_sub_epi32(_mm256_mullo_epi32(A, vdd), _mm256_mullo_epi32(B, vd2)));
          pf = _mm256_max_epi32(_mm256_min_epi32(pf, vMAX), vZ);
          __m256i q = _mm256_srai_epi32(_mm256_add_epi32(_mm256_sub_epi32(vMAX, pf), vH), COEF_PREC-PM_ACC);
          __m256i base = _mm256_srai_epi32(q, PM_ACC);
          __m256i off = _mm256_mullo_epi32(_mm256_and_si256(q, vSUB), vCS);
          __m256i i1 = _mm256_add_epi32(off, _mm256_add_epi32(base, vv0));
          __m256i i2 = _mm256_add_epi32(off, _mm256_add_epi32(base, vPS));
#if MRP_GATHER
          __m256 c1 = _mm256_i32gather_ps(cb0, i1, 4);
          __m256 c2 = _mm256_i32gather_ps(cb0, i2, 4);
#else
          // AVX2's gather is a microcoded loop; eight ordinary loads off
          // SIMD-computed indices are usually the faster way to say it
          ALIGN(32) int x1i[8], x2i[8];
          ALIGN(32) float f1[8], f2[8];
          _mm256_store_si256((__m256i*)x1i, i1);
          _mm256_store_si256((__m256i*)x2i, i2);
          for( int t = 0; t<8; t++ ) {
            f1[t] = cb0[x1i[t]];
            f2[t] = cb0[x2i[t]];
          }
          __m256 c1 = _mm256_load_ps(f1);
          __m256 c2 = _mm256_load_ps(f2);
#endif
          acc[g] = _mm256_add_ps(acc[g], _mm256_add_ps(c1, c2));
        }
        // float carries 24 bits; drain into the doubles before it matters
        if( ++since>=4096 ) {
          for( int g = 0; g<5; g++ ) {
            _mm256_store_ps(flush+g*8, acc[g]);
            acc[g] = _mm256_setzero_ps();
            for( int t = 0; t<8; t++ )
              if( g*8+t<33 )
                cbuf[g*8+t] += flush[g*8+t];
          }
          since = 0;
        }
      }
    }
    for( int g = 0; g<5; g++ ) {
      _mm256_store_ps(flush+g*8, acc[g]);
      for( int t = 0; t<8; t++ )
        if( g*8+t<33 )
          cbuf[g*8+t] += flush[g*8+t];
    }
#else
    for( size_t ci = cbeg[cl], ce = cbeg[cl+1]; ci<ce; ci += st ) {
      {
        const byte* p = org+cpo[ci];
        const short* pr = prd+cpq[ci];
        const char* pg = grp+cpq[ci];
        int d1 = int(p[o1]), d2 = int(p[o2]);
        int pf = int(pr[k])-(d1-d2)*(SR>>1)+d2*(SSR>>1);
        const PMod* pmg = pml[k][int(pg[k])];
        int v0 = int(p[k]);
        cost_t* cb = cbuf;
        for( int i = 0; i<SR; i++ ) {
          for( int j = 0; j<SSR; j++ ) {
            int q = Qprd(Clip(pf));
            const PMod* pm = pmg+(q&(NUM_SUBPM-1));
            int base = q>>PM_ACC;
            (*cb++) += pm->cost[base+v0]+pm->subcost[base];
            pf -= d2;
          }
          pf += d1+d2*(SSR-1);
        }
      }
    }
#endif
    for( int i = 0; i<SR*SSR; i++ )
      cbuf[i] = cbuf[i]*cost_t(st)+side[i];
    int b = (SR*SSR)>>1;
    for( int i = 0; i<SR*SSR; i++ )
      if( cbuf[i]<cbuf[b] )
        b = i;
    int i = (b/SSR)-(SR>>1), j = (b%SSR)-(SSR>>1);
    int y = cf[p1]+i, x = cf[p2]-i-j;
    if( y<-mxc )
      y = -mxc;
    else if( y>mxc )
      y = mxc;
    if( x<-mxc )
      x = -mxc;
    else if( x>mxc )
      x = mxc;
    i = y-cf[p1];
    j = x-cf[p2];
    if( i==0&&j==0 )
      return;
    cf[p1] = y;
    cf[p2] = x;
    for( size_t ci = cbeg[cl], ce = cbeg[cl+1]; ci<ce; ci++ ) {
      const byte* p = org+cpo[ci];
      short* pr = prd+cpq[ci];
      short* pe = errB+cpo[ci];
      int v = Clip(int(pr[k])+int(p[o1])*i+int(p[o2])*j);
      pr[k] = short(v);
      pe[k] = short(Econv(int(p[k]), v));
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
    for( int i = 0; i<=num_class; i++ )
      cnt[i] = 0;
    for( size_t i = 0; i<size_t(W)*H; i++ )
      cnt[int(cls[i])+1]++;
    cbeg[0] = 0;
    for( int i = 0; i<num_class; i++ )
      cbeg[i+1] = cbeg[i]+cnt[i+1];
    for( int i = 0; i<=num_class; i++ )
      cnt[i] = cbeg[i];
    for( uint y = 0; y<H; y++ )
      for( uint x = 0; x<W; x++ ) {
        int c = int(cls[size_t(y)*W+x]);
        size_t j = cnt[c]++;
        cpo[j] = uint(OI(y, x));
        cpq[j] = uint(PI(y, x));
      }
  }

  cost_t OptimizePredictorHost() {
    BuildClassList();
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        int n = nt[k];
        if( n<2 )
          continue;
        int d = 1+int(optpass%uint(n-1));
        for( int t = 0; t<n; t++ )
          OptimizeCoef(cl, k, t, (t+d)%n);
      }
    optpass++;
    PredictRegion(0, 0, H, W);
    return CalcCost(0, 0, H, W);
  }

#ifdef MRP_OPENCL
  // ---------------------------------------------------------------
  // The coefficient sweep on the device.
  //
  // The host sweeps (class, component, tap pair) in that nesting.  Two
  // classes never touch the same pixel or the same coefficient, so the
  // class loop can be turned inside out and run as one launch per
  // (component, tap pair) without changing the sequence of shifts any
  // one class sees -- which is what makes this a port of the search and
  // not a different search.
  //
  // prd and errB live on the device for the length of the pass; the host
  // rebuilds them from the coefficients at the end of it anyway.
  int CoefPick(int cl, uint k, int p1, int p2, const double* scan, int* pdi, int* pdj) {
    static const int SR = 11, SSR = 3;
    cost_t cbuf[SR*SSR];
    int* cf = coef[cl][k];
    size_t cnt = cbeg[cl+1]-cbeg[cl];
    if( cnt==0 )
      return 0;
    size_t st = 1;
    if( COEF_MAX&&cnt>COEF_MAX )
      st = cnt/COEF_MAX;
    for( int i = 0, n = 0; i<SR; i++ ) {
      int y = cf[p1]+i-(SR>>1);
      if( y<0 )
        y = -y;
      if( y>mxc )
        y = mxc;
      for( int j = 0; j<SSR; j++ ) {
        int x = cf[p2]-(i-(SR>>1))-(j-(SSR>>1));
        if( x<0 )
          x = -x;
        if( x>mxc )
          x = mxc;
        cbuf[n] = scan[n]*cost_t(st)+coef_cost[coef_m[k][p1]][y]+coef_cost[coef_m[k][p2]][x];
        n++;
      }
    }
    int b = (SR*SSR)>>1;
    for( int i = 0; i<SR*SSR; i++ )
      if( cbuf[i]<cbuf[b] )
        b = i;
    int i = (b/SSR)-(SR>>1), j = (b%SSR)-(SSR>>1);
    int y = cf[p1]+i, x = cf[p2]-i-j;
    if( y<-mxc )
      y = -mxc;
    else if( y>mxc )
      y = mxc;
    if( x<-mxc )
      x = -mxc;
    else if( x>mxc )
      x = mxc;
    i = y-cf[p1];
    j = x-cf[p2];
    if( i==0&&j==0 )
      return 0;
    cf[p1] = y;
    cf[p2] = x;
    *pdi = i;
    *pdj = j;
    return 1;
  }

  int CLClassList() {
    BuildClassList();
    if( !s_cb32 )
      s_cb32 = new uint[MRP_MAXCLASS+1];
    uint* cb32 = s_cb32;
    for( int i = 0; i<=num_class; i++ )
      cb32[i] = uint(cbeg[i]);
    size_t npix = size_t(W)*H;
    return g_cl.Write(g_cl.d_cpo, cpo, npix*sizeof(uint), "cpo")&&
           g_cl.Write(g_cl.d_cpq, cpq, npix*sizeof(uint), "cpq")&&
           g_cl.Write(g_cl.d_cbeg, cb32, size_t(num_class+1)*sizeof(uint), "cbeg");
  }

  // the batched search reads the committed error planes on the device.
  // Rows [y0, y1) of them, which is what one row of blocks just changed;
  // y0 == y1 == 0 means all of it, which is how the pass starts.
  int CLClassState(uint y0 = 0, uint y1 = 0) {
    const size_t row = BS*nc*sizeof(short);
    if( y1<=y0 )
      return g_cl.Write(g_cl.d_err, errB, size_t(H+PADT)*row, "err");
    return g_cl.Write(g_cl.d_err, errB+size_t(y0+PADT)*BS*nc,
                      size_t(y1-y0)*row, "err", size_t(y0+PADT)*row);
  }

  int CLState(int with_grp) {
    size_t bn = size_t(H+PADT)*BS, npix = size_t(W)*H;
    return g_cl.Write(g_cl.d_org, org, bn*nc, "org")&&
           g_cl.Write(g_cl.d_prd, prd, npix*nc*sizeof(short), "prd")&&
           g_cl.Write(g_cl.d_err, errB, bn*nc*sizeof(short), "err")&&
           (!with_grp||g_cl.Write(g_cl.d_grp, grp, npix*nc, "grp"));
  }

  cost_t OptimizePredictorCL() {
    if( !CLClassList()||!CLState(1) )
      return OptimizePredictorHost();
    if( !s_scan )
      s_scan = new double[size_t(MRP_MAXCLASS)*CL_NCAND];
    double* scan = s_scan;
    for( uint k = 0; k<nc; k++ ) {
      int n = nt[k];
      if( n<2 )
        continue;
      int d = 1+int(optpass%uint(n-1));
      for( int t = 0; t<n; t++ ) {
        int p1 = t, p2 = (t+d)%n;
        int o1 = toff[k][p1], o2 = toff[k][p2];
        if( !g_cl.CoefScan(num_class, int(k), o1, o2, scan) )
          goto done;
        for( int cl = 0; cl<num_class; cl++ ) {
          int di = 0, dj = 0;
          if( CoefPick(cl, k, p1, p2, scan+size_t(cl)*CL_NCAND, &di, &dj) )
            if( !g_cl.CoefApply(uint(cbeg[cl]), uint(cbeg[cl+1]), int(k), o1, o2, di, dj) )
              goto done;
        }
      }
    }
  done:
    optpass++;
    PredictRegion(0, 0, H, W); // and the host has prd and errB again
    return CalcCost(0, 0, H, W);
  }

  // ---------------------------------------------------------------
  // The group quantiser's two passes.  The dynamic program between them
  // is 63*4 independent problems of 512 states -- small, serial and
  // cheap -- so it stays where it is; what moves is the two sweeps over
  // the image that feed it.
  cost_t OptimizeGroupCL() {
    size_t nu = MAX_UPARA+2;
    if( !s_cbuf )
      s_cbuf = new cost_t[size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*nu];
    cost_t* cbuf = s_cbuf;
    // the histogram lands straight in the buffer the host's DP reads
    if( !CLClassList()||!CLState(0)||!g_cl.GroupHist(num_class, cbuf) )
      return OptimizeGroupHost();
    if( g_cl.GroupDPCL(num_class, th_cost, &th[0][0][0]) ) {
      // the DP wrote the thresholds; the group table follows from them
      for( int cl = 0; cl<num_class; cl++ )
        for( uint k = 0; k<nc; k++ ) {
          int u = 0;
          for( int gr = 0; gr<MRP_GROUP; gr++ )
            for(; u<th[cl][k][gr]&&u<=MAX_UPARA; u++ )
              uq[cl][k][u] = char(gr);
          for(; u<=MAX_UPARA; u++ )
            uq[cl][k][u] = char(MRP_GROUP-1);
        }
    } else
      GroupDP(cbuf, nu);
    cost_t cost = CalcCost(0, 0, H, W);
    if( opt_loop>1&&num_pm>1 ) {
      if( !s_pc2 )
        s_pc2 = new double[size_t(MAXC)*MRP_GROUP*NUM_PMODEL];
      double* pc2 = s_pc2;
      size_t npix = size_t(W)*H;
      if( !g_cl.Write(g_cl.d_grp, grp, npix*nc, "grp")||
          !g_cl.PmodelCost(int(npix), num_pm, pc2) )
        return cost;
      cost = 0.0;
      for( uint k = 0; k<nc; k++ )
        for( int g = 0; g<MRP_GROUP; g++ ) {
          double* p = pc2+(size_t(k)*MRP_GROUP+g)*NUM_PMODEL;
          int best = 0;
          for( int i = 1; i<NUM_PMODEL; i++ )
            if( p[i]<p[best] )
              best = i;
          pm_idx[k][g] = best;
          pml[k][g] = pmodels+(size_t(g)*num_pm+best)*NUM_SUBPM;
          pmlc[k][g] = pml[k][g]->cost;
          cost += p[best];
        }
    }
    return cost;
  }
#endif

  cost_t OptimizeGroup() {
#ifdef MRP_OPENCL
    // no fp64 means no group kernels; do not upload for nothing
    if( MRP_CL_GROUP&&g_cl.active&&g_cl.k_hist&&CLSync(cl_up==0) ) {
      cl_up = 1;
      return OptimizeGroupCL();
    }
#endif
    return OptimizeGroupHost();
  }

  cost_t OptimizePredictor() {
#ifdef MRP_OPENCL
    if( MRP_CL_COEF&&g_cl.active&&CLSync(cl_up==0) ) {
      cl_up = 1;
      return OptimizePredictorCL();
    }
#endif
    return OptimizePredictorHost();
  }
};

// -------------------------------------------------------------
// Side information: cost tables first, because the optimiser above spends
// them, then the coders that make them true.
// -------------------------------------------------------------
static const int PMCLASS_LEVEL = 32;
static const int PMCLASS_MAX = 16;

struct MRPCIO : MRPC {
  SPMod sp;

  void SetCoefCost() {
    for( int m = 0; m<16; m++ ) {
      sp.Set(mxc+1, m);
      for( int c = 0; c<=mxc; c++ )
        coef_cost[m][c] = sp.Cost(c)+(c!=0 ? 1.0 : 0.0);
    }
    for( uint k = 0; k<nc; k++ )
      for( int i = 0; i<NTMAX; i++ )
        coef_m[k][i] = 8;
  }

  void DefaultSideCosts() {
    double lc = log(double(num_class))/log(2.0);
    for( int i = 0; i<MRP_MAXCLASS; i++ )
      class_cost[i] = lc;
    for( int i = 0; i<(QT_DEPTH<<3); i++ )
      qtflag_cost[i] = 1.0;
    sp.Set(MAX_UPARA+2, 8);
    for( int i = 0; i<MAX_UPARA+2; i++ )
      th_cost[i] = sp.Cost(i);
  }

  // --- predictors ---------------------------------------------------
  cost_t CodePredictor(int measure) {
    cost_t total = 0;
    for( uint k = 0; k<nc; k++ )
      for( int i = 0; i<nt[k]; i++ ) {
        cost_t mc = 1e30;
        int mm = 0;
        for( int m = 0; m<16; m++ ) {
          cost_t c = 0;
          for( int cl = 0; cl<num_class; cl++ ) {
            int v = coef[cl][k][i];
            if( v<0 )
              v = -v;
            c += coef_cost[m][v];
          }
          if( c<mc ) {
            mc = c;
            mm = m;
          }
        }
        coef_m[k][i] = mm;
        total += mc;
        if( !measure ) {
          SPMod q;
          q.Set(16, -1);
          EncSP(q, mm);
          sp.Set(mxc+1, mm);
          for( int cl = 0; cl<num_class; cl++ ) {
            int v = coef[cl][k][i], s = (v<0);
            if( v<0 )
              v = -v;
            EncSP(sp, v);
            if( v>0 )
              rc.rc_Process(uint(s), 1, 2);
          }
        }
      }
    return total;
  }

  void DecodePredictor() {
    for( uint k = 0; k<nc; k++ ) {
      for( int i = 0; i<nt[k]; i++ ) {
        SPMod q;
        q.Set(16, -1);
        int mm = DecSP(q);
        sp.Set(mxc+1, mm);
        for( int cl = 0; cl<num_class; cl++ ) {
          int v = DecSP(sp);
          if( v>0 ) {
            uint f = rc.rc_GetFreq(2);
            uint s = (f>=1);
            rc.rc_Process(s, 1, 2);
            if( s )
              v = -v;
          }
          coef[cl][k][i] = v;
        }
      }
      for( int i = nt[k]; i<NTMAX; i++ )
        for( int cl = 0; cl<num_class; cl++ )
          coef[cl][k][i] = 0;
    }
  }

  // --- group thresholds and the shape of each group -----------------
  cost_t CodeThreshold(int measure) {
    cost_t mc = 1e30;
    int mm = 0;
    for( int m = 0; m<16; m++ ) {
      sp.Set(MAX_UPARA+2, m);
      cost_t c = 0;
      for( int cl = 0; cl<num_class; cl++ )
        for( uint k = 0; k<nc; k++ ) {
          int prev = 0;
          for( int gr = 0; gr<MRP_GROUP-1; gr++ ) {
            if( prev>MAX_UPARA )
              break;
            c += sp.Cost(th[cl][k][gr]-prev);
            prev = th[cl][k][gr];
          }
        }
      if( c<mc ) {
        mc = c;
        mm = m;
      }
    }
    sp.Set(MAX_UPARA+2, mm);
    for( int i = 0; i<MAX_UPARA+2; i++ )
      th_cost[i] = sp.Cost(i);
    cost_t total = mc;
    if( num_pm>1 )
      total += cost_t(nc)*MRP_GROUP*4.0;
    if( !measure ) {
      SPMod q;
      q.Set(16, -1);
      EncSP(q, mm);
      for( int cl = 0; cl<num_class; cl++ )
        for( uint k = 0; k<nc; k++ ) {
          int prev = 0;
          for( int gr = 0; gr<MRP_GROUP-1; gr++ ) {
            if( prev>MAX_UPARA )
              break;
            EncSP(sp, th[cl][k][gr]-prev);
            prev = th[cl][k][gr];
          }
        }
      SPMod u;
      u.Set(NUM_PMODEL, -1);
      for( uint k = 0; k<nc; k++ )
        for( int gr = 0; gr<MRP_GROUP; gr++ )
          EncSP(u, pm_idx[k][gr]);
    }
    return total;
  }

  void DecodeThreshold() {
    SPMod q;
    q.Set(16, -1);
    int mm = DecSP(q);
    sp.Set(MAX_UPARA+2, mm);
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        int prev = 0;
        for( int gr = 0; gr<MRP_GROUP-1; gr++ ) {
          if( prev>MAX_UPARA ) {
            th[cl][k][gr] = prev;
            continue;
          }
          prev += DecSP(sp);
          th[cl][k][gr] = prev;
        }
        th[cl][k][MRP_GROUP-1] = MAX_UPARA+1;
        int u2 = 0;
        for( int gr = 0; gr<MRP_GROUP; gr++ )
          for(; u2<th[cl][k][gr]&&u2<=MAX_UPARA; u2++ )
            uq[cl][k][u2] = char(gr);
        for(; u2<=MAX_UPARA; u2++ )
          uq[cl][k][u2] = char(MRP_GROUP-1);
      }
    SPMod u;
    u.Set(NUM_PMODEL, -1);
    for( uint k = 0; k<nc; k++ )
      for( int gr = 0; gr<MRP_GROUP; gr++ )
        pm_idx[k][gr] = DecSP(u);
  }

  // --- class map ----------------------------------------------------
  int* qindex;
  uint* qhist;
  int qn;

  void QtIndex(uint tly, uint tlx, uint blksize, uint width, int level) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx||tly>=bry )
      return;
    if( level>0 ) {
      int ctx = QtCtx(level, tly, tlx, blksize, width);
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( qtmap[level-1][qy*qtw[level-1]+qx]==1 ) {
        qindex[qn++] = -(ctx+2);
        qtctx[ctx+1]++;
        uint half = blksize>>1;
        QtIndex(tly, tlx, half, width, level-1);
        QtIndex(tly, tlx+half, half, width, level-1);
        QtIndex(tly+half, tlx, half, width, level-1);
        QtIndex(tly+half, tlx+half, half, brx, level-1);
        return;
      }
      qindex[qn++] = -(ctx+1);
      qtctx[ctx]++;
    }
    MtfClass(tly, tlx, blksize, width);
    int i = mtfbuf[int(cls[size_t(tly)*W+tlx])];
    qindex[qn++] = i;
    qhist[i]++;
  }

  cost_t CodeClass(int measure) {
    int level = (opt_loop>1) ? QT_DEPTH : 0;
    uint blksize = (opt_loop>1) ? MAX_BSIZE : BASE_BSIZE;
    for( int k = 0; k<(QT_DEPTH<<3); k++ )
      qtctx[k] = 0;
    for( int i = 0; i<num_class; i++ ) {
      qhist[i] = 0;
      mtfbuf[i] = i;
    }
    qn = 0;
    for( uint y = 0; y<H; y += blksize )
      for( uint x = 0; x<W; x += blksize )
        QtIndex(y, x, blksize, W, level);

    // one of seven probabilities per quadtree context, and a quantised
    // log-probability per class label: both transmitted, so the cost model
    // the optimiser used and the coder that follows agree exactly
    if( level>0 )
      for( int ctx = 0; ctx<(QT_DEPTH<<2); ctx++ ) {
        cost_t best = 1e30;
        int bi = 0;
        for( int i = 0; i<7; i++ ) {
          double p = QTREE_PROB[i];
          cost_t c = -log(p)*cost_t(qtctx[(ctx<<1)+1])-log(1.0-p)*cost_t(qtctx[ctx<<1]);
          if( c<best ) {
            best = c;
            bi = i;
          }
        }
        qtree_code[ctx] = bi;
        double p = QTREE_PROB[bi];
        qtflag_cost[(ctx<<1)+1] = -log(p)/log(2.0);
        qtflag_cost[ctx<<1] = -log(1.0-p)/log(2.0);
      }
    int mtf_code[MRP_MAXCLASS];
    double tot = 0.0;
    for( int i = 0; i<num_class; i++ )
      tot += double(qhist[i]);
    if( tot<=0.0 )
      tot = 1.0;
    for( int i = 0; i<num_class; i++ ) {
      double p = double(qhist[i])/tot;
      int m;
      if( p>0.0 ) {
        m = int(-log(p)/log(2.0)*(PMCLASS_LEVEL/PMCLASS_MAX));
        if( m>=PMCLASS_LEVEL )
          m = PMCLASS_LEVEL-1;
      } else
        m = PMCLASS_LEVEL-1;
      mtf_code[i] = m;
      p = exp(-log(2.0)*((double)m+0.5)*PMCLASS_MAX/PMCLASS_LEVEL);
      class_cost[i] = -log(p)/log(2.0);
      qhist[i] = uint(p*(1<<10));
      if( qhist[i]==0 )
        qhist[i] = 1;
    }
    cost_t cost = 0;
    for( int j = 0; j<qn; j++ ) {
      int i = qindex[j];
      cost += (i<0) ? qtflag_cost[-(i+1)] : class_cost[i];
    }
    if( measure )
      return cost;

    SPMod q;
    if( level>0 ) {
      q.Set(7, -1);
      for( int ctx = 0; ctx<(QT_DEPTH<<2); ctx++ )
        EncSP(q, qtree_code[ctx]);
    }
    q.Set(PMCLASS_LEVEL, -1);
    for( int i = 0; i<num_class; i++ )
      EncSP(q, mtf_code[i]);
    ClassCode(0);
    return cost;
  }

  // Walk qindex and code it, or (decode==1) rebuild the class map.
  SPMod cpm;
  SPMod qtp[QT_DEPTH<<2];
  void BuildClassPM() {
    cpm.size = num_class;
    cpm.cumfreq[0] = 0;
    for( int i = 0; i<num_class; i++ ) {
      cpm.freq[i] = qhist[i];
      cpm.cumfreq[i+1] = cpm.cumfreq[i]+cpm.freq[i];
    }
    for( int ctx = 0; ctx<(QT_DEPTH<<2); ctx++ ) {
      double p = QTREE_PROB[qtree_code[ctx]];
      qtp[ctx].size = 2;
      qtp[ctx].freq[0] = uint((1.0-p)*(1<<10))+1;
      qtp[ctx].freq[1] = uint(p*(1<<10))+1;
      qtp[ctx].cumfreq[0] = 0;
      qtp[ctx].cumfreq[1] = qtp[ctx].freq[0];
      qtp[ctx].cumfreq[2] = qtp[ctx].freq[0]+qtp[ctx].freq[1];
    }
  }

  void ClassCode(int) {
    BuildClassPM();
    for( int j = 0; j<qn; j++ ) {
      int i = qindex[j];
      if( i<0 ) {
        int ctx = -(i+1);
        EncSP(qtp[ctx>>1], ctx&1);
      } else
        EncSP(cpm, i);
    }
  }

  void DecodeClassMap() {
    int level = QT_DEPTH;
    uint blksize = MAX_BSIZE;
    SPMod q;
    if( level>0 ) {
      q.Set(7, -1);
      for( int ctx = 0; ctx<(QT_DEPTH<<2); ctx++ )
        qtree_code[ctx] = DecSP(q);
    }
    q.Set(PMCLASS_LEVEL, -1);
    for( int i = 0; i<num_class; i++ ) {
      int m = DecSP(q);
      double p = exp(-log(2.0)*((double)m+0.5)*PMCLASS_MAX/PMCLASS_LEVEL);
      qhist[i] = uint(p*(1<<10));
      if( qhist[i]==0 )
        qhist[i] = 1;
    }
    BuildClassPM();
    for( int i = 0; i<num_class; i++ )
      mtfbuf[i] = i;
    for( int l = 0; l<QT_DEPTH; l++ )
      memset(qtmap[l], 0, size_t(qtw[l])*qth[l]);
    for( uint y = 0; y<H; y += blksize )
      for( uint x = 0; x<W; x += blksize )
        QtDecode(y, x, blksize, W, level);
  }

  void QtDecode(uint tly, uint tlx, uint blksize, uint width, int level) {
    uint brx = (tlx+blksize<W) ? tlx+blksize : W;
    uint bry = (tly+blksize<H) ? tly+blksize : H;
    if( tlx>=brx||tly>=bry )
      return;
    if( level>0 ) {
      int ctx = QtCtx(level, tly, tlx, blksize, width);
      int bit = DecSP(qtp[ctx>>1]);
      int qy = int((tly/MIN_BSIZE)>>level), qx = int((tlx/MIN_BSIZE)>>level);
      if( bit ) {
        qtmap[level-1][qy*qtw[level-1]+qx] = 1;
        uint half = blksize>>1;
        QtDecode(tly, tlx, half, width, level-1);
        QtDecode(tly, tlx+half, half, width, level-1);
        QtDecode(tly+half, tlx, half, width, level-1);
        QtDecode(tly+half, tlx+half, half, brx, level-1);
        return;
      }
      qtmap[level-1][qy*qtw[level-1]+qx] = 0;
    }
    MtfClass(tly, tlx, blksize, width);
    int i = DecSP(cpm);
    int cl = 0;
    for( int j = 0; j<num_class; j++ )
      if( mtfbuf[j]==i ) {
        cl = j;
        break;
      }
    for( uint y = tly; y<bry; y++ )
      for( uint x = tlx; x<brx; x++ )
        cls[size_t(y)*W+x] = byte(cl);
  }

  // --- the image ----------------------------------------------------
  // Encoder and decoder run the identical loop: prediction, activity and
  // group all come out of what is already coded, so the only difference is
  // which way the symbol crosses the coder.
  void ResetBorders() {
    for( uint y = 0; y<PADT; y++ ) {
      byte* p = org+size_t(y)*BS*nc;
      short* e = errB+size_t(y)*BS*nc;
      for( size_t i = 0; i<size_t(BS)*nc; i++ ) {
        p[i] = 128;
        e[i] = 0;
      }
    }
    for( uint y = 0; y<H; y++ ) {
      byte* p = org+OI(y, 0);
      short* e = errB+OI(y, 0);
      for( int j = 1; j<=PADL; j++ )
        for( uint k = 0; k<nc; k++ ) {
          p[-j*int(nc)+int(k)] = 128;
          e[-j*int(nc)+int(k)] = 0;
        }
      p = org+OI(y, W-1);
      e = errB+OI(y, W-1);
      for( int j = 1; j<=PADR; j++ )
        for( uint k = 0; k<nc; k++ ) {
          p[j*int(nc)+int(k)] = 128;
          e[j*int(nc)+int(k)] = 0;
        }
    }
  }

  // What CodeImage is doing this pass.  It is a template parameter rather
  // than an argument because three of these four modes are not the encoder
  // and the encoder should not be paying a branch a symbol for them: at
  // -O2 every `CI` test below folds away and each mode compiles to the
  // loop it actually needs.  The dispatcher underneath is the only place
  // the mode is a run-time value.
  //
  //   CI_ENC   read org, emit
  //   CI_DEC   read the stream, write org
  //   CI_COST  read org, emit nothing, total up the exact code length --
  //            which is what makes ChooseSse a trial rather than a guess
  //   CI_PLOT  encode, and record where every bit went
  enum { CI_ENC = 0, CI_DEC = 1, CI_COST = 2, CI_PLOT = 3 };

  static INLINE cost_t Bits(uint fr, uint tot) {
    return -log(double(fr)/double(tot))/log(2.0);
  }

  template<int CI>
  cost_t CodeImageT() {
    cost_t total = 0;
    ResetBorders();
    sse.est = (sse_on>>8)&15;
    sse.SetRule((sse_on>>4)&15);
    // the residual of the pixel to the left and of the one above, for the
    // neighbourhood contexts.  A row of the latter, one entry a component.
    int lastr[MAXC];
    short* rowr = new short[size_t(W)*nc];
    memset(rowr, 0, size_t(W)*nc*sizeof(short));
    uint f[SSEB], f0[SSEB], cum[SSEB+1];
    int a0[SSEB], a1[SSEB];
    for( uint y = 0; y<H; y++ ) {
      BorderLeft(y);
      byte* p = org+OI(y, 0);
      short* pe = errB+OI(y, 0);
      const byte* pc = cls+size_t(y)*W;
      for( uint k = 0; k<nc; k++ )
        lastr[k] = 0;
      for( uint x = 0; x<W; x++, p += nc, pe += nc ) {
        int cl = int(pc[x]);
        short* rr = rowr+size_t(x)*nc;
        for( uint k = 0; k<nc; k++ ) {
          int v = Clip(Predict(k, p, coef[cl][k]));
          int u = Activity(k, pe);
          int gr = int(uq[cl][k][u]);
          int q = Qprd(v), base = q>>PM_ACC;
          const PMod* pm = pml[k][gr]+(q&(NUM_SUBPM-1));
          int s;
          cost_t bits = 0;
          if( sse_on&15 ) {
            // the band first, against the corrected distribution, then
            // the symbol within it, against the static table untouched
            int sc = SseCtx(k, gr, q, lastr[k], int(rr[k]));
            sse.Bands(sc, pm, base, base+MAXVAL+1, f, f0, cum, a0, a1);
            int bb;
            if( CI==CI_DEC ) {
              uint vv = rc.rc_GetFreq(SSE_TOT);
              bb = 0;
              // bounded on the right as well: a truncated stream can hand
              // back a value the table does not cover
              while( bb<sse.nb-1&&cum[bb+1]<=vv )
                bb++;
              rc.rc_Process(cum[bb], f[bb], SSE_TOT);
              s = DecSym(pm, a0[bb], a1[bb]);
              p[k] = byte(s-base);
            } else {
              s = base+int(p[k]);
              bb = int(sse.bmap[s]);
              if( CI!=CI_COST ) {
                rc.rc_Process(cum[bb], f[bb], SSE_TOT);
                EncSym(pm, a0[bb], a1[bb], s);
              }
              if( CI==CI_COST||CI==CI_PLOT )
                bits = Bits(f[bb], uint(SSE_TOT))+
                       Bits(pm->freq[s], pm->cumfreq[a1[bb]]-pm->cumfreq[a0[bb]]);
            }
            sse.Update(sc, f, f0, bb);
          } else if( CI==CI_DEC ) {
            s = DecSym(pm, base, base+MAXVAL+1);
            p[k] = byte(s-base);
          } else {
            s = base+int(p[k]);
            if( CI!=CI_COST )
              EncSym(pm, base, base+MAXVAL+1, s);
            if( CI==CI_COST||CI==CI_PLOT )
              bits = Bits(pm->freq[s], pm->cumfreq[base+MAXVAL+1]-pm->cumfreq[base]);
          }
          if( CI==CI_COST )
            total += bits;
          if( CI==CI_PLOT ) {
            plot_bits += bits;
            // 4.4 fixed point, saturating: a symbol can cost more than
            // sixteen bits and the byte cannot say so.  The address is
            // the raster's, not the codec's -- same mapping StoreOrg
            // uses, so a transposed encode plots the right way up and
            // the components come back in the order they went in.
            byte* pl = plotbuf+(tflip ? size_t(x)*stride+size_t(y)*nc
                                      : size_t(y)*stride+size_t(x)*nc);
            int b16 = int(bits*16.0+0.5);
            pl[ord[k]] = byte(b16>255 ? 255 : b16);
          }
          pe[k] = short(Econv(int(p[k]), v));
          lastr[k] = int(p[k])-((v+(1<<(COEF_PREC-1)))>>COEF_PREC);
          rr[k] = short(lastr[k]);
        }
        if( y==0&&int(x)+PADR+1<int(W)+PADR )
          BorderTopCol(int(x)+PADR+1, int(x));
      }
      BorderRight(y);
    }
    delete[] rowr;
    return total;
  }

  // ... and the one place the mode is a value rather than a type.
  cost_t CodeImage(int ci) {
    switch( ci ) {
      case CI_DEC:  return CodeImageT<CI_DEC>();
      case CI_COST: return CodeImageT<CI_COST>();
      case CI_PLOT: return CodeImageT<CI_PLOT>();
      default:      return CodeImageT<CI_ENC>();
    }
  }

  // How wide the predictor coefficients are allowed to be.
  //
  // MRP clamps them at +-2.0 and the alphabet the coefficient coder works
  // over follows from that, so it is not free: widening it costs bits on
  // every tap of every class whether any of them needed the room or not.
  // On a photograph none do -- the fit lands well inside +-2 and the
  // clamp costs nothing.  On a single-component plane it is a different
  // problem: there are no cross-component taps to carry the level, the
  // twenty spatial taps have to do all of it, and if the plane is palette
  // indices rather than a picture the optimal filter has large cancelling
  // coefficients that +-2 throws away.  Measured, +-4 is worth -4.2% on
  // piap_0 and -3.0% on t24p_0, costs +0.2% on the greyscale tiles, and
  // costs +4.4% on a 24-bit one.
  //
  // So it is chosen per image and transmitted.  It would be better chosen
  // by trial, and it cannot be: the effect does not appear until the
  // second optimisation loop -- loop 1's cost is *identical* across the
  // three ranges on two of the four tiles above and ranks them backwards
  // on a third -- so there is no cheap proxy, and a real trial is three
  // full searches.  MRPC_CRANGE pins it for a measurement.
  void ChooseCoefRange() {
    crange = 0;
    if( nc==1 )
      crange = 1;
    if( const char* ce = getenv("MRPC_CRANGE") ) {
      int r = atoi(ce);
      crange = (r<0) ? 0 : (r>=NCRANGE ? NCRANGE-1 : r);
    }
    mxc = CRANGE[crange]<<COEF_PREC;
    SetCoefCost();
    FitPredictor(1);
    PredictRegion(0, 0, H, W);
    PROG("mrpc: coefficients +-%d\n", CRANGE[crange]);
  }

  // Which residual model this image gets.  Every candidate is one extra
  // pass of the coding loop, and the coding loop is a percent of an
  // encode -- the optimisation above it runs twenty iterations of three
  // searches over the same pixels -- so this is close to free, and it
  // ends the guessing: a small image with 61k symbols cannot afford the
  // contexts a 260k-symbol one pays for, and no rule from the pixel count
  // gets that right on a picture-by-picture basis.  0 is in the list, so
  // the trial cannot lose to the codec without the correction.
  void ChooseSse() {
    static const int CAND[] = {0, 0x001, 0x011, 0x013, 0x111, 0x113, 0x211};
    const char* e = getenv("MRPC_SSE");
    if( e ) { // a measurement pins it
      sse_on = SseDefault();
      return;
    }
    int best = 0;
    cost_t bc = 1e30;
    for( int i = 0; i<int(sizeof(CAND)/sizeof(CAND[0])); i++ ) {
      sse_on = CAND[i];
      cost_t c = CodeImage(CI_COST);
      PROG(" %.0f", c/8.0);
      if( c<bc ) {
        bc = c;
        best = CAND[i];
      }
    }
    sse_on = best;
  }
};

// -------------------------------------------------------------
// Allocation and the two drivers
// -------------------------------------------------------------
// -------------------------------------------------------------
// The codec.  One class, both directions, in memory: mrpc_compress and
// mrpc_decompress below are thin wrappers on Compress and Decompress
// here.  Init and Quit bracket the whole thing -- no constructors, no
// destructors, and nothing allocated that Quit does not give back.
// -------------------------------------------------------------
struct Codec : MRPCIO {
  byte* img;      // the raster, interleaved, stride bytes per row
  uint pixbytes;  // stride*H
  int own_img;    // 1 when it is ours to free: the decoder allocates one
  int geom;       // ImgInit has run
  int prog_req;   // what the caller asked for: 0 quiet, 1 if big, 2 always
  int class_req;  // and how many classes: 0 for whatever the image suggests
  int trial_flip; // and whether to encode both orientations and keep the
                  // smaller: two encodes, no decode cost
  int trial_vmap; // and whether to try the used-set value maps the same
                  // way.  Both together is four encodes.

  // How many predictor classes this image gets.  The caller's number wins;
  // -DMRP_CLASS pins it at build time; otherwise MRP's own rule, which is
  // linear in the pixel count.
  //
  // That rule is demonstrably not the optimum -- swept against the truth it
  // is wrong by up to 27%, in both directions, and the right number depends
  // on the picture rather than its size: two 192x192 crops with the same
  // pixel count want 45 classes and 6.  Three replacements were measured
  // over a corpus and every one of them was worse on average, because the
  // objective is bimodal (see TUNING.md) and any rule that moves the count
  // sometimes lands in the bad mode and loses 20%.  MRP's rule is not good
  // at predicting the optimum; it is good at staying out of trouble, and
  // that turns out to be the more valuable property.  -n is how you do
  // better on an image you care about.
  int ClassCount() const {
    if( class_req>0 )
      return (class_req>MRP_MAXCLASS) ? MRP_MAXCLASS : (class_req<2 ? 2 : class_req);
    int n = MRP_CLASS ? MRP_CLASS : int(10.4e-5*double(W)*double(H)+13.8);
    if( n>MRP_MAXCLASS )
      n = MRP_MAXCLASS;
    if( n<2 )
      n = 2;
    return n;
  }
  BC (*o1)[256];  // the order-1 model the head and tail bytes go through
  BC (*hdr)[256]; // and the one the stream's own header goes through

  // --- the context itself ------------------------------------------
  void Init() {
    memset(this, 0, sizeof(*this));
    o1 = (BC(*)[256]) new BC[256*256];
    hdr = (BC(*)[256]) new BC[8*256];
    sse.Init(SSE_NCTX);
  }

  void Quit() {
    ImgQuit();
    pmset.Quit();
    sse.Quit();
    delete[](BC*) o1;
    delete[](BC*) hdr;
    o1 = 0;
    hdr = 0;
    delete[] s_cbuf;
    delete[] s_cb32;
    delete[] s_scan;
    delete[] s_pc2;
    s_cbuf = 0;
    s_cb32 = 0;
    s_scan = 0;
    s_pc2 = 0;
#ifdef MRP_OPENCL
    g_cl.Quit();
#endif
  }

  void ResetModels() {
    for( int i = 0; i<256*256; i++ )
      ((BC*)o1)[i].Init();
    for( int i = 0; i<8*256; i++ )
      ((BC*)hdr)[i].Init();
  }

  // --- the buffers a given image needs -----------------------------
  void ImgInit(uint W_, uint H_, uint nc_, uint stride_) {
    W = W_;
    H = H_;
    nc = nc_;
    stride = stride_;
    BS = W+PADL+PADR;
    num_class = ClassCount();
    if( num_class>MRP_MAXCLASS )
      num_class = MRP_MAXCLASS;
    if( num_class<2 )
      num_class = 2;
    size_t bn = size_t(H+PADT)*BS*nc;
    org = new byte[bn];
    memset(org, 128, bn);
    errB = new short[bn];
    memset(errB, 0, bn*sizeof(short));
    prd = new short[size_t(W)*H*nc];
    upara = new short[size_t(W)*H*nc];
    grp = new char[size_t(W)*H*nc];
    memset(grp, 0, size_t(W)*H*nc);
    cls = new byte[size_t(W)*H];
    memset(cls, 0, size_t(W)*H);
    coef = new int[MRP_MAXCLASS][MAXC][NTMAX];
    memset(coef, 0, size_t(MRP_MAXCLASS)*MAXC*NTMAX*sizeof(int));
    uq = new char[MRP_MAXCLASS][MAXC][MAX_UPARA+1];
    memset(uq, 0, size_t(MRP_MAXCLASS)*MAXC*(MAX_UPARA+1));
    for( int l = 0; l<QT_DEPTH; l++ ) {
      qtw[l] = int(((W+MIN_BSIZE-1)/MIN_BSIZE)>>(l+1))+2;
      qth[l] = int(((H+MIN_BSIZE-1)/MIN_BSIZE)>>(l+1))+2;
      qtmap[l] = new char[size_t(qtw[l])*qth[l]];
      memset(qtmap[l], 0, size_t(qtw[l])*qth[l]);
    }
    size_t nidx = (size_t(H/MIN_BSIZE+2))*(W/MIN_BSIZE+2)*2+256;
    cpo = new uint[size_t(W)*H];
    cpq = new uint[size_t(W)*H];
    qindex = new int[nidx];
    qhist = new uint[MRP_MAXCLASS];
    prdbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    errbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    for( uint k = 0; k<nc; k++ ) {
      ord[k] = int(k);
      plane[k] = 0;
    }
    rnd = MRP_SEED;
    optpass = 0;
    for( int i = 0; i<40; i++ ) {
      cA[i] = (i<33) ? i/3 : 0;
      cB[i] = (i<33) ? i%3 : 0;
    }
    opt_loop = 1;
    BuildTaps();
    geom = 1;
  }

  void ImgQuit() {
    if( !geom )
      return;
    delete[] org;
    delete[] errB;
    delete[] prd;
    delete[] upara;
    delete[] grp;
    delete[] cls;
    delete[] coef;
    delete[] uq;
    for( int l = 0; l<QT_DEPTH; l++ )
      delete[] qtmap[l];
    delete[] cpo;
    delete[] cpq;
    delete[] qindex;
    delete[] qhist;
    delete[] prdbuf;
    delete[] errbuf;
    for( uint k = 0; k<nc; k++ )
      delete[] plane[k];
    if( own_img )
      free(img);
    img = 0;
    own_img = 0;
    org = 0;
    errB = 0;
    prd = 0;
    upara = 0;
    grp = 0;
    cls = 0;
    coef = 0;
    uq = 0;
    cpo = 0;
    cpq = 0;
    qindex = 0;
    qhist = 0;
    prdbuf = 0;
    errbuf = 0;
    for( int k = 0; k<MAXC; k++ )
      plane[k] = 0;
    for( int l = 0; l<QT_DEPTH; l++ )
      qtmap[l] = 0;
    geom = 0;
  }

  // the stored raster -> the bordered buffer, in coding order
  // -----------------------------------------------------------------
  // Value maps
  // -----------------------------------------------------------------
  // MRP's probability model is a generalized Gaussian over the integers,
  // and its resolution is one level.  That is fine while a plane uses its
  // levels; it is very much not fine when the plane is four bits widened
  // to eight.  Every second, or sixteenth, or seventeenth residual is then
  // the only one that can occur, and the model has no way to say so: it
  // spreads its mass over all of them and pays log2 of the spacing on
  // every symbol.  Measured, a 24bpp image quantised to four bits a
  // component costs 34,991 bytes coded as it stands and 10,666 with the
  // levels closed up -- and the two forms this takes in the wild, v*16 and
  // v*17, are equally bad, so a mask of the constant bits catches only
  // half of them.
  //
  // So every raster component goes through a bijection on the way in and
  // its inverse on the way out, chosen by the encoder and transmitted:
  //
  //   0  identity
  //   1  the scale: (v - lo%g) / g, where g is the gcd of the gaps.  A
  //      change of units, so the predictor is unchanged in shape and the
  //      gain is exactly log2(g).  This is the free one, and it is on by
  //      default.
  //   2  the used set: v -> its rank among the values that occur.  Not
  //      linear, so it can warp what a linear predictor sees, but it
  //      closes up a plane whose levels are near a lattice without being
  //      on one -- quantised imagery, mostly -- which 1 cannot.  Worth a
  //      great deal where it works and expensive where it does not, with
  //      nothing measurable to tell the two apart, so it is reached by a
  //      trial rather than a rule: see CompressTrial.
  void SetValueMap(uint c) {
    for( int i = 0; i<256; i++ )
      vinv[c][i] = 0;
    if( vkind[c]==2 ) {
      int j = 0;
      for( int i = 0; i<256; i++ ) {
        vmap[c][i] = byte(j); // an unused value maps somewhere defined
        if( vused[c][i] ) {
          vinv[c][j] = byte(i);
          j++;
        }
      }
      return;
    }
    const int lo = (vkind[c]==1) ? vlo[c] : 0;
    const int g = (vkind[c]==1) ? vgs[c] : 1;
    for( int i = 0; i<256; i++ ) {
      int v = (i-lo)/g;
      vmap[c][i] = byte(v<0 ? 0 : (v>255 ? 255 : v));
    }
    for( int v = 0; v<256; v++ ) {
      int i = lo+v*g;
      vinv[c][v] = byte(i>255 ? 255 : i);
    }
  }

  void SetValueMaps() {
    for( uint c = 0; c<nc; c++ )
      SetValueMap(c);
  }

  void BuildValueMaps() {
    if( const char* e = getenv("MRPC_VMAP") ) // pins it for a measurement
      vmode = atoi(e);
    for( int c = 0; c<MAXC; c++ ) {
      vkind[c] = 0;
      vlo[c] = 0;
      vgs[c] = 1;
    }
    for( uint c = 0; c<nc; c++ ) {
      for( int i = 0; i<256; i++ )
        vused[c][i] = 0;
      for( uint y = 0; y<ih; y++ ) {
        const byte* r = img+size_t(y)*stride+c;
        for( uint x = 0; x<iw; x++, r += nc )
          vused[c][r[0]] = 1;
      }
      int lo = -1, hi = -1, n = 0;
      for( int i = 0; i<256; i++ )
        if( vused[c][i] ) {
          if( lo<0 )
            lo = i;
          hi = i;
          n++;
        }
      if( n<=1||vmode<1 ) { // a constant plane has nothing to close up
        SetValueMap(c);
        continue;
      }
      int g = 0;
      for( int i = lo; i<=hi; i++ )
        if( vused[c][i] ) {
          int a = g, b = i-lo;
          while( b ) {
            int t = a%b;
            a = b;
            b = t;
          }
          g = a;
        }
      if( g<1 )
        g = 1;
      // Only the scale, never the offset.  Dividing by g is a change of
      // units and the predictor does not notice; subtracting lo is not,
      // because 128 is written into the borders and into org before the
      // first pixel is read, and the clamp on a prediction is [0,255].
      // Measured, shifting a plane that had nothing to gain from it cost
      // 2.8% on piag_0 and 12.9% on t24p_1, so the offset is now only the
      // remainder that makes the division exact.
      vlo[c] = lo%g;
      vgs[c] = g;
      vkind[c] = (g>1) ? 1 : 0;
      // And the one that is not a change of units: rank among the values
      // that occur.  It closes up a plane whose levels are near a lattice
      // without being on one -- quantised imagery, mostly -- and it is
      // worth a great deal there, -27% to -32% on the pia tiles.  It is
      // also not free: it warps what a linear predictor sees, and on two
      // tiles of the corpus it costs 9% and 14%.  Nothing measurable
      // separates those from the fifteen it wins on, so it is not a rule
      // the encoder can apply on its own -- vmode 2 is reached by the
      // trial in CompressTrial, which encodes both ways and keeps the
      // smaller.  The test below only asks whether the plane's values have
      // holes in them at all: a contiguous run has nothing for this to
      // close up, and re-indexing it would only be the offset that rule 1
      // just declined to take.
      if( vmode>=2&&n<hi-lo+1 )
        vkind[c] = 2;
      SetValueMap(c);
    }
  }

  // The maps travel with the stream: kind, then what it needs.
  void CodeValueMaps(int dec) {
    SPMod u;
    u.Set(3, -1);
    for( uint c = 0; c<nc; c++ ) {
      if( dec )
        vkind[c] = DecSP(u);
      else
        EncSP(u, vkind[c]);
      if( vkind[c]==1 ) {
        vlo[c] = int(CodeBits(hdr[4], uint(vlo[c]), 8));
        vgs[c] = int(CodeBits(hdr[5], uint(vgs[c]), 8));
        if( vgs[c]<1 )
          vgs[c] = 1;
      } else if( vkind[c]==2 ) {
        // 256 bits, under a context of what the last one was: a lattice
        // is a run pattern and costs almost nothing to say
        int prev = 0;
        for( int i = 0; i<256; i++ ) {
          uint b = CodeBits(hdr[6]+prev*2, uint(vused[c][i]), 1);
          vused[c][i] = byte(b);
          prev = int(b);
        }
      }
    }
    SetValueMaps();
  }

  void LoadOrg() {
    for( uint y = 0; y<H; y++ ) {
      byte* d = org+OI(y, 0);
      if( tflip ) {
        // column y of the raster becomes row y of the image the codec
        // sees.  No second buffer: the gather is the transpose.
        const byte* s = img+size_t(y)*nc;
        for( uint x = 0; x<W; x++, s += stride, d += nc )
          for( uint k = 0; k<nc; k++ )
            d[k] = vmap[ord[k]][s[ord[k]]];
      } else {
        const byte* s = img+size_t(y)*stride;
        for( uint x = 0; x<W; x++, s += nc, d += nc )
          for( uint k = 0; k<nc; k++ )
            d[k] = vmap[ord[k]][s[ord[k]]];
      }
    }
    FillBorders();
    BuildPlanes();
  }

  void StoreOrg() {
    memset(img, 0, pixbytes);
    for( uint y = 0; y<H; y++ ) {
      const byte* s = org+OI(y, 0);
      if( tflip ) {
        byte* d = img+size_t(y)*nc;
        for( uint x = 0; x<W; x++, s += nc, d += stride )
          for( uint k = 0; k<nc; k++ )
            d[ord[k]] = vinv[ord[k]][s[k]];
      } else {
        byte* d = img+size_t(y)*stride;
        for( uint x = 0; x<W; x++, s += nc, d += nc )
          for( uint k = 0; k<nc; k++ )
            d[ord[k]] = vinv[ord[k]][s[k]];
      }
    }
  }

  // The class map in the raster's geometry: one byte a pixel, the index
  // of the class whose predictor and quantiser coded it.  The same
  // address mapping StoreOrg uses, so a transposed encode comes back the
  // right way up, and the row is `iw` wide because that is the raster's
  // width whichever way round the codec ran.
  void StoreClassMap() {
    for( uint y = 0; y<H; y++ ) {
      const byte* s = cls+size_t(y)*W;
      if( tflip )
        for( uint x = 0; x<W; x++ )
          qmapbuf[size_t(x)*iw+y] = s[x];
      else
        memcpy(qmapbuf+size_t(y)*iw, s, W);
    }
  }

  void SetPmodels() {
    for( uint k = 0; k<nc; k++ )
      for( int g = 0; g<MRP_GROUP; g++ ) {
        pml[k][g] = pmodels+(size_t(g)*num_pm+pm_idx[k][g])*NUM_SUBPM;
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
    memset(cls, 0, size_t(W)*H);
    int save = num_class;
    num_class = 1;
    FitPredictor(1);
    cost_t c = 0;
    for( int b = 0; b<tnb; b++ ) {
      PredictRegion(tb0[b], 0, tb1[b], W);
      c += CalcCost(tb0[b], 0, tb1[b], W);
    }
    num_class = save;
    return c;
  }


  // ---------------------------------------------------------------
  // How many classes.  MRP's rule was linear in the pixel count, and
  // measured against a sweep it is wrong by up to 27% -- because the
  // right number depends on the picture and not on its size.  Two 192x192
  // crops with the same pixel count want 45 classes and 6.
  //
  // So it is measured rather than predicted.  One iteration of the first
  // loop at each candidate, and the whole file's cost -- side information
  // included, which is what pays for a class -- decides.  Against a full
  // sweep of seven counts on five images that picks the sweep's own
  // winner four times out of five, and loses 0.39% on the fifth.
  //
  // It costs one loop-1 iteration per candidate.  -n skips it entirely.
  cost_t TrialClass(int n) {
    num_class = n;
    InitClass();
    opt_loop = 1;
    DesignPredictor(1);
    OptimizeGroup();
    OptimizeClass();
    // and one pass of the second loop, which is the one that matters here:
    // the first loop segments into flat 8x8 blocks, so it cannot see what a
    // quadtree does with a large class count, and a trial that stops after
    // it ranks every count within a percent of every other and then picks
    // the smallest.  Measured, that is the difference between a trial that
    // agrees with a full sweep and one that does not.
    opt_loop = 2;
#if OPT_PRED
    OptimizePredictor();
#endif
    cost_t side = CodePredictor(1);
    OptimizeGroup();
    side += CodeThreshold(1);
    cost_t c = OptimizeClass();
    side += CodeClass(1);
    return c+side;
  }

  void ChooseClass() {
    static const int CAND[] = {6, 10, 16, 24, 34, 45, 63};
    int best = 0;
    cost_t bc = 1e30;
    for( int i = 0; i<int(sizeof(CAND)/sizeof(CAND[0])); i++ ) {
      int n = CAND[i];
      if( n>MRP_MAXCLASS )
        n = MRP_MAXCLASS;
      if( i&&n<=best )
        continue; // the cap folded this one onto the last
      cost_t c = TrialClass(n);
      PROG(" %d:%.0f", n, c/8.0);
      if( c<bc ) {
        bc = c;
        best = n;
      }
    }
    num_class = best;
  }

  // Bands for the trial: a fixed pixel budget spread evenly down the
  // image, so a wide picture is sampled in more places rather than in one
  // deeper stripe.
  void SetTrialBands() {
    uint rows = uint(TRIAL_PIX/(W ? W : 1));
    if( rows<16 )
      rows = 16;
    if( rows>=H ) {
      tnb = 1;
      tb0[0] = 0;
      tb1[0] = H;
      return;
    }
    uint nb = TRIAL_BANDS, bh = rows/nb;
    if( bh<8 ) {
      bh = 8;
      nb = rows/bh;
      if( nb<1 )
        nb = 1;
    }
    tnb = int(nb);
    for( uint b = 0; b<nb; b++ ) {
      uint y0 = uint((double(H-bh)*double(b))/double(nb>1 ? nb-1 : 1));
      tb0[b] = y0;
      tb1[b] = y0+bh;
    }
  }

  void ChooseOrder() {
    SetTrialBands();
#if MRP_ORD==1
    if( nc>=2 ) {
      ord[0] = 1;
      ord[1] = 0;
    }
#elif MRP_ORD==2
    int perm[MAXC], best[MAXC];
    for( uint k = 0; k<nc; k++ )
      perm[k] = int(k), best[k] = int(k);
    cost_t bc = 1e30;
    int idx[MAXC];
    for( uint k = 0; k<nc; k++ )
      idx[k] = 0;
    // straightforward permutation walk; nc is 3 or 4
    int n = int(nc), c_[MAXC] = {0, 0, 0, 0}, i = 0;
    for(;; ) {
      for( int k = 0; k<n; k++ )
        ord[k] = perm[k];
      cost_t cc = TrialOrder();
      PROG(".");
      if( cc<bc ) {
        bc = cc;
        for( int k = 0; k<n; k++ )
          best[k] = perm[k];
      }
      while( i<n ) { // Heap's algorithm
        if( c_[i]<i ) {
          int a = (i&1) ? c_[i] : 0;
          int t = perm[a];
          perm[a] = perm[i];
          perm[i] = t;
          c_[i]++;
          i = 0;
          break;
        } else {
          c_[i] = 0;
          i++;
        }
      }
      if( i>=n )
        break;
    }
    (void)idx;
    for( int k = 0; k<n; k++ )
      ord[k] = best[k];
#endif
    tnb = 0; // back to the whole image
    LoadOrg();
  }

  // --- encoder ---------------------------------------------------
  void CodeParams(int dec) {
    if( dec )
      sse_on = int(CodeVar(0));
    else
      CodeVar(uint(sse_on));
    SPMod u;
    u.Set(NCRANGE, -1);
    if( dec )
      crange = DecSP(u);
    else
      EncSP(u, crange);
    mxc = CRANGE[crange]<<COEF_PREC;
    SetCoefCost();
    u.Set(MRP_MAXCLASS+1, -1);
    if( dec )
      num_class = DecSP(u);
    else
      EncSP(u, num_class);
    u.Set(int(nc), -1);
    for( uint k = 0; k<nc; k++ ) {
      if( dec )
        ord[k] = DecSP(u);
      else
        EncSP(u, ord[k]);
    }
    CodeValueMaps(dec);
  }

  void Encode() {
    // the widest clamp until ChooseCoefRange narrows it: the component
    // order trial below fits predictors too, and a clamp of zero would
    // give it nothing to choose between
    crange = NCRANGE-1;
    mxc = MAX_COEF;
    // before anything reads the raster: LoadOrg goes through these
    BuildValueMaps();
    num_pm = NUM_PMODEL;
    pmset.Init(num_pm, 0);
    pmset.Costs();
    pmodels = pmset.pm;
    for( uint k = 0; k<nc; k++ )
      for( int g = 0; g<MRP_GROUP; g++ )
        pm_idx[k][g] = NUM_SHAPE>>1; // the middle shape, no extra tail
    SetPmodels();
    SetCoefCost();
    DefaultSideCosts();

    PROG("mrpc: %ux%ux%u  %d classes  taps %d+%d*%d%s\n", W, H, nc, num_class, PRD_ORDER, int(nc)-1, XPRD_ORDER, XCUR ? "+cur" : "");
#ifdef MRP_OPENCL
    if( CLInit() ) {
      char d[1024];
      g_cl.Describe(d, sizeof(d));
      PROG("mrpc: opencl: %s\n", d);
    } else if( g_clopt.use )
      fprintf(stderr, "mrpc: opencl: unavailable, running on the host\n");
#endif
    double t0 = tnow();
    PROG("mrpc: order trial ");
    ChooseOrder();
    PROG(" order");
    for( uint k = 0; k<nc; k++ )
      PROG(" %d", ord[k]);
    PROG("  (%.1fs)\n", tnow()-t0);
    PROG("mrpc: -DMRP_CLASS=n and -DMAX_ITER=n are the time dials;"
         " both searches are linear in the class count\n");
    if( CLASS_TRIAL&&!class_req&&!MRP_CLASS&&W*H>=4096 ) {
      double tc = tnow();
      PROG("mrpc: class trial");
      ChooseClass();
      PROG("  -> %d classes  (%.1fs)\n", num_class, tnow()-tc);
    }
    InitClass();
    ChooseCoefRange();

    // saved best of each loop
    byte* cls_s = new byte[size_t(W)*H];
    int (*coef_s)[MAXC][NTMAX] = new int[MRP_MAXCLASS][MAXC][NTMAX];
    static int th_s[MRP_MAXCLASS][MAXC][MRP_GROUP];
    int pm_s[MAXC][MRP_GROUP];
    int ncl_s = num_class;
    // the quadtree travels with the class map it describes: restoring one
    // without the other emits flags for a segmentation that is no longer
    // there, and the decoder fills blocks the encoder never coded
    char* qt_s[QT_DEPTH];
    for( int l = 0; l<QT_DEPTH; l++ )
      qt_s[l] = new char[size_t(qtw[l])*qth[l]];
#define SAVE()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
        do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          ncl_s = num_class;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
          memcpy(cls_s, cls, size_t(W)*H);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          memcpy(coef_s, coef, size_t(num_class)*MAXC*NTMAX*sizeof(int));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
          memcpy(th_s, th, sizeof(th));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
          memcpy(pm_s, pm_idx, sizeof(pm_idx));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
          for( int l = 0; l<QT_DEPTH; l++ )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
          memcpy(qt_s[l], qtmap[l], size_t(qtw[l])*qth[l]);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
        } while( 0 )
#define LOAD()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
        do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          num_class = ncl_s;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
          memcpy(cls, cls_s, size_t(W)*H);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          memcpy(coef, coef_s, size_t(num_class)*MAXC*NTMAX*sizeof(int));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
          memcpy(th, th_s, sizeof(th));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
          memcpy(pm_idx, pm_s, sizeof(pm_idx));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
          for( int l = 0; l<QT_DEPTH; l++ )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
          memcpy(qtmap[l], qt_s[l], size_t(qtw[l])*qth[l]);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
          DefaultSideCosts();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
          SetPmodels();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
          RebuildUq();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
        } while( 0 )

    cost_t best = 1e30;
    int last = 0;
    opt_loop = 1;
    for( int it = 0; it<MAX_ITER; it++ ) {
      double ta = tnow();
      PROG("mrpc: [1:%2d] fit", it);
      cost_t c = DesignPredictor(it==0);
      PROG(" %.0fs group", tnow()-ta);
      double tb = tnow();
      c = OptimizeGroup();
      PROG(" %.0fs class", tnow()-tb);
      tb = tnow();
      c = OptimizeClass();
      PROG(" %.0fs = %.0f B  [%.0fs]%s\n", tnow()-tb, c/8.0, tnow()-t_start, (c<best) ? " *" : "");
      // an iteration only resets the patience counter if it bought
      // something worth the minute it cost
      if( c<best*(1.0-1.0/MIN_GAIN) )
        last = it;
      if( c<best ) {
        best = c;
        SAVE();
      }
      // and only then shed starved classes, so that the state before a
      // merge is already in the snapshot: if the smaller model does not
      // earn its place over the next few iterations, LOAD puts the larger
      // one back and the merge has cost nothing but time
      if( MergeStarved() ) {
        DefaultSideCosts();
        PredictRegion(0, 0, H, W);
        CalcCost(0, 0, H, W);
        last = it; // give the smaller model the same patience
      }
      if( it-last>=EXTRA_ITER )
        break;
    }
    LOAD();
    PredictRegion(0, 0, H, W);
    CalcCost(0, 0, H, W);

    opt_loop = 2;
    best = 1e30;
    last = 0;
    for( int it = 0; it<MAX_ITER; it++ ) {
      cost_t c = 0;
      double tb = tnow();
      PROG("mrpc: [2:%2d] coef", it);
#if OPT_PRED
      c = OptimizePredictor();
#endif
      PROG(" %.0fs group", tnow()-tb);
      tb = tnow();
      cost_t side = CodePredictor(1);
      c = OptimizeGroup();
      side += CodeThreshold(1);
      PROG(" %.0fs class", tnow()-tb);
      tb = tnow();
      c = OptimizeClass();
      side += CodeClass(1);
      c += side;
      PROG(" %.0fs = %.0f + %.0f side B  [%.0fs]%s\n", tnow()-tb, (c-side)/8.0, side/8.0, tnow()-t_start, (c<best) ? " *" : "");
      if( c<best*(1.0-1.0/MIN_GAIN) )
        last = it;
      if( c<best ) {
        best = c;
        SAVE();
      }
      if( MergeStarved() ) {
        DefaultSideCosts();
        PredictRegion(0, 0, H, W);
        CalcCost(0, 0, H, W);
        last = it;
      }
#if OPT_PRED
      if( it-last>=EXTRA_ITER )
        break;
#else
      if( it>last )
        break;
#endif
    }
    LOAD();
    PredictRegion(0, 0, H, W);
    CalcCost(0, 0, H, W);
    CodeClass(1); // refresh qindex and the costs
#undef SAVE
#undef LOAD
    delete[] cls_s;
    delete[] coef_s;
    for( int l = 0; l<QT_DEPTH; l++ )
      delete[] qt_s[l];

#if MRP_VERBOSE
    Dump("enc");
#endif
    // --- write it out
    PROG("mrpc: writing");
    // the last thing decided and the first thing written: which residual
    // model this image gets, costed rather than guessed
    PROG(" model");
    ChooseSse();
    PROG(" %#x", sse_on);
    // CodeParams goes here rather than before the search, because the
    // class count is not settled until the search stops -- MergeStarved
    // can still be shedding classes on the last iteration
    CodeParams(0);
    CodeClass(0);
    CodePredictor(0);
    CodeThreshold(0);
    CodeImage(plotbuf ? CI_PLOT : CI_ENC);
    // one branch an image rather than one a symbol: unlike a code length,
    // a class is known before the coding loop starts and does not have to
    // be picked out of it
    if( qmapbuf )
      StoreClassMap();
    PROG(" ... done  [%.0fs]\n", tnow()-t_start);
#ifdef MRP_OPENCL
    g_cl.Report(stderr);
#endif
  }

#if MRP_VERBOSE
  void Dump(const char* tag) {
    unsigned long hc = 0, hp = 0, ht = 0, hm = 0;
    for( size_t i = 0; i<size_t(W)*H; i++ )
      hc = hc*1000003+cls[i];
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        for( int i = 0; i<NTMAX; i++ )
          hp = hp*1000003+(unsigned)(coef[cl][k][i]+1000);
        for( int g = 0; g<MRP_GROUP; g++ )
          ht = ht*1000003+(unsigned)th[cl][k][g];
      }
    for( uint k = 0; k<nc; k++ )
      for( int g = 0; g<MRP_GROUP; g++ )
        hm = hm*1000003+(unsigned)pm_idx[k][g];
    fprintf(stderr, "%s: nc=%u ncls=%d ord=%d%d%d%d cls=%lx prd=%lx th=%lx pm=%lx\n", tag, nc, num_class, ord[0], ord[1], ord[2], nc>3 ? ord[3] : 0, hc&0xffffffff, hp&0xffffffff, ht&0xffffffff, hm&0xffffffff);
  }

#endif
  void RebuildUq() {
    for( int cl = 0; cl<num_class; cl++ )
      for( uint k = 0; k<nc; k++ ) {
        int u = 0;
        for( int gr = 0; gr<MRP_GROUP; gr++ )
          for(; u<th[cl][k][gr]&&u<=MAX_UPARA; u++ )
            uq[cl][k][u] = char(gr);
        for(; u<=MAX_UPARA; u++ )
          uq[cl][k][u] = char(MRP_GROUP-1);
      }
  }

  // --- decoder ---------------------------------------------------
  void Decode() {
    num_pm = NUM_PMODEL;
    pmset.Init(num_pm, 0);
    pmodels = pmset.pm;
    opt_loop = 2;
    CodeParams(1);
    DecodeClassMap();
    DecodePredictor();
    DecodeThreshold();
    SetPmodels();
#if MRP_VERBOSE
    Dump("dec");
#endif
    CodeImage(CI_DEC);
  }

  // --- the stream's own header --------------------------------------
  // The geometry has to come out of the stream before there is anything
  // allocated to hold it, so it goes first, a byte at a time through its
  // own contexts.  CodeBits ignores the value it is given when decoding
  // and returns what it read, so one routine does both directions --
  // which is the property that keeps an encoder and a decoder in step.
  uint CodeU8(uint v) {
    return CodeBits(hdr[0], v&255, 8);
  }

  // seven bits at a time, high bit for "there is more": a 37 pixel wide
  // image should not pay four bytes to say so
  uint CodeVar(uint v) {
    uint r = 0, sh = 0;
    for( int i = 0; i<5; i++ ) {
      uint more = (v>>((i+1)*7)) ? 128 : 0;
      uint b = CodeBits(hdr[(i<3) ? i : 3], ((v>>(i*7))&127)|more, 8);
      r |= (b&127)<<sh;
      sh += 7;
      if( !(b&128) )
        break;
    }
    return r;
  }

  // head and tail: whatever the caller wrapped the image in
  void CodeBytes(const byte* src, byte* dst, size_t n) {
    uint last = 0;
    for( size_t i = 0; i<n; i++ ) {
      uint c = CodeBits(o1[last], src ? src[i] : 0, 8);
      if( dst )
        dst[i] = byte(c);
      last = c;
    }
  }

  // the row padding is not part of the raster and is rarely zero
  void CodePad(int dec) {
    if( stride<=iw*nc )
      return;
    for( uint y = 0; y<ih; y++ )
      for( uint x = iw*nc; x<stride; x++ ) {
        byte* q = img+size_t(y)*stride+x;
        uint v = CodeBits(o1[2], dec ? 0 : uint(*q), 8);
        if( dec )
          *q = byte(v);
      }
  }

  // --- what the C API calls -----------------------------------------
  int Compress(const mrpc_image* im, const void* head, size_t headlen,
               const void* tail, size_t taillen, mrpc_blob* out) {
    if( !out )
      return MRPC_ERR_ARG;
    out->data = 0;
    out->size = 0;
    if( im&&(!im->data||im->width==0||im->height==0||
             im->ncomp<1||im->ncomp>MAXC||im->stride<im->width*im->ncomp) )
      return MRPC_ERR_ARG;
    if( headlen>0xFFFFFFFFu||taillen>0xFFFFFFFFu )
      return MRPC_ERR_ARG;

    // a two-pass coder that says nothing for an hour is
    // indistinguishable from a hung one, but only if asked
    g_prog = (prog_req>1)||(prog_req==1&&im&&
                            double(im->width)*double(im->height)>=MRP_PROGMIN);
    // ... and MRPC_PROG turns it on for a measurement without going
    // through the caller, which is how the numbers in the markdown next
    // to this file were collected
    if( const char* pe = getenv("MRPC_PROG") )
      g_prog = atoi(pe);
    t_start = tnow();

    ResetModels();
    rc.StartEncodeMem();
    CodeU8(MRPC_VERSION);
    CodeU8(uint(im ? (1|(tflip ? 2 : 0)) : 0));
    CodeVar(uint(headlen));
    CodeVar(uint(taillen));
    if( im ) {
      CodeVar(im->width);
      CodeVar(im->height);
      CodeU8(im->ncomp);
      CodeVar(im->stride);
    }
    CodeBytes((const byte*)head, 0, headlen);
    if( im ) {
      iw = im->width;
      ih = im->height;
      ImgInit(tflip ? ih : iw, tflip ? iw : ih, im->ncomp, im->stride);
      pixbytes = uint(size_t(im->stride)*ih);
      img = (byte*)im->data; // borrowed: the encoder only reads it
      own_img = 0;
      Encode();
      CodePad(0);
      img = 0;
      ImgQuit();
    }
    CodeBytes((const byte*)tail, 0, taillen);
    rc.FinishEncode();

    out->data = rc.buf; // the block the coder grew is the blob
    out->size = rc.len;
    rc.MemQuit();
    return out->data ? MRPC_OK : MRPC_ERR_MEM;
  }

  // The orientation trial.
  //
  // mrpc already tries all nc! component orders and nothing else.  A
  // picture with horizontal structure and one with vertical structure are
  // the same picture to a codec that can code either -- but not to this
  // one, whose taps, activity neighbourhood and quadtree all run one way.
  // Measured on the corpus, coding transposed is worse on average (+1.6%
  // mean, dominated by one tile at +39%) and better on ten of twenty-four,
  // by up to 4.8%; as a trial that keeps the smaller of the two it is
  // -0.41% mean on top of everything else here, and cannot lose.
  //
  // It costs exactly two encodes, which is why it is off by default: the
  // trial is a second full search -- measured at 2.2-2.4x -- and there is
  // no cheap proxy for it any more than there was for the coefficient
  // range.  -t turns it on.
  int CompressTrial(const mrpc_image* im, const void* head, size_t headlen,
                    const void* tail, size_t taillen, mrpc_blob* out) {
    tflip = 0;
    vmode = 1;
    const int nt = (trial_flip&&im&&im->width>1&&im->height>1) ? 2 : 1;
    const int nv = (trial_vmap&&im) ? 2 : 1;
    if( nt*nv<2 )
      return Compress(im, head, headlen, tail, taillen, out);
    mrpc_blob best;
    best.data = 0;
    best.size = 0;
    int rc = MRPC_ERR_DATA;
    for( int i = 0; i<nt*nv; i++ ) {
      tflip = i&1 ? (nt>1 ? 1 : 0) : 0;
      vmode = ((nv>1)&&(i>=nt)) ? 2 : 1;
      mrpc_blob b;
      b.data = 0;
      b.size = 0;
      int r = Compress(im, head, headlen, tail, taillen, &b);
      if( r==MRPC_OK&&(!best.data||b.size<best.size) ) {
        free(best.data);
        best = b;
        rc = MRPC_OK;
      } else {
        free(b.data);
        if( r!=MRPC_OK&&rc!=MRPC_OK )
          rc = r;
      }
    }
    tflip = 0;
    vmode = 1;
    if( rc!=MRPC_OK ) {
      free(best.data);
      return rc;
    }
    *out = best;
    return MRPC_OK;
  }

  // The two pictures an encode can hand back instead of a stream.
  //
  //   PIC_PLOT  the code length of every component, written into a raster
  //             of the caller's geometry as it is coded: no second model,
  //             no second pass, and no branch in the encoder that does not
  //             want one -- CI_PLOT is a separate instantiation of the
  //             coding loop, so `c` and `p` share the search and nothing
  //             else.  It sums to the size of the file `c` would have
  //             written, give or take what saturated.
  //   PIC_QMAP  the class map: one byte a pixel, the class whose predictor
  //             and quantiser coded it.  A class is settled before the
  //             coding loop runs, so this one is a copy at the end of the
  //             encode rather than anything inside it.
  //
  // Both come out of a real encode, and if the trials are on, out of
  // whichever encode won -- so the loop that runs them is one loop, and
  // only the buffer it fills and the shape of that buffer differ.  The
  // stream is thrown away either way; that is the point.
  enum { PIC_PLOT = 0, PIC_QMAP = 1 };

  int Picture(int kind, const mrpc_image* im, mrpc_image* out,
              double* bits, int* nclass) {
    // cleared before anything can go wrong, so that a caller who ignores
    // the return code is looking at an empty image rather than a stack
    if( bits )
      *bits = 0.0;
    if( nclass )
      *nclass = 0;
    if( out )
      memset(out, 0, sizeof(*out));
    if( !out||!im )
      return MRPC_ERR_ARG;
    if( !im->data||im->width==0||im->height==0||im->ncomp<1||
        im->ncomp>MAXC||im->stride<im->width*im->ncomp )
      return MRPC_ERR_ARG;
    // a class belongs to a pixel and not to a component, so the class map
    // is one plane, packed, whatever the image it describes was
    const uint pstride = (kind==PIC_QMAP) ? im->width : im->stride;
    const size_t n = size_t(pstride)*im->height;
    byte* best = 0;
    size_t bestsz = 0;
    double bestbits = 0.0;
    int bestclass = 0;
    int rc = MRPC_ERR_MEM;
    const int nt = (trial_flip&&im->width>1&&im->height>1) ? 2 : 1;
    const int nv = trial_vmap ? 2 : 1;
    for( int i = 0; i<nt*nv; i++ ) {
      byte* pb = (byte*)calloc(n ? n : 1, 1);
      if( !pb )
        break;
      mrpc_blob blob;
      blob.data = 0;
      blob.size = 0;
      plotbuf = (kind==PIC_QMAP) ? 0 : pb;
      qmapbuf = (kind==PIC_QMAP) ? pb : 0;
      plot_bits = 0.0;
      tflip = (i&1) ? (nt>1 ? 1 : 0) : 0;
      vmode = ((nv>1)&&(i>=nt)) ? 2 : 1;
      int r = Compress(im, 0, 0, 0, 0, &blob);
      plotbuf = 0;
      qmapbuf = 0;
      if( r==MRPC_OK&&(!best||blob.size<bestsz) ) {
        free(best);
        best = pb;
        bestsz = blob.size;
        bestbits = plot_bits;
        bestclass = num_class;
        rc = MRPC_OK;
      } else {
        free(pb);
        if( r!=MRPC_OK )
          rc = r;
      }
      free(blob.data);
    }
    tflip = 0;
    vmode = 1;
    if( rc!=MRPC_OK ) {
      free(best);
      return rc;
    }
    out->width = im->width;
    out->height = im->height;
    out->ncomp = (kind==PIC_QMAP) ? 1 : im->ncomp;
    out->stride = pstride;
    out->data = best;
    if( bits )
      *bits = bestbits;
    if( nclass )
      *nclass = bestclass;
    return MRPC_OK;
  }

  int Plot(const mrpc_image* im, mrpc_image* out, double* bits) {
    return Picture(PIC_PLOT, im, out, bits, 0);
  }

  int ClassMap(const mrpc_image* im, mrpc_image* out, int* nclass) {
    return Picture(PIC_QMAP, im, out, 0, nclass);
  }

  int Decompress(const void* data, size_t size, mrpc_image* im,
                 mrpc_blob* head, mrpc_blob* tail) {
    if( im ) {
      memset(im, 0, sizeof(*im));
    }
    if( head ) {
      head->data = 0;
      head->size = 0;
    }
    if( tail ) {
      tail->data = 0;
      tail->size = 0;
    }
    if( !data||size<2 )
      return MRPC_ERR_ARG;

    g_prog = 0; // decoding is one pass and says nothing
    ResetModels();
    rc.StartDecodeMem(data, size);
    if( CodeU8(0)!=MRPC_VERSION )
      return MRPC_ERR_VERSION;
    const uint hb = CodeU8(0);
    const uint has = hb&1;
    tflip = (hb>>1)&1;
    const uint headlen = CodeVar(0), taillen = CodeVar(0);
    uint w = 0, h = 0, ncc = 0, st = 0;
    if( has ) {
      w = CodeVar(0);
      h = CodeVar(0);
      ncc = CodeU8(0);
      st = CodeVar(0);
      // a stream that is not ours, or is truncated, decodes to whatever
      // the models make of zeroes; it does not get to name an allocation
      if( w==0||h==0||w>(1u<<20)||h>(1u<<20)||ncc<1||ncc>MAXC||
          st<w*ncc||size_t(st)*h>(size_t(1)<<32) )
        return MRPC_ERR_DATA;
    }
    if( headlen>(1u<<28)||taillen>(1u<<28) )
      return MRPC_ERR_DATA;

    byte* hp = 0;
    byte* tp = 0;
    if( headlen ) {
      hp = (byte*)malloc(headlen);
      if( !hp )
        return MRPC_ERR_MEM;
    }
    CodeBytes(0, hp, headlen);

    if( has ) {
      iw = w;
      ih = h;
      ImgInit(tflip ? h : w, tflip ? w : h, ncc, st);
      pixbytes = uint(size_t(st)*h);
      img = (byte*)malloc(pixbytes);
      own_img = 1;
      if( !img ) {
        free(hp);
        ImgQuit();
        return MRPC_ERR_MEM;
      }
      Decode();
      StoreOrg();
      CodePad(1);
    }

    if( taillen ) {
      tp = (byte*)malloc(taillen);
      if( !tp ) {
        free(hp);
        ImgQuit();
        return MRPC_ERR_MEM;
      }
    }
    CodeBytes(0, tp, taillen);
    rc.MemQuit(); // the input block is the caller's

    if( head ) {
      head->data = hp;
      head->size = headlen;
    } else
      free(hp);
    if( tail ) {
      tail->data = tp;
      tail->size = taillen;
    } else
      free(tp);
    if( has&&im ) {
      im->width = w;
      im->height = h;
      im->ncomp = ncc;
      im->stride = st;
      im->data = img;
      own_img = 0; // handed over
    }
    ImgQuit();
    return MRPC_OK;
  }
};


// -------------------------------------------------------------
// The C API.  A context is one Codec; everything else here is argument
// checking and the two conversions between the library's types and the
// codec's.
// -------------------------------------------------------------
// The codec has ALIGN(32) members that the AVX2 paths load with
// _mm256_load_si256, and malloc promises sixteen bytes.  The original
// program had one of these in static storage, where the attribute is
// honoured; a library hands them out, so it has to ask.
struct mrpc_ctx {
  Codec cd;
};

static const size_t MRPC_CTX_ALIGN = 64;

extern "C" {

MRPC_API mrpc_ctx* MRPC_CALL mrpc_init(const mrpc_opts* opts) {
  size_t n = (sizeof(mrpc_ctx)+MRPC_CTX_ALIGN-1)&~(MRPC_CTX_ALIGN-1);
  mrpc_ctx* c = (mrpc_ctx*)aligned_alloc(MRPC_CTX_ALIGN, n);
  if( !c )
    return 0;
  g_sigma.Init();
  c->cd.Init();
  c->cd.prog_req = opts ? opts->progress : 0;
  c->cd.class_req = opts ? opts->num_class : 0;
  c->cd.trial_flip = opts ? opts->trial_flip : 0;
  c->cd.trial_vmap = opts ? opts->trial_vmap : 0;
  c->cd.vmode = 1;
  g_clopt.use = opts ? opts->use_opencl : 1;
  g_clopt.plat = opts ? opts->platform : -1;
  g_clopt.dev = opts ? opts->device : -1;
  g_clopt.type = opts ? opts->device_type : 0;
  g_clopt.cache = opts ? opts->cache_kernels : 0;
  g_clopt.verbose = opts ? opts->verbose : 0;
#ifdef MRP_OPENCL
  // The device and its kernels are the context's, not the image's: open
  // and build them here, so that mrpc_device_used can be asked before
  // anything is compressed and so that the second image costs neither.
  if( g_clopt.use&&!g_cl.Open(g_clopt) ) {
    // Naming a device that is not there is worth failing for; not
    // finding one when none was asked for is not.
    if( g_clopt.dev>=0||g_clopt.plat>=0||g_clopt.type ) {
      c->cd.Quit();
      free(c);
      return 0;
    }
  }
  if( g_cl.ctx )
    g_cl.Program();
#endif
  return c;
}

MRPC_API void MRPC_CALL mrpc_quit(mrpc_ctx* c) {
  if( !c )
    return;
  c->cd.Quit();
  aligned_free(c);
}

MRPC_API const char* MRPC_CALL mrpc_device_used(mrpc_ctx* c) {
  static char buf[1024];
  (void)c;
#ifdef MRP_OPENCL
  if( g_cl.active||g_cl.ctx ) {
    g_cl.Describe(buf, sizeof(buf));
    return buf;
  }
#endif
  return "host";
}

MRPC_API int MRPC_CALL mrpc_compress(mrpc_ctx* c, const mrpc_image* img,
                                     const void* head, size_t headlen,
                                     const void* tail, size_t taillen,
                                     mrpc_blob* out) {
  if( !c )
    return MRPC_ERR_ARG;
  return c->cd.CompressTrial(img, head, headlen, tail, taillen, out);
}

MRPC_API int MRPC_CALL mrpc_decompress(mrpc_ctx* c, const void* data, size_t size,
                                       mrpc_image* img, mrpc_blob* head,
                                       mrpc_blob* tail) {
  if( !c )
    return MRPC_ERR_ARG;
  return c->cd.Decompress(data, size, img, head, tail);
}

MRPC_API int MRPC_CALL mrpc_plot(mrpc_ctx* c, const mrpc_image* img,
                                 mrpc_image* out, double* bits) {
  if( !c ) {
    if( bits )
      *bits = 0.0;
    return MRPC_ERR_ARG;
  }
  return c->cd.Plot(img, out, bits);
}

MRPC_API int MRPC_CALL mrpc_classmap(mrpc_ctx* c, const mrpc_image* img,
                                     mrpc_image* out, int* nclass) {
  if( !c ) {
    if( nclass )
      *nclass = 0;
    return MRPC_ERR_ARG;
  }
  return c->cd.ClassMap(img, out, nclass);
}

MRPC_API void MRPC_CALL mrpc_free(void* p) {
  free(p);
}

MRPC_API const char* MRPC_CALL mrpc_error(int rc) {
  switch( rc ) {
    case MRPC_OK:
      return "ok";
    case MRPC_ERR_ARG:
      return "bad argument";
    case MRPC_ERR_MEM:
      return "out of memory";
    case MRPC_ERR_DATA:
      return "not an mrpc stream, or a damaged one";
    case MRPC_ERR_VERSION:
      return "an mrpc stream from a later version";
    default:
      return "unknown error";
  }
}

MRPC_API int MRPC_CALL mrpc_device_count(void) {
#ifdef MRP_OPENCL
  CLDevList l;
  l.Enumerate(-1);
  return l.n;
#else
  return 0;
#endif
}

MRPC_API int MRPC_CALL mrpc_device_get(int index, mrpc_device* out) {
#ifdef MRP_OPENCL
  if( !out )
    return MRPC_ERR_ARG;
  memset(out, 0, sizeof(*out));
  CLDevList l;
  l.Enumerate(-1);
  if( index<0||index>=l.n )
    return MRPC_ERR_ARG;
  cl_device_id d = l.dev[index];
  size_t got = 0;
  clGetPlatformInfo(l.plat[index], CL_PLATFORM_NAME, sizeof(out->platform)-1, out->platform, &got);
  out->platform[(got<sizeof(out->platform)) ? got : sizeof(out->platform)-1] = 0;
  CLInfoStr(d, CL_DEVICE_NAME, out->name, sizeof(out->name));
  CLInfoStr(d, CL_DEVICE_VERSION, out->version, sizeof(out->version));
  CLInfoStr(d, CL_DRIVER_VERSION, out->driver, sizeof(out->driver));
  cl_device_type t = 0;
  cl_uint cu = 0, mhz = 0;
  cl_ulong gm = 0, lm = 0;
  size_t wg = 0;
  clGetDeviceInfo(d, CL_DEVICE_TYPE, sizeof(t), &t, 0);
  clGetDeviceInfo(d, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, 0);
  clGetDeviceInfo(d, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(mhz), &mhz, 0);
  clGetDeviceInfo(d, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(gm), &gm, 0);
  clGetDeviceInfo(d, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(lm), &lm, 0);
  clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wg), &wg, 0);
  out->type = (t&CL_DEVICE_TYPE_CPU) ? MRPC_DEV_CPU :
              (t&CL_DEVICE_TYPE_GPU) ? MRPC_DEV_GPU :
              (t&CL_DEVICE_TYPE_ACCELERATOR) ? MRPC_DEV_ACC : MRPC_DEV_ANY;
  out->units = int(cu);
  out->clock_mhz = int(mhz);
  out->global_mem = (unsigned long long)gm;
  out->local_mem = (unsigned long long)lm;
  out->max_work_group = wg;
  return MRPC_OK;
#else
  (void)index;
  (void)out;
  return MRPC_ERR_ARG;
#endif
}

} // extern "C"
