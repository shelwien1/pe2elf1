
static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

struct Rangecoder {
  uint f_DEC; // 0=encode, 1=decode;
  FILE* f;

  // The same coder over a memory block instead of a stream.  f==0 is
  // what says so -- not buf==0, which is also where an encode into
  // memory starts.  Encoding grows the block.
  //
  // Decoding past the end returns 0xFF, which is what getc gives this
  // coder at end of file: byte(EOF).  The decoder reads a few bytes
  // beyond what the encoder flushed -- it has to, it is 31 bits ahead --
  // and those bytes decide the last symbol, so a memory decode that
  // answered 0 there would lose the final byte of the stream.
  byte* buf;
  size_t pos, len, cap;

  void MemInit( byte* b, size_t n, size_t c ) { buf=b; pos=0; len=n; cap=c; }
  void MemQuit( void ) { buf=0; pos=len=cap=0; }

  int Grow( void ) {
    size_t nc = cap ? cap*2 : 65536;
    byte* nb = (byte*)realloc( buf, nc );
    if( !nb ) return 0;
    buf=nb; cap=nc; return 1;
  }

  byte get() {
    if( f==0 ) return (buf&&pos<len) ? buf[pos++] : byte(EOF);
    return byte(getc(f));
  }

  void put( byte c ) {
    if( f ) { putc(c,f); return; }
    if( pos>=cap && !Grow() ) return; // out of memory: the caller sees a short blob
    buf[pos++]=c;
    if( pos>len ) len=pos;
  }

  void StartEncode( FILE *F ) { f=F; buf=0; f_DEC=0; rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( FILE *F ) { f=F; buf=0; f_DEC=1; rc_Init(); }

  // ... and the two that work on a block.  StartEncodeMem starts with
  // nothing and grows; StartDecodeMem is handed the whole stream.
  void StartEncodeMem( void ) { f=0; buf=0; pos=len=cap=0; f_DEC=0; rc_Init(); }
  void StartDecodeMem( const void* p, size_t n ) {
    f=0; buf=(byte*)p; pos=0; len=n; cap=n; f_DEC=1; rc_Init();
  }

  static const uint NUM   = 32;
  static const uint sTOP  = 0x80000000U;
  static const uint Thres = 0x80000000U;

  union {
    struct {
      union {
        uint  low;  
        uint  code; 
      };
      uint  Carry;
    };
    qword lowc;
  };
  uint  FFNum;
  uint  Cache;
  uint  range;

  int   bits;

  void inpbit( void ) {
    if( (bits<<=1)>=0 ) bits = 0xFF000000 + get();
    code = code*2 + (((signed char)bits)<0);
  }

  void outbit( uint c ) {
    (bits<<=1)+=(c&1);
    if( bits>=0 ) {
      put( bits );
      bits = 0xFF000000;
    }
  }

  void rc_Init( void ) {
    low     = 0x00000000;
    Carry   = 0;    
    FFNum   = 0;
    range   = 1<<(NUM-1);
    Cache   = 0xC0000000;

    bits    = 0xFF000000;
    if( f_DEC ) {
      bits  = 0x00000000;
      for(int _=0; _<NUM-1; _++) inpbit();
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow=low;
      qword high=llow+range;
      qword mask=1;

      for( i=0; i<NUM; i++ ) {
        if( (llow|mask)<high ) llow|=mask,n--;
        (mask<<=1)+=1;
      }

      low = llow;
      if( (int(Cache)>=0) && ((n!=0) || ((Cache^Carry)!=1)) ) outbit( Cache^Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also 1s
      for( i=0; i<FFNum; i++ ) outbit( Carry^1 );
      for( i=0; i<n; i++ ) outbit( low>>31 ), low<<=1;
      while( byte(bits>>24)!=0xFF ) outbit(1);
    }
  }

  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  uint rc_BProcess( uint freq, uint bit ) { 
    uint x[] = { 0, freq, SCALE };
    if( f_DEC ) {
      uint val = rc_GetFreq( SCALE );
      bit = (val>=freq);
    }
    rc_Process( x[bit], x[1+bit]-x[bit], SCALE );
    return bit;
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {

    uint tmp  = range-mulRdiv( totFreq-cumFreq, totFreq );
    uint rnew = range-mulRdiv( totFreq-(cumFreq+freq), totFreq );

    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range = rnew - tmp;

    rc_Renorm();
  }

  void rc_Renorm( void ) {
    while( range<sTOP ) ShiftStuff();
  }

  void ShiftStuff( void ) {
    range<<=1;
    if( f_DEC ) {
      inpbit();
    } else {
      ShiftLow();
    }
  }

  void ShiftLow( void ) {
    if( (int(low)>=0) || Carry ) {
      if( int(Cache)>=0 ) outbit( Cache^Carry );
      for(; FFNum!=0; FFNum-- ) outbit(Carry^1);
      Cache = 2*(Cache&0xFF000000) + (int(low)<0);
      Carry = 0;
    } else FFNum++;
    low<<=1;
  }

};

