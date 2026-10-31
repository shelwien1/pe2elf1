// MP4: encoder
namespace mp4 {

static bool Encode1( const Buf& in, bool usexf, bool timing, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names, int& nxf, const char** why, std::string& log ) {
  const byte* d = in.data(); qword fs = in.size();
  std::vector<Track> T; std::vector<qword> bdo_pos; std::vector<char> bdo_ok;
  nxf = 0;

  std::vector<Box> top = Children( d, 0, fs );
  for( size_t i=0; i<top.size(); i++ ) {
    if( top[i].type!=FCC_BE('m','o','o','v') ) continue;
    std::vector<Box> mc = Children( d, top[i].pay(), top[i].end() );
    int ti=0;
    for( size_t j=0; j<mc.size(); j++ ) {
      if( mc[j].type!=FCC_BE('t','r','a','k') ) continue;
      Track t; t.idx = ti++; t.ok = SetupTrack( d, fs, mc[j], t );
      if( !usexf ) t.xform = XF_NONE;
      if( verbose && !t.ok ) { char l[64]; snprintf( l, sizeof(l), "trak %d: unusable sample tables, kept raw\n", t.idx ); log += l; }
      T.push_back( t );
    }
    ParseFragments( d, fs, top, top[i], T, bdo_pos );
    AddCovers( d, top[i], T );
    break;
  }
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    if( t.ok && t.bytes==0 ) t.ok=false;
    if( t.ok && !t.blob ) FinishCodec( d, t, usexf ), ChooseSizeMode( d, t );
    if( t.ok && t.xform==XF_ANNEXB ) {                 // mostly not NALs: no conversion (decided here, items depend on it)
      qword rawb = 0;
      for( size_t c=0; c<t.coff.size(); c++ )
        for( qword j=t.cfirst[c], o=t.coff[c]; j<t.cfirst[c]+t.spc[c]; o+=t.Size(j), j++ )
          if( t.Size(j) && CountUnits( PT_NAL, t.lensize, d+o, t.Size(j) )<0 ) rawb += t.Size(j);
      if( 2*rawb>t.bytes ) t.xform = XF_NONE, t.psets.clear();
    }
    t.tmok = timing && t.ok && !t.blob && !t.strunc && t.nsamp && t.sdur.size()==t.nsamp && t.scto.size()==t.nsamp &&
             (!t.has_stbl || t.hstts) && (!t.hstss || t.ssync.size()<=t.nsamp);
    t.tflags = 0;
    if( t.tmok ) {
      t.tflags = 1;
      if( t.szmode>=2 ) t.tflags |= 2 | ((t.hctts || t.fcto) ? 4 : 0);
    }
  }
  // chunk model: stsc entries regenerated, plain runs or aligned to a master track
  {
    std::vector<size_t> el;                         // eligible tracks
    std::vector<std::vector<qword> > dts( T.size() ), act( T.size() );
    std::vector<size_t> plain( T.size(), 0 );
    for( size_t i=0; i<T.size(); i++ ) {
      Track& t = T[i]; t.cflags = 0;
      if( !t.ok || t.blob || !t.has_stbl || !t.cstsc || !timing ) continue;
      t.cflags = 1 | (t.cslast ? 4 : 0); t.cmode = 0; t.ccol.assign( t.spc.begin(), t.spc.begin()+size_t(t.npc) );
      plain[i] = RunsCost( t.ccol );
      if( t.tmok && t.ts && !(t.xform==XF_OGG && t.szmode!=3) ) {
        el.push_back( i );
        dts[i].assign( size_t(t.nsamp+1), 0 );
        for( qword n=0; n<t.nsamp; n++ ) dts[i][size_t(n+1)] = dts[i][size_t(n)]+t.sdur[size_t(n)];
        qword a = 0;
        for( size_t c=0; c<t.npc; c++ ) a += t.spc[c], act[i].push_back( a );
      }
    }
    size_t bgain = 0, bm = ~size_t(0);
    for( size_t mi=0; mi<el.size() && el.size()>1; mi++ ) {
      size_t m = el[mi], gain = 0;
      std::vector<qword> B; qword a = 0;
      for( size_t c=0; c<T[m].npc; c++ ) B.push_back( dts[m][size_t(a)] ), a += T[m].spc[c];
      for( size_t ji=0; ji<el.size(); ji++ ) {
        size_t j = el[ji]; if( j==m ) continue;
        long long ca; int cr; long long cO; std::vector<qword> R;
        size_t cost = FitAligned( dts[j], act[j].size() ? act[j].back() : 0, T[j].ts, B, T[m].ts, act[j], ca, cr, cO, R );
        if( cost<plain[j] ) gain += plain[j]-cost;
      }
      if( gain>bgain ) bgain = gain, bm = m;
    }
    if( bm!=~size_t(0) ) {
      Track& m = T[bm];
      std::vector<qword> B; qword a = 0;
      for( size_t c=0; c<m.npc; c++ ) B.push_back( dts[bm][size_t(a)] ), a += m.spc[c];
      for( size_t ji=0; ji<el.size(); ji++ ) {
        size_t j = el[ji]; if( j==bm ) continue;
        Track& t = T[j];
        long long ca; int cr; long long cO; std::vector<qword> R;
        size_t cost = FitAligned( dts[j], act[j].size() ? act[j].back() : 0, t.ts, B, m.ts, act[j], ca, cr, cO, R );
        if( cost<plain[j] ) t.cmode = 1, t.cmaster = bm, t.ca = ca, t.crule = cr, t.cO = cO, t.ccol.swap( R );
      }
    }
  }

