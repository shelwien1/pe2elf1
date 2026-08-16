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
#define MAX_TOTFREQ (1<<20)
#define MIN_FREQ 1

#include "sh_common.inc"

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
Rangecoder rc;
uint f_DEC = 0;

// A plain adaptive binary probability: no ParamUpdater, one shift.
struct BC {
  word p;
  void Init() { p = SCALE/2; }
  INLINE uint Code( uint bit ) {
    bit = rc.rc_BProcess( p, bit );
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

// [group][shape][subposition].  The decoder builds only the shapes the
// stream names, so num_pm is 1 there.
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
      int idx = pm_idx ? pm_idx[gr] : i;
      for( int j=0; j<NUM_SUBPM; j++ ) pm[(gr*num_pm+i)*NUM_SUBPM+j].id = i;
      set_freqtable( pm+size_t(gr*num_pm+i)*NUM_SUBPM, pdf, MAXVAL, idx,
                     SIGMA[gr] );
    }
  delete[] pdf;
  return pm;
}

// The cost tables the optimiser reads instead of running the coder.
static void set_costs( PMod* pm, int n ) {
  const double a = 1.0/log(2.0);
  for( int i=0; i<n; i++ ) {
    PMod& p = pm[i];
    if( !p.cost ) { p.cost = new float[PMSIZE+MAXVAL+1]; p.subcost = p.cost+PMSIZE; }
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
  rc.rc_Process( pm->cumfreq[s]-off, pm->freq[s], tot );
}
static INLINE int DecSym( const PMod* pm, int lo, int hi ) {
  uint off = pm->cumfreq[lo], tot = pm->cumfreq[hi]-off;
  uint v = rc.rc_GetFreq( tot );
  int i=lo, j=hi-1;
  while( i<j ) { int k=(i+j)>>1; if( pm->cumfreq[k+1]-off <= v ) i=k+1; else j=k; }
  rc.rc_Process( pm->cumfreq[i]-off, pm->freq[i], tot );
  return i;
}
static INLINE void EncSP( const SPMod& p, int s ) {
  rc.rc_Process( p.cumfreq[s], p.freq[s], p.cumfreq[p.size] );
}
static INLINE int DecSP( const SPMod& p ) {
  uint v = rc.rc_GetFreq( p.cumfreq[p.size] );
  int i=0, j=p.size-1;
  while( i<j ) { int k=(i+j)>>1; if( p.cumfreq[k+1]<=v ) i=k+1; else j=k; }
  rc.rc_Process( p.cumfreq[i], p.freq[i], p.cumfreq[p.size] );
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
  int  uoff[NUM_UPELS], uw[NUM_UPELS];

  int  (*coef)[MAXC][NTMAX];  // [class][component][tap]
  int  th[MRP_MAXCLASS][MAXC][MRP_GROUP];
  char (*uq)[MAXC][MAX_UPARA+1];
  int  pm_idx[MAXC][MRP_GROUP];

  PMod*  pmodels;
  int    num_pm;
  PMod*  pml[MAXC][MRP_GROUP];

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

  // ---------------------------------------------------------------
  void PredictRegion( uint y0, uint x0, uint y1, uint x1 ) {
    for( uint y=y0; y<y1; y++ ) {
      const byte* p = org + OI(y,x0);
      short* pr = prd + PI(y,x0);
      short* pe = errB + OI(y,x0);
      const char* pc = cls + size_t(y)*W + x0;
      for( uint x=x0; x<x1; x++ ) {
        int cl = int(*pc++);
        for( uint k=0; k<nc; k++ ) {
          int v = Clip( Predict( k, p, coef[cl][k] ) );
          pr[k] = short(v);
          pe[k] = short( Econv( int(p[k]), v ) );
        }
        p += nc; pr += nc; pe += nc;
      }
    }
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
        for( uint k=0; k<nc; k++ ) {
          int u  = Activity( k, pe );
          pu[k]  = short(u);
          int gr = int(uq[cl][k][u]);
          pg[k]  = char(gr);
          int q  = Qprd( int(pr[k]) );
          const PMod* pm = pml[k][gr] + (q & (NUM_SUBPM-1));
          int base = q >> PM_ACC;
          cost += pm->cost[base + int(p[k])] + pm->subcost[base];
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
      double s=0, s2=0;
      for( uint i=0; i<BASE_BSIZE; i++ ) {
        const byte* p = org + OI(by+i,bx);
        for( uint j=0; j<BASE_BSIZE*nc; j++ ) { double v=p[j]; s+=v; s2+=v*v; }
      }
      var[b] = s2 - s*s/double(BASE_BSIZE*BASE_BSIZE*nc);
      idx[b] = int(b);
    }
    for( size_t i=1; i<nb; i++ ) {           // insertion sort by variance
      int t = idx[i]; size_t j = i;
      while( j>0 && var[idx[j-1]] > var[t] ) { idx[j] = idx[j-1]; j--; }
      idx[j] = t;
    }
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
  cost_t DesignPredictor( int mmse ) {
    static double mat[NTMAX][NTMAX+1];
    static double wg[MRP_GROUP];
    static int    piv[NTMAX];
    for( int g=0; g<MRP_GROUP; g++ )
      wg[g] = mmse ? 1.0 : 1.0/(SIGMA[g]*SIGMA[g]);
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
      int n = nt[k];
      for( int i=0; i<n; i++ ) for( int j=0; j<=n; j++ ) mat[i][j] = 0.0;
      for( uint y=0; y<H; y++ ) {
        const char* pc = cls + size_t(y)*W;
        const byte* p  = org + OI(y,0);
        const char* pg = grp + PI(y,0);
        for( uint x=0; x<W; x++, p+=nc, pg+=nc ) {
          if( int(pc[x])!=cl ) continue;
          double w = wg[int(pg[k])];
          const int* to = toff[k];
          for( int i=0; i<n; i++ ) {
            double a = w*double(p[to[i]]);
            for( int j=i; j<n; j++ ) mat[i][j] += a*double(p[to[j]]);
            mat[i][n] += a*double(p[k]);
          }
        }
      }
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
    for( uint y=0; y<H; y++ ) {
      const byte* p = org + OI(y,0);
      const short* pr = prd + PI(y,0);
      const short* pe = errB + OI(y,0);
      short* pu = upara + PI(y,0);
      const char* pc = cls + size_t(y)*W;
      for( uint x=0; x<W; x++, p+=nc, pr+=nc, pe+=nc, pu+=nc ) {
        int cl = int(pc[x]);
        for( uint k=0; k<nc; k++ ) {
          int u = Activity( k, pe );
          pu[k] = short(u);
          int q = Qprd( int(pr[k]) ), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
          int v = base + int(p[k]);
          for( int gr=0; gr<MRP_GROUP; gr++ ) {
            const PMod* pm = pml[k][gr] + sub;
            CB(cl,k,gr)[u+1] += pm->cost[v] + pm->subcost[base];
          }
        }
      }
    }
    static cost_t dp[MAX_UPARA+2];
    static int    tre[MRP_GROUP][MAX_UPARA+2];
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
      for( int gr=0; gr<MRP_GROUP; gr++ ) {
        cost_t* c = CB(cl,k,gr);
        for( size_t u=1; u<nu; u++ ) c[u] += c[u-1];
      }
      for( size_t u=0; u<nu; u++ ) { dp[u] = CB(cl,k,0)[u]; tre[0][u] = 0; }
      for( int gr=1; gr<MRP_GROUP; gr++ ) {
        const cost_t* c = CB(cl,k,gr);
        for( int th1=int(nu)-1; th1>=0; th1-- ) {
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
      int t = int(nu)-1;
      for( int gr=MRP_GROUP-1; gr>0; gr-- ) { t = tre[gr][t]; th[cl][k][gr-1] = t; }
      th[cl][k][MRP_GROUP-1] = MAX_UPARA+1;
      int u = 0;
      for( int gr=0; gr<MRP_GROUP; gr++ )
        for( ; u<th[cl][k][gr] && u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(gr);
      for( ; u<=MAX_UPARA; u++ ) uq[cl][k][u] = char(MRP_GROUP-1);
    }
    #undef CB
    cost_t cost = CalcCost( 0, 0, H, W );

    // and which generalized-Gaussian shape each group should use
    if( opt_loop>1 && num_pm>1 ) {
      static cost_t pc2[MAXC][MRP_GROUP][NUM_PMODEL];
      for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
        for( int i=0; i<NUM_PMODEL; i++ ) pc2[k][g][i] = 0.0;
      for( uint y=0; y<H; y++ ) {
        const byte* p = org + OI(y,0);
        const short* pr = prd + PI(y,0);
        const char* pg = grp + PI(y,0);
        for( uint x=0; x<W; x++, p+=nc, pr+=nc, pg+=nc )
          for( uint k=0; k<nc; k++ ) {
            int gr = int(pg[k]);
            int q = Qprd( int(pr[k]) ), base = q>>PM_ACC, sub = q&(NUM_SUBPM-1);
            int v = base + int(p[k]);
            for( int i=0; i<NUM_PMODEL; i++ ) {
              const PMod* pm = pmodels + (size_t(gr)*num_pm+i)*NUM_SUBPM + sub;
              pc2[k][gr][i] += pm->cost[v] + pm->subcost[base];
            }
          }
      }
      cost = 0.0;
      for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ ) {
        int best = 0;
        for( int i=1; i<NUM_PMODEL; i++ )
          if( pc2[k][g][i] < pc2[k][g][best] ) best = i;
        pm_idx[k][g] = best;
        pml[k][g] = pmodels + (size_t(g)*num_pm+best)*NUM_SUBPM;
        cost += pc2[k][g][best];
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
    for( int cl=0; cl<num_class; cl++ ) {
      size_t bp = size_t(cl)*bufsize*bufsize*nc;
      for( uint y=tly; y<bry; y++ ) {
        const byte* p = org + OI(y,tlx);
        short* pb = prdbuf + bp + (size_t(y%bufsize)*bufsize + tlx%bufsize)*nc;
        short* eb = errbuf + bp + (size_t(y%bufsize)*bufsize + tlx%bufsize)*nc;
        for( uint x=tlx; x<brx; x++, p+=nc, pb+=nc, eb+=nc ) {
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
  int FindClass( uint tly, uint tlx, uint bry, uint brx, uint bufsize ) {
    cost_t mc = 1e30; int mcl = 0;
    for( int cl=0; cl<num_class; cl++ ) {
      PutBlock( cl, tly, tlx, bry, brx, bufsize );
      cost_t c = class_cost[mtfbuf[cl]] + CalcCost( tly, tlx, bry, brx );
      if( c<mc ) { mc = c; mcl = cl; }
    }
    PutBlock( mcl, tly, tlx, bry, brx, bufsize );
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
    int cl = FindClass( tly, tlx, bry, brx, bufsize );
    cost_t qtcost = class_cost[mtfbuf[cl]];
    if( level>0 ) {
      int ctx = QtCtx( level, tly, tlx, blksize, width );
      cost_t cost1 = CalcCost( tly, tlx, bry, brx )
                   + class_cost[mtfbuf[cl]] + qtflag_cost[ctx];
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
    for( uint y=0; y<H; y+=blksize ) for( uint x=0; x<W; x+=blksize ) {
      SetPrdBuf( y, x, blksize );
      VbsClass( y, x, blksize, W, level, blksize );
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
    for( int i=0, n=0; i<SR; i++ ) {
      int y = cf[p1] + i - (SR>>1); if( y<0 ) y = -y; if( y>MAX_COEF ) y = MAX_COEF;
      for( int j=0; j<SSR; j++ ) {
        int x = cf[p2] - (i-(SR>>1)) - (j-(SSR>>1));
        if( x<0 ) x = -x; if( x>MAX_COEF ) x = MAX_COEF;
        cbuf[n++] = coef_cost[coef_m[k][p1]][y] + coef_cost[coef_m[k][p2]][x];
      }
    }
    int o1 = toff[k][p1], o2 = toff[k][p2];
    for( uint y=0; y<H; y++ ) {
      const char* pc = cls + size_t(y)*W;
      const byte* p = org + OI(y,0);
      const short* pr = prd + PI(y,0);
      const char* pg = grp + PI(y,0);
      for( uint x=0; x<W; x++, p+=nc, pr+=nc, pg+=nc ) {
        if( int(pc[x])!=cl ) continue;
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
    int b = (SR*SSR)>>1;
    for( int i=0; i<SR*SSR; i++ ) if( cbuf[i] < cbuf[b] ) b = i;
    int i = (b/SSR) - (SR>>1), j = (b%SSR) - (SSR>>1);
    int y = cf[p1] + i, x = cf[p2] - i - j;
    if( y<-MAX_COEF ) y = -MAX_COEF; else if( y>MAX_COEF ) y = MAX_COEF;
    if( x<-MAX_COEF ) x = -MAX_COEF; else if( x>MAX_COEF ) x = MAX_COEF;
    i = y - cf[p1]; j = x - cf[p2];
    if( i==0 && j==0 ) return;
    cf[p1] = y; cf[p2] = x;
    for( uint yy=0; yy<H; yy++ ) {
      const char* pc = cls + size_t(yy)*W;
      const byte* p = org + OI(yy,0);
      short* pr = prd + PI(yy,0);
      short* pe = errB + OI(yy,0);
      for( uint xx=0; xx<W; xx++, p+=nc, pr+=nc, pe+=nc ) {
        if( int(pc[xx])!=cl ) continue;
        int v = Clip( int(pr[k]) + int(p[o1])*i + int(p[o2])*j );
        pr[k] = short(v);
        pe[k] = short( Econv( int(p[k]), v ) );
      }
    }
  }
  // MRP draws the two taps at random.  Sweeping them instead -- every tap
  // paired with the one a fixed distance away, the distance rotating with
  // the iteration -- covers the same pairs, converges no worse, and makes
  // an encode reproducible: with the random draw the same file compresses
  // to sizes ~1% apart depending only on the seed, which is enough noise
  // to hide the parameter differences one is trying to measure.
  uint optpass;
  cost_t OptimizePredictor() {
    for( int cl=0; cl<num_class; cl++ ) for( uint k=0; k<nc; k++ ) {
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
          if( v>0 ) rc.rc_Process( uint(s), 1, 2 );
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
            uint f = rc.rc_GetFreq(2); uint s = (f>=1);
            rc.rc_Process( s, 1, 2 );
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
    if( num_pm>1 ) total += cost_t(nc)*MRP_GROUP*4.0;
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
      SPMod u; u.Set( NUM_PMODEL, -1 );
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
    SPMod u; u.Set( NUM_PMODEL, -1 );
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
    qindex = new int[nidx];
    qhist  = new uint[MRP_MAXCLASS];
    prdbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    errbuf = new short[size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE*nc];
    for( uint k=0; k<nc; k++ ) ord[k] = int(k);
    rnd = MRP_SEED; optpass = 0;
    opt_loop = 1;
    BuildTaps();
  }
  void Free() {
    delete[] org; delete[] errB; delete[] prd; delete[] upara;
    delete[] grp; delete[] cls; delete[] coef; delete[] uq;
    for( int l=0; l<QT_DEPTH; l++ ) delete[] qtmap[l];
    delete[] qindex; delete[] qhist; delete[] prdbuf; delete[] errbuf;
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
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      pml[k][g] = pmodels + (size_t(g)*num_pm + pm_idx[k][g])*NUM_SUBPM;
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
    DesignPredictor( 1 );
    cost_t c = CalcCost( 0, 0, H, W );
    num_class = save;
    return c;
  }
  void ChooseOrder() {
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
    num_pm  = NUM_PMODEL;
    pmodels = alloc_pmodels( num_pm, 0 );
    set_costs( pmodels, MRP_GROUP*num_pm*NUM_SUBPM );
    for( uint k=0; k<nc; k++ ) for( int g=0; g<MRP_GROUP; g++ )
      pm_idx[k][g] = NUM_PMODEL>>1;
    SetPmodels();
    SetCoefCost();
    DefaultSideCosts();

    ChooseOrder();
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
      cost_t c = DesignPredictor( it==0 );
      c = OptimizeGroup();
      c = OptimizeClass();
#if MRP_VERBOSE
      fprintf( stderr, "[%2d] %.0f\n", it, c/8.0 );
#endif
      if( c<best ) { best = c; last = it; SAVE(); }
      if( it-last >= EXTRA_ITER ) break;
    }
    LOAD();
    PredictRegion( 0, 0, H, W );
    CalcCost( 0, 0, H, W );

    opt_loop = 2;
    best = 1e30; last = 0;
    for( int it=0; it<MAX_ITER; it++ ) {
      cost_t c;
#if OPT_PRED
      c = OptimizePredictor();
#endif
      cost_t side = CodePredictor( 1 );
      c = OptimizeGroup();
      side += CodeThreshold( 1 );
      c = OptimizeClass();
      side += CodeClass( 1 );
      c += side;
#if MRP_VERBOSE
      fprintf( stderr, "(%2d) %.0f + %.0f side\n", it, (c-side)/8.0, side/8.0 );
#endif
      if( c<best ) { best = c; last = it; SAVE(); }
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
    // --- write it out
    CodeClass( 0 );
    CodePredictor( 0 );
    CodeThreshold( 0 );
    CodeImage( 0 );
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
    num_pm  = NUM_PMODEL;
    pmodels = alloc_pmodels( num_pm, 0 );
    opt_loop = 2;
    CodeParams( 1 );
    DecodeClassMap();
    DecodePredictor();
    DecodeThreshold();
    SetPmodels();
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
