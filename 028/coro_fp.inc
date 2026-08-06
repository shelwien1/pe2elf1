#include "lazyout.inc"

template < class Model, int inpbufsize=1<<16, int outbufsize = 1<<16, int inpbufsize1=inpbufsize > 
struct CoroFileProc : Model {
  using Model::f_quit;
  using Model::outbeg;
  using Model::coro_init;
  using Model::getoutsize;
  using Model::addinp;
  using Model::addout;
  using Model::coro_call;

//  enum { inpbufsize = 1<<16, outbufsize = 1<<16 };
  ALIGN(4096) byte inpbuf[inpbufsize];
  ALIGN(4096) byte outbuf[outbufsize];

  void pInit( void ) {
    coro_init();
    addout( outbuf, outbufsize );
  }

  /* Deferred output: `out.path` is set by the caller and the file is created on
   * the first byte actually written, so a run that dies early leaves nothing
   * behind rather than a zero-length file. */
  LazyOut out;

  void processfile( FILE* f, FILE* g, int f_init=1 ) {
    (void)g;
    if( f_init ) pInit();
    while( 1 ) {
      int r = coro_call(this); 
      if( r==0 ) break;
      if( r==1 ) {
        uint l = fread( inpbuf, 1, inpbufsize1, f );
//        if( l==0 ) break; // get/put don't support repeated failures
        if( l==0 ) f_quit=1; // get/put don't support repeated failures
        addinp( inpbuf, l ); 
      } else if( r==2 ) {
        { FILE* o = out.w(); if( o ) fwrite( outbuf, 1, outbufsize, o ); }
        addout( outbuf, outbufsize );
      }
    }
    { uint l_ = getoutsize();            // flush; never create an empty file
      if( l_ ) { FILE* o = out.w(); if( o ) fwrite( outbuf, 1, l_, o ); } }
  }
};
