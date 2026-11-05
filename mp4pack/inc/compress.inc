// inc/compress.inc -- c: avip c, the packers (PackJobs, PackFailed,
//  StreamMethod), their builds tried first (CheckBuilds, d's too), the
//  output checked first (OutputCheck), the archive
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- c

// the packer jobs for the streams in S
static int PackJobs() {
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    string u = Quote( "u" DS+s.name );
    int si = int(i);
    if( (s.m==M_H264P) || (s.m==M_M4VPACK) || (s.m==M_AC3PACK) || (s.m==M_AACPACK) || (s.m==M_OGGCOMP) ) {
      // one process on its group's threads, in parts (-c, else PartSize's);
      // aacpack and oggcomp: on as many as it has parts (AudioThreads), of
      // their own size unless -c gives one
      int g = m_group[s.m], ao = (s.m==M_AACPACK) || (s.m==M_OGGCOMP);
      string pk = s.name + m_ext[s.m];
      s.t = ao ? AudioThreads( s.m, s.size, ch[G_AUDIO] ) : th[g];
      s.cut = ao ? ch[G_AUDIO] : (s.t>1) ? PartSize( g, s.size, 2 ) : 0;
      Job* j = NewJob( g, s.t, string(m_name[s.m]) + " " + s.name, m_tool[s.m] );
      j->args = "c" + LevelSp(s.m) + " -t " + Num(s.t) + (s.cut ? " -c "+ChunkArg(s.cut) : string()) + (f_verbose ? "" : " -p")
              + " " + u + " " + Quote("p" DS+pk);
      j->prog = 1; j->work = s.size; j->sids.push_back( si ); j->outs.push_back( PPath(pk) );
      s.part.push_back( Part( 0, s.size, pk, "" ) );
    } else if( s.m==M_MP3PACK ) {
      string pk = s.name + m_ext[M_MP3PACK], pk2 = s.name + ".mp3o";
      s.t = th[G_AUDIO];
      Job* j = NewJob( G_AUDIO, s.t, "mp3pack " + s.name, "mp3pack" );
      if( s.t>1 ) s.cut = PartSize( G_AUDIO, s.size, 10 );
      j->args = "c -t " + Num(s.t) + (s.cut ? " -c "+ChunkArg(s.cut) : string()) + (f_verbose ? "" : " -p")
              + " " + u + " " + Quote("p" DS+pk) + " " + Quote("p" DS+pk2);
      j->prog = 1; j->work = s.size; j->sids.push_back( si ); j->outs.push_back( PPath(pk) ); j->outs.push_back( PPath(pk2) );
      s.part.push_back( Part( 0, s.size, pk, pk2 ) );
    } else if( s.m==M_STORE ) {
      CreateDirectoryW( (work+L"p" DS "store").c_str(), 0 );
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"p" DS "store" DS+W(s.name)).c_str() ) ) { DWORD e = GetLastError(); Msg( "cannot move " + U(UPath(s.name)) + SysErr(e) ); return 2; }
      s.part.push_back( Part( 0, s.size, "-", "" ) );
    }
  }
  // the fonts: one solid archive, whatever the threads (more only if their
  // names would not fit on one command line)
  Job* f = 0;
  int nf = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_TTFCOMP ) {
    if( f && (f->args.size()+S[i].name.size()>30000) ) f = 0;
    if( !f ) {
      string pk = nf++ ? "fonts" + Num(nf) + ".ttfc" : "fonts.ttfc";
      f = NewJob( G_FONT, 1, "ttfcomp", "ttfcomp" );
      f->cwd = "u" DS; f->args = "a " + Quote(".." DS "p" DS+pk); f->outs.push_back( PPath(pk) );
    }
    f->args += " " + Quote( S[i].name );
    f->work += S[i].size; f->sids.push_back( int(i) );
    f->prog = 3; f->expect = f->work/100*32; // (fonts come to about a third)
    f->label = "ttfcomp " + Num( f->sids.size() ) + ((f->sids.size()>1) ? " fonts" : " font");
    S[i].part.push_back( Part( 0, S[i].size, U(FileName(f->outs[0])), "" ) );
  }
  // the rest: one solid tangelo archive; with more threads, files over the
  // part size (-c, else a piece for each thread: PartSize) are cut into
  // pieces, an archive for each. tangelo takes them by name, in their own
  // directory (other, otherN)
  int n = th[G_OTHER], na = 0;
  Job* o = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_TANGELO ) {
    Stream& s = S[i];
    qword c = PartSize( G_OTHER, s.size, 0 );
    if( (n>1) && (s.size>c) ) {
      s.cut = c;
      qword k = std::min<qword>( (s.size+c-1)/c, 100000 );
      for( qword q=0; q<k; q++ ) {
        qword a = s.size*q/k, b = s.size*(q+1)/k;
        string d = "other" + Num(++na), pk = d + ".tgl", mem = s.name + ".part" + Num(q);
        wstring wd = work + W(d) + L"" DS;
        CreateDirectoryW( wd.c_str(), 0 );
        HANDLE h = OpenWrite( wd+W(mem), CREATE_ALWAYS );
        int r = (h==INVALID_HANDLE_VALUE) || CopyRange( UPath(s.name), a, b-a, h );
        DWORD e = r ? GetLastError() : 0;
        if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
        if( r ) return Fail( "cannot write " + U(wd+W(mem)) + SysErr(e), 2 );
        Job* j = NewJob( G_OTHER, 1, "tangelo " + s.name + "#" + Num(q), "tangelo" );
        j->cwd = d + DS; j->args = TangeloA( pk ) + " " + Quote(mem);
        j->prog = 1; j->work = b-a; j->outs.push_back( PPath(pk) ); j->del.push_back( wd );
        s.part.push_back( Part( a, b-a, pk, mem ) );
      }
      if( !f_keep ) Remove( UPath(s.name) );
    } else {
      if( !o ) {
        o = NewJob( G_OTHER, 1, "tangelo", "tangelo" );
        CreateDirectoryW( (work+L"other").c_str(), 0 );
        o->cwd = "other" DS; o->args = TangeloA( "other.tgl" ); o->prog = 1; o->outs.push_back( PPath("other.tgl") ); o->del.push_back( work+L"other" DS );
      }
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"other" DS+W(s.name)).c_str() ) ) { DWORD e = GetLastError(); Msg( "cannot move " + U(UPath(s.name)) + SysErr(e) ); return 2; }
      o->args += " " + Quote(s.name); o->work += s.size; o->sids.push_back( int(i) );
      s.part.push_back( Part( 0, s.size, "other.tgl", s.name ) );
    }
  }
  if( o ) o->label = "tangelo " + Num( o->sids.size() ) + ((o->sids.size()>1) ? " files" : " file");
  // the files the tangelo jobs pack are in their own directories now, not in u
  for( size_t i=0; i<J.size(); i++ ) if( J[i]->tool=="tangelo" ) J[i]->sids.clear();
  for( size_t i=0; i<J.size(); i++ ) for( size_t k=0; k<J[i]->sids.size(); k++ ) S[J[i]->sids[k]].refs++;
  return 0;
}

