// stream transforms on concatenated frame data: s = frames back to back, sz(j) = size of
// frame j (any callable), nf frames. The inverse functions return the raw frame data
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
// xb: NALs whose length doesn't end at the next start code (#idx, length)
template<class SZ> static void ToAnnexB( const Buf& s, const SZ& sz, qword nf, int ls, const Buf& xhdr, Buf& c, IdxLen& xa, IdxLen& xb ) {
  static const byte sc[4] = {0,0,0,1};
  c.clear(); c.reserve( size_t( s.size() + xhdr.size() + (s.size()>>6) ) );
  c.insert( c.end(), xhdr.begin(), xhdr.end() );
  xa.clear(); xb.clear();
  std::vector<qword> np, nl;                            // payload position in c, true length
  const byte* p = s.data(); qword o = 0;
  for( qword j=0; j<nf; j++ ) {
    qword z = sz(j);
    if( z==0 ) continue;
    if( CountUnits( PT_NAL, ls, p+o, z )>=0 ) {
      for( qword e=o+z; o<e; ) {
        qword L=0; for( int i=0; i<ls; i++ ) L = (L<<8)|p[o+i];
        o += ls;
        c.insert( c.end(), sc, sc+4 );
        np.push_back( c.size() ); nl.push_back( L );
        c.insert( c.end(), p+o, p+o+L ); o += L;
      }
    } else {                                            // not NALs: verbatim
      if( xa.size() && xa.back().first==np.size() ) xa.back().second += z;
      else xa.push_back( std::make_pair( qword(np.size()), z ) ), c.insert( c.end(), sc, sc+4 );
      c.insert( c.end(), p+o, p+o+z ); o += z;
    }
  }
  for( size_t i=0; i<np.size(); i++ )
    if( NextSC( c.data(), np[i], c.size() )!=np[i]+nl[i] ) xb.push_back( std::make_pair( qword(i), nl[i] ) );
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
template<class SZ> static void ToADTS( const Buf& s, const SZ& sz, qword nf, const byte* tpl, Buf& c, IdxLen& xa ) {
  c.clear(); c.reserve( size_t( s.size() + 7*nf ) );
  xa.clear();
  const byte* p = s.data(); qword o = 0;
  for( qword j=0; j<nf; j++ ) {
    qword z = sz(j);
    if( z==0 || z>ADTS_MAX ) xa.push_back( std::make_pair( j, z ) );
    else { byte h[7]; PutADTS( h, tpl, uint(z+7) ); c.insert( c.end(), h, h+7 ); }
    c.insert( c.end(), p+o, p+o+z ); o += z;
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
template<class SZ> static void ToOgg( const Buf& s, const SZ& sz, qword nf, const byte* const* vh, const qword* vl,
                                      const VorbisInfo& vi, uint serial, qword trim, Buf& c ) {
  c.clear(); c.reserve( size_t( s.size() + (s.size()>>6) + 8192 + vl[0]+vl[1]+vl[2] ) );
  OggWriter w( c, serial );
  for( int h=0; h<3; h++ ) w.Packet( vh[h], vl[h], 0, h!=1 );
  const byte* p = s.data(); qword o = 0, g = 0; uint prev = 0;
  for( qword j=0; j<nf; j++ ) {
    qword z = sz(j), u = VorbisDur( vi, p+o, z, prev );
    if( j+1==nf ) u -= std::min( u, trim );
    g += u;
    w.Packet( p+o, z, g, false ); o += z;
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
