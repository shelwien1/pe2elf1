// mp4gui.cpp -- mp4pack in a window (Windows): mp4pack.cpp itself, with the
// files (Open/Save dialogs, or dropped on the window), its options, and while
// it runs the progress of each of its tasks (avip, each stream's packer, the
// archive) and of the whole (also in the taskbar), the gain of each task when
// it is done (its bar green, then blue for the gain), and its messages, in
// place of the console. The Container box shows what the input holds
// (avip's description of it, or an archive's index). CPU Info shows the CPU
// and the tool builds. It takes mp4pack's command line too: c or d sets
// everything up, C or D starts it; -quit: it quits when that is done,
// -overwrite: an existing output is replaced without asking.
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
  ID_T, ID_FREE, ID_FREEUD, ID_F, ID_LH, ID_LA, ID_LT, ID_LOG, ID_START, ID_PAUSE, ID_STOP, ID_STATUS, ID_PHASE, ID_PCT, ID_INFO
};
enum { WM_LOG = WM_APP+1, WM_PROG, WM_DONE, WM_INFO }; // from the run's thread; from the input's description's
enum { T_TICK = 1, T_OPTS, T_INFO };            // timers: the clock while it runs; options changed; the input changed
static const int NJ = 12;                       // task rows, at most

static HINSTANCE hinst;
static HWND hwnd, hcpu;           // the window; the CPU Info window
static HFONT font, mono;          // the system's message font; a fixed one
static int bx, by;                // the font's average character width and height
static int mh, mw;                // the fixed font's line height and character width
static int rh, lw;                // a row's height; the labels' width
static int log_y;                 // where the log begins (the rest is above it)
static int nrows;                 // task rows shown

static HWND c_comp, c_dec, c_verify, c_cpu, l_in, c_in, c_inb, l_out, c_out, c_outb, l_w, c_w, c_wb, c_wtemp, c_wout, c_k;
static HWND g_opt, l_t, c_t, l_free, c_free, c_freeud, c_f, c_plan, l_l, l_lh, c_lh, l_la, c_la, l_lt, c_lt;
static HWND g_info, c_info;       // the input's container (the box above Progress)
static HWND g_prog, c_phase, c_bar, c_pct, c_time, c_jl[NJ], c_jb[NJ], c_jp[NJ];
static HWND c_log, c_status, c_start, c_pause, c_stop;
static int jl_gray[NJ], jp_color[NJ], pct_blue; // colors: a task waiting (gray); a gain (1, blue), failed (2, red)

static wstring out_auto;          // the output name we made from the input's (else the user's)
static wstring out_ok;            // an existing output the user chose to overwrite
static int f_overwrite, f_quit;   // -overwrite: any, without asking; -quit: quit when a run ends well
static int exit_code;             // the last run's, as mp4pack's
static int mode_dec;              // the mode as last set: a click that changes it swaps the files
static int swapping;              // (the input set by that: not a new one)
static string cl_c;               // -c and -i from the command line (no fields for them): for every run
static int cl_cap = -1;
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
struct JobRow { string label; int st; double frac; int est, gain; qword work, made; }; // frac -1: not known; est: a guess; gain: c's or d's
struct Prog { string phase; double frac; qword eta; vector<JobRow> jobs; };
static vector<Prog> prog_q;       //   the progress: each change of a job's state or the phase (none lost when they come faster than the window looks)
static int log_posted, prog_posted;
static Prog prog;                 // the progress shown
static qword prog_at;             // when it came
static double shown;              // the overall bar (it does not go back)
static string result;             // the sizes and the time, when all went well

// a task: a job, or a stream's parts (aacpack's, tangelo's pieces) together
struct Task { string key, label; int st, est, gain; double frac; qword work, made; }; // made: the packed bytes
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
  int gain = (phase=="packing") || (phase=="unpacking"); // (made: what it packed into, or unpacks)
  for( size_t i=0; i<all.size(); i++ ) {
    Job& j = *all[i];
    double f = (j.st==2) ? 1 : (j.st==5) ? j.frac : (j.st!=1) ? 0 : j.prog ? Frac( j ) : -1; // (stopped: where it was)
    if( (j.st==1) && (j.prog==4) && (f==0) ) f = -1; // (measured by what it has written, and nothing yet: ttfcomp x writes at the end)
    JobRow r = { j.label, j.st, f, (j.prog==4) && !f_dec, gain, j.work, j.made };
    p.jobs.push_back( r );
  }
  EnterCriticalSection( &qlock );
  int same = prog_q.size() && (prog_q.back().phase==p.phase) && (prog_q.back().jobs.size()==p.jobs.size());
  for( size_t i=0; same && (i<p.jobs.size()); i++ ) same = (prog_q.back().jobs[i].label==p.jobs[i].label) && (prog_q.back().jobs[i].st==p.jobs[i].st);
  if( same ) std::swap( prog_q.back(), p ); // (only further on: the latest will do)
  else prog_q.push_back( p );
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
//---- bars: when a task is done with a gain, |green|blue|

static double split[NJ+1];        // a bar's green share when done (what it came to; the rest, blue: the gain); -1: as the control draws it. [NJ]: the overall bar
static WNDPROC bar_proc;          // the progress bar's own

// a pixel (0xRRGGBB) in c's hue, its brightness and saturation kept (its
// channels' max and min): a gray one stays as it is
static DWORD Hue( DWORD v, COLORREF c ) {
  int r = (v>>16)&255, g = (v>>8)&255, b = v&255, x = std::max( r, std::max( g, b ) ), n = std::min( r, std::min( g, b ) );
  int cr = GetRValue(c), cg = GetGValue(c), cb = GetBValue(c), cx = std::max( cr, std::max( cg, cb ) ), cn = std::min( cr, std::min( cg, cb ) );
  r = n+(x-n)*(cr-cn)/(cx-cn); g = n+(x-n)*(cg-cn)/(cx-cn); b = n+(x-n)*(cb-cn)/(cx-cn);
  return (v&0xFF000000) | (r<<16) | (g<<8) | b;
}

