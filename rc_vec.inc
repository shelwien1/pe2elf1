
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

#if RC_DECSOA_ON
  // Decode-side lane state, SoA: what the original kept as range[RCNUM] and
  // friends, and what the kernels keep in vector registers. The rc[] objects
  // above go unused on decode; the arithmetic below is rc.inc's rc_Process /
  // ShiftCode line for line, reading the same bytes in the same order.
  ALIGN(64) uint d_range[RCNUM], d_code[RCNUM], d_rpre[RCNUM], d_pos[RCNUM];
  byte* d_base;
#endif

  // Bind every lane to its own substream buffer in the RCio and reset it.
  template< class IO >
  void rc_Init( IO& io ) {
#if RC_DECSPLIT_ON
    s_bulk = 0;   // header bits run fused; rc_Bulk switches the mode
#endif
#if RC_DECSOA_ON
    if( f_DEC ) {
      d_base = io.rows();
      for( uint i=0; i<RCNUM; i++ ) {
        uint q = uint( io.buf(i) - d_base );
        uint c = 0;
        for( uint k=0; k<t_RC::CODBYTES; k++ ) c = (c<<8) + d_base[q++];
        d_range[i] = 0xFFFFFFFFu; d_rpre[i] = 0;
        d_code[i] = c; d_pos[i] = q;
      }
      return;
    }
#endif
    for( uint i=0; i<RCNUM; i++ ) {
      if( f_DEC==0 ) rc[i].StartEncode( io.buf(i), IO::tmpbufsize );
      else           rc[i].StartDecode( io.buf(i) );
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
  void rc_Bulk( IO& io ) {
    s_base = d_base;
    (void)io;
    for( uint i=0; i<RCNUM; i++ ) {
      uint r=d_range[i], c=d_code[i], q=d_pos[i];
      uint n = (r<t_RC::sTOP) + (r<t_RC::gTOP);
      c -= d_rpre[i];
      for( uint k=n; k!=0; k-- ) c = (c<<8) + s_base[q++];
      r <<= n*8;
      s_range[i] = r;
      s_code[i]  = c;
      s_pos[i]   = q;
      s_ru[i]    = r>>SCALElog;
      s_T[i]     = c/s_ru[i];
      s_look[i]  = s_base[q] | (uint(s_base[q+1])<<8)
                 | (uint(s_base[q+2])<<16) | (uint(s_base[q+3])<<24);
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
#if RC_DECSOA_ON
    if( f_DEC ) {
      uint r=d_range[rcidx], c=d_code[rcidx];
      c -= d_rpre[rcidx];
      // Branchy renorm, the original Rangecoder1's: ~91% of bits refill
      // nothing and pay two predicted-not-taken tests, where the counted
      // branchless form pays its compares, the variable shift and the
      // bounds arithmetic on every bit. On the scalar decode path branchy
      // wins -- the same measurement the original made (RC.txt, 054) and
      // this port initially ignored.
      //
      // The cursor is touched only inside the refill -- nothing on the ~91%
      // no-refill path. This shape is tuned for clang (34.9 -> 36.5 on the
      // reference box, and it needs RC_DEC_LOCAL): gcc prefers an
      // unconditional load/store of the cursor (this form costs it a third),
      // but clang is what this coder is built with.
      if_e0( r<t_RC::sTOP ) {
        uint q = d_pos[rcidx];
        if_e0( r<t_RC::gTOP ) {
          r <<= 16;
          c = (c<<16) + (uint(d_base[q])<<8) + d_base[q+1];
          d_pos[rcidx] = q+2;
        } else {
          r <<= 8;
          c = (c<<8) + d_base[q];
          d_pos[rcidx] = q+1;
        }
      }
      uint rp = (r>>SCALElog)*freq;
      bit = (c>=rp);
      r -= rp;
      d_range[rcidx] = bit ? r : rp;
      d_rpre[rcidx]  = rp & (0-bit);
      d_code[rcidx]  = c;
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
