// cpuinfo.cpp -- the CPU, and the gcc/clang -march targets for it
//
//   cpuinfo [options]
//
// Prints the CPU (name, family/model, caches, the instruction sets and which
// of them the OS enables), the target gcc's and clang's -march=native pick
// for it, and the -march targets whose code runs on it, best first. Options
// in Usage() below; README.md has the details. x86-32 and x64, Windows and
// Linux (and other POSIX).

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <set>
#include "x86targets.h"

using namespace x86;
using std::string;

//---- the OS: threads and cores (0: not known)

static void Threads( int& threads, int& cores ) {
  threads = cores = 0;
#ifdef _WIN32
  typedef DWORD (WINAPI *gapc_t)( WORD );
  typedef BOOL (WINAPI *glpi_t)( int, void*, DWORD* );
  HMODULE k = GetModuleHandleW( L"kernel32.dll" );
  gapc_t gapc = (gapc_t)(void*)GetProcAddress( k, "GetActiveProcessorCount" ); // (Windows 7+)
  glpi_t glpi = (glpi_t)(void*)GetProcAddress( k, "GetLogicalProcessorInformationEx" );
  DWORD n = gapc ? gapc( 0xFFFF ) : 0; // ALL_PROCESSOR_GROUPS
  if( n==0 ) { SYSTEM_INFO si; GetSystemInfo( &si ); n = si.dwNumberOfProcessors; }
  threads = int( n );
  DWORD len = 0;
  if( glpi && !glpi( 0, 0, &len ) && (GetLastError()==ERROR_INSUFFICIENT_BUFFER) && len ) { // RelationProcessorCore
    std::vector<unsigned char> b( len );
    if( glpi( 0, &b[0], &len ) ) for( DWORD o=0; o+8<=len; cores++ ) { // records: relation, size, ...
      DWORD sz; memcpy( &sz, &b[o+4], 4 );
      if( sz<8 ) break;
      o += sz;
    }
  }
#else
#ifdef _SC_NPROCESSORS_ONLN
  long n = sysconf( _SC_NPROCESSORS_ONLN );
  if( n>0 ) threads = int( n );
#endif
  // the cores: the distinct sibling lists of the online cpus
  std::set<string> s;
  for( int i=0, miss=0; (i<4096) && (miss<64); i++ ) {
    char p[96]; snprintf( p, sizeof(p), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", i );
    FILE* f = fopen( p, "r" );
    if( !f ) { miss++; continue; }
    char l[256] = { 0 };
    if( fgets( l, sizeof(l), f ) ) s.insert( l );
    fclose( f );
  }
  cores = int( s.size() );
#endif
}

//---- output

static string Hex( uint x ) { char b[16]; snprintf( b, sizeof(b), "0x%X", x ); return b; }

static string Size( int kb ) {
  char b[32];
  if( (kb>=1024) && !(kb%1024) ) snprintf( b, sizeof(b), "%dM", kb/1024 );
  else if( (kb>=1024) && !(kb*100%1024) ) {  // 1.25M, 1.5M: exact, no float rounding (CRTs differ on halves)
    int h = kb*100/1024;
    if( h%10 ) snprintf( b, sizeof(b), "%d.%02dM", h/100, h%100 );
    else snprintf( b, sizeof(b), "%d.%dM", h/100, h%100/10 );
  }
  else snprintf( b, sizeof(b), "%dK", kb );
  return b;
}

// words after a label, wrapped at 79 columns
static void Words( const char* label, const std::vector<string>& w, int ind=10 ) {
  int col = printf( "%-*s", ind, label );
  for( size_t i=0; i<w.size(); i++ ) {
    if( (col>ind) && (col+1+int(w[i].size())>79) ) col = printf( "\n%*s", ind, "" )-1;
    col += printf( "%s%s", (col>ind) ? " " : "", w[i].c_str() );
  }
  printf( "\n" );
}

static std::vector<string> Names( const FSet& s, char cls=0 ) {
  std::vector<string> v;
  for( int f=0; f<N_FEAT; f++ ) if( Has( s, f ) && (!cls || (x86_feat[f].cls==cls)) ) v.push_back( x86_feat[f].name );
  return v;
}

static string Join( const std::vector<string>& v, const char* sep=" " ) {
  string s;
  for( size_t i=0; i<v.size(); i++ ) s += (i ? sep : "") + v[i];
  return s;
}

// a target's build name (-32: see BuildSuffix), with the compiler for a name
// only one of the chosen knows
static string Label( int t, int comp, int bits ) {
  const X86Target& x = x86_target[t];
  string s = string( x.name ) + BuildSuffix( t, bits );
  if( comp==C_BOTH ) {
    if( !(x.flags&T_GCC) ) s += "(clang)";
    if( !(x.flags&T_CLANG) ) s += "(gcc)";
  }
  return s;
}

// -march=native as -march=NAME and the feature flags it differs by
static void Native( const Cpu& c, const char* comp, const char* name, int bits ) {
  int g = !strcmp( comp, "gcc" );
  if( !name ) { printf( "          %-5s -march=native: none (gcc 15+ stops: Xeon Phi ISAs)\n", comp ); return; }
  int t = FindTarget( name );
  if( (t<0) || !(x86_target[t].flags&(g ? T_GCC : T_CLANG)) ) {
    if( g ) printf( "          gcc   -march=native = -march=%s and the features\n", name );
    else printf( "          clang -march=native = clang's default cpu (%s) and the features\n", (bits==64) ? "x86-64" : "pentium4 or so" );
    return;
  }
  FSet ts = g ? x86_target[t].g : x86_target[t].c, host = g ? c.gnat : c.cnat;
  if( bits==32 ) { ts = Sub( ts, Only64() ); host = Sub( host, Only64() ); }
  string s = string( "-march=" ) + name;
  FSet minus = Sub( ts, host ), plus = Sub( host, ts );
  int n = 0;
  for( int f=0; f<N_FEAT; f++ ) {
    const char* fl = g ? x86_feat[f].gcc : x86_feat[f].clang;
    if( !fl ) continue;
    if( Has( minus, f ) ) { s += string( " -mno-" ) + (fl+2); n++; }
    if( Has( plus, f ) ) { s += string( " " ) + fl; n++; }
  }
  if( n ) printf( "          %-5s -march=native = %s\n", comp, s.c_str() );
}

static const char* VendorName( int v ) {
  static const char* n[] = { "-", "intel", "amd", "hygon", "centaur", "zhaoxin" };
  return n[v];
}

static void Usage() {
  printf(
"cpuinfo -- the CPU, and the gcc/clang -march targets for it\n"
"\n"
"  cpuinfo [options]\n"
"\n"
"  (none)   the CPU, the features the OS lets it use, the names gcc's and\n"
"           clang's -march=native pick, and the targets whose code runs here,\n"
"           best first\n"
"  -l       the usable target names, best first, one per line\n"
"  -n       the best target name\n"
"  -a       all targets: the usable ones ranked, the others with what they lack\n"
"  -t NAME  one target: its features per compiler, and whether it runs here\n"
"           (NAME-32: its x86-32 build)\n"
"  -f       the features, one per line: here or not, class, compiler flags\n"
"  -c C     the targets of one compiler: gcc or clang (default: both; a name\n"
"           counts when its code runs here with every compiler that knows it)\n"
"  -32      targets for x86-32 builds (default in x86-32 cpuinfo, or when the CPU\n"
"           is not 64-bit): all names; those that are x64 targets too get \"-32\"\n"
"           (haswell-32), the x86-32-only ones stay (pentium4)\n"
"  -64      targets for x64 builds (default in x64 cpuinfo)\n"
"  -s       strict: also need the features compilers only use by intrinsics\n"
"           (aes sha sgx ...: class I), not only those they use on their own\n"
"  -d FILE  a CPUID dump (from -D, or AIDA64/InstLatx64 text) instead of this CPU\n"
"  -D       dump this CPU's CPUID leaves\n" );
}

int main( int argc, char** argv ) {
  int comp = C_BOTH, bits = 0, mode = 0, strict = 0;
  const char *dump = 0, *tname = 0;
  for( int i=1; i<argc; i++ ) {
    string a = argv[i];
    if( (a=="-l") || (a=="-n") || (a=="-a") || (a=="-f") || (a=="-D") ) mode = a[1];
    else if( a=="-t" && (i+1<argc) ) { mode = 't'; tname = argv[++i]; }
    else if( a=="-c" && (i+1<argc) ) {
      string c = argv[++i];
      if( c=="gcc" ) comp = C_GCC; else if( c=="clang" ) comp = C_CLANG; else if( c=="both" ) comp = C_BOTH;
      else { fprintf( stderr, "cpuinfo: -c takes gcc, clang or both\n" ); return 1; }
    }
    else if( a=="-32" ) bits = 32;
    else if( a=="-64" ) bits = 64;
    else if( a=="-s" ) strict = 1;
    else if( a=="-d" && (i+1<argc) ) dump = argv[++i];
    else if( (a=="-h") || (a=="-?") || (a=="--help") ) { Usage(); return 0; }
    else { fprintf( stderr, "cpuinfo: unknown option %s (-h: help)\n", a.c_str() ); return 1; }
  }

  static Cpuid d;
  if( dump ) {
    FILE* f = fopen( dump, "rb" );
    if( !f ) { fprintf( stderr, "cpuinfo: cannot read %s\n", dump ); return 2; }
    string s; char b[4096]; size_t k;
    while( (k = fread( b, 1, sizeof(b), f ))>0 ) s.append( b, k );
    fclose( f );
    if( !ParseCpuid( d, s.c_str() ) ) { fprintf( stderr, "cpuinfo: no CPUID leaves in %s\n", dump ); return 2; }
  } else if( !ReadCpuid( d ) ) {
    fprintf( stderr, "cpuinfo: no CPUID here\n" ); return 2;
  }
  if( mode=='D' ) { PrintCpuid( d, stdout ); return 0; }

  static Cpu c;
  Analyze( d, c );
  int t1 = -1, bitsopt = bits;
  if( mode=='t' ) {
    int tb;
    t1 = FindBuild( tname, &tb );
    if( t1<0 ) { fprintf( stderr, "cpuinfo: no target %s (-a lists them)\n", tname ); return 1; }
    if( strcmp( tname, x86_target[t1].name ) || !bits ) bits = tb;  // NAME-32, or no -32/-64
  }
  if( !bits ) bits = (Has( c.f, F_LM ) && (CodeBits()==64)) ? 64 : 32;
  static Match m[N_TARGET];
  int nall, nok = RankTargets( c, comp, bits, strict!=0, m, &nall );

  if( mode=='n' ) {
    if( !nok ) return 1;
    // the best group's first name every chosen compiler knows
    int g = x86_target[m[0].t].group, best = m[0].t;
    for( int t=0; t<N_TARGET; t++ ) if( x86_target[t].group==g ) {
      int need = ((comp&C_GCC) ? T_GCC : 0) | ((comp&C_CLANG) ? T_CLANG : 0);
      if( (x86_target[t].flags&need)==need ) { best = t; break; }
    }
    printf( "%s%s\n", x86_target[best].name, BuildSuffix( best, bits ) );
    return 0;
  }
  if( mode=='l' ) {
    for( int i=0; i<nok; i++ ) printf( "%s%s\n", x86_target[m[i].t].name, BuildSuffix( m[i].t, bits ) );
    return nok ? 0 : 1;
  }
  if( mode=='f' ) {
    for( int f=0; f<N_FEAT; f++ ) {
      const X86Feat& x = x86_feat[f];
      const char* st = Has( c.f, f ) ? "yes" : Has( c.hw, f ) ? "off" : "-";
      printf( "%-20s %-4s %c  %-24s %s\n", x.name, st, x.cls, x.gcc ? x.gcc : "-", x.clang ? x.clang : "-" );
    }
    return 0;
  }
  if( mode=='t' ) {
    int t = t1;
    const X86Target& x = x86_target[t];
    std::vector<string> al;
    for( int u=0; u<N_TARGET; u++ ) if( (u!=t) && (x86_target[u].group==x.group) ) al.push_back( Label( u, C_BOTH, bits ) );
    printf( "target    %s: %s%s%s, %s, %s %d\n", x.name, (x.flags&T_GCC) ? "gcc" : "", ((x.flags&T_GCC) && (x.flags&T_CLANG)) ? " " : "",
            (x.flags&T_CLANG) ? "clang" : "", (x.flags&T_64) ? "x86-32 and x64" : "x86-32 only", VendorName( x.vendor ), x.year );
    if( al.size() ) printf( "same as   %s\n", Join( al ).c_str() );
    if( x.flags&T_CLANG ) printf( "clang     model %s\n", x86_model[x.model] );
    if( x.flags&T_GCC ) printf( "gcc       processor %s\n", x86_gproc[x.gproc] );
    printf( "level     %s, %s\n", LevelOf( TargetSet( t, comp ) ) ? ("x86-64-v" + std::to_string( LevelOf( TargetSet( t, comp ) ) )).c_str() : "-",
            TierName( TierOf( TargetSet( t, comp ) ) ) );
    if( (x.flags&T_GCC) && (x.flags&T_CLANG) && Equal( x.g, x.c ) ) Words( "features", Names( x.g ) );
    else {
      if( x.flags&T_GCC ) Words( "gcc", Names( x.g ) );
      if( x.flags&T_CLANG ) Words( "clang", Names( x.c ) );
    }
    int r = -1;
    for( int i=0; i<nall; i++ ) if( m[i].t==t ) r = i;
    if( r<0 ) printf( "here      not for %s builds%s\n", (bits==64) ? "x64" : "x86-32", Known( t, comp ) ? "" : " (or the compiler chosen)" );
    else if( r<nok ) {
      // the rank of its group, as the list counts them
      int gr = 0, ng = 0;
      for( int i=0; i<nok; i++ ) if( !i || (x86_target[m[i].t].group!=x86_target[m[i-1].t].group) ) { ng++; if( i<=r ) gr = ng; }
      printf( "here      runs: rank %d of %d for %s builds (%s%s)\n", gr, ng, (bits==64) ? "x64" : "x86-32", x.name, BuildSuffix( t, bits ) );
      if( !Empty( m[r].miss ) ) Words( "", { "without " + Join( Names( m[r].miss ) ) + " (intrinsics only)" } );
    } else {
      printf( "here      does not run: lacks %s", Join( Names( m[r].miss, 'C' ) ).c_str() );
      std::vector<string> i = Names( m[r].miss, 'I' );
      printf( "%s%s%s\n", i.size() ? " [and " : "", Join( i ).c_str(), i.size() ? "]" : "" );
    }
    return 0;
  }

  // the summary
  if( c.brand[0] ) printf( "cpu       %s\n", c.brand );
  printf( "%-10s%s, family %s model %s stepping %u (%08X)", c.brand[0] ? "" : "cpu", c.vendor, Hex( c.family ).c_str(),
          Hex( c.model ).c_str(), c.stepping, c.sig );
  if( c.hybrid ) printf( ", hybrid" );
  if( c.hypervisor ) printf( ", hypervisor %s", c.hv[0] ? c.hv : "?" );
  printf( "\n" );
  if( !dump ) {
    int th, co;
    Threads( th, co );
    if( th ) {
      printf( "threads   %d", th );
      if( co ) printf( ", %d core%s", co, (co==1) ? "" : "s" );
      printf( "\n" );
    }
  }
  if( c.ncache ) {
    std::vector<string> w;
    for( int i=0; i<c.ncache; i++ ) {
      const Cache& k = c.cache[i];
      string s = "L" + std::to_string( k.level ) + ((k.type==1) ? "d" : (k.type==2) ? "i" : "") + " " + Size( k.kb );
      w.push_back( s + (i+1<c.ncache ? "," : "") );
    }
    Words( "caches", w );
  }
  {
    std::vector<string> w;
    if( !c.osxsave || !d.xcr0_set ) w.push_back( "no xsave: sse only" );
    else {
      static const struct { qword m; const char* n; } st[] = { { 2, "sse" }, { 4, "avx" }, { 0xE0, "avx512" },
        { 0x60000, "amx" }, { 0x80000, "apx" } };
      w.push_back( "saves" );
      for( size_t i=0; i<sizeof(st)/sizeof(st[0]); i++ ) if( (c.xcr0&st[i].m)==st[i].m ) w.push_back( st[i].n );
      char b[40]; snprintf( b, sizeof(b), "(xcr0 %llX)", (unsigned long long)c.xcr0 ); w.push_back( b );
    }
    Words( "os", w );
  }
  if( !dump ) printf( "code      %s%s\n", (CodeBits()==64) ? "x64" : "x86-32",
                      ((bits==32) && Has( c.f, F_LM ) && !bitsopt) ? " (the CPU is 64-bit: -64 lists the x64 builds)" : "" );
  printf( "level     %s\n", c.level ? ("x86-64-v" + std::to_string( c.level )).c_str() : "x86-32 only" );
  Words( "features", Names( c.f ) );
  FSet off = Sub( c.hw, c.f );
  if( !Empty( off ) ) Words( "off", { Join( Names( off ) ) + " (in the CPU; not enabled by the OS)" } );
  bool sm = c.gcc && !strcmp( c.gcc, c.clang );
  if( sm && (comp==C_BOTH) ) printf( "native    %s (gcc and clang)\n", c.gcc );
  else {
    if( comp&C_GCC ) printf( "native    gcc: %s\n", c.gcc ? c.gcc : "- (gcc 15+ refuses Xeon Phi)" );
    if( comp&C_CLANG ) printf( "%s clang: %s\n", (comp&C_GCC) ? "         " : "native   ", c.clang );
  }
  if( comp&C_GCC ) Native( c, "gcc", c.gcc, bits );
  if( (comp&C_CLANG) && (strcmp( c.clang, "generic" ) || !(comp&C_GCC)) ) Native( c, "clang", c.clang, bits );
  // a native name whose own code does not run here (a cpu with features off)
  for( int k=0; k<2; k++ ) {
    const char* nm = k ? c.clang : c.gcc;
    if( !nm || (k && sm) || !(comp&(k ? C_CLANG : C_GCC)) ) continue;
    int t = FindTarget( nm ), r = -1;
    for( int i=0; i<nall; i++ ) if( m[i].t==t ) r = i;
    if( (r>=nok) ) printf( "          (-march=%s by itself does not run here: no %s)\n", nm, Join( Names( m[r].miss, 'C' ) ).c_str() );
  }

  printf( "%s targets for %s, best first%s:\n", (bits==64) ? "x64" : "x86-32", (comp==C_BOTH) ? "gcc and clang" : (comp==C_GCC) ? "gcc" : "clang",
          strict ? " (strict)" : "" );
  // a line per group: rank, tier, x86-64 level, names, and the intrinsics-only
  // features of it this cpu lacks
  for( int i=0, rank=0; i<nok; rank++ ) {
    int lv = LevelOf( m[i].need );
    char b[64]; snprintf( b, sizeof(b), "%4d  %-7s %-3s ", rank+1, TierName( m[i].tier ), lv ? ("v" + std::to_string( lv )).c_str() : "-" );
    std::vector<string> w;
    int j = i;
    for( ; (j<nok) && (x86_target[m[j].t].group==x86_target[m[i].t].group); j++ ) w.push_back( Label( m[j].t, comp, bits ) );
    if( !Empty( m[i].miss ) ) w.push_back( "[no " + Join( Names( m[i].miss ) ) + "]" );
    Words( b, w, 19 );
    i = j;
  }
  if( !nok ) printf( "  none\n" );
  if( bits==64 ) {
    static Match m3[N_TARGET];
    int n3all, n3 = RankTargets( c, comp, 32, strict!=0, m3, &n3all );
    std::vector<string> w;
    for( int i=0; i<n3; i++ ) if( !(x86_target[m3[i].t].flags&T_64) ) w.push_back( Label( m3[i].t, comp, 32 ) );
    if( w.size() ) { printf( "x86-32 builds can also use:\n" ); Words( "", w, 6 ); }
  }
  if( mode=='a' ) {
    printf( "not usable here:\n" );
    for( int i=nok; i<nall; i++ ) {
      std::vector<string> cm = Names( m[i].miss, 'C' ), im = Names( m[i].miss, 'I' );
      printf( "  %-22s lacks %s%s%s%s\n", Label( m[i].t, comp, bits ).c_str(), Join( cm ).c_str(), im.size() ? " [" : "", Join( im ).c_str(),
              im.size() ? "]" : "" );
    }
  }
  return 0;
}
