
template<>
struct Model<0> : Coroutine {
  static const uint f_DEC=0;

  // The counter shape is a per-MODEL choice, not a per-build one: the encoder
  // and the decoder are separate types with separate stats[] and no shared
  // state, so each carries whatever counter its own loop likes. On this box
  // they disagree -- the decoder is always fused, this side follows
  // RC_FUSE_PP_ENC. Counter2 carries {p,state} in one dword and Counter keeps
  // the state alone with p in a side table.
  typedef SelCounter< RC_FUSE_PP_ENC >::t t_Counter;

  // o0 aligned bit context, and the counter table it indexes. cty is NOT
  // biased by -1: the bias would cost nothing on its own, but it makes
  // cty[2*c] land at an odd index. Index 0 goes unused instead; contexts run
  // 1..255, which is why stats[] is 0x200 entries.
  uint ctx;
  t_Counter* __restrict cty;

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


  // what the vector pass reports, shaped so rc_Write can take it directly
  struct t_res {
    uint lens[RCNUM];
    uint rc_len( uint i ) { return lens[i]; }
  } res;

#if !RC_RANS
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_safe rcY;
#endif

#if RC_VEC==0
  // The reference coding pass: RCNUM scalar coders from the same file the
  // generated kernel comes out of. Not a fallback -- the streams have to
  // match byte for byte, and for rANS that is what checks the vector path's
  // float32 division against this one's integer `/`. See RC_VEC.
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_S<0> > t_RC_ref;

  ALIGN(64) t_RC_ref rcS;
#endif

  // The model_api.inc entry points. Init loads the counter state machine
  // named on the command line; both ends need the same one.
  uint Init( void* p ) {
    uint r = 0;
    if( p!=0 ) {
      FILE* h = fopen( (char*)p, "rb" ); if( h==0 ) r=4; else {
        if( FSM[0].Load(h) ) r=5;
        fclose(h);
      }
    }
    return r;
  }
  void* Save( void* r0 = 0 ) { return 0; }
  uint  Quit( void ) { return 0; }

  // One full scalar coding pass over the cached block, for the
  // carry-propagating twin a lost carry falls back to. Bit i of the block goes
  // to lane i%RCNUM -- the vector pass in do_process walks the same order, it
  // has to.
  template< class RC >
  void proc_block( RC& rc, uint blksize ) {
    uint i,j,c,bit;

    uint blksize0 = blksize*8;
    uint blksize1 = blksize0 - (blksize0%RCNUM);

#if RC_RANS

    // rANS is LIFO -- the decoder's first symbol has to be the encoder's last
    // -- so the block is coded BACKWARDS: the tail bits (which are the block's
    // last), then the groups from the top down, then the 16 block-length bits,
    // then the flag that says whether they are there. Reverse that and it is
    // exactly the order model1.inc decodes in.
    //
    // Within a group the order is free: lane j codes only the bits with
    // i%RCNUM==j, and the lanes share nothing, so descending j would be the
    // same stream as ascending j.

    for( i=blksize0; i>blksize1; ) {
      --i;
      uint b = pbit[i];
      uint p = b&0x7FFF; b >>= 15;
      rc.rc_Process( i%RCNUM, p, b );
    }

    for( i=blksize1; i>0; ) {
      i -= RCNUM;
      for( j=0; j<RCNUM; j++ ) {
        uint b = pbit[i+j];
        uint p = b&0x7FFF; b >>= 15;
        rc.rc_Process( j, p, b );
      }
    }

    if( blksize!=BLKSIZE ) {
      for( j=16; j>0; ) {
        --j;
        bit = (blksize>>(15-j))&1;
        rc.rc_Process( j%RCNUM, hSCALE, bit );
      }
    }

    bit = (blksize!=BLKSIZE);
    rc.rc_Process( 0, hSCALE, bit );

#else

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

#endif

    rc.rc_Quit();
  }

  // The carry-propagating twin. Reached from either coding path when a carry
  // escaped the carryless one. rANS has no carries, so it has no twin.
#if !RC_RANS
  void recode_with_carry( uint blksize ) {
    rcY.rc_Init( rcio );
    proc_block( rcY, blksize );
    rcio.rc_Write( (*this), rcY );
  }
#endif

