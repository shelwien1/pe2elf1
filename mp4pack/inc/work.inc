// inc/work.inc -- the work directory: where it goes (-w, %TEMP%), made,
//  removed (-k: kept)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- the work directory

// a warning when the work directory's disk has less than this free
static void SpaceCheck( qword need ) {
  ULARGE_INTEGER fr;
  if( GetDiskFreeSpaceExW( work.c_str(), &fr, 0, 0 ) && (fr.QuadPart<need) )
    Msg( "warning: " + Num(fr.QuadPart>>20) + "MB free for the work directory, about " + Num(need>>20) + "MB may be needed (-w)" );
}

// where work directories go: -w's, else the temp directory ("": none)
static wstring WorkBase() {
  wstring base;
  if( w_base.size() ) base = FullPath( w_base );
  else {
    wchar_t t[MAX_PATH+2];
    DWORD n = GetTempPathW( MAX_PATH+1, t );
    if( (n==0) || (n>MAX_PATH) ) { Msg( "no temp directory; give one with -w" ); return wstring(); }
    base.assign( t, n );
  }
  if( !IsSep( base[base.size()-1] ) ) base += L"" DS;
  DWORD e = MakeDirs( LongPath(base) );
  if( !IsDir( LongPath(base) ) ) { Msg( "cannot make the " + string( w_base.size() ? "-w" : "temp" ) + " directory " + U(base) + DirErr( base, e ) ); return wstring(); }
  return base;
}

static int MakeWork() {
  wstring base = WorkBase();
  if( base.empty() ) return 1;
  DWORD e = 0;
  for( uint i=0; i<1000; i++ ) {
    wstring d = base + L"mp4pack_" + W(Hex(GetCurrentProcessId())) + L"_" + W(Num(i));
    if( CreateDirectoryW( d.c_str(), 0 ) ) { work = d+L"" DS; break; }
    if( (e = GetLastError())!=ERROR_ALREADY_EXISTS ) break;
  }
  if( work.empty() ) { Msg( "cannot make a work directory in " + U(base) + SysErr(e) ); return 1; }
  if( work.size()>MAX_PATH-64 ) { Msg( "the work directory's path is too long: " + U(work) ); return 1; }
  const wchar_t* sub[] = { L"u", L"p", L"log" };
  for( int i=0; i<3; i++ ) if( !CreateDirectoryW( (work+sub[i]).c_str(), 0 ) ) { e = GetLastError(); Msg( "cannot make " + U(work+sub[i]) + SysErr(e) ); return 1; }
  Msg( "work directory: " + U(work) );
  return 0;
}

// the work directory removed (or kept: -k)
static void DropWork() {
  if( work.empty() ) return;
  if( f_keep ) Msg( "work directory kept: " + U(work) );
  else for( int i=0; i<20; i++ ) { // (a virus scanner may hold on to a file for a moment)
    Remove( work );
    if( !IsDir( work ) ) break;
    if( i==19 ) Msg( "note: cannot remove the work directory " + U(work) );
    else Sleep( 100 );
  }
  work.clear();
}

