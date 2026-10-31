// MP4: sample timing model and chunk alignment
namespace mp4 {

//--- sample timing / video features (shared) ----------------------------------

// timing model inputs. ckind: 1 = H.264, 2 = HEVC (display order, features), 3 = Vorbis
// sample positions, 4 = key frames by frame class (fck, fckp); 0 = none. x[n] (n<=N): model
// abscissa; disp/fl: video only.
struct TimeIn { std::vector<qword> x, disp; std::vector<byte> fl; };

static bool TimeInputs( int ckind, int lensize, const Buf& hdr, const Buf& priv, const byte* s,
                        const std::vector<qword>& fo, qword nf, TimeIn& ti, int fck=0, uint fckp=0 ) {
  ti.x.resize( size_t(nf+1) ); ti.disp.clear(); ti.fl.clear();
  for( qword n=0; n<=nf; n++ ) ti.x[size_t(n)] = n;
  if( ckind==1 || ckind==2 ) {
    if( lensize<1 || lensize>4 ) return false;
    if( ckind==1 ) VideoFeat<H264State>( hdr.data(), hdr.size(), s, fo, nf, lensize, ti.disp, ti.fl );
    else VideoFeat<HevcState>( hdr.data(), hdr.size(), s, fo, nf, lensize, ti.disp, ti.fl );
  } else if( ckind==3 ) {
    const byte* hp[3]; qword hl[3]; VorbisInfo vi;
    if( !SplitXiphHeaders( priv.data(), priv.size(), hp, hl ) || !VorbisParse( hp, hl, vi ) ) return false;
    uint prev = 0; qword g = 0;
    for( qword f=0; f<nf; f++ ) ti.x[size_t(f)] = g, g += VorbisDur( vi, s+fo[size_t(f)], fo[size_t(f+1)]-fo[size_t(f)], prev );
    ti.x[size_t(nf)] = g;
  } else if( ckind==4 ) {
    ti.fl.resize( size_t(nf) );
    for( qword f=0; f<nf; f++ ) ti.fl[size_t(f)] = FrameClass( fck, s+fo[size_t(f)], fo[size_t(f+1)]-fo[size_t(f)], fckp )&FC_KEY ? VF_KEY : 0;
  } else if( ckind ) return false;
  return true;
}

// timing model parameters (see metainfo format)
struct TimeModel {
  int ckind, lensize, k, ks, smask; qword q, m, c; long long D; bool hascto, hassync;
  int fck; uint fckp;                       // ckind 4: frame class kind and parameter
  std::vector<qword> rdur, rcto;            // residual columns (zigzag)
  std::vector<qword> sexc;                  // sync exceptions (sample indices)
  bool hassdtp; byte smap[8]; std::vector<qword> sdexc, sdval;   // sdtp: value per feature class, exceptions
  qword Model( qword x ) const { return q*MulShift( x, m, k, c ); }
};

// durations and decode times from the model + residuals
static void TimeDur( const TimeModel& tm, const TimeIn& ti, qword N, std::vector<qword>& dur, std::vector<qword>& dts ) {
  dur.resize( size_t(N) ); dts.resize( size_t(N+1) ); dts[0] = 0;
  for( qword n=0; n<N; n++ ) {
    dur[size_t(n)] = tm.Model( ti.x[size_t(n+1)] ) - tm.Model( ti.x[size_t(n)] ) + qword( UnZigZag( tm.rdur[size_t(n)] ) );
    dts[size_t(n+1)] = dts[size_t(n)] + dur[size_t(n)];
  }
}

// durations, composition offsets, sync flags from the model + residuals
static void TimeApply( const TimeModel& tm, const TimeIn& ti, qword N,
                       std::vector<qword>& dur, std::vector<qword>& dts, std::vector<long long>& cto, std::vector<byte>& sync ) {
  TimeDur( tm, ti, N, dur, dts );
  cto.assign( size_t(N), 0 );
  if( tm.hascto )
    for( qword n=0; n<N; n++ ) {
      qword j = (ti.disp.size() ? ti.disp[size_t(n)] : n) + qword(tm.ks);
      cto[size_t(n)] = (long long)( dts[size_t(std::min(j,N))] - dts[size_t(n)] ) + tm.D + UnZigZag( tm.rcto[size_t(n)] );
    }
  sync.assign( size_t(N), 1 );
  if( tm.hassync ) {
    for( qword n=0; n<N; n++ ) sync[size_t(n)] = ti.fl.size() ? SyncPred( ti.fl[size_t(n)], tm.smask ) : 0;
    for( size_t i=0; i<tm.sexc.size(); i++ ) sync[size_t(tm.sexc[i])] ^= 1;
  }
}

static void PutTimeModel( Writer& w, const TimeModel& tm ) {
  w.u8( tm.ckind ); if( tm.ckind==1 || tm.ckind==2 ) w.u8( tm.lensize );
  if( tm.ckind==4 ) w.u8( tm.fck ), w.vint( tm.fckp );
  w.vint( tm.q ); w.u8( tm.k ); w.vint( tm.m ); w.vint( tm.c );
  PutCol( w, tm.rdur );
  w.u8( tm.hascto );
  if( tm.hascto ) w.u8( tm.ks ), w.vint( ZigZag( tm.D ) ), PutCol( w, tm.rcto );
  w.u8( tm.hassync ? tm.smask : 0xFF );
  if( tm.hassync ) PutIdxCol( w, tm.sexc );
  w.u8( tm.hassdtp );
  if( tm.hassdtp ) {
    w.bytes( tm.smap, 8 ); PutIdxCol( w, tm.sdexc );
    for( size_t i=0; i<tm.sdval.size(); i++ ) w.u8( uint(tm.sdval[i]) );
  }
}

static bool GetTimeModel( Reader& r, TimeModel& tm, qword N ) {
  tm.ckind = r.u8(); tm.lensize = 0;
  if( tm.ckind==1 || tm.ckind==2 ) tm.lensize = r.u8();
  tm.fck = 0; tm.fckp = 0;
  if( tm.ckind==4 ) { tm.fck = r.u8(); qword p = r.vint(); if( p>0xFFFFFFFFu || tm.fck>CK_VC1 ) return false; tm.fckp = uint(p); }
  tm.q = r.vint(); tm.k = r.u8(); tm.m = r.vint(); tm.c = r.vint();
  if( r.err || tm.ckind>4 || tm.k>63 ) return false;
  if( !GetCol( r, tm.rdur, N ) ) return false;
  tm.hascto = r.u8()!=0; tm.ks = 0; tm.D = 0;
  if( tm.hascto ) {
    tm.ks = r.u8(); tm.D = UnZigZag( r.vint() );
    if( r.err || tm.ks>16 || !GetCol( r, tm.rcto, N ) ) return false;
  }
  uint sm = r.u8(); tm.hassync = sm!=0xFF; tm.smask = tm.hassync ? int(sm) : 0;
  if( tm.hassync && !GetIdxCol( r, tm.sexc, N ) ) return false;
  tm.hassdtp = r.u8()!=0;
  if( tm.hassdtp ) {
    const byte* p = r.bytes(8); if( !p ) return false;
    memcpy( tm.smap, p, 8 );
    if( !GetIdxCol( r, tm.sdexc, N ) ) return false;
    tm.sdval.resize( tm.sdexc.size() );
    for( size_t i=0; i<tm.sdval.size(); i++ ) tm.sdval[i] = r.u8();
  }
  return !r.err;
}

static int SdtpClass( const TimeIn& ti, qword n ) {
  byte f = ti.fl.size() ? ti.fl[size_t(n)] : 0;
  return (f&VF_KEY ? 1 : 0) | (f&VF_I ? 2 : 0) | (f&VF_NREF ? 4 : 0);
}

// sdtp bytes of the first n samples
static void SdtpApply( const TimeModel& tm, const TimeIn& ti, qword n, Buf& v ) {
  v.resize( size_t(n) );
  for( qword k=0; k<n; k++ ) v[size_t(k)] = tm.smap[SdtpClass( ti, k )];
  for( size_t i=0; i<tm.sdexc.size(); i++ ) if( tm.sdexc[i]<n ) v[size_t(tm.sdexc[i])] = byte(tm.sdval[i]);
}

static void SdtpFit( TimeModel& tm, const TimeIn& ti, const Buf& sd ) {
  tm.hassdtp = true; tm.sdexc.clear(); tm.sdval.clear();
  qword h[8][256]; memset( h, 0, sizeof(h) );
  for( size_t k=0; k<sd.size(); k++ ) h[SdtpClass( ti, k )][sd[k]]++;
  for( int c=0; c<8; c++ ) { int b = 0; for( int v=1; v<256; v++ ) if( h[c][v]>h[c][b] ) b = v; tm.smap[c] = byte(b); }
  for( size_t k=0; k<sd.size(); k++ ) if( sd[k]!=tm.smap[SdtpClass( ti, k )] ) tm.sdexc.push_back( k ), tm.sdval.push_back( sd[k] );
}

// trun sample_flags derived from the sample class (tflags 16): a value per class (as sdtp),
// exceptions. Key frames have flags of their own (sync, depends on no other sample)
struct FlagMap { uint v[8]; std::vector<qword> exc, val; };

static void FlagsApply( const FlagMap& fm, const TimeIn& ti, qword N, std::vector<uint>& f ) {
  f.resize( size_t(N) );
  for( qword n=0; n<N; n++ ) f[size_t(n)] = fm.v[SdtpClass( ti, n )];
  for( size_t i=0; i<fm.exc.size(); i++ ) f[size_t(fm.exc[i])] = uint(fm.val[i]);
}

// sv: (sample index, flags) of the samples whose flags are derived, ascending
static void FlagsFit( FlagMap& fm, const TimeIn& ti, const std::vector<std::pair<qword,uint> >& sv ) {
  std::map<uint,qword> h[8];
  for( size_t k=0; k<sv.size(); k++ ) h[SdtpClass( ti, sv[k].first )][sv[k].second]++;
  for( int c=0; c<8; c++ ) {
    qword bc = 0; fm.v[c] = 0;
    for( std::map<uint,qword>::iterator it=h[c].begin(); it!=h[c].end(); ++it ) if( it->second>bc ) bc = it->second, fm.v[c] = it->first;
  }
  fm.exc.clear(); fm.val.clear();
  for( size_t k=0; k<sv.size(); k++ )
    if( sv[k].second!=fm.v[SdtpClass( ti, sv[k].first )] ) fm.exc.push_back( sv[k].first ), fm.val.push_back( sv[k].second );
}

static void PutFlagMap( Writer& w, const FlagMap& fm ) {
  for( int c=0; c<8; c++ ) w.vint( fm.v[c] );
  PutIdxCol( w, fm.exc ); PutCol( w, fm.val );
}

static bool GetFlagMap( Reader& r, FlagMap& fm, qword N ) {
  for( int c=0; c<8; c++ ) { qword v = r.vint(); if( v>0xFFFFFFFFu ) return false; fm.v[c] = uint(v); }
  if( r.err || !GetIdxCol( r, fm.exc, N ) || !GetCol( r, fm.val, fm.exc.size() ) ) return false;
  for( size_t i=0; i<fm.val.size(); i++ ) if( fm.val[i]>0xFFFFFFFFu ) return false;
  return true;
}

// encoder: fit the model to actual durations / offsets / sync flags
static void TimeFit( TimeModel& tm, const TimeIn& ti, qword N, const std::vector<qword>& dur,
                     const std::vector<long long>* cto, const std::vector<byte>* sync ) {
  std::vector<qword> dts( (size_t)(N+1), 0 ), e( (size_t)N );
  for( qword n=0; n<N; n++ ) dts[size_t(n+1)] = dts[size_t(n)]+dur[size_t(n)];
  // baseline: no model (residual = duration)
  tm.q = 1; tm.k = 0; tm.m = 0; tm.c = 0; tm.rdur = dur;
  for( size_t n=0; n<dur.size(); n++ ) tm.rdur[n] = ZigZag( (long long)dur[n] );
  size_t best = RunsCost( tm.rdur );
  // quantum: gcd of the durations but the last
  qword g = 0;
  for( qword n=0; n+1<N; n++ ) { qword a = dur[size_t(n)], b = g; while( b ) { qword t = a%b; a = b; b = t; } g = a; }
  qword qs[2] = { g ? g : 1, 1 };
  for( int qi=0; qi<(g>1 ? 2 : 1) && N>=2 && best>12; qi++ ) {     // (a model costs >12)
    qword q = qs[qi];
    double mx = 0, my = 0, sxx = 0, sxy = 0;
    for( qword n=0; n<=N; n++ ) mx += double(ti.x[size_t(n)]), my += double(dts[size_t(n)]/q);
    mx /= double(N+1); my /= double(N+1);
    for( qword n=0; n<=N; n++ ) { double a = double(ti.x[size_t(n)])-mx; sxx += a*a; sxy += a*(double(dts[size_t(n)]/q)-my); }
    if( !(sxx>0) ) continue;
    double s = sxy/sxx;
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
      for( int jo=0; jo<32; jo++ ) {
        qword c = qword( llround( ldexp( jo/32.0, k ) ) );
        qword p0 = q*MulShift( ti.x[0], m, k, c );
        for( qword n=0; n<N; n++ ) {
          qword p1 = q*MulShift( ti.x[size_t(n+1)], m, k, c );
          e[size_t(n)] = ZigZag( (long long)( dur[size_t(n)] - (p1-p0) ) ); p0 = p1;
        }
        size_t sz = RunsCost( e )+12;
        if( sz<best ) best = sz, tm.q = q, tm.k = k, tm.m = m, tm.c = c, tm.rdur = e;
      }
    }
  }
  // composition offsets: pts(n) = dts(disp(n)+ks) + D
  tm.hascto = cto!=0; tm.ks = 0; tm.D = 0; tm.rcto.clear();
  if( cto ) {
    size_t bc = ~size_t(0); std::vector<long long> base( (size_t)N );
    for( int ks=0; ks<=4; ks++ ) {
      std::map<long long,qword> hist;
      for( qword n=0; n<N; n++ ) {
        qword j = (ti.disp.size() ? ti.disp[size_t(n)] : n) + qword(ks);
        base[size_t(n)] = (long long)( dts[size_t(std::min(j,N))] - dts[size_t(n)] );
        hist[ (*cto)[size_t(n)] - base[size_t(n)] ]++;
      }
      long long D = 0; qword hc = 0;
      for( std::map<long long,qword>::iterator it=hist.begin(); it!=hist.end(); ++it ) if( it->second>hc ) hc = it->second, D = it->first;
      for( qword n=0; n<N; n++ ) e[size_t(n)] = ZigZag( (*cto)[size_t(n)] - base[size_t(n)] - D );
      size_t sz = RunsCost( e );
      if( sz<bc ) bc = sz, tm.ks = ks, tm.D = D, tm.rcto = e;
      if( !ti.disp.size() ) break;            // no reordering info: ks is irrelevant
    }
  }
  // sync flags
  tm.hassdtp = false;
  tm.hassync = sync!=0; tm.smask = 0; tm.sexc.clear();
  if( sync ) {
    static const int masks[5] = { 0, 2, 4, 8, 6 };
    size_t be = ~size_t(0);
    for( int mi=0; mi<5; mi++ ) {
      std::vector<qword> ex;
      for( qword n=0; n<N && ex.size()<be; n++ ) {
        bool p = ti.fl.size() ? SyncPred( ti.fl[size_t(n)], masks[mi] ) : false;
        if( (*sync)[size_t(n)]<2 && p!=((*sync)[size_t(n)]!=0) ) ex.push_back( n );
      }
      if( ex.size()<be ) be = ex.size(), tm.smask = masks[mi], tm.sexc.swap( ex );
      if( !ti.fl.size() ) break;
    }
  }
}

