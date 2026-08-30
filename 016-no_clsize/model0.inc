
template<>
struct Model<0> : Coroutine, PredictorE {
  static const uint f_DEC=0;

  typedef RCio<BLKSIZE,RCNUM> t_RCio;

  ALIGN(64) t_Counter stats[0x200];   // order0

  ALIGN(4096) t_RCio rcio;

  // The probability cache: one {p;bit} pair per coded bit, packed as
  // (bit<<15)+p -- p is clamped to [1,SCALE-1], so the pair fits a word.
  //
  // The model runs ahead over the whole block first, so the coding pass never
  // touches the model. That is what lets the vector coder sweep RCNUM lanes
  // at once, and what lets a block be re-coded from scratch when the
  // carryless pass loses a carry.
  ALIGN(4096) word pbit[BLKSIZE*8];

#if RC_CHUNK
  // The carry fallback re-codes the whole block out of pbit[], and the chunked
  // pass only ever holds one chunk of it. So the model is snapshotted before
  // the pass and the fallback rewinds and re-caches the block in full. The
  // snapshot is ~2 KB once per block; the restore runs about once in 2^33
  // coding steps, which is to say never.
  ALIGN(64) t_Counter ctysave[0x200];
  byte* inpsave;
  uint  ctxsave;
#endif

  // what the vector pass reports, shaped so rc_Write can take it directly
  struct t_res {
    uint lens[RCNUM];
    uint rc_len( uint i ) { return lens[i]; }
  } res;

  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_safe rcY;

  uint Init( void* p ) {
    return PredictorE::Init(p);
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
    rcY.rc_Init( rcio );
    proc_block( rcY, blksize );
    rcio.rc_Write( (*this), rcY );
  }

  // The scalar reference path (-C): carryless first, re-code with the twin
  // if any lane let a carry escape. At RC_LOWBYTES=8 that is a ~2^-33 event
  // per step, so the second pass practically never runs.
  // A substream that did not fit its buffer cannot be described by the 2-byte
  // length header either, so there is nothing to fall back on -- say so instead
  // of writing a stream that will not decode. rc_io.inc's blkmax assert makes
  // this unreachable for any BLKSIZE/RCNUM that compiles; it stays as the
  // backstop for the geometry knobs.
  void rc_overflow( void ) {
    fprintf( stderr, "coder: substream overflow -- BLKSIZE=%i is too large for "
                     "RCNUM=%i, the 2-byte length header tops out at %i bytes "
                     "per lane. Rebuild with RC_BLKSIZE<=%i or a higher RC_RCNUM.\n",
             int(BLKSIZE), int(RCNUM), 0xFFFF, int(t_RCio::blkmax) );
    exit(4);
  }

  // The model pass, out of line on purpose. It precalcs the {p;bit} pair for
  // every bit of the block, and it has no vector code -- but inside
  // do_process it was paying for do_process's. The generated kernel's
  // ALIGN(VECSIZE) lane arrays force `andq $-32, %rsp` in the prologue, and
  // dynamic stack realignment needs a frame pointer, so -fomit-frame-pointer
  // is overridden and %rbp leaves the allocator for the whole function; the
  // coding pass's hoisted values hold further registers across every one of
  // the 65536 iterations this loop runs. Out of line it gets its own frame,
  // no realignment, and a full register file. One call per block.
  //
  // Out of locals for the same reason it was before: cty[ctx].state's store
  // may-alias this->ctx, and inpptr is union-overlaid with pin[]
  // (coro3b.inc), so neither cursor is promoted and both would be stored once
  // per input byte -- and the ctx store is the same constant every time.
  NOINLINE
  void model_pass( uint blksize ) {
    uint c,i,j,bit,p,state;
    byte* __restrict ip = inpptr;
    uint lctx = ctx;
    for( i=0; i<blksize; i++ ) {
      c = *ip++;                          // get0(), against the local cursor
      for( j=0; j<8; j++ ) {
        P_S_L(lctx,state); p=P_P(state);
        bit = (c>>(7-j))&1;
        pbit[i*8+j] = (bit<<15)+p;
        P_update_L( lctx, state, bit );
      }
      lctx = 1;                           // P_byte: resets the bit context
    }
    inpptr = ip; ctx = lctx;
  }

