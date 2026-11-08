// Buf: the byte vector avip keeps its data in. It has the part of std::vector<byte>'s interface
// the code uses, and three kinds of storage:
//   - the heap, while it is small;
//   - a temporary file mapped into memory, once it would grow past spill_limit (AVIP_SPILL_MB,
//     256 MB by default): its pages belong to the file, so the system writes them out and drops
//     them as it needs memory instead of holding them in RAM. The files go to spill_dir (AVIP_TMP,
//     else the directory of the output) as avip-<pid>-<n>.tmp and are deleted with the Buf;
//     SaveBuf() renames one into place as an output file;
//   - a read-only mapping of a file (MapFileRead(), for the input and the streams): copies of it
//     share the mapping, and one that is written to becomes a copy of its own first.
// As with std::vector, anything that grows a Buf invalidates pointers into it.

static qword spill_limit = qword(256)<<20;
static std::string spill_dir;                             // "" = the current directory
static unsigned spill_seq = 0;
static int mem_debug = 0;                                 // AVIP_DEBUG_MEM: report spills and copies of views

// a temporary file can't be made or grown (no space, no directory): avip stops with this
struct SpillError { std::string what; };

static const qword SPILL_STEP = qword(64)<<20;            // spill files grow and take disk space in these steps

struct FileMap {
  int refs; byte* p; qword len, room;                     // mapped length; spill: bytes given disk space
  bool temp; std::string path;
#ifdef _WIN32
  HANDLE fh, mh;
#else
  int fd;
#endif
};

static void SpillFail( const FileMap* m, const char* what ) {
  SpillError e; e.what = std::string( what ) + " " + (m ? m->path : spill_dir);
  throw e;
}

#ifdef _WIN32

static bool FmMapView( FileMap* m, qword len, bool rw ) {
  m->mh = CreateFileMappingA( m->fh, NULL, rw ? PAGE_READWRITE : PAGE_READONLY, DWORD(len>>32), DWORD(len), NULL );
  if( !m->mh ) return false;
  m->p = (byte*)MapViewOfFile( m->mh, rw ? FILE_MAP_READ|FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, SIZE_T(len) );
  if( !m->p ) { CloseHandle( m->mh ); m->mh = 0; return false; }
  m->len = len;
  return true;
}

static void FmUnmap( FileMap* m ) {
  if( m->p ) UnmapViewOfFile( m->p );
  if( m->mh ) CloseHandle( m->mh );
  m->p = 0; m->mh = 0;
}

// a read-only mapping of the file name (its size in len), 0 if it can't be mapped (or is empty)
static FileMap* FmOpenRead( const char* name ) {
  HANDLE fh = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if( fh==INVALID_HANDLE_VALUE ) return 0;
  LARGE_INTEGER sz;
  if( !GetFileSizeEx( fh, &sz ) || sz.QuadPart<=0 || qword(sz.QuadPart)>SIZE_MAX ) { CloseHandle( fh ); return 0; }
  FileMap* m = new FileMap; m->refs = 1; m->p = 0; m->len = m->room = 0; m->temp = false; m->path = name; m->fh = fh; m->mh = 0;
  if( !FmMapView( m, qword(sz.QuadPart), false ) ) { CloseHandle( fh ); delete m; return 0; }
  return m;
}

static FileMap* FmSpill( qword cap ) {
  FileMap* m = new FileMap; m->refs = 1; m->p = 0; m->len = m->room = 0; m->temp = true; m->mh = 0;
  for( int k=0; k<100; k++ ) {                            // (a name left by an earlier run: the next one)
    char nm[64]; snprintf( nm, sizeof(nm), "avip-%lu-%u.tmp", (unsigned long)GetCurrentProcessId(), spill_seq++ );
    m->path = spill_dir.empty() ? std::string( nm ) : spill_dir + nm;
    m->fh = CreateFileA( m->path.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL );
    if( m->fh!=INVALID_HANDLE_VALUE || GetLastError()!=ERROR_FILE_EXISTS ) break;
  }
  if( m->fh==INVALID_HANDLE_VALUE ) { FileMap t = *m; delete m; SpillFail( &t, "can't create the temporary file" ); }
  if( !FmMapView( m, cap, true ) ) {                      // (the mapping extends the file: no disk space shows here)
    CloseHandle( m->fh ); DeleteFileA( m->path.c_str() ); FileMap t = *m; delete m;
    SpillFail( &t, "no room for the temporary file" );
  }
  m->room = cap;
  return m;
}

