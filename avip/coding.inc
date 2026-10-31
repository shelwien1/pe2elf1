static qword ZigZag( long long v ) { return v<0 ? 2*qword(-(v+1))+1 : 2*qword(v); }

static long long UnZigZag( qword z ) { return (z&1) ? -(long long)(z>>1)-1 : (long long)(z>>1); }

// run-length coded column: vint nruns, nruns x (vint value, vint runlength-1)
static void PutRuns( Writer& w, const std::vector<qword>& v ) {
  std::vector<std::pair<qword,qword> > r;
  for( size_t i=0; i<v.size(); i++ )
    if( r.size() && r.back().first==v[i] ) r.back().second++;
    else r.push_back( std::make_pair( v[i], qword(1) ) );
  w.vint( r.size() );
  for( size_t i=0; i<r.size(); i++ ) w.vint( r[i].first ), w.vint( r[i].second-1 );
}

// n = expected length, or ~0 for "any"
static bool GetRuns( Reader& r, std::vector<qword>& v, qword n ) {
  qword nr = r.vint(), cap = n==~qword(0) ? MAXCOUNT : n;
  v.clear();
  for( qword i=0; i<nr && !r.err; i++ ) {
    qword x = r.vint(), l = r.vint()+1;
    if( r.err || l==0 || l>cap-v.size() ) return false;
    v.insert( v.end(), size_t(l), x );
  }
  return !r.err && (n==~qword(0) || v.size()==n);
}

static size_t VintSize( qword a ) { size_t z = 1; while( a>=0x80 ) z++, a>>=7; return z; }

static size_t RunsCost( const std::vector<qword>& v ) {
  size_t z = 1;
  for( size_t i=0, j; i<v.size(); i=j ) {
    for( j=i+1; j<v.size() && v[j]==v[i]; j++ );
    z += VintSize( v[i] ) + VintSize( j-i-1 );
  }
  return z;
}




// round((x*m + c) / 2^k) mod 2^64, k in 0..63; exact integer math, same result everywhere
static qword MulShift( qword x, qword m, int k, qword c ) {
  qword xl = x&0xFFFFFFFFu, xh = x>>32, ml = m&0xFFFFFFFFu, mh = m>>32;
  qword ll = xl*ml, lh = xl*mh, hl = xh*ml, hh = xh*mh;
  qword mid = (ll>>32) + (lh&0xFFFFFFFFu) + (hl&0xFFFFFFFFu);
  qword lo = (mid<<32) | (ll&0xFFFFFFFFu), hi = hh + (lh>>32) + (hl>>32) + (mid>>32);
  lo += c; hi += lo<c;
  if( k==0 ) return lo;
  qword a = qword(1)<<(k-1); lo += a; hi += lo<a;
  return (lo>>k) | (hi<<(64-k));
}
