// inc/gui_controls.inc -- the controls: made, their text, sizes in dialog
//  units; the progress bars (green, then blue for the gain); the status line
//  cut in the middle
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

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

