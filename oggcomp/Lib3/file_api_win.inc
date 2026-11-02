
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "file_win.inc"

struct filehandle0 {

  HANDLE f;

  //  What ferror() is for the stdio backend.  ReadFile and WriteFile report
  //  failure in a BOOL that file_read/file_writ drop on the floor, so the
  //  wrappers below pick it up and keep the first GetLastError() they see;
  //  sticky, because a caller that writes a whole file and asks once at the
  //  end -- which is the useful way to ask -- would otherwise be told about
  //  the last write rather than the failed one.
  uint err;

  //  Set when f came from GetStdHandle rather than CreateFile.  A standard
  //  handle is on loan from whoever started the process: closing it would
  //  take stdout away from everything that runs after, and from the CRT's
  //  own atexit flush, so close() lets it be.
  uint borrowed;

  operator int( void ) {
    return ((byte*)f)-((byte*)0);
  }

  //  "-" is a name the stdio backend already reads as a standard stream, so
  //  this one has to as well or `oggcomp c - out.oc` writes a FILE called
  //  "-" on one build of the same program and pipes on the other.  Two
  //  overloads rather than strcmp, because open() and make() are templates
  //  that a wchar_t name instantiates just as readily.
  static int is_dash( const char* s )    { return s && s[0]=='-'  && s[1]==0; }
  static int is_dash( const wchar_t* s ) { return s && s[0]==L'-' && s[1]==0; }

  //  GetStdHandle gives INVALID_HANDLE_VALUE for an error and NULL where
  //  the process simply has no such handle -- a GUI subsystem build, or a
  //  service.  Both become the 0 that file_open returns for a failed open,
  //  so that a caller has one thing to test.
  HANDLE std_handle( uint which ) {
    HANDLE h = GetStdHandle( which );
    borrowed = 1;
    return h!=INVALID_HANDLE_VALUE ? h : 0;
  }

  template< typename CHAR >
  uint open( const CHAR* name ) {
    err = 0; borrowed = 0;
    f = is_dash(name) ? std_handle(STD_INPUT_HANDLE) : file_open( name );
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  uint make( const CHAR* name ) {
    err = 0; borrowed = 0;
    f = is_dash(name) ? std_handle(STD_OUTPUT_HANDLE) : file_make( name );
    return ((byte*)f)-((byte*)0);
  }

  int error( void ) {
    return err != 0;
  }

  //  Nothing is buffered on this side: writ() is WriteFile, and another
  //  handle on the file sees what it wrote.  Here so that the two backends
  //  are one interface; zero, like the stdio one when it succeeds.
  int flush( void ) {
    return 0;
  }

  //  Zero on success, like the stdio backend's fclose() and unlike the
  //  CloseHandle() underneath, which returns nonzero on success.  The two
  //  backends are one interface or they are nothing: a caller written as
  //  `if( h.close() ) failed();` -- which is how oggcomp.cpp reads it -- saw
  //  every successful close as a write error on this path.
  int close( void ) {
    if( borrowed ) { f = 0; return 0; }
    return file_close(f) ? 0 : 1;
  }

  qword size( void ) { return file_size(f); }

  void seek( qword ofs, uint typ=FILE_BEGIN ) {
    file_seek( f, ofs, typ );
  }

  qword tell( void ) {
    return file_tell(f);
  }

  template< typename BUF >
  uint read( BUF& buf ) { return read( &buf, sizeof(buf) )!=sizeof(buf); }

  //  A transfer that moved everything asked for cannot have failed, so only
  //  a short one is worth a question.  A short read is usually just the end
  //  of the file, where ReadFile succeeds and leaves the code at the zero
  //  set here -- which is why the slate is cleaned first: a Windows call
  //  that succeeds does not clear what the one before it left.
  uint read( void* _buf, uint len ) {
    SetLastError(0);
    uint r = file_read( f, _buf, len );
    if( (r!=len) && (err==0) ) err = GetLastError();
    return r;
  }

  template< typename BUF >
  uint sread( BUF& buf ) { return sread( &buf, sizeof(buf) )!=sizeof(buf); }

  uint sread( void* _buf, uint len ) {
    SetLastError(0);
    uint r = file_sread( f, _buf, len );
    if( (r!=len) && (err==0) ) err = GetLastError();
    return r;
  }

  int Getc( void ) {
    byte c;
    uint l = read(c);
    return l ? -1 : c;
  }

  uint writ( void* _buf, uint len ) {
    SetLastError(0);
    uint r = file_writ( f, _buf, len );
    if( r!=len ) {
      //  Unlike a short read, a short write is a failure even where
      //  WriteFile reported success -- that is what a full disk looks like
      //  from here -- so it gets a code whether or not Windows gave one.
      if( err==0 ) err = GetLastError();
      if( err==0 ) err = ERROR_HANDLE_DISK_FULL;
    }
    return r;
  }

  template< typename BUF >
  uint writ( BUF& buf ) {
    return writ( &buf, sizeof(buf) )!=sizeof(buf);
  }

};

struct filehandle : filehandle0 {
  filehandle() { f=0; err=0; borrowed=0; }

  filehandle( int ) { f=0; err=0; borrowed=0; }

  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    f_wr ? make(name) : open(name);
  }
};


