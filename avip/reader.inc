struct Reader {
  const byte* p; const byte* e; bool err;
  Reader( const byte* _p, size_t n ) : p(_p), e(_p+n), err(false) {}
  uint u8() { if( p>=e ) { err=true; return 0; } return *p++; }
  qword vint() {
    qword x=0; int sh=0;
    for(;;) {
      if( p>=e || sh>63 ) { err=true; return 0; }
      byte c = *p++; x |= qword(c&0x7F)<<sh;
      if( !(c&0x80) ) break;
      sh+=7;
    }
    return x;
  }
  const byte* bytes( qword n ) { if( qword(e-p)<n ) { err=true; return 0; } const byte* r=p; p+=n; return r; }
  std::string str() { qword n=vint(); const byte* q=bytes(n); return q ? std::string((const char*)q,size_t(n)) : std::string(); }
};
