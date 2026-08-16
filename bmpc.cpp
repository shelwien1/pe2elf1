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
#ifndef MIX_LEAN
#define MIX_LEAN     1   // Requires momentum_D = momentum_R = 0 (B0_M1_w and
                         // B0_M2_w, which the tuning drove to zero -- see
                         // README sec.2).  With no momentum the per-tap D and
                         // R that ParamUpdater carries are pure temporaries,
                         // and because the curvature is rank-1 the Newton
                         // denominator is the SAME for every tap.  So the
                         // whole update collapses to
                         //   w[i] = clamp( w[i] - clip(k*clip(gdot*x[i],Dc),
                         //                             stepMax), lo, hi )
                         // with k = NW/(R+inc) loop-invariant: one array
                         // instead of three, no per-tap divide, and a clean
                         // 8-wide AVX2 loop.  Set to 0 to go back through
                         // ParamUpdater tap by tap.
#endif
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
                         //      |e| for that component.
                         //  4 = the same EMA, but kept per (component,
                         //      activity bucket).  MRP fits its predictors
                         //      by normal equations weighted 1/sigma[gr]^2,
                         //      sigma being the spread of the residual in
                         //      that activity group -- the same statement
                         //      that an error of 6 is an outlier in a flat
                         //      region and business as usual in a busy
                         //      one, which a per-component knee cannot
                         //      say.  A fixed knee is
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
#ifndef MIX_CLS
#define MIX_CLS      1   // LPC weight sets per component, selected by the
                         // causal activity class (the RES_CTX bucket,
                         // folded down to MIX_CLS levels).
                         // MRP's central idea: one predictor is not enough
                         // for a whole image, so it segments the image into
                         // classes and fits a separate set of coefficients
                         // to each.  MRP finds the segmentation by two-pass
                         // code-length optimisation over a quadtree; the
                         // one-pass analogue is to let the class be
                         // something the decoder can compute too -- how
                         // busy the neighbourhood already is -- and let
                         // each class's weights adapt on its own pixels.
                         // Costs one Dot per pixel either way: only the
                         // selected set runs.
#endif
#ifndef MIX_LCLS
#define MIX_LCLS     1   // classes for the MIX_DUAL blend coordinate alone.
                         // The cheap half of MIX_CLS: both mixes keep
                         // seeing every pixel, so neither is starved, and
                         // only the one number that says which of them to
                         // believe is learned per class.
#endif
#ifndef LPC_INTRA
#define LPC_INTRA    1   // also feed the already-coded components of the
                         // current pixel (0..n_colors-1 extra inputs)
#endif
#ifndef RES_ZIGZAG
#define RES_ZIGZAG   1   // fold the residual before coding:
                         //  0 = none, r = (v-pb) & 255
                         //  1 = zigzag, |e|*2-(e<0), e the wrapped residual
                         //  2 = range-aware (MRP's e2E): the value is known
                         //      to lie in [lo,hi] -- the block's min/max
                         //      when BLK_STAT has them, 0..255 otherwise --
                         //      so only |e| <= th = min(pb-lo,hi-pb) has two
                         //      possible signs.  Zigzag that core onto
                         //      [0,2th] and lay the one-sided tail out
                         //      contiguously above it.  The image is the
                         //      prefix [0,hi-lo]: the impossible symbols
                         //      leave the alphabet instead of being
                         //      scattered through it by the wrap, so the
                         //      counters that see a narrow block and the
                         //      ones that see a wide one agree about what
                         //      symbol 12 means.
#endif
#ifndef RES_PLANE
#define RES_PLANE    1   // one order-0 residual model per colour component
#endif
#ifndef RES_CTX
#define RES_CTX      8   // activity buckets: |e| at W + N + NE, per plane.
                         // RES_CTX=1 collapses model 1 onto model 0.
#endif
#ifndef RES_CTXW
#define RES_CTXW     1   // what "activity" means for the RES_CTX bucket:
                         //  0 = |eW| + |eN| + |eNE|
                         //  1 = MRP's measure (mrp-05 encmrp.c, calc_uenc):
                         //      the 12 causal neighbours within radius 3,
                         //      weighted round(64/distance), sum >> 6.  Two
                         //      pixels of noise beside each other and two
                         //      three away are not the same amount of local
                         //      texture, and a three-tap sum cannot tell.
                         //  2 = the same, >> 7, which keeps the numbers in
                         //      the range the RES_CTX bucket edges were
                         //      picked for (the weights total 440, so >>6
                         //      is 6.9*mean|e| against the 3-tap 3*mean|e|)
#endif
#ifndef RES_EQW
#define RES_EQW      0   // 1 = model 2's signed context is the same 12
                         // neighbours, distance weighted and signed, rather
                         // than eW+eN.  The weighted magnitude sum (RES_CTXW)
                         // paid; this asks whether the direction does too.
#endif
#ifndef RES_EQ
#define RES_EQ      17   // signed buckets of eW+eN for model 2
#endif
#ifndef RES_NM
#define RES_NM       7   // residual models mixed:
                         //  0 = plane                       (order 0)
                         //  1 = plane x activity            (how noisy here)
                         //  2 = plane x signed eW+eN        (which way it missed)
                         //  3 = plane x signed error already
                         //      made on the previous component
                         //      of THIS pixel (cross-channel)
                         //  4 = plane x joint (qsigned eW, qsigned eN),
                         //      coarse -- direction, not just magnitude
                         //  5 = plane x predicted value, RES_VQ buckets
                         //  6 = plane x activity x the FRACTIONAL part of
                         //      the prediction, RES_FQ buckets.  The mix
                         //      lands on 137.9 and the coder is told 138;
                         //      MRP keeps that fraction and indexes its
                         //      pmf by it (bconv/fconv, PM_ACCURACY),
                         //      because it says which way the residual
                         //      leans -- the split between e=0 and e=+1 is
                         //      not the same at .1 as at .9.
                         //  7 = plane x fraction x signed eW+eN: the same
                         //      fraction against the direction the last two
                         //      neighbours missed in
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
#ifndef RES_FQ
#define RES_FQ       8   // buckets of the prediction's fraction, model 6
#endif
#ifndef RES_MF
#define RES_MF       0   // 1 = the mixer's weight set also depends on the
                         // prediction's fraction bucket: how much to trust
                         // each model may depend on whether the mix landed
                         // in the middle of a rounding interval or at its
                         // edge
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
#ifndef CT_ON
#define CT_ON        1   // reorder the colour components before coding.
                         // The raster is stored permuted: position j holds
                         // component ord[j].  Nothing else changes -- it is
                         // a permutation, so it is trivially invertible and
                         // the stream carries only the order.
                         //
                         // What it buys is entirely in the models: the mix's
                         // intra-pixel taps let position j see positions
                         // 0..j-1 of the same pixel (worth -9.6% on its own),
                         // and residual model 3 keys on the error just made
                         // on position j-1.  Both want the component that
                         // best explains the others to be coded FIRST.
#endif
#ifndef CT_METRIC
#define CT_METRIC    3   // how the order is chosen, over all n! of them:
                         //  0 = LMS on the raw values -- sum over positions
                         //      of Var(v_j | v_0..v_{j-1})
                         //  2 = the same in the gradient domain
                         //  3 = trial-code a sample of the raster under each
                         //      order and keep the cheapest
                         //
                         // 0 and 2 are cheap and both LOSE (+0.13%, +0.15%).
                         // They have to: reordering changes no plane's
                         // content, so any sum of conditional log variances
                         // is log det(Cov) -- identical for every order --
                         // and the LMS sum that does vary is measuring
                         // variance, while the order only matters through
                         // things variance cannot see (the mix's intra-pixel
                         // taps, and residual model 3 keying on the error
                         // just made on the previous position, which for
                         // position 0 has no previous).  Exhaustively coding
                         // t32 under all 24 orders spans 117353..126763, a
                         // 4.8% spread, and LMS picks 124436.  So measure it.