static void FmGrow( FileMap* m, qword cap ) {
  FmUnmap( m );
  if( !FmMapView( m, cap, true ) ) SpillFail( m, "no room to grow the temporary file" );
  m->room = cap;
}

static void FmRoom( FileMap*, qword ) {}                    // (extending the mapping took the disk space)

static void FmClose( FileMap* m ) {
  FmUnmap( m );
  CloseHandle( m->fh );
  if( m->temp ) DeleteFileA( m->path.c_str() );
}

// a spill file cut to size bytes and renamed name (moved across volumes if it has to be)
static bool FmKeep( FileMap* m, qword size, const char* name ) {
  FmUnmap( m );
  LARGE_INTEGER li; li.QuadPart = (LONGLONG)size;
  bool ok = SetFilePointerEx( m->fh, li, NULL, FILE_BEGIN ) && SetEndOfFile( m->fh );
  CloseHandle( m->fh );
  ok = ok && MoveFileExA( m->path.c_str(), name, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED );
  if( ok ) SetFileAttributesA( name, FILE_ATTRIBUTE_NORMAL );     // (an output now, no longer a temporary file)
  else DeleteFileA( m->path.c_str() );
  return ok;
}

#else

static FileMap* FmOpenRead( const char* name ) {
  int fd = open( name, O_RDONLY );
  if( fd<0 ) return 0;
  struct stat st;
  if( fstat( fd, &st )!=0 || !S_ISREG( st.st_mode ) || st.st_size<=0 || qword(st.st_size)>SIZE_MAX ) { close( fd ); return 0; }
  void* p = mmap( 0, size_t(st.st_size), PROT_READ, MAP_SHARED, fd, 0 );
  if( p==MAP_FAILED ) { close( fd ); return 0; }
  FileMap* m = new FileMap; m->refs = 1; m->p = (byte*)p; m->len = qword(st.st_size); m->room = 0; m->temp = false; m->path = name; m->fd = fd;
  return m;
}

static void FmUnmap( FileMap* m ) { if( m->p ) munmap( m->p, size_t(m->len) ); m->p = 0; }

static void FmMapRW( FileMap* m, qword cap ) {
  if( ftruncate( m->fd, off_t(cap) )!=0 ) SpillFail( m, "can't grow the temporary file" );
  void* p = mmap( 0, size_t(cap), PROT_READ|PROT_WRITE, MAP_SHARED, m->fd, 0 );
  if( p==MAP_FAILED ) SpillFail( m, "can't map the temporary file" );
  m->p = (byte*)p; m->len = cap;
}

static FileMap* FmSpill( qword cap ) {
  FileMap* m = new FileMap; m->refs = 1; m->p = 0; m->len = m->room = 0; m->temp = true;
  for( int k=0; k<100; k++ ) {                            // (a name left by an earlier run: the next one)
    char nm[64]; snprintf( nm, sizeof(nm), "avip-%lu-%u.tmp", (unsigned long)getpid(), spill_seq++ );
    m->path = spill_dir.empty() ? std::string( nm ) : spill_dir + nm;
    m->fd = open( m->path.c_str(), O_RDWR|O_CREAT|O_EXCL, 0666 );   // (as fopen makes files: it may become an output)
    if( m->fd>=0 || errno!=EEXIST ) break;
  }
  if( m->fd<0 ) { FileMap t = *m; delete m; SpillFail( &t, "can't create the temporary file" ); }
  try { FmMapRW( m, cap ); }
  catch( ... ) { close( m->fd ); unlink( m->path.c_str() ); FileMap t = *m; delete m; SpillFail( &t, "can't map the temporary file" ); }
  return m;
}

