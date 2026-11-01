// inc/cmdline.inc -- the command line (ParseArgs): mp4pack's, and mp4gui's
//  (which takes C, D, -quit and -overwrite too)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- the command line

// a command line (after the program's name): c or d, options into their
// settings (t_spec, c_spec, the levels, cap, w_base, the flags), at most two
// files into io; seen: the options given (t c l w i v f k, V for -verify).
// gui: mp4gui's, which takes C and D too, and -quit (Q) and -overwrite (O).
// 1 when it is not right: why (empty when the usage tells it)
static int ParseArgs( int argc, wchar_t** argv, int gui, int& mode, vector<wstring>& io, string& seen, string& why ) {
  int bits;
  mode = 0; io.clear(); seen.clear(); why.clear();
  for( int i=1; i<argc; i++ ) {
    wstring a = argv[i];
    if( !mode ) {
      if( (a==L"c") || (a==L"d") || (gui && ((a==L"C") || (a==L"D"))) ) { mode = int(a[0]); continue; }
      return 1;
    }
    if( (a.size()>=2) && (a[0]=='-') ) {
      wchar_t o = a[1];
      if( a==L"-verify" ) { f_verify = 1; seen += 'V'; }
      else if( (o=='t') || (o=='c') || (o=='l') || (o=='w') || (o=='i') ) {
        wstring v = a.substr( 2 );
        if( v.empty() ) { if( ++i>=argc ) return 1; v = argv[i]; }
        if( o=='t' ) t_spec = U(v); else if( o=='c' ) c_spec = U(v);
        else if( o=='l' ) { string ls = U(v); if( ParseLevels( ls.c_str() ) ) { why = "bad -l " + ls + " (up to 4 levels joined by ,: video, audio and tangelo 1 to 9)"; return 1; } }
        else if( o=='w' ) w_base = v;
        else if( (cap = BuildOf( U(v), bits ))<0 ) { why = "bad -i " + U(v) + " (a -march target: haswell, x86-64-v3, znver4 ...)"; return 1; }
        seen += char(o);
      } else if( a==L"-v" ) { f_verbose = 1; seen += 'v'; }
      else if( a==L"-f" ) { f_free = 1; seen += 'f'; }
      else if( a==L"-k" ) { f_keep = 1; seen += 'k'; }
      else if( gui && (a==L"-quit") ) seen += 'Q';
      else if( gui && (a==L"-overwrite") ) seen += 'O';
      else { why = "unknown option " + U(a); return 1; }
      continue;
    }
    if( io.size()>=2 ) return 1;
    io.push_back( a );
  }
  return !mode;
}

