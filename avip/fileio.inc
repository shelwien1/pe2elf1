static bool LoadFile( const char* name, Buf& b ) {
  FILE* f = fopen( name, "rb" ); if( !f ) return false;
  b.clear();
  if( fseek64(f,0,SEEK_END)==0 ) {
    long long sz = ftell64(f);
    if( sz>0 ) b.reserve( size_t(sz)+(1<<20) );     // room for the last read: no reallocation
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
