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

  /* Container name limits.  NAME_CAP is a FORMAT number -- the header parser
     rejects a longer member name, so it bounds what a decode can be asked to
     hold, and the writer never produces one because every name it writes came
     through safe_name() off argv.  PATH_CAP only has to hold out_dir + a name;
     an overlong join is reported, never truncated. */
  NAME_CAP = 4096,
  PATH_CAP = NAME_CAP*2,

  /* Largest block_align any legal file can have: the fmt chunk stores it in a
     u16.  This is what sizes the per-block scratch slabs in the two payload
     walks -- see xad_codec_ima.inc for why they are static and not locals. */
  BA_MAX = 65535,

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

/* ------------------------------------------------------ restrict pointers
 *
 * __restrict is a PROMISE: for the lifetime of the declaration, the object
 * pointed to is reached only through this pointer or one derived from it.
 * Breaking it is undefined behaviour the optimizer acts on silently -- it will
 * not warn, it will just keep a stale value in a register -- so these types are
 * used ONLY where the non-aliasing is structural, i.e. where two pointers come
 * from allocations that cannot be the same one, rather than merely where they
 * happen not to overlap today.
 *
 * WHERE THEY ARE USED: function parameters and block-scope locals, and nowhere
 * else.  The three cases that carry the weight are
 *   - the NLMS kernels, whose (weights, history) pair is two different arena
 *     blocks and can never be one;
 *   - the mixer's (weight row, st[]) pair, likewise;
 *   - the 26 context rows a coded symbol touches, which come from 26 DIFFERENT
 *     CtrTab/CtrSTab allocations, so no two of them can be the same row.
 *
 * WHERE THEY ARE DELIBERATELY NOT USED, because the promise would be false:
 *   - STRUCT MEMBERS, none of them.  Every model table, every weight array and
 *     every history is sub-allocated from Arena::base, so `base` and the table
 *     pointers reach the same bytes by construction, and Arena::prefault()
 *     writes through base after those pointers exist.  A member-level restrict
 *     would be asserting the opposite.
 *   - Hist::buf and Hist::h.  h points INTO buf and push() uses both -- they
 *     alias each other by design, which is the whole mechanism.
 *   - RC::rcio.  It points at one of this coroutine's own pins, so it and
 *     `this` reach the same object.
 * Getting one of these wrong would not show up as a crash; it would show up as
 * a changed archive, which is what the md5 matrix in md5s.sh is for. */
typedef u8* __restrict u8P;
typedef const u8* __restrict cu8P;
typedef u16* __restrict u16P;
typedef u32* __restrict u32P;
typedef i32* __restrict i32P;
typedef const i32* __restrict ci32P;
typedef int* __restrict intP;
typedef const int* __restrict cintP;

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
