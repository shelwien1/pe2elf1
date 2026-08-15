// -------------------------------------------------------------
// bmpc -- lossless LPC compressor for BMP files.
//
// The raster is predicted a byte at a time by a linear mix over an 8x8
// pixel block whose bottom row holds the pixel being coded, in the
// middle, with the three pixels to its right still unknown:
//
//        x-4 x-3 x-2 x-1  x  x+1 x+2 x+3
//   y-7   .   .   .   .   .   .   .   .        '.' known -> input
//   y-6   .   .   .   .   .   .   .   .        '?' unknown
//    :    .   .   .   .   .   .   .   .
//   y-1   .   .   .   .   .   .   .   .
//   y     .   .   .   .   ?   ?   ?   ?
//
// 7*8 + 4 = 60 known pixels, every colour component of each of them an
// input, so the mix has n_colors*60 taps per predicted byte (plus an
// optional bias) and there is one such mix per colour component.
//
// The residual is coded by the order-0 form of what coder0 does: the
// same adaptive Counter with its wr/mw/K ParamUpdaters, in a 255-node
// binary decomposition of the byte, with coder0's order-1 byte context
// dropped.  The mix weights are driven by the same ParamUpdater class
// through its loss-space entry point AccumGH().
//
// Anything that is not a plain uncompressed BMP raster (RLE, bitfields,
// <8bpp, a truncated pixel array, a non-BMP file) falls back to coder0's
// order-1 model over the whole file, so the tool is lossless for any
// input.
//
// Usage:  bmpc c <in> <out>
//         bmpc d <in> <out>
// -------------------------------------------------------------

// -------------------------------------------------------------
// Model shape toggles.  All-zero is the plain linear mix of part 2;
// see README-bmpc.md for what each one measured.
// -------------------------------------------------------------
#ifndef MIX_NEWTON
#define MIX_NEWTON   1   // curvature fed to ParamUpdater::AccumGH:
                         //  0 = per-tap diagonal  h_i = hdot*x_i^2
                         //  1 = exact rank-1      h_i = hdot*|x|^2
                         // The loss depends on w only through dot = w.x, so
                         // its Hessian is hdot*x x' -- rank 1, and entirely
                         // off-diagonal.  The Newton step along x is
                         //   dw = -(gdot/hdot) * x/|x|^2,
                         // which is what ParamUpdater emits when R tracks
                         // hdot*|x|^2 (i.e. NLMS).  The diagonal form instead
                         // gives every one of the n_colors*60 correlated taps
                         // its own full step and overshoots by ~n_taps.
#endif
#ifndef MIX_BASE
#define MIX_BASE     3   // operating point the mix is measured from:
                         //  0 = none (inputs are absolute, output is 128+mix)
                         //  1 = W    pixel to the left
                         //  2 = N    pixel above
                         //  3 = (W+N)/2
                         //  4 = MED(W,N,NW), the LOCO-I/JPEG-LS predictor
                         // pred = base + mix is the same affine family as a
                         // bare linear mix, reparameterised around a point
                         // that is already close to the answer.  With base 0
                         // every tap spikes together in dark/bright regions
                         // and the step overshoots.
#endif
#define MIX_CENTER (MIX_BASE!=0)
#ifndef MIX_DUAL
#define MIX_DUAL     1   // 1 = run two mixes over the same inputs at
                         // different rates and combine them convexly,
                         //   pred = base + dotA + L*(dotB-dotA),
                         // with L = sigma(z) and z driven by ParamUpdater
                         // -- i.e. the combination weight lives in the
                         // logistic domain.  Each mix is updated on its own
                         // error, z on the combined one (the standard
                         // combination-of-adaptive-filters scheme: a fast
                         // filter tracks, a slow one is accurate, and the
                         // combination is at least as good as either).
#endif
#ifndef MIX_LOSS
#define MIX_LOSS     1   // loss the weights descend:
                         //  0 = L2      gdot = e
                         //  1 = Huber   gdot = clip(e,HUB)
                         //  2 = L1      gdot = HUB*sign(e)
                         //  3 = Huber with the knee at HUBA * an EMA of
                         //      |e| for that component.  A fixed knee is
                         //      not scale free: 8bpp images want it ~4x
                         //      wider than 24/32bpp ones (and the wider
                         //      the mix, the more one outlier costs), so
                         //      the right knee is a multiple of how big
                         //      the errors actually are here.
                         // Image residuals are heavy-tailed: one edge can
                         // move 240 weights by the amount a hundred smooth
                         // pixels move them back.
#endif
#ifndef MIX_LOGISTIC
#define MIX_LOGISTIC 0   // inputs stretched, output squashed, CE loss
#endif
#ifndef MIX_BIAS
#define MIX_BIAS     0   // bias added to the mix:
                         //  0 = none
                         //  1 = an extra constant-1 input, so the mix learns
                         //      one global offset through ParamUpdater
                         //  2 = a per-context offset: an EMA of the residual
                         //      indexed by the quantised local gradients
                         //      (W-NW, NW-N, N-NE), one table per component.
                         //      Catches what a *linear* mix structurally
                         //      cannot -- the offset a texture class leaves
                         //      behind.  Same loss, so the mix is updated
                         //      with the error after correction.
#endif
#ifndef W_LOGIT
#define W_LOGIT      0   // w = LO + SPAN*sigma(z), optimise z
#endif
#ifndef LPC_INTRA
#define LPC_INTRA    1   // also feed the already-coded components of the
                         // current pixel (0..n_colors-1 extra inputs)
#endif
#ifndef RES_ZIGZAG
#define RES_ZIGZAG   1   // fold the residual to |e|*2-(e<0) before coding
#endif
#ifndef RES_PLANE
#define RES_PLANE    1   // one order-0 residual model per colour component
#endif
#ifndef RES_CTX
#define RES_CTX      8   // activity buckets: |e| at W + N + NE, per plane.
                         // RES_CTX=1 collapses model 1 onto model 0.
#endif
#ifndef RES_EQ
#define RES_EQ      17   // signed buckets of eW+eN for model 2
#endif
#ifndef RES_NM
#define RES_NM       6   // residual models mixed:
                         //  0 = plane                       (order 0)
                         //  1 = plane x activity            (how noisy here)
                         //  2 = plane x signed eW+eN        (which way it missed)
                         //  3 = plane x signed error already
                         //      made on the previous component
                         //      of THIS pixel (cross-channel)
                         //  4 = plane x joint (qsigned eW, qsigned eN),
                         //      coarse -- direction, not just magnitude
                         //  5 = plane x predicted value, RES_VQ buckets
                         // Their predictions are mixed in the logistic
                         // domain by a per-(plane,activity) weight vector
                         // driven by ParamUpdater -- the same rank-1 Newton
                         // step the pixel mix uses, with the cross-entropy
                         // hdot = p(1-p).