  // fragment indexes: marks at the top-level boxes except mdat from the first moof on;
  // sidx / tfra entries and the mfro size regenerated against them
  struct FragIdx { int kind; qword off, size; qword rc, j0; uint ref, ver, lc; Buf blob; bool ok; };
  std::vector<FragIdx> fx; std::vector<qword> marks;
  if( timing ) {
    size_t fm = top.size();
    for( size_t i=0; i<top.size(); i++ ) if( top[i].type==FCC_BE('m','o','o','f') ) { fm = i; break; }
    for( size_t i=fm; i<top.size(); i++ ) if( top[i].type!=FCC_BE('m','d','a','t') ) marks.push_back( top[i].pos );
    for( size_t i=0; i<top.size() && fm<top.size(); i++ ) {
      const Box& b = top[i];
      if( b.type==FCC_BE('s','i','d','x') && b.plen()>=24 ) {
        const byte* p = d+b.pay(); uint v = p[0];
        qword hl = v ? 32 : 24; if( b.plen()<hl ) continue;
        qword fo = v ? BE64(p+20) : BE32(p+16), rc = BE16(p+hl-2);
        if( b.plen()!=hl+12*rc || fo>fs ) continue;
        qword anchor = b.end()+fo;
        std::vector<qword>::iterator m = std::lower_bound( marks.begin(), marks.end(), anchor );
        if( m==marks.end() || *m!=anchor ) continue;
        FragIdx x; x.kind = TB_SIDX; x.off = b.pay()+hl; x.size = 12*rc; x.rc = rc; x.j0 = qword(m-marks.begin());
        x.ref = BE32(p+4); x.ver = v; x.lc = 0; x.ok = true; fx.push_back( x );
      }
      if( b.type==FCC_BE('m','f','r','a') ) {
        std::vector<Box> c = Children( d, b.pay(), b.end() );
        for( size_t j=0; j<c.size(); j++ ) {
          const byte* p = d+c[j].pay();
          if( c[j].type==FCC_BE('t','f','r','a') && c[j].plen()>=16 ) {
            uint v = p[0], lc = BE32(p+8)&63; qword n = BE32(p+12);
            qword esz = (v ? 16 : 8) + ((lc&3)+1) + (((lc>>2)&3)+1) + (((lc>>4)&3)+1);
            if( v>1 || (BE32(p+8)>>6) || c[j].plen()!=16+esz*n ) continue;
            FragIdx x; x.kind = TB_TFRA; x.off = c[j].pay()+16; x.size = esz*n; x.rc = n; x.j0 = 0;
            x.ref = BE32(p+4); x.ver = v; x.lc = lc; x.ok = true; fx.push_back( x );
          }
          if( c[j].type==FCC_BE('m','f','r','o') && c[j].plen()==8 && j+1==c.size() && BE32(p+4)==c[j].end()-b.pos ) {
            FragIdx x; x.kind = TB_MFRO; x.off = c[j].pay()+4; x.size = 4; x.rc = x.j0 = 0; x.ref = x.ver = x.lc = 0; x.ok = true; fx.push_back( x );
          }
        }
      }
    }
    if( fx.empty() ) marks.clear();
  }
  // resolve overlaps: drop offending tracks until the cut set is clean
  std::vector<Item> it; std::vector<qword> cut;
  bdo_ok.assign( bdo_pos.size(), 1 );
  for(;;) {
    it.clear();
    for( size_t k=0; k<bdo_pos.size(); k++ )
      if( bdo_ok[k] ) { Item x = { bdo_pos[k], 8, 0, TB_BDO, -1, k }; it.push_back(x); }
    for( size_t k=0; k<marks.size(); k++ ) { Item x = { marks[k], 0, 0, TB_MARK, -1, k }; it.push_back(x); }
    for( size_t k=0; k<fx.size(); k++ ) if( fx[k].ok ) { Item x = { fx[k].off, fx[k].size, fx[k].size, fx[k].kind, -1, k }; it.push_back(x); }
    for( size_t i=0; i<T.size(); i++ ) {
      Track& t = T[i]; if( !t.ok ) continue;
      for( size_t c=0; c<t.coff.size(); c++ ) { Item x = { t.coff[c], t.csz[c], t.csz[c], 0, int(i), c }; it.push_back(x); }
      if( t.has_stbl ) {
        qword n = (t.co64?8:4)*t.npc;
        Item s = { t.stco.pay()+8, n, n, t.co64?TB_CO64:TB_STCO, int(i), 0 }; it.push_back(s);
        if( t.szmode>=2 && t.nstp ) { Item z = { t.stsz.pay()+12, 4*t.nstp, 4*t.nstp, TB_STSZ, int(i), 0 }; it.push_back(z); }
      }
      for( size_t k=0; k<t.trun.size(); k++ )
        if( t.trun[k].h.flags&1 ) { Item x = { t.trun[k].b.pay()+8, 4, 0, TB_DOFF, int(i), k }; it.push_back(x); }
      if( (t.xform==XF_OGG || t.xform==XF_HDR) && t.dsilen ) { Item x = { t.dsipos, t.dsilen, t.dsilen, TB_DSI, int(i), 0 }; it.push_back(x); }
      if( t.szmode>=2 )
        for( size_t k=0; k<t.trun.size(); k++ ) {
          const TrunInfo& r = t.trun[k];
          int nd = int( (r.h.flags&0x200)!=0 ) + int( (r.h.flags&0x100) && (t.tflags&2) ) + int( (r.h.flags&0x800) && (t.tflags&4) );
          if( !nd || !r.h.count || t.spc[size_t(t.npc+k)]!=r.h.count ) continue;
          Item z = { r.b.pay()+r.h.hlen, qword(r.h.recsize)*r.h.count, 4*qword(nd)*r.h.count, TB_TRUN, int(i), k };
          it.push_back(z);
        }
      if( t.cflags&1 ) {
        qword n = 12*qword(BE32(d+t.stsc.pay()+4)); Item x = { t.stsc.pay()+8, n, n, TB_STSC, int(i), 0 }; it.push_back(x);
      }
      if( t.xform==XF_ANNEXB )                          // parameter sets: from the stream header
        for( size_t k=0; k<t.psets.size(); k++ ) if( t.psets[k].second ) { Item x = { t.psets[k].first, t.psets[k].second, t.psets[k].second, TB_PSET, int(i), k }; it.push_back(x); }
      if( t.tmok && t.hsdtp ) { qword n = t.sdv.size(); Item x = { t.sdtp.pay()+4, n, n, TB_SDTP, int(i), 0 }; if( n ) it.push_back(x); }
      if( t.tmok && t.has_stbl ) {                      // timing tables regenerated from the model
        if( t.cstts ) { qword n = 8*qword(BE32(d+t.stts.pay()+4)); Item x = { t.stts.pay()+8, n, n, TB_STTS, int(i), 0 }; it.push_back(x); }
        if( t.hctts && t.cctts ) { qword n = 8*qword(BE32(d+t.ctts.pay()+4)); Item x = { t.ctts.pay()+8, n, n, TB_CTTS, int(i), 0 }; it.push_back(x); }
        if( t.hstss && t.cstss ) { qword n = 4*qword(BE32(d+t.stss.pay()+4)); Item x = { t.stss.pay()+8, n, n, TB_STSS, int(i), 0 }; it.push_back(x); }
      }
      t.tflags &= ~8;
      if( t.tmok && t.tfdt.size() ) {                   // tfdt = base + dts of the traf's first sample
        std::vector<qword> dts( size_t(t.nsamp+1), 0 );
        for( qword n=0; n<t.nsamp; n++ ) dts[size_t(n+1)] = dts[size_t(n)]+t.sdur[size_t(n)];
        bool hb = false;
        for( size_t k=0; k<t.tfdt.size(); k++ ) {
          const Track::Tfdt& x = t.tfdt[k];
          if( x.trun>=t.trun.size() || x.pos>=t.trun[x.trun].b.pos ) continue;
          qword s0 = dts[size_t(t.cfirst[size_t(t.npc+x.trun)])];
          if( !hb ) t.tbase = (long long)(x.val-s0), hb = true;
          qword pv = qword(t.tbase)+s0;
          if( (x.ver ? pv : (pv&0xFFFFFFFFu))!=x.val ) continue;
          qword n = x.ver ? 8 : 4;
          Item z = { x.pos, n, n, TB_TFDT, int(i), k }; it.push_back(z); t.tflags |= 8;
        }
      }
    }
    std::sort( it.begin(), it.end() );
    bool bad=false;
    for( size_t k=1; k<it.size(); k++ )
      if( it[k-1].off+it[k-1].size>it[k].off ) {
        const Item* x[2] = { &it[k-1], &it[k] };
        for( int j=0; j<2; j++ )
          if( x[j]->kind==TB_BDO ) bdo_ok[size_t(x[j]->idx)]=0;
          else if( x[j]->kind==TB_SIDX || x[j]->kind==TB_TFRA || x[j]->kind==TB_MFRO ) fx[size_t(x[j]->idx)].ok = false;
          else if( x[j]->trk>=0 ) T[x[j]->trk].ok=false;
        bad=true;
      }
    if( !bad ) {
      // regions the decoder reads back from the struct must be intact
      for( size_t i=0; i<T.size(); i++ ) {
        Track& t = T[i]; if( !t.ok ) continue;
        if( t.has_stbl && !(t.cflags&1) && Intersects( t.stsc.pay(), t.stsc.end(), it ) ) t.ok=false, bad=true;
        if( t.has_stbl && t.szmode==0 && Intersects( t.stsz.pay(), t.stsz.end(), it ) ) t.ok=false, bad=true;
        for( size_t k=0; k<t.trun.size(); k++ ) {
          const TrunInfo& r = t.trun[k];
          qword e = t.szmode==0 ? r.b.end() : r.b.pay()+r.h.hlen;
          if( Intersects( r.b.pay(), e, it ) ) t.ok=false, bad=true;
        }
      }
    }
    if( !bad ) break;
    if( verbose ) log += "overlapping sample data/tables, dropping tracks\n";
  }
  // fragment index blobs (track set is final now): residuals vs the decoder's predictions
  {
    std::vector<int> tmi( T.size(), -1 ); int n = 0;
    for( size_t i=0; i<T.size(); i++ ) if( T[i].ok ) tmi[i] = n++;
    for( size_t q=0; q<it.size(); q++ ) {
      Item& x = it[q];
      if( x.kind!=TB_SIDX && x.kind!=TB_TFRA ) continue;
      FragIdx& f = fx[size_t(x.idx)];
      size_t ti = T.size();
      for( size_t i=0; i<T.size(); i++ ) if( T[i].ok && !T[i].blob && T[i].tid==f.ref ) { ti = i; break; }
      const Track* t = ti<T.size() ? &T[ti] : 0;
      bool tim = t && (t->tflags&1);
      std::vector<qword> dts;
      if( tim ) { dts.assign( size_t(t->nsamp+1), 0 ); for( qword s=0; s<t->nsamp; s++ ) dts[size_t(s+1)] = dts[size_t(s)]+t->sdur[size_t(s)]; }
      static const std::vector<qword> none;
      Writer w; std::vector<qword> col[5];
      const byte* e = d+f.off;
      if( f.kind==TB_SIDX ) {
        std::vector<qword> psz, pdur;
        SidxPredict( marks, f.j0, f.rc, t ? t->coff : none, t ? t->cfirst : none, t ? t->spc : none, tim ? t->sdur.data() : 0, psz, pdur );
        uint sap = 0;
        for( qword k=0; k<f.rc; k++, e+=12 ) {
          col[0].push_back( ZigZag( (long long)BE32(e) - (long long)uint(psz[size_t(k)]) ) );
          col[1].push_back( ZigZag( (long long)BE32(e+4) - (long long)uint(pdur[size_t(k)]) ) );
          col[2].push_back( BE32(e+8)^sap ); sap = BE32(e+8);
        }
        w.vint( f.rc ); w.vint( f.j0 ); w.vint( t ? qword(tmi[ti]+1) : 0 );
        for( int c=0; c<3; c++ ) PutColN( w, col[c] );
      } else {
        std::vector<qword> poff, ptime;
        TfraPredict( marks, f.rc, t ? size_t(t->npc) : 0, t ? t->coff : none, t ? t->cfirst : none, tim ? dts.data() : 0,
                     tim && (t->hctts || t->fcto) ? t->scto.data() : 0, t && (t->tflags&8) ? t->tbase : 0, poff, ptime );
        for( qword k=0; k<f.rc; k++ ) {
          qword tv, ov;
          if( f.ver ) tv = BE64(e), ov = BE64(e+8), e += 16; else tv = BE32(e), ov = BE32(e+4), e += 8;
          qword pt = ptime[size_t(k)], po = poff[size_t(k)];
          if( !f.ver ) pt &= 0xFFFFFFFFu, po &= 0xFFFFFFFFu;
          col[0].push_back( ZigZag( (long long)(tv-pt) ) ); col[1].push_back( ZigZag( (long long)(ov-po) ) );
          for( int c=0; c<3; c++ ) {
            int L = int((f.lc>>(2*c))&3)+1; qword v = 0;
            for( int b=0; b<L; b++ ) v = v<<8 | *e++;
            col[2+c].push_back( v );
          }
        }
        w.u8( f.ver ); w.u8( f.lc ); w.vint( f.rc ); w.vint( t ? qword(tmi[ti]+1) : 0 );
        for( int c=0; c<5; c++ ) PutColN( w, col[c] );
      }
      if( w.b.size()<f.size ) f.blob.swap( w.b ), x.rem = f.size-f.blob.size();
      else f.ok = false;
    }
    long long gain = -3*(long long)marks.size();       // a mark costs its op and a split raw op
    for( size_t q=0; q<it.size(); q++ )
      if( it[q].kind==TB_SIDX || it[q].kind==TB_TFRA || it[q].kind==TB_MFRO ) {
        const FragIdx& f = fx[size_t(it[q].idx)];
        if( f.ok ) gain += (long long)f.size - (long long)(it[q].kind==TB_MFRO ? 1 : f.blob.size()+1);
      }
    for( size_t q=it.size(); q-- >0; )                 // not worth it: keep those bytes in the struct
      if( ( (it[q].kind==TB_SIDX || it[q].kind==TB_TFRA) && !fx[size_t(it[q].idx)].ok ) ||
          ( gain<=0 && (it[q].kind==TB_SIDX || it[q].kind==TB_TFRA || it[q].kind==TB_MFRO || it[q].kind==TB_MARK) ) ) it.erase( it.begin()+q );
  }
  cut.assign( it.size()+1, 0 );
  for( size_t k=0; k<it.size(); k++ ) cut[k+1] = cut[k]+it[k].rem;
  for( size_t i=0; i<T.size(); i++ ) {               // master dropped: plain runs
    Track& t = T[i];
    if( t.ok && (t.cflags&1) && t.cmode==1 && !T[t.cmaster].ok ) t.cmode = 0, t.ccol.assign( t.spc.begin(), t.spc.begin()+size_t(t.npc) );
  }
  // chunk order group: chunk ops predicted as the group track with the smallest (next-b, index)
  {
    std::vector<std::pair<int,qword> > seq;
    for( size_t k=0; k<it.size(); k++ ) if( it[k].kind==0 ) seq.push_back( std::make_pair( it[k].trk, it[k].idx ) );
    std::vector<char> grp( T.size() ); std::vector<long long> b( T.size(), 0 );
    for( size_t i=0; i<T.size(); i++ ) grp[i] = timing && T[i].ok && !T[i].blob && T[i].coff.size();
    auto sim = [&]() {
      std::vector<qword> nx( T.size(), 0 ); size_t h = 0;
      for( size_t k=0; k<seq.size(); k++ ) {
        size_t p = T.size(); long long bk = 0;
        for( size_t u=0; u<T.size(); u++ ) {
          if( !grp[u] || nx[u]>=T[u].coff.size() ) continue;
          long long key = (long long)nx[u]-b[u];
          if( p==T.size() || key<bk ) p = u, bk = key;
        }
        h += p==size_t(seq[k].first) && seq[k].second==nx[size_t(seq[k].first)];
        nx[size_t(seq[k].first)] = seq[k].second+1;
      }
      return h;
    };
    size_t best = sim();
    for( int round=0; round<2; round++ )
      for( size_t i=0; i<T.size(); i++ ) {
        if( !grp[i] ) continue;
        long long bb = b[i];
        for( long long v=-2; v<=2; v++ ) { b[i] = v; size_t h = sim(); if( h>best ) best = h, bb = v; }
        b[i] = bb;
      }
    bool use = best*2>=seq.size() && best;
    for( size_t i=0; i<T.size(); i++ ) if( grp[i] && use ) T[i].cflags |= 2, T[i].cb = b[i];
  }

