// test_logistic.cpp -- standalone check of the xad_logistic.inc squash/stretch
// table construction ported to paq8hp's constants.  Builds nothing from the
// model; the point is to prove the mapping is right BEFORE it goes anywhere near
// paq8hpc.
//
//   g++ -O2 -o test_logistic test_logistic.cpp -lm && ./test_logistic
//
// paq8hp's own pair is:
//   squash(d)  33-node table + linear interpolation, d in [-2047,2047] -> [0,4095]
//   stretch(p) short[4096], built by inverting that squash
// so squash is a piecewise-linear approximation of the logistic on a 128-wide
// grid, and stretch inherits whatever squash got wrong.  The ported version
// computes stretch exactly (integer log2 + one multiply) and then finds squash as
// its inverse by binary search plus one linear interpolation, so the two agree
// with each other and with the real logistic.
//
// Constants: paq8hp is P_ONE=4096, ST_SCALE=256 -- the same shape as xadpcm's
// shipped configuration, which is why the arithmetic ports unchanged.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned long long u64;
typedef long long      i64;

static int pclamp(int v, int lo, int hi) { return v<lo ? lo : v>hi ? hi : v; }

enum {
  P_BITS   = 12,
  P_ONE    = 1<<P_BITS,   // 4096
  P_MID    = P_ONE/2,     // 2048
  P_MIN    = 1,
  P_MAX    = P_ONE-1,     // 4095
  ST_SCALE = 256,
  ST_MIN   = -2047,       // paq8hp's stretch range, and squash's clamp
  ST_MAX   =  2047,
};

// ---------------------------------------------------------------- baseline pair
// verbatim from paq8hp.hpp so the comparison is against what actually ships

static int old_squash(int d) {
  static const int t[33] = {1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,
    2047,2549,2994,3348,3607,3785,3901,3975,4022,4050,4068,4079,4085,4089,4092,
    4093,4094};
  if( d>2047 ) return 4095;
  if( d<-2047 ) return 0;
  int w = d&127;
  d = (d>>7)+16;
  return (t[d]*(128-w)+t[(d+1)]*w+64)>>7;
}

static short old_stt[P_ONE];
static void old_build_stretch() {
  int pi = 0;
  for( int x = -2047; x<=2047; ++x ) {
    int i = old_squash(x);
    for( int j = pi; j<=i; ++j ) old_stt[j] = x;
    pi = i+1;
  }
  old_stt[4095] = 2047;
}

// ---------------------------------------------------------------- ported pair
// This is xad_logistic.inc's arithmetic with xadpcm's names kept, so it can be
// diffed against the original.  No floating point anywhere in here.

static const i64 LN2_Q48 = 195103586505167LL;   // round(ln2 * 2^48)

static u32   LOG2[P_ONE];    // 65536*log2(i)
static short new_sqt[P_ONE]; // squash: d+P_MID -> p12   (short: values 0..4095)
static short new_stt[P_ONE]; // stretch: p12 -> d

struct STCalc {
  const u32* L; i64 K;
  i64 q8(int p) const {   // stretch in 1/256 units
    return (((i64)L[p] - (i64)L[P_ONE-p])*K + (1LL<<23))>>24;
  }
  int q0(int p) const {   // stretch, floor(x+0.5) -- correct for negative x too
    return int((((i64)L[p] - (i64)L[P_ONE-p])*K + (1LL<<31))>>32);
  }
};

static STCalc st_calc;

