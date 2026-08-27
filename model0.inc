
template<>
struct Model<0> : Coroutine, Predictor {
  static const uint f_DEC=0;

  // Slots the encoder can have coding at once. Only the device path uses more
  // than one -- the host coder is synchronous, so a second slot would be a
  // couple of megabytes it never looks at.
  static const uint NSLOT = RC_ISPC ? RC_DEV_NBLK : 1;

  typedef RCio<BLKSIZE,RCNUM,NSLOT> t_RCio;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) t_RCio rcio;

  // One block in the pipeline: its probability cache, and what the device
  // reports back about it. The cache is one {p;bit} pair per coded bit, packed
  // as (bit<<15)+p -- p is clamped to [1,SCALE-1], so the pair fits a word.
  //
  // The model runs ahead over the whole block first, so the coding pass never
  // touches the model. That is what let the original interleave RCNUM lanes in
  // one SIMD sweep; it is also what lets a block be re-coded from scratch when
  // the carryless coder loses a carry, and what lets the host model block n+1
  // while the device is still coding block n.
  struct t_slot {
    ALIGN(4096) word pbit[BLKSIZE*8];
    uint blksize;
    uint lens[RCNUM];    // filled by the device
    uint carry[RCNUM];
    // so rc_Write does not need to know where a block was coded
    uint rc_len( uint i ) { return lens[i]; }
  };

  ALIGN(4096) t_slot slot[NSLOT];

  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CL<0> > t_RC_fast;  // carryless
  typedef RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<0> > t_RC_safe;  // carry twin

  ALIGN(64) t_RC_fast rcC;
  ALIGN(64) t_RC_safe rcY;

  uint head, tail, nq;   // the slot ring: submitted but not yet written

  uint Init( void* p ) {
    uint r = Predictor::Init(p);
    if( r==0 ) {
      DEV_Geometry( t_RCio::rowsize, t_RCio::tmpbufsize );
      DEV_Init();
    }
    return r;
  }

  // One full coding pass over a cached block. Every coder takes it, which is
  // the whole point of caching {p;bit}: a re-code re-runs this and nothing
  // else. Bit i of the block goes to lane i%RCNUM.
  //
  // The kernel in rc_kernel.inc walks the same order per lane; it has to, or
  // the two would not produce the same stream.
  template< class RC >
  void proc_block( RC& rc, const word* pbit, uint blksize ) {
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
  void recode_with_carry( uint s ) {
#if RC_TRACE_FALLBACK
    fprintf( stderr, "coder: block re-coded on the host, carry lost\n" );
#endif
    rcY.rc_Init( rcio, s );
    proc_block( rcY, slot[s].pbit, slot[s].blksize );
    rc_check( rcY );
    rcio.rc_Write( (*this), rcY, s );
  }

  // Code a slot on the host and write it out: carryless first, re-code with
  // the carry-propagating twin if any lane let a carry escape. At
  // RC_LOWBYTES=8 that is a ~2^-33 event per step, so the second pass
  // practically never runs.
  void host_block( uint s ) {
#if RC_FORCE_CARRY
    recode_with_carry( s );
#else
    rcC.rc_Init( rcio, s );
    proc_block( rcC, slot[s].pbit, slot[s].blksize );

    if_e0( rcC.rc_Carries()!=0 ) {
      recode_with_carry( s );
    } else {
      rc_check( rcC );
      rcio.rc_Write( (*this), rcC, s );
    }
#endif
  }

  // Take the oldest block out of the ring and write it. Blocks are collected
  // in submission order, so the output stays in order however they were coded.
  // A device that has given up takes the host branch, one block at a time,
  // which is what keeps a mid-stream failure from reordering anything.
  void drain_one( void ) {
    uint s = head;

#if RC_ISPC && !RC_FORCE_CARRY
    if( DEV_Collect( s, rcio.rows(s), t_RCio::rowsize ) ) {
      uint ncarry = 0, novf = 0;
      for( uint i=0; i<RCNUM; i++ ) {
        ncarry += slot[s].carry[i];
        novf   += (slot[s].lens[i]>t_RCio::tmpbufsize);
      }
      if_e0( novf!=0 ) rc_overflow();
      if( ncarry==0 ) rcio.rc_Write( (*this), slot[s], s );
      else            recode_with_carry( s );
    } else
#endif
      host_block( s );

    head = (head+1)%NSLOT;
    nq--;
  }

  NOINLINE
  void do_process() {
    uint c,i,j,bit,p,blksize,state;

    P_Init(stats,DIM(stats));
    head = tail = nq = 0;

    while(1) {

      chkinp(); blksize=getinplen(); if( blksize>BLKSIZE ) blksize=BLKSIZE;

      // the slot about to be filled may still be coding on the device
      if( nq==NSLOT ) drain_one();

      t_slot& S = slot[tail];
      S.blksize = blksize;

      // model pass: precalc {p;bit} pairs for the whole block
      for( i=0; i<blksize; i++ ) {
        c = get0();
        for( j=0; j<8; j++ ) {
          P_S(state); p=pp[state];
          bit = (c>>(7-j))&1;
          S.pbit[i*8+j] = (bit<<15)+p;
          P_update( state, bit );
        }
        { uint cc=0; P_byte(stats,cc); } // resets the bit context
      }

      uint queued = 0;

#if RC_ISPC && !RC_FORCE_CARRY
      // Device path: the carryless coding pass, RCNUM lanes as RCNUM
      // work-items, writing straight into the substream rows rc_Write reads.
      // Submitting does not block, so the next block's model pass runs while
      // this one codes; the block is written when its slot comes back round.
      //
      // RC_FORCE_CARRY takes the device out: only the carryless coder has a
      // kernel, so "always use the carry twin" and "code on the device" are
      // not both possible, and the knob exists to exercise the twin.
      if( DEV_Active() ) {
        if( DEV_Submit( tail, S.pbit, blksize*8, blksize, S.lens, S.carry ) ) {
          tail = (tail+1)%NSLOT; nq++; queued = 1;
        } else {
          // the device gave up with blocks still queued behind it -- those were
          // submitted first and have to be written first, so drain before
          // touching this one
          while( nq ) drain_one();
        }
      }
#endif

      if( !queued ) {
        host_block( tail );
        tail = (tail+1)%NSLOT; head = tail;
      }

      if( blksize<BLKSIZE ) break;

    }

    while( nq ) drain_one();

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
