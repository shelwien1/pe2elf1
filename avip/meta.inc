//--- vp metainfo framing ---------------------------------------------------------
//
//   u8   hdr        bits 0..1 container (1 MP4, 2 MKV, 3 AVI), bits 2..7 format version (1);
//                   0x00: stored copy, the rest of the meta is the output
//   vint ntracks
//   per track: common head (below), then the container's track part
//   ops        container op program, ends with its END op
//   struct     residual container bytes = the rest of the meta
//
// Anything that doesn't parse as a meta is a plain copy of the output (no streams). The
// encoder writes unrecognized input as is, or as 0x00 + input when that copy would
// start with 0x00 or parse as a meta.
//
// Common track head:
//   ext        u8 index into ext_tab; 0: str ext follows. Stream file = prefix.<i>.<ext>,
//              i = track number in the meta
//   u8         xform | szmode<<3
//                xform 0 none, 1 Annex B: u8 lensize, vint hdrlen, idxlen raw runs, idxlen
//                      explicit NAL lengths; 2 ADTS: idxlen raw frames; 3 blob (the record
//                      ends here: one cut-out region, placed by a container op); 4 header
//                      prepended: vint hdrlen; 5 Ogg: u8 header packets
//                idxlen: vint n, n x {vint index delta, vint length}
//   vint nf    frames
//   sizes      szmode 0 explicit: column; 1 container tables (the struct holds them);
//              2 units: u8 ptype, [u8 pparam for NAL/AAC], column of v = units+1 or
//                0 = escape, column of the escape sizes (one per v==0);
//              3 from the transform (ADTS frame_length, Ogg packet lengths)

enum { FMT_STORED=0, FMT_MP4=1, FMT_MKV=2, FMT_AVI=3 };
enum { SZ_EXPLICIT=0, SZ_TABLES=1, SZ_UNITS=2, SZ_XFORM=3 };
static const int META_VER = 1;

static void PutMetaHdr( Writer& w, int fmt ) { w.u8( uint( fmt | META_VER<<2 ) ); }

// container of a meta by its header byte: FMT_*, or -1 (not a meta: plain copy)
static int MetaFormat( const byte* m, qword n ) {
  if( !n ) return -1;
  if( m[0]==0 ) return FMT_STORED;
  if( (m[0]>>2)!=META_VER || !(m[0]&3) ) return -1;
  return m[0]&3;
}

static const char* const ext_tab[] = { "",
  "h264", "hevc", "aac", "mp3", "mp2", "mp1", "ac3", "eac3", "dts", "ogg", "flac", "wav", "m4v", "mjpeg", "m2v",
  "vc1", "jpg", "png", "gif", "bmp", "webp", "rle8", "rle4", "dib", "av1", "opus", "srt", "ass", "ttf", "otf", "bin",
  0 };

static void PutExt( Writer& w, const std::string& e ) {
  for( int i=1; ext_tab[i]; i++ ) if( e==ext_tab[i] ) { w.u8( uint(i) ); return; }
  w.u8( 0 ); w.str( e );
}

