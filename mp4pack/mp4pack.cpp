// mp4pack.cpp -- lossless compression of whole mp4/mkv/avi files (Windows;
// Linux with winapi_posix.h, the Win32 functions used here on POSIX)
//
//   mp4pack c [options] input output     compress
//   mp4pack d [options] input output     decompress
//
// c: "avip c" splits the container into its streams and a .meta file in a work
// directory; each stream goes to its packer -- h264p (video), aacpack, mp3pack
// or oggcomp (audio), ttfcomp (fonts, all in one solid archive), "shar a22"
// (the rest; a22 or the level -l gives) -- and "shar a0" stores the packed
// files and an index (mp4pack.idx) as the output.
// d: "shar x", the unpackers, "avip d"; every stream and the result are checked
// against the sizes and CRC32s in the index.
//
// The tools are run from the directory of mp4pack.exe, each in the best build
// there for this CPU (h264p_znver4.exe, h264p_avx2.exe, h264p.exe, ...: by the
// -march target in the name, ranked by x86targets.h: GetTool), with the work
// directory as their current one and relative paths on their command lines:
// they take ANSI command lines, so only names that survive that reach them
// (ToolName). On Linux they are the Windows tools converted by pe2elf (named
// without .exe: h264p_avx2), which take '\' for '/', and UTF-8.

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define DS "\\"        // the directory separator, in literals: L"u" DS
#define EXE ".exe"     // the tools' extension
#else
#include "winapi_posix.h"
#define DS "/"
#define EXE ""
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../cpuinfo/x86targets.h" // the CPU, and the -march targets whose code runs on it

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long long qword;
using std::string;
using std::wstring;
using std::vector;

static const qword ALL = ~0ULL;

//---- what goes where

// -t/-c positions: the order the groups are given in
enum { G_VIDEO, G_AUDIO, G_FONT, G_OTHER, N_GROUP };
static const char* g_name[N_GROUP] = { "h264p", "audio", "ttfcomp", "shar" };

