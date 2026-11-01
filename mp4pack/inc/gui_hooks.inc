// inc/gui_hooks.inc -- the run's thread: its messages, progress and files
//  brought over to the window (msg_hook, status_hook, file_hook)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the run's thread, brought over

static void HookMsg( const string& s ) {
  EnterCriticalSection( &qlock );
  log_q += s + "\r\n";
  int post = !log_posted; log_posted = 1;
  LeaveCriticalSection( &qlock );
  if( post ) PostMessageW( hwnd, WM_LOG, 0, 0 );
}

static void HookStatus( double frac, qword eta, const vector<Job*>& all ) {
  Prog p;
  p.phase = phase; p.frac = frac; p.eta = eta;
  int gain = (phase=="packing") || (phase=="unpacking"); // (made: what it packed into, or unpacks)
  for( size_t i=0; i<all.size(); i++ ) {
    Job& j = *all[i];
    double f = (j.st==2) ? 1 : (j.st==5) ? j.frac : (j.st!=1) ? 0 : j.prog ? Frac( j ) : -1; // (stopped: where it was)
    if( (j.st==1) && (j.prog==4) && (f==0) ) f = -1; // (measured by what it has written, and nothing yet: ttfcomp x writes at the end)
    JobRow r = { j.label, j.st, f, (j.prog==4) && !f_dec, gain, j.work, j.made };
    p.jobs.push_back( r );
  }
  EnterCriticalSection( &qlock );
  int same = prog_q.size() && (prog_q.back().file<0) && (prog_q.back().phase==p.phase) && (prog_q.back().jobs.size()==p.jobs.size());
  for( size_t i=0; same && (i<p.jobs.size()); i++ ) same = (prog_q.back().jobs[i].label==p.jobs[i].label) && (prog_q.back().jobs[i].st==p.jobs[i].st);
  if( same ) std::swap( prog_q.back(), p ); // (only further on: the latest will do)
  else prog_q.push_back( p );
  int post = !prog_posted; prog_posted = 1;
  LeaveCriticalSection( &qlock );
  if( post ) PostMessageW( hwnd, WM_PROG, 0, 0 );
}

// a file of the list's run begins (r -1) or ends (r): in the progress's
// order, as a change of it
static void HookFile( int k, int r ) {
  Prog p;
  p.file = k; p.fr = r;
  EnterCriticalSection( &qlock );
  prog_q.push_back( p );
  int post = !prog_posted; prog_posted = 1;
  LeaveCriticalSection( &qlock );
  if( post ) PostMessageW( hwnd, WM_PROG, 0, 0 );
}

static DWORD WINAPI RunThread( void* ) {
  int r = list_run ? RunList( run_ins, run_out, run_append ) : RunList( vector<wstring>( 1, run_in ), run_out, 0 );
  PostMessageW( hwnd, WM_DONE, WPARAM(r), 0 );
  return 0;
}

