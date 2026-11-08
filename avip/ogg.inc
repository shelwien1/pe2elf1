//--- Vorbis in Ogg (shared) ----------------------------------------------------
//
// XF_OGG: frames are Vorbis packets; the converted stream is a plain Ogg Vorbis
//   file: the nhdr header packets (from CodecPrivate / esds), then one packet per
//   frame. Page 0 holds the identification header alone (BOS), the last header
//   packet ends a page, audio pages are closed at >=4096 body bytes or 255
//   segments (packets may span pages), the last page has EOS. granulepos follows
//   the Vorbis spec: PCM position after the last packet completed on the page
//   (first audio packet = 0 samples, then (prev_blocksize+cur_blocksize)/4).
//   Frame sizes come back from the packet boundaries; the decoder only uses the
//   segment tables (granules, CRCs and flags are redundant there).

static uint ogg_crc[256];

static uint OggCrc( const byte* p, size_t n ) {
  if( !ogg_crc[1] )
    for( uint i=0; i<256; i++ ) {
      uint r = i<<24;
      for( int k=0; k<8; k++ ) r = (r&0x80000000u) ? (r<<1)^0x04C11DB7u : r<<1;
      ogg_crc[i] = r;
    }
  uint c = 0;
  for( size_t i=0; i<n; i++ ) c = (c<<8) ^ ogg_crc[(c>>24)^p[i]];
  return c;
}

struct OggWriter {                                      // pages to an XOut (frames.inc): kept, or only counted
  XOut& o; uint serial, seq; Buf seg, body, page; qword gran; bool bos, cont, mid; qword last;
  OggWriter( XOut& _o, uint _s ) : o(_o), serial(_s), seq(0), gran(~qword(0)), bos(true), cont(false), mid(false), last(~qword(0)) {}
  static void Crc( byte* pg, size_t n ) {
    memset( pg+22, 0, 4 );
    uint c = OggCrc( pg, n );
    for( int i=0; i<4; i++ ) pg[22+i] = byte(c>>(8*i));
  }
  void Flush( uint fl=0 ) {
    if( seg.empty() ) return;
    byte h[27];
    memcpy( h, "OggS", 4 ); h[4] = 0;
    h[5] = byte( fl | (cont ? 1 : 0) | (bos ? 2 : 0) );
    for( int i=0; i<8; i++ ) h[6+i] = byte(gran>>(8*i));
    for( int i=0; i<4; i++ ) h[14+i] = byte(serial>>(8*i)), h[18+i] = byte(seq>>(8*i)), h[22+i] = 0;
    h[26] = byte( seg.size() );
    page.clear(); page.insert( page.end(), h, h+27 ); page.insert( page.end(), seg.begin(), seg.end() ); page.insert( page.end(), body.begin(), body.end() );
    Crc( page.data(), page.size() );
    last = o.n; o.Put( page.data(), page.size() );
    seq++; bos = false; cont = mid; gran = ~qword(0); seg.clear(); body.clear();
  }
  // g: granulepos after this packet; flush: the packet ends its page
  void Packet( const byte* p, qword n, qword g, bool flush ) {
    for( qword k=0;; ) {
      if( seg.size()==255 ) Flush();
      uint v = n-k>=255 ? 255u : uint(n-k);
      seg.push_back( byte(v) ); body.insert( body.end(), p+k, p+k+v ); k += v;
      if( v<255 ) break;
      mid = true;
    }
    mid = false; gran = g;
    if( flush || body.size()>=4096 ) Flush();
  }
  void End() {
    if( seg.size() ) Flush( 4 );
    else if( last!=~qword(0) && o.b && !((*o.b)[size_t(last+5)]&4) ) {   // (counted only: the size stays)
      Buf& b = *o.b;
      b[size_t(last+5)] |= 4;
      Crc( &b[size_t(last)], size_t(b.size()-last) );     // last page is at the end of o
    }
  }
};
