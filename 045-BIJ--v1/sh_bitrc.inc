
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

  // -DCDM_CHECK builds every coder invariant in as a runtime assertion.  It is
  // off by default (it costs a branch per operation); `make check` runs the
  // whole self-test under it.
#ifdef CDM_CHECK
  void chk( int ok, const char* what ) {
    if( !ok ) {
      fprintf( stderr, "CDM_CHECK failed: %s  (f_DEC=%i range=%08X low=%08X code=%08X nr=%u)\n",
               what, int(f_DEC), range, low(), code, nr );
      abort();
    }
  }
  // the decoder is only in sync while its codestream value lies in the window
  void chk_window( const char* what ) { if( f_DEC ) chk( uint(code-low())<range, what ); }
  void chk_norm( const char* what ) { chk( range>=sTOP, what ); }
#else
  void chk( int, const char* ) {}
  void chk_window( const char* ) {}
  void chk_norm( const char* ) {}
#endif

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

  void rc_Renorm ( void ) {
#ifdef CDM_CHECK
    chk( range>0, "rc_Renorm: range==0 would spin forever" );
    uint n=0; while( range<sTOP ) { ShiftStuff(); chk( ++n<=NUM, "rc_Renorm: too many shifts" ); }
    chk_norm( "rc_Renorm: not normalised on exit" );
    chk_window( "rc_Renorm: code left the window" );
#else
    while( range<sTOP ) ShiftStuff();
#endif
  }
  void rc_Renorm0( void ) {}
  void rc_Renorm1( void ) { rc_Renorm(); }  // SH1m's byte-granular shortcuts do not
  void rc_Renorm2( void ) { rc_Renorm(); }  // survive bit renormalisation

  // ---- scaling helpers (SH1m names, sh_bit state) -----------------------------
  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }
  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  // ---- n-ary: process_freqtable / process_range -------------------------------
  uint rc_GetFreq( uint totFreq ) {
    chk_window( "rc_GetFreq: entered out of window" );
    uint v = muldivR( code-low(), totFreq );
    chk( v<totFreq, "rc_GetFreq: value >= totFreq" );
    return v;
  }

  void rc_Arrange( uint totFreq ) {}

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    chk_norm( "rc_Process: entered unnormalised" );
    chk_window( "rc_Process: entered out of window" );
    chk( (freq>=1) && (cumFreq+freq<=totFreq) && (totFreq>=1), "rc_Process: bad freqs" );
    uint tmp  = range - mulRdiv( totFreq-cumFreq,        totFreq );
    uint rnew = range - mulRdiv( totFreq-(cumFreq+freq), totFreq );
    lowc += tmp;                      // both directions: low is explicit here
    range = rnew - tmp;
    chk( range>0, "rc_Process: range collapsed to 0" );
    drop_endings();
    rc_Renorm();
  }

  // ---- binary with a SCALE-scaled probability: rc_BProcess --------------------
  // SH1m: rnew = (range * (freq<<(32-SCALElog))) >> 32.  freq is in [1,SCALE-1]
  // so freq<<17 stays inside 32 bits, and with range >= 2^31 the split is never
  // degenerate (rnew >= 2^16).
  INLINE
  void rc_BProcess( uint freq, uint& bit ) {
    chk_norm( "rc_BProcess: entered unnormalised" );
    chk_window( "rc_BProcess: entered out of window" );
    chk( (freq>=1) && (freq<SCALE), "rc_BProcess: probability out of [1,SCALE)" );
    uint rnew = (qword(range)*(freq<<(32-SCALElog)))>>32;
    chk( (rnew>0) && (rnew<range), "rc_BProcess: degenerate split" );

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
    chk_norm( "rb_Process: entered unnormalised" );
    chk_window( "rb_Process: entered out of window" );
    // codec.inc guards (freq0==0)+(freq1==0) once per BYTE, so a count may be 0
    // here; the split is then degenerate (rnew==0 or rnew==range) and free.
    chk( freq0+freq1 >= 1, "rb_Process: n==0, rb_itotal[0] is not a probability" );
    chk( freq0+freq1 < DIM(rb_itotal), "rb_Process: n past rb_itotal" );
    uint itotal = rb_itotal[freq0+freq1];
    itotal *= freq0;
    uint rnew = ((qword(range)*itotal + 0xFFFFFFFFU)>>32);
    chk( rnew<=range, "rb_Process: rnew past range" );

    if( f_DEC ) bit = ((code-low()) >= rnew);

    // A count at 0 admits exactly one bit value, and codec.inc only re-checks
    // that once per BYTE, so the remaining bits of the byte are coded with the
    // count already at 0.  The split is then degenerate -- but only ALMOST:
    // rnew is range-(0xFFFFFFFF mod n)*range/2^32 rather than range, leaving a
    // sliver of codestream values the encoder can never produce.  Its measured
    // mass is ~1.6e-4 per self-test run, so an arbitrary archive does land
    // there, and the raw comparison would then return the forbidden bit,
    // wrapping the counter to 0xFFFFFFFF and indexing rb_itotal out of bounds
    // on the next call.  Clamping is a no-op on any encoder-produced stream.
    // (r044 has the same hazard; it just never decodes a foreign archive.)
    if( f_DEC ) bit = (bit | (freq0==0)) & (freq1!=0);

    freq0 -= bit^1;
    freq1 -= bit;

    lowc += rnew & (-bit);
    range = ((range-rnew-rnew) & (-bit)) + rnew;
    chk( range>0, "rb_Process: range collapsed to 0" );

    drop_endings();
  }

  // ---- stored bytes: rs_process_byte ------------------------------------------
  // SH1m divides range by 256, multiplies by freq=1 and shifts exactly one byte
  // back in.  Here the same division costs exactly 8 bit-shifts, which is what
  // rc_Renorm does: range >= 2^31 -> range/256 >= 2^23 -> 8 shifts -> >= 2^31.
  void rs_Arrange( uint totFreq ) {
    chk_norm( "rs_Arrange: entered unnormalised" );
    chk_window( "rs_Arrange: entered out of window" );
    range /= totFreq;
    chk( range>0, "rs_Arrange: range collapsed to 0" );
  }

  uint rs_GetFreq( void ) {
    uint v = (code-low())/range;
    chk( v<256, "rs_GetFreq: symbol out of range" );
    return v;
  }

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
