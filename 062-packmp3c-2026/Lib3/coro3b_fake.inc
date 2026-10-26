
struct Coroutine {
  byte* __restrict inpptr;
  byte* __restrict inpbeg;
  byte* __restrict inpend;
  byte* __restrict outptr;
  byte* __restrict outbeg;
  byte* __restrict outend;
  uint f_quit;

  template <typename T> 
  uint coro_call( T* that ) {
    that->do_process();
    return 0;
  }

  void chkinp( void ) { }

  void chkout( uint d=0 ) {}

  uint get( void ) { return *inpptr++; }
  void put( uint c ) { *outptr++ = c; }
  byte get0( void ) { return *inpptr++; }
  void put0( uint c ) { *outptr++ = c; }

  uint get4( void ) { 
    uint* __restrict ptr4 = (uint*)inpptr;
    const uint c  = *ptr4++;
    inpptr = (byte*)ptr4;
//    _mm_prefetch( inpptr, 0 );
    return c;
  }

  void put4( uint c ) { 
    uint* __restrict ptr4 = (uint*)outptr;
    *ptr4++ = c;
    outptr = (byte*)ptr4;
  }

  void coro_init( void ) {
    inpptr=inpbeg=inpend = 0;
    outptr=outbeg=outend = 0;
    f_quit=0;
  }

  uint getinplen() { return inpend-inpptr; } 
  uint getoutlen() { return outend-outptr; } 

  uint getinpleft() { return inpend-inpptr; }
  uint getinpsize() { return inpptr-inpbeg; }
  uint getoutleft() { return outend-outptr; }
  uint getoutsize() { return outptr-outbeg; }

  void addinp( byte* inp,uint inplen ) {
    inpbeg = inpptr = inp;
    inpend = &inp[inplen];
  }

  void addout( byte* out,uint outlen ) {
    outbeg = outptr = out;
    outend = &out[outlen];
  }

  void yield( void* p, int value ) {}

};