enum { M_STORE, M_SHAR, M_H264P, M_AACPACK, M_MP3PACK, M_OGGCOMP, M_TTFCOMP, N_METHOD };
static const char* m_name[N_METHOD] = { "store", "shar", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp" };
static const char* m_tool[N_METHOD] = { "", "shar", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp" };
static const int m_group[N_METHOD] = { G_OTHER, G_OTHER, G_VIDEO, G_AUDIO, G_AUDIO, G_AUDIO, G_FONT };

// by the extension avip gave the stream
static int Method( const string& name ) {
  size_t d = name.rfind( '.' );
  string e = (d==string::npos) ? string() : name.substr( d+1 );
  for( size_t i=0; i<e.size(); i++ ) if( (e[i]>='A') && (e[i]<='Z') ) e[i] += 'a'-'A';
  if( (e=="h264") || (e=="264") ) return M_H264P;
  if( e=="aac" ) return M_AACPACK;
  if( (e=="mp3") || (e=="mp2") || (e=="mp1") || (e=="mpa") ) return M_MP3PACK;
  if( (e=="ogg") || (e=="oga") ) return M_OGGCOMP;
  if( (e=="ttf") || (e=="otf") || (e=="ttc") ) return M_TTFCOMP;
  return M_SHAR;
}

//---- settings and state

static int th[N_GROUP];               // -t: threads of each group
static qword ch[N_GROUP];             // -c: part sizes, 0 = the default
static const int NO_LEVEL = -1000000;
static int lv[N_GROUP] = { NO_LEVEL, NO_LEVEL, NO_LEVEL, NO_LEVEL }; // -l: levels (NO_LEVEL: the tool's default)
static vector< vector<int> > stages;  // -t: groups run at once ('+'), stage after stage (',')
static int f_dec, f_verbose, f_keep, f_free;
static wstring w_base;                // -w

static wstring exe_dir;               // ours and the tools', with a trailing separator
static wstring work;                  // the work directory, with a trailing separator
static HANDLE h_err, h_jobobj, ev_abort, ev_done;
static volatile LONG f_abort;
static int f_con;                     // stderr is a console: there is a status line
static size_t status_len;             // its length on the screen
static qword t_start;
static qword t_run;                   // when what the status line follows began
static uint n_log;

//---- strings

static wstring W( const string& s ) {
  if( s.empty() ) return wstring();
  int n = MultiByteToWideChar( CP_UTF8, 0, s.data(), int(s.size()), 0, 0 );
  wstring w( n, 0 );
  if( n>0 ) MultiByteToWideChar( CP_UTF8, 0, s.data(), int(s.size()), &w[0], n );
  return w;
}

static string U( const wstring& w ) {
  if( w.empty() ) return string();
  int n = WideCharToMultiByte( CP_UTF8, 0, w.data(), int(w.size()), 0, 0, 0, 0 );
  string s( n, 0 );
  if( n>0 ) WideCharToMultiByte( CP_UTF8, 0, w.data(), int(w.size()), &s[0], n, 0, 0 );
  return s;
}

static string Num( qword v ) {
  char b[32]; int i = 31; b[i] = 0;
  do b[--i] = char('0'+v%10); while( v/=10 );
  return b+i;
}

static string INum( int v ) { return (v<0) ? "-"+Num( qword(-(long long)v) ) : Num( qword(v) ); }

static string Hex( uint v ) { char b[16]; snprintf( b, sizeof(b), "%08X", v ); return b; }

static string Pct( double a, double b ) { char s[32]; snprintf( s, sizeof(s), "%.2f%%", (b>0) ? 100*a/b : 0.0 ); return s; }

static string Clock( qword ms ) {
  uint s = uint( ms/1000 ); char b[32];
  snprintf( b, sizeof(b), "%u:%02u:%02u", s/3600, s/60%60, s%60 );
  return b;
}

static void Msg( const string& s );

// an error message (none after Ctrl+C: main says so) and the exit code
static int Fail( const string& m, int code ) {
  if( f_abort ) return 5;
  Msg( m );
  return code;
}

static string ExitCode( DWORD c ) { return (c>=0xC0000000u) ? "0x"+Hex(c) : Num(c); }

// a command line argument, as the MS C runtime splits them
static string Quote( const string& a ) {
  if( !a.empty() && (a.find_first_of( " \t\"" )==string::npos) ) return a;
  string r = "\"";
  size_t nb = 0;
  for( size_t i=0; i<a.size(); i++ ) {
    if( a[i]=='\\' ) { nb++; continue; }
    r.append( (a[i]=='"') ? nb*2+1 : nb, '\\' );
    nb = 0; r += a[i];
  }
  r.append( nb*2, '\\' );
  return r+"\"";
}

//---- console

static void Put( string s ) {
  DWORD n;
  if( f_con ) { wstring w = W(s); WriteConsoleW( h_err, w.data(), DWORD(w.size()), &n, 0 ); return; }
#ifdef _WIN32
  for( size_t i=0; (i=s.find('\n',i))!=string::npos; i+=2 ) s.insert( i, "\r" ); // (a file or pipe)
#endif
  WriteFile( h_err, s.data(), DWORD(s.size()), &n, 0 );
}

static void Msg( const string& s ) {
  if( status_len ) { Put( "\r"+string(status_len,' ')+"\r" ); status_len = 0; }
  Put( s+"\n" );
}

static size_t ConWidth() {
  CONSOLE_SCREEN_BUFFER_INFO ci;
  if( GetConsoleScreenBufferInfo( h_err, &ci ) && (ci.dwSize.X>20) ) return ci.dwSize.X;
  return 80;
}

//---- CRC32 (slicing by 8)

static uint crc_t[8][256];

static void CrcInit() {
  for( uint i=0; i<256; i++ ) {
    uint c = i;
    for( int k=0; k<8; k++ ) c = (c>>1) ^ (0xEDB88320u & (0u-(c&1)));
    crc_t[0][i] = c;
  }
  for( uint i=0; i<256; i++ ) for( int t=1; t<8; t++ ) crc_t[t][i] = (crc_t[t-1][i]>>8) ^ crc_t[0][crc_t[t-1][i]&255];
}

static uint Crc( uint crc, const byte* p, size_t n ) {
  crc = ~crc;
  for(; n && (size_t(p)&7); n-- ) crc = (crc>>8) ^ crc_t[0][(crc^*p++)&255];
  for(; n>=8; n-=8, p+=8 ) {
    uint a = crc ^ (p[0] | (p[1]<<8) | (p[2]<<16) | (uint(p[3])<<24));
    uint b = p[4] | (p[5]<<8) | (p[6]<<16) | (uint(p[7])<<24);
    crc = crc_t[7][a&255] ^ crc_t[6][(a>>8)&255] ^ crc_t[5][(a>>16)&255] ^ crc_t[4][a>>24]
        ^ crc_t[3][b&255] ^ crc_t[2][(b>>8)&255] ^ crc_t[1][(b>>16)&255] ^ crc_t[0][b>>24];
  }
  for(; n; n-- ) crc = (crc>>8) ^ crc_t[0][(crc^*p++)&255];
  return ~crc;
}

//---- files and paths

static wstring FullPath( const wstring& p ) {
  DWORD n = GetFullPathNameW( p.c_str(), 0, 0, 0 );
  if( n==0 ) return p;
  wstring r( n, 0 );
  n = GetFullPathNameW( p.c_str(), n, &r[0], 0 );
  r.resize( n );
  return r;
}

// our own calls take long paths in the \\?\ form
static wstring LongPath( const wstring& p ) {
#ifdef _WIN32
  if( (p.size()<MAX_PATH-12) || (p.compare(0,4,L"\\\\?\\")==0) ) return p;
  if( p.compare(0,2,L"\\\\")==0 ) return L"\\\\?\\UNC\\"+p.substr(2);
  return L"\\\\?\\"+p;
#else
  return p;
#endif
}

// a directory separator: '\' or '/' (on Linux '/' only)
static int IsSep( wchar_t c ) { return (c==L'/') || ((c==L'\\') && (DS[0]=='\\')); }

static wstring FileName( const wstring& p ) {
  size_t i = p.size();
  while( i && !IsSep( p[i-1] ) ) i--;
  return p.substr( i );
}

// 1 if a tool's ANSI command line carries the name as it is (on Linux it is
// UTF-8: a name with no '\' in it, and no byte that is not UTF-8)
static int AnsiOK( const wstring& w ) {
  if( w.empty() || (w.size()>=MAX_PATH-4) ) return 0;
#ifndef _WIN32
  for( size_t i=0; i<w.size(); i++ ) if( (w[i]==L'\\') || ((w[i]>=0xDC80) && (w[i]<=0xDCFF)) ) return 0;
#endif
  if( GetACP()==CP_UTF8 ) return 1;
  BOOL bad = 0;
  int n = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, w.c_str(), int(w.size()), 0, 0, 0, &bad );
  return (n>0) && !bad;
}

// an existing file's full name as a tool can take it: itself, else its 8.3
// form, else "" (then it has to be linked or copied into the work directory)
static wstring ToolName( const wstring& full ) {
  if( AnsiOK(full) ) return full;
  wchar_t s[MAX_PATH];
  DWORD n = GetShortPathNameW( LongPath(full).c_str(), s, MAX_PATH );
  if( (n>0) && (n<MAX_PATH) && (wcsncmp( s, L"\\\\?\\", 4 )!=0) && AnsiOK(s) ) return s;
  return wstring();
}

// what we accept as a stream or packed file name (from avip, or from an index)
static int SafeName( const string& n ) {
  if( n.empty() || (n.size()>200) || (n[0]=='.') || (n[0]==' ') || (n[n.size()-1]=='.') || (n[n.size()-1]==' ') ) return 0;
  for( size_t i=0; i<n.size(); i++ ) if( (byte(n[i])<32) || strchr( "\\/:*?\"<>|", n[i] ) ) return 0;
  string b = n.substr( 0, n.find('.') );
  for( size_t i=0; i<b.size(); i++ ) if( (b[i]>='a') && (b[i]<='z') ) b[i] -= 'a'-'A';
  static const char* dev[] = { "CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$" };
  for( size_t i=0; i<sizeof(dev)/sizeof(dev[0]); i++ ) if( b==dev[i] ) return 0;
  if( (b.size()==4) && ((b.compare(0,3,"COM")==0) || (b.compare(0,3,"LPT")==0)) && (b[3]>='0') && (b[3]<='9') ) return 0;
  return 1;
}

static int GetSize( const wstring& p, qword& sz ) {
  WIN32_FILE_ATTRIBUTE_DATA a;
  if( !GetFileAttributesExW( p.c_str(), GetFileExInfoStandard, &a ) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) return 1;
  sz = (qword(a.nFileSizeHigh)<<32) | a.nFileSizeLow;
  return 0;
}

static int Exists( const wstring& p ) { qword s; return GetSize( p, s )==0; }

static HANDLE OpenRead( const wstring& p ) {
  return CreateFileW( p.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0 );
}

static HANDLE OpenWrite( const wstring& p, DWORD how ) {
  return CreateFileW( p.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, how, 0, 0 );
}

// 1 when both name the same existing file
static int SameFile( const wstring& a, const wstring& b ) {
  HANDLE f = CreateFileW( a.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
  HANDLE g = CreateFileW( b.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
  BY_HANDLE_FILE_INFORMATION x, y;
  int r = (f!=INVALID_HANDLE_VALUE) && (g!=INVALID_HANDLE_VALUE) && GetFileInformationByHandle( f, &x ) && GetFileInformationByHandle( g, &y )
       && (x.dwVolumeSerialNumber==y.dwVolumeSerialNumber) && (x.nFileIndexHigh==y.nFileIndexHigh) && (x.nFileIndexLow==y.nFileIndexLow);
  if( f!=INVALID_HANDLE_VALUE ) CloseHandle( f );
  if( g!=INVALID_HANDLE_VALUE ) CloseHandle( g );
  return r;
}

struct IOBuf {
  byte* p; DWORD n;
  IOBuf( DWORD size ) { n = size; p = (byte*)VirtualAlloc( 0, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE ); }
  ~IOBuf() { if( p ) VirtualFree( p, 0, MEM_RELEASE ); }
};

// CRC32 and size of a file; 1 on an error
static int FileCrc( const wstring& path, uint& crc, qword& size ) {
  crc = 0; size = 0;
  HANDLE f = OpenRead( path );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  IOBuf b( 1<<20 );
  int r = (b.p==0);
  while( !r ) {
    DWORD n = 0;
    if( f_abort || !ReadFile( f, b.p, b.n, &n, 0 ) ) { r = 1; break; }
    if( n==0 ) break;
    crc = Crc( crc, b.p, n ); size += n;
  }
  CloseHandle( f );
  return r;
}

struct CrcTask { wstring path; uint crc; qword size; int err; };
static DWORD WINAPI CrcThread( void* a ) { CrcTask& t = *(CrcTask*)a; t.err = FileCrc( t.path, t.crc, t.size ); return 0; }

// len bytes from off of one file to the end of another
static int CopyRange( const wstring& from, qword off, qword len, HANDLE to ) {
  HANDLE f = OpenRead( from );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  IOBuf b( 1<<20 );
  LARGE_INTEGER o; o.QuadPart = LONGLONG(off);
  int r = (b.p==0) || !SetFilePointerEx( f, o, 0, FILE_BEGIN );
  while( !r && len ) {
    DWORD n = DWORD( std::min<qword>( b.n, len ) ), k = 0, w = 0;
    if( f_abort || !ReadFile( f, b.p, n, &k, 0 ) || (k!=n) || !WriteFile( to, b.p, k, &w, 0 ) || (w!=k) ) r = 1;
    len -= n;
  }
  CloseHandle( f );
  return r;
}

static int ReadAll( const wstring& path, string& t, qword limit ) {
  qword sz;
  if( GetSize( path, sz ) || (sz>limit) ) return 1;
  HANDLE f = OpenRead( path );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  t.assign( size_t(sz), 0 );
  DWORD n = 0;
  int r = sz && (!ReadFile( f, &t[0], DWORD(sz), &n, 0 ) || (n!=sz));
  CloseHandle( f );
  return r;
}

static int WriteAll( const wstring& path, const string& t ) {
  HANDLE f = OpenWrite( path, CREATE_ALWAYS );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  DWORD n = 0;
  int r = !t.empty() && (!WriteFile( f, t.data(), DWORD(t.size()), &n, 0 ) || (n!=t.size()));
  CloseHandle( f );
  return r;
}

// a file of this size (filled in later, at offsets)
static int MakeFile( const wstring& path, qword size ) {
  HANDLE f = OpenWrite( path, CREATE_ALWAYS );
  if( f==INVALID_HANDLE_VALUE ) return 1;
  LARGE_INTEGER o; o.QuadPart = LONGLONG(size);
  int r = !SetFilePointerEx( f, o, 0, FILE_BEGIN ) || !SetEndOfFile( f );
  CloseHandle( f );
  return r;
}

static void MakeDirs( const wstring& d ) { // d ends with a separator
  for( size_t i=1; i<d.size(); i++ ) if( IsSep(d[i]) && (d[i-1]!=':') && !IsSep(d[i-1]) ) CreateDirectoryW( d.substr(0,i).c_str(), 0 );
}

// a file, or a directory (named with a trailing separator) and all in it
static void Remove( const wstring& p ) {
  if( p.empty() ) return;
  if( !IsSep( p[p.size()-1] ) ) {
    if( !DeleteFileW( p.c_str() ) ) { SetFileAttributesW( p.c_str(), FILE_ATTRIBUTE_NORMAL ); DeleteFileW( p.c_str() ); }
    return;
  }
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (p+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      wstring n = fd.cFileName;
      if( (n==L".") || (n==L"..") ) continue;
      if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ) Remove( p+n+L"" DS );
      else if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) RemoveDirectoryW( (p+n).c_str() ); // (a link: not into it)
      else Remove( p+n );
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  RemoveDirectoryW( p.substr(0,p.size()-1).c_str() );
}

// avip's files in their order: s.0.*, s.1.*, ..., then the rest (s.meta)
static qword NameKey( const wstring& n ) {
  if( (n.size()<3) || (n[0]!='s') || (n[1]!='.') || (n[2]<'0') || (n[2]>'9') ) return ALL;
  qword v = 0;
  for( size_t i=2; (i<n.size()) && (n[i]>='0') && (n[i]<='9') && (v<(1ull<<40)); i++ ) v = v*10+(n[i]-'0');
  return v;
}

static bool NameLess( const wstring& a, const wstring& b ) {
  qword x = NameKey(a), y = NameKey(b);
  return (x!=y) ? (x<y) : (a<b);
}

static qword DirSize( const wstring& dir ) { // dir ends with a separator
  qword n = 0;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (dir+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      wstring f = fd.cFileName;
      if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) n += (qword(fd.nFileSizeHigh)<<32) | fd.nFileSizeLow;
      else if( (f!=L".") && (f!=L"..") ) n += DirSize( dir+f+L"" DS );
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  return n;
}

static vector<wstring> ListFiles( const wstring& dir ) {
  vector<wstring> r;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (dir+L"*").c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do if( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) r.push_back( fd.cFileName ); while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  std::sort( r.begin(), r.end(), NameLess );
  return r;
}

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

// the best build of a tool in our directory: base.exe or base_<target>.exe
// (on Linux base or base_<target>)
static const Tool& GetTool( const string& base ) {
  for( int i=0; i<n_tools; i++ ) if( tools[i].base==base ) return tools[i];
  Tool& t = tools[n_tools++];
  t.base = base; t.used = 0;
  int best = -1;
  string b = Lower( base ), e = EXE;
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW( (exe_dir+W(base)+L"*" EXE).c_str(), &fd );
  if( h!=INVALID_HANDLE_VALUE ) {
    do {
      if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) continue;
      string n = U(fd.cFileName), l = Lower(n);
      if( (l.size()<b.size()+e.size()) || (l.compare( 0, b.size(), b )!=0) || (l.compare( l.size()-e.size(), e.size(), e )!=0) ) continue;
      string x = l.substr( b.size(), l.size()-e.size()-b.size() ); // "" or "_haswell"
      int bits = 64, tg = x.empty() ? x86::FindTarget( "x86-64" ) : (x[0]=='_') ? BuildOf( x.substr(1), bits ) : -1;
      if( tg<0 ) continue; // (another tool's)
      int r = BuildRank( tg, bits );
      if( x.empty() && (r<0) ) r = 2*x86::N_TARGET; // (taken to run anywhere)
      if( r<0 ) { t.later += (t.later.size() ? "; " : "") + n + (cpu_ok ? " lacks " + Lacks( tg, bits ) : string()); continue; }
      if( (best<0) || (r<best) ) { best = r; t.file = n; t.path = exe_dir + fd.cFileName; }
    } while( FindNextFileW( h, &fd ) );
    FindClose( h );
  }
  return t;
}

static string NoTool( const Tool& t ) {
  if( t.later.empty() ) return "no " + t.base + " in " + U(exe_dir);
  if( !cpu_ok ) return "no " + t.base + " build without a target, the only kind run with no CPUID (only " + t.later + ")";
  return "no " + t.base + " build that runs here" + ((cap>=0) ? " and on " + string( x86::x86_target[cap].name ) : string()) + " (" + t.later + ")";
}

// a warning when the work directory's disk has less than this free
static void SpaceCheck( qword need ) {
  ULARGE_INTEGER fr;
  if( GetDiskFreeSpaceExW( work.c_str(), &fr, 0, 0 ) && (fr.QuadPart<need) )
    Msg( "warning: " + Num(fr.QuadPart>>20) + "MB free for the work directory, about " + Num(need>>20) + "MB may be needed (-w)" );
}

static int MakeWork() {
  wstring base;
  if( w_base.size() ) base = FullPath( w_base );
  else {
    wchar_t t[MAX_PATH+2];
    DWORD n = GetTempPathW( MAX_PATH+1, t );
    if( (n==0) || (n>MAX_PATH) ) { Msg( "no temp directory; give one with -w" ); return 1; }
    base.assign( t, n );
  }
  if( !IsSep( base[base.size()-1] ) ) base += L"" DS;
  MakeDirs( base );
  for( uint i=0; i<1000; i++ ) {
    wstring d = base + L"mp4pack_" + W(Hex(GetCurrentProcessId())) + L"_" + W(Num(i));
    if( CreateDirectoryW( d.c_str(), 0 ) ) { work = d+L"" DS; break; }
    if( GetLastError()!=ERROR_ALREADY_EXISTS ) break;
  }
  if( work.empty() ) { Msg( "cannot make a work directory in " + U(base) ); return 1; }
  if( work.size()>MAX_PATH-64 ) { Msg( "the work directory's path is too long: " + U(work) ); return 1; }
  const wchar_t* sub[] = { L"u", L"p", L"log" };
  for( int i=0; i<3; i++ ) if( !CreateDirectoryW( (work+sub[i]).c_str(), 0 ) ) { Msg( "cannot make " + U(work+sub[i]) ); return 1; }
  Msg( "work directory: " + U(work) );
  return 0;
}

//---- running the tools

// a file region fed to a tool's stdin, or its stdout written to a file
struct Pump {
  wstring path;
  qword off, len;         // stdout: len is the most it may write (ALL: any)
  int dir;                // 0: file -> stdin, 1: stdout -> file
  int create;             // stdout: create the file (else write into the existing one)
  HANDLE pipe, file, th;  // our end; the file; the thread
  volatile qword done;
  volatile LONG err;      // 1: file I/O, 2: the tool did not read all of it, 3: it wrote more than len
  Pump() : off(0), len(0), dir(0), create(0), pipe(0), file(0), th(0), done(0), err(0) {}
};

static DWORD WINAPI PumpThread( void* arg ) {
  Pump& p = *(Pump*)arg;
  IOBuf b( 1<<20 );
  byte tmp[4096];
  byte* buf = b.p ? b.p : tmp;
  DWORD bn = b.p ? b.n : DWORD(sizeof(tmp));
  LARGE_INTEGER o; o.QuadPart = LONGLONG(p.off);
  if( !SetFilePointerEx( p.file, o, 0, FILE_BEGIN ) ) p.err = 1;
  if( p.dir==0 ) {
    while( !p.err && (p.done<p.len) ) {
      DWORD n = DWORD( std::min<qword>( bn, p.len-p.done ) ), k = 0, w = 0;
      if( !ReadFile( p.file, buf, n, &k, 0 ) || (k==0) ) { p.err = 1; break; }
      if( !WriteFile( p.pipe, buf, k, &w, 0 ) || (w!=k) ) { p.err = 2; break; } // it closed its stdin
      p.done += k;
    }
  } else {
    while(1) { // until it closes its stdout; after an error, only drained so that it can finish
      DWORD k = 0, w = 0;
      if( !ReadFile( p.pipe, buf, bn, &k, 0 ) ) break;
      if( p.err || (k==0) ) continue;
      if( k>p.len-p.done ) { p.err = 3; continue; }
      if( !WriteFile( p.file, buf, k, &w, 0 ) || (w!=k) ) { p.err = 1; continue; }
      p.done += k;
    }
  }
  CloseHandle( p.pipe ); p.pipe = 0; // stdin: its EOF
  CloseHandle( p.file ); p.file = 0;
  return 0;
}

// the pipe and the file; child = the pipe's end for the tool (inheritable)
static int PumpOpen( Pump& p, HANDLE& child ) {
  HANDLE r, w;
  p.done = 0; p.err = 0; p.th = 0;
  p.file = p.dir ? OpenWrite( p.path, p.create ? CREATE_ALWAYS : OPEN_EXISTING ) : OpenRead( p.path );
  if( p.file==INVALID_HANDLE_VALUE ) { p.file = 0; return 1; }
  if( !CreatePipe( &r, &w, 0, 1<<16 ) ) { CloseHandle( p.file ); p.file = 0; return 1; }
  child = p.dir ? w : r;
  p.pipe = p.dir ? r : w;
  SetHandleInformation( child, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
  return 0;
}

static void PumpClose( Pump& p ) {
  if( p.pipe ) { CloseHandle( p.pipe ); p.pipe = 0; }
  if( p.file ) { CloseHandle( p.file ); p.file = 0; }
}

struct Job {
  string label;           // for the messages
  string tool;            // "h264p": its build is found by GetTool
  string args;            // the rest of its command line
  string cwd;             // its current directory, in the work directory
  int g, w;               // group; threads it takes from the group's
  int prog;               // its progress: 0 -, 1 the last "NN%" in its log, 2 its stdin, 3 its stdout
  int report;             // a line when it is done
  int quiet;              // its output to its log even with -v, its command line not shown
  qword work;             // the bytes it packs (c) or unpacks (d), for the progress
  vector<int> sids;       // c: the streams it packs
  vector<wstring> outs;   // files it has to make (c: their sizes are reported)
  vector<wstring> del;    // removed once it is done (unless -k)
  Pump in, out;           // in.path/out.path empty: none
  int st;                 // 0 waiting, 1 running, 2 done, 3 failed, 4 skipped
  int wu;                 // threads it took
  HANDLE hp;
  int exited;             // hp is signaled
  wstring log;
  DWORD code;
  qword t0, t;
  double frac;
  string why;
  Job() : g(G_OTHER), w(1), prog(0), report(1), quiet(0), work(0), st(0), wu(0), hp(0), exited(0), code(0), t0(0), t(0), frac(0) { out.dir = 1; }
};

static HANDLE Inheritable( HANDLE h ) {
  HANDLE d = 0;
  if( h && (h!=INVALID_HANDLE_VALUE) && !DuplicateHandle( GetCurrentProcess(), h, GetCurrentProcess(), &d, 0, TRUE, DUPLICATE_SAME_ACCESS ) ) d = 0;
  return d;
}

// Only this thread starts processes, and the tool's ends of its pipes and log
// are inheritable just for its own CreateProcess, so no tool holds another's
// pipe open (which would keep that one from seeing EOF).
static int Start( Job& j ) {
  Tool& t = (Tool&)GetTool( j.tool );
  if( t.path.empty() ) { j.why = NoTool(t); j.st = 3; return 1; }
  t.used = 1;
  wstring exe = t.path;
  wstring cmd = L"\"" + exe + L"\" " + W(j.args);
  wstring cwd = work + W(j.cwd);
  if( f_verbose && !j.quiet ) Msg( "> " + (j.cwd.size() ? "("+j.cwd+") " : string()) + t.file + " " + j.args
                     + (j.in.path.size() ? " < "+U(j.in.path) : string()) + (j.out.path.size() ? " > "+U(j.out.path) : string()) );
  SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
  HANDLE c_in = 0, c_out = 0, c_err = 0;
  string err;
  if( j.in.path.size() ) { if( PumpOpen( j.in, c_in ) ) err = "cannot read " + U(j.in.path); }
  else {
    c_in = CreateFileW( L"NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, 0 );
    if( c_in==INVALID_HANDLE_VALUE ) c_in = 0;
  }
  if( err.empty() && j.out.path.size() && PumpOpen( j.out, c_out ) ) err = "cannot write " + U(j.out.path);
  if( err.empty() ) {
    if( f_verbose && !j.quiet ) c_err = Inheritable( h_err );
    else {
      j.log = work + L"log" DS + W(Num(++n_log)) + L"_" + W(j.tool) + L".txt";
      c_err = CreateFileW( j.log.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, &sa, CREATE_ALWAYS, 0, 0 );
      if( c_err==INVALID_HANDLE_VALUE ) c_err = 0;
    }
  }
  PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(pi) );
  if( err.empty() ) {
    STARTUPINFOW si; memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = c_in;
    si.hStdOutput = c_out ? c_out : c_err; // shar's progress is on stdout
    si.hStdError = c_err;
    vector<wchar_t> cl( cmd.begin(), cmd.end() ); cl.push_back( 0 );
    // suspended until it is in our job object: it dies with us
    if( !CreateProcessW( exe.c_str(), &cl[0], 0, 0, TRUE, CREATE_SUSPENDED, 0, cwd.c_str(), &si, &pi ) )
      err = "cannot run " + U(exe) + " (error " + Num(GetLastError()) + ")";
  }
  if( c_in ) CloseHandle( c_in );
  if( c_out ) CloseHandle( c_out );
  if( c_err ) CloseHandle( c_err );
  if( err.size() ) { PumpClose( j.in ); PumpClose( j.out ); j.why = err; j.st = 3; return 1; }
  if( h_jobobj ) AssignProcessToJobObject( h_jobobj, pi.hProcess );
  ResumeThread( pi.hThread );
  CloseHandle( pi.hThread );
  j.hp = pi.hProcess; j.exited = 0; j.st = 1; j.t0 = GetTickCount64(); j.frac = 0;
  if( j.in.pipe ) j.in.th = CreateThread( 0, 0, PumpThread, &j.in, 0, 0 );
  if( j.out.pipe ) j.out.th = CreateThread( 0, 0, PumpThread, &j.out, 0, 0 );
  if( (j.in.pipe && !j.in.th) || (j.out.pipe && !j.out.th) ) {
    TerminateProcess( j.hp, 1 );
    if( !j.in.th ) PumpClose( j.in );
    if( !j.out.th ) PumpClose( j.out );
  }
  return 0;
}

// notes what of it has finished (a pump's handle is closed then, so that it is
// not waited for again); 1 when the process has exited and its pumps are through
static int Ended( Job& j ) {
  if( !j.exited && (WaitForSingleObject( j.hp, 0 )==WAIT_OBJECT_0) ) j.exited = 1;
  Pump* pp[2] = { &j.in, &j.out };
  for( int i=0; i<2; i++ ) if( pp[i]->th && (WaitForSingleObject( pp[i]->th, 0 )==WAIT_OBJECT_0) ) { CloseHandle( pp[i]->th ); pp[i]->th = 0; }
  return j.exited && !j.in.th && !j.out.th;
}

// 0: it did what it should
static int Finish( Job& j ) {
  DWORD code = 1;
  GetExitCodeProcess( j.hp, &code );
  CloseHandle( j.hp ); j.hp = 0;
  j.code = code; j.t = GetTickCount64()-j.t0;
  Pump* pp[2] = { &j.in, &j.out };
  for( int i=0; i<2; i++ ) { if( pp[i]->th ) CloseHandle( pp[i]->th ); pp[i]->th = 0; PumpClose( *pp[i] ); }
  string why;
  if( code ) why = "exit code " + ExitCode(code);
  else if( j.in.path.size() && (j.in.err==2) ) why = "it did not read all of its input";
  else if( j.in.path.size() && (j.in.err || (j.in.done!=j.in.len)) ) why = "cannot read " + U(j.in.path);
  else if( j.out.path.size() && (j.out.err==3) ) why = "it wrote more than expected";
  else if( j.out.path.size() && j.out.err ) why = "cannot write " + U(j.out.path);
  else if( j.out.path.size() && (j.out.len!=ALL) && (j.out.done!=j.out.len) ) why = "it wrote less than expected";
  else for( size_t i=0; i<j.outs.size(); i++ ) if( !Exists(j.outs[i]) ) { why = "it did not make " + U(j.outs[i]); break; }
  j.why = why;
  j.st = why.empty() ? 2 : 3;
  return j.st==3;
}

// the end of what the tool wrote, a few lines
static void LogTail( const Job& j ) {
  string t;
  if( j.log.empty() ) return;
  HANDLE f = OpenRead( j.log );
  if( f==INVALID_HANDLE_VALUE ) return;
  LARGE_INTEGER sz, o; char b[1536]; DWORD n = 0;
  if( GetFileSizeEx( f, &sz ) ) {
    o.QuadPart = (sz.QuadPart>LONGLONG(sizeof(b))) ? sz.QuadPart-LONGLONG(sizeof(b)) : 0;
    if( SetFilePointerEx( f, o, 0, FILE_BEGIN ) ) ReadFile( f, b, sizeof(b), &n, 0 );
  }
  CloseHandle( f );
  vector<string> l( 1 );
  for( DWORD i=0; i<n; i++ ) {
    if( (b[i]=='\r') || (b[i]=='\n') ) { if( l.back().size() ) l.push_back( string() ); }
    else if( byte(b[i])>=32 ) l.back() += b[i];
  }
  if( l.back().empty() ) l.pop_back();
  for( size_t i=(l.size()>10) ? l.size()-10 : 0; i<l.size(); i++ ) Msg( "  | " + l[i] );
}

// the last "NN.N%" the tool wrote to its log, as 0..1 (-1: none)
static double LogPct( const wstring& log ) {
  HANDLE f = OpenRead( log );
  if( f==INVALID_HANDLE_VALUE ) return -1;
  LARGE_INTEGER sz, o; char b[512]; DWORD n = 0;
  if( GetFileSizeEx( f, &sz ) ) {
    o.QuadPart = (sz.QuadPart>LONGLONG(sizeof(b))) ? sz.QuadPart-LONGLONG(sizeof(b)) : 0;
    if( SetFilePointerEx( f, o, 0, FILE_BEGIN ) ) ReadFile( f, b, sizeof(b), &n, 0 );
  }
  CloseHandle( f );
  for( int i=int(n)-1; i>0; i-- ) if( b[i]=='%' ) {
    int k = i;
    while( (k>0) && (((b[k-1]>='0') && (b[k-1]<='9')) || (b[k-1]=='.')) ) k--;
    if( k==i ) continue;
    double v = atof( string( b+k, b+i ).c_str() )/100;
    return (v<0) ? 0 : (v>1) ? 1 : v;
  }
  return -1;
}

static double Frac( Job& j ) {
  if( j.st==2 ) return 1;
  if( j.st!=1 ) return 0;
  double f = -1;
  if( j.prog==1 ) f = j.log.size() ? LogPct( j.log ) : -1;
  if( (j.prog==2) && j.in.len ) f = double(j.in.done)/double(j.in.len);
  if( (j.prog==3) && j.work ) f = std::min( 1.0, double(j.out.done)/double(j.work) );
  if( f>=0 ) j.frac = f;
  return j.frac;
}

// [elapsed] overall%, eta, and what runs; on a console only
static void Status( const vector<Job*>& all, const vector<Job*>& run ) {
  static qword last;
  if( !f_con || f_verbose ) return;
  if( status_len && (GetTickCount64()-last<500) ) return;
  last = GetTickCount64();
  double tot = 0, done = 0;
  int any = 0; // a job that tells its progress
  for( size_t i=0; i<all.size(); i++ ) if( all[i]->st!=4 ) {
    double w = double( std::max<qword>( all[i]->work, 1 ) );
    tot += w; done += w*Frac( *all[i] );
    any |= all[i]->prog;
  }
  qword el = GetTickCount64()-t_start;
  string s = "[" + Clock(el) + "]";
  if( any ) s += " " + Pct( done, tot );
  if( any && (done>0) && (done<tot) && (done>tot/200) ) s += " eta " + Clock( qword( double(GetTickCount64()-t_run)*(tot-done)/done ) );
  for( size_t i=0; i<run.size(); i++ ) s += "  " + run[i]->label + (run[i]->prog ? " "+Pct( Frac(*run[i]), 1 ) : string());
  size_t w = ConWidth()-1;
  wstring ws = W(s);
  if( ws.size()>w ) ws.resize( w );
  size_t n = ws.size();
  if( n<status_len ) ws.append( status_len-n, L' ' );
  Put( "\r"+U(ws) );
  status_len = n;
}

static int Fallback( Job& j );
static int Skip( const Job& j );
static void Succeeded( Job& j );
static string LevelArg( int m );

// Runs the waiting jobs of these groups, the groups at once, each group's in
// its order within its threads (a job waits for the ones before it). 0: all
// done (failed packers fell back), 1: one failed, 2: Ctrl+C.
static int RunStage( const vector<Job*>& all, const vector<int>& groups ) {
  vector<Job*> run;
  int used[N_GROUP] = { 0 }, fail = 0;
  while(1) {
    for( size_t gi=0; gi<groups.size() && !fail && !f_abort; gi++ ) {
      int g = groups[gi], budget = std::max( th[g], 1 );
      for( size_t i=0; i<all.size(); i++ ) {
        Job& j = *all[i];
        if( (j.g!=g) || (j.st!=0) ) continue;
        if( Skip(j) ) { j.st = 4; continue; }
        int w = std::max( 1, std::min( j.w, budget ) );
        if( used[g] && (used[g]+w>budget) ) break;
        if( Start(j) ) {
          if( Fallback(j) ) continue;
          Msg( "error: " + j.label + ": " + j.why );
          fail = 1;
          for( size_t k=0; k<run.size(); k++ ) TerminateProcess( run[k]->hp, 1 );
          break;
        }
        j.wu = w; used[g] += w;
        run.push_back( &j );
      }
    }
    if( run.empty() ) break;
    vector<HANDLE> h( 1, ev_abort ); // what has not finished yet
    for( size_t i=0; i<run.size(); i++ ) {
      if( !run[i]->exited ) h.push_back( run[i]->hp );
      if( run[i]->in.th ) h.push_back( run[i]->in.th );
      if( run[i]->out.th ) h.push_back( run[i]->out.th );
    }
    WaitForMultipleObjects( DWORD( std::min<size_t>( h.size(), MAXIMUM_WAIT_OBJECTS ) ), &h[0], FALSE, 1000 );
    if( f_abort && !fail ) { fail = 2; for( size_t i=0; i<run.size(); i++ ) TerminateProcess( run[i]->hp, 1 ); }
    for( size_t i=0; i<run.size(); ) {
      Job& j = *run[i];
      if( !Ended(j) ) { i++; continue; }
      run.erase( run.begin()+i );
      used[j.g] -= j.wu;
      if( Finish(j)==0 ) { Succeeded( j ); continue; }
      // the tools get Ctrl+C too, and may end before our handler has run
      if( (j.code==0xC000013Au) && !f_abort ) { f_abort = 1; SetEvent( ev_abort ); }
      if( f_abort && !fail ) { fail = 2; for( size_t k=0; k<run.size(); k++ ) TerminateProcess( run[k]->hp, 1 ); }
      if( fail ) continue; // (we stopped it)
      if( Fallback(j) ) continue;
      Msg( "error: " + j.label + ": " + j.why );
      LogTail( j );
      fail = 1;
      for( size_t k=0; k<run.size(); k++ ) TerminateProcess( run[k]->hp, 1 );
    }
    Status( all, run );
  }
  return fail;
}

static int RunOne( Job& j ) {
  vector<Job*> v( 1, &j );
  vector<int> g( 1, j.g );
  j.report = 0;
  t_run = GetTickCount64();
  return RunStage( v, g );
}

// a tool's exit code for this command line, run quietly; -1: it did not run
static long long Probe( const string& tool, const string& args ) {
  Job j; j.tool = tool; j.args = args; j.quiet = 1;
  if( Start( j ) ) return -1;
  if( WaitForSingleObject( j.hp, 10000 )!=WAIT_OBJECT_0 ) { TerminateProcess( j.hp, 1 ); WaitForSingleObject( j.hp, 5000 ); }
  Finish( j );
  return j.code;
}

//---- streams and the index

struct Part {
  qword off, len;   // what of the stream it holds
  string pk;        // the packed file in p, or the archive it is in ("-": p/store/<name>)
  string pk2;       // mp3pack's "outside" file; its name in a shar archive
  Part( qword o, qword l, const string& a, const string& b ) : off(o), len(l), pk(a), pk2(b) {}
};

struct Stream {
  string name;      // as avip named it, in u (from the index: checked by SafeName)
  qword size;
  uint crc;
  int m;            // method
  vector<Part> part;
  int refs;         // c: jobs yet to read it
  int failed;       // c: its packer failed; it goes to shar instead
  Stream() : size(0), crc(0), m(M_SHAR), refs(0), failed(0) {}
};

static vector<Stream> S;
static vector<Job*> J;      // the packers (or unpackers)
static string src_name;
static qword src_size;
static uint src_crc;

static wstring UPath( const string& name ) { return work + L"u" DS + W(name); }
static wstring PPath( const string& name ) { return work + L"p" DS + W(name); }

static Job* NewJob( int g, int w, const string& label, const string& tool ) {
  Job* j = new Job;
  j->g = g; j->w = std::max( w, 1 ); j->label = label; j->tool = tool;
  J.push_back( j );
  return j;
}

// c: a part of a stream whose packer failed elsewhere is not run
static int Skip( const Job& j ) {
  if( f_dec ) return 0;
  for( size_t i=0; i<j.sids.size(); i++ ) if( S[j.sids[i]].failed ) return 1;
  return 0;
}

// c: a packer failed; its streams get packed by shar at the end instead
static int Fallback( Job& j ) {
  if( f_dec || j.sids.empty() || f_abort ) return 0;
  string n;
  for( size_t i=0; i<j.sids.size(); i++ ) { n += (i ? ", " : "") + S[j.sids[i]].name; S[j.sids[i]].failed = 1; S[j.sids[i]].refs--; }
  Msg( "warning: " + j.label + ": " + j.why + "; " + n + " will be packed with shar " + LevelArg(M_SHAR) + " instead" );
  LogTail( j );
  return 1;
}

static void Succeeded( Job& j ) {
  if( j.report ) {
    string s = "  " + j.label + ": ";
    if( !f_dec ) {
      qword o = 0, sz;
      for( size_t i=0; i<j.outs.size(); i++ ) if( !GetSize( j.outs[i], sz ) ) o += sz;
      s += Num(j.work) + " -> " + Num(o) + " (" + Pct( double(o), double(j.work) ) + ")";
    } else s += Num(j.work);
    Msg( s + ", " + Clock(j.t) );
  }
  for( size_t i=0; i<j.sids.size(); i++ ) {
    Stream& s = S[j.sids[i]];
    if( (--s.refs==0) && !s.failed && !f_keep ) Remove( UPath(s.name) );
  }
  if( !f_keep ) for( size_t i=0; i<j.del.size(); i++ ) Remove( j.del[i] );
}

static string src_tools; // d: the tool builds the archive was made with

static string IndexText() {
  string t = "mp4pack 1\nsrc " + Num(src_size) + " " + Hex(src_crc) + " " + src_name + "\ntools";
  for( int i=0; i<n_tools; i++ ) if( tools[i].used || (tools[i].base=="shar") ) t += " " + tools[i].file;
  t += "\n";
  for( size_t i=0; i<S.size(); i++ ) {
    t += "stream " + Num(S[i].size) + " " + Hex(S[i].crc) + " " + m_name[S[i].m] + " " + S[i].name + "\n";
    for( size_t k=0; k<S[i].part.size(); k++ ) {
      const Part& p = S[i].part[k];
      t += "part " + Num(p.off) + " " + Num(p.len) + " " + p.pk + (p.pk2.size() ? " "+p.pk2 : string()) + "\n";
    }
  }
  return t + "end\n";
}

static string Field( const string& l, size_t& i ) {
  size_t e = l.find( ' ', i );
  if( e==string::npos ) e = l.size();
  string f = l.substr( i, e-i );
  i = (e<l.size()) ? e+1 : e;
  return f;
}

static int Number( const string& s, qword& v ) {
  v = 0;
  if( s.empty() || (s.size()>19) ) return 1;
  for( size_t i=0; i<s.size(); i++ ) { if( (s[i]<'0') || (s[i]>'9') ) return 1; v = v*10+(s[i]-'0'); }
  return 0;
}

static int HexNum( const string& s, uint& v ) {
  v = 0;
  if( s.size()!=8 ) return 1;
  for( size_t i=0; i<8; i++ ) {
    char c = s[i];
    int d = ((c>='0') && (c<='9')) ? c-'0' : ((c>='A') && (c<='F')) ? c-'A'+10 : -1;
    if( d<0 ) return 1;
    v = v*16+d;
  }
  return 0;
}

// p\mp4pack.idx into S; the names are checked, so that none leads out of the work directory
static int ReadIndex( string& why ) {
  string t;
  if( ReadAll( PPath("mp4pack.idx"), t, 1<<26 ) ) { why = "no mp4pack.idx in it: not an mp4pack archive?"; return 1; }
  vector<string> L;
  for( size_t i=0; i<t.size(); ) {
    size_t e = t.find( '\n', i );
    if( e==string::npos ) e = t.size();
    string l = t.substr( i, e-i );
    if( l.size() && (l[l.size()-1]=='\r') ) l.erase( l.size()-1 );
    L.push_back( l );
    i = e+1;
  }
  why = "bad mp4pack.idx";
  if( L.empty() || (L[0]!="mp4pack 1") ) { why = "not an mp4pack archive, or a later version"; return 1; }
  int f_src = 0, f_end = 0;
  for( size_t n=1; n<L.size() && !f_end; n++ ) {
    const string& l = L[n];
    size_t i = 0;
    string k = Field( l, i );
    if( k=="src" ) {
      if( Number( Field(l,i), src_size ) || HexNum( Field(l,i), src_crc ) ) return 1;
      src_name = l.substr( i ); f_src = 1;
      for( size_t q=0; q<src_name.size(); q++ ) if( byte(src_name[q])<32 ) src_name[q] = '?'; // (only shown)
    } else if( k=="stream" ) {
      Stream s;
      if( Number( Field(l,i), s.size ) || HexNum( Field(l,i), s.crc ) ) return 1;
      string m = Field( l, i );
      for( s.m=0; (s.m<N_METHOD) && (m!=m_name[s.m]); s.m++ );
      s.name = l.substr( i );
      if( (s.m==N_METHOD) || !SafeName(s.name) ) return 1;
      for( size_t q=0; q<S.size(); q++ ) if( lstrcmpiW( W(S[q].name).c_str(), W(s.name).c_str() )==0 ) return 1;
      S.push_back( s );
    } else if( k=="part" ) {
      qword o, z;
      if( S.empty() || Number( Field(l,i), o ) || Number( Field(l,i), z ) ) return 1;
      string a = Field( l, i ), b = l.substr( i );
      if( ((a!="-") || (S.back().m!=M_STORE)) && !SafeName(a) ) return 1;
      if( b.size() && !SafeName(b) ) return 1;
      S.back().part.push_back( Part( o, z, a, b ) );
    } else if( k=="tools" ) { // (only shown)
      src_tools = l.substr( i );
      for( size_t q=0; q<src_tools.size(); q++ ) if( byte(src_tools[q])<32 ) src_tools[q] = '?';
    }
    else if( k=="end" ) f_end = 1;
    else if( l.size() ) return 1;
  }
  if( !f_src || !f_end ) return 1;
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    qword o = 0;
    if( s.part.empty() ) return 1;
    for( size_t k=0; k<s.part.size(); k++ ) {
      Part& p = s.part[k];
      if( (p.off!=o) || (p.len>s.size-o) ) return 1;
      if( ((s.m==M_MP3PACK) || (s.m==M_SHAR)) && p.pk2.empty() ) return 1;
      o += p.len;
    }
    if( o!=s.size ) return 1;
    if( (s.part.size()>1) && (s.m!=M_AACPACK) && (s.m!=M_SHAR) ) return 1;
  }
  return 0;
}

//---- options

// "3+3,1,1", "50%+50%-2": threads of h264p, the audio packers, ttfcomp and
// shar, in that order. Each is a number, or a percentage of the threads there
// are (all of the CPU's, or the free ones: -f), rounded up. '+' runs a group
// together with the ones before it, ',' after them. Missing ones: the audio's
// is the video's, after it; ttfcomp and shar get 1, both at once, after the
// rest ("4" = "4,4,1+1", "3+3" = "3+3,1+1"). A "-N" at the end takes N from
// the threads there are, for the percentages; alone, for the default
// (100%,100% = N,N,1+1: all of them for h264p, then for the audio).
static int ParseThreads( string spec, int all ) {
  int sub = 0;
  size_t m = spec.rfind( '-' );
  if( m!=string::npos ) {
    if( (m+1>=spec.size()) || (spec.size()-m>4) ) return 1;
    for( size_t i=m+1; i<spec.size(); i++ ) { if( (spec[i]<'0') || (spec[i]>'9') ) return 1; sub = sub*10+(spec[i]-'0'); }
    spec.erase( m );
  }
  if( spec.empty() ) spec = "100%,100%";
  int max = std::max( 1, all-sub ), g = 0;
  const char* s = spec.c_str();
  char sep = ',';
  stages.clear();
  while(1) {
    if( (g>=N_GROUP) || (*s<'0') || (*s>'9') ) return 1;
    int v = 0;
    while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>10000 ) return 1; }
    if( *s=='%' ) { s++; v = std::min( 64, (max*v+99)/100 ); }
    else if( v>64 ) return 1;
    th[g] = v;
    if( (g==0) || (sep==',') ) stages.push_back( vector<int>() );
    stages.back().push_back( g++ );
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    sep = *s++;
  }
  for( int g0=g; g<N_GROUP; g++ ) {
    th[g] = (g==G_AUDIO) ? th[G_VIDEO] : 1;
    if( (g!=G_OTHER) || (g0>G_FONT) ) stages.push_back( vector<int>() ); // (shar with a missing ttfcomp)
    stages.back().push_back( g );
  }
  return 0;
}

