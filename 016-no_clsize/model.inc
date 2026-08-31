
#include "misc/intrin.inc"

#define EXTERN extern
#include "FSM.cpp"

#include "rc_config.inc"
#include "rc_io.inc"

// The scalar coders. Whichever file RC_RANS selects is the same one
// mk_kernel.sh generates the vector kernel from, so the scalar class and the
// generated one are two shapes of one source -- which is what makes RC_VEC=0
// a reference rather than a second implementation. The class name comes in
// through a macro, so neither file needs to know it is being included twice.
//
// Rangecoder_S  the coder the model uses at RC_VEC=0. For rANS it is the only
//               scalar class; for the range coder it is the CARRYLESS one, so
//               that its stream is the generated kernel's byte for byte.
// Rangecoder_CY the range coder's carry-propagating twin, which the encoder
//               falls back to for a block where a lane lost a carry. rANS has
//               no carries and no twin.

#if RC_RANS

#define Rangecoder Rangecoder_S
#include "rans.inc"
#undef Rangecoder

#else

#define RC_CARRYLESS 0
#define Rangecoder Rangecoder_CY
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#define RC_CARRYLESS 1
#define Rangecoder Rangecoder_S
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#endif

#include "rc_vec.inc"

#include "counter.inc"



//  The rANS encoder's division.
//
//  Left to the COMPILE, like the staging macros below and for the same reason:
//  mk_kernel.sh resolves rans.inc against the -D set alone, with no idea what
//  target the result will be compiled for, and any correct division produces
//  the same stream -- so this is exactly the kind of decision that does not
//  belong in the generated kernel.
//
//  The default is a float32 RECIPROCAL and a multiply, not a divide, and that
//  is the whole point of the shape. A vdivps is ~23 cycles of latency and the
//  loop-carried x cannot afford it; the reciprocal depends only on f, which
//  comes out of pbit[], so writing it this way leaves the divider off the
//  chain entirely and puts one vmulps on it instead. Measured on cascadelake,
//  against ((a)*RANS_BIAS)/(b) -- the same value, the same instruction count,
//  the divide on the chain instead of beside it -- 57.85 -> 64.39 MB/s, best
//  of 5 round-robin rounds on 20 MB of enwik8. Worth 11%.
//
//  That is the whole of what the divide costs, and the two obvious next steps
//  both make it worse, because once the divider is off the chain this sweep is
//  bound by instruction throughput rather than by latency:
//
//    - replacing vdivps with the hardware reciprocal approximation and a
//      Newton step (-ffast-math; -mrecip alone does not reach it, and nothing
//      short of -ffast-math makes clang emit vrcp14ps here) is 57.37 against
//      64.34. The divider port is otherwise idle, so vdivps is nearly free
//      where the ALU instructions that replace it are not.
//    - an ALU-only reciprocal, same reason, 10% slower.
//
//  The sweep's own shape says why: 82 instructions per 16-lane group, of which
//  21 are register moves and spills. It is short of registers, not of cycles.
//
//  RANS_BIAS is the one-sidedness, folded into the reciprocal so that it costs
//  nothing on the chain either. The quotient is below 1<<XSH and float32
//  carries 2^-24 of relative error over the three roundings between f and the
//  quotient, so scaling the reciprocal down by 2^-19 is bigger than that error
//  -- the truncation is never high -- and 2^-19 of a quotient below 2^16 is an
//  eighth, so it is never more than one low either. rans.inc's masked add
//  corrects that one case.
//
//  Both conversions around it are the SIGNED ones, which every target has; x
//  is below 2^31 and the quotient below 2^16, so nothing is misread as
//  negative.
#ifndef RANS_BIAS
#define RANS_BIAS 0.99999809265f     /* 1 - 2^-19 */
#endif
#ifndef RANS_DIV
#define RANS_DIV( a, b ) ( (a) * (RANS_BIAS/(b)) )
#endif

//  Where it is NOT worth putting it: ahead of the renormalisation. The
//  quotient rans.inc wants is floor((x>>sh)/f), and floor(floor(x/2^sh)/f) is
//  floor(x/(f*2^sh)) -- so the divide can run on x BEFORE the shift, with the
//  shift applied to the result afterwards by subtracting it from the float's
//  exponent field (exact: sh is 0/8/16 and the quotient stays normal). That
//  takes the two compares which pick sh off the front of the chain, six cycles
//  of it, and the generated sweep is one instruction shorter. It measures
//  SLOWER -- 63.0 against 63.8 MB/s, best of 6 over 6 round-robin rounds on
//  20 MB of enwik8 -- which is the same answer RCNUM=32 gives: this sweep is
//  not bound by its dependency chain, so shortening it buys nothing and the
//  extra exponent arithmetic costs. Not kept; the measurement is why.



