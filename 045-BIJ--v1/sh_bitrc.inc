
// ---- sh_bit's bijective bitwise rangecoder, wearing Rangecoder_SH1m's API ------
//
// The coder core (lowc/range/code, the reserved inverse-FO endings, ShiftLow,
// rc_Quit) is sh_bit's, unchanged in substance.  On top of it sit the coding
// primitives CDM's codec.inc calls, reimplemented against this state.
//
// Two structural differences from Rangecoder_SH1m drive every formula below:
//
//   * sh_bit tracks `low` explicitly and leaves `code` as the raw codestream
//     value, because the ending machinery works in absolute codestream
//     coordinates (in_window(v) == uint(v-low()) < range).  SH1m normalises low
//     to zero and subtracts from `code` instead.  So every SH1m `code >= rnew`
//     becomes `code-low() >= rnew`, and every `code -= x` becomes nothing at all
//     -- `lowc += x` alone carries both directions.
//
//   * sh_bit renormalises one BIT at a time with range >= 2^31; SH1m
//     renormalises a BYTE at a time with range >= 2^24.  SH1m's rc_Renorm1 /
//     rc_Renorm2 are byte-granular shortcuts (shift once / at most twice) that
//     are exact only under the byte invariant.  Here they are all the full
//     `while( range<sTOP )` loop: after rb_Process with freq0=1, n=2049 the
//     range can lose 11 bits at a stroke, which two shifts would not recover.
//
// SCALElog/SCALE come from cdm.inc (15 / 1<<15); NUM/sTOP are the coder's own.

template< int f_DEC_ >
struct Rangecoder_SHB : RC_IO< Rangecoder_SHB<f_DEC_> > {

  typedef RC_IO< Rangecoder_SHB<f_DEC_> > t_io;
  using t_io::f_EOF;
  using t_io::bij_get;
  using t_io::bij_put;
  using t_io::io_init;
  using t_io::put_flush;

  // Compile-time direction, the way Rangecoder_SH1m has it: sh_bits carries a
  // runtime f_DEC, which would cost a branch in every primitive of CDM's inner
  // loop.  This shadows the base's member; nothing in RC_IO reads it.
  enum { f_DEC = f_DEC_ };

  enum { NUM = 32, sTOP = 0x80000000U };

  qword lowc;    // interval low = uint(lowc), carry = lowc>>32
  uint  code;    // decoder: raw codestream value
  uint  range;
  uint  FFNum, Cache;

  enum { RESV_MAX = 4090, RESV_DIM = 4096 };
  uint resv[RESV_DIM];
  uint nr;

  uint rc_outbits; // TRACE_ON: codestream bits emitted so far

  // ---- bit I/O ----------------------------------------------------------------
  void inpbit( void ) { code = code*2 + bij_get(); }
  void outbit( uint c ) { bij_put( (byte)c ); rc_outbits++; }

  // ---- reserved inverse-FO endings (sh_bit, verbatim) -------------------------
  uint low( void ) { return uint(lowc); }
  uint in_window( uint v ) { return uint(v-low()) < range; }   // low <= v < low+range

  uint is_reserved( uint v ) {
    for( uint i=0; i<nr; i++ ) if( resv[i]==v ) return 1;
    return 0;
  }

  void reserve( uint e ) { if( nr<RESV_MAX ) resv[nr++] = e; }

  void drop_endings( void ) {
    uint j = 0;
    for( uint i=0; i<nr; i++ ) if( in_window(resv[i]) ) resv[j++] = resv[i];
    nr = j;
  }

  // shortlex-first inverse-FO value in the window, not reserved.
  // t = trailing-1s count = NUM-n; t=NUM is the all-ones value.
  uint find_ending( void ) {
    for( int t=NUM; t>=0; t-- ) {
      uint base = ((1ULL<<t)-1);
      uint cnt  = (1ULL<<31)>>t; cnt += (cnt==0);
      uint step = uint(2ULL<<t);
      cnt = (cnt>4096) ? 4096 : cnt;
      uint v = base;
      for( uint k=0; k<cnt; k++ ) {
        if( in_window(v) && !is_reserved(v) ) return v;
        v += step;
      }
    }
    return low();
  }

  // The message may end HERE.  Encoder: reserve this boundary's ending so no
  // longer message can flush to it.  Decoder: stop if the codestream has run out
  // and its value is exactly that ending.
  //
  // CDM calls this only at block boundaries -- the only points at which its
  // encoder can stop -- not at every coding step the way sh_bit's order-0 model
  // does.  The injectivity argument survives the thinning unchanged: what it
  // needs is that an ending live at *stop point* k is never the final ending at
  // a later *stop point*, and reserving at exactly the stop points gives that.
  uint rc_ending( void ) {
    uint e = find_ending();
    if( f_DEC && f_EOF && (code==e) ) return 1;
    reserve(e);
    return 0;
  }

  // ---- renormalisation --------------------------------------------------------
  void ShiftLow( void ) { // encoder
    uint carry = uint(lowc>>32);
    if( int(lowc)>=0 || carry ) {
      if( int(Cache)>=0 ) outbit(Cache^carry);
      for( ; FFNum; FFNum-- ) outbit(carry^1);
      Cache = 2*(Cache&0xFF000000) + (int(lowc)<0);
    } else FFNum++;
    lowc = uint(lowc<<1);
  }

