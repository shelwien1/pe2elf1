// compare GccName()/gnat with gcc's own host_detect_local_cpu output (gcc_native.tsv)
#include "x86targets.h"
#include <string>
#include <map>
#include <set>
#include <sstream>
using namespace x86;
int main( int argc, char** argv ) {
  // argv[1]: gcc_native.tsv, argv[2]: dumps dir prefix list file (paths)
  std::map<std::string,std::string> nat;
  { FILE* f = fopen( argv[1], "r" ); char l[16384];
    while( fgets( l, sizeof(l), f ) ) { char* t = strchr( l, '\t' ); if( !t ) continue; *t = 0; std::string v = t+1; while( v.size() && (v.back()=='\n') ) v.pop_back(); nat[l] = v; } fclose( f ); }
  std::map<std::string,int> flag2f;
  for( int f=0; f<N_FEAT; f++ ) if( x86_feat[f].gcc ) flag2f[x86_feat[f].gcc+2] = f;
  int n = 0, nbad = 0, fbad = 0;
  std::map<std::string,int> fcount;
  for( int i=2; i<argc; i++ ) {
    FILE* f = fopen( argv[i], "rb" ); if( !f ) continue;
    std::string s; char b[4096]; size_t k; while( (k=fread(b,1,sizeof(b),f))>0 ) s.append(b,k); fclose(f);
    static Cpuid d; if( !ParseCpuid( d, s.c_str() ) ) continue;
    static Cpu c; Analyze( d, c );
    const char* b0 = strrchr( argv[i], '/' ); b0 = b0 ? b0+1 : argv[i];
    std::string g = nat[b0]; n++;
    std::istringstream is( g ); std::string tok, name; std::set<int> on, off;
    bool err = false;
    while( is>>tok ) {
      if( tok=="ERROR" ) err = true;
      else if( tok.compare( 0, 7, "-march=" )==0 ) name = tok.substr( 7 );
      else if( tok.compare( 0, 5, "-mno-" )==0 ) { auto it = flag2f.find( tok.substr( 5 ) ); if( it!=flag2f.end() ) off.insert( it->second ); }
      else if( tok.compare( 0, 2, "-m" )==0 ) { auto it = flag2f.find( tok.substr( 2 ) ); if( it!=flag2f.end() ) on.insert( it->second ); }
    }
    std::string mine = c.gcc ? c.gcc : "";
    if( err ) name = "";
    if( name!=mine ) { nbad++; printf( "NAME %s: gcc %s, mine %s\n", b0, name.c_str(), mine.c_str() ); }
    std::string fd;
    for( int fi : on ) if( !Has( c.gnat, fi ) ) { fd += std::string( " +" ) + x86_feat[fi].name; fcount[std::string("+")+x86_feat[fi].name]++; }
    for( int fi : off ) if( Has( c.gnat, fi ) ) { fd += std::string( " -" ) + x86_feat[fi].name; fcount[std::string("-")+x86_feat[fi].name]++; }
    if( fd.size() ) { fbad++; if( fbad<=12 ) printf( "FEAT %s: gcc vs mine:%s\n", b0, fd.c_str() ); }
  }
  printf( "dumps %d: names differ %d, feature sets differ %d\n", n, nbad, fbad );
  for( auto& p : fcount ) printf( "  %s x%d\n", p.first.c_str(), p.second );
}
