// command line driver: container detection, file io, dispatch

static const int FMT_NONE = 0;

static int Probe( const byte* h, qword n ) {
  if( n>=4 && h[0]==0x1A && h[1]==0x45 && h[2]==0xDF && h[3]==0xA3 ) return FMT_MKV;
  if( n>=12 && !memcmp(h,"RIFF",4) && (!memcmp(h+8,"AVI ",4) || !memcmp(h+8,"AVIX",4)) ) return FMT_AVI;
  static const char* bx[] = { "ftyp","styp","moov","mdat","free","skip","wide","moof","sidx","pnot",0 };
  if( n>=8 ) for( int i=0; bx[i]; i++ ) if( !memcmp(h+4,bx[i],4) ) return FMT_MP4;
  return FMT_NONE;
}

template<class M, bool (*Parse)( const Buf&, M& ), std::string (*Name)( const M&, size_t ),
         bool (*Dec)( M&, const std::vector<Buf>&, Sink&, const char** )>
static int DecodeWith( const Buf& meta, const char* prefix, const char* oname ) {
  M m;
  if( !Parse( meta, m ) ) { fprintf( stderr, "bad metainfo %s.meta\n", prefix ); return 4; }
  std::vector<Buf> S( m.trk.size() );
  for( size_t i=0; i<S.size(); i++ ) {
    std::string sn = std::string(prefix)+Name( m, i );
    if( !LoadFile(sn.c_str(),S[i]) ) { fprintf( stderr, "can't read %s\n", sn.c_str() ); return 2; }
  }
  FILE* g = fopen( oname, "wb" );
  if( !g ) { fprintf( stderr, "can't write %s\n", oname ); return 3; }
  Sink out( g, 0 ); const char* why="";
  bool ok = Dec( m, S, out, &why );
  if( fclose(g)!=0 ) ok=false, why="write error";
  if( !ok ) { fprintf( stderr, "restore failed: %s\n", why ); remove( oname ); return 4; }
  printf( "%s: %llu\n", oname, (unsigned long long)out.pos );
  return 0;
}

// would the decoder take x for a meta (rather than a plain copy)?
static bool ParsesAsMeta( const Buf& x ) {
  int f = MetaFormat( x.data(), x.size() );
  if( f==FMT_STORED ) return true;
  if( f==FMT_AVI ) { avi::Meta m; return avi::ParseMeta( x, m ); }
  if( f==FMT_MP4 ) { mp4::Meta m; return mp4::ParseMeta( x, m ); }
  if( f==FMT_MKV ) { mkv::Meta m; return mkv::ParseMeta( x, m ); }
  return false;
}

// unrecognized input: the meta is the input itself, or 0x00 + input if that would be
// mistaken for a meta
static void PlainCopy( const Buf& in, Buf& meta ) {
  meta.clear();
  if( ParsesAsMeta( in ) ) meta.push_back( 0 );
  meta.insert( meta.end(), in.begin(), in.end() );
}

static bool SaveOut( const char* name, const byte* p, qword n ) {
  if( !SaveFile( name, p, n ) ) { fprintf( stderr, "can't write %s\n", name ); return false; }
  printf( "%s: %llu\n", name, (unsigned long long)n );
  return true;
}

// info about a vp meta: container, per-track stream name, transform, frames, size model
template<class M, bool (*Parse)( const Buf&, M& )>
static void MetaInfo( const Buf& meta, const char* fmt ) {
  static const char* xfn[] = { "none", "annexb", "adts", "blob", "hdr", "ogg" };
  static const char* szn[] = { "explicit", "tables", "units", "xform" };
  M m; Parse( meta, m );
  printf( "vp meta: %s, %llu bytes, %llu tracks\n", fmt, (unsigned long long)meta.size(), (unsigned long long)m.trk.size() );
  for( size_t i=0; i<m.trk.size(); i++ ) {
    const TrackHead& h = m.trk[i].h;
    printf( "  %-16s xform=%s", TrackSuffix( i, h.ext ).c_str(), xfn[h.xform] );
    if( h.xform==XF_BLOB ) { printf( "\n" ); continue; }
    printf( " frames=%llu sizes=%s", (unsigned long long)h.nf, szn[h.szmode] );
    if( h.szmode==SZ_UNITS ) {
      qword ne = 0; for( size_t k=0; k<h.col.size(); k++ ) ne += h.col[k]==0;
      printf( "/%s esc=%llu", ptname[h.ptype], (unsigned long long)ne );
    }
    printf( "\n" );
  }
}