  std::vector<int> tm( T.size(), -1 ); int nt=0;
  for( size_t i=0; i<T.size(); i++ ) if( T[i].ok ) tm[i]=nt++;

  // ops + struct
  Writer op; Buf st; st.reserve( size_t(fs - cut.back()) );
  std::vector<qword> next( T.size(), 0 );
  qword cur=0; size_t prun_at = 0; qword prun_n = 0; bool prun_open = false; qword npred = 0, nchops = 0;
  for( size_t k=0; k<it.size(); k++ ) {
    const Item& x = it[k];
    if( x.off>cur ) { prun_open = false; op.u8(OP_RAW); op.vint(x.off-cur); st.insert( st.end(), d+cur, d+x.off ); cur=x.off; }
    if( x.kind!=0 ) prun_open = false;
    int ti = x.trk<0 ? -1 : tm[x.trk];
    if( x.kind==TB_BDO ) {
      byte b[8]; PutBE64( b, x.off-BE64(d+x.off) );
      op.u8(OP_TABLE); op.u8(TB_BDO); st.insert( st.end(), b, b+8 );
    } else if( x.kind==TB_MARK ) {
      op.u8(OP_MARK);
    } else if( x.kind==TB_MFRO ) {
      op.u8(OP_TABLE); op.u8(x.kind);
    } else if( x.kind==TB_SIDX || x.kind==TB_TFRA ) {
      const Buf& bl = fx[size_t(x.idx)].blob;
      op.u8(OP_TABLE); op.u8(x.kind); st.insert( st.end(), bl.begin(), bl.end() );
    } else if( x.kind==TB_DOFF ) {
      const Track& t = T[x.trk];
      byte b[4]; PutBE32( b, uint(x.off) - uint(t.coff[size_t(t.npc+x.idx)]) + BE32(d+x.off) );
      op.u8(OP_TABLE); op.u8(TB_DOFF); op.vint(ti); st.insert( st.end(), b, b+4 );
    } else if( x.kind==0 ) {
      size_t p = T.size(); long long bk = 0;            // predicted chunk (as OP_CRUN)
      for( size_t u=0; u<T.size(); u++ ) {
        if( !(T[u].ok && (T[u].cflags&2)) || next[u]>=T[u].coff.size() ) continue;
        long long key = (long long)next[u]-T[u].cb;
        if( p==T.size() || key<bk ) p = u, bk = key;
      }
      nchops++;
      if( p==size_t(x.trk) && x.idx==next[x.trk] ) {
        if( prun_open ) op.b.resize( prun_at ), prun_n++;
        else prun_at = op.b.size(), prun_n = 1;
        op.u8(OP_CRUN); op.vint(prun_n); prun_open = true; npred++;
      } else {
        prun_open = false;
        if( x.idx==next[x.trk] && ti<OP_RAW ) op.u8(ti);
        else op.u8(OP_CHUNK), op.vint(ti), op.vint(x.idx);
      }
      next[x.trk] = x.idx+1;
    } else {
      op.u8(OP_TABLE); op.u8(x.kind); op.vint(ti);
      if( x.kind==TB_TRUN ) {                  // keep the per-sample fields that aren't derived
        const Track& t = T[x.trk];
        const TrunHdr& h = t.trun[size_t(x.idx)].h;
        for( qword j=0; j<h.count; j++ ) {
          const byte* r = d+x.off+j*h.recsize;
          for( uint fb=0x100; fb<=0x800; fb<<=1 ) {
            if( !(h.flags&fb) ) continue;
            bool der = fb==0x200 || (fb==0x100 && (t.tflags&2)) || (fb==0x800 && (t.tflags&4));
            if( !der ) st.insert( st.end(), r, r+4 );
            r += 4;
          }
        }
      }
    }
    cur += x.size;
  }
  st.insert( st.end(), d+cur, d+fs ); op.u8(OP_END);

