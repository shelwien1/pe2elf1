// inc/decompress.inc -- d: a member of an archive: its files, the unpackers,
//  every stream checked, avip d (a file packed as it is: its stream), the
//  output checked
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

//---- d

// d of the member m of archive in to out
static int Decompress( const wstring& in, const Member& m, const wstring& out ) {
  qword isz = m.size;
  if( SameFile( LongPath(in), LongPath(out) ) ) { Msg( "the output is the input" ); return 1; }
  Msg( "mp4pack d: " + U(in) + (m.off ? " (from " + Num(m.off) + ")" : string()) + " -> " + U(out) );
  if( OutputCheck( out ) || MakeWork() ) return 2;

  Job x; x.label = "extract"; x.prog = 2; x.work = isz;
  int r = ReadArchive( in, m.off, work+L"p" DS, x );
  if( r ) return r;
  string why;
  if( ReadIndex( why ) ) { Msg( U(in) + ": " + why ); return 4; }
  Msg( "restoring " + src_name + " (" + Num(src_size) + " bytes) from " + Num(S.size()) + ((S.size()==1) ? " file" : " files")
     + (src_direct ? ", packed as it is" : "") );
  if( f_verbose && src_tools.size() ) Msg( "packed with " + src_tools );
  SpaceCheck( src_size );
  for( size_t i=0; i<S.size(); i++ ) if( (S[i].m!=M_STORE) && GetTool( m_tool[S[i].m] ).path.empty() ) {
    Msg( NoTool( GetTool( m_tool[S[i].m] ) ) ); return 2;
  }
  // an earlier mp4pack's archive: its aacpack and oggcomp streams are of
  // their builds from before their threads, which the later ones cannot unpack
  for( size_t i=0; i<S.size(); i++ ) if( (src_ver<2) && ((S[i].m==M_AACPACK) || (S[i].m==M_OGGCOMP)) ) {
    Msg( "error: an earlier mp4pack's archive: " + S[i].name + " needs its " + m_name[S[i].m] ); // (short: mp4gui's status line shows it)
    Msg( "  " + GetTool( m_tool[S[i].m] ).file + " cannot unpack what " + m_name[S[i].m] + " packed before it had threads: d this archive with that mp4pack and its tools" );
    return 2;
  }
  if( (r = CheckBuilds())!=0 ) return r;

  // the unpackers
  vector<string> ttf, tgl; // archives, in their order
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    string u = Quote( "u" DS+s.name );
    const Part& p0 = s.part[0];
    if( (s.m==M_H264P) || (s.m==M_M4VPACK) || (s.m==M_AC3PACK) || (s.m==M_AACPACK) || (s.m==M_OGGCOMP) ) {
      // (they take their threads from the stream; aacpack and oggcomp as many
      // as it has parts, of which AudioThreads makes a guess, for the stage)
      int g = m_group[s.m];
      int n = ((s.m==M_AACPACK) || (s.m==M_OGGCOMP)) ? AudioThreads( s.m, s.size, 0 ) : th[g];
      Job* j = NewJob( g, n, string(m_name[s.m]) + " " + s.name, m_tool[s.m] );
      j->args = string("d") + (f_verbose ? "" : " -p") + " " + Quote("p" DS+p0.pk) + " " + u;
      j->prog = 1; j->work = s.size; j->outs.push_back( UPath(s.name) ); j->del.push_back( PPath(p0.pk) );
    } else if( s.m==M_MP3PACK ) {
      Job* j = NewJob( G_AUDIO, th[G_AUDIO], "mp3pack " + s.name, "mp3pack" );
      j->args = string("d") + (f_verbose ? "" : " -p") + " " + Quote("p" DS+p0.pk) + " " + Quote("p" DS+p0.pk2) + " " + u;
      j->prog = 1; j->work = s.size; j->outs.push_back( UPath(s.name) ); j->del.push_back( PPath(p0.pk) ); j->del.push_back( PPath(p0.pk2) );
    } else if( s.m==M_STORE ) {
      if( !MoveFileW( (work+L"p" DS "store" DS+W(s.name)).c_str(), UPath(s.name).c_str() ) ) { Msg( "missing from the archive: " + s.name ); return 4; }
    } else {
      vector<string>& v = (s.m==M_TTFCOMP) ? ttf : tgl;
      for( size_t k=0; k<s.part.size(); k++ ) if( std::find( v.begin(), v.end(), s.part[k].pk )==v.end() ) v.push_back( s.part[k].pk );
    }
  }
  for( size_t a=0; a<ttf.size(); a++ ) { // the fonts under their names, in the archive's order
    Job* j = NewJob( G_FONT, 1, "ttfcomp " + ttf[a], "ttfcomp" );
    j->cwd = "u" DS; j->args = "x " + Quote(".." DS "p" DS+ttf[a]); j->del.push_back( PPath(ttf[a]) );
    for( size_t i=0; i<S.size(); i++ ) if( (S[i].m==M_TTFCOMP) && (S[i].part[0].pk==ttf[a]) ) {
      j->args += " " + Quote(S[i].name); j->work += S[i].size; j->outs.push_back( UPath(S[i].name) );
    }
    j->prog = 3; j->expect = j->work;
  }
  for( size_t a=0; a<tgl.size(); a++ ) { // the files under their names, in the archive's order (the streams')
    Job* j = NewJob( G_OTHER, 1, "tangelo " + tgl[a], "tangelo" );
    j->cwd = "u" DS; j->args = string("x") + (f_verbose ? "" : " -p") + " " + Quote(".." DS "p" DS+tgl[a]); j->prog = 1; j->del.push_back( PPath(tgl[a]) );
    for( size_t i=0; i<S.size(); i++ ) if( S[i].m==M_TANGELO ) for( size_t k=0; k<S[i].part.size(); k++ ) if( S[i].part[k].pk==tgl[a] ) {
      j->args += " " + Quote(S[i].part[k].pk2); j->work += S[i].part[k].len; j->outs.push_back( UPath(S[i].part[k].pk2) );
    }
  }
  for( size_t i=0; i<J.size(); i++ ) { // what each unpacks (the files it removes when done)
    qword z;
    for( size_t k=0; k<J[i]->del.size(); k++ ) if( !GetSize( J[i]->del[k], z ) ) J[i]->made += z;
  }
  if( J.size() ) Msg( "running " + StageText() );
  if( (r = RunStages())!=0 ) return r;

  // tangelo's pieces together; then every stream as it was
  for( size_t i=0; i<S.size(); i++ ) {
    Stream& s = S[i];
    if( (s.m!=M_TANGELO) || ((s.part.size()==1) && (s.part[0].pk2==s.name)) ) continue;
    HANDLE h = OpenWrite( UPath(s.name), CREATE_ALWAYS );
    r = (h==INVALID_HANDLE_VALUE);
    DWORD e = r ? GetLastError() : 0;
    for( size_t k=0; (k<s.part.size()) && !r; k++ ) {
      if( (r = CopyRange( UPath(s.part[k].pk2), 0, s.part[k].len, h ))!=0 ) e = GetLastError();
      if( !f_keep ) Remove( UPath(s.part[k].pk2) );
    }
    if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
    if( r ) return Fail( "cannot put " + s.name + " together" + SysErr(e), 2 );
  }
  for( size_t i=0; i<S.size(); i++ ) {
    uint c; qword z;
    if( FileCrc( UPath(S[i].name), c, z ) ) return Fail( "missing: " + S[i].name, 4 );
    if( (z!=S[i].size) || (c!=S[i].crc) ) {
      Msg( "error: " + S[i].name + " (" + m_name[S[i].m] + ") is not as it was: " + Num(z) + " bytes, CRC " + Hex(c)
         + " instead of " + Num(S[i].size) + ", " + Hex(S[i].crc) );
      return 4;
    }
  }

  // avip d, to the output if it can take its name (or its 8.3 one), else in
  // the work directory, moved after; a file packed as it is: its stream moved
  // there
  if( src_direct ) {
    if( !MoveFileExW( UPath(S[0].name).c_str(), LongPath(out).c_str(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED ) ) {
      DWORD e = GetLastError(); Msg( "cannot move the output to " + U(out) + SysErr(e) ); return 2;
    }
  } else {
    wstring op = out;
    int tmp = 0;
    if( !AnsiOK(out) ) {
      HANDLE h = OpenWrite( LongPath(out), CREATE_ALWAYS );
      if( h!=INVALID_HANDLE_VALUE ) CloseHandle( h );
      op = ToolName( out );
      if( op.empty() ) { op = L"output.tmp"; tmp = 1; }
    }
    Job a; a.label = "avip d"; a.tool = "avip"; a.args = "d u" DS "s " + Quote(U(op));
    a.prog = 3; a.expect = src_size; a.outs.push_back( tmp ? work+op : LongPath(op) );
    r = RunOne( a );
    if( r ) { Remove( LongPath(out) ); return (r==2) ? 5 : 3; }
    if( tmp && !MoveFileExW( (work+op).c_str(), LongPath(out).c_str(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED ) ) {
      DWORD e = GetLastError(); Msg( "cannot move the output to " + U(out) + SysErr(e) ); return 2;
    }
  }
  uint c; qword z;
  if( FileCrc( LongPath(out), c, z ) ) { DWORD e = GetLastError(); return Fail( "cannot read " + U(out) + SysErr(e), 2 ); }
  if( (z!=src_size) || (c!=src_crc) ) {
    Msg( "error: the output is not as the input was: " + Num(z) + " bytes, CRC " + Hex(c) + " instead of " + Num(src_size) + ", " + Hex(src_crc) );
    if( !f_keep ) Remove( LongPath(out) );
    return 4;
  }
  Msg( Num(isz) + " -> " + Num(z) + ", CRC " + Hex(c) + " OK, " + Clock( GetTickCount64()-t_start ) );
  return 0;
}

