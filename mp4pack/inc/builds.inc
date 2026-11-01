// inc/builds.inc -- the CPU (its threads, the -march targets whose code runs
//  on it) and the tool builds for it (GetTool)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- the CPU, and the tool builds for it

// Builds of a tool are named by the -march target they are made for, gcc's
// and clang's names as x86targets.h has them: h264p_haswell.exe,
// h264p_x86-64-v3.exe, h264p_znver4.exe; x86-32 builds by their build names
// (h264p_haswell-32.exe, h264p_pentium4.exe); h264p.exe is taken to run
// anywhere (on Linux: h264p_haswell, h264p). The targets whose code runs on
// this CPU are ranked by x86targets.h (RankTargets: the vector ISA, what
// -march=native picks, the x86-64 level ...), and the build of the best one
// there is is used, x64 builds before x86-32 ones.

static string Lower( string s ) {
  for( size_t i=0; i<s.size(); i++ ) if( (s[i]>='A') && (s[i]<='Z') ) s[i] += 'a'-'A';
  return s;
}

static int cpu_threads, cpu_cores;
static x86::Cpu cpu;                    // this CPU (its features less what -i leaves out)
static int cpu_ok;                      // its CPUID was read
static string cpu_best;                 // the best target for it
static int cap = -1;                    // -i: only builds that run on this target
static x86::Match rk[2][x86::N_TARGET]; // the targets of x64 and x86-32 builds, usable ones first, best first
static int rk_n[2], rk_all[2];          // how many usable, how many in all

// the target a build name names, and its bits: haswell (x64), haswell-32 and
// pentium4 (x86-32); also the level names of earlier mp4pack versions (avx2:
// x86-64-v3 ...). -1: none
static int BuildOf( string s, int& bits ) {
  static const char* const alias[][2] = {
    { "x64", "x86-64" }, { "sse2", "x86-64" }, { "v1", "x86-64" }, { "sse3", "nocona" }, { "ssse3", "core2" },
    { "sse4", "penryn" }, { "sse41", "penryn" }, { "sse4.1", "penryn" }, { "sse42", "x86-64-v2" },
    { "sse4.2", "x86-64-v2" }, { "v2", "x86-64-v2" }, { "avx", "sandybridge" }, { "avx2", "x86-64-v3" },
    { "v3", "x86-64-v3" }, { "avx512", "x86-64-v4" }, { "v4", "x86-64-v4" } };
  s = Lower( s );
  for( size_t i=0; i<sizeof(alias)/sizeof(alias[0]); i++ ) if( s==alias[i][0] ) s = alias[i][1];
  return x86::FindBuild( s.c_str(), &bits );
}

// a build's place among the ones that run here (lower is better); -1: it does not
static int BuildRank( int t, int bits ) {
  int k = (bits==32);
  for( int i=0; i<rk_n[k]; i++ ) if( rk[k][i].t==t ) return k*x86::N_TARGET+i;
  return -1;
}

// what a build needs that the CPU (or -i's target) lacks: the features the
// compilers use on their own
static string Lacks( int t, int bits ) {
  int k = (bits==32);
  string s;
  for( int i=0; i<rk_all[k]; i++ ) if( rk[k][i].t==t ) {
    for( int f=0; f<x86::N_FEAT; f++ ) if( x86::Has( rk[k][i].miss, f ) && (x86::x86_feat[f].cls=='C') ) s += string( s.size() ? " " : "" ) + x86::x86_feat[f].name;
    break;
  }
  return s;
}

// the CPU by CPUID, and the targets of the builds it runs, ranked (-i: those
// that run on its target too)
static void CpuTargets() {
  static x86::Cpuid d;
  cpu_ok = x86::ReadCpuid( d );
  if( !cpu_ok ) return; // (none runs, as far as we know)
  x86::Analyze( d, cpu );
  for( int pass=0; pass<1+(cap>=0); pass++ ) {
    if( pass ) cpu.f = x86::And( cpu.f, x86::TargetSet( cap, x86::C_BOTH ) );
    for( int k=0; k<2; k++ ) rk_n[k] = x86::RankTargets( cpu, x86::C_BOTH, k ? 32 : 64, false, rk[k], &rk_all[k] );
    if( !pass && (rk_n[0] || rk_n[1]) ) cpu_best = x86::x86_target[rk[rk_n[0] ? 0 : 1][0].t].name;
  }
}

