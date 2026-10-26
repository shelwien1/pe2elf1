
#define xInp (this->pin[3])
#define xOut (this->pin[2])

struct mp3rw : mp3parse {

//  coro_pin_w( mp3rw, xOut, 3 );
//  coro_pin_w( mp3rw, xInp, 4 ); // dummy for addinp

  #include "frame_dump_io.inc"

  void qut( uint c ) { xOut.put(c); }
  void qutv( qword c ) {
    for( uint flag=0; flag==0; c>>=7 ) qut( (c&0x7F) | (flag=(c<128)<<7) );
  }

  void write_garbage( void ) {
    qutv(GrbLen); uint i, l=(GrbLen+7)/8;
    for( i=0; i<l; i++ ) qut(Garbage[i]);
  }

  uint Init( void ) { 
    coro_init();
    //xOut.pin_init();
    Input();
    init_frame_dump();
    return 0;
  }

  void do_process() {

    uint i,l,f_mp3; int r;
    qword mp3len,binlen,r_pos,mp3num;

    f_mp3=0; binlen=0; mp3len=0;

    while(1) {

      r_pos = inppos-eofcount-(BufPtr/8);
      r = ReadFrame();

      if( r==0 ) {

        // if its first frame in a sequence, write size of bin chunk
        if( f_mp3==0 ) { 
          write_frame_dump( binlen*2+0, 1 );
          //putv( 4096*8 ); // for GrbLen
          //putv( binlen*2+0 ); 
          mp3len=r_pos; mp3num=0; 
        }

        write_frame_dump();
        f_mp3=1; mp3num++; // just count frames

      } else {

        if( f_mp3 ) {
          ReadFlush();
          mp3len = r_pos - mp3len; //inppos-eofcount-mp3len;
          qutv(mp3len); f_mp3=0; binlen=0;
          //qutv(mp3num);
          write_garbage();
        }

        if( r==1 ) {
          write_frame_dump( binlen*2+1, 1 );
          //putv( 4096*8 ); // for GrbLen
          //putv( binlen*2+1 );
          break;
        }

        if( r==3 ) continue; // layer III -> I/II: that frame starts the next sequence

        qut(r); binlen++;
      } 

    }

    yield(this,0);
  }

};

#undef xInp
#undef xOut
