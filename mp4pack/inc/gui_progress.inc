// inc/gui_progress.inc -- a run shown: the controls enabled, the status line,
//  the log, the overall bar and a row for each task, the time
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- running

static void Enable() {
  int idle = !running, dec = Checked( c_dec );
  HWND all[] = { c_comp, c_dec, l_in, c_in, c_inb, l_out, c_out, c_outb, l_w, c_w, c_wb, c_wtemp, c_wout, c_k,
                 l_t, c_t, l_free, c_free, c_freeud, c_f, c_start, c_listb, c_listclr }; // (the list's row shows it while it runs)
  for( size_t i=0; i<sizeof(all)/sizeof(all[0]); i++ ) EnableWindow( all[i], idle );
  HWND copt[] = { c_verify, l_l, l_lh, c_lh, l_la, c_la, l_lt, c_lt }; // (c only)
  for( size_t i=0; i<sizeof(copt)/sizeof(copt[0]); i++ ) EnableWindow( copt[i], idle && !dec );
  EnableWindow( c_pause, running && !f_abort );
  EnableWindow( c_stop, running && !f_abort );
}

// the status line; cut: a failure's, whose end (the reason) is to be seen:
// cut in the middle when it does not fit (else at the end, by the control)
static void SetStatus( const string& s, int cut = 0 ) {
  status_text = W(s); status_cut = cut;
  SetText( c_status, cut ? MiddleCut( c_status, status_text ) : status_text );
}

static void ShowLog() {
  string s;
  EnterCriticalSection( &qlock );
  s.swap( log_q ); log_posted = 0;
  LeaveCriticalSection( &qlock );
  if( s.empty() ) return;
  // the result, the first of: "5939468 -> 2020271 (34.01%), 0:00:21" (c),
  // "... -> ..., CRC ... OK, ..." (d; Verify's comes after c's)
  for( size_t i=0; (i<s.size()) && result.empty(); ) {
    size_t e = s.find( "\r\n", i );
    if( e==string::npos ) e = s.size();
    if( (s[i]>='0') && (s[i]<='9') && (s.substr( i, e-i ).find( " -> " )!=string::npos) ) result = s.substr( i, e-i );
    i = e+2;
  }
  int n = GetWindowTextLengthW( c_log );
  SendMessageW( c_log, EM_SETSEL, n, n );
  SendMessageW( c_log, EM_REPLACESEL, FALSE, (LPARAM)W(s).c_str() );
}

static int Verifying( const string& ph ) { return run_verify && ((ph=="extract") || (ph=="unpacking") || (ph=="avip d")); }

// the whole run's share done, from its phase's (-1: no phase): c is mostly
// packing (avip c and the archive go at the disk's speed), d mostly
// unpacking (extracting the archive and avip d at the disk's); whole: with
// Verify, c in the first half, its d in the second
static double Total( const string& ph, double f, int whole ) {
  static const struct { const char* ph; double a, b; } map[] = {
    { "avip c", 0, .03 }, { "packing", .03, .97 }, { "archive", .97, 1 },
    { "extract", 0, .03 }, { "unpacking", .03, .97 }, { "avip d", .97, 1 } };
  for( size_t i=0; i<sizeof(map)/sizeof(map[0]); i++ ) if( ph==map[i].ph ) {
    double t = map[i].a + (map[i].b-map[i].a)*std::max( f, 0.0 );
    if( whole && run_verify ) t = (i<3) ? t/2 : .5+t/2;
    return t;
  }
  return -1;
}

static wstring Percent( double f ) { wchar_t b[16]; swprintf( b, 16, L"%.1f%%", ((f>-0.0005) && (f<0.0005)) ? 0.0 : 100*f ); return b; }

// a bar that runs to and fro (comctl32 6) for what does not tell how far it is
static void Marquee( HWND b, int on ) {
  LONG_PTR st = GetWindowLongPtrW( b, GWL_STYLE );
  if( !(st&PBS_MARQUEE)==!on ) return;
  if( !on ) SendMessageW( b, PBM_SETMARQUEE, FALSE, 0 ); // (stopped while it has the style)
  SetWindowLongPtrW( b, GWL_STYLE, on ? (st|PBS_MARQUEE) : (st&~LONG_PTR(PBS_MARQUEE)) );
  if( on ) SendMessageW( b, PBM_SETMARQUEE, TRUE, 40 );
}