// logical processors in all processor groups, and cores
static void CpuThreads() {
  typedef DWORD (WINAPI *gapc_t)( WORD );
  typedef BOOL (WINAPI *glpi_t)( int, void*, DWORD* );
  HMODULE k = GetModuleHandleW( L"kernel32.dll" );
  gapc_t gapc = (gapc_t)(void*)GetProcAddress( k, "GetActiveProcessorCount" ); // (Windows 7+)
  glpi_t glpi = (glpi_t)(void*)GetProcAddress( k, "GetLogicalProcessorInformationEx" );
  DWORD n = gapc ? gapc( 0xFFFF ) : 0; // ALL_PROCESSOR_GROUPS
  if( n==0 ) { SYSTEM_INFO si; GetSystemInfo( &si ); n = si.dwNumberOfProcessors; }
  cpu_threads = std::max<int>( n, 1 );
  cpu_cores = 0;
  DWORD len = 0;
  if( glpi && !glpi( 0, 0, &len ) && (GetLastError()==ERROR_INSUFFICIENT_BUFFER) && len ) { // RelationProcessorCore
    vector<byte> b( len );
    if( glpi( 0, &b[0], &len ) ) for( DWORD o=0; o+8<=len; cpu_cores++ ) { // records: relation, size, ...
      DWORD sz; memcpy( &sz, &b[o+4], 4 );
      if( sz<8 ) break;
      o += sz;
    }
  }
}

static qword FT( const FILETIME& f ) { return (qword(f.dwHighDateTime)<<32) | f.dwLowDateTime; }

// -f: the threads free now, by the idle share of the CPUs over a second
static int FreeThreads() {
  FILETIME i0, k0, u0, i1, k1, u1;
  if( !GetSystemTimes( &i0, &k0, &u0 ) ) return cpu_threads;
  Sleep( 1000 );
  if( !GetSystemTimes( &i1, &k1, &u1 ) ) return cpu_threads;
  double idle = double( FT(i1)-FT(i0) ), all = double( (FT(k1)-FT(k0)) + (FT(u1)-FT(u0)) ); // (kernel time has the idle time)
  if( all<=0 ) return cpu_threads;
  return std::max( 1, std::min( cpu_threads, int( cpu_threads*idle/all+0.5 ) ) );
}

// a tool: its build to run (path empty: none that runs here), and the ones that do not
struct Tool { string base, file, later; wstring path; int used; };
static Tool tools[16];
static int n_tools;

// a file's place among the builds of a tool that run here (lower is better),
// and its target and bits; -1: it does not run here, -2: not a build of base
static int BuildPlace( const string& base, const string& file, int& tg, int& bits ) {
  string b = Lower( base ), e = EXE, l = Lower( file );
  if( (l.size()<b.size()+e.size()) || (l.compare( 0, b.size(), b )!=0) || (l.compare( l.size()-e.size(), e.size(), e )!=0) ) return -2;
  string x = l.substr( b.size(), l.size()-e.size()-b.size() ); // "" or "_haswell"
  bits = 64; tg = x.empty() ? x86::FindTarget( "x86-64" ) : (x[0]=='_') ? BuildOf( x.substr(1), bits ) : -1;
  if( tg<0 ) return -2; // (another tool's)
  int r = BuildRank( tg, bits );
  return (x.empty() && (r<0)) ? 2*x86::N_TARGET : r; // (taken to run anywhere)
}

// the best build of a tool in our directory: base.exe or base_<target>.exe
// (on Linux base or base_<target>)
static const Tool& GetTool( const string& base ) {
  for( int i=0; i<n_tools; i++ ) if( tools[i].base==base ) return tools[i];
  Tool& t = tools[n_tools++];
  t.base = base; t.used = 0;
  int best = -1;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (exe_dir+W(base)+L"*" EXE).c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) continue;
      string n = U(fd.cFileName);
      int tg, bits, r = BuildPlace( base, n, tg, bits );
      if( r==-2 ) continue;
      if( r<0 ) { t.later += (t.later.size() ? "; " : "") + n + (cpu_ok ? " lacks " + Lacks( tg, bits ) : string()); continue; }
      if( (best<0) || (r<best) ) { best = r; t.file = n; t.path = exe_dir + fd.cFileName; }
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  return t;
}

// the tools, in the order they are told of (the first two are needed)
static const string tool_names[] = { "avip", "tangelo", "h264p", "m4vpack", "aacpack", "ac3pack", "mp3pack", "oggcomp", "ttfcomp" };
static const int N_TOOLS = int( sizeof(tool_names)/sizeof(tool_names[0]) );

static string NoTool( const Tool& t ) {
  if( t.later.empty() ) return "no " + t.base + " in " + U(exe_dir);
  if( !cpu_ok ) return "no " + t.base + " build without a target, the only kind run with no CPUID (only " + t.later + ")";
  return "no " + t.base + " build that runs here" + ((cap>=0) ? " and on " + string( x86::x86_target[cap].name ) : string()) + " (" + t.later + ")";
}