// streams whose packer failed: into one more tangelo archive (stored with -t ...+0)
static int PackFailed() {
  Job* x = 0;
  for( size_t i=0; i<S.size(); i++ ) if( S[i].failed ) {
    Stream& s = S[i];
    for( size_t k=0; k<s.part.size(); k++ ) {
      Remove( PPath(s.part[k].pk) );
      if( s.part[k].pk2.size() ) Remove( PPath(s.part[k].pk2) );
    }
    s.part.clear();
    if( th[G_OTHER]==0 ) {
      CreateDirectoryW( (work+L"p" DS "store").c_str(), 0 );
      if( !MoveFileW( UPath(s.name).c_str(), (work+L"p" DS "store" DS+W(s.name)).c_str() ) ) { DWORD e = GetLastError(); Msg( "cannot move " + U(UPath(s.name)) + SysErr(e) ); return 2; }
      s.m = M_STORE; s.part.push_back( Part( 0, s.size, "-", "" ) );
      continue;
    }
    if( !x ) {
      x = NewJob( G_OTHER, 1, "tangelo (fallback)", "tangelo" );
      CreateDirectoryW( (work+L"fallback").c_str(), 0 );
      x->cwd = "fallback" DS; x->args = TangeloA( "fallback.tgl" ); x->prog = 1; x->outs.push_back( PPath("fallback.tgl") ); x->del.push_back( work+L"fallback" DS );
    }
    if( !MoveFileW( UPath(s.name).c_str(), (work+L"fallback" DS+W(s.name)).c_str() ) ) { DWORD e = GetLastError(); Msg( "cannot move " + U(UPath(s.name)) + SysErr(e) ); return 2; }
    x->args += " " + Quote(s.name); x->work += s.size;
    s.m = M_TANGELO; s.part.push_back( Part( 0, s.size, "fallback.tgl", s.name ) );
  }
  if( !x ) return 0;
  vector<int> g( 1, G_OTHER );
  int r = RunStage( J, g );
  return r ? ((r==2) ? 5 : 3) : 0;
}

