struct Writer {
  Buf b;
  void u8( uint x ) { b.push_back( byte(x) ); }
  void vint( qword x ) { while( x>=0x80 ) b.push_back( byte(x|0x80) ), x>>=7; b.push_back( byte(x) ); }
  void bytes( const void* p, size_t n ) { const byte* q=(const byte*)p; b.insert( b.end(), q, q+n ); }
  void str( const std::string& s ) { vint( s.size() ); bytes( s.data(), s.size() ); }
};
