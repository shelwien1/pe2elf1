
//  RC_IO  -- sh_v1xN_s.cpp's RC_IO with the FILE* swapped for a byte buffer.
//  RCio   -- sh_v1xN_io.inc's block I/O: the substream size headers and the
//            memcpy in/out of the coroutine's stream buffers.
//
//  One RC_IO per rangecoder instance, one buffer per instance: RCNUM instances
//  give RCNUM substreams, which is exactly the block layout the vector coder
//  produced.

//----------------------------------------------------------------------------
//  Per-lane byte cursor. CRTP base of the coder in rc.inc, same as in
//  sh_v1xN_s.cpp -- StartEncode/StartDecode call back into rc_Init, and
//  get()/put() are what ShiftLow/ShiftCode reach for.
//----------------------------------------------------------------------------
template< class Rangecoder >
struct RC_IO {

  // sh_v1xN's arrangement: the substream is written and read BACKWARDS.
  // The cursor starts at the top of the lane's row and every get/put
  // decrements, so the coder's stream order is descending addresses and
  // rc_Write's plain ascending memcpy stores the payload last-emitted-first.
  byte* __restrict beg;   // top of this lane's payload (its first byte)
  byte* __restrict ptr;   // cursor -- encoding, it starts RC_SKIP above beg
  byte* __restrict end;   // one below the last writable byte (encoder only)
  uint  f_OVF;            // encoder: set if the substream did not fit

  // Reads past the end of the substream return 0xFF. That is not an accident:
  // RCio::rc_Read pads the RC_FF_PADSIZE bytes below each substream with
  // 0xFF, which is what rc_Quit's minimal flush relies on -- it drops the
  // tail bytes the decoder can reconstruct as 0xFF, and only writes the rest.
  // It is also what byte(getc()) gives past EOF, which is how sh_v1xN_s.cpp
  // got it for free.
  byte get( void ) { return *ptr--; }

  // No prefix test here. The zero prefix (RC_SKIP bytes -- see rc_config.inc)
  // is stored like any other byte, into the room above beg, and rc_Write's
  // payload copy stops below it. put() is the coder's one hot store; the
  // top-of-row start is the buffer equivalent of rc_Write's
  // tmpbufsize-1-ofs in sh_v1xN_io.inc.
  void put( byte c ) {
#if RC_IO_CHECK
    if_e0( ptr<=end ) { f_OVF=1; return; }
#endif
    *ptr-- = c;
  }

