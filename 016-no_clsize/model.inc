
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
#elif RC_SCATTER_SKIP==9
#warning "RC_SCATTER_SKIP=9 drops the commit: this build does not encode correctly"
#define RC_FLUSHALL() ((void)0)
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


// ---------------------------------------------------------------------------
// RC_DEC_RENORM 11 and 12: the decoder's cached refill window, filled for the
// whole group at once.
//
// 9 caches the window in lane state and reloads it per lane, which takes the
// load's LATENCY off the chain but leaves its data movement in the lane body.
// clang auto-vectorises the branchless renorm -- 127 vector ops in the group
// loop against the branchy shape's 16 -- and then has to extract each lane's
// tmpptr out of a vector to form the address and insert the loaded word back:
// 36 vpextrd, 24 vpinsrd and 20 vmovd per group, essentially all port 5.
//
// A gather does the same sixteen loads with the indices left where they
// already are. 11 masks it on dsh!=0, which is the same "the store is dead"
// reading RC_SCATTER_SKIP takes on the encoder -- a lane that did not
// renormalise did not move its cursor, so its cached window is still correct
// and the lane can be left out. 12 is the unmasked control.
//
// The group's refill has to run after every lane has renormalised, so it goes
// where the encoder's RC_FLUSHALL does. RC_REFILL_ONE covers the paths that
// code a single lane at a time (the block header, and the tail below RCNUM).
#if RC_VEC && (RC_DEC_RENORM==11 || RC_DEC_RENORM==12 || RC_DEC_RENORM==14)

#define RC_REFILL_ONE( k )  ( win[k] = (uint&)tmpbase[ tmpptr[k]-3 ] )

// 14 is the shape the split wants: the reload lifted out of the lane body like
// 11, but refilled with plain scalar loads rather than a gather. 11 and 12
// measured the gather and it lost badly; what 14 is for is PLACEMENT -- the
// loads are for the NEXT group, so once they are out of rc_DecPre they can be
// issued after the divide and left to overlap the model's serial walk.
#if RC_DEC_RENORM==14
#define RC_REFILL_W 0
#elif defined(__AVX512F__) && (RCNUM%16)==0
#define RC_REFILL_W 16
#elif defined(__AVX2__) && (RCNUM%8)==0
#define RC_REFILL_W 8
#else
#define RC_REFILL_W 0
#endif

#if RC_REFILL_W==16
#define RC_REFILL_GROUP( k )                                             \
  { __m512i _p = _mm512_sub_epi32(                                       \
        _mm512_loadu_si512( (const void*)&tmpptr[k] ),                   \
        _mm512_set1_epi32(3) );                                          \
    __m512i _w = _mm512_loadu_si512( (const void*)&win[k] );             \
    RC_REFILL_GATHER( k );                                               \
    _mm512_storeu_si512( (void*)&win[k], _w ); }
#if RC_DEC_RENORM==11
#define RC_REFILL_GATHER( k )                                            \
    _w = _mm512_mask_i32gather_epi32( _w,                                \
           _mm512_cmpneq_epi32_mask(                                     \
             _mm512_loadu_si512( (const void*)&dsh[k] ),                 \
             _mm512_setzero_si512() ),                                   \
           _p, (const void*)tmpbase, 1 )
#else
#define RC_REFILL_GATHER( k )                                            \
    _w = _mm512_i32gather_epi32( _p, (const void*)tmpbase, 1 )
#endif

#elif RC_REFILL_W==8
#define RC_REFILL_GROUP( k )                                             \
  { __m256i _p = _mm256_sub_epi32(                                       \
        _mm256_loadu_si256( (const __m256i*)&tmpptr[k] ),                \
        _mm256_set1_epi32(3) );                                          \
    __m256i _w = _mm256_loadu_si256( (const __m256i*)&win[k] );          \
    RC_REFILL_GATHER( k );                                               \
    _mm256_storeu_si256( (__m256i*)&win[k], _w ); }
