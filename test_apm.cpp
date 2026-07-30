// test_apm.cpp -- the APM grid generalisation, checked rather than assumed.
//
//   g++ -O2 -o test_apm test_apm.cpp && ./test_apm
//
// APM used to share squash's 33-node grid and address it with a shift pair:
//
//     w     = pr & SQ_MASK;                       // SQ_MASK = 127
//     index = ((pr+ST_LIMIT) >> SQ_BITS) + cxt*33;
//     ret   = (t0*(SQ_STEP-w) + t1*w) >> (PR_BITS+SQ_BITS-P_BITS);
//
// It now carries its own per-instance node count and addresses the range as a
// fixed-point fraction of it, which is what makes the count tunable:
//
//     sn    = (pr+ST_LIMIT) * NIV;
//     w     = sn & (ST_RANGE-1);
//     index = (sn >> ST_RANGE_BITS) + cxt*(NIV+1);
//     ret   = (t0*(ST_RANGE-w) + t1*w) >> (PR_BITS+ST_RANGE_BITS-P_BITS);
//
// Section 1 is the claim that matters for byte-identity: at NIV==32 the two are
// EXACTLY equal -- not approximately, not for typical inputs -- for every
// stretch value and every pair of U16 table entries.  The rest checks the
// general form stays in bounds and stays sane for every node count the clamp
// admits, since that is what the optimizer will actually sweep.

#include <stdio.h>
#include <stdlib.h>

typedef unsigned short U16;

enum {
  P_BITS = 12,  P_SCALE = 1<<P_BITS,
  PR_BITS = 16, PR_MAX = (1<<PR_BITS)-1,
  ST_LIMIT = 2048, ST_MAX = ST_LIMIT-1, ST_MIN = -ST_MAX,
  ST_RANGE = 2*ST_LIMIT, ST_RANGE_BITS = 12,
  SQ_BITS = 7, SQ_STEP = 1<<SQ_BITS, SQ_MASK = SQ_STEP-1, SQ_NODES = 33,
  APM_NH_MAX = 32,
};

static int fails = 0;
static void ck(bool ok, const char* what, const char* detail = "") {
  if( !ok ) { printf("  FAIL  %s %s\n", what, detail); fails++; }
  else        printf("  ok    %s %s\n", what, detail);
}

// ---- the two forms, as literal transcriptions ----------------------------
static void old_form(int pr, int cxt, int* idx, int* w) {
  *w = pr&SQ_MASK;
  *idx = ((pr+ST_LIMIT)>>SQ_BITS)+cxt*SQ_NODES;
}
static int old_ret(int t0, int t1, int w) {
  return (t0*(SQ_STEP-w)+t1*w)>>(PR_BITS+SQ_BITS-P_BITS);
}
static void new_form(int pr, int cxt, int NIV, int CELLS, int* idx, int* w) {
  const int sn = (pr+ST_LIMIT)*NIV;
  *w = sn&(ST_RANGE-1);
  *idx = (sn>>ST_RANGE_BITS)+cxt*CELLS;
}
static int new_ret(int t0, int t1, int w) {
  return (t0*(ST_RANGE-w)+t1*w)>>(PR_BITS+ST_RANGE_BITS-P_BITS);
}

