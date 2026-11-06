// inc/gui_run.inc -- a run started (a file, or the list), paused, stopped,
//  and ended
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- a run started and ended

static void Problem( const wstring& s ) { MessageBoxW( hwnd, s.c_str(), L"mp4gui", MB_OK|MB_ICONWARNING ); }

// the outputs of a run that are there already: as RunFiles names them
// (ArchiveOut, MemberOuts)
static vector<wstring> OutputsThere( const vector<wstring>& ins, const wstring& out, int dir ) {
  vector<wstring> v, used;
  wstring d = out;
  if( dir && d.size() && !IsSep( d[d.size()-1] ) ) d += L"" DS;
  for( size_t i=0; i<ins.size(); i++ ) {
    vector<wstring> o;
    if( !f_dec ) o.push_back( ArchiveOut( ins[i], d, dir, used ) );
    else {
      vector<Member> m;
      string why;
      ScanArchive( ins[i], m, why );
      o = MemberOuts( ins[i], m, d, dir, used );
    }
    for( size_t k=0; k<o.size(); k++ ) if( Exists( LongPath( o[k] ) ) ) v.push_back( o[k] );
  }
  return v;
}

static void OnStart() {
  if( running ) return;
  wstring in = Text( c_in ), out = Text( c_out );
  if( list.empty() && (in.size()>1) && (in[0]=='@') ) { // (a list typed in, as mp4pack's @list: to the List)
    vector<wstring> v;
    string why;
    if( ReadList( in.substr( 1 ), v, why ) || v.empty() ) { Problem( v.empty() && why.empty() ? L"No files in the list " + in.substr( 1 ) + L"." : W(why) ); return; }
    ListAdd( v );
    out = Text( c_out );
  }
  int lst = !list.empty();
  if( !lst && in.empty() ) { Problem( L"Choose the file to compress or decompress (Input), or files for the List." ); SetFocus( c_in ); return; }
  if( out.empty() ) {
    Problem( lst ? L"Choose the output: a directory (ending with \\) for an output of each file, or a file for one archive of them all." : L"Choose the output file." );
    SetFocus( c_out ); return;
  }
  string why;
  if( Options( why ) ) { Problem( W(why) ); return; }
  vector<wstring> ins;
  run_list.clear();
  int done = 0;
  if( lst ) { // those not done yet (all, when all are)
    if( ListNext()<0 ) for( size_t i=0; i<list.size(); i++ ) list[i].st = 0;
    for( size_t i=0; i<list.size(); i++ ) if( list[i].st!=2 ) { run_list.push_back( int(i) ); ins.push_back( list[i].path ); } else done++;
  } else {
    in = FullPath( in );
    if( !Exists( LongPath(in) ) ) { Problem( L"There is no file " + in + L"." ); SetFocus( c_in ); return; }
    ins.push_back( in );
  }
  wstring fo = FullPath( out );
  int dir = OutIsDir( out, ins.size() ); // (as RunFiles has it)
  int append = lst && !dir && !f_dec && done && Exists( LongPath(fo) ); // (c into one archive, some done: added to it)
  if( !f_overwrite && !append ) {
    vector<wstring> there = OutputsThere( ins, fo, dir );
    if( there.size() && !((there.size()==1) && (there[0]==out_ok)) ) {
      wstring q = there[0];
      for( size_t i=1; (i<there.size()) && (i<4); i++ ) q += L"\n" + there[i];
      if( there.size()>4 ) q += L"\n...";
      q = (there.size()==1) ? q + L" exists. Replace it?" : W(Num(there.size())) + L" of the files to make are there already:\n" + q + L"\nReplace them?";
      if( MessageBoxW( hwnd, q.c_str(), L"mp4gui", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 )!=IDYES ) return;
    }
  }
  Save();
  if( info.path.size() && (info.kind==0) ) { // (its avip would vie with the run's for the file)
    InfoStop();
    info.kind = 3; info.text = "(not read: the run came first; it is when the run ends)";
    info_held = 1;
    InfoShow();
  }
  list_run = lst; run_ins = ins; run_append = append; run_k = 0;
  run_in = ins[0]; run_out = fo; run_dec = f_dec; run_verify = f_verify;
  if( lst ) { swapping = 1; SetText( c_in, ins[0] ); swapping = 0; }
  SetText( c_log, L"" ); ClearProg(); result.clear();
  EnterCriticalSection( &qlock ); log_q.clear(); prog_q.clear(); LeaveCriticalSection( &qlock );
  f_abort = 0; f_pause = 0; ResetEvent( ev_abort );
  running = 1; run_t0 = prog_at = GetTickCount64();
  HookMsg( "> " + CommandText() );
  h_run = CreateThread( 0, 0, RunThread, 0, 0, 0 );
  if( !h_run ) { running = 0; Problem( L"Cannot start a thread." ); return; }
  Enable(); SetFocus( c_stop );
  RunStatus();
  SetTimer( hwnd, T_TICK, 1000, 0 );
  TaskbarShow( TBPF_NORMAL, 0 );
  Title();
}

// Pause: mp4pack holds the tools (and starts none) until Resume; the clock
// leaves the pause out
static void OnPause() {
  if( !running || f_abort ) return;
  int on = !f_pause;
  f_pause = on; SetEvent( ev_wake );
  if( on ) paused_at = GetTickCount64();
  else { qword d = GetTickCount64()-paused_at; run_t0 += d; prog_at += d; }
  SetText( c_pause, on ? L"&Resume" : L"P&ause" );
  RunStatus();
  TaskbarShow( on ? TBPF_PAUSED : TBPF_NORMAL, shown );
  Title();
}