#if RC_DEC_RENORM==11
// AVX2 gathers take a vector mask, not a k-mask, and consume it destructively
#define RC_REFILL_GATHER( k )                                            \
    _w = _mm256_mask_i32gather_epi32( _w, (const int*)tmpbase, _p,       \
           _mm256_cmpeq_epi32( _mm256_setzero_si256(),                   \
             _mm256_cmpeq_epi32(                                         \
               _mm256_loadu_si256( (const __m256i*)&dsh[k] ),            \
               _mm256_setzero_si256() ) ), 1 )
#else
#define RC_REFILL_GATHER( k )                                            \
    _w = _mm256_i32gather_epi32( (const int*)tmpbase, _p, 1 )
#endif
#endif

#if RC_REFILL_W
#define RC_REFILLALL()  { for( uint _k=0; _k<RCNUM; _k+=RC_REFILL_W )    \
    RC_REFILL_GROUP( _k ) }
#else
// no gather on this target: the same refill, lane by lane
#define RC_REFILLALL()  { RC_UNROLL for( uint _k=0; _k<RCNUM; _k++ )     \
    RC_REFILL_ONE( _k ); }
#endif

#else
#define RC_REFILL_ONE( k )  ((void)0)
#define RC_REFILLALL()      ((void)0)
#endif


// ---------------------------------------------------------------------------
// RC_DEC_SPLIT's batched divide: val = code / (range>>SCALElog), every lane.
//
// x86 has no integer SIMD divide, so this goes through doubles. It is exact,
// and provably so rather than by luck. rc_DecPre has just renormalised, so
// range >= sTOP = 1<<24 and d = range>>15 lands in [1<<9, 1<<17); code < range;
// and the quotient is below 1<<15. Both operands are integers well inside a
// double's 53-bit mantissa, so each converts exactly and the quotient is within
// one ulp -- which leaves it at most one too high. One downward correction
// settles it, and q*d <= code means that multiply cannot overflow 32 bits.
//
// Measured over 6.4M random (code,range) pairs plus the exact-multiple and
// one-below boundaries: no mismatches. float32 is NOT usable here -- code does
// not fit its mantissa, so it needs a correction upwards as well, and that one
// computes (q+1)*d, which DOES overflow 32 bits when q is near its maximum. It
// is no faster either.
#if RC_VEC && RC_DEC_SPLIT

#if defined(__AVX512F__) && (RCNUM%8)==0
#define RC_DIV( k0, kn )                                                 \
  { for( uint _k=(k0); _k<(k0)+(kn); _k+=8 ) {                           \
      __m256i _c = _mm256_loadu_si256( (const __m256i*)&code[_k] );      \
      __m256i _d = _mm256_srli_epi32(                                    \
          _mm256_loadu_si256( (const __m256i*)&range[_k] ), SCALElog );  \
      __m256i _q = _mm512_cvttpd_epu32( _mm512_div_pd(                   \
          _mm512_cvtepu32_pd(_c), _mm512_cvtepu32_pd(_d) ) );            \
      __m256i _t = _mm256_mullo_epi32( _q, _d );                         \
      _q = _mm256_mask_sub_epi32( _q,                                    \
             _mm256_cmpgt_epu32_mask( _t, _c ), _q,                      \
             _mm256_set1_epi32(1) );                                     \
      _mm256_storeu_si256( (__m256i*)&val[_k], _q ); } }
#define RC_DIVALL() RC_DIV( 0, RCNUM )
#else
// no AVX-512 here: the scalar divide, which is what the shape is trying to
// avoid. Correct, and only useful for checking the rest of the structure.
#define RC_DIV( k0, kn )                                                 \
  { RC_UNROLL for( uint _k=(k0); _k<(k0)+(kn); _k++ )                    \
      val[_k] = code[_k] / (range[_k]>>SCALElog); }
#define RC_DIVALL() RC_DIV( 0, RCNUM )
#endif

#endif


template< int f_DEC > struct Model;

#include "model0.inc"

#include "model1.inc"
