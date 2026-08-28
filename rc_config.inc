
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
#define RC_LOWSPLIT 0
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

// Bounds-check every byte the encoder writes into its substream buffer. The
// buffer is sized for the worst case the model can produce (see rc_io.inc), so
// this can only fire on a misconfiguration -- but a silent overflow would be a
// buffer overrun, so it is on by default.
#ifndef RC_IO_CHECK
#define RC_IO_CHECK 1
#endif
