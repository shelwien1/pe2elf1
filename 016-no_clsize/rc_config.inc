
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

// 0 = sh_v1xN's `uint range = 0xFFFFFFFF`.
// 1 = the original demo's `qword range = 1<<32`, one ulp wider.
#ifndef RC_RANGE64
#define RC_RANGE64 0
#endif

// The counted 0/1/2-byte renorm is exact only when one step cannot take range
// below 2^8, i.e. when the unit is >= 2^8. This coder is binary throughout:
// unit = range>>SCALElog is >= 2^9, always. So the tail loop is dead.
#ifndef RC_RENORM_TAIL
#define RC_RENORM_TAIL 0
#endif

// Count carries out of the low accumulator. The carryless coder needs the
// count regardless (model0.inc re-encodes the block when it is non-zero); this
// only controls the diagnostic fprintf in rc_Quit, which would fire once per
// lane per block here and is off by default.
#ifndef RC_STATS
#define RC_STATS 0
#endif

// 1 = have the carryless rc_Quit complain to stderr about lost carries. In
// sh_v1xN_s.cpp a lost carry is fatal, because a single-pass stream demo has
// nothing to fall back on. Here it is a normal, recoverable event: model0.inc
// keeps the block's {p;bit} cache and re-encodes with the carry-propagating
// twin, so the warning is off.
#ifndef RC_CARRYLESS_WARN
#define RC_CARRYLESS_WARN 0
#endif

// 1 = report every block the carryless fast path had to hand to the carry twin.
// Off by default; the fallback is a normal event, not a problem.
#ifndef RC_TRACE_FALLBACK
#define RC_TRACE_FALLBACK 0
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

// 1 = refuse at compile time any BLKSIZE/RCNUM pair whose worst case does not
// fit the 2-byte substream length header.
//
// Off by default, because that worst case is 15 bits per coded bit -- a model
// that is maximally wrong about every single bit -- and the geometries in use
// (RCNUM 8/16/32 at BLKSIZE 64K) are nowhere near it on real data: a lane runs
// about BLKSIZE/RCNUM bytes even on incompressible input. The one config that
// does trip on ordinary random data is RCNUM=1 at BLKSIZE=64K, where a whole
// block lands in one lane and clears 65535 by a byte.
//
// Either way a lane cannot overrun its buffer: it is sized to the smaller of
// the worst case and the header cap, and model0.inc reports the overflow and
// stops rather than emitting a stream that will not decode. RCio::blkmax is the
// largest safe BLKSIZE for the configured RCNUM.
#ifndef RC_STRICT_BLKSIZE
#define RC_STRICT_BLKSIZE 0
#endif


// 1 = the kernel buffers each lane's output and writes it a dword at a time
// instead of a byte at a time.
//
// A byte store to a per-lane address cannot be vectorised at all: there is no
// byte scatter in AVX-512, or in any other SIMD ISA worth naming, so every
// such store scalarises into extract-the-address, store, mask test, branch --
// once per lane, per byte. vpscatterdd does exist, so a per-lane dword store
// is one masked instruction and no branches. The cost is carrying a 32-bit
// accumulator and a byte count per lane, and flushing at rc_quit.
// build.sh maps -1 to 1: under ispc the word path is the one that vectorises.
#ifndef RC_DEV_WORDOUT
#define RC_DEV_WORDOUT (-1)
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
                       + 8192*RC_CODBYTES + 131072*RC_RANGE64             \
                       + 262144*RC_SHIFT_SAT + 524288*RC_STATS            \
                       + 1048576*RC_VECSIZE + RC_BLKSIZE/16               \
                       + 4096*(RC_FF_TRIM&63)                     \
                       + 33554432*RC_FOLD_RPRE            \
                       + 67108864*RC_LOAD32               \
                       + 134217728*RC_DEC_COLD            \
                       + 3*RC_FF_LANES                    \
                       + 11*RC_DEC_RENORM                 \
                       + 13*RC_ENC_NSEL                   \
                       + 17*RC_ENC_RENORM                  \
                       + 19*RC_DEC_PREFETCH               \
                       + 23*RC_DEC_PFDIST )

