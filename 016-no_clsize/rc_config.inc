
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
// Every knob that reaches the GENERATOR belongs here, and since build.sh and
// gc.bat hand mk_kernel.sh the compile's own -march that includes the ones
// derived from the target: RC_SCATTER and RC_SCATTER_W resolve against
// __AVX512F__ and friends while rc.inc is preprocessed, exactly as they do
// in the compile, and a kernel generated for one target and compiled for
// another fails this assert instead of miscommitting. RC_VECOUT and
// RC_CARRYLESS are excluded -- mk_kernel.sh forces them, and the compile sets
// RC_CARRYLESS both ways.
#define RC_KERNEL_CONF ( RC_RCNUM + 64*RC_LOWBYTES                        \
                       + 8192*RC_CODBYTES                                 \
                       + 1048576*RC_VECSIZE + RC_BLKSIZE/16               \
                       + 4096*(RC_FF_TRIM&63)                             \
                       + 33554432*RC_FOLD_RPRE                            \
                       + 13*RC_ENC_NSEL                                   \
                       + 17*RC_ENC_RENORM                                 \
                       + 23*RC_DEC_VRENORM                                \
                       + 29*RC_DEC_VRENORM_VF + 31*RC_DEC_VRENORM_SLOTS   \
                       + 37*RC_DEC_VRENORM_TAIL + 41*RC_DEC_VRENORM_GATHER \
                       + 43*RC_DEC_ZSLOT + 47*RC_DEC_VRENORM_PROBE         \
                       + 53*RC_SCATTER + 59*RC_SCATTER_W                  \
                       + 61*RC_SCATTER_SKIP + 67*RC_DEC_RENORM_ZS )

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
// stcl/stad and rc_FlushAll in rc.inc, and the group loop in model0.inc.
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

// Lanes per rc_FlushAll call: a scatter's width, or the whole group where
// the commit is the scalar block (and rc_FlushAll is nothing at all when the
// build is not staging).
#define RC_FLUSH_LANES ( RC_SCATTER_W ? RC_SCATTER_W : RC_RCNUM )

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
#define RC_ENC_CL   RC_LOW_CL   // the width cascade, in rc.inc
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