// the bar as the control draws it (full: its theme's look, in its color),
// in green up to g of its fill and blue after
static void PaintSplit( HWND h, HDC dc, double g ) {
  RECT r; GetClientRect( h, &r );
  int w = r.right, ht = r.bottom;
  if( (w<=0) || (ht<=0) ) return;
  BITMAPINFO bi; memset( &bi, 0, sizeof(bi) );
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -ht;
  bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
  DWORD* px = 0;
  HBITMAP bm = CreateDIBSection( dc, &bi, DIB_RGB_COLORS, (void**)&px, 0, 0 );
  if( !bm || !px ) { if( bm ) DeleteObject( bm ); CallWindowProcW( bar_proc, h, WM_PRINTCLIENT, (WPARAM)dc, PRF_CLIENT ); return; }
  HDC m = CreateCompatibleDC( dc );
  HGDIOBJ o = SelectObject( m, bm );
  FillRect( m, &r, GetSysColorBrush( COLOR_BTNFACE ) ); // (under rounded corners)
  CallWindowProcW( bar_proc, h, WM_PRINTCLIENT, (WPARAM)m, PRF_CLIENT );
  GdiFlush();
  // the fill: where the middle row has color (not the frame's gray)
  int a = -1, z = -1;
  for( int x=0; x<w; x++ ) {
    DWORD v = px[size_t(ht/2)*w+x];
    int c[3] = { int((v>>16)&255), int((v>>8)&255), int(v&255) };
    if( std::max( c[0], std::max( c[1], c[2] ) )-std::min( c[0], std::min( c[1], c[2] ) )>=32 ) { if( a<0 ) a = x; z = x; }
  }
  if( a<0 ) { a = 0; z = w-1; }
  int x0 = a+int( (z-a+1)*g+.5 );
  for( int y=0; y<ht; y++ ) for( int x=0; x<w; x++ ) {
    DWORD& v = px[size_t(y)*w+x];
    v = Hue( v, (x<x0) ? RGB( 6, 176, 37 ) : RGB( 6, 37, 176 ) );
  }
  BitBlt( dc, 0, 0, w, ht, m, 0, 0, SRCCOPY );
  SelectObject( m, o ); DeleteDC( m ); DeleteObject( bm );
}

static LRESULT CALLBACK BarProc( HWND h, UINT msg, WPARAM wp, LPARAM lp ) {
  int i = 0;
  while( (i<NJ) && (c_jb[i]!=h) ) i++; // (none of them: the overall bar)
  if( split[i]>=0 ) {
    if( msg==WM_ERASEBKGND ) return 1;
    if( msg==WM_PAINT ) {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint( h, &ps );
      PaintSplit( h, dc, split[i] );
      EndPaint( h, &ps );
      return 0;
    }
  }
  return CallWindowProcW( bar_proc, h, msg, wp, lp );
}

//---- the Container box: no scroll bars; the left button drags its text

static WNDPROC info_edit;         // the edit control's own procedure
static int drag;                  // the left button holds the box's text
static POINT drag_at;             // where it took it
static int drag_line, drag_x;     // its first line shown and its offset to the left then (pixels)
static int info_w;                // its longest line's width (pixels)

// how far its text is scrolled to the left, in pixels: where the start of
// the first line shown is, from where it is when not scrolled
static int InfoX( HWND h ) {
  RECT r;
  SendMessageW( h, EM_GETRECT, 0, (LPARAM)&r );
  LRESULT i = SendMessageW( h, EM_LINEINDEX, SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ), 0 );
  LRESULT p = (i<0) ? -1 : SendMessageW( h, EM_POSFROMCHAR, WPARAM(i), 0 );
  return (p==-1) ? 0 : int(r.left)-short( LOWORD(p) );
}

// The text follows the mouse while the left button holds it (by lines and
// characters: EM_LINESCROLL), from where it was when taken, so it is where
// the mouse says after it stops at an end too: the last line at the bottom,
// the longest one's end at the right edge. No selection with the mouse then
// (the keyboard selects, the right button's menu copies); the wheel scrolls
// as ever.
static LRESULT CALLBACK InfoProc( HWND h, UINT msg, WPARAM wp, LPARAM lp ) {
  switch( msg ) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
      SetFocus( h ); SetCapture( h );
      drag = 1; drag_at.x = short( LOWORD(lp) ); drag_at.y = short( HIWORD(lp) );
      drag_line = int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ); drag_x = InfoX( h );
      SetCursor( LoadCursorW( 0, (LPCWSTR)IDC_SIZEALL ) );
      return 0;
    case WM_MOUSEMOVE:
      if( !drag ) break;
      if( (mh>0) && (mw>0) ) {
        RECT r;
        SendMessageW( h, EM_GETRECT, 0, (LPARAM)&r );
        int lines = int( SendMessageW( h, EM_GETLINECOUNT, 0, 0 ) ), shown = std::max( int(r.bottom-r.top)/mh, 1 );
        int line = drag_line-(short( HIWORD(lp) )-drag_at.y)/mh, x = drag_x-(short( LOWORD(lp) )-drag_at.x);
        line = std::max( 0, std::min( line, lines-shown ) );
        x = std::max( 0, std::min( x, info_w-int(r.right-r.left) ) );
        int dl = line-int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ), dc = (x-InfoX( h ))/mw;
        if( dl || dc ) {
          SendMessageW( h, EM_LINESCROLL, WPARAM(dc), LPARAM(dl) );
          RECT c; GetClientRect( h, &c ); // (below the lines: drawn anew, not left as it was)
          if( c.bottom>r.bottom ) { c.top = r.bottom; InvalidateRect( h, &c, TRUE ); }
        }
      }
      return 0;
    case WM_LBUTTONUP:
      if( !drag ) break;
      drag = 0;
      if( GetCapture()==h ) ReleaseCapture();
      return 0;
    case WM_CAPTURECHANGED: drag = 0; break;
    case WM_SETCURSOR:
      if( LOWORD(lp)==HTCLIENT ) { SetCursor( LoadCursorW( 0, (LPCWSTR)(drag ? IDC_SIZEALL : IDC_HAND) ) ); return TRUE; }
      break;
  }
  return CallWindowProcW( info_edit, h, msg, wp, lp );
}

