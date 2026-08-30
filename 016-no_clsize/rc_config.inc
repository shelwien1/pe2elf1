
#pragma once

// Coder configuration: block geometry plus the knobs of the scalar rangecoder
// in rc.inc (which is sh_v1xN_s.cpp's, moved here unchanged apart from being
// parameterised by class name).
//
// SCALElog and friends live here rather than in FSM.cpp because rc.inc, the
// kernel build mapping in build.sh and the model all need them; FSM.cpp
// includes this file for them.
enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, hSCALE=SCALE/2, eSCALE=16*SCALE };

// 0 = the real setjmp/stack-copy coroutine of Lib3/coro3b.inc, so the coder
// streams through fixed 64K buffers.
// 1 = Lib3/coro3b_fake.inc, which calls do_process straight through and needs
// the whole file to fit in one buffer. Handy when the coroutine's stack copying
// gets in the way of a debugger or a sanitizer.
#ifndef CORO_FAKE
#define CORO_FAKE 0
#endif

// Stream buffer sizes, shared by coder.cpp and the model (model1.inc asserts
// outbufsize >= BLKSIZE, because the decode loop fills a whole block before it
// checks the output buffer).
#if CORO_FAKE==0
enum{ inpbufsize=1<<16, outbufsize=1<<16 };
#else
enum{ inpbufsize=1<<30, outbufsize=1<<30 };
#endif

// Bytes of input coded per block. One block = one set of RCNUM substreams,
// each with its own 2-byte length header. The decoder's output buffer has to
// hold a whole block, so outbufsize >= BLKSIZE (checked in model1.inc).
#ifndef RC_BLKSIZE
#define RC_BLKSIZE (1<<16)
#endif

// Number of rangecoder substreams per block. Bit i of the block is coded by
// lane i%RCNUM, so the lanes are independent and the original could run them
// as SIMD lanes. Here they are just RCNUM independent scalar coders -- the
// "fake vector" of rc_vec.inc -- but the bit-to-lane mapping and the resulting
// stream layout are unchanged.
#ifndef RC_RCNUM
#define RC_RCNUM 16
#endif
// The decoder emits a group's output bytes as one 16-bit store (what used to
// be RC_DEC_PUTW, now unconditional), so a group has to be an even number of
// output bytes: RCNUM/8 must be even, i.e. RCNUM a multiple of 16. Without
// this the build is silently wrong rather than broken -- RCNUM=8 roundtripped
// to garbage before the guard came back.
#if (RC_RCNUM/8)%2 || RC_RCNUM%8
#error "RC_RCNUM must be a multiple of 16: a group's output bytes are stored in pairs"
#endif

// Alignment for the lane array. Meaningless for the scalar coders, kept so the
// object still lays out the way the vector version needed it to.
#ifndef RC_VECSIZE
#define RC_VECSIZE 32
#endif

// Enums, not macros: rc_io.inc and rc_vec.inc take BLKSIZE and RCNUM as
// template parameters of the same name, the way sh_v1xN.inc did. Override from
// the command line with -DRC_BLKSIZE / -DRC_RCNUM / -DRC_VECSIZE.
enum { BLKSIZE=RC_BLKSIZE, RCNUM=RC_RCNUM, VECSIZE=RC_VECSIZE };

//--- sh_v1xN_s.cpp rangecoder knobs -----------------------------------------

// Width of the low accumulator, in bytes. 4..8.
// 4 is the classic 32-bit-low coder. Wider only buys carry headroom: a carry
// has LOWBYTES-CODBYTES extra bytes to travel through inside the register
// before it can escape into already-emitted output, which is what lets the
// carryless coder drop carry propagation from the hot path entirely.
//
// At 8 a carry is a ~2^-33 event per step, so the carryless fast path in
// model0.inc practically never has to fall back. Lower values make the
// fallback fire often, which is the cheap way to exercise it: book1 loses
// carries below 7 (3 at LOWBYTES=6, 565 at 5, 143379 at 4).
// The refill reads a 32-bit window that starts at the cursor, so the payload
// needs four bytes of room above it: RC_SKIP = RC_LOWBYTES - RC_CODBYTES must
// be at least 4, which rc_io.inc asserts. At the default RC_CODBYTES=4 that
// pins RC_LOWBYTES to 8; RC_CODBYTES=3 also admits 7.
#ifndef RC_LOWBYTES
#define RC_LOWBYTES 8
#endif