#endif
#ifndef RES_VQ
#define RES_VQ     256   // buckets of the predicted byte for model 5
#endif
#ifndef RES_JQ
#define RES_JQ       5   // per-axis buckets of the joint eW/eN model 4
#endif
#ifndef RES_MD
#define RES_MD       1   // 1 = give the mixer a separate weight set per
                         // depth in the bit tree; the models' relative
                         // worth differs a lot between the top bits of the
                         // residual and the noise in the bottom ones
#endif
#ifndef RES_MH
#define RES_MH       0   // curvature the residual mixer is given:
                         //  0 = hdot 1      -> R = |s|^2
                         //  1 = hdot p(1-p) -> the true cross-entropy one.
                         // The true one collapses to ~0 exactly where the
                         // model is confident, which is most of the time, so
                         // the normalised step blows up there; the pixel mix
                         // hits the same wall with |x|^2 (see README sec.2).
#endif
#ifndef RES_SSE
#define RES_SSE      0   // 1 = refine the mixed probability through an APM
                         // indexed by (plane, tree node, quantised
                         // stretch(p)), interpolated between two buckets
#endif
#ifndef BLK_STAT
#define BLK_STAT     1   // per-block min / max / always-1 mask / always-0
                         // mask, per colour component, over B0_BLKLW x
                         // B0_BLKLH blocks.  The four of them are four small
                         // pictures, coded by a second Raster with the same
                         // model, ahead of the raster itself; then every
                         // pixel knows which of the 256 byte values its
                         // block can still hold, and a bit of the residual
                         // that only one of them allows is not coded at all.
#endif
#ifndef BLK_SVQ
#define BLK_SVQ     16   // residual model 5's bucket count for the statistic
                         // picture; it carries 4x the components, so the
                         // raster's 256 would be 100 MB of counters for a
                         // picture a few hundred pixels wide
#endif
#ifndef BLK_PACK
#define BLK_PACK     0   // code the statistic planes relative to each other
                         // (max-min, and vs min&max, or vs min|max) instead
                         // of raw.  Exact and it shrinks the numbers -- and
                         // it loses: the raw planes are *pictures*, smooth
                         // enough for the mix and the CM to predict, and the
                         // transform trades that structure for smaller
                         // magnitudes the coder was not paying much for.
#endif
#ifndef BLK_RQ
#define BLK_RQ       9   // buckets of the block's range (max-min): 0 for a
                         // constant block, else 1+floor(log2(range))
#endif
#ifndef BLK_MCTX
#define BLK_MCTX     1   // add that bucket to the residual mixer's context.
                         // The per-pixel activity EMA says how noisy the
                         // last few pixels were; the block range says how
                         // noisy this whole region is allowed to be.
#endif
#ifndef BLK_CLAMP
#define BLK_CLAMP    1   // clamp the prediction into the block's [min,max]
#endif
#ifndef STATS
#define STATS        0   // -DSTATS=1: dump predictor diagnostics to stderr
#endif

#include "sh_common.inc"

// coder0's counter constants come along frozen: they are shared with
// coder0.cpp and were tuned on book1, so IDX/opt.pl runs against bmpc must
// not move them.  Only the B0 block below is in the search space.
#define FREEZE_C0 1
#include "sh_counter0.inc"
#include "MOD/sh_model-B0_h.inc"
#include "config_b.hpp"

// -------------------------------------------------------------
// Block geometry.  PADL/PADR/PADT are what the block reaches outside
// the image; the raster buffer carries that much border so the input
// gather is branch-free.
// -------------------------------------------------------------
static const int BW   = 8;    // block width  (pixels)
static const int BH   = 8;    // block height (pixels)
static const int PADL = 4;    // pixels of the bottom row left of x
static const int PADR = 3;    // unknown pixels right of x
static const int PADT = BH-1; // known rows above y
static const int NPIX = (BH-1)*BW + PADL;    // 60 known pixels
// Components one Raster can carry.  The raster itself needs 4; the block
// statistics are coded as one picture of 4 planes x 4 components.
static const uint MAXC = 16;

// MIX_BIAS==2 context: three local gradients, 9 buckets each.
static const int BQ   = 9;
static const int NCTX = BQ*BQ*BQ;
static INLINE int qgrad( int d ) {
  int T1 = B0_BT1, T2 = B0_BT2, T3 = B0_BT3;
  int s = d<0 ? -1 : 1; if( d<0 ) d = -d;
  if( d==0 ) return 4;
  if( d<T1 ) return 4+s;
  if( d<T2 ) return 4+2*s;
  if( d<T3 ) return 4+3*s;
  return 4+4*s;
}

// -------------------------------------------------------------
// Shared coding state
// -------------------------------------------------------------
ALIGN(64) Rangecoder rc;
static uint f_DEC;

typedef Counter Ctr;

// Order-1 model for the header, the palette and the fallback path --
// coder0's model exactly.  Allocated rather than static because the
// LPC path needs almost none of it.
static Ctr (*o1)[256];

static void InitCtr( Ctr* p, uint n ) { for( uint i=0; i<n; i++ ) p[i].Init(); }

// coder0's inner loop, factored so encode and decode run the very same
// instructions (which is what keeps the two sides' floats in step).
static INLINE uint CodeByte( Ctr* m, uint c ) {
  uint cxt, bit, p;
  for( cxt=1; cxt<256; ) {
    bit = (c>>7)&1;
    p   = m[cxt].Predict();
    bit = rc.rc_BProcess( p, bit );
    m[cxt].C_Update( bit );
    c<<=1; cxt += cxt+bit;
  }
  return byte(cxt);
}

// -------------------------------------------------------------
// The mix
// -------------------------------------------------------------
typedef ParamUpdater<1,Config_LW> WUpd;

// dL/d(dot) for the three losses; hdot stays 1 (Gauss-Newton), so the
// step is NW*LossG(e)/|x|^2 * x -- the Huber/L1 forms just bound how far
// one outlier is allowed to drag the whole tap vector.
static INLINE float LossG( float e, float knee ) {
#if   MIX_LOSS==1 || MIX_LOSS==3
  return clip( e, knee );
#elif MIX_LOSS==2
  return (e<0.0f) ? -knee : knee;
#else
  (void)knee;
  return e;
#endif
}

static INLINE float squash( float x ) { return 1.0f/(1.0f+expf(-x)); }
static const float SMAX = 12.0f;                 // stretch domain bound
static const int   SSEQ = 32;                    // APM buckets
static INLINE float stp( float p ) {             // stretch of a probability
  p = clamp( p, 1.0f/65536.0f, 1.0f-1.0f/65536.0f );
  return logf(p/(1.0f-p));
}
// per-axis quantiser for the joint eW/eN context of residual model 4
static INLINE int qjoint( int d ) {
  int h = RES_JQ>>1;
  int s = d<0 ? -1 : 1; if( d<0 ) d = -d;
  int q = 0; while( q<h && d >= (2<<(2*q)) ) q++;
  return h + s*q;
}

