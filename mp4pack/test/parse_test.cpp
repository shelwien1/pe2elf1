// Tests of mp4pack's -t, -c and -l parsing. Build like mp4pack.exe (with
// -municode) and run: parse_test.exe prints the failures, exits with 1 if any.
// On Linux: g++ -std=c++11 -pthread -o parse_test test/parse_test.cpp
#define wmain mp4pack_wmain
#define WINAPI_POSIX_NO_MAIN // (Linux: the main below)
#include "../mp4pack.cpp"
#undef wmain

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

// the switches the packers get for these levels: h264p aacpack mp3pack ttfcomp shar
static void L( const char* spec, const char* want ) {
  for( int g=0; g<N_GROUP; g++ ) lv[g] = NO_LEVEL;
  string got;
  if( ParseLevels( spec ) ) got = "error";
  else {
    const int ms[] = { M_H264P, M_AACPACK, M_MP3PACK, M_TTFCOMP, M_SHAR };
    for( int i=0; i<5; i++ ) got += (i ? "," : "") + LevelArg( ms[i] );
  }
  if( got!=want ) { printf( "FAIL -l %s: %s, not %s\n", spec, got.c_str(), want ); fails++; }
}

int wmain( int, wchar_t** ) {
  T( "3+3+1+1", 8, 0, "3+3+1+1" );
  T( "3,3,1,1", 8, 0, "3,3,1,1" );
  T( "3+3,1,1", 8, 0, "3+3,1,1" );
  T( "3,3+1,1", 8, 0, "3,3+1,1" );
  T( "3+3,1+1", 8, 0, "3+3,1+1" );
  T( "4", 8, 0, "4,4,1+1" );           // missing: audio as video, after it; ttfcomp and shar 1, at once
  T( "3+1", 8, 0, "3+1,1+1" );
  T( "3+3", 8, 0, "3+3,1+1" );
  T( "3+3,2", 8, 0, "3+3,2,1" );       // (ttfcomp given: shar after it)
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
  L( "", ",,,,a22" );                  // all the defaults
  L( "5,9,9,20", "-5,-9,,,a20" );      // ttfcomp and mp3pack take none
  L( "5", "-5,,,,a22" );               // missing: the default
  L( ",,,19", ",,,,a19" );
  L( "1+3", "-1,-3,,,a22" );           // '+' as ','
  L( "7,,,-5", "-7,,,,a-5" );          // zstd's fast levels
  L( ",,,0", ",,,,a0" );               // shar stores
  L( ",,,-131072", ",,,,a-131072" );
  L( "9,9,9,22,", "error" );           // 5 levels
  L( "0", "error" );                   // h264p and aacpack: 1 to 9
  L( "10", "error" );
  L( ",0", "error" );
  L( "-5", "error" );
  L( ",,,23", "error" );               // shar: up to 22
  L( ",,,-131073", "error" );
  L( "5x", "error" );
  L( "5,-", "error" );
  L( ",,,--5", "error" );
  printf( fails ? "%d failed\n" : "all passed\n", fails );
  return fails!=0;
}

#ifndef _WIN32
int main() { return wmain( 0, 0 ); }
#endif