static void FmGrow( FileMap* m, qword cap ) { FmUnmap( m ); FmMapRW( m, cap ); }

// disk space for the first n bytes, taken a step at a time: a write to a mapped page the disk has
// no room for would kill the process (SIGBUS), where a failed allocation can be reported
static void FmRoom( FileMap* m, qword n ) {
  if( n<=m->room ) return;
  qword r = std::min( m->len, (n+SPILL_STEP-1)/SPILL_STEP*SPILL_STEP );
#if defined(__linux__)
  int e = posix_fallocate( m->fd, off_t(m->room), off_t(r-m->room) );
  if( e==ENOSPC || e==EFBIG || e==EIO ) SpillFail( m, "no room for the temporary file" );
#endif
  m->room = r;
}

static void FmClose( FileMap* m ) {
  FmUnmap( m );
  close( m->fd );
  if( m->temp ) unlink( m->path.c_str() );
}

static bool FmKeep( FileMap* m, qword size, const char* name ) {
  FmUnmap( m );
  bool ok = ftruncate( m->fd, off_t(size) )==0;
  if( ok && rename( m->path.c_str(), name )!=0 ) {         // another file system: copied
    FILE* g = fopen( name, "wb" );
    ok = g!=0;
    if( ok && size ) {
      void* p = mmap( 0, size_t(size), PROT_READ, MAP_SHARED, m->fd, 0 );
      ok = p!=MAP_FAILED && fwrite( p, 1, size_t(size), g )==size;
      if( p!=MAP_FAILED ) munmap( p, size_t(size) );
    }
    if( g && fclose( g )!=0 ) ok = false;
    unlink( m->path.c_str() );
  }
  close( m->fd );
  return ok;
}

#endif

static void FmRelease( FileMap* m ) { if( m && --m->refs==0 ) { FmClose( m ); delete m; } }

class Buf {
  byte* p_; size_t n_, cap_;
  FileMap* m_;                                            // 0: heap; temp: a spill file of its own; else a shared read-only view

  bool IsView() const { return m_ && !m_->temp; }

  // storage for need bytes (n_ kept): the heap grows by doubling, past spill_limit it moves to a
  // spill file, which grows in eighths
  void Grow( size_t need ) {
    if( IsView() ) Own( need );
    if( need<=cap_ ) return;
    if( !m_ ) {
      size_t c = std::max( need, std::max( cap_*2, size_t(64) ) );
      if( qword(c)<=spill_limit ) {
        byte* q = (byte*)realloc( p_, c );
        if( !q ) throw std::bad_alloc();
        p_ = q; cap_ = c; return;
      }
      ToSpill( need );
      return;
    }
    qword c = std::max( qword(need), qword(cap_)+qword(cap_)/8+SPILL_STEP );
    c = (c+SPILL_STEP-1)/SPILL_STEP*SPILL_STEP;
    FmGrow( m_, c );
    p_ = m_->p; cap_ = size_t(c);
  }

  void ToSpill( size_t need ) {
    qword c = std::max( qword(need), spill_limit+SPILL_STEP );
    c = (c+SPILL_STEP-1)/SPILL_STEP*SPILL_STEP;
    FileMap* m = FmSpill( c );
    if( mem_debug ) fprintf( stderr, "avip: %llu bytes spill to %s\n", (unsigned long long)need, m->path.c_str() );
    FmRoom( m, n_ );
    if( n_ ) memcpy( m->p, p_, n_ );
    free( p_ );
    m_ = m; p_ = m->p; cap_ = size_t(c);
  }

  // a view becomes storage of its own, room for need bytes
  void Own( size_t need ) {
    FileMap* v = m_; const byte* src = p_; size_t n = n_;
    if( mem_debug && n>=spill_limit ) fprintf( stderr, "avip: %llu bytes of %s copied\n", (unsigned long long)n, v->path.c_str() );
    p_ = 0; n_ = 0; cap_ = 0; m_ = 0;
    try {
      Grow( std::max( need, n ) );
      if( m_ ) FmRoom( m_, n );
      memcpy( p_, src, n ); n_ = n;
    } catch( ... ) { FmRelease( v ); throw; }
    FmRelease( v );
  }

