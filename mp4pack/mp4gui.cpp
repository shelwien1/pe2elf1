// mp4gui.cpp -- mp4pack in a window (Windows): mp4pack.cpp itself, with the
// files (Open/Save dialogs, or dropped on the window), its options, and while
// it runs the progress of each of its tasks (each stream's packer, avip, the
// final shar) and of the whole (also in the taskbar), the gain of each task
// when it is done, and its messages, in place of the command line. CPU Info
// shows the CPU and the tool builds.
//
// Run, mp4pack's whole run, works on a thread of its own here; msg_hook and
// status_hook bring its messages and progress over. Stop sets its abort flag
// as Ctrl+C does, Pause its f_pause (the tools are held, not ended). The
// settings are kept in mp4gui.ini beside it.
//
// Plain Win32 (MinGW-w64; the same with the MSVC target, without -municode):
//   g++ -std=c++11 -O2 -s -static -municode -mwindows -o mp4gui.exe mp4gui.cpp
//       -lcomdlg32 -lole32 -lshell32 -lgdi32
// mp4gui.exe goes next to the tools, as mp4pack.exe does.

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7: the taskbar's progress
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#define MP4PACK_NO_MAIN
#include "mp4pack.cpp"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

enum {
  ID_COMP = 100, ID_DEC, ID_VERIFY, ID_CPU, ID_IN, ID_INB, ID_OUT, ID_OUTB, ID_W, ID_WB, ID_WTEMP, ID_WOUT, ID_K,
  ID_T, ID_FREE, ID_FREEUD, ID_F, ID_LH, ID_LA, ID_LS, ID_LOG, ID_START, ID_PAUSE, ID_STOP, ID_STATUS, ID_PHASE, ID_PCT
};
enum { WM_LOG = WM_APP+1, WM_PROG, WM_DONE };  // from the run's thread
enum { T_TICK = 1, T_OPTS };                    // timers: the clock while it runs; options changed
static const int NJ = 12;                       // task rows, at most

static HINSTANCE hinst;
static HWND hwnd, hcpu;           // the window; the CPU Info window
static HFONT font, mono;          // the system's message font; a fixed one
static int bx, by;                // the font's average character width and height
static int rh, lw;                // a row's height; the labels' width
static int log_y;                 // where the log begins (the rest is above it)
static int nrows;                 // task rows shown

static HWND c_comp, c_dec, c_verify, c_cpu, l_in, c_in, c_inb, l_out, c_out, c_outb, l_w, c_w, c_wb, c_wtemp, c_wout, c_k;
static HWND g_opt, l_t, c_t, l_free, c_free, c_freeud, c_f, c_plan, l_l, l_lh, c_lh, l_la, c_la, l_ls, c_ls;
static HWND g_prog, c_phase, c_bar, c_pct, c_time, c_jl[NJ], c_jb[NJ], c_jp[NJ];
static HWND c_log, c_status, c_start, c_pause, c_stop;
static int jl_gray[NJ], jp_color[NJ], pct_blue; // colors: a task waiting (gray); a gain (1, blue), failed (2, red)

static wstring out_auto;          // the output name we made from the input's (else the user's)
static wstring out_ok;            // an existing output the user chose to overwrite
static int plan_bad;
static string details;            // the CPU Info window's text (made when idle)

//---- the run

static HANDLE h_run;              // its thread
static int running, closing;      // closing: the window goes when it ends
static wstring run_in, run_out;
static int run_dec, run_verify;   // d; c, then Verify's d
static qword run_t0, paused_at;
static CRITICAL_SECTION qlock;    // for what the run's thread hands over:
static string log_q;              //   messages (UTF-8 lines)
struct JobRow { string label; int st; double frac; int est, pack; qword work, made; }; // frac -1: not known; est: a guess
struct Prog { string phase; double frac; qword eta; vector<JobRow> jobs; };
static Prog prog_q;               //   the progress
static int log_posted, prog_posted;
static Prog prog;                 // the progress shown
static qword prog_at;             // when it came
static double shown;              // the overall bar (it does not go back)
static string result;             // the sizes and the time, when all went well

// a task: a job, or a stream's parts (aacpack's, shar's pieces) together
struct Task { string key, label; int st, est, pack; double frac; qword work, made; };
static vector<Task> tasks;        // this run's, as they came

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
  int pack = !f_dec && (phase=="packing");
  for( size_t i=0; i<all.size(); i++ ) {
    Job& j = *all[i];
    double f = (j.st==2) ? 1 : (j.st==5) ? j.frac : (j.st!=1) ? 0 : j.prog ? Frac( j ) : -1; // (stopped: where it was)
    if( (j.st==1) && (j.prog==4) && (f==0) ) f = -1; // (measured by what it has written, and nothing yet: ttfcomp x writes at the end)
    JobRow r = { j.label, j.st, f, (j.prog==4) && !f_dec, pack, j.work, j.made };
    p.jobs.push_back( r );
  }
  EnterCriticalSection( &qlock );
  std::swap( prog_q, p );
  int post = !prog_posted; prog_posted = 1;
  LeaveCriticalSection( &qlock );
  if( post ) PostMessageW( hwnd, WM_PROG, 0, 0 );
}

static DWORD WINAPI RunThread( void* ) {
  int r = Run( run_in, run_out );
  PostMessageW( hwnd, WM_DONE, WPARAM(r), 0 );
  return 0;
}

//---- the taskbar's progress (Windows 7+; none where it is not there)

static const GUID clsid_taskbar = { 0x56fdf344, 0xfd6d, 0x11d0, { 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90 } };
static const GUID iid_taskbar3 = { 0xea1afb91, 0x9e28, 0x4b86, { 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf } };
static ITaskbarList3* taskbar;
static UINT wm_taskbar;           // "TaskbarButtonCreated": it can be used from then on

static void TaskbarInit() {
  if( taskbar ) return;
  void* p = 0;
  if( SUCCEEDED( CoCreateInstance( clsid_taskbar, 0, CLSCTX_INPROC_SERVER, iid_taskbar3, &p ) ) ) {
    taskbar = (ITaskbarList3*)p;
    if( FAILED( taskbar->HrInit() ) ) { taskbar->Release(); taskbar = 0; }
  }
}

static void TaskbarShow( TBPFLAG st, double f ) {
  if( !taskbar ) return;
  taskbar->SetProgressState( hwnd, st );
  if( (st==TBPF_NORMAL) || (st==TBPF_PAUSED) ) taskbar->SetProgressValue( hwnd, ULONGLONG(f*1000), 1000 );
}

//---- controls

static int DX( int n ) { return MulDiv( n, bx, 4 ); } // dialog units: quarters of the average width
static int DY( int n ) { return MulDiv( n, by, 8 ); } // and eighths of the height

static wstring Text( HWND h ) {
  int n = GetWindowTextLengthW( h );
  vector<wchar_t> b( n+1 );
  n = GetWindowTextW( h, &b[0], n+1 );
  wstring s( &b[0], n );
  size_t a = s.find_first_not_of( L" \t" ), e = s.find_last_not_of( L" \t" );
  return (a==wstring::npos) ? wstring() : s.substr( a, e+1-a );
}