  NOINLINE
  void do_process() {
    uint c,i,j,bit,p,blksize,state;

    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the carry twin. Locals, so the sweep's stores alias nothing.
// The sweep reads and writes the lane arrays as vectors, so they carry vector
// alignment. The decoder's copy does not -- see model1.inc.
#define RC_KALIGN ALIGN(VECSIZE)
#include "rc_vecD.inc"
#undef  RC_KALIGN
    static_assert( KERNEL_CONF == RC_KERNEL_CONF,
      "rc_vecD.inc was generated against a different RC_* configuration than "
      "this build. Re-run mk_kernel.sh with the same -D set -- build.sh takes "
      "it as arguments and gc.bat has %defs% for it." );

    P_Init(stats,DIM(stats));

    while(1) {

      chkinp(); blksize=getinplen(); if( blksize>BLKSIZE ) blksize=BLKSIZE;

#if !RC_CHUNK
      model_pass( blksize );
#endif

      // The vector coding pass: the carryless coder over all RCNUM lanes,
      // straight into the substream rows. The unrolled sweep is what clang
      // auto-vectorizes -- constant lane indices over local arrays, exactly
      // the original sh_v1xN encode side. A lane that lost a carry (FFNum)
      // sends the whole block to the scalar twin.
      {

        tmpbase = rcio.rows();
        // the substreams grow DOWN from the top of each row; the cursor
        // starts one below the first byte's slot because ShiftLow's store
        // stages its word at [tmpptr..tmpptr+1] (see rc.inc)
        for( j=0; j<RCNUM; j++ ) { tmpptr[j] = (j+1)*t_RCio::rowsize-2; rc_Init(j); }

        // The block-length header codes one lane at a time, and comes back
        // to lane 0 -- the flag bit stages it, then j=0 stages it again --
        // so these commit per bit rather than per group.
        bit = (blksize!=BLKSIZE);
        rc_Process( 0, hSCALE, bit ); RC_FLUSH_ONE( 0 );

        if( bit ) {
          for( j=0; j<16; j++ ) {
            bit = (blksize>>(15-j))&1;
            rc_Process( j%RCNUM, hSCALE, bit ); RC_FLUSH_ONE( j%RCNUM );
          }
        }

#if RC_CHUNK
        inpsave = inpptr; ctxsave = ctx;
        memcpy( ctysave, stats, sizeof(stats) );
#endif
        // RC_CHUNK: model pass and sweep interleaved over CHUNK input bytes
        // at a time, both working the low end of pbit[] so the buffer that
        // travels between them is CHUNK*16 bytes instead of 1 MB. RC_CHUNK is
        // even, so every chunk is a whole number of RCNUM-bit groups and bit i
        // still lands in lane i%RCNUM exactly as the scalar reference puts it.
        for( uint _c0=0; _c0<blksize; _c0+=RC_CHUNK_N ) {
          uint _cn = blksize-_c0; if( _cn>RC_CHUNK_N ) _cn = RC_CHUNK_N;
#if RC_CHUNK
          model_pass( _cn );
#endif
        {
          uint blksize0 = _cn*8;
          uint blksize1 = blksize0 - (blksize0%RCNUM);

          // One rc_Process per lane per group, so the RCNUM staged stores
          // are RCNUM distinct addresses in RCNUM distinct rows and the
          // group can commit them together.
#if RC_SWEEP_NEGIDX
          // speed_plan_next.md 5.2: `add/cmp/jb` is two fused uops per group;
          // biasing the base and counting a negative index up to zero makes
          // the add set ZF itself, so it is `add/jne` -- one.
          {
            const word* __restrict _pb = pbit + blksize1;
            for( int _i = -int(blksize1); _i<0; _i += RCNUM ) {
              RC_UNROLL
              for( j=0; j<RCNUM; j++ ) {
                uint b = _pb[_i+int(j)];
                uint f = b&0x7FFF; b >>= 15;
                rc_Process( j, f, b );
              }
              RC_FLUSHALL();
            }
            i = blksize1;
          }
#else
          for( i=0; i<blksize1; i+=RCNUM ) {
            RC_UNROLL
            for( j=0; j<RCNUM; j++ ) {
              uint b = pbit[i+j];
              uint f = b&0x7FFF; b >>= 15;
              rc_Process( j, f, b );
            }
            RC_FLUSHALL();
          }
#endif

          // fewer than RCNUM bits left: consecutive i, so still one lane each
          for( ; i<blksize0; i++ ) {
            uint b = pbit[i];
            uint f = b&0x7FFF; b >>= 15;
            rc_Process( i%RCNUM, f, b ); RC_FLUSH_ONE( i%RCNUM );
          }
        }
        }

        for( j=0; j<RCNUM; j++ ) rc_Quit(j);

        uint ncarry = uint(-1), novf = 0;
        for( j=0; j<RCNUM; j++ ) {
          uint len = ((j+1)*t_RCio::rowsize-1) - tmpptr[j];
          len = len>RC_SKIP ? len-RC_SKIP : 0;
          res.lens[j] = len;
          if( j<RC_FF_LANES ) ncarry &= FFNum[j];   // folded: see RC_FF_LANES
          novf   += (len>t_RCio::tmpbufsize);
        }
        if_e0( novf!=0 ) rc_overflow();
        if( ncarry!=0 ) rcio.rc_Write( (*this), res );   // no lane lost a carry
        else {
#if RC_CHUNK
          // rewind the model and cache the whole block, which is what
          // proc_block reads
          inpptr = inpsave; ctx = ctxsave;
          memcpy( stats, ctysave, sizeof(stats) );
          model_pass( blksize );
#endif
          recode_with_carry( blksize );
        }

      }

      if( blksize<BLKSIZE ) break;

    }

    yield(this,0);
  }

};