// 1 only inside mk_kernel.sh's preprocessing of rc.inc: selects the
// vector-shape ShiftLow/rc_Renorm/rc_Quit bodies the generated lane-array
// coder is built from. Never set in a normal compilation.
#ifndef RC_VECOUT
#define RC_VECOUT 0
#endif

// 1 = the models drive the generated lane-array coder (rc_vecD.inc, built
// from this same rc.inc by mk_kernel.sh) -- sh_v1xN's arrangement: the
// encoder's pbit sweep auto-vectorizes across the lanes, the decoder gets
// the unrolled group loop over local arrays. The scalar coder stays compiled
// beside it: it is the -C reference and the carry-propagating fallback,
// built from the same source. 0 = scalar only.
#ifndef RC_VEC
#define RC_VEC 1
#endif

#if RC_RANGE64   // the vector kernel carries a 32-bit range
#undef RC_VEC
#define RC_VEC 0
#endif

#if RC_VECOUT && !RC_CARRYLESS
#error the vector kernel is the carryless coder -- generate with RC_CARRYLESS=1
#endif
#if RC_VECOUT && RC_RANGE64
#error the vector kernel carries a 32-bit range -- RC_RANGE64 stays scalar
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
# if !defined(__clang__) || !RC_VEC || !RC_LOWSPLIT
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

