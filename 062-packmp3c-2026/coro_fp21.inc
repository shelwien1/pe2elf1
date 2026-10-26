
// coro3 pin[i] yields 1+i when its buffer needs service, so the second input
// has to be pin[2] to arrive here as r==3; must match xInp in mp3rs.inc.
#define xInp (this->pin[2])
#define xOut (this->pin[3])

template < class Model, int inpbufsize=1<<16, int in2bufsize=1<<16, int outbufsize = 1<<16 > 
struct CoroFileProc21 : Model {

  using Model::f_quit;
  using Model::outptr;
  using Model::coro_init;
  using Model::coro_call;
  using Model::addinp;
  using Model::addout;

  ALIGN(4096) byte inpbuf[inpbufsize];
  ALIGN(4096) byte in2buf[in2bufsize];
  ALIGN(4096) byte outbuf[outbufsize];

  void processfile( FILE* f, FILE* h, FILE* g ) {
    uint l,r;
    coro_init();
    addout( outbuf, outbufsize );
    uint& f_quit2 = xInp.f_EOF;
    while( 1 ) {
      r = coro_call(this); 
      if( r==0 ) break;
      if( r==1 ) {
        l = fread( inpbuf, 1, inpbufsize, f );
        if( l==0 ) f_quit=1; // get/put don't support repeated failures
        addinp( inpbuf, l ); 
      } else if( r==3 ) {
        l = fread( in2buf, 1, in2bufsize, h );
        if( l==0 ) f_quit2=1;
        //addin2( in2buf, l ); 
        xInp.addinp( in2buf, l ); 
      } else if( r==2 ) {
        fwrite( outbuf, 1, outbufsize, g );
        addout( outbuf, outbufsize );
      }
    }
    fwrite( outbuf, 1,outptr-outbuf, g ); // flush
  }
};

#undef xInp
#undef xOut