static HWND Bar() {
  HWND h = Make( PROGRESS_CLASSW, L"", PBS_SMOOTH );
  SendMessageW( h, PBM_SETRANGE32, 0, 1000 );
  SendMessageW( h, PBM_SETBARCOLOR, 0, RGB( 6, 176, 37 ) ); // (classic bars: green, as in Windows' themes; a theme keeps its own)
  WNDPROC p = (WNDPROC)SetWindowLongPtrW( h, GWLP_WNDPROC, (LONG_PTR)BarProc );
  if( !bar_proc ) bar_proc = p;
  return h;
}

static HWND Combo( int id, DWORD st, const char* const* items, int n ) {
  HWND h = Make( L"COMBOBOX", L"", WS_TABSTOP|WS_VSCROLL|st, id );
  for( int i=0; i<n; i++ ) SendMessageW( h, CB_ADDSTRING, 0, (LPARAM)W(items[i]).c_str() );
  return h;
}

static void Place( HWND h, int x, int y, int w, int ht ) { SetWindowPos( h, 0, x, y, std::max( w, 0 ), std::max( ht, 0 ), SWP_NOZORDER|SWP_NOACTIVATE ); }

static wstring status_text;       // the status line as set (SetStatus), for Layout to fit again
static int status_cut;            // it is cut in the middle to fit

// s, its middle cut out ("...") when it is wider than h: its start, up to a
// third of the width, and as much of its end as fits
static wstring MiddleCut( HWND h, const wstring& s ) {
  RECT rc;
  GetClientRect( h, &rc );
  int w = rc.right-rc.left;
  HDC dc = GetDC( h );
  HGDIOBJ f0 = SelectObject( dc, (HGDIOBJ)SendMessageW( h, WM_GETFONT, 0, 0 ) );
  SIZE z;
  wstring r = s;
  if( (w>0) && GetTextExtentPoint32W( dc, s.c_str(), int(s.size()), &z ) && (z.cx>w) ) {
    int a = 0;
    GetTextExtentExPointW( dc, s.c_str(), int(s.size()), w/3, &a, 0, &z );
    size_t head = size_t( std::max( a, 0 ) ), tail = 0;
    if( head && IS_HIGH_SURROGATE( s[head-1] ) ) head--;
    for( ; head+tail<s.size(); tail++ ) { // (one more character of the end, while it fits)
      wstring t = s.substr( 0, head ) + L"..." + s.substr( s.size()-tail-1 );
      if( !GetTextExtentPoint32W( dc, t.c_str(), int(t.size()), &z ) || (z.cx>w) ) break;
    }
    if( tail && IS_LOW_SURROGATE( s[s.size()-tail] ) ) tail--;
    r = s.substr( 0, head ) + L"..." + s.substr( s.size()-tail );
  }
  SelectObject( dc, f0 );
  ReleaseDC( h, dc );
  return r;
}

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

static void MakeControls() {
  for( int i=0; i<=NJ; i++ ) split[i] = -1;
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
  l_lh = Label( L"video" ); c_lh = Combo( ID_LH, CBS_DROPDOWNLIST, l_items, 10 );
  l_la = Label( L"audio" ); c_la = Combo( ID_LA, CBS_DROPDOWNLIST, l_items, 10 );
  l_lt = Label( L"tangelo" ); c_lt = Combo( ID_LT, CBS_DROPDOWNLIST, l_items, 10 );

  g_info = Group( L"Container" );
  c_info = Make( L"EDIT", L"", WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL, ID_INFO, WS_EX_CLIENTEDGE );
  SendMessageW( c_info, WM_SETFONT, (WPARAM)mono, FALSE );
  info_edit = (WNDPROC)SetWindowLongPtrW( c_info, GWLP_WNDPROC, (LONG_PTR)InfoProc ); // (dragged to scroll)

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
  int lc = DX(50), w1 = TextWidth( L"video" )+sp, w2 = TextWidth( L"audio" )+sp, w3 = TextWidth( L"tangelo" )+sp;
  a = xc;
  Place( l_l, x, y, lw, rh );
  Place( l_lh, a, y, w1, rh ); a += w1; Place( c_lh, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_la, a, y, w2, rh ); a += w2; Place( c_la, a, y, lc, rh+DY(100) ); a += lc+sp*3;
  Place( l_lt, a, y, w3, rh ); a += w3; Place( c_lt, a, y, lc, rh+DY(100) ); y += rh+DY(4);
  Place( g_opt, x0, gy, wl, y-gy ); y += gap;

  gy = y; y += gc; // the input's container: 6 lines
  int ih = 6*mh+2*GetSystemMetrics( SM_CYEDGE )+2; // (the edit's 1 pixel above and below them; no part of a 7th: what is below the lines is not scrolled with them)
  Place( c_info, x0+pad, y, wl-2*pad, ih ); y += ih+DY(5);
  Place( g_info, x0, gy, wl, y-gy ); y += gap;

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
  if( status_cut ) SetText( c_status, MiddleCut( c_status, status_text ) );
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
  return s + " " + Quote( U(run_in) ) + " " + Quote( U(run_out) );
}