// signed quantiser for the eW+eN context of residual model 2
static INLINE int qsigned( int d ) {
  int s = d<0 ? -1 : 1; if( d<0 ) d = -d;
  int h = RES_EQ>>1;
  int q = 0; while( q<h && d >= (1<<q) ) q++;
  return h + s*q;
}

// -------------------------------------------------------------
// A 256-bit set of byte values, and the two maps that carry a block's
// legal-value set into the alphabet the residual tree actually codes.
// Everything here is exact set algebra -- it changes what is coded, not
// how likely anything is.
// -------------------------------------------------------------
struct VSet {
  qword m[4];
  INLINE int has( uint v ) const { return int((m[v>>6]>>(v&63))&1); }
  // any member in [lo, lo+len)?  len is a power of two and lo a multiple
  // of it, so the range is either whole words or inside one word.
  INLINE int any( uint lo, uint len ) const {
    if( len>=64 ) {
      qword o = 0;
      for( uint i=lo>>6, e=(lo+len)>>6; i<e; i++ ) o |= m[i];
      return o!=0;
    }
    return ( m[lo>>6] >> (lo&63) & ((qword(1)<<len)-1) ) != 0;
  }
  uint count() const {
    uint c = 0;
    for( uint i=0; i<4; i++ ) for( qword w=m[i]; w; w&=w-1 ) c++;
    return c;
  }
};

// bit i of dst = bit (i+n) of src, mod 256.  Turns the set of legal
// VALUES into the set of legal raw residuals r = (v-pb)&255.
static INLINE void rotr256( VSet& d, const VSet& s, uint n ) {
  uint w = (n>>6)&3, b = n&63;
  if( b==0 ) { for( uint i=0; i<4; i++ ) d.m[i] = s.m[(i+w)&3]; }
  else for( uint i=0; i<4; i++ )
    d.m[i] = (s.m[(i+w)&3]>>b) | (s.m[(i+w+1)&3]<<(64-b));
}

// bit i -> bit 2i (Morton spread), and a 64-bit bit reversal.
static INLINE qword spread32( uint x ) {
  qword v = x;
  v = (v | (v<<16)) & 0x0000FFFF0000FFFFull;
  v = (v | (v<< 8)) & 0x00FF00FF00FF00FFull;
  v = (v | (v<< 4)) & 0x0F0F0F0F0F0F0F0Full;
  v = (v | (v<< 2)) & 0x3333333333333333ull;
  v = (v | (v<< 1)) & 0x5555555555555555ull;
  return v;
}
static INLINE qword revbits64( qword v ) {
  v = ((v>> 1)&0x5555555555555555ull) | ((v&0x5555555555555555ull)<< 1);
  v = ((v>> 2)&0x3333333333333333ull) | ((v&0x3333333333333333ull)<< 2);
  v = ((v>> 4)&0x0F0F0F0F0F0F0F0Full) | ((v&0x0F0F0F0F0F0F0F0Full)<< 4);
  v = ((v>> 8)&0x00FF00FF00FF00FFull) | ((v&0x00FF00FF00FF00FFull)<< 8);
  v = ((v>>16)&0x0000FFFF0000FFFFull) | ((v&0x0000FFFF0000FFFFull)<<16);
  return (v>>32) | (v<<32);
}

// The zigzag fold r -> z is a permutation of 0..255, so it permutes the
// set too:  z=2r for r in [0,127] (e>=0) and z=255-2r' for r'=r-128
// (e<0).  The first is the low half spread onto the even positions; the
// second is the high half reversed, spread, and shifted onto the odd
// ones.  ~30 instructions instead of 256 bit tests.
static INLINE void zigzagset( VSet& d, const VSet& s ) {
  qword e0 = spread32(uint(s.m[0])), e1 = spread32(uint(s.m[0]>>32));
  qword e2 = spread32(uint(s.m[1])), e3 = spread32(uint(s.m[1]>>32));
  qword h0 = revbits64(s.m[3]),      h1 = revbits64(s.m[2]);
  qword o0 = spread32(uint(h0)),     o1 = spread32(uint(h0>>32));
  qword o2 = spread32(uint(h1)),     o3 = spread32(uint(h1>>32));
  d.m[0] = e0 | (o0<<1);
  d.m[1] = e1 | (o1<<1) | (o0>>63);
  d.m[2] = e2 | (o2<<1) | (o1>>63);
  d.m[3] = e3 | (o3<<1) | (o2>>63);
}

// One mixer weight vector: RES_NM weights in the logistic domain.
struct BitMix {
  float w[RES_NM];
  ParamUpdater<1,Config_BM> u[RES_NM];
  void Init() {
    for( uint i=0; i<RES_NM; i++ ) { w[i] = BM_W0; u[i].Init(BM_W0); }
  }
};
static INLINE float stretch( float p ) { return logf(p/(1.0f-p)); }

template<class cfg>
struct LPCMixT {
  typedef ParamUpdater<1,cfg> U;
  int    n;      // taps
  float* w;      // effective weights, dense -> the dot product vectorises
  U*     u;      // one ParamUpdater per tap (val = w, or z when W_LOGIT)

  void Init( int n_, int seed_tap ) {
    n = n_;
    w = new float[n];
    u = new U[n];
    for( int i=0; i<n; i++ ) {
      float w0 = (i==seed_tap) ? LW_W0 : 0.0f;
#if W_LOGIT
      // z = logit((w0-LO)/SPAN); the box is symmetric so w0=0 gives z=0.
      float s = (w0 - LW_LO) / LW_SPAN;
      s = clamp( s, 1e-4f, 1.0f-1e-4f );
      u[i].Init( logf(s/(1.0f-s)) );
#else
      u[i].Init( w0 );
#endif
      w[i] = w0;
    }
  }

  INLINE float Dot( const float* restrict x ) const {
    float s = 0.0f;
    for( int i=0; i<n; i++ ) s += w[i]*x[i];
    return s;
  }

  // gdot = dL/d(dot), hdot = d2L/d(dot)2; the per-tap chain is
  // g_i = gdot*x_i, h_i = hdot*x_i^2 (Gauss-Newton diagonal).
  INLINE void Update( const float* restrict x, float gdot, float hdot, float xx ) {
#if MIX_NEWTON
    const float hc = hdot * xx;
#endif
    for( int i=0; i<n; i++ ) {
      float xi = x[i];
      float g  = gdot * xi;
#if MIX_NEWTON
      float h  = hc;
#else
      float h  = hdot * (xi*xi);
#endif
#if W_LOGIT
      // chain to z, w = LO + SPAN*sigma(z): f' = SPAN*s(1-s),
      // f'' = f'(1-2s)  -- the same map coder0 uses for mw (OPT_MWLGT).
      float s  = (w[i] - LW_LO) / LW_SPAN;
      float f1 = LW_SPAN * s * (1.0f-s);
      float f2 = f1 * (1.0f - 2.0f*s);
      float hz = h*(f1*f1) + g*f2;
      u[i].AccumGH( g*f1, hz );
      u[i].Apply( u[i].StepRaw(), -32.0f, 32.0f );
      w[i] = LW_LO + LW_SPAN/(1.0f+expf(-u[i].val));
#else
      u[i].AccumGH( g, h );
      u[i].Apply( u[i].StepRaw(), cfg::minVal, cfg::maxVal );
      w[i] = u[i].val;
#endif
    }
  }
};

