// per-sample video features: VF_KEY = IDR/IRAP, VF_RP = recovery point SEI, VF_I = intra first slice
enum { VF_KEY=1, VF_RP=2, VF_I=4, VF_NREF=8 };   // NREF: non-reference picture

struct H264State {                           // POC per picture (types 0/1/2; MMCO5 ignored)
  struct Sps { bool ok; int log2mfn, poctype, log2mpl, dpoaz, offnr, offt2b, ncyc, fmo, sepc; int cyc[256]; };
  struct Pps { bool ok; int sps, bfpo; };
  std::vector<Sps> sps; std::vector<Pps> pps;
  int prevMsb, prevLsb, prevFN; long long prevFNO;
  H264State() : sps(32), pps(256), prevMsb(0), prevLsb(0), prevFN(0), prevFNO(0) {
    for( size_t i=0; i<sps.size(); i++ ) sps[i].ok = false;
    for( size_t i=0; i<pps.size(); i++ ) pps[i].ok = false;
  }
  void Sps_( const byte* p, size_t n ) {
    BitsMSB r; r.Load( p+1, n-1, 4096 );
    Sps s; memset( &s, 0, sizeof(s) );
    int prof = r.u(8); r.u(16); uint id = r.ue();
    if( id>31 ) return;
    if( prof==100 || prof==110 || prof==122 || prof==244 || prof==44 || prof==83 || prof==86 ||
        prof==118 || prof==128 || prof==138 || prof==139 || prof==134 || prof==135 ) {
      uint cf = r.ue();
      if( cf==3 ) s.sepc = r.u(1);
      r.ue(); r.ue(); r.u(1);
      if( r.u(1) )
        for( int i=0; i<(cf!=3 ? 8 : 12) && !r.err; i++ )
          if( r.u(1) )
            for( int j=0, sz=i<6 ? 16 : 64, last=8, next=8; j<sz && !r.err; j++ ) {
              if( next ) next = (last + r.se() + 256) & 255;
              if( next ) last = next;
            }
    }
    s.log2mfn = r.ue()+4; s.poctype = r.ue();
    if( s.poctype==0 ) s.log2mpl = r.ue()+4;
    else if( s.poctype==1 ) {
      s.dpoaz = r.u(1); s.offnr = r.se(); s.offt2b = r.se(); s.ncyc = r.ue();
      if( s.ncyc>255 ) return;
      for( int i=0; i<s.ncyc; i++ ) s.cyc[i] = r.se();
    }
    r.ue(); r.u(1); r.ue(); r.ue(); s.fmo = r.u(1);
    if( r.err || s.log2mfn>16 || s.poctype>2 || (s.poctype==0 && s.log2mpl>16) ) return;
    s.ok = true; sps[id] = s;
  }
  void Pps_( const byte* p, size_t n ) {
    BitsMSB r; r.Load( p+1, n-1, 64 );
    uint id = r.ue(), sid = r.ue(); r.u(1); int bf = r.u(1);
    if( r.err || id>255 || sid>31 ) return;
    pps[id].ok = true; pps[id].sps = int(sid); pps[id].bfpo = bf;
  }
  bool Slice( const byte* p, size_t n, long long& poc, bool& intra ) {
    int ref = (p[0]>>5)&3; bool idr = (p[0]&31)==5;
    BitsMSB r; r.Load( p+1, n-1, 96 );
    r.ue(); uint st = r.ue(); uint pid = r.ue();
    if( r.err || pid>255 || !pps[pid].ok || !sps[pps[pid].sps].ok ) return false;
    intra = st%5==2 || st%5==4;
    const Pps& pp = pps[pid]; const Sps& s = sps[pp.sps];
    if( s.sepc ) r.u(2);
    int fn = r.u( s.log2mfn ), field = 0, bottom = 0;
    if( !s.fmo ) { field = r.u(1); if( field ) bottom = r.u(1); }
    if( idr ) r.ue();
    int lsb = 0, dpb = 0, dp0 = 0, dp1 = 0;
    if( s.poctype==0 ) { lsb = r.u( s.log2mpl ); if( pp.bfpo && !field ) dpb = r.se(); }
    else if( s.poctype==1 && !s.dpoaz ) { dp0 = r.se(); if( pp.bfpo && !field ) dp1 = r.se(); }
    if( r.err ) return false;
    long long top, bot;
    if( s.poctype==0 ) {
      if( idr ) prevMsb = prevLsb = 0;
      int mx = 1<<s.log2mpl, msb = prevMsb;
      if( lsb<prevLsb && prevLsb-lsb>=mx/2 ) msb = prevMsb+mx;
      else if( lsb>prevLsb && lsb-prevLsb>mx/2 ) msb = prevMsb-mx;
      top = (long long)msb+lsb; bot = field ? top : top+dpb;
      if( ref ) prevMsb = msb, prevLsb = lsb;
    } else {
      long long fno = idr ? 0 : prevFN>fn ? prevFNO+(1LL<<s.log2mfn) : prevFNO;
      if( s.poctype==1 ) {
        long long afn = s.ncyc ? fno+fn : 0, e = 0;
        if( !ref && afn>0 ) afn--;
        if( afn>0 ) {
          long long dc = 0; for( int i=0; i<s.ncyc; i++ ) dc += s.cyc[i];
          e = (afn-1)/s.ncyc*dc;
          for( int i=0; i<=int((afn-1)%s.ncyc); i++ ) e += s.cyc[i];
        }
        if( !ref ) e += s.offnr;
        top = e+dp0; bot = field ? e+s.offt2b+dp0 : top+s.offt2b+dp1;
      } else top = bot = idr ? 0 : 2*(fno+fn)-(ref ? 0 : 1);
      prevFNO = fno; prevFN = fn;
    }
    poc = field ? (bottom ? bot : top) : std::min( top, bot );
    return true;
  }
  // one NAL; vcl: first slice of the picture already seen; reset: POC period starts
  void Nal( const byte* p, size_t n, bool& vcl, int& fl, long long& poc, bool& reset ) {
    if( n<2 ) return;
    int ty = p[0]&31;
    if( ty==7 ) Sps_( p, n );
    else if( ty==8 ) Pps_( p, n );
    else if( ty==6 ) {                       // SEI: recovery point?
      BitsMSB r; r.Load( p+1, n-1, 256 );
      while( !r.err && r.pos+16<=8*r.b.size() ) {
        uint t = 0, s = 0, c;
        do { c = r.u(8); t += c; } while( c==255 && !r.err );
        do { c = r.u(8); s += c; } while( c==255 && !r.err );
        if( r.err ) break;
        if( t==6 ) { fl |= VF_RP; break; }
        r.skip( size_t(s)*8 );
      }
    } else if( (ty==1 || ty==5) && !vcl ) {
      long long q; bool in = false;
      if( Slice( p, n, q, in ) ) {
        vcl = true; poc = q;
        if( ty==5 ) fl |= VF_KEY, reset = true;
        if( in ) fl |= VF_I;
        if( !(p[0]&0x60) ) fl |= VF_NREF;
      }
    }
  }
};