#endif
#ifndef CT_ROWS
#define CT_ROWS     32   // rows per sample band
#endif
#ifndef CT_SCR
#define CT_SCR       2   // screen band = full band / CT_SCR
#endif
#ifndef CT_KEEP
#define CT_KEEP      0   // Two-stage search: score every order on a short
                         // screen band, keep the best CT_KEEP, then score
                         // those on the full band.  n!*rows row-encodes
                         // become n!*rows/4 + CT_KEEP*rows, ~2.4x less work
                         // for the same answer -- a short band is unreliable
                         // for *picking* the winner (16 rows already picks
                         // the identity everywhere) but fine for throwing
                         // away the clearly bad ones.  0 = no screen, score
                         // every order on the full band -- the default,
                         // because the screen is a weak signal and pruning
                         // costs 0.12-0.37% for ~1.9x encode.  Set it to 8
                         // if encode time matters more than that.
#endif
#ifndef CT_FRAC
#define CT_FRAC     32   // sample about 1/CT_FRAC of the rows, but never
                         // less than one band -- so the trial cost is
                         // bounded by n! * CT_ROWS rows however big the
                         // image is.  A larger sample buys nothing here
                         // (1/8 and 1/32 pick the same order everywhere)
                         // and costs several times the encode.
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
#ifndef BLK_DEP
#define BLK_DEP      1   // code the four statistic planes under the exact
                         // constraints they place on each other:
                         //   max >= min
                         //   mask1 (always-1) is a subset of min & max
                         //   mask0 (always-0) is a subset of ~(min | max)
                         //     -- from which mask0 & mask1 == 0 follows
                         //   min == max  <=>  mask1 == ~mask0, and then all
                         //     four are the same byte, so the masks cost
                         //     nothing at all
                         // Same exclusion machinery as the raster: the bits
                         // those leave undetermined are the only ones coded.
#endif
#ifndef BLK_MASKS
#define BLK_MASKS    2   // the two mask planes:
                         //  0 = never coded (min/max only)
                         //  1 = always
                         //  2 = decided per image, one flagged byte.
                         // On photographs the masks pin down almost nothing
                         // that [min,max] has not already pinned down, and
                         // coding them loses.  On data with structure in the
                         // low bits -- quantised, palette-mapped, 16-bit
                         // halves -- they are the whole story.  The encoder
                         // can tell which it has: compare the bits they save
                         // on the raster against the bits they cost, which
                         // after the inter-plane constraints is exactly
                         // popcount(min&max) + popcount(~(min|max)).
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
#ifndef FASTM
#define FASTM     1      // fast exp/log in the counter; see sh_common.inc
#endif
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

#if RES_CTXW
// MRP's context pixels: every causal neighbour within distance 3, in
// distance order (mrp-05 common.c, dyx[] truncated to NUM_UPELS=12), and
// its weight round(64/distance) from init_ctx_weight.
static const int UPELS = 12;
static const int UD[UPELS][2] = { {0,-1},{-1,0},
                                  {0,-2},{-1,-1},{-2,0},{-1,1},
                                  {0,-3},{-1,-2},{-2,-1},{-3,0},{-2,1},{-1,2} };
static const int UW[UPELS]    = {   64,64,  32,45,32,45,  21,29,29,21,29,29 };
#endif
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

// Measure mode.  Everything above the arithmetic coder is unchanged -- the
// models predict and adapt exactly as they would -- but instead of emitting
// the bit we add its codelength to a counter.  That makes it possible to
// ask what a coding decision would actually cost without coding anything,
// which is the only honest way to choose between things the model reacts
// to.  Encoder side only, obviously: it needs to know the bit.
static uint   g_measure = 0;
static double g_bits    = 0.0;