// 1 = hold the low accumulator as two uints instead of one qword. Stream-
// neutral: it is the same accumulator, so output is byte-identical either way.
//
// sh_v1xN_s.cpp defaults this on, because in the vector coder two uint lanes
// map onto one ymm each where a qword lane needs two. There is no vector coder
// here -- the host coder is scalar and the kernel holds low as one ulong
// whatever this says -- so the reason is gone, and measuring says so:
//
//                                      split=1   split=0
//   carryless, LOWBYTES=8, book1        19.75     19.84   -0.5%
//   carryless, LOWBYTES=8, 10 MB        14.37     14.56   -1.3%
//   carry twin, LOWBYTES=4, book1       19.59     20.08   -2.4%
//   decoder, any                          --        --     none
//
// The decoder is untouched by construction: low_Add, low_Top and low_Shift8
// are all under `if( f_DEC==0 )`. The carry twin at LOWBYTES=4 loses most,
// because there LOWBITS is 32 and the split version shifts a lowh that the
// mask then clears anyway.
#ifndef RC_LOWSPLIT
#define RC_LOWSPLIT 1
#endif






// 1 = skip the carryless fast path in the encoder and always code with the
// carry-propagating twin. Testing knob: it makes the fallback coder the only
// producer, so its output gets exercised on every block. Only the carryless
// coder has a kernel, so this also takes the OpenCL path out of the build.
//
// Note the two coders' output is not always identical: where the block ends in
// a run of 0xFF the carry twin trims them at the flush and the carryless coder
// has already written them. Both decode.
#ifndef RC_FORCE_CARRY
#define RC_FORCE_CARRY 0
#endif






// Full unroll of a group loop whose trip count is RCNUM: with j constant in
// every unrolled body, rc[j]'s fields become fixed addresses and the
// bit-position tests become compile-time -- which is most of what made the
// original sh_v1xN decoder fast (its "#pragma unroll(RCNUM)"), and none of it
// is vectorization.
#if defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
 #define RC_UNROLL _Pragma("unroll")
#elif defined(__GNUC__)
 #define RC_UNROLL _Pragma("GCC unroll 64")
#else
 #define RC_UNROLL
#endif


// A fingerprint of everything the generated kernel is resolved against.
// mk_kernel.sh bakes it into rc_vecD.inc and the compile static_asserts the
// same expression, so a kernel generated with one -D set and compiled with
// another is a build error rather than a silently wrong coder -- which is the
// whole reason build.sh takes its -D set as arguments and gc.bat has one
// %defs%.
//
// Only knobs that reach the GENERATOR belong here. Nothing target-derived:
// the kernel does not depend on the ISA, and anything the compile decides for
// itself is a macro the compile defines (see model.inc). RC_VECOUT and
// RC_CARRYLESS are excluded too -- mk_kernel.sh forces them, and the compile
// sets RC_CARRYLESS both ways.
#define RC_KERNEL_CONF ( RC_RCNUM + 64*RC_LOWBYTES + 2048*RC_LOWSPLIT      \
                       + 8192*RC_CODBYTES                                 \
                       + 1048576*RC_VECSIZE + RC_BLKSIZE/16               \
                       + 4096*(RC_FF_TRIM&63)                             \
                       + 33554432*RC_FOLD_RPRE                            \
                       + 13*RC_ENC_NSEL                                   \
                       + 17*RC_ENC_RENORM )

// 1 only inside mk_kernel.sh's preprocessing of rc.inc: selects the
// vector-shape ShiftLow/rc_Renorm/rc_Quit bodies the generated lane-array
// coder is built from. Never set in a normal compilation.
#ifndef RC_VECOUT
#define RC_VECOUT 0
#endif


#if RC_VECOUT && !RC_CARRYLESS
#error the vector kernel is the carryless coder -- generate with RC_CARRYLESS=1
#endif

