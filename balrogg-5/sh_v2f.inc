/*  A binary range coder, supplied with the task as sh_v2f.inc and adapted
    here only in how it reaches its bytes: the original read and wrote a
    FILE*, this one a memory buffer, so the embed side can hand it a length
    header in front of the payload and the extract side can look at what came
    back before deciding how much of it is real.  The coder itself -- the
    carry handling, the bit-at-a-time renormalisation, rc_Process -- is
    unchanged.

    Steganography here is a decode in one direction and an encode in the
    other.  Filling a quantization gap means choosing between two integers,
    and the choice is made by *decoding* a symbol out of the payload under a
    model that says how likely each integer is.  Recovering the payload means
    *encoding* the choices back under the same model, which returns the bytes
    the decoder consumed.  So the tool never stores the payload; it stores the
    choices, and the choices are the payload read through the model.  */

typedef u8  byte;
typedef u32 uint;
typedef uint64_t qword;

/*  `lowc` aliases (low, Carry) as one 64-bit counter so that adding to it
    propagates into the carry flag.  That is a layout assumption, and the rest
    of this tree is careful to be endian-neutral, so say so rather than
    miscode silently on a big-endian build.  */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "sh_v2f.inc aliases (low, Carry) as one 64-bit counter: little-endian only"
#endif
#endif

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

/*  The byte source and sink.  Reading past the end yields 0xFF, which is what
    the original's byte(getc(f)) did at EOF, so a payload shorter than the
    capacity behaves exactly as before.  */
struct rc_buf {
  u8 * p;
  sz n, cap, at;

  void init() { p = nullptr;  n = cap = at = 0; }
  void free_() { ::free(p);  init(); }

  byte get() { return at < n ? p[at++] : (byte) (at++, 0xFF); }

  void put(byte c) {
    if (n == cap) {
      sz want = cap ? cap * 2 : 1 << 12;
      u8 * q = (u8 *) realloc(p, want);
      if (!q) FATAL_CODE(BLR_EXIT_IO, "out of memory for %llu payload bytes",
                         (unsigned long long) want);
      p = q;  cap = want;
    }
    p[n++] = c;
  }
};

struct Rangecoder {
  uint f_DEC; // 0=encode, 1=decode;
  rc_buf * b;
  byte get() { return b->get(); }
  void put( byte c ) { b->put(c); }
  void StartEncode( rc_buf *B ) { b=B; f_DEC=0; rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( rc_buf *B ) { b=B; f_DEC=1; rc_Init(); }

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
      for(uint _=0; _<NUM-1; _++) inpbit();
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

      low = (uint) llow;
      if( (int(Cache)>=0) && ((n!=0) || ((Cache^Carry)!=1)) ) outbit( Cache^Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also 1s
      for( i=0; i<FFNum; i++ ) outbit( Carry^1 );
      for( i=0; i<n; i++ ) outbit( low>>31 ), low<<=1;
      while( byte(bits>>24)!=0xFF ) outbit(1);
    }
  }

  uint muldivR( uint a, uint b ) { return (uint) ((qword(a)*b)/range); }

  uint mulRdiv( uint a, uint c ) { return (uint) ((qword(a)*range)/c); }

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  uint rc_BProcess( uint freq, uint bit ) {
    uint x[] = { 0, freq, (uint) SCALE };
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