static Task* TaskOf( const string& key ) {
  for( size_t i=0; i<tasks.size(); i++ ) if( tasks[i].key==key ) return &tasks[i];
  Task t = Task();
  t.key = t.label = key;
  tasks.push_back( t );
  return &tasks.back();
}

// the progress's jobs into this run's tasks: a stream's parts (label#N) as
// one; Verify's d as one
static void Tasks( const Prog& p ) {
  if( Verifying( p.phase ) ) {
    Task* t = TaskOf( "verify" );
    t->label = "verify: d, the CRCs";
    t->st = 1; t->frac = Total( p.phase, p.frac, 0 );
    return;
  }
  struct Sum { string key; int n, wait, run, known, done, fail, stop, est, gain; double w, f; qword work, made; };
  vector<Sum> v;
  for( size_t i=0; i<p.jobs.size(); i++ ) {
    const JobRow& j = p.jobs[i];
    string k = j.label.substr( 0, j.label.find( '#' ) );
    size_t s = 0;
    while( (s<v.size()) && (v[s].key!=k) ) s++;
    if( s==v.size() ) { Sum z = Sum(); z.key = k; v.push_back( z ); }
    Sum& a = v[s];
    double w = double( std::max<qword>( j.work, 1 ) );
    a.n++; a.w += w; a.work += j.work; a.made += j.made; a.est |= j.est; a.gain |= j.gain;
    if( j.st==0 ) a.wait++;
    else if( j.st==1 ) { a.run++; if( j.frac>=0 ) { a.known++; a.f += w*j.frac; } }
    else if( j.st==2 ) { a.done++; a.f += w; }
    else if( j.st==3 ) a.fail++;
    else if( j.st==5 ) { a.stop++; a.f += w*j.frac; }
  }
  for( size_t i=0; i<v.size(); i++ ) {
    const Sum& a = v[i];
    Task* t = TaskOf( a.key );
    t->label = a.key + ((a.n>1) ? " (" + Num(a.n) + " parts)" : string());
    t->st = a.fail ? 3 : a.stop ? 5 : (a.run || (a.done && a.wait)) ? 1 : a.wait ? 0 : 2;
    t->frac = ((t->st==1) && !a.known && !a.done) ? -1 : a.f/a.w;
    t->est = a.est; t->gain = a.gain; t->work = a.work; t->made = a.made;
  }
}

// a bar's split: green to g, blue after it (-1: none); [NJ]: the overall bar
static void Split( int i, double g ) {
  if( g==split[i] ) return;
  split[i] = g;
  InvalidateRect( (i<NJ) ? c_jb[i] : c_bar, 0, TRUE );
}

// row i: a task (0: hidden), its bar, and its share done; when done its gain
// (in blue, and its bar green for what it came to, blue for the gain;
// Verify: OK), or that it failed (red) or was stopped
static void RowShow( int i, const Task* t ) {
  HWND c[3] = { c_jl[i], c_jb[i], c_jp[i] };
  if( !t ) { for( int k=0; k<3; k++ ) ShowWindow( c[k], SW_HIDE ); Split( i, -1 ); return; }
  int unknown = (t->st==1) && (t->frac<0), color = 0;
  double g = -1;
  SetText( c_jl[i], W(t->label) );
  Marquee( c_jb[i], unknown );
  if( !unknown ) SendMessageW( c_jb[i], PBM_SETPOS, int( std::max( t->frac, 0.0 )*1000 ), 0 );
  wstring p;
  if( t->st==2 ) {
    if( t->key=="verify" ) { p = L"OK"; color = 1; }
    else if( t->gain && t->work ) { g = double(t->made)/double(t->work); p = Percent( 1-g ); color = 1; g = std::min( g, 1.0 ); }
    else p = Percent( 1 );
  } else if( t->st==3 ) { p = L"failed"; color = 2; }
  else if( t->st==5 ) p = L"stopped";
  else if( (t->st==1) && !unknown ) p = (t->est ? L"~" : L"") + Percent( t->frac );
  SetText( c_jp[i], p );
  Split( i, g );
  if( (color!=jp_color[i]) || ((t->st==0)!=jl_gray[i]) ) {
    jp_color[i] = color; jl_gray[i] = (t->st==0);
    InvalidateRect( c_jp[i], 0, TRUE ); InvalidateRect( c_jl[i], 0, TRUE );
  }
  for( int k=0; k<3; k++ ) ShowWindow( c[k], SW_SHOWNA );
}