// 1 = ShiftLow stages its store in the lane arrays instead of performing it,
// and the model commits a whole group of them with one vpscatterdd -- see
// stcl/stad in rc.inc and RC_FLUSHALL in model0.inc.
//
// The store is the coder's one hot store, and it goes to a per-lane address.
// SLP -- which is what handles the unrolled RCNUM-lane sweep, since the sweep
// is straight-line code and not a loop -- never forms a scatter: it vectorizes
// the arithmetic and then emits an address extract plus a scalar store per
// lane, 32 uops per group. Staging into stcl[]/stad[] leaves the sweep with
// nothing but contiguous array stores, which SLP does vectorize, and moves
// the scattering to one intrinsic after the group.
//
// Only where that trade is available: AVX-512 for the scatter, RCNUM a whole
// number of 16-lane scatters, RC_LOWSPLIT for the 32-bit store shape (at
// LOWSPLIT=0 with LOWBYTES=8 the store is a qword and would want a different
// scatter), and clang, because the trade is with SLP -- gcc does not vectorize
// the sweep at all, so staging buys it nothing and costs it two arrays of real
// memory traffic (22.97 -> 19.06 MB/s here). Everywhere else the store stays
// exactly where it is: without a scatter the staged form is strictly worse.
//
// Nothing about the stream changes: the same dword lands at the same address
// with the same bytes, one group later, and the encoder never reads tmpbase
// while coding. t.sh checks that against the scalar coder.
// Lanes per commit, and which instruction commits them:
//   16  vpscatterdd zmm      (AVX-512F)
//    8  vpscatterdd ymm      (AVX-512VL)
//    0  no scatter at all -- a block of RCNUM scalar stores (AVX2 and below)
//
// On AVX-512 this has to match the width the sweep's arithmetic came out at,
// not the widest the target has. clang defaults to -mprefer-vector-width=256
// on skylake-avx512 and later, so the staged arrays live in ymm halves; asking
// for a 16-lane scatter there makes it spill both halves and reload them as
// one zmm, and the 32-byte store under a 64-byte load cannot store-forward.
// Measured on cascadelake: 49.6 MB/s that way against 66.4 for the direct
// store, where the 8-lane scatter is 66.9. Pair -mprefer-vector-width=512 with
// -DRC_SCATTER_W=16 if you want to measure the wide one (72.1 MB/s here,
// still behind the 8-lane default).
#ifndef RC_SCATTER_W
# if defined(__AVX512VL__)
#  define RC_SCATTER_W 8
# elif defined(__AVX512F__)
#  define RC_SCATTER_W 16
# else
#  define RC_SCATTER_W 0
# endif
#endif

#ifndef RC_SCATTER
# if !defined(__clang__) || !RC_LOWSPLIT
#  define RC_SCATTER 0
# elif RC_SCATTER_W==0
   // the scalar block: no ISA requirement, but it is a straight trade of port-5
   // shuffles for port-4 stores and the staged arrays cost registers, so it
   // stops paying as RCNUM grows. Measured on AVX2/cascadelake, 20 MB of
   // enwik8: RCNUM=8 +3.3%, RCNUM=16 +6.6%, RCNUM=32 -3.2%.
#  define RC_SCATTER (RC_RCNUM<=16)
# else
#  define RC_SCATTER ((RC_RCNUM%16)==0)
# endif
#endif

