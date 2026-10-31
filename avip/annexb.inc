// avcC / hvcC parameter sets -> Annex B header
static bool AvcHdr( const byte* p, qword n, Buf& h, PosLen* pl=0 ) {
  static const byte sc[4] = {0,0,0,1};
  h.clear(); if( pl ) pl->clear();
  if( n<6 ) return false;
  qword o = 5;
  for( int k=0; k<3; k++ ) {             // SPS, PPS, SPS ext (high profiles; optional, dropped if broken)
    if( k==2 ) {
      uint prof = p[1];
      if( !(prof==100 || prof==110 || prof==122 || prof==144) || n-o<4 ) break;
      o += 3;
    }
    if( o>=n ) return false;
    uint cnt = k==0 ? (p[o]&31) : p[o]; o++;
    size_t mark = h.size(), pmark = pl ? pl->size() : 0;
    for( uint i=0; i<cnt; i++ ) {
      if( n-o<2 || BE16(p+o)>n-o-2 ) { if( k<2 ) return false; h.resize(mark); if( pl ) pl->resize(pmark); return true; }
      uint L = BE16(p+o); o+=2;
      if( pl ) pl->push_back( std::make_pair( o, qword(L) ) );
      h.insert( h.end(), sc, sc+4 ); h.insert( h.end(), p+o, p+o+L ); o+=L;
    }
  }
  return true;
}

static bool HvcHdr( const byte* p, qword n, Buf& h, PosLen* pl=0 ) {
  h.clear(); if( pl ) pl->clear();
  if( n<23 ) return false;
  qword o = 22; uint na = p[o++];
  for( uint a=0; a<na; a++ ) {
    if( n-o<3 ) return false;
    uint cnt = BE16(p+o+1); o+=3;
    for( uint i=0; i<cnt; i++ ) {
      if( n-o<2 ) return false;
      uint L = BE16(p+o); o+=2;
      if( L>n-o ) return false;
      if( pl ) pl->push_back( std::make_pair( o, qword(L) ) );
      static const byte sc[4] = {0,0,0,1};
      h.insert( h.end(), sc, sc+4 ); h.insert( h.end(), p+o, p+o+L ); o+=L;
    }
  }
  return true;
}
