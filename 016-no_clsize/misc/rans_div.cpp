//  rans_div -- an exhaustive proof, by enumeration, that rans.inc's float32
//  division is exact for every frequency the model can produce.
//
//      clang++ -O2 -std=c++17 -I../../Lib3 misc/rans_div.cpp -o rans_div && ./rans_div
//
//  t_matrix.sh runs it. Takes the same -D set as the coder, so a build that
//  moves RC_RANS_KLOG, SCALElog or RANS_BIAS is checked at its own settings.
//
//  What is being checked
//  ---------------------
//  rans.inc's encoder wants q = floor(x_post / f), where x_post is the state
//  after renormalisation. It computes it as
//
//      rcp = fl( RANS_BIAS / f )                 once per symbol, off the chain
//      v   = fl( fl(x_pre) * rcp ) * 2^-sh       the scaling is exact
//      q   = (uint)(int) v                       truncation
//
//  and then corrects q by at most one, upwards, with a masked add. For that
//  correction to be sufficient the truncation has to be one-sided:
//
//      (A) v is NEVER above x_post/f      -- else the correction makes it worse,
//                                            and the remainder wraps negative
//      (B) v is NEVER more than 1 below   -- else one correction is not enough
//
//  Both reduce to a condition on the single quantity f*rcp, which is why this
//  can be exhaustive rather than sampled. Writing u = 2^-24 for float32's unit
//  roundoff, X = x_post/f for the true quotient and using X < 2^XSH:
//
//      (A)  fl(fl(x)*rcp) <= x*rcp*(1+u)^2 < x/f   <==  f*rcp < 1/(1+u)^2
//      (B)  fl(fl(x)*rcp) >= x*rcp*(1-u)^2,  and
//           X - X*f*rcp*(1-u)^2 < 1 for all X < 2^XSH
//                                                  <==  f*rcp > (1-2^-XSH)/(1-u)^2
//
//  So for every f in [1, SCALE-1] the program computes f*rcp exactly -- rcp has
//  24 significant bits and f at most 15, so the product is exact in double --
//  and checks it against that window. Nothing about x is sampled: the two
//  bounds above quantify over every x the coder can reach, including the ones
//  where fl(x) rounds up (x is up to 2^31 and float32 has 24 bits, so it does).
//
//  The window is not tight by accident. RANS_BIAS = 1-2^-19 sits about 32x
//  inside the (A) edge and about 500x inside the (B) edge, so the check also
//  reports the margins -- a bias that drifted towards either end would still
//  pass, and the margins are what says by how much.

#include <stdio.h>
#include <math.h>

typedef unsigned int uint;

#include "../rc_config.inc"

#ifndef RANS_BIAS
#define RANS_BIAS 0.99999809265f     /* keep in step with model.inc */
#endif

int main( void ) {
  const int    XSH = RC_RANS_KLOG + 8;
  const double u   = 1.0/16777216.0;                     // 2^-24
  const double hi  = 1.0/((1.0+u)*(1.0+u));              // f*rcp must be below
  const double lo  = (1.0 - ldexp(1.0,-XSH))/((1.0-u)*(1.0-u));  // ... and above

  double worst_hi = 0, worst_lo = 1;
  uint   at_hi = 0, at_lo = 0, bad = 0;

  for( uint f=1; f<SCALE; f++ ) {
    const float  rcp = float(RANS_BIAS) / float(int(f));
    const double fr  = double(f) * double(rcp);          // exact: 15 + 24 bits
    if( fr >= hi || fr <= lo ) {
      if( bad<8 ) printf( "  f=%-6u f*rcp=%.17g  outside (%.17g, %.17g)\n", f, fr, lo, hi );
      bad++;
    }
    if( fr > worst_hi ) { worst_hi = fr; at_hi = f; }
    if( fr < worst_lo ) { worst_lo = fr; at_lo = f; }
  }

  printf( "rans_div: SCALElog=%i RC_RANS_KLOG=%i XSH=%i bias=%.9g\n",
          int(SCALElog), int(RC_RANS_KLOG), XSH, double(RANS_BIAS) );
  printf( "  window          ( %.12f , %.12f )\n", lo, hi );
  printf( "  closest to hi     %.12f at f=%u   (margin %.3gx of 1-f*rcp)\n",
          worst_hi, at_hi, (1.0-worst_hi)/(1.0-hi) );
  printf( "  closest to lo     %.12f at f=%u   (margin %.3gx of 1-f*rcp)\n",
          worst_lo, at_lo, (1.0-lo)/(1.0-worst_lo) );

  if( bad ) { printf( "  FAILED: %u of %u frequencies\n", bad, uint(SCALE-1) ); return 1; }
  printf( "  ok: all %u frequencies give a one-sided truncation within 1\n", uint(SCALE-1) );
  return 0;
}
