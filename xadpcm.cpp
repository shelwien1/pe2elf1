// xadpcm.cpp -- standalone lossless compressor for IMA- and MS-ADPCM .wav files
//
// The ADPCM codes are not transformed, only modelled: at every sample the
// reconstructed signal is predicted, the prediction is pushed through the same
// quantizer the encoder used, and the resulting predicted code q^ becomes
// the primary context for coding the real code (Huang, "Lossless Compression
// for mu-Law (A-Law) and IMA ADPCM on the Basis of a Fast RLS Algorithm" --
// with the paper's Huffman-of(q - q^) replaced by context mixing, since the
// difference is only one of several contexts worth having).
//
//   * signal predictor: 3-stage cascaded NLMS (48/192/192 taps) over the
//     reconstructed stream, each stage predicting the previous stage's
//     residual, stage 1 initialised to linear extrapolation.  Integer only,
//     energy-normalised by a power of two -- encoder and decoder must agree
//     bit for bit under any build flags.
//   * q^ = Q(r^ - r[t-1] | step), plus the quantizer's leftover as a
//     confidence bucket: it says how close the prediction sat to the cell
//     boundary, which is exactly when the real code lands one step away.
//     The partial sums (stage 1, stages 1+2) are quantized too and kept as
//     separate contexts -- short and long horizons disagree on transients and
//     on periodic material, and the mixer is better placed to choose than any
//     single filter length is.
//   * codes are coded as raw MSB-first bit trees (sign bit, then magnitude)
//     over 11 contexts: (q^,conf), step index bucket, previous code, previous
//     code pair, other channel at the same instant, (q^,prev), (q^,index),
//     (q^,other), (index,prev,conf), and the two shorter-horizon q^ views,
//     plus a slow twin of the primary.  Logistic mixing selected by
//     (node,conf), then two APM stages on (q^,node) and (index,node).
//   * block headers ride the same predictor: the verbatim first sample of a
//     block is coded as a residual against r^, the step index against the
//     index carried out of the previous block, the reserved byte as a flag.
//   * everything the old sidecar carried -- RIFF header (regenerated when it
//     matches the canonical ADPCM-XQ layout, else literal), trailing chunks,
//     odd bytes and unused bits of partial blocks -- goes into the same
//     stream through an order-1 literal model.
//
// MS ADPCM (WAVE_FORMAT_ADPCM, 0x02) rides the same machinery.  Only the
// three format-specific pieces change:
//
//   * the signal model is the block's own 2-tap predictor
//     P = (iSamp1*c1 + iSamp2*c2)/256 with the coefficient pair the block
//     header selects, and the code is the residual (r - P)/iDelta rounded to
//     a 4-bit signed cell, so q^ = Q((r^ - P)/iDelta) and the confidence
//     bucket is the *signed* position of r^ - P inside its cell -- MS rounds
//     to nearest, so which side of centre the prediction fell on says which
//     way the real code is likely to miss.
//   * the IMA step index is replaced by a log bucket of iDelta, four steps
//     to the octave, as the scale context.
//   * the block header is 7 bytes per channel (coefficient set, iDelta,
//     iSamp1, iSamp2, each field planar across the channels).  iSamp2 is the
//     block's first sample in time and iSamp1 the second, so both are coded
//     as residuals against the signal predictor; iDelta is coded against the
//     previous block's iDelta and the coefficient index order-1 against the
//     previous block's.  Nibbles are high-first and strictly interleaved by
//     channel, so no deinterleave step is needed.
//
// A stream may hold any number of wavs: the encoder scans for RIFF/WAVE headers
// that also parse as ADPCM with a usable geometry, and each hit starts a new
// segment.  Anything ahead of the first wav rides along as literal data;
// anything between two wavs lands in the earlier one's tail, since a segment
// spans exactly up to the next one's start.  The decoder never repeats the scan
// (it reads the segment table), so a false RIFF inside coded audio costs a
// little ratio and nothing else.
//
// At a boundary the ADPCM decoder state and the signal predictor always reset.
// What the MODEL keeps is the caller's choice, because it only pays when the
// next wav resembles the last and nothing inside the codec can tell whether it
// does: default resets everything, -s carries statistics along a run of wavs of
// the same codec and code width, -ss carries them unconditionally.  The mode is
// recorded in the container, so the decoder needs no flag.
//
// The signal predictor runs TWO cascades side by side over the same signal,
// with independently tuned shapes, and the mixer decides per file which to
// believe.  Separate optimizer runs kept landing on genuinely different optima
// -- one long-horizon, one short with a wider cross-channel stage -- each
// clearly better on some material and clearly worse on other.  That is not a
// tuning ambiguity to resolve by picking a winner: it says the two shapes see
// different things.  The B-side shape is the N*B / MU*B / EFL*B / PW0B block in
// the idx file; setting it equal to the primary makes the second cascade
// redundant rather than harmful.
//
// Stereo is modelled twice over: once in the signal domain, by a cross-channel
// NLMS stage, and once in the code domain, by nine contexts that cross this
// channel's recent codes with the other channel's, the deepest of them hashed.
// The code-domain half turned out to matter far more on real music -- see the
// joint-context block in the Codec.  Working set is about 75 MB, most of it the
// two match models' history and hash tables.
//
// The signal predictor is a 4-stage NLMS cascade.  Three stages work on the
// channel's own reconstructed signal at different horizons; the fourth reads
// the OTHER channel's recent deltas and picks up what they leave over.  That
// last one exists because a byte-aligned general-purpose coder gets same-instant
// stereo correlation for free -- in MS ADPCM a byte is one nibble from each
// channel -- and on real music that is worth about a bit per nibble pair.  It is
// short on purpose (8 taps): longer ones add gradient noise on material whose
// channels are only loosely related, and measured worse on every file.
//
// Two match models run alongside the context models: they look for the last
// occurrence of the recent code sequence and predict what followed it.  A short
// window (5 codes) acts as a high-order context; a long one (12) catches actual
// repeats -- looped samples, duplicated assets, the same file twice in a solid
// stream.  Both build their history from decoded symbols, so nothing about a
// match is transmitted, and the history survives a wav boundary on purpose.
//
// container: "XAC1" u8 version, varint nseg, varint lead, per-segment
// parameters, an optional file table, u32 crc32, rc stream.  decode rebuilds
// the whole stream and verifies the crc before writing anything.
//
// handles 2/3/4/5-bit ADPCM-XQ code widths, mono/multichannel IMA, mono/stereo
// MS, non-standard MS coefficient tables, truncated and partial final blocks.
//
// Every rate in the coder is a 16-bit fixed-point MULTIPLIER rather than a
// shift, and every tunable number lives in IDX/*.idx instead of a #define:
//
//   IDX/xad-G0.idx   counter rate schedules, mixer, APM, both NLMS cascades,
//                    and the coder-shape values that are safe to sweep
//   IDX/xad-C0.idx   the shared context quantizer: match length
//   IDX/xad-I0.idx   IMA-only: the step-index quantizer
//   IDX/xad-M0.idx   MS-only: the iDelta quantizer, the nibble bit-tree order,
//                    the confidence rule, the iDelta-predictor tracker decay
//
// What is NOT in an idx file is the arithmetic contract -- probability
// resolution, counter width, array capacities, stream identifiers -- because
// sweeping those does not produce a differently-tuned codec, it produces a
// broken one.  That block is at the top of this file and says so.
//
// idx2inc.pl turns those into MOD/*.inc.  The Debug build leaves each value in
// the binary as a pattern string, which IDX/opt.pl flips bit by bit against a
// corpus; import.pl folds the winners back into the .idx sources.  The Const
// build evaluates the patterns at preprocess time and is what ships.  Both
// builds code identically -- only the indirection differs.
//
// STRUCTURE (rev 016 -> coroutine port).  The codec is a Lib3 coroutine:
//
//   Coroutine  <-  RC<MD>  <-  Xad<MD>  <-  XadDrive<Xad<MD>>
//
// The range coder is not an object the frontend owns, it is a LAYER of the
// frontend -- one coroutine, one stack -- so when the output window fills deep
// inside the bit tree, the pin's yield suspends the whole codec call stack and
// returns to the driver for a flush.  Every byte the process reads or writes
// moves through a coroutine pin: pin[0] is the input window, pin[1] the output
// window, and XadDrive services them from stdio.  Nothing is fread whole any
// more -- in particular the coded stream is no longer materialised in memory on
// either side.
//
// MD is the direction (0 encode, 1 decode) as a template parameter, so every
// `enc ?` in the model folds at compile time and neither instantiation carries
// the other's code.  Three more parameters are compile-time because they are
// fixed for a whole segment and were being retested per coded bit: FMT (IMA or
// MS), BPS (2..5, and 4 for MS), and XST (mono or multi-channel).  They
// parameterise the payload WALK, not the model tables -- see the note on
// Codec<MD> for why templating the tables on FMT would break -ss.
//
// build: ./mk.sh            Debug (optimizable)
//        ./mk.sh release    Const (ship this)
//        or, without perl:  g++ -std=gnu++20 -O2 -I. -ILib3 xadpcm.cpp -o xadpcm
// tune:  ./mk.sh && perl IDX/opt.pl opt.lst
//        cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! >t && mv t $f; done
// usage: xadpcm c input.wav output
//        xadpcm d output restored.wav

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

/* Lib3.  common.inc supplies byte/word/uint/qword, INLINE/NOINLINE/ALIGN,
   if_e0/if_e1 and __assume, all of which coro3b.inc needs; coro3b.inc defines
   Coroutine, which RC derives from.  Both must precede everything below. */
#include "common.inc"
#include "coro3b.inc"

// #include "adpcm-lib.h"

int adpcm_block_size_to_sample_count(int block_size, int num_chans, int bps) {
  return (block_size-num_chans*4)/num_chans*8/bps+1;
}

/* Deliberately NOT `using namespace std`: the IDX generator emits Const-mode
   tables typed `byte` and `word`, and a using-directive would make those names
   ambiguous against std::byte rather than shadowing it.  common.inc declares
   both at global scope, so they win over std's. */
using std::string;
using std::vector;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
typedef int64_t i64;

#ifndef __min
#define __min(a, b) ((a)<(b) ? (a) : (b))
#define __max(a, b) ((a)>(b) ? (a) : (b))
#endif

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

