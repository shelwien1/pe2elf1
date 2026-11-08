// the file name as b: mapped (read-only) where it can be, else read
static bool LoadFile( const char* name, Buf& b ) {
  if( FileMap* m = FmOpenRead( name ) ) { b.AdoptView( m ); return true; }
  FILE* f = fopen( name, "rb" ); if( !f ) return false;
  b.clear();
  if( fseek64(f,0,SEEK_END)==0 ) {
    long long sz = ftell64(f);
    if( sz>0 ) b.reserve( size_t(sz)+(1<<20) );     // room for the last read: no reallocation (a hint, as Buf takes it)
    fseek64( f, 0, SEEK_SET );
  }
  const size_t K = 1<<20; size_t n;
  do {
    size_t o = b.size(); b.resize( o+K );
    n = fread( &b[o], 1, K, f ); b.resize( o+n );
  } while( n==K );
  bool err = ferror(f)!=0; fclose(f);
  return !err;
}

static bool SaveFile( const char* name, const byte* p, size_t n ) {
  FILE* f = fopen( name, "wb" ); if( !f ) return false;
  bool ok = n==0 || fwrite( p, 1, n, f )==n;
  if( fclose(f)!=0 ) ok = false;
  return ok;
}

// b as the file name: a spill file renamed into place (b is empty after), else written
static bool SaveBuf( const char* name, Buf& b ) {
  bool ok;
  if( b.KeepAs( name, ok ) ) return ok;
  const Buf& c = b;
  return SaveFile( name, c.data(), c.size() );
}