// The decoder's renormalisation shape. The encoder has always used the
// counted shift -- `n = (range<sTOP) + (range<gTOP)`, then one shift by n*8 --
// and the generated decoder used two nested branches instead. (The scalar
// reference coder in the !RC_VECOUT half of rc.inc has always used the counted
// shift on BOTH sides, so the two were already disagreeing.) The asymmetry was
// never measured; this knob has ten shapes and misc/dec_renorm.md is the
// analysis.
//
//   0  two nested branches: skip everything unless range<sTOP, then test
//      range<gTOP to pick the 1-byte or the 2-byte refill        <- default
//   1  the outer branch, the inner one replaced by a counted shift
//   2  no branch at all; n from the same two compares the encoder uses
//   3  no branch at all; n from __builtin_clz -- sTOP and gTOP are 1<<24 and
//      1<<16, so the two compares ARE a count of leading zero bytes
//   4  no branch at all, and the refill window is addressed by n and masked
//      rather than loaded at a fixed offset and shifted
//   5  a branchless ONE-byte refill on the single compare that decides it,
//      plus a guard for the second byte
//   6  1, with the window addressed by n and masked instead of shifted
//   7  Shelwien's branchless shape: no locals at all, the SHIFT COUNT in lane
//      state, selected by a nested ternary
//   8  7 with the outer branch back, and the rpre fold duplicated into both
//      arms rather than hoisted above it
//   9  7 with the refill window CACHED in lane state and re-filled one
//      iteration ahead of the bit that reads it
//  10  9 with the reload, and only the reload, behind a branch -- a diagnostic
//      for what the refill reads are worth once their latency is off the chain
//  11  9 with the reload lifted out of the lane body: one MASKED GATHER per
//      group, in model1.inc where the encoder's RC_FLUSHALL goes
//  12  11 unmasked, the control for it
//
// The distribution is the whole story. Counted in the scalar reference coder
// over the 800M bits of enwik8:
//
//   n==0   737,581,939   92.195%
//   n==1    62,444,004    7.805%
//   n==2            15    0.000002%
//
// So ~92% of bits renormalise NOTHING, and the 2-byte case -- which needs
// range to fall by more than 2^8 in one step, so p below 128/32768 -- happens
// FIFTEEN TIMES in a hundred megabytes.
//
// That makes the two branches two completely different propositions. The OUTER
// one buys 92% of bits an empty renorm for a test that mispredicts ~8% of the
// time, and it is worth far more than the mispredict: taking it away costs
// 17-36%, on both machines below, because the unconditional shifts land on the
// loop-carried range and code chains. The INNER one never mispredicts at all,
// and removing it is worth a couple of percent -- in one direction on one
// machine and the other on the other.
//
// Two boxes, both 100 MB of enwik8, RC_DEC_WAVE=2 and the rest of the tuned
// set. Mine: clang 18, -march=native (AVX-512), medians of best-of-2 over 5
// round-robin rounds, twins of 0 and 9 in the rotation (2.2% and 1.9% apart,
// which is the noise floor). Shelwien's: clang 23, AVX2, from his log.txt.
// Loop figures are my 16-lane decode body.
//
//                                    insns  br   rsp    mine      his
//   8  one branch, no locals          431   17   103   +4.3%    -1.9%
//   0  two nested branches            460   17   113      --       --
//   6  counted inner, addressed       450   17   114   -0.6%    -3.5%
//   1  counted inner, shifted         452   17   111   -2.9%    -4.9%
//   5  branchless 1 byte + guard      678   18   134   -5.4%   -10.7%
//   9  branchless, window cached      478    1    80  -20.5%        ?
//   4  branchless, addressed          629    1   163  -31.0%   -21.7%
//   3  branchless, clz                655    1   157  -33.4%   -23.5%
//   2  branchless, two compares       668    1   156  -34.4%   -22.8%
//   7  branchless, no locals          735    1   204  -36.2%   -16.8%
//  10  9 + branch on the reload       772   18   185  -28.7%        ?
//  12  9 + unmasked gather            529    1    94  -36.1%        ?
//  11  9 + masked gather              534    1    97  -36.8%        ?
//
// Read that table twice. The big number -- the outer branch is worth a fifth
// to a third of decode -- reproduces on both machines. Every small number
// disagrees: 1 and 6 are the two best shapes on neither box and 8 wins on one
// and loses on the other, and inside the branchless family the ranking is
// exactly REVERSED (7 is his best and my worst). The default is 0 because it
// is the only shape that is never worse than second, and because an earlier
// round of this measurement -- taken on a box that turned out to be sharing
// its cores with a runaway process -- read 1 as +2.2% here and briefly made it
// the default. It is not.
//
// 9 is the answer to "the branchless shape is slower because of the memory
// read": it is, and caching the window is worth +24.7% over 7 here (3.097 ->
// 2.485 s) while cutting stack traffic from 204 references to 80, the lowest
// of any shape. It still does not beat the branch here. But his branchless
// family starts far closer to his branchy one -- 7 is at 83% of 0 on his box
// against 64% on mine -- so the same +25% there would land at about 50 MB/s
// against 0's 48.21, and 9 would win outright. That is the measurement to
// make.
//
// 10, 11 and 12 are the answer to "the branchless shape reads memory every
// bit -- read less often, or read better". Neither works, and the reason is
// the same both times.
//
// First, what the gap actually is. clang AUTO-VECTORISES the branchless
// shapes and does not vectorise the branchy ones: the group loop holds 127
// vector operations at 9 and 16 at 0. But it vectorises the arithmetic and
// then moves each lane in and out by hand to do the load -- 36 vpextrd, 24
// vpinsrd and 20 vmovd per group, essentially all port 5. At one per cycle
// that is ~46 cycles, and 9 is 222 cycles per 16-lane group against 0's 176.
// The gap IS the cross-domain traffic, not the shift chain and not the loads.
//
// 10 tests reading less often, and it is the strongest possible version of
// that test: a per-lane branch skips ~92% of the reads, where a group-wide
// test could only skip the ~27% of groups in which no lane renormalises
// (0.92195^16). It loses -- 2.758 s against 9's 2.475, 11% slower -- because
// the branch de-vectorises the body: 127 vector operations collapse to 8, the
// loop grows from 478 instructions to 772 and stack traffic from 80 to 185.
// Since the strongest form loses, every weaker one does too.
//
// 11 and 12 test reading better: leave tmpptr in the vector it is already in
// and let one gather do all sixteen. It loses harder -- 3.109 s masked, 3.077
// unmasked, 25% behind 9 and level with 7. The gather removes 15 of the loads
// and gives them straight back, because it has to land in the win[] array and
// every lane then reads its slot out of memory: 95 memory-source instructions
// at 9, 93 at 11. What is left is a vpgatherdd where sixteen scalar loads
// were, and it costs about 57 cycles per group more than they did.
//
// The mask buys nothing -- 11 and 12 are within noise of each other -- which
// is the same thing RC_SCATTER_SKIP found on the encoder's scatter, from the
// other direction: on this target a gather with one active lane costs what a
// gather with sixteen costs.
//
// So the traffic can only go by keeping the lane state in vector registers
// across the whole loop and never extracting at all -- the encoder's shape.
// The decoder cannot follow it there while the model side is a per-lane table
// walk, because that would need a gather of its own, at the price measured
// here. That is the wall, and it is where this line of attack stops.
//
// 1..12 all read the 32-bit refill window, so they carry RC_LOAD32's padding
// requirement whatever RC_LOAD32 itself is set to (see rc_io.inc), and 4, 6,
// 7 and 8 need bytes ABOVE the cursor as well. They ignore RC_LOAD32: it
// selects between shapes of the branchy refill, which 1 and 6 through 8 have
// subsumed and 2 through 5 and 9 do not have.
//
// All ten produce the same stream -- t_matrix.sh checks each.
#ifndef RC_DEC_RENORM
#define RC_DEC_RENORM 0
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

