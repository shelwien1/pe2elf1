// inc/gui_info.inc -- the Container box: what the input holds (avip's
//  description, or an archive's index), made in the background; its text
//  dragged with the mouse
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

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

// the box's text scrolled to this first line and this offset to the left
// (pixels), as far as they go: the last line at the bottom, the longest
// line's end at the right edge (by lines and characters: EM_LINESCROLL)
static void InfoScroll( HWND h, int line, int x ) {
  if( (mh<=0) || (mw<=0) ) return;
  RECT r;
  SendMessageW( h, EM_GETRECT, 0, (LPARAM)&r );
  int lines = int( SendMessageW( h, EM_GETLINECOUNT, 0, 0 ) ), shown = std::max( int(r.bottom-r.top)/mh, 1 );
  line = std::max( 0, std::min( line, lines-shown ) );
  x = std::max( 0, std::min( x, info_w-int(r.right-r.left) ) );
  int dl = line-int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ), dc = (x-InfoX( h ))/mw;
  if( !dl && !dc ) return;
  SendMessageW( h, EM_LINESCROLL, WPARAM(dc), LPARAM(dl) );
  RECT c; GetClientRect( h, &c ); // (below the lines: drawn anew, not left as it was)
  if( c.bottom>r.bottom ) { c.top = r.bottom; InvalidateRect( h, &c, TRUE ); }
}

// the box's selection set, its text staying where it is (EM_SETSEL brings
// the caret's place into view)
static void InfoSelect( HWND h, int a, int z ) {
  int line = int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ), x = InfoX( h );
  SendMessageW( h, EM_SETSEL, WPARAM(a), LPARAM(z) );
  InfoScroll( h, line, x );
}

// The text follows the mouse while the left button holds it, from where it
// was when taken (so it is where the mouse says after it stops at an end
// too). The box has no caret: it shows text, nothing is typed or selected in
// it (no selection with the mouse, nor all of it when the Tab key comes to
// it; Ctrl+A selects it all, and Ctrl+C or the right button's menu copies
// it; a click ends the selection).
// The keys move the text, as the wheel does: the arrows and Page Up/Down,
// Home and End (with Ctrl: the top and the bottom).
static LRESULT CALLBACK InfoProc( HWND h, UINT msg, WPARAM wp, LPARAM lp ) {
  switch( msg ) {
    case WM_SETFOCUS: { LRESULT r = CallWindowProcW( info_edit, h, msg, wp, lp ); DestroyCaret(); return r; } // (the one it makes)
    case WM_GETDLGCODE: return CallWindowProcW( info_edit, h, msg, wp, lp ) & ~LRESULT(DLGC_HASSETSEL);
    case WM_KEYDOWN: {
      int line = int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ), x = InfoX( h ), ctrl = GetKeyState( VK_CONTROL )<0;
      RECT r;
      SendMessageW( h, EM_GETRECT, 0, (LPARAM)&r );
      int page = std::max( int(r.bottom-r.top)/std::max( mh, 1 )-1, 1 ), end = 1<<30;
      switch( wp ) {
        case VK_UP: line--; break;
        case VK_DOWN: line++; break;
        case VK_PRIOR: line -= page; break;
        case VK_NEXT: line += page; break;
        case VK_LEFT: x -= 8*mw; break;
        case VK_RIGHT: x += 8*mw; break;
        case VK_HOME: if( ctrl ) line = 0; else x = 0; break;
        case VK_END: if( ctrl ) line = end; else x = end; break;
        case 'A': if( ctrl ) { InfoSelect( h, 0, -1 ); return 0; } // (else its own: none in wine's)
        default: return CallWindowProcW( info_edit, h, msg, wp, lp ); // (Tab, Ctrl+C ...)
      }
      InfoScroll( h, line, x );
      return 0;
    }
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
      DWORD a = 0, z = 0;
      SetFocus( h );
      SendMessageW( h, EM_GETSEL, (WPARAM)&a, (LPARAM)&z );
      if( a!=z ) InfoSelect( h, 0, 0 );
      SetCapture( h );
      drag = 1; drag_at.x = short( LOWORD(lp) ); drag_at.y = short( HIWORD(lp) );
      drag_line = int( SendMessageW( h, EM_GETFIRSTVISIBLELINE, 0, 0 ) ); drag_x = InfoX( h );
      SetCursor( LoadCursorW( 0, (LPCWSTR)IDC_SIZEALL ) );
      return 0;
    }
    case WM_MOUSEMOVE:
      if( !drag ) break;
      if( mh>0 ) InfoScroll( h, drag_line-(short( HIWORD(lp) )-drag_at.y)/mh, drag_x-(short( LOWORD(lp) )-drag_at.x) );
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

