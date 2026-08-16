// -------------------------------------------------------------
// sh_rcm.inc -- mrpc's rangecoder, in the shape of sh_rcv7.inc.
//
// sh_v2f.inc is a bit-oriented carryless coder: it renormalises ONE BIT at
// a time (inpbit/outbit assemble the bytes by hand), and each multi-symbol
// operation costs three 64-bit divisions -- one in rc_GetFreq and two in
// rc_Process, because `range - (tot-cum)*range/tot` cannot share its
// divide with `range - (tot-cum-freq)*range/tot`.  At ~2.6 bits a symbol
// that is ~2.6 trips through a bit loop plus three divisions per symbol,
// and on mrpc's decoder it was over half the time.
//
// This is the same coder said the other way:
//
//   * the range is renormalised a BYTE at a time (sTOP = 2^24), so the
//     loop runs about a third of a time per symbol instead of 2.6 times,
//     and each trip is a getc/putc rather than bit assembly;
//   * one division per symbol -- r = range/tot once, then multiplies --
//     and the binary path has none at all, being a multiply-and-shift
//     exactly as sh_rcv7.inc writes it.
//
// The price is that the range floor is 2^24 rather than 2^31, so the
// frequency total has to come down with it (see MAX_TOTFREQ in mrpc.cpp)
// or the truncation `range = (range/tot)*tot` throws away real bits.
//
// It is a separate file because sh_v2f.inc is shared with coder0 and bmpc,
// whose streams must not move.
// -------------------------------------------------------------

#ifndef RCM_PREC
#define RCM_PREC 1
#endif

struct RangecoderM {
  static const uint sTOP  = 1u<<24;
  static const uint Thres = 0xFF000000u;

  FILE* f;
  uint  f_DEC;
  uint  range, code, FFNum, Cache;
  qword lowc;

  // FinishEncode stops as soon as low is all-ones, i.e. it omits the
  // trailing 0xFF bytes; the decoder has to supply them.  Returning 0 past
  // the end instead costs the last few symbols of every stream.
  INLINE byte get() { int c = getc(f); return byte(c<0 ? 0xFF : c); }
  INLINE void put( uint c ) { putc( int(c&0xFF), f ); }

  void StartEncode( FILE* F ) { f=F; f_DEC=0; Init(); }
  void StartDecode( FILE* F ) { f=F; f_DEC=1; Init(); }
  void Init( void ) {
    range = 0xFFFFFFFFu; lowc = 0; FFNum = 0; Cache = uint(-1); code = 0; r_ = 1;
    if( f_DEC ) for( uint i=0; i<4; i++ ) code = (code<<8) + get();
  }

  INLINE void ShiftLow( void ) {
    uint Carry = uint(lowc>>32), low = uint(lowc);
    if( low<Thres || Carry ) {
      if( Cache!=uint(-1) ) put( Cache+Carry );
      for( ; FFNum!=0; FFNum-- ) put( Carry-1 );
      Cache = low>>24;
    } else FFNum++;
    lowc = qword(low<<8);
  }

  // sh_rcv7.inc's Quit: settle low to the shortest value still inside
  // [low, low+range) rather than flushing all NUM bytes.
  void FinishEncode( void ) {
    qword nlow, high = lowc + range;
    nlow = lowc|0xFFull;       if( nlow<high ) lowc = nlow;
    nlow = lowc|0xFFFFull;     if( nlow<high ) lowc = nlow;
    nlow = lowc|0xFFFFFFull;   if( nlow<high ) lowc = nlow;
    nlow = lowc|0xFFFFFFFFull; if( nlow<high ) lowc = nlow;
    if( lowc==0xFFFFFFFFull ) FFNum = 0;
    uint Carry = uint(lowc>>32), low = uint(lowc);
    if( Cache!=uint(-1) ) put( Cache+Carry );
    for( ; FFNum!=0; FFNum-- ) put( Carry-1 );
    while( low!=0xFFFFFFFFu ) { put( low>>24 ); low = (low<<8)|0xFF; }
  }

  // --- multi-symbol -------------------------------------------------
  // `r = range/tot` then `range = r*freq` gives every symbol an interval
  // of r*freq where an exact coder would give range*freq/tot, so each one
  // is short by up to tot/range -- with tot at 2^20 and the byte-renorm
  // floor at 2^24 that is ~0.3% of the file, which is not free.
  //
  // RCM_PREC carries 16 more bits in r: one division still, 64-bit
  // multiplies (three cycles) instead of 32-bit, and the shortfall drops
  // by 2^16.  The top symbol takes the remainder either way.
#if RCM_PREC
  typedef qword rtype;
  INLINE rtype Ratio( uint tot ) { return (qword(range)<<16)/tot; }
  INLINE uint  Span( rtype r, uint f ) { return uint((r*f)>>16); }
#else
  typedef uint rtype;
  INLINE rtype Ratio( uint tot ) { return range/tot; }
  INLINE uint  Span( rtype r, uint f ) { return uint(r)*f; }
#endif
  rtype r_;

  INLINE void Encode( uint cum, uint freq, uint tot ) {
    rtype r = Ratio(tot);
    uint  c = Span(r,cum);
    lowc += c;
    range = (cum+freq==tot) ? range - c : Span(r,cum+freq) - c;
    while( range<sTOP ) { range <<= 8; ShiftLow(); }
  }
  // GetFreq keeps r for the Decode that must follow it
  INLINE uint GetFreq( uint tot ) {
    r_ = Ratio(tot);
#if RCM_PREC
    // the largest v with Span(r_,v) <= code, which is NOT (code<<16)/r_:
    // Span truncates, so the inverse has to round the other way or the
    // symbol at an interval boundary comes out one too low
    uint v = uint( ((qword(code)<<16) + 0xFFFF) / r_ );
#else
    uint v = code/r_;
#endif
    return (v>=tot) ? tot-1 : v;    // the top symbol owns the remainder
  }
  INLINE void Decode( uint cum, uint freq, uint tot ) {
    uint c = Span(r_,cum);
    code -= c;
    range = (cum+freq==tot) ? range - c : Span(r_,cum+freq) - c;
    while( range<sTOP ) { range <<= 8; code = (code<<8) + get(); }
  }

  // --- binary, for the header and fallback models -------------------
  // freq is P(bit==0) out of SCALE.  No division: sh_rcv7.inc's Process.
  INLINE uint BProcess( uint freq, uint bit ) {
    uint rnew = uint( (qword(range) * (qword(freq)<<(32-SCALElog))) >> 32 );
    if( f_DEC ) bit = (code>=rnew);
    if( bit ) {
      range -= rnew;
      if( f_DEC ) code -= rnew; else lowc += rnew;
    } else range = rnew;
    while( range<sTOP ) {
      range <<= 8;
      if( f_DEC ) code = (code<<8) + get(); else ShiftLow();
    }
    return bit;
  }
};
