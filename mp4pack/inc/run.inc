// inc/run.inc -- the run: what it needs once (Init), each file (FileBegin,
//  Verify, DecompressArchive), the files of a list (RunFiles), the CPU and
//  the tools (Setup), all of it (RunList)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- the run: c or d of each file

// what runs need once: stderr (a console?), the CRC table, our directory
// (the tools'), the abort event, and the job object the tools die with
static void Init() {
  h_err = GetStdHandle( STD_ERROR_HANDLE );
  DWORD cm;
  f_con = GetConsoleMode( h_err, &cm ) ? 1 : 0;
  // no "has stopped working" dialogs from a tool that crashes (inherited)
  SetErrorMode( SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX );
  CrcInit();
  vector<wchar_t> b( 32768 );
  DWORD n = GetModuleFileNameW( 0, &b[0], DWORD(b.size()) );
  exe_dir.assign( &b[0], n );
  exe_dir.erase( exe_dir.size()-FileName( exe_dir ).size() );
  ev_abort = CreateEventW( 0, TRUE, FALSE, 0 );
  ev_wake = CreateEventW( 0, FALSE, FALSE, 0 );
  // the tools die with us, however we end
  h_jobobj = CreateJobObjectW( 0, 0 );
  if( h_jobobj ) {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
    memset( &li, 0, sizeof(li) );
    li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if( !SetInformationJobObject( h_jobobj, JobObjectExtendedLimitInformation, &li, sizeof(li) ) ) { CloseHandle( h_jobobj ); h_jobobj = 0; }
  }
}

// a run's state as before it (mp4gui makes one run after another); not
// f_abort, which the caller clears
static void NewRun() {
  for( size_t i=0; i<J.size(); i++ ) delete J[i];
  J.clear(); S.clear();
  for( int i=0; i<n_tools; i++ ) tools[i] = Tool();
  n_tools = 0;
  src_name.clear(); src_size = 0; src_crc = 0; src_tools.clear();
  work.clear(); n_log = 0; status_len = 0; phase.clear();
}

// the archive member just made, d into a temporary file beside the work
// directories, which Decompress checks (each stream and the whole, by the
// index's CRCs); the file is removed after (-verify)
static int Verify( const wstring& archive, const Member& m ) {
  wstring base = WorkBase();
  if( base.empty() ) return 2;
  wstring tmp = base + L"mp4pack_" + W(Hex(GetCurrentProcessId())) + L"_verify.tmp";
  Msg( "verify: d into " + U(tmp) );
  NewRun();
  t_start = GetTickCount64(); // (its own time)
  f_dec = 1;
  int r = Decompress( archive, m, tmp );
  if( r && run_err.empty() ) run_err = last_msg;
  f_dec = 0;
  Remove( LongPath(tmp) );
  DropWork();
  if( !f_abort ) Msg( r ? "verify failed" : "verified" );
  return r;
}

static void (*file_hook)( int k, int r ); // a frontend's: file k of a run begins (r -1), or has ended (r: how)

// a run's output is a directory, for an output of each file: it ends with a
// separator, or it is one (d of more files: always)
static int OutIsDir( const wstring& out, size_t n ) {
  return (out.size() && IsSep( out[out.size()-1] )) || IsDir( LongPath( FullPath(out) ) ) || (f_dec && (n>1));
}

// a file of a run: its state anew, its clock; and after it its error, if it
// is the run's first (before DropWork's notes), and its work directory away
static void FileBegin( int& units ) { NewRun(); last_msg.clear(); t_start = GetTickCount64(); units++; }
static void FileEnd( int r ) { if( r && run_err.empty() ) run_err = last_msg; DropWork(); }

// d of archive in: its member to out (a file), or each of its members into
// the directory out (dir; and always when it has more than one): named after
// the archive when it has one (x.mkv.mpk -> x.mkv), else as each was (its
// index: their names in a list's archive). units, ok: the files restored or
// not, and well
static int DecompressArchive( const wstring& in, const wstring& out, int dir, vector<wstring>& used, int& units, int& ok ) {
  qword isz;
  if( InputSize( in, isz ) ) { units++; return 2; } // (a file not restored: of how many, not known)
  vector<Member> v;
  string why;
  int r = ScanArchive( in, v, why ), bad = r;
  if( r && v.empty() ) { units++; Msg( U(in) + ": " + why ); return r; }
  if( r ) Msg( U(in) + ": " + why + "; the " + Num(v.size()) + ((v.size()==1) ? " file" : " files") + " before that are restored" );
  if( v.size()>1 ) Msg( U(in) + ": " + Num(v.size()) + " files" );
  vector<wstring> outs = MemberOuts( in, v, out, dir, used );
  int r0 = 0;
  for( size_t i=0; (i<v.size()) && !f_abort && !out_failed; i++ ) {
    const wstring& o = outs[i];
    if( v.size()>1 ) Msg( "file " + Num(i+1) + " of " + Num(v.size()) + " in it: " + v[i].src );
    FileBegin( units );
    int k = Decompress( in, v[i], o );
    FileEnd( k );
    if( k ) { if( !r0 ) r0 = k; continue; }
    ok++; bytes_in += v[i].size; bytes_out += src_size;
  }
  if( bad ) { units++; if( !r0 ) r0 = bad; if( run_err.empty() ) run_err = U(in) + ": " + why; } // (what is lost: one more not restored)
  return f_abort ? 5 : r0;
}