static INLINE uint Process( uint p, uint bit ) {
  if( g_measure ) {
    // p is P(bit==0) scaled by SCALE, as rc_BProcess wants it
    double pr = double(bit ? (SCALE-p) : p) / double(SCALE);
    if( pr < 1e-9 ) pr = 1e-9;
    g_bits -= log2(pr);
    return bit;
  }
  return rc.rc_BProcess( p, bit );
}

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
    bit = Process( p, bit );
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
  INLINE void put( uint v ) { m[v>>6] |= qword(1)<<(v&63); }
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
static INLINE uint ctz64( qword v ) {
#if defined(__GNUC__) || defined(__clang__)
  return uint(__builtin_ctzll(v));
#else
  uint n=0; while( ((v>>n)&1)==0 ) n++; return n;
#endif
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

// MRP's range-aware error mapping (mrp-05/src/common.c, e2E/E2e), with
// [lo,hi] the block's range rather than the whole 0..255.  th is how far
// the prediction sits from the nearer bound: inside +-th both signs are
// possible and the fold is the plain zigzag, outside it only one sign is,
// and the tail continues the alphabet upwards instead of interleaving a
// bit that carries nothing.  sgn is that forced sign (1 = e>0), which is
// the side away from the nearer bound.
static INLINE uint e2E( int e, int th ) {
  uint a = uint(e<0 ? -e : e);
  if( a <= uint(th) ) return (e<0) ? (a+a-1) : (a+a);
  return uint(th) + a;                       // 2th + (a-th)
}
static INLINE int E2e( uint E, int th, int sgn ) {
  if( E <= uint(th+th) ) return (E&1) ? -int((E+1)>>1) : int(E>>1);
  int a = int(E) - th;
  return sgn ? a : -a;
}

// The four small sets the statistic planes are drawn from.
static INLINE void set_clear( VSet& S ) { S.m[0]=S.m[1]=S.m[2]=S.m[3]=0; }
static INLINE void set_lt( VSet& S, uint n ) {           // { v : v < n }
  for( uint i=0; i<4; i++ ) {
    if( (i+1)*64 <= n ) S.m[i] = ~qword(0);
    else if( i*64 >= n ) S.m[i] = 0;
    else S.m[i] = (qword(1) << (n-i*64)) - 1;
  }
}
static INLINE void set_one( VSet& S, uint v ) {
  set_clear(S); S.m[v>>6] = qword(1)<<(v&63);
}
static INLINE void set_ge( VSet& S, uint lo ) {          // { v : v >= lo }
  for( uint i=0; i<4; i++ ) {
    if( (i+1)*64 <= lo ) S.m[i] = 0;
    else if( i*64 >= lo ) S.m[i] = ~qword(0);
    else S.m[i] = ~qword(0) << (lo-i*64);
  }
}
static INLINE void set_sub( VSet& S, uint m ) {          // { v : v & ~m == 0 }
  set_clear(S);
  for( uint v=0; v<256; v++ ) if( (v&~m&255)==0 ) S.m[v>>6] |= qword(1)<<(v&63);
}
static INLINE void set_sup( VSet& S, uint m ) {          // { v : v & m == m }
  set_clear(S);
  for( uint v=0; v<256; v++ ) if( (v&m)==m ) S.m[v>>6] |= qword(1)<<(v&63);
}

// One mixer weight vector: RES_NM weights in the logistic domain.
// Config_BM has both momenta at zero, so as with the pixel mix (MIX_LEAN)
// the per-weight D and R are temporaries and the Newton denominator is
// shared -- which matters here because this runs per *bit*: it takes
// RES_NM divisions per bit down to one, and the struct from 4*RES_NM
// floats to RES_NM.
struct BitMix {
  float w[RES_NM];
  void Init() { for( uint i=0; i<RES_NM; i++ ) w[i] = BM_W0; }

  INLINE void Update( const float* s, float gdot, float hc ) {
    float R = clamp( clip( hc, Config_BM::grad2_clip ), 0.0f, Config_BM::R_clip )
            + Config_BM::inc;
    float k = Config_BM::NW / R;
    for( uint i=0; i<RES_NM; i++ ) {
      float g = clip( gdot*s[i], Config_BM::D_clip );
      w[i] = clamp( w[i] - clip( k*g, Config_BM::stepMax ),
                    Config_BM::minVal, Config_BM::maxVal );
    }
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
    // Padded to a whole AVX2 lane group and 32-byte aligned: Dot() and the
    // lean Update() load eight at a time.
    int na = (n_+7)&~7;
    w = (float*)aligned_alloc( 32, size_t(na)*sizeof(float) );
    for( int i=0; i<na; i++ ) w[i] = 0.0f;
    n = n_;
    u = 0;
#if MIX_LEAN && MIX_NEWTON && !W_LOGIT
    for( int i=0; i<n; i++ ) w[i] = (i==seed_tap) ? LW_W0 : 0.0f;
#else
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
#endif
  }

  // w comes from aligned_alloc, so it goes back through free(), not
  // delete[] -- the trial coder builds and tears down a Raster per order.
  void Free() {
    if( w ) { ::free(w); w = 0; }
    if( u ) { delete[] u; u = 0; }
  }

  INLINE float Dot( const float* restrict x ) const {
#if defined(__AVX2__)
    // Four accumulators so the FMA latency (4-5 cycles) is covered by
    // independent chains rather than serialised on one.
    __m256 a0 = _mm256_setzero_ps(), a1 = a0, a2 = a0, a3 = a0;
    int i = 0;
    for( ; i+32<=n; i+=32 ) {
      _mm_prefetch( (const char*)(w+i+64), _MM_HINT_T0 );
      a0 = _mm256_fmadd_ps( _mm256_load_ps(w+i   ), _mm256_load_ps(x+i   ), a0 );
      a1 = _mm256_fmadd_ps( _mm256_load_ps(w+i+ 8), _mm256_load_ps(x+i+ 8), a1 );
      a2 = _mm256_fmadd_ps( _mm256_load_ps(w+i+16), _mm256_load_ps(x+i+16), a2 );
      a3 = _mm256_fmadd_ps( _mm256_load_ps(w+i+24), _mm256_load_ps(x+i+24), a3 );
    }
    for( ; i+8<=n; i+=8 )
      a0 = _mm256_fmadd_ps( _mm256_load_ps(w+i), _mm256_load_ps(x+i), a0 );
    a0 = _mm256_add_ps( _mm256_add_ps(a0,a1), _mm256_add_ps(a2,a3) );
    __m128 h = _mm_add_ps( _mm256_castps256_ps128(a0), _mm256_extractf128_ps(a0,1) );
    h = _mm_add_ps( h, _mm_movehl_ps(h,h) );
    h = _mm_add_ss( h, _mm_shuffle_ps(h,h,1) );
    float s = _mm_cvtss_f32(h);
    for( ; i<n; i++ ) s += w[i]*x[i];
    return s;
#else
    float s = 0.0f;
    for( int i=0; i<n; i++ ) s += w[i]*x[i];
    return s;
#endif
  }

  // gdot = dL/d(dot), hdot = d2L/d(dot)2; the per-tap chain is
  // g_i = gdot*x_i, h_i = hdot*x_i^2 (Gauss-Newton diagonal).
  INLINE void Update( const float* restrict x, float gdot, float hdot, float xx ) {
#if MIX_NEWTON
    const float hc = hdot * xx;
#endif
#if MIX_LEAN && MIX_NEWTON && !W_LOGIT
    {
      // R is the same for every tap, so the Newton denominator leaves the
      // loop entirely and what is left is two clips and an FMA per tap.
      float R = clip( hc, cfg::grad2_clip );
      R = clamp( R, 0.0f, cfg::R_clip ) + cfg::inc;
      float k = cfg::NW / R;
#if defined(__AVX2__)
      const __m256 vg = _mm256_set1_ps(gdot);
      const __m256 vk = _mm256_set1_ps(k);
      const __m256 vD = _mm256_set1_ps(cfg::D_clip);
      const __m256 vS = _mm256_set1_ps(cfg::stepMax);
      const __m256 vlo = _mm256_set1_ps(cfg::minVal);
      const __m256 vhi = _mm256_set1_ps(cfg::maxVal);
      int i = 0;
      for( ; i+8<=n; i+=8 ) {
        __m256 g = _mm256_mul_ps( vg, _mm256_load_ps(x+i) );
        g = _mm256_min_ps( _mm256_max_ps( g, _mm256_sub_ps(_mm256_setzero_ps(),vD) ), vD );
        __m256 st = _mm256_mul_ps( vk, g );
        st = _mm256_min_ps( _mm256_max_ps( st, _mm256_sub_ps(_mm256_setzero_ps(),vS) ), vS );
        __m256 nw = _mm256_sub_ps( _mm256_load_ps(w+i), st );
        _mm256_store_ps( w+i, _mm256_min_ps( _mm256_max_ps(nw,vlo), vhi ) );
      }
      for( ; i<n; i++ ) {
        float g = clip( gdot*x[i], cfg::D_clip );
        w[i] = clamp( w[i] - clip( k*g, cfg::stepMax ), cfg::minVal, cfg::maxVal );
      }
#else
      for( int i=0; i<n; i++ ) {
        float g = clip( gdot*x[i], cfg::D_clip );
        w[i] = clamp( w[i] - clip( k*g, cfg::stepMax ), cfg::minVal, cfg::maxVal );
      }
#endif
      return;
    }
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
  // How this Raster's legal-value sets are obtained:
  //   0  no constraint
  //   1  the per-block table below (the raster)
  //   2  derived from this pixel's earlier components (the statistic
  //      picture, whose four planes constrain each other exactly)
  uint lmode, nc0;
  VSet lsc;                // scratch for mode 2
  uint blw, blh, nbx;      // log2 block size, blocks per row
  const VSet* lset;        // [block][component] legal value sets
  const byte* lrng;        // [block][component][2] its min and max
  const VSet* lg[MAXC];    // the sets in force at the current pixel
  const byte* lr[MAXC];
  int   rq[MAXC];          // its range bucket
#endif
#if RES_ZIGZAG==2
  int   e_th, e_sgn, e_rg; // the fold in force for the symbol being coded
#endif
#if RES_CTXW
  int   uoff[UPELS];       // UD[] in emap units, fixed once bstride is known
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
  int    qcls[MAXC];          // the activity bucket of the current pixel
#if RES_NM>6
  int    actq[MAXC];          // k*RES_CTX + activity bucket, for model 6
#endif
#if RES_NM>7
  int    eqc[MAXC];           // qsigned(eW+eN), for model 7
#endif
#if RES_MF
  int    mctxb[MAXC];         // mctx before the fraction is folded in
#endif
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
#if MIX_LOSS==4
  float  madc[MAXC*RES_CTX];  // MIX_LOSS==4: the same, per activity bucket
#endif
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

#if BLK_STAT
  // The legal set in force for component k of the pixel at p, and its
  // bounds.  Mode 2 builds it from the components already decoded into p:
  // plane 0 is min, 1 is max, 2 is the always-1 mask, 3 the always-0 mask.
  INLINE const VSet* CurSet( uint k, const byte* p, int& lo, int& hi ) {
    lo = 0; hi = 255;
    if( lmode==1 ) { lo = lr[k][0]; hi = lr[k][1]; return lg[k]; }
#if BLK_DEP
    if( lmode==2 ) {
      uint s = k/nc0, j = k%nc0;
      if( s==0 ) return 0;                       // min: unconstrained
      uint mn = p[j];
      if( s==1 ) { set_ge(lsc,mn); lo = int(mn); return &lsc; }
      uint mx = p[nc0+j];
      if( mn==mx ) { set_one(lsc,mn); lo = hi = int(mn); return &lsc; }
      if( s==2 ) { uint m = mn&mx;      set_sub(lsc,m); hi = int(m); }
      else       { uint m = (mn|mx)&255; set_sup(lsc,m); lo = int(m); }
      return &lsc;
    }
#endif
    return 0;
  }
#endif

  INLINE float Knee( uint k ) const {
#if MIX_LOSS==3
    return LW_HUBA * mad[k];
#elif MIX_LOSS==4
    return LW_HUBA * madc[k*RES_CTX + uint(qcls[k])];
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
    lset = 0; lrng = 0; lmode = 0; nc0 = 0;
#endif
    bstride = W + PADL + PADR;
    size_t bn = size_t(H+PADT) * bstride * nc;
    buf = new byte[bn];
    memset( buf, 128, bn );     // border and not-yet-known pixels

#if RES_CTXW
    for( int i=0; i<UPELS; i++ )
      uoff[i] = (UD[i][0]*int(bstride) + UD[i][1]) * int(nc);
#endif

    ni = NPIX*nc;
#if LPC_INTRA
    ni += nc-1;                 // components of this pixel already coded
#endif
#if MIX_BIAS==1
    ni += 1;
#endif
    // 32-byte aligned and padded: Dot/Update load it eight at a time
    x   = (float*)aligned_alloc( 32, size_t((ni+15)&~7)*sizeof(float) );
    for( int i=0; i<((ni+15)&~7); i++ ) x[i] = 0.0f;
    mix = new LPCMix[nc*MIX_CLS];
    // Seed each mix on "same component of the pixel to the left", which
    // is input (NPIX-1)*nc + k -- the last of the bottom-row group.
    // Without centring the mix has to reproduce the left neighbour itself,
    // so seed that tap at 1; with centring the offset already does it and
    // every tap seeds at 0.
    for( uint k=0; k<nc; k++ ) for( uint c=0; c<MIX_CLS; c++ )
      mix[k*MIX_CLS+c].Init( ni, MIX_CENTER ? -1 : int((NPIX-1)*nc + k) );
#if MIX_DUAL
    mix2 = new LPCMix2[nc*MIX_CLS];
    lam  = new ParamUpdater<1,Config_LM>[nc*MIX_LCLS];
    for( uint k=0; k<nc; k++ ) for( uint c=0; c<MIX_CLS; c++ )
      mix2[k*MIX_CLS+c].Init( ni, MIX_CENTER ? -1 : int((NPIX-1)*nc + k) );
    for( uint i=0; i<nc*MIX_LCLS; i++ )
      lam[i].Init( 0.0f );              // sigma(0) = 1/2, an even blend
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
#if RES_NM>6
    nrow[6] = nc*RES_CTX*RES_FQ;
#endif
#if RES_NM>7
    nrow[7] = nc*RES_FQ*RES_EQ;
#endif
    for( uint m=0; m<RES_NM; m++ ) {
      res[m] = new Ctr[256*nrow[m]];
      InitCtr( res[m], 256*nrow[m] );
    }
    emap = new signed char[bn];
    memset( emap, 0, bn );

    nmix = nc*RES_CTX*(BLK_MCTX?BLK_RQ:1)*(RES_MF?RES_FQ:1)*(RES_MD?8:1);
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
#if MIX_LOSS==4
    for( uint i=0; i<MAXC*RES_CTX; i++ ) madc[i] = LW_MAD0;
#endif

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
      const signed char* eo  = emap + o;
      const signed char* eW  = eo - size_t(nc);
      const signed char* eN  = eo - size_t(bstride)*nc;
      for( uint k=0; k<nc; k++ ) {
        int w = eW[k], n = eN[k];
#if RES_CTXW || RES_EQW
        int s = 0, sg = 0;
        for( int i=0; i<UPELS; i++ ) {
          int t = eo[uoff[i]+int(k)];
          s += UW[i]*(t<0?-t:t); sg += UW[i]*t;
        }
        (void)sg;
#endif
#if RES_CTXW
        uint a = uint(s>>(RES_CTXW==2?7:6));
#else
        int ne = eN[nc+k];
        int aw = w<0?-w:w, an = n<0?-n:n, ane = ne<0?-ne:ne;
        uint a = uint(aw+an+ane);
#endif
        uint q = 0; while( q+1 < RES_CTX && a >= (2u<<q) ) q++;
        rbase[0][k] = (RES_PLANE ? int(k) : 0) * 256;
#if RES_NM>1
        rbase[1][k] = int(k*RES_CTX + q) * 256;
#endif
#if RES_NM>2
#if RES_EQW
        rbase[2][k] = int(k*RES_EQ + uint(qsigned(sg>>6))) * 256;
#else
        rbase[2][k] = int(k*RES_EQ + uint(qsigned(w+n))) * 256;
#endif
#endif
#if RES_NM>4
        rbase[4][k] = (int(k)*RES_JQ*RES_JQ + qjoint(w)*RES_JQ + qjoint(n)) * 256;
#endif
#if RES_NM>6
        actq[k] = int(k*RES_CTX + q);
#endif
#if RES_NM>7
#if RES_EQW
        eqc[k] = qsigned(sg>>6);
#else
        eqc[k] = qsigned(w+n);
#endif
#endif
        qcls[k] = int(q);
#if BLK_STAT && BLK_MCTX
        mctx[k] = int((k*RES_CTX + q)*BLK_RQ + uint(rq[k]));
#else
        mctx[k] = int(k*RES_CTX + q);
#endif
#if RES_MF
        mctxb[k] = mctx[k];
#endif
        cerr[k] = 0;
      }
    }
    int i = 0;
    float s = 0.0f;
#if !MIX_LOGISTIC && defined(__AVX2__)
    // Fast path.  Two things the generic loop below does per input that it
    // does not need to: a `% nc` (an integer division, 240 of them per
    // pixel) and a 256-entry table lookup.  With linear inputs the map is
    // affine -- i2f[v] = v/128 - 1 -- so the byte converts straight to
    // float, and the operating point folds into the same FMA if the base
    // is pre-replicated over a lane group.  8*nc is a multiple of 8 for
    // every nc we support (8, 24, 32), so the row runs vectorise whole.
    {
      ALIGN(32) float bv[BW*MAXC];
      for( uint j=0; j<BW*nc; j++ ) bv[j] = 1.0f + base[j%nc];
      const __m256 vs = _mm256_set1_ps(1.0f/128.0f);
      __m256 acc = _mm256_setzero_ps();
      for( int dy=-PADT; dy<=0; dy++ ) {
        const byte* r = At( px-PADL, py+dy );
        uint nb = (dy<0) ? BW*nc : PADL*nc;
        _mm_prefetch( (const char*)(r+bstride*nc), _MM_HINT_T0 );
        uint j = 0;
        for( ; j+8<=nb; j+=8 ) {
          __m256i b8 = _mm256_cvtepu8_epi32( _mm_loadl_epi64((const __m128i*)(r+j)) );
          __m256  t  = _mm256_fmsub_ps( _mm256_cvtepi32_ps(b8), vs,
                                        _mm256_load_ps(bv+j) );
          _mm256_store_ps( x+i, t );
          acc = _mm256_fmadd_ps( t, t, acc );
          i += 8;
        }
        for( ; j<nb; j++ ) {           // only the 4*nc bottom-row tail
          float t = float(r[j])*(1.0f/128.0f) - bv[j];
          x[i++] = t; s += t*t;
        }
      }
      __m128 h = _mm_add_ps( _mm256_castps256_ps128(acc), _mm256_extractf128_ps(acc,1) );
      h = _mm_add_ps( h, _mm_movehl_ps(h,h) );
      h = _mm_add_ss( h, _mm_shuffle_ps(h,h,1) );
      s += _mm_cvtss_f32(h);
    }
#else
    {   // generic path: pre-replicate the base so there is no % in the loop
      ALIGN(32) float bg[BW*MAXC];
      for( uint j=0; j<BW*nc; j++ ) bg[j] = base[j%nc];
      for( int dy=-PADT; dy<=0; dy++ ) {
        const byte* r = At( px-PADL, py+dy );
        uint nb = (dy<0) ? BW*nc : PADL*nc;
        for( uint j=0; j<nb; j++ ) { float t = i2f[r[j]] - bg[j]; x[i++] = t; s += t*t; }
      }
    }
#endif
#if MIX_BIAS==1
    s += 1.0f;
#endif
    xx = s;
  }

  // One byte: predict, code the residual, update the mix.
  INLINE uint Byte( uint k, byte* p, uint v ) {
    // Which weight set, and which blend coordinate, this pixel's class
    // asks for.  Both fold the RES_CTX activity bucket down uniformly.
    const uint mk = (MIX_CLS >1) ? k*MIX_CLS  + (uint(qcls[k])*MIX_CLS )/RES_CTX : k;
    const uint lk = (MIX_LCLS>1) ? k*MIX_LCLS + (uint(qcls[k])*MIX_LCLS)/RES_CTX : k;
    (void)lk;
    LPCMix& m = mix[mk];
    float dotA = m.Dot(x);
    float dot  = dotA + base[k];
#if MIX_DUAL
    float dotB = mix2[mk].Dot(x);
    float lm   = squash( lam[lk].val );
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

    int _lo = 0, _hi = 255;
#if BLK_STAT
    const VSet* ls = lmode ? CurSet(k,p,_lo,_hi) : 0;
#if BLK_CLAMP
    // Nothing outside the legal range can occur, so neither should a
    // prediction.  Free, and it never hurts.
    if( ls ) { if( pb<_lo ) pb = _lo; else if( pb>_hi ) pb = _hi; }
#endif
#endif
#if RES_ZIGZAG==2
    // The fold needs the same bounds the decoder will have; when the block
    // statistics are off or say nothing they are simply 0..255, and e2E
    // degenerates to MRP's own full-range form.
#if !BLK_CLAMP
    if( pb<_lo ) pb = _lo; else if( pb>_hi ) pb = _hi;
#endif
    { int dl = pb-_lo, dh = _hi-pb;
      e_th  = dl<dh ? dl : dh;
      e_sgn = int(dl<=dh);           // the tail lies away from the near bound
      e_rg  = _hi-_lo; }
#endif

    vctx[k] = int((uint(pb)*vq)>>8);
#if RES_NM>6 || RES_MF
    // pb is int(f) with the rounding half already folded into f, so f-pb is
    // where inside the rounding interval the mix actually landed.
    { int fq = int( (f - float(pb)) * float(RES_FQ) );
      if( fq<0 ) fq = 0; else if( fq>=RES_FQ ) fq = RES_FQ-1;
#if RES_NM>6
      rbase[6][k] = (actq[k]*RES_FQ + fq) * 256;
#endif
#if RES_NM>7
      rbase[7][k] = ((int(k)*RES_FQ + fq)*RES_EQ + eqc[k]) * 256;
#endif
#if RES_MF
      mctx[k] = mctxb[k]*RES_FQ + fq;
#endif
    }
#endif

#if RES_ZIGZAG==2
    uint r = e2E( int(v)-pb, e_th );
#else
    uint r = (v - uint(pb)) & 255;
#if RES_ZIGZAG
    int e = int(r); if( e>=128 ) e -= 256;
    r = (e<0) ? uint(-e-e-1) : uint(e+e);
#endif
#endif
#if STATS
    s_h[r&255]++;
#endif
#if BLK_STAT
    r = CodeRes( k, r, uint(pb), ls );
#else
    r = CodeRes( k, r, uint(pb) );
#endif
#if RES_ZIGZAG==2
    v = uint(pb + E2e( r, e_th, e_sgn ));
#else
#if RES_ZIGZAG
    e = (r&1) ? -int((r+1)>>1) : int(r>>1);
    r = uint(e) & 255;
#endif
    v = (uint(pb) + r) & 255;
#endif

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
    mix2[mk].Update( x, LossG(dotB+base[k]-t, LW_HUB2), 1.0f, xx );
    {
      float f1 = lm*(1.0f-lm);            // dL/dz = gdot*dd*f1
      float u1 = dd*f1;
      float gz = LossG(ew, Knee(k)) * u1;
      float hz = u1*u1 + LossG(ew,Knee(k))*dd*f1*(1.0f-2.0f*lm);
      lam[lk].AccumGH( gz, hz );
      lam[lk].Apply( lam[lk].StepRaw(), Config_LM::minVal, Config_LM::maxVal );
    }
#else
    m.Update( x, LossG(ew, Knee(k)), 1.0f, xx );
#endif
    mad[k] += (fabsf(ew)-mad[k]) * MAD_R;
#if MIX_LOSS==4
    { float& mc = madc[k*RES_CTX + uint(qcls[k])];
      mc += (fabsf(ew)-mc) * MAD_R; }
#endif
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
#if BLK_STAT
  INLINE uint CodeRes( uint k, uint c, uint pb, const VSet* lv ) {
#else
  INLINE uint CodeRes( uint k, uint c, uint pb ) {
#endif
    uint cxt, bit;
#if BLK_STAT
    // Carry the legal VALUE set into the alphabet this tree codes: first
    // r = (v-pb)&255, then the zigzag fold if it is on.  A subtree holding
    // no legal symbol is not a choice, so its bit is not coded.
    VSet ls; uint nls = 256;
    if( lv ) {
#if RES_ZIGZAG==2
      // e2E maps [lo,hi] onto [0,hi-lo].  When the block's set IS the whole
      // range -- which it is unless the mask planes cut into it -- the image
      // is the prefix [0,rg] and needs no per-symbol work at all.  When it
      // is not, walk the set bits (30-odd of them, not 256).
      nls = lv->count();
      if( nls == uint(e_rg)+1 ) set_lt( ls, nls );
      else {
        set_clear( ls );
        for( uint i=0; i<4; i++ )
          for( qword w=lv->m[i]; w; w&=w-1 )
            ls.put( e2E( int(i*64 + uint(ctz64(w))) - int(pb), e_th ) );
      }
#else
      VSet t; rotr256( t, *lv, pb );
#if RES_ZIGZAG
      zigzagset( ls, t );
#else
      ls = t;
#endif
      nls = ls.count();
#endif
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
#if RES_NM>6
    Ctr* m6 = res[6] + rbase[6][k];
#endif
#if RES_NM>7
    Ctr* m7 = res[7] + rbase[7][k];
#endif
    BitMix* mxb = bmix + size_t(mctx[k])*(RES_MD?8:1);
#if RES_SSE
    Ctr* ap = sse + size_t(k)*256*(SSEQ+1);
#endif

    // The tree walks node 1, then 2 or 3, then 4..7.  Those first three are
    // known before the walk starts and live in six different tables, up to
    // 25 MB apart, so pull them in now rather than stalling on each in turn.
    // Deeper nodes depend on bits not yet coded and are left alone.
#if defined(__SSE2__)
    #define PF3(m) do { _mm_prefetch((const char*)((m)+1),_MM_HINT_T0); \
                        _mm_prefetch((const char*)((m)+2),_MM_HINT_T0); \
                        _mm_prefetch((const char*)((m)+3),_MM_HINT_T0); } while(0)
    PF3(m0);
#if RES_NM>1
    PF3(m1);
#endif
#if RES_NM>2
    PF3(m2);
#endif
#if RES_NM>3
    PF3(m3);
#endif
#if RES_NM>4
    PF3(m4);
#endif
#if RES_NM>5
    PF3(m5);
#endif
#if RES_NM>6
    PF3(m6);
#endif
#if RES_NM>7
    PF3(m7);
#endif
    #undef PF3
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
#if RES_NM>6
      m6[cxt].Predict();            s[6] = stp( m6[cxt].pK );
#endif
#if RES_NM>7
      m7[cxt].Predict();            s[7] = stp( m7[cxt].pK );
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

      bit = Process( pi, bit );

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
#if RES_NM>6
      m6[cxt].C_Update( bit );
#endif
#if RES_NM>7
      m7[cxt].C_Update( bit );
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
        mx.Update( s, gdot, hc );
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

  void Free() {
    for( uint k=0; k<nc*MIX_CLS; k++ ) mix[k].Free();
#if MIX_DUAL
    for( uint k=0; k<nc*MIX_CLS; k++ ) mix2[k].Free();
#endif
    delete[] buf; ::free(x); delete[] mix; delete[] emap;
    delete[] bmix; delete[] padm;
    for( uint m=0; m<RES_NM; m++ ) delete[] res[m];
#if MIX_DUAL
    delete[] mix2; delete[] lam;
#endif
#if RES_SSE
    delete[] sse;
#endif
#if MIX_BIAS==2
    delete[] bias;
#endif
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
// Colour component order
//
// One permutation for the whole image, chosen by enumerating all n! of
// them and scoring each by LMS: the sum over coding positions of the
// residual variance of that component given the ones before it.  Least
// squares needs only the (n+1)x(n+1) matrix of second moments, so one pass
// over the raster serves every order and every position.
// -------------------------------------------------------------
#if CT_ON

struct ColorOrder {
  static const uint CTC = 4;     // components this can order
  uint nc, on;
  byte ord[MAXC];                // ord[j] = original component coded j-th

  void Reset( uint nc_ ) {
    nc = nc_; on = 0;
    for( uint j=0; j<nc; j++ ) ord[j] = byte(j);
  }
  INLINE uint N() const { return nc>CTC ? CTC : nc; }

  // The permutation itself.  n, not nc, bounds the loops: left to infer
  // `nc <= 4` from the size of a local int[4], clang at -Ofast
  // -march=haswell -flto vectorises the q[ord[j]] gather into something
  // that returns garbage in lane 0.  The bound is real, so state it.
  void Forward( byte* img, uint W, uint H, uint stride ) const {
    if( !on ) return;
    const uint n = N();
    for( uint y=0; y<H; y++ ) {
      byte* q = img + size_t(y)*stride;
      for( uint x=0; x<W; x++, q+=nc ) {
        int v[MAXC]; const byte* o = ord;
        for( uint j=0; j<n; j++ ) v[j] = q[o[j]];
        for( uint j=0; j<n; j++ ) q[j] = byte(v[j]);
      }
    }
  }
  void Inverse( byte* img, uint W, uint H, uint stride ) const {
    if( !on ) return;
    const uint n = N();
    for( uint y=0; y<H; y++ ) {
      byte* q = img + size_t(y)*stride;
      for( uint x=0; x<W; x++, q+=nc ) {
        int v[MAXC]; const byte* o = ord;
        for( uint j=0; j<n; j++ ) v[j] = q[j];
        for( uint j=0; j<n; j++ ) q[o[j]] = byte(v[j]);
      }
    }
  }

  // M[a][b] = sum of x_a*x_b, index nc standing for the constant 1.  x is
  // the pixel value, or its horizontal/vertical difference when CT_METRIC
  // is 2 -- the codec codes a spatial residual, so which component best
  // explains the others is a question about their *gradients*.
  static void Moments( const byte* img, uint W, uint H, uint stride, uint nc,
                       double M[5][5] ) {
    for( uint a=0; a<=nc; a++ ) for( uint b=0; b<=nc; b++ ) M[a][b] = 0.0;
    double n = 0.0;
#if CT_METRIC==2
    uint ylim = (H>1) ? H-1 : 0;                 // needs the row below
#else
    uint ylim = H;
#endif
    for( uint y=0; y<ylim; y++ ) {
      const byte* q = img + size_t(y)*stride;
      const byte* r = q + stride;
      for( uint x=0; x<W; x++, q+=nc, r+=nc ) {
        double x0[4];
#if CT_METRIC==2
        if( x==0 ) continue;                     // needs the pixel to the left
        for( uint a=0; a<nc; a++ ) {
          int dx = int(q[a]) - int((q-nc)[a]);   // horizontal difference
          int dy = int(r[a]) - int(q[a]);        // vertical difference
          x0[a] = double(dx + dy);
        }
#else
        for( uint a=0; a<nc; a++ ) x0[a] = q[a];
#endif
        for( uint a=0; a<nc; a++ ) {
          for( uint b=a; b<nc; b++ ) M[a][b] += x0[a]*x0[b];
          M[a][nc] += x0[a];          // upper triangle only; the mirror
        }                             // below would otherwise wipe these
        n += 1.0;
      }
    }
    M[nc][nc] = n;
    for( uint a=0; a<=nc; a++ ) for( uint b=0; b<a; b++ ) M[a][b] = M[b][a];
  }

  // residual variance of component o[j] on o[0..j-1] plus a constant
  static double Solve( const double M[5][5], uint nc, const byte* o, uint j ) {
    uint n = j+1;
    double A[5][6];
    for( uint a=0; a<n; a++ ) {
      uint ia = (a<j) ? o[a] : nc;
      for( uint b=0; b<n; b++ ) A[a][b] = M[ia][(b<j)?o[b]:nc];
      A[a][n] = M[ia][o[j]];
    }
    for( uint c=0; c<n; c++ ) {
      uint p = c; double best = fabs(A[c][c]);
      for( uint r=c+1; r<n; r++ ) if( fabs(A[r][c])>best ) { best=fabs(A[r][c]); p=r; }
      if( best < 1e-6 ) { for( uint a=0; a<n; a++ ) A[a][n] = 0.0; break; }
      if( p!=c ) for( uint b=c; b<=n; b++ ) { double t=A[c][b]; A[c][b]=A[p][b]; A[p][b]=t; }
      for( uint r=0; r<n; r++ ) if( r!=c ) {
        double f = A[r][c]/A[c][c];
        for( uint b=c; b<=n; b++ ) A[r][b] -= f*A[c][b];
      }
    }
    double sse = M[o[j]][o[j]];
    for( uint a=0; a<n; a++ ) {
      double v = (fabs(A[a][a])<1e-12) ? 0.0 : A[a][n]/A[a][a];
      sse -= v * ((a<j) ? M[o[a]][o[j]] : M[nc][o[j]]);
    }
    return sse<0.0 ? 0.0 : sse;
  }

#if CT_METRIC==3
  // Cost of coding `sub` (W x rows, no padding) under the order in ord[],
  // in bits, with a model that starts cold exactly as the real one does.
  static double Trial( const byte* sub, uint W, uint rows, uint nc,
                       const byte* o );

  // Pick the order by trial coding a sample.  The sample is contiguous
  // bands, not scattered rows: the mix reaches seven rows up, so scattered
  // rows would measure a model that never sees a vertical neighbour.
  void Search( const byte* img, uint W, uint H, uint stride ) {
    uint band = CT_ROWS, want = H/CT_FRAC;
    if( want < band ) want = (H<band) ? H : band;
    uint nb = (want + band - 1)/band; if( nb<1 ) nb = 1;
    uint rows = 0;
    byte* sub = new byte[size_t(nb)*band*W*nc];
    for( uint b=0; b<nb; b++ ) {
      uint y0 = (H>band) ? uint((qword(b)*(H-band))/((nb>1)?(nb-1):1)) : 0;
      for( uint k=0; k<band && y0+k<H; k++ )
        memcpy( sub + size_t(rows++)*W*nc, img + size_t(y0+k)*stride, W*nc );
    }

    byte perm[MAXC], best[MAXC];
    for( uint i=0; i<nc; i++ ) best[i] = byte(i);
    double bestcost = 1e300, identcost = 1e300;
    uint fact = 1; for( uint i=2; i<=nc; i++ ) fact *= i;
    uint nkeep = (CT_KEEP<=0 || CT_KEEP>=int(fact)) ? fact : uint(CT_KEEP);
    uint screen = (nkeep<fact && rows>16) ? (rows/CT_SCR) : rows;
    double cand[24], cscr[24]; byte pv[24*MAXC];
    for( uint p=0; p<fact; p++ ) {
      byte pool[MAXC]; uint m = p;
      for( uint i=0; i<nc; i++ ) pool[i] = byte(i);
      for( uint i=0, left=nc; i<nc; i++, left-- ) {
        uint d = m % left; m /= left;
        perm[i] = pool[d];
        for( uint t=d; t+1<left; t++ ) pool[t] = pool[t+1];
      }
      cand[p] = cscr[p] = Trial( sub, W, screen, nc, perm );
      for( uint i=0; i<nc; i++ ) pv[p*MAXC+i] = perm[i];
    }
    // keep the CT_KEEP cheapest of the screen, always including the identity
    uint keep[24], nk = 0;
    for( uint t=0; t<fact && nk<nkeep; t++ ) {
      uint bi = 0; double bc = 1e300;
      for( uint p=0; p<fact; p++ ) if( cand[p]<bc ) { bc=cand[p]; bi=p; }
      cand[bi] = 1e300; keep[nk++] = bi;
    }
    int has0 = 0;
    for( uint t=0; t<nk; t++ ) if( keep[t]==0 ) has0 = 1;
    if( !has0 ) keep[nk++] = 0;

    for( uint t=0; t<nk; t++ ) {
      uint p = keep[t];
      for( uint i=0; i<nc; i++ ) perm[i] = pv[p*MAXC+i];
      double c = (screen==rows) ? cscr[p] : Trial( sub, W, rows, nc, perm );
      if( p==0 ) identcost = c;                  // permutation 0 is identity
      if( c < bestcost ) { bestcost = c; for( uint i=0; i<nc; i++ ) best[i]=perm[i]; }
    }
    delete[] sub;
    // Only move off the identity for a margin.  The sample is one band; a
    // real win is 4-5% and shows up on any band, while a tie decided by
    // sampling noise costs a couple of tenths on the whole file.
    if( !(bestcost < identcost*(double(B0_CTM)/1024.0)) )
      for( uint i=0; i<nc; i++ ) best[i] = byte(i);
    for( uint i=0; i<nc; i++ ) ord[i] = best[i];
  }
#endif

  void Fit( const byte* img, uint W, uint H, uint stride, uint nc_ ) {
    Reset(nc_);
    if( nc<2 || nc>CTC ) return;
#ifdef CT_FORCE
    {   // experiment: pin the order to the CT_FORCE'th permutation
      byte pool[MAXC]; uint m = CT_FORCE;
      for( uint i=0; i<nc; i++ ) pool[i] = byte(i);
      for( uint i=0, left=nc; i<nc; i++, left-- ) {
        uint d = m % left; m /= left;
        ord[i] = pool[d];
        for( uint t=d; t+1<left; t++ ) pool[t] = pool[t+1];
      }
      on = 1;
      return;
    }
#endif
#if CT_METRIC==3
    Search( img, W, H, stride );
    on = 1;
    for( uint i=0; i<nc; i++ ) if( ord[i]!=byte(i) ) return;
    on = 0;
    return;
#else
    double M[5][5]; Moments( img, W, H, stride, nc, M );
    double N = M[nc][nc];
    if( N<16.0 ) return;

    byte perm[MAXC], best[MAXC];
    for( uint i=0; i<nc; i++ ) best[i] = byte(i);
    double bestcost = 1e300;
    uint fact = 1; for( uint i=2; i<=nc; i++ ) fact *= i;
    for( uint p=0; p<fact; p++ ) {
      byte pool[MAXC]; uint m = p;                 // factorial number system
      for( uint i=0; i<nc; i++ ) pool[i] = byte(i);
      for( uint i=0, left=nc; i<nc; i++, left-- ) {
        uint d = m % left; m /= left;
        perm[i] = pool[d];
        for( uint t=d; t+1<left; t++ ) pool[t] = pool[t+1];
      }
      double cost = M[perm[0]][perm[0]] - M[nc][perm[0]]*M[nc][perm[0]]/N;
      for( uint j=1; j<nc; j++ ) cost += Solve( M, nc, perm, j );
      if( cost < bestcost ) { bestcost = cost; for( uint i=0; i<nc; i++ ) best[i]=perm[i]; }
    }
    for( uint i=0; i<nc; i++ ) ord[i] = best[i];
    on = 1;
    for( uint i=0; i<nc; i++ ) if( ord[i]!=byte(i) ) return;
    on = 0;                                        // identity: say nothing
#endif
  }

  void Code( void ) {
    on = CodeByte( o1[1], on ) & 1;
    if( !on ) { Reset(nc); return; }
    for( uint j=0; j<nc; j++ ) ord[j] = byte( CodeByte( o1[2], ord[j] ) % nc );
  }
};
#endif

#if CT_ON && CT_METRIC==3
double ColorOrder::Trial( const byte* sub, uint W, uint rows, uint nc,
                          const byte* o ) {
  uint sz = W*nc;
  byte* t = new byte[size_t(rows)*sz];
  for( uint y=0; y<rows; y++ ) {
    const byte* q = sub + size_t(y)*sz;
    byte* d = t + size_t(y)*sz;
    for( uint x=0; x<W; x++, q+=nc, d+=nc )
      for( uint j=0; j<nc; j++ ) d[j] = q[o[j]];
  }
  Raster* r = new Raster;
  r->Init( W, rows, nc, sz );
  double save = g_bits; uint sm = g_measure;
  g_measure = 1; g_bits = 0.0;
  r->Code( t );
  double cost = g_bits;
  g_measure = sm; g_bits = save;
  r->Free(); delete r; delete[] t;
  return cost;
}
#endif

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
  uint nst;                // 2 or 4 planes -- see BLK_MASKS
  byte* st;                // [nb][NST][nc] -- the planes, as a picture
  VSet* set;               // [nb][nc]    -- derived legal value sets
  byte* rng;               // [nb][nc][2] -- and their min/max

  void Init( uint W, uint H, uint nc_ ) {
    blw = blkl(B0_BLKLW); blh = blkl(B0_BLKLH);
    nc = nc_; nst = (BLK_MASKS==0) ? 2 : 4;
    nbx = (W + (1u<<blw) - 1) >> blw;
    nby = (H + (1u<<blh) - 1) >> blh;
    nb  = nbx*nby;
    st  = new byte[size_t(nb)*nst*nc];
    memset( st, 0, size_t(nb)*nst*nc );
    set = new VSet[size_t(nb)*nc];
    rng = new byte[size_t(nb)*nc*2];
  }

  // encoder side: measure the raster
  void Scan( const byte* img, uint W, uint H, uint stride ) {
    for( size_t i=0; i<size_t(nb)*nc; i++ ) {
      st[(i/nc)*nst*nc + 0*nc + i%nc] = 255;   // min
      st[(i/nc)*nst*nc + 1*nc + i%nc] = 0;     // max
      if( nst>2 ) {
        st[(i/nc)*nst*nc + 2*nc + i%nc] = 255;   // always-1 mask
        st[(i/nc)*nst*nc + 3*nc + i%nc] = 0;     // always-0 mask, as its OR
      }
    }
    for( uint y=0; y<H; y++ ) {
      const byte* q = img + size_t(y)*stride;
      byte* b = st + (size_t(y>>blh)*nbx)*nst*nc;
      for( uint x=0; x<W; x++ ) {
        byte* c = b + size_t(x>>blw)*nst*nc;
        for( uint k=0; k<nc; k++ ) {
          uint v = q[x*nc+k];
          if( v < c[0*nc+k] ) c[0*nc+k] = byte(v);
          if( v > c[1*nc+k] ) c[1*nc+k] = byte(v);
          if( nst>2 ) { c[2*nc+k] &= byte(v); c[3*nc+k] |= byte(v); }
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
      byte* c = st + b*nst*nc;
      for( uint k=0; k<nc; k++ ) {
        uint mn = c[0*nc+k], mx = c[1*nc+k];
        uint am = (nst>2) ? c[2*nc+k] : 0, om = (nst>2) ? c[3*nc+k] : 255;
        // an empty block (the image is not a whole number of blocks) has
        // min=255,max=0; that survives the round trip as max-min = 1.
        if( mn>mx ) { c[1*nc+k]=byte((mx-mn)&255); if(nst>2){c[2*nc+k]=0;c[3*nc+k]=0;} continue; }
        c[1*nc+k] = byte(mx-mn);
        if( nst>2 ) {
          c[2*nc+k] = byte((mn&mx) & ~am);
          c[3*nc+k] = byte(om & ~(mn|mx));
        }
      }
    }
  }
  void Unpack() {
    for( size_t b=0; b<nb; b++ ) {
      byte* c = st + b*nst*nc;
      for( uint k=0; k<nc; k++ ) {
        uint mn = c[0*nc+k], mx = (mn + c[1*nc+k]) & 255;
        c[1*nc+k] = byte(mx);
        if( nst>2 ) {
          c[2*nc+k] = byte((mn&mx) & ~c[2*nc+k]);
          c[3*nc+k] = byte((mn|mx) |  c[3*nc+k]);
        }
      }
    }
  }

  // Do the mask planes pay?  Their benefit is the alphabet they remove
  // from every pixel of the block; their cost, after the inter-plane
  // constraints, is exactly the bits those constraints leave free.  Both
  // are countable here, so the encoder decides and flags it.
  int MasksPay( uint W, uint H ) const {
    double gain = 0.0, cost = 0.0;
    for( size_t b=0; b<nb; b++ ) {
      uint bx = uint(b)%nbx, by = uint(b)/nbx;
      uint w = ((bx+1)<<blw)<=W ? (1u<<blw) : W-(bx<<blw);
      uint h = ((by+1)<<blh)<=H ? (1u<<blh) : H-(by<<blh);
      const byte* c = st + b*nst*nc;
      for( uint k=0; k<nc; k++ ) {
        uint mn = c[0*nc+k], mx = c[1*nc+k], am = c[2*nc+k], om = c[3*nc+k];
        if( mn>mx ) continue;
        uint n = 0;
        for( uint v=mn; v<=mx; v++ ) if( (v&am)==am && (v&~om&255)==0 ) n++;
        if( n==0 ) continue;
        gain += double(w)*double(h) * log2( double(mx-mn+1)/double(n) );
        uint fb = 0;
        for( uint t=(mn&mx);        t; t&=t-1 ) fb++;
        for( uint t=(~(mn|mx))&255; t; t&=t-1 ) fb++;
        cost += fb;
      }
    }
    return gain > cost;
  }

  // Dropping the mask planes changes the picture's layout, so the encoder
  // has to compact what Scan() filled in at four planes.  (The decoder
  // never sees the wide form: the picture is decoded at the narrow stride.)
  void Shrink() {
    for( size_t b=0; b<nb; b++ ) memmove( st + b*2*nc, st + b*4*nc, 2*nc );
    nst = 2;
  }

  // both sides: st -> the legal sets.  A block with no pixels in it (the
  // image is not a whole number of blocks) keeps min=255/max=0, which
  // yields the empty set; give those every value so nothing is excluded.
  void Build() {
    for( size_t b=0; b<nb; b++ ) for( uint k=0; k<nc; k++ ) {
      const byte* c = st + b*nst*nc;
      uint mn = c[0*nc+k], mx = c[1*nc+k];
      uint am = (nst>2) ? c[2*nc+k] : 0, om = (nst>2) ? c[3*nc+k] : 255;
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

#if CT_ON
    static ColorOrder ct;
    ct.Reset( b.nc );
    if( f_DEC==0 ) ct.Fit( img, b.W, b.H, b.stride, b.nc );
    ct.Code();
    if( f_DEC==0 ) ct.Forward( img, b.W, b.H, b.stride );
#endif

    static Raster ras;
    ras.Init( b.W, b.H, b.nc, b.stride );

#if BLK_STAT
    // the four statistic planes, as one picture of 4*n_colors components,
    // coded by its own Raster ahead of the raster it describes
    static BlkStats bs;
    bs.Init( b.W, b.H, b.nc );
    if( f_DEC==0 ) bs.Scan( img, b.W, b.H, b.stride );
#if BLK_MASKS==2
    {   // one flagged byte says whether the mask planes are in the stream
      uint fl = (f_DEC==0) ? uint(bs.MasksPay(b.W,b.H)) : 0;
      fl = CodeByte( o1[0], fl ) & 1;
      if( fl==0 ) { if( f_DEC==0 ) bs.Shrink(); else bs.nst = 2; }
    }
#endif
    if( f_DEC==0 && BLK_PACK ) bs.Pack();
    {
      static Raster sras;
      uint nc2 = bs.nst*b.nc;
      sras.Init( bs.nbx, bs.nby, nc2, bs.nbx*nc2, BLK_SVQ );
      sras.lmode = BLK_DEP ? 2 : 0;
      sras.nc0   = b.nc;
      sras.Code( bs.st );
    }
    if( BLK_PACK ) bs.Unpack();
    bs.Build();
    ras.lset = bs.set; ras.lrng = bs.rng; ras.lmode = 1;
    ras.blw = bs.blw;  ras.blh = bs.blh;  ras.nbx = bs.nbx;
#endif

    ras.Code( img );
#if CT_ON
    if( f_DEC ) ct.Inverse( img, b.W, b.H, b.stride );
#endif
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
