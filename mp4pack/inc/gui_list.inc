// inc/gui_list.inc -- the List row: files done one after another (shown,
//  added, cleared; a file of its run begins or ends)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the list (the List row)

// The files of the list are done one after another, when Start is clicked
// with them there: those not done yet (all, when all are). The row shows how
// many there are; its drop-down shows them, the next to be done (or the one
// running) highlighted, those done gray, those failed red. "..." adds files
// (the Open dialog, more at once), Clear takes them all away.

// the next to be done (the one running): the first not done; -1: none
static int ListNext() {
  for( size_t i=0; i<list.size(); i++ ) if( list[i].st!=2 ) return int(i);
  return -1;
}

// the row as it is closed: how many, and how far
static wstring ListSummary() {
  if( list.empty() ) return L"no files (\"...\" adds some, or drop them here)";
  int n[4] = { 0, 0, 0, 0 };
  for( size_t i=0; i<list.size(); i++ ) n[list[i].st&3]++;
  string t = Num(list.size()) + ((list.size()==1) ? " file" : " files");
  string a;
  if( n[2] ) a += Num(n[2]) + " done";
  if( n[3] ) a += (a.size() ? ", " : "") + Num(n[3]) + " failed";
  if( n[1] ) a += (a.size() ? ", " : "") + string( "the " ) + Num(ListNext()+1) + (((ListNext()+1)%10==1) && ((ListNext()+1)%100!=11) ? "st" :
                  ((ListNext()+1)%10==2) && ((ListNext()+1)%100!=12) ? "nd" : ((ListNext()+1)%10==3) && ((ListNext()+1)%100!=13) ? "rd" : "th") + " running";
  return W( a.size() ? t + ": " + a : t );
}

// the drop-down's items anew, the next one highlighted
static void ListShow() {
  SendMessageW( c_list, WM_SETREDRAW, FALSE, 0 );
  if( int( SendMessageW( c_list, CB_GETCOUNT, 0, 0 ) )!=int( list.size() ) ) {
    SendMessageW( c_list, CB_RESETCONTENT, 0, 0 );
    for( size_t i=0; i<list.size(); i++ ) SendMessageW( c_list, CB_ADDSTRING, 0, (LPARAM)list[i].path.c_str() );
  }
  SendMessageW( c_list, CB_SETCURSEL, WPARAM( ListNext() ), 0 );
  SendMessageW( c_list, WM_SETREDRAW, TRUE, 0 );
  InvalidateRect( c_list, 0, TRUE );
}

// an item of the drop-down, or the row closed (ODS_COMBOBOXEDIT: how many)
static void ListDraw( const DRAWITEMSTRUCT* d ) {
  int closed = (d->itemState & ODS_COMBOBOXEDIT) || (int(d->itemID)<0) || (d->itemID>=list.size());
  int sel = (d->itemState & ODS_SELECTED) && !(closed && !(d->itemState & ODS_FOCUS));
  COLORREF fg = GetSysColor( sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT );
  wstring t;
  if( closed ) {
    t = ListSummary();
    if( list.empty() || !IsWindowEnabled( c_list ) ) fg = GetSysColor( COLOR_GRAYTEXT );
  } else {
    const LItem& e = list[d->itemID];
    t = e.path + ((e.st==3) ? L"  (failed)" : (e.st==1) ? L"  (running)" : L"");
    if( !sel && (e.st==2) ) fg = GetSysColor( COLOR_GRAYTEXT );
    if( !sel && (e.st==3) ) fg = RGB( 192, 0, 0 );
  }
  FillRect( d->hDC, &d->rcItem, GetSysColorBrush( sel ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
  RECT r = d->rcItem; r.left += DX(2); r.right -= DX(1);
  SetBkMode( d->hDC, TRANSPARENT );
  SetTextColor( d->hDC, fg );
  vector<wchar_t> b( t.begin(), t.end() ); b.push_back( 0 ); // (DT_PATH_ELLIPSIS: in the middle of the path, the name kept)
  DrawTextW( d->hDC, &b[0], -1, &r, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|(closed ? DT_END_ELLIPSIS : DT_PATH_ELLIPSIS) );
  if( closed && (d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT) ) DrawFocusRect( d->hDC, &d->rcItem );
}

// files added to the list (each once), in the order given; the mode by
// them (all archives: Decompress; none: Compress), and the output
static void ListAdd( const vector<wstring>& f ) {
  for( size_t i=0; i<f.size(); i++ ) {
    wstring p = FullPath( f[i] );
    int in = 0;
    for( size_t k=0; (k<list.size()) && !in; k++ ) in = !lstrcmpiW( list[k].path.c_str(), p.c_str() );
    if( !in ) { LItem e = { p, 0 }; list.push_back( e ); }
  }
  size_t arc = 0;
  for( size_t k=0; k<list.size(); k++ ) arc += IsArchive( list[k].path );
  if( list.size() && ((arc==list.size()) || !arc) ) { Check( c_dec, arc!=0 ); Check( c_comp, !arc ); }
  mode_dec = Checked( c_dec );
  ListShow();
  AutoOut();
  Enable();
}

static void ListClear() {
  list.clear();
  ListShow();
  AutoOut(); // (the input's again)
  Enable();
}

// "...": files for the list, from the Open dialog (more at once), sorted by
// name (the dialog gives them in an order of its own)
static void ListBrowse() {
  vector<wstring> v = FileDialog( 2 );
  if( v.size() ) ListAdd( v );
}

static void ClearProg();

// a file of the list's run begins (r -1: it is the next, in Input; the
// progress anew) or has ended (r: done, failed, or waiting again: stopped)
static void ListFile( int k, int r ) {
  if( !list_run || (k<0) || (k>=int( run_list.size() )) || (run_list[k]>=int( list.size() )) ) return;
  LItem& e = list[run_list[k]];
  if( r<0 ) {
    e.st = 1; run_k = k;
    swapping = 1; SetText( c_in, e.path ); swapping = 0;
    if( k ) ClearProg();
  } else e.st = !r ? 2 : (r==5) ? 0 : 3;
  ListShow();
}

