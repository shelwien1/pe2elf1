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
// parameters, an optional file table, u32 crc32, rc stream.
//
// The decoder is STREAMING: it produces the output a window at a time and writes
// as it goes, so its memory is flat in the size of the file.  The crc is
// therefore checked after the last byte rather than before the first, and a
// mismatch removes what was written -- see XadDrive::discard_output.  -b restores
// the old order for the case where that is not good enough (a pipe).  The ENCODER
// still needs the whole input, and that is the format's doing rather than the
// codec's: nseg, the segment geometry and the crc all sit ahead of the first
// coded byte.  PLAN-streaming.md §5 is what that would take.
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
// idx2inc.pl turns those into the generated MOD headers.  The Debug build leaves each value in
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
// The program is ONE translation unit assembled from parts, the way
// dxt5comp_core.inc assembles the d5c codec.  xadpcm.cpp is a table of contents
// plus main(); every part is a .inc included below, and every entity is defined
// exactly once, in one file.  There are no separately compiled modules.
//
//   xad_prelude.inc    the arithmetic contract and the clamp helpers
//   xad_idxparam.inc   the IDX bridge: NCONF, and IDX values as template
//                      parameters in both build modes (see the file)
//   xad_util.inc       crc32b, byte order, varints, bit packing
//   xad_logistic.inc   squash/stretch tables, the counter rate schedules
//   xad_rc.inc         RC<MD> : Coroutine
//   xad_counter.inc    Ctr / CtrS and their tables
//   xad_mix.inc        Mixer<MLR,WCL>, APM<APN,APR>
//   xad_ima.inc        IMA ADPCM arithmetic
//   xad_ms.inc         MS ADPCM arithmetic
//   xad_pred.inc       Pred<VAR> -- the NLMS cascade
//   xad_match.inc      MatchModel<MORD,MHB,MTB>
//   xad_wav.inc        RIFF parsing, Params, analyze()
//   xad_codec.inc      Codec<MD>, itself assembled from
//                        xad_codec_bits.inc  one mixed bit, one ADPCM code
//                        xad_codec_ima.inc   the IMA block walk
//                        xad_codec_ms.inc    the MS block walk
//   xad_arc.inc        the container: segment table, wav scan, geometry
//   xad_front.inc      Xad<MD> -- the frontend coroutine
//   xad_drive.inc      XadDrive<> -- the pump, the carriers, reporting
//
// build: ./mk.sh            Debug (optimizable)
//        ./mk.sh release    Const (ship this)
//        or, without perl:  g++ -std=gnu++20 -O2 -I. -ILib3 xadpcm.cpp -o xadpcm
// tune:  ./mk.sh && perl IDX/opt.pl opt.lst
//        cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! >t && mv t $f; done
// usage: xadpcm c input.wav output
//        xadpcm d output restored.wav

// <cmath> was here for init_tables().  The logistic tables are integer now and
// nothing else on the coding path is floating point, so the codec no longer
// depends on libm; the doubles left in xad_drive.inc are report formatting.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
/* <string> and <vector> were here.  Both are gone: Buf<T> and NameTab in
   xad_util.inc cover what they were being asked for (append, index, one-shot
   fill), the per-block scratch is static storage sized by the format's own
   u16 block_align, and paths are fixed char[] with a reported -- never silent --
   truncation.  See PLAN-vectors.md. */
#ifdef __linux__
#include <sys/mman.h>   // the counter arena asks for huge pages
#endif

/* Lib3.  common.inc supplies byte/word/uint/qword, INLINE/NOINLINE/ALIGN,
   if_e0/if_e1 and __assume, all of which coro3b.inc needs; coro3b.inc defines
   Coroutine, which RC derives from.  Both must precede everything below. */
#include "common.inc"
#include "coro3b.inc"