// the threads as resolved: "3+3,1+1"
static string ThreadText() {
  string t;
  for( size_t s=0; s<stages.size(); s++ ) for( size_t i=0; i<stages[s].size(); i++ )
    t += (t.empty() ? "" : i ? "+" : ",") + Num( th[stages[s][i]] );
  return t;
}

// "4,512k": part sizes in the same order; MB, or KB with k (GB with g); 0 or
// a missing one: the default
static int ParseChunks( const char* s ) {
  for( int g=0; g<N_GROUP; g++ ) ch[g] = 0;
  for( int g=0; ; g++ ) {
    if( (g>=N_GROUP) || (*s<'0') || (*s>'9') ) return 1;
    qword v = 0;
    while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>(1u<<30) ) return 1; }
    qword u = 1<<20;
    if( (*s=='k') || (*s=='K') ) { u = 1<<10; s++; }
    else if( (*s=='m') || (*s=='M') ) s++;
    else if( (*s=='g') || (*s=='G') ) { u = 1<<30; s++; }
    ch[g] = v*u;
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    s++;
  }
  return 0;
}

// "5,9,,20": levels of h264p, the audio packer, ttfcomp and shar, in that
// order ('+' as ','); missing or empty: the tool's default. h264p and aacpack
// take 1 to 9 (-N; which of them, their builds say: CheckLevels), shar zstd's
// -131072 to 22 (aN; 0 stores); ttfcomp, mp3pack and oggcomp take none, theirs
// is not used.
static int ParseLevels( const char* s ) {
  for( int g=0; g<N_GROUP; g++ ) lv[g] = NO_LEVEL;
  for( int g=0; ; g++ ) {
    if( g>=N_GROUP ) return 1;
    if( *s && (*s!=',') && (*s!='+') ) {
      int neg = (*s=='-');
      s += neg;
      if( (*s<'0') || (*s>'9') ) return 1;
      int v = 0;
      while( (*s>='0') && (*s<='9') ) { v = v*10+(*s++-'0'); if( v>1000000 ) return 1; }
      lv[g] = neg ? -v : v;
    }
    if( *s==0 ) break;
    if( (*s!='+') && (*s!=',') ) return 1;
    s++;
  }
  for( int g=G_VIDEO; g<=G_AUDIO; g++ ) if( (lv[g]!=NO_LEVEL) && ((lv[g]<1) || (lv[g]>9)) ) return 1;
  return (lv[G_OTHER]!=NO_LEVEL) && ((lv[G_OTHER]<-131072) || (lv[G_OTHER]>22));
}