  // Re-code the block with the carry-propagating twin, for a block where the
  // vector pass let a carry escape. At RC_LOWBYTES=8 that is a ~2^-33 event
  // per step, so this practically never runs.
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
#if RC_DEFER_UPD>1 || defined(RC_DEFER_CHECK)
    // PRICING ONLY, and encoder-only: what a deferred counter update costs in
    // ratio. Every wide-SIMD decode proposal needs one, because at depth 0
    // every in-flight byte reads and writes cty[1] -- so a decoder that reads
    // the counters for G bytes before writing any of them does not reproduce
    // model_pass's order, whatever else it gets right. The fix has to be in
    // the FORMAT: a defined G-byte adaptation lag on both ends.
    //
    // Here the bytes of a group of G all read the state as of the start of the
    // group and their writes land in order at the end, last writer winning --
    // exactly "read all, then write all". G=1 is the current stream and is the
    // check that this path is otherwise faithful. The decoder is NOT changed:
    // the output of this build does not decode, and the only number wanted
    // from it is the block size.
    {
      const uint G = RC_DEFER_UPD;
      uint ua[8*RC_DEFER_UPD], uv[8*RC_DEFER_UPD];
      for( uint i0=0; i0<blksize; i0+=G ) {
        uint n = 0;
        const uint ge = (i0+G<blksize) ? i0+G : blksize;
        for( i=i0; i<ge; i++ ) {
          c = *ip++;
          for( j=0; j<8; j++ ) {
            state = cty[lctx].Get(); p = t_Counter::P(state);
            bit = (c>>(7-j))&1;
            pbit[i*8+j] = (bit<<15)+p;
            // record the write instead of performing it: Update against a
            // scratch copy leaves the live counter alone
            t_Counter tmp = cty[lctx];
            tmp.Update( state, bit );
            ua[n] = lctx; uv[n] = tmp.Get(); n++;
            (lctx*=2) += bit;
          }
          lctx = 1;
        }
        for( uint t=0; t<n; t++ ) cty[ua[t]].Set( uv[t] );
      }
    }
#else
    for( i=0; i<blksize; i++ ) {
      c = *ip++;                          // get0(), against the local cursor
      for( j=0; j<8; j++ ) {
        state = cty[lctx].Get(); p = t_Counter::P(state);
        bit = (c>>(7-j))&1;
        pbit[i*8+j] = (bit<<15)+p;
        cty[lctx].Update( state, bit );
        (lctx*=2) += bit;
      }
      lctx = 1;                           // end of byte: reset the bit context
    }
#endif
    inpptr = ip; ctx = lctx;
  }

  NOINLINE
  void do_process() {
    uint c,i,j,bit,p,blksize,state;

    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the carry twin. Locals, so the sweep's stores alias nothing.
// The sweep reads and writes the lane arrays as vectors, so they carry vector
// alignment. The decoder's copy does not -- see model1.inc.
#if RC_VEC
#define RC_KALIGN ALIGN(VECSIZE)
#include "rc_vecD.inc"
#undef  RC_KALIGN
    static_assert( KERNEL_CONF == RC_KERNEL_CONF,
      "rc_vecD.inc was generated against a different RC_* configuration than "
      "this build. Re-run mk_kernel.sh with the same -D set -- build.sh takes "
      "it as arguments and gc.bat has %defs% for it." );
#endif

    for( i=0; i<DIM(stats); i++ ) stats[i].Reset();
    ctx = 1; cty = stats;

    while(1) {

      chkinp(); blksize=getinplen(); if( blksize>BLKSIZE ) blksize=BLKSIZE;

      model_pass( blksize );

      // The vector coding pass: the coder over all RCNUM lanes, straight into
      // the substream rows. The unrolled sweep is what clang auto-vectorizes
      // -- constant lane indices over local arrays, exactly the original
      // sh_v1xN encode side. For the range coder, a lane that lost a carry
      // (FFNum) sends the whole block to the scalar twin; rANS has no carries.
#if RC_VEC
      {

        tmpbase = rcio.rows();
        // The range coder's substreams grow DOWN from the top of each row, and
        // its cursor starts one below the first byte's slot because the store
        // stages its word at [tmpptr..tmpptr+1] (see rc.inc). rANS grows UP
        // from the base and its cursor IS the next byte's slot (rans.inc).
#if RC_RANS
        for( j=0; j<RCNUM; j++ ) { tmpptr[j] = j*t_RCio::rowsize; rc_Init(j); }
#else
        for( j=0; j<RCNUM; j++ ) { tmpptr[j] = (j+1)*t_RCio::rowsize-2; rc_Init(j); }
#endif

        {
          uint blksize0 = blksize*8;
          uint blksize1 = blksize0 - (blksize0%RCNUM);

#if RC_RANS

          // Backwards, because rANS is LIFO -- proc_block above has the
          // argument, and the two orders have to agree. The tail bits are the
          // block's last, so they go first; then the groups from the top down;
          // then the header, at the bottom of this block.
          //
          // Fewer than RCNUM tail bits at consecutive i, so each lands on a
          // different lane and commits on its own.
          for( i=blksize0; i>blksize1; ) {
            --i;
            uint b = pbit[i];
            uint f = b&0x7FFF; b >>= 15;
            rc_Process( i%RCNUM, f, b ); RC_FLUSH_ONE( i%RCNUM );
          }

          // One rc_Process per lane per group, so the RCNUM staged stores
          // are RCNUM distinct addresses in RCNUM distinct rows and the
          // group can commit them together.
          // Through a restrict pointer, not pbit[i+j]. Descending, i is an
          // unsigned counter clang rewrites into a negative-going one, and
          // i+j is then a 32-bit sum it cannot prove monotone -- so the
          // sixteen loads stop being one contiguous vector load and become
          // eight leas and eight vpinsrw per half group, 20 instructions per
          // group of the 117 this loop had. Hoisting the base does the
          // addition once, in 64 bits, and pb[j] is a constant offset again.
          for( i=blksize1; i>0; ) {
            i -= RCNUM;
            const word* __restrict pb = &pbit[i];
            RC_UNROLL
            for( j=0; j<RCNUM; j++ ) {
              uint b = pb[j];
              uint f = b&0x7FFF; b >>= 15;
              rc_Process( j, f, b );
            }
            RC_FLUSHALL();
          }

          // The block-length header comes LAST here and the flag after it, so
          // that the decoder sees the flag first. Lane 0 takes the flag and
          // also header bit 0, so these commit per bit rather than per group.
          if( blksize!=BLKSIZE ) {
            for( j=16; j>0; ) {
              --j;
              bit = (blksize>>(15-j))&1;
              rc_Process( j%RCNUM, hSCALE, bit ); RC_FLUSH_ONE( j%RCNUM );
            }
          }
          bit = (blksize!=BLKSIZE);
          rc_Process( 0, hSCALE, bit ); RC_FLUSH_ONE( 0 );

#else

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
            rc_Process( i%RCNUM, f, b ); RC_FLUSH_ONE( i%RCNUM );
          }

#endif
        }

        for( j=0; j<RCNUM; j++ ) rc_Quit(j);

        uint novf = 0;
        for( j=0; j<RCNUM; j++ ) {
          // rANS's cursor IS the next byte's slot and started at the row base,
          // so the distance it travelled is the length. The range coder's
          // sits one below, and its own rc_Quit steps it up before the final
          // put1 run, which leaves the payload RC_SKIP further down: the zero
          // prefix, which rc_Write drops.
#if RC_RANS
          uint len = tmpptr[j] - j*t_RCio::rowsize;
#else
          uint len = ((j+1)*t_RCio::rowsize-1) - tmpptr[j];
          len = len>RC_SKIP ? len-RC_SKIP : 0;
#endif
          res.lens[j] = len;
          novf   += (len>t_RCio::tmpbufsize);
        }
        if_e0( novf!=0 ) rc_overflow();

#if RC_RANS
        rcio.rc_Write( (*this), res );
#else
        uint ncarry = uint(-1);
        RC_UNROLL
        for( j=0; j<RC_FF_LANES; j++ ) ncarry &= FFNum[j];   // folded: see RC_FF_LANES
        if( ncarry!=0 ) rcio.rc_Write( (*this), res );   // no lane lost a carry
        else {
          recode_with_carry( blksize );
        }
#endif

      }
#else // RC_VEC==0 -- the scalar reference pass
      {
        rcS.rc_Init( rcio );
        proc_block( rcS, blksize );
#if RC_RANS
        rcio.rc_Write( (*this), rcS );
#else
        if( rcS.rc_Carries()==0 ) rcio.rc_Write( (*this), rcS );
        else recode_with_carry( blksize );
#endif
      }
#endif

      if( blksize<BLKSIZE ) break;

    }

    yield(this,0);
  }

};
