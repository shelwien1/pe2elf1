// column of n values (n known to the reader): u8 h = P | rep<<4, then the values, for P>0
// as zigzag(v[k]-v[k-P]) with v[k<P] taken as 0, coded as
//   rep 0: runs: vint nruns, nruns x {vint value, vint length-1}
//   rep 1: vint values; a value equal to the previous one is followed by vint count of
//          further repeats (runs are maximal)
//   rep 2: vint values
// The encoder picks P and rep by the estimated size after a later LZ-type compressor
// (LzCost) plus a tenth of the raw size, so the meta stays short and compressible.

static size_t RepCost( const std::vector<qword>& v ) {
  size_t z = 0;
  for( size_t i=0, j; i<v.size(); i=j ) {
    for( j=i+1; j<v.size() && v[j]==v[i]; j++ );
    z += VintSize( v[i] ); if( j-i>1 ) z += VintSize( v[i] ) + VintSize( j-i-2 );
  }
  return z;
}

static void PeriodDelta( const std::vector<qword>& v, int P, std::vector<qword>& e ) {
  e.resize( v.size() );
  for( size_t k=0; k<v.size(); k++ ) e[k] = P ? ZigZag( (long long)v[k] - (long long)(k>=size_t(P) ? v[k-P] : 0) ) : v[k];
}

// estimated compressed size in bits: greedy LZ77 parse (3-byte hash chains, depth 8),
// match = log2(distance)+log2(length)+3 bits, literal = adaptive order-0 code length
static double LzCost( const byte* b, size_t n ) {
  if( n<4 ) return 8.0*n;
  int hb = 8; while( hb<16 && (size_t(1)<<hb)<n ) hb++;
  std::vector<uint32_t> head( size_t(1)<<hb, ~0u ), prev( n, ~0u );
  uint cnt[256] = {0}; uint tot = 0; double bits = 0;
  auto H = [&]( size_t p ) { return ( (uint(b[p])<<16 | uint(b[p+1])<<8 | b[p+2]) * 2654435761u ) >> (32-hb); };
  auto ins = [&]( size_t p ) { if( p+3<=n ) { uint h = H(p); prev[p] = head[h]; head[h] = uint32_t(p); } };
  for( size_t i=0; i<n; ) {
    size_t bl = 0, bd = 0;
    if( i+3<=n ) {
      uint32_t p = head[H(i)];
      for( int d=0; d<8 && p!=~0u; d++, p = prev[p] ) {
        size_t L = 0; while( i+L<n && L<273 && b[p+L]==b[i+L] ) L++;
        if( L>bl ) bl = L, bd = i-p;
      }
    }
    if( bl>=3 ) {
      bits += log2( double(bd+1) ) + log2( double(bl) ) + 3;
      for( size_t q=i; q<i+bl; q++ ) ins( q );
      i += bl;
    } else {
      bits -= log2( (cnt[b[i]]+0.5) / (tot+128.0) ); cnt[b[i]]++; tot++;
      ins( i ); i++;
    }
  }
  return bits;
}

static void PutColAs( Writer& w, const std::vector<qword>& e, int P, int rp ) {
  w.u8( uint( P | rp<<4 ) );
  if( rp==0 ) { PutRuns( w, e ); return; }
  if( rp==2 ) { for( size_t i=0; i<e.size(); i++ ) w.vint( e[i] ); return; }
  for( size_t i=0, j; i<e.size(); i=j ) {
    for( j=i+1; j<e.size() && e[j]==e[i]; j++ );
    w.vint( e[i] ); if( j-i>1 ) w.vint( e[i] ), w.vint( j-i-2 );
  }
}

static void PutCol( Writer& w, const std::vector<qword>& v ) {
  std::vector<qword> e;
  // small columns: shortest form
  size_t r0 = RunsCost( v ), r1 = RepCost( v );
  if( std::min( r0, r1 )<48 ) { PutColAs( w, v, 0, r1<r0 ? 1 : 0 ); return; }
  double best = 0; int bp = -1, br = 0; size_t bsz = 0;
  for( int P=0; P<=8; P++ ) {
    PeriodDelta( v, P, e );
    for( int rp=0; rp<3; rp++ ) {
      Writer t; PutColAs( t, e, P, rp );
      double c = LzCost( t.b.data(), t.b.size() ) + 0.8*double(t.b.size());   // + 0.1 x raw bits
      if( bp<0 || c<best || (c==best && t.b.size()<bsz) ) best = c, bp = P, br = rp, bsz = t.b.size();
    }
  }
  PeriodDelta( v, bp, e );
  PutColAs( w, e, bp, br );
}

static bool GetCol( Reader& r, std::vector<qword>& v, qword n ) {
  uint h = r.u8(), P = h&15, rp = h>>4;
  if( r.err || P>8 || rp>2 || n>MAXCOUNT ) return false;
  if( rp==0 ) { if( !GetRuns( r, v, n ) ) return false; }
  else if( rp==2 ) {
    v.resize( size_t(n) );
    for( size_t i=0; i<v.size(); i++ ) { v[i] = r.vint(); if( r.err ) return false; }
  } else {
    v.clear();
    while( v.size()<n ) {
      qword x = r.vint(); if( r.err ) return false;
      if( v.size() && v.back()==x ) {
        qword c = r.vint();
        if( r.err || c>=n-v.size() ) return false;
        v.insert( v.end(), size_t(c+1), x );
      } else v.push_back( x );
    }
  }
  if( P ) for( size_t k=0; k<v.size(); k++ ) v[k] = qword( UnZigZag( v[k] ) + (long long)(k>=P ? v[k-P] : 0) );
  return true;
}

// sorted list of distinct indices < lim: vint n, column of gaps (first index, then
// index - previous - 1)
static void PutIdxCol( Writer& w, const std::vector<qword>& v ) {
  w.vint( v.size() );
  if( v.empty() ) return;
  std::vector<qword> g( v.size() );
  for( size_t i=0; i<v.size(); i++ ) g[i] = i ? v[i]-v[i-1]-1 : v[i];
  PutCol( w, g );
}

static bool GetIdxCol( Reader& r, std::vector<qword>& v, qword lim ) {
  qword n = r.vint();
  if( r.err || n>lim || n>MAXCOUNT ) return false;
  v.clear();
  if( !n ) return true;
  if( !GetCol( r, v, n ) ) return false;
  qword p = 0;
  for( size_t i=0; i<v.size(); i++ ) {
    qword x = i ? p+1+v[i] : v[i];
    if( x<p || x>=lim ) return false;
    v[i] = p = x;
  }
  return true;
}

// column variant for counts that are often tiny: n<=4 values as plain vints
static void PutColN( Writer& w, const std::vector<qword>& v ) {
  if( v.size()<=4 ) { for( size_t i=0; i<v.size(); i++ ) w.vint( v[i] ); return; }
  PutCol( w, v );
}

static bool GetColN( Reader& r, std::vector<qword>& v, qword n ) {
  if( n<=4 ) { v.resize( size_t(n) ); for( size_t i=0; i<v.size(); i++ ) v[i] = r.vint(); return !r.err; }
  return GetCol( r, v, n );
}
