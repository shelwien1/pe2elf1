
//  The RCNUM-lane rangecoder of sh_v1xN.inc, faked.
//
//  sh_v1xN.inc kept one array per field (range[RCNUM], lowc[RCNUM], ...) so the
//  compiler could put a lane in each SIMD element and run rc_Process on all
//  RCNUM of them at once; that is what the perl-generated macro includes were
//  for, and why every method took an rcidx. This does the same job with RCNUM
//  independent scalar coders from rc.inc instead -- so the interface, the lane
//  assignment and the resulting substream layout are unchanged, and only the
//  vectorisation is gone.
//
//  t_RC is one of the two classes rc.inc produces: the carryless fast coder or
//  its carry-propagating twin.

template< int BLKSIZE, int RCNUM, class t_RC >
struct RangecoderN {

  static const int  f_DEC    = t_RC::RC_f_DEC;
  static const uint CODBYTES = t_RC::CODBYTES;
  static const uint LOWBYTES = t_RC::LOWBYTES;

  ALIGN(VECSIZE) t_RC rc[RCNUM];

  // Bind every lane to its own substream buffer in the RCio and reset it.
  template< class IO >
  void rc_Init( IO& io, uint slot=0 ) {
    for( uint i=0; i<RCNUM; i++ ) {
      if( f_DEC==0 ) rc[i].StartEncode( io.buf(i,slot), IO::tmpbufsize );
      else           rc[i].StartDecode( io.buf(i,slot) );
    }
  }

  INLINE
  void rc_Process( uint rcidx, uint freq, uint& bit ) {
    rc[rcidx].rc_Process( freq, bit );
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) for( uint i=0; i<RCNUM; i++ ) rc[i].FinishEncode();
  }

  uint rc_len( uint rcidx ) { return rc[rcidx].rc_len(); }

  // Carryless only: carries that escaped the low accumulators over this block.
  // Non-zero means this coder's output is not decodable and model0.inc has to
  // re-encode the block with the carry-propagating twin. Always 0 for that
  // twin, so the same call is safe on both.
  uint rc_Carries( void ) {
    uint c = 0;
    for( uint i=0; i<RCNUM; i++ ) c += rc[i].carry_lost();
    return c;
  }

  // A lane whose output did not fit its buffer. Only reachable by configuring
  // BLKSIZE/RCNUM past what a 2-byte length header can describe.
  uint rc_Overflow( void ) {
    uint c = 0;
    for( uint i=0; i<RCNUM; i++ ) c += rc[i].f_OVF;
    return c;
  }

};