// What to do about the lanes that staged nothing. A ShiftLow with n==0 leaves
// the cursor where it was, so the window it staged is written again by that
// lane's next store before anything reads the row: its store is dead and can
// simply not happen. And that is the common case -- the coder emits about
// 0.08 bytes per coded bit, so a lane renormalises on about one bit in
// thirteen, only ~1.3 of 16 lanes owe a store, and a whole group stages
// nothing about a quarter of the time.
//
//   0  scatter every lane every group
//   1  scatter under a k-mask of the lanes with n!=0
//   2  the same, and branch past the scatter when that mask is empty
//   3  scatter every lane, but branch past the whole thing when every n is 0
//   9  probe: no scatter at all. Does NOT encode correctly -- it is here to
//      measure what the scatter costs on a given machine, which is what says
//      whether any of this is worth doing there. Time it with a plain encode,
//      `./coder c <file> /dev/null ../FSM0.txt 6`; do not put it through t.sh,
//      whose decode step would be handed a stream that does not decode.
//
// At RC_SCATTER_W==0 there is no mask, so only 0 (always commit) and non-zero
// (branch past a group where no lane emitted a byte) differ there.
//
// 0..3 all produce the same stream -- t.sh checks each. On cascadelake only 0
// and 1 are worth having, on either target. 20 MB of enwik8, clang 18, best of
// 6x5 (the box turbos to 3.235 GHz measured, not its 2.8 GHz base -- every
// cycle figure in this file is at the measured clock):
//
//   AVX-512, -march=native            AVX2, -march=skylake
//   0  unmasked          75.9 MB/s    0  block of scalar stores  66.9 MB/s
//   1  masked            74.7         3  + branch, 1 per group   60.0
//   2  masked + branch   54.2         3  + branch, 1 per 8 lanes 50.0
//   3  unmasked + branch 53.8         9  probe: no commit        75.1
//   9  probe: no scatter 77.6
//   -  probe: mask forced 0  74.8
//
// The probes are the whole story. On AVX-512 a vpscatterdd whose mask is empty
// costs what one with a full mask costs, so masking cannot buy anything and 1
// only pays for the stn array; and 0 against 9 puts the scatter's ENTIRE cost
// at 1.85 cycles per 16-bit group against 30.8 for the sweep and 85.3 for the
// encode, so 2.2% of encode is the ceiling on every scheme that aims to
// scatter less often.
//
// On AVX2 the commit is worth far more -- 10.6 cycles per group -- and the
// branch STILL loses: one branch per group, taken about a quarter of the time
// and unpredictable, costs 11.2 cycles where it could save at most 2.8. Two
// branches cost 32.6. There is no frequency at which this pays; it is the
// misprediction, not the work.
//
// Where scatter cost scales with the number of active lanes -- Ice Lake and
// later, Zen 4 -- 1 is the one to try on AVX-512: same instruction count, no
// branch, ~1.3 lanes of work instead of 16. Build 9 first: if 0 and 9 are
// close on your machine too, nothing in this direction can pay there either.
//
// 9 drops the commit on BOTH paths, so the same probe answers the same
// question on AVX2 -- what the block of RCNUM scalar stores costs there. It
// used to fall into the "non-zero means branch" reading at RC_SCATTER_W==0,
// which measured the branch instead and made the two targets incomparable.
// See misc/avx2_vs_avx512.md for what the pair of probes is for.
#ifndef RC_SCATTER_SKIP
#define RC_SCATTER_SKIP 0
#endif
#if RC_SCATTER_SKIP<0 || RC_SCATTER_SKIP>1
#error "RC_SCATTER_SKIP is 0 or 1: the branch variants and the no-commit probe are gone"
#endif


// The encoder half of the same two lessons. rc_Renorm's f_DEC==0 arm has
// always been `n = (range<sTOP) + (range<gTOP)`, a byte count in a local.
//
//   0  that
//   1  the SHIFT COUNT from a nested ternary, in a const local
//   2  the same, in lane state (dsh) instead of a local
//
// The encoder is not the decoder: its sweep is vector code over lane arrays
// that clang auto-vectorises, so a scalar local there is a vector register,
// not sixteen scalar ones, and the pressure argument that makes the decoder
// care does not apply -- to the LOCALS. It turns out the SELECTION is worth
// something anyway.
//
// Measured here with twins on both arms, 15 round-robin rounds of best-of-3,
// 100 MB of enwik8:
//
//   NSEL=0   91.82  92.37   mean 92.10
//   NSEL=1   93.97  94.51   mean 94.24    +2.3%
//
// The twins are 0.6% apart and the two arms do not overlap, so +2.3% is about
// four times the noise floor. A first pass at 7 rounds without twins on both
// sides read this as flat and this comment said so; it was under-powered.
//
// The win is entirely in the ternary, and RC_ENC_RENORM below is the evidence:
// carrying the shift count all the way into ShiftLow instead of round-tripping
// it through >>3 and *8 adds nothing, and removing every local from ShiftLow's
// body adds nothing. Only the 0 -> 1 selection change moves. The mechanism is
// thin -- {0,8,16} from two blends against {0,1,2} from two compares and a
// vpslld -- which is why the default has not moved: his box is not this one and
// the last small one-box win in this file inverted on it. One build says
// whether it transfers.
//
// His own log cannot answer it: the encode column there sits between 80.0 and
// 81.8 MB/s across all forty rows, but those forty rows vary the DECODER's
// shape, so they say nothing about this knob.
//
// 1 was also the bug that found rc_macro.pl's missing parentheses. ShiftLow's
// body is `n*8`, so `ShiftLow( sh>>3 )` expanded to `sh>>3*8` -- `sh>>24`,
// always 0 -- and encoded a stream whose decode never terminated. The
// generator parenthesises its macro parameters now, and mk_kernel.sh fails if
// it is stale.
#ifndef RC_ENC_NSEL
#define RC_ENC_NSEL 0
#endif

