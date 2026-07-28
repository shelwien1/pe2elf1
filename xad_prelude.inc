/* xad_prelude.inc -- part of xadpcm.cpp; not standalone.
 *
 * types are already in place from Lib3's common.inc; what is here is the
 * arithmetic contract -- the numbers that are NOT tunables -- and the clamp
 * helpers the generated IDX headers need for their sizes.  Included BEFORE
 * sh_mapping.inc and the generated MOD headers for that reason.  The half of the IDX bridge
 * that has to come AFTER them is in xad_idxparam.inc. */

/* The arithmetic contract, and NOT tunables.  Every number below either sizes
   an array, names a stream, or fixes the coder's fixed-point convention: change
   one and you do not get a differently-tuned codec, you get a broken one.  That
   is the line between this block and the idx files -- anything opt.pl can sweep
   freely lives there, anything that would break if swept lives here.

   The mixer pair is the one that bites.  MIX_NIN is the weight-row stride and
   MIX_MAXIN the st[] capacity, and both must stay AHEAD of the number of add()
   calls per bit: overrunning st[] writes into the mixer's own nx and cx fields,
   an intra-object overflow ASan does not instrument, so it presents as a
   segfault at -O0 and a live spin at -O2.  Raise both when adding a context. */
enum {
  P_BITS = 12,         // range coder probability resolution
  P_ONE = 1<<P_BITS,   // 4096
  P_MAX = P_ONE-1,     // 4095, clamped away from certainty
  P_MIN = 1,
  P_MID = P_ONE>>1,   // 2048
  CTR_BITS = 16,      // counter probability storage width
  CTR_ONE = 1<<CTR_BITS,
  CTR_MAX = CTR_ONE-1,        // 65535, target for a 1 bit
  CTR_INIT = CTR_ONE>>1,      // 32768, unbiased prior
  CTR_SH = CTR_BITS-P_BITS,   // storage -> coder resolution
  CTR_NRATE = 16,             // rate-schedule entries; the index saturates here
  ST_MAX = P_MID-1,           // stretch domain is +-ST_MAX
  ST_MIN = -ST_MAX,

  RC_TOP = 1<<24,   // range coder renormalisation threshold
  RC_PRIME = 5,     // bytes the decoder primes its window with

  /* Mixer geometry.  MIX_NIN is the weight-row stride, so it must be >= the
     number of add() calls per bit: the rows are strided by nin, and an extra
     input reads -- then upd() WRITES -- into the next context's row.  Silent,
     and it corrupts a different context rather than crashing, so adding a
     counter to the tree without widening this does exactly that. */
  MIX_NIN = 28, // bias + 21 counters + slow twin, five spare
  /* st[] capacity.  This must be >= the number of add() calls per bit, and
     nothing checks it: overflowing st[] writes into the Mixer's own nx/cx
     fields, which is an INTRA-object overflow that ASan does not see, so the
     symptom is a wild weight-row index -- a segfault at -O0 and a live spin at
     -O2.  Keep real headroom over MIX_NIN. */
  MIX_MAXIN = 40,
  /* Weight-row STRIDE, as opposed to MIX_NIN which is the contract's floor on
     it.  Layout only: the loops run to this constant instead of to nx, which is
     26 on every mbit call, and st[26..MIX_STRIDE) is zeroed once at init and
     never written again.  A zero input contributes 0 to the dot product and its
     weight update is (0*el)>>20 = 0, so the wider loop is bit-exact -- and with
     a compile-time trip count it is four whole vectors with no remainder.  At
     stride 28 a row was 112 B and straddled two or three cache lines; at 32,
     64B-aligned, it is exactly two. */
  MIX_STRIDE = 32,
  MIX_SH = 2*P_BITS-8,     // dot product -> stretch domain
  MIX_LRSH = 20,           // weight step: st*err*MW >> MIX_LRSH

  /* code_signed: a zigzagged value as a magnitude class then the mantissa
     below the leading one.  The class is coded as a fixed SGN_CBITS-bit tree,
     so the tree's node space is 1<<SGN_CBITS wide. */
  SGN_CBITS = 5,
  SGN_NODES = 1<<SGN_CBITS,
  SGN_NMODEL = 6, // distinct header fields with their own rows

  /* Literal byte models.  Each stream is 256 rows of order-1 context. */
  LIT_W = 256,
  LIT_NSTREAM = 5,
  LIT_HEAD = 0,  // stored RIFF header
  LIT_TAIL = 1,  // trailing chunks
  LIT_MISC = 2,  // runt blocks, partial groups, odd bits
  LIT_BPRED = 3, // MS coefficient-set byte, order-1
  LIT_GAP = 4,   // bytes ahead of the first wav in a solid stream
  /* Two MISC uses key on a synthetic previous byte rather than a real one, to
     get their own rows: they share a row with any literal whose predecessor
     happened to equal these, which costs a little dilution and no correctness. */

  // -v accounting buckets
  ST_CODE = 0,
  ST_HDR = 1,
  ST_LIT = 2,
  ST_FLAG = 3,
  ST_NCAT = 4,

  /* code_signed models.  Each is one header field with its own class and
     mantissa rows; SGN_NMODEL above must cover them. */
  SM_SAMP = 0,   // IMA/MS first sample of a block
  SM_INDEX = 1,  // IMA step index
  SM_DELTA = 2,  // MS iDelta against the running value
  SM_SAMP1 = 3,  // MS iSamp1
  SM_DELTAH = 4, // MS iDelta against the previous header
};

/* -v byte accounting.  `stat[scat] += CLT[pf][b]` ran on every coded bit and
   kept the 16 KB CLT table hot in L1 for the benefit of a report nobody asks
   for in a normal run.  XAD_STATS=1 puts it back; the default build has neither
   the adds nor the table.  Both builds code identically -- this is accounting,
   not model state. */
#ifndef XAD_STATS
#define XAD_STATS 0
#endif
#if XAD_STATS
#define XSTAT(cat, p, b) do { stat[cat] += CLT[p][b]; } while( 0 )
#else
#define XSTAT(cat, p, b) do { } while( 0 )
#endif

static constexpr int pclamp(int v, int lo, int hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

#define PMUL(x) pclamp((x), 1, 65535) // 16-bit fixed-point rate multiplier
#define PAPN(x) pclamp((x), 2, 2048)  // APM interpolation interval count
