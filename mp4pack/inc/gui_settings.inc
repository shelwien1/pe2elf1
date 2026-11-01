// inc/gui_settings.inc -- the settings in mp4gui.ini; the options from the
//  controls (Options), and the command line they come to
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- settings: mp4gui.ini beside us

static wstring Ini() { return exe_dir + L"mp4gui.ini"; }

static wstring IniGet( const wchar_t* k, const wchar_t* def ) {
  wchar_t b[4096];
  GetPrivateProfileStringW( L"mp4gui", k, def, b, 4096, Ini().c_str() );
  return b;
}

static void IniSet( const wchar_t* k, const wstring& v ) { WritePrivateProfileStringW( L"mp4gui", k, v.c_str(), Ini().c_str() ); }

static void Load() {
  Check( c_verify, IniGet( L"verify", L"0" )==L"1" );
  SetText( c_t, IniGet( L"threads", L"100%,100%" ) );
  SetText( c_free, IniGet( L"free", L"0" ) );
  Check( c_f, IniGet( L"count_free", L"0" )==L"1" );
  SendMessageW( c_lh, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)IniGet( L"level_video", L"default" ).c_str() );
  SendMessageW( c_la, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)IniGet( L"level_audio", L"default" ).c_str() );
  SendMessageW( c_lt, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)IniGet( L"level_tangelo", L"default" ).c_str() );
  SetText( c_w, IniGet( L"work", L"" ) );
  Check( c_k, IniGet( L"keep", L"0" )==L"1" );
  if( SendMessageW( c_lh, CB_GETCURSEL, 0, 0 )<0 ) SendMessageW( c_lh, CB_SETCURSEL, 0, 0 );
  if( SendMessageW( c_la, CB_GETCURSEL, 0, 0 )<0 ) SendMessageW( c_la, CB_SETCURSEL, 0, 0 );
  if( SendMessageW( c_lt, CB_GETCURSEL, 0, 0 )<0 ) SendMessageW( c_lt, CB_SETCURSEL, 0, 0 );
  // the window where it was, if that is on a screen
  RECT r;
  if( swscanf( IniGet( L"window", L"" ).c_str(), L"%ld %ld %ld %ld", &r.left, &r.top, &r.right, &r.bottom )==4
      && (r.right-r.left>100) && (r.bottom-r.top>100) && MonitorFromRect( &r, MONITOR_DEFAULTTONULL ) ) {
    WINDOWPLACEMENT wp; memset( &wp, 0, sizeof(wp) );
    wp.length = sizeof(wp); wp.showCmd = SW_HIDE; wp.rcNormalPosition = r;
    SetWindowPlacement( hwnd, &wp );
  }
}

static void Save() {
  IniSet( L"verify", Checked( c_verify ) ? L"1" : L"0" );
  IniSet( L"threads", Text( c_t ) );
  IniSet( L"free", Text( c_free ) );
  IniSet( L"count_free", Checked( c_f ) ? L"1" : L"0" );
  IniSet( L"level_video", Text( c_lh ) );
  IniSet( L"level_audio", Text( c_la ) );
  IniSet( L"level_tangelo", Text( c_lt ) );
  IniSet( L"work", Text( c_w ) );
  IniSet( L"keep", Checked( c_k ) ? L"1" : L"0" );
  WINDOWPLACEMENT wp; wp.length = sizeof(wp);
  if( GetWindowPlacement( hwnd, &wp ) ) {
    const RECT& r = wp.rcNormalPosition;
    wchar_t b[64]; swprintf( b, 64, L"%ld %ld %ld %ld", r.left, r.top, r.right, r.bottom );
    IniSet( L"window", b );
  }
}

//---- the options

// -t: the threads, and -N for those to keep free
static string TSpec() {
  string t = U( Text( c_t ) );
  int k = _wtoi( Text( c_free ).c_str() );
  return (k>0) ? t + "-" + Num(k) : t;
}

// -l: "5,9,,7" ("" for the defaults)
static string LSpec() {
  int h = int( SendMessageW( c_lh, CB_GETCURSEL, 0, 0 ) ), a = int( SendMessageW( c_la, CB_GETCURSEL, 0, 0 ) ), t = int( SendMessageW( c_lt, CB_GETCURSEL, 0, 0 ) );
  return ((h>0) ? Num(10-h) : string()) + "," + ((a>0) ? Num(10-a) : string()) + ",," + ((t>0) ? Num(10-t) : string());
}

// the work directory's field, with %VARIABLES% as they are now
static wstring WorkDir() {
  wstring s = Text( c_w );
  DWORD n = ExpandEnvironmentStringsW( s.c_str(), 0, 0 );
  if( !n ) return s;
  vector<wchar_t> b( n+1 );
  ExpandEnvironmentStringsW( s.c_str(), &b[0], n+1 );
  return &b[0];
}

// the options into mp4pack's settings, as wmain sets them; 1 (and why) if one
// is bad. The part sizes are mp4pack's own (a part for each thread, as the
// memory allows: PartSize), the tool builds any that run here, unless the
// command line gave -c or -i.
static int Options( string& why ) {
  f_dec = Checked( c_dec );
  f_verify = !f_dec && Checked( c_verify );
  f_free = Checked( c_f ); f_keep = Checked( c_k ); f_verbose = 0;
  t_spec = TSpec();
  if( ParseThreads( t_spec, cpu_threads ) ) { why = "Threads: " + t_spec + " is not N or N% (up to 4, joined by + or ,), with a -N at the end maybe."; return 1; }
  c_spec = cl_c;
  string l = f_dec ? string() : LSpec();
  if( ParseLevels( l.c_str() ) ) { why = "Levels: 1 to 9."; return 1; }
  cap = cl_cap;
  w_base = WorkDir();
  return 0;
}

// the same as a command line, for the log
static string CommandText() {
  string s = string( "mp4pack " ) + (f_dec ? "d" : "c");
  if( t_spec.size() && (t_spec!="100%,100%") ) s += " -t " + Quote( t_spec );
  if( f_free ) s += " -f";
  if( !f_dec ) {
    string l = LSpec();
    if( l!=",,," ) s += " -l " + Quote( l );
  }
  if( c_spec.size() ) s += " -c " + Quote( c_spec );
  if( cap>=0 ) s += string( " -i " ) + x86::x86_target[cap].name;
  if( w_base.size() ) s += " -w " + Quote( U(w_base) );
  if( f_keep ) s += " -k";
  if( f_verify ) s += " -verify";
  return s + " " + (list_run ? "@(" + Num(run_ins.size()) + ((run_ins.size()==1) ? " file" : " files") + " of the list" + (run_append ? ", added to the archive" : "") + ")" : Quote( U(run_in) )) + " " + Quote( U(run_out) );
}