static void SetText( HWND h, const wstring& s ) {
  if( Text( h )!=s ) SetWindowTextW( h, s.c_str() ); // (no flicker for the same)
}

// a path pasted in quotes (as Explorer's "Copy as path" gives it): without
// them, as no file name has any; 1 if there were
static int Unquote( HWND h ) {
  int n = GetWindowTextLengthW( h );
  vector<wchar_t> b( n+1 );
  wstring s( &b[0], GetWindowTextW( h, &b[0], n+1 ) );
  if( s.find( L'"' )==wstring::npos ) return 0;
  s.erase( std::remove( s.begin(), s.end(), L'"' ), s.end() );
  SetWindowTextW( h, s.c_str() );
  SendMessageW( h, EM_SETSEL, s.size(), s.size() );
  return 1;
}

static int Checked( HWND h ) { return SendMessageW( h, BM_GETCHECK, 0, 0 )==BST_CHECKED; }
static void Check( HWND h, int on ) { SendMessageW( h, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0 ); }

static HWND Make( const wchar_t* cls, const wchar_t* text, DWORD style, int id = 0, DWORD ex = 0 ) {
  HWND h = CreateWindowExW( ex, cls, text, WS_CHILD|WS_VISIBLE|style, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)id, hinst, 0 );
  SendMessageW( h, WM_SETFONT, (WPARAM)font, FALSE );
  return h;
}

static HWND Label( const wchar_t* t ) { return Make( L"STATIC", t, SS_LEFT|SS_CENTERIMAGE ); }
static HWND Note( int id = 0 ) { return Make( L"STATIC", L"", SS_LEFTNOWORDWRAP|SS_NOPREFIX|SS_CENTERIMAGE|SS_ENDELLIPSIS, id ); }
static HWND Edit( int id, DWORD st = 0 ) { return Make( L"EDIT", L"", WS_TABSTOP|ES_AUTOHSCROLL|st, id, WS_EX_CLIENTEDGE ); }
static HWND Button( const wchar_t* t, int id, DWORD st = BS_PUSHBUTTON ) { return Make( L"BUTTON", t, WS_TABSTOP|st, id ); }
static HWND Group( const wchar_t* t ) { return Make( L"BUTTON", t, BS_GROUPBOX ); }
static HWND Bar() { HWND h = Make( PROGRESS_CLASSW, L"", PBS_SMOOTH ); SendMessageW( h, PBM_SETRANGE32, 0, 1000 ); return h; }

static HWND Combo( int id, DWORD st, const char* const* items, int n ) {
  HWND h = Make( L"COMBOBOX", L"", WS_TABSTOP|WS_VSCROLL|st, id );
  for( int i=0; i<n; i++ ) SendMessageW( h, CB_ADDSTRING, 0, (LPARAM)W(items[i]).c_str() );
  return h;
}

static void Place( HWND h, int x, int y, int w, int ht ) { SetWindowPos( h, 0, x, y, std::max( w, 0 ), std::max( ht, 0 ), SWP_NOZORDER|SWP_NOACTIVATE ); }

static int TextWidth( const wchar_t* t ) {
  HDC dc = GetDC( hwnd );
  HGDIOBJ o = SelectObject( dc, font );
  SIZE sz = { 0, 0 };
  GetTextExtentPoint32W( dc, t, int(wcslen(t)), &sz );
  SelectObject( dc, o ); ReleaseDC( hwnd, dc );
  return sz.cx;
}

static const char* const t_items[] = { "100%,100%", "50%+50%", "75%+25%", "1" };
static const char* const l_items[] = { "default", "9", "8", "7", "6", "5", "4", "3", "2", "1" };
static const char* const s_items[] = { "default", "22", "20", "19", "15", "9", "5", "1", "0", "-1", "-5" };

static void MakeControls() {
  c_comp = Button( L"&Compress", ID_COMP, BS_AUTORADIOBUTTON|WS_GROUP );
  c_dec = Button( L"&Decompress", ID_DEC, BS_AUTORADIOBUTTON );
  c_verify = Button( L"&Verify", ID_VERIFY, BS_AUTOCHECKBOX|WS_GROUP );
  c_cpu = Button( L"CPU I&nfo", ID_CPU );
  l_in = Label( L"&Input" ); c_in = Edit( ID_IN ); c_inb = Button( L"...", ID_INB );
  l_out = Label( L"&Output" ); c_out = Edit( ID_OUT ); c_outb = Button( L"...", ID_OUTB );
  l_w = Label( L"&Work dir" ); c_w = Edit( ID_W ); c_wb = Button( L"...", ID_WB );
  c_wtemp = Button( L"%TEMP%", ID_WTEMP ); c_wout = Button( L"Output dir", ID_WOUT ); c_k = Button( L"&keep it", ID_K, BS_AUTOCHECKBOX );

  g_opt = Group( L"Options" );
  l_t = Label( L"&Threads" ); c_t = Combo( ID_T, CBS_DROPDOWN|CBS_AUTOHSCROLL, t_items, 4 );
  l_free = Label( L"keep free" ); c_free = Edit( ID_FREE, ES_NUMBER|ES_RIGHT );
  c_freeud = Make( UPDOWN_CLASSW, L"", UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_NOTHOUSANDS, ID_FREEUD );
  c_f = Button( L"only currently idle t&hreads", ID_F, BS_AUTOCHECKBOX );
  c_plan = Note();
  l_l = Label( L"&Levels" );
  l_lh = Label( L"h264p" ); c_lh = Combo( ID_LH, CBS_DROPDOWNLIST, l_items, 10 );
  l_la = Label( L"aacpack" ); c_la = Combo( ID_LA, CBS_DROPDOWNLIST, l_items, 10 );
  l_ls = Label( L"shar" ); c_ls = Combo( ID_LS, CBS_DROPDOWN, s_items, 11 );

  g_prog = Group( L"Progress" );
  c_phase = Note( ID_PHASE ); c_bar = Bar(); c_pct = Make( L"STATIC", L"", SS_RIGHT|SS_CENTERIMAGE, ID_PCT );
  c_time = Note();
  for( int i=0; i<NJ; i++ ) { c_jl[i] = Note(); c_jb[i] = Bar(); c_jp[i] = Make( L"STATIC", L"", SS_RIGHT|SS_CENTERIMAGE ); }

  c_log = Make( L"EDIT", L"", WS_TABSTOP|ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_HSCROLL|ES_AUTOVSCROLL|ES_AUTOHSCROLL, ID_LOG, WS_EX_CLIENTEDGE );
  SendMessageW( c_log, WM_SETFONT, (WPARAM)mono, FALSE );
  SendMessageW( c_log, EM_SETLIMITTEXT, 0x7FFFFFFE, 0 );
  c_status = Note( ID_STATUS );
  c_start = Button( L"&Start", ID_START, BS_DEFPUSHBUTTON );
  c_pause = Button( L"P&ause", ID_PAUSE );
  c_stop = Button( L"Sto&p", ID_STOP );

  SendMessageW( c_freeud, UDM_SETBUDDY, (WPARAM)c_free, 0 );
  SendMessageW( c_freeud, UDM_SETRANGE32, 0, std::max( cpu_threads-1, 0 ) );
  SendMessageW( c_w, EM_SETCUEBANNER, TRUE, (LPARAM)L"the temp directory" ); // (comctl32 6)
  const wchar_t* lab[] = { L"&Input", L"&Output", L"&Work dir", L"&Threads", L"&Levels" };
  lw = 0;
  for( int i=0; i<5; i++ ) lw = std::max( lw, TextWidth( lab[i] ) );
  lw += DX(6);
}

