// inc/lists.inc -- lists (@list: ReadList), and the names of the outputs in a
//  directory (UniqueName, ArchiveOut, MemberOuts)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- lists, and the names of the outputs

// the files of a list (@list): one on each line, as on the command line
// (relative names: from the current directory). UTF-8, or UTF-16 with its
// BOM; else (on Windows) the ANSI code page, or the OEM one where only that
// names a file that is there ("dir /b > list" in a console writes that).
// Blank lines are left out, and spaces and quotes around a name. 0, or 1
// with why
static int ReadList( const wstring& path, vector<wstring>& v, string& why ) {
  string t;
  qword sz;
  v.clear();
  if( GetSize( LongPath(path), sz ) ) { DWORD e = GetLastError(); why = "cannot read the list " + U(path) + SysErr(e); return 1; }
  if( sz>(64u<<20) ) { why = "the list " + U(path) + " is too big for one (" + Num(sz) + " bytes)"; return 1; }
  if( ReadAll( LongPath(path), t, 64u<<20 ) ) { DWORD e = GetLastError(); why = "cannot read the list " + U(path) + SysErr(e); return 1; }
  vector<wstring> raw;      // its lines
  vector<string> bytes;     // the same as they are (ANSI), to try the OEM code page on
  int u16 = (t.size()>=2) && (byte(t[0])+byte(t[1])==0xFF+0xFE) && ((byte(t[0])==0xFF) || (byte(t[0])==0xFE));
  if( u16 ) { // UTF-16 (LE: FF FE, BE: FE FF), to UTF-8 and on
    int be = (byte(t[0])==0xFE);
    wstring w;
    for( size_t i=2; i+1<t.size(); i+=2 ) {
      unsigned c = be ? (byte(t[i])<<8)|byte(t[i+1]) : byte(t[i])|(byte(t[i+1])<<8);
      if( (sizeof(wchar_t)==4) && (c>=0xD800) && (c<0xDC00) && (i+3<t.size()) ) { // (Linux: a pair is one)
        unsigned d = be ? (byte(t[i+2])<<8)|byte(t[i+3]) : byte(t[i+2])|(byte(t[i+3])<<8);
        if( (d>=0xDC00) && (d<0xE000) ) { c = 0x10000+((c-0xD800)<<10)+(d-0xDC00); i += 2; }
      }
      w += wchar_t(c);
    }
    t = U(w);
  } else if( (t.size()>=3) && (byte(t[0])==0xEF) && (byte(t[1])==0xBB) && (byte(t[2])==0xBF) ) t.erase( 0, 3 );
  // UTF-8 when it is that (on Linux: always, as the names are)
  int utf8 = 1;
#ifdef _WIN32
  if( !u16 ) utf8 = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, t.data(), int(t.size()), 0, 0 )>0 || t.empty();
#endif
  for( size_t i=0; i<t.size(); ) {
    size_t e = t.find( '\n', i );
    if( e==string::npos ) e = t.size();
    string l = t.substr( i, e-i );
    i = e+1;
    while( l.size() && strchr( "\r\t ", l[l.size()-1] ) ) l.erase( l.size()-1 );
    while( l.size() && strchr( "\t ", l[0] ) ) l.erase( 0, 1 );
    if( (l.size()>=2) && (l[0]=='"') && (l[l.size()-1]=='"') ) l = l.substr( 1, l.size()-2 );
    if( l.empty() ) continue;
    bytes.push_back( l );
    if( utf8 ) raw.push_back( W(l) );
#ifdef _WIN32
    else {
      wstring w[2] = { W( l, CP_ACP ), W( l, CP_OEMCP ) };
      raw.push_back( (!Exists( LongPath( FullPath(w[0]) ) ) && Exists( LongPath( FullPath(w[1]) ) )) ? w[1] : w[0] );
    }
#endif
  }
  v = raw;
  return 0;
}

// name, unless it is in used already (an output of this run in a directory,
// which the next would write over): then "name (2).ext" ...; added to used
static wstring UniqueName( const wstring& name, vector<wstring>& used ) {
  wstring r = name;
  size_t d = name.rfind( L'.' );
  if( (d==wstring::npos) || (d==0) ) d = name.size();
  for( int k=2; ; k++ ) {
    int in = 0;
    for( size_t i=0; (i<used.size()) && !in; i++ ) in = (lstrcmpiW( used[i].c_str(), r.c_str() )==0);
    if( !in ) break;
    r = name.substr( 0, d ) + L" (" + W(Num(k)) + L")" + name.substr( d );
  }
  used.push_back( r );
  return r;
}

// c: the archive of in: out, or in the directory out (dir; ending with a
// separator) one named after it (x.mkv -> x.mkv.mpk); used: the names given
// there in the run, so that none is given twice
static wstring ArchiveOut( const wstring& in, const wstring& out, int dir, vector<wstring>& used ) {
  return dir ? out + UniqueName( FileName(in), used ) + L".mpk" : out;
}

// d: the outputs of the members v of archive in: out (one member), or files
// in the directory out (dir; always for more members): named after the
// archive when it has one (x.mkv.mpk -> x.mkv), else as each was (its index;
// file<k> for a name that is not allowed)
static vector<wstring> MemberOuts( const wstring& in, const vector<Member>& v, const wstring& out, int dir, vector<wstring>& used ) {
  vector<wstring> o;
  wstring d = out;
  if( v.size()>1 ) dir = 1;
  if( dir && d.size() && !IsSep( d[d.size()-1] ) ) d += L"" DS;
  for( size_t k=0; k<v.size(); k++ ) {
    wstring name = FileName( in );
    if( !dir ) { o.push_back( out ); continue; }
    if( (v.size()==1) && IsArchive( name ) ) name.erase( name.size()-4 );
    else name = SafeName( v[k].src ) ? W( v[k].src ) : W( "file" + Num(k+1) );
    o.push_back( d + UniqueName( name, used ) );
  }
  return o;
}