// a packer's switch for its group's level: h264p's and aacpack's -N (none for
// their default), shar's aN (a22 by default); none for the others
static string LevelArg( int m ) {
  int l = lv[m_group[m]];
  if( m==M_SHAR ) return "a" + INum( (l==NO_LEVEL) ? 22 : l );
  if( (l==NO_LEVEL) || ((m!=M_H264P) && (m!=M_AACPACK)) ) return string();
  return "-" + Num(l);
}

// the same with a space before it, if there is one
static string LevelSp( int m ) { string a = LevelArg(m); return a.size() ? " "+a : a; }

// a part size for h264p's and mp3pack's -c: MB, or KB with a k
static string ChunkArg( qword c ) {
  c = std::min<qword>( std::max<qword>( c, 1<<10 ), 1u<<30 );
  return (c&((1<<20)-1)) ? Num(c>>10)+"k" : Num(c>>20);
}

static string StageText() {
  string t;
  for( size_t s=0; s<stages.size(); s++ ) {
    string u;
    for( size_t i=0; i<stages[s].size(); i++ ) {
      int g = stages[s][i], n = 0;
      for( size_t k=0; k<J.size(); k++ ) n += (J[k]->g==g);
      if( n ) u += string( u.size() ? " + " : "" ) + g_name[g] + " " + Num(th[g]);
    }
    if( u.size() ) t += (t.size() ? ", then " : "") + u;
  }
  return t;
}