// aligned chunk model: samples of track j (times tj[i], timescale sj) before master boundary E
// (time e, timescale sm): tj*sm < e*sj + O (rule 0) or <= (rule 1)
static qword CountBefore( const std::vector<qword>& tj, qword N, qword sj, qword e, qword sm, long long O, int rule ) {
  U128 L0 = U128::Mul( e, sj );
  qword lo = 0, hi = N;
  while( lo<hi ) {
    qword mid = (lo+hi)/2;
    U128 a = U128::Mul( tj[size_t(mid)], sm ), b = L0;
    if( O>=0 ) b = b.Add( qword(O) ); else a = a.Add( qword(-O) );
    if( rule ? a.Le( b ) : a.Lt( b ) ) lo = mid+1; else hi = mid;
  }
  return lo;
}

// predicted cumulative sample counts per chunk (cum[c] = samples in chunks 0..c) of a dependent
// track: chunk c ends at master chunk c-a+1's start
static void AlignedCum( const std::vector<qword>& tj, qword N, qword sj, const std::vector<qword>& B, qword sm,
                        long long a, int rule, long long O, qword nch, std::vector<qword>& cum ) {
  cum.resize( size_t(nch) );
  for( qword c=0; c<nch; c++ ) {
    long long kk = (long long)c-a+1;
    if( c+1==nch || kk>=(long long)B.size() ) cum[size_t(c)] = N;
    else if( kk<1 ) cum[size_t(c)] = 0;
    else cum[size_t(c)] = CountBefore( tj, N, sj, B[size_t(kk)], sm, O, rule );
  }
}

} // namespace mp4
