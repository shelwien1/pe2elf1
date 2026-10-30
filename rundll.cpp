// rundll - a tiny user-mode loader for EXEs converted to DLLs by exe2dll.
//
//   rundll <dll>,<export> [args...]     (rundll32-style, one token)
//   rundll <dll> <export> [args...]     (space-separated)
//
// It uses the ordinary Windows loader - LoadLibrary + GetProcAddress - to map
// the converted DLL and then calls the exported entry (default name "run",
// which exe2dll points at the program's original C-runtime startup). Before
// calling it, rundll rewrites the process command line in place so the
// program's CRT parses argv exactly as if you had run the original EXE as
//     <dll>  [args...]
// i.e. argv[0] is the module path and argv[1..] are your args, verbatim.
//
// There is no manual mapping, no reflective loading, no runtime code patching
// and no debugger: just LoadLibrary on a valid DLL. It runs in your own user
// context and needs no elevation (unlike launching some things through the
// system rundll32, this is just a normal console program).
//
// The embedded program runs to completion and the CRT ends the process the
// same way the EXE would (main returns -> exit -> ExitProcess), so code after
// the call normally does not run. The process exit code is the tool's own.
//
// Build (MinGW-w64):  x86_64-w64-mingw32-g++ -O2 -o rundll.exe rundll.cpp
// Build (MSVC):       cl /O2 /EHsc rundll.cpp

#include <windows.h>
#include <winternl.h>     // NtCurrentTeb(), PEB, RTL_USER_PROCESS_PARAMETERS
#include <stdio.h>
#include <string>

// GCC/MinGW emits a spurious -Warray-bounds for the gs-relative read inside
// the inlined NtCurrentTeb() at -O2. The access is correct; silence the noise.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

// ---- minimal command-line tokeniser (locates token boundaries in place) ----
static char* skipspace( char* p ) { while( *p==' ' || *p=='\t' ) p++; return p; }
static char* skiptoken( char* p ) {              // respects "double quotes"
  int q = 0;
  while( *p ) {
    if( *p=='"' ) { q = !q; p++; continue; }
    if( !q && (*p==' ' || *p=='\t') ) break;
    p++;
  }
  return p;
}
static void stripquotes( std::string& s ) {
  std::string r; r.reserve( s.size() );
  for( size_t i=0; i<s.size(); i++ ) if( s[i] != '"' ) r += s[i];
  s.swap( r );
}
static size_t comma_outside_quotes( const std::string& s ) {
  int q = 0;
  for( size_t i=0; i<s.size(); i++ ) {
    if( s[i]=='"' ) q = !q;
    else if( s[i]==',' && !q ) return i;
  }
  return std::string::npos;
}
static std::wstring towide( const std::string& s ) {
  if( s.empty() ) return std::wstring();
  int n = MultiByteToWideChar( CP_ACP, 0, s.c_str(), (int)s.size(), NULL, 0 );
  std::wstring w( n, 0 );
  MultiByteToWideChar( CP_ACP, 0, s.c_str(), (int)s.size(), &w[0], n );
  return w;
}

static void perr( const char* what, const char* arg ) {
  DWORD e = GetLastError();
  char* msg = NULL;
  FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, e, 0, (LPSTR)&msg, 0, NULL );
  fprintf( stderr, "rundll: %s '%s' failed (error %lu: %s)\n",
           what, arg?arg:"", (unsigned long)e, msg?msg:"?" );
  if( msg ) LocalFree( msg );
}