static int RunStages() {
  t_run = GetTickCount64();
  for( size_t s=0; s<stages.size(); s++ ) {
    int r = RunStage( J, stages[s] );
    if( r ) return (r==2) ? 5 : 3;
  }
  return 0;
}

//---- c

// the frame length of an ADTS header, or 0
static uint AdtsLen( const byte* p ) {
  if( (p[0]!=0xFF) || ((p[1]&0xF6)!=0xF0) || (((p[2]>>2)&15)>12) ) return 0;
  uint n = ((p[3]&3)<<11) | (p[4]<<3) | (p[5]>>5);
  return (n>=((p[1]&1) ? 7u : 9u)) ? n : 0;
}

// The first ADTS frame within 64K from t, with 3 frames in a row (or up to the
// end) to be sure of it; t if there is none: aacpack keeps what is not AAC as
// it is, so a cut elsewhere only costs some compression.
static qword AdtsCut( HANDLE f, qword t, qword size ) {
  const uint WIN = 1<<16, B = WIN+3*8192+8;
  vector<byte> b( B );
  LARGE_INTEGER o; o.QuadPart = LONGLONG(t);
  DWORD n = 0;
  if( !SetFilePointerEx( f, o, 0, FILE_BEGIN ) || !ReadFile( f, &b[0], DWORD( std::min<qword>( B, size-t ) ), &n, 0 ) ) return t;
  for( uint i=0; (i<WIN) && (i+7<=n); i++ ) {
    uint p = i, k;
    for( k=0; (k<3) && (t+p<size) && (p+7<=n); k++ ) {
      uint l = AdtsLen( &b[p] );
      if( l==0 ) break;
      p += l;
    }
    if( (k==3) || ((k>0) && (t+p==size)) ) return t+i;
  }
  return t;
}

