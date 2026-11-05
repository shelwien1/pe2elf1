// Tests of mp4pack's -t, -c and -l parsing, of the index's, and of the threads
// aacpack and oggcomp get. Build like mp4pack.exe (with -municode) and run:
// parse_test.exe prints the failures, exits with 1 if any.
// On Linux: g++ -std=c++11 -pthread -o parse_test test/parse_test.cpp
#define WINAPI_POSIX_NO_MAIN // (Linux: the main below)
#pragma GCC diagnostic ignored "-Wunused-function" // (the engine's run: not used here)
#include "../inc/mp4pack.inc"

static int fails;

static void T( const char* spec, int all, int dec, const char* want ) {
  f_dec = dec;
  int r = ParseThreads( spec, all );
  string got = r ? "error" : ThreadText();
  if( got!=want ) { printf( "FAIL -t %s (%d threads%s): %s, not %s\n", spec, all, dec ? ", d" : "", got.c_str(), want ); fails++; }
}

static void C( const char* spec, const char* want ) {
  string got;
  if( ParseChunks( spec ) ) got = "error";
  else for( int g=0; g<N_GROUP; g++ ) got += (g ? "," : "") + Num( ch[g] );
  if( got!=want ) { printf( "FAIL -c %s: %s, not %s\n", spec, got.c_str(), want ); fails++; }
}

// the switches the packers get for these levels: h264p aacpack oggcomp mp3pack ttfcomp tangelo
static void L( const char* spec, const char* want ) {
  for( int g=0; g<N_GROUP; g++ ) lv[g] = NO_LEVEL;
  string got;
  if( ParseLevels( spec ) ) got = "error";
  else {
    const int ms[] = { M_H264P, M_AACPACK, M_OGGCOMP, M_MP3PACK, M_TTFCOMP, M_TANGELO };
    for( int i=0; i<6; i++ ) got += (i ? "," : "") + LevelArg( ms[i] );
  }
  if( got!=want ) { printf( "FAIL -l %s: %s, not %s\n", spec, got.c_str(), want ); fails++; }
}

// an index: "version streams", or error
static void I( const char* what, const string& text, const char* want ) {
  Index x;
  string why, got = ParseIndex( text, x, why ) ? "error" : Num(x.ver) + " " + Num(x.s.size());
  if( got!=want ) { printf( "FAIL index %s: %s, not %s\n", what, got.c_str(), want ); fails++; }
}

// aacpack's and oggcomp's threads for a stream, with -c part and -t threads
static void A( int m, qword size, qword part, int threads, int want ) {
  th[G_AUDIO] = threads;
  int got = AudioThreads( m, size, part );
  if( got!=want ) { printf( "FAIL %s %u bytes, part %u, %d threads: %d, not %d\n", m_name[m], uint(size), uint(part), threads, got, want ); fails++; }
}

