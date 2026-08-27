
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

#if RC_DECSPLIT_ON
#include "rc_kernel_ispc.h"
#endif

template< int BLKSIZE, int RCNUM, class t_RC >
struct RangecoderN {

  static const int  f_DEC    = t_RC::RC_f_DEC;
  static const uint CODBYTES = t_RC::CODBYTES;
  static const uint LOWBYTES = t_RC::LOWBYTES;

  ALIGN(VECSIZE) t_RC rc[RCNUM];

  // Bind every lane to its own substream buffer in the RCio and reset it.
  template< class IO >
  void rc_Init( IO& io, uint slot=0 ) {
#if RC_DECSPLIT_ON
    s_bulk = 0;   // header bits run fused; rc_Bulk switches the mode
#endif
    for( uint i=0; i<RCNUM; i++ ) {
      if( f_DEC==0 ) rc[i].StartEncode( io.buf(i,slot), IO::tmpbufsize );
      else           rc[i].StartDecode( io.buf(i,slot) );
    }
  }

#if RC_DECSPLIT_ON
  // -------------------------------------------------------------------------
  // The decoder split (RC_DECSPLIT, rc_split_ispc_v1.md): per bit only the
  // decision runs here -- one compare against the lane's threshold, exact
  // because  code >= (range>>SCALElog)*p  is  p <= floor(code/(range>>15))
  // for integers -- and the {p;bit} pair is logged. Every 16 bits the ispc
  // batch (rc_decode_batch in rc_kernel.c) brings those 16 lanes current:
  // update, counted renorm, refill, next thresholds. At RCNUM=16 the batch's
  // result feeds the very next group; at RCNUM=64 a group's batch is not
  // consumed for another three, which is what lets it pipeline.
  //
  // The header bits run fused (s_bulk==0): their lane sequence revisits lane
  // 0 immediately, which the group rotation cannot express. rc_Bulk converts
  // the lanes to split state once the model enters the strict i%RCNUM order.
  // A partial group at block end is simply dropped -- every one of its
  // decisions used a threshold no other bit has touched since the last
  // batch, and the block's lanes are never read again.
  ALIGN(64) uint s_range[RCNUM], s_code[RCNUM], s_pos[RCNUM];
  ALIGN(64) uint s_ru[RCNUM], s_T[RCNUM], s_look[RCNUM];
  word  s_pb[16];
  byte* s_base;
  uint  s_cnt, s_qbase, s_bulk;

  // one lazy step, exactly rc.inc's rc_Renorm+ShiftCode, then the lane's
  // state as the batch keeps it: eager, rpre folded, threshold ready
  template< class IO >
  void rc_Bulk( IO& io, uint slot=0 ) {
    s_base = io.rows(slot);
    for( uint i=0; i<RCNUM; i++ ) {
      t_RC& r = rc[i];
      uint n = (r.range<t_RC::sTOP) + (r.range<t_RC::gTOP);
      r.code -= r.rpre; r.rpre = 0;
      for( uint k=n; k!=0; k-- ) r.code = (r.code<<8) + r.get();
      r.range <<= n*8;
      s_range[i] = uint(r.range);
      s_code[i]  = r.code;
      s_pos[i]   = uint(r.ptr - s_base);
      s_ru[i]    = uint(r.range)>>SCALElog;
      s_T[i]     = s_code[i]/s_ru[i];
      s_look[i]  = r.ptr[0] | (uint(r.ptr[1])<<8)
                 | (uint(r.ptr[2])<<16) | (uint(r.ptr[3])<<24);
    }
    s_cnt=0; s_qbase=0; s_bulk=1;
  }

  void dec_batch( void ) {
    // the pairs cross by value in four qwords; assembling them from the
    // word log stays word-sized, so the stores forward cleanly
    qword pk[4];
    for( uint k=0; k<4; k++ )
      pk[k] = qword(s_pb[k*4])        | (qword(s_pb[k*4+1])<<16)
            | (qword(s_pb[k*4+2])<<32) | (qword(s_pb[k*4+3])<<48);
    uint q = s_qbase;
    ispc::rc_decode_batch( s_range+q, s_code+q, s_pos+q, s_ru+q, s_T+q,
                           s_look+q, s_base, pk[0], pk[1], pk[2], pk[3] );
    s_cnt = 0;
    s_qbase = (q+16)%RCNUM;
  }
#endif

  INLINE
  void rc_Process( uint rcidx, uint freq, uint& bit ) {
#if RC_DECSPLIT_ON
    if( f_DEC && s_bulk ) {
      uint b = (freq <= s_T[rcidx]);
      s_pb[s_cnt] = word((freq<<1)|b);
      bit = b;
      if( ++s_cnt==16 ) dec_batch();
      return;
    }
#endif
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