// a row of controls is rh high; the rest follows from the font, and the
// task rows from how many tasks there are
static void Layout() {
  RECT rc; GetClientRect( hwnd, &rc );
  int W = rc.right, H = rc.bottom;
  int m = DX(6), gap = DY(3), pad = DX(6), gc = by+DY(1), sp = DX(4); // gc: a group box's caption
  int x0 = m, wl = W-2*m, xr = x0+wl, bw = DX(18), y = m;
  int cw = TextWidth( L"CPU Info" )+DX(12);
  Place( c_comp, x0, y, DX(52), rh ); Place( c_dec, x0+DX(56), y, DX(60), rh );
  Place( c_verify, x0+DX(124), y, TextWidth( L"Verify" )+DX(16), rh );
  Place( c_cpu, xr-cw, y, cw, rh ); y += rh+gap;
  Place( l_in, x0, y, lw, rh ); Place( c_in, x0+lw, y, wl-lw-bw-sp, rh ); Place( c_inb, xr-bw, y, bw, rh ); y += rh+gap;
  Place( l_out, x0, y, lw, rh ); Place( c_out, x0+lw, y, wl-lw-bw-sp, rh ); Place( c_outb, xr-bw, y, bw, rh ); y += rh+gap;
  int kw = TextWidth( L"keep it" )+DX(16), ow = TextWidth( L"Output dir" )+DX(10), tw = TextWidth( L"%TEMP%" )+DX(10), a = xr;
  Place( c_k, a-kw, y, kw, rh ); a -= kw+sp;
  Place( c_wout, a-ow, y, ow, rh ); a -= ow+sp;
  Place( c_wtemp, a-tw, y, tw, rh ); a -= tw+sp;
  Place( c_wb, a-bw, y, bw, rh ); a -= bw+sp;
  Place( l_w, x0, y, lw, rh ); Place( c_w, x0+lw, y, a-x0-lw, rh ); y += rh+gap;

  int gy = y, x = x0+pad, w = wl-2*pad, xc = x+lw;
  y += gc;
  int tcw = DX(64), fw = DX(30), lfw = TextWidth( L"keep free" )+sp;
  Place( l_t, x, y, lw, rh ); Place( c_t, xc, y, tcw, rh+DY(80) );
  Place( l_free, xc+tcw+sp*2, y, lfw, rh ); Place( c_free, xc+tcw+sp*2+lfw, y, fw, rh );
  SendMessageW( c_freeud, UDM_SETBUDDY, (WPARAM)c_free, 0 ); // (it moves with it)
  a = xc+tcw+sp*4+lfw+fw;
  Place( c_f, a, y, x+w-a, rh ); y += rh;
  Place( c_plan, xc, y, x+w-xc, by+DY(3) ); y += by+DY(3)+gap;
  int lc = DX(50), w1 = TextWidth( L"h264p" )+sp, w2 = TextWidth( L"aacpack" )+sp, w3 = TextWidth( L"shar" )+sp;
  a = xc;
  Place( l_l, x, y, lw, rh );
  Place( l_lh, a, y, w1, rh ); a += w1; Place( c_lh, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_la, a, y, w2, rh ); a += w2; Place( c_la, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_ls, a, y, w3, rh ); a += w3; Place( c_ls, a, y, lc, rh+DY(100) ); y += rh+DY(4);
  Place( g_opt, x0, gy, wl, y-gy ); y += gap;

  gy = y; y += gc;
  int pw = DX(30), jw = DX(116), bx0 = x0+pad+jw, bw0 = xr-pad-pw-sp-bx0, jh = by+DY(4);
  Place( c_phase, x0+pad, y, jw, rh ); Place( c_bar, bx0, y+DY(2), bw0, rh-DY(4) ); Place( c_pct, xr-pad-pw, y, pw, rh ); y += rh;
  Place( c_time, x0+pad, y, wl-2*pad, jh ); y += jh+gap;
  for( int i=0; i<NJ; i++ ) {
    int yi = y+std::min( i, nrows )*jh; // (the hidden ones at the end)
    Place( c_jl[i], x0+pad+DX(8), yi, jw-DX(8), jh ); Place( c_jb[i], bx0, yi+DY(2), bw0, jh-DY(4) ); Place( c_jp[i], xr-pad-pw, yi, pw, jh );
  }
  y += nrows*jh+DY(4);
  Place( g_prog, x0, gy, wl, y-gy ); y += gap;

  log_y = y;
  int yb = H-m-rh, bb = DX(52);
  Place( c_log, x0, y, wl, yb-gap-y );
  Place( c_status, x0, yb, wl-3*bb-4*sp, rh );
  Place( c_start, xr-3*bb-2*sp, yb, bb, rh );
  Place( c_pause, xr-2*bb-sp, yb, bb, rh );
  Place( c_stop, xr-bb, yb, bb, rh );
  InvalidateRect( hwnd, 0, TRUE );
}

// the window taller when the log would be too short (more task rows)
static void Fit() {
  RECT rc, wr, wa;
  GetClientRect( hwnd, &rc );
  int need = log_y+DY(36)+DY(3)+rh+DX(6);
  if( (rc.bottom>=need) || IsZoomed( hwnd ) ) return;
  GetWindowRect( hwnd, &wr );
  int h = wr.bottom-wr.top+need-rc.bottom;
  if( SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 ) ) h = std::min<int>( h, wa.bottom-wr.top );
  SetWindowPos( hwnd, 0, 0, 0, wr.right-wr.left, h, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
}

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
  SendMessageW( c_lh, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)IniGet( L"level_h264p", L"default" ).c_str() );
  SendMessageW( c_la, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)IniGet( L"level_aacpack", L"default" ).c_str() );
  SetText( c_ls, IniGet( L"level_shar", L"default" ) );
  SetText( c_w, IniGet( L"work", L"" ) );
  Check( c_k, IniGet( L"keep", L"0" )==L"1" );
  if( SendMessageW( c_lh, CB_GETCURSEL, 0, 0 )<0 ) SendMessageW( c_lh, CB_SETCURSEL, 0, 0 );
  if( SendMessageW( c_la, CB_GETCURSEL, 0, 0 )<0 ) SendMessageW( c_la, CB_SETCURSEL, 0, 0 );
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
  IniSet( L"level_h264p", Text( c_lh ) );
  IniSet( L"level_aacpack", Text( c_la ) );
  IniSet( L"level_shar", Text( c_ls ) );
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