// the packer jobs for the streams in S
static int PackJobs() {
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    string u = Quote( "u" DS+s.name );
    int si = int(i);
    if( s.m==M_H264P ) {
      int n = th[G_VIDEO];
      string pk = s.name + ".h264p";
      Job* j = NewJob( G_VIDEO, n, "h264p " + s.name, "h264p" );
      j->args = "c" + LevelSp(M_H264P) + " -t " + Num(n) + (((n>1) && ch[G_VIDEO]) ? " -c "+ChunkArg(ch[G_VIDEO]) : string()) + (f_verbose ? "" : " -p")
              + " " + u + " " + Quote("p" DS+pk);
      j->prog = 1; j->work = s.size; j->sids.push_back( si ); j->outs.push_back( PPath(pk) );
      s.part.push_back( Part( 0, s.size, pk, "" ) );
    } else if( s.m==M_AACPACK ) {
      // with n threads: n parts (1MB at least), or parts of -c's size at least
      qword n = th[G_AUDIO];
      qword k = ch[G_AUDIO] ? s.size/ch[G_AUDIO] : std::min<qword>( n, s.size>>20 );
      k = (n>1) ? std::min<qword>( std::max<qword>( k, 1 ), 4096 ) : 1;
      vector<qword> cut( 1, 0 );
      if( k>1 ) {
        HANDLE f = OpenRead( UPath(s.name) );
        if( f==INVALID_HANDLE_VALUE ) { Msg( "cannot read " + U(UPath(s.name)) ); return 2; }
        for( qword q=1; q<k; q++ ) {
          qword p = AdtsCut( f, s.size*q/k, s.size );
          if( (p>cut.back()) && (p<s.size) ) cut.push_back( p );
        }
        CloseHandle( f );
      }
      cut.push_back( s.size );
      for( size_t q=0; q+1<cut.size(); q++ ) {
        string pk = s.name + ((cut.size()>2) ? "."+Num(q) : string()) + ".aacp";
        Job* j = NewJob( G_AUDIO, 1, "aacpack " + s.name + ((cut.size()>2) ? "#"+Num(q) : string()), "aacpack" );
        j->args = "c" + LevelSp(M_AACPACK) + " - " + Quote("p" DS+pk);
        j->in.path = UPath(s.name); j->in.off = cut[q]; j->in.len = cut[q+1]-cut[q];
        j->prog = 2; j->work = j->in.len; j->sids.push_back( si ); j->outs.push_back( PPath(pk) );
        s.part.push_back( Part( cut[q], cut[q+1]-cut[q], pk, "" ) );
      }
    } else if( s.m==M_MP3PACK ) {
      int n = th[G_AUDIO];
      string pk = s.name + ".mp3p", pk2 = s.name + ".mp3o";
      Job* j = NewJob( G_AUDIO, n, "mp3pack " + s.name, "mp3pack" );
      j->args = "c -t " + Num(n) + (((n>1) && ch[G_AUDIO]) ? " -c "+ChunkArg(ch[G_AUDIO]) : string()) + (f_verbose ? "" : " -p")
              + " " + u + " " + Quote("p" DS+pk) + " " + Quote("p" DS+pk2);
      j->prog = 1; j->work = s.size; j->sids.push_back( si ); j->outs.push_back( PPath(pk) ); j->outs.push_back( PPath(pk2) );
      s.part.push_back( Part( 0, s.size, pk, pk2 ) );
    } else if( s.m==M_OGGCOMP ) {
      string pk = s.name + ".oggc";
      Job* j = NewJob( G_AUDIO, 1, "oggcomp " + s.name, "oggcomp" );
      j->args = "c " + u + " " + Quote("p" DS+pk);
      j->work = s.size; j->sids.push_back( si ); j->outs.push_back( PPath(pk) );
      s.part.push_back( Part( 0, s.size, pk, "" ) );
    } else if( s.m==M_STORE ) {
      CreateDirectoryW( (work+L"p" DS "store").c_str(), 0 );
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"p" DS "store" DS+W(s.name)).c_str() ) ) { Msg( "cannot move " + U(UPath(s.name)) ); return 2; }
      s.part.push_back( Part( 0, s.size, "-", "" ) );
    }
  }
  // the fonts: one solid archive, whatever the threads (more only if their
  // names would not fit on one command line)
  Job* f = 0;
  int nf = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_TTFCOMP ) {
    if( f && (f->args.size()+S[i].name.size()>30000) ) f = 0;
    if( !f ) {
      string pk = nf++ ? "fonts" + Num(nf) + ".ttfc" : "fonts.ttfc";
      f = NewJob( G_FONT, 1, "ttfcomp", "ttfcomp" );
      f->cwd = "u" DS; f->args = "a " + Quote(".." DS "p" DS+pk); f->outs.push_back( PPath(pk) );
    }
    f->args += " " + Quote( S[i].name );
    f->work += S[i].size; f->sids.push_back( int(i) );
    f->label = "ttfcomp " + Num( f->sids.size() ) + ((f->sids.size()>1) ? " fonts" : " font");
    S[i].part.push_back( Part( 0, S[i].size, U(FileName(f->outs[0])), "" ) );
  }
  // the rest: one solid archive; with more threads, files over the part size
  // (-c, default 64MB) are cut into pieces, an archive for each
  int n = th[G_OTHER], na = 0;
  qword c = ch[G_OTHER] ? ch[G_OTHER] : (64<<20);
  Job* o = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_SHAR ) {
    Stream& s = S[i];
    if( (n>1) && (s.size>c) ) {
      qword k = std::min<qword>( (s.size+c-1)/c, 100000 );
      for( qword q=0; q<k; q++ ) {
        qword a = s.size*q/k, b = s.size*(q+1)/k;
        string d = "other" + Num(++na), pk = d + ".shar", mem = s.name + ".part" + Num(q);
        wstring wd = work + W(d) + L"" DS;
        CreateDirectoryW( wd.c_str(), 0 );
        HANDLE h = OpenWrite( wd+W(mem), CREATE_ALWAYS );
        int r = (h==INVALID_HANDLE_VALUE) || CopyRange( UPath(s.name), a, b-a, h );
        if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
        if( r ) return Fail( "cannot write " + U(wd+W(mem)), 2 );
        Job* j = NewJob( G_OTHER, 1, "shar " + s.name + "#" + Num(q), "shar" );
        j->args = LevelArg(M_SHAR) + " " + Quote("p" DS+pk) + " " + Quote(d+DS);
        j->prog = 1; j->work = b-a; j->outs.push_back( PPath(pk) ); j->del.push_back( wd );
        s.part.push_back( Part( a, b-a, pk, mem ) );
      }
      if( !f_keep ) Remove( UPath(s.name) );
    } else {
      if( !o ) {
        o = NewJob( G_OTHER, 1, "shar", "shar" );
        CreateDirectoryW( (work+L"other").c_str(), 0 );
        o->args = LevelArg(M_SHAR) + " p" DS "other.shar other" DS; o->prog = 1; o->outs.push_back( PPath("other.shar") ); o->del.push_back( work+L"other" DS );
      }
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"other" DS+W(s.name)).c_str() ) ) { Msg( "cannot move " + U(UPath(s.name)) ); return 2; }
      o->work += s.size; o->sids.push_back( int(i) );
      s.part.push_back( Part( 0, s.size, "other.shar", s.name ) );
    }
  }
  if( o ) o->label = "shar " + Num( o->sids.size() ) + ((o->sids.size()>1) ? " files" : " file");
  // the files the shar jobs pack are in their own directories now, not in u
  for( size_t i=0; i<J.size(); i++ ) if( J[i]->tool=="shar" ) J[i]->sids.clear();
  for( size_t i=0; i<J.size(); i++ ) for( size_t k=0; k<J[i]->sids.size(); k++ ) S[J[i]->sids[k]].refs++;
  return 0;
}

// streams whose packer failed: into one more shar archive (stored with -t ...+0)
static int PackFailed() {
  Job* x = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].failed ) {
    Stream& s = S[i];
    for( size_t k=0; k<s.part.size(); k++ ) {
      Remove( PPath(s.part[k].pk) );
      if( s.part[k].pk2.size() ) Remove( PPath(s.part[k].pk2) );
    }
    s.part.clear();
    if( th[G_OTHER]==0 ) {
      CreateDirectoryW( (work+L"p" DS "store").c_str(), 0 );
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"p" DS "store" DS+W(s.name)).c_str() ) ) { Msg( "cannot move " + U(UPath(s.name)) ); return 2; }
      s.m = M_STORE; s.part.push_back( Part( 0, s.size, "-", "" ) );
      continue;
    }
    if( !x ) {
      x = NewJob( G_OTHER, 1, "shar (fallback)", "shar" );
      CreateDirectoryW( (work+L"fallback").c_str(), 0 );
      x->args = LevelArg(M_SHAR) + " p" DS "fallback.shar fallback" DS; x->prog = 1; x->outs.push_back( PPath("fallback.shar") ); x->del.push_back( work+L"fallback" DS );
    }
    if( !MoveFileW( UPath(s.name).c_str(), (work+L"fallback" DS+W(s.name)).c_str() ) ) { Msg( "cannot move " + U(UPath(s.name)) ); return 2; }
    x->work += s.size;
    s.m = M_SHAR; s.part.push_back( Part( 0, s.size, "fallback.shar", s.name ) );
  }
  if( !x ) return 0;
  vector<int> g( 1, G_OTHER );
  int r = RunStage( J, g );
  return r ? ((r==2) ? 5 : 3) : 0;
}

// c: the levels -l gives h264p and aacpack, if their streams are here: their
// builds have to have them (a build without the level exits with 2 before it
// opens its input, else with 1 as the input is not there)
static int CheckLevels() {
  const int ms[2] = { M_H264P, M_AACPACK };
  for( int i=0; i<2; i++ ) {
    int m = ms[i], l = lv[m_group[m]], any = 0;
    for( size_t k=0; k<S.size(); k++ ) any |= (S[k].m==m);
    if( !any || (l==NO_LEVEL) || (Probe( m_tool[m], "c -" + Num(l) + " mp4pack.none mp4pack.none.out" )!=2) ) continue;
    string has;
    for( int k=1; k<=9; k++ ) if( Probe( m_tool[m], "c -" + Num(k) + " mp4pack.none mp4pack.none.out" )==1 ) has += " " + Num(k);
    Msg( "error: " + GetTool( m_tool[m] ).file + " has no level " + Num(l) + (has.size() ? " (its levels:" + has + ")" : string()) + "; see -l" );
    return 1;
  }
  return 0;
}