typedef LPCMixT<Config_LW>  LPCMix;
typedef LPCMixT<Config_LW2> LPCMix2;

// -------------------------------------------------------------
// Raster codec
// -------------------------------------------------------------
struct Raster {
  uint W, H, nc, stride;   // pixels, components/pixel, row bytes incl. pad
  uint vq;                 // buckets of residual model 5 (runtime: the stat
                           // picture carries 4x the components and cannot
                           // afford nc*256*256 counters)
#if BLK_STAT
  uint blw, blh, nbx;      // log2 block size, blocks per row
  const VSet* lset;        // [block][component] legal value sets
  const byte* lrng;        // [block][component][2] its min and max
  const VSet* lg[MAXC];    // the sets in force at the current pixel
  const byte* lr[MAXC];
  int   rq[MAXC];          // its range bucket
#endif
  byte*  buf;              // bordered raster, nc bytes/pixel
  uint   bstride;          // buffer row in pixels
  int    ni;               // mix taps
  float* x;                // input vector
  LPCMix* mix;             // one per component
#if MIX_DUAL
  LPCMix2* mix2;           // the second, differently-paced mix
  ParamUpdater<1,Config_LM>* lam;   // z of the convex combination
#endif
  Ctr*   res[RES_NM];      // residual models, see RES_NM
  // Row offset of each model for the current pixel.  Rows 0/1/2/4 are filled
  // in Gather(); models 3 and 5 key off data that only exists part-way
  // through the pixel (the previous component's error, the predicted byte),
  // so they compute their offset at use.
  int    rbase[RES_NM][MAXC];
  signed char* emap;       // clipped prediction error, same geometry as buf
  BitMix* bmix;            // mixer weights, per (plane, activity[, depth])
  uint   nmix;
  int    mctx[MAXC];          // which set the current component uses
  int    cerr[MAXC];          // errors already made on this pixel
  int    vctx[MAXC];          // predicted-value bucket, per component
#if RES_SSE
  Ctr*   sse;              // [nc][256 nodes][SSEQ+1] APM
#endif
  Ctr*   padm;             // order-0 model for the row padding bytes
  float  i2f[256];         // byte -> mix input
  float  base[MAXC];          // i2f[] of the operating point, per component
  float  mad[MAXC];           // MIX_LOSS==3: EMA of |e| per component
#if MIX_BIAS==2
  float* bias;             // [nc][NCTX] EMA of the residual
  int    bctx[MAXC];          // context of the current pixel, per component
#endif
  float  xx;               // |x|^2 of the current input vector
#if STATS
  double s_mix, s_left;    // sum |error| of the mix / of the left neighbour
  qword  s_n;
  qword  s_h[256];         // residual histogram, for its order-0 entropy
#endif

  INLINE float Knee( uint k ) const {
#if MIX_LOSS==3
    return LW_HUBA * mad[k];
#else
    (void)k; return LW_HUB;
#endif
  }

  byte* At( int px, int py ) {
    return buf + (size_t(py+PADT)*bstride + (px+PADL))*nc;
  }

  void Init( uint W_, uint H_, uint nc_, uint stride_, uint vq_ = RES_VQ ) {
    W = W_; H = H_; nc = nc_; stride = stride_; vq = vq_;
#if BLK_STAT
    lset = 0; lrng = 0;
#endif
    bstride = W + PADL + PADR;
    size_t bn = size_t(H+PADT) * bstride * nc;
    buf = new byte[bn];
    memset( buf, 128, bn );     // border and not-yet-known pixels

    ni = NPIX*nc;
#if LPC_INTRA
    ni += nc-1;                 // components of this pixel already coded
#endif
#if MIX_BIAS==1
    ni += 1;
#endif
    x   = new float[ni+8];
    mix = new LPCMix[nc];
    // Seed each mix on "same component of the pixel to the left", which
    // is input (NPIX-1)*nc + k -- the last of the bottom-row group.
    // Without centring the mix has to reproduce the left neighbour itself,
    // so seed that tap at 1; with centring the offset already does it and
    // every tap seeds at 0.
    for( uint k=0; k<nc; k++ )
      mix[k].Init( ni, MIX_CENTER ? -1 : int((NPIX-1)*nc + k) );
#if MIX_DUAL
    mix2 = new LPCMix2[nc];
    lam  = new ParamUpdater<1,Config_LM>[nc];
    for( uint k=0; k<nc; k++ ) {
      mix2[k].Init( ni, MIX_CENTER ? -1 : int((NPIX-1)*nc + k) );
      lam[k].Init( 0.0f );          // sigma(0) = 1/2, an even blend
    }
#endif
#if MIX_BIAS==1
    x[ni-1] = 1.0f;
#endif

#if MIX_BIAS==2
    bias = new float[nc*NCTX];
    for( uint i=0; i<nc*NCTX; i++ ) bias[i] = 0.0f;
#endif
    // model 0 is the order-0 tree (per plane when RES_PLANE); models 1 and 2
    // add a context row each.
    uint nrow[RES_NM];
    nrow[0] = RES_PLANE ? nc : 1;
#if RES_NM>1
    nrow[1] = nc*RES_CTX;
#endif
#if RES_NM>2
    nrow[2] = nc*RES_EQ;
#endif
#if RES_NM>3
    nrow[3] = nc*RES_EQ;
#endif
#if RES_NM>4
    nrow[4] = nc*RES_JQ*RES_JQ;
#endif
#if RES_NM>5
    nrow[5] = nc*vq;
#endif
    for( uint m=0; m<RES_NM; m++ ) {
      res[m] = new Ctr[256*nrow[m]];
      InitCtr( res[m], 256*nrow[m] );
    }
    emap = new signed char[bn];
    memset( emap, 0, bn );

    nmix = nc*RES_CTX*(BLK_MCTX?BLK_RQ:1)*(RES_MD?8:1);
    bmix = new BitMix[nmix];
    for( uint i=0; i<nmix; i++ ) bmix[i].Init();
#if RES_SSE
    sse = new Ctr[nc*256*(SSEQ+1)];
    InitCtr( sse, nc*256*(SSEQ+1) );
#endif
    padm = new Ctr[256];
    InitCtr( padm, 256 );

#if STATS
    s_mix = s_left = 0.0; s_n = 0; memset( s_h, 0, sizeof(s_h) );
#endif

    for( uint k=0; k<4; k++ ) mad[k] = LW_MAD0;

    for( int v=0; v<256; v++ ) {
#if MIX_LOGISTIC
      i2f[v] = clip( stretch( (float(v)+0.5f)*(1.0f/256.0f) ), LW_SQC );
#else
      i2f[v] = (float(v)-128.0f)*(1.0f/128.0f);
#endif
    }
  }