// vp file: what "vp c" would produce, without writing anything (container analysis and
// verification run in memory); for a vp meta, its track list
static int Info( const char* fn ) {
  Buf in;
  if( !LoadFile(fn,in) ) { fprintf( stderr, "can't read %s\n", fn ); return 2; }
  int f = Probe( in.data(), in.size() );
  static const char* fname[] = { "unknown", "MP4/MOV", "Matroska/WebM", "AVI" };
  if( f==FMT_NONE ) {
    if( !ParsesAsMeta( in ) ) { printf( "%s: unknown format, %llu bytes (stored as a plain copy)\n", fn, (unsigned long long)in.size() ); return 0; }
    printf( "%s: ", fn );
    int mf = MetaFormat( in.data(), in.size() );
    if( mf==FMT_STORED ) printf( "vp meta: stored copy, %llu bytes\n", (unsigned long long)in.size()-1 );
    else if( mf==FMT_AVI ) MetaInfo<avi::Meta,avi::ParseMeta>( in, "AVI" );
    else if( mf==FMT_MP4 ) MetaInfo<mp4::Meta,mp4::ParseMeta>( in, "MP4/MOV" );
    else MetaInfo<mkv::Meta,mkv::ParseMeta>( in, "Matroska/WebM" );
    return 0;
  }
  printf( "%s: %s, %llu bytes\n", fn, fname[f], (unsigned long long)in.size() );
  fflush( stdout );
  int v0 = verbose; verbose = 1;
  Buf meta; std::vector<Buf> S; std::vector<std::string> N;
  bool ok = f==FMT_MKV ? mkv::Encode( in, meta, S, N ) : f==FMT_AVI ? avi::Encode( in, meta, S, N ) : mp4::Encode( in, meta, S, N );
  verbose = v0;
  if( ok && S.empty() && meta.size()>=in.size() ) ok = false;
  if( !ok ) { printf( "no gain or not restorable: stored as a plain copy\n" ); return 0; }
  qword tot = meta.size();
  for( size_t i=0; i<S.size(); i++ ) tot += S[i].size(), printf( "%s: %llu\n", N[i].c_str()+1, (unsigned long long)S[i].size() );
  printf( "meta: %llu\n", (unsigned long long)meta.size() );
  printf( "%llu -> %llu\n", (unsigned long long)in.size(), (unsigned long long)tot );
  return 0;
}

static int Main( int argc, char** argv ) {
  // vp file: info. Else mode c|d first, then options, then the two file names ("--" ends options)
  if( argc>1 && strcmp(argv[1],"c") && strcmp(argv[1],"d") ) {
    int a = 1;
    while( a<argc && argv[a][0]=='-' && argv[a][1] ) {
      if( !strcmp(argv[a],"--") ) { a++; break; }
      if( !strcmp(argv[a],"-v") ) a++;
      else { fprintf( stderr, "unknown option %s\n", argv[a] ); return 1; }
    }
    if( argc-a==1 ) return Info( argv[a] );
  }
  bool ok = argc>1 && (!strcmp(argv[1],"c") || !strcmp(argv[1],"d"));
  int a = 2;
  while( ok && a<argc && argv[a][0]=='-' && argv[a][1] ) {
    if( !strcmp(argv[a],"--") ) { a++; break; }
    if( !strcmp(argv[a],"-v") ) verbose=1, a++;
    else { fprintf( stderr, "unknown option %s\n", argv[a] ); ok = false; }
  }
  if( !ok || argc-a!=2 ) {
    fprintf( stderr,
      "vp - lossless mp4/mkv/avi dump/restore\n"
      "  vp c [-v] input out_prefix   split into out_prefix.meta + per-track streams\n"
      "  vp d [-v] inp_prefix output  restore\n"
      "  vp file                      info: container, tracks, what c would write (nothing is written)\n" );
    return 1;
  }
  const char* f1 = argv[a]; const char* f2 = argv[a+1];
  if( argv[1][0]=='c' ) {
    Buf in, meta;
    if( !LoadFile(f1,in) ) { fprintf( stderr, "can't read %s\n", f1 ); return 2; }
    std::vector<Buf> S; std::vector<std::string> N;
    int f = Probe( in.data(), in.size() );
    bool ok = false;
    if( f==FMT_MKV ) ok = mkv::Encode( in, meta, S, N );
    else if( f==FMT_AVI ) ok = avi::Encode( in, meta, S, N );
    else if( f==FMT_MP4 ) ok = mp4::Encode( in, meta, S, N );
    if( ok && S.empty() && meta.size()>=in.size() ) ok = false;     // nothing gained over a copy
    else if( !ok && f!=FMT_NONE ) fprintf( stderr, "warning: storing as a plain copy\n" );
    if( !ok ) { PlainCopy( in, meta ); S.clear(); N.clear(); }
    std::string mn = std::string(f2)+".meta";
    if( !SaveFile(mn.c_str(),meta.data(),meta.size()) ) { fprintf( stderr, "can't write %s\n", mn.c_str() ); return 3; }
    qword tot = meta.size();
    for( size_t i=0; i<S.size(); i++ ) {
      std::string sn = std::string(f2)+N[i];
      if( !SaveFile(sn.c_str(),S[i].data(),S[i].size()) ) { fprintf( stderr, "can't write %s\n", sn.c_str() ); return 3; }
      tot += S[i].size();
      printf( "%s: %llu\n", sn.c_str(), (unsigned long long)S[i].size() );
    }
    printf( "%s: %llu\n", mn.c_str(), (unsigned long long)meta.size() );
    printf( "%llu -> %llu\n", (unsigned long long)in.size(), (unsigned long long)tot );
  } else {
    Buf meta;
    std::string mn = std::string(f1)+".meta";
    if( !LoadFile(mn.c_str(),meta) ) { fprintf( stderr, "can't read %s\n", mn.c_str() ); return 2; }
    int f = MetaFormat( meta.data(), meta.size() );
    if( f==FMT_STORED ) return SaveOut( f2, meta.data()+1, meta.size()-1 ) ? 0 : 3;
    if( f==FMT_AVI && ParsesAsMeta( meta ) ) return DecodeWith<avi::Meta,avi::ParseMeta,avi::TrackName,avi::Decode>( meta, f1, f2 );
    if( f==FMT_MP4 && ParsesAsMeta( meta ) ) return DecodeWith<mp4::Meta,mp4::ParseMeta,mp4::TrackName,mp4::Decode>( meta, f1, f2 );
    if( f==FMT_MKV && ParsesAsMeta( meta ) ) return DecodeWith<mkv::Meta,mkv::ParseMeta,mkv::TrackName,mkv::Decode>( meta, f1, f2 );
    return SaveOut( f2, meta.data(), meta.size() ) ? 0 : 3;     // plain copy
  }
  return 0;
}