// The rest of the encoder's renorm, and the thing RC_ENC_NSEL alone could not
// reach. That knob picks the SHIFT COUNT with a ternary -- and then handed
// ShiftLow `dsh>>3`, whose first statement was `uint sh = n*8`. The shift
// count was selected, divided by eight, passed, and multiplied by eight again,
// on the chain, every bit. Whatever the ternary was worth, the round trip was
// spending it: that is why 0/1/2 all measured inside the noise.
//
// This knob fixes the direction and then keeps going, because ShiftLow still
// declares three more locals -- sh, cl and _hi -- and cl and _hi are each read
// exactly once.
//
//   0  ShiftLow takes the BYTE count and derives the shift: `uint sh = n*8`,
//      plus `uint cl` and `uint _hi`                              <- default
//   1  ShiftLow takes the SHIFT COUNT and derives the byte count for tmpptr,
//      which is the only thing that wants it. No sh local, no round trip
//   2  1, and cl and _hi written out where they were read, so the body
//      declares nothing at all
//
// Measured, 15 rounds, twins, against the 92.10 baseline of RC_ENC_NSEL=0:
//
//   RC_ENC_RENORM  0      1      2
//   RC_ENC_NSEL=1  92.29  91.58  91.88     (and 92.05/91.71 for the 2/2 twins)
//
// So the answer is no. Every shape that uses the ternary lands at +2 to +2.7%
// and they are within a noise floor of each other; the round trip and the
// locals are both free. clang folds (x>>3)*8 back to a mask, and the locals
// are vector registers in the sweep, exactly as the argument above says. The
// knob stays because it makes the encoder's renorm say what it means and
// because it is the control that proves the ternary is the whole effect --
// not because it is faster.
//
// The direction is the point. Downstream of the selection, three of the four
// uses want the shift -- lowh<<sh, lowl<<sh, the _hi extraction -- and only
// tmpptr wants the count, so selecting the shift and deriving the count costs
// one shift where the reverse costs a multiply on the chain. It is the same
// argument the decoder's dsh made, and RC_ENC_NSEL=1/2 was making it in the
// right place and then undoing it one call deeper.
//
// stn[] takes the count as a store-is-dead flag and model.inc only ever
// compares it against zero, so handing it the shift instead is the same
// predicate with no fixup.
//
// 2 leaves the !RC_LOWSPLIT path and the !RC_FOLD_RPRE carry block alone --
// both declare locals of their own and neither is the default shape.
#ifndef RC_ENC_RENORM
#define RC_ENC_RENORM 0
#endif





#if RC_ENC_RENORM>=1
#define RC_ENC_SH      (n)
#define RC_ENC_NB      ((n)>>3)
#define RC_ENC_ARG(x)  (x)
#else
#define RC_ENC_SH      sh
#define RC_ENC_NB      (n)
#define RC_ENC_ARG(x)  ((x)>>3)
#endif

#if RC_ENC_RENORM>=2
#define RC_ENC_HI   ((lowl >> 16) >> (16-RC_ENC_SH))
#if RC_LOWBYTES==8
#define RC_ENC_CL   lowh
#elif RC_LOWBYTES>=6
#define RC_ENC_CL   (lowh << (64-LOWBITS))
#elif RC_LOWBYTES==5
#define RC_ENC_CL   ((lowh<<24) | (lowl>>8))
#else
#define RC_ENC_CL   lowl
#endif
#else
#define RC_ENC_CL   cl
#endif