// -l: "5,9,,19" ("" for the defaults)
static string LSpec() {
  int h = int( SendMessageW( c_lh, CB_GETCURSEL, 0, 0 ) ), a = int( SendMessageW( c_la, CB_GETCURSEL, 0, 0 ) );
  string s = U( Text( c_ls ) );
  if( s.empty() || ((s[0]|32)=='d') ) s.clear(); // ("default")
  return ((h>0) ? Num(10-h) : string()) + "," + ((a>0) ? Num(10-a) : string()) + ",," + s;
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
// memory allows: PartSize), the tool builds any that run here.
static int Options( string& why ) {
  f_dec = Checked( c_dec );
  f_verify = !f_dec && Checked( c_verify );
  f_free = Checked( c_f ); f_keep = Checked( c_k ); f_verbose = 0;
  t_spec = TSpec();
  if( ParseThreads( t_spec, cpu_threads ) ) { why = "Threads: " + t_spec + " is not N or N% (up to 4, joined by + or ,), with a -N at the end maybe."; return 1; }
  c_spec.clear();
  string l = f_dec ? string() : LSpec();
  if( ParseLevels( l.c_str() ) ) { why = "Levels: shar's is -131072 to 22 (0 stores)."; return 1; }
  cap = -1;
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
  if( w_base.size() ) s += " -w " + Quote( U(w_base) );
  if( f_keep ) s += " -k";
  return s + " " + Quote( U(run_in) ) + " " + Quote( U(run_out) ) + (f_verify ? "  (and Verify)" : "");
}

//---- the CPU and the tool builds

static const string tool_names[] = { "avip", "shar", "h264p", "aacpack", "mp3pack", "oggcomp", "ttfcomp" };

static string Pad( string s, size_t n ) { if( s.size()<n ) s.append( n-s.size(), ' ' ); return s; }

// words after a label, wrapped at 78 columns
static void Words( string& o, const string& label, const vector<string>& w, size_t ind = 10 ) {
  string l = Pad( label, ind );
  size_t col = l.size();
  o += l;
  for( size_t i=0; i<w.size(); i++ ) {
    if( (col>ind) && (col+1+w[i].size()>78) ) { o += "\r\n" + string( ind, ' ' ); col = ind; }
    if( col>ind ) { o += " "; col++; }
    o += w[i]; col += w[i].size();
  }
  o += "\r\n";
}

static vector<string> Names( const x86::FSet& s, char cls = 0 ) {
  vector<string> v;
  for( int f=0; f<x86::N_FEAT; f++ ) if( x86::Has( s, f ) && (!cls || (x86::x86_feat[f].cls==cls)) ) v.push_back( x86::x86_feat[f].name );
  return v;
}

static string KB( int kb ) { return ((kb>=1024) && !(kb%1024)) ? Num(kb/1024) + "M" : Num(kb) + "K"; }

// what cpuinfo says of the CPU: its name, caches, features, the names
// -march=native gives it, and the targets whose code runs on it, best first
static string CpuText() {
  static x86::Cpuid d;
  static x86::Cpu c;
  static x86::Match m[x86::N_TARGET];
  if( !x86::ReadCpuid( d ) ) return "no CPUID here: only tool builds without a target are run\r\n";
  x86::Analyze( d, c );
  string o;
  char b[256];
  if( c.brand[0] ) o += string( "cpu       " ) + c.brand + "\r\n";
  snprintf( b, sizeof(b), "%-10s%s, family 0x%X model 0x%X stepping %u (%08X)", c.brand[0] ? "" : "cpu", c.vendor, c.family, c.model, c.stepping, c.sig );
  o += b;
  if( c.hybrid ) o += ", hybrid";
  if( c.hypervisor ) o += string( ", hypervisor " ) + (c.hv[0] ? c.hv : "?");
  o += "\r\nthreads   " + Num(cpu_threads) + (cpu_cores ? ", " + Num(cpu_cores) + " cores" : string()) + "\r\n";
  MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms);
  if( GlobalMemoryStatusEx( &ms ) ) o += "memory    " + Num(ms.ullTotalPhys>>20) + "MB, " + Num(ms.ullAvailPhys>>20) + "MB free\r\n";
  vector<string> w;
  for( int i=0; i<c.ncache; i++ ) {
    const x86::Cache& k = c.cache[i];
    w.push_back( "L" + Num(k.level) + ((k.type==1) ? "d " : (k.type==2) ? "i " : " ") + KB( k.kb ) + ((i+1<c.ncache) ? "," : "") );
  }
  if( w.size() ) Words( o, "caches", w );
  w.clear();
  if( !c.osxsave || !d.xcr0_set ) w.push_back( "no xsave: sse only" );
  else {
    static const struct { qword m; const char* n; } st[] = { { 2, "sse" }, { 4, "avx" }, { 0xE0, "avx512" }, { 0x60000, "amx" }, { 0x80000, "apx" } };
    w.push_back( "saves" );
    for( size_t i=0; i<sizeof(st)/sizeof(st[0]); i++ ) if( (c.xcr0&st[i].m)==st[i].m ) w.push_back( st[i].n );
  }
  Words( o, "os", w );
  o += "level     " + (c.level ? "x86-64-v" + Num(c.level) : string( "x86-32 only" )) + "\r\n";
  Words( o, "features", Names( c.f ) );
  x86::FSet off = x86::Sub( c.hw, c.f );
  if( !x86::Empty( off ) ) { w = Names( off ); w.push_back( "(in the CPU; not enabled by the OS)" ); Words( o, "off", w ); }
  if( c.gcc && !strcmp( c.gcc, c.clang ) ) o += string( "native    " ) + c.gcc + " (gcc and clang)\r\n";
  else o += string( "native    gcc: " ) + (c.gcc ? c.gcc : "-") + ", clang: " + c.clang + "\r\n";
  int bits = x86::Has( c.f, x86::F_LM ) ? 64 : 32, nall, n = x86::RankTargets( c, x86::C_BOTH, bits, false, m, &nall );
  o += string( "\r\n" ) + ((bits==64) ? "x64" : "x86-32") + " targets whose code runs here, best first (as builds are ranked):\r\n";
  for( int i=0, rank=1; i<n; rank++ ) {
    int lv = x86::LevelOf( m[i].need ), j = i;
    snprintf( b, sizeof(b), "%4d  %-7s %-3s ", rank, x86::TierName( m[i].tier ), lv ? ("v" + Num(lv)).c_str() : "-" );
    w.clear();
    for( ; (j<n) && (x86::x86_target[m[j].t].group==x86::x86_target[m[i].t].group); j++ ) w.push_back( string( x86::x86_target[m[j].t].name ) + x86::BuildSuffix( m[j].t, bits ) );
    if( !x86::Empty( m[i].miss ) ) { vector<string> v = Names( m[i].miss ); string s; for( size_t k=0; k<v.size(); k++ ) s += (k ? " " : "[no ") + v[k]; w.push_back( s + "]" ); }
    Words( o, b, w, 19 );
    i = j;
  }
  if( !n ) o += "  none\r\n";
  return o;
}

