// inc/gui_cpu.inc -- the CPU Info window: the CPU, the targets, every tool
//  build; the options' effects shown (Refresh)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

//---- the CPU and the tool builds

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
  for( int i=0; i<N_TOOLS; i++ ) {
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