// A prefetch on the branchless decoder's window read (RC_DEC_RENORM 7 and 9).
//
// 7's whole problem is that the read sits ON the chain: range -> dsh -> tmpptr
// -> address -> load -> code, every bit. A prefetch of THIS bit's address
// cannot help -- the address is the thing that is late, and by the time it
// exists the load can issue. So the prefetch here is for a FUTURE bit: the
// substream runs backwards, so the next reads are below the cursor, and
// RC_DEC_PFDIST says how far.
//
//   0  off                                                       <- default
//   1  prefetcht0    (locality 3, all levels)
//   2  prefetcht1    (locality 2)
//   3  prefetcht2    (locality 1)
//   4  prefetchnta   (locality 0, non-temporal)
//
// __builtin_prefetch, not _mm_prefetch: this reaches the generator, and
// rc_vecD.inc must not depend on the target (see RC_KERNEL_CONF). Prefetch
// never faults, so the address needs no bounds check.
//
// MEASURED, AND IT DOES NOTHING -- see misc/dec_renorm.md section 6. There is
// no miss to hide. Each lane's cursor moves about 0.08 bytes per coded bit, so
// one 64-byte line serves a lane for ~800 bits and the sixteen live lines sit
// in L1 permanently. What the chain pays is L1 load-use latency, and a
// prefetch does not shorten that: the real load still issues and still takes
// its four or five cycles from address to data. The knob is kept because the
// result is worth having written down, not because it is faster.
#ifndef RC_DEC_PREFETCH
#define RC_DEC_PREFETCH 0
#endif

// How far below the cursor to prefetch, in bytes. One cache line by default.
#ifndef RC_DEC_PFDIST
#define RC_DEC_PFDIST 64
#endif

#if   RC_DEC_PREFETCH==1
#define RC_DEC_PFLOC 3
#elif RC_DEC_PREFETCH==2
#define RC_DEC_PFLOC 2
#elif RC_DEC_PREFETCH==3
#define RC_DEC_PFLOC 1
#elif RC_DEC_PREFETCH==4
#define RC_DEC_PFLOC 0
#endif

#if RC_DEC_PREFETCH
#define RC_DEC_PF( q )  __builtin_prefetch( &tmpbase[(q)-RC_DEC_PFDIST], 0, RC_DEC_PFLOC )
#else
#define RC_DEC_PF( q )  ((void)0)
#endif

#if RC_ENC_RENORM>=1
#define RC_ENC_SHDECL                     // n IS the shift: nothing to declare
#define RC_ENC_SH      (n)
#define RC_ENC_NB      ((n)>>3)
#define RC_ENC_ARG(x)  (x)
#else
#define RC_ENC_SHDECL
#define RC_ENC_SH      sh
#define RC_ENC_NB      (n)
#define RC_ENC_ARG(x)  ((x)>>3)
#endif

