
#include "misc/intrin.inc"

#define EXTERN extern
#include "FSM.cpp"

#include "rc_config.inc"
extern int g_use_vec;   // -C: the scalar reference coder
#include "rc_io.inc"

// rc.inc is the scalar rangecoder of sh_v1xN_s.cpp, and it is included twice:
// once carryless (the encoder's fast path, which drops carry propagation and
// only flags a lost carry) and once with carry propagation (the twin the
// encoder falls back to for that block, and the decoder's coder -- the decode
// path is identical in both, RC_CARRYLESS only touches ShiftLow and rc_Quit).
// The class name comes in through a macro, so rc.inc itself stays the file it
// was moved from.

#define RC_CARRYLESS 1
#define Rangecoder Rangecoder_CL
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#define RC_CARRYLESS 0
#define Rangecoder Rangecoder_CY
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#include "rc_vec.inc"

#include "counter.inc"

#include "predict.inc"


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
// Only RC_SCATTER_SKIP 0 (always commit) and non-zero (branch past a group
// where no lane emitted a byte) mean anything here; there is no mask to apply.
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
#define RC_EQ0      _mm512_cmpeq_epi32_mask
#define RC_FULL     0xFFFFu
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
#define RC_EQ0      _mm256_cmpeq_epi32_mask
#define RC_FULL     0xFFu
#endif

#if   RC_SCATTER_SKIP==0
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, RC_SCAT_U ) }
#elif RC_SCATTER_SKIP==1
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, RC_SCAT_M ) }
#elif RC_SCATTER_SKIP==2
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, if_e0( _m ) RC_SCAT_M ) }
#elif RC_SCATTER_SKIP==3
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_EQ0, if_e1( _m!=RC_FULL ) RC_SCAT_U ) }
#else
#warning "RC_SCATTER_SKIP=9 drops the scatter: this build does not encode correctly"
#define RC_FLUSHALL() { for( uint _k=0; _k<RCNUM; _k+=RC_SCATTER_W )     \
    RC_SCATTER_GROUP( _k, RC_NE0, (void)_m ) }
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
