
#define xInp (this->pin[2])
#define xOut (this->pin[3])

struct mp3rs : mp3parse {

//  coro_pin_w( mp3rs, xInp, 3 );
//  coro_pin_w( mp3rs, xOut, 4 ); // dummy for addout

  #include "frame_dump_io.inc"

  uint het( void ) { return xInp.get(); }
  qword hetv( void ) {
    qword r = 0;
    for( uint l=0,c=0; (c&0x80)==0; l+=7 ) r |= qword((c=het())&0x7F)<<l;
    return r;
  }

  void read_garbage( void ) {
    GrbLen = hetv();
    uint i,l=(GrbLen+7)/8;
    for( i=0; i<l; i++ ) Garbage[i]=het();
  }

  uint Init( void ) { 
    coro_init();
    //xInp.pin_init();
    Output();
    init_frame_dump();
    return 0;
  }

  void do_process( void ) {
    qword len;
    uint i,l,f_EOF=0,f_skip=1;

    while( f_EOF==0 ) {
      while( read_frame_dump(len,f_EOF)==0 ) WriteFrame();

      if( f_skip ) f_skip=0; else {
        read_garbage();
        WriteFlush();
      }

      outpos+=len;
      while( len-- ) put( het() ); 

      outlen = outpos + hetv(); // size limit for WriteFrame/WriteFlush
    }

    yield(this,0);
  }

};

#undef xInp
#undef xOut
