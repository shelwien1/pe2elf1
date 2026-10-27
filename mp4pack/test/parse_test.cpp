// Tests of mp4pack's -t and -c parsing. Build like mp4pack.exe (with
// -municode) and run: parse_test.exe prints the failures, exits with 1 if any.
#define wmain mp4pack_wmain
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

int wmain( int, wchar_t** ) {
  T( "3+3+1+1", 8, 0, "3+3+1+1" );
  T( "3,3,1,1", 8, 0, "3,3,1,1" );
  T( "3+3,1,1", 8, 0, "3+3,1,1" );
  T( "3,3+1,1", 8, 0, "3,3+1,1" );
  T( "3+3,1+1", 8, 0, "3+3,1+1" );
  T( "4", 8, 0, "4,4,1,1" );           // missing: audio as video, ttfcomp and shar 1, in turn
  T( "3+1", 8, 0, "3+1,1,1" );
  T( "3+3", 8, 0, "3+3,1,1" );
  T( "3+3,2", 8, 0, "3+3,2,1" );
  T( "3+3+2", 8, 0, "3+3+2,1" );
  T( "0", 8, 0, "0,0,1,1" );
  T( "0,0,0,0", 8, 0, "0,0,0,0" );
  T( "64", 8, 0, "64,64,1,1" );
  T( "", 8, 0, "8+8,1,1" );            // the default
  T( "", 8, 1, "8+8,1,1" );
  T( "", 100, 0, "64+64,1,1" );        // (64 at most)
  T( "50%+50%-2", 8, 0, "3+3,1,1" );   // 8-2 = 6, half of it each
  T( "50%+50%,1+1-2", 8, 0, "3+3,1+1" );
  T( "50%+50%", 7, 0, "4+4,1,1" );     // rounded up
  T( "33%", 8, 0, "3,3,1,1" );
  T( "1%", 8, 0, "1,1,1,1" );
  T( "0%", 8, 0, "0,0,1,1" );
  T( "100%", 8, 0, "8,8,1,1" );
  T( "200%+1", 8, 0, "16+1,1,1" );
  T( "100%", 200, 0, "64,64,1,1" );
  T( "-2", 8, 0, "6+6,1,1" );          // -N alone: the default, N threads left
  T( "-2", 8, 1, "6+6,1,1" );
  T( "-10", 8, 0, "1+1,1,1" );         // (1 at least)
  T( "3+3-2", 8, 0, "3+3,1,1" );       // (numbers are numbers)
  T( "100%-0", 8, 0, "8,8,1,1" );
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
  printf( fails ? "%d failed\n" : "all passed\n", fails );
  return fails!=0;
}
