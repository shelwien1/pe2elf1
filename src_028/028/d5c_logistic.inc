/* d5c_logistic.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * squash / stretch / code-length tables and init_tables().
 * SQT and STT feed every mixed bit, hence the checksum guard here. */

//------------------------------------------------------------------- logistic tables

// log-bucket: 0 for exact (v==0), else 1+floor(log2(v)) -- feeds IDX threshold maps
/* 1 + floor(log2 v), 0 for v == 0.  The loop this replaces ran up to 18 times
 * on the values actually seen (p^ confidence distances reach 195075) and sits
 * on the dependency chain that produces a coding context, twice per texel.
 * bsr does it in one instruction.  Equivalence was checked exhaustively over
 * the whole of [0, INT_MAX], not sampled. */
static inline int lgb(int v) {
#if defined(__GNUC__)
  return v ? 32 - __builtin_clz(u32(v)) : 0;
#else
  int r = 1; while(v > 1) { v >>= 1; r++; } return v ? r : 0;
#endif
}

static int SQT[4096];    // squash: d+2048 -> p12
static short STT[4096];  // stretch: p12 -> d
static u16 CLT[4096][2]; // code length: -log2(P(bit)) in 1/256 bit, for stats
static void init_tables() {
  const int cw[16] = {G0_CW0, G0_CW1, G0_CW2, G0_CW3, G0_CW4, G0_CW5, G0_CW6, G0_CW7,
                      G0_CW8, G0_CW9, G0_CW10, G0_CW11, G0_CW12, G0_CW13, G0_CW14, G0_CW15};
  for(int i = 0; i < 16; i++) CW[i] = u32(PMUL(cw[i]));
  /* SQT and STT feed every mixed bit, so they must be bit-identical on every
   * build that has to interoperate.  They used to be built with exp()/log(),
   * which made that a property of the C library: a -ffast-math exp, or simply
   * a different libm, could round one entry differently and produce archives
   * the other side cannot decode.  The margin was measured at 1.6e-7 absolute
   * (8e-11 relative) -- safe for a correctly-rounded libm, but only two or
   * three orders from an approximated one, and not a property worth resting a
   * format on.
   *
   * They are now pure integer arithmetic.  LOG2[] is 65536*log2(i) built by
   * repeated squaring: square, renormalize below 2^32, count the shifts.  The
   * +31 corrects the truncation bias that renormalization accumulates.
   *
   * stretch(p) = 256*ln(p/(4096-p)) = 256*ln2 * (log2(p) - log2(4096-p)),
   * so one LUT difference and one multiply give it; KST is 256*ln2/2^16 in
   * Q32, and the >>32 with a +2^31 bias reproduces floor(x+0.5) exactly,
   * including for negative x -- which round-away-from-zero does NOT, and that
   * mismatch alone accounted for half the table on the first attempt.
   *
   * squash is the inverse, found by binary search on a Q8 stretch and one
   * linear interpolation between the bracketing pair, so it rounds the real
   * crossing rather than picking the nearest tabulated stretch.
   *
   * Against the old float tables: STT differs in 6 of 4096 entries and SQT in
   * 65, every one of them by exactly 1, and both stay monotone. */
  {
    static u32 LOG2[8192];
    for(u32 i = 0; i < 8192; i++) {
      u64 w = i; u32 k = 0;
      for(int j = 0; j < 16; j++) {
        w = w * w; k = k + k;
        while(w >= (1ULL << 32)) { w = (w + 1) >> 1; k++; }
      }
      LOG2[i] = k + (i > 1 ? 31u : 0u);
    }
    const i64 KST = 11629080;                   // 256*ln2 / 2^16, in Q32
    struct S {
      const u32* L; i64 K;
      i64 q8(int p) const {                     // stretch in 1/256 units
        return (((i64)L[p] - (i64)L[4096 - p]) * K + (1LL << 23)) >> 24;
      }
      int q0(int p) const {                     // stretch, floor(x+0.5)
        return int((((i64)L[p] - (i64)L[4096 - p]) * K + (1LL << 31)) >> 32);
      }
    } st = { LOG2, KST };

    for(int p = 0; p < 4096; p++) {
      int pi = p < 1 ? 1 : p > 4095 ? 4095 : p;
      int d = st.q0(pi);
      STT[p] = short(d < -2047 ? -2047 : d > 2047 ? 2047 : d);
      // code length in 1/256 bit: 256*(12 - log2(p)); stats only, never coded
      i64 c1 = (3072LL * 65536 - (i64)LOG2[pi] * 256 + 32768) >> 16;
      i64 c0 = (3072LL * 65536 - (i64)LOG2[4096 - pi] * 256 + 32768) >> 16;
      CLT[p][1] = u16(c1 < 0 ? 0 : c1);
      CLT[p][0] = u16(c0 < 0 ? 0 : c0);
    }
    for(int d = -2048; d < 2048; d++) {
      i64 t = (i64)d << 8;
      int lo = 1, hi = 4095;
      while(lo < hi) { int m = (lo + hi) >> 1; if(st.q8(m) < t) lo = m + 1; else hi = m; }
      int best = lo;
      if(lo > 1) {
        i64 s1 = st.q8(lo), s0 = st.q8(lo - 1);
        if(s1 > t && s1 != s0) {
          i64 frac = ((t - s0) * 256 + (s1 - s0) / 2) / (s1 - s0);
          best = (frac >= 128) ? lo : lo - 1;
        }
      }
      SQT[d + 2048] = best < 1 ? 1 : best > 4095 ? 4095 : best;
    }
  }
  { u32 h = 2166136261u;                       // FNV-1a over the two tables
    for(int i = 0; i < 4096; i++) { h = (h ^ u32(SQT[i])) * 16777619u; h = (h ^ u32(u16(STT[i]))) * 16777619u; }
    if(D5C_SQSTT_HASH && h != u32(D5C_SQSTT_HASH)) {
      fprintf(stderr, "dxt5comp: squash/stretch table checksum %08X, expected %08X -- "
                      "the tables are pure integer arithmetic, so this means a corrupted or mismatched build\n", h, u32(D5C_SQSTT_HASH));
      abort();
    }
    if(getenv("D5C_SQSTT")) fprintf(stderr, "d5c: squash/stretch checksum %08X\n", h);
  }
}
static inline int squash(int d) { return SQT[(d < -2047 ? -2047 : d > 2047 ? 2047 : d) + 2048]; }

/* The stream-side buffers -- the coded stream, the verbatim blob area, a
 * decoded container gap, the external endpoint plane -- are sized by the DATA,
 * not by the texture geometry, so unlike everything else in this file they
 * cannot become Table()s or MAX_DIM slabs.  What they can do is stop being
 * std::vector: append and index is all they are ever asked for, and a local
 * type is what lets the core drop its last standard-container dependency and
 * hand the same buffer to a frontend that has its own conventions.
 *
 * Growth is 1.5x, so a stream of N bytes still costs amortized O(N).  There
 * are no exceptions here (the shipping builds use -fno-exceptions), so an
 * allocation failure aborts, which is what a vector would do anyway. */