static void build_new_tables() {
  // 65536*log2(i) by repeated squaring: square, renormalise below 2^32, count
  // the shifts.  The +31 corrects the truncation bias renormalisation
  // accumulates.  i^(2^16) = w * 2^k with w in [2^31,2^32), so
  // 2^16*log2(i) = k + log2(w) ~= k + 31.
  for( u32 i = 0; i<P_ONE; i++ ) {
    u64 w = i; u32 k = 0;
    for( int j = 0; j<16; j++ ) {
      w = w*w; k = k+k;
      while( w >= (1ULL<<32) ) { w = (w+1)>>1; k++; }
    }
    LOG2[i] = k + (i>1 ? 31u : 0u);
  }

  const i64 KST = (i64(ST_SCALE)*LN2_Q48 + (1LL<<31))>>32;   // S*ln2*2^16
  st_calc.L = LOG2; st_calc.K = KST;

  // stretch(p) = S*ln(p/(N-p)) = S*ln2*(log2(p) - log2(N-p))
  for( int p = 0; p<P_ONE; p++ ) {
    int pi = pclamp(p, P_MIN, P_MAX);
    new_stt[p] = short(pclamp(st_calc.q0(pi), ST_MIN, ST_MAX));
  }

  // squash = the inverse of the stretch above, by binary search on a Q8 stretch
  // plus one linear interpolation, so it rounds the real crossing rather than
  // snapping to the nearest tabulated stretch.
  for( int d = -P_MID; d<P_MID; d++ ) {
    i64 t = (i64)d<<8;
    int lo = P_MIN, hi = P_MAX;
    while( lo<hi ) { int m = (lo+hi)>>1; if( st_calc.q8(m)<t ) lo = m+1; else hi = m; }
    int best = lo;
    if( lo>P_MIN ) {
      i64 s1 = st_calc.q8(lo), s0 = st_calc.q8(lo-1);
      if( s1>t && s1!=s0 ) {
        i64 frac = ((t-s0)*256 + (s1-s0)/2)/(s1-s0);
        best = (frac>=128) ? lo : lo-1;
      }
    }
    new_sqt[d+P_MID] = short(pclamp(best, P_MIN, P_MAX));
  }
}

// paq8hp's squash clamps at +-2047 and returns the saturated 0 / 4095 outside
// that.  Keeping exactly that shape means the only thing this port changes is
// the accuracy of the interior mapping -- in particular squash() can still
// return 0, so the coder's `if(p<1) p=1` guard stays meaningful rather than
// silently becoming dead code.
static int new_squash(int d) {
  if( d>ST_MAX ) return P_ONE-1;
  if( d<ST_MIN ) return 0;
  return new_sqt[d+P_MID];
}

// ---------------------------------------------------------------- checks

static int fails = 0;
static void ck(bool ok, const char* what, const char* detail = "") {
  if( !ok ) { printf("  FAIL  %s %s\n", what, detail); fails++; }
  else        printf("  ok    %s %s\n", what, detail);
}

