// The tool builds mp4pack would run on other CPUs, by their CPUID dumps (the
// text cpuinfo reads: InstLatx64's *CPUID*.txt, or cpuinfo -D):
//   builds_test [-i T] TOOLDIR DUMP...
// prints for each dump the CPU's best target and the build of each tool in
// TOOLDIR that would run there (as mp4pack's "tools:" line), or why none.
// The builds can be empty files: only their names count. Build like
// parse_test (on Linux: g++ -std=c++11 -pthread -o builds_test test/builds_test.cpp).
#include "../../cpuinfo/x86targets.h"
namespace x86 {
static const char* dump;
inline bool ReadDump( Cpuid& d ) { return ParseCpuid( d, dump ); }
}
#define ReadCpuid ReadDump // (CpuTargets reads the dump, not this CPU)
#define wmain mp4pack_wmain
#define WINAPI_POSIX_NO_MAIN // (Linux: the main below)
#include "../mp4pack.cpp"
#undef wmain

int wmain( int argc, wchar_t** argv ) {
  int a = 1, ic = -1, bits;
  if( (argc>2) && (wstring( argv[1] )==L"-i") ) {
    if( (ic = BuildOf( U(argv[2]), bits ))<0 ) { printf( "bad -i %s\n", U(argv[2]).c_str() ); return 1; }
    a = 3;
  }
  if( argc<a+2 ) { printf( "usage: builds_test [-i T] TOOLDIR DUMP...\n" ); return 1; }
  exe_dir = FullPath( argv[a] );
  if( !IsSep( exe_dir[exe_dir.size()-1] ) ) exe_dir += L"" DS;
  const string all[] = { "avip", "shar", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp" };
  for( int i=a+1; i<argc; i++ ) {
    string text;
    if( ReadAll( FullPath( argv[i] ), text, 1<<24 ) ) { printf( "cannot read %s\n", U(argv[i]).c_str() ); return 2; }
    x86::dump = text.c_str();
    for( int j=0; j<n_tools; j++ ) tools[j] = Tool();
    n_tools = 0; rk_n[0] = rk_n[1] = 0; cpu_best.clear(); cap = ic;
    CpuTargets();
    string s = U( FileName( argv[i] ) ) + ": " + (!cpu_ok ? string( "not a CPUID dump" ) : cpu_best + (cpu.level ? " (x86-64-v" + Num(cpu.level) + ")" : string())) + "\n ";
    for( int j=0; j<7; j++ ) {
      const Tool& t = GetTool( all[j] );
      if( t.path.size() ) s += " " + t.file;
      else if( t.later.size() ) s += " [" + NoTool( t ) + "]";
    }
    printf( "%s\n", s.c_str() );
  }
  return 0;
}

#ifndef _WIN32
int main( int argc, char** argv ) {
  vector<wstring> a;
  for( int i=0; i<argc; i++ ) a.push_back( WpWide( string( argv[i] ) ) );
  vector<wchar_t*> w;
  for( int i=0; i<argc; i++ ) w.push_back( &a[i][0] );
  return wmain( argc, &w[0] );
}
#endif
