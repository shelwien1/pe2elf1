// inc/gui_files.inc -- the input and the output: the output named after the
//  input, the mode by it, the file dialogs, the work directory's
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the files

// the output named after the input: x.mkv -> x.mkv.mpk (c), x.mkv.mpk ->
// x.mkv (d)
static wstring AutoName( const wstring& f ) {
  return f.empty() ? f : !Checked( c_dec ) ? f + L".mpk" : IsArchive( f ) ? f.substr( 0, f.size()-4 ) : f + L".out";
}

// the output from the input (with a list: the directory of its first file,
// an output of each there), unless the user chose one
static void AutoOut() {
  wstring o = Text( c_out );
  if( o.size() && (o!=out_auto) ) return;
  if( list.size() ) { const wstring& f = list[0].path; out_auto = f.substr( 0, f.size()-FileName( f ).size() ); }
  else out_auto = AutoName( Text( c_in ) );
  SetText( c_out, out_auto );
}

static void Enable();

// a new input: the mode by its name (.mpk: d), and its output
static void InputChanged() {
  if( swapping ) return;
  wstring f = Text( c_in );
  if( f.size() && list.empty() ) { Check( c_dec, IsArchive( f ) ); Check( c_comp, !IsArchive( f ) ); } // (a list's files say it)
  mode_dec = Checked( c_dec );
  AutoOut();
  Enable(); // (Verify and the levels: c only)
}

// Compress or Decompress clicked: when that changes the mode, the files swap
// places (x.mkv -> x.mkv.mpk, then back: x.mkv.mpk -> x.mkv)
static void ModeClicked() {
  int dec = Checked( c_dec );
  if( (dec!=mode_dec) && list.empty() ) { // (a list stays as it is)
    wstring i = Text( c_in ), o = Text( c_out );
    swapping = 1; SetText( c_in, o ); SetText( c_out, i ); swapping = 0;
    out_auto = (i.size() && (AutoName( o )==i)) ? i : wstring(); // (named after it still: so it goes on)
    mode_dec = dec;
  }
  AutoOut();
  Enable();
}

static void SetInput( const wstring& f ) { SetText( c_in, f ); SetFocus( c_start ); }

// the Open dialog (kind 0: the input; 2: files for the list, more at once)
// or the Save one (1: the output), starting where the file is; the files
// chosen (none: cancelled), more sorted by name (the dialog gives them in an
// order of its own)
static vector<wstring> FileDialog( int kind ) {
  vector<wchar_t> f( (kind==2) ? 1<<20 : 32768, 0 );
  wstring cur = (kind==1) ? Text( c_out ) : ((kind==2) && list.size()) ? list.back().path : Text( c_in ), dir;
  if( cur.size() && (cur.size()<f.size()) ) { // its directory, and its name (but for the list)
    wstring full = FullPath( cur ), name = FileName( full );
    dir = full.substr( 0, full.size()-name.size() );
    if( kind!=2 ) wcsncpy( &f[0], name.c_str(), f.size()-1 );
  }
  OPENFILENAMEW o; memset( &o, 0, sizeof(o) );
  o.lStructSize = sizeof(o); o.hwndOwner = hwnd; o.lpstrFile = &f[0]; o.nMaxFile = DWORD(f.size());
  o.lpstrInitialDir = dir.size() ? dir.c_str() : 0;
  o.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_ENABLESIZING|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  vector<wstring> v;
  if( kind!=1 ) {
    o.lpstrFilter = L"Video files and mp4pack archives\0*.mp4;*.m4v;*.mkv;*.avi;*.mpk\0Video files (mp4, mkv, avi)\0*.mp4;*.m4v;*.mkv;*.avi\0"
                    L"mp4pack archives (*.mpk)\0*.mpk\0All files\0*.*\0";
    o.lpstrTitle = kind ? L"Files to add to the list" : L"The file to compress or decompress";
    o.Flags |= OFN_FILEMUSTEXIST|(kind ? OFN_ALLOWMULTISELECT : 0);
    if( !GetOpenFileNameW( &o ) ) return v;
  } else {
    int dec = Checked( c_dec );
    o.lpstrFilter = dec ? L"All files\0*.*\0" : L"mp4pack archives (*.mpk)\0*.mpk\0All files\0*.*\0";
    o.lpstrDefExt = dec ? 0 : L"mpk";
    o.lpstrTitle = dec ? L"The file to restore" : L"The archive to make";
    o.Flags |= OFN_OVERWRITEPROMPT;
    if( !GetSaveFileNameW( &o ) ) return v;
  }
  // one: its path; more: the directory, then their names, each ending with 0, and a 0 after them
  wstring first = &f[0];
  const wchar_t* p = &f[0]+first.size()+1;
  if( (kind!=2) || !*p ) v.push_back( first );
  else {
    if( first.size() && !IsSep( first[first.size()-1] ) ) first += L"\\";
    for( ; *p; p += wcslen( p )+1 ) v.push_back( first + p );
    std::sort( v.begin(), v.end(), []( const wstring& a, const wstring& b ) { return lstrcmpiW( a.c_str(), b.c_str() )<0; } );
  }
  return v;
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