// each tool's builds here: the one run (*), the others that run, those that
// do not and why
static string BuildsText() {
  string o = "tool builds in " + U(exe_dir) + ":\r\n";
  for( int i=0; i<7; i++ ) {
    const Tool& t = GetTool( tool_names[i] );
    vector< std::pair<int,string> > v;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW( (exe_dir+W(tool_names[i])+L"*" EXE).c_str(), &fd );
    if( h!=INVALID_HANDLE_VALUE ) {
      do {
        if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) continue;
        string n = U(fd.cFileName);
        int tg, bits, r = BuildPlace( tool_names[i], n, tg, bits );
        if( r==-2 ) continue;
        string s = string( (n==t.file) ? "* " : "  " ) + Pad( n, 30 ) + " " + x86::x86_target[tg].name + x86::BuildSuffix( tg, bits );
        if( r<0 ) s += cpu_ok ? ": lacks " + Lacks( tg, bits ) : string( ": no CPUID to tell" );
        v.push_back( std::make_pair( (r<0) ? 1<<30 : r, s ) );
      } while( FindNextFileW( h, &fd ) );
      FindClose( h );
    }
    std::stable_sort( v.begin(), v.end(), []( const std::pair<int,string>& a, const std::pair<int,string>& b ) { return a.first<b.first; } );
    if( v.empty() ) o += Pad( tool_names[i], 9 ) + "none\r\n";
    for( size_t k=0; k<v.size(); k++ ) o += Pad( k ? "" : tool_names[i], 9 ) + v[k].second + "\r\n";
  }
  return o;
}

// the options' effects, while nothing runs: the threads as they come out (or
// what is not right), and the CPU Info window's text
static void Refresh() {
  if( running ) return;
  cap = -1;
  NewRun(); // (the tool builds found before)
  CpuTargets();
  details = CpuText() + "\r\n" + BuildsText();
  if( hcpu ) SetText( GetDlgItem( hcpu, 1 ), W(details) );
  string ts = TSpec(), p;
  plan_bad = ParseThreads( ts, cpu_threads );
  if( plan_bad ) p = "bad: N or N% (up to 4) joined by + or ,";
  else {
    p = "= " + ThreadText() + ": ";
    for( size_t s=0; s<stages.size(); s++ ) {
      p += s ? ", then " : "";
      for( size_t k=0; k<stages[s].size(); k++ ) p += string( k ? " + " : "" ) + g_name[stages[s][k]] + " " + Num(th[stages[s][k]]);
    }
    if( Checked( c_f ) ) p += " (of the idle ones then)";
  }
  SetText( c_plan, W(p) );
  InvalidateRect( c_plan, 0, TRUE );
}

//---- the CPU Info window

static LRESULT CALLBACK CpuProc( HWND h, UINT msg, WPARAM wp, LPARAM lp ) {
  switch( msg ) {
    case WM_CREATE: {
      HWND e = CreateWindowExW( 0, L"EDIT", W(details).c_str(), WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOHSCROLL|ES_AUTOVSCROLL,
                                0, 0, 0, 0, h, (HMENU)1, hinst, 0 );
      SendMessageW( e, WM_SETFONT, (WPARAM)mono, FALSE );
      return 0;
    }
    case WM_SIZE: MoveWindow( GetDlgItem( h, 1 ), 0, 0, LOWORD(lp), HIWORD(lp), TRUE ); return 0;
    case WM_SETFOCUS: SetFocus( GetDlgItem( h, 1 ) ); return 0;
    case WM_DESTROY: hcpu = 0; return 0;
  }
  return DefWindowProcW( h, msg, wp, lp );
}

static void ShowCpu() {
  if( hcpu ) { ShowWindow( hcpu, SW_SHOWNORMAL ); SetForegroundWindow( hcpu ); return; }
  HDC dc = GetDC( hwnd );
  HGDIOBJ o = SelectObject( dc, mono );
  TEXTMETRICW tm; GetTextMetricsW( dc, &tm );
  SelectObject( dc, o ); ReleaseDC( hwnd, dc );
  RECT r = { 0, 0, tm.tmAveCharWidth*92, tm.tmHeight*40 };
  AdjustWindowRectEx( &r, WS_OVERLAPPEDWINDOW, FALSE, 0 );
  r.right += GetSystemMetrics( SM_CXVSCROLL );
  hcpu = CreateWindowExW( 0, L"mp4gui_cpu", L"CPU Info - mp4gui", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                          r.right-r.left, r.bottom-r.top, hwnd, 0, hinst, 0 );
}

//---- the files

static int IsArchive( const wstring& f ) { return (f.size()>4) && (lstrcmpiW( f.c_str()+f.size()-4, L".mpk" )==0); }

// the output from the input, unless the user chose one: x.mkv -> x.mkv.mpk
// (c), x.mkv.mpk -> x.mkv (d)
static void AutoOut() {
  wstring o = Text( c_out ), f = Text( c_in ), n;
  if( o.size() && (o!=out_auto) ) return;
  if( f.size() ) n = !Checked( c_dec ) ? f + L".mpk" : IsArchive( f ) ? f.substr( 0, f.size()-4 ) : f + L".out";
  out_auto = n;
  SetText( c_out, n );
}

static void Enable();

// a new input: the mode by its name (.mpk: d), and its output
static void InputChanged() {
  wstring f = Text( c_in );
  if( f.size() ) { Check( c_dec, IsArchive( f ) ); Check( c_comp, !IsArchive( f ) ); }
  AutoOut();
  Enable(); // (Verify and the levels: c only)
}

static void SetInput( const wstring& f ) { SetText( c_in, f ); SetFocus( c_start ); }

static wstring FileDialog( int save ) {
  vector<wchar_t> f( 32768, 0 );
  wstring cur = Text( save ? c_out : c_in ), dir;
  if( cur.size() && (cur.size()<f.size()) ) { // start where the file is: its directory, its name
    wstring full = FullPath( cur ), name = FileName( full );
    dir = full.substr( 0, full.size()-name.size() );
    wcsncpy( &f[0], name.c_str(), f.size()-1 );
  }
  OPENFILENAMEW o; memset( &o, 0, sizeof(o) );
  o.lStructSize = sizeof(o); o.hwndOwner = hwnd; o.lpstrFile = &f[0]; o.nMaxFile = DWORD(f.size());
  o.lpstrInitialDir = dir.size() ? dir.c_str() : 0;
  o.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_ENABLESIZING|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  if( !save ) {
    o.lpstrFilter = L"Video files and mp4pack archives\0*.mp4;*.m4v;*.mkv;*.avi;*.mpk\0Video files (mp4, mkv, avi)\0*.mp4;*.m4v;*.mkv;*.avi\0"
                    L"mp4pack archives (*.mpk)\0*.mpk\0All files\0*.*\0";
    o.lpstrTitle = L"The file to compress or decompress";
    o.Flags |= OFN_FILEMUSTEXIST;
    if( !GetOpenFileNameW( &o ) ) return wstring();
  } else {
    int dec = Checked( c_dec );
    o.lpstrFilter = dec ? L"All files\0*.*\0" : L"mp4pack archives (*.mpk)\0*.mpk\0All files\0*.*\0";
    o.lpstrDefExt = dec ? 0 : L"mpk";
    o.lpstrTitle = dec ? L"The file to restore" : L"The archive to make";
    o.Flags |= OFN_OVERWRITEPROMPT;
    if( !GetSaveFileNameW( &o ) ) return wstring();
  }
  return &f[0];
}