  void Room( size_t n ) { if( m_ ) FmRoom( m_, n ); }        // (a spill file: disk space for n bytes)

  void Free() {
    if( m_ ) FmRelease( m_ ); else free( p_ );
    p_ = 0; n_ = cap_ = 0; m_ = 0;
  }

  // a byte range of this Buf's own storage (inserted into it: copied first)
  bool Inside( const byte* a ) const { return p_ && a>=p_ && a<p_+n_; }

  byte* Write() { if( IsView() ) Own( n_ ); return p_; }

public:
  typedef byte value_type; typedef size_t size_type; typedef byte* iterator; typedef const byte* const_iterator;
  typedef byte& reference; typedef const byte& const_reference;

  Buf() : p_(0), n_(0), cap_(0), m_(0) {}
  explicit Buf( size_t n ) : p_(0), n_(0), cap_(0), m_(0) { resize( n ); }
  Buf( size_t n, byte v ) : p_(0), n_(0), cap_(0), m_(0) { resize( n, v ); }
  template<class It, class = typename std::enable_if<!std::is_integral<It>::value>::type>
  Buf( It a, It b ) : p_(0), n_(0), cap_(0), m_(0) { insert( end(), a, b ); }
  Buf( const Buf& o ) : p_(0), n_(0), cap_(0), m_(0) { *this = o; }
  Buf( Buf&& o ) noexcept : p_(o.p_), n_(o.n_), cap_(o.cap_), m_(o.m_) { o.p_ = 0; o.n_ = o.cap_ = 0; o.m_ = 0; }
  ~Buf() { Free(); }

  Buf& operator=( const Buf& o ) {
    if( this==&o ) return *this;
    if( o.IsView() ) { Free(); m_ = o.m_; m_->refs++; p_ = o.p_; n_ = cap_ = o.n_; return *this; }
    clear(); insert( end(), o.p_, o.p_+o.n_ );
    return *this;
  }
  Buf& operator=( Buf&& o ) noexcept {
    if( this!=&o ) { Free(); p_ = o.p_; n_ = o.n_; cap_ = o.cap_; m_ = o.m_; o.p_ = 0; o.n_ = o.cap_ = 0; o.m_ = 0; }
    return *this;
  }

  // the read-only mapping m of a file (FmOpenRead) as this Buf's contents
  void AdoptView( FileMap* m ) { Free(); m_ = m; p_ = m->p; n_ = cap_ = size_t(m->len); }

  // a spill file holding exactly this Buf, cut to size and renamed name; false: not a spill file
  // (or it failed: the Buf is empty either way when it was one)
  bool KeepAs( const char* name, bool& ok ) {
    if( !m_ || !m_->temp ) return false;
    FileMap* m = m_; qword n = n_;
    p_ = 0; n_ = cap_ = 0; m_ = 0;
    ok = FmKeep( m, n, name );
    delete m;
    return true;
  }

  size_t size() const { return n_; }
  bool empty() const { return n_==0; }
  size_t capacity() const { return cap_; }
  const byte* data() const { return p_; }
  byte* data() { return Write(); }
  const byte* begin() const { return p_; }
  const byte* end() const { return p_+n_; }
  byte* begin() { return Write(); }
  byte* end() { return Write()+n_; }
  const byte& operator[]( size_t i ) const { return p_[i]; }
  byte& operator[]( size_t i ) { return Write()[i]; }
  const byte& back() const { return p_[n_-1]; }
  byte& back() { return Write()[n_-1]; }
  const byte& front() const { return p_[0]; }
  byte& front() { return Write()[0]; }

