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
