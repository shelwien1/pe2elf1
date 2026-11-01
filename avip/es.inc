// Elementary streams: a raw video stream (MPEG-4 Part 2 .m4v, MPEG-1/2 .m1v/.m2v, H.264, HEVC)
// is the single track of the file, as it is; this names it for the compressor that takes it.
//
// meta: vint ntracks (1), common head (frames: the file in pieces of up to 2^30 bytes, sizes
//   explicit); no ops, no struct
namespace es {

struct DTrack { TrackHead h; };
struct Meta { std::vector<DTrack> trk; };

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_ES ) return false;
  Reader r( m.data()+1, m.size()-1 );
  qword nt = r.vint(); if( r.err || nt!=1 ) return false;
  M.trk.resize( 1 );
  const TrackHead& h = M.trk[0].h;
  return GetHead( r, M.trk[0].h ) && h.xform==XF_NONE && h.szmode==SZ_EXPLICIT && r.p==r.e;
}

static std::string TrackName( const Meta& M, size_t i ) { return TrackSuffix( i, M.trk[i].h.ext ); }

static bool Decode( Meta& M, const std::vector<Buf>& S, Sink& out, const char** why ) {
  qword n = 0; const TrackHead& h = M.trk[0].h;
  for( size_t k=0; k<h.col.size(); k++ ) n += h.col[k];
  if( S.size()!=1 || n!=S[0].size() ) { *why = "stream size mismatch"; return false; }
  out.put( S[0].data(), S[0].size() );
  if( out.err || (out.ref && out.pos!=out.ref->size()) ) { *why = out.ref ? "output differs" : "write error"; return false; }
  return true;
}

// the kind of a raw video stream at d (the extension of its track), "" if it isn't one: it
// starts with a start code, and has no system start codes (packs, PES packets) in its first
// 256 KB (MPEG-4 Part 2 uses some of their values for objects that video streams don't have)
static std::string Kind( const byte* d, qword n ) {
  if( n<5 || d[0] || d[1] || !(d[2]==1 || (d[2]==0 && d[3]==1)) ) return "";
  qword m = n<(qword(1)<<18) ? n : qword(1)<<18;
  for( qword i=Next001( d, 0, m ); i+4<=m; i=Next001( d, i+3, m ) ) if( d[i+3]>=0xBA ) return "";
  std::string k = mpg::EsKind( 0xE0, -1, d, n );
  return k=="bin" ? "" : k;
}

static bool Encode( const Buf& in, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names ) {
  std::string k = Kind( in.data(), in.size() );
  if( k.empty() ) return false;
  TrackHead h; h.ext = k; h.xform = XF_NONE; h.szmode = SZ_EXPLICIT;
  for( qword p=0; p<in.size(); p+=qword(1)<<30 ) h.col.push_back( std::min( qword(in.size())-p, qword(1)<<30 ) );
  h.nf = h.col.size();
  Writer w; PutMetaHdr( w, FMT_ES ); w.vint( 1 ); PutHead( w, h );
  meta.swap( w.b );
  streams.assign( 1, in ); names.assign( 1, TrackSuffix( 0, k ) );
  if( verbose ) printf( "elementary stream: %s, %llu bytes -> %s\n", k.c_str(), (unsigned long long)in.size(), names[0].c_str() );
  Meta M; Sink chk( 0, &in ); const char* why = "";
  return ParseMeta( meta, M ) && Decode( M, streams, chk, &why );
}

} // namespace es
