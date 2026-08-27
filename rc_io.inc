
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

  byte* __restrict beg;   // start of this lane's substream buffer
  byte* __restrict ptr;   // cursor
  byte* __restrict end;   // one past the last writable byte (encoder only)
  uint  Skip;             // encoder: leading zero bytes still to drop
  uint  f_OVF;            // encoder: set if the substream did not fit

  // Reads past the end of the substream return 0xFF. That is not an accident:
  // RCio::rc_Read pads each substream with RC_FF_PADSIZE bytes of 0xFF, which
  // is what rc_Quit's minimal flush relies on -- it drops the tail bytes the
  // decoder can reconstruct as 0xFF, and only writes the rest. It is also what
  // byte(getc()) gives past EOF, which is how sh_v1xN_s.cpp got it for free.
  byte get( void ) { return *ptr++; }

  // The encoder's first LOWBYTES-CODBYTES bytes are the zero prefix: additions
  // land in the bottom 32 bits of low while ShiftLow emits the top byte, so the
  // top has not been reached yet. That lag is the point of the wide accumulator,
  // and the prefix is not part of the stream. This is the buffer equivalent of
  // rc_Write's ofs = LOWBYTES+1-CODBYTES in sh_v1xN_io.inc; the +1 there is the
  // Cache byte, which the Cache!=-1 guard in rc.inc's ShiftLow accounts for.
  void put( byte c ) {
    if_e0( Skip ) { Skip--; return; }
#if RC_IO_CHECK
    if_e0( ptr>=end ) { f_OVF=1; return; }
#endif
    *ptr++ = c;
  }

  void StartEncode( byte* buf, uint bufsize ) {
    beg = ptr = buf; end = buf+bufsize;
    f_OVF = 0;
    Skip = Rangecoder::LOWBYTES - Rangecoder::CODBYTES;
    ((Rangecoder*)this)->rc_Init();
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  void StartDecode( byte* buf ) {
    beg = ptr = buf; end = 0;
    f_OVF = 0; Skip = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  // encoder: bytes written so far, i.e. this lane's substream length
  uint rc_len( void ) const { return ptr-beg; }
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

  // 0xFF run the decoder is allowed to read past the end of a substream. The
  // encoder's flush drops up to LOWBYTES tail bytes, and the decoder runs
  // CODBYTES ahead of the encoder, so it over-reads by at most a handful of
  // bytes; 64 is well clear of that.
  static const uint RC_FF_PADSIZE = 64;

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
  static const uint rowsize = tmpbufsize+RC_FF_PADSIZE;

  ALIGN(4096) byte tmpbuf[RCNUM][rowsize];

  byte* buf( uint rcidx ) { return &tmpbuf[rcidx][0]; }

  NOINLINE
  void rc_Read( Coroutine& C ) {
    uint rcidx,i,l,n;

    uint rclen[RCNUM];
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
        memcpy( &tmpbuf[rcidx][i], (prbyte)C.inpptr, n );
        C.inpptr += n;
      }
      for( i=0; i<RC_FF_PADSIZE; i++ ) tmpbuf[rcidx][l+i] = 0xFF;
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
        memcpy( (prbyte)C.outptr, &tmpbuf[rcidx][i], n );
        C.outptr += n;
      }
    } // rcidx
  }

};
