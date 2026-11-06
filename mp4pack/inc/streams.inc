// inc/streams.inc -- the streams of avip and their parts, the jobs of the
//  packers, and the index (mp4pack.idx: IndexText, ParseIndex, ReadIndex)
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- streams and the index

struct Part {
  qword off, len;   // what of the stream it holds
  string pk;        // the packed file in p, or the archive it is in ("-": p/store/<name>)
  string pk2;       // mp3pack's "outside" file; its name in a tangelo archive
  Part( qword o, qword l, const string& a, const string& b ) : off(o), len(l), pk(a), pk2(b) {}
};

struct Stream {
  string name;      // as avip named it, in u (from the index: checked by SafeName)
  qword size;
  uint crc;
  int m;            // method
  vector<Part> part;
  int refs;         // c: jobs yet to read it
  int failed;       // c: its packer failed; it goes to tangelo instead
  qword cut;        // c: the part size its packer got (0: none)
  int t;            // c: the threads its packer got (0: none of its own, as tangelo's)
  Stream() : size(0), crc(0), m(M_TANGELO), refs(0), failed(0), cut(0), t(0) {}
};

static vector<Stream> S;
static vector<Job*> J;      // the packers (or unpackers)
static string src_name;
static qword src_size;
static uint src_crc;

static wstring UPath( const string& name ) { return work + L"u" DS + W(name); }
static wstring PPath( const string& name ) { return work + L"p" DS + W(name); }

// The one stream of a file avip cannot split (not mp4, mkv or avi), packed
// as it is: "s.0" and the file's extension, lowercase, which names its
// packer (Method); none if that is not 1 to 8 letters and digits.
static string DirectName( const wstring& file ) {
  wstring n = FileName( file );
  size_t d = n.rfind( L'.' );
  string e;
  if( (d!=wstring::npos) && (n.size()-d-1>=1) && (n.size()-d-1<=8) ) for( size_t i=d+1; i<n.size(); i++ ) {
    wchar_t c = n[i];
    if( (c>='A') && (c<='Z') ) c += 'a'-'A';
    if( ((c<'a') || (c>'z')) && ((c<'0') || (c>'9')) ) { e.clear(); break; }
    e += char(c);
  }
  return e.size() ? "s.0." + e : "s.0";
}

static Job* NewJob( int g, int w, const string& label, const string& tool ) {
  Job* j = new Job;
  j->g = g; j->w = std::max( w, 1 ); j->label = label; j->tool = tool;
  J.push_back( j );
  return j;
}

// c: a part of a stream whose packer failed elsewhere is not run
static int Skip( const Job& j ) {
  if( f_dec ) return 0;
  for( size_t i=0; i<j.sids.size(); i++ ) if( S[j.sids[i]].failed ) return 1;
  return 0;
}

// c: a packer failed; its streams get packed by tangelo at the end instead
static int Fallback( Job& j ) {
  if( f_dec || j.sids.empty() || f_abort ) return 0;
  string n;
  for( size_t i=0; i<j.sids.size(); i++ ) { n += (i ? ", " : "") + S[j.sids[i]].name; S[j.sids[i]].failed = 1; S[j.sids[i]].refs--; }
  Msg( "warning: " + j.label + ": " + j.why + "; " + n + " will be packed with tangelo" + LevelSp(M_TANGELO) + " instead" );
  LogTail( j );
  return 1;
}

static void Succeeded( Job& j ) {
  qword sz;
  if( !f_dec ) {
    j.made = 0;
    for( size_t i=0; i<j.outs.size(); i++ ) if( !GetSize( j.outs[i], sz ) ) j.made += sz;
  }
  if( j.report ) {
    string s = "  " + j.label + ": ";
    if( !f_dec ) s += Num(j.work) + " -> " + Num(j.made) + " (" + Pct( double(j.made), double(j.work) ) + ")";
    else s += Num(j.work);
    Msg( s + ", " + Clock(j.t) );
  }
  for( size_t i=0; i<j.sids.size(); i++ ) {
    Stream& s = S[j.sids[i]];
    if( (--s.refs==0) && !s.failed && !f_keep ) Remove( UPath(s.name) );
  }
  if( !f_keep ) for( size_t i=0; i<j.del.size(); i++ ) Remove( j.del[i] );
}

static string src_tools; // d: the tool builds the archive was made with
static int src_ver;       // d: its index's version (1: an earlier mp4pack's, see ParseIndex)
static int src_direct;    // the file is the one stream, packed as it is (avip cannot split it)

static string IndexText() {
  string t = string( "mp4pack " ) + (src_direct ? "3" : "2") + "\nsrc " + Num(src_size) + " " + Hex(src_crc) + " " + src_name + "\ntools";
  for( int i=0; i<n_tools; i++ ) if( tools[i].used ) t += " " + tools[i].file;
  t += src_direct ? "\ndirect\n" : "\n";
  for( size_t i=0; i<S.size(); i++ ) {
    t += "stream " + Num(S[i].size) + " " + Hex(S[i].crc) + " " + m_name[S[i].m] + " " + S[i].name + "\n";
    for( size_t k=0; k<S[i].part.size(); k++ ) {
      const Part& p = S[i].part[k];
      t += "part " + Num(p.off) + " " + Num(p.len) + " " + p.pk + (p.pk2.size() ? " "+p.pk2 : string()) + "\n";
    }
  }
  return t + "end\n";
}

