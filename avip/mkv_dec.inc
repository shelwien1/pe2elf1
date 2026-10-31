// MKV: restore
namespace mkv {

static bool Restore( Meta& M, std::vector<const Buf*> S, Sink& out, const char** why ) {
  #define FAIL(s) { *why = s; return false; }
  const byte* st = M.st;
  // frame sizes -> offsets in the raw track streams; codec positions
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i]; const Buf& s = *S[i];
    if( t.xform==XF_BLOB ) continue;
    if( t.szmode==3 ) {
      if( t.xsz.size()!=t.nsamp ) FAIL("transform size mismatch");
      t.sz = t.xsz;
    } else if( t.szmode==2 ) {
      t.sz.resize( size_t(t.nsamp) );
      qword pos=0, ke=0;
      for( qword k=0; k<t.nsamp; k++ ) {
        uint v = t.ucnt[size_t(k)]; qword z;
        if( v==0 ) {
          if( ke>=t.esc.size() ) FAIL("escape underflow");
          z = t.esc[size_t(ke++)];
        } else if( v==1 ) z = 0;
        else {
          z = UnitsLen( t.ptype, t.pparam, s.data()+pos, s.size()-pos, v-1 );
          if( z==0 ) FAIL("unit parse failed");
        }
        if( z>0xFFFFFFFFu || z>s.size()-pos ) FAIL("stream too short");
        t.sz[size_t(k)] = uint(z); pos+=z;
      }
    }
    t.fo.resize( size_t(t.nsamp+1) ); t.fo[0] = 0;
    for( size_t k=0; k<t.nsamp; k++ ) t.fo[k+1] = t.fo[k]+t.sz[k];
    if( t.fo[size_t(t.nsamp)]!=s.size() ) FAIL("stream size mismatch");
    if( t.tsmode && t.ckind && !CodecPositions( t.ckind, t.cparam, t.xhdr, t.priv, s.data(), t.fo, t.nsamp, t.cpos ) )
      FAIL("bad timestamp model");
    t.kf.clear();
    if( t.km ) {
      std::vector<const byte*> fp( size_t(t.nsamp) ); std::vector<qword> fz( size_t(t.nsamp) );
      for( size_t k=0; k<t.nsamp; k++ ) fp[k] = s.data()+t.fo[k], fz[k] = t.sz[k];
      KeyPred( t.km, t.kp, fp.data(), fz.data(), t.nsamp, t.kf );
    }
    if( t.strip.size() ) {                    // header stripping: blocks hold the frames without the prefix
      qword L = t.strip.size(), so = 0;
      t.sbuf.clear(); t.sbuf.reserve( size_t( s.size()-std::min( qword(s.size()), L*t.nsamp ) ) );
      for( size_t k=0; k<t.nsamp; k++ ) {
        if( t.sz[k]<L || memcmp( s.data()+so, t.strip.data(), size_t(L) ) ) FAIL("strip prefix mismatch");
        t.sbuf.insert( t.sbuf.end(), s.data()+so+L, s.data()+so+t.sz[k] );
        so += t.sz[k]; t.sz[k] -= uint(L); t.fo[k+1] = t.fo[k]+t.sz[k];
      }
      S[i] = &t.sbuf;
    }
  }
  size_t nt = M.trk.size();
  std::vector<qword> fi( nt ), bi( nt ), li( nt ), gi( nt ), ci( nt );
  std::vector<qword> marks, segs, csize; std::vector<uint> markid;
  struct CueBlk { long long abs, clmark, rel; qword tnum, dur; bool relok, hasdur; };
  std::vector<CueBlk> eb;
  Buf hb, cb;
  for( int pass=0; pass<2; pass++ ) {
    qword pos = 0, sp = 0, cts = 0, cstart = 0;
    size_t nseg = 0, ncl = 0, cseg = 0; long long mi = 0, nmk = 0, lastcue = -1; bool open = false;
    eb.clear();
    std::fill( fi.begin(), fi.end(), 0 ); std::fill( bi.begin(), bi.end(), 0 );
    std::fill( li.begin(), li.end(), 0 ); std::fill( gi.begin(), gi.end(), 0 ); std::fill( ci.begin(), ci.end(), 0 );
    for( size_t i=0; i<nt; i++ ) M.trk[i].r16 = 0, M.trk[i].haslast = false, M.trk[i].lastabs = 0;
    byte mb[4]; int nmb = 0; bool pend = false;       // pass 0: ID of the element at the latest mark
    // pass 1: output is held back while CRC-32 elements are pending (value position, covered end;
    // nested, innermost last), then patched
    struct Crc { qword at, end; }; std::vector<Crc> crcs; Buf hold; qword hbase = 0;
    auto out1 = [&]( const byte* p, qword n, qword at ) {
      while( n ) {
        if( crcs.empty() ) { out.put( p, n ); return; }
        qword e = crcs.back().end, k = std::min( n, e-at );
        hold.insert( hold.end(), p, p+k ); p += k; n -= k; at += k;
        while( crcs.size() && crcs.back().end==at ) {
          qword c = crcs.back().at-hbase; crcs.pop_back();
          PutLE32( &hold[size_t(c)], Crc32( &hold[size_t(c+4)], hold.size()-c-4 ) );
        }
        if( crcs.empty() ) out.put( hold.data(), hold.size() ), hold.clear();
      }
    };
    auto emit = [&]( const byte* p, qword n ) {
      if( pass ) out1( p, n, pos );
      else if( pend )
        for( qword j=0; j<n && pend; j++ ) {
          mb[nmb++] = p[j];
          uint id; int il = ReadID( mb, nmb, id );
          if( il ) markid.back() = id, pend = false;
          else if( nmb==4 || mb[0]<0x10 ) pend = false;
        }
      pos += n;
    };
    auto markhere = [&]() { if( !pass ) marks.push_back( pos ), markid.push_back( 0 ), pend = true, nmb = 0; nmk++; };
    auto vint = [&]( qword v ) { byte b[8]; int l = VintLen(v); PutVint( b, l, v ); emit( b, l ); };
    // timestamp of the next block of track ti, given the current cluster timestamp
    auto nextts = [&]( size_t ti ) -> uint {
      DTrack& t = M.trk[ti];
      uint r = (t.r16+t.te[size_t(bi[ti])]) & 0xFFFF;
      if( !t.tsmode ) return r;
      qword x = t.ckind ? (fi[ti]<t.cpos.size() ? t.cpos[size_t(fi[ti])] : 0) : fi[ti];
      return uint( ModelAt( t, size_t(bi[ti]), x ) - cts + r ) & 0xFFFF;
    };
    // model time of the next block (tracks with a timestamp model)
    auto ptskey = [&]( size_t ti ) -> long long {
      const DTrack& t = M.trk[ti];
      qword x = t.ckind ? (fi[ti]<t.cpos.size() ? t.cpos[size_t(fi[ti])] : 0) : fi[ti];
      return (long long)ModelAt( t, size_t(bi[ti]), x ) + int16_t(uint16_t( (t.r16+t.te[size_t(bi[ti])]) & 0xFFFF ));
    };
    // track of the next block: smallest interleave key (modelonly: tracks with a model)
    auto predict = [&]( bool modelonly ) -> size_t {
      size_t bt = nt; long long bk = 0;
      for( size_t ti=0; ti<nt; ti++ ) {
        const DTrack& t = M.trk[ti];
        if( t.xform==XF_BLOB || bi[ti]>=t.bfl.size() || (modelonly && !t.tsmode) || t.seg!=(nseg ? cseg : 0) ) continue;
        long long key = !t.tsmode ? (long long)cts + int16_t(uint16_t(nextts(ti))) :
                        t.kmode ? (long long)ModelAt( t, size_t(bi[ti]), fi[ti] ) : ptskey( ti );
        key += t.kbias;
        if( bt==nt || key<bk ) bt = ti, bk = key;
      }
      return bt;
    };
    // block ops: OP_BLOCK (sizes in struct), OP_SB, OP_GB
    auto block = [&]( int code, size_t ti ) -> bool {
      DTrack& t = M.trk[ti];
      const byte* q = st+sp; qword av = M.stlen-sp, r = 0, o = 0; uint id = 0; int il = 0, l = 0;
      if( code==OP_BLOCK ) {
        il = ReadID( q, av, id );
        if( !il ) FAIL("bad block id");
        l = ReadVint( q+il, av-il, r );
        if( !l ) FAIL("bad size vint");
        o = il+l;
      }
      size_t k = size_t(bi[ti]);
      if( k>=t.bfl.size() ) FAIL("block columns underflow");
      uint fl = t.bfl[k], tc = nextts( ti );
      if( code==OP_SB && t.km && fi[ti]<t.kf.size() && t.kf[size_t(fi[ti])] ) fl ^= 0x80;
      bi[ti]++; t.r16 = (t.r16+t.te[k]) & 0xFFFF;
      long long abs = (long long)cts + int16_t(uint16_t(tc)), pred = t.haslast ? t.lastabs-abs : 0;
      t.lastabs = abs; t.haslast = true;
      int lacing = (fl>>1)&3, nf = 1; qword lo = o;
      bool regen = false;
      if( lacing ) {
        if( li[ti]>=t.breg.size() ) FAIL("lace columns underflow");
        regen = t.breg[size_t(li[ti]++)]!=0;
        int n0;
        if( regen && M.ver>=4 ) {                   // frame count from the column
          if( ci[ti]>=t.lcnt.size() ) FAIL("lace columns underflow");
          n0 = int(t.lcnt[size_t(ci[ti]++)])+1;
        } else {
          if( o>=av ) FAIL("struct overrun");
          n0 = q[o]+1;
        }
        if( M.ver>=4 ) {                            // lacing type relative to the predicted one
          if( qword(n0)>t.nsamp-fi[ti] ) FAIL("frame overrun");
          int p = LacePred( &t.sz[size_t(fi[ti])], n0 );
          lacing = (lacing-1+p-1)%3+1; fl = (fl&~6u) | uint(lacing<<1);
        }
        if( regen ) { nf = n0; if( M.ver<4 ) o++; }
        else if( !ParseLace( q, av, lacing, o, nf ) ) FAIL("bad lace header");
      }
      hb.assign( t.tvb.begin(), t.tvb.end() );
      hb.push_back( byte(tc>>8) ); hb.push_back( byte(tc) ); hb.push_back( byte(fl) );
      if( qword(nf)>t.nsamp-fi[ti] ) FAIL("frame overrun");
      qword a = t.fo[size_t(fi[ti])], data = t.fo[size_t(fi[ti]+nf)]-a;
      if( regen ) {
        if( lacing==2 ) hb.push_back( byte(nf-1) );
        else MakeLace( lacing, &t.sz[size_t(fi[ti])], nf, hb );
      } else hb.insert( hb.end(), q+lo, q+o );
      qword bs = hb.size()+data;
      CueBlk ce = { abs, nmk-1, 0, t.tnum, 0, false, false };
      if( code!=OP_BLOCK ) ce.relok = open, ce.rel = (long long)(pos-cstart);
      if( code==OP_BLOCK ) {
        byte vb[8]; PutVint( vb, l, (r+bs)&VintMask(l) );
        emit( q, il ); emit( vb, l );
      } else if( code==OP_SB ) {
        static const byte sb = 0xA3;
        emit( &sb, 1 ); vint( bs );
      } else {
        if( gi[ti]>=t.gmode.size() ) FAIL("group columns underflow");
        qword gm = t.gmode[size_t(gi[ti]++)];
        cb.clear();
        if( gm==1 ) {                                 // predicted ReferenceBlock
          int n = MinSLen( pred );
          cb.push_back( 0xFB ); cb.push_back( byte(0x80|n) );
          for( int j=n-1; j>=0; j-- ) cb.push_back( byte( qword(pred)>>(8*j) ) );
        } else if( gm>=2 ) {
          for( qword c=0; c<gm-2; c++ ) {
            uint cid; qword cs; bool unk;
            int cil = ReadID( q+o, av-o, cid ), csl = cil ? ReadVint( q+o+cil, av-o-cil, cs, &unk ) : 0;
            if( !csl || unk || cs>av-o-cil-csl ) FAIL("bad group child");
            size_t at = cb.size()+cil+csl;
            cb.insert( cb.end(), q+o, q+o+cil+csl+cs );
            if( cid==0xFB && cs>=1 && cs<=8 ) {       // ReferenceBlock: stored minus the prediction
              qword v = ReadUintBE( &cb[at], cs ) + qword(pred);
              for( qword j=0; j<cs; j++ ) cb[at+size_t(j)] = byte( v>>(8*(cs-1-j)) );
            }
            o += cil+csl+cs;
          }
        }
        static const byte gb = 0xA0, bb = 0xA1;
        emit( &gb, 1 ); vint( 1+VintLen(bs)+bs+cb.size() ); emit( &bb, 1 ); vint( bs );
        if( gm>=2 )                                   // BlockDuration, for cues
          for( size_t p=0; p<cb.size(); ) {
            uint cid; qword cs; int cil = ReadID( &cb[p], cb.size()-p, cid ), csl = cil ? ReadVint( &cb[p+cil], cb.size()-p-cil, cs ) : 0;
            if( !csl ) break;
            if( cid==0x9B && cs<=8 ) ce.dur = ReadUintBE( &cb[p+cil+csl], cs ), ce.hasdur = true;
            p += cil+csl+size_t(cs);
          }
      }
      eb.push_back( ce );
      emit( hb.data(), hb.size() ); emit( S[ti]->data()+a, data );
      if( code==OP_GB ) emit( cb.data(), cb.size() );
      fi[ti] += nf;
      sp += o;
      return true;
    };
    for( size_t i=0; i<M.ops.size(); i++ ) {
      const DOp& d = M.ops[i];
      if( d.code==OP_RAW || d.code==OP_END ) {
        qword n = d.code==OP_RAW ? d.n : M.stlen-sp;
        if( n>M.stlen-sp ) FAIL("struct overrun");
        emit( st+sp, n ); sp+=n;
      } else if( d.code==OP_BLOB ) {
        size_t t = size_t(d.t);
        if( fi[t] ) FAIL("blob reused");
        emit( S[t]->data(), S[t]->size() ); fi[t] = 1;
      } else if( d.code==OP_PRIV ) {
        const Buf& p = M.trk[size_t(d.t)].priv;
        emit( p.data(), p.size() );
      } else if( d.code==OP_GSIZE ) {
        size_t ti = size_t(d.t); DTrack& t = M.trk[ti];
        qword r; int l = ReadVint( st+sp, M.stlen-sp, r );
        if( !l ) FAIL("bad size vint");
        if( d.n>t.nsamp-fi[ti] ) FAIL("frame overrun");
        qword data = t.fo[size_t(fi[ti]+d.n)]-t.fo[size_t(fi[ti])];
        byte vb[8]; PutVint( vb, l, (r+data)&VintMask(l) );
        emit( vb, l ); sp += l;
      } else if( d.code==OP_CUE ) {
        long long bix = lastcue+1+UnZigZag( d.n );
        if( bix<0 || qword(bix)>=eb.size() || !nseg || open ) FAIL("bad cue reference");
        const CueBlk& c = eb[size_t(bix)];
        if( c.abs<0 || c.clmark<0 || ((d.t&1) && !c.relok) || ((d.t&2) && !c.hasdur) ) FAIL("bad cue reference");
        Buf in, cp;
        PutUintElem( in, 0xF7, c.tnum ); PutUintElem( in, 0xF1, marks[size_t(c.clmark)]-segs[cseg] );
        if( d.t&1 ) PutUintElem( in, 0xF0, qword(c.rel) );
        if( d.t&2 ) PutUintElem( in, 0xB2, c.dur );
        PutUintElem( cp, 0xB3, qword(c.abs) );
        byte vb[8]; int vl = VintLen( in.size() ); PutVint( vb, vl, in.size() );
        cp.push_back( 0xB7 ); cp.insert( cp.end(), vb, vb+vl ); cp.insert( cp.end(), in.begin(), in.end() );
        static const byte cpid = 0xBB;
        emit( &cpid, 1 ); vint( cp.size() ); emit( cp.data(), cp.size() );
        lastcue = bix;
      } else if( d.code==OP_CLUSTER ) {
        if( open ) FAIL("nested cluster");
        markhere();
        emit( ID_CLUSTER_B, 4 );
        if( pass ) { if( ncl>=csize.size() ) FAIL("cluster count"); vint( csize[ncl] ); }
        cstart = pos; open = true;
      } else if( d.code==OP_CEND ) {
        if( !open ) FAIL("cluster end without start");
        if( !pass ) { qword z = pos-cstart; csize.push_back( z ); pos += VintLen(z); }
        open = false; ncl++;
      } else if( d.code==OP_CTS || d.code==OP_CTSP ) {
        size_t bt = predict( true );                // prediction: time of the next block
        qword pc = bt<nt ? qword( ptskey( bt ) ) : 0;
        if( d.code==OP_CTSP ) {
          int n = 1; while( n<8 && (pc>>(8*n)) ) n++;
          byte e[10] = { 0xE7, byte(0x80|n) };
          for( int j=0; j<n; j++ ) e[2+j] = byte( pc>>(8*(n-1-j)) );
          emit( e, 2+n ); cts = pc;
        } else {
          const byte* q = st+sp; qword av = M.stlen-sp, n; uint id; bool unk;
          int il = ReadID( q, av, id ), l = il ? ReadVint( q+il, av-il, n, &unk ) : 0;
          if( !l || id!=0xE7 || unk || n>8 || n>av-il-l ) FAIL("bad cluster timestamp");
          qword v = ReadUintBE( q+il+l, n );
          if( M.ver>=4 ) {
            v += pc; if( n<8 ) v &= (qword(1)<<(8*n))-1;
            byte e[8]; for( qword j=0; j<n; j++ ) e[j] = byte( v>>(8*(n-1-j)) );
            emit( q, il+l ); emit( e, n );
          } else emit( q, il+l+n );
          cts = v; sp += il+l+n;
        }
      } else if( d.code==OP_MARK ) {
        if( open ) FAIL("mark inside cluster");
        markhere();
      } else if( d.code==OP_VOID ) {
        byte vh[9] = { 0xEC }; PutVint( vh+1, int(d.t), d.n ); emit( vh, 1+d.t );
        static const byte z[4096] = {0};
        for( qword n=d.n; n; ) { qword k = std::min( n, qword(sizeof(z)) ); emit( z, k ); n -= k; }
      } else if( d.code==OP_CRC ) {
        if( pass ) {
          qword e = d.n ? pos+6+(d.n-1) : open && ncl<csize.size() ? cstart+csize[ncl] : 0;
          if( e<pos+6 || (crcs.size() && e>crcs.back().end) ) FAIL("bad crc range");
          if( crcs.empty() ) hbase = pos;
          Crc c = { pos+2, e }; crcs.push_back( c );
        } else if( !d.n && !open ) FAIL("bad crc range");
        static const byte ce[6] = { 0xBF, 0x84, 0, 0, 0, 0 };
        emit( ce, 6 );
      } else if( d.code==OP_SEEK ) {
        int il = int(d.t&3)+1, pl = int(d.t>>2)+1;
        if( open || !nseg ) FAIL("bad seek entry");
        mi += UnZigZag( d.n );
        byte e[32]; int n = 0;
        e[n++] = 0x4D; e[n++] = 0xBB; e[n++] = byte( 0x80|(6+il+pl) );      // inner size < 127
        e[n++] = 0x53; e[n++] = 0xAB; e[n++] = byte(0x80|il);
        uint id = 0; qword v = 0;
        if( pass ) {
          if( mi<0 || qword(mi)>=marks.size() ) FAIL("bad mark index");
          id = markid[size_t(mi)]; v = marks[size_t(mi)]-segs[cseg];
          if( (il<4 && (id>>(8*il))) || (pl<8 && (v>>(8*pl))) ) FAIL("seek entry overflow");
        }
        for( int j=il-1; j>=0; j-- ) e[n++] = byte(id>>(8*j));
        e[n++] = 0x53; e[n++] = 0xAC; e[n++] = byte(0x80|pl);
        for( int j=pl-1; j>=0; j-- ) e[n++] = byte(v>>(8*j));
        emit( e, n );
      } else if( d.code==OP_SEG ) {
        if( open ) FAIL("segment inside cluster");
        if( !pass ) segs.push_back( pos );
        cseg = nseg++;
      } else if( d.code==OP_POS ) {
        const byte* q = st+sp; qword av = M.stlen-sp, n; uint id; bool unk;
        int il = ReadID( q, av, id ), l = il ? ReadVint( q+il, av-il, n, &unk ) : 0;
        if( !l || unk || n<1 || n>8 || open || !nseg ) FAIL("bad position element");
        mi += UnZigZag( d.n );
        emit( q, il+l ); sp += il+l;
        byte vb[8] = {0};
        if( pass ) {
          if( mi<0 || qword(mi)>=marks.size() ) FAIL("bad mark index");
          qword v = marks[size_t(mi)]-segs[cseg];
          if( n<8 && (v>>(8*n)) ) FAIL("position overflow");
          for( qword k=0; k<n; k++ ) vb[k] = byte( v>>(8*(n-1-k)) );
        }
        emit( vb, n );
      } else if( d.code==OP_PRUN ) {                 // blocks of the tracks predicted by timestamp order
        for( qword j=0; j<d.n; j++ ) {
          size_t bt = predict( false );
          if( bt==nt ) FAIL("no block to predict");
          if( !block( M.trk[bt].dkind==0 ? OP_SB : M.trk[bt].dkind==1 ? OP_GB : OP_BLOCK, bt ) ) return false;
        }
      } else if( !block( d.code, size_t(d.t) ) ) return false;
    }
    if( open ) FAIL("unterminated cluster");
    if( crcs.size() ) FAIL("unterminated crc");
    if( sp!=M.stlen ) FAIL("struct not consumed");
    for( size_t i=0; i<nt; i++ )
      if( fi[i]!=(M.trk[i].xform==XF_BLOB ? 1 : M.trk[i].nsamp) || bi[i]!=M.trk[i].bfl.size() ||
          li[i]!=M.trk[i].breg.size() || gi[i]!=M.trk[i].gmode.size() || ci[i]!=M.trk[i].lcnt.size() )
        FAIL("track not consumed");

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
    if( t.xform==XF_NONE || t.xform==XF_BLOB ) continue;
    bool ok = t.xform==XF_ANNEXB ? UnAnnexB( t, S[i], R[i] ) : t.xform==XF_ADTS ? UnADTS( t, S[i], R[i] ) :
              t.xform==XF_OGG ? UnOgg( t, S[i], R[i] ) : UnHdr( t, S[i], R[i] );
    if( !ok ) { *why = "bad converted stream"; return false; }
    if( t.xform==XF_ANNEXB ) t.xhdr.assign( S[i].begin(), S[i].begin()+size_t(t.hdrlen) );
    P[i] = &R[i];
  }
  return Restore( M, P, out, why );
}

} // namespace mkv
