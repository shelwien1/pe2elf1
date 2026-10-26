
// coro3 pin[i] yields 1+i when its buffer needs service, so the second output
// has to be pin[2] to arrive here as r==3; must match xOut in mp3rw.inc.
#define xInp (this->pin[3])
#define xOut (this->pin[2])

template < class Model, int inpbufsize=1<<16, int outbufsize = 1<<16, int ou2bufsize=1<<16 > 
struct CoroFileProc12 : Model {

  using Model::f_quit;
  using Model::coro_init;
  using Model::coro_call;
  using Model::addinp;
  using Model::addout;
  using Model::getoutsize;

//  enum { inpbufsize = 1<<16, outbufsize = 1<<16 };

  ALIGN(4096) 
  byte inpbuf[inpbufsize];

  ALIGN(4096)
  byte outbuf[outbufsize];

  ALIGN(4096)
  byte ou2buf[ou2bufsize];

  void processfile( FILE* f, FILE* g, FILE* h ) {
    uint l=0,r;
    coro_init();
    //xOut.pin_init();
    addout( outbuf, outbufsize );
    xOut.addout( ou2buf, ou2bufsize );

    while( 1 ) {
      r = coro_call(this); 

      if( r==1 ) {
        l = fread( inpbuf, 1, inpbufsize, f ); if( l==0 ) f_quit=1;
        addinp( inpbuf, l );
      } 

      if( (r==2) || (r==0) ) {
        fwrite( outbuf, 1, getoutsize(), g );
        addout( outbuf, outbufsize );
      }
  
      if( (r==3) || (r==0) ) {
        fwrite( ou2buf, 1, xOut.getoutsize(), h );
        xOut.addout( ou2buf, ou2bufsize );
      }

      if( r==0 ) break;
    }

//    fwrite( outbuf, 1,outptr-outbuf, g ); // flush
//    fwrite( ou2buf, 1,xOut.getoutsize(), h ); // flush

  }
};

#undef xInp
#undef xOut
