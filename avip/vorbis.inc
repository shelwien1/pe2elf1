struct VorbisInfo { uint rate, bs[2]; int nmodes, mbits; byte mflag[64]; };

static int ILog( uint x ) { int n=0; while( x ) n++, x>>=1; return n; }

struct VBits {                               // Vorbis bit packing: LSB first
  const byte* p; qword nb, pos; bool err;
  uint get( int k ) {
    uint v = 0;
    for( int i=0; i<k; i++ ) {
      if( pos>=nb ) { err=true; return 0; }
      v |= uint( (p[pos>>3]>>(pos&7))&1 ) << i; pos++;
    }
    return v;
  }
  void skip( qword k ) { if( k>nb-pos ) err=true, pos=nb; else pos+=k; }
};

// largest r with r^dim <= ent (dim>=1, ent < 2^24)
static qword Lookup1Values( qword ent, qword dim ) {
  qword lo = 0, hi = ent;
  while( lo<hi ) {
    qword m = (lo+hi+1)/2, x = 1;
    for( qword i=0; i<dim && x<=ent; i++ ) x *= m;
    if( x<=ent ) lo = m; else hi = m-1;
  }
  return lo;
}

// identification + setup header -> block sizes and per-mode block flags.
// Full forward parse of the setup header (Vorbis I spec, section 4.2.4).
static bool VorbisParse( const byte* const hp[3], const qword hl[3], VorbisInfo& v ) {
  const byte* id = hp[0];
  if( hl[0]<30 || id[0]!=1 || memcmp(id+1,"vorbis",6) ) return false;
  uint ver = id[7]|(id[8]<<8)|(id[9]<<16)|(uint(id[10])<<24), ch = id[11];
  uint rate = id[12]|(id[13]<<8)|(id[14]<<16)|(uint(id[15])<<24);
  uint b0 = id[28]&15, b1 = id[28]>>4;
  if( ver || !ch || !rate || b0<6 || b1>13 || b0>b1 || !(id[29]&1) ) return false;
  v.rate = rate; v.bs[0] = 1u<<b0; v.bs[1] = 1u<<b1;
  if( hl[1]<7 || hp[1][0]!=3 || memcmp(hp[1]+1,"vorbis",6) ) return false;
  if( hl[2]<7 || hp[2][0]!=5 || memcmp(hp[2]+1,"vorbis",6) ) return false;
  VBits b; b.p = hp[2]; b.nb = 8*hl[2]; b.pos = 56; b.err = false;
  int cb = b.get(8)+1;                       // codebooks
  for( int i=0; i<cb && !b.err; i++ ) {
    if( b.get(24)!=0x564342 ) return false;
    qword dim = b.get(16), ent = b.get(24);
    if( b.get(1) ) {                         // ordered
      qword cur = 0; b.get(5);
      while( cur<ent && !b.err ) cur += b.get( ILog( uint(ent-cur) ) );
      if( cur>ent ) return false;
    } else {
      bool sparse = b.get(1)!=0;
      for( qword j=0; j<ent && !b.err; j++ ) if( !sparse || b.get(1) ) b.get(5);
    }
    int lt = b.get(4);
    if( lt>2 || (lt && !dim) ) return false;
    if( lt ) {
      b.get(32); b.get(32);
      int vb = b.get(4)+1; b.get(1);
      qword nv = lt==1 ? Lookup1Values( ent, dim ) : ent*dim;
      b.skip( nv*vb );
    }
  }
  int nt = b.get(6)+1;                       // time domain transforms (placeholders)
  for( int i=0; i<nt; i++ ) if( b.get(16) ) return false;
  int nf = b.get(6)+1;                       // floors
  for( int i=0; i<nf && !b.err; i++ ) {
    int ft = b.get(16);
    if( ft==0 ) {
      b.get(8); b.get(16); b.get(16); b.get(6); b.get(8);
      int nbk = b.get(4)+1;
      for( int j=0; j<nbk; j++ ) b.get(8);
    } else if( ft==1 ) {
      int np = b.get(5), pc[32], maxc = -1, cdim[16];
      for( int j=0; j<np; j++ ) pc[j] = b.get(4), maxc = std::max( maxc, pc[j] );
      for( int j=0; j<=maxc; j++ ) {
        cdim[j] = b.get(3)+1;
        int sc = b.get(2);
        if( sc ) b.get(8);
        for( int k=0; k<(1<<sc); k++ ) b.get(8);
      }
      b.get(2); int rb = b.get(4);
      for( int j=0; j<np; j++ ) b.skip( qword(cdim[pc[j]])*rb );
    } else return false;
  }
  int nr = b.get(6)+1;                       // residues
  for( int i=0; i<nr && !b.err; i++ ) {
    if( b.get(16)>2 ) return false;
    b.get(24); b.get(24); b.get(24);
    int ncl = b.get(6)+1, casc[64];
    b.get(8);
    for( int j=0; j<ncl; j++ ) { casc[j] = b.get(3); if( b.get(1) ) casc[j] |= b.get(5)<<3; }
    for( int j=0; j<ncl; j++ ) for( int k=0; k<8; k++ ) if( casc[j]>>k&1 ) b.get(8);
  }
  int nm = b.get(6)+1, cb2 = ILog( ch-1 );   // mappings
  for( int i=0; i<nm && !b.err; i++ ) {
    if( b.get(16) ) return false;
    int sm = b.get(1) ? b.get(4)+1 : 1;
    if( b.get(1) ) { int st = b.get(8)+1; b.skip( qword(st)*2*cb2 ); }
    if( b.get(2) ) return false;
    if( sm>1 ) b.skip( 4*qword(ch) );
    b.skip( 24*qword(sm) );
  }
  v.nmodes = b.get(6)+1;                     // modes
  for( int i=0; i<v.nmodes; i++ ) {
    v.mflag[i] = byte( b.get(1) );
    if( b.get(16) || b.get(16) || int(b.get(8))>=nm ) return false;
  }
  if( !b.get(1) || b.err ) return false;     // framing bit
  v.mbits = ILog( uint(v.nmodes-1) );
  return true;
}

// PCM samples returned by one audio packet; prev = blocksize of the previous
// decodable packet (0 = none yet). Packets that don't decode return nothing.
static uint VorbisDur( const VorbisInfo& v, const byte* p, qword n, uint& prev ) {
  if( n==0 || (p[0]&1) ) return 0;
  uint m = (p[0]>>1) & ((1u<<v.mbits)-1);
  if( int(m)>=v.nmodes ) return 0;
  uint cur = v.bs[v.mflag[m]], d = prev ? (prev+cur)>>2 : 0;
  prev = cur;
  return d;
}
