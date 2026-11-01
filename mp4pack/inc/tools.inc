// inc/tools.inc -- a tool's process (Spawn), as a job (Start, Finish): its
//  stdin and stdout pumped from and to files, its log
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

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
  DWORD sys;              // err 1: the system's error (0: none, the file ended)
  Pump() : off(0), len(0), dir(0), create(0), pipe(0), file(0), th(0), done(0), err(0), sys(0) {}
};

static DWORD WINAPI PumpThread( void* arg ) {
  Pump& p = *(Pump*)arg;
  IOBuf b( 1<<20 );
  byte tmp[4096];
  byte* buf = b.p ? b.p : tmp;
  DWORD bn = b.p ? b.n : DWORD(sizeof(tmp));
  LARGE_INTEGER o; o.QuadPart = LONGLONG(p.off);
  if( !SetFilePointerEx( p.file, o, 0, FILE_BEGIN ) ) { p.sys = GetLastError(); p.err = 1; }
  if( p.dir==0 ) {
    while( !p.err && (p.done<p.len) ) {
      DWORD n = DWORD( std::min<qword>( bn, p.len-p.done ) ), k = 0;
      BOOL ok = ReadFile( p.file, buf, n, &k, 0 );
      if( !ok || (k==0) ) { p.sys = ok ? 0 : GetLastError(); p.err = 1; break; }
      for( DWORD o=0; o<k; ) { // in 64K pieces: done follows what the tool has taken (the progress)
        DWORD m = std::min<DWORD>( k-o, 1<<16 ), w = 0;
        if( !WriteFile( p.pipe, buf+o, m, &w, 0 ) || (w!=m) ) { p.err = 2; break; } // it closed its stdin
        o += m; p.done += m;
      }
    }
  } else {
    while(1) { // until it closes its stdout; after an error, only drained so that it can finish
      DWORD k = 0;
      if( !ReadFile( p.pipe, buf, bn, &k, 0 ) ) break;
      if( p.err || (k==0) ) continue;
      if( k>p.len-p.done ) { p.err = 3; continue; }
      if( WriteN( p.file, buf, k ) ) { p.sys = GetLastError(); p.err = 1; continue; }
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
  p.done = 0; p.err = 0; p.sys = 0; p.th = 0;
  p.file = p.dir ? OpenWrite( p.path, p.create ? CREATE_ALWAYS : OPEN_EXISTING ) : OpenRead( p.path );
  if( p.file==INVALID_HANDLE_VALUE ) { p.sys = GetLastError(); p.file = 0; return 1; }
  if( !CreatePipe( &r, &w, 0, 1<<16 ) ) { p.sys = GetLastError(); CloseHandle( p.file ); p.file = 0; return 1; }
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
  int prog;               // its progress: 0 -, 1 the last "NN%" in its log, 2 its stdin, 3 its stdout,
                          // 4 the size of its outs against expect
  int report;             // a line when it is done
  int quiet;              // its output to its log even with -v, its command line not shown
  qword work;             // the bytes it packs (c) or unpacks (d), for the progress
  qword expect;           // prog 4: the bytes it makes (d: exact; c: by a typical ratio)
  qword made;             // the packed bytes: c, those of its outs when it is done; d, those it unpacks
  vector<int> sids;       // c: the streams it packs
  vector<wstring> outs;   // files it has to make (c: their sizes are reported)
  vector<wstring> del;    // removed once it is done (unless -k)
  Pump in, out;           // in.path/out.path empty: none
  int st;                 // 0 waiting, 1 running, 2 done, 3 failed, 4 skipped, 5 stopped (by us: Ctrl+C, or another failed)
  int wu;                 // threads it took
  HANDLE hp;
  int exited;             // hp is signaled
  wstring log;
  DWORD code;
  qword t0, t;
  double frac;
  string why;
  Job() : g(G_OTHER), w(1), prog(0), report(1), quiet(0), work(0), expect(0), made(0), st(0), wu(0), hp(0), exited(0), code(0), t0(0), t(0), frac(0) { out.dir = 1; }
};

static HANDLE Inheritable( HANDLE h ) {
  HANDLE d = 0;
  if( h && (h!=INVALID_HANDLE_VALUE) && !DuplicateHandle( GetCurrentProcess(), h, GetCurrentProcess(), &d, 0, TRUE, DUPLICATE_SAME_ACCESS ) ) d = 0;
  return d;
}

// a tool's process: exe with these arguments (its command line: "exe"
// args) in the directory cwd, with these handles as its stdin, stdout and
// stderr (inheritable: none else is meanwhile), in our job object (it dies
// with us: suspended until it is in); flags: more of CreateProcess's. Its
// handle, or 0 (why)
static HANDLE Spawn( const wstring& exe, const string& args, const wstring& cwd, HANDLE in, HANDLE out, HANDLE err, DWORD flags, string& why ) {
  wstring cmd = L"\"" + exe + L"\" " + W(args);
  vector<wchar_t> cl( cmd.begin(), cmd.end() ); cl.push_back( 0 );
  STARTUPINFOW si; memset( &si, 0, sizeof(si) );
  si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = in; si.hStdOutput = out; si.hStdError = err;
  PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(pi) );
  if( !CreateProcessW( exe.c_str(), &cl[0], 0, 0, TRUE, CREATE_SUSPENDED|flags|tool_flags, 0, cwd.c_str(), &si, &pi ) ) {
    DWORD e = GetLastError();
    why = "cannot run " + U(exe) + SysErr(e) + " (error " + Num(e) + ")";
    return 0;
  }
  if( h_jobobj ) AssignProcessToJobObject( h_jobobj, pi.hProcess );
  ResumeThread( pi.hThread );
  CloseHandle( pi.hThread );
  return pi.hProcess;
}

// Only this thread starts processes, and the tool's ends of its pipes and log
// are inheritable just for its own CreateProcess, so no tool holds another's
// pipe open (which would keep that one from seeing EOF).
static int Start( Job& j ) {
  Tool& t = (Tool&)GetTool( j.tool );
  if( t.path.empty() ) { j.why = NoTool(t); j.st = 3; return 1; }
  t.used = 1;
  wstring exe = t.path;
  wstring cwd = work + W(j.cwd);
  if( f_verbose && !j.quiet ) Msg( "> " + (j.cwd.size() ? "("+j.cwd+") " : string()) + t.file + " " + j.args
                     + (j.in.path.size() ? " < "+U(j.in.path) : string()) + (j.out.path.size() ? " > "+U(j.out.path) : string()) );
  SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
  HANDLE c_in = 0, c_out = 0, c_err = 0;
  string err;
  if( j.in.path.size() ) { if( PumpOpen( j.in, c_in ) ) err = "cannot read " + U(j.in.path) + SysErr(j.in.sys); }
  else {
    c_in = CreateFileW( L"NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, 0 );
    if( c_in==INVALID_HANDLE_VALUE ) c_in = 0;
  }
  if( err.empty() && j.out.path.size() && PumpOpen( j.out, c_out ) ) err = "cannot write " + U(j.out.path) + SysErr(j.out.sys);
  if( err.empty() ) {
    if( f_verbose && !j.quiet ) c_err = Inheritable( h_err );
    else {
      j.log = work + L"log" DS + W(Num(++n_log)) + L"_" + W(j.tool) + L".txt";
      c_err = CreateFileW( j.log.c_str(), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, &sa, CREATE_ALWAYS, 0, 0 );
      if( c_err==INVALID_HANDLE_VALUE ) c_err = 0;
    }
  }
  HANDLE hp = 0;
  if( err.empty() ) hp = Spawn( exe, j.args, cwd, c_in, c_out ? c_out : c_err, c_err, 0, err ); // (stdout to the log too: some tools write their progress there)
  if( c_in ) CloseHandle( c_in );
  if( c_out ) CloseHandle( c_out );
  if( c_err ) CloseHandle( c_err );
  if( err.size() ) { PumpClose( j.in ); PumpClose( j.out ); j.why = err; j.st = 3; return 1; }
  j.hp = hp; j.exited = 0; j.st = 1; j.t0 = GetTickCount64(); j.frac = 0;
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
  else if( j.in.path.size() && (j.in.err || (j.in.done!=j.in.len)) ) why = "cannot read " + U(j.in.path) + SysErr( j.in.err ? j.in.sys : 0 );
  else if( j.out.path.size() && (j.out.err==3) ) why = "it wrote more than expected";
  else if( j.out.path.size() && j.out.err ) why = "cannot write " + U(j.out.path) + SysErr(j.out.sys);
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

