// compare ClangName()/cnat with LLVM's own getHostCPUName/getHostCPUFeatures (llvm_native.tsv)
#include "x86targets.h"
#include <string>
#include <map>
#include <set>
#include <sstream>
using namespace x86;
int main( int argc, char** argv ) {
  std::map<std::string,std::pair<std::string,std::string>> nat;
  { FILE* f = fopen( argv[1], "r" ); static char l[65536];
    while( fgets( l, sizeof(l), f ) ) { char* t = strchr( l, '\t' ); if( !t ) continue; *t = 0; char* u = strchr( t+1, '\t' ); if( !u ) continue; *u = 0;
      std::string v = u+1; while( v.size() && (v.back()=='\n') ) v.pop_back(); nat[l] = std::make_pair( std::string(t+1), v ); } fclose( f ); }
  std::map<std::string,std::string> ren = { {"egpr","apxf"}, {"push2pop2","apxf"}, {"ppx","apxf"}, {"ndd","apxf"}, {"ccmp","apxf"}, {"nf","apxf"}, {"cf","apxf"}, {"zu","apxf"}, {"jmpabs","apxf"} };
  int n = 0, nbad = 0, fbad = 0; std::map<std::string,int> fcount; std::set<std::string> unknown, notllvm;
  for( int i=2; i<argc; i++ ) {
    FILE* f = fopen( argv[i], "rb" ); if( !f ) continue;
    std::string s; char b[4096]; size_t k; while( (k=fread(b,1,sizeof(b),f))>0 ) s.append(b,k); fclose(f);
    static Cpuid d; if( !ParseCpuid( d, s.c_str() ) ) continue;
    static Cpu c; Analyze( d, c );
    const char* b0 = strrchr( argv[i], '/' ); b0 = b0 ? b0+1 : argv[i];
    auto& e = nat[b0]; n++;
    if( e.first!=c.clang ) { nbad++; printf( "NAME %s: llvm %s, mine %s\n", b0, e.first.c_str(), c.clang ); }
    std::istringstream is( e.second ); std::string tok; std::map<int,bool> lf;
    while( is>>tok ) {
      bool on = tok[0]=='+'; std::string nm = tok.substr( 1 );
      if( ren.count( nm ) ) nm = ren[nm];
      int fi = FindFeature( nm.c_str() );
      if( fi<0 ) { unknown.insert( nm ); continue; }
      if( lf.count( fi ) ) lf[fi] = lf[fi] || on; else lf[fi] = on;
    }
    std::string fd;
    for( auto& p : lf ) if( p.second!=Has( c.cnat, p.first ) ) { fd += std::string( p.second ? " +" : " -" ) + x86_feat[p.first].name; fcount[std::string(p.second?"+":"-")+x86_feat[p.first].name]++; }
    for( int fi=0; fi<N_FEAT; fi++ ) if( !lf.count( fi ) ) notllvm.insert( x86_feat[fi].name ); // (not in LLVM's list: not compared)
    if( fd.size() ) { fbad++; if( fbad<=12 ) printf( "FEAT %s: llvm vs mine:%s\n", b0, fd.c_str() ); }
  }
  printf( "dumps %d: names differ %d, feature sets differ %d\n", n, nbad, fbad );
  for( auto& p : fcount ) printf( "  %s x%d\n", p.first.c_str(), p.second );
  std::string u; for( auto& x : unknown ) u += " " + x; printf( "llvm features not modelled here:%s\n", u.c_str() );
  u = ""; for( auto& x : notllvm ) u += " " + x; printf( "features llvm does not report (not compared):%s\n", u.c_str() );
}
