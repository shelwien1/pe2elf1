// inc/files.inc -- files and paths: long paths, names the tools can take
//  (ANSI), names allowed in an archive, sizes, reading and writing,
//  directories made and removed
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- files and paths

static wstring FullPath( const wstring& p ) {
  DWORD n = GetFullPathNameW( p.c_str(), 0, 0, 0 );
  if( n==0 ) return p;
  wstring r( n, 0 );
  n = GetFullPathNameW( p.c_str(), n, &r[0], 0 );
  r.resize( n );
  return r;
}

// our own calls take long paths in the \\?\ form
static wstring LongPath( const wstring& p ) {
#ifdef _WIN32
  if( (p.size()<MAX_PATH-12) || (p.compare(0,4,L"\\\\?\\")==0) ) return p;
  if( p.compare(0,2,L"\\\\")==0 ) return L"\\\\?\\UNC\\"+p.substr(2);
  return L"\\\\?\\"+p;
#else
  return p;
#endif
}

// a directory separator: '\' or '/' (on Linux '/' only)
static int IsSep( wchar_t c ) { return (c==L'/') || ((c==L'\\') && (DS[0]=='\\')); }

static wstring FileName( const wstring& p ) {
  size_t i = p.size();
  while( i && !IsSep( p[i-1] ) ) i--;
  return p.substr( i );
}

// 1 if a tool's ANSI command line carries the name as it is (on Linux it is
// UTF-8: a name with no '\' in it, and no byte that is not UTF-8)
static int AnsiOK( const wstring& w ) {
  if( w.empty() || (w.size()>=MAX_PATH-4) ) return 0;
#ifndef _WIN32
  for( size_t i=0; i<w.size(); i++ ) if( (w[i]==L'\\') || ((w[i]>=0xDC80) && (w[i]<=0xDCFF)) ) return 0;
#endif
  if( GetACP()==CP_UTF8 ) return 1;
  BOOL bad = 0;
  int n = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, w.c_str(), int(w.size()), 0, 0, 0, &bad );
  return (n>0) && !bad;
}

// an existing file's full name as a tool can take it: itself, else its 8.3
// form, else "" (then it has to be linked or copied into the work directory)
static wstring ToolName( const wstring& full ) {
  if( AnsiOK(full) ) return full;
  wchar_t s[MAX_PATH];
  DWORD n = GetShortPathNameW( LongPath(full).c_str(), s, MAX_PATH );
  if( (n>0) && (n<MAX_PATH) && (wcsncmp( s, L"\\\\?\\", 4 )!=0) && AnsiOK(s) ) return s;
  return wstring();
}

// what we accept as a stream or packed file name (from avip, or from an index)
static int SafeName( const string& n ) {
  if( n.empty() || (n.size()>200) || (n[0]=='.') || (n[0]==' ') || (n[n.size()-1]=='.') || (n[n.size()-1]==' ') ) return 0;
  for( size_t i=0; i<n.size(); i++ ) if( (byte(n[i])<32) || strchr( "\\/:*?\"<>|", n[i] ) ) return 0;
  string b = n.substr( 0, n.find('.') );
  for( size_t i=0; i<b.size(); i++ ) if( (b[i]>='a') && (b[i]<='z') ) b[i] -= 'a'-'A';
  static const char* dev[] = { "CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$" };
  for( size_t i=0; i<sizeof(dev)/sizeof(dev[0]); i++ ) if( b==dev[i] ) return 0;
  if( (b.size()==4) && ((b.compare(0,3,"COM")==0) || (b.compare(0,3,"LPT")==0)) && (b[3]>='0') && (b[3]<='9') ) return 0;
  return 1;
}

static int GetSize( const wstring& p, qword& sz ) {
  WIN32_FILE_ATTRIBUTE_DATA a;
  if( !GetFileAttributesExW( p.c_str(), GetFileExInfoStandard, &a ) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) return 1;
  sz = (qword(a.nFileSizeHigh)<<32) | a.nFileSizeLow;
  return 0;
}

static int Exists( const wstring& p ) { qword s; return GetSize( p, s )==0; }

// an archive's name: x.mpk
static int IsArchive( const wstring& f ) { return (f.size()>4) && (lstrcmpiW( f.c_str()+f.size()-4, L".mpk" )==0); }