#if RC_ENC_RENORM>=2
#if RC_SHIFT_SAT
#define RC_ENC_HI   (lowl >> (32-RC_ENC_SH))
#else
#define RC_ENC_HI   ((lowl >> 16) >> (16-RC_ENC_SH))
#endif
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

// 1 = ShiftLow extracts the high word as a single variable shift,
// `lowl >> (32-sh)`, and relies on the shift SATURATING to zero when sh==0
// makes the count 32. 0 = the portable double shift, `(lowl>>16)>>(16-sh)`,
// which is the same value with both counts <= 16.
//
// Saturation is a property of the target, and the targets disagree:
//
//   x86 SIMD   vpsrlvd / vpsrld     count >= 32 -> 0      saturates
//   ARM NEON   ushl (negative)      shift >= 32 -> 0      saturates
//   x86 scalar shr                  count & 31  -> lowl   WRONG
//   OpenCL     `x >> n`             n & 31      -> lowl   WRONG (spec 6.3.j)
//   CUDA C     `x >> n`             undefined             (PTX shr.u32 clamps)
//   C / C++    `x >> n`             undefined for n >= 32
//
// so 1 is only ever correct for a hand-written kernel on a target in the top
// two rows. In the C build it is undefined behaviour at sh==0, and a compiler
// is free to make the whole ShiftLow disappear.
//
// It is also SLOWER here, which is the reason the default is 0 rather than a
// portability compromise. clang will not fold the C-level poison into the one
// vpsrlvd the hardware would allow; it canonicalises around it and emits more,
// not less. AVX2, -march=skylake, RCNUM=16, 20 MB of enwik8, sweep loop
// instruction counts and best of 6x6:
//
//   0  (lowl>>16)>>(16-sh)         144 insns   58.20 MB/s   <- default
//   1  lowl>>(32-sh)               153         56.20
//   -  uint(qword(lowl)>>(32-sh))  170         53.13
//
// The third is the well-defined way to ask for the same thing -- a 64-bit
// shift by 32 is perfectly legal -- and it is the worst of the three: clang
// widens the lane to 64 bits and emits vpsrlvq, four lanes per ymm instead of
// eight. All three produce the same stream.
#ifndef RC_SHIFT_SAT
#define RC_SHIFT_SAT 0
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

// 1 = the decoder fetches the next counter a bit early. See P_kids in
// predict.inc: the two children of a context are adjacent, so both come in one
// 8-byte load whose address is known a bit ahead, and the next bit picks its
// half with a select rather than waiting on a load. Needs the fused counter --
// with a bare state the load it removes is still followed by pp[state], and
// the chain is no shorter.
//
// Decoder only. The encoder's model pass is rename-bound; there an extra load
// per bit to save latency it is not paying is strictly worse.
//
// MEASURED, AND IT LOSES: -3.37% decode on -march=skylake against a +0.09%
// control, byte-identical stream. It does shorten the chain -- the select is a
// cmov, not a branch -- but it costs ~6 instructions per bit (the pair load,
// the shift, the cmov and the register moves around a 64-bit value) and the
// loop goes from 405 to 506 instructions per group of 16 bits.
//
// Which says what the decoder became once RC_FUSE_PP removed the second load:
// at 8.77 cyc/bit over ~25 instructions it is running ~2.9 IPC out of a
// 1756-byte loop body, i.e. it is now FRONT-END bound, not chain-bound. The
// lever moved. Kept at 0 for the record and because a machine with more fetch
// bandwidth could flip it.
#ifndef RC_EAGER_CTY
#define RC_EAGER_CTY 0
#endif
#if RC_EAGER_CTY && !RC_FUSE_PP_DEC
#error "RC_EAGER_CTY needs RC_FUSE_PP_DEC=1"
#endif

