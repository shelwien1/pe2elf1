// A track's frames as the analyses and the stream conversions read them, without the track's
// raw stream having to be made: frame f is P(f), Z(f) bytes. Three kinds:
//   - Flat: a stream, the frames back to back from base at offsets off (FlatFrames: off[nf] is
//     the end; or sizes sz; no off: one frame, the whole stream); end: the stream's end
//   - InPlace: frames where they lie, at base+off[f], sz[f] bytes (the mapped input)
//   - Gather: frames in pieces (TS payloads), put together by gf into a scratch buffer; sizes sz
// A pointer P() returns stays good until the next call of P().
struct FrameSrc {
  const byte* base; const qword* off; const uint* sz;     // sz 0: sizes from off[f+1]
  qword end;                                               // Flat: the stream's end; else ~0
  std::function<void(qword,Buf&)> gf;                      // Gather: frame f appended to the buffer
  mutable Buf scratch;
  FrameSrc() : base(0), off(0), sz(0), end(~qword(0)) {}
  bool Flat() const { return end!=~qword(0); }
  const byte* P( qword f ) const {
    if( gf ) { scratch.clear(); gf( f, scratch ); const Buf& s = scratch; return s.data(); }
    return off ? base+off[size_t(f)] : base;
  }
  qword Z( qword f ) const { return sz ? qword(sz[size_t(f)]) : off ? off[size_t(f+1)]-off[size_t(f)] : end; }
};

static FrameSrc FlatFrames( const byte* s, const std::vector<qword>& fo ) {
  FrameSrc r; r.base = s; r.off = fo.data(); r.end = fo.back(); return r;
}
static FrameSrc WholeStream( const byte* s, qword n ) { FrameSrc r; r.base = s; r.end = n; return r; }
static FrameSrc InPlaceFrames( const byte* d, const std::vector<qword>& off, const std::vector<uint>& sz ) {
  FrameSrc r; r.base = d; r.off = off.data(); r.sz = sz.data(); return r;
}

// frames [a,b) of fs back to back, appended to o
static void AppendFrames( const FrameSrc& fs, qword a, qword b, Buf& o ) {
  for( qword f=a; f<b; f++ ) { qword z = fs.Z( f ); if( z ) { const byte* p = fs.P( f ); o.insert( o.end(), p, p+z ); } }
}

// where a stream goes: kept in b, or only counted (b 0: avip's info pass, which writes
// nothing); h: its first bytes (up to 16: what checks of its kind look at)
struct XOut {
  Buf* b; qword n; byte h[16];
  explicit XOut( Buf* _b ) : b(_b), n(0) {}
  void Reset() { if( b ) b->clear(); n = 0; }
  void Put( const byte* p, qword k ) {
    for( qword i=0; i<k && n+i<16; i++ ) h[n+i] = p[i];
    if( b ) b->insert( b->end(), p, p+k );
    n += k;
  }
};

// the frames of fs back to back (a track's stream as it is), appended to o
static void PutFrames( const FrameSrc& fs, qword nf, XOut& o ) {
  for( qword f=0; f<nf; f++ ) {
    qword z = fs.Z( f );
    if( !z ) continue;
    if( o.b || o.n<16 ) o.Put( fs.P( f ), z ); else o.n += z;
  }
}

// the first bytes of the stream fs's frames make: at least k of them (all, if it has fewer)
static void HeadBytes( const FrameSrc& fs, qword nf, qword k, Buf& h ) {
  h.clear();
  for( qword f=0; f<nf && h.size()<k; f++ ) AppendFrames( fs, f, f+1, h );
}

// the stream fs's frames make, read going forward: At( p, k ) gives its bytes [p,p+k) (all
// within the stream), good until the next At(); Drop( p ): nothing before p is read again
struct StreamWin {
  const FrameSrc& fs; qword nf, nx, w0; Buf w;              // w: stream bytes from w0, frames [..,nx)
  StreamWin( const FrameSrc& _fs, qword _nf ) : fs(_fs), nf(_nf), nx(0), w0(0) {}
  const byte* At( qword p, qword k ) {
    if( fs.Flat() ) return fs.base+p;
    while( p+k>w0+w.size() && nx<nf ) AppendFrames( fs, nx, nx+1, w ), nx++;
    const Buf& c = w; return c.data()+size_t(p-w0);
  }
  void Drop( qword p ) {
    if( fs.Flat() || p-w0<(qword(1)<<20) || 2*(p-w0)<w.size() ) return;
    size_t k = size_t( std::min( p-w0, qword(w.size()) ) );
    w.erase( w.begin(), w.begin()+k ); w0 += k;
  }
};