// the size of a file another process is writing (on Windows a directory
// entry's size lags until the writer closes it: this asks the file itself)
static int LiveSize( const wstring& p, qword& sz ) {
#ifdef _WIN32
  HANDLE f = CreateFileW( p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  LARGE_INTEGER s;
  int r = !GetFileSizeEx( f, &s );
  CloseHandle( f );
  sz = r ? 0 : qword(s.QuadPart);
  return r;
#else
  return GetSize( p, sz );
#endif
}

static int IsDir( const wstring& p ) {
  WIN32_FILE_ATTRIBUTE_DATA a;
  return GetFileAttributesExW( p.c_str(), GetFileExInfoStandard, &a ) && (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

static HANDLE OpenRead( const wstring& p ) {
  return CreateFileW( p.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0 );
}

static HANDLE OpenWrite( const wstring& p, DWORD how ) {
  return CreateFileW( p.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, how, 0, 0 );
}

// 1 when both name the same existing file
static int SameFile( const wstring& a, const wstring& b ) {
  HANDLE f = CreateFileW( a.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
  HANDLE g = CreateFileW( b.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
  BY_HANDLE_FILE_INFORMATION x, y;
  int r = (f!=INVALID_HANDLE_VALUE) && (g!=INVALID_HANDLE_VALUE) && GetFileInformationByHandle( f, &x ) && GetFileInformationByHandle( g, &y )
       && (x.dwVolumeSerialNumber==y.dwVolumeSerialNumber) && (x.nFileIndexHigh==y.nFileIndexHigh) && (x.nFileIndexLow==y.nFileIndexLow);
  if( f!=INVALID_HANDLE_VALUE ) CloseHandle( f );
  if( g!=INVALID_HANDLE_VALUE ) CloseHandle( g );
  return r;
}

struct IOBuf {
  byte* p; DWORD n;
  IOBuf( DWORD size ) { n = size; p = (byte*)VirtualAlloc( 0, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE ); }
  ~IOBuf() { if( p ) VirtualFree( p, 0, MEM_RELEASE ); }
};

// CRC32 and size of a file; 1 on an error (GetLastError's then)
static int FileCrc( const wstring& path, uint& crc, qword& size ) {
  crc = 0; size = 0;
  HANDLE f = OpenRead( path );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  IOBuf b( 1<<20 );
  int r = (b.p==0);
  DWORD e = r ? ERROR_NOT_ENOUGH_MEMORY : 0;
  while( !r ) {
    DWORD n = 0;
    if( f_abort || !ReadFile( f, b.p, b.n, &n, 0 ) ) { e = f_abort ? 0 : GetLastError(); r = 1; break; }
    if( n==0 ) break;
    crc = Crc( crc, b.p, n ); size += n;
  }
  CloseHandle( f );
  SetLastError( e );
  return r;
}

struct CrcTask { wstring path; uint crc; qword size; int err; DWORD sys; };
static DWORD WINAPI CrcThread( void* a ) { CrcTask& t = *(CrcTask*)a; t.err = FileCrc( t.path, t.crc, t.size ); t.sys = t.err ? GetLastError() : 0; return 0; }

// n bytes, all of them: 0, or 1 (an error, or fewer). A write that wrote
// less is tried again for the rest, which tells the error (wine writes what
// fits on a full disk, and says it went well)
static int ReadN( HANDLE f, void* p, DWORD n ) { DWORD k = 0; return !ReadFile( f, p, n, &k, 0 ) || (k!=n); }
static int WriteN( HANDLE f, const void* p, size_t n ) {
  for( const byte* b = (const byte*)p; n; ) {
    DWORD k = 0;
    if( !WriteFile( f, b, DWORD(n), &k, 0 ) ) return 1;
    if( k==0 ) { SetLastError( 0 ); return 1; } // (none, and no error: not known why)
    b += k; n -= k;
  }
  return 0;
}

// len bytes from off of one file to the end of another; 0, or 1 (an error:
// GetLastError's, 0 if the file ended first)
static int CopyRange( const wstring& from, qword off, qword len, HANDLE to ) {
  HANDLE f = OpenRead( from );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  IOBuf b( 1<<20 );
  LARGE_INTEGER o; o.QuadPart = LONGLONG(off);
  int r = (b.p==0) || !SetFilePointerEx( f, o, 0, FILE_BEGIN );
  DWORD e = !r ? 0 : b.p ? GetLastError() : ERROR_NOT_ENOUGH_MEMORY;
  while( !r && len ) {
    DWORD n = DWORD( std::min<qword>( b.n, len ) ), k = 0;
    SetLastError( 0 );
    if( f_abort || !ReadFile( f, b.p, n, &k, 0 ) || (k!=n) || WriteN( to, b.p, k ) ) { e = GetLastError(); r = 1; }
    len -= n;
  }
  CloseHandle( f );
  SetLastError( e );
  return r;
}

static int ReadAll( const wstring& path, string& t, qword limit ) {
  qword sz;
  if( GetSize( path, sz ) || (sz>limit) ) return 1;
  HANDLE f = OpenRead( path );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  t.assign( size_t(sz), 0 );
  DWORD n = 0;
  int r = sz && (!ReadFile( f, &t[0], DWORD(sz), &n, 0 ) || (n!=sz));
  CloseHandle( f );
  return r;
}

// (these two: 1 on an error, GetLastError's)
static int WriteAll( const wstring& path, const string& t ) {
  HANDLE f = OpenWrite( path, CREATE_ALWAYS );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  int r = WriteN( f, t.data(), t.size() );
  DWORD e = r ? GetLastError() : 0;
  CloseHandle( f );
  SetLastError( e );
  return r;
}

// a file of this size (filled in later, at offsets)
static int MakeFile( const wstring& path, qword size ) {
  HANDLE f = OpenWrite( path, CREATE_ALWAYS );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  LARGE_INTEGER o; o.QuadPart = LONGLONG(size);
  int r = !SetFilePointerEx( f, o, 0, FILE_BEGIN ) || !SetEndOfFile( f );
  DWORD e = r ? GetLastError() : 0;
  CloseHandle( f );
  SetLastError( e );
  return r;
}

// d (ending with a separator) and the directories above it, those that are
// not there; 0, or the error of the first that could not be made (one that
// cannot be made is followed by ones that cannot be, for want of it)
static DWORD MakeDirs( const wstring& d ) {
  DWORD e = 0;
  for( size_t i=1; i<d.size(); i++ ) if( IsSep(d[i]) && (d[i-1]!=':') && !IsSep(d[i-1]) ) {
    wstring p = d.substr( 0, i );
    if( CreateDirectoryW( p.c_str(), 0 ) ) { e = 0; continue; }
    DWORD x = GetLastError();
    e = IsDir( p ) ? 0 : e ? e : x; // (\\server, \\? are not directories, and fail)
  }
  return e;
}

// why directory d (ending with a separator) could not be made, MakeDirs's
// error e: as ": reason", or ": <file> is a file" when one is in the way
static string DirErr( const wstring& d, DWORD e ) {
  for( size_t i=1; i<d.size(); i++ ) if( IsSep(d[i]) && Exists( LongPath( d.substr( 0, i ) ) ) ) return ": " + U( d.substr( 0, i ) ) + " is a file";
  return SysErr(e);
}

// a file, or a directory (named with a trailing separator) and all in it
static void Remove( const wstring& p ) {
  if( p.empty() ) return;
  if( !IsSep( p[p.size()-1] ) ) {
    if( !DeleteFileW( p.c_str() ) ) { SetFileAttributesW( p.c_str(), FILE_ATTRIBUTE_NORMAL ); DeleteFileW( p.c_str() ); }
    return;
  }
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (p+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      wstring n = fd.cFileName;
      if( (n==L".") || (n==L"..") ) continue;
      if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ) Remove( p+n+L"" DS );
      else if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) RemoveDirectoryW( (p+n).c_str() ); // (a link: not into it)
      else Remove( p+n );
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  RemoveDirectoryW( p.substr(0,p.size()-1).c_str() );
}

// avip's files in their order: s.0.*, s.1.*, ..., then the rest (s.meta)
static qword NameKey( const wstring& n ) {
  if( (n.size()<3) || (n[0]!='s') || (n[1]!='.') || (n[2]<'0') || (n[2]>'9') ) return ALL;
  qword v = 0;
  for( size_t i=2; (i<n.size()) && (n[i]>='0') && (n[i]<='9') && (v<(1ull<<40)); i++ ) v = v*10+(n[i]-'0');
  return v;
}

static bool NameLess( const wstring& a, const wstring& b ) {
  qword x = NameKey(a), y = NameKey(b);
  return (x!=y) ? (x<y) : (a<b);
}

static qword DirSize( const wstring& dir ) { // dir ends with a separator
  qword n = 0;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (dir+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      wstring f = fd.cFileName;
      if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) n += (qword(fd.nFileSizeHigh)<<32) | fd.nFileSizeLow;
      else if( (f!=L".") && (f!=L"..") ) n += DirSize( dir+f+L"" DS );
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  return n;
}

static vector<wstring> ListFiles( const wstring& dir ) {
  vector<wstring> r;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (dir+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) r.push_back( fd.cFileName ); while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  std::sort( r.begin(), r.end(), NameLess );
  return r;
}

