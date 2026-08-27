
template<>
struct Model<0> : Coroutine, Predictor {
  static const uint f_DEC=0;

  typedef RCio<BLKSIZE,RCNUM> t_RCio;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) t_RCio rcio;

  // Probability cache: one {p;bit} pair per coded bit of the block, packed as
  // (bit<<15)+p -- p is clamped to [1,SCALE-1], so the pair fits a word. The
  // model runs ahead over the whole block first, so the coding pass never
  // touches the model -- which is what let the original interleave RCNUM lanes
  // in one SIMD sweep, what lets the block be re-coded from scratch when the
  // carryless coder loses a carry, and what the OpenCL path uploads.
  ALIGN(4096) word pbit[BLKSIZE*8];

  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CL<0> > t_RC_fast;  // carryless
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_fast rcC;
  ALIGN(64) t_RC_safe rcY;

  // The lengths the device reports, wearing the same rc_len(rcidx) face the
  // coders wear, so rc_Write does not need to know where a block came from.
  struct t_devlens {
    uint len[RCNUM];
    uint rc_len( uint i ) { return len[i]; }
  };
  t_devlens cl_lens;
  uint cl_carry[RCNUM];

  uint Init( void* p ) {
    uint r = Predictor::Init(p);
    if( r==0 ) {
      CL_Geometry( t_RCio::rowsize, t_RCio::tmpbufsize );
      CL_Init();
    }
    return r;
  }

  // One full coding pass over the cached block. Both coders take it, which is
  // the whole point of caching {p;bit}: the fallback re-runs this and nothing
  // else. Bit i of the block goes to lane i%RCNUM.
  //
  // The kernel in rc_kernel.inc walks the same order per lane; it has to, or
  // the two would not produce the same stream.
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

  // The carry-propagating twin, on the host. Reached from either coding path
  // when a carry escaped the carryless one.
  void recode_with_carry( uint blksize ) {
#if RC_TRACE_FALLBACK
    fprintf( stderr, "coder: block re-coded on the host, carry lost\n" );
#endif
    rcY.rc_Init(rcio);
    proc_block( rcY, blksize );
    rc_check( rcY );
    rcio.rc_Write( (*this), rcY );
  }

  NOINLINE
  void do_process() {
    uint c,i,j,bit,p,blksize,state;

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

      uint done = 0;

#if RC_OPENCL && !RC_FORCE_CARRY
      // Device path: the carryless coding pass, RCNUM lanes as RCNUM
      // work-items, writing straight into the substream rows rc_Write reads.
      // Everything else about the block -- the fallback and the block format
      // -- is what it was.
      //
      // RC_FORCE_CARRY takes the device out: only the carryless coder has a
      // kernel, so "always use the carry twin" and "code on the device" are
      // not both possible, and the knob exists to exercise the twin.
      if( CL_Active() ) {
        if( CL_EncodeBlock( pbit, blksize*8, blksize, &rcio.tmpbuf[0][0],
                            t_RCio::rowsize, cl_lens.len, cl_carry ) ) {
          uint ncarry = 0, novf = 0;
          for( j=0; j<RCNUM; j++ ) {
            ncarry += cl_carry[j];
            novf   += (cl_lens.len[j]>t_RCio::tmpbufsize);
          }
          if_e0( novf!=0 ) rc_overflow();
          if( ncarry==0 ) rcio.rc_Write( (*this), cl_lens );
          else            recode_with_carry( blksize );
          done = 1;
        }
        // otherwise the device gave up and switched itself off; the host
        // coder below picks the block up as if it had never been asked
      }
#endif

      // Host path: carryless first, re-code with the carry-propagating twin if
      // any lane let a carry escape. At RC_LOWBYTES=8 that is a ~2^-33 event
      // per step, so the second pass practically never runs.
      if( !done ) {
#if RC_FORCE_CARRY
        recode_with_carry( blksize );
#else
        rcC.rc_Init(rcio);
        proc_block( rcC, blksize );

        if_e0( rcC.rc_Carries()!=0 ) {
          recode_with_carry( blksize );
        } else {
          rc_check( rcC );
          rcio.rc_Write( (*this), rcC );
        }
#endif
      }

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