static constexpr int pclamp(int v, int lo, int hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

#define PMUL(x) pclamp((x), 1, 65535) // 16-bit fixed-point rate multiplier
#define PAPN(x) pclamp((x), 2, 2048)  // APM interpolation interval count

/* IDX-generated parameters and context quantizers -- see the IDX directory.
   In the
   Debug build these are `mapping` objects whose pattern strings live in the
   binary, where opt.pl flips their bits; in the Const build idx2inc.pl has
   already folded them to literals. */
#include "sh_mapping.inc"
#include "MOD/xad-C0_h.inc"
#include "MOD/xad-C0_p.inc"
#include "MOD/xad-G0_h.inc"
#include "MOD/xad-I0_h.inc"
#include "MOD/xad-I0_p.inc"
#include "MOD/xad-M0_h.inc"
#include "MOD/xad-M0_p.inc"

#define NCONF pclamp(G0_NCONF, 1, 16)

static int verbose = 0;

// ------------------------------------------------------------------- helpers

/* No load_file / save_file any more.  Both directions read and write through
   coroutine pins (Xad::slurp, Xad::putn and the RC's own pin), and the only
   stdio left in the program is in XadDrive, which is the one place allowed to
   know what a FILE* is. */

static u32 crc32b(const u8* d, size_t n) {
  static u32 T[256];
  static bool init = false;
  if( !init ) {
    for( u32 i = 0; i<256; i++ ) {
      u32 c = i;
      for( int k = 0; k<8; k++ )
        c = c&1 ? 0xEDB88320u^(c>>1) : c>>1;
      T[i] = c;
    }
    init = true;
  }
  u32 c = 0xFFFFFFFFu;
  for( size_t i = 0; i<n; i++ )
    c = T[(c^d[i])&0xFF]^(c>>8);
  return c^0xFFFFFFFFu;
}

static INLINE u32 rd16(const u8* p) {
  return u32(p[0])|(u32(p[1])<<8);
}

static INLINE u32 rd32(const u8* p) {
  return rd16(p)|(rd16(p+2)<<16);
}

static INLINE void wr16(u8* p, u32 v) {
  p[0] = u8(v);
  p[1] = u8(v>>8);
}

static INLINE void wr32(u8* p, u32 v) {
  wr16(p, v&0xFFFF);
  wr16(p+2, v>>16);
}

static INLINE u64 zig(i64 v) {
  return (u64(v)<<1)^u64(v>>63);
}

static INLINE i64 unzig(u64 u) {
  return i64(u>>1)^-i64(u&1);
}

/* Strip any directory part from a stored name.  Archives name their members,
   and a member called ../../etc/x must not be able to say where it lands. */
static string safe_name(const string &s) {
  size_t k = s.find_last_of("/\\");
  string b = k==string::npos ? s : s.substr(k+1);
  if( b.empty()||b==string(".")||b==string("..") )
    b = "member";
  return b;
}

// ------------------------------------------------------------------- logistic tables

static const int ST_SCALE = pclamp(G0_ST_SCALE, 16, 2048);

static int SQT[P_ONE];    // squash: d+P_MID -> p12
static short STT[P_ONE];  // stretch: p12 -> d
static u16 CLT[P_ONE][2]; // -log2 P(bit) in 1/ST_SCALE bit, accounting only
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

static void init_tables() {
  const int ccw[CTR_NRATE] = {G0_CW0, G0_CW1, G0_CW2, G0_CW3, G0_CW4, G0_CW5, G0_CW6, G0_CW7, G0_CW8, G0_CW9, G0_CW10, G0_CW11, G0_CW12, G0_CW13, G0_CW14, G0_CW15};
  const int hcw[CTR_NRATE] = {G0_HW0, G0_HW1, G0_HW2, G0_HW3, G0_HW4, G0_HW5, G0_HW6, G0_HW7, G0_HW8, G0_HW9, G0_HW10, G0_HW11, G0_HW12, G0_HW13, G0_HW14, G0_HW15};
  const int lcw[CTR_NRATE] = {G0_LW0, G0_LW1, G0_LW2, G0_LW3, G0_LW4, G0_LW5, G0_LW6, G0_LW7, G0_LW8, G0_LW9, G0_LW10, G0_LW11, G0_LW12, G0_LW13, G0_LW14, G0_LW15};
  fill_rates(RS_CODE, ccw, G0_CW_NMAX, G0_CP_INIT);
  fill_rates(RS_HDR, hcw, G0_HW_NMAX, G0_HP_INIT);
  fill_rates(RS_LIT, lcw, G0_LW_NMAX, G0_LP_INIT);
  const int mcw[CTR_NRATE] = {G0_MW0, G0_MW1, G0_MW2, G0_MW3, G0_MW4, G0_MW5, G0_MW6, G0_MW7, G0_MW8, G0_MW9, G0_MW10, G0_MW11, G0_MW12, G0_MW13, G0_MW14, G0_MW15};
  fill_rates(RS_MATCH, mcw, G0_MW_NMAX, G0_MP_INIT);
  for( int d = -P_MID; d<P_MID; d++ ) {
    int p = int(double(P_ONE)/(1.0+exp(-double(d)/double(ST_SCALE)))+0.5);
    SQT[d+P_MID] = p<P_MIN ? P_MIN : p>P_MAX ? P_MAX : p;
  }
  for( int p = 0; p<P_ONE; p++ ) {
    double q = p<P_MIN ? P_MIN : p>P_MAX ? P_MAX : p;
    int d = int(floor(double(ST_SCALE)*log(q/(double(P_ONE)-q))+0.5));
    STT[p] = short(d<ST_MIN ? ST_MIN : d>ST_MAX ? ST_MAX : d);
    CLT[p][1] = u16(-double(ST_SCALE)*log2(q/double(P_ONE))+0.5);
    CLT[p][0] = u16(-double(ST_SCALE)*log2(1.0-q/double(P_ONE))+0.5);
  }
}

static INLINE int squash(int d) {
  return SQT[(d<ST_MIN ? ST_MIN : d>ST_MAX ? ST_MAX : d)+P_MID];
}

// ------------------------------------------------------------------- range coder (LZMA-style binary)

/* MD 0 = encode, 1 = decode.  `enc` is a class constant carrying the name the
   runtime flag had, so every `if(enc)` and `enc ? :` here and in Codec folds at
   compile time and the bodies did not need editing.  The decode instantiation
   carries no shift_low / cache / carry code at all, and the encode one no
   code/rd path.

   RC derives from Coroutine and is meant to be a BASE of the frontend that
   drives it (Xad<MD>), not a separate coroutine instance: one Coroutine, one
   stack, RC as a layer in the chain.  The compressed bytes move through `rcio`,
   one of that coroutine's pins, so when the window runs dry deep inside the bit
   tree the pin's yield suspends the whole codec call stack and returns to the
   driver for a refill or a flush.

   rcio defaults to the inherited put()/get() streams -- pin[1] when encoding,
   pin[0] when decoding -- which is exactly what xadpcm wants, since the coded
   stream IS its output (resp. input).  The argument exists for frontends with a
   second stream. */
template<int MD>
struct RC : Coroutine {
  enum { enc = (MD==0) };
  coro3_pin* rcio = nullptr;
  u32 range = 0, code = 0;
  u64 low = 0;
  u8 cache = 0;
  u64 cachesz = 0;
  /* Replaces the old out->size() deltas: with the stream going straight out
     through a pin there is no buffer to measure. */
  u64 emitted = 0, consumed = 0;

  coro3_pin* rc_defpin() {
    return &pin[enc ? 1 : 0];
  }

  void init_enc(coro3_pin* p = nullptr) {
    rcio = p ? p : rc_defpin();
    low = 0;
    range = 0xFFFFFFFFu;
    cache = 0;
    cachesz = 1;
    emitted = 0;
  }

  void init_dec(coro3_pin* p = nullptr) {
    rcio = p ? p : rc_defpin();
    range = 0xFFFFFFFFu;
    code = 0;
    consumed = 0;
    for( int i = 0; i<RC_PRIME; i++ )
      code = code<<8|rd();
  }

  /* A pin get() returns uint(-1) at EOF+f_quit; mapping that to 0 keeps the old
     `inpos<insize ? in[inpos++] : 0` behaviour exactly -- a truncated stream
     decodes to zeros and is caught by the entry crc rather than overrunning. */
  u8 rd() {
    uint v = rcio->get();
    consumed++;
    return v>255 ? 0 : u8(v);
  }

  void shift_low() {
    if( u32(low)<0xFF000000u||(low>>32)!=0 ) {
      u8 c = cache;
      do {
        rcio->put(u8(c+u8(low>>32)));
        emitted++;
        c = 0xFF;
      } while( --cachesz );
      cache = u8(low>>24);
    }
    cachesz++;
    low = u32(low)<<8;
  }

  // p1 = P(bit==1), 12-bit, in [1,4095]
  INLINE int bit(int p1, int b) {
    u32 bound = (range>>P_BITS)*u32(p1);
    if( enc ) {
      if( b )
        range = bound;
      else {
        low += bound;
        range -= bound;
      }
      while( range<u32(RC_TOP) ) {
        range <<= 8;
        shift_low();
      }
    } else {
      b = code<bound;
      if( b )
        range = bound;
      else {
        code -= bound;
        range -= bound;
      }
      while( range<u32(RC_TOP) ) {
        code = code<<8|rd();
        range <<= 8;
      }
    }
    return b;
  }

  void flush() {
    for( int i = 0; i<RC_PRIME; i++ )
      shift_low();
  }

  // raw bytes straight onto the coded stream, outside any model
  void raw_put(const u8* d, size_t n) {
    for( size_t i = 0; i<n; i++ ) {
      rcio->put(d[i]);
      emitted++;
    }
  }

  void raw_get(u8* d, size_t n) {
    for( size_t i = 0; i<n; i++ )
      d[i] = rd();
  }
};

// ------------------------------------------------------------------- counters / mixer / apm

/* The counters, mixer and APM all step by a 16-bit fixed-point MULTIPLIER
   rather than a shift.  mul = 65536>>shift reproduces any shift exactly -- the
   product is exact and the >>16 is the same arithmetic shift, negative deltas
   included -- so this is byte-identical to the shift model at the seeded
   values.  What it buys is the space between the powers of two, which a shift
   schedule can never reach and which is where the rate schedule (the biggest
   knob found by hand) is most likely to still have something left. */

struct Ctr {
  u16 p = CTR_INIT;
  u16 n = 0;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit, const RateSet &rs) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(rs.cw[n]))>>CTR_BITS));
    if( n<rs.nmax )
      n++;
  }
};

struct CtrS { // slow fixed-rate twin of the primary code counter
  u16 p = CTR_INIT;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(PMUL(G0_SLOW_MUL)))>>CTR_BITS));
  }
};

struct CtrTab {
  vector<Ctr> v;
  int w = 0;

  void init(int rows, int width, const RateSet &rs) {
    Ctr c;
    c.p = rs.init;
    w = width;
    v.assign(size_t(rows)*size_t(width), c);
  }

  INLINE Ctr*row(int r) {
    return &v[size_t(r)*size_t(w)];
  }
};

struct CtrSTab {
  vector<CtrS> v;
  int w = 0;

  void init(int rows, int width) {
    CtrS c;
    c.p = u16(pclamp(G0_SP_INIT, 1, CTR_MAX));
    w = width;
    v.assign(size_t(rows)*size_t(width), c);
  }

  INLINE CtrS*row(int r) {
    return &v[size_t(r)*size_t(w)];
  }
};

struct Mixer {
  int nin = 0;
  vector<int> w;
  int st[MIX_MAXIN];
  int nx = 0, cx = 0, pr_ = P_MID, lr = 256;

  void init(int ni, int nctx, int lrate) {
    nin = ni;
    lr = PMUL(lrate);
    w.assign(size_t(ni)*size_t(nctx), G0_W_INIT);
  }

  INLINE void add(int s) {
    st[nx++] = s;
  }

  INLINE int mix(int ctx) {
    cx = ctx;
    i64 s = 0;
    int* W = &w[size_t(cx)*size_t(nin)];
    for( int i = 0; i<nx; i++ )
      s += i64(st[i])*W[i];
    pr_ = squash(int(s>>MIX_SH));
    return pr_;
  }

  INLINE void upd(int bit) {
    int err = (bit<<P_BITS)-pr_;
    int* W = &w[size_t(cx)*size_t(nin)];
    int cl = 1<<pclamp(G0_W_CLAMPL, 8, 29);
    for( int i = 0; i<nx; i++ ) {
      int nw = W[i]+int((i64(st[i])*i64(err)*i64(lr))>>MIX_LRSH);
      W[i] = nw<-cl ? -cl : nw>cl ? cl : nw;
    }
    nx = 0;
  }
};

/* The interpolation fraction is P_BITS wide whatever N is, so the interval
   count stops being tied to a power of two: N = 32 gives the old 33 cells and
   the old >>7 / &127 split exactly, and anything else is now reachable. */
struct APM {
  vector<u16> t;
  int idx = 0, wt = 0;
  int N = 0, ncl = 0, rt = 0; // set by init() from the idx values

  void init(int nctx, int n, int rate) {
    N = PAPN(n);
    ncl = N+1;
    rt = PMUL(rate);
    t.assign(size_t(nctx)*size_t(ncl), 0);
    for( int c = 0; c<nctx; c++ )
      for( int j = 0; j<ncl; j++ )
        t[size_t(c)*size_t(ncl)+size_t(j)] = u16(squash(j*P_ONE/N-P_MID)<<CTR_SH);
  }

  INLINE int pp(int pr, int cx) {
    int s = STT[pr]+P_MID;
    int sn = s*N, i = sn>>P_BITS, f = sn&P_MAX;
    idx = cx*ncl+i;
    wt = f;
    return (t[idx]*(P_ONE-f)+t[idx+1]*f)>>CTR_BITS;
  }

  INLINE void upd(int bit) {
    int g = bit ? CTR_MAX : 0; // rt = 65536 -> full step at full weight
    t[idx] = u16(t[idx]+int((i64(g-int(t[idx]))*i64(P_ONE-wt)*i64(rt))>>(P_BITS+CTR_BITS)));
    t[idx+1] = u16(t[idx+1]+int((i64(g-int(t[idx+1]))*i64(wt)*i64(rt))>>(P_BITS+CTR_BITS)));
  }
};

// ------------------------------------------------------------------- IMA ADPCM arithmetic
// mirrors adpcm-lib; drives prediction only, so a mismatch would cost ratio,
// never correctness

static const u16 step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

/* These four all take the code width, and all four sit on the per-symbol path:
   index_update ran a switch on it once per coded symbol, and the other three
   ran bps-1-trip loops.  The callers now know BPS at compile time, so it
   becomes a template parameter here too -- same arithmetic, no branch, the
   loops fully unrolled.  The tables are at file scope rather than function
   statics so the `if constexpr` arms do not each carry their own. */