int wmain( int, wchar_t** ) {
  T( "3+3+1+1", 8, 0, "3+3+1+1" );
  T( "3,3,1,1", 8, 0, "3,3,1,1" );
  T( "3+3,1,1", 8, 0, "3+3,1,1" );
  T( "3,3+1,1", 8, 0, "3,3+1,1" );
  T( "3+3,1+1", 8, 0, "3+3,1+1" );
  T( "4", 8, 0, "4,4,1+1" );           // missing: audio as video, after it; ttfcomp and tangelo 1, at once
  T( "3+1", 8, 0, "3+1,1+1" );
  T( "3+3", 8, 0, "3+3,1+1" );
  T( "3+3,2", 8, 0, "3+3,2,1" );       // (ttfcomp given: tangelo after it)
  T( "3+3+2", 8, 0, "3+3+2,1" );
  T( "0", 8, 0, "0,0,1+1" );
  T( "0,0,0,0", 8, 0, "0,0,0,0" );
  T( "64", 8, 0, "64,64,1+1" );
  T( "", 8, 0, "8,8,1+1" );            // the default
  T( "", 8, 1, "8,8,1+1" );
  T( "", 100, 0, "64,64,1+1" );        // (64 at most)
  T( "50%+50%-2", 8, 0, "3+3,1+1" );   // 8-2 = 6, half of it each
  T( "50%+50%,1+1-2", 8, 0, "3+3,1+1" );
  T( "50%+50%", 7, 0, "4+4,1+1" );     // rounded up
  T( "33%", 8, 0, "3,3,1+1" );
  T( "1%", 8, 0, "1,1,1+1" );
  T( "0%", 8, 0, "0,0,1+1" );
  T( "100%", 8, 0, "8,8,1+1" );
  T( "200%+1", 8, 0, "16+1,1+1" );
  T( "100%", 200, 0, "64,64,1+1" );
  T( "-2", 8, 0, "6,6,1+1" );          // -N alone: the default, N threads left
  T( "-2", 8, 1, "6,6,1+1" );
  T( "-10", 8, 0, "1,1,1+1" );         // (1 at least)
  T( "3+3-2", 8, 0, "3+3,1+1" );       // (numbers are numbers)
  T( "100%-0", 8, 0, "8,8,1+1" );
  T( "100%+100%+100%+100%-4", 8, 0, "4+4+4+4" );
  T( "3+3,1,1,1", 8, 0, "error" );     // 5 groups
  T( "65", 8, 0, "error" );
  T( "3+", 8, 0, "error" );
  T( "+3", 8, 0, "error" );
  T( "3,,3", 8, 0, "error" );
  T( "3x", 8, 0, "error" );
  T( "-", 8, 0, "error" );
  T( "3-", 8, 0, "error" );
  T( "3--2", 8, 0, "error" );
  T( "-2-2", 8, 0, "error" );
  T( "50%%", 8, 0, "error" );
  T( "%", 8, 0, "error" );
  T( "-2+3", 8, 0, "error" );
  T( "3-2a", 8, 0, "error" );
  C( "4", "4194304,0,0,0" );
  C( "4,512k", "4194304,524288,0,0" );
  C( "0+1g,0,64", "0,1073741824,0,67108864" );
  C( "4m,2M,1K,3", "4194304,2097152,1024,3145728" );
  C( "1,2,3,4,5", "error" );
  C( "4x", "error" );
  C( ",4", "error" );
  L( "", ",,,,," );                    // all the defaults
  L( "5,9,9,7", "-5,-9,-9,,,-7" );     // ttfcomp and mp3pack take none
  L( "5", "-5,,,,," );                 // missing: the default
  L( ",,,1", ",,,,,-1" );
  L( "1+3", "-1,-3,-3,,," );           // '+' as ','
  L( "7,,,9", "-7,,,,,-9" );
  L( "9,9,9,9,", "error" );            // 5 levels
  L( "0", "error" );                   // h264p, aacpack and tangelo: 1 to 9
  L( "10", "error" );
  L( ",0", "error" );
  L( "-5", "error" );
  L( ",,,0", "error" );                // (storing is -t ...+0)
  L( ",,,10", "error" );
  L( ",,,-5", "error" );
  L( "5x", "error" );
  L( "5,-", "error" );
  L( ",,,--5", "error" );
  const string src = "src 100 0000ABCD x.mkv\ntools aacpack.exe\n", aac = "stream 100 0000ABCD aacpack x.1.aac\n";
  I( "2", "mp4pack 2\n" + src + aac + "part 0 100 x.1.aac.aacp\nend\n", "2 1" );
  I( "1", "mp4pack 1\n" + src + aac + "part 0 100 x.1.aac.aacp\nend\n", "1 1" );
  I( "1, aacpack in parts", "mp4pack 1\n" + src + aac + "part 0 60 x.1.0.aacp\npart 60 40 x.1.1.aacp\nend\n", "1 1" );
  I( "2, aacpack in parts", "mp4pack 2\n" + src + aac + "part 0 60 x.1.0.aacp\npart 60 40 x.1.1.aacp\nend\n", "error" ); // (one process for it now)
  I( "2, tangelo's pieces", "mp4pack 2\n" + src + "stream 100 0000ABCD tangelo x.2.bin\npart 0 60 other1.tgl x.2.bin.part0\npart 60 40 other2.tgl x.2.bin.part1\nend\n", "2 1" );
  I( "2, a gap", "mp4pack 2\n" + src + aac + "part 0 60 x.1.aac.aacp\nend\n", "error" );
  I( "3", "mp4pack 3\n" + src + aac + "part 0 100 x.1.aac.aacp\nend\n", "error" ); // (a later version)
  I( "no end", "mp4pack 2\n" + src + aac + "part 0 100 x.1.aac.aacp\n", "error" );
  A( M_AACPACK, 3600000, 0, 8, 3 );           // parts of 1MB at least: 3
  A( M_AACPACK, 19500000, 0, 8, 8 );
  A( M_AACPACK, 19500000, 0, 64, 18 );
  A( M_AACPACK, 900000, 0, 8, 1 );            // (1 at least)
  A( M_AACPACK, 19500000, 4<<20, 8, 4 );      // -c 4
  A( M_AACPACK, 19500000, 512<<10, 64, 37 );  // -c 512k
  A( M_OGGCOMP, 2200000, 0, 8, 8 );           // parts of 64KB at least
  A( M_OGGCOMP, 300000, 0, 8, 4 );
  A( M_OGGCOMP, 18543, 0, 8, 1 );
  A( M_OGGCOMP, 9500000, 4<<20, 8, 2 );       // -c 4
  A( M_OGGCOMP, 2200000, 0, 0, 1 );           // (-t ,0: none; d: 1 at least)
  printf( fails ? "%d failed\n" : "all passed\n", fails );
  return fails!=0;
}

#ifndef _WIN32
int main() { return wmain( 0, 0 ); }
#endif