  // Gather the 60 known pixels of the block into x[].
  INLINE void Gather( int px, int py ) {
#if MIX_BASE==0
    for( uint k=0; k<nc; k++ ) base[k] = 0.0f;
#else
    const byte* pW = At( px-1, py );
    const byte* pN = At( px,   py-1 );
    const byte* pNW = At( px-1, py-1 );
    for( uint k=0; k<nc; k++ ) {
#if   MIX_BASE==1
      int b = pW[k];
#elif MIX_BASE==2
      int b = pN[k];
#elif MIX_BASE==3
      int b = (int(pW[k]) + int(pN[k])) >> 1;
#else
      int a = pW[k], c = pN[k], d = pNW[k], b;
      if( d >= (a>c?a:c) )      b = (a<c?a:c);
      else if( d <= (a<c?a:c) ) b = (a>c?a:c);
      else                      b = a + c - d;
#endif
      base[k] = i2f[b];
    }
#endif
#if MIX_BIAS==2
    {
      const byte* pNE = At( px+1, py-1 );
      for( uint k=0; k<nc; k++ )
        bctx[k] = int(k)*NCTX
                + (qgrad(int(pW[k])-int(pNW[k]))*BQ
                +  qgrad(int(pNW[k])-int(pN[k])))*BQ
                +  qgrad(int(pN[k])-int(pNE[k]));
    }
#endif
#if BLK_STAT
    if( lset ) {
      size_t b = (size_t(py)>>blh)*nbx + (size_t(px)>>blw);
      for( uint k=0; k<nc; k++ ) {
        lg[k] = lset + b*nc + k;
        lr[k] = lrng + (b*nc+k)*2;
        uint rg = uint(lr[k][1]) - uint(lr[k][0]);
        uint q = 0; while( rg ) { q++; rg>>=1; }
        rq[k] = int(q>=BLK_RQ ? BLK_RQ-1 : q);
      }
    } else for( uint k=0; k<nc; k++ ) rq[k] = 0;
#endif
    {
      size_t o = size_t(At(px,py) - buf);
      const signed char* eW  = emap + o - size_t(nc);
      const signed char* eN  = emap + o - size_t(bstride)*nc;
      const signed char* eNE = eN + nc;
      for( uint k=0; k<nc; k++ ) {
        int w = eW[k], n = eN[k], ne = eNE[k];
        int aw = w<0?-w:w, an = n<0?-n:n, ane = ne<0?-ne:ne;
        uint a = uint(aw+an+ane);
        uint q = 0; while( q+1 < RES_CTX && a >= (2u<<q) ) q++;
        rbase[0][k] = (RES_PLANE ? int(k) : 0) * 256;
#if RES_NM>1
        rbase[1][k] = int(k*RES_CTX + q) * 256;
#endif
#if RES_NM>2
        rbase[2][k] = int(k*RES_EQ + uint(qsigned(w+n))) * 256;
#endif
#if RES_NM>4
        rbase[4][k] = (int(k)*RES_JQ*RES_JQ + qjoint(w)*RES_JQ + qjoint(n)) * 256;
#endif
#if BLK_STAT && BLK_MCTX
        mctx[k] = int((k*RES_CTX + q)*BLK_RQ + uint(rq[k]));
#else
        mctx[k] = int(k*RES_CTX + q);
#endif
        cerr[k] = 0;
      }
    }
    int i = 0;
    float s = 0.0f;
    for( int dy=-PADT; dy<0; dy++ ) {
      const byte* r = At( px-PADL, py+dy );
      for( uint j=0; j<BW*nc; j++ ) { float t = i2f[r[j]] - base[j%nc]; x[i++] = t; s += t*t; }
    }
    const byte* r = At( px-PADL, py );
    for( uint j=0; j<PADL*nc; j++ ) { float t = i2f[r[j]] - base[j%nc]; x[i++] = t; s += t*t; }
#if MIX_BIAS==1
    s += 1.0f;
#endif
    xx = s;
  }

  // One byte: predict, code the residual, update the mix.
  INLINE uint Byte( uint k, byte* p, uint v ) {
    LPCMix& m = mix[k];
    float dotA = m.Dot(x);
    float dot  = dotA + base[k];
#if MIX_DUAL
    float dotB = mix2[k].Dot(x);
    float lm   = squash( lam[k].val );
    float dd   = dotB - dotA;
    dot += lm*dd;
#endif
#if MIX_BIAS==2
    dot += bias[bctx[k]];
#endif

    int pb;
#if MIX_LOGISTIC
    float pp = squash(dot);
    float f  = clamp( pp*256.0f, 0.0f, 255.0f );
    pb = int(f);
#else
    float f  = clamp( dot*128.0f + 128.5f, 0.0f, 255.0f );
    pb = int(f);
#endif

#if STATS
    {
      int lf = int(p[int(k)-int(nc)]);      // same component, pixel to the left
      s_mix  += fabs(double(int(v)-pb));
      s_left += fabs(double(int(v)-lf));
      s_n++;
    }
#endif

#if BLK_STAT && BLK_CLAMP
    // The block cannot hold anything outside [min,max], so neither can a
    // sensible prediction.  Free, and it never hurts.
    if( lset ) {
      int lo = lr[k][0], hi = lr[k][1];
      if( pb<lo ) pb = lo; else if( pb>hi ) pb = hi;
    }
#endif

    vctx[k] = int((uint(pb)*vq)>>8);

    uint r = (v - uint(pb)) & 255;
#if RES_ZIGZAG
    int e = int(r); if( e>=128 ) e -= 256;
    r = (e<0) ? uint(-e-e-1) : uint(e+e);
#endif
#if STATS
    s_h[r&255]++;
#endif
    r = CodeRes( k, r, uint(pb) );
#if RES_ZIGZAG
    e = (r&1) ? -int((r+1)>>1) : int(r>>1);
    r = uint(e) & 255;
#endif
    v = (uint(pb) + r) & 255;

    p[k] = byte(v);
    {
      int d = int(v) - pb;
      d = (d<-128) ? -128 : (d>127 ? 127 : d);
      emap[size_t(p-buf)+k] = (signed char)d;
      cerr[k] = d;
    }

    // --- weight update -----------------------------------------------
#if MIX_LOGISTIC
    // cross-entropy against the target's position in [0,1]:
    // dL/ddot = p-t, d2L/ddot2 = p(1-p).
    float t = (float(v)+0.5f)*(1.0f/256.0f);
    float ew = pp-t;
    m.Update( x, LossG(ew, Knee(k)), pp*(1.0f-pp), xx );
    mad[k] += (fabsf(ew)-mad[k]) * MAD_R;
#if MIX_BIAS==2
    bias[bctx[k]] += (t-dot) * BIAS_R;
#endif
#else
    // squared error: dL/ddot = dot-t, d2L/ddot2 = 1.
    float t = (float(v)-128.0f)*(1.0f/128.0f);
    float ew = dot-t;
#if MIX_DUAL
    // each mix descends its own error, the blend descends the joint one
    m.Update( x, LossG(dotA+base[k]-t, Knee(k)), 1.0f, xx );
    mix2[k].Update( x, LossG(dotB+base[k]-t, LW_HUB2), 1.0f, xx );
    {
      float f1 = lm*(1.0f-lm);            // dL/dz = gdot*dd*f1
      float u1 = dd*f1;
      float gz = LossG(ew, Knee(k)) * u1;
      float hz = u1*u1 + LossG(ew,Knee(k))*dd*f1*(1.0f-2.0f*lm);
      lam[k].AccumGH( gz, hz );
      lam[k].Apply( lam[k].StepRaw(), Config_LM::minVal, Config_LM::maxVal );
    }
#else
    m.Update( x, LossG(ew, Knee(k)), 1.0f, xx );
#endif
    mad[k] += (fabsf(ew)-mad[k]) * MAD_R;
#if MIX_BIAS==2
    bias[bctx[k]] += (t-dot) * BIAS_R;
#endif
#endif

#if LPC_INTRA
    // this component now becomes an input for the next one
    if( k+1 < nc ) { float t = i2f[v] - base[k]; x[NPIX*nc+k] = t; xx += t*t; }
#endif
    return v;
  }