static const signed char ima_t3[4] = {-1, -1, 1, 2};
static const signed char ima_t4[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static const signed char ima_t5[16] = {-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16};

template<int BPS>
static INLINE int index_update(int index, int code) {
  if constexpr( BPS==2 )
    index += (code&1)*3-1;
  else if constexpr( BPS==3 )
    index += ima_t3[code&3];
  else if constexpr( BPS==5 )
    index += ima_t5[code&15];
  else
    index += ima_t4[code&7];
  return index<0 ? 0 : index>88 ? 88 : index;
}

// magnitude a code reconstructs to: base + the set bit weights
template<int BPS>
static INLINE int code_magnitude(int code, int step) {
  const int nb = BPS-1;
  int mag = step>>nb;
  for( int i = 0; i<nb; i++ )
    if( code&(1<<(nb-1-i)) )
      mag += step>>i;
  return mag;
}

template<int BPS>
static INLINE i32 ima_apply(i32 pcm, int code, int step) {
  int mag = code_magnitude<BPS>(code, step);
  pcm += (code&(1<<(BPS-1))) ? -mag : mag;
  return pcm<-32768 ? -32768 : pcm>32767 ? 32767 : pcm;
}

// the standard IMA quantizer: greedy from the coarsest weight down.
// leftover says how far into its cell the value sat.
template<int BPS>
static INLINE int ima_quantize(i32 diff, int step, int &leftover) {
  const int nb = BPS-1;
  int code = 0;
  i32 a = diff<0 ? -diff : diff;
  for( int i = 0; i<nb; i++ ) {
    i32 w = step>>i;
    if( a>=w ) {
      code |= 1<<(nb-1-i);
      a -= w;
    }
  }
  leftover = int(a>0x7FFF ? 0x7FFF : a);
  return code|(diff<0 ? 1<<nb : 0);
}

// ------------------------------------------------------------------- MS ADPCM arithmetic
// mirrors libavcodec/adpcm.c (adpcm_ms_expand_nibble and its encoder twin);
// drives prediction only, so a mismatch would cost ratio, never correctness

static const int MS_MAXCOEF = 32;
static const i32 MS_DELTA_MAX = 2796202; // INT_MAX/768, ffmpeg's ceiling

static const short ms_adapt[16] = {230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230};

static const short ms_std_coef[7][2] = {{256, 0}, {512, -256}, {0, 0}, {192, 64}, {240, 0}, {460, -208}, {392, -232}};

static INLINE i32 clip16(i64 v) {
  return i32(v<-32768 ? -32768 : v>32767 ? 32767 : v);
}

// the block's 2-tap prediction.  The coefficients are the 256-scaled pair the
// fmt chunk carries and the division truncates toward zero, which is what
// ffmpeg's (s1*c1/4 + s2*c2/4)/64 comes to for any table with multiple-of-4
// entries -- the standard one included.
static INLINE i32 ms_predict(i32 s1, i32 s2, i32 c1, i32 c2) {
  return i32((i64(s1)*i64(c1)+i64(s2)*i64(c2))/256);
}

static INLINE i32 ms_apply(i32 pred, int nib, i32 delta) {
  return clip16(i64(pred)+i64((nib&8) ? nib-16 : nib)*i64(delta));
}

static INLINE i32 ms_delta_update(i32 delta, int nib) {
  i32 d = i32((i64(ms_adapt[nib])*i64(delta))>>8);
  return d<16 ? 16 : d>MS_DELTA_MAX ? MS_DELTA_MAX : d;
}

// the MS quantizer: round half away from zero into a 4-bit signed cell.  err
// is the signed leftover, i.e. where inside its cell the value sat -- MS
// rounds to nearest, so the sign of it says which neighbour is the near miss.
static INLINE int ms_quantize(i64 diff, i32 delta, i64 &err) {
  i32 d = delta<1 ? 1 : delta;
  i64 q = (diff+(diff>=0 ? d/2 : -(d/2)))/d;
  q = q<-8 ? -8 : q>7 ? 7 : q;
  err = diff-q*i64(d);
  return int(q&15);
}

/* iDelta has no small index the way the IMA step has, so it is read in the log
   domain -- the octave plus two bits below the leading one, four steps to the
   octave over the whole legal range -- and C0's threshold map decides how many
   of those steps actually become separate contexts.  The map is what the
   optimizer moves: it can widen the buckets where iDelta does not discriminate
   and keep them fine where it does, which no single shift can express. */
/* Resolution of the log reading, NOT a free parameter: MS_DLOG_MAX+1 is the
   number of input positions C0's Md pattern has to cover, so changing either
   of these means relengthening that pattern to match. */
static const int MS_DLOG_SUB = 2;                                                                     // bits kept below the leading one
static const int MS_DLOG_E0 = 4;                                                                      // iDelta floors at 16, so log2 >= 4
static const int MS_DLOG_EMAX = 21;                                                                   // iDelta <= MS_DELTA_MAX
static const int MS_DLOG_MAX = ((MS_DLOG_EMAX-MS_DLOG_E0)<<MS_DLOG_SUB)+(1<<MS_DLOG_SUB)-1;           // 71

static INLINE int ms_dlog(i32 delta) {
  u32 d = delta<16 ? 16u : u32(delta);
  int e = 0;
  for( u32 x = d; x >>= 1; )
    e++;
  int b = ((e-MS_DLOG_E0)<<MS_DLOG_SUB)+int((d>>(e-MS_DLOG_SUB))&((1<<MS_DLOG_SUB)-1));
  return b<0 ? 0 : b>MS_DLOG_MAX ? MS_DLOG_MAX : b;
}

static INLINE int ms_bucket(i32 delta) {
  return MakeMd(ms_dlog(delta));
}

/* Confidence: where inside its cell the prediction sat.  MS rounds to nearest,
   so the signed position carries a direction the IMA leftover cannot -- a
   prediction just below the upper edge misses upward.  MSCONF 1 keeps only the
   distance, for comparison. */

static INLINE int ms_conf(i64 e, i32 delta) {
  i32 d = delta<1 ? 1 : delta;
  int c;
  if( !M0_MSCONF )
    c = int(((e+i64(d/2))*NCONF)/(i64(d)+1));
  else {
    i64 a = e<0 ? -e : e;
    c = int((a*2*NCONF)/(i64(d)+1));
  }
  return c<0 ? 0 : c>NCONF-1 ? NCONF-1 : c;
}

/* Bit-tree order for the nibble.  The MS code is a two's-complement quantized
   residual, so its natural bit order splits on the sign and then runs the
   negative half backwards.  MSMAP picks what the tree asks first:
     0  raw two's complement
     1  sign, then magnitude  (the IMA layout)
     2  magnitude, then sign  (zigzag) */
static u8 MS_M[16], MS_U[16];

static void init_ms_map() {
  int m = pclamp(M0_MSMAP, 0, 2);
  for( int i = 0; i<16; i++ ) {
    int v = i<8 ? i : i-16, s;
    if( m==0 )
      s = i; // raw two's complement
    else if( m==1 )
      s = v>=0 ? v : 8+(-v-1);       // sign, then magnitude
    else
      s = v>=0 ? 2*v : -2*v-1;         // magnitude, then sign (zigzag)
    MS_M[i] = u8(s);
    MS_U[s] = u8(i);
  }
}

// ------------------------------------------------------------------- signal predictor

#define WSH pclamp(G0_WSH, 4, 24)                    // weight fixed-point scale
static const int HB = 10, HN = 1<<HB, HM = HN-1;     // history ring, NOT tunable
static const int PRED_NMAX = 512;                    // widest stage the lengths may ask for, NOT tunable
static const int PRED_DMAX = pclamp(G0_PRED_DMAX, 1, 1<<20);
static const int PRED_SHMAX = pclamp(G0_PRED_SHMAX, 8, 62);
static_assert(PRED_NMAX<=HN, "a stage cannot be longer than the history ring");

/* Stage lengths are IDX Numbers rather than array bounds, so the storage is
   sized for the widest set the optimizer may ask for and the loops run to the
   configured length.  In the Const build those are integer constant
   expressions, so the loops bound exactly as they did when they were #defines;
   only the Debug build pays for the indirection. */
struct Pred {
  i32 w1[PRED_NMAX], w2[PRED_NMAX], w3[PRED_NMAX], w4[PRED_NMAX];
  i32 e0[HN], e1[HN], e2[HN], ex[HN];
  u64 en0, en1, en2, enx;
  u32 t;
  i32 last, p1, p2, p3, p4, d0last;
  int n1, n2, n3, n4, xset;
  int mu1, mu2, mu3, mu4, fl1, fl2, fl3, fl4, pw0;

  void init(int var) {
    setvar(var);
    reset(0);
  }

  /* Two cascades run side by side over the same signal, with independently
     tuned shapes, and the mixer decides per file which to believe.  Separate
     optimizer runs kept landing on genuinely different optima -- one long
     (48/192/192 taps) and one short (14/113/10 with a wider cross stage) --
     each clearly better on some material and clearly worse on other.  That is
     not a tuning ambiguity to be resolved by picking a winner: it says the two
     shapes see different things, so the model should have both. */
  void setvar(int var) {
    n1 = pclamp(var ? G0_N1B : G0_N1, 1, PRED_NMAX);
    n2 = pclamp(var ? G0_N2B : G0_N2, 1, PRED_NMAX);
    n3 = pclamp(var ? G0_N3B : G0_N3, 1, PRED_NMAX);
    n4 = pclamp(var ? G0_N4B : G0_N4, 0, PRED_NMAX);
    mu1 = var ? G0_MU1B : G0_MU1;
    mu2 = var ? G0_MU2B : G0_MU2;
    mu3 = var ? G0_MU3B : G0_MU3;
    mu4 = var ? G0_MU4B : G0_MU4;
    fl1 = pclamp(var ? G0_EFL1B : G0_EFL1, 1, 1<<20);
    fl2 = pclamp(var ? G0_EFL2B : G0_EFL2, 1, 1<<20);
    fl3 = pclamp(var ? G0_EFL3B : G0_EFL3, 1, 1<<20);
    fl4 = pclamp(var ? G0_EFL4B : G0_EFL4, 1, 1<<20);
    pw0 = var ? G0_PW0B : G0_PW0;
  }

  void reset(int keepw) {
    if( !keepw ) {
      memset(w1, 0, sizeof(w1));
      memset(w2, 0, sizeof(w2));
      memset(w3, 0, sizeof(w3));
      memset(w4, 0, sizeof(w4)); // no prior on the other channel: start at silence
      w1[0] = pw0;               // seeded at r[t-1] + (r[t-1]-r[t-2])
    }
    memset(ex, 0, sizeof(ex));
    memset(e0, 0, sizeof(e0));
    memset(e1, 0, sizeof(e1));
    memset(e2, 0, sizeof(e2));
    en0 = en1 = en2 = enx = 0;
    t = 0;
    xset = 0;
    last = p1 = p2 = p3 = p4 = d0last = 0;
  }

  INLINE i32 dot(const i32* w, const i32* h, int n) const {
    i64 s = 0;
    for( int i = 0; i<n; i++ )
      s += i64(w[i])*i64(h[(t-1-u32(i))&HM]);
    return i32(s>>WSH);
  }

  /* What this channel's own three stages predict for its NEXT delta, without
     touching any state.  It is what the channel coded FIRST at an instant has
     to offer the others: its real delta is not available to them yet, but this
     is, and both sides can compute it identically. */
  INLINE i32 predict_own(int depth) const {
    if( depth<1 )
      return 0;
    i32 v = dot(w1, e0, n1);
    if( depth>1 )
      v += dot(w2, e1, n2);
    if( depth>2 )
      v += dot(w3, e2, n3);
    return v;
  }

  INLINE i32 dotx(const i32* w, const i32* h, int n) const {
    i64 s = 0;
    for( int i = 0; i<n; i++ )
      s += i64(w[i])*i64(h[(t-u32(i))&HM]);
    return i32(s>>WSH);
  }

  INLINE void adaptx(i32* w, const i32* h, int n, i32 err, u64 en, int g, int fl) {
    if( !err )
      return;
    int sh = 0;
    for( u64 e = en+u64(n)*u64(fl); e>1; e >>= 1 )
      sh++;
    sh += g-WSH;
    sh = sh<0 ? 0 : sh>PRED_SHMAX ? PRED_SHMAX : sh;
    for( int i = 0; i<n; i++ )
      w[i] += i32((i64(err)*i64(h[(t-u32(i))&HM]))>>sh);
  }

  /* The other channel's most recent delta, filed at this channel's own time
     index.  Which sample that is falls out of the coding order for free: the
     channel coded second at an instant sees the first one's delta from the
     SAME instant, the one coded first sees the previous instant's -- exactly
     what causality allows, with no special case.  In MS stereo that same-instant
     value is what a byte-aligned general-purpose coder gets for nothing, since
     a byte there is one nibble from each channel. */
  INLINE void set_cross(i32 v) {
    if( !n4||xset )
      return;
    i32 old = ex[(t-u32(n4))&HM];
    enx += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    ex[t&HM] = v;
    xset = 1;
  }

  // NLMS: step normalised by the window energy, so adaptation does not depend
  // on how loud the passage is.  Power-of-two normalisation keeps it integer.
  INLINE void adapt(i32* w, const i32* h, int n, i32 err, u64 en, int g, int fl) {
    if( !err )
      return;
    int s = 0;
    for( u64 e = en+u64(n)*u64(fl); e>1; e >>= 1 )
      s++;
    int sh = s+g-WSH;
    if( sh<0 )
      sh = 0;
    else if( sh>PRED_SHMAX )
      sh = PRED_SHMAX;
    for( int i = 0; i<n; i++ ) {
      i32 x = h[(t-1-u32(i))&HM];
      w[i] += i32((i64(err)*i64(x))>>sh);
    }
  }

  INLINE void push(i32* h, u64 &en, int n, i32 v) {
    i32 old = h[(t-u32(n))&HM];
    en += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    h[t&HM] = v;
  }

  // predicted next-sample delta (r^[t] - r[t-1])
  INLINE i32 predict() {
    set_cross(0); // header samples have no cross value; fill the slot
    p1 = dot(w1, e0, n1);
    p2 = dot(w2, e1, n2);
    p3 = dot(w3, e2, n3);
    p4 = n4 ? dotx(w4, ex, n4) : 0;
    i32 d = p1+p2+p3+p4;
    return d<-PRED_DMAX ? -PRED_DMAX : d>PRED_DMAX ? PRED_DMAX : d;
  }

  INLINE i32 predicted_sample() {
    i32 v = last+predict();
    return v<-32768 ? -32768 : v>32767 ? 32767 : v;
  }

  // r = the sample that actually turned up
  INLINE void update(i32 r) {
    set_cross(0);
    i32 d0 = r-last, r1 = d0-p1, r2 = r1-p2, r3 = r2-p3, r4 = r3-p4;
    adapt(w1, e0, n1, r1, en0, mu1, fl1);
    adapt(w2, e1, n2, r2, en1, mu2, fl2);
    adapt(w3, e2, n3, r3, en2, mu3, fl3);
    if( n4 )
      adaptx(w4, ex, n4, r4, enx, mu4, fl4);
    push(e0, en0, n1, d0);
    push(e1, en1, n2, r1);
    push(e2, en2, n3, r2);
    d0last = d0;
    t++;
    xset = 0;
    last = r;
  }
};

// ------------------------------------------------------------------- wav parsing

static const int MAX_CHANS = 8;
static const int CANON_HEAD = 60; // riff 12 + fmt 8+20 + fact 8+4 + data 8

// the MS fmt chunk is 18 + wSamplesPerBlock + wNumCoef + the coefficient pairs
static INLINE int canon_head_ms(int ncoef) {
  return 62+4*ncoef;     // 90 for the usual seven
}

struct Wav {
  const u8* fmt = nullptr, * fact = nullptr;
  u32 fmt_size = 0, fact_size = 0;
  size_t data_off = 0, data_size = 0;
};

static bool parse_wav(const u8* f, size_t n, Wav &w) {
  if( n<12||memcmp(f, "RIFF", 4)||memcmp(f+8, "WAVE", 4) )
    return false;
  for( size_t pos = 12; pos+8<=n; ) {
    const u8* id = f+pos;
    u32 size = rd32(f+pos+4);
    size_t body = pos+8;
    u32 avail = u32(body+size<=n ? size : n-body);
    if( !memcmp(id, "fmt ", 4) ) {
      w.fmt = f+body;
      w.fmt_size = avail;
    } else if( !memcmp(id, "fact", 4) ) {
      w.fact = f+body;
      w.fact_size = avail;
    } else if( !memcmp(id, "data", 4) ) {
      w.data_off = body;
      w.data_size = avail;
      return true;
    }
    pos = body+size+(size&1);
  }
  return false;
}

// the header ADPCM-XQ writes; when it matches, it costs a flag instead of 60 bytes
static void build_canon_head(u8* h, int chans, int bps, u32 rate, int ba, int spb, u32 nsamples, u32 dsize) {
  memcpy(h, "RIFF", 4);
  wr32(h+4, 52+dsize);
  memcpy(h+8, "WAVE", 4);
  memcpy(h+12, "fmt ", 4);
  wr32(h+16, 20);
  wr16(h+20, 0x11);
  wr16(h+22, u32(chans));
  wr32(h+24, rate);
  wr32(h+28, rate*u32(ba)/u32(spb));
  wr16(h+32, u32(ba));
  wr16(h+34, u32(bps));
  wr16(h+36, 2);
  wr16(h+38, u32(spb));
  memcpy(h+40, "fact", 4);
  wr32(h+44, 4);
  wr32(h+48, nsamples);
  memcpy(h+52, "data", 4);
  wr32(h+56, dsize);
}

// the same for MS: fixed layout, only the coefficient table varies in length
static void build_canon_head_ms(u8* h, int chans, u32 rate, int ba, int spb, u32 nsamples, u32 dsize, const i32 (*coef)[2], int ncoef) {
  u32 fmtsz = 22+4*u32(ncoef);
  int at = 40;
  memcpy(h, "RIFF", 4);
  wr32(h+4, u32(canon_head_ms(ncoef)-8)+dsize);
  memcpy(h+8, "WAVE", 4);
  memcpy(h+12, "fmt ", 4);
  wr32(h+16, fmtsz);
  wr16(h+20, 0x02);
  wr16(h+22, u32(chans));
  wr32(h+24, rate);
  wr32(h+28, spb>0 ? rate*u32(ba)/u32(spb) : 0);
  wr16(h+32, u32(ba));
  wr16(h+34, 4);
  wr16(h+36, fmtsz-18);     // cbSize
  wr16(h+38, u32(spb));
  wr16(h+at, u32(ncoef));
  at += 2;
  for( int i = 0; i<ncoef; i++, at += 4 ) {
    wr16(h+at, u32(coef[i][0])&0xFFFF);
    wr16(h+at+2, u32(coef[i][1])&0xFFFF);
  }
  memcpy(h+at, "fact", 4);
  wr32(h+at+4, 4);
  wr32(h+at+8, nsamples);
  memcpy(h+at+12, "data", 4);
  wr32(h+at+16, dsize);
}

// ------------------------------------------------------------------- codec

enum { FMT_IMA = 0, FMT_MS = 1 };

/* How much a wav inherits from the one before it.
   OFF is the default because carrying statistics is only a win when the next
   file resembles the last: an adaptive model primed on unrelated material is
   worse than one primed on nothing, and there is no way to tell from inside
   which case a given archive is.  SAME is the safe middle -- share only along a
   run of wavs of the same codec and code width, where a context row means the
   same thing on both sides of the boundary.  ALL shares regardless and is for
   when the caller knows the material is homogeneous. */
enum { SOLID_OFF = 0, SOLID_SAME = 1, SOLID_ALL = 2 };

static bool same_type(const struct Params& a, const struct Params& b);

struct Params {
  int format = FMT_IMA;
  int chans = 1, bps = 4, block_align = 0, canonical = 0;
  int ncoef = 0, spb = 0; // MS only
  i32 coef[MS_MAXCOEF][2] = {};
  u32 rate = 0, crc = 0;
  i64 factdiff = 0; // nblk*spb - the fact chunk's count
  size_t data_size = 0, head_size = 0, tail_size = 0, orig_size = 0;

  int hdr_bytes() const {
    return format==FMT_MS ? 7*chans : 4*chans;
  }
};

static INLINE int16_t i16_of(const u8* p) {
  return int16_t(u16(p[0])|u16(u16(p[1])<<8));
}

static INLINE u32 get_bits(const u8* b, size_t bitpos, int nbits) {
  u32 v = u32(b[bitpos>>3])>>(bitpos&7);
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    v |= u32(b[i++])<<have;
    have += 8;
  }
  return v&((1u<<nbits)-1);
}

