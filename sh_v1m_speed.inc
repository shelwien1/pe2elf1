
// Rangecoder from sh_v1m, with the Lib3 Coroutine base replaced by plain
// stdio: get()/put() are getc()/putc() on the two FILE* handles set by
// rc_SetFiles().  Coding logic is unchanged, so the bitstream is identical.

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

template< int f_DEC >
struct Rangecoder {

  static const uint NUM   = 4;
  static const uint sTOP  = 0x01000000U;
  static const uint gTOP  = 0x00010000U;
  static const uint Thres = 0xFF000000U;

  FILE* f;      // input  stream (plaintext when encoding, code when decoding)
  FILE* g;      // output stream (code when encoding, plaintext when decoding)
  qword nget;   // bytes pulled from f  (progress display only)
  qword nput;   // bytes pushed to   g  (progress display only)

  union {
    struct {
      uint  low;
      uint  Carry;
    };
    qword lowc;
  };
  uint  code;
  uint  FFNum;
  uint  Cache;
  qword range;

  void rc_SetFiles( FILE* _f, FILE* _g ) { f=_f; g=_g; nget=0; nput=0; }

  // EOF yields uint(-1), exactly like coro3_pin::get() did on f_quit
  uint get( void ) { uint c = getc(f); nget += (c!=uint(-1)); return c; }

  // putc truncates to a byte, exactly like coro3_pin::put0()'s *ptr++ = c
  void put( uint c ) { putc( c, g ); nput++; }

  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }

  void rc_Renorm( void ) {
    while( range<sTOP ) ShiftStuff();
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {

    uint tmp  = range-mulRdiv( totFreq-cumFreq, totFreq );
    uint rnew = range-mulRdiv( totFreq-(cumFreq+freq), totFreq );

    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range = rnew - tmp;

    rc_Renorm();
  }

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  // PAQ_DECFAST: the binary decode decision without the 64-bit divide.
  //
  // The caller's test is  muldivR(code,4096) >= px[1]  with px[1] = 4096-p, i.e.
  //   floor(code*4096/range) >= 4096-p
  //   <=> code*4096 >= (4096-p)*range
  //   <=> code >= ceil((4096-p)*range/4096)
  // and rc_Process's own boundary for bit 0 is
  //   rnew = range - mulRdiv(p,4096) = range - floor(p*range/4096)
  //        = ceil((4096-p)*range/4096)
  // the same value.  So the comparison is exactly `code >= that boundary`, and
  // the boundary is a multiply-shift (totFreq is the literal 4096, so mulRdiv's
  // divide is a >>12).  Bit-identical, decode only.
  uint rc_Decide( uint p ) {
    return code >= (range - mulRdiv(p,4096));
  }

  void ShiftStuff( void ) {
    range = (range<<8)+0x00;
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    code  = (code<<8)+0x00;
    uint c = get();
    FFNum += (c==-1);
    code += byte(c);
  }

  void ShiftLow( void ) {
    if( (low<Thres) || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for(;FFNum!=0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

  void rc_Init( void ) {
    low   = 0;
    FFNum = 0;
    Carry = 0;
    Cache = -1;
    if( f_DEC==1 ) {
      for(int _=0; _<NUM; _++) ShiftCode();
    }
    range = 1ULL<<32;//0xFFFFFFFF;
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow=low;
      qword high=llow+range;

      if( (llow|      0xFF) < high ) low|=      0xFF,n--;
      if( (llow|    0xFFFF) < high ) low|=    0xFFFF,n--;
      if( (llow|  0xFFFFFF) < high ) low|=  0xFFFFFF,n--;
      if( (llow|0xFFFFFFFF) < high ) low|=0xFFFFFFFF,n--;

      if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

};
