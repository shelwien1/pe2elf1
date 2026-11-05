// inc/options.inc -- -t, -c and -l: parsed, told, the stages they make; the
//  part sizes; the tools' level arguments
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- options

// "3+3,1,1", "50%+50%-2": threads of the video packers, the audio packers, ttfcomp and
// tangelo, in that order. Each is a number, or a percentage of the threads
// there are (all of the CPU's, or the free ones: -f), rounded up. '+' runs a
// group together with the ones before it, ',' after them. Missing ones: the
// audio's is the video's, after it; ttfcomp and tangelo get 1, both at once,
// after the rest ("4" = "4,4,1+1", "3+3" = "3+3,1+1"). A "-N" at the end
// takes N from the threads there are, for the percentages; alone, for the
// default (100%,100% = N,N,1+1: all of them for h264p, then for the audio).
static int ParseThreads( string spec, int all ) {
  int sub = 0;
  size_t m = spec.rfind( '-' );
  if( m!=string::npos ) {
    if( (m+1>=spec.size()) || (spec.size()-m>4) ) return 1;
    for( size_t i=m+1; i<spec.size(); i++ ) { if( (spec[i]<'0') || (spec[i]>'9') ) return 1; sub = sub*10+(spec[i]-'0'); }
    spec.erase( m );
  }
  if( spec.empty() ) spec = "100%,100%";
  int max = std::max( 1, all-sub ), g = 0;
  const char* s = spec.c_str();
  char sep = ',';
  stages.clear();
  while(1) {
    if( (g>=N_GROUP) || (*s<'0') || (*s>'9') ) return 1;
    int v = 0;
    while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>10000 ) return 1; }
    if( *s=='%' ) { s++; v = std::min( 64, (max*v+99)/100 ); }
    else if( v>64 ) return 1;
    th[g] = v;
    if( (g==0) || (sep==',') ) stages.push_back( vector<int>() );
    stages.back().push_back( g++ );
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    sep = *s++;
  }
  for( int g0=g; g<N_GROUP; g++ ) {
    th[g] = (g==G_AUDIO) ? th[G_VIDEO] : 1;
    if( (g!=G_OTHER) || (g0>G_FONT) ) stages.push_back( vector<int>() ); // (tangelo with a missing ttfcomp)
    stages.back().push_back( g );
  }
  return 0;
}

// the threads as resolved: "3+3,1+1"
static string ThreadText() {
  string t;
  for( size_t s=0; s<stages.size(); s++ ) for( size_t i=0; i<stages[s].size(); i++ )
    t += (t.empty() ? "" : i ? "+" : ",") + Num( th[stages[s][i]] );
  return t;
}

// "4,512k": part sizes in the same order; MB, or KB with k (GB with g); 0 or
// a missing one: the default
static int ParseChunks( const char* s ) {
  for( int g=0; g<N_GROUP; g++ ) ch[g] = 0;
  for( int g=0; ; g++ ) {
    if( (g>=N_GROUP) || (*s<'0') || (*s>'9') ) return 1;
    qword v = 0;
    while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>(1u<<30) ) return 1; }
    qword u = 1<<20;
    if( (*s=='k') || (*s=='K') ) { u = 1<<10; s++; }
    else if( (*s=='m') || (*s=='M') ) s++;
    else if( (*s=='g') || (*s=='G') ) { u = 1<<30; s++; }
    ch[g] = v*u;
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    s++;
  }
  return 0;
}

// "5,9,,7": levels of the video packer, the audio packer, ttfcomp and tangelo,
// in that order ('+' as ','); missing or empty: the tool's default. h264p,
// m4vpack, aacpack, ac3pack, oggcomp and tangelo take 1 to 9 (-N; which of
// them their builds have, they say: CheckBuilds); ttfcomp and mp3pack take
// none, theirs is not used.
static int ParseLevels( const char* s ) {
  for( int g=0; g<N_GROUP; g++ ) lv[g] = NO_LEVEL;
  for( int g=0; ; g++ ) {
    if( g>=N_GROUP ) return 1;
    if( *s && (*s!=',') && (*s!='+') ) {
      int neg = (*s=='-');
      s += neg;
      if( (*s<'0') || (*s>'9') ) return 1;
      int v = 0;
      while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>1000000 ) return 1; }
      lv[g] = neg ? -v : v;
    }
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    s++;
  }
  for( int g=G_VIDEO; g<N_GROUP; g++ ) if( (lv[g]!=NO_LEVEL) && ((lv[g]<1) || (lv[g]>9)) && (g!=G_FONT) ) return 1;
  return 0;
}