  // One residual byte through the RES_NM models, mixed in the logistic
  // domain, optionally refined by the APM.  Encode and decode run the very
  // same instructions; only the bit source differs, inside rc_BProcess.
  INLINE uint CodeRes( uint k, uint c, uint pb ) {
    uint cxt, bit;
#if BLK_STAT
    // Carry the block's legal VALUE set into the alphabet this tree codes:
    // first r = (v-pb)&255, then the zigzag fold if it is on.  A subtree
    // holding no legal symbol is not a choice, so its bit is not coded.
    VSet ls; uint nls = 256;
    if( lset ) {
      VSet t; rotr256( t, lg[k][0], pb );
#if RES_ZIGZAG
      zigzagset( ls, t );
#else
      ls = t;
#endif
      nls = ls.count();
    }
#endif
    Ctr* m0 = res[0] + rbase[0][k];
#if RES_NM>1
    Ctr* m1 = res[1] + rbase[1][k];
#endif
#if RES_NM>2
    Ctr* m2 = res[2] + rbase[2][k];
#endif
#if RES_NM>3
    Ctr* m3 = res[3] + (int(k)*RES_EQ + qsigned(k ? cerr[k-1] : 0))*256;
#endif
#if RES_NM>4
    Ctr* m4 = res[4] + rbase[4][k];
#endif
#if RES_NM>5
    Ctr* m5 = res[5] + (int(k*vq) + vctx[k])*256;
#endif
    BitMix* mxb = bmix + size_t(mctx[k])*(RES_MD?8:1);
#if RES_SSE
    Ctr* ap = sse + size_t(k)*256*(SSEQ+1);
#endif

    cxt = 1;
    for( uint d=0; cxt<256; d++ ) {
      bit = (c>>7)&1;

#if BLK_STAT
      if( nls<256 ) {
        uint len = 128u>>d, lo0 = (cxt - (1u<<d)) << (8-d);
        int a0 = ls.any(lo0,len), a1 = ls.any(lo0+len,len);
        if( a0 != a1 ) {                 // only one side is possible
          bit = uint(a1);
          c<<=1; cxt += cxt+bit;
          continue;                      // not coded, not modelled
        }
      }
#endif

      float s[RES_NM];
      m0[cxt].Predict();            s[0] = stp( m0[cxt].pK );
#if RES_NM>1
      m1[cxt].Predict();            s[1] = stp( m1[cxt].pK );
#endif
#if RES_NM>2
      m2[cxt].Predict();            s[2] = stp( m2[cxt].pK );
#endif
#if RES_NM>3
      m3[cxt].Predict();            s[3] = stp( m3[cxt].pK );
#endif
#if RES_NM>4
      m4[cxt].Predict();            s[4] = stp( m4[cxt].pK );
#endif
#if RES_NM>5
      m5[cxt].Predict();            s[5] = stp( m5[cxt].pK );
#endif
#if RES_MD
      // depth = index of the leading 1 of cxt, i.e. how many bits are in
      BitMix& mx = mxb[ (cxt>=128)?7 : (cxt>=64)?6 : (cxt>=32)?5 : (cxt>=16)?4
                      : (cxt>=8)?3 : (cxt>=4)?2 : (cxt>=2)?1 : 0 ];
#else
      BitMix& mx = mxb[0];
#endif
      float dot = 0.0f, xx = 0.0f;
      for( uint i=0; i<RES_NM; i++ ) { dot += mx.w[i]*s[i]; xx += s[i]*s[i]; }
      float pm = squash( clip(dot, SMAX) );

#if RES_SSE
      // APM: bucket stretch(pm) and interpolate the two neighbouring cells.
      float t  = (clip(stp(pm), SMAX)/SMAX + 1.0f) * (0.5f*SSEQ);
      int   bq = int(t); if( bq<0 ) bq=0; if( bq>SSEQ-1 ) bq=SSEQ-1;
      float fr = t - float(bq);
      Ctr& a0 = ap[size_t(cxt)*(SSEQ+1) + bq];
      Ctr& a1 = ap[size_t(cxt)*(SSEQ+1) + bq + 1];
      a0.Predict(); a1.Predict();
      float pa = a0.pK*(1.0f-fr) + a1.pK*fr;
      float p  = pm*(1.0f-SSEW) + pa*SSEW;
#else
      float p  = pm;
#endif
      uint pi = uint( clamp( p*float(SCALE) ) );

      bit = rc.rc_BProcess( pi, bit );

      m0[cxt].C_Update( bit );
#if RES_NM>1
      m1[cxt].C_Update( bit );
#endif
#if RES_NM>2
      m2[cxt].C_Update( bit );
#endif
#if RES_NM>3
      m3[cxt].C_Update( bit );
#endif
#if RES_NM>4
      m4[cxt].C_Update( bit );
#endif
#if RES_NM>5
      m5[cxt].C_Update( bit );
#endif
#if RES_SSE
      a0.C_Update( bit ); a1.C_Update( bit );
#endif

      // Mixer: cross entropy on the mixed probability, rank-1 curvature.
      // Counter::pK -- and therefore pm -- is P(bit==0), which is what
      // rc_BProcess wants as its freq; the target is 1-bit, not bit.
      {
        float gdot = pm - float(1-bit);
#if RES_MH
        float hc   = pm*(1.0f-pm)*xx;
#else
        float hc   = xx;
#endif
        for( uint i=0; i<RES_NM; i++ ) {
          mx.u[i].AccumGH( gdot*s[i], hc );
          mx.u[i].Apply( mx.u[i].StepRaw(), Config_BM::minVal, Config_BM::maxVal );
          mx.w[i] = mx.u[i].val;
        }
      }

      c<<=1; cxt += cxt+bit;
    }
    return byte(cxt);
  }