static void BrowseWork() {
  BROWSEINFOW b; memset( &b, 0, sizeof(b) );
  b.hwndOwner = hwnd; b.lpszTitle = L"The folder for mp4pack's work directories (the unpacked streams need about the input's size)";
  b.ulFlags = BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
  LPITEMIDLIST p = SHBrowseForFolderW( &b );
  if( !p ) return;
  wchar_t d[MAX_PATH];
  if( SHGetPathFromIDListW( p, d ) ) SetText( c_w, d );
  CoTaskMemFree( p );
}

// the work directory where the output goes (or the input, without one)
static void WorkByOutput() {
  wstring f = Text( c_out );
  if( f.empty() ) f = Text( c_in );
  if( f.empty() ) return;
  f = FullPath( f );
  SetText( c_w, f.substr( 0, f.size()-FileName( f ).size() ) );
}

//---- running

static void Enable() {
  int idle = !running, dec = Checked( c_dec );
  HWND all[] = { c_comp, c_dec, l_in, c_in, c_inb, l_out, c_out, c_outb, l_w, c_w, c_wb, c_wtemp, c_wout, c_k,
                 l_t, c_t, l_free, c_free, c_freeud, c_f, c_start };
  for( size_t i=0; i<sizeof(all)/sizeof(all[0]); i++ ) EnableWindow( all[i], idle );
  HWND copt[] = { c_verify, l_l, l_lh, c_lh, l_la, c_la, l_ls, c_ls }; // (c only)
  for( size_t i=0; i<sizeof(copt)/sizeof(copt[0]); i++ ) EnableWindow( copt[i], idle && !dec );
  EnableWindow( c_pause, running && !f_abort );
  EnableWindow( c_stop, running && !f_abort );
}

static void SetStatus( const string& s ) { SetText( c_status, W(s) ); }

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

static int Verifying( const string& ph ) { return run_verify && ((ph=="shar x") || (ph=="unpacking") || (ph=="avip d")); }

// the whole run's share done, from its phase's (-1: no phase): c is mostly
// packing (avip c and shar a0 go at the disk's speed), d mostly unpacking;
// whole: with Verify, c in the first half, its d in the second
static double Total( const string& ph, double f, int whole ) {
  static const struct { const char* ph; double a, b; } map[] = {
    { "avip c", 0, .03 }, { "packing", .03, .97 }, { "shar a0", .97, 1 },
    { "shar x", 0, .03 }, { "unpacking", .03, .97 }, { "avip d", .97, 1 } };
  for( size_t i=0; i<sizeof(map)/sizeof(map[0]); i++ ) if( ph==map[i].ph ) {
    double t = map[i].a + (map[i].b-map[i].a)*std::max( f, 0.0 );
    if( whole && run_verify ) t = (i<3) ? t/2 : .5+t/2;
    return t;
  }
  return -1;
}

static wstring Percent( double f ) { wchar_t b[16]; swprintf( b, 16, L"%.1f%%", 100*f ); return b; }

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
  struct Sum { string key; int n, wait, run, known, done, fail, stop, est, pack; double w, f; qword work, made; };
  vector<Sum> v;
  for( size_t i=0; i<p.jobs.size(); i++ ) {
    const JobRow& j = p.jobs[i];
    string k = j.label.substr( 0, j.label.find( '#' ) );
    size_t s = 0;
    while( (s<v.size()) && (v[s].key!=k) ) s++;
    if( s==v.size() ) { Sum z = Sum(); z.key = k; v.push_back( z ); }
    Sum& a = v[s];
    double w = double( std::max<qword>( j.work, 1 ) );
    a.n++; a.w += w; a.work += j.work; a.made += j.made; a.est |= j.est; a.pack |= j.pack;
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
    t->est = a.est; t->pack = a.pack; t->work = a.work; t->made = a.made;
  }
}

// row i: a task (0: hidden), its bar, and its share done; when done its gain
// (blue; Verify: OK), or that it failed (red) or was stopped
static void RowShow( int i, const Task* t ) {
  HWND c[3] = { c_jl[i], c_jb[i], c_jp[i] };
  if( !t ) { for( int k=0; k<3; k++ ) ShowWindow( c[k], SW_HIDE ); return; }
  int unknown = (t->st==1) && (t->frac<0), color = 0;
  SetText( c_jl[i], W(t->label) );
  Marquee( c_jb[i], unknown );
  if( !unknown ) SendMessageW( c_jb[i], PBM_SETPOS, int( std::max( t->frac, 0.0 )*1000 ), 0 );
  wstring p;
  if( t->st==2 ) {
    if( t->key=="verify" ) { p = L"OK"; color = 1; }
    else if( t->pack && t->work ) { p = Percent( 1-double(t->made)/double(t->work) ); color = 1; }
    else p = Percent( 1 );
  } else if( t->st==3 ) { p = L"failed"; color = 2; }
  else if( t->st==5 ) p = L"stopped";
  else if( (t->st==1) && !unknown ) p = (t->est ? L"~" : L"") + Percent( t->frac );
  SetText( c_jp[i], p );
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
  wchar_t t[128];
  if( running ) swprintf( t, 128, L"%.0f%% %ls - mp4gui", 100*shown, f_pause ? L"paused" : W( (Verifying( prog.phase ) ? "verify: " : "") + prog.phase ).c_str() );
  SetText( hwnd, running ? t : L"mp4gui" );
}

// the status line while it runs
static void RunStatus() {
  SetStatus( f_abort ? "Stopping..." : f_pause ? "Paused: the tools are held." : Verifying( prog.phase ) ? "Verifying: d of the archive..."
             : run_dec ? "Decompressing..." : "Compressing..." );
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
  EnterCriticalSection( &qlock );
  prog = prog_q; prog_posted = 0;
  LeaveCriticalSection( &qlock );
  if( !f_pause ) prog_at = GetTickCount64();
  double t = Total( prog.phase, prog.frac, 1 );
  if( t>=0 ) shown = std::max( shown, t );
  Marquee( c_bar, (shown==0) && (prog.frac<0) ); // (nothing known yet: avip c)
  SetText( c_phase, W( (Verifying( prog.phase ) ? "verify: " : "") + prog.phase ) );
  SendMessageW( c_bar, PBM_SETPOS, int( shown*1000 ), 0 );
  SetText( c_pct, Percent( shown ) );
  Tasks( prog );
  ShowRows();
  Title();
  RunStatus();
  TaskbarShow( f_pause ? TBPF_PAUSED : TBPF_NORMAL, shown );
  ShowTime();
}