  void clear() { if( m_ ) Free(); else n_ = 0; }          // (a spill file or view goes: its disk space with it)
  void reserve( size_t c ) {                              // (a hint: none past spill_limit, spill files grow as they fill)
    if( c>cap_ && !m_ && qword(c)<=spill_limit ) Grow( c );
  }
  void resize( size_t n ) { resize( n, 0 ); }
  void resize( size_t n, byte v ) {                       // (a view shrinks in place: nothing is written)
    if( n>n_ ) { Grow( n ); Room( n ); memset( p_+n_, v, n-n_ ); }
    n_ = n;
  }
  void push_back( byte v ) {
    if( n_==cap_ || IsView() ) Grow( n_+1 );
    if( m_ && qword(n_)+1>m_->room ) Room( n_+1 );
    p_[n_++] = v;
  }
  void pop_back() { n_--; }

  template<class It, class = typename std::enable_if<!std::is_integral<It>::value>::type>
  void assign( It a, It b ) { Buf t( a, b ); swap( t ); }
  void assign( size_t n, byte v ) { clear(); resize( n, v ); }

  iterator insert( const byte* pos, size_t cnt, byte v ) {
    size_t at = size_t(pos-p_);
    if( !cnt ) return Write()+at;
    Grow( n_+cnt ); Room( n_+cnt );
    memmove( p_+at+cnt, p_+at, n_-at ); memset( p_+at, v, cnt ); n_ += cnt;
    return p_+at;
  }
  iterator insert( const byte* pos, byte v ) { return insert( pos, 1, v ); }
  iterator insert( const byte* pos, const byte* a, const byte* b ) {
    size_t at = size_t(pos-p_), cnt = size_t(b-a);
    if( !cnt ) return Write()+at;
    if( Inside( a ) ) { Buf t( a, b ); return insert( p_+at, t.p_, t.p_+cnt ); }
    Grow( n_+cnt ); Room( n_+cnt );
    memmove( p_+at+cnt, p_+at, n_-at ); memcpy( p_+at, a, cnt ); n_ += cnt;
    return p_+at;
  }
  iterator insert( const byte* pos, byte* a, byte* b ) { return insert( pos, (const byte*)a, (const byte*)b ); }
  template<class It, class = typename std::enable_if<!std::is_integral<It>::value && !std::is_same<It,byte*>::value &&
                                                     !std::is_same<It,const byte*>::value>::type>
  iterator insert( const byte* pos, It a, It b ) {
    Buf t; t.reserve( size_t(std::distance( a, b )) );
    for( ; a!=b; ++a ) t.push_back( byte(*a) );
    return insert( pos, t.p_, t.p_+t.n_ );
  }

  iterator erase( const byte* a, const byte* b ) {
    size_t i = size_t(a-p_), j = size_t(b-p_);
    Write(); memmove( p_+i, p_+j, n_-j ); n_ -= j-i;
    return p_+i;
  }
  iterator erase( const byte* a ) { return erase( a, a+1 ); }

  void swap( Buf& o ) noexcept { std::swap( p_, o.p_ ); std::swap( n_, o.n_ ); std::swap( cap_, o.cap_ ); std::swap( m_, o.m_ ); }

  bool operator==( const Buf& o ) const { return n_==o.n_ && (!n_ || !memcmp( p_, o.p_, n_ )); }
  bool operator!=( const Buf& o ) const { return !(*this==o); }
};

// spill settings from the environment (AVIP_SPILL_MB, AVIP_TMP, AVIP_DEBUG_MEM); dir: the default
// directory for spill files (the output's), "" for the current one
static void SpillSetup( const char* outname ) {
  const char* e = getenv( "AVIP_SPILL_MB" );
  if( e && atoi( e )>0 ) spill_limit = qword(atoi( e ))<<20;
  mem_debug = getenv( "AVIP_DEBUG_MEM" ) ? 1 : 0;
  const char* t = getenv( "AVIP_TMP" );
  std::string d = t && *t ? std::string( t ) : std::string( outname );
  if( !(t && *t) ) {                                      // the output's directory
    size_t k = d.find_last_of( "/\\" );
#ifdef _WIN32
    size_t c = d.find( ':' );
    if( k==std::string::npos && c!=std::string::npos ) k = c;
#endif
    d = k==std::string::npos ? std::string() : d.substr( 0, k+1 );
  } else if( d.back()!='/' && d.back()!='\\' ) d += '/';
  spill_dir = d;
}