static int Compress( const wstring& in, const wstring& out ) {
  if( GetSize( LongPath(in), src_size ) ) { Msg( "cannot open " + U(in) ); return 2; }
  if( SameFile( LongPath(in), LongPath(out) ) ) { Msg( "the output is the input" ); return 1; }
  src_name = U( FileName(in) );
  Msg( "mp4pack c: " + U(in) + " (" + Num(src_size) + " bytes) -> " + U(out) );
  if( MakeWork() ) return 2;
  SpaceCheck( 2*src_size ); // the streams, and what they are packed into

  // avip c, with the input as it can take its name; meanwhile its CRC32
  wstring ip = ToolName( in );
  if( ip.empty() ) {
    wstring e = FileName( in );
    e = (e.rfind('.')!=wstring::npos) ? e.substr( e.rfind('.') ) : L"";
    ip = L"input" + (AnsiOK(e) && (e.size()<16) ? e : wstring());
    int linked = CreateHardLinkW( (work+ip).c_str(), LongPath(in).c_str(), 0 );
#ifndef _WIN32
    if( !linked ) linked = CreateSymbolicLinkW( (work+ip).c_str(), LongPath(in).c_str(), 0 ); // (another file system)
#endif
    if( !linked ) {
      Msg( "note: avip cannot take the input's name: copying it to the work directory" );
      if( !CopyFileW( LongPath(in).c_str(), (work+ip).c_str(), FALSE ) ) { Msg( "cannot copy the input to " + U(work+ip) ); return 2; }
    }
  }
  CrcTask ct; ct.path = LongPath(in); ct.err = 1;
  HANDLE cth = CreateThread( 0, 0, CrcThread, &ct, 0, 0 );
  if( !cth ) CrcThread( &ct );
  Job a; a.label = "avip c"; a.tool = "avip"; a.args = "c " + Quote(U(ip)) + " u" DS "s";
  int r = RunOne( a );
  if( cth ) { WaitForSingleObject( cth, INFINITE ); CloseHandle( cth ); }
  if( r ) { return (r==2) ? 5 : 3; }
  if( ct.err || (ct.size!=src_size) ) return Fail( "cannot read " + U(in), 2 );
  src_crc = ct.crc;
  if( FileName(ip)==ip ) Remove( work+ip ); // (our link or copy)

  vector<wstring> names = ListFiles( work+L"u" DS );
  string nobuild[N_METHOD];
  if( names.empty() ) { Msg( "avip c made no files" ); return 3; }
  Msg( "avip c: " + Num(names.size()) + ((names.size()==1) ? " file, " : " files, ") + Clock(a.t) );
  for( size_t i=0; i<names.size(); i++ ) {
    Stream s;
    s.name = U(names[i]);
    if( !SafeName(s.name) ) { Msg( "avip c made an unexpected file: " + s.name ); return 3; }
    if( FileCrc( UPath(s.name), s.crc, s.size ) ) return Fail( "cannot read " + U(UPath(s.name)), 2 );
    s.m = s.size ? Method(s.name) : M_STORE;
    if( (s.m>=M_H264P) && (th[m_group[s.m]]==0) ) s.m = M_SHAR;
    if( (s.m>=M_H264P) && GetTool( m_tool[s.m] ).path.empty() ) {
      string& n = nobuild[s.m];
      n += (n.size() ? ", " : "") + s.name;
      s.m = M_SHAR;
    }
    if( (s.m==M_SHAR) && (th[G_OTHER]==0) ) s.m = M_STORE;
    S.push_back( s );
  }
  for( int m=0; m<N_METHOD; m++ ) if( nobuild[m].size() ) Msg( "note: " + NoTool( GetTool( m_tool[m] ) ) + "; to shar " + LevelArg(M_SHAR) + ": " + nobuild[m] );
  if( (r = CheckLevels())!=0 ) return r;
  if( (r = PackJobs())!=0 ) return r;

  for( size_t i=0; i<S.size(); i++ ) {
    const Stream& s = S[i];
    string d = m_name[s.m];
    if( s.m==M_H264P ) d += LevelSp(M_H264P) + " -t " + Num(th[G_VIDEO]) + (((th[G_VIDEO]>1) && ch[G_VIDEO]) ? " -c "+ChunkArg(ch[G_VIDEO]) : string());
    if( s.m==M_MP3PACK ) d += " -t " + Num(th[G_AUDIO]) + (((th[G_AUDIO]>1) && ch[G_AUDIO]) ? " -c "+ChunkArg(ch[G_AUDIO]) : string());
    if( s.m==M_AACPACK ) d += LevelSp(M_AACPACK) + ((s.part.size()>1) ? ", " + Num(s.part.size()) + " parts" : string());
    if( s.m==M_SHAR ) d += LevelSp(M_SHAR) + " (" + s.part[0].pk + ((s.part.size()>1) ? ", " + Num(s.part.size()) + " pieces" : string()) + ")";
    string n = "  " + s.name;
    if( n.size()<20 ) n.append( 20-n.size(), ' ' );
    string z = Num(s.size);
    if( z.size()<12 ) z.insert( 0, 12-z.size(), ' ' );
    Msg( n + z + "  " + d );
  }
  if( J.size() ) Msg( "running " + StageText() );
  if( (r = RunStages())!=0 ) return r;
  if( (r = PackFailed())!=0 ) return r;

  // the index and all packed, stored as they are: the output
  if( WriteAll( PPath("mp4pack.idx"), IndexText() ) ) { Msg( "cannot write " + U(PPath("mp4pack.idx")) ); return 2; }
  Job f; f.label = "shar a0"; f.tool = "shar"; f.args = "a0 - p" DS;
  f.out.path = LongPath(out); f.out.create = 1; f.out.len = ALL; f.prog = 3;
  f.work = DirSize( work+L"p" DS );
  r = RunOne( f );
  if( r ) { Remove( LongPath(out) ); return (r==2) ? 5 : 3; }
  qword osz = f.out.done;
  Msg( Num(src_size) + " -> " + Num(osz) + " (" + Pct( double(osz), double(src_size) ) + "), " + Clock( GetTickCount64()-t_start ) );
  return 0;
}

//---- d

static int Decompress( const wstring& in, const wstring& out ) {
  qword isz;
  if( GetSize( LongPath(in), isz ) ) { Msg( "cannot open " + U(in) ); return 2; }
  if( SameFile( LongPath(in), LongPath(out) ) ) { Msg( "the output is the input" ); return 1; }
  Msg( "mp4pack d: " + U(in) + " -> " + U(out) );
  if( MakeWork() ) return 2;

  Job x; x.label = "shar x"; x.tool = "shar"; x.args = "x - p" DS;
  x.in.path = LongPath(in); x.in.len = isz; x.prog = 2; x.work = isz;
  int r = RunOne( x );
  if( r ) { return (r==2) ? 5 : 3; }
  string why;
  if( ReadIndex( why ) ) { Msg( U(in) + ": " + why ); return 4; }
  Msg( "restoring " + src_name + " (" + Num(src_size) + " bytes) from " + Num(S.size()) + ((S.size()==1) ? " file" : " files") );
  if( f_verbose && src_tools.size() ) Msg( "packed with " + src_tools );
  SpaceCheck( src_size );
  for( size_t i=0; i<S.size(); i++ ) if( (S[i].m!=M_STORE) && GetTool( m_tool[S[i].m] ).path.empty() ) {
    Msg( NoTool( GetTool( m_tool[S[i].m] ) ) ); return 2;
  }

  // the unpackers
  vector<string> ttf, sha; // archives, in their order
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    string u = Quote( "u" DS+s.name );
    const Part& p0 = s.part[0];
    if( s.m==M_H264P ) {
      Job* j = NewJob( G_VIDEO, th[G_VIDEO], "h264p " + s.name, "h264p" );
      j->args = string("d") + (f_verbose ? "" : " -p") + " " + Quote("p" DS+p0.pk) + " " + u;
      j->prog = 1; j->work = s.size; j->outs.push_back( UPath(s.name) ); j->del.push_back( PPath(p0.pk) );
    } else if( s.m==M_AACPACK ) {
      if( MakeFile( UPath(s.name), s.size ) ) { Msg( "cannot write " + U(UPath(s.name)) ); return 2; }
      for( size_t k=0; k<s.part.size(); k++ ) {
        const Part& p = s.part[k];
        Job* j = NewJob( G_AUDIO, 1, "aacpack " + s.name + ((s.part.size()>1) ? "#"+Num(k) : string()), "aacpack" );
        j->args = "d " + Quote("p" DS+p.pk) + " -";
        j->out.path = UPath(s.name); j->out.off = p.off; j->out.len = p.len;
        j->prog = 3; j->work = p.len; j->del.push_back( PPath(p.pk) );
      }
    } else if( s.m==M_MP3PACK ) {
      Job* j = NewJob( G_AUDIO, th[G_AUDIO], "mp3pack " + s.name, "mp3pack" );
      j->args = string("d") + (f_verbose ? "" : " -p") + " " + Quote("p" DS+p0.pk) + " " + Quote("p" DS+p0.pk2) + " " + u;
      j->prog = 1; j->work = s.size; j->outs.push_back( UPath(s.name) ); j->del.push_back( PPath(p0.pk) ); j->del.push_back( PPath(p0.pk2) );
    } else if( s.m==M_OGGCOMP ) {
      Job* j = NewJob( G_AUDIO, 1, "oggcomp " + s.name, "oggcomp" );
      j->args = "d " + Quote("p" DS+p0.pk) + " " + u;
      j->work = s.size; j->outs.push_back( UPath(s.name) ); j->del.push_back( PPath(p0.pk) );
    } else if( s.m==M_STORE ) {
      if( !MoveFileW( (work+L"p" DS "store" DS+W(s.name)).c_str(), UPath(s.name).c_str() ) ) { Msg( "missing from the archive: " + s.name ); return 4; }
    } else {
      vector<string>& v = (s.m==M_TTFCOMP) ? ttf : sha;
      for( size_t k=0; k<s.part.size(); k++ ) if( std::find( v.begin(), v.end(), s.part[k].pk )==v.end() ) v.push_back( s.part[k].pk );
    }
  }
  for( size_t a=0; a<ttf.size(); a++ ) { // the fonts under their names, in the archive's order
    Job* j = NewJob( G_FONT, 1, "ttfcomp " + ttf[a], "ttfcomp" );
    j->cwd = "u" DS; j->args = "x " + Quote(".." DS "p" DS+ttf[a]); j->del.push_back( PPath(ttf[a]) );
    for( size_t i=0; i<S.size(); i++ ) if( (S[i].m==M_TTFCOMP) && (S[i].part[0].pk==ttf[a]) ) {
      j->args += " " + Quote(S[i].name); j->work += S[i].size; j->outs.push_back( UPath(S[i].name) );
    }
  }
  for( size_t a=0; a<sha.size(); a++ ) {
    Job* j = NewJob( G_OTHER, 1, "shar " + sha[a], "shar" );
    j->args = "x " + Quote("p" DS+sha[a]) + " u" DS; j->prog = 1; j->del.push_back( PPath(sha[a]) );
    for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_SHAR ) for( size_t k=0; k<S[i].part.size(); k++ ) if( S[i].part[k].pk==sha[a] ) {
      j->work += S[i].part[k].len; j->outs.push_back( UPath(S[i].part[k].pk2) );
    }
  }
  if( J.size() ) Msg( "running " + StageText() );
  if( (r = RunStages())!=0 ) return r;

  // shar pieces together; then every stream as it was
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    if( (s.m!=M_SHAR) || ((s.part.size()==1) && (s.part[0].pk2==s.name)) ) continue;
    HANDLE h = OpenWrite( UPath(s.name), CREATE_ALWAYS );
    r = (h==INVALID_HANDLE_VALUE);
    for( size_t k=0; (k<s.part.size()) && !r; k++ ) {
      r = CopyRange( UPath(s.part[k].pk2), 0, s.part[k].len, h );
      if( !f_keep ) Remove( UPath(s.part[k].pk2) );
    }
    if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
    if( r ) return Fail( "cannot put " + s.name + " together", 2 );
  }
  for( size_t i=0; i<S.size(); i++ ) {
    uint c; qword z;
    if( FileCrc( UPath(S[i].name), c, z ) ) return Fail( "missing: " + S[i].name, 4 );
    if( (z!=S[i].size) || (c!=S[i].crc) ) {
      Msg( "error: " + S[i].name + " (" + m_name[S[i].m] + ") is not as it was: " + Num(z) + " bytes, CRC " + Hex(c)
         + " instead of " + Num(S[i].size) + ", " + Hex(S[i].crc) );
      return 4;
    }
  }

  // avip d, to the output if it can take its name (or its 8.3 one), else in
  // the work directory, moved after
  wstring op = out;
  int tmp = 0;
  if( !AnsiOK(out) ) {
    HANDLE h = OpenWrite( LongPath(out), CREATE_ALWAYS );
    if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
    op = ToolName( out );
    if( op.empty() ) { op = L"output.tmp"; tmp = 1; }
  }
  Job a; a.label = "avip d"; a.tool = "avip"; a.args = "d u" DS "s " + Quote(U(op));
  r = RunOne( a );
  if( r ) { Remove( LongPath(out) ); return (r==2) ? 5 : 3; }
  if( tmp && !MoveFileExW( (work+op).c_str(), LongPath(out).c_str(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED ) ) {
    Msg( "cannot move the output to " + U(out) ); return 2;
  }
  uint c; qword z;
  if( FileCrc( LongPath(out), c, z ) ) return Fail( "cannot read " + U(out), 2 );
  if( (z!=src_size) || (c!=src_crc) ) {
    Msg( "error: the output is not as the input was: " + Num(z) + " bytes, CRC " + Hex(c) + " instead of " + Num(src_size) + ", " + Hex(src_crc) );
    if( !f_keep ) Remove( LongPath(out) );
    return 4;
  }
  Msg( Num(isz) + " -> " + Num(z) + ", CRC " + Hex(c) + " OK, " + Clock( GetTickCount64()-t_start ) );
  return 0;
}

