
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
  //
  // rANS writes the other way, from the base of the row upwards, because its
  // renormalisation emits the LOW byte of the state first -- see the byte
  // order note in rans.inc. Its decoder still reads downwards, from the top
  // of the chunk: rc_Write reverses the substream on the way out, and a LIFO
  // decoder wants it reversed.
#if RC_RANS
  void put( byte c ) {
    *ptr++ = c;
  }
#else
  void put( byte c ) {
    *ptr-- = c;
  }
#endif

  // top is where the first emitted byte goes -- the highest byte of the lane's
  // row for the range coder, whose prefix then sits above the payload and
  // whose cursor descends, and the row's BASE for rANS, whose cursor ascends.
  // bufsize is the payload capacity; RCio::rowsize covers it plus the pad.
  void StartEncode( byte* top, uint bufsize ) {
#if RC_RANS
    beg = ptr = top; end = top+bufsize;
#else
    ptr = top; beg = top-RC_PAYTOP; end = beg-bufsize;
#endif
    ((Rangecoder*)this)->rc_Init();
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  // top is the byte the decoder wants first, at its highest address, and the
  // cursor descends from there. For the range coder that is the substream's
  // first emitted byte, which rc_Write put first in the file; for rANS it is
  // its LAST emitted byte, which rc_Write put last -- the same address either
  // way, because the two encoders write their rows in opposite directions.
  void StartDecode( byte* top ) {
    beg = ptr = top; end = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  // encoder: payload bytes written, i.e. this lane's substream length. A lane
  // that coded nothing can stop inside the prefix -- 16 header bits over 64
  // lanes leaves most of them with no work -- and owes the stream nothing.
  // rANS has no prefix and grows the other way, so its length is just the
  // distance the cursor travelled; every lane owes at least its four-byte
  // state flush.
#if RC_RANS
  uint rc_len( void ) const { return uint(ptr-beg); }
#else
  uint rc_len( void ) const { return ptr<beg ? uint(beg-ptr) : 0; }
#endif
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
#if RC_RANS
  // rANS never reads past the end of its own substream, so the pad is not
  // there to be read -- only to be READABLE: the refill is a 32-bit load three
  // bytes below the descending cursor, and the cursor's last step is the
  // bottom of the chunk.
  //
  // It does not read the whole substream either. The decoder renormalises at
  // the HEAD of a step, so the refill before step i undoes the encoder's
  // renormalisation before ITS step i-1 -- and the encoder's very first
  // renormalisation, on the block's last bit, has no step after it to undo it.
  // Those 0..2 bytes sit at the bottom of the row and are never consumed.
  // Harmless, and only reachable when that bit's frequency is under
  // SCALE>>RC_RANS_KLOG; the alternative is renormalising after the step
  // instead, which costs more than the bytes are worth (see rans.inc).
  static_assert( FF_PADSIZE >= 3,
    "the rANS refill loads 32 bits at tmpptr-3 and the cursor descends to the "
    "bottom of the chunk; RC_FF_PADSIZE must cover the three bytes below it" );
  // Its encoder runs the other way, from the row's base upwards, and its store
  // is a dword of which only the low bytes are committed -- so the row also
  // has to hold three bytes past the longest possible substream.
  static_assert( RC_ROWTOP + RC_FF_PADSIZE >= 3,
    "the rANS encoder's dword store overshoots the substream by three bytes; "
    "the row must have room for them" );
#else
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
#endif

  // Worst case output for one lane: bits_per_lane (plus the 17 block-length
  // header bits, which all land in the low lanes) at SCALElog bits each, since
  // the model's p is clamped to [1,SCALE-1] and so one bit can cost at most
  // SCALElog bits. Plus the flush.
  //
  // The same bound holds for rANS and for the same reason: a symbol of
  // frequency f multiplies the state by at most M/f <= M, i.e. it can push at
  // most SCALElog bits out, and the flush is four bytes against the range
  // coder's RC_LOWBYTES (>=4). So this over-estimates rANS rather than
  // under-estimating it, which is the direction that is safe.
  //
  // Capped at 0xFFFF because the length header is two bytes: with the default
  // geometry the true worst case (61543 at BLKSIZE=64K, RCNUM=16) is below the
  // cap anyway, but a low RCNUM would push it over, and there the cap plus the
  // vector path's length check turns a silent overrun into a loud error.
  static const uint rc_worst = ((BLKSIZE*8+RCNUM-1)/RCNUM + 17)*SCALElog/8
                             + RC_LOWBYTES + 64;
  static const uint tmpbufsize = macro_Min( rc_worst, 0xFFFFu );

  // Largest BLKSIZE this RCNUM can describe. Below it, rc_worst fits the length
  // header and a substream can never overflow whatever the data does.
  static const uint blkmax = (0xFFFFu - RC_LOWBYTES - 96)*RCNUM/SCALElog;

  // Unconditional: it costs nothing at run time and it is what makes the
  // runtime overflow guard unnecessary -- below blkmax a substream cannot
  // overflow whatever the data does, so there is nothing left to check for.
  static_assert( uint(BLKSIZE) <= blkmax,
    "BLKSIZE is too large for RCNUM: one lane's worst-case output would not fit "
    "the 2-byte substream length header. Lower RC_BLKSIZE or raise RC_RCNUM." );

  // Row stride of tmpbuf. The OpenCL path writes its lanes into rows of
  // exactly this pitch, so the payload readback lands where rc_Write expects.
  // Rounded up to 4 so a row start is dword-aligned: the kernel writes each
  // lane's substream a dword at a time, because AVX-512 has no byte scatter --

  static const uint rowsize = ((RC_ROWTOP+tmpbufsize+FF_PADSIZE)+3) & ~3u;

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
      // the decoder reads downward, so the over-read pad sits BELOW the chunk.
      // The range coder READS this run -- rc_Quit leaves the trailing 0xFFs
      // out and the decoder reconstructs them from here. rANS never reads past
      // its own last byte and only needs the three bytes its refill load
      // reaches below the cursor to be mapped and defined.
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
        // The range coder's payload spans [rowsize-RC_PAYTOP-l, rowsize-1-
        // RC_PAYTOP]: its top byte sits RC_PAYTOP below the row top (above it,
        // the zero prefix), and the ascending copy emits it
        // last-emitted-first. rANS wrote its row upwards from the base, so the
        // same ascending copy emits it first-emitted-first -- which, since a
        // LIFO decoder consumes in reverse, is the same thing.
#if RC_RANS
        memcpy( (prbyte)C.outptr, &tmpbuf[rcidx][i], n );
#else
        memcpy( (prbyte)C.outptr, &tmpbuf[rcidx][rowsize-RC_PAYTOP-l+i], n );
#endif
        C.outptr += n;
      }
    } // rcidx
  }

};