static bool GetExt( Reader& r, std::string& e ) {
  uint i = r.u8(); int n = 0; while( ext_tab[n] ) n++;
  if( i>=uint(n) ) return false;
  e = i ? std::string( ext_tab[i] ) : r.str();
  if( r.err || e.empty() || e.size()>64 ) return false;
  for( size_t k=0; k<e.size(); k++ ) {
    char c = e[k];
    if( !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-') ) return false;
  }
  return true;
}

static std::string TrackSuffix( size_t i, const std::string& ext ) {
  char b[32]; snprintf( b, sizeof(b), ".%u.", unsigned(i) ); return b + ext;
}

static void PutIdxLenList( Writer& w, const IdxLen& x ) {
  w.vint( x.size() ); qword prev = 0;
  for( size_t k=0; k<x.size(); k++ ) w.vint( x[k].first-prev ), w.vint( x[k].second ), prev = x[k].first;
}

static bool GetIdxLenList( Reader& r, IdxLen& x, qword lim ) {
  qword n = r.vint(), idx = 0;
  if( r.err || n>lim || n>MAXCOUNT ) return false;
  x.resize( size_t(n) );
  for( size_t k=0; k<x.size(); k++ ) {
    qword dlt = r.vint(); if( k && !dlt ) return false;
    idx += dlt; x[k].first = idx; x[k].second = r.vint();
    if( r.err || idx>=lim ) return false;
  }
  return !r.err;
}

struct TrackHead {
  std::string ext;
  int xform, lensize, nhdr; qword hdrlen; IdxLen xa, xb;   // transform and its side info
  qword nf;                                                // frames
  int szmode, ptype, pparam;                               // size model
  std::vector<qword> col, esc;                             // explicit sizes or units+1; escape sizes
  TrackHead() : xform(0), lensize(0), nhdr(0), hdrlen(0), nf(0), szmode(0), ptype(0), pparam(0) {}
};

static bool PtypeHasParam( int pt ) { return pt==PT_NAL || pt==PT_AAC; }

static void PutHead( Writer& w, const TrackHead& h ) {
  PutExt( w, h.ext );
  w.u8( uint( h.xform | h.szmode<<3 ) );
  if( h.xform==XF_ANNEXB ) { w.u8( uint(h.lensize) ); w.vint( h.hdrlen ); PutIdxLenList( w, h.xa ); PutIdxLenList( w, h.xb ); }
  else if( h.xform==XF_ADTS ) PutIdxLenList( w, h.xa );
  else if( h.xform==XF_HDR ) w.vint( h.hdrlen );
  else if( h.xform==XF_OGG ) w.u8( uint(h.nhdr) );
  if( h.xform==XF_BLOB ) return;
  w.vint( h.nf );
  if( h.szmode==SZ_EXPLICIT ) PutCol( w, h.col );
  else if( h.szmode==SZ_UNITS ) {
    w.u8( uint(h.ptype) ); if( PtypeHasParam( h.ptype ) ) w.u8( uint(h.pparam) );
    PutCol( w, h.col ); PutCol( w, h.esc );
  }
}

static bool GetHead( Reader& r, TrackHead& h ) {
  if( !GetExt( r, h.ext ) ) return false;
  uint b = r.u8(); h.xform = int(b&7); h.szmode = int(b>>3);
  if( r.err || h.xform>XF_OGG || h.szmode>SZ_XFORM ) return false;
  h.lensize = h.nhdr = 0; h.hdrlen = 0; h.xa.clear(); h.xb.clear(); h.nf = 0; h.ptype = h.pparam = 0;
  h.col.clear(); h.esc.clear();
  if( h.xform==XF_ANNEXB ) {
    h.lensize = int(r.u8()); h.hdrlen = r.vint();
    if( r.err || h.lensize<1 || h.lensize>4 || !GetIdxLenList( r, h.xa, MAXCOUNT ) || !GetIdxLenList( r, h.xb, MAXCOUNT ) ) return false;
  } else if( h.xform==XF_ADTS ) { if( !GetIdxLenList( r, h.xa, MAXCOUNT ) ) return false; }
  else if( h.xform==XF_HDR ) h.hdrlen = r.vint();
  else if( h.xform==XF_OGG ) h.nhdr = int(r.u8());
  if( h.xform==XF_BLOB ) return !r.err && h.szmode==0;
  h.nf = r.vint();
  if( r.err || h.nf>MAXCOUNT ) return false;
  if( h.xform==XF_ADTS && h.xa.size() && h.xa.back().first>=h.nf ) return false;
  if( h.szmode==SZ_XFORM && h.xform!=XF_ADTS && h.xform!=XF_OGG ) return false;
  if( h.szmode==SZ_EXPLICIT ) { if( !GetCol( r, h.col, h.nf ) ) return false; }
  else if( h.szmode==SZ_UNITS ) {
    h.ptype = int(r.u8()); if( PtypeHasParam( h.ptype ) ) h.pparam = int(r.u8());
    if( r.err || h.ptype<PT_NAL || h.ptype>PT_M2V || !GetCol( r, h.col, h.nf ) ) return false;
    qword ne = 0; for( size_t k=0; k<h.col.size(); k++ ) ne += h.col[k]==0;
    if( !GetCol( r, h.esc, ne ) ) return false;
  }
  for( size_t k=0; k<h.col.size(); k++ ) if( h.col[k]>0xFFFFFFFFu ) return false;
  for( size_t k=0; k<h.esc.size(); k++ ) if( h.esc[k]>0xFFFFFFFFu ) return false;
  return !r.err;
}

// frame sizes from the head's size model and the raw (untransformed) stream s;
// xsz: sizes recovered by the inverse transform (SZ_XFORM)
static bool HeadSizes( const TrackHead& h, const Buf& s, const std::vector<uint>& xsz, std::vector<uint>& sz, const char** why ) {
  sz.resize( size_t(h.nf) );
  if( h.szmode==SZ_EXPLICIT ) { for( size_t k=0; k<h.nf; k++ ) sz[k] = uint(h.col[k]); return true; }
  if( h.szmode==SZ_XFORM ) { if( xsz.size()!=h.nf ) { *why = "transform size mismatch"; return false; } sz = xsz; return true; }
  if( h.szmode!=SZ_UNITS ) { *why = "no sizes"; return false; }
  qword pos = 0, ke = 0;
  for( qword k=0; k<h.nf; k++ ) {
    qword v = h.col[size_t(k)], z;
    if( v==0 ) { if( ke>=h.esc.size() ) { *why = "escape underflow"; return false; } z = h.esc[size_t(ke++)]; }
    else if( v==1 ) z = 0;
    else { z = UnitsLen( h.ptype, h.pparam, s.data()+pos, s.size()-pos, v-1 ); if( z==0 ) { *why = "unit parse failed"; return false; } }
    if( z>0xFFFFFFFFu || z>s.size()-pos ) { *why = "stream too short"; return false; }
    sz[size_t(k)] = uint(z); pos += z;
  }
  return true;
}

// inverse transform of a stored stream c to raw frame data r (sizes from ADTS/Ogg in xsz,
// Ogg header packets in priv)
static bool HeadUntransform( const TrackHead& h, const Buf& c, Buf& r, std::vector<uint>& xsz, Buf& priv ) {
  xsz.clear(); priv.clear();
  switch( h.xform ) {
  case XF_NONE: case XF_BLOB: r = c; return true;
  case XF_ANNEXB: return FromAnnexB( c, h.hdrlen, h.lensize, h.xa, h.xb, r );
  case XF_ADTS: return FromADTS( c, h.nf, h.xa, r, xsz );
  case XF_OGG: return FromOgg( c, h.nf, h.nhdr, r, xsz, priv );
  case XF_HDR: if( h.hdrlen>c.size() ) return false; r.assign( c.begin()+size_t(h.hdrlen), c.end() ); return true;
  }
  return false;
}

// unit-derived size model for frames at d+foff[j] (sizes fsz) of raw stream raw: the
// column and escapes if cheaper than explicit sizes (sizes checked in stream context)
static bool ChooseUnits( int ptype, int pparam, const byte* d, const std::vector<qword>& foff, const std::vector<uint>& fsz,
                         const Buf& raw, TrackHead& h ) {
  qword n = fsz.size();
  if( ptype==PT_NONE || !n ) return false;
  auto colcost = []( const std::vector<qword>& v ) { Writer w; PutCol( w, v ); return w.b.size(); };
  std::vector<qword> ex( fsz.begin(), fsz.end() );
  size_t best = colcost( ex );
  std::vector<qword> v( (size_t)n ), es; qword o = 0, ne = 0;
  for( qword j=0; j<n && ne<n/4+8; o+=fsz[size_t(j)], j++ ) {
    qword z = fsz[size_t(j)];
    if( !z ) { v[size_t(j)] = 1; continue; }
    long long u = CountUnits( ptype, pparam, d+foff[size_t(j)], z );
    if( u>0 && UnitsLen( ptype, pparam, raw.data()+o, raw.size()-o, qword(u) )==z ) v[size_t(j)] = qword(u)+1;
    else v[size_t(j)] = 0, es.push_back( z ), ne++;
  }
  if( ne>=n/4+8 ) return false;
  size_t cost = colcost( v ) + colcost( es ) + 2;
  if( cost>=best ) return false;
  h.szmode = SZ_UNITS; h.ptype = ptype; h.pparam = pparam; h.col.swap( v ); h.esc.swap( es );
  return true;
}