//---- main

static BOOL WINAPI CtrlHandler( DWORD t ) {
  f_abort = 1;
  SetEvent( ev_abort );
  // closing the console ends the process when this returns: let main clean up first
  if( (t==CTRL_CLOSE_EVENT) || (t==CTRL_LOGOFF_EVENT) || (t==CTRL_SHUTDOWN_EVENT) ) WaitForSingleObject( ev_done, 4500 );
  return TRUE;
}

static const char usage[] =
"mp4pack - lossless compression of mp4/mkv/avi files with avip, h264p, aacpack,\n"
"mp3pack, oggcomp, ttfcomp and shar, from mp4pack's own directory: the best\n"
"build of each for this CPU, <tool>_<target>" EXE " by -march target names\n"
"(h264p_haswell" EXE ", h264p_x86-64-v3" EXE ", h264p_znver4" EXE ") or <tool>" EXE "\n"
"usage: mp4pack c [options] input output   compress\n"
"       mp4pack d [options] input output   decompress\n"
"options:\n"
"  -t T  threads of h264p, the audio packer, ttfcomp and shar (the other\n"
"        files), in that order; joined by + they run at once, by , one after\n"
"        another: -t 3+3+1+1 (all at once), -t 3,3,1,1 (in turn), -t 3+3,1,1.\n"
"        Missing ones: audio as video, after it; ttfcomp and shar 1, both\n"
"        at once, after the rest (-t 4 = 4,4,1+1; -t 3+3 = 3+3,1+1). N% is\n"
"        that share of the threads there are (all, or the free ones: -f),\n"
"        rounded up; a -N at the end leaves N of them out first:\n"
"        -t 50%+50%-2 = 3+3,1+1 with 8 threads. Alone, -N works on the\n"
"        default, 100%,100% (= N,N,1+1: all the threads for h264p, then for\n"
"        the audio).\n"
"        0: no packer for that group, its files go to shar (for shar\n"
"        itself: they are stored as they are)\n"
"  -c C  part sizes, in the same order: MB, or KB with k (e.g. -c 4,512k);\n"
"        0 or missing: the default. h264p and mp3pack get it as their -c;\n"
"        aacpack's input is cut at ADTS frames into parts of at least that\n"
"        (default: one part per thread, 1MB at least); with more than one\n"
"        thread, other files bigger than it (default 64MB) are cut into\n"
"        pieces, an archive for each. The fonts make one solid archive.\n"
"  -l L  compression levels, in the same order, joined by , (or +); missing\n"
"        or empty: the tool's default. h264p and aacpack: 1 to 9, as their\n"
"        builds have them (now h264p 9 7 5 3 1, aacpack 9; 9 is the strongest\n"
"        and the default); shar: zstd's, -131072 to 22 (default 22; 0 stores).\n"
"        ttfcomp, mp3pack and oggcomp have none. E.g. -l 5,9,,19\n"
"  -f    count the threads free now (the CPUs' idle share over a second)\n"
"  -i T  use only tool builds whose code runs on the -march target T as well\n"
"        (haswell, x86-64-v3 ...; default: any that runs on this CPU)\n"
#ifdef _WIN32
"  -w D  make the work directory in D (default: %TEMP%)\n"
#else
"  -w D  make the work directory in D (default: $TMPDIR, or /tmp)\n"
#endif
"  -k    keep the work directory\n"
"  -v    show the tools' command lines and output\n";

int wmain( int argc, wchar_t** argv ) {
  h_err = GetStdHandle( STD_ERROR_HANDLE );
  DWORD cm;
  f_con = GetConsoleMode( h_err, &cm ) ? 1 : 0;
  // no "has stopped working" dialogs from a tool that crashes (inherited)
  SetErrorMode( SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX );
  CrcInit();
  {
    vector<wchar_t> b( 32768 );
    DWORD n = GetModuleFileNameW( 0, &b[0], DWORD(b.size()) );
    exe_dir.assign( &b[0], n );
    exe_dir.erase( exe_dir.size()-FileName( exe_dir ).size() );
  }

  int mode = 0, nio = 0, f_t = 0, f_c = 0, bits;
  wstring io[2];
  string ts, cs, ls;
  for( int i=1; i<argc; i++ ) {
    wstring a = argv[i];
    if( !mode ) {
      if( (a==L"c") || (a==L"d") ) { mode = a[0]; continue; }
      break;
    }
    if( (a.size()>=2) && (a[0]=='-') ) {
      wchar_t o = a[1];
      if( ((o=='t') || (o=='c') || (o=='l') || (o=='w') || (o=='i')) ) {
        wstring v = a.substr( 2 );
        if( v.empty() ) { if( ++i>=argc ) { mode = 0; break; } v = argv[i]; }
        if( o=='t' ) { ts = U(v); f_t = 1; } else if( o=='c' ) { cs = U(v); f_c = 1; }
        else if( o=='l' ) { ls = U(v); if( ParseLevels( ls.c_str() ) ) { Msg( "bad -l " + ls + " (up to 4 levels joined by ,: h264p and aacpack 1 to 9, shar -131072 to 22)" ); return 1; } }
        else if( o=='w' ) w_base = v;
        else if( (cap = BuildOf( U(v), bits ))<0 ) { Msg( "bad -i " + U(v) + " (a -march target: haswell, x86-64-v3, znver4 ...)" ); return 1; }
      } else if( a==L"-v" ) f_verbose = 1;
      else if( a==L"-f" ) f_free = 1;
      else if( a==L"-k" ) f_keep = 1;
      else { Msg( "unknown option " + U(a) ); return 1; }
      continue;
    }
    if( nio>=2 ) { mode = 0; break; }
    io[nio++] = a;
  }
  if( !mode || (nio!=2) ) { Put( usage ); return 1; }
  f_dec = (mode=='d');

  // the CPU: its threads (or the free ones), and the tool builds it runs
  CpuThreads();
  CpuTargets();
  string c = "cpu: " + Num(cpu_threads) + " threads" + (cpu_cores ? ", " + Num(cpu_cores) + " cores" : string());
  if( !cpu_ok ) c += ", no CPUID (builds without a target only)";
  else if( cpu_best.size() ) c += ", " + cpu_best + (cpu.level ? " (x86-64-v" + Num(cpu.level) + ")" : string());
  if( cap>=0 ) c += ", builds for " + string( x86::x86_target[cap].name );
  int n = cpu_threads;
  if( f_free ) { n = FreeThreads(); c += ", " + Num(n) + " free"; }
  if( ParseThreads( f_t ? ts : string(), n ) ) { Msg( "bad -t " + ts + " (up to 4 of N or N% joined by + or , and maybe a -N)" ); return 1; }
  if( f_c && ParseChunks( cs.c_str() ) ) { Msg( "bad -c " + cs ); return 1; }
  Msg( c + "; -t " + (f_t ? ts+" = " : string()) + ThreadText() );
  const string all[] = { "avip", "shar", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp" };
  string tl;
  for( int i=0; i<7; i++ ) { const Tool& t = GetTool( all[i] ); if( t.path.size() ) tl += " " + t.file; }
  Msg( "tools:" + (tl.size() ? tl : string(" none")) );
  for( int i=0; i<2; i++ ) if( GetTool( all[i] ).path.empty() ) { Msg( NoTool( GetTool( all[i] ) ) ); return 2; }

  ev_abort = CreateEventW( 0, TRUE, FALSE, 0 );
  ev_done = CreateEventW( 0, TRUE, FALSE, 0 );
  SetConsoleCtrlHandler( CtrlHandler, TRUE );
  // the tools die with us, however we end
  h_jobobj = CreateJobObjectW( 0, 0 );
  if( h_jobobj ) {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
    memset( &li, 0, sizeof(li) );
    li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if( !SetInformationJobObject( h_jobobj, JobObjectExtendedLimitInformation, &li, sizeof(li) ) ) { CloseHandle( h_jobobj ); h_jobobj = 0; }
  }
  t_start = GetTickCount64();

  wstring in = FullPath( io[0] ), out = FullPath( io[1] );
  int r = f_dec ? Decompress( in, out ) : Compress( in, out );
  if( f_abort ) { Msg( "aborted" ); r = 5; }
  if( work.size() ) {
    if( f_keep ) Msg( "work directory kept: " + U(work) );
    else Remove( work );
  }
  SetEvent( ev_done );
  return r;
}
