
template<>
struct Model<0> : Coroutine, Predictor {
  static const uint f_DEC=0;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) RCio<BLKSIZE,RCNUM> rcio;

  // Probability cache: one {p;bit} pair per coded bit of the block, packed as
  // (bit<<15)+p. The model runs ahead over the whole block first, so the coding
  // pass never touches the model -- which is what let the original interleave
  // RCNUM lanes in one SIMD sweep, and what lets the block be re-coded from
  // scratch when the carryless coder loses a carry.
  ALIGN(4096) int pbit[BLKSIZE*8];

  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CL<0> > t_RC_fast;  // carryless
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_fast rcC;
  ALIGN(64) t_RC_safe rcY;

  // One full coding pass over the cached block. Both coders take it, which is
  // the whole point of caching {p;bit}: the fallback re-runs this and nothing
  // else. Bit i of the block goes to lane i%RCNUM.
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

      // coding pass: carryless first, and re-code with the carry-propagating
      // twin if any lane let a carry escape. At RC_LOWBYTES=8 that is a
      // ~2^-33 event per step, so the second pass practically never runs.
#if RC_FORCE_CARRY
      rcY.rc_Init(rcio);
      proc_block( rcY, blksize );
      rc_check( rcY );
      rcio.rc_Write( (*this), rcY );
#else
      rcC.rc_Init(rcio);
      proc_block( rcC, blksize );

      if_e0( rcC.rc_Carries()!=0 ) {
#if RC_TRACE_FALLBACK
        fprintf( stderr, "coder: block re-coded, %u carr%s lost\n",
                 rcC.rc_Carries(), rcC.rc_Carries()==1?"y":"ies" );
#endif
        rcY.rc_Init(rcio);
        proc_block( rcY, blksize );
        rc_check( rcY );
        rcio.rc_Write( (*this), rcY );
      } else {
        rc_check( rcC );
        rcio.rc_Write( (*this), rcC );
      }
#endif

      if( blksize<BLKSIZE ) break;

    }

    yield(this,0);
  }

  // A substream that did not fit its buffer cannot be described by the 2-byte
  // length header either, so there is nothing to fall back on -- say so instead
  // of writing a stream that will not decode. Unreachable with any BLKSIZE and
  // RCNUM whose worst case fits 0xFFFF (see RCio::rc_worst).
  template< class RC >
  void rc_check( RC& rc ) {
    if_e0( rc.rc_Overflow()!=0 ) {
      fprintf( stderr, "coder: substream overflow -- BLKSIZE=%i is too large for "
                       "RCNUM=%i, the 2-byte length header tops out at %i bytes "
                       "per lane. Rebuild with RC_BLKSIZE<=%i or a higher RC_RCNUM.\n",
               int(BLKSIZE), int(RCNUM), 0xFFFF,
               int(RCio<BLKSIZE,RCNUM>::blkmax) );
      exit(4);
    }
  }

};
