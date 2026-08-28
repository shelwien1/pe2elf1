
// Committing the store ShiftLow staged (rc.inc's stcl/stad), when RC_SCATTER
// says it is staged at all. RC_FLUSHALL is the whole group in one go: the
// arrays are RCNUM lanes of dword and dword-offset, which is exactly a
// vpscatterdd, and it is the instruction SLP will not produce on its own.
//
// RC_SCATTER_W lanes per scatter -- it has to match the width the sweep's
// arithmetic came out at, see rc_config.inc. RCNUM is a multiple of 16 either
// way, RC_SCATTER requires it. Unaligned loads: the lane arrays carry VECSIZE
// alignment, which is not the scatter's width.
#if RC_SCATTER

#define RC_FLUSH1( k ) rc_Flush(k)

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
#define RC_FLUSH1( k ) ((void)0)
#define RC_FLUSHALL()  ((void)0)
#endif

template<>
struct Model<0> : Coroutine, Predictor {
  static const uint f_DEC=0;

  typedef RCio<BLKSIZE,RCNUM> t_RCio;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) t_RCio rcio;

  // The probability cache: one {p;bit} pair per coded bit, packed as
  // (bit<<15)+p -- p is clamped to [1,SCALE-1], so the pair fits a word.
  //
  // The model runs ahead over the whole block first, so the coding pass never
  // touches the model. That is what lets the vector coder sweep RCNUM lanes
  // at once, and what lets a block be re-coded from scratch when the
  // carryless pass loses a carry.
  ALIGN(4096) word pbit[BLKSIZE*8];

  // what the vector pass reports, shaped so rc_Write can take it directly
  struct t_res {
    uint lens[RCNUM];
    uint rc_len( uint i ) { return lens[i]; }
  } res;

  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CL<0> > t_RC_fast;  // carryless
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_fast rcC;
  ALIGN(64) t_RC_safe rcY;

  uint Init( void* p ) {
    return Predictor::Init(p);
  }

  // One full scalar coding pass over the cached block. Both scalar coders
  // take it: the -C reference (carryless, checked against the vector pass
  // byte for byte by t.sh) and the carry-propagating twin a lost carry
  // falls back to. Bit i of the block goes to lane i%RCNUM -- the vector
  // pass in do_process walks the same order, it has to.
  template< class RC >
  void proc_block( RC& rc, uint blksize ) {
    uint i,j,c,bit;

    // Block length: one flag bit, then 16 bits when the block is short.
    // BLKSIZE itself does not fit in 16 bits, hence the flag.
    bit = (blksize!=BLKSIZE);
    rc.rc_Process( 0, hSCALE, bit );

    if( bit ) {
      for( j=0,c=0; j<16; j++ ) {
        bit = (blksize>>(15-j))&1;
        rc.rc_Process( j%RCNUM, hSCALE, bit );
        c = c*2 + (bit>0);
      }
    }

    uint blksize0 = blksize*8;
    uint blksize1 = blksize0 - (blksize0%RCNUM);

    for( i=0; i<blksize1; i+=RCNUM ) {
      for( j=0; j<RCNUM; j++ ) {
        uint b = pbit[i+j];
        uint p = b&0x7FFF; b >>= 15;
        rc.rc_Process( j, p, b );
      }
    }

    for( ; i<blksize0; i++ ) {
      uint b = pbit[i];
      uint p = b&0x7FFF; b >>= 15;
      rc.rc_Process( i%RCNUM, p, b );
    }

    rc.rc_Quit();
  }

  // The carry-propagating twin. Reached from either coding path when a carry
  // escaped the carryless one.
  void recode_with_carry( uint blksize ) {
#if RC_TRACE_FALLBACK
    fprintf( stderr, "coder: block re-coded on the host, carry lost\n" );
#endif
    rcY.rc_Init( rcio );
    proc_block( rcY, blksize );
    rc_check( rcY );
    rcio.rc_Write( (*this), rcY );
  }

  // The scalar reference path (-C): carryless first, re-code with the twin
  // if any lane let a carry escape. At RC_LOWBYTES=8 that is a ~2^-33 event
  // per step, so the second pass practically never runs.
  void host_block( uint blksize ) {
#if RC_FORCE_CARRY
    recode_with_carry( blksize );
#else
    rcC.rc_Init( rcio );
    proc_block( rcC, blksize );

    if_e0( rcC.rc_Carries()!=0 ) {
      recode_with_carry( blksize );
    } else {
      rc_check( rcC );
      rcio.rc_Write( (*this), rcC );
    }
#endif
  }

  NOINLINE
  void do_process() {
    uint c,i,j,bit,p,blksize,state;

#if RC_VEC
    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the scalar reference
    // and the carry twin. Locals, so the sweep's stores alias nothing.
#include "rc_vecD.inc"
    static_assert( SCATTER_GEN == RC_SCATTER,
      "rc_vecD.inc was generated with a different RC_SCATTER than this build. "
      "mk_kernel.sh has to see the same -march and -D set as the compile; "
      "build.sh passes both." );
#endif

    P_Init(stats,DIM(stats));

    while(1) {

      chkinp(); blksize=getinplen(); if( blksize>BLKSIZE ) blksize=BLKSIZE;

      // model pass: precalc {p;bit} pairs for the whole block
      for( i=0; i<blksize; i++ ) {
        c = get0();
        for( j=0; j<8; j++ ) {
          P_S(state); p=pp[state];
          bit = (c>>(7-j))&1;
          pbit[i*8+j] = (bit<<15)+p;
          P_update( state, bit );
        }
        { uint cc=0; P_byte(stats,cc); } // resets the bit context
      }

#if RC_VEC && !RC_FORCE_CARRY
      // The vector coding pass: the carryless coder over all RCNUM lanes,
      // straight into the substream rows. The unrolled sweep is what clang
      // auto-vectorizes -- constant lane indices over local arrays, exactly
      // the original sh_v1xN encode side. A lane that lost a carry (FFNum)
      // sends the whole block to the scalar twin.
      if_e1( g_use_vec ) {

        tmpbase = rcio.rows();
        // the substreams grow DOWN from the top of each row; the cursor
        // starts one below the first byte's slot because ShiftLow's store
        // stages its word at [tmpptr..tmpptr+1] (see rc.inc)
        for( j=0; j<RCNUM; j++ ) { tmpptr[j] = (j+1)*t_RCio::rowsize-2; rc_Init(j); }

        // The block-length header codes one lane at a time, and comes back
        // to lane 0 -- the flag bit stages it, then j=0 stages it again --
        // so these commit per bit rather than per group.
        bit = (blksize!=BLKSIZE);
        rc_Process( 0, hSCALE, bit ); RC_FLUSH1( 0 );

        if( bit ) {
          for( j=0; j<16; j++ ) {
            bit = (blksize>>(15-j))&1;
            rc_Process( j%RCNUM, hSCALE, bit ); RC_FLUSH1( j%RCNUM );
          }
        }

        {
          uint blksize0 = blksize*8;
          uint blksize1 = blksize0 - (blksize0%RCNUM);

          // One rc_Process per lane per group, so the RCNUM staged stores
          // are RCNUM distinct addresses in RCNUM distinct rows and the
          // group can commit them together.
          for( i=0; i<blksize1; i+=RCNUM ) {
            RC_UNROLL
            for( j=0; j<RCNUM; j++ ) {
              uint b = pbit[i+j];
              uint f = b&0x7FFF; b >>= 15;
              rc_Process( j, f, b );
            }
            RC_FLUSHALL();
          }

          // fewer than RCNUM bits left: consecutive i, so still one lane each
          for( ; i<blksize0; i++ ) {
            uint b = pbit[i];
            uint f = b&0x7FFF; b >>= 15;
            rc_Process( i%RCNUM, f, b ); RC_FLUSH1( i%RCNUM );
          }
        }

        for( j=0; j<RCNUM; j++ ) rc_Quit(j);

        uint ncarry = 0, novf = 0;
        for( j=0; j<RCNUM; j++ ) {
          uint len = ((j+1)*t_RCio::rowsize-1) - tmpptr[j];
          len = len>RC_SKIP ? len-RC_SKIP : 0;
          res.lens[j] = len;
          ncarry += FFNum[j];
          novf   += (len>t_RCio::tmpbufsize);
        }
        if_e0( novf!=0 ) rc_overflow();
        if( ncarry==0 ) rcio.rc_Write( (*this), res );
        else            recode_with_carry( blksize );

      } else
#endif
        host_block( blksize );

      if( blksize<BLKSIZE ) break;

    }

    yield(this,0);
  }

  // A substream that did not fit its buffer cannot be described by the 2-byte
  // length header either, so there is nothing to fall back on -- say so instead
  // of writing a stream that will not decode. Unreachable with any BLKSIZE and
  // RCNUM whose worst case fits 0xFFFF (see RCio::rc_worst).
  NOINLINE
  void rc_overflow( void ) {
    fprintf( stderr, "coder: substream overflow -- BLKSIZE=%i is too large for "
                     "RCNUM=%i, the 2-byte length header tops out at %i bytes "
                     "per lane. Rebuild with RC_BLKSIZE<=%i or a higher RC_RCNUM.\n",
             int(BLKSIZE), int(RCNUM), 0xFFFF, int(t_RCio::blkmax) );
    exit(4);
  }

  template< class RC >
  void rc_check( RC& rc ) {
    if_e0( rc.rc_Overflow()!=0 ) rc_overflow();
  }

};

#undef RC_FLUSH1
#undef RC_FLUSHALL