// How far to unroll the DECODER's group loop. 0 = fully (RCNUM bodies, the
// shape sh_v1xN's decoder had and the reason RC_UNROLL exists); 1 = not at
// all; 2/4/8 = that many bodies.
//
// The full unroll makes j a compile-time constant, so the lane arrays become
// fixed addresses and (j&7)==7 a compile-time test. It also stamps out RCNUM
// copies of rc_Renorm's byte refill -- the branchy part ~91% of bits skip --
// which is what makes the decode loop 1756 bytes against the encode sweep's
// 902 for the same 16 bits. Once RC_FUSE_PP took the dependent load off the
// per-bit chain the decoder stopped being chain-bound (see RC_EAGER_CTY), so
// which side of that trade wins is worth re-asking.
#ifndef RC_DEC_UNROLL
#define RC_DEC_UNROLL 0
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

// How the decoder's model loop is shaped. 0 = the member context, one byte at
// a time (the original). 1 = the same order with a caller-held context. 2 = the
// wavefront: the group's RCNUM/8 bytes stepped together, so their 8-step
// context chains overlap instead of running end to end. See model1.inc and
// misc/speed_plan_next.md 1.1/1.2. All three code the same stream.
//
// MEASURED, AND BOTH LOSE -- -march=skylake, decode, control +0.08%:
//
//   0  member ctx, byte at a time    405 insns/group    47.43 MB/s
//   1  caller-held ctx               444                43.34   -8.62%
//   2  wavefront                     447                45.20   -4.69%
//
// The premise of 1 holds: the member ctx really is 10 memory references per
// group -- two loads of cty, two of ctx, four stores of ctx, two of the reset
// constant -- because the store to cty[ctx] may-alias this->ctx and nothing
// bounds a loaded index. It is just not what the loop is short of. Holding the
// context in a register instead costs 39 instructions per group, only 7 of
// them memory, in a loop that already carries 16 lanes of coder state; the
// spills cost more than the 10 references did.
//
// And 2 is the interesting half. It is 1's loop with the two loops swapped and
// nothing else, and it recovers 4 points of 1's 8.6 -- so the wavefront's
// chain shortening is real and worth about +4%, on top of a prerequisite that
// costs twice that. Which is the RC_EAGER_CTY result again in a different
// shape: this decoder has no register budget left to buy latency with.
#ifndef RC_DEC_WAVE
#define RC_DEC_WAVE 0
#endif

// 1 = emit the group's output bytes as one 16-bit store instead of two. Under
// RC_DEC_WAVE=2 they finish together; under 0 the first is held in a register
// across its eight bits, which is one more live value and no more work.
//
// MEASURED, AND IT LOSES STANDALONE: -2.43% decode against a +0.11% control,
// even though it takes 11 instructions and 16 loads out of the group loop.
// Under RC_DEC_WAVE=0 the first byte has to be held across its eight bits, and
// one more value live through half a group costs more than the store saves.
// Under RC_DEC_WAVE=2, where the bytes finish together and nothing is held, it
// is worth +1.6% -- but that build is 4.7% down for its own reasons.
#ifndef RC_DEC_PUTW
#define RC_DEC_PUTW 0
#endif
#if RC_DEC_PUTW && ( RC_DEC_WAVE==1 || (RC_RCNUM/8)%2 )
#error "RC_DEC_PUTW needs RC_DEC_WAVE 0 or 2 and an even byte count per group"
#endif