//---- the CPU and the tool builds

static const string tool_names[] = { "avip", "tangelo", "h264p", "m4vpack", "aacpack", "ac3pack", "mp3pack", "oggcomp", "ttfcomp" };

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
  for( size_t i=0; i<sizeof(tool_names)/sizeof(tool_names[0]); i++ ) {
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

//---- the input's container: the box above Progress

// A moment after the input is set (T_INFO), avip describes it in its "file"
// mode (the container, its tracks, the files c would make of it), run in the
// background; an mp4pack archive is described by its index instead. The
// files avip would make are shown with what packs each, as the options stand
// (StreamMethod). The last few inputs' descriptions are kept (the mode switch
// swaps the files). Only while no run runs: Start stops a description still
// being made, and it is made when the run ends.

struct Info { wstring path; qword size, time; int kind; string text, arg; }; // kind: 0 being made, 1 avip's (arg: the input as it got it), 2 an archive's, 3 a note
struct InfoJob { LONG gen; wstring path; HANDLE proc, out; };              // for the thread: proc 0: an archive
struct InfoMsg { LONG gen; int kind; string text; };                       // from it
static Info info;                 // the input's
static vector<Info> info_kept;    // the last few made
static volatile LONG info_gen;    // the request's now (an older one's result is dropped)
static HANDLE info_proc;          // its avip
static int info_held;             // to be made when the run ends
static wstring info_shown;        // the box's text

static vector<string> Lines( const string& t ) {
  vector<string> l;
  for( size_t i=0; i<t.size(); ) {
    size_t e = t.find( '\n', i );
    if( e==string::npos ) e = t.size();
    string s = t.substr( i, e-i );
    if( s.size() && (s[s.size()-1]=='\r') ) s.erase( s.size()-1 );
    if( s.size() ) l.push_back( s );
    i = e+1;
  }
  return l;
}

static string Right( const string& s, size_t n ) { return (s.size()<n) ? string( n-s.size(), ' ' )+s : s; }

// an mp4pack archive by its index: the file it was made of, each stream and
// what it came to with its method (the streams in a solid archive together),
// the tools (on the thread: nothing of the run's)
static string ArchiveText( const wstring& path ) {
  HANDLE f = OpenRead( LongPath(path) );
  if( f==INVALID_HANDLE_VALUE ) { DWORD e = GetLastError(); return "cannot read it" + SysErr(e); }
  byte h[16];
  string idx, why;
  vector<string> fn; vector<qword> fz; // its files
  LARGE_INTEGER z, o, pos; z.QuadPart = 0; o.QuadPart = 0;
  GetFileSizeEx( f, &z );
  if( ReadN( f, h, 12 ) || memcmp( h, "mp4pack", 7 ) ) why = "not an mp4pack archive";
  else if( h[7]!=ARC_VERSION ) why = "an mp4pack archive of another version (" + Num(h[7]) + ", not " + Num(ARC_VERSION) + ")";
  qword n = why.size() ? 0 : GetLE( h+8, 4 );
  for( qword i=0; (i<n) && why.empty(); i++ ) {
    string a;
    if( !ReadN( f, h, 2 ) ) a.resize( size_t( GetLE( h, 2 ) ) );
    if( a.empty() || ReadN( f, &a[0], DWORD(a.size()) ) || ReadN( f, h, 8 ) ) { why = "x"; break; }
    qword len = GetLE( h, 8 );
    o.QuadPart = LONGLONG(len);
    if( (a=="mp4pack.idx") && idx.empty() && (len<(1u<<20)) ) { idx.resize( size_t(len) ); if( len && ReadN( f, &idx[0], DWORD(len) ) ) why = "x"; }
    else if( !SetFilePointerEx( f, o, 0, FILE_CURRENT ) ) why = "x";
    fn.push_back( a ); fz.push_back( len );
  }
  o.QuadPart = 0;
  if( why.empty() && (!SetFilePointerEx( f, o, &pos, FILE_CURRENT ) || (pos.QuadPart!=z.QuadPart)) ) why = "x";
  CloseHandle( f );
  if( why=="x" ) why = "not all of an mp4pack archive (cut short, or damaged)";
  if( why.empty() && idx.empty() ) why = "an mp4pack archive without its index";
  if( why.size() ) return why;

  struct St { string name, method; qword size; int parts; vector<string> files; }; // files: its parts' in the archive
  vector<St> st;
  string src, tools;
  vector<string> l = Lines( idx );
  for( size_t k=0; k<l.size(); k++ ) {
    size_t i = 0;
    string key = Field( l[k], i );
    if( key=="src" ) src = l[k].substr( i );
    else if( key=="tools" ) tools = l[k].substr( i );
    else if( key=="stream" ) { St s; qword v = 0; Number( Field( l[k], i ), v ); s.size = v; s.parts = 0; Field( l[k], i ); s.method = Field( l[k], i ); s.name = l[k].substr( i ); st.push_back( s ); }
    else if( (key=="part") && st.size() ) {
      Field( l[k], i ); Field( l[k], i );
      string pk = Field( l[k], i ), pk2 = l[k].substr( i );
      vector<string>& v = st.back().files;
      st.back().parts++;
      if( pk=="-" ) pk = "store/" + st.back().name;
      if( std::find( v.begin(), v.end(), pk )==v.end() ) v.push_back( pk );
      if( pk2.size() && (std::find( fn.begin(), fn.end(), pk2 )!=fn.end()) ) v.push_back( pk2 ); // (mp3pack's: a file; tangelo's: a name in its archive)
    }
  }
  size_t i0 = 0;
  string a = Field( src, i0 ), c = Field( src, i0 );
  qword ssz = 0;
  int ok = !Number( a, ssz );
  string r = "mp4pack archive, version " + Num(ARC_VERSION) + ": " + Num(fn.size()) + " files, " + Num(z.QuadPart) + " bytes"
           + (ok ? " (" + Pct( double(z.QuadPart), double(ssz) ) + ")" : string()) + "\r\n";
  if( ok ) r += "of " + src.substr( i0 ) + ": " + a + " bytes, CRC32 " + c + "\r\n";
  // a stream alone in its files: its line; those in a solid archive: the archive's
  size_t w = 0, wz = 0;
  for( size_t k=0; k<st.size(); k++ ) { w = std::max( w, st[k].name.size() ); wz = std::max( wz, Num( st[k].size ).size() ); }
  for( size_t k=0; k<fn.size(); k++ ) w = std::max( w, fn[k].size() );
  vector<string> done;
  for( size_t k=0; k<st.size(); k++ ) {
    qword p = 0;
    string shared;
    for( size_t q=0; q<st[k].files.size(); q++ ) {
      const string& x = st[k].files[q];
      for( size_t e=0; e<fn.size(); e++ ) if( fn[e]==x ) p += fz[e];
      for( size_t s=0; s<st.size(); s++ ) if( (s!=k) && (std::find( st[s].files.begin(), st[s].files.end(), x )!=st[s].files.end()) ) shared = x;
    }
    if( shared.empty() ) {
      r += Pad( st[k].name, w ) + "  " + Right( Num(st[k].size), wz ) + " -> " + Right( Num(p), wz ) + "  " + st[k].method
         + ((st[k].parts>1) ? ", " + Num(st[k].parts) + " parts" : string()) + "\r\n";
      continue;
    }
    if( std::find( done.begin(), done.end(), shared )!=done.end() ) continue;
    done.push_back( shared );
    qword u = 0, sz = 0;
    string names;
    for( size_t s=0; s<st.size(); s++ ) if( std::find( st[s].files.begin(), st[s].files.end(), shared )!=st[s].files.end() ) { u += st[s].size; names += " " + st[s].name; }
    for( size_t e=0; e<fn.size(); e++ ) if( fn[e]==shared ) sz = fz[e];
    r += Pad( shared, w ) + "  " + Right( Num(u), wz ) + " -> " + Right( Num(sz), wz ) + "  " + st[k].method + ":" + names + "\r\n";
  }
  if( tools.size() ) r += "made with " + tools + "\r\n";
  return r;
}

// avip's description, its lines in this order: the container (without the
// path avip got), what it says of the tracks (and attachments), the files c
// would make, each with what would pack it (packers: as the options stand),
// and last what it says of the container's layout
static string AvipText( const string& t, const string& arg, int packers ) {
  vector<string> l = Lines( t ), fn;
  vector<qword> fz;
  string head, total, body, tail;
  for( size_t i=0; i<l.size(); i++ ) {
    string s = l[i];
    size_t c = s.find( ": " ), d = s.find( " -> " );
    qword v, v2;
    if( i==0 ) { if( s.compare( 0, arg.size()+2, arg+": " )==0 ) s.erase( 0, arg.size()+2 ); head = s; }
    else if( (c!=string::npos) && (s.find( ' ' )==c+1) && !Number( s.substr( c+2 ), v ) ) { fn.push_back( s.substr( 0, c ) ); fz.push_back( v ); } // "0.h264: 41182"
    else if( (d!=string::npos) && !Number( s.substr( 0, d ), v ) && !Number( s.substr( d+4 ), v2 ) ) total = s.substr( d+4 ); // "239558 -> 234618"
    else if( !s.compare( 0, 7, "layout:" ) || !s.compare( 0, 7, "struct:" ) || !s.compare( 0, 10, "chunk ops:" ) ) tail += s + "\r\n";
    else body += s + "\r\n";
  }
  string r = head;
  if( total.size() ) r += "; avip c: " + Num(fn.size()) + ((fn.size()==1) ? " file, " : " files, ") + total + " bytes";
  r += "\r\n" + body;
  size_t w = 0, wz = 0;
  for( size_t i=0; i<fn.size(); i++ ) { w = std::max( w, fn[i].size() ); wz = std::max( wz, Num(fz[i]).size() ); }
  for( size_t i=0; i<fn.size(); i++ ) {
    r += Pad( fn[i], w ) + "  " + Right( Num(fz[i]), wz );
    int nb = 0, m = packers ? StreamMethod( "s."+fn[i], fz[i], &nb ) : -1;
    if( m>=0 ) r += "  " + string( m_name[m] ) + (nb ? string( " (no " ) + m_tool[Method( "s."+fn[i] )] + " build runs here)" : string());
    r += "\r\n";
  }
  return r + tail;
}

// the input's description into the box (the packers as the options stand)
static void InfoShow() {
  string s;
  if( info.path.size() ) {
    if( info.kind==0 ) s = "reading " + U( FileName( info.path ) ) + " ...";
    else if( info.kind==1 ) s = AvipText( info.text, info.arg, !running && !plan_bad );
    else s = info.text;
  }
  wstring w = W(s);
  if( w==info_shown ) return;
  info_shown = w;
  SetWindowTextW( c_info, w.c_str() );
  info_w = 0;
  HDC dc = GetDC( c_info );
  HGDIOBJ f0 = SelectObject( dc, mono );
  for( size_t i=0; i<w.size(); ) {
    size_t e = w.find( L'\r', i );
    if( e==wstring::npos ) e = w.size();
    SIZE z;
    if( (e>i) && GetTextExtentPoint32W( dc, w.c_str()+i, int(e-i), &z ) ) info_w = std::max( info_w, int(z.cx) );
    i = e+2;
  }
  SelectObject( dc, f0 );
  ReleaseDC( c_info, dc );
}

static DWORD WINAPI InfoThread( void* a ) {
  InfoJob* j = (InfoJob*)a;
  InfoMsg* m = new InfoMsg;
  m->gen = j->gen;
  if( !j->proc ) { m->kind = 2; m->text = ArchiveText( j->path ); }
  else {
    string b;
    char t[4096];
    DWORD n = 0, code = 1;
    while( ReadFile( j->out, t, sizeof(t), &n, 0 ) && n ) if( b.size()<(1u<<20) ) b.append( t, n );
    WaitForSingleObject( j->proc, INFINITE );
    GetExitCodeProcess( j->proc, &code );
    CloseHandle( j->out ); CloseHandle( j->proc );
    // (the tools write in the ANSI code page)
    int k = MultiByteToWideChar( CP_ACP, 0, b.data(), int(b.size()), 0, 0 );
    wstring u( std::max( k, 0 ), 0 );
    if( k>0 ) MultiByteToWideChar( CP_ACP, 0, b.data(), int(b.size()), &u[0], k );
    m->kind = code ? 3 : 1; m->text = U(u);
    if( code && m->text.empty() ) m->text = "avip ended with exit code " + ExitCode( code );
  }
  if( !PostMessageW( hwnd, WM_INFO, 0, (LPARAM)m ) ) delete m;
  delete j;
  return 0;
}

static void InfoStop() {
  InterlockedIncrement( &info_gen );
  if( info_proc ) { TerminateProcess( info_proc, 1 ); CloseHandle( info_proc ); info_proc = 0; }
}

// avip's description of info.path begun (or the archive's): avip takes it
// on its command line (ANSI) by its path, else by its name in its directory
// (avip's current one), else by its 8.3 name
static void InfoStart() {
  InfoJob* j = new InfoJob;
  j->gen = info_gen; j->path = info.path; j->proc = 0; j->out = 0;
  byte sig[8];
  HANDLE f = OpenRead( LongPath(info.path) );
  int arc = (f!=INVALID_HANDLE_VALUE) && !ReadN( f, sig, 7 ) && !memcmp( sig, "mp4pack", 7 );
  if( f!=INVALID_HANDLE_VALUE ) CloseHandle( f );
  if( !arc ) {
    const Tool& t = GetTool( "avip" );
    wstring dir = exe_dir, name = FileName( info.path ), a;
    if( AnsiOK( info.path ) ) a = info.path;
    else if( AnsiOK( name ) && (info.path.size()-name.size()<MAX_PATH-2) ) { a = L"." DS + name; dir = info.path.substr( 0, info.path.size()-name.size() ); } // (".\": not "c" or "d", avip's commands)
    else a = ToolName( info.path );
    string why = t.path.empty() ? NoTool( t ) : a.empty() ? "avip cannot take this file's name on its command line (not in the ANSI code page, and no 8.3 name)" : string();
    if( why.empty() ) {
      SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
      HANDLE r = 0, w = 0, nul = CreateFileW( L"NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, 0 );
      PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(pi) );
      if( !CreatePipe( &r, &w, 0, 1<<16 ) ) { DWORD e = GetLastError(); why = "cannot make a pipe" + SysErr(e); r = w = 0; }
      else {
        SetHandleInformation( w, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT ); // (no run runs: no tool starts meanwhile to get it too)
        STARTUPINFOW si; memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = (nul==INVALID_HANDLE_VALUE) ? 0 : nul; si.hStdOutput = w; si.hStdError = w;
        wstring cmd = L"\"" + t.path + L"\" " + W( Quote( U(a) ) );
        vector<wchar_t> cl( cmd.begin(), cmd.end() ); cl.push_back( 0 );
        if( CreateProcessW( t.path.c_str(), &cl[0], 0, 0, TRUE, CREATE_SUSPENDED|BELOW_NORMAL_PRIORITY_CLASS|tool_flags, 0, dir.c_str(), &si, &pi ) ) {
          if( h_jobobj ) AssignProcessToJobObject( h_jobobj, pi.hProcess ); // (it dies with us)
          ResumeThread( pi.hThread ); CloseHandle( pi.hThread );
          DuplicateHandle( GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &info_proc, 0, FALSE, DUPLICATE_SAME_ACCESS );
          j->proc = pi.hProcess; j->out = r; r = 0;
          info.arg = U(a);
        } else { DWORD e = GetLastError(); why = "cannot run " + U(t.path) + SysErr(e); }
      }
      if( w ) CloseHandle( w );
      if( r ) CloseHandle( r );
      if( nul!=INVALID_HANDLE_VALUE ) CloseHandle( nul );
    }
    if( why.size() ) { delete j; info.kind = 3; info.text = why; InfoShow(); return; }
  }
  HANDLE th = CreateThread( 0, 0, InfoThread, j, 0, 0 );
  if( th ) CloseHandle( th );
  else {
    if( j->proc ) { TerminateProcess( j->proc, 1 ); CloseHandle( j->proc ); CloseHandle( j->out ); }
    if( info_proc ) { CloseHandle( info_proc ); info_proc = 0; }
    delete j;
    info.kind = 3; info.text = "cannot start a thread";
  }
  InfoShow();
}

// T_INFO, the input set: its description made, unless it is there for it as
// it is (its path, size and time); while a run runs, when it ends
static void InfoRequest() {
  if( running ) { info_held = 1; return; }
  info_held = 0;
  wstring t = Text( c_in ), p = t.size() ? FullPath( t ) : t;
  WIN32_FILE_ATTRIBUTE_DATA a;
  int ok = p.size() && GetFileAttributesExW( LongPath(p).c_str(), GetFileExInfoStandard, &a ) && !(a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  qword size = ok ? (qword(a.nFileSizeHigh)<<32) | a.nFileSizeLow : 0, time = ok ? FT( a.ftLastWriteTime ) : 0;
  if( ok && (p==info.path) && (size==info.size) && (time==info.time) && (info.kind!=3) ) return;
  InfoStop();
  info = Info();
  if( ok ) {
    info.path = p; info.size = size; info.time = time;
    for( size_t i=0; i<info_kept.size(); i++ ) if( (info_kept[i].path==p) && (info_kept[i].size==size) && (info_kept[i].time==time) ) { info = info_kept[i]; break; }
    if( info.kind==0 ) { InfoStart(); return; }
  }
  InfoShow(); // (none: an empty box)
}

// WM_INFO: a description made (dropped if another is asked for now)
static void InfoDone( InfoMsg* m ) {
  if( m->gen==info_gen ) {
    if( info_proc ) { CloseHandle( info_proc ); info_proc = 0; }
    info.kind = m->kind; info.text = m->text;
    if( m->kind!=3 ) {
      for( size_t i=0; i<info_kept.size(); i++ ) if( info_kept[i].path==info.path ) { info_kept.erase( info_kept.begin()+i ); break; }
      info_kept.insert( info_kept.begin(), info );
      if( info_kept.size()>4 ) info_kept.pop_back();
    }
    InfoShow();
  }
  delete m;
}

// the options' effects, while nothing runs: the threads as they come out (or
// what is not right), and the CPU Info window's text
static void Refresh() {
  if( running ) return;
  cap = cl_cap;
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
    if( cl_c.size() ) p += "; -c " + cl_c;
    if( cl_cap>=0 ) p += string( "; -i " ) + x86::x86_target[cl_cap].name;
  }
  SetText( c_plan, W(p) );
  InvalidateRect( c_plan, 0, TRUE );
  InfoShow(); // (what packs what)
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

// the output named after the input: x.mkv -> x.mkv.mpk (c), x.mkv.mpk ->
// x.mkv (d)
static wstring AutoName( const wstring& f ) {
  return f.empty() ? f : !Checked( c_dec ) ? f + L".mpk" : IsArchive( f ) ? f.substr( 0, f.size()-4 ) : f + L".out";
}

// the output from the input, unless the user chose one
static void AutoOut() {
  wstring o = Text( c_out );
  if( o.size() && (o!=out_auto) ) return;
  out_auto = AutoName( Text( c_in ) );
  SetText( c_out, out_auto );
}

static void Enable();

// a new input: the mode by its name (.mpk: d), and its output
static void InputChanged() {
  if( swapping ) return;
  wstring f = Text( c_in );
  if( f.size() ) { Check( c_dec, IsArchive( f ) ); Check( c_comp, !IsArchive( f ) ); }
  mode_dec = Checked( c_dec );
  AutoOut();
  Enable(); // (Verify and the levels: c only)
}

// Compress or Decompress clicked: when that changes the mode, the files swap
// places (x.mkv -> x.mkv.mpk, then back: x.mkv.mpk -> x.mkv)
static void ModeClicked() {
  int dec = Checked( c_dec );
  if( dec!=mode_dec ) {
    wstring i = Text( c_in ), o = Text( c_out );
    swapping = 1; SetText( c_in, o ); SetText( c_out, i ); swapping = 0;
    out_auto = (i.size() && (AutoName( o )==i)) ? i : wstring(); // (named after it still: so it goes on)
    mode_dec = dec;
  }
  AutoOut();
  Enable();
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
  wchar_t t[128];
  if( running ) swprintf( t, 128, L"%.0f%% %ls - mp4gui", 100*shown, f_pause ? L"paused" : W( (Verifying( prog.phase ) ? "verify: " : "") + prog.phase ).c_str() );
  SetText( hwnd, running ? t : L"mp4gui" );
}

// the status line while it runs
static void RunStatus() {
  SetStatus( f_abort ? "Stopping..." : f_pause ? "Paused." : Verifying( prog.phase ) ? "Verifying: d of the archive..."
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
  vector<Prog> q;
  EnterCriticalSection( &qlock );
  q.swap( prog_q ); prog_posted = 0;
  LeaveCriticalSection( &qlock );
  if( q.empty() ) return;
  for( size_t i=0; i<q.size(); i++ ) { // (each change, in its order)
    Tasks( q[i] );
    double t = Total( q[i].phase, q[i].frac, 1 );
    if( t>=0 ) shown = std::max( shown, t );
  }
  prog = q.back();
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
  if( !f_overwrite && (out!=out_ok) && Exists( LongPath(out) )
      && (MessageBoxW( hwnd, (out + L" exists. Replace it?").c_str(), L"mp4gui", MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 )!=IDYES) ) return;
  Save();
  if( info.path.size() && (info.kind==0) ) { // (its avip would vie with the run's for the file)
    InfoStop();
    info.kind = 3; info.text = "(not read: the run came first; it is when the run ends)";
    info_held = 1;
    InfoShow();
  }
  run_in = in; run_out = out; run_dec = f_dec; run_verify = f_verify;
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
    // the gain of it all: the archive against the file (c: out to in; d: in to out)
    pct_blue = !GetSize( LongPath(run_in), a ) && !GetSize( LongPath(run_out), b ) && a && b;
    double g = !pct_blue ? -1 : run_dec ? double(a)/double(b) : double(b)/double(a);
    SetText( c_pct, pct_blue ? Percent( 1-g ) : Percent( 1 ) );
    Split( NJ, pct_blue ? std::min( g, 1.0 ) : -1 );
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

//---- the command line

// mp4gui's command line: mp4pack's (c or d, its options, input [output]) sets
// the mode, the options given and the files; C or D starts it too. -quit and
// -overwrite are mp4gui's own. Else input [output]: a file dropped on
// mp4gui.exe, "Open with". 1: start
static int CommandLine() {
  int argc = 0, mode = 0;
  wchar_t** argv = CommandLineToArgvW( GetCommandLineW(), &argc );
  if( !argv ) return 0;
  vector<wstring> io;
  string seen, why;
  wstring a1 = (argc>1) ? argv[1] : L"";
  if( (a1==L"c") || (a1==L"d") || (a1==L"C") || (a1==L"D") ) {
    t_spec.clear(); c_spec.clear(); w_base.clear(); cap = -1; f_free = f_keep = f_verify = f_verbose = 0;
    if( ParseArgs( argc, argv, 1, mode, io, seen, why ) || (c_spec.size() && ParseChunks( c_spec.c_str() )) ) {
      if( why.empty() ) why = c_spec.size() ? "bad -c " + c_spec : "mp4gui [c|d|C|D [options] input [output]]";
      Problem( L"The command line: " + W(why) + L"\n\nmp4pack's: c or d, its options, the input and maybe the output. C or D starts at once; "
               L"-quit quits when it is done, -overwrite replaces an existing output without asking." );
      mode = 0; io.clear();
    }
  } else for( int i=1; (i<argc) && (i<3); i++ ) io.push_back( argv[i] );
  LocalFree( argv );
  if( mode ) {
    if( seen.find( 't' )!=string::npos ) { // a -N at the end: keep free
      string t = t_spec;
      int k = 0;
      size_t m = t.rfind( '-' );
      if( (m!=string::npos) && (m+1<t.size()) && (t.find_first_not_of( "0123456789", m+1 )==string::npos) ) { k = atoi( t.c_str()+m+1 ); t.erase( m ); }
      SetText( c_t, W( t.size() ? t : string( "100%,100%" ) ) );
      SetText( c_free, W(Num(k)) );
    }
    if( seen.find( 'f' )!=string::npos ) Check( c_f, 1 );
    if( seen.find( 'l' )!=string::npos ) {
      SendMessageW( c_lh, CB_SETCURSEL, (lv[G_VIDEO]==NO_LEVEL) ? 0 : 10-lv[G_VIDEO], 0 );
      SendMessageW( c_la, CB_SETCURSEL, (lv[G_AUDIO]==NO_LEVEL) ? 0 : 10-lv[G_AUDIO], 0 );
      SendMessageW( c_lt, CB_SETCURSEL, (lv[G_OTHER]==NO_LEVEL) ? 0 : 10-lv[G_OTHER], 0 );
    }
    if( seen.find( 'w' )!=string::npos ) SetText( c_w, w_base );
    if( seen.find( 'k' )!=string::npos ) Check( c_k, 1 );
    if( seen.find( 'V' )!=string::npos ) Check( c_verify, 1 );
    if( seen.find( 'c' )!=string::npos ) cl_c = c_spec;
    if( seen.find( 'i' )!=string::npos ) cl_cap = cap;
    f_quit = seen.find( 'Q' )!=string::npos;
    f_overwrite = seen.find( 'O' )!=string::npos;
  }
  if( io.size() ) SetInput( FullPath( io[0] ) ); // (the mode by its name...)
  if( mode ) { Check( c_dec, (mode|32)=='d' ); Check( c_comp, (mode|32)=='c' ); mode_dec = Checked( c_dec ); } // (...unless it is given)
  if( io.size()>1 ) SetText( c_out, FullPath( io[1] ) );
  else AutoOut();
  return ((mode=='C') || (mode=='D')) && io.size();
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
        case ID_COMP: case ID_DEC: if( code==BN_CLICKED ) ModeClicked(); return 0;
        case ID_IN: if( code==EN_CHANGE ) { SetTimer( h, T_INFO, 300, 0 ); if( !Unquote( c_in ) ) InputChanged(); } return 0;
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
      if( wp==T_INFO ) { KillTimer( h, T_INFO ); InfoRequest(); }
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
    case WM_INFO: InfoDone( (InfoMsg*)lp ); return 0;
    case WM_CLOSE:
      if( running ) {
        if( !closing && (MessageBoxW( h, L"mp4pack is running. Stop it and quit?", L"mp4gui", MB_YESNO|MB_ICONQUESTION )==IDYES) ) { closing = 1; OnStop(); }
        return 0; // (the window goes when it has stopped)
      }
      DestroyWindow( h );
      return 0;
    case WM_DESTROY:
      InfoStop();
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
  SelectObject( dc, mono ); GetTextMetricsW( dc, &tm ); mh = tm.tmHeight; mw = tm.tmAveCharWidth;
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
  int go = CommandLine();
  Refresh();
  Enable();
  ClearProg();
  SetStatus( GetTool( "avip" ).path.size() ? "Ready." : "No avip here: mp4gui.exe goes next to the tools." );
  ShowWindow( hwnd, show );
  UpdateWindow( hwnd );
  if( !running ) SetFocus( Text( c_in ).empty() ? c_in : c_start );
  if( go ) PostMessageW( hwnd, WM_COMMAND, ID_START, 0 ); // (C, D)

  MSG m;
  while( GetMessageW( &m, 0, 0, 0 )>0 ) {
    if( IsDialogMessageW( hwnd, &m ) ) continue; // (Tab, Enter, Alt+the letters)
    TranslateMessage( &m );
    DispatchMessageW( &m );
  }
  if( taskbar ) taskbar->Release();
  CoUninitialize();
  return exit_code;
}
