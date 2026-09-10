
struct filehandle0 {

  FILE* f;

  template< typename CHAR >
  int open( const CHAR* name ) {
    if( strcmp(name,"-")==0 ) f = stdin; else
    f = fopen( name, "rb" );
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  int make( const CHAR* name ) {
    if( strcmp(name,"-")==0 ) f = stdout; else
    f = fopen( name, "wb" ); 
    return ((byte*)f)-((byte*)0);
  }

  int close( void ) {
    return fclose(f);
  }

  uint read( void* _buf, uint len ) {
    return fread(_buf,1,len,f);
  }

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


struct filehandle : filehandle0 {
  filehandle() { f=0; }
};


