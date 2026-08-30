
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
// The decoder emits a group's output bytes as one 16-bit store, so a group has
// to be an even number of output bytes: RCNUM/8 must be even, i.e. RCNUM a
// multiple of 16. Without this guard the build is silently wrong rather than
// broken -- RCNUM=8 roundtripped to garbage.
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
#define RC_KERNEL_CONF ( RC_RCNUM + 64*RC_LOWBYTES                        \
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
// number of 16-lane scatters, and clang, because the trade is with SLP -- gcc
// does not vectorize
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
# if !defined(__clang__)
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
// simply not happen. That is the common case -- the coder emits about 0.08
// bytes per coded bit, so only ~1.3 of 16 lanes owe a store, and a whole group
// stages nothing about a quarter of the time.
//
//   0  commit every lane every group
//   1  with a scatter (RC_SCATTER_W!=0), scatter under a k-mask of the lanes
//      with n!=0; without one, branch past a group where no lane owes a store
//
// Both produce the same stream. 20 MB of enwik8, clang 18, best of 6x5 -- the
// box turbos to 3.235 GHz measured, not its 2.8 GHz base, and every cycle
// figure in this file is at the measured clock:
//
//   AVX-512, -march=native     0 unmasked 75.9 MB/s     1 masked 74.7
//   AVX2,    -march=skylake    0 scalar   66.9          1 branch 60.0
//
// So neither profile is leaving anything on the table: defs_avx512 takes 1 and
// defs_avx2 takes 0. On AVX-512 a vpscatterdd with an empty mask costs what a
// full one costs, so masking cannot buy anything and 1 only pays for the stn
// array -- a probe with the scatter removed entirely ran 77.6, putting its
// whole cost at 1.85 cycles per 16-bit group against 30.8 for the sweep, so
// 2.2% of encode is the ceiling on any scheme that scatters less often.
// Without a scatter the only lever is a branch, and one unpredictable branch
// per group costs 11.2 cycles where it could save at most 2.8; it is the
// misprediction, not the work. misc/avx2_vs_avx512.md has the probe pair.
//
// Where scatter cost DOES scale with the number of active lanes -- Ice Lake
// and later, Zen 4 -- 1 is the one that could pay properly: same instruction
// count, no branch, ~1.3 lanes of work instead of 16.
#ifndef RC_SCATTER_SKIP
#define RC_SCATTER_SKIP 0
#endif
#if RC_SCATTER_SKIP<0 || RC_SCATTER_SKIP>1
#error "RC_SCATTER_SKIP is 0 or 1: the branch variants and the no-commit probe are gone"
#endif


// How rc_Renorm's f_DEC==0 arm picks the renorm shift. It has always been
// `n = (range<sTOP) + (range<gTOP)`, a byte count in a local.
//
//   0  that                                     <- defs_avx2
//   1  the SHIFT COUNT from a nested ternary, in a const local  <- defs_avx512
//   2  the same, in lane state (dsh) instead of a local
//
// The encoder is not the decoder: its sweep is vector code that clang
// auto-vectorises, so a scalar local there is one vector register, not sixteen
// scalar ones, and the register-pressure argument behind the decoder's shape
// does not apply. But the SELECTION is worth something anyway. Twins on both
// arms, 15 round-robin rounds of best-of-3, 100 MB of enwik8:
//
//   NSEL=0   91.82  92.37   mean 92.10
//   NSEL=1   93.97  94.51   mean 94.24    +2.3%
//
// Twins 0.6% apart and the arms do not overlap, so that is about four times
// the noise floor. It reads bigger stacked on the rest of defs_avx512 (+7.1%
// there), which is why the two profiles disagree and this stayed a knob. The
// mechanism is thin -- {0,8,16} from two blends against {0,1,2} from two
// compares and a vpslld -- so it is not obvious it survives a different box.
//
// misc/dec_renorm.md is where the decoder half of this was measured.
#ifndef RC_ENC_NSEL
#define RC_ENC_NSEL 0
#endif

// The rest of the encoder's renorm: what ShiftLow is handed and what it
// declares. At RC_ENC_NSEL>=1 the shift count is selected and then handed to
// ShiftLow as `dsh>>3`, whose first statement was `uint sh = n*8` -- selected,
// divided by eight, passed, multiplied by eight again, on the chain.
//
//   0  ShiftLow takes the BYTE count and derives the shift, plus locals for
//      cl and _hi                                            <- defs_avx2
//   1  ShiftLow takes the SHIFT COUNT and derives the byte count for tmpptr,
//      the only thing that wants it. No round trip
//   2  1, and cl and _hi written out where they were read, so the body
//      declares nothing at all                               <- defs_avx512
//
// Measured, 15 rounds, twins, against RC_ENC_NSEL=0's 92.10 baseline:
//
//   RC_ENC_RENORM  0      1      2
//   RC_ENC_NSEL=1  92.29  91.58  91.88   (2/2 twins 92.05/91.71)
//
// All within a noise floor of each other: the round trip and the locals are
// both free, because clang folds (x>>3)*8 back to a mask and the locals are
// vector registers in the sweep. The knob stays because it makes the encoder's
// renorm say what it means, and because it is the control proving the ternary
// in RC_ENC_NSEL is the whole effect -- not because it is faster.
//
// stn[] takes the byte count only as a store-is-dead flag and model.inc only
// compares it against zero, so handing it the shift instead is the same
// predicate with no fixup.
//
// 2 leaves the !RC_FOLD_RPRE carry block alone -- it declares locals of its
// own and is not the default shape.
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
#if RC_LOWBYTES!=8
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


