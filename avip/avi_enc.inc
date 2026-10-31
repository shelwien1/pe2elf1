// AVI: encoder
namespace avi {

static bool Encode1( const Buf& in, bool usexf, bool regen, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names,
                     int& nxf, const char** why, std::string& log ) {
  const byte* d = in.data(); qword fs = in.size();
  Parser P( d, fs ); P.File();
  nxf = 0;
  // streams from the strl lists
  std::vector<Track> T( P.strl.size() );
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; const StrlRec& r = P.strl[i];
    t.idx = int(i); t.ok = r.strh && r.strhl>=48 && r.strf && i<100;
    t.fcc = t.ok ? LE32(d+r.strh) : 0; t.handler = t.ok ? LE32(d+r.strh+4) : 0;
    t.scale = t.ok ? LE32(d+r.strh+20) : 0; t.rate = t.ok ? LE32(d+r.strh+24) : 0;
    t.start = t.ok ? LE32(d+r.strh+28) : 0; t.ssize = t.ok ? LE32(d+r.strh+44) : 0;
    t.strf = d+r.strf; t.strfl = r.strfl; t.ckid = 0; t.bytes = 0;
    t.grp = false; t.bias = 0; t.himap = t.hxmap = false; t.xmap = 0; t.padv = 0;
  }
  // chunk ids: the most common id per stream number
  {
    std::vector<std::map<uint,qword> > cnt( T.size() );
    for( size_t k=0; k<P.mch.size(); k++ ) {
      uint id = P.mch[k].id; int c0 = int(id&255)-'0', c1 = int((id>>8)&255)-'0';
      if( c0<0 || c0>9 || c1<0 || c1>9 || !P.mch[k].padok ) continue;
      size_t sn = size_t(c0*10+c1);
      if( sn<T.size() ) cnt[sn][id]++;
    }
    for( size_t i=0; i<T.size(); i++ ) {
      qword bc = 0;
      for( std::map<uint,qword>::iterator it=cnt[i].begin(); it!=cnt[i].end(); ++it ) if( it->second>bc ) bc = it->second, T[i].ckid = it->first;
      if( !bc ) T[i].ok = false;
    }
  }
  for( size_t k=0; k<P.mch.size(); k++ ) {
    MChunk& c = P.mch[k];
    int c0 = int(c.id&255)-'0', c1 = int((c.id>>8)&255)-'0';
    if( c0<0 || c0>9 || c1<0 || c1>9 || !c.padok ) continue;
    size_t sn = size_t(c0*10+c1);
    if( sn>=T.size() || !T[sn].ok || T[sn].ckid!=c.id || c.size>0x7FFFFFFFu ) continue;
    Track& t = T[sn];
    c.trk = int(sn); c.idx = t.fsz.size();
    t.foff.push_back( c.pos+8 ); t.fsz.push_back( uint(c.size) ); t.cidx.push_back( k );
    t.fpad.push_back( (c.size&1) ? d[c.pos+8+c.size] : -1 ); t.bytes += c.size;
  }
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    if( !t.fsz.size() ) t.ok = false;
    if( !t.ok ) continue;
    SetupTrack( t, d );
    if( !usexf ) t.xform = XF_NONE, t.xhdr.clear();
    t.cls.resize( t.fsz.size() );
    for( size_t k=0; k<t.fsz.size(); k++ ) t.cls[k] = byte( FrameClass( t.ckind, d+t.foff[k], t.fsz[k], t.ckparam ) );
    // pad bytes
    qword h[256] = {0};
    for( size_t k=0; k<t.fpad.size(); k++ ) if( t.fpad[k]>=0 ) h[t.fpad[k]]++;
    int b = 0; for( int v=1; v<256; v++ ) if( h[v]>h[b] ) b = v;
    t.padv = b;
    for( size_t k=0; k<t.fpad.size(); k++ ) if( t.fpad[k]>=0 && t.fpad[k]!=b ) t.pexc.push_back( k ), t.pval.push_back( qword(t.fpad[k]) );
  }
  std::vector<int> tm( T.size(), -1 ); int nt = 0;
  for( size_t i=0; i<T.size(); i++ ) if( T[i].ok ) tm[i] = nt++;
  for( size_t k=0; k<P.mch.size(); k++ ) if( P.mch[k].trk>=0 && !T[size_t(P.mch[k].trk)].ok ) P.mch[k].trk = -1;

  // items
  std::vector<Item> it;
  for( size_t k=0; k<P.lists.size(); k++ ) {
    const ListRec& l = P.lists[k];
    Item a = { l.pos, l.pos+12, K_LIST, -1, l.depth, l.ty, 0, 0 }; it.push_back( a );
    Item b = { l.end, l.end, K_LEND, -1, l.depth, 0, 0, 0 }; it.push_back( b );
  }
  for( size_t k=0; k<P.mch.size(); k++ ) {
    const MChunk& c = P.mch[k];
    if( c.trk<0 ) continue;
    Item a = { c.pos, c.pos+8+c.size+(c.size&1), K_CHUNK, c.trk, 0, c.idx, 0, c.movi }; it.push_back( a );
  }
  // header field model: same context as the decoder builds (header bytes before the first
  // complete movi list; chunk statistics of the tracks in the meta)
  int nfld = 0;
  if( regen ) {
    qword capend = std::min( P.movis.empty() ? fs : P.movis[0]-8, qword(1)<<22 );
    Parser Pc( d, capend ); Pc.File();
    HdrCtx hc; hc.h = d; hc.P = &Pc; HdrVideo( hc );
    hc.has.assign( 100, 0 ); hc.nall.assign( 100, 0 ); hc.nfirst.assign( 100, 0 ); hc.maxsz.assign( 100, 0 ); hc.bytes.assign( 100, 0 );
    for( size_t i=0; i<T.size(); i++ ) {
      const Track& t = T[i]; if( !t.ok ) continue;
      int c0 = int(t.ckid&255)-'0', c1 = int((t.ckid>>8)&255)-'0';
      size_t s = size_t(c0*10+c1);
      hc.has[s] = 1; hc.nall[s] = t.fsz.size(); hc.bytes[s] = t.bytes;
      for( size_t k=0; k<t.fsz.size(); k++ ) {
        hc.maxsz[s] = std::max( hc.maxsz[s], qword(t.fsz[k]) );
        if( P.movis.size()<2 || t.foff[k]-8<P.movis[1] ) hc.nfirst[s]++;
      }
    }
    std::vector<std::pair<int,LeafRec> > recs;
    if( Pc.avih.pos ) recs.push_back( std::make_pair( int(HR_AVIH), Pc.avih ) );
    if( Pc.dmlh.pos ) recs.push_back( std::make_pair( int(HR_DMLH), Pc.dmlh ) );
    for( size_t i=0; i<Pc.strl.size(); i++ ) {
      const StrlRec& r = Pc.strl[i];
      if( r.strh ) { LeafRec x = { r.strh, r.strhl }; recs.push_back( std::make_pair( int(HR_STRH), x ) ); }
      if( r.strh && r.strhl>=4 && LE32(d+r.strh)==FCC_LE('v','i','d','s') && r.strf ) { LeafRec x = { r.strf, r.strfl }; recs.push_back( std::make_pair( int(HR_BIH), x ) ); }
      if( r.vprp ) { LeafRec x = { r.vprp, r.vprpl }; recs.push_back( std::make_pair( int(HR_VPRP), x ) ); }
    }
    for( size_t q=0; q<recs.size(); q++ ) {
      int ty = recs[q].first; qword pos = recs[q].second.pos, len = HdrRecLen( ty, recs[q].second.size );
      if( !len || pos+len>capend ) continue;
      int s = HdrStream( Pc, ty, pos );
      qword codes = 0; int ci = 0;
      for( int f=0; f<hr_nf[ty]; f++ ) {
        if( hr_pred[ty][f][0]==PR_NONE ) continue;
        uint have = LE32(d+pos+4*f), v;
        for( int k=0; k<3; k++ )
          if( hr_pred[ty][f][k]!=PR_NONE && HdrPredict( hc, s, hr_pred[ty][f][k], v ) && v==have ) { codes |= qword(k+1)<<(2*ci); break; }
        ci++;
      }
      if( !codes ) continue;
      Item a = { pos, pos+len, K_FLD, -1, 99, qword(ty), codes, 0 }; it.push_back( a ); nfld++;
    }
  }
  // index regeneration: positions/sizes must match exactly (original layout = restored layout)
  int nidx1 = 0, nix = 0, nindx = 0;
  std::vector<std::vector<qword> > ixn( T.size() ), ixb( T.size() );   // per track ix chunks in file order: entries, bytes
  if( regen ) {
    // idx1 for the last complete movi before it
    std::vector<std::vector<size_t> > mchunks( P.movis.size() );   // stream chunks per complete movi
    for( size_t k=0; k<P.mch.size(); k++ ) if( P.mch[k].trk>=0 && P.mch[k].movi>=0 ) mchunks[size_t(P.mch[k].movi)].push_back( k );
    std::vector<std::vector<size_t> > mrec( P.movis.size() );      // complete 'rec ' lists per complete movi
    for( size_t k=0; k<P.lists.size(); k++ ) {
      const ListRec& l = P.lists[k];
      if( l.ty!=FCC_LE('r','e','c',' ') ) continue;
      for( size_t m=0; m<P.movis.size(); m++ ) if( P.movis[m]<l.pos && l.pos<P.movis[m]-8+8+LE32(d+P.movis[m]-4) ) mrec[m].push_back( k );
    }
    std::vector<std::vector<qword> > fl8( T.size() );            // idx1 flags per frame (or ~0)
    for( size_t i=0; i<T.size(); i++ ) fl8[i].assign( T[i].fsz.size(), ~qword(0) );
    std::vector<Item> ix1;
    for( size_t q=0; q<P.idx1.size(); q++ ) {
      const LeafRec& x = P.idx1[q];
      long long mv = -1;
      for( size_t m=0; m<P.movis.size(); m++ ) if( P.movis[m]<x.pos ) mv = (long long)m;
      if( mv<0 || x.size%16 ) continue;
      // elements in file order: stream chunks (index into P.mch) and rec lists (~index into P.lists)
      std::vector<std::pair<qword,long long> > el;
      for( size_t j=0; j<mchunks[size_t(mv)].size(); j++ ) el.push_back( std::make_pair( P.mch[mchunks[size_t(mv)][j]].pos, (long long)mchunks[size_t(mv)][j] ) );
      bool recs = x.size/16==el.size()+mrec[size_t(mv)].size() && mrec[size_t(mv)].size();
      if( recs ) for( size_t j=0; j<mrec[size_t(mv)].size(); j++ ) el.push_back( std::make_pair( P.lists[mrec[size_t(mv)][j]].pos, ~(long long)mrec[size_t(mv)][j] ) );
      std::sort( el.begin(), el.end() );
      if( x.size/16!=el.size() ) continue;
      int kind = -1; bool ok = true; qword rfl = 0; bool hr = false;
      for( size_t j=0; j<el.size() && ok; j++ ) {
        const byte* e = d+x.pos+8+16*j; qword ep = el[j].first;
        if( kind<0 ) kind = LE32(e+8)==uint(ep-P.movis[size_t(mv)]) ? 0 : LE32(e+8)==uint(ep) ? 1 : 2;
        if( kind>1 ) { ok = false; break; }
        qword off = ep-(kind ? 0 : P.movis[size_t(mv)]);
        if( el[j].second<0 ) {
          const ListRec& l = P.lists[size_t(~el[j].second)];
          if( !hr ) rfl = LE32(e+4), hr = true;
          ok = LE32(e)==FCC_LE('r','e','c',' ') && LE32(e+4)==rfl && LE32(e+8)==uint(off) && LE32(e+12)==uint(l.end-l.pos-8);
        } else {
          const MChunk& c = P.mch[size_t(el[j].second)];
          ok = LE32(e)==c.id && LE32(e+12)==uint(c.size) && LE32(e+8)==uint(off);
        }
      }
      if( !ok || kind<0 ) continue;
      for( size_t j=0; j<el.size(); j++ ) if( el[j].second>=0 ) { const MChunk& c = P.mch[size_t(el[j].second)]; fl8[size_t(c.trk)][size_t(c.idx)] = LE32(d+x.pos+8+16*j+4); }
      Item a = { x.pos+8, x.pos+8+x.size, K_IDX1, -1, 0, qword(kind | (recs ? 2 : 0)), rfl, 0 }; ix1.push_back( a );
    }
    if( ix1.size()==1 ) {                                         // one idx1 (more would share the flag columns)
      it.push_back( ix1[0] ); nidx1 = 1;
      for( size_t i=0; i<T.size(); i++ ) {
        Track& t = T[i]; if( !t.ok ) continue;
        std::map<qword,qword> h[8];
        bool any = false;
        for( size_t k=0; k<t.fsz.size(); k++ ) if( fl8[i][k]!=~qword(0) ) h[t.cls[k]][fl8[i][k]]++, any = true;
        if( !any ) continue;
        t.himap = true;
        for( int c=0; c<8; c++ ) {
          qword bv = c&FC_KEY ? 0x10 : 0, bc = 0;
          for( std::map<qword,qword>::iterator q=h[c].begin(); q!=h[c].end(); ++q ) if( q->second>bc ) bc = q->second, bv = q->first;
          t.imap[c] = bv;
        }
        for( size_t k=0; k<t.fsz.size(); k++ )
          if( fl8[i][k]!=~qword(0) && fl8[i][k]!=t.imap[t.cls[k]] ) t.iexc.push_back( k ), t.ival.push_back( fl8[i][k] );
      }
    }
    // ix## standard indexes
    std::vector<qword> ixcur( T.size(), 0 );
    std::vector<std::vector<std::pair<qword,qword> > > ixl( T.size() );   // per track: (ix pos, total size)
    std::vector<std::vector<int> > nk( T.size() );
    for( size_t i=0; i<T.size(); i++ ) nk[i].assign( T[i].fsz.size(), -1 );
    for( size_t q=0; q<P.ix.size(); q++ ) {
      const LeafRec& x = P.ix[q]; const byte* h = d+x.pos+8;
      if( x.size<24 || LE16(h)!=2 || h[2]!=0 || h[3]!=1 ) continue;
      qword n = LE32(h+4); uint cid = LE32(h+8); qword base = LE64(h+12);
      if( 24+8*n>x.size ) continue;
      size_t ti = T.size();
      for( size_t i=0; i<T.size(); i++ ) if( T[i].ok && T[i].ckid==cid ) ti = i;
      if( ti==T.size() ) continue;
      Track& t = T[ti]; qword nf = t.fsz.size();
      qword first = ixcur[ti];
      if( n ) {                                                    // locate the first entry's chunk
        qword target = base + LE32(h+24);
        if( !(first<nf && t.foff[size_t(first)]==target) ) {
          std::vector<qword>::iterator p2 = std::lower_bound( t.foff.begin(), t.foff.end(), target );
          if( p2==t.foff.end() || *p2!=target ) continue;
          first = qword(p2-t.foff.begin());
        }
      }
      if( first+n>nf ) continue;
      bool ok = true;
      for( qword j=0; j<n && ok; j++ ) {
        const byte* e = h+24+8*j; qword k = first+j;
        ok = LE32(e)==uint(t.foff[size_t(k)]-base) && (LE32(e+4)&0x7FFFFFFFu)==t.fsz[size_t(k)];
      }
      if( !ok ) continue;
      for( qword j=0; j<n; j++ ) nk[ti][size_t(first+j)] = (LE32(h+24+8*j+4)>>31)&1;
      long long mv = n ? P.mch[size_t(t.cidx[size_t(first)])].movi : -1;
      int kind; qword res;
      if( n && mv>=0 && base==P.movis[size_t(mv)] ) kind = 0, res = 0;
      else if( n && base==t.foff[size_t(first)] ) kind = 1, res = 0;
      else if( n && mv>=0 ) kind = 0, res = base-P.movis[size_t(mv)];
      else kind = 1, res = n ? base-t.foff[size_t(first)] : base;
      Item a = { x.pos+20, x.pos+32+8*n, K_IX, int(ti), 0, qword(kind), res, (long long)first-(long long)ixcur[ti] }; it.push_back( a );
      ixl[ti].push_back( std::make_pair( x.pos, x.size+8 ) );
      { qword s = 0; for( qword j=0; j<n; j++ ) s += t.fsz[size_t(first+j)]; ixn[ti].push_back( n ); ixb[ti].push_back( s ); }
      ixcur[ti] = first+n; nix++;
    }
    for( size_t i=0; i<T.size(); i++ ) {
      Track& t = T[i]; if( !t.ok ) continue;
      int h[8][2]; memset( h, 0, sizeof(h) ); bool any = false;
      for( size_t k=0; k<t.fsz.size(); k++ ) if( nk[i][k]>=0 ) h[t.cls[k]][nk[i][k]]++, any = true;
      if( !any ) {                                                 // ix chunks of the track with no entries still need the op
        for( size_t q=0; q<it.size(); q++ ) if( it[q].kind==K_IX && it[q].trk==int(i) ) any = true;
        if( !any ) continue;
      }
      t.hxmap = true; t.xmap = 0;
      for( int c=0; c<8; c++ ) if( h[c][1]>h[c][0] ) t.xmap |= 1<<c;
      for( size_t k=0; k<t.fsz.size(); k++ ) if( nk[i][k]>=0 && nk[i][k]!=((t.xmap>>t.cls[k])&1) ) t.xexc.push_back( k );
    }
    // indx super indexes
    for( size_t i=0; i<P.strl.size() && i<T.size(); i++ ) {
      Track& t = T[i]; qword p = P.strl[i].indx;
      if( !t.ok || !p ) continue;
      qword size = LE32(d+p+4); const byte* h = d+p+8;
      if( size<24 || LE16(h)!=4 || h[2]!=0 || h[3]!=0 || LE32(h+8)!=t.ckid ) continue;
      qword n = LE32(h+4);
      if( 24+16*n>size || n>ixl[i].size() ) continue;
      bool ok = true;
      for( qword j=0; j<n && ok; j++ ) {
        const byte* e = h+24+16*j;
        long long res = (long long)LE32(e+12) - (long long)(t.ssize ? ixb[i][size_t(j)]/t.ssize : ixn[i][size_t(j)]);
        ok = LE64(e)==ixl[i][size_t(j)].first && LE32(e+8)==ixl[i][size_t(j)].second && res>=-0x7FFFFFFFLL && res<=0x7FFFFFFFLL;
      }
      if( !ok ) continue;
      Item a = { p+32, p+32+16*n, K_INDX, int(i), 0, n, 0, 0 }; it.push_back( a ); nindx++;
    }
  }
  std::stable_sort( it.begin(), it.end(), []( const Item& a, const Item& b ) {
    if( a.pos!=b.pos ) return a.pos<b.pos;
    if( (a.kind==K_LEND)!=(b.kind==K_LEND) ) return a.kind==K_LEND;      // list ends first, innermost first
    if( a.kind==K_LEND ) return a.depth>b.depth;
    return a.depth<b.depth;
  } );

  // chunk order: per-track time keys, biases fitted to the actual interleaving
  std::vector<std::vector<qword> > keys( T.size() );
  bool usepred = false; qword phit = 0, pall = 0;
  if( regen ) {
    for( size_t i=0; i<T.size(); i++ ) {
      Track& t = T[i];
      t.grp = t.ok && t.scale && t.rate;
      if( !t.grp ) continue;
      keys[i].resize( t.fsz.size() ); qword cb = 0;
      for( size_t k=0; k<t.fsz.size(); k++ ) keys[i][k] = TimeKey( (t.ssize ? cb/t.ssize : k) + t.start, t.scale, t.rate ), cb += t.fsz[k];
    }
    std::vector<int> seq;
    for( size_t q=0; q<it.size(); q++ ) if( it[q].kind==K_CHUNK ) seq.push_back( it[q].trk );
    std::vector<long long> bias( T.size(), 0 );
    size_t lim = std::min( seq.size(), size_t(100000) );
    auto sim = [&]() {
      std::vector<qword> nx( T.size(), 0 ); size_t h = 0;
      for( size_t q=0; q<lim; q++ ) {
        size_t bt = T.size(); long long bk = 0;
        for( size_t u=0; u<T.size(); u++ ) {
          if( !T[u].grp || nx[u]>=T[u].fsz.size() ) continue;
          long long kk = (long long)keys[u][size_t(nx[u])]+bias[u];
          if( bt==T.size() || kk<bk ) bt = u, bk = kk;
        }
        h += bt==size_t(seq[q]); nx[size_t(seq[q])]++;
      }
      return h;
    };
    // coordinate descent: with the other biases fixed, every order decision bounds bias[i] to
    // an interval; sweep for the value inside the most intervals (midpoint of the best region)
    const long long INF = 1LL<<62;
    size_t best = sim();
    for( int round=0; round<3; round++ ) {
      bool moved = false;
      for( size_t i=0; i<T.size(); i++ ) {
        if( !T[i].grp ) continue;
        std::vector<std::pair<long long,int> > ev;              // +1 at lo, -1 after hi
        long long always = 0;
        std::vector<qword> nx( T.size(), 0 );
        for( size_t q=0; q<lim; q++ ) {
          size_t A = size_t(seq[q]);
          bool ip = T[i].grp && nx[i]<T[i].fsz.size();
          long long ki = ip ? (long long)keys[i][size_t(nx[i])] : 0;
          if( A==i ) {
            long long U = INF;                                  // key_i+b < (kj,j) for every other pending j
            for( size_t j=0; j<T.size(); j++ ) {
              if( j==i || !T[j].grp || nx[j]>=T[j].fsz.size() ) continue;
              long long kj = (long long)keys[j][size_t(nx[j])]+bias[j];
              U = std::min( U, kj-ki-(i<j ? 0 : 1) );
            }
            if( T[i].grp ) ev.push_back( std::make_pair( -INF, 1 ) ), ev.push_back( std::make_pair( U, -1 ) );
          } else {
            bool wins = T[A].grp && nx[A]<T[A].fsz.size();      // A must beat every track but i
            long long kA = wins ? (long long)keys[A][size_t(nx[A])]+bias[A] : 0;
            for( size_t j=0; j<T.size() && wins; j++ ) {
              if( j==i || j==A || !T[j].grp || nx[j]>=T[j].fsz.size() ) continue;
              long long kj = (long long)keys[j][size_t(nx[j])]+bias[j];
              if( !(kA<kj || (kA==kj && A<j)) ) wins = false;
            }
            if( wins ) {
              if( !ip ) always++;
              else ev.push_back( std::make_pair( kA-ki+(A<i ? 0 : 1), 1 ) ), ev.push_back( std::make_pair( INF, -1 ) );
            }
          }
          nx[A]++;
        }
        // intervals are [lo, hi]: count(b) = #(lo<=b) - #(hi<b)
        std::vector<std::pair<long long,int> > pts;
        for( size_t k=0; k<ev.size(); k+=2 ) pts.push_back( std::make_pair( ev[k].first, 1 ) ), pts.push_back( std::make_pair( ev[k+1].first+(ev[k+1].first<INF ? 1 : 0), -1 ) );
        std::sort( pts.begin(), pts.end() );
        long long c = 0, bc = -1, blo = 0, bhi = 0;
        for( size_t k=0; k<pts.size(); ) {
          long long x = pts[k].first;
          while( k<pts.size() && pts[k].first==x ) c += pts[k].second, k++;
          long long nxp = k<pts.size() ? pts[k].first : INF;
          if( c>bc ) bc = c, blo = x, bhi = nxp-1;
        }
        if( bc<0 ) continue;
        long long nb = blo<=-INF/2 ? (bhi>=INF/2 ? 0 : bhi) : bhi>=INF/2 ? blo : blo+(bhi-blo)/2;
        if( nb!=bias[i] ) {
          long long ob = bias[i]; bias[i] = nb;
          size_t h = sim();
          if( h>best ) best = h, moved = true; else bias[i] = ob;
        }
        (void)always;
      }
      if( !moved ) break;
    }
    usepred = best*2>=lim && best>0;
    for( size_t i=0; i<T.size(); i++ ) { T[i].bias = bias[i]; if( !usepred ) T[i].grp = false; }
  }

  // ops + struct
  Writer op; Buf st; st.reserve( size_t(fs>>6) );
  qword cur = 0; size_t prun_at = 0; qword prun_n = 0; bool prun_open = false;
  std::vector<qword> h4( 4096, ~qword(0) );                    // repeat finder: last position per 4-byte hash
  auto raw = [&]( qword to ) {
    qword i = cur, rs = cur;
    auto flush = [&]( qword e ) { if( e>rs ) { prun_open = false; op.u8(OP_RAW); op.vint(e-rs); st.insert( st.end(), d+rs, d+e ); } };
    while( i<to ) {
      if( !d[i] ) {                                             // zero runs >= 32 bytes -> OP_ZERO
        qword e = i; while( e<to && !d[e] ) e++;
        if( e-i>=32 ) { flush( i ); prun_open = false; op.u8(OP_ZERO); op.vint(e-i); i = rs = e; continue; }
      }
      if( i+4<=to && i>=1 ) {                                   // runs with a period <= 64 -> OP_REP
        uint hh = (LE32(d+i)*2654435761u)>>20; qword c = h4[hh]; h4[hh] = i;
        if( c!=~qword(0) && c<i && i-c<=64 ) {
          qword P = i-c, e = i; while( e<to && d[e]==d[e-P] ) e++;
          if( e-i>=32 ) { flush( i ); prun_open = false; op.u8(OP_REP); op.vint(P); op.vint(e-i); i = rs = e; continue; }
        }
      }
      i++;
    }
    flush( to ); cur = to;
  };
  std::vector<qword> nx( T.size(), 0 );
  for( size_t q=0; q<it.size(); q++ ) {
    const Item& x = it[q];
    if( x.pos<cur ) continue;
    raw( x.pos );
    if( x.kind!=K_CHUNK ) prun_open = false;
    switch( x.kind ) {
    case K_LIST:
      if( LE32(d+x.pos)==FCC_LE('L','I','S','T') && x.a==FCC_LE('r','e','c',' ') ) op.u8(OP_RLIST);
      else op.u8(OP_LIST), st.insert( st.end(), d+x.pos, d+x.pos+4 ), st.insert( st.end(), d+x.pos+8, d+x.pos+12 );
      cur = x.end; break;
    case K_LEND: op.u8(OP_LEND); break;
    case K_FLD: {
      int ty = int(x.a);
      op.u8(OP_FLD); op.u8(uint(ty));
      for( int b=0; b<(2*HdrNCodes(ty)+7)/8; b++ ) op.u8( uint(x.b>>(8*b))&255 );
      for( int f=0, ci=0; f<hr_nf[ty]; f++ ) {
        uint cd = hr_pred[ty][f][0]==PR_NONE ? 0 : uint(x.b>>(2*ci++))&3;
        if( !cd ) st.insert( st.end(), d+x.pos+4*f, d+x.pos+4*f+4 );
      }
      cur = x.end;
    } break;
    case K_IDX1: op.u8(OP_IDX1); op.u8(uint(x.a)); if( x.a&2 ) op.vint(x.b); cur = x.end; break;
    case K_IX: {
      op.u8(OP_IX); op.vint(qword(tm[size_t(x.trk)])); op.u8(uint(x.a)); op.vint( ZigZag( x.c ) );
      byte e[8]; PutLE64( e, x.b ); st.insert( st.end(), e, e+8 ); st.insert( st.end(), d+x.pos+8, d+x.pos+12 );
      cur = x.end;
    } break;
    case K_INDX: {
      Track& t = T[size_t(x.trk)];
      op.u8(OP_INDX); op.vint(qword(tm[size_t(x.trk)]));
      for( qword j=0; j<x.a; j++ ) {               // duration residuals vs the track's ix chunks
        size_t ti = size_t(x.trk);
        long long res = (long long)LE32(d+x.pos+16*j+12) - (long long)(t.ssize ? ixb[ti][size_t(j)]/t.ssize : ixn[ti][size_t(j)]);
        byte e[4]; PutLE32( e, uint(int32_t(res)) ); st.insert( st.end(), e, e+4 );
      }
      cur = x.end;
    } break;
    case K_CHUNK: {
      size_t ti = size_t(x.trk);
      // predicted chunk (as op 0xF5)
      size_t bt = T.size(); long long bk = 0;
      for( size_t u=0; u<T.size(); u++ ) {
        if( !T[u].grp || nx[u]>=T[u].fsz.size() ) continue;
        long long kk = (long long)keys[u][size_t(nx[u])]+T[u].bias;
        if( bt==T.size() || kk<bk ) bt = u, bk = kk;
      }
      pall++;
      if( bt==ti && x.a==nx[ti] ) {
        if( prun_open ) op.b.resize( prun_at ), prun_n++;
        else prun_at = op.b.size(), prun_n = 1;
        op.u8(OP_PRUN); op.vint(prun_n); prun_open = true; phit++;
      } else {
        prun_open = false;
        if( x.a!=nx[ti] ) { *why = "chunk order"; return false; }
        if( tm[ti]<0x40 ) op.u8(uint(tm[ti])); else op.u8(OP_CHUNK), op.vint(qword(tm[ti]));
      }
      nx[ti]++;
      cur = x.end;
    } break;
    }
  }
  raw( fs ); op.u8(OP_END);

  Writer w; PutMetaHdr( w, FMT_AVI );
  w.vint(nt);
  streams.clear(); names.clear();
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; if( !t.ok ) continue;
    Buf s; s.reserve( size_t(t.bytes) );
    for( size_t k=0; k<t.fsz.size(); k++ ) s.insert( s.end(), d+t.foff[k], d+t.foff[k]+t.fsz[k] );
    ChooseSizeMode( d, t, s );
    t.h.xform = t.xform; t.h.nf = t.fsz.size(); t.h.hdrlen = 0;
    if( t.xform==XF_ADTS ) { Buf x; ToADTS( t, s, x ); s.swap( x ); nxf++; }
    else if( t.xform==XF_HDR ) { Buf x = t.xhdr; x.insert( x.end(), s.begin(), s.end() ); s.swap( x ); nxf++; t.h.hdrlen = t.xhdr.size(); }
    if( t.ext=="mp2" && s.size()>=2 && s[0]==0xFF && (s[1]&0xE0)==0xE0 ) {
      int lay = (s[1]>>1)&3; if( lay==3 ) t.ext = "mp1"; else if( lay==1 ) t.ext = "mp3";
    }
    streams.push_back( Buf() ); streams.back().swap( s );
    t.h.ext = t.xext.size() && (t.xform!=XF_NONE || t.ckind!=CK_RAW) ? t.xext : t.ext;
    if( t.xform==XF_NONE && t.xext=="wav" ) t.h.ext = t.ext;
    std::string suf = TrackSuffix( names.size(), t.h.ext );
    names.push_back( suf );
    PutHead( w, t.h );
    // chunk id
    size_t ti = size_t(tm[i]);
    uint sn = uint( ((t.ckid&255)-'0')*10 + (((t.ckid>>8)&255)-'0') ), ty = 0;
    for( uint q=1; q<=5; q++ ) if( ((t.ckid>>16)&255)==byte(ck_types[q][0]) && (t.ckid>>24)==byte(ck_types[q][1]) ) ty = q;
    if( ty ) w.u8( ty | (sn!=ti ? 8 : 0) | uint(t.ckind)<<4 ), sn!=ti ? w.u8( sn ) : (void)0;
    else { w.u8( uint(t.ckind)<<4 ); byte ck[4]; PutLE32( ck, t.ckid ); w.bytes( ck, 4 ); }
    if( t.ckind==CK_VC1 ) w.u8( t.ckparam );
    bool defmap = true;
    for( int c=0; c<8; c++ ) if( t.imap[c]!=qword(c&FC_KEY ? 0x10 : 0) ) defmap = false;
    uint fl = (t.padv || t.pexc.size() ? 1 : 0) | (t.grp ? 2 : 0) | (t.himap ? 4 : 0) | (t.himap && !defmap ? 8 : 0) | (t.hxmap ? 16 : 0);
    w.u8( fl );
    if( fl&1 ) { w.u8( t.padv ); PutIdxCol( w, t.pexc ); for( size_t k=0; k<t.pval.size(); k++ ) w.u8( uint(t.pval[k]) ); }
    if( t.grp ) w.vint( t.scale ), w.vint( t.rate ), w.vint( t.ssize ), w.vint( t.start ), w.vint( ZigZag( t.bias ) );
    if( t.himap ) {
      if( fl&8 ) for( int c=0; c<8; c++ ) w.vint( t.imap[c] );
      PutIdxCol( w, t.iexc ); PutCol( w, t.ival );
    }
    if( t.hxmap ) w.u8( uint(t.xmap) ), PutIdxCol( w, t.xexc );
    if( verbose ) {
      char l[300];
      snprintf( l, sizeof(l), "stream %d: %-4s %-4s %8llu chunks %11llu bytes  sizes=%s%s pad=%02x/%llu idx1=%s/%llu ix=%s/%llu -> %s\n",
        t.idx, FccStr(t.fcc).c_str(), t.fcc==FCC_LE('v','i','d','s') ? FccStr(LE32(t.strf+16)).c_str() : t.ext.c_str(),
        (unsigned long long)t.fsz.size(), (unsigned long long)t.bytes,
        t.h.szmode==SZ_XFORM ? "adts" : t.h.szmode==SZ_UNITS ? ptname[t.h.ptype] : "explicit", t.h.szmode==SZ_UNITS ? (std::string(" esc=")+std::to_string(t.h.esc.size())).c_str() : "",
        t.padv, (unsigned long long)t.pexc.size(), t.himap ? "map" : "-", (unsigned long long)t.iexc.size(),
        t.hxmap ? "map" : "-", (unsigned long long)t.xexc.size(), suf.c_str() );
      log += l;
    }
  }
  if( verbose ) {
    char l[200];
    snprintf( l, sizeof(l), "layout: %llu lists, idx1 %d, ix %d, indx %d, header records %d; chunk order predicted %llu/%llu%s; struct %llu, ops %llu\n",
      (unsigned long long)P.lists.size(), nidx1, nix, nindx, nfld, (unsigned long long)phit, (unsigned long long)pall, usepred ? "" : " (unused)",
      (unsigned long long)st.size(), (unsigned long long)op.b.size() );
    log += l;
  }
  w.bytes( op.b.data(), op.b.size() ); w.bytes( st.data(), st.size() );
  meta.swap( w.b );

  // verify: run the decoder on the actual output
  Meta M; Sink chk( 0, &in ); *why = "meta parse";
  return ParseMeta(meta,M) && Decode(M,streams,chk,why);
}

static bool Encode( const Buf& in, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names ) {
  // fallbacks: without stream conversion, without index/order regeneration, without both
  static const bool lv[4][2] = { {true,true}, {false,true}, {true,false}, {false,false} };
  const char* why = ""; int nxf = 0, nxf0 = 0; std::string log;
  for( int i=0; i<4; i++ ) {
    if( i>0 && !lv[i][0] && !nxf0 ) continue;
    log.clear();
    bool ok = Encode1( in, lv[i][0], lv[i][1], meta, streams, names, nxf, &why, log );
    if( i==0 ) nxf0 = nxf;
    if( ok ) { if( verbose ) fputs( log.c_str(), stdout ); return true; }
    fprintf( stderr, "warning: verify failed (%s)%s\n", why, i<3 ? ", retrying with fewer transforms" : "" );
  }
  meta.clear(); streams.clear(); names.clear();
  return false;
}

} // namespace avi