  Writer w; PutMetaHdr( w, FMT_MP4 );
  w.vint(nt);
  streams.clear(); names.clear();
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; if( !t.ok ) continue;
    if( t.blob ) {
      streams.push_back( Buf( d+t.coff[0], d+t.coff[0]+t.csz[0] ) );
      TrackHead h; h.ext = t.name; h.xform = XF_BLOB; PutHead( w, h );
      names.push_back( TrackSuffix( names.size(), h.ext ) );
      if( verbose ) { char l[128]; snprintf( l, sizeof(l), "covr: %llu bytes -> %s\n", (unsigned long long)t.csz[0], names.back().c_str() ); log += l; }
      continue;
    }
    // stream: samples in sample order, converted to an elementary stream format if possible
    Buf s; s.reserve( size_t(t.bytes) );
    for( size_t c=0; c<t.coff.size(); c++ ) s.insert( s.end(), d+t.coff[c], d+t.coff[c]+t.csz[c] );
    Buf x; bool conv = false;
    if( t.xform!=XF_NONE ) {
      if( t.xform==XF_ANNEXB ) ToAnnexB( t, s, x );
      else if( t.xform==XF_OGG ) ToOgg( t, s, x, uint(t.idx+1), 0 );
      else if( t.xform==XF_HDR ) x.reserve( t.xhdr.size()+s.size() ), x.assign( t.xhdr.begin(), t.xhdr.end() ), x.insert( x.end(), s.begin(), s.end() );
      else ToADTS( t, s, x );
      qword rawb=0; for( size_t k=0; k<t.xa.size(); k++ ) rawb += t.xa[k].second;
      if( t.xform==XF_ANNEXB && 2*rawb>s.size() ) t.xform = XF_NONE;   // mostly not NALs: keep raw
      else conv = true;
    }
    if( t.tmok ) {                            // timing model, from the inputs the decoder will have
      int ck = t.xform==XF_OGG ? 3 : t.lensize && t.xext=="h264" ? 1 : t.lensize && t.xext=="hevc" ? 2 :
               t.hstss && t.fck!=CK_RAW && t.fck!=CK_AUDIO ? 4 : 0;
      std::vector<qword> fo( size_t(t.nsamp+1), 0 );
      for( qword k=0; k<t.nsamp; k++ ) fo[size_t(k+1)] = fo[size_t(k)]+t.Size(k);
      Buf pv, none;
      if( ck==3 ) { XiphLace( t.vl, 3, pv ); for( int h=0; h<3; h++ ) pv.insert( pv.end(), t.vh[h], t.vh[h]+t.vl[h] ); }
      TimeIn ti;
      if( !TimeInputs( ck, t.lensize, t.xform==XF_ANNEXB ? t.xhdr : none, pv, s.data(), fo, t.nsamp, ti, t.fck, t.fckp ) ) ck = 0, TimeInputs( 0, 0, none, none, s.data(), fo, t.nsamp, ti );
      std::vector<byte> sy;
      if( t.hstss ) { sy = t.ssync; sy.resize( size_t(t.nsamp), 2 ); }
      t.tm.ckind = ck; t.tm.lensize = ck==1 || ck==2 ? t.lensize : 0; t.tm.fck = ck==4 ? t.fck : 0; t.tm.fckp = ck==4 ? t.fckp : 0;
      TimeFit( t.tm, ti, t.nsamp, t.sdur, (t.hctts || t.fcto) ? &t.scto : 0, t.hstss ? &sy : 0 );
      if( ck==4 ) {                           // frame classes worse than no sync prediction: drop them
        qword ns = 0; for( size_t k=0; k<sy.size(); k++ ) ns += sy[k]==1;
        if( t.tm.sexc.size()>=ns ) {
          ck = 0; TimeInputs( 0, 0, none, none, s.data(), fo, t.nsamp, ti ); t.tm.ckind = 0; t.tm.fck = 0; t.tm.fckp = 0;
          TimeFit( t.tm, ti, t.nsamp, t.sdur, (t.hctts || t.fcto) ? &t.scto : 0, &sy );
        }
      }
      if( t.hsdtp && t.sdv.size() ) SdtpFit( t.tm, ti, t.sdv );
      if( verbose ) {
        char l[160]; size_t nz = 0, nc = 0;
        for( size_t k=0; k<t.tm.rdur.size(); k++ ) nz += t.tm.rdur[k]!=0;
        for( size_t k=0; k<t.tm.rcto.size(); k++ ) nc += t.tm.rcto[k]!=0;
        if( t.tm.m ) snprintf( l, sizeof(l), " time=%s/%llu*%.6f dur_resid=%llu", ck==1 ? "h264" : ck==2 ? "hevc" : ck==3 ? "vorbis" : "index",
          (unsigned long long)t.tm.q, ldexp( double(t.tm.m), -t.tm.k ), (unsigned long long)nz );
        else snprintf( l, sizeof(l), " time=durations" );   // no model: the duration column is cheaper
        t.tinfo = l;
        if( ck==4 ) t.tinfo += " sync=class";
        if( t.tm.hascto ) snprintf( l, sizeof(l), " cto_resid=%llu", (unsigned long long)nc ), t.tinfo += l;
        if( t.tm.hassync ) snprintf( l, sizeof(l), " sync_exc=%llu", (unsigned long long)t.tm.sexc.size() ), t.tinfo += l;
        if( t.tm.hassdtp ) snprintf( l, sizeof(l), " sdtp_exc=%llu", (unsigned long long)t.tm.sdexc.size() ), t.tinfo += l;
      }
    }
    if( conv ) s.swap( x ), nxf++;
    if( t.ext=="mp3" && s.size()>=2 && s[0]==0xFF && (s[1]&0xE0)==0xE0 ) {
      int lay = (s[1]>>1)&3;
      if( lay==3 ) t.ext = "mp1"; else if( lay==2 ) t.ext = "mp2";
    }
    streams.push_back( Buf() ); streams.back().swap( s );

