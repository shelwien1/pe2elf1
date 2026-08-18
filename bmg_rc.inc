// bmg_rc.inc -- the carryless range coder, taken from bcdr5.cpp unchanged in
// semantics.  Two interfaces: rc_Process() for a frequency-table symbol and
// rc_BProcess() for a binary decision against a 15-bit probability.
//
// rc_BProcess()'s p is P(bit==0): "bound" is the sub-range the zero branch
// keeps, which is the same convention Counter::P uses, so a counter's P can be
// handed straight to the coder with no complement anywhere in the model.

// Probability scale constants
static const uint SCALElog = 15;
static const uint SCALE  = 1 << SCALElog;
static const uint hSCALE = SCALE >> 1;
static const uint mSCALE = SCALE - 1;
static const uint eSCALE = 16*SCALE;

struct Rangecoder {
  int f_DEC;   // 0=encode, 1=decode

  // The stream lives in memory: the encoder needs its own length before it can
  // decide whether coding beat storing (section 4.2's raw fallback), and a
  // memory cursor is the cheapest way to have it.  put() past the end is
  // dropped rather than grown -- an output that long has already lost to
  // storing, and the caller checks.
  byte* mem;
  qword mpos, mcap;

  // 0xFF past the end, not 0.  rc_Quit() drops a tail byte whenever setting
  // the low bits of `low` to ones still lands inside [low,high) -- the omitted
  // bytes are ones, and bcdr5's FILE* reader got that for free from
  // byte(EOF).  Returning 0 here loses the last coded symbol.
  int  get() { return (mpos<mcap) ? mem[mpos++] : 0xFF; }
  void put(int c) { if( mpos<mcap ) mem[mpos]=byte(c); mpos++; }

  static const uint NUM   = 4;
  static const uint sTOP  = 0x01000000U;
  static const uint Thres = 0xFF000000U;

  union {
    struct {
      uint low;
      uint Carry;
    };
    qword lowc;
  };
  uint code;
  uint FFNum;
  uint Cache;
  uint range;

  void rc_Renorm( void ) {
    while( range < sTOP ) ShiftStuff();
  }

  uint rc_GetFreq( uint totFreq ) {
    return code / (range / totFreq);
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint scale = range / totFreq;
    uint tmp = scale * cumFreq;
    uint rnew = scale * (cumFreq + freq);
    if( cumFreq + freq == totFreq ) rnew = range;
    if( f_DEC ) code -= tmp; else lowc += tmp;
    range = rnew - tmp;
    rc_Renorm();
  }

  // Binary process with probability p == P(bit==0), out of SCALE
  uint rc_BProcess( uint p, uint bit ) {
    uint bound = (qword(range)*(p<<(32-SCALElog)))>>32;
    if( bound == 0 ) bound = 1;
    if( bound >= range ) bound = range - 1;

    if( f_DEC ) bit = (code >= bound);

    if( bit ) {
      if( f_DEC ) code -= bound; else lowc += bound;
      range -= bound;
    } else {
      range = bound;
    }

    rc_Renorm();
    return bit;
  }

  void ShiftStuff( void ) {
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    range = (range<<8);
    code  = (code<<8);
    uint c = get();
    code += byte(c);
  }

  void ShiftLow( void ) {
    range = (range<<8);
    if( (low<Thres)||Carry ) {
      if( Cache!=uint(-1) ) put( Cache+Carry );
      for(; FFNum!=0; FFNum-- ) put( Carry-1 );
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

  void rcInit( void ) {
    range = 0xFFFFFFFF;
    low   = 0;
    FFNum = 0;
    Carry = 0;
    Cache = -1;
  }

  void rc_Init( void ) {
    rcInit();
    if( f_DEC==1 ) {
      for( uint _=0; _<NUM; _++ ) ShiftCode();
      range = 0xFFFFFFFF;
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;
      qword llow=low;
      qword high=llow+range;
      if( (llow|0xFF)<high ) low|=      0xFF, n--;
      if( (llow|0xFFFF)<high ) low|=    0xFFFF, n--;
      if( (llow|0xFFFFFF)<high ) low|=  0xFFFFFF, n--;
      if( (llow|0xFFFFFFFF)<high ) low|=0xFFFFFFFF, n--;
      if( (Cache!=uint(-1))&&((n!=0)||(Cache+Carry!=0xFF)) ) put( Cache+Carry );
      if( (n==0)&&(Carry==0) ) FFNum=0;
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

  void init( byte* p, qword cap, int isDec ) {
    mem = p; mcap = cap; mpos = 0; f_DEC = isDec; rc_Init();
  }

  void flush( void ) {
    if( f_DEC==0 ) rc_Quit();
  }
};
