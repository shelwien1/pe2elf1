/* xad_logistic.inc -- part of xadpcm.cpp; not standalone.
 *
 * squash / stretch / code-length tables, the per-role counter rate schedules,
 * and init_tables().  SQT and STT feed every mixed bit. */

static const int ST_SCALE = pclamp(G0_ST_SCALE, 16, 2048);

static int SQT[P_ONE];    // squash: d+P_MID -> p12
static short STT[P_ONE];  // stretch: p12 -> d
/* STT with Ctr::pr()'s clamp already applied.  p is u16 and CTR_SH is 4, so
   p>>CTR_SH is in [0,4095]: the upper clamp in pr() is dead and the lower one
   only ever maps 0 to 1.  Folding that into the table turns two compares and a
   select into a load, 26 times per coded bit. */
static short STC[P_ONE];
#if XAD_STATS
static u16 CLT[P_ONE][2]; // -log2 P(bit) in 1/ST_SCALE bit, accounting only
#endif
/* A counter's adaptation is three numbers -- the rate schedule by update
   count, where that schedule stops advancing, and the prior it starts from --
   and the three model roles want different ones.  The code tree sees a symbol
   per sample and wants to track; the block-header fields see a few values per
   block; the literal bytes are near-static.  One shared schedule made all three
   compromise, so each role now carries its own, and the set travels with the
   call site rather than with the counter, which keeps Ctr at four bytes. */
struct RateSet {
  u32 cw[CTR_NRATE];
  int nmax;
  u16 init;
};
static RateSet RS_CODE, RS_HDR, RS_LIT, RS_MATCH;

static void fill_rates(RateSet &r, const int* cw, int nmax, int init) {
  for( int i = 0; i<CTR_NRATE; i++ )
    r.cw[i] = u32(PMUL(cw[i]));
  r.nmax = pclamp(nmax, 1, CTR_NRATE-1);
  r.init = u16(pclamp(init, 1, CTR_MAX));
}

/* ln 2 in Q48, i.e. round(ln2 * 2^48).  The one irrational the tables need, and
   the only place a transcendental enters the format -- as a literal, evaluated
   once by a human, not by whatever libm the build happens to link.  Q48 is not
   arbitrary: the derivation below multiplies it by ST_SCALE (<= 2048 = 2^11)
   and the product must stay inside i64, which caps the format at Q52; Q48
   leaves margin and still rounds KST correctly for every ST_SCALE in [16,2048]
   -- checked against a 60-digit ln2 over the whole range, not sampled. */
static const i64 LN2_Q48 = 195103586505167LL;

/* FNV-1a over SQT and STT at the shipped ST_SCALE.  See the guard at the foot
   of init_tables() for what it is for and why it only fires at ST_SCALE 256.
   -DXAD_SQSTT_HASH=0 disables it, which is what you want while deliberately
   changing the tables -- otherwise the first build of a change aborts before it
   can be measured. */
#ifndef XAD_SQSTT_HASH
#define XAD_SQSTT_HASH 0x7BCBA716u
#endif