static INLINE void put_bits(u8* b, size_t bitpos, int nbits, u32 v) {
  b[bitpos>>3] |= u8(v<<(bitpos&7));
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    b[i++] |= u8(v>>have);
    have += 8;
  }
}

/* Match model.
   The context models above all look a handful of codes back.  What they cannot
   see is that a passage occurred BEFORE -- a looped sample, a repeated sound
   effect, a long silence, the same asset appearing twice in a solid stream --
   and that is exactly the redundancy a general-purpose compressor finds with
   LZ and this codec was blind to.

   It works on the coded symbols in coding order (channels interleaved, blocks
   concatenated), because that is the sequence that actually repeats: two
   stretches of identical audio produce identical codes only when the encoder
   state matches too, and ADPCM resynchronises that state at every block
   header, so real repeats show up as exact runs here.

   Both sides build the history from symbols they already have, so nothing
   about the match is transmitted.  In a solid stream the history deliberately
   survives a wav boundary -- a sound effect shared by two files is the case
   this is for -- while the match length resets, since the new file's opening
   does not continue the old file's ending. */
struct MatchModel {
  vector<u8> hist; // coded symbols, in coding order
  vector<u32> tab; // hash of the last `order` symbols -> position
  u32 hmask = 0, tmask = 0;
  u32 pos = 0;                // where the next symbol lands
  u32 ptr = 0;                // history position the prediction reads
  u32 len = 0;                // symbols matched so far, 0 = no match
  u32 h = 0, bpow = 1;        // rolling window hash, and B^(order-1)
  u32 floor_ = 0;             // matches before here are out of bounds
  int order = 0, expect = -1; // set by init() from the idx values

  static const u32 B = 0x9E3779B1u;

  void init(int ord, int histbits, int tabbits) {
    order = pclamp(ord, 2, 64);
    hist.assign(size_t(1)<<pclamp(histbits, 8, 28), 0);
    tab.assign(size_t(1)<<pclamp(tabbits, 8, 26), 0);
    hmask = u32(hist.size()-1);
    tmask = u32(tab.size()-1);
    bpow = 1;
    for( int i = 1; i<order; i++ )
      bpow *= B;
    pos = ptr = len = h = 0;
    floor_ = 0;
    expect = -1;
  }

  /* Forget everything without touching the tables.  Clearing a 16 MB hash per
     wav would dominate the run on a directory of short files, and it is not
     needed: raising the floor makes every entry from before this point fail the
     bounds test, so old positions can never be matched into. */
  void barrier() {
    floor_ = pos;
    len = 0;
    expect = -1;
  }

  void newseg() { // keep what was learned, drop the position
    len = 0;
    expect = -1;
  }

  /* An exact window hash: the symbol leaving the window is subtracted before
     the new one shifts in, so the hash names the last `order` symbols and
     nothing older -- unlike a decayed multiplicative hash, whose effective
     order drifts with the multiplier. */
  INLINE void push(int sym) {
    u32 old = pos>=u32(order) ? u32(hist[(pos-u32(order))&hmask])+1 : 0;
    hist[pos&hmask] = u8(sym);
    h = (h-old*bpow)*B+u32(sym)+1;
    pos++;
  }

  INLINE u32 slot() const {
    return (h*0x2545F491u)&tmask;
  }

  // after the real symbol is known: extend the run, or look for a new one
  INLINE void update(int sym) {
    if( len&&int(hist[ptr&hmask])==sym ) {
      ptr++;
      if( len<0xFFFF )
        len++;
    } else
      len = 0;
    push(sym);
    u32 sl = slot();
    if( !len&&pos>u32(order) ) {
      u32 cand = tab[sl];
      if( cand>floor_&&pos-cand<=hmask ) {
        ptr = cand;
        len = u32(order); // the hash asserts this much; prediction proves it
      }
    }
    tab[sl] = pos;
    expect = len ? int(hist[ptr&hmask]) : -1;
  }
};

/* Codec is templated on the DIRECTION only, deliberately not on the format.
   The obvious extra parameter would be FMT, and it is wrong here: -ss
   (SOLID_ALL) carries counters, mixer weights, APM tables and the match history
   across a wav boundary whatever the codec on either side, and two
   Codec<MD,FMT> objects cannot share one set of tables -- a stream holding an
   MS wav and an IMA wav would silently lose the carry-over that mode exists
   for.  So FMT parameterises the payload WALK below (code_data_ima<BPS,XST>,
   code_data_ms<XST>, code_symbol<FMT,BPS>), where everything it decides still
   folds, while the tables stay in one object.

   `rc` is a reference to the frontend, which IS the range coder (Xad<MD>
   derives from RC<MD>), so every rc.bit() call site reads as it always did. */
template<int MD>
struct Codec {
  enum { enc = (MD==0) };
  RC<MD> &rc;
  explicit Codec(RC<MD> &r) : rc(r) {}
  Params pm;
  /* Two of them, because "the last k codes occurred before" means two different
     things at two scales.  A short window is really a high-order context model:
     it fires constantly, is right rather more often than chance, and gives the
     mixer a view further back than any of the fixed contexts reach.  A long
     window fires rarely and is nearly certain when it does -- that is the one
     that catches a looped sample or a repeated asset.  One model cannot be both:
     the sweep below the short one bottoms out around five codes and the long one
     needs twelve or more, and each setting is clearly worse at the other's job. */
  MatchModel mmS, mmL;
  int nsym = 0, nbit = 0; // set by init()/begin_seg()
  const int hj_bits = pclamp(G0_HJ_BITS, 8, 24);
  const int sgn_maxnb = pclamp(G0_SGN_MAXNB, 8, 62);
  int nib_rows = 89; // IMA step index buckets, or the MS iDelta ones
  int sgn_ncls = 20; // magnitude classes that get their own mantissa rows

  // --- code models (row width = alphabet, node index 1..alphabet-1)
  CtrTab tQ;       // (q^, conf)
  CtrSTab tQs;     // slow twin of the above
  CtrTab tI;       // step index bucket
  CtrTab tP;       // previous code, same channel
  CtrTab tP2;      // (prev, prev2) hashed
  CtrTab tX;       // other channel at the same instant
  CtrTab tQP;      // (q^, prev) hashed
  CtrTab tQI;      // (q^, index bucket)
  CtrTab tQX;      // (q^, other channel)
  CtrTab tIP;      // (index bucket, prev, conf)
  CtrTab tQ2;      // short-horizon predicted code (stages 1+2 only)
  CtrTab tQ3;      // shortest view (stage 1 only)
  CtrTab tMS, tML; // match: (length bucket, expected bit), row 0 = no prediction

  /* Joint contexts: own code history crossed with the other channel's.  Every
     context above this point is either purely own-channel or a bare look at the
     other one, and that is the structural difference from a byte-oriented
     general-purpose CM -- such a coder sees an (L,R) nibble pair as one symbol,
     so its order-N byte context is automatically a joint stereo history several
     instants deep.  Crossing the two here is what closes that gap; measured
     together they are worth 2.3% on real music. */
  CtrTab tPX;   // (own previous, other channel now) -- the joint pair
  CtrTab tXX;   // (other channel now, other channel before) -- its trajectory
  CtrTab tPPX;  // (own previous two, other channel now)
  CtrTab tPX2;  // (own previous, other channel now and before)
  CtrTab tP3;   // own previous three codes
  CtrTab tPPXX; // (own previous two, other channel now and before)
  CtrTab tP4;   // own previous four codes
  CtrTab tQPX;  // (q^, own previous, other channel now)

  /* Deeper than direct indexing can reach: own last three codes crossed with
     the other channel's last two would want 1.2M rows.  At that depth most
     cells are never visited, so it is hashed into a fixed table instead and a
     collision costs less than the context is worth. */
  CtrTab tHJ;

  /* The second cascade's own view, plus the agreement between the two.  When
     both shapes land on the same code the next symbol is far more predictable
     than either alone implies, and that is exactly what the single-cascade
     model could not express. */
  CtrTab tQB;  // (q^ from cascade B, its confidence)
  CtrTab tQAB; // (q^ from A, q^ from B)
  Mixer mxC;
  APM apA, apB;

  // --- block headers / literals
  CtrTab tHnb, tHmn, tRsv, tLit;

  struct Ch {
    Pred pd, pdB;
    i32 pcm = 0;
    int index = 0, prev = 0, prev2 = 0, prev3 = 0, prev4 = 0, cur = 0;
    // MS: the decoder state plus what the previous block header held and what
    // the previous block's last nibble left iDelta at
    i32 samp1 = 0, samp2 = 0, delta = 16, hdelta = 16, rdelta = 16, c1 = 256, c2 = 0;
    u64 errR = 0, errH = 0;
    int bprev = 0;
  } ch[MAX_CHANS];

  u64 stat[ST_NCAT] = {};
  int scat = 0;

  /* yield() copies stkptrH-stkptrL bytes into Coroutine::stk[STKPAD] with no
     bound check, and an overflow there is an intra-object overwrite ASan does
     not instrument -- the same shape as the MIX_MAXIN bug in README-dual.txt,
     and it would present as a wild jump rather than a clean crash.  So measure
     rather than assume: -v prints the high-water mark against STKPAD.  Sampled
     once per coded symbol, which is below every frame that matters. */
  u64 stk_hi = 0;

  INLINE void stk_probe() {
    if( !verbose )
      return;
    char here;
    u64 d = u64((const char*)rc.stkptrH-&here);
    if( d>stk_hi )
      stk_hi = d;
  }

  /* Allocation and per-segment reset are separate because a solid stream holds
     several wavs: the tables are sized ONCE from the widest geometry any
     segment needs and then keep their statistics across segment boundaries,
     while the signal-domain state -- ADPCM decoder, NLMS filters, channel
     history -- is reset at each boundary because the new wav's samples have
     nothing to do with the old one's.  That split is the whole feature.

     Sizing from the maximum rather than per-segment matters: contexts are
     indexed with the SEGMENT's nsym and nib_rows, which are <= the allocated
     ones, so a 4-bit wav after a 5-bit one still lands on the rows it would
     have hit alone.  When every segment shares one geometry the maxima equal
     it and the layout is exactly what a single-file run produces. */
  void init(const Params &p, int maxsym, int maxnib) {
    amaxsym = maxsym;
    amaxnib = maxnib;
    mmS.init(G0_MATCH_ORDER, G0_MATCH_HB, G0_MATCH_TB);
    mmL.init(G0_MATCH_ORDER2, G0_MATCH_HB, G0_MATCH_TB);
    reset_stats();
    memset(stat, 0, sizeof(stat));
    begin_seg(p, true);
  }

  /* Back to the state the model had before it saw anything.  Everything except
     the match model's storage is refilled -- a couple of megabytes, cheap enough
     to do per wav -- and the match model uses its barrier instead. */
  void reset_stats() {
    nsym = amaxsym;
    nib_rows = amaxnib;
    sgn_ncls = pclamp(G0_SGN_NCLS, 2, sgn_maxnb+1);
    tQ.init(nsym*NCONF, nsym, RS_CODE);
    tQs.init(nsym*NCONF, nsym);
    tI.init(nib_rows, nsym, RS_CODE);
    tP.init(nsym, nsym, RS_CODE);
    tP2.init(nsym*nsym, nsym, RS_CODE);
    tX.init(nsym+1, nsym, RS_CODE);
    tQP.init(nsym*nsym, nsym, RS_CODE);
    tQI.init(nsym*nib_rows, nsym, RS_CODE);
    tQX.init(nsym*(nsym+1), nsym, RS_CODE);
    tIP.init(nib_rows*nsym*NCONF, nsym, RS_CODE);
    tQ2.init(nsym*nsym*NCONF, nsym, RS_CODE);
    tQ3.init(nsym*nsym*nsym, nsym, RS_CODE);
    tMS.init(1+2*C0_Ml_Volume, nsym, RS_MATCH);
    tML.init(1+2*C0_Ml_Volume, nsym, RS_MATCH);
    tPX.init(nsym*(nsym+1), nsym, RS_CODE);
    tXX.init((nsym+1)*(nsym+1), nsym, RS_CODE);
    tPPX.init(nsym*nsym*(nsym+1), nsym, RS_CODE);
    tPX2.init(nsym*(nsym+1)*(nsym+1), nsym, RS_CODE);
    tP3.init(nsym*nsym*nsym, nsym, RS_CODE);
    tPPXX.init(nsym*nsym*(nsym+1)*(nsym+1), nsym, RS_CODE);
    tP4.init(nsym*nsym*nsym*nsym, nsym, RS_CODE);
    tQPX.init(nsym*nsym*(nsym+1), nsym, RS_CODE);
    tHJ.init(1<<hj_bits, nsym, RS_CODE);
    tQB.init(nsym*NCONF, nsym, RS_CODE);
    tQAB.init(nsym*nsym, nsym, RS_CODE);
    mmS.barrier();
    mmL.barrier();
    mxC.init(MIX_NIN, nsym*NCONF*(G0_MATCH_MIXSEL ? 2 : 1), G0_MLR);
    apA.init(nsym*nsym, G0_AP_N_A, G0_AP_R_A);
    apB.init(nib_rows*nsym, G0_AP_N_B, G0_AP_R_B);
    tHnb.init(SGN_NMODEL, SGN_NODES, RS_HDR);
    tHmn.init(SGN_NMODEL*sgn_ncls, SGN_NODES, RS_HDR);
    tRsv.init(1, 2, RS_LIT); // one node, two branches
    tLit.init(LIT_NSTREAM*LIT_W, LIT_W, RS_LIT);
  }

