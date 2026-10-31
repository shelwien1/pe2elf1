// AVI: restore
namespace avi {

static bool Restore( Meta& M, const std::vector<const Buf*>& S, Sink& out, const char** why ) {
  #define FAIL(s) { *why = s; return false; }
  const byte* st = M.st;
  size_t nt = M.trk.size();
  for( size_t i=0; i<nt; i++ ) {
    DTrack& t = M.trk[i]; const Buf& s = *S[i];
    if( !HeadSizes( t.h, s, t.xsz, t.sz, why ) ) return false;
    t.fo.resize( size_t(t.nf+1) ); t.fo[0] = 0;
    for( size_t k=0; k<t.nf; k++ ) t.fo[k+1] = t.fo[k]+t.sz[k];
    if( t.fo[size_t(t.nf)]!=s.size() ) FAIL("stream size mismatch");
    t.cls.assign( size_t(t.nf), 0 );
    if( t.himap || t.hxmap || t.hamap )
      for( size_t k=0; k<t.nf; k++ ) t.cls[k] = byte( FrameClass( t.ckind, s.data()+t.fo[k], t.sz[k], t.ckparam ) );
    t.alt.assign( size_t(t.nf), 0 );
    if( t.hamap ) {
      ExcCur a; a.Reset( &t.aexc );
      for( size_t k=0; k<t.nf; k++ ) t.alt[k] = byte( ((t.amap>>t.cls[k])&1) ^ (a.At( k ) ? 1 : 0) );
    }
    t.cpos.assign( size_t(t.nf), 0 ); t.cmovi.assign( size_t(t.nf), -1 ); t.ix.clear();
  }
  std::vector<qword> next( nt ), ixcur( nt ), key( nt ), lsize, lpos, mpos;
  std::vector<ExcCur> pc( nt ), ic( nt ), xc( nt );
  std::vector<std::vector<std::pair<size_t,qword> > > mch;    // stream chunks per movi list
  Buf tb;
  // header field model: header bytes captured in pass 0 (predicted fields as 0), parsed
  // after it; chunk statistics per stream number
  const qword CAPMAX = qword(1)<<22;
  Buf cap; bool capon = true, anyfld = false;
  for( size_t i=0; i<M.ops.size(); i++ ) anyfld |= M.ops[i].code==OP_FLD;
  std::vector<Parser> hpv; Parser* HP = 0; HdrCtx hc;
  for( int pass=0; pass<2; pass++ ) {
    qword pos = 0, sp = 0; size_t nlist = 0, nmovi = 0;
    long long curmovi = -1, lastmovi = -1;
    std::vector<std::pair<qword,size_t> > stk; std::vector<bool> stkmovi;
    std::fill( next.begin(), next.end(), 0 ); std::fill( ixcur.begin(), ixcur.end(), 0 );
    if( !pass ) mpos.clear(), mch.clear();
    for( size_t i=0; i<nt; i++ ) {
      pc[i].Reset( &M.trk[i].pexc ); ic[i].Reset( &M.trk[i].iexc ); xc[i].Reset( &M.trk[i].xexc );
    }
    byte hist[256]; qword hw = 0;                     // last output bytes (OP_REP source)
    auto emit = [&]( const byte* p, qword n ) {
      if( !pass && anyfld && capon ) {
        qword c = std::min( n, CAPMAX-std::min( CAPMAX, pos ) );
        cap.insert( cap.end(), p, p+c );
        if( c<n ) capon = false;
      }
      if( pass ) {
        out.put( p, n );
        for( qword j = n>256 ? n-256 : 0; j<n; j++ ) hist[(hw+j)&255] = p[j];
        hw += n;
      }
      pos += n;
    };
    auto units = [&]( size_t i ) -> qword {
      const DTrack& t = M.trk[i]; qword k = next[i];
      return (t.ssize ? t.fo[size_t(k)]/t.ssize : k) + t.start;
    };
    for( size_t i=0; i<nt; i++ ) if( M.trk[i].grp && M.trk[i].nf ) key[i] = TimeKey( units(i), M.trk[i].scale, M.trk[i].rate );
    auto chunk = [&]( size_t ti ) -> bool {
      DTrack& t = M.trk[ti]; qword k = next[ti];
      if( k>=t.nf ) FAIL("chunk overrun");
      byte h[8]; PutLE32( h, t.alt[size_t(k)] ? t.ckid2 : t.ckid ); PutLE32( h+4, t.sz[size_t(k)] );
      if( !pass ) {
        t.cpos[size_t(k)] = pos; t.cmovi[size_t(k)] = curmovi;
        if( curmovi>=0 ) mch[size_t(curmovi)].push_back( std::make_pair( ti, k ) );
      }
      emit( h, 8 ); emit( S[ti]->data()+t.fo[size_t(k)], t.sz[size_t(k)] );
      if( t.sz[size_t(k)]&1 ) {
        byte pb = byte(t.padv);
        if( pc[ti].At( k ) ) pb = byte(t.pval[pc[ti].i]);
        emit( &pb, 1 );
      }
      next[ti] = k+1;
      if( t.grp && k+1<t.nf ) key[ti] = TimeKey( units(ti), t.scale, t.rate );
      return true;
    };
    for( size_t i=0; i<M.ops.size(); i++ ) {
      const DOp& d = M.ops[i];
      if( d.code==OP_RAW || d.code==OP_END ) {
        qword n = d.code==OP_RAW ? d.n : M.stlen-sp;
        if( n>M.stlen-sp ) FAIL("struct overrun");
        emit( st+sp, n ); sp += n;
      } else if( d.code==OP_ZERO ) {
        static const byte z[4096] = {0};
        for( qword n=d.n; n; ) { qword c = std::min( n, qword(sizeof(z)) ); emit( z, c ); n -= c; }
      } else if( d.code==OP_REP ) {
        if( d.k>pos ) FAIL("bad repeat");
        if( !pass && anyfld && capon ) {              // capture: repeat from the captured bytes
          if( cap.size()!=pos || d.k>cap.size() ) capon = false;
          else for( qword j=0; j<d.n && capon; j++ ) { if( cap.size()>=CAPMAX ) capon = false; else cap.push_back( cap[cap.size()-size_t(d.k)] ); }
        }
        if( pass ) {
          byte b[256];
          for( qword n=d.n; n; ) {
            qword c = std::min( n, qword(sizeof(b)) );
            for( qword j=0; j<c; j++ ) { b[j] = hist[(hw-d.k)&255]; hist[hw&255] = b[j]; hw++; out.put( b+j, 1 ); }
            n -= c;
          }
        }
        pos += d.n;
      } else if( d.code==OP_CHUNK ) {
        if( !chunk( size_t(d.t) ) ) return false;
      } else if( d.code==OP_PRUN ) {
        for( qword j=0; j<d.n; j++ ) {
          size_t bt = nt; long long bk = 0;
          for( size_t u=0; u<nt; u++ ) {
            const DTrack& g = M.trk[u];
            if( !g.grp || next[u]>=g.nf ) continue;
            long long kk = (long long)key[u]+g.bias;
            if( bt==nt || kk<bk ) bt = u, bk = kk;
          }
          if( bt==nt ) FAIL("no chunk to predict");
          if( !chunk( bt ) ) return false;
        }
      } else if( d.code==OP_FLD ) {
        int ty = int(d.k); qword n = 4*qword(hr_nf[ty]);
        int s = pass ? HdrStream( *HP, ty, pos ) : -1;
        byte e[4*17];
        for( int f=0, ci=0; f<hr_nf[ty]; f++ ) {
          uint cd = hr_pred[ty][f][0]==PR_NONE ? 0 : uint(d.n>>(2*ci++))&3;
          if( !cd ) { if( 4>M.stlen-sp ) FAIL("struct overrun"); memcpy( e+4*f, st+sp, 4 ); sp += 4; continue; }
          uint v = 0;
          if( pass && !HdrPredict( hc, s, hr_pred[ty][f][cd-1], v ) ) FAIL("header prediction");
          PutLE32( e+4*f, v );
        }
        emit( e, n );
      } else if( d.code==OP_LIST || d.code==OP_RLIST ) {
        static const byte rec[8] = { 'L','I','S','T','r','e','c',' ' };
        const byte* lh = d.code==OP_LIST ? st+sp : rec;
        if( d.code==OP_LIST && 8>M.stlen-sp ) FAIL("struct overrun");
        uint ty = LE32(lh+4);
        if( ty==FCC_LE('m','o','v','i') && d.code==OP_LIST ) capon = false;
        byte h[4];
        if( !pass ) { if( lpos.size()<=nlist ) lpos.resize( nlist+1 ); lpos[nlist] = pos; }
        if( ty==FCC_LE('r','e','c',' ') && curmovi>=0 && !pass ) mch[size_t(curmovi)].push_back( std::make_pair( ~size_t(0), qword(nlist) ) );
        emit( lh, 4 );
        if( pass ) { if( nlist>=lsize.size() ) FAIL("list count"); PutLE32( h, uint(lsize[nlist]) ); }
        emit( h, 4 );
        stk.push_back( std::make_pair( pos, nlist++ ) );
        bool mv = ty==FCC_LE('m','o','v','i');
        stkmovi.push_back( mv );
        if( mv ) {                            // movi ordinal: lists opened so far; position of the 'movi' fourcc
          if( !pass ) mpos.push_back( pos ), mch.push_back( std::vector<std::pair<size_t,qword> >() );
          curmovi = lastmovi = (long long)nmovi++;
        }
        emit( lh+4, 4 ); if( d.code==OP_LIST ) sp += 8;
      } else if( d.code==OP_LEND ) {
        if( stk.empty() ) FAIL("list end without start");
        qword z = pos-stk.back().first;
        if( z>0xFFFFFFFFu ) FAIL("list too big");
        if( !pass ) { if( lsize.size()<=stk.back().second ) lsize.resize( stk.back().second+1 ); lsize[stk.back().second] = z; }
        if( stkmovi.back() ) curmovi = -1;
        stk.pop_back(); stkmovi.pop_back();
      } else if( d.code==OP_IDX1 ) {
        if( sp<4 || lastmovi<0 ) FAIL("bad idx1");
        qword n = LE32(st+sp-4)/16;
        const std::vector<std::pair<size_t,qword> >& ch = mch[size_t(lastmovi)];
        qword ne = 0;
        for( size_t j=0; j<ch.size(); j++ ) ne += ch[j].first!=~size_t(0) || (d.k&2);
        if( n!=ne ) FAIL("idx1 count mismatch");
        qword base = (d.k&1)==0 ? mpos[size_t(lastmovi)] : 0;
        for( size_t j=0; j<ch.size(); j++ ) {
          if( ch[j].first==~size_t(0) ) {               // 'rec ' list
            if( !(d.k&2) ) continue;
            byte e[16]; memcpy( e, "rec ", 4 ); PutLE32( e+4, uint(d.n) );
            qword lo = size_t(ch[j].second); PutLE32( e+8, uint(lpos[lo]-base) ); PutLE32( e+12, pass ? uint(lsize[lo]) : 0 );
            emit( e, 16 ); continue;
          }
          DTrack& t = M.trk[ch[j].first]; qword k = ch[j].second;
          if( !t.himap ) FAIL("no idx1 map");
          qword fl = t.imap[t.cls[size_t(k)]];
          if( ic[ch[j].first].At( k ) ) fl = t.ival[ic[ch[j].first].i];
          byte e[16]; PutLE32( e, t.alt[size_t(k)] ? t.ckid2 : t.ckid ); PutLE32( e+4, uint(fl) ); PutLE32( e+8, uint(t.cpos[size_t(k)]-base) ); PutLE32( e+12, t.sz[size_t(k)] );
          emit( e, 16 );
        }
      } else if( d.code==OP_IX ) {
        DTrack& t = M.trk[size_t(d.t)];
        if( sp<20 || 12>M.stlen-sp || !t.hxmap ) FAIL("bad ix");
        qword n = LE32(st+sp-8), total = 8+qword(LE32(st+sp-16));
        qword first = ixcur[size_t(d.t)] + qword( UnZigZag( d.n ) );
        if( first>t.nf || n>t.nf-first ) FAIL("ix range");
        qword bytes = t.fo[size_t(first+n)]-t.fo[size_t(first)];
        if( !pass ) { IxRec x = { pos-20, total, n, bytes }; t.ix.push_back( x ); }
        byte e[12]; qword base = 0;
        if( pass ) {
          if( n ) {
            if( d.k==0 ) { long long mv = t.cmovi[size_t(first)]; if( mv<0 ) FAIL("ix ref"); base = mpos[size_t(mv)]; }
            else base = t.cpos[size_t(first)]+8;
          }
          base += LE64(st+sp);
        }
        PutLE64( e, base ); memcpy( e+8, st+sp+8, 4 ); emit( e, 12 ); sp += 12;
        for( qword j=0; j<n; j++ ) {
          qword k = first+j; uint s32 = t.sz[size_t(k)];
          bool nk = ((t.xmap>>t.cls[size_t(k)])&1)!=0;
          if( xc[size_t(d.t)].At( k ) ) nk = !nk;
          if( s32>>31 ) FAIL("ix size overflow");
          PutLE32( e, uint( t.cpos[size_t(k)]+8-base ) ); PutLE32( e+4, s32 | (nk ? 0x80000000u : 0) );
          emit( e, 8 );
        }
        ixcur[size_t(d.t)] = first+n;
      } else if( d.code==OP_INDX ) {
        DTrack& t = M.trk[size_t(d.t)];
        if( sp<20 ) FAIL("bad indx");
        qword n = LE32(st+sp-20);
        if( 4*n>M.stlen-sp ) FAIL("struct overrun");
        for( qword j=0; j<n; j++ ) {
          byte e[16] = {0};
          if( pass ) {
            if( j>=t.ix.size() ) FAIL("indx ref");
            const IxRec& x = t.ix[size_t(j)];
            qword dur = (t.ssize ? x.bytes/t.ssize : x.n) + qword( (long long)int32_t( LE32(st+sp+4*j) ) );
            PutLE64( e, x.pos ); PutLE32( e+8, uint(x.total) ); PutLE32( e+12, uint(dur) );
          }
          emit( e, 16 );
        }
        sp += 4*n;
      } else FAIL("bad op");
    }
    if( !stk.empty() ) FAIL("unterminated list");
    if( sp!=M.stlen ) FAIL("struct not consumed");
    for( size_t i=0; i<nt; i++ ) if( next[i]!=M.trk[i].nf ) FAIL("track not consumed");
    if( !pass && anyfld ) {                           // header field model context
      for( size_t k=0; k<lsize.size() && k<lpos.size(); k++ )   // list sizes are known now
        if( lpos[k]+8<=cap.size() ) PutLE32( &cap[size_t(lpos[k]+4)], uint(lsize[k]) );
      hpv.push_back( Parser( cap.data(), cap.size() ) ); HP = &hpv[0]; HP->File();
      hc.h = cap.data(); hc.P = HP; HdrVideo( hc );
      hc.has.assign( 100, 0 ); hc.nall.assign( 100, 0 ); hc.nfirst.assign( 100, 0 ); hc.maxsz.assign( 100, 0 ); hc.bytes.assign( 100, 0 );
      for( size_t i=0; i<nt; i++ ) {
        const DTrack& t = M.trk[i];
        int c0 = int(t.ckid&255)-'0', c1 = int((t.ckid>>8)&255)-'0';
        if( c0<0 || c0>9 || c1<0 || c1>9 ) continue;
        size_t s = size_t(c0*10+c1);
        hc.has[s] = 1; hc.nall[s] = t.nf; hc.bytes[s] = t.fo[size_t(t.nf)];
        for( size_t k=0; k<t.nf; k++ ) {
          hc.maxsz[s] = std::max( hc.maxsz[s], qword(t.sz[k]) );
          if( mpos.size()<2 || t.cpos[k]<mpos[1] ) hc.nfirst[s]++;
        }
      }
    }
  }
  if( out.err || (out.ref && out.pos!=out.ref->size()) ) FAIL( out.ref ? "output differs" : "write error" );
  return true;
  #undef FAIL
}

// undo stream transforms, then restore. S holds the track streams as stored.
static std::string TrackName( const Meta& M, size_t i ) { return TrackSuffix( i, M.trk[i].h.ext ); }

static bool Decode( Meta& M, const std::vector<Buf>& S, Sink& out, const char** why ) {
  std::vector<Buf> R( M.trk.size() ); std::vector<const Buf*> P( M.trk.size() );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i]; P[i] = &S[i];
    if( t.h.xform==XF_NONE ) continue;
    Buf priv;
    if( !HeadUntransform( t.h, S[i], R[i], t.xsz, priv ) ) { *why = "bad converted stream"; return false; }
    P[i] = &R[i];
  }
  return Restore( M, P, out, why );
}

} // namespace avi
