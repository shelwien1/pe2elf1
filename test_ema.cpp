// test_ema.cpp -- the shift-to-multiplier change, checked over the whole domain.
//
//   g++ -O2 -o test_ema test_ema.cpp && ./test_ema
//
// Every adaptive counter used to move by a SHIFT, with each caller folding its
// own rounding into the target:
//
//   StateMap/APM:  v + (( (y?PR_MAX+(1<<r)-1:0) - v) >> r)
//   SSCM:          v + (( (y<<PR_BITS)+(1<<(r-1)) - v) >> r)
//
// It moves by a 16-bit multiplier now, with the rounding as a bias on the
// product instead:
//
//   ema(v,target,mul,bias) = v + (((target-v)*mul + bias) >> EMA_BITS)
//   StateMap/APM:  ema(v, y?PR_MAX:0,   mul, y?EMA_CEIL:0)
//   SSCM:          ema(v, y<<PR_BITS,   mul, EMA_HALF)
//
// The claim is that at mul == EMA_SCALE>>r the two are EXACTLY equal -- every
// stored value, every rate the clamp admits, both bit values.  That is not
// obvious: the old form floors a sum, the new one floors a scaled product, and
// the fudge terms moved from one side of the division to the other.  Section 1
// checks it exhaustively, which is cheap enough that there is no reason to
// sample.
//
// EMA_MUL_MIN_RATE is where the sweep starts, and it is not 1.  PEMA caps mul
// at EMA_MUL_MAX = EMA_SCALE/4 so the product stays inside int, which means the
// fastest expressible rate is 1/4 -- the old shift form could also spell 1/2 and
// a full step.  Nothing in the model used either (the fastest shipped rate is
// a1's 1/8) and a counter that jumps half way to its target every bit is not an
// adaptive counter, but it IS a narrowing, so section 5 states it as a fact
// about the clamp rather than leaving the sweep silently starting at 2.
//
// Sections 2-4 are about what the change is FOR: the rates between the powers
// of two, which a shift cannot reach at all.

#include <stdio.h>
#include <stdlib.h>

enum {
  PR_BITS = 16, PR_ONE = 1<<PR_BITS, PR_MAX = PR_ONE-1,
  EMA_BITS = 16, EMA_SCALE = 1<<EMA_BITS,
  EMA_MUL_MAX = EMA_SCALE/4,
  EMA_MUL_MIN_RATE = 2,        // EMA_SCALE>>2 == EMA_MUL_MAX: the fastest legal rate
  EMA_CEIL = EMA_SCALE-1, EMA_HALF = EMA_SCALE/2,
};

static int fails = 0;
static void ck(bool ok, const char* what, const char* detail = "") {
  if( !ok ) { printf("  FAIL  %s %s\n", what, detail); fails++; }
  else        printf("  ok    %s %s\n", what, detail);
}

// ---- the two forms, as literal transcriptions -----------------------------
static int old_sm (int v, int y, int r) { return v + (((y ? PR_MAX+(1<<r)-1 : 0) - v) >> r); }
static int old_scm(int v, int y, int r) { return v + ((((y<<PR_BITS)+(1<<(r-1))) - v) >> r); }

static int ema(int v, int target, int mul, int bias) {
  return v + (((target-v)*mul + bias)>>EMA_BITS);
}
static int new_sm (int v, int y, int mul) { return ema(v, y?PR_MAX:0, mul, y?EMA_CEIL:0); }
static int new_scm(int v, int y, int mul) { return ema(v, y<<PR_BITS, mul, EMA_HALF); }