static void init_tables() {
  const int ccw[CTR_NRATE] = {G0_CW0, G0_CW1, G0_CW2, G0_CW3, G0_CW4, G0_CW5, G0_CW6, G0_CW7, G0_CW8, G0_CW9, G0_CW10, G0_CW11, G0_CW12, G0_CW13, G0_CW14, G0_CW15};
  const int hcw[CTR_NRATE] = {G0_HW0, G0_HW1, G0_HW2, G0_HW3, G0_HW4, G0_HW5, G0_HW6, G0_HW7, G0_HW8, G0_HW9, G0_HW10, G0_HW11, G0_HW12, G0_HW13, G0_HW14, G0_HW15};
  const int lcw[CTR_NRATE] = {G0_LW0, G0_LW1, G0_LW2, G0_LW3, G0_LW4, G0_LW5, G0_LW6, G0_LW7, G0_LW8, G0_LW9, G0_LW10, G0_LW11, G0_LW12, G0_LW13, G0_LW14, G0_LW15};
  fill_rates(RS_CODE, ccw, G0_CW_NMAX, G0_CP_INIT);
  fill_rates(RS_HDR, hcw, G0_HW_NMAX, G0_HP_INIT);
  fill_rates(RS_LIT, lcw, G0_LW_NMAX, G0_LP_INIT);
  const int mcw[CTR_NRATE] = {G0_MW0, G0_MW1, G0_MW2, G0_MW3, G0_MW4, G0_MW5, G0_MW6, G0_MW7, G0_MW8, G0_MW9, G0_MW10, G0_MW11, G0_MW12, G0_MW13, G0_MW14, G0_MW15};
  fill_rates(RS_MATCH, mcw, G0_MW_NMAX, G0_MP_INIT);

  /* SQT and STT feed every mixed bit, so they must be bit-identical on every
     build that has to interoperate.  They used to be built with exp()/log(),
     which made that a property of the C LIBRARY rather than of this source: a
     libm that rounds one entry differently produces archives the other side
     cannot decode, and nothing in the build would say so.
     How close that was: the tightest entry is STT[2050], where 256*ln(2050/2046)
     lands 1.6e-7 (3.2e-7 relative) from a rounding boundary; SQT's tightest is
     4.3e-8 relative.  Against a correctly-rounded libm at 1 ulp that is nine
     orders of margin and the hazard is theoretical -- glibc here builds the same
     tables under -O3, -Ofast and -ffast-math, measured, all three.  Against an
     APPROXIMATED exp/log, the kind a vendor fast-math library ships at ~1e-7
     relative, it is two or three orders and the hazard is real.  Not a property
     worth resting an archive format on either way.
     They are now pure integer arithmetic, which retires the question: this file
     no longer contains a floating-point operation, and neither does anything
     else on the coding path, so -ffast-math has nothing left to perturb.  That
     does not make -Ofast worth adopting -- it was measured at 0.5% on gcc and
     -1% on clang, inside the noise, and mk.sh still ships -O3 -- it just stops
     it being a correctness question.  Speed was never the point here either;
     init_tables() runs once per process.  LOG2[] is 65536*log2(i) built by
     repeated squaring: square, renormalize below 2^32, count the shifts.  The
     +31 corrects the truncation bias that renormalization accumulates.
     stretch(p) = S*ln(p/(N-p)) = S*ln2 * (log2(p) - log2(N-p)), so one LUT
     difference and one multiply give it; KST is S*ln2/2^16 in Q32, and the >>32
     with a +2^31 bias reproduces floor(x+0.5) exactly, INCLUDING for negative x
     -- which round-away-from-zero does NOT.  (>> on a negative i64 is arithmetic
     in C++20, which this builds as.)
     squash is the inverse, found by binary search on a Q8 stretch and one linear
     interpolation between the bracketing pair, so it rounds the real crossing
     rather than picking the nearest tabulated stretch.  That makes SQT the exact
     inverse of the STT the mixer actually sees, which the float pair -- two
     independently rounded transcendentals -- was only approximately.
     Against the old float tables at the shipped ST_SCALE=256: STT differs in 6
     of 4096 entries and SQT in 65, every one of them by exactly 1, and both stay
     monotone.  Over the whole legal ST_SCALE range [16,2048] the largest
     disagreement anywhere in either table is still 1, and monotonicity holds. */
  {
    /* 65536*log2(i).  Indexed by p and by P_ONE-p with p in [P_MIN,P_MAX], so
       [1,P_ONE-1] is the whole live range and P_ONE entries are enough.  static
       rather than automatic: 16 KB is a quarter of a coroutine stack, and while
       init_tables() runs on the main stack today, nothing here says it must. */
    static u32 LOG2[P_ONE];
    for( u32 i = 0; i<P_ONE; i++ ) {
      u64 w = i; u32 k = 0;
      for( int j = 0; j<16; j++ ) {
        w = w*w; k = k+k;
        while( w >= (1ULL<<32) ) { w = (w+1)>>1; k++; }
      }
      LOG2[i] = k + (i>1 ? 31u : 0u);
    }
    // S*ln2*2^16 in Q32.  Q48 * 2^11 stays well inside i64; see LN2_Q48.
    const i64 KST = (i64(ST_SCALE)*LN2_Q48 + (1LL<<31))>>32;
    struct S {
      const u32* L; i64 K;
      i64 q8(int p) const {                     // stretch in 1/256 units
        return (((i64)L[p] - (i64)L[P_ONE-p])*K + (1LL<<23))>>24;
      }
      int q0(int p) const {                     // stretch, floor(x+0.5)
        return int((((i64)L[p] - (i64)L[P_ONE-p])*K + (1LL<<31))>>32);
      }
    } st = { LOG2, KST };

    for( int p = 0; p<P_ONE; p++ ) {
      int pi = pclamp(p, P_MIN, P_MAX);
      int d = st.q0(pi);
      STT[p] = short(pclamp(d, ST_MIN, ST_MAX));
#if XAD_STATS
      /* code length in 1/ST_SCALE bit: S*(P_BITS - log2 p).  Stats only, never
         coded, so it does not have to agree with the float version -- but it is
         the same arithmetic, so it does, to within the same 1 unit. */
      i64 c1 = ((i64)ST_SCALE*P_BITS*65536 - (i64)LOG2[pi]*ST_SCALE + 32768)>>16;
      i64 c0 = ((i64)ST_SCALE*P_BITS*65536 - (i64)LOG2[P_ONE-pi]*ST_SCALE + 32768)>>16;
      CLT[p][1] = u16(c1<0 ? 0 : c1);
      CLT[p][0] = u16(c0<0 ? 0 : c0);
#endif
    }
    for( int d = -P_MID; d<P_MID; d++ ) {
      i64 t = (i64)d<<8;
      int lo = P_MIN, hi = P_MAX;
      while( lo<hi ) { int m = (lo+hi)>>1; if( st.q8(m)<t ) lo = m+1; else hi = m; }
      int best = lo;
      if( lo>P_MIN ) {
        i64 s1 = st.q8(lo), s0 = st.q8(lo-1);
        if( s1>t && s1!=s0 ) {
          i64 frac = ((t-s0)*256 + (s1-s0)/2)/(s1-s0);
          best = (frac>=128) ? lo : lo-1;
        }
      }
      SQT[d+P_MID] = pclamp(best, P_MIN, P_MAX);
    }
  }

  /* Second pass on purpose.  STC[0] reads STT[P_MIN], and P_MIN > 0, so folding
     this into the loop above reads a table entry that has not been written yet
     -- which is exactly the bug this line replaced. */
  for( int p = 0; p<P_ONE; p++ )
    STC[p] = STT[p<P_MIN ? P_MIN : p];

  /* The tables are the format.  Now that they are integer they are reproducible,
     so a one-line checksum can say so out loud: a build whose SQT/STT differ
     from the shipped pair cannot decode a shipped archive, and this catches that
     at startup instead of as garbage audio.  Only asserted at the shipped
     ST_SCALE -- opt.pl sweeps that parameter, and a sweep must be free to
     produce different tables, which is the point of sweeping it. */
  { u32 h = 2166136261u;                        // FNV-1a over the two tables
    for( int i = 0; i<P_ONE; i++ ) {
      h = (h ^ u32(SQT[i]))*16777619u;
      h = (h ^ u32(u16(STT[i])))*16777619u;
    }
    if( XAD_SQSTT_HASH && ST_SCALE==256 && h!=XAD_SQSTT_HASH ) {
      fprintf(stderr, "xadpcm: squash/stretch table checksum %08X, expected %08X -- "
                      "the tables are pure integer arithmetic, so this means a corrupted "
                      "or mismatched build\n", h, unsigned(XAD_SQSTT_HASH));
      abort();
    }
    if( getenv("XAD_SQSTT") ) fprintf(stderr, "xadpcm: squash/stretch checksum %08X (ST_SCALE=%d)\n", h, ST_SCALE);
  }
}

static INLINE int squash(int d) {
  return SQT[(d<ST_MIN ? ST_MIN : d>ST_MAX ? ST_MAX : d)+P_MID];
}
