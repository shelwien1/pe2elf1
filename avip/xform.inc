// stream transforms on a track's frames (fs, frames.inc: in place in the input, or a stream),
// written to an XOut (kept, or only counted). The inverse functions return the raw frame data
// and, where the transform carries them, the frame sizes.
//
// XF_ANNEXB: raw = sequence of [lensize-byte BE length][NAL], converted to hdr (parameter
//   sets, each 00 00 00 01 + NAL) + per NAL 00 00 00 01 + NAL. Frames that don't parse as
//   NALs are copied verbatim after their own 00 00 00 01 ("raw runs", positioned by NAL
//   index, explicit length). A NAL's length is implied by the next 00 00 00 01 (or stream
//   end); NALs where that's wrong (embedded start code, trailing zeros) get an explicit
//   length.
// XF_ADTS: each frame gets a 7-byte ADTS header (frame_length = size+7); frames that
//   don't fit (empty or > 8184 bytes) are copied verbatim with explicit size.
// XF_HDR: a header is prepended (WAV header for PCM-like audio, FLAC metadata, codec
//   headers for video streams that don't carry them in-band); the decoder strips it.
// XF_OGG: Vorbis packets -> a plain Ogg Vorbis file (header packets first). Page 0 holds
//   the identification header alone (BOS), the last header packet ends a page, audio
//   pages are closed at >=4096 body bytes or 255 segments, the last page has EOS;
//   granulepos follows the Vorbis spec. Frame sizes come back from the packet boundaries.

static const qword ADTS_MAX = 8191-7;

// length-prefixed NAL units -> Annex B (4-byte start codes), xhdr (parameter sets) first.
// xa: verbatim runs of frames that aren't NAL sequences (before NAL #idx, length);
// xb: NALs whose length doesn't end at the next start code (#idx, length): those with an
// 00 00 00 01 inside (what follows a NAL is a start code or the end, and none can begin in a
// NAL's last bytes and end in that start code, whose only 01 is its last byte)
static void ToAnnexB( const FrameSrc& fs, qword nf, int ls, const Buf& xhdr, XOut& c, IdxLen& xa, IdxLen& xb ) {
  static const byte sc[4] = {0,0,0,1};
  c.Reset();
  c.Put( xhdr.data(), xhdr.size() );
  xa.clear(); xb.clear();
  qword nn = 0;                                         // NALs so far
  for( qword j=0; j<nf; j++ ) {
    qword z = fs.Z( j );
    if( z==0 ) continue;
    const byte* p = fs.P( j );
    if( CountUnits( PT_NAL, ls, p, z )>=0 ) {
      for( qword o=0; o<z; ) {
        qword L=0; for( int i=0; i<ls; i++ ) L = (L<<8)|p[o+i];
        o += ls;
        c.Put( sc, 4 );
        if( NextSC( p+o, 0, L )<L ) xb.push_back( std::make_pair( nn, L ) );
        c.Put( p+o, L ); o += L; nn++;
      }
    } else {                                            // not NALs: verbatim
      if( xa.size() && xa.back().first==nn ) xa.back().second += z;
      else xa.push_back( std::make_pair( nn, z ) ), c.Put( sc, 4 );
      c.Put( p, z );
    }
  }
}

static bool FromAnnexB( const Buf& c, qword hdrlen, int ls, const IdxLen& xa, const IdxLen& xb, Buf& r ) {
  const byte* p = c.data(); qword n = c.size(), pos = hdrlen;
  if( pos>n ) return false;
  r.clear(); r.reserve( size_t(n-pos) );
  size_t ka=0, kb=0;
  for( qword i=0;; i++ ) {
    if( ka<xa.size() && xa[ka].first==i ) {             // raw run before NAL i
      qword L = xa[ka++].second;
      if( n-pos<4 || p[pos] || p[pos+1] || p[pos+2] || p[pos+3]!=1 ) return false;
      pos += 4;
      if( L>n-pos ) return false;
      r.insert( r.end(), p+pos, p+pos+L ); pos+=L;
    }
    if( pos==n ) break;
    if( n-pos<4 || p[pos] || p[pos+1] || p[pos+2] || p[pos+3]!=1 ) return false;
    pos += 4;
    qword L;
    if( kb<xb.size() && xb[kb].first==i ) { L = xb[kb++].second; if( L>n-pos ) return false; }
    else L = NextSC( p, pos, n )-pos;
    if( L>>(8*ls) ) return false;
    byte b[4]; for( int k=ls-1; k>=0; k-- ) b[k]=byte(L>>(8*(ls-1-k)));
    r.insert( r.end(), b, b+ls );
    r.insert( r.end(), p+pos, p+pos+L ); pos+=L;
  }
  return ka==xa.size() && kb==xb.size();
}

