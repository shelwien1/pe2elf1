// dr3_tune.cpp - codes the streams that "dicrank3 e -D DIR" dumps, with
// dicrank3's model (dr3_model.inc), without recomputing the context vectors:
// a run takes seconds instead of a minute, which is what tuning the IDX knobs
// with IDX/opt.pl needs.
//
//   dr3_tune c DIR/STREAMS OUT    code STREAMS into OUT (the form opt.pl runs: "exe c FILE OUT")
//   dr3_tune d DIR/STREAMS IN     decode IN and check it against the dump
//
// DIR is the dump folder; '/' and '\' both separate it, and without one the
// dump is read from the current folder. STREAMS: any of s (explicit-word set),
// b (membership bitmap), h (explicit-word order) and r (runs); they are coded
// in dicrank3's order s b h r, so "DIR/sbhr" reproduces dicrank3's range coder
// stream byte for byte. An opt.pl corpus list names stream sets as "files":
// dump/sb, dump/h, dump/r. DR3_NOCX=<stream>:<hex mask> replaces the contexts
// whose bits are set by a constant, for ablations (streams: 0 runs,
// 1 explicit-word order, 2 membership bitmap, 3 explicit-word set).
//
// Dump files (little-endian):
//   words.txt    one line per frequency rank: word count df lowercase-count
//   bitmap.bin   u32 n, u8 bits[n] (explicit-word set over ranks 0..n-1);
//                u32 m, u32 rank[m], u8 bits[m] (membership bitmap)
//   head.bin     u32 S, u32 rank[S], u32 seq[S] (dictionary order as indices
//                into rank), i32 gram[S*S]
//   runs.bin     u32 T, u32 R, u32 rank[T], u32 run[T], i64 score[T*R]
//                (each tail word's score for every run, 0 if not started)
//
// build: see build.sh (./build.sh tune makes dr3_tune.tune, the build opt.pl patches)

#include <string>

#include "dr3_model.inc"

static void Die( const char* m, const char* a = "" ) { fprintf( stderr, "dr3_tune: %s%s\n", m, a ); exit( 1 ); }

static std::string ReadAll( const std::string& fn ) {
  FILE* f = fopen( fn.c_str(), "rb" ); if( !f ) Die( "cannot open ", fn.c_str() );
  std::string s; char buf[1 << 16]; size_t n;
  while( (n = fread( buf, 1, sizeof buf, f )) > 0 ) s.append( buf, n );
  fclose( f );
  return s;
}

struct Reader {
  std::string d; size_t pos = 0;
  template<class T> T Get() { T x; Need( sizeof(T) ); memcpy( &x, d.data() + pos, sizeof(T) ); pos += sizeof(T); return x; }
  template<class T> std::vector<T> Vec( size_t n ) { std::vector<T> v(n); Need( n * sizeof(T) ); if( n ) memcpy( v.data(), d.data() + pos, n * sizeof(T) ); pos += n * sizeof(T); return v; }
  void Need( size_t n ) { if( d.size() - pos < n ) Die( "truncated dump" ); }
};

// the dumped scores as the run coder's score provider
struct DumpScores {
  const int64_t* s; uint R;
  const int64_t* Score( uint j ) { return s + size_t(j) * R; }
  void Add( uint, uint ) {}
};