// 1 = apply rc_Process's rpre to low where it is PRODUCED, instead of leaving
// it pending for the next ShiftLow to add. Encoder only -- f_DEC is a template
// parameter, and the decoder still needs rpre for ShiftCode's `code -= rpre`.
//
// Stream-neutral, and not by luck: nothing reads low between the two points,
// so it is the same addition in the same order. The carry detection and the
// FFNum mask move along with it, so the instruction count barely changes --
// what changes is that rpre stops being loop-carried, which is worth two ymm
// in a sweep whose live state is 19 ymm in a 16-register file. That is the
// single biggest item in misc/model0_process_skl.asm's accounting: 27 of the
// loop's ~158 uops are spill traffic.
//
// It does exactly what it was supposed to do to the instruction stream, and it
// is still SLOWER. AVX2, -march=skylake, RCNUM=16, 20 MB of enwik8, medians of
// best-of-6 over 8 round-robin runs, with a copy of the baseline binary in the
// rotation as the noise control:
//
//   0  rpre applied at the next ShiftLow   145 insns, 26 st   73.61 MB/s
//   1  rpre applied in rc_Process          139 insns, 24 st   71.73   -2.55%
//   -  control (baseline binary again)     identical          73.59   -0.03%
//
// Six fewer instructions, two fewer stores, three fewer vextracti128 -- and
// 2.6% off, against a control that says the noise floor is 0.03%. The likely
// reason is that the staged store's value now depends on the SAME iteration's
// vpmulld rather than the previous one's: the fold buys uops and spends the
// slack the scheduler was hiding the 10-cycle multiply in. Not isolated, so
// read it as a hypothesis -- what is measured is the loss.
//
// Kept because it is the one structural way to shrink that live set, and a
// wider machine (or a target where the multiply is cheaper) could flip it.
#if !RC_LOWSPLIT || RC_LOWBYTES!=8
#undef  RC_FOLD_RPRE
#define RC_FOLD_RPRE 0
#endif
#ifndef RC_FOLD_RPRE
#define RC_FOLD_RPRE 0
#endif

// 1 = fold pp[] into the counter step. The model carries {p:16, state:16} per
// context instead of a bare state, and FSM2[state*2+bit] (a dword, still ONE
// scaled index) already holds the next state's probability -- so the step
// reads one table where it read two, and the probability arrives WITH the
// counter instead of one dependent load later.
//
// Stream-neutral: identical p, identical states, identical output.
//
// Set per MODEL, because the two disagree. The decoder is chain-bound: its
// per-bit critical path is the cty load, then pp[state] whose address is that
// load's result, then the multiply, the compare and the context update. Taking
// the second load off that chain is worth ~18% on BOTH targets, and a probe
// that breaks only that load (p computed from state, so the stream diverges but
// the shape does not) says 20.3% is the ceiling.
//
// The encoder's model pass is rename-bound, not chain-bound, and clang
// micro-fuses `add pp(,%state,2),%p` into one instruction -- so the load being
// removed was never costing an instruction slot, and what happens instead is
// target-dependent codegen luck:
//
//   -march=skylake        (AVX2)     encode +2.71%   decode +17.97%
//   -march=native         (AVX-512)  encode -4.39%   decode +18.26%
//
// The AVX-512 loss is SLP: with p arriving as a dword, clang decides to build
// the eight {bit,p} pairs of a byte into vectors -- 16 `vpinsrd` and 10 `vmovd`
// per input byte, all port 5 -- to merge eight 2-byte pbit stores into two wide
// ones. It is not a knob away: `-fno-slp-vectorize` also scalarises the coding
// sweep (-63.9%), and clang does not forward it to the LTO backend anyway.
//
// And it does not survive a compiler upgrade. clang 23.1.0 still emits the
// gather (12 vpinsrd instead of 16) and is FASTER for it. Same box, same
// source, -march=native, -Ofast -std=c++23, 20 MB of enwik8:
//
//   clang 18.1.3   encode 85.09 -> 77.09   -9.41%   (control -0.14%)
//   clang 23.1.0   encode 81.25 -> 84.48   +3.98%   (control +0.17%)
//
// So the default follows the compiler, and the cut sits between the two
// versions actually measured -- 19..22 are untested, and this is one -D away
// in either direction if a build lands on the wrong side of it.
#ifndef RC_FUSE_PP_ENC
# if defined(__clang__) && defined(__AVX512F__) && (__clang_major__ < 19)
#  define RC_FUSE_PP_ENC 0
# else
#  define RC_FUSE_PP_ENC 1
# endif
#endif

#ifndef RC_FUSE_PP_DEC
#define RC_FUSE_PP_DEC 1
#endif