static void OnStop() {
  if( !running || f_abort ) return;
  f_abort = 1; SetEvent( ev_abort ); // (as mp4pack's Ctrl+C; held tools end too)
  RunStatus();
  Enable();
}

static void OnDone( int r ) {
  WaitForSingleObject( h_run, INFINITE );
  CloseHandle( h_run ); h_run = 0;
  ShowProg(); // (the last of it: a list's last file's end)
  running = 0; f_pause = 0;
  int lr = list_run;
  if( lr ) {
    list_run = 0;
    for( size_t i=0; i<list.size(); i++ ) if( list[i].st==1 ) list[i].st = 0; // (stopped before its end came)
    ListShow();
  }
  KillTimer( hwnd, T_TICK );
  SetText( c_pause, L"P&ause" );
  ShowLog();
  static const char* what[] = { "", "bad options", "a file cannot be read or written, or a tool is missing", "a tool failed",
                                "not an mp4pack archive, or what came out is not as it was", "stopped" };
  int vf = 0; // Verify's d failed: the archive was made, but not right
  for( size_t i=0; i<tasks.size(); i++ ) if( tasks[i].key=="verify" ) {
    tasks[i].st = !r ? 2 : (r==5) ? 5 : 3; tasks[i].frac = r ? tasks[i].frac : 1;
    vf = (tasks[i].st==3);
  }
  if( lr ) { // a list's: how it all went (mp4pack's last line), the gain of what went well
    string sum = run_summary.size() ? run_summary : result.size() ? result : run_err; // (one file: its own line)
    double pk = double( run_dec ? bytes_in : bytes_out ), un = double( run_dec ? bytes_out : bytes_in ); // (packed, unpacked)
    shown = (r==5) ? shown : 1;
    Marquee( c_bar, 0 );
    SendMessageW( c_bar, PBM_SETPOS, int( shown*1000 ), 0 );
    int ok = !r && (pk>0) && (un>0);
    SetText( c_pct, ok ? GainText( pk, un, pct_blue ) : r ? L"" : Percent( 1 ) );
    if( !ok ) pct_blue = 0;
    Split( NJ, ok ? GainSplit( pk, un ) : -1 );
    InvalidateRect( c_pct, 0, TRUE );
    SetText( c_phase, !r ? (run_verify ? L"all done, verified" : L"all done") : (r==5) ? L"stopped" : L"not all done" );
    SetStatus( !r ? "Done: " + sum : (r==5) ? "Stopped: " + sum : "Not all done: " + sum, r && (r!=5) );
    TaskbarShow( (!r || (r==5)) ? TBPF_NOPROGRESS : TBPF_ERROR, 0 );
  } else if( r==0 ) {
    qword a = 0, b = 0;
    shown = 1;
    Marquee( c_bar, 0 );
    SendMessageW( c_bar, PBM_SETPOS, 1000, 0 );
    // the gain of it all: the archive against the file (c: out to in; d: in to out)
    int ok = !GetSize( LongPath(run_in), a ) && !GetSize( LongPath(run_out), b ) && a && b;
    double pk = double( run_dec ? a : b ), un = double( run_dec ? b : a );
    SetText( c_pct, ok ? GainText( pk, un, pct_blue ) : Percent( 1 ) );
    if( !ok ) pct_blue = 0;
    Split( NJ, ok ? GainSplit( pk, un ) : -1 );
    InvalidateRect( c_pct, 0, TRUE );
    SetText( c_phase, run_verify ? L"done, verified" : L"done" );
    SetStatus( "Done: " + result + (run_verify ? "; verified" : "") );
    TaskbarShow( TBPF_NOPROGRESS, 0 );
  } else {
    // mp4pack's message of what went wrong; else what its exit code means
    string why = run_err;
    if( why.compare( 0, 7, "error: " )==0 ) why.erase( 0, 7 );
    if( why.empty() ) why = ((r>0) && (r<5)) ? what[r] : "exit code " + Num(r);
    if( vf && (r==4) && run_err.empty() ) why = "the archive does not decompress to the input";
    SetText( c_phase, (r==5) ? L"stopped" : vf ? L"verify failed" : L"failed" );
    SetStatus( (r==5) ? string( "Stopped." ) : vf ? "Verify failed: " + why + ". The archive: " + result + "."
                                                  : "Failed: " + why, (r!=5) && !vf );
    TaskbarShow( (r==5) ? TBPF_NOPROGRESS : TBPF_ERROR, 0 );
  }
  ShowRows();
  ShowTime();
  Title();
  Enable();
  SetFocus( c_start );
  exit_code = r;
  if( closing || (f_quit && !r) ) { DestroyWindow( hwnd ); return; } // (-quit: done, verified too with Verify)
  if( info_held ) InfoRequest(); // (the input's description, put off by the run)
  if( GetForegroundWindow()!=hwnd ) { // (it may have taken long)
    FLASHWINFO fw = { sizeof(fw), hwnd, FLASHW_ALL|FLASHW_TIMERNOFG, 0, 0 };
    FlashWindowEx( &fw );
  }
  Refresh();
}