// c and d: the builds of the packers of the streams in S, tried on an empty
// file (a build that takes the switches packs it, with an exit code of 0):
// aacpack's and oggcomp's have to take -t (their builds from before it do
// not, and cannot unpack what the later ones packed: mp4pack runs the best
// build for this CPU in its directory, an earlier one too), and for c a level
// -l gives has to be one the build has. 0; else said why, 2 (a build that
// cannot do) or 1 (a level it has not)
static int CheckBuilds() {
  const int ms[] = { M_H264P, M_M4VPACK, M_AACPACK, M_AC3PACK, M_OGGCOMP };
  const string a = " -t 1 mp4pack.none mp4pack.none.out";
  int made = 0, r = 0;
  for( size_t i=0; (i<sizeof(ms)/sizeof(ms[0])) && !r; i++ ) {
    int m = ms[i], l = f_dec ? NO_LEVEL : lv[m_group[m]], mt = (m==M_AACPACK) || (m==M_OGGCOMP), any = 0;
    for( size_t k=0; k<S.size(); k++ ) any |= (S[k].m==m);
    if( !any || ((l==NO_LEVEL) && !mt) ) continue;
    if( !made ) { made = 1; WriteAll( work+L"mp4pack.none", string() ); }
    const string& file = GetTool( m_tool[m] ).file;
    long long b = Probe( m_tool[m], "c" + a );
    if( b<0 ) continue; // (it does not run: its job says why)
    if( b ) { // (no telling its levels then)
      if( mt ) {
        Msg( "error: " + file + " does not take -t: an earlier build?" ); // (short: mp4gui's status line shows it)
        Msg( "  a build of " + string(m_name[m]) + " from before its threads? mp4pack runs the best build in its directory for this CPU: remove the earlier ones" );
        r = 2;
      }
      continue;
    }
    if( (l==NO_LEVEL) || (Probe( m_tool[m], "c -" + Num(l) + a )==0) ) continue;
    string has;
    for( int k=1; k<=9; k++ ) if( Probe( m_tool[m], "c -" + Num(k) + a )==0 ) has += " " + Num(k);
    Msg( "error: " + file + " has no level " + Num(l) + (has.size() ? " (its levels:" + has + ")" : string()) + "; see -l" );
    r = 1;
  }
  if( made ) { Remove( work+L"mp4pack.none" ); Remove( work+L"mp4pack.none.out" ); }
  return r;
}