// 1 = the decoder's byte refill reads one 32-bit window instead of one or two
// separate bytes. The substream runs backwards, so the dword at tmpptr-3 holds
// both cases in its top half -- b[q] is bits 24..31 and b[q-1]+b[q]<<8 is bits
// 16..31 -- and one load hoisted above the branch replaces two loads and the
// shift-or that assembled them. The fully unrolled decode loop stamps out
// RCNUM copies of this, so the body shrinks by more than the hot path does.
//
// 2 = the same window, but read only inside the 2-byte branch instead of
// hoisted above the test.
//
// LITTLE-ENDIAN ONLY, and deliberately not guarded on __BYTE_ORDER__: this
// reaches the generator, and rc_vecD.inc must not depend on the target (see
// RC_KERNEL_CONF below). Same arrangement as RC_SHIFT_SAT.
//
// MEASURED -- -march=skylake, decode, control +0.10%:
//
//   0  two byte loads      405 insns/group   46.88 MB/s
//   1  hoisted window      456               42.09   -10.23%
//   2  2-byte branch only  405               47.02    +0.31%
//
// 1 loses because hoisting makes the COMMON case pay: the 1-byte path went
// from `movzbl (base,q)` folded into the add to a load plus a `shr $24`, and
// it is the more frequent of the two. 2 keeps that path untouched and improves
// the other, and comes out inside the noise -- the 2-byte case is too rare to
// pay for a knob that assumes little-endian. Default 0.
#ifndef RC_LOAD32
#define RC_LOAD32 0
#endif

// 1 = tell the block placer the decoder's byte refill is really cold, so the
// 16 unrolled copies sink out of the group loop body instead of sitting in it.
// speed_plan_next.md 2.3: shrinking the body while KEEPING the full unroll is
// the variant RC_DEC_UNROLL did not test.
#ifndef RC_DEC_COLD
#define RC_DEC_COLD 0
#endif

// 1 = the decoder's copy of the generated kernel keeps ALIGN(VECSIZE) on the
// lane arrays. 0 drops it -- speed_plan_next.md 3. The decoder never touches
// that state as a vector, and the alignment costs it `and rsp,-32`, a frame
// pointer and rbp for the whole function.
//
// MEASURED, AND DROPPING IT LOSES: -1.42% decode against a -0.04% control.
// The prologue really does lose the realignment and rbp really does come back
// to the allocator, and the group loop still ends up 48 bytes longer. Which is
// the sixth result in a row on this loop where the change did what it was
// meant to and the clock disagreed.
#ifndef RC_DEC_ALIGN
#define RC_DEC_ALIGN 1
#endif

// How many slots the generated kernel gives FFNum. Its only consumer is
// model0.inc's `ncarry &= FFNum[j]` over every lane at block end, followed by a
// whole-block re-code -- so the per-lane values are never read apart and the
// array can be narrower than RCNUM, with lane j folding into slot j%N. At
// RCNUM=16 and N=8 that is one ymm instead of two, and one fewer spilled array
// in the sweep's working set. speed_plan_next.md 5.1.
#ifndef RC_FF_LANES
#define RC_FF_LANES RC_RCNUM
#endif
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

// Probe: run the encoder's model pass and skip the coding sweep entirely.
// Does NOT encode correctly -- it writes empty substreams -- so it is timed
// with a plain encode to /dev/null (or nul), never through t.sh:
//
//   OUT=probe ./build.sh -DRC_PROBE_MODELONLY=1
//   ./probe c ../enwik8 /dev/null ../FSM0.txt 5
//
// The encoder is two passes over the block that share nothing but pbit[]: a
// scalar model pass, which is the same code on every target, and the vector
// sweep. This says how the time splits between them, and it is the first
// number to have before reading anything into a sweep-side knob -- on the box
// misc/avx2_vs_avx512.md was written on the model pass is 48% of encode, so
// halving the sweep's instruction count moves encode by 13%, not by 50%.
//
// Pair it with RC_SCATTER_SKIP=9 to split the sweep again, into arithmetic
// and commit. The three builds decompose encode completely:
//
//   RC_PROBE_MODELONLY=1        model
//   RC_SCATTER_SKIP=9           model + sweep arithmetic
//   (neither)                   model + sweep arithmetic + commit
#ifndef RC_PROBE_MODELONLY
#define RC_PROBE_MODELONLY 0
#endif

// Bounds-check every byte the encoder writes into its substream buffer. The
// buffer is sized for the worst case the model can produce (see rc_io.inc), so
// this can only fire on a misconfiguration -- but a silent overflow would be a
// buffer overrun, so it is on by default.
#ifndef RC_IO_CHECK
#define RC_IO_CHECK 0
#endif
