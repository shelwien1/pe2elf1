// inc/gui_cmdline.inc -- mp4gui's command line (mp4pack's: c or d sets it all
//  up, C or D starts it too)
// (a part of mp4gui: mp4gui.cpp includes it, in its order)

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
  if( io.size() && (io[0].size()>1) && (io[0][0]=='@') ) { // a list: to the List
    vector<wstring> v;
    if( ReadList( io[0].substr( 1 ), v, why ) || v.empty() ) { Problem( L"The command line: " + (v.empty() && why.empty() ? L"no files in the list " + io[0].substr( 1 ) : W(why)) ); io.clear(); mode = 0; }
    else ListAdd( v );
  } else if( io.size() ) SetInput( FullPath( io[0] ) ); // (the mode by its name...)
  if( mode ) { Check( c_dec, (mode|32)=='d' ); Check( c_comp, (mode|32)=='c' ); mode_dec = Checked( c_dec ); } // (...unless it is given)
  if( io.size()>1 ) SetText( c_out, FullPath( io[1] ) );
  else AutoOut();
  return ((mode=='C') || (mode=='D')) && io.size();
}

