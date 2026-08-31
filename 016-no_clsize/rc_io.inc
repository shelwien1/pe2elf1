
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
  void put( byte c ) {
    *ptr-- = c;
  }

  // top is the highest byte of the lane's row (where the first emitted byte
  // goes); bufsize is the payload capacity, and the prefix sits above it --
  // RCio::rowsize covers both plus the pad below.
  void StartEncode( byte* top, uint bufsize ) {
    ptr = top; beg = top-RC_SKIP; end = beg-bufsize;
    ((Rangecoder*)this)->rc_Init();
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  // top is the payload's first byte: its highest address, since rc_Read put
  // the chunk back in file order and the file holds it last-emitted-first.
  void StartDecode( byte* top ) {
    beg = ptr = top; end = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  // encoder: payload bytes written, i.e. this lane's substream length. A lane
  // that coded nothing can stop inside the prefix -- 16 header bits over 64
  // lanes leaves most of them with no work -- and owes the stream nothing.
  uint rc_len( void ) const { return ptr<beg ? uint(beg-ptr) : 0; }

  // rc_Quit drops as many flush bytes as the fill allows, and rc_Read's 0xFF
  // padding stands in for them; the length in the block header is what tells
  // the reader where the substream ended. See RC_HDR_IO for the stream that
  // has neither.
  static const uint QUIT_ALIGN = 0;
};


//----------------------------------------------------------------------------
//  The header coder's cursor: bytes straight to and from the coroutine's
//  stream, in emission order.
//
//  RC_IO above runs its substream backwards and lets rc_Quit drop as many
//  flush bytes as the interval allows, because the block header carries the
//  length: the reader knows where the payload ends before it starts. The
//  header stream is the one that cannot -- its length is what it is on its way
//  to describing -- so it is the other way round on both counts.
//
//  Forwards, because a backwards reader has to start at the last byte, and
//  that position IS the length. Here the reader starts at the first byte and
//  walks up.
//
//  And with rc_Quit's ALIGNED flush rather than its 0xFF one, which is where
//  the byte count comes from. A reader consumes CODBYTES at rc_Init plus one
//  byte per renorm; a writer emits one byte per renorm plus the flush, less
//  the RC_SKIP zero prefix. The renorm counts are the same sequence on both
//  sides, so
//
//      bytes read = bytes written + LOWBYTES - (flush bytes)
//
//  and the flush length settles the over-read. The reader cannot be told what
//  it was -- that is the length again -- so it has to derive it, and rc_Quit's
//  j = (bsr(range)-1)/8 is derived from the one value both sides hold at the
//  flush. The reader mirrors the flush's renorm (FinishDecode) and asks its
//  own `range` the same question; the j bytes it over-read are the payload's
//  first, kept in tail[] for rc_Read to put back.
//
//  Why the flush has to be the aligned one, and not the cheaper 0xFF one that
//  every payload substream uses: rc_Quit drops trailing bytes that the fill
//  made 0xFF, and a payload row can afford that because rc_Read pads below it
//  with 0xFF -- the reader running off the end reads back exactly what was
//  dropped. This stream has no pad under it. What follows is the payload, so a
//  reader running off the end folds THOSE bytes into `code` in place of the
//  0xFF the writer assumed, which drops the reconstructed value below `low`
//  whenever they are smaller and flips the last bits decoded. Measured, when
//  this stream still used the 0xFF flush: at RCNUM=32 the head byte of the
//  last lane of block 9 of enwik8 came back 0x00 for 0x01 -- one bit, from a
//  three-byte over-read landing on payload. The aligned flush is the fix: it
//  drops only bytes that any value may occupy.
//----------------------------------------------------------------------------
template< class Rangecoder >
struct RC_HDR_IO {

  static const uint QUIT_ALIGN = 1;

  Coroutine* C;
  uint  nio;                 // bytes read (decode) or emitted (encode) so far
  uint  jdrop;               // decode: how many of tail[] are the payload's
  byte  tail[RC_CODBYTES];   // decode: the last bytes read, newest last

  byte get( void ) {
    const byte c = byte(C->get());
    for( uint i=0; i+1<RC_CODBYTES; i++ ) tail[i] = tail[i+1];
    tail[RC_CODBYTES-1] = c;
    nio++;
    return c;
  }

  // The RC_SKIP leading bytes are low's zero prefix, the same one rc_Write
  // drops from a lane's row -- the accumulator is LOWBYTES wide and only its
  // bottom CODBYTES are live, so its top RC_SKIP bytes leave before anything
  // has been added to them. They are not part of the stream.
  void put( byte c ) {
    if( nio>=RC_SKIP ) C->put( c );
    nio++;
  }

  void StartEncode( Coroutine& c ) {
    C = &c; nio = 0;
    ((Rangecoder*)this)->rc_Init();
  }

  void StartDecode( Coroutine& c ) {
    C = &c; nio = 0; jdrop = 0;
    for( uint i=0; i<RC_CODBYTES; i++ ) tail[i] = 0xFF;
    ((Rangecoder*)this)->rc_Init();   // reads CODBYTES bytes
  }

  void FinishEncode( void ) { ((Rangecoder*)this)->rc_Quit(); }

  // rc_Quit opens with a renorm, to apply the last pending rpre; rc_Quit's
  // body is encoder-only, so the reader has to make that renorm itself or it
  // stops one shift short of the writer and the byte count does not line up.
  // Then `range` is what it was when the writer chose its flush length, and
  // asking it the same question gives back the same j.
  void FinishDecode( void ) {
    ((Rangecoder*)this)->rc_Renorm();
    jdrop = ((Rangecoder*)this)->quit_drop();
  }

  // decode: the over-read bytes, oldest first -- the payload's first jdrop
  const byte* spill( void ) const { return tail + (RC_CODBYTES-jdrop); }

  uint rc_len ( void ) const { return (nio>RC_SKIP) ? nio-RC_SKIP : 0; }  // encode
  uint rc_read( void ) const { return nio; }                              // decode

  // n bits, top first, against a run of n counters -- one per bit position.
  // The counters are the caller's and live across blocks; see RCio::hdrctr.
  void PutBits( uint v, uint n, Counter* __restrict c ) {
    for( ; n!=0; n--,c++ ) {
      uint b = (v>>(n-1))&1;         // rc_Process takes the bit by reference
      const uint s = c->Get();
      ((Rangecoder*)this)->rc_Process( Counter::P(s), b );
      c->Update( s, b );
    }
  }

  // nb bits, top first: the top `nctx` through a binary-tree context -- the
  // counter picked by the bits already sent, not by position -- and the rest
  // at p=1/2 with no counter. The tree index runs 1..(1<<nctx)-1, so the table
  // is 1<<nctx entries with index 0 unused.
  void PutCtx( uint v, uint nb, uint nctx, Counter* __restrict t ) {
    uint j, c = 1;
    for( j=0; j<nctx; j++ ) {
      uint b = (v>>(nb-1-j))&1;
      const uint s = t[c].Get();
      ((Rangecoder*)this)->rc_Process( Counter::P(s), b );
      t[c].Update( s, b );
      c = c*2 + b;
    }
    for( ; j<nb; j++ ) {
      uint b = (v>>(nb-1-j))&1;
      ((Rangecoder*)this)->rc_Process( hSCALE, b );
    }
  }

  uint GetCtx( uint nb, uint nctx, Counter* __restrict t ) {
    uint j, b, c = 1, v = 0;
    for( j=0; j<nctx; j++ ) {
      const uint s = t[c].Get();
      ((Rangecoder*)this)->rc_Process( Counter::P(s), b );
      t[c].Update( s, b );
      c = c*2 + b; v = v*2 + b;
    }
    for( ; j<nb; j++ ) {
      ((Rangecoder*)this)->rc_Process( hSCALE, b );
      v = v*2 + b;
    }
    return v;
  }

  uint GetBits( uint n, Counter* __restrict c ) {
    uint v = 0;
    for( ; n!=0; n--,c++ ) {
      uint b; const uint s = c->Get();
      ((Rangecoder*)this)->rc_Process( Counter::P(s), b );
      c->Update( s, b );
      v = v*2 + b;
    }
    return v;
  }
};


//----------------------------------------------------------------------------
//  Block I/O. Compressed format:
//
//    per block:  header stream -- RCNUM x 16-bit substream length, then one
//                8-bit head byte per NON-EMPTY substream, in lane order,
//                through a rangecoder of their own (see below)
//                RCNUM x substream payload minus its head byte, in lane order
//
//  The head byte is a substream's first byte in the file, which -- the
//  substreams run backwards, see RC_IO above -- is the LAST byte rc_Quit
//  writes, the one whose low bits the flush fills with 1s. It is the most
//  skewed byte in the block by a wide margin (order-0 entropy 6.45 bits
//  against ~8 for the payload; misc/flush_bits.md has the distribution), which
//  is why it is up here with the lengths and not down in the payload.
//
//  The header goes through its own scalar carry-propagating rangecoder --
//  RC_HDR_IO above is its cursor -- against per-lane counters carried across
//  blocks: a binary-tree context for the length's high byte, a counter per bit
//  position for everything else. See hdrctr/hdrhi below.
//
//  The header stream's own length is written nowhere, and needs no marker: the
//  field layout is fixed, so the reader knows when to stop, and RC_HDR_IO's
//  full flush makes it stop on the writer's last byte exactly. The payload
//  starts at the next one.
//
//  A lane that coded nothing has no head byte and contributes none -- the
//  lengths come first, so both sides agree on which lanes are in the row.
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

  // The header model: a counter per bit position per lane, carried across
  // blocks. A lane's own history is what makes this worth having -- the lanes
  // are not interchangeable, they are fixed slices of the bit stream with
  // fixed jobs, and their lengths sit in tight bands a long way apart (over
  // enwik8 at RCNUM=16, lane 0 averages 127 bytes and lane 5 averages 3523).
  // One shared set of 24 sees the union of those bands and learns almost
  // nothing about the top bits; per lane it is 8.9 bits a length instead of
  // 11.5, and the whole header 48 KB instead of 56 KB against 73 KB raw.
  //
  // No context beyond the position: a bit does not see the bits above it in
  // its own value. That is the next thing to try, not a claim that it does not
  // matter -- see misc/flush_bits.md.
  //
  // The head byte is not coded whole. Its trailing run of 1 bits is k%8 of the
  // flush that wrote it -- exactly, because rc_Quit's fill stops at a k whose
  // bit in low is 0 -- so the run is sent first, in its own 3-bit field, and
  // the byte's bottom r+1 bits then follow from it: r ones and the zero that
  // ended the run. Only bits 7..r+1 are left to code. Coding the run and the
  // remainder separately is the byte's own entropy either way, but a
  // position-only model cannot see that structure inside a byte, so splitting
  // it out is worth 0.35 bits a head. r is capped at 7 rather than 8: an
  // all-ones head byte cannot come out of a payload lane's flush (that byte
  // would have been dropped), but the carry twin's degenerate n==0 flush can
  // end on one, and at r==7 the top bit is coded like any other.
  // The length gets a context instead of a position: a binary tree over the
  // bits already sent, RC_HDR_CTXBITS deep, per lane. It is the field with a
  // shape worth conditioning on -- a lane's lengths sit in a narrow band, so
  // its high byte takes 5 to 10 distinct values with two dominating, and a
  // position-only model pays for the top bits over and over. Below the tree
  // the bits are random and go out at p=1/2 with no counter at all, which
  // measures BETTER than a position counter there: an adaptive counter on a
  // random bit costs more than it saves. See RC_HDR_CTXBITS.
  static const uint HDR_LENBITS  = 16;
  static const uint HDR_CTXBITS  = RC_HDR_CTXBITS;
  static const uint HDR_RUNBITS  = 3;                        // k%8, capped at 7
  static const uint HDR_HEADBITS = 8;                        // the head byte
  static const uint HDR_RUNPOS   = 0;
  static const uint HDR_HEADPOS  = HDR_RUNPOS + HDR_RUNBITS;
  static const uint HDR_BITS     = HDR_HEADPOS + HDR_HEADBITS;
  Counter hdrctr[RCNUM][HDR_BITS];
  Counter hdrlen[RCNUM][1u<<HDR_CTXBITS];                    // the length tree

  // Where the head byte's coded part starts, given the run. Below 7 the run's
  // own terminating zero is bit r, so bits 7..r+1 are free; at 7 there is no
  // terminating zero inside the byte and bit 7 is free. The counters are
  // indexed from bit 7 down, so a bit always meets the same counter.
  static uint hdr_headsh( uint r ) { return (r<7) ? r+1 : 7; }

  // NOINLINE for model_pass's reason (model0.inc): this runs once per file,
  // but its address arithmetic inside do_process costs registers in the loop
  // that runs 800 million times. Inline it and the decoder loses 2%.
  NOINLINE
  void hdr_Init( void ) {
    for( uint i=0; i<RCNUM; i++ ) {
      for( uint j=0; j<HDR_BITS; j++ ) hdrctr[i][j].Reset();
      for( uint j=0; j<(1u<<HDR_CTXBITS); j++ ) hdrlen[i][j].Reset();
    }
  }

  byte* buf( uint rcidx ) { return &tmpbuf[rcidx][0]; }
  byte* rows( void ) { return &tmpbuf[0][0]; }

  // The header, out of line, on both sides. It is RCNUM x ~27 coded bits once
  // a block against a 32 KB counter table, next to nothing beside the payload
  // it describes -- but inlined into rc_Read/rc_Write its counter walks make
  // those functions several times bigger, and that moves do_process's hot
  // loops. Which is worth a few percent either way on this build; see
  // misc/flush_bits.md. Out of line they cannot.
  //
  // Fills rclen[] and head[], and hands back the bytes the header stream's
  // reader over-read: they are the payload's first, and rc_Read puts them back
  // in front of it.
  template< class t_Hdr >
  NOINLINE
  void hdr_Read( Coroutine& C, byte* __restrict head, byte* __restrict spill, uint& spn ) {
    uint rcidx, l;

    t_Hdr hdr;
    hdr.StartDecode( C );
    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = hdr.GetCtx( HDR_LENBITS, HDR_CTXBITS, hdrlen[rcidx] );
      // A valid stream never exceeds tmpbufsize; a corrupt header could, and
      // the payload copy and the FF padding below both index by it.
      rclen[rcidx] = Min<uint>( l, tmpbufsize );
    }
    // the run row, then the head row: one byte per non-empty lane, put back at
    // the bottom of the lane's chunk -- where rc_Write lifted it from
    uint hrun[RCNUM];
    for( rcidx=0; rcidx<RCNUM; rcidx++ )
      hrun[rcidx] = rclen[rcidx] ? hdr.GetBits( HDR_RUNBITS, hdrctr[rcidx]+HDR_RUNPOS ) : 0;

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      if( rclen[rcidx]==0 ) { head[rcidx] = 0xFF; continue; }
      const uint r  = hrun[rcidx];
      const uint sh = hdr_headsh(r);
      head[rcidx] = byte( (hdr.GetBits( HDR_HEADBITS-sh, hdrctr[rcidx]+HDR_HEADPOS )<<sh) | ((1u<<r)-1) );
    }
    hdr.FinishDecode();

    spn = hdr.jdrop;
    for( uint i=0; i<spn; i++ ) spill[i] = hdr.spill()[i];
  }

  template< class t_Hdr >
  NOINLINE
  void rc_Read( Coroutine& C ) {
    uint rcidx,i,l,n;

    // ...and the bytes the header stream's reader over-read go back in front
    // of the payload. They can span lanes, so this is a prefix over the whole
    // run rather than a fix-up on lane 0. At the end of the file there may be
    // nothing for them to go back into, and they will be the 0xFF that C.get()
    // gives past EOF -- which is what the writer's aligned flush allowed for.
    byte head[RCNUM], spill[RC_CODBYTES];
    uint spn;
    hdr_Read<t_Hdr>( C, head, spill, spn );
    const byte* sp = spill;

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rclen[rcidx];
      if( l ) {
        tmpbuf[rcidx][FF_PADSIZE] = head[rcidx];
        i = 0;
        for( ; spn && (i<l-1); spn--,i++ ) tmpbuf[rcidx][FF_PADSIZE+1+i] = *sp++;
        for( ; i<l-1; i+=n ) {
          C.chkinp(); n = Min<uint>( l-1-i, C.getinplen() );
          // truncated stream: chkinp has nothing left to hand us. Stop rather
          // than spin -- the FF padding below fills the hole and the block
          // decodes to garbage, which is the best a truncated file can do.
          if_e0( n==0 ) break;
          memcpy( &tmpbuf[rcidx][FF_PADSIZE+1+i], (prbyte)C.inpptr, n );
          C.inpptr += n;
        }
      }
      // the decoder reads downward, so the over-read pad sits BELOW the chunk
      for( i=0; i<FF_PADSIZE; i++ ) tmpbuf[rcidx][i] = 0xFF;
    } // rcidx
  }

  // The header, out of line -- see hdr_Read above for why. Fills rclen[].
  template< class t_Hdr, class RCV >
  NOINLINE
  void hdr_Write( Coroutine& C, RCV& rc, uint* __restrict rclen ) {
    uint rcidx, l;

    t_Hdr hdr;
    hdr.StartEncode( C );
    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rc.rc_len(rcidx);
      hdr.PutCtx( l, HDR_LENBITS, HDR_CTXBITS, hdrlen[rcidx] );
      rclen[rcidx] = l;
    }

    // The head bytes. The payload spans [rowsize-RC_SKIP-l, rowsize-1-RC_SKIP]:
    // its top byte sits RC_SKIP below the row top (above it, the zero prefix),
    // and the ascending copy below emits it last-emitted-first -- so the byte
    // at the BOTTOM of that span is the one the file would lead with, and it
    // is the one that comes up here instead.
    //
    // The run row first: r is the byte's trailing 1s, which is k%8 of the
    // flush that wrote it, and it fixes the byte's bottom r+1 bits.
    byte hbyte[RCNUM]; uint hrun[RCNUM];
    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rclen[rcidx];
      if( l==0 ) continue;
      const uint b = tmpbuf[rcidx][rowsize-RC_SKIP-l];
      uint r = 0; while( (r<7) && ((b>>r)&1) ) r++;
      hbyte[rcidx] = byte(b); hrun[rcidx] = r;
      hdr.PutBits( r, HDR_RUNBITS, hdrctr[rcidx]+HDR_RUNPOS );
    }

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      if( rclen[rcidx]==0 ) continue;
      const uint sh = hdr_headsh( hrun[rcidx] );
      hdr.PutBits( hbyte[rcidx]>>sh, HDR_HEADBITS-sh, hdrctr[rcidx]+HDR_HEADPOS );
    }
    hdr.FinishEncode();
  }

  template< class t_Hdr, class RCV >
  NOINLINE
  void rc_Write( Coroutine& C, RCV& rc ) {
    uint rcidx,i,l,n;

    uint rclen[RCNUM];
    hdr_Write<t_Hdr>( C, rc, rclen );

    for( rcidx=0; rcidx<RCNUM; rcidx++ ) {
      l = rclen[rcidx];
      if( l==0 ) continue;
      for( i=0; i<l-1; i+=n ) {
        C.chkout(); n = Min<uint>( l-1-i, C.getoutlen() );
        memcpy( (prbyte)C.outptr, &tmpbuf[rcidx][rowsize-RC_SKIP-l+1+i], n );
        C.outptr += n;
      }
    } // rcidx
  }

};