// The decoder's renorm as a BRANCHLESS VECTOR PASS over a run of lanes,
// instead of a branch at the head of every rc_Process.
//
//   0   the one-branch refill in rc.inc: `if( range<sTOP )` per lane per bit,
//       ~92% not taken, and the refill's masked load only on the taken arm
//   W   no renorm inside rc_Process at all. A lane is renormalised AFTER its
//       bit, W lanes at a time, in a loop the compiler auto-vectorises: two
//       compares for the shift count, the cursor decrement, the two shifts,
//       and the bytes shifted in taken from a per-lane cached window dword
//       (misc/dec_renorm.md's shape 9, lifted into lane state) -- no
//       branch, no memory but the lane arrays. Then the lanes that shifted,
//       ~1.2 of 16 a group, reload their window in scalar code off a bit
//       mask: RC_DEC_VRENORM_SLOTS of them unconditionally, a loop for the
//       rest. See rc_RenormRun in rc.inc, and misc/vrenorm.md for how it
//       got there.
//
// Why a pass and not the branchless shapes misc/dec_renorm.md buried: those
// put the always-executed shifts and the refill load on each lane's own chain
// at the head of its step, sixteen times a group, in scalar code. The pass
// takes them off the step entirely -- a lane's next use is a whole group
// away -- and does them eight lanes to an instruction. dec_vectorize.md saw
// the same thing from the other side: inside a clean whole-group renorm
// pass, branchless beat every branchy shape by 12%. It just could not carry
// the barriers of the four-pass split around it. Here the model stays
// interleaved; only the renorm moves.
//
// Measured, paired rounds, 100 MB of enwik8, clang 18, -march=native on
// the Cascade Lake VM, medians of six best-of-3 decode passes, twins of the
// baseline 0.8% apart:
//
//   0                                   45.58 MB/s
//   16, the pass alone (probe 3)        48.24     +5.8%    the ceiling
//   16, pass + mask + 2 slots, no loop  46.34     +1.7%    (probe 6)
//   16, the whole thing, SLOTS=2        43.14     -5.4%
//   16, SLOTS=3                         44.23     -3.3%    (second run, base 45.75)
//   16, SLOTS=4                         43.61     -4.7%
//   16, SLOTS=4 again                   44.47     -0.5%    (third run, base 44.70)
//   16, SLOTS=4, TAIL=1                 44.15     -1.2%
//   8,  two passes a group              40.33    -11.5%
//   16, gather shape (GATHER=1)         33.26    -27.0%
//
// So the pass is real and the refill eats it: the mask and the slots that
// keep the stream honest cost what the pass bought, and at SLOTS=4 the
// build sits within about a percent of 0 -- the noise floor -- with the
// tail's shape not mattering. The default stays 0; RC_DEC_VRENORM_SLOTS
// and RC_DEC_VRENORM_TAIL below are the levers, misc/vrenorm.md has every
// row and what would change the answer (a core with cheap gathers, a
// wider window, the AVX2 box).
//
// W is the number of lanes a pass covers, and it fixes WHERE the passes go.
// The group form of the decode loop (model1.inc) steps the NB=RCNUM/8 bytes
// of a group together, one bit position at a time, so after step j the lanes
// of bit positions <=j are done for this group. With the lane arrays laid
// out bit-major (slot = j*NB + m, see RC_SLOT), those lanes are a contiguous
// run, and a pass of W lanes fires every W/NB steps over the run just
// finished. W=RCNUM is one pass at the group's end; W=RCNUM/2 is two passes,
// after steps 3 and 7 -- which measures worse: the fixed cost is per pass,
// and the slack it buys was never the problem.
//
// The kernel changes with it: rc_Process's decoder arm stops calling
// rc_Renorm and applies rpre to code where it is produced (`code -= rpre`
// with rpre already masked), and rc_Renorm's decoder arm -- still used, for
// the block-length bits and the tail -- drops its rpre fold. The stream does
// not change; t_matrix.sh checks it against the reference.
//
// Constraints: W must divide RCNUM, be a multiple of NB=RCNUM/8, and W/NB
// must divide 8. At RCNUM=16 that is 2, 4, 8 or 16.
#ifndef RC_DEC_VRENORM
#define RC_DEC_VRENORM 0
#endif
#if RC_DEC_VRENORM<0 || RC_DEC_VRENORM>RC_RCNUM
#error "RC_DEC_VRENORM is 0 or a lane count up to RC_RCNUM"
#endif
#if RC_DEC_VRENORM && ( RC_RCNUM%RC_DEC_VRENORM || RC_DEC_VRENORM%(RC_RCNUM/8) \
                       || 8%(RC_DEC_VRENORM/(RC_RCNUM/8)) )
#error "RC_DEC_VRENORM must divide RC_RCNUM, be a multiple of RC_RCNUM/8, and cover 1, 2, 4 or 8 steps"
#endif
// ...and the steps between passes: a pass covers W lanes = W/NB bit positions
#define RC_DEC_VRENORM_STEPS ( RC_DEC_VRENORM/(RC_RCNUM/8) )

// Vector width for the pass's loop: the width the compiler is TOLD to use,
// through `#pragma clang loop vectorize_width`. 8 is a ymm of dwords, and
// clang's default on AVX-512 anyway (-mprefer-vector-width=256). 16 asks for
// a zmm gather where W allows it. Only meaningful with RC_DEC_VRENORM.
#ifndef RC_DEC_VRENORM_VF
# if RC_DEC_VRENORM>=8
#  define RC_DEC_VRENORM_VF 8
# else
#  define RC_DEC_VRENORM_VF RC_DEC_VRENORM
# endif
#endif