// c: what packs a file of avip's: its packer by the extension, if its group
// has threads (-t) and the packer a build that runs here (else tangelo;
// nobuild: 1 when for want of a build), tangelo if that has threads, else
// it is stored as it is (an empty one too). mp4gui shows it before a run.
static int StreamMethod( const string& name, qword size, int* nobuild = 0 ) {
  int m = size ? Method(name) : M_STORE;
  if( (m>=M_H264P) && (th[m_group[m]]==0) ) m = M_TANGELO;
  if( (m>=M_H264P) && GetTool( m_tool[m] ).path.empty() ) { if( nobuild ) *nobuild = 1; m = M_TANGELO; }
  if( (m==M_TANGELO) && (th[G_OTHER]==0) ) m = M_STORE;
  return m;
}

// c and d: the input's size; 2 (with the reason) when it cannot be opened
static int InputSize( const wstring& in, qword& size ) {
  if( !GetSize( LongPath(in), size ) ) return 0;
  DWORD e = GetLastError();
  Msg( IsDir( LongPath(in) ) ? "the input is a directory: " + U(in) : "cannot open " + U(in) + SysErr(e) );
  return 2;
}

// c and d: the output's directory made if it is not there, and the output
// tried (a file there is opened for writing, not changed; else one is made
// and removed), so that a run that cannot write its output ends now, not
// after all the work. 0, or 2
static int OutputCheck( const wstring& out ) {
  wstring lp = LongPath(out), d = out.substr( 0, out.size()-FileName(out).size() );
  if( FileName(out).empty() || IsDir( lp ) ) { Msg( "the output is a directory: " + U(out) ); return 2; }
  if( d.size() && !IsDir( LongPath(d) ) ) {
    DWORD e = MakeDirs( LongPath(d) );
    if( IsDir( LongPath(d) ) ) Msg( "made the directory " + U(d) );
    else if( e ) { Msg( "cannot make the directory " + U(d) + DirErr( d, e ) ); out_failed = 1; return 2; } // (else: no drive? as the file's open tells)
  }
  int had = Exists( lp );
  HANDLE f = OpenWrite( lp, had ? OPEN_EXISTING : CREATE_NEW );
  if( f==INVALID_HANDLE_VALUE ) { DWORD e = GetLastError(); Msg( "cannot write " + U(out) + SysErr(e) ); out_failed = 1; return 2; }
  CloseHandle( f );
  if( !had ) Remove( lp );
  return 0;
}

