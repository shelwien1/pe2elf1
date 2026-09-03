
// BitstringWrap<IO> -- bijective bitstring <-> bytestring wrap.
//
// IO supplies `int get()` (-1 = end of input) and `void put(int)`; nothing else.
// Both methods are strictly sequential: they consume get() until EOF and emit via
// put(), never seek or clear -- the caller owns stream positioning.
//   bit2byte(): get() = bits (0/1),  put() = bytes
//   byte2bit(): get() = bytes,       put() = bits (0/1)

template< class IO >
struct BitstringWrap : IO {
  using IO::IO; // constructor of the I/O base
  using IO::get; using IO::put; // dependent base

  // Convert bitstream to bytestream
  void bit2byte( void ) {
    int K=2;
    int c,bn=7; int b=0;

    while(1) {
      c = get(); if( c==-1 ) break;
      b |= (c<<bn); --bn;
      if( bn==-1 ) {
        K = (b==0x80)?K:(b==0x00);
        put(b);
        bn=7; b=0;
      }
    }

    if( bn==7 ) {
      if( K==0 ) put(0x80);
    } else {
      put(b|(1<<bn));
    }
  }

  // Convert bytestream to bitstream
  void byte2bit( void ) {
    int K=2;
    int i,c; int b=-1;

    while(1) {
      c = get(); if( c==-1 ) break;
      if( b!=-1 ) {
        K = (b==0x80)?K:(b==0x00);
        for( i=0x80; i!=0; i>>=1 ) put((b&i)>0);
      }
      b = c;
    }

    if( b!=-1 ) {
      int f_EOF = (b!=0x00) && ((b!=0x80)||(K==0));
      for( i=0x80; i!=0; i>>=1 ) { if( f_EOF && ((b&(i-1))==0) ) break; put((b&i)>0); }
    }
  }
};