// raw AAC frames -> ADTS (template tpl, frame_length per frame); xa: frames left raw
// (empty or too long for ADTS): (#frame, size)
static void ToADTS( const FrameSrc& fs, qword nf, const byte* tpl, XOut& c, IdxLen& xa ) {
  c.Reset();
  xa.clear();
  for( qword j=0; j<nf; j++ ) {
    qword z = fs.Z( j );
    if( z==0 || z>ADTS_MAX ) xa.push_back( std::make_pair( j, z ) );
    else { byte h[7]; PutADTS( h, tpl, uint(z+7) ); c.Put( h, 7 ); }
    if( z ) c.Put( fs.P( j ), z );
  }
}

static bool FromADTS( const Buf& c, qword nf, const IdxLen& xa, Buf& r, std::vector<uint>& xsz ) {
  const byte* p = c.data(); qword n = c.size(), pos = 0;
  r.clear(); r.reserve( size_t(n) );
  xsz.resize( size_t(nf) );
  size_t ka=0;
  for( qword k=0; k<nf; k++ ) {
    qword L;
    if( ka<xa.size() && xa[ka].first==k ) {
      L = xa[ka++].second;
      if( L>n-pos ) return false;
    } else {
      if( n-pos<7 || p[pos]!=0xFF || (p[pos+1]&0xF7)!=0xF1 ) return false;
      qword fl = (qword(p[pos+3]&3)<<11) | (qword(p[pos+4])<<3) | (p[pos+5]>>5);
      if( fl<7 || fl>n-pos ) return false;
      pos += 7; L = fl-7;
    }
    if( L>0xFFFFFFFFu ) return false;
    xsz[size_t(k)] = uint(L);
    r.insert( r.end(), p+pos, p+pos+L ); pos+=L;
  }
  return pos==n && ka==xa.size();
}

// Vorbis packets -> Ogg Vorbis file (3 header packets vh/vl first); trim: samples cut
// from the last packet's duration (end trimming)
static void ToOgg( const FrameSrc& fs, qword nf, const byte* const* vh, const qword* vl,
                   const VorbisInfo& vi, uint serial, qword trim, XOut& c ) {
  static const byte none[1] = {0};
  c.Reset();
  OggWriter w( c, serial );
  for( int h=0; h<3; h++ ) w.Packet( vh[h], vl[h], 0, h!=1 );
  qword g = 0; uint prev = 0;
  for( qword j=0; j<nf; j++ ) {
    qword z = fs.Z( j ); const byte* p = z ? fs.P( j ) : none;
    qword u = VorbisDur( vi, p, z, prev );
    if( j+1==nf ) u -= std::min( u, trim );
    g += u;
    w.Packet( p, z, g, false );
  }
  w.End();
}

// Ogg -> packets: the first nhdr packets go to priv (Xiph-laced, CodecPrivate layout),
// the rest are the frames (sizes in xsz)
static bool FromOgg( const Buf& c, qword nf, int nhdr, Buf& r, std::vector<uint>& xsz, Buf& priv ) {
  const byte* p = c.data(); qword n = c.size(), pos = 0, np = 0, cur = 0;
  Buf hb; std::vector<qword> hl;
  r.clear(); r.reserve( size_t(n) );
  xsz.clear(); priv.clear();
  while( pos<n ) {
    if( n-pos<27 || memcmp(p+pos,"OggS",4) || p[pos+4] ) return false;
    qword ns = p[pos+26], h = 27+ns, bl = 0;
    if( n-pos<h ) return false;
    const byte* sg = p+pos+27;
    for( qword i=0; i<ns; i++ ) bl += sg[i];
    if( n-pos-h<bl ) return false;
    const byte* b = p+pos+h;
    for( qword i=0; i<ns; i++ ) {
      uint v = sg[i];
      if( np>=qword(nhdr) ) r.insert( r.end(), b, b+v ); else hb.insert( hb.end(), b, b+v );
      b += v; cur += v;
      if( v<255 ) {
        if( np>=qword(nhdr) ) {
          if( cur>0xFFFFFFFFu || xsz.size()>=nf ) return false;
          xsz.push_back( uint(cur) );
        } else hl.push_back( cur );
        np++; cur = 0;
      }
    }
    pos += h+bl;
  }
  if( cur || xsz.size()!=nf || np!=qword(nhdr)+nf ) return false;
  if( nhdr ) XiphLace( hl.data(), nhdr, priv ), priv.insert( priv.end(), hb.begin(), hb.end() );
  return true;
}