static void ClearProg() {
  prog = Prog(); shown = 0; tasks.clear(); pct_blue = 0;
  SetText( c_phase, L"" ); SetText( c_pct, L"" ); SetText( c_time, L"" );
  Marquee( c_bar, 0 );
  SendMessageW( c_bar, PBM_SETPOS, 0, 0 );
  ShowRows();
}

static void Problem( const wstring& s ) { MessageBoxW( hwnd, s.c_str(), L"mp4gui", MB_OK|MB_ICONWARNING ); }

static void OnStart() {
  if( running ) return;
  wstring in = Text( c_in ), out = Text( c_out );
  if( in.empty() ) { Problem( L"Choose the file to compress or decompress (Input)." ); SetFocus( c_in ); return; }
  if( out.empty() ) { Problem( L"Choose the output file." ); SetFocus( c_out ); return; }
  in = FullPath( in ); out = FullPath( out );
  if( !Exists( LongPath(in) ) ) { Problem( L"There is no file " + in + L"." ); SetFocus( c_in ); return; }
  string why;
  if( Options( why ) ) { Problem( W(why) ); return; }
  if( (out!=out_ok) && Exists( LongPath(out) )
      && (MessageBoxW( hwnd, (out + L" exists. Replace it?").c_str(), L"mp4gui", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 )!=IDYES) ) return;
  Save();
  run_in = in; run_out = out; run_dec = f_dec; run_verify = f_verify;
  SetText( c_log, L"" ); ClearProg(); result.clear();
  EnterCriticalSection( &qlock ); log_q.clear(); prog_q = Prog(); LeaveCriticalSection( &qlock );
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
  running = 0; f_pause = 0;
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
  if( r==0 ) {
    qword a, b;
    shown = 1;
    Marquee( c_bar, 0 );
    SendMessageW( c_bar, PBM_SETPOS, 1000, 0 );
    pct_blue = !run_dec && !GetSize( LongPath(run_in), a ) && !GetSize( LongPath(run_out), b ) && a; // (c: the gain of it all)
    SetText( c_pct, pct_blue ? Percent( 1-double(b)/double(a) ) : Percent( 1 ) );
    InvalidateRect( c_pct, 0, TRUE );
    SetText( c_phase, run_verify ? L"done, verified" : L"done" );
    SetStatus( "Done: " + result + (run_verify ? "; verified" : "") );
    TaskbarShow( TBPF_NOPROGRESS, 0 );
  } else {
    string why = ((r>0) && (r<5)) ? what[r] : "exit code " + Num(r);
    if( vf && (r==4) ) why = "the archive does not decompress to the input";
    SetText( c_phase, (r==5) ? L"stopped" : vf ? L"verify failed" : L"failed" );
    SetStatus( (r==5) ? string( "Stopped." ) : vf ? "Verify failed: " + why + " (see the log). The archive: " + result + "."
                                                  : "Failed: " + why + " (see the log)" );
    TaskbarShow( (r==5) ? TBPF_NOPROGRESS : TBPF_ERROR, 0 );
  }
  ShowRows();
  ShowTime();
  Title();
  Enable();
  SetFocus( c_start );
  if( GetForegroundWindow()!=hwnd ) { // (it may have taken long)
    FLASHWINFO fw = { sizeof(fw), hwnd, FLASHW_ALL|FLASHW_TIMERNOFG, 0, 0 };
    FlashWindowEx( &fw );
  }
  if( closing ) DestroyWindow( hwnd );
  else Refresh();
}

//---- the window

static LRESULT CALLBACK WndProc( HWND h, UINT msg, WPARAM wp, LPARAM lp ) {
  if( (msg==wm_taskbar) && wm_taskbar ) { TaskbarInit(); return 0; }
  switch( msg ) {
    case WM_SIZE: if( c_stop ) Layout(); return 0;
    case WM_GETMINMAXINFO: {
      if( !bx ) break;
      RECT r = { 0, 0, DX(380), log_y+DY(36)+rh+DX(6) };
      AdjustWindowRectEx( &r, GetWindowLongW( h, GWL_STYLE ), FALSE, GetWindowLongW( h, GWL_EXSTYLE ) );
      MINMAXINFO* mm = (MINMAXINFO*)lp;
      mm->ptMinTrackSize.x = r.right-r.left; mm->ptMinTrackSize.y = r.bottom-r.top;
      return 0;
    }
    case WM_COMMAND: {
      int id = LOWORD(wp), code = HIWORD(wp);
      switch( id ) {
        case IDOK: case ID_START: OnStart(); return 0; // (Enter)
        case ID_PAUSE: OnPause(); return 0;
        case ID_STOP: OnStop(); return 0;
        case ID_CPU: ShowCpu(); return 0;
        case ID_INB: { wstring f = FileDialog( 0 ); if( f.size() ) SetInput( f ); return 0; }
        case ID_OUTB: { wstring f = FileDialog( 1 ); if( f.size() ) { out_ok = f; SetText( c_out, f ); } return 0; }
        case ID_WB: BrowseWork(); return 0;
        case ID_WTEMP: SetText( c_w, L"%TEMP%" ); return 0;
        case ID_WOUT: WorkByOutput(); return 0;
        case ID_COMP: case ID_DEC: if( code==BN_CLICKED ) { AutoOut(); Enable(); } return 0;
        case ID_IN: if( (code==EN_CHANGE) && !Unquote( c_in ) ) InputChanged(); return 0;
        case ID_OUT: if( code==EN_CHANGE ) Unquote( c_out ); return 0;
        case ID_W: if( code==EN_CHANGE ) Unquote( c_w ); return 0;
        case ID_T: case ID_FREE: case ID_F:
          if( (code==EN_CHANGE) || (code==CBN_EDITCHANGE) || (code==CBN_SELCHANGE) || (code==BN_CLICKED) ) SetTimer( h, T_OPTS, 150, 0 );
          return 0;
      }
      break;
    }
    case WM_TIMER:
      if( wp==T_TICK ) ShowTime();
      if( wp==T_OPTS ) { KillTimer( h, T_OPTS ); Refresh(); }
      return 0;
    case WM_CTLCOLORSTATIC: {
      HWND c = (HWND)lp;
      COLORREF col = CLR_INVALID;
      if( (c==c_plan) && plan_bad ) col = RGB( 192, 0, 0 );
      if( (c==c_pct) && pct_blue ) col = RGB( 0, 0, 208 );
      for( int i=0; i<NJ; i++ ) {
        if( (c==c_jp[i]) && jp_color[i] ) col = (jp_color[i]==1) ? RGB( 0, 0, 208 ) : RGB( 192, 0, 0 );
        if( (c==c_jl[i]) && jl_gray[i] ) col = GetSysColor( COLOR_GRAYTEXT );
      }
      if( col!=CLR_INVALID ) {
        HDC dc = (HDC)wp;
        SetTextColor( dc, col ); SetBkColor( dc, GetSysColor( COLOR_BTNFACE ) );
        return (LRESULT)GetSysColorBrush( COLOR_BTNFACE );
      }
      break;
    }
    case WM_DROPFILES: {
      HDROP d = (HDROP)wp;
      UINT n = DragQueryFileW( d, 0, 0, 0 );
      if( n && !running ) {
        vector<wchar_t> f( n+1 );
        DragQueryFileW( d, 0, &f[0], n+1 );
        SetInput( &f[0] );
      }
      DragFinish( d );
      return 0;
    }
    case WM_LOG: ShowLog(); return 0;
    case WM_PROG: if( running ) ShowProg(); return 0;
    case WM_DONE: OnDone( int(wp) ); return 0;
    case WM_CLOSE:
      if( running ) {
        if( !closing && (MessageBoxW( h, L"mp4pack is running. Stop it and quit?", L"mp4gui", MB_YESNO|MB_ICONQUESTION )==IDYES) ) { closing = 1; OnStop(); }
        return 0; // (the window goes when it has stopped)
      }
      DestroyWindow( h );
      return 0;
    case WM_DESTROY:
      Save();
      PostQuitMessage( 0 );
      return 0;
  }
  return DefWindowProcW( h, msg, wp, lp );
}