  // The image bytes live in memory: the encoder needs the whole raster up
  // front to compute the block statistics, and the stat planes are then
  // coded by a second Raster over an array that was never a file at all.
  void Row( byte* img, uint y ) {
    byte* q = img + size_t(y)*stride;
    byte* p = At(0,y);
    // Give pixel 0 a base: its left border mirrors the pixel above it.
    for( int j=-PADL; j<0; j++ ) memcpy( At(j,int(y)), At(0,int(y)-1), nc );
    for( uint px=0; px<W; px++, p+=nc ) {
      Gather( px, y );
#if LPC_INTRA
      for( uint j=0; j<nc-1; j++ ) x[NPIX*nc+j] = 0.0f;
#endif
      for( uint k=0; k<nc; k++ ) {
        uint v = Byte( k, p, q[px*nc+k] );
        q[px*nc+k] = byte(v);
      }
      // edge replication: the border columns of this row mirror its
      // outermost pixels, so the block never reads the 128 fill once a
      // row has started.
      if( px==0 )   for( int j=-PADL; j<0; j++ ) memcpy( At(j,y), p, nc );
      if( px==W-1 ) for( int j=1; j<=PADR; j++ ) memcpy( At(int(W)-1+j,y), p, nc );
    }
    // row padding to the 4-byte boundary
    for( uint j=W*nc; j<stride; j++ ) q[j] = byte( CodeByte( padm, q[j] ) );
  }

  void Code( byte* img ) {
    for( uint y=0; y<H; y++ ) Row(img,y);
#if STATS
    double h0 = 0.0;
    for( int i=0; i<256; i++ ) if( s_h[i] )
      h0 -= double(s_h[i]) * log2(double(s_h[i])/double(s_n));
    fprintf( stderr,
      "[stats] %ux%u nc=%u taps=%d  |e|mix %.3f  |e|left %.3f  H0(res) %.4f bpc\n",
      W, H, nc, ni, s_mix/double(s_n), s_left/double(s_n), h0/double(s_n) );
#endif
  }
};

// -------------------------------------------------------------
// Per-block statistics
//
// For every B x B block and every colour component: the minimum, the
// maximum, the AND of all its bytes (bits that are always 1) and their OR
// (whose complement is the bits that are always 0).  Four numbers per
// block per component -- four small pictures, one pixel per block, so the
// same Raster that codes the raster codes them, ahead of it.
//
// Together they say which of the 256 byte values the block can still
// hold.  A residual bit that only one of those values allows carries no
// information and is not coded.
// -------------------------------------------------------------
#if BLK_STAT

// IDX-FORMAT sec.5: the pattern is the search space, the clamp is the
// contract.  A block edge of 1..12 doublings is what the geometry allows.
static INLINE uint blkl( int v ) { return uint( v<1 ? 1 : (v>12 ? 12 : v) ); }

struct BlkStats {
  uint nbx, nby, nb, nc, blw, blh;
  byte* st;                // [nb][4][nc] -- the four planes, as a picture
  VSet* set;               // [nb][nc]    -- derived legal value sets
  byte* rng;               // [nb][nc][2] -- and their min/max

  void Init( uint W, uint H, uint nc_ ) {
    blw = blkl(B0_BLKLW); blh = blkl(B0_BLKLH);
    nc = nc_;
    nbx = (W + (1u<<blw) - 1) >> blw;
    nby = (H + (1u<<blh) - 1) >> blh;
    nb  = nbx*nby;
    st  = new byte[size_t(nb)*4*nc];
    memset( st, 0, size_t(nb)*4*nc );
    set = new VSet[size_t(nb)*nc];
    rng = new byte[size_t(nb)*nc*2];
  }

  // encoder side: measure the raster
  void Scan( const byte* img, uint W, uint H, uint stride ) {
    for( size_t i=0; i<size_t(nb)*nc; i++ ) {
      st[(i/nc)*4*nc + 0*nc + i%nc] = 255;   // min
      st[(i/nc)*4*nc + 1*nc + i%nc] = 0;     // max
      st[(i/nc)*4*nc + 2*nc + i%nc] = 255;   // and
      st[(i/nc)*4*nc + 3*nc + i%nc] = 0;     // or
    }
    for( uint y=0; y<H; y++ ) {
      const byte* q = img + size_t(y)*stride;
      byte* b = st + (size_t(y>>blh)*nbx)*4*nc;
      for( uint x=0; x<W; x++ ) {
        byte* c = b + size_t(x>>blw)*4*nc;
        for( uint k=0; k<nc; k++ ) {
          uint v = q[x*nc+k];
          if( v < c[0*nc+k] ) c[0*nc+k] = byte(v);
          if( v > c[1*nc+k] ) c[1*nc+k] = byte(v);
          c[2*nc+k] &= byte(v);
          c[3*nc+k] |= byte(v);
        }
      }
    }
  }

  // The four planes are far from independent, and the dependence is exact:
  //   max >= min                          -> code max-min
  //   and is a subset of min & max        -> code which of those bits it drops
  //   or  is a superset of min | max      -> code which bits it adds
  // so planes 1..3 become mostly-zero pictures.  Undoing it needs only the
  // same block's min and max, so it costs the coder nothing to unpick.
  void Pack() {
    for( size_t b=0; b<nb; b++ ) {
      byte* c = st + b*4*nc;
      for( uint k=0; k<nc; k++ ) {
        uint mn = c[0*nc+k], mx = c[1*nc+k], am = c[2*nc+k], om = c[3*nc+k];
        // an empty block (the image is not a whole number of blocks) has
        // min=255,max=0; that survives the round trip as max-min = 1.
        if( mn>mx ) { c[1*nc+k]=byte((mx-mn)&255); c[2*nc+k]=0; c[3*nc+k]=0; continue; }
        c[1*nc+k] = byte(mx-mn);
        c[2*nc+k] = byte((mn&mx) & ~am);
        c[3*nc+k] = byte(om & ~(mn|mx));
      }
    }
  }
  void Unpack() {
    for( size_t b=0; b<nb; b++ ) {
      byte* c = st + b*4*nc;
      for( uint k=0; k<nc; k++ ) {
        uint mn = c[0*nc+k], mx = (mn + c[1*nc+k]) & 255;
        c[1*nc+k] = byte(mx);
        c[2*nc+k] = byte((mn&mx) & ~c[2*nc+k]);
        c[3*nc+k] = byte((mn|mx) |  c[3*nc+k]);
      }
    }
  }

