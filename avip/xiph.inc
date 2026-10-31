// Xiph-laced (Matroska CodecPrivate, ffmpeg esds) or 16-bit-length (00 1E ...)
// header blob -> 3 header packets
static bool SplitXiphHeaders( const byte* p, qword n, const byte* hp[3], qword hl[3] ) {
  if( n>=2 && p[0]==0 && p[1]==30 ) {
    qword o = 0;
    for( int i=0; i<3; i++ ) {
      if( n-o<2 ) return false;
      qword L = BE16(p+o); o+=2;
      if( L>n-o ) return false;
      hp[i] = p+o; hl[i] = L; o+=L;
    }
    return true;
  }
  if( n<3 || p[0]!=2 ) return false;
  qword o = 1, tot = 0;
  for( int i=0; i<2; i++ ) {
    qword s=0; uint c;
    do { if( o>=n ) return false; c = p[o++]; s += c; } while( c==255 );
    hl[i] = s; tot += s;
  }
  if( tot>n-o ) return false;
  hl[2] = n-o-tot;
  hp[0] = p+o; hp[1] = hp[0]+hl[0]; hp[2] = hp[1]+hl[1];
  return true;
}

// Xiph lace header for n packets: count-1, then the sizes of packets 0..n-2
static void XiphLace( const qword* hl, int n, Buf& o ) {
  o.push_back( byte(n-1) );
  for( int i=0; i<n-1; i++ ) { qword s = hl[i]; while( s>=255 ) o.push_back(255), s-=255; o.push_back( byte(s) ); }
}

// canonical Xiph-laced blob of the header packets; the codec config is cut from
// the struct (and rebuilt from the Ogg stream) only when it is exactly this
static bool XiphCanonical( const byte* const hp[3], const qword hl[3], const byte* p, qword n ) {
  Buf h; XiphLace( hl, 3, h );
  for( int i=0; i<3; i++ ) h.insert( h.end(), hp[i], hp[i]+hl[i] );
  return h.size()==n && !memcmp( h.data(), p, size_t(n) );
}