// c or d (f_dec) of each of ins in turn, to out: a directory (out ends with
// a separator, or is one; d of a list: always) for an output of each,
// named after it (x.mkv -> x.mkv.mpk; d: x.mkv.mpk -> x.mkv); else c makes
// one archive of them all (a member for each; append: added to the one
// there), d makes out of its one. A file that fails is told and left; the
// rest go on, unless the output cannot be written (out_failed) or Ctrl+C.
// The exit code: the first that failed's
static int RunFiles( const vector<wstring>& ins, const wstring& out0, int append ) {
  int n = int( ins.size() ), units = 0, ok = 0, r0 = 0, started = append, tried = 0;
  wstring out = FullPath( out0 );
  int dir = OutIsDir( out0, n );
  if( dir && out.size() && !IsSep( out[out.size()-1] ) ) out += L"" DS;
  vector<wstring> used;
  string failed;
  qword t0 = GetTickCount64();
  bytes_in = bytes_out = 0; out_failed = 0;
  for( int k=0; (k<n) && !f_abort && !out_failed; k++ ) {
    wstring in = FullPath( ins[k] );
    if( n>1 ) Msg( "== file " + Num(k+1) + " of " + Num(n) + ": " + U(in) );
    if( file_hook ) file_hook( k, -1 );
    tried++;
    int r, u = units, o = ok;
    if( !f_dec ) {
      wstring to = ArchiveOut( in, out, dir, used );
      Member m;
      FileBegin( units );
      r = Compress( in, to, !dir && started, m );
      if( !r && !dir ) started = 1;
      FileEnd( r );
      if( !r && f_verify && !f_abort ) r = Verify( to, m );
      if( !r ) { ok++; bytes_in += src_size; bytes_out += m.size; }
    } else r = DecompressArchive( in, out, dir, used, units, ok );
    if( r && !r0 ) r0 = r;
    if( (ok-o<units-u) && (r!=5) ) failed += (failed.size() ? ", " : "") + U( FileName(in) );
    if( file_hook ) file_hook( k, r );
  }
  if( (n>1) || (units>1) ) { // how it all went
    string a = Num(units) + " files: ";
    if( f_abort ) a += "stopped; ";
    if( ok<units ) a += Num(ok) + " done, " + Num(units-ok) + " not" + (failed.size() ? " (" + failed + ")" : string()) + "; ";
    if( tried<n ) a += Num(n-tried) + " not tried" + (out_failed ? string( " (the output cannot be written)" ) : string()) + "; ";
    if( ok ) a += Num(bytes_in) + " -> " + Num(bytes_out) + (f_dec ? string() : " (" + Pct( double(bytes_out), double(bytes_in) ) + ")") + ", ";
    a += Clock( GetTickCount64()-t0 );
    Msg( a );
    run_summary = a;
  }
  return f_abort ? 5 : r0;
}

// what a run needs once, with the options as set: the CPU's threads (or the
// free ones: -f), -t and -c, the tool builds; 0, or the exit code
static int Setup() {
  CpuThreads();
  CpuTargets();
  string c = "cpu: " + Num(cpu_threads) + " threads" + (cpu_cores ? ", " + Num(cpu_cores) + " cores" : string());
  if( !cpu_ok ) c += ", no CPUID (builds without a target only)";
  else if( cpu_best.size() ) c += ", " + cpu_best + (cpu.level ? " (x86-64-v" + Num(cpu.level) + ")" : string());
  if( cap>=0 ) c += ", builds for " + string( x86::x86_target[cap].name );
  int n = cpu_threads;
  if( f_free ) { n = FreeThreads(); c += ", " + Num(n) + " free"; }
  if( ParseThreads( t_spec, n ) ) { Msg( "bad -t " + t_spec + " (up to 4 of N or N% joined by + or , and maybe a -N)" ); run_err = last_msg; return 1; }
  for( int g=0; g<N_GROUP; g++ ) ch[g] = 0;
  if( c_spec.size() && ParseChunks( c_spec.c_str() ) ) { Msg( "bad -c " + c_spec ); run_err = last_msg; return 1; }
  Msg( c + "; -t " + (t_spec.size() ? t_spec+" = " : string()) + ThreadText() );
  string tl;
  for( int i=0; i<N_TOOLS; i++ ) { const Tool& t = GetTool( tool_names[i] ); if( t.path.size() ) tl += " " + t.file; }
  Msg( "tools:" + (tl.size() ? tl : string(" none")) );
  for( int i=0; i<2; i++ ) if( GetTool( tool_names[i] ).path.empty() ) { Msg( NoTool( GetTool( tool_names[i] ) ) ); run_err = last_msg; return 2; }
  return 0;
}

// c or d (f_dec) of the files ins to out (RunFiles), with the options as
// set: t_spec, c_spec, the levels (ParseLevels), cap (-i), w_base and the
// flags; the exit code
static int RunList( const vector<wstring>& ins, const wstring& out, int append ) {
  NewRun();
  held = 0;
  last_msg.clear(); run_err.clear(); run_summary.clear();
  int r = Setup();
  if( r ) return r;
  t_start = GetTickCount64();
  r = RunFiles( ins, out, append );
  if( f_abort ) { Msg( "aborted" ); r = 5; }
  return r;
}