// Chunk size, in input bytes, for interleaving the model pass with the coding
// sweep. 0 = off, the model pass runs once over the whole block.
//
// pbit[] is BLKSIZE*8 words -- 1 MB written and 1 MB read per 64 KB block, 32
// bytes of traffic per input byte, for a value produced and consumed in the
// same order. Chunking makes that buffer CHUNK*16 bytes and keeps it in L1.
// Must be even so each chunk is a whole number of RCNUM-bit groups.
//
// misc/avx2_profile.md section 8 has "chunked model/sweep, 2048" at +10.27
// cyc/grp, i.e. a loss, and blames tmpbuf evicting FSM+pp from L2. That
// mechanism was never real -- FSM0.txt has 256 states, so the model's whole
// working set is 2 KB -- and this version wins. The difference is where pbit
// is indexed: both passes work its low end and the chunk is rewritten in
// place, so the buffer between them is 32 KB at CHUNK=2048 rather than a 1 MB
// window that walks. Measured, encode, byte-identical stream:
//
//     CHUNK      256    512   1024   2048   4096   8192
//     skylake  +3.84  +3.53  +4.16  +3.69  +3.17  +3.77   (control -0.95)
//     rerun       --  +2.64  +2.58  +3.18     --     --    (control +0.02)
//     native      --     --     -- +2.56     --     --    (control -0.18)
//
// Flat across two decades of chunk size, which says it is the traffic and not
// a cache-capacity knee. Decode is untouched -- it has no model pass to split.
#ifndef RC_CHUNK
#define RC_CHUNK 2048
#endif
#if RC_CHUNK && (RC_CHUNK&1)
#error "RC_CHUNK must be even"
#endif
#define RC_CHUNK_N ( RC_CHUNK ? RC_CHUNK : RC_BLKSIZE )






// How many slots the generated kernel gives FFNum. Its only consumer is
// model0.inc's `ncarry &= FFNum[j]` over every lane at block end, followed by a
// whole-block re-code -- so the per-lane values are never read apart and the
// array can be narrower than RCNUM, with lane j folding into slot j%N. At
// RCNUM=16 and N=8 that is one ymm instead of two, and one fewer spilled array
// in the sweep's working set. speed_plan_next.md 5.1.
// Fixed at 8: both defs profiles set it, and it reaches the generator.
#define RC_FF_LANES 8
#if RC_RCNUM % RC_FF_LANES
#error "RC_FF_LANES must divide RC_RCNUM"
#endif

// 1 = the coding sweep walks pbit[] with a negative index counting up to zero,
// off a biased base, so the group loop's latch is `add/jne` (one fused uop)
// instead of `add/cmp/jb` (two). speed_plan_next.md 5.2.
//
// MEASURED, AND IT DOES NOT FIRE: clang emits `add $16,%rcx; cmp $-16,%rcx;
// jl` -- it keeps the compare rather than using the add's own flags -- so the
// instruction count is unchanged at 142 per group and the timing follows
// (-0.93% encode against a -0.46% control). The other three items in 5.2 are
// register-allocator and constant-placement decisions with no source spelling
// at all.
#ifndef RC_SWEEP_NEGIDX
#define RC_SWEEP_NEGIDX 0
#endif

// Width of the code register, in bytes. 4 everywhere -- it is what the decoder
// holds and what sTOP/gTOP are written against -- but named, because the zero
// prefix below is the difference between it and the low accumulator.
#ifndef RC_CODBYTES
#define RC_CODBYTES 4
#endif
#if RC_CODBYTES<3 || RC_CODBYTES>4
#error "RC_CODBYTES must be 3 or 4"
#endif

// The encoder's leading zero prefix. Additions land in the bottom 32 bits of
// low while ShiftLow emits the top byte, so the top has not been reached yet;
// that lag is the point of the wide accumulator, and the prefix is not part of
// the stream. It is written into the substream row like any other byte and
// dropped by rc_Write, which starts each lane's payload past it -- rather than
// tested for in put(), which is the coder's one hot store.
#define RC_SKIP (RC_LOWBYTES - RC_CODBYTES)

// The decoder reads 0xFF past the end of a substream, because rc_Read pads it
// with them, and that is what lets rc_Quit leave the tail of its flush out.
// Two numbers have to agree for it to be safe:
//
//   RC_FF_TRIM     the most bytes rc_Quit may leave out
//   RC_FF_PADSIZE  the 0xFF run rc_Read writes past each substream
//
// and the pad has to cover the trim plus the LOWBYTES the minimal flush drops
// on its own. The carry-propagating coder's flush drops a whole trailing 0xFF
// run, and that run is as long as the data makes it -- 6384 bytes on a
// repeating {0xFF,0x00} -- so the trim has to be capped rather than assumed
// small. Leaving it uncapped decodes to the wrong bytes, silently.
#ifndef RC_FF_TRIM
#define RC_FF_TRIM 32
#endif
#ifndef RC_FF_PADSIZE
#define RC_FF_PADSIZE (RC_FF_TRIM + 2*RC_LOWBYTES + 16)
#endif


