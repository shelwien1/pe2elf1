// MKV: encoder
namespace mkv {

static bool Encode1( const Buf& in, bool usexf, bool feat, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names,
                     int& nxf, const char** why, std::string& log ) {
  const byte* d = in.data(); qword fs = in.size();
  std::vector<Track> T; std::vector<Blk> B;
  Parser P( d, fs, T, B ); P.File();
  nxf = 0;
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    if( !t.blob && t.nsamp==0 ) t.ok = false;
    if( t.ok && !t.blob ) t.Unstrip( d );
    if( t.ok ) FinishCodec( d, t );
    if( !usexf ) t.xform = XF_NONE;
    if( t.ok && !t.blob ) ChooseSizeMode( d, t );
  }
  std::stable_sort( B.begin(), B.end(), []( const Blk& a, const Blk& b ) { return a.pos!=b.pos ? a.pos<b.pos : Prio(a.kind)<Prio(b.kind); } );
  std::vector<int> tm( T.size(), -1 ); int nt=0;
  for( size_t i=0; i<T.size(); i++ ) if( T[i].ok ) tm[i] = nt++;

  // ops + struct. Run twice: the first pass numbers the marks for the position elements.
  Writer op; Buf st; st.reserve( size_t(fs>>4) );
  qword cur = 0; bool bad = false;
  std::map<qword,long long> markat;
  int nops[4] = {0,0,0,0};                          // clusters, marks, positions, timestamps
  size_t prun_at = 0; qword prun_n = 0; bool prun_open = false;   // last op is an extendable OP_PRUN
  std::map<std::string,qword> stc;                  // struct composition (verbose)
  auto rawstat = [&]( qword a, qword b ) {          // raw bytes [a,b) by enclosing level-1 element
    size_t lo = 0, hi = P.L1.size();
    while( lo<hi ) { size_t m = (lo+hi)/2; if( P.L1[m].end<=a ) lo = m+1; else hi = m; }
    for( qword p=a; p<b; ) {
      while( lo<P.L1.size() && P.L1[lo].end<=p ) lo++;
      qword e = b; const char* nm = "other";
      if( lo<P.L1.size() && P.L1[lo].pos<=p ) {
        uint id = P.L1[lo].id; e = std::min( b, P.L1[lo].end );
        nm = id==ID_CLUSTER ? "clusters" : id==ID_CUES ? "cues" : id==ID_SEEKHEAD ? "seekhead" : id==ID_TRACKS ? "tracks" :
             id==ID_INFO ? "info" : id==ID_TAGS ? "tags" : id==ID_ATTACHMENTS ? "attachments" : id==ID_CHAPTERS ? "chapters" :
             id==0xEC ? "void" : "other";
      } else if( lo<P.L1.size() ) e = std::min( b, P.L1[lo].pos );
      stc[nm] += e-p; p = e;
    }
  };
  qword phit = 0, pall = 0; bool usepred = false;   // timestamp-order prediction of the block track
  auto opc = [&]( uint c ) { prun_open = false; op.u8(c); };
  auto raw = [&]( qword to ) {
    if( to>cur ) {
      opc(OP_RAW); op.vint(to-cur); st.insert( st.end(), d+cur, d+to );
      if( verbose ) rawstat( cur, to );
      cur = to;
    }
  };
  auto run = [&]( int mode ) {                     // 0: number marks, kinds; 1: count predictions; 2: output
    bool dry = mode<2;
    op.b.clear(); st.clear(); cur = 0; bad = false; prun_open = false; stc.clear();
    for( int i=0; i<4; i++ ) nops[i] = 0;
    for( size_t i=0; i<T.size(); i++ ) {
      Track& t = T[i];
      t.bcts.clear(); t.gmode.clear(); t.bflx.clear(); t.lcnt.clear(); t.nbe = t.nfe = 0; t.haslast = false; t.lastabs = 0;
      t.nsb = t.ngb = t.nlb = 0; t.privcut = false;
      if( mode==0 ) t.kc[0] = t.kc[1] = t.kc[2] = 0, t.ksb.clear(), t.km = 0;
    }
    std::vector<qword> opened; long long nmark = 0, prevmi = 0; qword cts = 0, segst = 0; bool inseg = false; int curseg = 0, nsegop = 0;
    // emitted blocks, for OP_CUE: time, track number, cluster mark, relative position, duration
    struct EBlk { long long abs, clmark, rel; qword tnum, dur; bool relok, hasdur; };
    std::vector<EBlk> eb; std::map<std::pair<long long,qword>,std::vector<size_t> > ebk;
    std::vector<qword> markpos; qword cdata = 0; long long lastcue = -1;
    for( size_t k=0; k<B.size(); k++ ) {
      const Blk& b = B[k];
      if( b.kind<=K_PRIV && !T[size_t(b.trk)].ok ) continue;
      if( !feat && b.kind>=K_CLUSTER ) continue;
      if( b.kind==K_CEND ) {
        std::vector<qword>::iterator it = std::find( opened.begin(), opened.end(), b.val );
        if( it==opened.end() ) continue;
        if( b.pos<cur ) { bad = true; continue; }
        raw( b.pos ); opc(OP_CEND); opened.erase( it );
        continue;
      }
      if( b.pos<cur ) continue;
      int ti = b.trk>=0 ? tm[size_t(b.trk)] : -1;
      size_t st0 = st.size(), rw0 = 0;
      for( std::map<std::string,qword>::iterator it=stc.begin(); it!=stc.end(); ++it ) rw0 += it->second;
      switch( b.kind ) {
      case K_BLOB: raw( b.pos ); opc(OP_BLOB); op.vint(ti); cur = b.end; break;
      case K_PRIV:
        if( T[size_t(b.trk)].xform!=XF_OGG ) break;
        raw( b.pos ); opc(OP_PRIV); op.vint(ti); cur = b.end; T[size_t(b.trk)].privcut = true;
        break;
      case K_CLUSTER:
        raw( b.pos ); opc(OP_CLUSTER); cur = b.end; opened.push_back( b.pos ); markpos.push_back( b.pos ); cdata = b.end;
        if( mode==0 ) markat[b.pos] = nmark;
        nmark++; nops[0]++;
        break;
      case K_MARK:
        raw( b.pos ); opc(OP_MARK); markpos.push_back( b.pos );
        if( mode==0 ) markat[b.pos] = nmark;
        nmark++; nops[1]++;
        break;
      case K_SEG: raw( b.pos ); opc(OP_SEG); segst = b.pos; inseg = true; curseg = nsegop++; break;
      case K_CTS: {
        size_t pt = T.size(); long long pk = 0;     // prediction: time of the next block (as the decoder)
        for( size_t j=0; j<T.size(); j++ ) {
          const Track& u = T[j];
          if( !u.ok || u.blob || !u.tsm || u.nbe>=u.btc.size() || u.seg!=curseg ) continue;
          long long key = IKey( u, cts );
          if( pt==T.size() || key<pk ) pt = j, pk = key;
        }
        qword pc = pt<T.size() ? qword( PtsKey( T[pt] ) ) : 0;
        qword n = b.end-b.hend; int pn = 1;
        while( pn<8 && (pc>>(8*pn)) ) pn++;
        raw( b.pos );
        if( b.val==pc && n==qword(pn) && b.hend-b.pos==2 ) opc(OP_CTSP);
        else {
          opc(OP_CTS); st.insert( st.end(), d+b.pos, d+b.hend );
          qword v = b.val-pc;
          for( qword j=0; j<n; j++ ) st.push_back( byte( v>>(8*(n-1-j)) ) );
        }
        cur = b.end; cts = b.val; nops[3]++;
      } break;
      case K_VOID:
        raw( b.pos ); opc(OP_VOID); op.vint( b.val ); op.u8( b.szl ); cur = b.end;
        break;
      case K_CRC: {                                  // first child of the cluster just opened: covers the rest of it
        bool cl = !opened.empty() && opened.back()==b.gpos && cdata==b.pos;
        raw( b.pos ); opc(OP_CRC); op.vint( cl ? 0 : b.val+1 ); cur = b.end;
      } break;
      case K_SEEK: {
        if( mode<2 || !inseg || segst!=b.seg || !opened.empty() ) break;
        std::map<qword,long long>::iterator it = markat.find( b.seg+b.val );
        if( it==markat.end() ) break;
        uint id = 0; qword tp = b.seg+b.val;
        int il = tp<fs ? ReadID( d+tp, fs-tp, id ) : 0;
        if( !il || il!=(b.cfl&3)+1 || id!=b.ctrack ) break;
        raw( b.pos ); opc(OP_SEEK); op.vint( ZigZag( it->second-prevmi ) ); op.u8( b.cfl ); prevmi = it->second;
        cur = b.end; nops[2]++;
      } break;
      case K_CUE: {
        if( mode<2 || !inseg || segst!=b.seg || !opened.empty() ) break;
        std::map<std::pair<long long,qword>,std::vector<size_t> >::iterator it = ebk.find( std::make_pair( (long long)b.val, b.ctrack ) );
        if( it==ebk.end() ) break;
        long long bix = -1;
        for( size_t j=0; j<it->second.size() && bix<0; j++ ) {
          const EBlk& c = eb[it->second[j]];
          if( c.clmark<0 || markpos[size_t(c.clmark)]-segst!=b.ccpos ) continue;
          if( (b.cfl&1) && (!c.relok || qword(c.rel)!=b.crel) ) continue;
          if( (b.cfl&2) && (!c.hasdur || c.dur!=b.cdur) ) continue;
          bix = (long long)it->second[j];
        }
        if( bix<0 ) break;
        raw( b.pos ); opc(OP_CUE); op.vint( ZigZag( bix-lastcue-1 ) ); op.u8( b.cfl ); lastcue = bix;
        cur = b.end; nops[2]++;
      } break;
      case K_POS: {
        if( dry || !inseg || segst!=b.seg ) break;
        std::map<qword,long long>::iterator it = markat.find( b.seg+b.val );
        if( it==markat.end() ) break;
        raw( b.pos ); opc(OP_POS); op.vint( ZigZag( it->second-prevmi ) ); prevmi = it->second;
        st.insert( st.end(), d+b.pos, d+b.hend ); cur = b.end; nops[2]++;
      } break;
      case K_BLOCK: {
        Track& t = T[size_t(b.trk)];
        if( t.nbe>=t.btc.size() ) { bad = true; break; }
        long long abs = (long long)cts + int16_t(uint16_t(t.btc[size_t(t.nbe)])), pred = t.haslast ? t.lastabs-abs : 0;
        qword data = b.end-b.hend, bsz = b.end-b.szpos-b.szl;
        bool idok = b.szpos-b.pos==1 && d[b.pos]==(b.gend ? 0xA1 : 0xA3) && b.szl==VintLen(bsz);
        bool sb = feat && !b.gend && idok, gb = false;
        std::vector<Elem> ch;
        if( feat && b.gend && idok && b.gpos>=cur && b.gidl==1 && d[b.gpos]==0xA0 && b.pos==b.gpos+1+b.gszl && b.gszl==VintLen(b.gend-b.pos) ) {
          gb = true;
          for( qword p=b.end; p<b.gend; ) {
            Elem y;
            if( !GetElem(d,p,b.gend,y) || y.unk || y.trunc ) { gb = false; break; }
            ch.push_back( y ); p = y.end;
          }
        }
        bool laced = b.hend>b.lpos;
        // track predicted by timestamp order (as the decoder's OP_PRUN)
        size_t pt = T.size(); long long pk = 0;
        for( size_t j=0; j<T.size(); j++ ) {
          const Track& u = T[j];
          if( !u.ok || u.blob || u.nbe>=u.btc.size() || u.seg!=curseg ) continue;
          long long key = IKey( u, cts );
          if( pt==T.size() || key<pk ) pt = j, pk = key;
        }
        int kind = gb ? 1 : sb ? 0 : 2;
        if( mode==0 ) t.kc[kind]++;
        if( mode==1 ) phit += pt==size_t(b.trk), pall++;
        // op for this block: extend/start an OP_PRUN when predicted, else explicit
        auto blkop = [&]() {
          if( !dry && usepred && pt==size_t(b.trk) && kind==t.dkind ) {
            if( prun_open ) { prun_n++; op.b.resize( prun_at ); }
            else prun_at = op.b.size(), prun_n = 1;
            op.u8(OP_PRUN); op.vint(prun_n); prun_open = true;
          } else if( kind==1 ) { if( ti<0x40 ) opc(0x40+ti); else opc(OP_GB), op.vint(ti); }
          else if( kind==0 ) { if( ti<0x40 ) opc(ti); else opc(OP_SB), op.vint(ti); }
          else { if( ti<0x60 ) opc(0x80+ti); else opc(OP_BLOCK), op.vint(ti); }
        };
        // flags: lacing type coded relative to the predicted one
        uint fl = t.bfl[size_t(t.nbe)], la = (fl>>1)&3;
        if( kind==0 ) {
          if( mode==0 ) t.ksb.push_back( std::make_pair( t.nfe, byte(fl>>7) ) );
          if( t.km && t.nfe<t.kf.size() && t.kf[size_t(t.nfe)] ) fl ^= 0x80;
        }
        if( la ) {
          if( t.nfe+b.nf>t.fsz.size() ) { bad = true; break; }
          int p = LacePred( &t.fsz[size_t(t.nfe)], b.nf );
          fl = (fl&~6u) | ((1+(la+3-uint(p))%3)<<1);
        }
        t.bflx.push_back( byte(fl) );
        if( laced && b.regen ) t.lcnt.push_back( qword(b.nf-1) );
        if( gb ) {
          raw( b.gpos ); blkop();
          if( laced && !b.regen ) st.insert( st.end(), d+b.lpos, d+b.hend );
          qword gm = ch.size()+2;
          if( ch.empty() ) gm = 0;
          else if( ch.size()==1 && ch[0].id==0xFB && ch[0].idl==1 && ch[0].szl==1 && ch[0].size>=1 && ch[0].size<=8 ) {
            int sh = int(64-8*ch[0].size);
            long long v = (long long)( ReadUintBE( d+ch[0].pos+2, ch[0].size )<<sh ) >> sh;
            if( v==pred && int(ch[0].size)==MinSLen(pred) ) gm = 1;
          }
          if( gm>=2 )
            for( size_t c=0; c<ch.size(); c++ ) {
              const Elem& y = ch[c];
              st.insert( st.end(), d+y.pos, d+y.pos+y.hdr );
              if( y.id==0xFB && y.size>=1 && y.size<=8 ) {    // ReferenceBlock: store minus the prediction
                qword v = ReadUintBE( d+y.pos+y.hdr, y.size ) - qword(pred);
                for( qword j=0; j<y.size; j++ ) st.push_back( byte( v>>(8*(y.size-1-j)) ) );
              } else st.insert( st.end(), d+y.pos+y.hdr, d+y.end );
            }
          t.gmode.push_back( gm ); cur = b.gend; t.ngb++;
        } else if( sb ) {
          raw( b.pos ); blkop();
          if( laced && !b.regen ) st.insert( st.end(), d+b.lpos, d+b.hend );
          cur = b.end; t.nsb++;
        } else {
          if( b.gend && b.gpos+b.gidl>=cur ) {
            raw( b.gpos+b.gidl );
            opc(OP_GSIZE); op.vint(ti); op.vint(b.nf);
            PutResid( st, d+b.gpos+b.gidl, b.gszl, data ); cur = b.gpos+b.gidl+b.gszl;
          }
          raw( b.pos ); blkop();
          st.insert( st.end(), d+b.pos, d+b.szpos );                        // element ID
          PutResid( st, d+b.szpos, b.szl, data+(b.hend-b.szpos-b.szl) );     // size - header - frames
          if( laced && !b.regen ) st.insert( st.end(), d+b.lpos, d+b.hend );   // lace header / count
          cur = b.end; t.nlb++;
        }
        {
          EBlk e = { abs, nmark-1, 0, t.num, 0, false, false };
          if( kind!=2 ) e.relok = !opened.empty(), e.rel = (long long)( (kind==1 ? b.gpos : b.pos) - cdata );
          if( kind==1 )
            for( size_t c=0; c<ch.size(); c++ )
              if( ch[c].id==0x9B && ch[c].size<=8 && t.gmode.back()>=2 ) e.dur = ReadUintBE( d+ch[c].pos+ch[c].hdr, ch[c].size ), e.hasdur = true;
          ebk[std::make_pair( abs, t.num )].push_back( eb.size() ); eb.push_back( e );
        }
        t.lastabs = abs; t.haslast = true; t.bcts.push_back( cts ); t.nbe++; t.nfe += b.nf;
      } break;
      }
      if( verbose && mode==2 ) {                    // non-raw struct bytes of this item
        qword rw = 0;
        for( std::map<std::string,qword>::iterator it=stc.begin(); it!=stc.end(); ++it ) rw += it->second;
        qword add = st.size()-st0-(rw-rw0);
        if( add ) stc[ b.kind==K_CTS ? "cluster timestamps" : b.kind==K_POS ? "position headers" : "block lace/children/legacy" ] += add;
      }
    }
    if( verbose ) rawstat( cur, fs );
    st.insert( st.end(), d+cur, d+fs ); opc(OP_END);
    if( opened.size() ) bad = true;
  };
  run( 0 );
  // key prediction model per track: fewest mismatches with the SimpleBlock key bits
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; t.km = 0; t.kp = 0; t.kf.clear();
    if( !t.ok || t.blob || t.ksb.empty() ) continue;
    std::vector<const byte*> fp( size_t(t.nsamp) ); std::vector<qword> fz( size_t(t.nsamp) );
    for( size_t k=0; k<t.nsamp; k++ ) fp[k] = t.FB(d)+t.FO()[k], fz[k] = t.Size(k);
    int cand[4][2]; int nc = 0;
    cand[nc][0] = 1, cand[nc++][1] = 0;
    if( t.codec=="V_MPEG4/ISO/AVC" && !t.enc && t.lensize ) cand[nc][0] = 2, cand[nc++][1] = t.lensize;
    if( t.codec=="V_MPEGH/ISO/HEVC" && !t.enc && t.lensize ) cand[nc][0] = 3, cand[nc++][1] = t.lensize;
    if( t.fck>0 ) cand[nc][0] = 16+t.fck, cand[nc++][1] = int(t.fckp);
    size_t best = 0; for( size_t k=0; k<t.ksb.size(); k++ ) best += t.ksb[k].second;
    for( int c=0; c<nc; c++ ) {
      std::vector<byte> kf; KeyPred( cand[c][0], cand[c][1], fp.data(), fz.data(), t.nsamp, kf );
      size_t mis = 0;
      for( size_t k=0; k<t.ksb.size(); k++ ) mis += t.ksb[k].first<kf.size() && kf[size_t(t.ksb[k].first)]!=t.ksb[k].second;
      if( mis<best ) best = mis, t.km = cand[c][0], t.kp = cand[c][1], t.kf.swap( kf );
    }
  }
  if( bad ) { *why = "layout"; return false; }
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    if( t.ok && !t.blob && t.nbe!=t.btc.size() ) { *why = "block skipped"; return false; }
    t.dkind = t.kc[1]>t.kc[0] && t.kc[1]>=t.kc[2] ? 1 : t.kc[2]>t.kc[0] && t.kc[2]>t.kc[1] ? 2 : 0;
  }
  // streams: frames in block order, converted to an elementary stream format if possible;
  // timestamp models (they need the cluster timestamps from the layout pass)
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    if( !t.ok || t.blob ) continue;
    Buf& s = t.stream; Buf x; s.reserve( size_t(t.bytes) );
    if( t.strip.size() ) s.swap( t.sbuf ); else for( size_t j=0; j<t.nsamp; j++ ) s.insert( s.end(), d+t.foff[j], d+t.foff[j]+t.fsz[j] );
    if( t.xform!=XF_NONE ) {
      if( t.xform==XF_ANNEXB ) ToAnnexB( t, s, x );
      else if( t.xform==XF_ADTS ) ToADTS( t, s, x );
      else if( t.xform==XF_OGG ) {               // DiscardPadding of the last block -> final granulepos
        qword trim = t.disc>0 && t.disc<(1LL<<40) ? ( qword(t.disc)*t.vi.rate + 500000000 ) / 1000000000 : 0;
        ToOgg( t, s, x, uint(t.idx+1), trim );
      }
      else { x = t.xhdr; x.insert( x.end(), s.begin(), s.end() ); }
      qword rawb=0; for( size_t k=0; k<t.xa.size(); k++ ) rawb += t.xa[k].second;
      if( t.xform==XF_ANNEXB && 2*rawb>s.size() ) t.xform = XF_NONE;   // mostly not NALs: keep raw
      else t.conv = true;
    }
    // codec positions for the timestamp model (same inputs as the decoder will have)
    int ckind = 0, cparam = 0;
    if( t.codec=="V_MPEG4/ISO/AVC" && !t.enc && t.strip.empty() && t.lensize ) ckind = 1, cparam = t.lensize;
    else if( t.codec=="V_MPEGH/ISO/HEVC" && !t.enc && t.strip.empty() && t.lensize ) ckind = 3, cparam = t.lensize;
    else if( t.xform==XF_OGG ) ckind = 2;
    if( ckind ) {
      std::vector<qword> fo( size_t(t.nsamp+1), 0 );
      for( size_t j=0; j<t.nsamp; j++ ) fo[j+1] = fo[j]+t.Size(j);
      Buf hdr, pv;
      if( t.xform==XF_ANNEXB ) hdr = t.xhdr;
      if( ckind==2 ) { XiphLace( t.vl, 3, pv ); for( int h=0; h<3; h++ ) pv.insert( pv.end(), t.vh[h], t.vh[h]+t.vl[h] ); }
      if( !CodecPositions( ckind, cparam, hdr, pv, s.data(), fo, t.nsamp, t.cpos ) ) ckind = 0, t.cpos.clear();
    }
    ChooseTs( t, ckind, cparam, P.tcs );
    if( t.conv ) s.swap( x ), nxf++;
  }
  ChooseKeys( T, B );
  run( 1 );
  usepred = phit*10>=pall*9;                       // muxers interleaving by timestamp (mkvmerge)
  run( 2 );
  if( bad ) { *why = "layout"; return false; }

  Writer w; PutMetaHdr( w, FMT_MKV );
  w.vint(nt);
  streams.clear(); names.clear();
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; if( !t.ok ) continue;
    if( t.blob ) {
      streams.push_back( Buf( d+t.foff[0], d+t.foff[0]+t.bytes ) );
      TrackHead h; h.ext = t.name; h.xform = XF_BLOB; PutHead( w, h );
      names.push_back( TrackSuffix( names.size(), h.ext ) );
      if( verbose ) { char l[160]; snprintf( l, sizeof(l), "attachment %-24s %10llu bytes -> %s\n", t.codec.c_str(), (unsigned long long)t.bytes, names.back().c_str() ); log += l; }
      continue;
    }
    streams.push_back( Buf() ); streams.back().swap( t.stream );
    if( t.ext=="mjpeg" && t.nsamp==1 ) t.ext = "jpg";      // e.g. ffmpeg's attached_pic track
    if( t.szmode==3 && t.xform!=XF_ADTS && t.xform!=XF_OGG ) t.szmode = 0;
    TrackHead h;
    h.ext = t.xform!=XF_NONE ? t.xext : t.ext;
    h.xform = t.xform; h.nhdr = t.xform==XF_OGG ? 3 : 0; h.lensize = t.xform==XF_ANNEXB ? t.lensize : 0;
    if( t.xform==XF_ANNEXB || t.xform==XF_HDR ) h.hdrlen = t.xhdr.size();
    if( t.xform==XF_ANNEXB || t.xform==XF_ADTS ) h.xa = t.xa;
    if( t.xform==XF_ANNEXB ) h.xb = t.xb;
    h.nf = t.nsamp; h.szmode = t.szmode;
    if( t.szmode==0 ) h.col.assign( t.FZ().begin(), t.FZ().begin()+size_t(t.nsamp) );
    if( t.szmode==2 ) h.ptype = t.ptype, h.pparam = t.pparam, h.col.assign( t.ucnt.begin(), t.ucnt.end() ), h.esc = t.esc;
    PutHead( w, h );
    std::string suf = TrackSuffix( names.size(), h.ext ); names.push_back( suf );
    w.u8( uint(t.tvb.size()) | (t.strip.size() ? 0x80 : 0) ); w.bytes( t.tvb.data(), t.tvb.size() );
    if( t.strip.size() ) w.u8( uint(t.strip.size()) ), w.bytes( t.strip.data(), t.strip.size() );
    w.vint( t.btc.size() ); w.u8( uint( t.dkind | t.kmode<<2 ) ); w.vint( ZigZag( t.kbias ) ); w.vint( t.seg );
    w.u8( uint(t.km) ); if( t.km>=2 ) w.u8( uint(t.kp) );
    WriteTs( t, w );
    std::vector<qword> v( t.bflx.begin(), t.bflx.end() ); PutCol( w, v );
    v.assign( t.breg.begin(), t.breg.end() ); PutCol( w, v );
    w.vint( t.lcnt.size() ); PutCol( w, t.lcnt );
    w.vint( t.gmode.size() ); PutCol( w, t.gmode );
    if( verbose ) {
      char l[256];
      snprintf( l, sizeof(l), "track %d: %-16s %8llu frames %7llu blocks %10llu bytes  sizes=%s",
        t.idx, t.codec.c_str(), (unsigned long long)t.nsamp, (unsigned long long)t.nblk,
        (unsigned long long)t.bytes, t.szmode==3 ? (t.xform==XF_OGG ? "ogg" : "adts") : t.szmode==2 ? ptname[t.ptype] : "list" );
      log += l;
      if( t.szmode==2 ) snprintf( l, sizeof(l), " esc=%llu", (unsigned long long)t.esc.size() ), log += l;
      if( t.breg.size() ) {
        qword nr=0; for( size_t j=0; j<t.breg.size(); j++ ) nr += t.breg[j];
        snprintf( l, sizeof(l), " laced=%llu/%llu", (unsigned long long)nr, (unsigned long long)t.breg.size() ), log += l;
      }
      snprintf( l, sizeof(l), " %s blocks=%d+%d+%d(sb+gb+raw)", t.tsinfo.c_str(), t.nsb, t.ngb, t.nlb ), log += l;
      if( t.xform==XF_ANNEXB ) snprintf( l, sizeof(l), "  -> annexb hdr=%llu raw=%llu expl=%llu",
        (unsigned long long)t.xhdr.size(), (unsigned long long)t.xa.size(), (unsigned long long)t.xb.size() ), log += l;
      if( t.xform==XF_ADTS ) snprintf( l, sizeof(l), "  -> adts esc=%llu", (unsigned long long)t.xa.size() ), log += l;
      if( t.xform==XF_HDR ) snprintf( l, sizeof(l), "  -> hdr=%llu", (unsigned long long)t.xhdr.size() ), log += l;
      if( t.xform==XF_OGG ) snprintf( l, sizeof(l), "  -> ogg hdr=%llu+%llu+%llu bs=%u/%u modes=%d%s",
        (unsigned long long)t.vl[0], (unsigned long long)t.vl[1], (unsigned long long)t.vl[2], t.vi.bs[0], t.vi.bs[1], t.vi.nmodes,
        t.privcut ? " codecprivate=cut" : "" ), log += l;
      log += "\n";
    }
  }
  if( verbose ) {
    char l[200];
    snprintf( l, sizeof(l), "layout: %d clusters, %d marks, %d positions, %d cluster timestamps; block order predicted %llu/%llu%s; struct %llu, ops %llu\n",
      nops[0], nops[1], nops[2], nops[3], (unsigned long long)phit, (unsigned long long)pall, usepred ? "" : " (unused)",
      (unsigned long long)st.size(), (unsigned long long)op.b.size() );
    log += l; log += "struct:";
    for( std::map<std::string,qword>::iterator it=stc.begin(); it!=stc.end(); ++it )
      snprintf( l, sizeof(l), " %s %llu", it->first.c_str(), (unsigned long long)it->second ), log += l;
    log += "\n";
  }
  w.bytes( op.b.data(), op.b.size() ); w.bytes( st.data(), st.size() );
  meta.swap( w.b );

  // verify: run the decoder on the actual output
  Meta M; Sink chk( 0, &in ); *why = "meta parse";
  return ParseMeta(meta,M) && Decode(M,streams,chk,why);
}

static bool Encode( const Buf& in, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names ) {
  // fallbacks: without stream conversion, without the v4 layout ops, without both
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

} // namespace mkv