// an mp4pack archive by its index: the file it was made of, each stream and
// what it came to with its method (the streams in a solid archive together),
// the tools (on the thread: nothing of the run's)
static string ArchiveText( const wstring& path ) {
  vector<Member> v;
  string why;
  int bad = ScanArchive( path, v, why );
  if( v.empty() ) return why;
  qword all = 0, src = 0;
  for( size_t i=0; i<v.size(); i++ ) { all += v[i].size; src += v[i].src_size; }
  if( (v.size()>1) || bad ) { // more files: a line for each (what each was and came to)
    string r = "mp4pack archive, version " + Num(v[0].ver) + ": " + Num(v.size()) + " files, " + Num(all) + " bytes"
             + (src ? " (" + Pct( double(all), double(src) ) + ")" : string()) + "\r\n";
    size_t w = 0, wz = 0;
    for( size_t i=0; i<v.size(); i++ ) { w = std::max( w, v[i].src.size() ); wz = std::max( wz, Num( v[i].src_size ).size() ); }
    for( size_t i=0; i<v.size(); i++ )
      r += Pad( v[i].src, w ) + "  " + Right( Num(v[i].src_size), wz ) + " -> " + Right( Num(v[i].size), wz )
         + (v[i].src_size ? " (" + Pct( double(v[i].size), double(v[i].src_size) ) + ")" : string()) + "\r\n";
    if( bad ) r += why + " after these\r\n";
    return r;
  }
  const Member& m = v[0];
  const vector<string>& fn = m.names;
  const vector<qword>& fz = m.sizes;
  Index x;
  if( ParseIndex( m.idx, x, why ) ) return why;
  // each stream's files in the archive: its parts' (stored: store/<name>;
  // mp3pack's two; tangelo's pk2 is a name in its archive)
  struct St { string name, method; qword size; int parts; vector<string> files; };
  vector<St> st;
  for( size_t k=0; k<x.s.size(); k++ ) {
    const Stream& a = x.s[k];
    St e; e.name = a.name; e.method = m_name[a.m]; e.size = a.size; e.parts = int( a.part.size() );
    for( size_t q=0; q<a.part.size(); q++ ) {
      string pk = (a.part[q].pk=="-") ? "store/" + a.name : a.part[q].pk, pk2 = a.part[q].pk2;
      if( std::find( e.files.begin(), e.files.end(), pk )==e.files.end() ) e.files.push_back( pk );
      if( pk2.size() && (std::find( fn.begin(), fn.end(), pk2 )!=fn.end()) ) e.files.push_back( pk2 );
    }
    st.push_back( e );
  }
  string r = "mp4pack archive, version " + Num(m.ver) + ": " + Num(m.size) + " bytes"
           + (x.src_size ? " (" + Pct( double(m.size), double(x.src_size) ) + ")" : string()) + "\r\n"
           + "of " + x.src_name + ": " + Num(x.src_size) + " bytes, CRC32 " + Hex(x.src_crc) + "\r\n";
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
  if( x.tools.size() ) r += "made with " + x.tools + "\r\n";
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
    m->kind = code ? 3 : 1; m->text = U( W( b, CP_ACP ) ); // (the tools write in the ANSI code page)
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
      if( !CreatePipe( &r, &w, 0, 1<<16 ) ) { DWORD e = GetLastError(); why = "cannot make a pipe" + SysErr(e); r = w = 0; }
      else {
        SetHandleInformation( w, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT ); // (no run runs: no tool starts meanwhile to get it too)
        HANDLE hp = Spawn( t.path, Quote( U(a) ), dir, (nul==INVALID_HANDLE_VALUE) ? 0 : nul, w, w, BELOW_NORMAL_PRIORITY_CLASS, why );
        if( hp ) {
          DuplicateHandle( GetCurrentProcess(), hp, GetCurrentProcess(), &info_proc, 0, FALSE, DUPLICATE_SAME_ACCESS );
          j->proc = hp; j->out = r; r = 0;
          info.arg = U(a);
        }
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