static string Field( const string& l, size_t& i ) {
  size_t e = l.find( ' ', i );
  if( e==string::npos ) e = l.size();
  string f = l.substr( i, e-i );
  i = (e<l.size()) ? e+1 : e;
  return f;
}

static int Number( const string& s, qword& v ) {
  v = 0;
  if( s.empty() || (s.size()>19) ) return 1;
  for( size_t i=0; i<s.size(); i++ ) { if( (s[i]<'0') || (s[i]>'9') ) return 1; v = v*10+(s[i]-'0'); }
  return 0;
}

static int HexNum( const string& s, uint& v ) {
  v = 0;
  if( s.size()!=8 ) return 1;
  for( size_t i=0; i<8; i++ ) {
    char c = s[i];
    int d = ((c>='0') && (c<='9')) ? c-'0' : ((c>='A') && (c<='F')) ? c-'A'+10 : -1;
    if( d<0 ) return 1;
    v = v*16+d;
  }
  return 0;
}

// an index (mp4pack.idx): its version, the file it was made of, the tool
// builds, the streams with their parts
struct Index { int ver, direct; string src_name, tools; qword src_size; uint src_crc; vector<Stream> s; Index() : ver(0), direct(0), src_size(0), src_crc(0) {} };

// an index's text into x; the names are checked, so that none leads out of
// a directory (d: the work directory). 0, or 1 (why). The version is the
// one an mp4pack needs to read it: 3 with "direct" (the file is its one
// stream, packed as it is: avip cannot split it, nor restore it), else 2.
// Version 1 is an mp4pack's from before aacpack and oggcomp had threads: its
// streams of theirs are of their builds of then, which the later ones do not
// unpack (and aacpack's could be in parts, a process for each).
static int ParseIndex( const string& t, Index& x, string& why ) {
  vector<string> L = Lines( t );
  vector<Stream>& S = x.s;
  why = "bad mp4pack.idx";
  x.ver = L.empty() ? 0 : (L[0]=="mp4pack 1") ? 1 : (L[0]=="mp4pack 2") ? 2 : (L[0]=="mp4pack 3") ? 3 : 0;
  if( x.ver==0 ) { why = "not an mp4pack archive, or a later version"; return 1; }
  int f_src = 0, f_end = 0;
  for( size_t n=1; n<L.size() && !f_end; n++ ) {
    const string& l = L[n];
    size_t i = 0;
    string k = Field( l, i );
    if( k=="src" ) {
      if( Number( Field(l,i), x.src_size ) || HexNum( Field(l,i), x.src_crc ) ) return 1;
      x.src_name = l.substr( i ); f_src = 1;
      for( size_t q=0; q<x.src_name.size(); q++ ) if( byte(x.src_name[q])<32 ) x.src_name[q] = '?'; // (only shown)
    } else if( k=="stream" ) {
      Stream s;
      if( Number( Field(l,i), s.size ) || HexNum( Field(l,i), s.crc ) ) return 1;
      string m = Field( l, i );
      for( s.m=0; (s.m<N_METHOD) && (m!=m_name[s.m]); s.m++ );
      s.name = l.substr( i );
      if( (s.m==N_METHOD) || !SafeName(s.name) ) return 1;
      for( size_t q=0; q<S.size(); q++ ) if( lstrcmpiW( W(S[q].name).c_str(), W(s.name).c_str() )==0 ) return 1;
      S.push_back( s );
    } else if( k=="part" ) {
      qword o, z;
      if( S.empty() || Number( Field(l,i), o ) || Number( Field(l,i), z ) ) return 1;
      string a = Field( l, i ), b = l.substr( i );
      if( ((a!="-") || (S.back().m!=M_STORE)) && !SafeName(a) ) return 1;
      if( b.size() && !SafeName(b) ) return 1;
      S.back().part.push_back( Part( o, z, a, b ) );
    } else if( k=="tools" ) { // (only shown)
      x.tools = l.substr( i );
      for( size_t q=0; q<x.tools.size(); q++ ) if( byte(x.tools[q])<32 ) x.tools[q] = '?';
    } else if( (k=="direct") && (i==l.size()) && (x.ver>=3) ) x.direct = 1;
    else if( k=="end" ) f_end = 1;
    else return 1;
  }
  if( !f_src || !f_end ) return 1;
  if( x.direct && ((S.size()!=1) || (S[0].size!=x.src_size) || (S[0].crc!=x.src_crc)) ) return 1; // (the stream is the file)
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    qword o = 0;
    if( s.part.empty() ) return 1;
    for( size_t k=0; k<s.part.size(); k++ ) {
      Part& p = s.part[k];
      if( (p.off!=o) || (p.len>s.size-o) ) return 1;
      if( ((s.m==M_MP3PACK) || (s.m==M_TANGELO)) && p.pk2.empty() ) return 1;
      o += p.len;
    }
    if( o!=s.size ) return 1;
    if( (s.part.size()>1) && (s.m!=M_TANGELO) && ((s.m!=M_AACPACK) || (x.ver>1)) ) return 1;
  }
  return 0;
}

// p\mp4pack.idx into S and src_*
static int ReadIndex( string& why ) {
  string t;
  Index x;
  if( ReadAll( PPath("mp4pack.idx"), t, 1<<26 ) ) { why = "no mp4pack.idx in it: not an mp4pack archive?"; return 1; }
  if( ParseIndex( t, x, why ) ) return 1;
  S = x.s; src_name = x.src_name; src_size = x.src_size; src_crc = x.src_crc; src_tools = x.tools; src_ver = x.ver; src_direct = x.direct;
  return 0;
}