  // both sides: st -> the legal sets.  A block with no pixels in it (the
  // image is not a whole number of blocks) keeps min=255/max=0, which
  // yields the empty set; give those every value so nothing is excluded.
  void Build() {
    for( size_t b=0; b<nb; b++ ) for( uint k=0; k<nc; k++ ) {
      const byte* c = st + b*4*nc;
      uint mn = c[0*nc+k], mx = c[1*nc+k], am = c[2*nc+k], om = c[3*nc+k];
      VSet& S = set[b*nc+k];
      if( mn>mx ) {                       // empty block: constrain nothing
        for( uint i=0; i<4; i++ ) S.m[i] = ~qword(0);
        byte* R0 = rng + (b*nc+k)*2; R0[0]=0; R0[1]=255; continue;
      }
      byte* R = rng + (b*nc+k)*2;
      for( uint i=0; i<4; i++ ) S.m[i] = 0;
      for( uint v=mn; v<=mx; v++ )
        if( (v&am)==am && (v&~om&255)==0 ) S.m[v>>6] |= qword(1)<<(v&63);
      if( S.count()==0 ) {                    // cannot happen on real data
        for( uint i=0; i<4; i++ ) S.m[i] = ~qword(0); mn=0; mx=255;
      }
      R[0] = byte(mn); R[1] = byte(mx);
    }
  }
};
#endif

// -------------------------------------------------------------
// BMP header parsing
// -------------------------------------------------------------
static uint get16( const byte* p ) { return p[0] | (p[1]<<8); }
static uint get32( const byte* p ) { return p[0] | (p[1]<<8) | (p[2]<<16) | (uint(p[3])<<24); }

static const uint HDR0 = 54;   // BITMAPFILEHEADER + BITMAPINFOHEADER

struct BmpInfo {
  uint ok, off, W, H, nc, stride, pixbytes;
};

static BmpInfo ParseBMP( const byte* d, uint flen ) {
  BmpInfo b; memset( &b, 0, sizeof(b) );
  if( flen<HDR0 ) return b;
  if( d[0]!='B' || d[1]!='M' ) return b;
  uint off  = get32(d+10);
  uint isz  = get32(d+14);
  int  w    = int(get32(d+18));
  int  h    = int(get32(d+22));
  uint pl   = get16(d+26);
  uint bpp  = get16(d+28);
  uint comp = get32(d+30);
  if( isz<40 || pl!=1 || comp!=0 ) return b;
  if( bpp!=8 && bpp!=24 && bpp!=32 ) return b;
  if( w<=0 || h==0 ) return b;
  uint H = uint( h<0 ? -h : h );
  uint W = uint(w);
  if( off<HDR0 || off>flen ) return b;
  if( off < 14+isz ) return b;
  uint stride = ((W*bpp+31)/32)*4;
  if( stride==0 || H > (0xFFFFFFFFu/stride) ) return b;
  uint pix = stride*H;
  if( pix > flen-off ) return b;
  b.ok = 1; b.off = off; b.W = W; b.H = H; b.nc = bpp/8;
  b.stride = stride; b.pixbytes = pix;
  return b;
}

// -------------------------------------------------------------
int main( int argc, char** argv ) {
  uint c, f_len, f_pos;
  FILE *f, *g;

#if defined(__SSE3__) || defined(__x86_64__) || defined(_M_X64)
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

  if( argc<4 ) {
    printf(
      "bmpc - lossless LPC compressor for BMP files\n"
      "\n"
      "Usage: %s <mode> <input> <output>\n"
      "\n"
      "  <mode>    'c' compress, 'd' decompress\n"
      "\n"
      "The raster is predicted by an adaptive n_colors*60-tap mix over the\n"
      "8x8 block above and left of each pixel; the residual is coded by\n"
      "coder0's counter, order 0.  Non-raster BMPs and non-BMP files fall\n"
      "back to coder0's order-1 model.\n", argv[0] );
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

  o1 = (Ctr(*)[256]) new Ctr[256*256];
  InitCtr( (Ctr*)o1, 256*256 );

  uint last_c = 0;
  f_pos = 0;

  // -- header + palette (or the whole file in fallback mode) ----------
  // The decoder only learns b.off after the first HDR0 bytes come out.
  uint nhdr = b.ok ? (f_DEC ? HDR0 : b.off) : f_len;
  for( ; f_pos<nhdr; f_pos++ ) {
    c = (f_DEC==0) ? uint(getc(f)) : 0;
    c = CodeByte( o1[last_c], c );
    if( f_DEC ) putc( int(c), g );
    if( f_pos<HDR0 ) hdr[f_pos] = byte(c);
    last_c = c;
    // the decoder learns the geometry from the bytes it just produced
    if( b.ok && f_pos==HDR0-1 && f_DEC ) {
      b = ParseBMP( hdr, f_len );
      if( b.ok==0 ) return 5;         // encoder said BMP, header says no
      nhdr = b.off;
    }
  }

  // -- raster ---------------------------------------------------------
  if( b.ok ) {
    byte* img = new byte[b.pixbytes];
    if( f_DEC==0 ) { if( fread(img,1,b.pixbytes,f)!=b.pixbytes ) return 6; }
    else memset( img, 0, b.pixbytes );

    static Raster ras;
    ras.Init( b.W, b.H, b.nc, b.stride );

#if BLK_STAT
    // the four statistic planes, as one picture of 4*n_colors components,
    // coded by its own Raster ahead of the raster it describes
    static BlkStats bs;
    bs.Init( b.W, b.H, b.nc );
    if( f_DEC==0 ) { bs.Scan( img, b.W, b.H, b.stride ); if( BLK_PACK ) bs.Pack(); }
    {
      static Raster sras;
      uint nc2 = 4*b.nc;
      sras.Init( bs.nbx, bs.nby, nc2, bs.nbx*nc2, BLK_SVQ );
      sras.Code( bs.st );
    }
    if( BLK_PACK ) bs.Unpack();
    bs.Build();
    ras.lset = bs.set; ras.lrng = bs.rng;
    ras.blw = bs.blw;  ras.blh = bs.blh;  ras.nbx = bs.nbx;
#endif

    ras.Code( img );
    if( f_DEC ) fwrite( img, 1, b.pixbytes, g );
    f_pos += b.pixbytes;

    // -- trailer ------------------------------------------------------
    last_c = 0;
    for( ; f_pos<f_len; f_pos++ ) {
      c = (f_DEC==0) ? uint(getc(f)) : 0;
      c = CodeByte( o1[last_c], c );
      if( f_DEC ) putc( int(c), g );
      last_c = c;
    }
  }

  if( f_DEC==0 ) rc.FinishEncode();

  fclose(g);
  fclose(f);
  return 0;
}