  /* Enter a new wav.  Everything the counters, mixer and APM have learned
     stays; everything that describes where the previous signal had got to
     goes.  G0_SEG_KEEPW keeps the NLMS weights across the boundary as a warm
     start -- the histories are cleared either way, since predicting the first
     samples of one file from the tail of another is worse than predicting
     from silence. */
  void begin_seg(const Params &p, bool fresh) {
    mmS.newseg();
    mmL.newseg();
    pm = p;
    nbit = p.bps;
    int segsym = 1<<p.bps;
    int segnib = p.format==FMT_MS ? M0_Md_Volume : I0_Ib_Volume;
    nsym = segsym;
    nib_rows = segnib;
    for( int k = 0; k<MAX_CHANS; k++ ) {
      ch[k].pd.setvar(0);
      ch[k].pdB.setvar(1);
      ch[k].pd.reset(!fresh&&G0_SEG_KEEPW);
      ch[k].pdB.reset(!fresh&&G0_SEG_KEEPW);
      ch[k].pcm = 0;
      ch[k].index = ch[k].prev = ch[k].prev2 = ch[k].prev3 = ch[k].prev4 = ch[k].cur = 0;
      ch[k].samp1 = ch[k].samp2 = 0;
      ch[k].delta = ch[k].hdelta = ch[k].rdelta = 16;
      ch[k].errR = ch[k].errH = 0;
      ch[k].bprev = 0;
      ch[k].c1 = pm.ncoef ? pm.coef[0][0] : 256;
      ch[k].c2 = pm.ncoef ? pm.coef[0][1] : 0;
    }
  }

  int amaxsym = 0, amaxnib = 0; // set by init()

  // one mixed bit
  INLINE int mbit(Ctr** cs, int ncs, CtrS* sl, int mctx, int actxA, int actxB, int b) {
    mxC.nx = 0;
    mxC.add(G0_BIAS);
    for( int i = 0; i<ncs; i++ )
      mxC.add(STT[cs[i]->pr()]);
    if( sl )
      mxC.add(STT[sl->pr()]);
    int p = mxC.mix(mctx);
    int pa = apA.pp(p, actxA);
    int a1 = pclamp(G0_AB1, 0, 4), a2 = pclamp(G0_AB2, 0, 4);
    int pf = (a1*pa+(4-a1)*p)>>2;
    pf = pf<P_MIN ? P_MIN : pf>P_MAX ? P_MAX : pf;
    int pb = apB.pp(pf, actxB);
    pf = (a2*pb+(4-a2)*pf)>>2;
    pf = pf<P_MIN ? P_MIN : pf>P_MAX ? P_MAX : pf;
    b = rc.bit(pf, b);
    stat[scat] += CLT[pf][b];
    for( int i = 0; i<ncs; i++ )
      cs[i]->upd(b, RS_CODE);
    if( sl )
      sl->upd(b);
    mxC.upd(b);
    apA.upd(b);
    apB.upd(b);
    return b;
  }

  /* One ADPCM code, MSB-first over its raw bits (sign, then magnitude).

     FMT and BPS are compile-time here.  BPS gives the tree's trip count and the
     alphabet width that scales twenty-odd row indices below; FMT selects the
     scale-context table's height, and nothing else -- which is why it is FMT
     and not the height itself that is the parameter: M0_Md_Volume is
     `1*M0_d0.Size` in the Debug build, i.e. load-time initialised from a
     mapping object, so it is not a constant expression there.

     The three enum/const declarations shadow the runtime members with exactly
     the values begin_seg() puts in them for this segment.  The ALLOCATION sites
     in reset_stats() keep using the runtime amaxsym / amaxnib, which are the
     maxima over every segment in a solid stream and are >= these. */
  template<int FMT, int BPS>
  int code_symbol(int k, int qhat, int conf, int ib, int xch, int q2, int q3, int sym, int xch2, int qhatB, int confB) {
    enum { nsym = 1<<BPS, nbit = BPS };
    const int nib_rows = FMT ? M0_Md_Volume : I0_Ib_Volume;
    Ch &c = ch[k];
    Ctr* rQ = tQ.row(qhat*NCONF+conf);
    CtrS* rQs = tQs.row(qhat*NCONF+conf);
    Ctr* rI = tI.row(ib);
    Ctr* rP = tP.row(c.prev);
    Ctr* rP2 = tP2.row(c.prev*nsym+c.prev2);
    Ctr* rX = tX.row(xch);
    Ctr* rQP = tQP.row(qhat*nsym+c.prev);
    Ctr* rQI = tQI.row(qhat*nib_rows+ib);
    Ctr* rQX = tQX.row(qhat*(nsym+1)+xch);
    Ctr* rQ2 = tQ2.row((qhat*nsym+q2)*NCONF+conf);
    Ctr* rQ3 = tQ3.row((qhat*nsym+q2)*nsym+q3);
    Ctr* rIP = tIP.row((ib*nsym+c.prev)*NCONF+conf);
    Ctr* rPX = tPX.row(c.prev*(nsym+1)+xch);
    Ctr* rXX = tXX.row(xch*(nsym+1)+xch2);
    Ctr* rPPX = tPPX.row((c.prev*nsym+c.prev2)*(nsym+1)+xch);
    Ctr* rPX2 = tPX2.row((c.prev*(nsym+1)+xch)*(nsym+1)+xch2);
    Ctr* rP3 = tP3.row((c.prev*nsym+c.prev2)*nsym+c.prev3);
    Ctr* rPPXX = tPPXX.row(((c.prev*nsym+c.prev2)*(nsym+1)+xch)*(nsym+1)+xch2);
    Ctr* rP4 = tP4.row(((c.prev*nsym+c.prev2)*nsym+c.prev3)*nsym+c.prev4);
    Ctr* rQPX = tQPX.row((qhat*nsym+c.prev)*(nsym+1)+xch);
    u32 hj = u32(((c.prev*nsym+c.prev2)*nsym+c.prev3)*(nsym+1)+xch)*u32(nsym+1)+u32(xch2);
    Ctr* rHJ = tHJ.row(int((hj*0x9E3779B1u)>>(32-hj_bits)));
    Ctr* rQB = tQB.row(qhatB*NCONF+confB);
    Ctr* rQAB = tQAB.row(qhat*nsym+qhatB);
    /* One row per (length bucket, expected bit) plus row 0 for "no useful
       prediction", which covers both no match at all and a match the bits
       already decoded have contradicted.  Keying on the expected BIT rather
       than feeding the expectation as a sign means the counter learns how far
       to trust a match of this length, separately for each node -- and when
       the match is wrong, row 0 keeps that damage out of the trusted rows. */
    int slb = MakeMl(int(mmS.len)), llb = MakeMl(int(mmL.len));
    int sex = mmS.expect, lex = mmL.expect;
    int sok = sex>=0, lok = lex>=0;
    int msel = G0_MATCH_MIXSEL ? lok : 0;
    stk_probe();
    scat = ST_CODE;
    int nd = 1;
    for( int j = nbit-1; j>=0; j-- ) {
      int srow = 0, lrow = 0, sbit = 0, lbit = 0;
      if( sok ) {
        sbit = (sex>>j)&1;
        srow = 1+2*slb+sbit;
      }
      if( lok ) {
        lbit = (lex>>j)&1;
        lrow = 1+2*llb+lbit;
      }
      Ctr* cs[MIX_NIN];
      int n = 0;
      cs[n++] = tMS.row(srow)+nd;
      cs[n++] = tML.row(lrow)+nd;
      cs[n++] = rQ+nd;
      cs[n++] = rI+nd;
      cs[n++] = rP+nd;
      cs[n++] = rP2+nd;
      cs[n++] = rX+nd;
      cs[n++] = rQP+nd;
      cs[n++] = rQI+nd;
      cs[n++] = rQX+nd;
      cs[n++] = rIP+nd;
      cs[n++] = rQ2+nd;
      cs[n++] = rQ3+nd;
      cs[n++] = rPX+nd;
      cs[n++] = rXX+nd;
      cs[n++] = rPPX+nd;
      cs[n++] = rPX2+nd;
      cs[n++] = rP3+nd;
      cs[n++] = rPPXX+nd;
      cs[n++] = rP4+nd;
      cs[n++] = rQPX+nd;
      cs[n++] = rHJ+nd;
      cs[n++] = rQB+nd;
      cs[n++] = rQAB+nd;
      int b = mbit(cs, n, rQs+nd, (nd*NCONF+conf)*(G0_MATCH_MIXSEL ? 2 : 1)+msel, qhat*nsym+nd, ib*nsym+nd, enc ? (sym>>j)&1 : -1);
      // a match the decoded bits have contradicted says nothing about the rest
      if( sok&&b!=sbit )
        sok = 0;
      if( lok&&b!=lbit )
        lok = 0;
      nd = nd+nd+b;
    }
    int out = nd-nsym;
    mmS.update(out);
    mmL.update(out);
    return out;
  }

  // signed value as magnitude class + mantissa (zigzag)
  i64 code_signed(int model, i64 v) {
    u64 u = enc ? ((u64(v)<<1)^u64(v>>63)) : 0;
    int nb = 0;
    if( enc )
      while( nb<sgn_maxnb&&(u>>nb) )
        nb++;
    scat = ST_HDR;
    Ctr* r = tHnb.row(model);
    int nd = 1;
    for( int j = SGN_CBITS-1; j>=0; j-- ) {
      Ctr* c = r+nd;
      int p = c->pr();
      int b = rc.bit(p, enc ? (nb>>j)&1 : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_HDR);
      nd = nd+nd+b;
    }
    nb = nd-SGN_NODES;
    if( nb>sgn_maxnb )
      nb = sgn_maxnb;
    if( !enc )
      u = nb ? (u64(1)<<(nb-1)) : 0;
    for( int j = nb-2; j>=0; j-- ) {   // mantissa below the leading one
      Ctr* c = tHmn.row(model*sgn_ncls+(nb<sgn_ncls ? nb : sgn_ncls-1))+(j<SGN_NODES-1 ? j+1 : SGN_NODES-1);
      int p = c->pr();
      int b = rc.bit(p, enc ? int((u>>j)&1) : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_HDR);
      if( !enc )
        u |= u64(u32(b))<<j;
    }
    return i64(u>>1)^-i64(u&1);
  }

  int code_byte(int stream, int prev, int v) {
    scat = ST_LIT;
    Ctr* r = tLit.row(stream*LIT_W+prev);
    int nd = 1;
    for( int j = 7; j>=0; j-- ) {
      Ctr* c = r+nd;
      int p = c->pr();
      int b = rc.bit(p, enc ? (v>>j)&1 : -1);
      stat[scat] += CLT[p][b];
      c->upd(b, RS_LIT);
      nd = nd+nd+b;
    }
    return nd-LIT_W;
  }

  void code_bytes(int stream, u8* d, size_t n) {
    int prev = 0;
    for( size_t i = 0; i<n; i++ ) {
      int v = code_byte(stream, prev, enc ? d[i] : 0);
      if( !enc )
        d[i] = u8(v);
      prev = v;
    }
  }

  // ---------------------------------------------------------------- payload

  /* One pass over the ADPCM data in block order.  Encoder and decoder walk the
     same geometry and run the same predictor, so nothing about the layout or
     the prediction has to be stored. */

  /* The one runtime dispatch: it runs once per segment, and everything the
     walk is going to be asked about the geometry is settled by it.  MS is
     4-bit by definition (analyze and get_seg both refuse anything else), so it
     needs only the stereo split; IMA gets one instance per code width as well.
     The bps default arm is 4 -- it is unreachable, since both entry points
     validate bps to [2,5]. */
  void code_data(u8* data, size_t n) {
    const int xst = pm.chans>1;
    if( pm.format==FMT_MS ) {
      if( xst )
        code_data_ms<1>(data, n);
      else
        code_data_ms<0>(data, n);
      return;
    }
    switch( pm.bps ) {
    case 2:
      xst ? code_data_ima<2, 1>(data, n) : code_data_ima<2, 0>(data, n);
      break;
    case 3:
      xst ? code_data_ima<3, 1>(data, n) : code_data_ima<3, 0>(data, n);
      break;
    case 5:
      xst ? code_data_ima<5, 1>(data, n) : code_data_ima<5, 0>(data, n);
      break;
    default:
      xst ? code_data_ima<4, 1>(data, n) : code_data_ima<4, 0>(data, n);
      break;
    }
  }

