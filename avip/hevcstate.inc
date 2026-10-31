struct HevcState {                           // POC per picture (layer 0)
  struct Sps { bool ok; int log2poc, sepc; };
  struct Pps { bool ok; int sps, dep, outfl, extra; };
  std::vector<Sps> sps; std::vector<Pps> pps;
  int prevTid0Poc; bool first;
  HevcState() : sps(16), pps(64), prevTid0Poc(0), first(true) {
    for( size_t i=0; i<sps.size(); i++ ) sps[i].ok = false;
    for( size_t i=0; i<pps.size(); i++ ) pps[i].ok = false;
  }
  void Sps_( const byte* p, size_t n ) {
    BitsMSB r; r.Load( p+2, n-2, 1024 );
    r.u(4); int msl = r.u(3); r.u(1);
    r.skip( 96 );                            // general profile/tier/level
    int pp[8], lp[8];
    for( int i=0; i<msl; i++ ) pp[i] = r.u(1), lp[i] = r.u(1);
    if( msl>0 ) for( int i=msl; i<8; i++ ) r.u(2);
    for( int i=0; i<msl; i++ ) { if( pp[i] ) r.skip( 88 ); if( lp[i] ) r.skip( 8 ); }
    uint id = r.ue();
    if( id>15 ) return;
    Sps s; s.sepc = 0;
    uint cf = r.ue(); if( cf==3 ) s.sepc = r.u(1);
    r.ue(); r.ue();
    if( r.u(1) ) r.ue(), r.ue(), r.ue(), r.ue();
    r.ue(); r.ue();
    s.log2poc = int(r.ue())+4;
    if( r.err || s.log2poc>16 ) return;
    s.ok = true; sps[id] = s;
  }
  void Pps_( const byte* p, size_t n ) {
    BitsMSB r; r.Load( p+2, n-2, 64 );
    uint id = r.ue(), sid = r.ue();
    Pps q; q.dep = r.u(1); q.outfl = r.u(1); q.extra = r.u(3);
    if( r.err || id>63 || sid>15 ) return;
    q.ok = true; q.sps = int(sid); pps[id] = q;
  }
  void Nal( const byte* p, size_t n, bool& vcl, int& fl, long long& poc, bool& reset ) {
    if( n<3 ) return;
    int ty = (p[0]>>1)&63, layer = ((p[0]&1)<<5)|(p[1]>>3), tid = (p[1]&7)-1;
    if( layer ) return;
    if( ty==33 ) Sps_( p, n );
    else if( ty==34 ) Pps_( p, n );
    else if( ty==39 ) {                      // prefix SEI: recovery point?
      BitsMSB r; r.Load( p+2, n-2, 256 );
      while( !r.err && r.pos+16<=8*r.b.size() ) {
        uint t = 0, s = 0, c;
        do { c = r.u(8); t += c; } while( c==255 && !r.err );
        do { c = r.u(8); s += c; } while( c==255 && !r.err );
        if( r.err ) break;
        if( t==6 ) { fl |= VF_RP; break; }
        r.skip( size_t(s)*8 );
      }
    } else if( ty<32 && !vcl ) {
      BitsMSB r; r.Load( p+2, n-2, 64 );
      if( !r.u(1) ) return;                  // not the first slice segment of a picture
      bool irap = ty>=16 && ty<=23, idr = ty==19 || ty==20;
      if( irap ) r.u(1);
      uint pid = r.ue();
      if( r.err || pid>63 || !pps[pid].ok || !sps[pps[pid].sps].ok ) return;
      const Pps& q = pps[pid]; const Sps& s = sps[q.sps];
      r.u( q.extra ); uint st = r.ue();
      if( q.outfl ) r.u(1);
      if( s.sepc ) r.u(2);
      int lsb = idr ? 0 : int( r.u( s.log2poc ) );
      if( r.err ) return;
      bool norasl = idr || (ty>=16 && ty<=18) || (ty==21 && first);
      int mx = 1<<s.log2poc, msb = 0;
      if( !norasl ) {
        int pl = prevTid0Poc & (mx-1), pm = prevTid0Poc - pl;
        msb = pm;
        if( lsb<pl && pl-lsb>=mx/2 ) msb = pm+mx;
        else if( lsb>pl && lsb-pl>mx/2 ) msb = pm-mx;
      }
      int pc = msb+lsb;
      if( tid==0 && !(ty<=14 && !(ty&1)) && !(ty>=6 && ty<=9) ) prevTid0Poc = pc;   // not RADL/RASL/sub-layer non-ref
      vcl = true; poc = pc; first = false;
      if( irap ) fl |= VF_KEY;
      if( norasl ) reset = true;
      if( st==2 ) fl |= VF_I;
      if( ty<=14 && !(ty&1) ) fl |= VF_NREF;
    }
  }
};