int main() {
  old_build_stretch();
  build_new_tables();
  char buf[256];

  printf("\n=== 1. LOG2 table vs 65536*log2(i) ===\n");
  {
    double worst = 0; int wi = 0;
    for( int i = 2; i<P_ONE; i++ ) {
      double ref = 65536.0*log2((double)i);
      double e = fabs((double)LOG2[i] - ref);
      if( e>worst ) { worst = e; wi = i; }
    }
    snprintf(buf,sizeof buf,"max err %.3f at i=%d (of 65536*log2 ~ %.0f)", worst, wi, 65536.0*log2((double)wi));
    ck(worst < 2.0, "LOG2 within 2 units", buf);
    ck(LOG2[1]==0, "LOG2[1]==0");
  }

  printf("\n=== 2. new stretch vs S*ln(p/(N-p)) ===\n");
  {
    int worst = 0, wp = 0;
    for( int p = P_MIN; p<=P_MAX; p++ ) {
      double ref = ST_SCALE*log((double)p/(double)(P_ONE-p));
      int refr = (int)floor(pclamp((int)floor(ref+0.5), ST_MIN, ST_MAX)+0.5);
      int e = abs(new_stt[p]-refr);
      if( e>worst ) { worst = e; wp = p; }
    }
    snprintf(buf,sizeof buf,"max |diff| %d at p=%d", worst, wp);
    ck(worst<=1, "new stretch matches float reference within 1", buf);
  }

  printf("\n=== 3. monotonicity ===\n");
  {
    bool ms = true, mq = true;
    for( int p = 1; p<P_ONE; p++ ) if( new_stt[p] < new_stt[p-1] ) ms = false;
    for( int i = 1; i<P_ONE; i++ ) if( new_sqt[i] < new_sqt[i-1] ) mq = false;
    ck(ms, "new stretch is non-decreasing in p");
    ck(mq, "new squash is non-decreasing in d");
    bool mo = true;
    for( int p = 1; p<P_ONE; p++ ) if( old_stt[p] < old_stt[p-1] ) mo = false;
    ck(mo, "(baseline stretch is non-decreasing too)");
  }

  printf("\n=== 4. ranges / no overflow at the use sites ===\n");
  {
    int smin = 1<<30, smax = -(1<<30), qmin = 1<<30, qmax = -(1<<30);
    for( int i = 0; i<P_ONE; i++ ) {
      if( new_stt[i]<smin ) smin = new_stt[i];
      if( new_stt[i]>smax ) smax = new_stt[i];
      if( new_sqt[i]<qmin ) qmin = new_sqt[i];
      if( new_sqt[i]>qmax ) qmax = new_sqt[i];
    }
    snprintf(buf,sizeof buf,"stretch [%d,%d] squash [%d,%d]", smin, smax, qmin, qmax);
    ck(smin>=ST_MIN && smax<=ST_MAX && qmin>=0 && qmax<=P_ONE-1, "in range", buf);
    // APM's ctor does squash(x)*16 into a U16
    int am = 0;
    for( int j = 0; j<33; j++ ) { int v = new_squash((j-16)*128)*16; if( v>am ) am = v; }
    snprintf(buf,sizeof buf,"max APM init value %d (u16 limit 65535)", am);
    ck(am<=65535, "squash(d)*16 fits u16 for APM ctor", buf);
    // the coder needs squash to be able to hit 0 for the p<1 clamp to matter,
    // and must never exceed 4095
    ck(new_squash(ST_MIN-1)==0, "squash saturates to 0 below ST_MIN");
    ck(new_squash(ST_MAX+1)==P_ONE-1, "squash saturates to 4095 above ST_MAX");
  }

  printf("\n=== 5. squash vs the true logistic  N/(1+exp(-d/S)) ===\n");
  {
    double wn = 0, wo = 0; int dn = 0, dq = 0;
    for( int d = ST_MIN; d<=ST_MAX; d++ ) {
      double ref = (double)P_ONE/(1.0+exp(-(double)d/ST_SCALE));
      double en = fabs((double)new_squash(d) - ref);
      double eo = fabs((double)old_squash(d) - ref);
      if( en>wn ) { wn = en; dn = d; }
      if( eo>wo ) { wo = eo; dq = d; }
    }
    printf("        baseline max err %6.2f at d=%5d\n", wo, dq);
    printf("        ported   max err %6.2f at d=%5d\n", wn, dn);
    snprintf(buf,sizeof buf,"%.2f -> %.2f  (%.1fx better)", wo, wn, wn>0?wo/wn:0.0);
    ck(wn < wo, "ported squash is closer to the logistic than the baseline", buf);
    ck(wn <= 1.0, "ported squash within 1.0 of the real logistic everywhere");
  }

  printf("\n=== 6. round trip: stretch(squash(d)) == d ===\n");
  {
    int wn = 0, wo = 0, dn = 0, dq = 0;
    long long sn = 0, so = 0, n = 0;
    for( int d = ST_MIN; d<=ST_MAX; d++ ) {
      int en = abs(new_stt[new_squash(d)] - d);
      int eo = abs(old_stt[old_squash(d)] - d);
      if( en>wn ) { wn = en; dn = d; }
      if( eo>wo ) { wo = eo; dq = d; }
      sn += en; so += eo; n++;
    }
    printf("        baseline max %4d at d=%5d, mean %.3f\n", wo, dq, (double)so/n);
    printf("        ported   max %4d at d=%5d, mean %.3f\n", wn, dn, (double)sn/n);
    snprintf(buf,sizeof buf,"max %d -> %d, mean %.3f -> %.3f", wo, wn, (double)so/n, (double)sn/n);
    ck(wn <= wo, "ported round trip no worse than baseline", buf);
  }

  printf("\n=== 7. round trip: squash(stretch(p)) == p ===\n");
  {
    int wn = 0, wo = 0, pn = 0, po = 0;
    long long sn = 0, so = 0, n = 0;
    for( int p = P_MIN; p<=P_MAX; p++ ) {
      int en = abs(new_squash(new_stt[p]) - p);
      int eo = abs(old_squash(old_stt[p]) - p);
      if( en>wn ) { wn = en; pn = p; }
      if( eo>wo ) { wo = eo; po = p; }
      sn += en; so += eo; n++;
    }
    printf("        baseline max %4d at p=%5d, mean %.3f\n", wo, po, (double)so/n);
    printf("        ported   max %4d at p=%5d, mean %.3f\n", wn, pn, (double)sn/n);
    snprintf(buf,sizeof buf,"max %d -> %d, mean %.3f -> %.3f", wo, wn, (double)so/n, (double)sn/n);
    ck(wn <= wo, "ported round trip no worse than baseline", buf);
    // squash(stretch(p)) == p is NOT achievable and must not be asserted:
    // d(stretch)/dp = S*N/(p*(N-p)) = 0.25 at the midpoint, so four distinct p
    // share each integer d there and no left inverse exists.  The correct
    // statement for a non-injective map is the pseudo-inverse pair below --
    // squash must land on *a* p carrying the same stretch value, i.e.
    // stretch(squash(stretch(p))) == stretch(p), and symmetrically.
    {
      int mult = 0, run = 1;
      for( int q = P_MIN+1; q<=P_MAX; q++ ) {
        if( new_stt[q]==new_stt[q-1] ) run++; else { if(run>mult) mult=run; run=1; }
      }
      snprintf(buf,sizeof buf,"up to %d distinct p share one stretch value", mult);
      ck(mult>=2, "stretch is many-to-one, so no left inverse can exist", buf);

      int bad1 = 0, first1 = 0;
      for( int p = P_MIN; p<=P_MAX; p++ )
        if( new_stt[new_squash(new_stt[p])] != new_stt[p] ) { if(!bad1) first1 = p; bad1++; }
      snprintf(buf,sizeof buf,"%d violations%s", bad1, bad1? "" : " (stretch o squash o stretch == stretch)");
      if( bad1 ) snprintf(buf,sizeof buf,"%d violations, first p=%d", bad1, first1);
      ck(bad1==0, "pseudo-inverse: stretch(squash(stretch(p))) == stretch(p)", buf);

      int bad2 = 0, first2 = 0;
      for( int d = ST_MIN; d<=ST_MAX; d++ )
        if( new_squash(new_stt[new_squash(d)]) != new_squash(d) ) { if(!bad2) first2 = d; bad2++; }
      snprintf(buf,sizeof buf,"%d violations%s", bad2, bad2? "" : " (squash o stretch o squash == squash)");
      if( bad2 ) snprintf(buf,sizeof buf,"%d violations, first d=%d", bad2, first2);
      ck(bad2==0, "pseudo-inverse: squash(stretch(squash(d))) == squash(d)", buf);
    }
  }

  printf("\n=== 8. determinism / reproducibility ===\n");
  {
    // rebuild and compare: the builder must be a pure function of the constants
    static short s1[P_ONE], q1[P_ONE];
    memcpy(s1,new_stt,sizeof s1); memcpy(q1,new_sqt,sizeof q1);
    memset(new_stt,0,sizeof new_stt); memset(new_sqt,0,sizeof new_sqt);
    memset(LOG2,0,sizeof LOG2);
    build_new_tables();
    ck(memcmp(s1,new_stt,sizeof s1)==0 && memcmp(q1,new_sqt,sizeof q1)==0,
       "rebuilding produces identical tables");
    u32 h = 2166136261u;                       // FNV-1a, as xad_logistic.inc does
    for( int i = 0; i<P_ONE; i++ ) {
      h = (h ^ u32((u16)new_sqt[i]))*16777619u;
      h = (h ^ u32((u16)new_stt[i]))*16777619u;
    }
    printf("        FNV-1a over (SQT,STT) = %08X\n", h);
    // xad_logistic.inc ships XAD_SQSTT_HASH = 0x7BCBA716 as the checksum of its
    // tables at ST_SCALE=256.  paq8hp's constants (P_ONE=4096, ST_SCALE=256,
    // P_MIN/P_MAX=1/4095, ST_MIN/ST_MAX=-+2047) coincide with xadpcm's shipped
    // ones, so a faithful port must reproduce that value bit for bit.  This is
    // the strongest single check here: it pins every one of the 8192 entries.
    snprintf(buf,sizeof buf,"%08X == xadpcm's XAD_SQSTT_HASH", h);
    ck(h==0x7BCBA716u, "tables are bit-identical to xad_logistic.inc's", buf);
  }

  printf("\n=== 9. how much the tables actually differ ===\n");
  {
    int ns = 0, nq = 0, ms = 0, mq = 0;
    for( int p = 0; p<P_ONE; p++ ) {
      int d = abs(new_stt[p]-old_stt[p]);
      if( d ) { ns++; if( d>ms ) ms = d; }
    }
    for( int d = ST_MIN; d<=ST_MAX; d++ ) {
      int e = abs(new_squash(d)-old_squash(d));
      if( e ) { nq++; if( e>mq ) mq = e; }
    }
    printf("        stretch: %d/%d entries differ, max %d\n", ns, P_ONE, ms);
    printf("        squash : %d/%d entries differ, max %d\n", nq, 2*2047+1, mq);
  }

  printf("\n%s (%d failures)\n\n", fails? "FAILED" : "ALL CHECKS PASSED", fails);
  return fails ? 1 : 0;
}