/* g++ -Os miscompiles the Lib3 coroutine: the second yield() receives a
   corrupted `this` and the stack-copy memcpy walks off the end.  Bisected to
   the coroutine port itself, so it is not a consequence of any later change,
   and a bare twenty-line Lib3 coroutine survives -Os -- it takes a deeper call
   chain to expose.  Every other level is correct AND byte-identical: g++
   -O0/-O1/-O2/-O3/-Ofast and clang++ -O0..-O3/-Os/-Oz/-Ofast.
   __builtin_setjmp/__builtin_longjmp are documented by GCC as a language-runtime
   facility with strict constraints rather than a general mechanism, and
   coro3b.inc already records one earlier -O2/-Ofast miscompile of a previous
   implementation, so this is that hazard again rather than a new one.  Refuse
   the build instead of shipping a binary that crashes on the second window. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__OPTIMIZE_SIZE__)
#error "g++ -Os miscompiles the Lib3 coroutine (see STATUS-speed.md); build with -O2/-O3, or use clang++"
#endif

// #include "adpcm-lib.h"

int adpcm_block_size_to_sample_count(int block_size, int num_chans, int bps) {
  return (block_size-num_chans*4)/num_chans*8/bps+1;
}

/* There is no `using namespace std` and now nothing to pull in from it either.
   Keeping it out was always deliberate -- the IDX generator emits Const-mode
   tables typed `byte` and `word`, and a using-directive would make those names
   ambiguous against std::byte rather than shadowing it; common.inc declares both
   at global scope, so they win over std's. */
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

#include "xad_prelude.inc"

/* IDX-generated parameters and context quantizers -- see the IDX directory.
   In the Debug build these are `mapping` objects whose pattern strings live in
   the binary, where opt.pl flips their bits; in the Const build idx2inc.pl has
   already folded them to literals. */
#include "sh_mapping.inc"
#include "MOD/xad-C0_h.inc"
#include "MOD/xad-C0_p.inc"
#include "MOD/xad-G0_h.inc"
#include "MOD/xad-I0_h.inc"
#include "MOD/xad-I0_p.inc"
#include "MOD/xad-M0_h.inc"
#include "MOD/xad-M0_p.inc"
#include "xad_idxparam.inc"

/* ---------------------------------------------------------------------------
   The rest of the program, bottom up.  Every part is included into THIS
   translation unit -- no separately compiled modules, and every entity defined
   exactly once, in one file.

   Beyond ordinary bottom-up dependency order, two constraints are load-bearing
   and neither is visible from the file names:

     common.inc / coro3b.inc  must precede xad_rc.inc, because RC derives from
                              Coroutine.  They are included above, by the .cpp,
                              rather than by xad_rc.inc itself: a part that
                              pulls its own prerequisites makes the order
                              depend on who included whom first.

     xad_prelude.inc          must precede sh_mapping.inc and the generated
                              MOD headers, which need pclamp and the contract
                              constants for their
                              sizes; and xad_idxparam.inc must FOLLOW them,
                              because it switches on USE_NEW, which they are
                              what define.  That cycle is the whole reason the
                              IDX bridge is two files.  Do not merge them.
   ------------------------------------------------------------------------ */
#include "xad_util.inc"      // crc32b, byte order, varints, bit packing
#include "xad_logistic.inc"  // squash/stretch tables, counter rate schedules
#include "xad_simd.inc"      // the two NLMS kernels (scalar / SSE4.1 / AVX2)
#include "xad_rc.inc"        // RC<MD> : Coroutine -- the range coder
#include "xad_counter.inc"   // Ctr / CtrS and their tables
#include "xad_mix.inc"       // Mixer<MLR,WCL>, APM<APN,APR>
#include "xad_ima.inc"       // IMA ADPCM arithmetic
#include "xad_ms.inc"        // MS ADPCM arithmetic
#include "xad_pred.inc"      // Pred<VAR> -- the NLMS cascade
#include "xad_match.inc"     // MatchModel<ORD,HB,TB>
#include "xad_wav.inc"       // RIFF parsing, Params, analyze()
#include "xad_codec.inc"     // Codec<MD> -- the model
#include "xad_arc.inc"       // the container: segment table, scan, geometry
#include "xad_front.inc"     // Xad<MD> -- the frontend coroutine
#include "xad_drive.inc"     // XadDrive<> -- the pump, the carriers, reporting

// ------------------------------------------------------------------- top level
// ------------------------------------------------------------------- top level