  void ShiftStuff( void ) {
    range <<= 1;
    if( f_DEC ) { inpbit(); lowc = uint(lowc<<1); } else ShiftLow();
    for( uint i=0; i<nr; i++ ) resv[i] = (resv[i]<<1) | 1;   // endings keep their 1-tail
  }

  void rc_Renorm ( void ) { while( range<sTOP ) ShiftStuff(); }
  void rc_Renorm0( void ) {}
  void rc_Renorm1( void ) { rc_Renorm(); }  // SH1m's byte-granular shortcuts do not
  void rc_Renorm2( void ) { rc_Renorm(); }  // survive bit renormalisation

  // ---- scaling helpers (SH1m names, sh_bit state) -----------------------------
  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }
  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  // ---- n-ary: process_freqtable / process_range -------------------------------
  uint rc_GetFreq( uint totFreq ) { return muldivR( code-low(), totFreq ); }

  void rc_Arrange( uint totFreq ) {}

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint tmp  = range - mulRdiv( totFreq-cumFreq,        totFreq );
    uint rnew = range - mulRdiv( totFreq-(cumFreq+freq), totFreq );
    lowc += tmp;                      // both directions: low is explicit here
    range = rnew - tmp;
    drop_endings();
    rc_Renorm();
  }

  // ---- binary with a SCALE-scaled probability: rc_BProcess --------------------
  // SH1m: rnew = (range * (freq<<(32-SCALElog))) >> 32.  freq is in [1,SCALE-1]
  // so freq<<17 stays inside 32 bits, and with range >= 2^31 the split is never
  // degenerate (rnew >= 2^16).
  INLINE
  void rc_BProcess( uint freq, uint& bit ) {
    uint rnew = (qword(range)*(freq<<(32-SCALElog)))>>32;

    if( f_DEC ) bit = ((code-low()) >= rnew);

    range = ((range-rnew-rnew) & (-bit)) + rnew;
    lowc += rnew & (-bit);

    drop_endings();
    rc_Renorm();
  }

  // ---- binary with decrementing counts: rb_Process (CDM's enumerative core) ---
  // itotal = (2^32-1)/n scaled by freq0; since freq0 <= n the product stays
  // inside 32 bits.  As in SH1m this does NOT renormalise -- the caller's
  // rc_Renorm1/rc_Renorm2 does, and here those are the full loop.
  INLINE
  void rb_Process( uint& bit, uint& freq0, uint& freq1 ) {
    uint itotal = rb_itotal[freq0+freq1];
    itotal *= freq0;
    uint rnew = ((qword(range)*itotal + 0xFFFFFFFFU)>>32);

    if( f_DEC ) bit = ((code-low()) >= rnew);

    freq0 -= bit^1;
    freq1 -= bit;

    lowc += rnew & (-bit);
    range = ((range-rnew-rnew) & (-bit)) + rnew;

    drop_endings();
  }

  // ---- stored bytes: rs_process_byte ------------------------------------------
  // SH1m divides range by 256, multiplies by freq=1 and shifts exactly one byte
  // back in.  Here the same division costs exactly 8 bit-shifts, which is what
  // rc_Renorm does: range >= 2^31 -> range/256 >= 2^23 -> 8 shifts -> >= 2^31.
  void rs_Arrange( uint totFreq ) { range /= totFreq; }

  uint rs_GetFreq( void ) { return (code-low())/range; }

  void rs_Process( uint cumFreq, uint freq, uint totFreq ) {
    lowc += cumFreq*range;
    range *= freq;
    drop_endings();
    rc_Renorm();
  }

  uint rs_process_byte( uint value=0 ) {
    rs_Arrange( 256 );
    if( f_DEC ) value = rs_GetFreq();
    rs_Process( value, 1, 256 );
    return value;
  }

  // ---- init / quit -------------------------------------------------------------
  uint rb_itotal[32768+1];

  void rcInit( void ) {
    lowc  = 0;
    FFNum = 0;
    range = 1u<<(NUM-1);
    Cache = 0xC0000000;  // skip the first 2 bits: no cache yet, and range is 31-bit
    nr    = 0;
    code  = 0;
    rc_outbits = 0;
    if( f_DEC ) for( uint i=0; i<NUM-1; i++ ) inpbit();
  }

  void rc_Init( void ) {
    for( uint i=1; i<DIM(rb_itotal); i++ ) rb_itotal[i] = 0xFFFFFFFFU/i;
    rcInit();
  }

  void rc_Quit( void ) {
    if( f_DEC ) return;

    uint i, n = NUM;

    uint e = find_ending();
    lowc += uint(e-lowc);           // flush to the ending
    range = 1;

    qword llow = (uint)lowc, high = llow+range, mask = 1;
    for( i=0; i<NUM; i++ ) {
      if( (llow|mask)<high ) llow |= mask, n--;
      (mask <<= 1) += 1;
    }
    uint lw = (uint)llow;
    if( Cache==0xC0000000 ) return;  // nothing was ever emitted: the empty codestream
    uint carry = (uint)(lowc>>32);
    uint emitCache = ((int)Cache>=0) && ((n!=0) || (Cache^(carry!=1)));
    if( (n==0) && (carry==0) ) FFNum = 0;
    if( emitCache ) outbit(Cache^carry);
    for( i=0; i<FFNum; i++ ) outbit(carry^1);
    for( i=0; i<n; i++ ) outbit(lw>>31), lw <<= 1;
  }

  void StartCode( void ) { io_init(); rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); put_flush(); }
};
