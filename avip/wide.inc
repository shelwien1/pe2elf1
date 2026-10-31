// exact 128-bit products for cross-timescale time comparisons
struct U128 {
  qword hi, lo;
  static U128 Mul( qword x, qword m ) {
    qword xl = x&0xFFFFFFFFu, xh = x>>32, ml = m&0xFFFFFFFFu, mh = m>>32;
    qword ll = xl*ml, lh = xl*mh, hl = xh*ml, hh = xh*mh;
    qword mid = (ll>>32) + (lh&0xFFFFFFFFu) + (hl&0xFFFFFFFFu);
    U128 r; r.lo = (mid<<32) | (ll&0xFFFFFFFFu); r.hi = hh + (lh>>32) + (hl>>32) + (mid>>32);
    return r;
  }
  U128 Add( qword v ) const { U128 r = *this; r.lo += v; r.hi += r.lo<v; return r; }
  bool Lt( const U128& b ) const { return hi!=b.hi ? hi<b.hi : lo<b.lo; }
  bool Le( const U128& b ) const { return !b.Lt( *this ); }
  // this-b clamped to +-2^62
  long long Diff( const U128& b ) const {
    const long long L = 1LL<<62;
    if( Lt( b ) ) { long long d = -b.Diff( *this ); return d; }
    qword h = hi-b.hi-(lo<b.lo), l = lo-b.lo;
    return h || l>qword(L) ? L : (long long)l;
  }
};

// floor(a*b*2^16/c), saturated at 2^62 (exact; chunk time keys)
static qword TimeKey( qword a, qword b, qword c ) {
  const qword SAT = qword(1)<<62;
  if( !c ) return SAT;
  qword al = a&0xFFFFFFFFu, ah = a>>32, bl = b&0xFFFFFFFFu, bh = b>>32;
  qword ll = al*bl, lh = al*bh, hl = ah*bl, hh = ah*bh;
  qword mid = (ll>>32) + (lh&0xFFFFFFFFu) + (hl&0xFFFFFFFFu);
  qword lo = (mid<<32) | (ll&0xFFFFFFFFu), hi = hh + (lh>>32) + (hl>>32) + (mid>>32);
  if( hi>>48 ) return SAT;
  hi = (hi<<16) | (lo>>48); lo <<= 16;
  if( hi>=c ) return SAT;
  qword q = 0, r = hi;
  for( int i=63; i>=0; i-- ) {
    bool top = (r>>63)!=0;
    r = (r<<1) | ((lo>>i)&1);
    if( top || r>=c ) r -= c, q |= qword(1)<<i;
  }
  return q<SAT ? q : SAT;
}
