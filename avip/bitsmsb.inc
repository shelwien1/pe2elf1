struct BitsMSB {                             // MSB-first reader over an unescaped RBSP
  Buf b; size_t pos; bool err;
  void Load( const byte* p, size_t n, size_t cap ) {
    b.clear(); pos = 0; err = false;
    int z = 0;
    for( size_t i=0; i<n && b.size()<cap; i++ ) {
      if( z>=2 && p[i]==3 ) { z = 0; continue; }
      z = p[i] ? 0 : z+1; b.push_back( p[i] );
    }
  }
  uint u( int k ) {
    uint v = 0;
    for( int i=0; i<k; i++ ) {
      if( pos>=8*b.size() ) { err=true; return 0; }
      v = (v<<1) | ((b[pos>>3]>>(7-(pos&7)))&1); pos++;
    }
    return v;
  }
  void skip( size_t k ) { if( pos+k>8*b.size() ) err = true, pos = 8*b.size(); else pos += k; }
  uint ue() { int z = 0; while( !u(1) ) if( err || ++z>30 ) { err=true; return 0; } return z ? (1u<<z)-1+u(z) : 0; }
  int se() { uint k = ue(); return (k&1) ? int((k+1)>>1) : -int(k>>1); }
};

// H.264 / HEVC SEI payloads at p (after the NAL header): is one a recovery point (type 6)?
static bool SeiRecovery( const byte* p, size_t n ) {
  BitsMSB r; r.Load( p, n, 256 );
  while( !r.err && r.pos+16<=8*r.b.size() ) {
    uint t = 0, s = 0, c;
    do { c = r.u(8); t += c; } while( c==255 && !r.err );
    do { c = r.u(8); s += c; } while( c==255 && !r.err );
    if( r.err ) break;
    if( t==6 ) return true;
    r.skip( size_t(s)*8 );
  }
  return false;
}