// Rewrite the process command line that the loaded program's CRT will read.
// We only ever write a string SHORTER than the original (we drop the
// "rundll.exe " prefix and the ",export" selector), so it always fits in the
// existing buffers - we never grow them. Returns false if, unexpectedly, it
// would not fit (checked, never overflowed).
static bool set_commandline( const std::string& nl ) {
  // ANSI command line (what a narrow-CRT `main` program parses).
  char* ca = GetCommandLineA();
  size_t calen = ca ? strlen(ca) : 0;
  if( nl.size() > calen ) {
    fprintf( stderr, "rundll: internal: patched command line too long (%zu > %zu)\n", nl.size(), calen );
    return false;
  }
  memcpy( ca, nl.c_str(), nl.size()+1 );

  // Wide command line (what a `wmain` program, or PEB readers, parse).
  std::wstring wl = towide( nl );
  wchar_t* cw = GetCommandLineW();
  size_t cwlen = cw ? wcslen(cw) : 0;
  if( cw && wl.size() <= cwlen )
    wmemcpy( cw, wl.c_str(), wl.size()+1 );

  // Keep the PEB's UNICODE_STRING consistent (some runtimes read it directly).
  PEB* peb = (PEB*)NtCurrentTeb()->ProcessEnvironmentBlock;
  if( peb && peb->ProcessParameters ) {
    UNICODE_STRING* cl = &peb->ProcessParameters->CommandLine;
    if( cl->Buffer && cl->Buffer != cw && (wl.size()+1)*sizeof(wchar_t) <= cl->MaximumLength )
      wmemcpy( cl->Buffer, wl.c_str(), wl.size()+1 );
    cl->Length = (USHORT)( wl.size() * sizeof(wchar_t) );
  }
  return true;
}

static int usage( const char* a0 ) {
  fprintf( stderr,
    "rundll - run an exe2dll-converted DLL and pass it a command line.\n"
    "usage: %s <dll>,<export> [args...]\n"
    "       %s <dll> <export> [args...]\n"
    "example: %s ttfcomp.dll,run input.ttf output.bin\n",
    a0, a0, a0 );
  return 2;
}

int main() {
  // Parse our OWN raw command line so we can forward the tail verbatim
  // (preserving quoting/spacing) rather than re-joining a split argv.
  char* orig = GetCommandLineA();

  char* p  = skipspace( orig );
  char* a0 = p;  p = skiptoken( p );              // argv[0] = this exe
  std::string self( a0, p-a0 ); stripquotes( self );
  if( self.empty() ) self = "rundll";

  p = skipspace( p );
  if( !*p ) return usage( self.c_str() );

  char* specStart = p;  p = skiptoken( p );       // "<dll>,<export>" or "<dll>"
  char* specEnd   = p;
  std::string spec( specStart, specEnd-specStart );

  std::string dll, exp;
  char* remain;
  size_t comma = comma_outside_quotes( spec );
  if( comma != std::string::npos ) {
    dll = spec.substr( 0, comma );
    exp = spec.substr( comma+1 );
    remain = specEnd;
  } else {
    dll = spec;
    char* q = skipspace( specEnd );
    if( !*q ) { fprintf( stderr, "rundll: missing export name\n" ); return usage( self.c_str() ); }
    char* exStart = q;  q = skiptoken( q );
    exp = std::string( exStart, q-exStart );
    remain = q;
  }
  stripquotes( dll );
  stripquotes( exp );
  if( dll.empty() || exp.empty() ) return usage( self.c_str() );

  remain = skipspace( remain );
  std::string args( remain );                     // verbatim tail (may be empty)

  // Load the converted DLL through the normal loader (applies relocations,
  // binds imports, runs the DllMain stub which just returns TRUE).
  HMODULE h = LoadLibraryA( dll.c_str() );
  if( !h ) { perr( "LoadLibrary", dll.c_str() ); return 1; }

  FARPROC proc = GetProcAddress( h, exp.c_str() );
  if( !proc ) { perr( "GetProcAddress", exp.c_str() ); return 1; }

  // Command line the embedded program should see:  "<dll>" <args...>
  // -> argv[0] = dll path, argv[1..] = args, exactly like running the EXE.
  std::string nl = "\"" + dll + "\"";
  if( !args.empty() ) nl += " " + args;
  if( !set_commandline( nl ) ) return 1;

  // Call the exported entry. For an exe2dll-converted tool this is the
  // program's original CRT startup: it initialises the CRT, runs main(),
  // and the CRT ends the process itself - so this call normally never returns.
  ((void (*)(void))proc)();

  return 0;
}