int main() {
  char b[256];

  printf("\n=== 1. mul == EMA_SCALE>>r is EXACTLY the old shift (exhaustive) ===\n");
  {
    // StateMap and APM: target PR_MAX / 0, round away from v.
    // r runs from the fastest the clamp admits to 16, one step in 65536; the
    // shipped rates (3..10) sit in the middle of that.
    long long n = 0; int bad = 0, br = 0, bv = 0, by = 0;
    for( int r = EMA_MUL_MIN_RATE; r<=EMA_BITS; ++r ) {
      const int mul = EMA_SCALE>>r;
      for( int y = 0; y<2; ++y )
        for( int v = 0; v<=PR_MAX; ++v ) {
          n++;
          if( old_sm(v,y,r) != new_sm(v,y,mul) ) {
            if(!bad) { br = r; bv = v; by = y; }
            bad++;
          }
        }
    }
    snprintf(b,sizeof b,"%d wrong of %lld (v,y,rate) cases", bad, n);
    if( bad ) snprintf(b,sizeof b,"%d wrong of %lld, first at v=%d y=%d rate=%d -> old %d new %d",
                       bad, n, bv, by, br, old_sm(bv,by,br), new_sm(bv,by,EMA_SCALE>>br));
    ck(bad==0, "StateMap/APM form identical at every stored value and rate", b);
  }
  {
    // SmallStationaryContextMap: target PR_ONE / 0, round to nearest.
    // Same range; note the old form's (1<<(r-1)) needs r>=1 anyway, so the
    // multiplier form is the one that generalises rather than the shift.
    long long n = 0; int bad = 0, br = 0, bv = 0, by = 0;
    for( int r = EMA_MUL_MIN_RATE; r<=EMA_BITS; ++r ) {
      const int mul = EMA_SCALE>>r;
      for( int y = 0; y<2; ++y )
        for( int v = 0; v<=PR_MAX; ++v ) {
          n++;
          if( old_scm(v,y,r) != new_scm(v,y,mul) ) {
            if(!bad) { br = r; bv = v; by = y; }
            bad++;
          }
        }
    }
    snprintf(b,sizeof b,"%d wrong of %lld (v,y,rate) cases", bad, n);
    if( bad ) snprintf(b,sizeof b,"%d wrong of %lld, first at v=%d y=%d rate=%d -> old %d new %d",
                       bad, n, bv, by, br, old_scm(bv,by,br), new_scm(bv,by,EMA_SCALE>>br));
    ck(bad==0, "SSCM form identical at every stored value and rate", b);
  }
  {
    // the exact rates the model ships with, called out separately so a failure
    // says which counter broke rather than just "rate 7"
    struct { const char* who; int r; } R[] = {
      {"SM_MUL_0 (StateMap, early)",7}, {"SM_MUL_1 (StateMap, mid)",8},
      {"SM_MUL_2 (StateMap, late)",9},  {"A1_MUL (a1)",3},
      {"A2_MUL (a2)",7},                {"A3..A6_MUL (a3-a6)",6},
      {"SCM_MUL_EARLY",9},              {"SCM_MUL_LATE",10},
    };
    int bad = 0;
    for( unsigned k = 0; k<sizeof(R)/sizeof(R[0]); ++k )
      for( int y = 0; y<2; ++y )
        for( int v = 0; v<=PR_MAX; ++v )
          if( old_sm(v,y,R[k].r) != new_sm(v,y,EMA_SCALE>>R[k].r) ) bad++;
    snprintf(b,sizeof b,"all 8 shipped rates, both bit values, all %d stored values", PR_ONE);
    ck(bad==0, "the shipped seeds specifically", b);
  }

  printf("\n=== 2. the rounding rules are what the callers had ===\n");
  {
    // "round away from v" is the property that lets a counter reach PR_MAX
    // instead of stalling one step short.  Checked at the top of the range,
    // where a floor would stall.
    int bad = 0;
    for( int r = EMA_MUL_MIN_RATE; r<=12; ++r ) {
      const int mul = EMA_SCALE>>r;
      for( int v = PR_MAX-4; v<=PR_MAX; ++v ) {
        int nv = new_sm(v,1,mul);
        if( v<PR_MAX && nv<=v ) bad++;      // must make progress
        if( nv>PR_MAX ) bad++;              // must not overshoot
      }
    }
    ck(bad==0, "y==1 always advances toward PR_MAX and never past it");
  }
  {
    int bad = 0;
    for( int r = EMA_MUL_MIN_RATE; r<=12; ++r ) {
      const int mul = EMA_SCALE>>r;
      for( int v = 0; v<=4; ++v ) {
        int nv = new_sm(v,0,mul);
        if( v>0 && nv>=v ) bad++;
        if( nv<0 ) bad++;
      }
    }
    ck(bad==0, "y==0 always advances toward 0 and never past it");
  }
  {
    // SSCM's nearest-rounding is symmetric in a way the other is not: it can
    // stall.  That is the shipped behaviour, so state it rather than "fix" it.
    const int mul = EMA_SCALE>>10;
    int stall = 0;
    for( int v = PR_MAX-40; v<=PR_MAX; ++v ) if( new_scm(v,1,mul)==v ) stall++;
    snprintf(b,sizeof b,"%d of 41 top values are fixed points at rate 10", stall);
    ck(stall>0, "SSCM's nearest-rounding CAN stall short (unchanged behaviour)", b);
  }

  printf("\n=== 3. the point of the change: rates a shift cannot reach ===\n");
  {
    // between 1/128 and 1/256 a shift has nothing.  The multiplier has 255 of
    // them, and each must be a genuine intermediate: monotone in mul, and
    // strictly between the two neighbouring shifts.
    const int v = 0, y = 1;
    const int fast = new_sm(v,y,EMA_SCALE>>7);      // 1/128
    const int slow = new_sm(v,y,EMA_SCALE>>8);      // 1/256
    int bad = 0, between = 0, prev = -1;
    for( int mul = (EMA_SCALE>>8)+1; mul<(EMA_SCALE>>7); ++mul ) {
      int nv = new_sm(v,y,mul);
      if( nv<slow || nv>fast ) bad++;
      if( nv>slow && nv<fast ) between++;
      if( nv<prev ) bad++;
      prev = nv;
    }
    snprintf(b,sizeof b,"%d multipliers between 1/256 and 1/128, %d land strictly between",
             (EMA_SCALE>>7)-(EMA_SCALE>>8)-1, between);
    ck(bad==0 && between>0, "the gap between two shifts is populated and monotone", b);
  }
  {
    // a schedule step is mul>>1, which is exactly rate+1 at the seeds
    int bad = 0;
    for( int r = EMA_MUL_MIN_RATE; r<EMA_BITS; ++r )
      if( (EMA_SCALE>>r)>>1 != (EMA_SCALE>>(r+1)) ) bad++;
    ck(bad==0, "MUL>>1 is rate+1, so the *_SW schedules mean what they did");
  }

  printf("\n=== 4. no int overflow anywhere the clamp admits ===\n");
  {
    // |target-v| peaks at PR_ONE (SSCM's y==1 against a zero counter), mul at
    // EMA_MUL_MAX, plus the largest bias.
    long long worst = (long long)PR_ONE*EMA_MUL_MAX + EMA_CEIL;
    snprintf(b,sizeof b,"max |target-v|*mul + bias = %lld, INT_MAX = %d", worst, 2147483647);
    ck(worst < 2147483647LL, "the product fits int, so no 64-bit widening", b);

    // and confirm it by evaluating there rather than only computing the bound
    int nv = ema(0, PR_ONE, EMA_MUL_MAX, EMA_CEIL);
    snprintf(b,sizeof b,"ema(0,PR_ONE,EMA_MUL_MAX,EMA_CEIL) = %d", nv);
    ck(nv>0 && nv<=PR_ONE, "evaluating at the extreme gives a sane result", b);
  }
  {
    // the clamp's floor matters too: mul==0 would freeze a counter forever
    int nv = ema(1000, PR_MAX, 1, EMA_CEIL);
    snprintf(b,sizeof b,"the slowest legal rate, 1/65536, still moves: 1000 -> %d", nv);
    ck(nv>1000, "mul==1 (the clamp's floor) still makes progress", b);
  }

  printf("\n=== 5. where the clamp actually bites, stated rather than skipped ===\n");
  {
    // EMA_MUL_MAX is the reason section 1 starts at rate 2.  Say what that
    // costs: the two rates the old shift form could spell and this one cannot.
    snprintf(b,sizeof b,"fastest expressible rate is 1/%d (mul %d); a shift could also do 1/2 and 1/1",
             EMA_SCALE/EMA_MUL_MAX, EMA_MUL_MAX);
    ck(EMA_MUL_MAX == (EMA_SCALE>>EMA_MUL_MIN_RATE), "the clamp is exactly rate 2", b);

    // and that the excluded rate really would overflow, i.e. the clamp is load
    // bearing rather than superstition.  Computed in 64-bit so the check itself
    // does not overflow.
    long long over = (long long)PR_ONE*(EMA_SCALE>>1) + EMA_CEIL;
    snprintf(b,sizeof b,"rate 1 would need %lld > INT_MAX %d", over, 2147483647);
    ck(over > 2147483647LL, "the excluded rate is excluded because it overflows int", b);

    // nothing shipped is anywhere near it
    const int fastest_shipped = 3;   // a1
    snprintf(b,sizeof b,"fastest shipped rate is a1's 1/%d, %dx slower than the ceiling",
             1<<fastest_shipped, (1<<fastest_shipped)/(EMA_SCALE/EMA_MUL_MAX));
    ck(fastest_shipped >= EMA_MUL_MIN_RATE, "every shipped rate is inside the clamp", b);
  }

  printf("\n%s (%d failures)\n\n", fails? "FAILED" : "ALL CHECKS PASSED", fails);
  return fails ? 1 : 0;
}
