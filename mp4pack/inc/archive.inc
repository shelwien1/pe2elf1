// inc/archive.inc -- the archive: members of the packed files and the index
//  (WriteArchive, ScanArchive, ReadArchive)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- the archive

// The output: an archive of one or more members, one after another, a
// member for each file packed into it (c of a list into one file). A member
// is the packed files and the index, stored as they are, one after another
// (the numbers little-endian):
//   "mp4pack" (7 bytes), the format's version (1 byte: ARC_VERSION), the
//   number of files (4 bytes); then for each file the length of its name (2
//   bytes), the name (UTF-8, '/' between directories: store/s.0.h264), its
//   size (8 bytes) and its bytes.
// mp4pack.idx goes first, then the rest by name. Version 1 had no more than
// one member (its archives are read as well).

static const int ARC_VERSION = 2;

static void PutLE( string& s, qword v, int n ) { for( int i=0; i<n; i++ ) s += char( byte( v>>(8*i) ) ); }
static qword GetLE( const byte* p, int n ) { qword v = 0; for( int i=n-1; i>=0; i-- ) v = (v<<8) | p[i]; return v; }

// the files under dir (ending with a separator), by names relative to it
static void ListAll( const wstring& dir, const string& pre, vector<string>& v ) {
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (dir+L"*").c_str(), &fd );
  if( h==INVALID_HANDLE_VALUE ) return;
  do {
    wstring n = fd.cFileName;
    if( (n==L".") || (n==L"..") ) continue;
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ListAll( dir+n+L"" DS, pre+U(n)+"/", v );
    else v.push_back( pre+U(n) );
  } while( FindNextFileW( h, &fd ) );
  FindClose( h );
}

// a name in the archive as a path under dir
static wstring ArcPath( const wstring& dir, const string& n ) {
  wstring p = dir+W(n);
  for( size_t i=dir.size(); i<p.size(); i++ ) if( p[i]=='/' ) p[i] = DS[0];
  return p;
}

// a name from an archive: plain names (SafeName's) joined by '/'
static int ArcName( const string& n ) {
  for( size_t a=0; ; ) {
    size_t b = n.find( '/', a );
    if( !SafeName( n.substr( a, (b==string::npos) ? string::npos : b-a ) ) ) return 0;
    if( b==string::npos ) return 1;
    a = b+1;
  }
}

// the archive's progress shown, and a wait while Pause holds; 1 when it is to
// stop (Ctrl+C)
static int ArcWait( Job& j ) {
  vector<Job*> jv( 1, &j );
  Status( jv, jv );
  if( f_pause ) j.t0 += PauseWait();
  return f_abort!=0;
}

// a member of an archive: where it is and its size, its index, its files
// (their names and sizes), and the file it was made of (the index's src)
struct Member { qword off, size, src_size; int ver; string idx, src; vector<string> names; vector<qword> sizes; Member() : off(0), size(0), src_size(0), ver(0) {} };