// 1 = the pass loads for EVERY lane, and the compiler turns the sixteen
// loads into vpgatherdd. It is the obvious shape, and on this box it is the
// worst thing in the whole experiment: a vpgatherdd ymm here costs 25 ticks
// of throughput in isolation (misc/gather_bench.cpp) -- the Downfall
// microcode mitigation serialises gathers on Skylake-generation cores, and
// this VM reports "Not affected" while timing exactly like one that is. Two
// of them a group is 59 cycles on a 139-cycle group. `-mno-gather`, which
// scalarises them into loads and vpinsrd, is port 5 for 23 cycles instead.
//
// 0 = the pass does everything but the load, and the lanes that took bytes
// (~1.2 of 16 a group) are refilled by scalar code off a bit mask. See
// rc_RefillRun in rc.inc. The default; 1 is for a core whose gathers are
// not serialised, where it is one instruction against a dozen.
//
// On AVX2 the gather is spelled out (rc_RenormV8): the loop vectoriser only
// forms one on a core LLVM marks fast-gather, Skylake and up, and a
// Haswell-tuned build -- -march=haswell, or native on one -- gets the
// sixteen loads scalarised into vpextrd/vpinsrd chunks instead, on clang 18
// and 23 alike. The intrinsic gathers wherever AVX2 is. The auto-vectorised
// loop is what remains for a pass narrower than eight lanes, and for
// targets without AVX2.
#ifndef RC_DEC_VRENORM_GATHER
#define RC_DEC_VRENORM_GATHER 0
#endif
#if RC_DEC_ZSLOT && !RC_DEC_VRENORM_GATHER
#error "RC_DEC_ZSLOT selects the gather's address: it needs RC_DEC_VRENORM_GATHER"
#endif

// How many lanes the pass's refill reloads UNCONDITIONALLY before it falls
// into a loop for the rest. A lane shifts on ~8% of its bits, so of 16 lanes
// ~1.3 shift a group: P(more than 2) is 13%, more than 3 is 3.3%, more than
// 4 is 0.6%. Each slot is six instructions with nothing to predict; the
// loop's exit is a data-dependent branch that costs a mispredict whenever
// the count crosses it. See misc/vrenorm.md for what each setting measures.
#ifndef RC_DEC_VRENORM_SLOTS
#define RC_DEC_VRENORM_SLOTS 4
#endif
#if RC_DEC_VRENORM_SLOTS<1 || RC_DEC_VRENORM_SLOTS>4
#error "RC_DEC_VRENORM_SLOTS is 1..4"
#endif

// What the refill does about the lanes past RC_DEC_VRENORM_SLOTS:
//   0  a loop over what is left of the mask
//   1  reload every window of the run, straight-line -- 3 instructions a
//      lane, W lanes, behind the one branch. No loop in the group body.
//   2  nothing: the probe that measures the slots alone. Garbage out.
// The rare path's cost is its probability times its length either way;
// what differs is what a loop does to the code around it. Measured in
// misc/vrenorm.md: at SLOTS=4 the two are within a percent of each other
// and of the baseline, and 1 never beats 0.
#ifndef RC_DEC_VRENORM_TAIL
#define RC_DEC_VRENORM_TAIL 0
#endif
#if RC_DEC_VRENORM_TAIL<0 || RC_DEC_VRENORM_TAIL>2
#error "RC_DEC_VRENORM_TAIL is 0, 1 or 2"
#endif

// 1 = Shelwien's branchless per-lane refill with the address-selected load
// (misc/vrenorm.md 6): rc_Renorm's decode arm, at the head of every step,
// selects the shift with a nested ternary and loads a dword from EITHER the
// row at the new cursor or the lane's own zero slot -- the address is
// selected, the load always happens, and the mask (0xFF/0xFFFF/0) is
// applied on top. Not the pass: the scalar shape, per bit. Only without
// RC_DEC_VRENORM. Measured here at 24.04 MB/s against 45.52, -47%, paired
// rounds -- misc/vrenorm.md 6 has why.
#ifndef RC_DEC_RENORM_ZS
#define RC_DEC_RENORM_ZS 0
#endif
#if RC_DEC_RENORM_ZS && RC_DEC_VRENORM
#error "RC_DEC_RENORM_ZS is the per-step refill; RC_DEC_VRENORM replaces it"
#endif