  template<int BPS, int XST>
  void code_data_ima(u8* data, size_t n) {
    enum { bps = BPS };
    const int nch = pm.chans, hdr = nch*4, ba = pm.block_align;
    vector<u8> cbuf[MAX_CHANS];
    vector<int> code[MAX_CHANS];
    for( int k = 0; k<nch; k++ ) {
      cbuf[k].assign(size_t(ba)+8, 0);
      code[k].assign(size_t(ba)*8/size_t(bps)+8, 0);
    }

    for( size_t pos = 0; pos<n; ) {
      size_t left = n-pos, len = left<size_t(ba) ? left : size_t(ba);
      u8* blk = data+pos;

      if( len<size_t(hdr) ) { // runt tail
        code_bytes(LIT_MISC, blk, len);
        break;
      }

      // ---- block header: first sample, step index, reserved byte
      for( int k = 0; k<nch; k++ ) {
        Ch &c = ch[k];
        i32 pred = c.pd.predicted_sample();
        i32 val = i32(code_signed(SM_SAMP, enc ? i64(i16_of(blk+k*4))-pred : 0)+pred);
        val = val<-32768 ? -32768 : val>32767 ? 32767 : val;
        int ix = int(code_signed(SM_INDEX, enc ? i64(blk[k*4+2])-c.index : 0)+c.index)&0xFF;
        int rsv = enc ? (blk[k*4+3]!=0) : 0;
        scat = ST_FLAG;
        Ctr* rf = tRsv.row(0)+1;
        int p = rf->pr();
        rsv = rc.bit(p, enc ? rsv : -1);
        stat[ST_FLAG] += CLT[p][rsv];
        rf->upd(rsv, RS_LIT);
        int rb = rsv ? code_byte(LIT_MISC, 0, enc ? blk[k*4+3] : 0) : 0;
        if( !enc ) {
          blk[k*4] = u8(val);
          blk[k*4+1] = u8(u32(val)>>8);
          blk[k*4+2] = u8(ix);
          blk[k*4+3] = u8(rb);
        }
        c.pcm = val;
        c.index = ix>88 ? 88 : ix;
        c.pd.update(val);
        c.pdB.update(val); // the block's own first sample
      }

      size_t body = len-size_t(hdr), groups = body/size_t(hdr), rem = body%size_t(hdr);
      size_t cbytes = groups*4, bits = cbytes*8, ncode = bits/size_t(bps), tailbits = bits%size_t(bps);

      if( enc )
        for( int k = 0; k<nch; k++ ) {
          for( size_t j = 0; j<cbytes; j++ )
            cbuf[k][j] = blk[size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3)];
          for( size_t i = 0; i<ncode; i++ )
            code[k][i] = int(get_bits(&cbuf[k][0], i*size_t(bps), bps));
        }

      // ---- codes, in time order so a channel can see the others at t
      for( size_t i = 0; i<ncode; i++ )
        for( int k = 0; k<nch; k++ ) {
          Ch &c = ch[k];
          int step = step_table[c.index];
          int leftover = 0;
          /* XST folds the whole cross-channel apparatus away for mono, where
             ch[k-1] is ch[k] itself and every one of these reads back zero. */
          i32 cross = 0, crossB = 0;
          if constexpr( XST ) {
            cross = (k||!G0_XPRED) ? ch[(k+nch-1)%nch].pd.d0last : ch[nch-1].pd.predict_own(G0_XPRED);
            crossB = ch[(k+nch-1)%nch].pdB.d0last;
          }
          c.pd.set_cross(cross);
          i32 dhat = c.pd.predict();
          int qhat = ima_quantize<BPS>(dhat, step, leftover);
          c.pdB.set_cross(crossB);
          int loB = 0;
          int qhatB = ima_quantize<BPS>(c.pdB.predict(), step, loB);
          int lo2 = 0;
          int q2 = ima_quantize<BPS>(c.pd.p1+c.pd.p2, step, lo2);
          int q3 = ima_quantize<BPS>(c.pd.p1, step, lo2);
          const int base = step>>(BPS-1);
          int conf = int(u32(leftover)*u32(NCONF)/u32(2*base+1));
          conf = conf>NCONF-1 ? NCONF-1 : conf;
          int confB = int(u32(loB)*u32(NCONF)/u32(2*base+1));
          confB = confB>NCONF-1 ? NCONF-1 : confB;
          int ib = MakeIb(c.index);
          Ch &o = ch[k ? 0 : nch-1];
          enum { NSYM = 1<<BPS };
          int xch = XST ? o.cur+1 : 0, xch2 = XST ? o.prev2+1 : 0;
          xch = xch>NSYM ? NSYM : xch;
          xch2 = xch2>NSYM ? NSYM : xch2;
          int sym = code_symbol<0, BPS>(k, qhat, conf, ib, xch, q2, q3, enc ? code[k][i] : 0, xch2, qhatB, confB);
          if( !enc )
            code[k][i] = sym;
          c.pcm = ima_apply<BPS>(c.pcm, sym, step);
          c.index = index_update<BPS>(c.index, sym);
          c.prev4 = c.prev3;
          c.prev3 = c.prev2;
          c.prev2 = c.prev;
          c.prev = sym;
          c.cur = sym;
          c.pd.update(c.pcm);
          c.pdB.update(c.pcm);
        }

      if( !enc )
        for( int k = 0; k<nch; k++ ) {
          memset(&cbuf[k][0], 0, cbytes);
          for( size_t i = 0; i<ncode; i++ )
            put_bits(&cbuf[k][0], i*size_t(bps), bps, u32(code[k][i]));
        }

      // ---- unused trailing bits of the last group, then any partial group
      if( tailbits )
        for( int k = 0; k<nch; k++ ) {
          int v = enc ? int(get_bits(&cbuf[k][0], ncode*size_t(bps), int(tailbits))) : 0;
          v = code_byte(LIT_MISC, (pclamp(G0_LIT_CTX_TAILBITS, 0, LIT_W-MAX_CHANS)+k)&(LIT_W-1), v);
          if( !enc )
            put_bits(&cbuf[k][0], ncode*size_t(bps), int(tailbits), u32(v));
        }

      if( !enc )
        for( int k = 0; k<nch; k++ )
          for( size_t j = 0; j<cbytes; j++ )
            blk[size_t(hdr)+(j&~size_t(3))*size_t(nch)+size_t(k*4)+(j&3)] = cbuf[k][j];

      if( rem )
        code_bytes(LIT_MISC, blk+size_t(hdr)+groups*size_t(hdr), rem);

      pos += len;
    }
  }

  /* MS blocks: 7 header bytes per channel laid out field by field (coefficient
     set, iDelta, iSamp1, iSamp2, each planar across the channels), then plain
     nibbles, high one first, interleaved by channel one sample at a time -- no
     group shuffle to undo.  iSamp2 precedes iSamp1 in time; both are coded as
     residuals against the signal predictor, which is then fed with them in
     that order so the body starts from the real state. */

  /* MS is 4-bit by definition, so XST is the only compile-time parameter this
     walk needs; BPS is spelled 4 at every call into the shared bit tree. */
  template<int XST>
  void code_data_ms(u8* data, size_t n) {
    const int nch = pm.chans;
    const size_t hdr = size_t(7*nch), ba = size_t(pm.block_align);
    vector<u8> nib(ba*2+8, 0);

    for( size_t pos = 0; pos<n; ) {
      size_t left = n-pos, len = left<ba ? left : ba;
      u8* blk = data+pos;

      if( len<hdr ) { // runt tail
        code_bytes(LIT_MISC, blk, len);
        break;
      }

      // ---- block header
      for( int k = 0; k<nch; k++ ) { // coefficient set, order-1
        Ch &c = ch[k];
        int bp = code_byte(LIT_BPRED, c.bprev, enc ? blk[k] : 0);
        if( !enc )
          blk[k] = u8(bp);
        c.bprev = bp;
        int ci = bp<pm.ncoef ? bp : 0;
        c.c1 = pm.coef[ci][0];
        c.c2 = pm.coef[ci][1];
      }
      /* iDelta.  Two guesses are available and which one wins depends on the
         encoder: ffmpeg simply carries its running state across the block
         boundary (clamped to 16), making that residual exactly zero, while
         encoders that re-fit iDelta per block track their own previous header
         more closely.  Both sides can score the two from decoded data alone,
         so pick per channel by decayed absolute error rather than by guessing
         at build time -- it costs a couple of adds and picks up either. */
      for( int k = 0; k<nch; k++ ) {
        Ch &c = ch[k];
        size_t o = size_t(nch)+size_t(2*k);
        i32 pr = c.rdelta<16 ? 16 : c.rdelta, ph = c.hdelta;
        int run = c.errR<=c.errH;
        i32 p = run ? pr : ph;
        i32 v = i32(code_signed(run ? SM_DELTA : SM_DELTAH, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        int dsh = pclamp(M0_MSDSH, 1, 20);
        c.errR += u64(pr>v ? pr-v : v-pr)-(c.errR>>dsh);
        c.errH += u64(ph>v ? ph-v : v-ph)-(c.errH>>dsh);
        c.hdelta = c.delta = v;
      }
      for( int k = 0; k<nch; k++ ) { // iSamp2 -- first in time
        Ch &c = ch[k];
        size_t o = size_t(5*nch)+size_t(2*k);
        i32 p = c.pd.predicted_sample();
        i32 v = clip16(code_signed(SM_SAMP, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        c.samp2 = v;
        c.pd.update(v);
        c.pdB.update(v);
      }
      for( int k = 0; k<nch; k++ ) { // iSamp1 -- second in time
        Ch &c = ch[k];
        size_t o = size_t(3*nch)+size_t(2*k);
        i32 p = c.pd.predicted_sample();
        i32 v = clip16(code_signed(SM_SAMP1, enc ? i64(i16_of(blk+o))-p : 0)+p);
        if( !enc )
          wr16(blk+o, u32(v)&0xFFFF);
        c.samp1 = v;
        c.pd.update(v);
        c.pdB.update(v);
      }

      // ---- nibbles
      size_t body = len-hdr, nn = body*2, ngrp = nn/size_t(nch);

      if( enc )
        for( size_t j = 0; j<body; j++ ) {
          nib[2*j] = blk[hdr+j]>>4;
          nib[2*j+1] = blk[hdr+j]&15;
        }

      for( size_t i = 0; i<ngrp; i++ )
        for( int k = 0; k<nch; k++ ) {
          Ch &c = ch[k];
          size_t at = i*size_t(nch)+size_t(k);
          i32 P = ms_predict(c.samp1, c.samp2, c.c1, c.c2);
          i32 dl = c.delta<1 ? 1 : c.delta;
          i64 e = 0, e2 = 0, e3 = 0;
          i32 cross = 0, crossB = 0;
          if constexpr( XST ) {
            cross = (k||!G0_XPRED) ? ch[(k+nch-1)%nch].pd.d0last : ch[nch-1].pd.predict_own(G0_XPRED);
            crossB = ch[(k+nch-1)%nch].pdB.d0last;
          }
          c.pd.set_cross(cross);
          i32 d = c.pd.predict();
          int qhat = ms_quantize(i64(clip16(i64(c.pd.last)+d))-P, dl, e);
          c.pdB.set_cross(crossB);
          i64 eB = 0;
          int qhatB = ms_quantize(i64(clip16(i64(c.pdB.last)+c.pdB.predict()))-P, dl, eB);
          int q2 = ms_quantize(i64(clip16(i64(c.pd.last)+c.pd.p1+c.pd.p2))-P, dl, e2);
          int q3 = ms_quantize(i64(clip16(i64(c.pd.last)+c.pd.p1))-P, dl, e3);
          int conf = ms_conf(e, dl);
          int confB = ms_conf(eB, dl);
          int ib = ms_bucket(dl);
          Ch &o = ch[k ? 0 : nch-1];
          enum { NSYM = 16 };
          int xch = XST ? o.cur+1 : 0, xch2 = XST ? o.prev2+1 : 0;
          xch = xch>NSYM ? NSYM : xch;
          xch2 = xch2>NSYM ? NSYM : xch2;
          int sym = code_symbol<1, 4>(k, MS_M[qhat], conf, ib, xch, MS_M[q2], MS_M[q3], enc ? int(MS_M[nib[at]]) : 0, xch2, MS_M[qhatB], confB);
          int nb = MS_U[sym];
          if( !enc )
            nib[at] = u8(nb);
          i32 s = ms_apply(P, nb, c.delta);
          c.samp2 = c.samp1;
          c.samp1 = s;
          c.delta = ms_delta_update(c.delta, nb);
          c.prev4 = c.prev3;
          c.prev3 = c.prev2;
          c.prev2 = c.prev;
          c.prev = sym;
          c.cur = sym;
          c.pd.update(s);
          c.pdB.update(s);
        }

      for( size_t at = ngrp*size_t(nch); at<nn; at++ ) {   // a nibble left over
        int v = code_byte(LIT_MISC, pclamp(G0_LIT_CTX_NIBBLE, 0, LIT_W-1), enc ? nib[at] : 0);
        if( !enc )
          nib[at] = u8(v&15);
      }

      if( !enc )
        for( size_t j = 0; j<body; j++ )
          blk[hdr+j] = u8((nib[2*j]<<4)|nib[2*j+1]);

      for( int k = 0; k<nch; k++ )
        ch[k].rdelta = ch[k].delta;

      pos += len;
    }
  }
};

// ------------------------------------------------------------------- top level

static const char MAGIC[4] = {'X', 'A', 'C', '1'};
static const u8 VERSION = 4;

// samples a full block of the given geometry decodes to
static int block_samples(const Params &pm) {
  if( pm.format==FMT_MS )
    return (pm.block_align-7*pm.chans)*2/pm.chans+2;
  return adpcm_block_size_to_sample_count(pm.block_align, pm.chans, pm.bps);
}

/* Analyze one wav living at base[0 .. extent).  `extent` is where the NEXT wav
   starts (or the end of the stream), so head+data+tail always covers it
   exactly and nothing can fall between two segments. */
static bool analyze(const u8* base, size_t extent, const char* name, Params &pm, Wav &w, bool loud) {
#define AFAIL(...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          if( loud )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
          fprintf(stderr, __VA_ARGS__);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
          return false;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
        } while( 0 )
  if( !parse_wav(base, extent, w) )
    AFAIL("error: '%s' is not a RIFF/WAVE file with a data chunk\n", name);
  if( !w.fmt||w.fmt_size<16 )
    AFAIL("error: '%s': missing or short fmt chunk\n", name);
  u32 tag = rd16(w.fmt);
  if( tag==0x11 )
    pm.format = FMT_IMA;
  else if( tag==0x02 )
    pm.format = FMT_MS;
  else
    AFAIL("error: '%s' is neither IMA-ADPCM (0x11) nor MS-ADPCM (0x02)\n", name);

  pm.chans = int(rd16(w.fmt+2));
  pm.rate = rd32(w.fmt+4);
  pm.block_align = int(rd16(w.fmt+12));
  pm.bps = int(rd16(w.fmt+14));
  pm.spb = int(w.fmt_size>=20 ? rd16(w.fmt+18) : 0);
  pm.data_size = w.data_size;
  pm.head_size = w.data_off;
  pm.tail_size = extent-(w.data_off+w.data_size);
  pm.orig_size = extent;

  if( pm.format==FMT_MS ) {
    if( pm.chans<1||pm.chans>2 )
      AFAIL("error: MS-ADPCM is mono or stereo only, got %d channels\n", pm.chans);
    if( pm.bps!=4 )
      AFAIL("error: unsupported MS-ADPCM code width %d (need 4 bits)\n", pm.bps);
    if( pm.block_align<7*pm.chans )
      AFAIL("error: block size %d smaller than the block header\n", pm.block_align);
    // the coefficient table; a file without a usable one still codes fine, the
    // standard set just drives the prediction instead
    pm.ncoef = int(w.fmt_size>=22 ? rd16(w.fmt+20) : 0);
    if( pm.ncoef<1||pm.ncoef>MS_MAXCOEF||w.fmt_size<22+4*u32(pm.ncoef) )
      pm.ncoef = 0;
    if( pm.ncoef ) {
      for( int i = 0; i<pm.ncoef; i++ ) {
        pm.coef[i][0] = int16_t(rd16(w.fmt+22+4*i));
        pm.coef[i][1] = int16_t(rd16(w.fmt+24+4*i));
      }
    } else {
      pm.ncoef = 7;
      for( int i = 0; i<7; i++ ) {
        pm.coef[i][0] = ms_std_coef[i][0];
        pm.coef[i][1] = ms_std_coef[i][1];
      }
    }
  } else {
    if( pm.chans<1||pm.chans>MAX_CHANS )
      AFAIL("error: unsupported channel count %d\n", pm.chans);
    if( pm.bps<2||pm.bps>5 )
      AFAIL("error: unsupported code width %d (need 2-5 bits)\n", pm.bps);
    if( pm.block_align<pm.chans*4 )
      AFAIL("error: block size %d smaller than the block header\n", pm.block_align);
  }
#undef AFAIL

  // can the RIFF header be regenerated instead of stored?
  int dspb = block_samples(pm);
  u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
  u32 fs = (w.fact&&w.fact_size>=4) ? rd32(w.fact) : 0;
  pm.canonical = 0;
  if( w.fact&&w.fact_size==4&&pm.spb>0&&pm.spb==dspb ) {
    u8 probe[64+4*MS_MAXCOEF];
    size_t cs;
    if( pm.format==FMT_MS ) {
      cs = size_t(canon_head_ms(pm.ncoef));
      build_canon_head_ms(probe, pm.chans, pm.rate, pm.block_align, pm.spb, fs, u32(pm.data_size), pm.coef, pm.ncoef);
    } else {
      cs = CANON_HEAD;
      build_canon_head(probe, pm.chans, pm.bps, pm.rate, pm.block_align, pm.spb, fs, u32(pm.data_size));
    }
    if( pm.head_size==cs&&!memcmp(probe, base, cs) ) {
      pm.canonical = 1;
      pm.factdiff = i64(nblk)*i64(dspb)-i64(fs);
    }
  }
  return true;
}

// ------------------------------------------------------------------- solid stream

struct Seg {
  Params pm;
  size_t off = 0; // where this wav's RIFF sits in the stream
};

struct Arc {
  int solid = SOLID_OFF;
  vector<Seg> segs;
  size_t lead = 0; // bytes ahead of the first wav
  size_t total = 0;
  u32 crc = 0;
  vector<string> names; // empty unless several inputs were given
  vector<u64> sizes;
};

/* Find every wav in the stream.  A candidate is a RIFF/WAVE header that also
   parses as IMA or MS ADPCM with a usable geometry, which is a strong enough
   filter that audio data almost never trips it -- and when it does, the cost is
   only ratio: the decoder never repeats this scan, it reads the segment table
   out of the container, and each segment's head+data+tail covers its extent
   exactly, so a wav split down the middle still restores byte for byte. */
/* "Same type" is codec AND code width, not codec alone.  A 4-bit and a 2-bit
   IMA file share a format tag but not an alphabet: their tree nodes and context
   rows index differently, so statistics carried between them would describe the
   wrong thing.  Channel count and block size are deliberately left out -- those
   change what the contexts contain, not what they mean. */
static bool same_type(const Params &a, const Params &b) {
  return a.format==b.format&&a.bps==b.bps;
}

static void scan_segments(const vector<u8> &buf, Arc &ar) {
  const u8* d = buf.data();
  size_t n = buf.size();
  vector<size_t> cand;
  for( size_t i = 0; i+12<=n; ) {
    const void* hit = memchr(d+i, 'R', n-11-i);
    if( !hit )
      break;
    i = size_t((const u8*)hit-d);
    if( !memcmp(d+i, "RIFF", 4)&&!memcmp(d+i+8, "WAVE", 4) ) {
      Params pm;
      Wav w;
      if( analyze(d+i, n-i, "", pm, w, false) ) {
        cand.push_back(i);
        // skip past this wav's data; a nested RIFF inside coded audio is noise
        i += w.data_off+w.data_size;
        continue;
      }
    }
    i++;
  }
  ar.total = n;
  ar.lead = cand.empty() ? n : cand[0];
  for( size_t k = 0; k<cand.size(); k++ ) {
    Seg sg;
    sg.off = cand[k];
    size_t extent = (k+1<cand.size() ? cand[k+1] : n)-cand[k];
    Wav w;
    if( !analyze(d+sg.off, extent, "", sg.pm, w, false) )
      continue; // cannot happen: the same call succeeded above
    ar.segs.push_back(sg);
  }
}

/* put_seg / get_seg moved into Xad<MD> below: they are now pull/push over the
   coroutine pins rather than index arithmetic over a fully-loaded buffer.  The
   field order and encoding are unchanged, so the header bytes are identical. */

static void write_canon(u8* dst, const Params &pm) {
  int spb = block_samples(pm);
  u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
  u32 fs = u32(i64(nblk)*i64(spb)-pm.factdiff);
  if( pm.format==FMT_MS )
    build_canon_head_ms(dst, pm.chans, pm.rate, pm.block_align, spb, fs, u32(pm.data_size), pm.coef, pm.ncoef);
  else
    build_canon_head(dst, pm.chans, pm.bps, pm.rate, pm.block_align, spb, fs, u32(pm.data_size));
}

// widest geometry any segment needs; the tables are allocated once from this
static void arc_geometry(const Arc &ar, int &maxsym, int &maxnib) {
  maxsym = 16;
  maxnib = 1;
  for( size_t i = 0; i<ar.segs.size(); i++ ) {
    const Params &p = ar.segs[i].pm;
    int ns = 1<<p.bps, nb = p.format==FMT_MS ? M0_Md_Volume : I0_Ib_Volume;
    if( ns>maxsym )
      maxsym = ns;
    if( nb>maxnib )
      maxnib = nb;
  }
  if( ar.segs.size()==1 )
    maxsym = 1<<ar.segs[0].pm.bps;   // a lone wav is sized exactly as it always was
}


// ------------------------------------------------------------------- frontend

/* Xad<MD> IS the coroutine: it derives from RC<MD>, which derives from
   Coroutine, so there is one object, one stack, and the range coder is a layer
   of the chain rather than a second instance.  pin[0] carries the input window
   and pin[1] the output window; the RC's rcio defaults to whichever of the two
   the direction wants, so the coded stream never touches a staging buffer.

   The base is dependent on MD, so unqualified lookup does not see it -- hence
   the using-declarations rather than `this->` on every use, as in ddsdet. */
template<int MD>
struct Xad : RC<MD> {
  typedef RC<MD> RCB;
  using RCB::pin;
  using RCB::f_quit;
  using RCB::chkinp;
  using RCB::addinp;
  using RCB::addout;
  using RCB::coro_init;
  using RCB::coro_call;
  using RCB::getoutsize;
  using RCB::stkptrH;
  using RCB::emitted;

  Codec<MD> cx;
  Xad() : cx(*this) {}

  Arc ar;
  /* Encoding, this is the container as it arrived; decoding, it is the stream
     being rebuilt.  Both directions need a whole image: the encoder because
     nseg, the segment geometry and the crc32 all have to be in the header
     before the first coded byte, and they come from a full scan; the decoder
     because it verifies the crc before writing anything.  What no longer needs
     a buffer -- and is the point of the port -- is the CODED side, which now
     moves a window at a time through pin[1] (resp. pin[0]). */
  vector<u8> src;
  vector<string> in_names; // encode: one per input file, when there is more than one
  vector<u64> in_sizes;
  string name;             // the file whose name goes in a diagnostic
  int solid = SOLID_OFF;
  int rc_err = 0;
  int hdr_eof = 0;
  u64 hdr_bytes = 0;       // container header, i.e. everything ahead of the rc stream

  // ------------------------------------------------------------- pin helpers

  /* Block write straight into the output window.  chkout() after each chunk
     keeps put()'s "ptr < end on entry" invariant, yielding r==2 to the driver
     exactly when the window fills. */
  void putn(const u8* p, size_t n) {
    for(;; ) {
      size_t room = size_t(pin[1].end-pin[1].ptr);
      size_t k = n<room ? n : room;
      if( k ) {
        memcpy(pin[1].ptr, p, k);
        pin[1].ptr += k;
        p += k;
        n -= k;
      }
      pin[1].chkout();
      if( !n )
        return;
    }
  }

  /* The whole container in, a window at a time.  f_quit is the driver's "no
     more input files"; chkinp() yields r==1 for the next window. */
  void slurp() {
    for(;; ) {
      coro3_pin &I = pin[0];
      if( I.ptr<I.end ) {
        src.insert(src.end(), I.ptr, I.end);
        I.ptr = I.end;
        continue;
      }
      if( f_quit )
        return;
      chkinp();
    }
  }

  void hput(u8 c) {
    pin[1].put(c);
    hdr_bytes++;
  }

  void hput_varint(u64 x) {
    while( x>=0x80 ) {
      hput(u8(u8(x&0x7F)|0x80));
      x >>= 7;
    }
    hput(u8(x));
  }

  int hget() {
    uint c = pin[0].get();
    if( c>255 ) {
      hdr_eof = 1;
      return -1;
    }
    return int(c);
  }

  u64 hget_varint() {
    u64 v = 0;
    int sh = 0;
    for(;; ) {
      int c = hget();
      if( c<0||sh>63 ) {
        hdr_eof = 1;
        return 0;
      }
      v |= u64(u32(c)&0x7F)<<sh;
      if( !(c&0x80) )
        return v;
      sh += 7;
    }
  }

  int fail(const char* what) {
    fprintf(stderr, "error: '%s': %s\n", name.c_str(), what);
    rc_err = 1;
    return 0;
  }

  // ------------------------------------------------------- segment table i/o

  void put_seg(const Params &pm) {
    hput(u8(pm.canonical));
    hput(u8(pm.format));
    hput(u8(pm.chans));
    hput(u8(pm.bps));
    hput_varint(u64(pm.block_align));
    hput_varint(u64(pm.data_size));
    hput_varint(u64(pm.tail_size));
    if( pm.canonical ) {
      hput_varint(pm.rate);
      hput_varint(zig(pm.factdiff));
    } else
      hput_varint(u64(pm.head_size));
    if( pm.format==FMT_MS ) {
      // the usual seven costs a zero byte; anything else is spelled out, since
      // both sides have to predict with the same table
      bool std = pm.ncoef==7;
      for( int i = 0; std&&i<7; i++ )
        std = pm.coef[i][0]==ms_std_coef[i][0]&&pm.coef[i][1]==ms_std_coef[i][1];
      hput(u8(std ? 0 : pm.ncoef));
      if( !std )
        for( int i = 0; i<pm.ncoef; i++ ) {
          hput_varint(zig(pm.coef[i][0]));
          hput_varint(zig(pm.coef[i][1]));
        }
    }
  }

  bool get_seg(Params &pm) {
    pm.canonical = hget();
    pm.format = hget();
    pm.chans = hget();
    pm.bps = hget();
    if( hdr_eof )
      return fail("truncated segment table"), false;
    pm.block_align = int(hget_varint());
    pm.data_size = size_t(hget_varint());
    pm.tail_size = size_t(hget_varint());
    if( pm.canonical ) {
      pm.rate = u32(hget_varint());
      pm.factdiff = unzig(hget_varint());
    } else
      pm.head_size = size_t(hget_varint());
    if( pm.format==FMT_MS ) {
      int nc = hget();
      if( hdr_eof )
        return fail("truncated segment table"), false;
      pm.ncoef = nc;
      if( !pm.ncoef ) {
        pm.ncoef = 7;
        for( int i = 0; i<7; i++ ) {
          pm.coef[i][0] = ms_std_coef[i][0];
          pm.coef[i][1] = ms_std_coef[i][1];
        }
      } else if( pm.ncoef>MS_MAXCOEF )
        return fail("bad coefficient count"), false;
      else
        for( int i = 0; i<pm.ncoef; i++ ) {
          pm.coef[i][0] = i32(unzig(hget_varint()));
          pm.coef[i][1] = i32(unzig(hget_varint()));
        }
    }
    if( hdr_eof )
      return fail("truncated segment table"), false;
    if( pm.format!=FMT_IMA&&pm.format!=FMT_MS )
      return fail("unknown payload format"), false;
    if( pm.format==FMT_MS ? (pm.chans<1||pm.chans>2||pm.bps!=4||pm.block_align<7*pm.chans) : (pm.chans<1||pm.chans>MAX_CHANS||pm.bps<2||pm.bps>5||pm.block_align<pm.chans*4) )
      return fail("bad parameters"), false;

    int spb = block_samples(pm);
    if( pm.canonical ) {
      if( spb<=0 )
        return fail("bad block size"), false;
      pm.spb = spb;
      pm.head_size = pm.format==FMT_MS ? size_t(canon_head_ms(pm.ncoef)) : size_t(CANON_HEAD);
    }
    pm.orig_size = pm.head_size+pm.data_size+pm.tail_size;
    return true;
  }

  // ------------------------------------------------------------------ encode

  void do_encode() {
    slurp();
    if( rc_err ) // the driver could not open one of the inputs
      return;
    if( src.empty() ) {
      fprintf(stderr, "error: nothing to compress\n");
      rc_err = 1;
      return;
    }
    ar.solid = solid;
    ar.names = in_names;
    ar.sizes = in_sizes;
    scan_segments(src, ar);
    if( ar.segs.empty() ) {
      // say why, using the first input's own diagnostics
      Params pm;
      Wav w;
      analyze(src.data(), src.size(), name.c_str(), pm, w, true);
      rc_err = 1;
      return;
    }
    ar.crc = crc32b(src.data(), src.size());

    for( int i = 0; i<4; i++ )
      hput(u8(MAGIC[i]));
    hput(VERSION);
    hput(u8(ar.solid));
    hput_varint(u64(ar.segs.size()));
    hput_varint(u64(ar.lead));
    for( size_t i = 0; i<ar.segs.size(); i++ )
      put_seg(ar.segs[i].pm);
    hput_varint(u64(ar.names.size())); // 0 = one unnamed stream
    for( size_t i = 0; i<ar.names.size(); i++ ) {
      hput_varint(u64(ar.names[i].size()));
      for( size_t j = 0; j<ar.names[i].size(); j++ )
        hput(u8(ar.names[i][j]));
      hput_varint(ar.sizes[i]);
    }
    u8 c4[4];
    wr32(c4, ar.crc);
    for( int i = 0; i<4; i++ )
      hput(c4[i]);

    u8* d = src.data();
    int maxsym, maxnib;
    arc_geometry(ar, maxsym, maxnib);
    Params p0;
    cx.init(ar.segs.empty() ? p0 : ar.segs[0].pm, maxsym, maxnib);
    this->init_enc();
    if( ar.lead )
      cx.code_bytes(LIT_GAP, d, ar.lead);
    for( size_t i = 0; i<ar.segs.size(); i++ ) {
      const Params &pm = ar.segs[i].pm;
      u8* q = d+ar.segs[i].off;
      bool keep = i&&(ar.solid==SOLID_ALL||(ar.solid==SOLID_SAME&&same_type(ar.segs[i-1].pm, pm)));
      if( i&&!keep )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( !pm.canonical )
        cx.code_bytes(LIT_HEAD, q, pm.head_size);
      cx.code_data(q+pm.head_size, pm.data_size);
      cx.code_bytes(LIT_TAIL, q+pm.head_size+pm.data_size, pm.tail_size);
    }
    this->flush();
  }

  // ------------------------------------------------------------------ decode

  void do_decode() {
    u8 m[4];
    for( int i = 0; i<4; i++ ) {
      int c = hget();
      if( c<0 ) {
        fprintf(stderr, "error: '%s' is not an xadpcm file\n", name.c_str());
        rc_err = 1;
        return;
      }
      m[i] = u8(c);
    }
    if( memcmp(m, MAGIC, 4) ) {
      fprintf(stderr, "error: '%s' is not an xadpcm file\n", name.c_str());
      rc_err = 1;
      return;
    }
    if( hget()!=VERSION )
      return (void)fail("made by a different xadpcm version");
    int sl = hget();
    if( sl<0||sl>SOLID_ALL )
      return (void)fail("unknown solid mode");
    ar.solid = sl;
    u64 nseg = hget_varint();
    ar.lead = size_t(hget_varint());
    if( hdr_eof||nseg>(1u<<24) )
      return (void)fail("truncated or implausible header");
    ar.segs.resize(size_t(nseg));
    for( size_t i = 0; i<ar.segs.size(); i++ )
      if( !get_seg(ar.segs[i].pm) )
        return;
    u64 nfile = hget_varint();
    if( hdr_eof||nfile>(1u<<20) )
      return (void)fail("truncated header");
    for( u64 i = 0; i<nfile; i++ ) {
      u64 ln = hget_varint();
      if( hdr_eof||ln>4096 )
        return (void)fail("bad file table");
      string s;
      s.resize(size_t(ln));
      for( u64 j = 0; j<ln; j++ ) {
        int c = hget();
        if( c<0 )
          return (void)fail("bad file table");
        s[size_t(j)] = char(c);
      }
      ar.names.push_back(s);
      ar.sizes.push_back(hget_varint());
    }
    u8 c4[4];
    for( int i = 0; i<4; i++ ) {
      int c = hget();
      if( c<0 )
        return (void)fail("truncated header");
      c4[i] = u8(c);
    }
    ar.crc = rd32(c4);

    size_t total = ar.lead;
    for( size_t i = 0; i<ar.segs.size(); i++ )
      total += ar.segs[i].pm.orig_size;
    ar.total = total;
    /* The file table has to fit inside what the segment table says the stream
       is.  Checked here, before a byte is decoded, rather than while writing
       members out -- the driver's split sink is downstream of this. */
    u64 named = 0;
    for( size_t i = 0; i<ar.sizes.size(); i++ )
      named += ar.sizes[i];
    if( named>u64(total) )
      return (void)fail("file table overruns the stream");
    src.assign(total, 0);

    int maxsym, maxnib;
    arc_geometry(ar, maxsym, maxnib);
    Params p0;
    cx.init(ar.segs.empty() ? p0 : ar.segs[0].pm, maxsym, maxnib);
    this->init_dec();

    u8* d = src.data();
    if( ar.lead )
      cx.code_bytes(LIT_GAP, d, ar.lead);
    size_t at = ar.lead;
    for( size_t i = 0; i<ar.segs.size(); i++ ) {
      Params &pm = ar.segs[i].pm;
      ar.segs[i].off = at;
      u8* q = d+at;
      bool keep = i&&(ar.solid==SOLID_ALL||(ar.solid==SOLID_SAME&&same_type(ar.segs[i-1].pm, pm)));
      if( i&&!keep )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( pm.canonical )
        write_canon(q, pm);
      else
        cx.code_bytes(LIT_HEAD, q, pm.head_size);
      cx.code_data(q+pm.head_size, pm.data_size);
      cx.code_bytes(LIT_TAIL, q+pm.head_size+pm.data_size, pm.tail_size);
      at += pm.orig_size;
    }

    if( crc32b(src.data(), src.size())!=ar.crc )
      return (void)fail("crc mismatch after decode");
    putn(src.data(), src.size());
  }

  void do_process() {
    if constexpr( MD==0 )
      do_encode();
    else
      do_decode();
    yield(this, 0);
  }
};

// ------------------------------------------------------------------- driver

/* The coroutine pump.  Lib3's CoroFileProc services r==1 (refill pin0 from one
   FILE*) and r==2 (flush pin1 to one FILE*); xadpcm needs three things it does
   not have:
     - several inputs behind pin0, because `xadpcm c a.wav b.wav out` codes the
       concatenation of them as one stream;
     - an output that may be a file, a memory buffer (-t keeps the coded stream
       in memory so it can be verified before anything is written), or a
       SEQUENCE of files split by the decoded file table;
     - not setting f_quit until the LAST input is exhausted, or a multi-file
       encode would stop after the first.
   XadDrive is the only part of the program that knows what a FILE* is. */
template<class Carrier>
struct XadDrive : Carrier {
  enum { bufsize = 1<<16 };
  ALIGN(4096) byte inpbuf[bufsize];
  ALIGN(4096) byte outbuf[bufsize];

  // input: either a list of paths, opened in turn, or one memory image
  char** in_paths = nullptr;
  int in_n = 0, in_i = 0;
  FILE* in_f = nullptr;
  const u8* in_mem = nullptr;
  size_t in_memn = 0, in_mempos = 0;

  // output: at most one of these is set
  const char* out_path = nullptr;
  string out_dir;    // out_path plus a trailing separator, for the split sink
  vector<u8>* out_mem = nullptr;
  int out_split = 0; // out_path names a directory when the file table is non-empty
  FILE* out_f = nullptr;
  size_t sp_i = 0;
  u64 sp_left = 0;

  /* Next window of input.  Advances through the path list, so a zero-length
     input cannot make the pump hand back 0 bytes forever: the loop only
     returns 0 once there is no path left.  An empty file still contributes its
     (name, 0) pair, which is what today's ar.sizes.push_back(one.size())
     records for it. */
  uint feed_files() {
    for(;; ) {
      if( !in_f ) {
        if( in_i>=in_n )
          return 0;
        const char* p = in_paths[in_i++];
        in_f = fopen(p, "rb");
        if( !in_f ) {
          fprintf(stderr, "error: can't read '%s'\n", p);
          this->rc_err = 1;
          in_i = in_n;
          return 0;
        }
        if( in_n>1 ) {
          this->in_names.push_back(safe_name(string(p)));
          this->in_sizes.push_back(0);
        }
      }
      uint l = uint(fread(inpbuf, 1, bufsize, in_f));
      if( l ) {
        if( in_n>1 )
          this->in_sizes.back() += l;
        return l;
      }
      fclose(in_f);
      in_f = nullptr;
    }
  }

  void sink_split(const byte* p, size_t n) {
    while( n ) {
      if( !out_f ) {
        if( sp_i>=this->ar.names.size() )
          return; // the carrier already checked the table against the total
        string path = out_dir+safe_name(this->ar.names[sp_i]);
        out_f = fopen(path.c_str(), "wb");
        if( !out_f ) {
          fprintf(stderr, "error: can't write '%s'\n", path.c_str());
          this->rc_err = 1;
          return;
        }
        sp_left = this->ar.sizes[sp_i];
      }
      size_t k = u64(n)<sp_left ? n : size_t(sp_left);
      if( k )
        fwrite(p, 1, k, out_f);
      p += k;
      n -= k;
      sp_left -= k;
      if( !sp_left ) {
        fclose(out_f);
        out_f = nullptr;
        sp_i++;
      }
    }
  }

  /* Opened lazily, so a run that fails before producing an output byte (a bad
     magic, a crc mismatch) leaves no file behind -- the property the old
     "decode fully, then save_file" order had for free. */
  void sink(const byte* p, size_t n) {
    if( !n )
      return;
    if( out_mem ) {
      out_mem->insert(out_mem->end(), p, p+n);
      return;
    }
    if( out_split&&!this->ar.names.empty() ) {
      sink_split(p, n);
      return;
    }
    if( !out_f ) {
      out_f = fopen(out_path, "wb");
      if( !out_f ) {
        fprintf(stderr, "error: can't write '%s'\n", out_path);
        this->rc_err = 1;
        return;
      }
    }
    fwrite(p, 1, n, out_f);
  }

  uint run() {
    uint r = 0;
    this->coro_init();
    this->addout(outbuf, bufsize);
    for(;; ) {
      r = this->coro_call(this);
      if( r==1 ) {
        if( in_mem ) {
          size_t left = in_memn-in_mempos;
          uint k = left>size_t(bufsize) ? uint(bufsize) : uint(left);
          if( !k )
            this->f_quit = 1;
          this->addinp((byte*)in_mem+in_mempos, k);
          in_mempos += k;
        } else {
          uint l = feed_files();
          if( !l )
            this->f_quit = 1;
          this->addinp(inpbuf, l);
        }
      } else { // r==0 quit, r==2 pin1 full
        uint l = this->getoutsize();
        if( l )
          sink(outbuf, l);
        if( r!=2 )
          break;
        this->addout(outbuf, bufsize);
      }
    }
    if( in_f ) {
      fclose(in_f);
      in_f = nullptr;
    }
    if( out_f ) {
      fclose(out_f);
      out_f = nullptr;
    }
    return r;
  }
};

/* One carrier per direction, in BSS.  They are declared here, after the
   MOD/*_h.inc includes, so the mapping objects those headers instantiate are
   constructed first -- Codec's hj_bits / sgn_maxnb member initialisers read
   G0_* values that come from them. */
static XadDrive< Xad<0> > Menc;
static XadDrive< Xad<1> > Mdec;

// ------------------------------------------------------------------- reporting

template<int MD>
static void report(const Codec<MD> &cx, const Arc &ar, size_t insz, u64 outsz) {
  size_t ncode = 0;
  for( size_t i = 0; i<ar.segs.size(); i++ ) {
    const Params &pm = ar.segs[i].pm;
    int spb = block_samples(pm);
    u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
    if( spb>0 )
      ncode += size_t(nblk)*size_t(spb-(pm.format==FMT_MS ? 2 : 1))*size_t(pm.chans);
  }
  fprintf(stderr, " %zu -> %llu bytes, %.4f of original", insz, (unsigned long long)outsz, double(outsz)/double(insz));
  if( ncode )
    fprintf(stderr, ", %.3f bits/code", 8.0*double(outsz)/double(ncode));
  fprintf(stderr, "\n");
  if( verbose ) {
    static const char* cat[ST_NCAT] = {"codes  ", "headers", "literal", "flags  "};
    for( int i = 0; i<ST_NCAT; i++ )
      if( cx.stat[i] )
        fprintf(stderr, "  %s %9.1f bytes\n", cat[i], double(cx.stat[i])/(8.0*double(ST_SCALE)));
    fprintf(stderr, "  coroutine stack high-water %llu of %u bytes\n",
            (unsigned long long)cx.stk_hi, unsigned(Coroutine::STKPAD));
  }
}

// what a segment is, in one line
static void describe(const Params &pm) {
  fprintf(stderr, "   %s %d-bit, %d ch, %u Hz, block %d%s\n", pm.format==FMT_MS ? "MS-ADPCM " : "IMA-ADPCM", pm.bps, pm.chans, pm.rate, pm.block_align, pm.canonical ? "" : ", header stored");
}

// ------------------------------------------------------------------- top level

static int compress(char** inp, int nin, const char* outp, bool test, int solid) {
  vector<u8> coded; // -t only: the archive, kept back until it has been verified
  Menc.in_paths = inp;
  Menc.in_n = nin;
  Menc.solid = solid;
  Menc.name = inp[0];
  if( test )
    Menc.out_mem = &coded;
  else
    Menc.out_path = outp;
  Menc.run();
  if( Menc.rc_err )
    return 1;

  if( test ) {
    Mdec.in_mem = coded.data();
    Mdec.in_memn = coded.size();
    vector<u8> back;
    Mdec.out_mem = &back;
    Mdec.name = outp;
    Mdec.run();
    if( Mdec.rc_err )
      return fprintf(stderr, "error: self-test failed, output not written\n"), 1;
    if( back.size()!=Menc.src.size()||memcmp(back.data(), Menc.src.data(), back.size()) )
      return fprintf(stderr, "error: self-test mismatch, output not written\n"), 1;
    FILE* f = fopen(outp, "wb");
    if( !f||(!coded.empty()&&fwrite(coded.data(), 1, coded.size(), f)!=coded.size()) ) {
      if( f )
        fclose(f);
      return fprintf(stderr, "error: can't write '%s'\n", outp), 1;
    }
    if( fclose(f) )
      return fprintf(stderr, "error: can't write '%s'\n", outp), 1;
  }

  const Arc &ar = Menc.ar;
  if( nin>1||ar.segs.size()>1||ar.lead ) {
    static const char* sm[3] = {"each wav independent", "solid within a type", "solid throughout"};
    fprintf(stderr, "%d file(s) -> %s: %zu wav(s)%s, %s\n", nin, outp, ar.segs.size(), ar.lead ? ", plus leading data" : "", sm[solid<0 ? 0 : solid>2 ? 2 : solid]);
  } else
    fprintf(stderr, "%s -> %s:\n", inp[0], outp);
  if( verbose||ar.segs.size()<=4 )
    for( size_t i = 0; i<ar.segs.size(); i++ )
      describe(ar.segs[i].pm);
  report(Menc.cx, ar, Menc.src.size(), Menc.hdr_bytes+Menc.emitted);
  return 0;
}

static int decompress(char* inp, const char* outp) {
  /* One argument does for both `d archive restored.wav` and `d archive dir/`:
     the driver splits only when the archive actually carries a file table, and
     it learns that while the header is parsed -- which is before the decoder
     can have produced an output byte. */
  string dir(outp);
  if( !dir.empty()&&dir[dir.size()-1]!='/'&&dir[dir.size()-1]!='\\' )
    dir += '/';
  Mdec.in_paths = &inp;
  Mdec.in_n = 1;
  Mdec.name = inp;
  Mdec.out_path = outp;
  Mdec.out_dir = dir;
  Mdec.out_split = 1;
  Mdec.run();
  if( Mdec.rc_err )
    return 1;
  const Arc &ar = Mdec.ar;
  if( ar.names.empty() )
    fprintf(stderr, "%s -> %s: %zu bytes, crc ok\n", inp, outp, Mdec.src.size());
  else
    fprintf(stderr, "%s -> %s: %zu file(s), %zu bytes, crc ok\n", inp, outp, ar.names.size(), Mdec.src.size());
  return 0;
}

int main(int argc, char** argv) {
  init_tables();
  init_ms_map();
  bool test = false;
  vector<char*> a;
  int mode = 0, solid = SOLID_OFF;
  for( int i = 1; i<argc; i++ ) {
    char* s = argv[i];
    if( s[0]=='-'&&s[1] ) {
      for( const char* o = s+1; *o; o++ )
        switch( *o ) {
        case 'v':
          verbose = 1;
          break;
        case 't':
          test = true;
          break;
        case 's':
          solid = solid<SOLID_ALL ? solid+1 : SOLID_ALL;
          break;
        default:
          return fprintf(stderr, "xadpcm: unknown option '%c'\n", *o), 1;
        }
    } else if( !mode&&(!strcmp(s, "c")||!strcmp(s, "d")) )
      mode = s[0];
    else
      a.push_back(s);
  }
  if( mode=='c'&&a.size()>=2 )
    return compress(&a[0], int(a.size())-1, a[a.size()-1], test, solid);
  if( mode=='d'&&a.size()==2 )
    return decompress(a[0], a[1]);
  fprintf(stderr, "\n xadpcm - lossless compressor for IMA-ADPCM and MS-ADPCM wav files\n\n"
          " usage: xadpcm c input.wav output              compress\n"
          "        xadpcm c a.wav b.wav ... output        compress several, solid\n"
          "        xadpcm d output restored.wav           restore one\n"
          "        xadpcm d output outdir/                restore several\n\n"
          " Wavs are found by scanning for RIFF/WAVE headers, so a single input\n"
          " holding concatenated wavs is handled like several inputs, and anything\n"
          " that is not a wav rides along as literal data.  By default each wav is\n"
          " modelled from scratch; -s and -ss let later wavs inherit what earlier\n"
          " ones taught the model, which pays off when they are alike and costs when\n"
          " they are not.\n\n"
          " options: -s  solid within a run of wavs of the same codec and code width\n"
          "          -ss solid across every wav, whatever the type\n"
          "          -t  verify by decoding in memory before writing (compress)\n"
          "          -v  per-stream byte accounting\n\n");
  return 1;
}
