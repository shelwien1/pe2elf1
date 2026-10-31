// MKV: block timestamp models and interleave keys
namespace mkv {

static void ChooseTs( Track& t, int ckind, int cparam, qword tcs ) {
  size_t n = t.btc.size();
  std::vector<qword> fidx( n ), e( n ), best( n );
  qword nf = 0;
  for( size_t k=0; k<n; k++ ) fidx[k] = nf, nf += t.bnf[k];
  for( size_t k=0; k<n; k++ ) best[k] = ZigZag( int16_t( uint16_t( t.btc[k] - (k ? t.btc[k-1] : 0) ) ) );
  // size of the run-length coding of a column, without building it
  auto rlsize = []( const std::vector<qword>& v ) {
    size_t z = 1;
    for( size_t i=0, j; i<v.size(); i=j ) {
      for( j=i+1; j<v.size() && v[j]==v[i]; j++ );
      z += VintLen(v[i]<<1) + VintLen(qword(j-i-1)<<1);   // ~vint byte lengths
    }
    return z;
  };
  // no model: cluster Timestamps that equal the first block's time can't be predicted (0xFE)
  size_t cbonus = 0;
  for( size_t k=0; k<n; k++ ) if( (k==0 || t.bcts[k]!=t.bcts[k-1]) && t.btc[k]==0 ) cbonus += 3;
  size_t bsz = rlsize( best )+cbonus; int bc = -1, bk = 0; qword bm = 0, bo = 0;
  std::vector<double> A( n ), X( n );
  for( size_t k=0; k<n; k++ ) A[k] = double( (long long)t.bcts[k] + int16_t(uint16_t(t.btc[k])) );
  for( int c=0; c<2 && n>=2; c++ ) {
    int ck = c ? ckind : 0;
    if( c && (!ckind || t.cpos.size()<nf) ) continue;
    for( size_t k=0; k<n; k++ ) X[k] = double( ck ? t.cpos[size_t(fidx[k])] : fidx[k] );
    double mx = 0, ma = 0, sxx = 0, sxa = 0;
    for( size_t k=0; k<n; k++ ) mx += X[k], ma += A[k];
    mx /= double(n); ma /= double(n);
    for( size_t k=0; k<n; k++ ) sxx += (X[k]-mx)*(X[k]-mx), sxa += (X[k]-mx)*(A[k]-ma);
    if( !(sxx>0) ) continue;
    double s = sxa/sxx;
    if( !(s>0) || s>1e15 ) continue;
    std::vector<double> cand( 1, s );
    double x = s; long long h1 = 1, h2 = 0, k1 = 0, k2 = 1;
    for( int it=0; it<24; it++ ) {
      double a = floor(x);
      if( a>1e12 ) break;
      long long ai = (long long)a, h = ai*h1+h2, kk = ai*k1+k2;
      if( kk>100000000 || h>(1LL<<50) ) break;
      if( h>0 ) cand.push_back( double(h)/double(kk) );
      h2 = h1; h1 = h; k2 = k1; k1 = kk;
      double fr = x-a;
      if( fr<1e-12 ) break;
      x = 1/fr;
    }
    for( size_t ci=0; ci<cand.size(); ci++ ) {
      double cs = cand[ci];
      int k = 0;
      while( k<62 && ldexp(cs,k+1)<9.0e18 ) k++;
      qword m = qword( llround( ldexp(cs,k) ) );
      for( int jo=0; jo<32; jo++ ) {               // fractional offsets: they decide the rounding
        qword c = qword( llround( ldexp( jo/32.0, k ) ) );
        uint prev = 0;
        for( size_t j=0; j<n; j++ ) {
          qword xx = ck ? t.cpos[size_t(fidx[j])] : fidx[j];
          uint pr = uint( MulShift( xx, m, k, c ) - t.bcts[j] ) & 0xFFFF, r16 = (t.btc[j]-pr) & 0xFFFF;
          e[j] = ZigZag( int16_t(uint16_t(r16-prev)) ); prev = r16;
        }
        size_t sz = rlsize( e )+16;
        if( sz<bsz ) bsz = sz, bc = ck, bk = k, bm = m, bo = c, best.swap( e );
      }
    }
  }
  // piecewise model (appended parts, VFR): per block one of ns scales x 32 offsets, chosen
  // by a Viterbi search (costs ~ bytes: nonzero residual delta 4, state change 3)
  std::vector<qword> pst, pe, pks, pms; int pck = -1;
  size_t nz = 0; for( size_t j=0; j<n; j++ ) nz += best[j]!=0;
  if( n>=64 && nz>16 )
    for( int c=0; c<2; c++ ) {
      int ck = c ? ckind : 0;
      if( c && (!ckind || t.cpos.size()<nf) ) continue;
      if( !c && ckind==2 ) continue;                // audio: sample positions only
      // scales: the best single model's, and common frame durations near it
      double base = bc==ck && bc>=0 ? ldexp( double(bm), -bk ) : 0;
      std::vector<double> sc;
      if( base>0 ) sc.push_back( base );
      if( ck!=2 ) {
        static const double fps[] = { 24000/1001., 24, 25, 30000/1001., 30, 48, 50, 60000/1001., 60, 120000/1001., 120 };
        for( size_t i=0; i<sizeof(fps)/sizeof(fps[0]); i++ ) {
          double v = 1e9/fps[i]/double(tcs);
          if( base>0 && (v<base*0.5 || v>base*2) ) continue;
          bool dup = false; for( size_t q=0; q<sc.size(); q++ ) dup |= fabs(sc[q]-v)<1e-9*v;
          if( !dup && sc.size()<8 ) sc.push_back( v );
        }
      }
      size_t ns = sc.size(), S = ns*32;
      if( !ns || qword(n)*S>(qword(1)<<26) ) continue;
      std::vector<int> ks( ns ); std::vector<qword> ms( ns );
      for( size_t i=0; i<ns; i++ ) {
        int k = 0; while( k<62 && ldexp(sc[i],k+1)<9.0e18 ) k++;
        ks[i] = k; ms[i] = qword( llround( ldexp(sc[i],k) ) );
      }
      auto resid = [&]( size_t j, size_t st ) -> int {
        qword xx = ck ? t.cpos[size_t(fidx[j])] : fidx[j]; int kk = ks[st>>5];
        uint pr = uint( MulShift( xx, ms[st>>5], kk, (qword(st&31)<<kk)>>5 ) - t.bcts[j] ) & 0xFFFF;
        return int16_t( uint16_t( t.btc[j]-pr ) );
      };
      const long long NZ = 4, SW = 3, INF = (long long)1<<60;
      std::vector<long long> prev( S ), cur( S ); std::vector<int> rp( S ), rc( S );
      std::vector<byte> bp( n*S );
      for( size_t b=0; b<S; b++ ) rp[b] = resid( 0, b ), prev[b] = rp[b] ? NZ : 0;
      std::vector<std::pair<int,size_t> > tab( 1024 ); std::vector<long long> tcst( 1024 );
      for( size_t j=1; j<n; j++ ) {
        long long mn = INF; size_t am = 0;
        for( size_t a=0; a<S; a++ ) if( prev[a]<mn ) mn = prev[a], am = a;
        for( size_t h=0; h<tab.size(); h++ ) tab[h].second = size_t(-1);
        for( size_t a=0; a<S; a++ ) {               // per previous residual value: cheapest state
          size_t h = size_t(uint16_t(rp[a])*2654435761u) & 1023;
          while( tab[h].second!=size_t(-1) && tab[h].first!=rp[a] ) h = (h+1)&1023;
          if( tab[h].second==size_t(-1) || prev[a]<tcst[h] ) tab[h] = std::make_pair( rp[a], a ), tcst[h] = prev[a];
        }
        for( size_t b=0; b<S; b++ ) {
          rc[b] = resid( j, b );
          long long stay = prev[b] + (rc[b]!=rp[b] ? NZ : 0), sw = mn+NZ+SW; size_t from = am;
          size_t h = size_t(uint16_t(rc[b])*2654435761u) & 1023;
          while( tab[h].second!=size_t(-1) && tab[h].first!=rc[b] ) h = (h+1)&1023;
          if( tab[h].second!=size_t(-1) && tcst[h]+SW<sw ) sw = tcst[h]+SW, from = tab[h].second;
          if( stay<=sw ) cur[b] = stay, bp[j*S+b] = byte(b); else cur[b] = sw, bp[j*S+b] = byte(from);
        }
        prev.swap( cur ); rp.swap( rc );
      }
      size_t bs = 0; for( size_t b=1; b<S; b++ ) if( prev[b]<prev[bs] ) bs = b;
      std::vector<qword> st( n ), ee( n );
      for( size_t j=n; j-->0; ) { st[j] = bs; if( j ) bs = bp[j*S+bs]; }
      int pr = 0;
      for( size_t j=0; j<n; j++ ) { int r = resid( j, size_t(st[j]) ); ee[j] = ZigZag( int16_t(uint16_t(r-pr)) ); pr = r; }
      size_t sz = rlsize( ee )+rlsize( st )+4+ns*10;
      if( sz<bsz ) {
        bsz = sz; pck = ck; pst.swap( st ); pe.swap( ee );
        pks.assign( ks.begin(), ks.end() ); pms.swap( ms );
      }
    }
  if( pck>=0 ) {                                    // piecewise model wins
    t.tcol.swap( pe ); t.r16v.assign( n, 0 );
    t.tsm = 2; t.tck = pck; t.tcp = cparam; t.tst.swap( pst ); t.tsks.swap( pks ); t.tsms.swap( pms );
    uint r16 = 0; size_t nch = 0;
    for( size_t j=0; j<n; j++ ) r16 = (r16+uint(UnZigZag(t.tcol[j]))) & 0xFFFF, t.r16v[j] = r16, nch += j && t.tst[j]!=t.tst[j-1];
    char info[96];
    snprintf( info, sizeof(info), "ts=%s/piecewise*%d,%llu-changes", pck==0 ? "frame" : pck==1 ? "h264" : pck==3 ? "hevc" : "vorbis", int(t.tsks.size()), (unsigned long long)nch );
    t.tsinfo = info;
    return;
  }
  t.tcol.swap( best ); t.r16v.assign( n, 0 );
  char info[96];
  if( bc<0 ) { t.tsm = 0; snprintf( info, sizeof(info), "ts=delta" ); }
  else {
    t.tsm = 1; t.tck = bc; t.tcp = cparam; t.tk = bk; t.tm = bm; t.tc = bo;
    uint r16 = 0;
    for( size_t j=0; j<n; j++ ) r16 = (r16+uint(UnZigZag(t.tcol[j]))) & 0xFFFF, t.r16v[j] = r16;
    snprintf( info, sizeof(info), "ts=%s*%.6f%+.3f", bc==0 ? "frame" : bc==1 ? "h264" : bc==3 ? "hevc" : "vorbis", ldexp( double(bm), -bk ), ldexp( double(bo), -bk ) );
  }
  t.tsinfo = info;
}

// model time of track u's next block, and its interleave key (as the decoder computes them)
static qword EncModelAt( const Track& u, size_t j, qword x ) {
  if( u.tsm==1 ) return MulShift( x, u.tm, u.tk, u.tc );
  qword s = u.tst[j]; size_t si = size_t(s>>5); int kk = int(u.tsks[si]);
  return MulShift( x, u.tsms[si], kk, (qword(s&31)<<kk)>>5 );
}
static long long PtsKey( const Track& u ) {
  return (long long)EncModelAt( u, size_t(u.nbe), u.tck ? u.cpos[size_t(u.nfe)] : u.nfe ) + int16_t(uint16_t(u.r16v[size_t(u.nbe)]));
}
static long long IKey( const Track& u, qword cts ) {
  long long k = !u.tsm ? (long long)cts + int16_t(uint16_t(u.btc[size_t(u.nbe)])) :
                u.kmode ? (long long)EncModelAt( u, size_t(u.nbe), u.nfe ) : PtsKey( u );
  return k + u.kbias;
}

// interleave key parameters: kmode/kbias per track maximizing the order prediction hits
static void ChooseKeys( std::vector<Track>& T, const std::vector<Blk>& B ) {
  std::vector<int> seq; std::vector<size_t> tr;
  for( size_t i=0; i<T.size(); i++ ) if( T[i].ok && !T[i].blob && T[i].btc.size() ) tr.push_back( i );
  if( tr.size()<2 ) return;
  std::vector<int> map( T.size(), -1 );
  for( size_t i=0; i<tr.size(); i++ ) map[tr[i]] = int(i);
  for( size_t k=0; k<B.size(); k++ ) if( B[k].kind==K_BLOCK && map[size_t(B[k].trk)]>=0 ) seq.push_back( map[size_t(B[k].trk)] );
  if( seq.size()>20000 ) seq.resize( 20000 );      // the search runs on a prefix
  size_t n = tr.size();
  // per track, per block: key parts
  std::vector<std::vector<long long> > pk( n ), dk( n ); std::vector<std::vector<qword> > ck( n );
  for( size_t i=0; i<n; i++ ) {
    Track& t = T[tr[i]]; size_t nb = t.btc.size();
    pk[i].resize( nb ); dk[i].resize( nb ); ck[i].resize( nb );
    qword nfe0 = t.nfe, nbe0 = t.nbe; t.nfe = 0;
    for( size_t j=0; j<nb; j++ ) {
      t.nbe = j; ck[i][j] = t.bcts[j];
      if( t.tsm ) pk[i][j] = PtsKey( t ), dk[i][j] = (long long)EncModelAt( t, j, t.nfe );
      else pk[i][j] = dk[i][j] = int16_t(uint16_t(t.btc[j]));
      t.nfe += t.bnf[j];
    }
    t.nfe = nfe0; t.nbe = nbe0;
  }
  std::vector<int> km( n, 0 ); std::vector<long long> kb( n, 0 );
  auto hits = [&]() {
    std::vector<size_t> p( n, 0 ); size_t h = 0;
    for( size_t s=0; s<seq.size(); s++ ) {
      size_t a = size_t(seq[s]); qword cts = ck[a][p[a]];
      size_t bt = n; long long bk = 0;
      for( size_t i=0; i<n; i++ ) {
        if( p[i]>=pk[i].size() || T[tr[i]].seg!=T[tr[a]].seg ) continue;
        const Track& t = T[tr[i]];
        long long key = (!t.tsm ? (long long)cts : 0) + (km[i] ? dk[i][p[i]] : pk[i][p[i]]) + kb[i];
        if( bt==n || key<bk ) bt = i, bk = key;
      }
      h += bt==a; p[a]++;
    }
    return h;
  };
  size_t best = hits();
  if( best*1000>=seq.size()*995 ) return;
  static const long long bias[] = { 0, 1, -1, 2, -2, 4, -4, 8, -8, 12, -12, 16, -16, 24, -24, 32, -32, 48, -48, 64, -64,
    96, -96, 128, -128, 192, -192, 256, -256, 384, -384, 512, -512, 768, -768, 1024, -1024, 2048, -2048 };
  for( int round=0; round<2; round++ )
    for( size_t i=0; i<n; i++ ) {
      const Track& t = T[tr[i]];
      int bkm = km[i]; long long bkb = kb[i];
      for( int m=0; m<(t.tsm && t.tck!=2 ? 2 : 1); m++ ) {
        long long mb = 0; size_t mh = 0;              // coarse, then fine around the best
        for( size_t j=0; j<sizeof(bias)/sizeof(bias[0]); j++ ) {
          km[i] = m; kb[i] = bias[j];
          size_t h = hits();
          if( h>mh ) mh = h, mb = bias[j];
        }
        for( long long dlt=-40; dlt<=40; dlt++ ) {
          km[i] = m; kb[i] = mb+dlt;
          size_t h = hits();
          if( h>mh ) mh = h, mb += dlt, dlt = -41;  // restart around the new best
        }
        if( mh>best ) best = mh, bkm = m, bkb = mb;
      }
      km[i] = bkm; kb[i] = bkb;
    }
  for( size_t i=0; i<n; i++ ) T[tr[i]].kmode = km[i], T[tr[i]].kbias = kb[i];
}

static void WriteTs( const Track& t, Writer& w ) {
  w.u8( t.tsm );
  if( t.tsm ) {
    w.u8( t.tck );
    if( t.tck==1 || t.tck==3 ) w.u8( t.tcp );
    if( t.tsm==1 ) w.u8( t.tk ), w.vint( t.tm ), w.vint( t.tc );
    else {
      w.u8( t.tsks.size() );
      for( size_t i=0; i<t.tsks.size(); i++ ) w.u8( t.tsks[i] ), w.vint( t.tsms[i] );
      PutCol( w, t.tst );
    }
  }
  PutCol( w, t.tcol );
}

} // namespace mkv