static int compress(char** inp, int nin, const char* outp, bool test, int solid) {
  Buf<u8> coded; // -t only: the archive, kept back until it has been verified
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
    Buf<u8> back;
    Mdec.out_mem = &back;
    Mdec.name = outp;
    Mdec.run();
    if( Mdec.rc_err )
      return fprintf(stderr, "error: self-test failed, output not written\n"), 1;
    if( back.size()!=Menc.src.size()||memcmp(back.data(), Menc.src.data(), back.size()) )
      return fprintf(stderr, "error: self-test mismatch, output not written\n"), 1;
    FILE* f = xfopen(outp, true);
    if( !f||(!coded.empty()&&fwrite(coded.data(), 1, coded.size(), f)!=coded.size()) ) {
      if( f )
        xfclose(outp, f);
      return fprintf(stderr, "error: can't write '%s'\n", outp), 1;
    }
    if( is_std(outp) ? fflush(f)!=0 : fclose(f)!=0 )
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
  report(Menc.cx, ar, Menc.src.size(), Menc.hdr_bytes+Menc.emitted, Menc.stk_hi);
  return 0;
}

static int decompress(char* inp, const char* outp, bool buffered) {
  /* One argument does for both `d archive restored.wav` and `d archive dir/`:
     the driver splits only when the archive actually carries a file table, and
     it learns that while the header is parsed -- which is before the decoder
     can have produced an output byte. */
  /* Built straight into the driver's own buffer.  A path that does not fit is
     refused here rather than truncated -- the split sink would otherwise write
     every member into the wrong directory. */
  /* A split archive needs somewhere to put its members, and a pipe is not it.
     Refused here rather than by writing every member into one stream, which
     would look like it worked. */
  if( is_std(outp) )
    Mdec.out_split = 0;
  size_t dl = strlen(outp);
  if( dl+2>sizeof(Mdec.out_dir) )
    return fprintf(stderr, "xadpcm: output path '%s' is too long\n", outp), 1;
  memcpy(Mdec.out_dir, outp, dl);
  if( dl&&Mdec.out_dir[dl-1]!='/'&&Mdec.out_dir[dl-1]!='\\' )
    Mdec.out_dir[dl++] = '/';
  Mdec.out_dir[dl] = 0;
  Mdec.out_split = is_std(outp) ? 0 : 1;
  Mdec.in_paths = &inp;
  Mdec.in_n = 1;
  Mdec.name = inp;
  Mdec.out_path = outp;
  Mdec.buffered = buffered;
  Mdec.run();
  if( Mdec.rc_err )
    return 1;
  const Arc &ar = Mdec.ar;
  if( verbose )
    fprintf(stderr, "  coroutine stack high-water %llu of %u bytes\n",
            (unsigned long long)Mdec.stk_hi, unsigned(Coroutine::STKPAD));
  if( ar.names.empty() )
    fprintf(stderr, "%s -> %s: %llu bytes, crc ok\n", inp, outp, (unsigned long long)Mdec.outn);
  else
    fprintf(stderr, "%s -> %s: %zu file(s), %llu bytes, crc ok\n", inp, outp, ar.names.size(), (unsigned long long)Mdec.outn);
  return 0;
}

int main(int argc, char** argv) {
  xad_simd_init();
  init_tables();
  init_ms_map();
  bool test = false, buffered = false;
  /* argv minus the mode word and the switches, compacted in place.  argv and
     the array itself are modifiable by the standard, and the write index can
     only trail the read index -- a[an] with an < i is a slot already consumed --
     so no buffer is needed at all, and no bound to pick for one. */
  char** a = argv;
  int an = 0;
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
        case 'b':
          buffered = true;
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
      a[an++] = s;
  }
  if( mode=='c'&&an>=2 )
    return compress(a, an-1, a[an-1], test, solid);
  if( mode=='d'&&an==2 )
    return decompress(a[0], a[1], buffered);
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
          "          -b  rebuild the whole stream and verify the crc BEFORE writing\n"
          "              a byte of it (decompress).  Without it the output is written\n"
          "              as it is decoded and a crc failure removes what was written,\n"
          "              which cannot be done once the output is a pipe.\n"
          "          -v  per-stream byte accounting\n\n");
  return 1;
}