// the archive of what is in dir (ending with a separator) as out, or a
// member of it added to the archive out (append), each file removed once it
// is in (unless -k); job j shows it (its work: the files' bytes; out.done:
// those in so far; made: the member's size); m: where it went. 0, or 2
// (cannot read or write: out_failed when out cannot be written), 5
// (stopped). What failed is taken off again (the archive removed, or what
// it was before it got more)
static int WriteArchive( const wstring& dir, const wstring& out, Job& j, int append, Member& m ) {
  vector<string> v;
  ListAll( dir, "", v );
  std::sort( v.begin(), v.end(), []( const string& a, const string& b ) { return (a=="mp4pack.idx") || ((b!="mp4pack.idx") && (a<b)); } );
  phase = j.label; t_run = j.t0 = GetTickCount64(); j.st = 1;
  HANDLE f = OpenWrite( LongPath(out), append ? OPEN_EXISTING : CREATE_ALWAYS );
  if( f==INVALID_HANDLE_VALUE ) { DWORD e = GetLastError(); j.st = 3; out_failed = 1; Msg( "cannot write " + U(out) + SysErr(e) ); return 2; }
  LARGE_INTEGER at; at.QuadPart = 0;
  if( append ) { LARGE_INTEGER z0; z0.QuadPart = 0; SetFilePointerEx( f, z0, &at, FILE_END ); }
  m = Member(); m.off = qword(at.QuadPart);
  IOBuf b( 1<<20 );
  string h = "mp4pack";
  PutLE( h, ARC_VERSION, 1 ); PutLE( h, v.size(), 4 );
  string why;
  DWORD e = 0; // a write's error
  int r = (b.p==0) || WriteN( f, h.data(), h.size() );
  if( r ) e = b.p ? GetLastError() : ERROR_NOT_ENOUGH_MEMORY;
  for( size_t i=0; (i<v.size()) && !r; i++ ) {
    wstring p = ArcPath( dir, v[i] );
    qword size, left;
    HANDLE g = GetSize( p, size ) ? INVALID_HANDLE_VALUE : OpenRead( p );
    if( g==INVALID_HANDLE_VALUE ) { DWORD x = GetLastError(); why = "cannot read " + U(p) + SysErr(x); r = 2; break; }
    h.clear(); PutLE( h, v[i].size(), 2 ); h += v[i]; PutLE( h, size, 8 );
    if( (r = WriteN( f, h.data(), h.size() ))!=0 ) e = GetLastError();
    for( left = size; left && !r; ) {
      DWORD n = DWORD( std::min<qword>( b.n, left ) ), k = 0;
      BOOL ok = ReadFile( g, b.p, n, &k, 0 );
      if( !ok || (k!=n) ) { DWORD x = ok ? 0 : GetLastError(); why = "cannot read " + U(p) + SysErr(x); r = 2; break; }
      if( (r = WriteN( f, b.p, k ))!=0 ) { e = GetLastError(); break; }
      left -= k; j.done += k;
      if( ArcWait( j ) ) r = 5;
    }
    CloseHandle( g );
    if( !r && !f_keep ) Remove( p );
  }
  LARGE_INTEGER z;
  if( !r ) j.made = GetFileSizeEx( f, &z ) ? qword(z.QuadPart)-m.off : 0;
  if( r && append ) { SetFilePointerEx( f, at, 0, FILE_BEGIN ); SetEndOfFile( f ); } // (as it was)
  CloseHandle( f );
  if( (r==1) || (r==2) ) { if( why.empty() ) { why = "cannot write " + U(out) + SysErr(e); out_failed = 1; } r = 2; }
  j.st = r ? 3 : 2; j.t = GetTickCount64()-j.t0;
  if( status_hook ) { vector<Job*> jv( 1, &j ); Status( jv, jv, 1 ); }
  if( r && !append ) Remove( LongPath(out) );
  if( r && why.size() ) Msg( why );
  m.size = j.made;
  return r;
}

// the members of an archive (each one's files listed and its index read,
// the rest passed over). 0, or 2 (cannot read it), 4 (not an mp4pack
// archive, or not all of one: v has the members whole before what is
// wrong); why: what is wrong with it
static int ScanArchive( const wstring& path, vector<Member>& v, string& why ) {
  v.clear(); why.clear();
  HANDLE f = OpenRead( LongPath(path) );
  if( f==INVALID_HANDLE_VALUE ) { DWORD e = GetLastError(); why = "cannot read it" + SysErr(e); return 2; }
  LARGE_INTEGER z, o; z.QuadPart = 0;
  GetFileSizeEx( f, &z );
  qword end = qword(z.QuadPart), off = 0;
  byte h[16];
  int r = 0;
  while( !r && ((off<end) || v.empty()) ) {
    Member m; m.off = off;
    o.QuadPart = LONGLONG(off);
    if( !SetFilePointerEx( f, o, 0, FILE_BEGIN ) || ReadN( f, h, 12 ) || memcmp( h, "mp4pack", 7 ) ) {
      why = v.empty() ? "not an mp4pack archive" : "damaged after its " + Num(v.size()) + ((v.size()==1) ? " file" : " files");
      r = 4; break;
    }
    if( (h[7]<1) || (h[7]>ARC_VERSION) ) { why = "an mp4pack archive of another version (" + Num(h[7]) + ", not " + Num(ARC_VERSION) + ")"; r = 4; break; }
    m.ver = h[7];
    qword n = GetLE( h+8, 4 ), pos = off+12;
    for( qword i=0; (i<n) && !r; i++ ) {
      string a;
      if( !ReadN( f, h, 2 ) ) a.resize( size_t( GetLE( h, 2 ) ) );
      if( a.empty() || ReadN( f, &a[0], DWORD(a.size()) ) || ReadN( f, h, 8 ) ) { r = 4; break; }
      qword len = GetLE( h, 8 );
      pos += 10+a.size();
      if( (len>end) || (pos+len>end) ) { r = 4; break; }
      o.QuadPart = LONGLONG(len);
      if( (a=="mp4pack.idx") && m.idx.empty() && (len<(1u<<20)) ) { m.idx.resize( size_t(len) ); if( len && ReadN( f, &m.idx[0], DWORD(len) ) ) r = 4; }
      else if( !SetFilePointerEx( f, o, 0, FILE_CURRENT ) ) r = 4;
      pos += len;
      m.names.push_back( a ); m.sizes.push_back( len );
    }
    if( r ) { why = "not all of an mp4pack archive (cut short, or damaged)"; break; }
    if( m.idx.empty() ) { why = "no index in its file " + Num(v.size()+1) + ": not an mp4pack archive?"; r = 4; break; }
    Index x; // (the file it was made of; a bad index: none, as d tells)
    if( !ParseIndex( m.idx, x, why ) ) { m.src = x.src_name; m.src_size = x.src_size; }
    why.clear();
    m.size = pos-off;
    v.push_back( m );
    off = pos;
  }
  CloseHandle( f );
  return r;
}

