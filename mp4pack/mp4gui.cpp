// mp4gui.cpp -- mp4pack in a window (Windows): mp4pack's engine itself
// (inc/mp4pack.inc), with the files (Open/Save dialogs, or dropped on the
// window), its options, and while
// it runs the progress of each of its tasks (avip, each stream's packer, the
// archive) and of the whole (also in the taskbar), the gain of each task when
// it is done (its bar green, then blue for the gain), and its messages, in
// place of the console. The List row holds files to do one after another
// (mp4pack's @list); the Container box shows what the input holds (avip's
// description of it, or an archive's index). CPU Info shows the CPU and
// the tool builds. It takes mp4pack's command line too: c or d sets
// everything up, C or D starts it; -quit: it quits when that is done,
// -overwrite: an existing output is replaced without asking.
//
// RunList, mp4pack's whole run, works on a thread of its own here; msg_hook,
// status_hook and file_hook bring its messages, progress and a list's files
// over. Stop sets its abort flag as Ctrl+C does, Pause its f_pause (the tools
// are held, not ended). The settings are kept in mp4gui.ini beside it. This
// file is the window's main: its parts are in inc/gui_*.inc, by purpose.
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
#include "inc/mp4pack.inc" // (mp4pack's engine)
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "inc/gui_state.inc"
#include "inc/gui_hooks.inc"
#include "inc/gui_taskbar.inc"
#include "inc/gui_controls.inc"
#include "inc/gui_info.inc"
#include "inc/gui_layout.inc"
#include "inc/gui_settings.inc"
#include "inc/gui_cpu.inc"
#include "inc/gui_files.inc"
#include "inc/gui_list.inc"
#include "inc/gui_progress.inc"
#include "inc/gui_run.inc"
#include "inc/gui_cmdline.inc"

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
        case ID_INB: { vector<wstring> f = FileDialog( 0 ); if( f.size() ) SetInput( f[0] ); return 0; }
        case ID_OUTB: { vector<wstring> f = FileDialog( 1 ); if( f.size() ) { out_ok = f[0]; SetText( c_out, f[0] ); } return 0; }
        case ID_WB: BrowseWork(); return 0;
        case ID_WTEMP: SetText( c_w, L"%TEMP%" ); return 0;
        case ID_WOUT: WorkByOutput(); return 0;
        case ID_COMP: case ID_DEC: if( code==BN_CLICKED ) ModeClicked(); return 0;
        case ID_LIST: // (a view of it: the next one stays the one highlighted)
          if( (code==CBN_CLOSEUP) || ((code==CBN_SELCHANGE) && !SendMessageW( c_list, CB_GETDROPPEDSTATE, 0, 0 )) ) SendMessageW( c_list, CB_SETCURSEL, WPARAM( ListNext() ), 0 );
          return 0;
        case ID_LISTB: ListBrowse(); return 0;
        case ID_LISTCLR: ListClear(); return 0;
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
    case WM_DROPFILES: { // one: the input (unless there is a list); more: to the list
      HDROP d = (HDROP)wp;
      UINT k = DragQueryFileW( d, 0xFFFFFFFF, 0, 0 );
      vector<wstring> v;
      for( UINT i=0; (i<k) && !running; i++ ) {
        UINT n = DragQueryFileW( d, i, 0, 0 );
        vector<wchar_t> f( n+1 );
        DragQueryFileW( d, i, &f[0], n+1 );
        v.push_back( &f[0] );
      }
      DragFinish( d );
      if( (v.size()==1) && list.empty() ) SetInput( v[0] );
      else if( v.size() ) ListAdd( v );
      return 0;
    }
    case WM_MEASUREITEM: {
      MEASUREITEMSTRUCT* m = (MEASUREITEMSTRUCT*)lp;
      if( m->CtlID!=ID_LIST ) break;
      m->itemHeight = (int(m->itemID)==-1) ? std::max( rh-6, by ) : by+DY(3); // (closed: as high as the rest of a row)
      return TRUE;
    }
    case WM_DRAWITEM:
      if( ((DRAWITEMSTRUCT*)lp)->CtlID!=ID_LIST ) break;
      ListDraw( (DRAWITEMSTRUCT*)lp );
      return TRUE;
    case WM_LOG: ShowLog(); return 0;
    case WM_PROG: if( running ) ShowProg(); return 0;
    case WM_DONE: OnDone( int(wp) ); return 0;
    case WM_INFO: InfoDone( (InfoMsg*)lp ); return 0;
    case WM_NEXTDLGCTL: { // (the Tab key in a multiline edit, the log or the box: on, as in a dialog)
      HWND n = LOWORD(lp) ? (HWND)wp : GetNextDlgTabItem( h, GetFocus(), wp!=0 );
      if( n ) SetFocus( n );
      return 0;
    }
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
  msg_hook = HookMsg; status_hook = HookStatus; file_hook = HookFile;
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
  ListShow();
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
