// timetest - run a command, print (and append to log.txt) its wall-clock time.
// Replacement for the author's timetest tool referenced by the original t.bat;
// output format matches the lines in log.txt:  "0.087s: fpaq0mw c ..\book1 1"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>
#ifndef _WIN32
#include <sys/wait.h>
#endif

int main( int argc, char** argv ) {
  if( argc<2 ) { printf("timetest <command> [args...]\n"); return 1; }

  std::string cmd;
  for( int i=1; i<argc; i++ ) { if( i>1 ) cmd += ' '; cmd += argv[i]; }

  auto t0 = std::chrono::steady_clock::now();
  int r = system( cmd.c_str() );
  double s = std::chrono::duration<double>( std::chrono::steady_clock::now()-t0 ).count();

  printf( "%.3fs: %s\n", s, cmd.c_str() );

  // Appends to log.txt, as the author's tool did. TIMETEST_LOG redirects that,
  // which is how test.sh/test.bat keep a run out of the experiment log until it
  // has verified: the timing lines would otherwise already be in log.txt, with
  // no size line after them, and log.pl would attribute them to the next run.
  const char* logname = getenv("TIMETEST_LOG");
  if( (logname==0) || (logname[0]==0) ) logname = "log.txt";
  if( FILE* f = fopen(logname,"a") ) { fprintf( f, "%.3fs: %s\n", s, cmd.c_str() ); fclose(f); }

#ifdef _WIN32
  return r;
#else
  // On POSIX system() returns an encoded wait status, not an exit code: a child
  // that exited with 6 comes back as 1536, and returning that from main() leaves
  // the shell 1536 & 255 == 0. Undecoded, every failure would read as success -
  // which would quietly disarm test.sh's "set -e" and every exit() in main.inc.
  if( r==-1 ) return 1;
  return WIFEXITED(r) ? WEXITSTATUS(r) : 128+WTERMSIG(r);
#endif
}