// the files of the member of archive in at off into dir (ending with a
// separator); job j shows it (in.len: the member's size; in.done: read so
// far). 0, or 2 (cannot read or write), 4 (not an mp4pack archive, or not
// all of one), 5 (stopped)
static int ReadArchive( const wstring& in, qword off, const wstring& dir, Job& j ) {
  phase = j.label; t_run = j.t0 = GetTickCount64(); j.st = 1;
  HANDLE f = OpenRead( LongPath(in) );
  if( f==INVALID_HANDLE_VALUE ) { DWORD e = GetLastError(); j.st = 3; Msg( "cannot read " + U(in) + SysErr(e) ); return 2; }
  IOBuf b( 1<<20 );
  byte h[16];
  string why;
  LARGE_INTEGER o; o.QuadPart = LONGLONG(off);
  int r = (b.p==0) ? 2 : 0;
  if( r ) why = "cannot read " + U(in) + SysErr( ERROR_NOT_ENOUGH_MEMORY );
  if( !r && (!SetFilePointerEx( f, o, 0, FILE_BEGIN ) || ReadN( f, h, 12 ) || memcmp( h, "mp4pack", 7 )) ) { why = U(in) + ": not an mp4pack archive"; r = 4; }
  if( !r && ((h[7]<1) || (h[7]>ARC_VERSION)) ) { why = U(in) + ": an mp4pack archive of another version (" + Num(h[7]) + ", not " + Num(ARC_VERSION) + ")"; r = 4; }
  qword files = r ? 0 : GetLE( h+8, 4 );
  j.done = 12;
  for( qword i=0; (i<files) && !r; i++ ) {
    string n;
    if( ReadN( f, h, 2 ) ) { r = 4; break; }
    n.resize( size_t( GetLE( h, 2 ) ) );
    if( n.empty() || ReadN( f, &n[0], DWORD(n.size()) ) || ReadN( f, h, 8 ) ) { r = 4; break; }
    if( !ArcName( n ) ) { why = U(in) + ": a name that is not allowed in it: " + n; r = 4; break; }
    qword left = GetLE( h, 8 );
    j.done += 10+n.size();
    wstring p = ArcPath( dir, n );
    MakeDirs( p.substr( 0, p.size()-FileName( p ).size() ) );
    HANDLE g = OpenWrite( p, CREATE_NEW ); // (each name once)
    if( g==INVALID_HANDLE_VALUE ) {
      DWORD e = GetLastError();
      if( Exists( p ) ) { why = U(in) + ": " + n + " is in it twice"; r = 4; }
      else { why = "cannot write " + U(p) + SysErr(e); r = 2; }
      break;
    }
    while( left && !r ) {
      DWORD k = DWORD( std::min<qword>( b.n, left ) );
      if( ReadN( f, b.p, k ) ) r = 4;
      else if( WriteN( g, b.p, k ) ) { DWORD e = GetLastError(); why = "cannot write " + U(p) + SysErr(e); r = 2; }
      else { left -= k; j.done += k; if( ArcWait( j ) ) r = 5; }
    }
    CloseHandle( g );
  }
  CloseHandle( f );
  if( (r==4) && why.empty() ) why = U(in) + ": not all of an mp4pack archive (cut short, or damaged)";
  j.st = r ? 3 : 2; j.t = GetTickCount64()-j.t0;
  if( status_hook ) { vector<Job*> jv( 1, &j ); Status( jv, jv, 1 ); }
  if( why.size() ) Msg( why );
  return r;
}

