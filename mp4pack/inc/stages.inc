// inc/stages.inc -- running the jobs: the progress (Status, status_hook),
//  Pause (the tools held), a stage of groups at once (RunStage), one job
//  (RunOne), a tool's exit code (Probe)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- running the jobs

static double Frac( Job& j ) {
  if( j.st==2 ) return 1;
  if( j.st!=1 ) return 0;
  double f = -1;
  if( j.prog==1 ) f = j.log.size() ? LogPct( j.log ) : -1;
  if( (j.prog==2) && j.work ) f = std::min( 1.0, double(j.done)/double(j.work) );
  if( (j.prog==3) && j.expect ) {
    qword n = 0, z;
    for( size_t i=0; i<j.outs.size(); i++ ) if( !LiveSize( j.outs[i], z ) ) n += z;
    f = std::min( 0.99, double(n)/double(j.expect) ); // (all of it only when it is done)
  }
  if( f>=0 ) j.frac = f;
  return j.frac;
}

// what runs now ("avip c", "packing" ...), and a frontend's hook for the
// progress, called instead of the status line: the share done (-1: no job
// tells), the time left in ms (0: not known yet), the jobs (st: which run)
static string phase;
static void (*status_hook)( double frac, qword eta, const vector<Job*>& all );

// [elapsed] overall%, eta, and what runs; on a console only (or status_hook;
// force: now, as when a stage ends)
static void Status( const vector<Job*>& all, const vector<Job*>& run, int force = 0 ) {
  static qword last;
  if( !status_hook && (!f_con || f_verbose) ) return;
  if( !force && (status_len || status_hook) && (GetTickCount64()-last<500) ) return;
  last = GetTickCount64();
  double tot = 0, done = 0;
  int any = 0; // a job that tells its progress
  for( size_t i=0; i<all.size(); i++ ) if( all[i]->st!=4 ) {
    double w = double( std::max<qword>( all[i]->work, 1 ) );
    tot += w; done += w*Frac( *all[i] );
    any |= all[i]->prog;
  }
  int eta = any && (done>0) && (done<tot) && (done>tot/200);
  qword now = GetTickCount64(), left = eta ? qword( double(now-t_run)*(tot-done)/done ) : 0;
  // and no sooner than the job running that has the most left, at its own
  // pace (h264p is much slower by the byte than the rest; not by a guess)
  if( eta ) for( size_t i=0; i<run.size(); i++ ) {
    const Job& j = *run[i];
    if( j.prog && ((j.prog!=3) || f_dec) && (j.frac>0.02) && (j.frac<1) ) left = std::max( left, qword( double(now-j.t0)*(1-j.frac)/j.frac ) );
  }
  if( status_hook ) { status_hook( any ? done/tot : -1, left, all ); return; }
  qword el = GetTickCount64()-t_start;
  string s = "[" + Clock(el) + "]";
  if( any ) s += " " + Pct( done, tot );
  if( eta ) s += " eta " + Clock( left );
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
static string LevelSp( int m );

static int held;                      // the tools running are held (Pause)
static qword t_held;                  // since when

// a tool's process held (all its threads suspended), or let go on
static void Hold( HANDLE hp, int on ) {
#ifdef _WIN32
  typedef LONG (WINAPI *nt_t)( HANDLE );
  nt_t f = (nt_t)(void*)GetProcAddress( GetModuleHandleW( L"ntdll.dll" ), on ? "NtSuspendProcess" : "NtResumeProcess" );
  if( f ) f( hp );
#else
  kill( pid_t( GetProcessId( hp ) ), on ? SIGSTOP : SIGCONT );
#endif
}

// Pause, or on again: the tools running held or let go, and the clocks set
// back by the pause
static void HoldAll( const vector<Job*>& run ) {
  held = !held;
  qword now = GetTickCount64();
  for( size_t i=0; i<run.size(); i++ ) Hold( run[i]->hp, held );
  if( held ) t_held = now;
  else {
    qword d = now-t_held;
    t_start += d; t_run += d;
    for( size_t i=0; i<run.size(); i++ ) run[i]->t0 += d;
  }
}

// while Pause holds, for work of our own (not a tool's): how long it held,
// the clocks set back by that
static qword PauseWait() {
  if( !f_pause || f_abort ) return 0;
  qword t = GetTickCount64();
  while( f_pause && !f_abort ) { HANDLE w[2] = { ev_abort, ev_wake }; WaitForMultipleObjects( 2, w, FALSE, 1000 ); }
  qword d = GetTickCount64()-t;
  t_start += d; t_run += d;
  return d;
}

// Runs the waiting jobs of these groups, the groups at once, each group's in
// its order within its threads (a job waits for the ones before it). 0: all
// done (failed packers fell back), 1: one failed, 2: Ctrl+C.
static int RunStage( const vector<Job*>& all, const vector<int>& groups ) {
  vector<Job*> run;
  int used[N_GROUP] = { 0 }, fail = 0;
  while(1) {
    if( (held!=(f_pause!=0)) && !fail && !f_abort ) HoldAll( run );
    for( size_t gi=0; gi<groups.size() && !fail && !f_abort && !held; gi++ ) {
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
    if( run.empty() ) {
      if( held && !f_abort && !fail ) { // (paused before the next one starts)
        HANDLE w[2] = { ev_abort, ev_wake };
        WaitForMultipleObjects( 2, w, FALSE, 1000 );
        Status( all, run );
        continue;
      }
      if( f_abort && !fail ) fail = 2; // (stopped before it began)
      break;
    }
    vector<HANDLE> h( 1, ev_abort ); // what has not finished yet
    h.push_back( ev_wake );
    for( size_t i=0; i<run.size(); i++ ) h.push_back( run[i]->hp );
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
      if( fail ) { j.st = 5; continue; } // (we stopped it)
      if( Fallback(j) ) continue;
      Msg( "error: " + j.label + ": " + j.why );
      LogTail( j );
      fail = 1;
      for( size_t k=0; k<run.size(); k++ ) TerminateProcess( run[k]->hp, 1 );
    }
    Status( all, run );
  }
  if( status_hook ) Status( all, run, 1 ); // (how it ended)
  return fail;
}

static int RunOne( Job& j ) {
  vector<Job*> v( 1, &j );
  vector<int> g( 1, j.g );
  j.report = 0;
  t_run = GetTickCount64();
  phase = j.label;
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