//  Staging the coder's one hot store, and committing it.
//
//  These live here rather than in rc.inc because they are the COMPILE's
//  decision, not the generator's: mk_kernel.sh resolves rc.inc against the
//  -D set alone, with no idea what target the result will be compiled for,
//  and rc_vecD.inc has to stay valid for any of them. So the generated
//  ShiftLow calls RC_STAGE_CL and rc_Quit calls RC_FLUSH_ONE, and what those
//  mean is settled below, where __AVX512F__ and friends are known.
//
//  RC_FLUSHALL is the whole group in one go: the staged arrays are RCNUM lanes
//  of dword and dword-offset, which is exactly a vpscatterdd -- the one
//  instruction SLP will not produce on its own -- or, where there is no
//  scatter, a block of scalar stores that at least needs no address
//  extraction. RC_SCATTER_W lanes at a time, and it has to match the width the
//  sweep's arithmetic came out at; see rc_config.inc. Unaligned loads: the
//  lane arrays carry VECSIZE alignment, which is not the scatter's width.
#if RC_SCATTER

// ShiftLow stages its store here instead of performing it; RC_FLUSH_ONE
// commits one lane, RC_FLUSHALL a whole group.
#define RC_STAGE_INIT( k, ad )       { stcl[k] = 0; stad[k] = (ad); stn[k] = 0; }
#define RC_STAGE_CL( k, ad, v, nn )  { stcl[k] = (v); stad[k] = (ad); stn[k] = (nn); }
#define RC_FLUSH_ONE( k )            ( (uint&)tmpbase[stad[k]] = stcl[k] )

// RC_SCATTER_W==0 -- no scatter instruction on this target, so the group
// commits as a straight block of RCNUM scalar stores read back out of the
// staged arrays. That is still the whole point: the arrays got there as
// contiguous vector stores, so the address extraction the inline store needed
// -- 24 vpextrd plus 4 vextracti128 per group, every one of them on port 5 --
// is gone, and what is left lands on the store port instead. Measured on AVX2
// (skylake, RCNUM=16, 20 MB of enwik8): 103.1 -> 96.7 cycles per 16-bit group.
//
// There is no mask to apply here, so RC_SCATTER_SKIP 1..3 all read as the one
// thing that is left: branch past a group where no lane emitted a byte. 9 is
// the probe, and it means on this path what it means on the other -- no commit
// at all, so the two targets can be asked the same question.
#if RC_SCATTER_W==0

#define RC_COMMIT_ALL()                                                  \
  { RC_UNROLL for( uint _i=0; _i<RCNUM; _i++ )                           \
      (uint&)tmpbase[stad[_i]] = stcl[_i]; }

#if RC_SCATTER_SKIP==0
#define RC_FLUSHALL() RC_COMMIT_ALL()
#else
#define RC_FLUSHALL()                                                    \
  { uint _z = 0;                                                         \
    RC_UNROLL for( uint _i=0; _i<RCNUM; _i++ ) _z |= stn[_i];            \
    if_e0( _z ) RC_COMMIT_ALL() }
#endif

#else

// One sub-group: load the staged lanes, work out which of them owe a store,
// and scatter under RC_SCATTER_SKIP's reading of that. RC_SCATTER_W lanes at
// a time, and the compare gives a k-mask directly -- the scatter's own mask
// operand -- so testing the lanes and masking them costs the same one
// instruction.
#if RC_SCATTER_W==16
#define RC_SCATTER_GROUP( k, MK, SCAT )                                  \
  { __m512i _a = _mm512_loadu_si512( (const void*)&stad[k] );            \
    __m512i _v = _mm512_loadu_si512( (const void*)&stcl[k] );            \
    __mmask16 _m = MK( _mm512_loadu_si512( (const void*)&stn[k] ),       \
                       _mm512_setzero_si512() );                         \
    SCAT; }
#define RC_SCAT_M   _mm512_mask_i32scatter_epi32( (void*)tmpbase, _m, _a, _v, 1 )
#define RC_SCAT_U   _mm512_i32scatter_epi32( (void*)tmpbase, _a, _v, 1 )
#define RC_NE0      _mm512_cmpneq_epi32_mask
#else
#define RC_SCATTER_GROUP( k, MK, SCAT )                                  \
  { __m256i _a = _mm256_loadu_si256( (const __m256i*)&stad[k] );         \
    __m256i _v = _mm256_loadu_si256( (const __m256i*)&stcl[k] );         \
    __mmask8 _m = MK( _mm256_loadu_si256( (const __m256i*)&stn[k] ),     \
                      _mm256_setzero_si256() );                          \
    SCAT; }
#define RC_SCAT_M   _mm256_mask_i32scatter_epi32( (void*)tmpbase, _m, _a, _v, 1 )
#define RC_SCAT_U   _mm256_i32scatter_epi32( (void*)tmpbase, _a, _v, 1 )
#define RC_NE0      _mm256_cmpneq_epi32_mask
#endif

#if   RC_SCATTER_SKIP==0
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, RC_SCAT_U ) }
#elif RC_SCATTER_SKIP==1
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, RC_SCAT_M ) }
#endif

#endif  /* RC_SCATTER_W==0 */

#else

// Not staging: ShiftLow performs the store where it always did, and the
// commit points are nothing at all. The lane arrays go unwritten and cost
// nothing.
#define RC_STAGE_INIT( k, ad )       ((void)0)
#define RC_STAGE_CL( k, ad, v, nn )  ( (void)( (uint&)tmpbase[ad] = (v) ) )
#define RC_FLUSH_ONE( k )            ((void)0)
#define RC_FLUSHALL()                ((void)0)
#endif


template< int f_DEC > struct Model;

#include "model0.inc"

#include "model1.inc"