    TrackHead h;
    h.ext = t.xform!=XF_NONE ? t.xext : t.ext;
    h.xform = t.xform; h.nhdr = t.xform==XF_OGG ? 3 : 0;
    if( t.xform==XF_ANNEXB ) h.lensize = t.lensize, h.hdrlen = t.xhdr.size(), h.xb = t.xb;
    if( t.xform==XF_HDR ) h.hdrlen = t.xhdr.size();
    if( t.xform==XF_ANNEXB || t.xform==XF_ADTS ) h.xa = t.xa;
    h.nf = t.nsamp; h.szmode = t.szmode==0 ? SZ_TABLES : t.szmode;
    if( t.szmode==2 ) h.ptype = t.ptype, h.pparam = t.pparam, h.col.assign( t.ucnt.begin(), t.ucnt.end() ), h.esc = t.esc;
    PutHead( w, h );
    std::string suf = TrackSuffix( names.size(), h.ext ); names.push_back( suf );
    w.u8( t.has_stbl );
    if( t.has_stbl ) {
      w.vint( t.nch_stbl ); w.vint( StructOff(t.stsc.pay(),it,cut) ); w.vint( t.stsc.plen() );
      if( t.szmode==0 ) w.u8( t.stz2 ), w.vint( StructOff(t.stsz.pay(),it,cut) ), w.vint( t.stsz.plen() );
    }
    w.vint( t.trun.size() );
    qword prev=0;
    for( size_t k=0; k<t.trun.size(); k++ ) {
      qword p = StructOff( t.trun[k].b.pay(), it, cut );
      w.vint( p-prev ); prev=p;
      if( t.szmode==0 ) w.vint( !(t.trun[k].h.flags&0x200) && t.trun[k].h.count ? t.fsz[size_t(t.cfirst[size_t(t.npc+k)]-t.ss.count)] : 0 );
    }
    w.u8( t.tflags );
    if( t.tflags&1 ) PutTimeModel( w, t.tm );
    if( t.tflags&8 ) w.vint( ZigZag( t.tbase ) );
    w.u8( t.cflags );
    if( t.cflags&1 ) {
      w.vint( t.sdi ); w.u8( t.cmode );
      if( t.cmode==1 ) {
        w.vint( qword(tm[t.cmaster]) ); w.vint( t.ts ); w.vint( T[t.cmaster].ts );
        w.vint( ZigZag( t.ca ) ); w.u8( t.crule ); w.vint( ZigZag( t.cO ) );
      }
      PutCol( w, t.ccol );
    }
    if( t.cflags&2 ) w.vint( ZigZag( t.cb ) );
    if( verbose ) {
      char l[256];
      snprintf( l, sizeof(l), "trak %d: %-4s %8llu samples %7llu chunks (%llu trun) %10llu bytes  sizes=%s",
        t.idx, FourccStr(t.fourcc).c_str(), (unsigned long long)t.nsamp, (unsigned long long)t.coff.size(),
        (unsigned long long)t.trun.size(), (unsigned long long)t.bytes, t.szmode==3 ? (t.xform==XF_OGG ? "ogg" : "adts") : ptname[t.ptype] );
      log += l;
      if( t.szmode==2 ) snprintf( l, sizeof(l), " esc=%llu", (unsigned long long)t.esc.size() ), log += l;
      log += t.tinfo;
      if( t.cflags&1 ) {
        size_t nz = 0; for( size_t k=0; k<t.ccol.size(); k++ ) nz += t.cmode==1 && t.ccol[k]!=0;
        if( t.cmode==1 ) snprintf( l, sizeof(l), " stsc=aligned(trak%d,resid=%llu)", T[t.cmaster].idx, (unsigned long long)nz );
        else snprintf( l, sizeof(l), " stsc=runs" );
        log += l;
      }
      if( t.xform==XF_ANNEXB ) snprintf( l, sizeof(l), "  -> annexb hdr=%llu raw=%llu expl=%llu",
        (unsigned long long)t.xhdr.size(), (unsigned long long)t.xa.size(), (unsigned long long)t.xb.size() ), log += l;
      if( t.xform==XF_ADTS ) snprintf( l, sizeof(l), "  -> adts esc=%llu", (unsigned long long)t.xa.size() ), log += l;
      if( t.xform==XF_OGG ) snprintf( l, sizeof(l), "  -> ogg hdr=%llu+%llu+%llu bs=%u/%u modes=%d%s",
        (unsigned long long)t.vl[0], (unsigned long long)t.vl[1], (unsigned long long)t.vl[2], t.vi.bs[0], t.vi.bs[1], t.vi.nmodes,
        t.dsilen ? " dsi=cut" : "" ), log += l;
      log += "\n";
    }
  }
  if( verbose ) {
    char l[160];
    snprintf( l, sizeof(l), "chunk ops: %llu of %llu predicted; struct %llu, ops %llu\n", (unsigned long long)npred, (unsigned long long)nchops,
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
  // fallbacks: without stream conversion, without the timing model, without both
  static const bool lv[4][2] = { {true,true}, {false,true}, {true,false}, {false,false} };
  const char* why = ""; int nxf = 0, nxf0 = 0; std::string log;
  for( int i=0; i<4; i++ ) {
    if( i>0 && !lv[i][0] && !nxf0 ) continue;       // no conversions: same as with them
    log.clear();
    bool ok = Encode1( in, lv[i][0], lv[i][1], meta, streams, names, nxf, &why, log );
    if( i==0 ) nxf0 = nxf;
    if( ok ) { if( verbose ) fputs( log.c_str(), stdout ); return true; }
    fprintf( stderr, "warning: verify failed (%s)%s\n", why, i<3 ? ", retrying with fewer transforms" : "" );
  }
  meta.clear(); streams.clear(); names.clear();
  return false;
}

} // namespace mp4