static void ShowRows() {
  int n = int( std::min<size_t>( tasks.size(), size_t(NJ) ) );
  if( n!=nrows ) { nrows = n; Layout(); Fit(); }
  for( int i=0; i<NJ; i++ ) {
    if( (i==NJ-1) && (tasks.size()>size_t(NJ)) ) { // the rest in one row
      Task r = Task();
      int st[6] = { 0 };
      double f = 0;
      for( size_t j=i; j<tasks.size(); j++ ) { st[tasks[j].st]++; f += std::max( tasks[j].frac, 0.0 ); }
      r.label = "+" + Num(tasks.size()-i) + " more"; r.st = st[3] ? 3 : st[5] ? 5 : st[1] ? 1 : st[0] ? 0 : 2; r.frac = f/double(tasks.size()-i);
      RowShow( i, &r );
    } else RowShow( i, (i<n) ? &tasks[i] : 0 );
  }
}

static void Title() {
  wchar_t t[160];
  string k = list_run ? Num(run_k+1) + "/" + Num(run_list.size()) + " " : string();
  if( running ) swprintf( t, 160, L"%ls%.0f%% %ls - mp4gui", W(k).c_str(), 100*shown, f_pause ? L"paused" : W( (Verifying( prog.phase ) ? "verify: " : "") + prog.phase ).c_str() );
  SetText( hwnd, running ? t : L"mp4gui" );
}

// the status line while it runs
static void RunStatus() {
  string k = (list_run && !f_abort) ? "File " + Num(run_k+1) + " of " + Num(run_list.size()) + ": " : string();
  SetStatus( k + (f_abort ? "Stopping..." : f_pause ? "Paused." : Verifying( prog.phase ) ? "Verifying: d of the archive..."
                  : run_dec ? "Decompressing..." : "Compressing...") );
}

static void ShowTime() {
  if( f_pause ) return; // (the clock stops)
  qword now = GetTickCount64();
  string s = "elapsed " + Clock( now-run_t0 );
  if( running && prog.eta ) s += ", about " + Clock( std::max<long long>( (long long)(prog.eta)-(long long)(now-prog_at), 0 ) ) + " left of "
                                 + (Verifying( prog.phase ) ? "verify: " : "") + prog.phase;
  SetText( c_time, W(s) );
}

static void ShowProg() {
  vector<Prog> q;
  EnterCriticalSection( &qlock );
  q.swap( prog_q ); prog_posted = 0;
  LeaveCriticalSection( &qlock );
  if( q.empty() ) return;
  int last = -1;
  for( size_t i=0; i<q.size(); i++ ) { // (each change, in its order)
    if( q[i].file>=0 ) { ListFile( q[i].file, q[i].fr ); continue; } // (a list's file begins or ends)
    Tasks( q[i] );
    double t = Total( q[i].phase, q[i].frac, 1 );
    if( t>=0 ) shown = std::max( shown, t );
    last = int(i);
  }
  if( last<0 ) { RunStatus(); return; }
  prog = q[last];
  if( !f_pause ) prog_at = GetTickCount64();
  Marquee( c_bar, (shown==0) && (prog.frac<0) ); // (nothing known yet: avip c)
  SetText( c_phase, W( (Verifying( prog.phase ) ? "verify: " : "") + prog.phase ) );
  SendMessageW( c_bar, PBM_SETPOS, int( shown*1000 ), 0 );
  SetText( c_pct, Percent( shown ) );
  ShowRows();
  Title();
  RunStatus();
  TaskbarShow( f_pause ? TBPF_PAUSED : TBPF_NORMAL, shown );
  ShowTime();
}

static void ClearProg() {
  prog = Prog(); shown = 0; tasks.clear(); pct_blue = 0;
  Split( NJ, -1 );
  SetText( c_phase, L"" ); SetText( c_pct, L"" ); SetText( c_time, L"" );
  Marquee( c_bar, 0 );
  SendMessageW( c_bar, PBM_SETPOS, 0, 0 );
  ShowRows();
}

