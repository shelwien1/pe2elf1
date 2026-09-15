// timetest - run a command, print (and append to log.txt) its wall-clock time.
// Replacement for the author's timetest tool referenced by the original t.bat;
// output format matches the lines in log.txt:  "0.087s: fpaq0mw c ..\book1 1"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>

int main( int argc, char** argv ) {
  if( argc<2 ) { printf("timetest <command> [args...]\n"); return 1; }

  std::string cmd;
  for( int i=1; i<argc; i++ ) { if( i>1 ) cmd += ' '; cmd += argv[i]; }

  auto t0 = std::chrono::steady_clock::now();
  int r = system( cmd.c_str() );
  double s = std::chrono::duration<double>( std::chrono::steady_clock::now()-t0 ).count();

  printf( "%.3fs: %s\n", s, cmd.c_str() );
  if( FILE* f = fopen("log.txt","a") ) { fprintf( f, "%.3fs: %s\n", s, cmd.c_str() ); fclose(f); }

  return r;
}