int main( int argc, char** argv ) {
  if( argc != 4 || (strcmp( argv[1], "c" ) && strcmp( argv[1], "d" )) ) {
    fprintf( stderr, "usage: dr3_tune c|d DIR/STREAMS FILE   (STREAMS from sbhr, DIR = the dump folder)\n" );
    return 2;
  }
  int dec = argv[1][0] == 'd';
  // DIR/STREAMS: the dump folder, then the streams to code
  std::string arg = argv[2], dir = ".", which = arg;
  size_t cut = arg.find_last_of( "/\\" );
  if( cut != std::string::npos ) { dir = cut ? arg.substr( 0, cut ) : arg.substr( 0, 1 ); which = arg.substr( cut + 1 ); }
  if( which.empty() || which.find_first_not_of( "sbhr" ) != std::string::npos ) Die( "streams must be letters from sbhr: ", argv[2] );
  auto has = [&]( char c ) { return which.find( c ) != std::string::npos; };

  // word features by rank
  std::vector<std::string> spell;
  std::vector<WordF> feat;
  {
    std::string s = ReadAll( dir + "/words.txt" );
    std::vector<uint> cnt;
    size_t i = 0;
    while( i < s.size() ) {
      size_t e = s.find( '\n', i ); if( e == std::string::npos ) Die( "bad words.txt" );
      char w[256]; uint c, d, l;
      if( sscanf( s.c_str() + i, "%255s %u %u %u", w, &c, &d, &l ) != 4 ) Die( "bad words.txt" );
      spell.push_back( w ); cnt.push_back( c ); cnt.push_back( d ); cnt.push_back( l );
      i = e + 1;
    }
    for( size_t r = 0; r < spell.size(); r++ )
      feat.push_back( MakeWordF( spell[r].c_str(), int(spell[r].size()), cnt[3*r], cnt[3*r+1], cnt[3*r+2], uint(r) ) );
  }
  auto ptrs = [&]( const std::vector<uint32_t>& ranks ) {
    std::vector<const WordF*> p;
    for( uint32_t r : ranks ) { if( r >= feat.size() ) Die( "rank beyond words.txt" ); p.push_back( &feat[r] ); }
    return p;
  };

  Reader br; br.d = ReadAll( dir + "/bitmap.bin" );
  uint nh = br.Get<uint32_t>();
  std::vector<uint8_t> hbits = br.Vec<uint8_t>( nh );
  uint nb = br.Get<uint32_t>();
  std::vector<uint32_t> border = br.Vec<uint32_t>( nb );
  std::vector<uint8_t> bbits = br.Vec<uint8_t>( nb );
  std::vector<uint32_t> hranks( nh ); for( uint i = 0; i < nh; i++ ) hranks[i] = i;
  size_t S_ones = 0; for( uint8_t b : hbits ) S_ones += b;   // the explicit-word set ends with its S-th one

  uint S = 0; std::vector<uint32_t> head, seq; std::vector<int32_t> gram;
  if( has( 'h' ) ) {
    Reader hr; hr.d = ReadAll( dir + "/head.bin" );
    S = hr.Get<uint32_t>(); head = hr.Vec<uint32_t>( S ); seq = hr.Vec<uint32_t>( S );
    gram = hr.Vec<int32_t>( size_t(S) * S );
  }
  uint T = 0, R = 0; std::vector<uint32_t> tail, run; std::vector<int64_t> score;
  if( has( 'r' ) ) {
    Reader rr; rr.d = ReadAll( dir + "/runs.bin" );
    T = rr.Get<uint32_t>(); R = rr.Get<uint32_t>();
    tail = rr.Vec<uint32_t>( T ); run = rr.Vec<uint32_t>( T );
    score = rr.Vec<int64_t>( size_t(T) * R );
  }

  Dr3Fpu fpu;
  ModelInit();
  if( const char* nc = getenv( "DR3_NOCX" ) ) {
    int sd; unsigned long long m;
    if( sscanf( nc, "%d:%llx", &sd, &m ) == 2 && sd >= 0 && sd < 4 ) g_m->nocx[sd] = m;
  }
  FILE* f = fopen( argv[3], dec ? "rb" : "wb" ); if( !f ) Die( "cannot open ", argv[3] );
  if( dec ) g_m->rc.StartDecode( f ); else g_m->rc.StartEncode( f );
  std::vector<uint8_t> hb( hbits ), bb( bbits );
  std::vector<uint32_t> sq_( seq ), rn( run );
  if( dec ) { std::fill( hb.begin(), hb.end(), 0 ); std::fill( bb.begin(), bb.end(), 0 ); std::fill( sq_.begin(), sq_.end(), 0 ); std::fill( rn.begin(), rn.end(), 0 ); }
  if( has( 's' ) ) {
    auto p = ptrs( hranks );
    if( CodeBitmap( dec, kHeadSet, nh, S_ones, [&]( size_t i ) -> const WordF& { return *p[i]; }, hb.data() ) != nh )
      Die( "explicit-word set does not end with its last explicit word" );
  }
  if( has( 'b' ) ) { auto p = ptrs( border ); CodeBitmap( dec, kBitmap, nb, SIZE_MAX, [&]( size_t i ) -> const WordF& { return *p[i]; }, bb.data() ); }
  if( has( 'h' ) ) { auto p = ptrs( head ); CodeHead( dec, S, p.data(), gram.data(), sq_.data() ); }
  if( has( 'r' ) ) {
    auto p = ptrs( tail );
    DumpScores prov{ score.data(), R };
    CodeRuns( dec, T, R, p.data(), rn.data(), prov );
  }
  long bytes = 0;
  if( !dec ) { g_m->rc.FinishEncode(); bytes = ftell( f ); }
  fclose( f );
  static const char* name[4] = { "runs", "explicit-word order", "membership bitmap", "explicit-word set" };
  double total = 0;
  for( int s : { kHeadSet, kBitmap, kHead, kRuns } ) {
    if( !g_m->decisions[s] ) continue;
    printf( "%-20s %9.1f bytes %9llu decisions\n", name[s], g_m->bits[s] / 8, g_m->decisions[s] );
    total += g_m->bits[s] / 8;
  }
  if( !dec ) { printf( "%-20s %9.1f bytes, %ld coded\n", "total", total, bytes ); return 0; }
  int ok = (!has( 's' ) || hb == hbits) && (!has( 'b' ) || bb == bbits) && (!has( 'h' ) || sq_ == seq) && (!has( 'r' ) || rn == run);
  printf( ok ? "decode OK\n" : "decode FAILED\n" );
  return !ok;
}