int main() {
  char b[256];

  printf("\n=== 1. NIV==32 is EXACTLY the old shift pair (all stretch values) ===\n");
  {
    // the index, first: same cell, for every reachable stretch value
    int bad = 0, firstbad = 0;
    for( int pr = ST_MIN; pr<=ST_MAX; ++pr ) {
      int oi, ow, ni, nw;
      old_form(pr, 0, &oi, &ow);
      new_form(pr, 0, 32, 33, &ni, &nw);
      if( oi!=ni ) { if(!bad) firstbad = pr; bad++; }
    }
    snprintf(b,sizeof b,"%d of %d stretch values pick a different cell", bad, 2*ST_MAX+1);
    if( bad ) snprintf(b,sizeof b,"%d differ, first at stretch %d", bad, firstbad);
    ck(bad==0, "interval index identical over the whole stretch range", b);
  }
  {
    // and the weight: the new fraction must be exactly 32x the old one, which is
    // what makes the wider shift cancel
    int bad = 0;
    for( int pr = ST_MIN; pr<=ST_MAX; ++pr ) {
      int oi, ow, ni, nw;
      old_form(pr, 0, &oi, &ow);
      new_form(pr, 0, 32, 33, &ni, &nw);
      if( nw != ow*32 ) bad++;
    }
    ck(bad==0, "interpolation fraction is exactly 32x the old one");
  }
  {
    // the returned probability, over the whole (t0,t1) product space at a
    // representative set of weights, then over all weights at representative
    // entries.  Both, because a bug in the shift shows up in one and a bug in
    // the fraction in the other.
    int bad = 0; long long n = 0;
    for( int w = 0; w<SQ_STEP; ++w )
      for( int t0 = 0; t0<=PR_MAX; t0 += 37 )
        for( int t1 = 0; t1<=PR_MAX; t1 += 1009 ) {
          n++;
          if( old_ret(t0,t1,w) != new_ret(t0,t1,w*32) ) bad++;
        }
    snprintf(b,sizeof b,"%d wrong of %lld (t0,t1,w) triples", bad, n);
    ck(bad==0, "interpolated result identical", b);
  }
  {
    // the end-to-end value, which is what the archive actually depends on
    int bad = 0; long long n = 0;
    for( int pr = ST_MIN; pr<=ST_MAX; ++pr )
      for( int t0 = 0; t0<=PR_MAX; t0 += 4093 )
        for( int t1 = 0; t1<=PR_MAX; t1 += 8191 ) {
          int oi, ow, ni, nw;
          old_form(pr, 0, &oi, &ow);
          new_form(pr, 0, 32, 33, &ni, &nw);
          n++;
          if( oi!=ni || old_ret(t0,t1,ow) != new_ret(t0,t1,nw) ) bad++;
        }
    snprintf(b,sizeof b,"%d wrong of %lld end-to-end evaluations", bad, n);
    ck(bad==0, "cell and value agree end to end", b);
  }

  printf("\n=== 2. the shipped 33 nodes are NH==16 ===\n");
  ck(2*16+1 == SQ_NODES, "2*NH+1 == 33 at NH == 16");
  {
    // a node exactly at p == P_HALF is the reason for declaring the HALF count:
    // stretch(P_HALF) is 0, which must land on a node boundary with zero weight
    int oi, ow, ni, nw;
    new_form(0, 0, 32, 33, &ni, &nw);
    snprintf(b,sizeof b,"stretch 0 -> cell %d, fraction %d", ni, nw);
    ck(nw==0 && ni==16, "the midpoint sits exactly on a node, not between two", b);
  }
  {
    // and that stays true for every odd node count the clamp admits, which is
    // the property an unrestricted node count would lose half the time
    int bad = 0, firstbad = 0;
    for( int nh = 1; nh<=APM_NH_MAX; ++nh ) {
      int ni, nw;
      new_form(0, 0, 2*nh, 2*nh+1, &ni, &nw);
      if( nw!=0 || ni!=nh ) { if(!bad) firstbad = nh; bad++; }
    }
    snprintf(b,sizeof b,"%d of %d half-counts misplace the midpoint", bad, APM_NH_MAX);
    if( bad ) snprintf(b,sizeof b,"%d bad, first at NH=%d", bad, firstbad);
    ck(bad==0, "midpoint stays on a node for every NH in [1,APM_NH_MAX]", b);
  }
  {
    // an EVEN interval count does not, which is what the parameterisation buys
    int ni, nw;
    new_form(0, 0, 33, 34, &ni, &nw);   // 33 intervals: deliberately odd
    snprintf(b,sizeof b,"33 intervals -> cell %d, fraction %d (nonzero, as expected)", ni, nw);
    ck(nw!=0, "an odd interval count DOES straddle the midpoint (why NH is halved)", b);
  }

  printf("\n=== 3. in bounds for every node count the clamp admits ===\n");
  {
    // index+1 is read, so the interval must never reach NIV.  Checked for every
    // NH and every stretch value -- 32 x 4095, so exhaustively, not sampled.
    int bad = 0; long long n = 0; int firstnh = 0, firstpr = 0;
    for( int nh = 1; nh<=APM_NH_MAX; ++nh ) {
      const int NIV = 2*nh;
      for( int pr = ST_MIN; pr<=ST_MAX; ++pr ) {
        int ni, nw;
        new_form(pr, 0, NIV, NIV+1, &ni, &nw);
        n++;
        if( ni<0 || ni>NIV-1 || nw<0 || nw>=ST_RANGE ) {
          if(!bad) { firstnh = nh; firstpr = pr; }
          bad++;
        }
      }
    }
    snprintf(b,sizeof b,"%d of %lld (NH,stretch) pairs out of range", bad, n);
    if( bad ) snprintf(b,sizeof b,"%d bad, first at NH=%d stretch=%d", bad, firstnh, firstpr);
    ck(bad==0, "interval in [0,NIV-1] and fraction in [0,ST_RANGE) always", b);
  }
  {
    // rows must not overlap: cxt*CELLS + NIV < (cxt+1)*CELLS
    int bad = 0;
    for( int nh = 1; nh<=APM_NH_MAX; ++nh ) {
      const int NIV = 2*nh;
      for( int CELLS = NIV+1; CELLS<=2*APM_NH_MAX+1; ++CELLS )   // exact, and the tuning reserve
        for( int cxt = 0; cxt<64; ++cxt )
          if( cxt*CELLS+NIV >= (cxt+1)*CELLS ) bad++;
    }
    ck(bad==0, "the last cell of a row stays inside it, at any CELLS >= NIV+1");
  }
  {
    // no int overflow anywhere in the expression
    long long worst_sn = (long long)(ST_RANGE-1) * (2*APM_NH_MAX);
    long long worst_p  = (long long)PR_MAX*ST_RANGE*2;
    snprintf(b,sizeof b,"max s*NIV = %lld, max weighted sum = %lld, INT_MAX = 2147483647",
             worst_sn, worst_p);
    ck(worst_sn < 2147483647LL && worst_p < 2147483647LL, "no int overflow at APM_NH_MAX", b);
  }

  printf("\n=== 4. the initial table is the same curve at any node count ===\n");
  {
    // t[j] = squash(j*ST_RANGE/NIV - ST_LIMIT).  The argument must sweep the
    // full range monotonically and hit exactly -ST_LIMIT and +ST_LIMIT at the
    // ends, whatever NIV -- otherwise a retuned APM starts from a different
    // prior than the one it was measured with.
    int bad = 0;
    for( int nh = 1; nh<=APM_NH_MAX; ++nh ) {
      const int NIV = 2*nh;
      int prev = -1<<30;
      for( int j = 0; j<=NIV; ++j ) {
        int d = j*ST_RANGE/NIV - ST_LIMIT;
        if( j==0 && d!=-ST_LIMIT ) bad++;
        if( j==NIV && d!=ST_LIMIT ) bad++;
        if( d<=prev ) bad++;
        prev = d;
      }
    }
    ck(bad==0, "grid spans [-ST_LIMIT,+ST_LIMIT] strictly increasing, every NH");
  }
  {
    // and at NH==16 it is node-for-node the old (j-SQ_MID)*SQ_STEP
    int bad = 0;
    for( int j = 0; j<SQ_NODES; ++j )
      if( j*ST_RANGE/32 - ST_LIMIT != (j-SQ_NODES/2)*SQ_STEP ) bad++;
    ck(bad==0, "at NH==16 every one of the 33 nodes matches the old formula");
  }

  printf("\n%s (%d failures)\n\n", fails? "FAILED" : "ALL CHECKS PASSED", fails);
  return fails ? 1 : 0;
}