// comctl32 version 6 (the themed controls) without a manifest of our own: an
// activation context from the one shell32.dll has (resource 124), as
// programs without one do; else the classic look
static void Styles() {
  wchar_t d[MAX_PATH];
  UINT n = GetSystemDirectoryW( d, MAX_PATH );
  if( n && (n<MAX_PATH-16) ) {
    wstring dir = d, src = dir + L"\\shell32.dll";
    ACTCTXW a; memset( &a, 0, sizeof(a) );
    a.cbSize = sizeof(a);
    a.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID|ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    a.lpSource = src.c_str(); a.lpResourceName = MAKEINTRESOURCEW( 124 ); a.lpAssemblyDirectory = dir.c_str();
    HANDLE c = CreateActCtxW( &a );
    ULONG_PTR cookie;
    if( c!=INVALID_HANDLE_VALUE ) ActivateActCtx( c, &cookie );
  }
  typedef BOOL (WINAPI *icc_t)( const INITCOMMONCONTROLSEX* );
  HMODULE m = LoadLibraryW( L"comctl32.dll" );
  icc_t icc = m ? (icc_t)(void*)GetProcAddress( m, "InitCommonControlsEx" ) : 0;
  INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_PROGRESS_CLASS|ICC_UPDOWN_CLASS|ICC_STANDARD_CLASSES };
  if( icc ) icc( &ic );
}

// the system's message font (Segoe UI), and a fixed one for the log
static void Fonts() {
  NONCLIENTMETRICSW nm; memset( &nm, 0, sizeof(nm) );
  nm.cbSize = sizeof(nm);
  if( !SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof(nm), &nm, 0 ) ) {
    nm.cbSize = sizeof(nm)-sizeof(int); // (before Vista: no iPaddedBorderWidth)
    SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, nm.cbSize, &nm, 0 );
  }
  font = CreateFontIndirectW( &nm.lfMessageFont );
  LOGFONTW lf = nm.lfMessageFont;
  lf.lfWeight = FW_NORMAL; lf.lfItalic = 0; lf.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;
  wcscpy( lf.lfFaceName, L"Consolas" );
  mono = CreateFontIndirectW( &lf );
  HDC dc = GetDC( 0 );
  HGDIOBJ o = SelectObject( dc, font );
  TEXTMETRICW tm; GetTextMetricsW( dc, &tm );
  SIZE sz; GetTextExtentPoint32W( dc, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &sz );
  bx = (sz.cx/26+1)/2; by = tm.tmHeight;
  SelectObject( dc, o ); ReleaseDC( 0, dc );
  rh = by+DY(6);
}

int WINAPI wWinMain( HINSTANCE hi, HINSTANCE, LPWSTR, int show ) {
  hinst = hi;
  typedef BOOL (WINAPI *dpi_t)( void ); // sharp on high-DPI screens (we scale by the font)
  dpi_t dpi = (dpi_t)(void*)GetProcAddress( GetModuleHandleW( L"user32.dll" ), "SetProcessDPIAware" );
  if( dpi ) dpi();
  Styles();
  CoInitializeEx( 0, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE );
  InitializeCriticalSection( &qlock );
  Init(); // mp4pack's
  tool_flags = DETACHED_PROCESS; // (no console for each tool: it has none of ours to share)
  msg_hook = HookMsg; status_hook = HookStatus;
  CpuThreads();
  Fonts();

  WNDCLASSEXW wc; memset( &wc, 0, sizeof(wc) );
  wc.cbSize = sizeof(wc); wc.lpfnWndProc = WndProc; wc.hInstance = hi;
  wc.hCursor = LoadCursorW( 0, (LPCWSTR)IDC_ARROW ); wc.hIcon = LoadIconW( 0, (LPCWSTR)IDI_APPLICATION );
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1); wc.lpszClassName = L"mp4gui";
  RegisterClassExW( &wc );
  wc.lpfnWndProc = CpuProc; wc.lpszClassName = L"mp4gui_cpu"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  RegisterClassExW( &wc );

  hwnd = CreateWindowExW( WS_EX_CONTROLPARENT|WS_EX_ACCEPTFILES, L"mp4gui", L"mp4gui", WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT,
                          DX(420), DY(400), 0, 0, hi, 0 );
  if( !hwnd ) return 1;
  wm_taskbar = RegisterWindowMessageW( L"TaskbarButtonCreated" );
  MakeControls();
  Layout();
  { // as big as the controls need, with room for 6 task rows and a log of about 10 lines
    RECT r = { 0, 0, DX(420), log_y+(by+DY(4))*6+DY(8*10)+DY(3)+rh+DX(6) }, wa;
    AdjustWindowRectEx( &r, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT|WS_EX_ACCEPTFILES );
    int w = r.right-r.left, ht = r.bottom-r.top;
    if( SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 ) ) { w = std::min<int>( w, wa.right-wa.left ); ht = std::min<int>( ht, wa.bottom-wa.top ); }
    SetWindowPos( hwnd, 0, 0, 0, w, ht, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
  }
  Load();
  Check( c_comp, 1 );
  int argc = 0; // mp4gui.exe input [output] (a file dropped on it, "Open with")
  wchar_t** argv = CommandLineToArgvW( GetCommandLineW(), &argc );
  if( argv && (argc>1) ) SetInput( FullPath( argv[1] ) );
  if( argv && (argc>2) ) SetText( c_out, FullPath( argv[2] ) );
  if( argv ) LocalFree( argv );
  Refresh();
  Enable();
  ClearProg();
  SetStatus( GetTool( "avip" ).path.size() ? "Ready." : "No avip here: mp4gui.exe goes next to the tools." );
  ShowWindow( hwnd, show );
  UpdateWindow( hwnd );
  if( !running ) SetFocus( Text( c_in ).empty() ? c_in : c_start );

  MSG m;
  while( GetMessageW( &m, 0, 0, 0 )>0 ) {
    if( IsDialogMessageW( hwnd, &m ) ) continue; // (Tab, Enter, Alt+the letters)
    TranslateMessage( &m );
    DispatchMessageW( &m );
  }
  if( taskbar ) taskbar->Release();
  CoUninitialize();
  return 0;
}
