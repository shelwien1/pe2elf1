
template < class Model > 
struct CoroFileProc : Model {
  using Model::f_quit;
  using Model::outbeg;
  using Model::coro_init;
  using Model::getoutsize;
  using Model::addinp;
  using Model::addout;
  using Model::coro_call;

  enum{ inpbufsize=1<<16, outbufsize=1<<16 };

  ALIGN(4096) byte inpbuf[inpbufsize];
  ALIGN(4096) byte outbuf[outbufsize];

  // Returns the final coroutine status: 0 on success, 3 on a propagated
  // error (e.g. zstd decompression failure). Existing callers that ignore
  // the return value still work; new error-aware paths can check it.
  uint processfile( filehandle f, filehandle g, byte* _inpptr=0, byte* _inpend=0 ) {
    uint l,r=0;

    coro_init();
    addout( outbuf, outbufsize );
    if( _inpptr!=0 ) addinp( _inpptr, _inpend-_inpptr );

    while(1) {
      r = coro_call(this);
//printf( "r=%i\n", r );
      if( r==1 ) {
        l = f.sread( inpbuf, inpbufsize );
        if( l==0 ) f_quit=1; // get/put don't support repeated failures
        g_prog_in += l; // archive (compressed) bytes consumed: x/t input
        addinp( inpbuf, l );
      } else { // r0=quit, r3=error
        l = getoutsize();
        if( l>0 ) g.writ( (byte*)outbeg, l );
        if( r!=2 ) break;
        addout( outbuf, outbufsize );
      } // if
    } // while

    return r;
  } // func

};