  // top is the highest byte of the lane's row (where the first emitted byte
  // goes); bufsize is the payload capacity, and the prefix sits above it --
  // RCio::rowsize covers both plus the pad below.
  void StartEncode( byte* top, uint bufsize ) {
    ptr = top; beg = top-RC_SKIP; end = beg-bufsize;
    f_OVF = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  // top is the payload's first byte: its highest address, since rc_Read put
  // the chunk back in file order and the file holds it last-emitted-first.
  void StartDecode( byte* top ) {
    beg = ptr = top; end = 0;
    f_OVF = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  // encoder: payload bytes written, i.e. this lane's substream length. A lane
  // that coded nothing can stop inside the prefix -- 16 header bits over 64
  // lanes leaves most of them with no work -- and owes the stream nothing.
  uint rc_len( void ) const { return ptr<beg ? uint(beg-ptr) : 0; }
};


//----------------------------------------------------------------------------
//  Block I/O. Compressed format, unchanged from sh_v1xN_io.inc:
//
//    per block:  RCNUM x 2-byte little-endian substream length
//                RCNUM x substream payload, in lane order
//
//  The block length itself is not in the header -- it is coded inside the
//  rangecoder data (a flag bit plus 16 bits when the block is short), see
//  model0.inc / model1.inc.
//----------------------------------------------------------------------------
template< int BLKSIZE, int RCNUM >
struct RCio {
  typedef byte* __restrict prbyte;

  // The 0xFF run the decoder is allowed to read past the end of a substream.
  // It has to cover both things rc_Quit can leave out: the minimal flush's own
  // tail, which is at most LOWBYTES, and the trailing 0xFF run the
  // carry-propagating coder drops, which is capped at RC_FF_TRIM precisely so
  // that this constant can exist -- see rc_config.inc.
  static const uint FF_PADSIZE = RC_FF_PADSIZE;
  static_assert( FF_PADSIZE >= RC_FF_TRIM + RC_LOWBYTES,
    "RC_FF_PADSIZE must cover RC_FF_TRIM plus the minimal flush's own tail" );
  // The 32-bit refill window reaches three bytes below the cursor, so the pad
  // has to cover three more than the cursor itself can descend to.
  static_assert( FF_PADSIZE >= RC_FF_TRIM + RC_LOWBYTES + 3,
    "the 32-bit refill reads tmpbase[tmpptr-3]; RC_FF_PADSIZE must cover it" );
  // ...and the refill addresses that window from the cursor, so the load can
  // start at the cursor itself (or one above it, when nothing is shifted in)
  // and read up to tmpptr+4 -- stream the cursor has already passed, but it
  // still has to be inside the row. The cursor never rises above the payload's
  // top byte, which sits RC_SKIP below the row top, so RC_SKIP alone decides it.
  static_assert( RC_SKIP >= 4,
    "the refill reads above tmpptr; RC_SKIP (= RC_LOWBYTES minus RC_CODBYTES) "
    "must leave four bytes above the payload" );

  // Worst case output for one lane: bits_per_lane (plus the 17 block-length
  // header bits, which all land in the low lanes) at SCALElog bits each, since
  // the model's p is clamped to [1,SCALE-1] and so one bit can cost at most
  // SCALElog bits. Plus the flush.
  //
  // Capped at 0xFFFF because the length header is two bytes: with the default
  // geometry the true worst case (61543 at BLKSIZE=64K, RCNUM=16) is below the
  // cap anyway, but a low RCNUM would push it over, and there the cap plus the
  // RC_IO_CHECK overflow flag turns a silent overrun into a loud error.
  static const uint rc_worst = ((BLKSIZE*8+RCNUM-1)/RCNUM + 17)*SCALElog/8
                             + RC_LOWBYTES + 64;
  static const uint tmpbufsize = macro_Min( rc_worst, 0xFFFFu );

  // Largest BLKSIZE this RCNUM can describe. Below it, rc_worst fits the length
  // header and a substream can never overflow whatever the data does.
  static const uint blkmax = (0xFFFFu - RC_LOWBYTES - 96)*RCNUM/SCALElog;

#if RC_STRICT_BLKSIZE
  static_assert( uint(BLKSIZE) <= blkmax,
    "BLKSIZE is too large for RCNUM: one lane's worst-case output would not fit "
    "the 2-byte substream length header. Lower RC_BLKSIZE, raise RC_RCNUM, or "
    "drop RC_STRICT_BLKSIZE and rely on the runtime guard." );
#endif

  // Row stride of tmpbuf. The OpenCL path writes its lanes into rows of
  // exactly this pitch, so the payload readback lands where rc_Write expects.
  // Rounded up to 4 so a row start is dword-aligned: the kernel writes each
  // lane's substream a dword at a time, because AVX-512 has no byte scatter --
  // see RC_DEV_WORDOUT in rc_kernel.c.
  static const uint rowsize = ((RC_SKIP+tmpbufsize+FF_PADSIZE)+3) & ~3u;

  // rowsize is also the stride between the RCNUM lane cursors, and all of them
  // are hot and written every group (model0.inc: tmpptr[j] = (j+1)*rowsize-2).
  // The stride therefore decides how the cursors spread over L1d's 64 sets
  // (64x64B = 4096) and the L1 dTLB's sets (16x4096 = 65536): j*stride mod P
  // takes P/gcd(stride,P) distinct values, so the cursors stay on distinct
  // sets only while gcd(stride,P)*RCNUM <= P. Nothing in the & ~3u above
  // defends that -- a change to RC_FF_TRIM, RC_LOWBYTES, SCALElog or BLKSIZE
  // could land rowsize on a multiple of 4096 and put all RCNUM cursors into
  // one 8-way L1 set. It holds by a wide margin today (rowsize=61612=4*73*211,
  // gcd 4 either way), which is exactly why it would go unnoticed.
  static constexpr uint sgcd( uint a, uint b ) { return b ? sgcd(b,a%b) : a; }
  static_assert( sgcd(rowsize,4096)*RCNUM <= 4096,
    "RCio::rowsize crowds the RCNUM lane cursors into too few L1d sets -- "
    "nudge RC_FF_PADSIZE by 4" );
  static_assert( sgcd(rowsize,65536)*RCNUM <= 65536,
    "RCio::rowsize crowds the RCNUM lane cursors into too few L1 dTLB sets -- "
    "nudge RC_FF_PADSIZE by 4" );

  // One set of substream rows: the decoder reads a block into them, the
  // encoder writes a block straight into them. Each lane's substream sits at
  // the TOP of its row and grows down (see RC_IO above); the payload chunk in
  // the file is that region copied ascending, i.e. last-emitted-first, so the
  // decoder places it back ascending and walks it from its top.
  ALIGN(4096) byte tmpbuf[RCNUM][rowsize];

  // decoder: the substream lengths of the current block, from rc_Read's
  // headers -- the decode cursors start at FF_PADSIZE-1+rclen[rcidx].
  uint rclen[RCNUM];

  byte* buf( uint rcidx ) { return &tmpbuf[rcidx][0]; }
  byte* rows( void ) { return &tmpbuf[0][0]; }

  NOINLINE
  void rc_Read( Coroutine& C ) {
    uint rcidx,i,l,n;

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l  = byte(C.get());
      l += byte(C.get())*256;
      // A valid stream never exceeds tmpbufsize; a corrupt header could, and
      // the payload copy and the FF padding below both index by it.
      rclen[rcidx] = Min<uint>( l, tmpbufsize );
    }

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rclen[rcidx];
      for( i=0; i<l; i+=n ) {
        C.chkinp(); n = Min<uint>( l-i, C.getinplen() );
        // truncated stream: chkinp has nothing left to hand us. Stop rather
        // than spin -- the FF padding below fills the hole and the block
        // decodes to garbage, which is the best a truncated file can do.
        if_e0( n==0 ) break;
        memcpy( &tmpbuf[rcidx][FF_PADSIZE+i], (prbyte)C.inpptr, n );
        C.inpptr += n;
      }
      // the decoder reads downward, so the over-read pad sits BELOW the chunk
      for( i=0; i<FF_PADSIZE; i++ ) tmpbuf[rcidx][i] = 0xFF;
    } // rcidx
  }

  template< class RCV >
  NOINLINE
  void rc_Write( Coroutine& C, RCV& rc ) {
    uint rcidx,i,l,n;

    uint rclen[RCNUM];
    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rc.rc_len(rcidx);
      C.put( byte(l) );
      C.put( byte(l>>8) );
      rclen[rcidx] = l;
    }

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rclen[rcidx];
      for( i=0; i<l; i+=n ) {
        C.chkout(); n = Min<uint>( l-i, C.getoutlen() );
        // the payload spans [rowsize-RC_SKIP-l, rowsize-1-RC_SKIP]: its top
        // byte sits RC_SKIP below the row top (above it, the zero prefix),
        // and the ascending copy emits it last-emitted-first
        memcpy( (prbyte)C.outptr, &tmpbuf[rcidx][rowsize-RC_SKIP-l+i], n );
        C.outptr += n;
      }
    } // rcidx
  }

};