// Probes on the pass, for taking it apart -- all garbage out, the block
// structure survives and the work is the same. 0 is the pass.
//   1  no load from the row: the refill (or the gather shape's load) takes
//      the cursor for data
//   2  the pass twice, back to back; the second finds nothing to shift, so
//      what it adds is the cost of a pass with nothing to wait for
//   3  no refill at all
//   4  the lane mask alone, kept live
// With RC_DEC_VRENORM_TAIL=2 and RC_DEC_VRENORM_SLOTS the slots come next,
// and RC_DEC_VRENORM_VF=1 is one more: the same pass, scalar, unrolled.
#ifndef RC_DEC_VRENORM_PROBE
#define RC_DEC_VRENORM_PROBE 0
#endif
#if RC_DEC_VRENORM_PROBE<0 || RC_DEC_VRENORM_PROBE>4
#error "RC_DEC_VRENORM_PROBE is 0..4"
#endif

// 1 = the pass's gather does not read a lane that shifted nothing: its index
// is SELECTED to a dword of zeros kept next to the lane rows (RCio::zslot,
// rewritten -- and so brought into L1 -- by rc_Read at every block), so what
// comes back for an idle lane is 0 whatever the mask does. The scalar version
// of the trick is misc/cmov_probe.md's address-select. Only meaningful with
// RC_DEC_VRENORM_GATHER.
//
// 0 = the gather's own mask. In the spelled-out AVX2 body (rc_RenormV8) an
// idle lane is masked off and takes the zero source without touching memory
// -- the same effect in hardware, one blend cheaper. In the auto-vectorised
// loop every lane loads the dword at its cursor, which is always a legal
// read (the window is inside the row by RC_SKIP>=4, rc_io.inc) and for an
// idle lane a hot line, and the mask (1<<0)-1 == 0 throws it away; clang
// tends to fold that into a masked gather by itself. 1 is the shape asked
// for; 0 is the control, and measures the same.
#ifndef RC_DEC_ZSLOT
#define RC_DEC_ZSLOT 0
#endif
#if RC_DEC_ZSLOT && !RC_DEC_VRENORM
#error "RC_DEC_ZSLOT needs RC_DEC_VRENORM"
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
// How many of the substream length's 16 bits go through the header model's
// binary-tree context, top first; the rest are coded at p=1/2 with no counter
// at all. A length's low bits carry no structure a counter can find, and an
// adaptive counter on a random bit costs MORE than it saves -- at 8 bits of
// context, dropping the low byte's eight position counters for p=1/2 is worth
// 393 bytes on enwik8 by itself.
//
// The tree is per lane, so this is RCNUM<<this counters, and past 10 the
// deeper contexts see too few of a lane's 1526 lengths to train:
//
//   bits   enwik8       counters/lane   state
//      8   62,488,351          256       8 KB
//      9   62,487,641          512      16 KB
//     10   62,487,196         1024      32 KB     <- default
//     11   62,487,243         2048      64 KB
//     12   62,487,412         4096     128 KB
//     13   62,487,677         8192     256 KB
#ifndef RC_HDR_CTXBITS
#define RC_HDR_CTXBITS 10
#endif
#if RC_HDR_CTXBITS<1 || RC_HDR_CTXBITS>16
#error "RC_HDR_CTXBITS must be 1..16"
#endif

#ifndef RC_FF_TRIM
#define RC_FF_TRIM 32
#endif
#ifndef RC_FF_PADSIZE
#define RC_FF_PADSIZE (RC_FF_TRIM + 2*RC_LOWBYTES + 16)
#endif


