
#if defined(_MSC_VER)
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#elif !defined(_LARGEFILE64_SOURCE) && !defined(__MINGW32__)
#define fseeko64 fseeko
#define ftello64 ftello
#endif

//  A standard stream reaches a Windows process in text mode: every 0x0A on
//  the way out becomes 0x0D 0x0A, and every 0x0D 0x0A on the way in becomes
//  0x0A.  On a file that means something, that is not a formatting choice,
//  it is corruption -- and silent, since nothing fails.  Measured before
//  this was here: `oggcomp d in.oc - > out.ogg` gave 168196 bytes for an
//  input of 167685 and exited 0.  fopen already asks for "rb"/"wb"; only
//  the two streams that are handed over already open need telling.
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#define FILE_API_SETBIN(h) _setmode( _fileno(h), _O_BINARY )
#else
#define FILE_API_SETBIN(h) ((void)0)
#endif

struct filehandle0 {

  FILE* f;

  operator int( void ) {
    return ((byte*)f)-((byte*)0);
  }

  //  Set when f is stdin or stdout rather than a file this struct opened.
  //  Those are on loan from whoever started the process: fclose()ing one
  //  would take it away from everything that runs after, and from the CRT's
  //  own flush at exit.
  uint borrowed;

  template< typename CHAR >
  int open( const CHAR* name ) {
    borrowed = 0;
    if( strcmp(name,"-")==0 ) { f = stdin; borrowed = 1; FILE_API_SETBIN(f); } else
    f = fopen( name, "rb" );
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  int make( const CHAR* name ) {
    borrowed = 0;
    if( strcmp(name,"-")==0 ) { f = stdout; borrowed = 1; FILE_API_SETBIN(f); } else
    f = fopen( name, "wb" ); 
    return ((byte*)f)-((byte*)0);
  }

  int close( void ) {
    //  A borrowed stream is not ours to close, but it is ours to flush:
    //  the caller asks close() whether the bytes got out, and for stdout
    //  they have not until the buffer has gone.
    if( borrowed ) { int r = fflush(f); f = 0; return r; }
    return fclose(f);
  }

  //  Has this handle seen a read or write error?  The caller wants the
  //  question, not the mechanism: the WinAPI backend has no FILE* to hand
  //  to ferror() and answers it from a flag its own read/writ keep.
  int error( void ) {
    return ferror(f);
  }

  qword size( void ) { 
    qword p = tell();
    seek( 0, SEEK_END );
    qword s = tell();
    seek( p, SEEK_SET );
    return s; 
  }

  void seek( qword ofs, uint typ=SEEK_SET ) {
    fseeko64( f, ofs, typ );
  }

  qword tell( void ) {
    return ftello64(f);
  }

  template< typename BUF >
  uint read( BUF& buf ) {
    return read( &buf, sizeof(buf) )!=sizeof(buf);
  }

  uint read( void* _buf, uint len ) {
    return fread(_buf,1,len,f);
  }

  template< typename BUF >
  uint sread( BUF& buf ) { return sread( &buf, sizeof(buf) )!=sizeof(buf); }

  uint sread( void* _buf, uint len ) { 
    byte* buf = (byte*)_buf;
    uint l=0, r;

    do {
      r = read( buf+l, len-l );
      l += r;
    } while( (r>0) && (l<len) );

    return l;
  }

  int Getc( void ) {
    byte c;
    uint l = read(c);
    return l ? -1 : c;
  }

  uint writ( void* _buf, uint len ) {
    return fwrite( _buf, 1,len, f );
  }

  template< typename BUF >
  uint writ( BUF& buf ) {
    return writ( &buf, sizeof(buf) )!=sizeof(buf);
  }

};


struct filehandle : filehandle0 {
  filehandle() { f=0; borrowed=0; }

  filehandle( int ) { f=0; borrowed=0; }

  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    f_wr ? make(name) : open(name);
  }
};