// c of in to out (append: as a member added to the archive out); m: where
// it went in out
static int Compress( const wstring& in, const wstring& out, int append, Member& m ) {
  if( InputSize( in, src_size ) ) return 2;
  if( SameFile( LongPath(in), LongPath(out) ) ) { Msg( "the output is the input" ); return 1; }
  src_name = U( FileName(in) );
  Msg( "mp4pack c: " + U(in) + " (" + Num(src_size) + " bytes) -> " + U(out) );
  if( OutputCheck( out ) || MakeWork() ) return 2;
  SpaceCheck( 2*src_size ); // the streams, and what they are packed into

  // avip c, with the input as it can take its name; meanwhile its CRC32
  wstring ip = ToolName( in );
  if( ip.empty() ) {
    wstring e = FileName( in );
    e = (e.rfind('.')!=wstring::npos) ? e.substr( e.rfind('.') ) : L"";
    ip = L"input" + (AnsiOK(e) && (e.size()<16) ? e : wstring());
    int linked = CreateHardLinkW( (work+ip).c_str(), LongPath(in).c_str(), 0 );
#ifndef _WIN32
    if( !linked ) linked = CreateSymbolicLinkW( (work+ip).c_str(), LongPath(in).c_str(), 0 ); // (another file system)
#endif
    if( !linked ) {
      Msg( "note: avip cannot take the input's name: copying it to the work directory" );
      if( !CopyFileW( LongPath(in).c_str(), (work+ip).c_str(), FALSE ) ) { DWORD e = GetLastError(); Msg( "cannot copy the input to " + U(work+ip) + SysErr(e) ); return 2; }
    }
  }
  CrcTask ct; ct.path = LongPath(in); ct.err = 1;
  HANDLE cth = CreateThread( 0, 0, CrcThread, &ct, 0, 0 );
  if( !cth ) CrcThread( &ct );
  Job a; a.label = "avip c"; a.tool = "avip"; a.args = "c " + Quote(U(ip)) + " u" DS "s";
  int r = RunOne( a );
  if( cth ) { WaitForSingleObject( cth, INFINITE ); CloseHandle( cth ); }
  if( r ) { return (r==2) ? 5 : 3; }
  if( ct.err || (ct.size!=src_size) ) return Fail( "cannot read " + U(in) + SysErr( ct.err ? ct.sys : 0 ), 2 );
  src_crc = ct.crc;
  if( FileName(ip)==ip ) Remove( work+ip ); // (our link or copy)

  vector<wstring> names = ListFiles( work+L"u" DS );
  string nobuild[N_METHOD];
  if( names.empty() ) { Msg( "avip c made no files" ); return 3; }
  Msg( "avip c: " + Num(names.size()) + ((names.size()==1) ? " file, " : " files, ") + Clock(a.t) );
  for( size_t i=0; i<names.size(); i++ ) {
    Stream s;
    s.name = U(names[i]);
    if( !SafeName(s.name) ) { Msg( "avip c made an unexpected file: " + s.name ); return 3; }
    if( FileCrc( UPath(s.name), s.crc, s.size ) ) { DWORD e = GetLastError(); return Fail( "cannot read " + U(UPath(s.name)) + SysErr(e), 2 ); }
    int nb = 0;
    s.m = StreamMethod( s.name, s.size, &nb );
    if( nb ) { string& n = nobuild[Method(s.name)]; n += (n.size() ? ", " : "") + s.name; }
    S.push_back( s );
  }
  for( int m=0; m<N_METHOD; m++ ) if( nobuild[m].size() ) Msg( "note: " + NoTool( GetTool( m_tool[m] ) ) + "; to tangelo" + LevelSp(M_TANGELO) + ": " + nobuild[m] );
  if( (r = CheckBuilds())!=0 ) return r;
  if( (r = PackJobs())!=0 ) return r;

  for( size_t i=0; i<S.size(); i++ ) {
    const Stream& s = S[i];
    string d = m_name[s.m];
    if( s.t ) d += LevelSp(s.m) + " -t " + Num(s.t) + (s.cut ? " -c "+ChunkArg(s.cut) : string());
    if( s.m==M_TANGELO ) d += LevelSp(M_TANGELO) + " (" + s.part[0].pk + ((s.part.size()>1) ? ", " + Num(s.part.size()) + " pieces" : string()) + ")";
    Msg( Pad( "  " + s.name, 20 ) + Right( Num(s.size), 12 ) + "  " + d );
  }
  if( J.size() ) Msg( "running " + StageText() );
  if( (r = RunStages())!=0 ) return r;
  if( (r = PackFailed())!=0 ) return r;

  // the index and all packed, stored as they are: the output
  if( WriteAll( PPath("mp4pack.idx"), IndexText() ) ) { DWORD e = GetLastError(); Msg( "cannot write " + U(PPath("mp4pack.idx")) + SysErr(e) ); return 2; }
  Job f; f.label = "archive"; f.prog = 2; f.work = DirSize( work+L"p" DS );
  if( (r = WriteArchive( work+L"p" DS, out, f, append, m ))!=0 ) return r;
  qword osz = f.made;
  Msg( Num(src_size) + " -> " + Num(osz) + " (" + Pct( double(osz), double(src_size) ) + "), " + Clock( GetTickCount64()-t_start ) );
  return 0;
}

