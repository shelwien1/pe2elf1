// inc/messages.inc -- the messages: to the console or a frontend (msg_hook);
//  the last one, and a run's error and summary
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- console

static void Put( string s ) {
  DWORD n;
  if( f_con ) { wstring w = W(s); WriteConsoleW( h_err, w.data(), DWORD(w.size()), &n, 0 ); return; }
#ifdef _WIN32
  for( size_t i=0; (i=s.find('\n',i))!=string::npos; i+=2 ) s.insert( i, "\r" ); // (a file or pipe)
#endif
  WriteFile( h_err, s.data(), DWORD(s.size()), &n, 0 );
}

// a frontend in this program (mp4gui) takes the messages here instead
static void (*msg_hook)( const string& s );
static string last_msg;   // the last message that is not indented (not a tool's output: "  | ...")
static string run_err;    // what made a run fail: the last such message when it did (the first file's that failed; mp4gui shows it)
static string run_summary;// a list's: how it went (its last line)
static qword bytes_in, bytes_out; // a list's: the bytes of the inputs and outputs that went well
static int out_failed;    // the output cannot be written: a list stops

static void Msg( const string& s ) {
  if( s.size() && (s[0]!=' ') ) last_msg = s;
  if( msg_hook ) { msg_hook( s ); return; }
  if( status_len ) { Put( "\r"+string(status_len,' ')+"\r" ); status_len = 0; }
  Put( s+"\n" );
}

static size_t ConWidth() {
  CONSOLE_SCREEN_BUFFER_INFO ci;
  if( GetConsoleScreenBufferInfo( h_err, &ci ) && (ci.dwSize.X>20) ) return ci.dwSize.X;
  return 80;
}

