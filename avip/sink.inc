struct Sink {
  FILE* f; const Buf* ref; qword pos; bool err;
  Sink( FILE* _f, const Buf* _r ) : f(_f), ref(_r), pos(0), err(false) {}
  void put( const byte* p, qword n ) {
    if( err || n==0 ) return;
    if( f ) { if( fwrite(p,1,size_t(n),f)!=n ) err=true; }
    else if( ref ) { if( n>ref->size()-pos || memcmp(ref->data()+pos,p,size_t(n)) ) err=true; }
    pos+=n;
  }
};
