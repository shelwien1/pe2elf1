// inc/strings.inc -- strings: UTF-8 and wide (W, U), numbers, percentages,
//  times, lines, columns, the system's reasons for errors (SysErr), command
//  line quoting
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- strings

// UTF-8 (or another code page: CP_ACP, the tools' output and ANSI lists) to
// wide (on Linux UTF-8 only: the names are that)
static wstring W( const string& s, UINT cp = CP_UTF8 ) {
  if( s.empty() ) return wstring();
  int n = MultiByteToWideChar( cp, 0, s.data(), int(s.size()), 0, 0 );
  wstring w( std::max( n, 0 ), 0 );
  if( n>0 ) MultiByteToWideChar( cp, 0, s.data(), int(s.size()), &w[0], n );
  return w;
}

static string U( const wstring& w ) {
  if( w.empty() ) return string();
  int n = WideCharToMultiByte( CP_UTF8, 0, w.data(), int(w.size()), 0, 0, 0, 0 );
  string s( n, 0 );
  if( n>0 ) WideCharToMultiByte( CP_UTF8, 0, w.data(), int(w.size()), &s[0], n, 0, 0 );
  return s;
}

static string Num( qword v ) {
  char b[32]; int i = 31; b[i] = 0;
  do b[--i] = char('0'+v%10); while( v/=10 );
  return b+i;
}

static string Hex( uint v ) { char b[16]; snprintf( b, sizeof(b), "%08X", v ); return b; }

static string Pct( double a, double b ) { char s[32]; snprintf( s, sizeof(s), "%.2f%%", (b>0) ? 100*a/b : 0.0 ); return s; }

// the lines of a text (LF or CRLF), the empty ones left out
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

// s in a column n wide: on the left, on the right
static string Pad( string s, size_t n ) { if( s.size()<n ) s.append( n-s.size(), ' ' ); return s; }
static string Right( const string& s, size_t n ) { return (s.size()<n) ? string( n-s.size(), ' ' )+s : s; }

static string Clock( qword ms ) {
  uint s = uint( ms/1000 ); char b[32];
  snprintf( b, sizeof(b), "%u:%02u:%02u", s/3600, s/60%60, s%60 );
  return b;
}

// the speed of bytes in ms: MB/s (MB: 2^20 bytes, as -c's), two places, or
// two figures below 0.01 (a small file's)
static string Speed( qword bytes, qword ms ) {
  double v = double(bytes)/double(1<<20)/(double( std::max<qword>( ms, 1 ) )/1000);
  char b[32];
  snprintf( b, sizeof(b), (v>=0.01) ? "%.2f MB/s" : "%.2g MB/s", v );
  return b;
}

static void Msg( const string& s );

// an error message (none after Ctrl+C: main says so) and the exit code
static int Fail( const string& m, int code ) {
  if( f_abort ) return 5;
  Msg( m );
  return code;
}

static string ExitCode( DWORD c ) { return (c>=0xC0000000u) ? "0x"+Hex(c) : Num(c); }

// the system's reason for error e (GetLastError's, taken right after the
// call that failed), as ": reason" to end a message with; "" for none
static string SysErr( DWORD e ) {
  if( e==0 ) return string();
  wchar_t b[512];
  DWORD n = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, e, 0, b, 512, 0 );
  if( n>511 ) n = 0;
  while( n && ((b[n-1]==L'\r') || (b[n-1]==L'\n') || (b[n-1]==L' ') || (b[n-1]==L'.')) ) n--;
  wstring w( b, n );
  for( size_t i; (i = w.find( L"%1" ))!=wstring::npos; ) w.replace( i, 2, L"it" ); // ("%1 is not a valid Win32 application")
  return ": " + (n ? U(w) : "error " + Num(e));
}

// a command line argument, as the MS C runtime splits them
static string Quote( const string& a ) {
  if( !a.empty() && (a.find_first_of( " \t\"" )==string::npos) ) return a;
  string r = "\"";
  size_t nb = 0;
  for( size_t i=0; i<a.size(); i++ ) {
    if( a[i]=='\\' ) { nb++; continue; }
    r.append( (a[i]=='"') ? nb*2+1 : nb, '\\' );
    nb = 0; r += a[i];
  }
  r.append( nb*2, '\\' );
  return r+"\"";
}

