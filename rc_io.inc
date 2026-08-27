
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

  byte* __restrict beg;   // start of this lane's payload
  byte* __restrict ptr;   // cursor -- encoding, it starts RC_SKIP before beg
  byte* __restrict end;   // one past the last writable byte (encoder only)
  uint  f_OVF;            // encoder: set if the substream did not fit

  // Reads past the end of the substream return 0xFF. That is not an accident:
  // RCio::rc_Read pads each substream with RC_FF_PADSIZE bytes of 0xFF, which
  // is what rc_Quit's minimal flush relies on -- it drops the tail bytes the
  // decoder can reconstruct as 0xFF, and only writes the rest. It is also what
  // byte(getc()) gives past EOF, which is how sh_v1xN_s.cpp got it for free.
  byte get( void ) { return *ptr++; }

  // No prefix test here. The zero prefix (RC_SKIP bytes -- see rc_config.inc)
  // is stored like any other byte, into the room StartEncode left for it ahead
  // of beg, and rc_Write starts the payload at beg. put() is the coder's one
  // hot store, and this is the buffer equivalent of rc_Write's
  // ofs = LOWBYTES+1-CODBYTES in sh_v1xN_io.inc; the +1 there is the Cache
  // byte, which the Cache!=-1 guard in rc.inc's ShiftLow accounts for.
  void put( byte c ) {
#if RC_IO_CHECK
    if_e0( ptr>=end ) { f_OVF=1; return; }
#endif
    *ptr++ = c;
  }

  // bufsize is the payload capacity; the prefix sits in front of it, so the row
  // has to be RC_SKIP longer than that -- which is what RCio::rowsize is.
  void StartEncode( byte* buf, uint bufsize ) {
    ptr = buf; beg = buf+RC_SKIP; end = beg+bufsize;
    f_OVF = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  void StartDecode( byte* buf ) {
    beg = ptr = buf; end = 0;
    f_OVF = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  // encoder: payload bytes written, i.e. this lane's substream length. A lane
  // that coded nothing can stop inside the prefix -- 16 header bits over 64
  // lanes leaves most of them with no work -- and owes the stream nothing.
  uint rc_len( void ) const { return ptr>beg ? uint(ptr-beg) : 0; }
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
template< int BLKSIZE, int RCNUM, int NSLOT >
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
  // see RC_CL_WORDOUT in rc_kernel.c.
  static const uint rowsize = ((RC_SKIP+tmpbufsize+FF_PADSIZE)+3) & ~3u;

  // NSLOT sets of substream rows. The decoder wants one; the encoder wants one
  // per block it can have in flight on the device, because a block's rows are
  // what the kernel writes into and they have to stay put until that block is
  // collected and written out.
  ALIGN(4096) byte tmpbuf[NSLOT][RCNUM][rowsize];

  byte* buf( uint rcidx, uint slot=0 ) { return &tmpbuf[slot][rcidx][0]; }
  byte* rows( uint slot=0 ) { return &tmpbuf[slot][0][0]; }

  NOINLINE
  void rc_Read( Coroutine& C, uint slot=0 ) {
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
        memcpy( &tmpbuf[slot][rcidx][i], (prbyte)C.inpptr, n );
        C.inpptr += n;
      }
      for( i=0; i<FF_PADSIZE; i++ ) tmpbuf[slot][rcidx][l+i] = 0xFF;
    } // rcidx
  }

  template< class RCV >
  NOINLINE
  void rc_Write( Coroutine& C, RCV& rc, uint slot=0 ) {
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
        memcpy( (prbyte)C.outptr, &tmpbuf[slot][rcidx][RC_SKIP+i], n );
        C.outptr += n;
      }
    } // rcidx
  }

};
