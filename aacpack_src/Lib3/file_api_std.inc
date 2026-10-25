
// (what aacpack uses of Lib3's file API: a FILE* that "-" opens as stdin,
// and reads and writes of whole buffers)

struct filehandle0 {

  FILE* f;

  template< typename CHAR >
  void open( const CHAR* name ) {
    if( strcmp(name,"-")==0 ) f = stdin; else
    f = fopen( name, "rb" );
  }

  uint read( void* _buf, uint len ) {
    return fread(_buf,1,len,f);
  }

  // len bytes, fewer only where the input ends
  uint sread( void* _buf, uint len ) { 
    byte* buf = (byte*)_buf;
    uint l=0, r;

    do {
      r = read( buf+l, len-l );
      l += r;
    } while( (r>0) && (l<len) );

    return l;
  }

  uint writ( void* _buf, uint len ) {
    return fwrite( _buf, 1,len, f );
  }

};
