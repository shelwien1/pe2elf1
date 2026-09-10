
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

  operator int( void ) {
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  uint open( const CHAR* name ) {
    err = 0;
    f = file_open( name ); 
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  uint make( const CHAR* name ) {
    err = 0;
    f = file_make( name );
    return ((byte*)f)-((byte*)0);
  }

  int error( void ) {
    return err != 0;
  }

  //  Zero on success, like the stdio backend's fclose() and unlike the
  //  CloseHandle() underneath, which returns nonzero on success.  The two
  //  backends are one interface or they are nothing: a caller written as
  //  `if( h.close() ) failed();` -- which is how oggcomp.cpp reads it -- saw
  //  every successful close as a write error on this path.
  int close( void ) {
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
  filehandle() { f=0; err=0; }

  filehandle( int ) { f=0; err=0; }

  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    f_wr ? make(name) : open(name);
  }
};