// a packer's switch for its group's level: -N (none for its default) for those
// that take one; none for the others
static string LevelArg( int m ) {
  int l = lv[m_group[m]];
  if( (l==NO_LEVEL) || (m==M_STORE) || (m==M_MP3PACK) || (m==M_TTFCOMP) ) return string();
  return "-" + Num(l);
}

// the same with a space before it, if there is one
static string LevelSp( int m ) { string a = LevelArg(m); return a.size() ? " "+a : a; }

// tangelo's command line for an archive in p, from a directory beside p (the
// files to go in it follow)
static string TangeloA( const string& pk ) { return "a" + LevelSp(M_TANGELO) + (f_verbose ? "" : " -p") + " " + Quote(".." DS "p" DS+pk); }

// a part size for a packer's -c: MB, or KB with a k
static string ChunkArg( qword c ) {
  c = std::min<qword>( std::max<qword>( c, 1<<10 ), 1u<<30 );
  return (c&((1<<20)-1)) ? Num(c>>10)+"k" : Num(c>>20);
}

// the part size for a stream of group g: -c's, else a part for each of the
// group's threads (the size over them, 1MB at least). perbyte: the memory a
// tool takes for each byte of a part (h264p about 2, m4vpack and ac3pack less,
// mp3pack 10; tangelo takes the same for any): then no more than its
// threads' share of 3/4 of the memory free now (with the threads that run
// at once), and 1024MB, the tools' most. (aacpack and oggcomp: AudioThreads)
static qword PartSize( int g, qword size, int perbyte ) {
  if( ch[g] ) return ch[g];
  qword n = qword( std::max( th[g], 1 ) ), p = std::max<qword>( (size+n-1)/n, 1<<20 );
  if( perbyte ) {
    int t = 0;
    for( size_t s=0; s<stages.size(); s++ ) if( std::find( stages[s].begin(), stages[s].end(), g )!=stages[s].end() )
      for( size_t i=0; i<stages[s].size(); i++ ) t += th[stages[s][i]];
    MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms);
    qword avail = GlobalMemoryStatusEx( &ms ) ? ms.ullAvailPhys : (qword(1)<<32);
    qword share = avail/4*3/qword( std::max( t, 1 ) ), base = 128<<20; // (the rest of what a tool takes)
    p = std::min( p, std::max<qword>( (share>base) ? (share-base)/perbyte : 0, 1<<20 ) );
    p = std::min<qword>( p, 1u<<30 );
  }
  return p;
}

// aacpack's and oggcomp's threads for a stream: as many as it has parts, up
// to the audio's (-t). They cut it into parts of the size part (-c), else of
// their own (aacpack 1MB, oggcomp four for each thread, 64KB to 4MB): of
// that much at least. Each thread takes about 35MB (aacpack) or 55MB
// (oggcomp) with those. d: they take the threads from the stream, and this
// (with no part) is a guess of them, for the stage
static int AudioThreads( int m, qword size, qword part ) {
  if( !part ) part = (m==M_OGGCOMP) ? (64<<10) : (1<<20);
  return int( std::max<qword>( 1, std::min<qword>( qword( std::max( th[G_AUDIO], 1 ) ), size/part ) ) );
}

static string StageText() {
  string t;
  for( size_t s=0; s<stages.size(); s++ ) {
    string u;
    for( size_t i=0; i<stages[s].size(); i++ ) {
      int g = stages[s][i], n = 0;
      for( size_t k=0; k<J.size(); k++ ) n += (J[k]->g==g);
      if( n ) u += string( u.size() ? " + " : "" ) + g_name[g] + " " + Num(th[g]);
    }
    if( u.size() ) t += (t.size() ? ", then " : "") + u;
  }
  return t;
}

static int RunStages() {
  t_run = GetTickCount64();
  phase = f_dec ? "unpacking" : "packing";
  for( size_t s=0; s<stages.size(); s++ ) {
    int r = RunStage( J, stages[s] );
    if( r ) return (r==2) ? 5 : 3;
  }
  return 0;
}

