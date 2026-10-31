// MP4: restore
namespace mp4 {

static bool Restore( Meta& M, const std::vector<const Buf*>& S, Sink& out, const char** why ) {
  #define FAIL(s) { *why = s; return false; }
  const byte* st = M.st;
  // chunk model: regenerated samples-per-chunk of the stbl chunks
  size_t nt = M.trk.size();
  std::vector<std::vector<qword> > cspc( nt );
  for( int pm=0; pm<2; pm++ )                 // plain ones first: masters
    for( size_t i=0; i<nt; i++ ) {
      DTrack& t = M.trk[i];
      if( !(t.cflags&1) || t.cmode!=pm ) continue;
      if( pm==0 ) { cspc[i] = t.ccol; continue; }
      const DTrack& m = M.trk[size_t(t.cmaster)];
      if( !(m.cflags&1) || m.cmode!=0 ) FAIL("bad chunk master");
      std::vector<qword> dm, dj;
      if( !EarlyDts( m, *S[size_t(t.cmaster)], dm ) || !EarlyDts( t, *S[i], dj ) ) FAIL("bad chunk model");
      qword ntr = 0;                          // stbl samples = all - trun samples
      for( size_t k=0; k<t.trun.size(); k++ ) { TrunHdr h; ParseTrunHdr( st+t.trun[k], M.stlen-t.trun[k], h ); ntr += h.count; }
      if( ntr>t.nsamp ) FAIL("bad chunk model");
      std::vector<qword> B; qword a = 0;
      for( size_t c=0; c<cspc[size_t(t.cmaster)].size(); c++ ) {
        if( a>m.nsamp ) FAIL("bad chunk model");
        B.push_back( dm[size_t(a)] ); a += cspc[size_t(t.cmaster)][c];
      }
      std::vector<qword> cum;
      AlignedCum( dj, t.nsamp-ntr, t.csj, B, t.csm, t.ca, t.crule, t.cO, t.nch_stbl, cum );
      cspc[i].resize( size_t(t.nch_stbl) );
      for( qword c=0, prev=0; c<t.nch_stbl; c++ ) {
        qword v = cum[size_t(c)] + qword( UnZigZag( t.ccol[size_t(c)] ) );
        if( v<prev ) FAIL("bad chunk model");
        cspc[i][size_t(c)] = v-prev; prev = v;
      }
    }
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i]; const Buf& s = *S[i];
    if( t.xform==XF_BLOB ) {                 // one chunk = the whole stream
      t.npc=0; t.nconst=0; t.csize=0; t.sz.clear();
      t.spc.assign(1,0); t.cfirst.assign(1,0); t.csz.assign(1,s.size()); t.cso.assign(1,0); t.cpos.assign(1,0);
      continue;
    }
    // chunk map: stbl chunks, then one per trun, cut after nsamp samples.
    // nsamp < total = truncated file: chunks past the cut are absent (their bytes
    // are in the struct), the last present one may hold only its first samples.
    std::vector<uint> spc;
    qword nst=0;
    if( t.has_stbl ) {
      if( t.cflags&1 ) {
        for( size_t c=0; c<cspc[i].size(); c++ ) { if( cspc[i][c]>0xFFFFFFFFu ) FAIL("bad chunk model"); spc.push_back( uint(cspc[i][c]) ); }
      } else {
        if( t.stsc_ptr>M.stlen || t.stsc_len>M.stlen-t.stsc_ptr ) FAIL("bad stsc ptr");
        if( !ParseStsc( st+t.stsc_ptr, t.stsc_len, t.nch_stbl, spc ) ) FAIL("bad stsc");
      }
      for( size_t c=0; c<spc.size(); c++ ) nst+=spc[c];
    }
    for( size_t k=0; k<t.trun.size(); k++ ) {
      TrunHdr h; ParseTrunHdr( st+t.trun[k], M.stlen-t.trun[k], h );
      spc.push_back( h.count );
    }
    t.spc.clear();
    qword tot=0;
    for( size_t c=0; c<spc.size(); c++ ) tot += spc[c];
    if( t.nsamp>tot ) FAIL("sample count mismatch");
    if( t.nsamp==tot ) t.spc.assign( spc.begin(), spc.end() );
    else for( qword c=0, a=0; a<t.nsamp; c++ ) { qword m = std::min( qword(spc[size_t(c)]), t.nsamp-a ); t.spc.push_back(m); a+=m; }
    t.npc = std::min( qword(t.spc.size()), t.nch_stbl );
    qword ntp = t.spc.size()-t.npc;           // present truns
    qword nsp = 0;                            // present stbl samples
    for( size_t c=0; c<t.npc; c++ ) nsp += t.spc[c];
    // sample sizes
    t.csize=0; t.nconst=0; t.sz.clear();
    if( t.szmode==0 ) {
      if( t.has_stbl ) {
        SizeInfo si;
        if( t.sz_ptr>M.stlen || t.sz_len>M.stlen-t.sz_ptr ) FAIL("bad stsz ptr");
        if( !ParseSizes( st+t.sz_ptr, t.sz_len, t.stz2!=0, ~qword(0)>>1, si ) ) FAIL("bad stsz");
        if( si.count!=nst ) FAIL("stsz count mismatch");
        if( si.csize ) t.csize=si.csize, t.nconst=nsp; else si.v.resize( size_t(nsp) ), t.sz.swap( si.v );
      }
      for( size_t k=0; k<ntp; k++ ) {
        const byte* p = st+t.trun[k]; TrunHdr h; ParseTrunHdr( p, M.stlen-t.trun[k], h );
        if( h.flags&0x200 ) {
          if( qword(h.recsize)*h.count > M.stlen-t.trun[k]-h.hlen ) FAIL("trun overrun");
          for( qword j=0; j<t.spc[t.npc+k]; j++ ) t.sz.push_back( BE32(p+h.hlen+j*h.recsize+h.szoff) );
        } else {
          if( t.tdef[k]>0xFFFFFFFFu ) FAIL("bad default size");
          t.sz.insert( t.sz.end(), size_t(t.spc[t.npc+k]), uint(t.tdef[k]) );
        }
      }
    } else if( t.szmode==3 ) {
      if( t.xsz.size()!=t.nsamp ) FAIL("transform size mismatch");
      t.sz = t.xsz;
    } else {
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
    // chunk sizes and stream offsets
    size_t nc = t.spc.size();
    t.cfirst.resize(nc); t.csz.resize(nc); t.cso.resize(nc); t.cpos.assign(nc,0);
    qword si=0, so=0;
    for( size_t c=0; c<nc; c++ ) {
      qword n=t.spc[c], b=0;
      t.cfirst[c]=si; t.cso[c]=so;
      if( si+n<=t.nconst ) b = n*t.csize; else for( qword k=0; k<n; k++ ) b += t.Size(si+k);
      t.csz[c]=b; si+=n; so+=b;
    }
    if( so!=s.size() ) FAIL("stream size mismatch");
    if( t.tflags&1 ) {                        // timing columns
      std::vector<qword> fo( size_t(t.nsamp+1), 0 );
      for( qword k=0; k<t.nsamp; k++ ) fo[size_t(k+1)] = fo[size_t(k)]+t.Size(k);
      TimeIn ti; Buf none;
      if( !TimeInputs( t.tm.ckind, t.tm.lensize, t.xform==XF_ANNEXB ? t.xhdr : none, t.priv, s.data(), fo, t.nsamp, ti, t.tm.fck, t.tm.fckp ) ) FAIL("bad timing model");
      TimeApply( t.tm, ti, t.nsamp, t.dur, t.dts, t.cto, t.sync );
      if( t.tm.hassdtp ) SdtpApply( t.tm, ti, nsp, t.sdtp );
    }
  }
  // pass 0: layout and checks, pass 1: emit
  std::vector<qword> next( M.trk.size() ), ntrun( M.trk.size() ), marks;
  Buf tb;
  for( int pass=0; pass<2; pass++ ) {
    qword pos=0, sp=0;
    std::fill( next.begin(), next.end(), 0 );
    std::fill( ntrun.begin(), ntrun.end(), 0 );
    for( size_t i=0; i<M.trk.size(); i++ ) M.trk[i].pcur = 0;
    for( size_t i=0; i<M.ops.size(); i++ ) {
      const DOp& d = M.ops[i];
      if( d.code==OP_RAW || d.code==OP_END ) {
        qword n = d.code==OP_RAW ? d.n : M.stlen-sp;
        if( n>M.stlen-sp ) FAIL("struct overrun");
        if( pass ) out.put( st+sp, n );
        pos+=n; sp+=n;
      } else if( d.code==OP_TABLE && d.n==TB_BDO ) {
        if( 8>M.stlen-sp ) FAIL("struct overrun");
        if( pass ) { byte b[8]; PutBE64( b, pos-BE64(st+sp) ); out.put( b, 8 ); }
        pos+=8; sp+=8;
      } else if( d.code==OP_TABLE && d.n==TB_MARK ) {
        if( !pass ) { if( marks.size() && pos<marks.back() ) FAIL("bad mark"); marks.push_back( pos ); }
      } else if( d.code==OP_TABLE && d.n==TB_MFRO ) {
        if( pass ) {
          size_t j = size_t( std::upper_bound( marks.begin(), marks.end(), pos ) - marks.begin() );
          if( !j ) FAIL("bad mfro");
          byte b[4]; PutBE32( b, uint( pos+4-marks[j-1] ) ); out.put( b, 4 );
        }
        pos+=4;
      } else if( d.code==OP_TABLE && (d.n==TB_SIDX || d.n==TB_TFRA) ) {
        Reader rr( st+sp, size_t(M.stlen-sp) );
        uint ver = 0, lc = 0;
        if( d.n==TB_TFRA ) { ver = rr.u8(); lc = rr.u8(); if( ver>1 || lc>63 ) FAIL("bad tfra"); }
        qword cnt = rr.vint(), j0 = d.n==TB_SIDX ? rr.vint() : 0, tr = rr.vint();
        if( rr.err || cnt>MAXCOUNT || tr>M.trk.size() ) FAIL("bad fragment index");
        int ncol = d.n==TB_SIDX ? 3 : 5;
        std::vector<qword> col[5];
        for( int c=0; c<ncol; c++ ) if( !GetColN( rr, col[c], cnt ) ) FAIL("bad fragment index");
        qword esz = d.n==TB_SIDX ? 12 : (ver ? 16 : 8) + ((lc&3)+1) + (((lc>>2)&3)+1) + (((lc>>4)&3)+1);
        qword n = esz*cnt;
        if( pass ) {
          const DTrack* t = tr ? &M.trk[size_t(tr-1)] : 0;
          bool tim = t && (t->tflags&1) && t->xform!=XF_BLOB;
          static const std::vector<qword> none;
          tb.clear();
          if( d.n==TB_SIDX ) {
            std::vector<qword> psz, pdur;
            SidxPredict( marks, j0, cnt, t ? t->cpos : none, t ? t->cfirst : none, t ? t->spc : none, tim ? t->dur.data() : 0, psz, pdur );
            uint sap = 0;
            for( qword k=0; k<cnt; k++ ) {
              byte e[12];
              PutBE32( e, uint( psz[size_t(k)] + qword(UnZigZag( col[0][size_t(k)] )) ) );
              PutBE32( e+4, uint( pdur[size_t(k)] + qword(UnZigZag( col[1][size_t(k)] )) ) );
              sap ^= uint(col[2][size_t(k)]); PutBE32( e+8, sap );
              tb.insert( tb.end(), e, e+12 );
            }
          } else {
            std::vector<qword> poff, ptime;
            TfraPredict( marks, cnt, t ? size_t(t->npc) : 0, t ? t->cpos : none, t ? t->cfirst : none, tim ? t->dts.data() : 0,
                         tim && t->tm.hascto ? t->cto.data() : 0, t && (t->tflags&8) ? t->tbase : 0, poff, ptime );
            for( qword k=0; k<cnt; k++ ) {
              byte e[32]; size_t o = 0;
              qword tv = ptime[size_t(k)] + qword(UnZigZag( col[0][size_t(k)] )), ov = poff[size_t(k)] + qword(UnZigZag( col[1][size_t(k)] ));
              if( ver ) PutBE64( e, tv ), PutBE64( e+8, ov ), o = 16; else PutBE32( e, uint(tv) ), PutBE32( e+4, uint(ov) ), o = 8;
              for( int q=0; q<3; q++ ) {
                int L = int((lc>>(2*q))&3)+1; qword v = col[2+q][size_t(k)];
                for( int b=L-1; b>=0; b-- ) e[o++] = byte(v>>(8*b));
              }
              tb.insert( tb.end(), e, e+o );
            }
          }
          out.put( tb.data(), n );
        }
        pos+=n; sp+=qword(rr.p-(st+sp));
      } else if( d.code==OP_TABLE ) {
        DTrack& t = M.trk[d.t];
        qword n, ns=0;
        if( d.n==TB_DSI ) {
          if( (t.xform!=XF_OGG && t.xform!=XF_HDR) || t.priv.empty() ) FAIL("bad table");
          n = t.priv.size();
          if( pass ) tb = t.priv;
        } else if( d.n==TB_DOFF ) {
          if( 4>M.stlen-sp || sp<8 ) FAIL("struct overrun");
          size_t k = std::lower_bound( t.trun.begin(), t.trun.end(), sp-8 ) - t.trun.begin();
          if( k>=t.trun.size() || t.trun[k]!=sp-8 || t.npc+k>=t.spc.size() ) FAIL("bad data_offset ref");
          n = ns = 4;
          if( pass ) { tb.resize(4); PutBE32( &tb[0], uint(t.cpos[size_t(t.npc+k)]) - uint(pos) + BE32(st+sp) ); }
        } else if( d.n==TB_STCO || d.n==TB_CO64 ) {
          if( !t.has_stbl ) FAIL("bad table");
          n = (d.n==TB_STCO ? 4 : 8)*t.npc;
          if( pass ) {
            tb.resize( size_t(n) );
            for( size_t c=0; c<t.npc; c++ )
              if( d.n==TB_CO64 ) PutBE64( &tb[8*c], t.cpos[c] );
              else { if( t.cpos[c]>>32 ) FAIL("stco overflow"); PutBE32( &tb[4*c], uint(t.cpos[c]) ); }
          }
        } else if( d.n==TB_STSZ ) {
          if( !t.has_stbl || t.szmode<2 ) FAIL("bad table");
          qword nst = t.npc ? t.cfirst[size_t(t.npc-1)]+t.spc[size_t(t.npc-1)] : 0;
          n = 4*nst;
          if( pass ) {
            tb.resize( size_t(n) );
            for( size_t k=0; k<nst; k++ ) PutBE32( &tb[4*k], t.Size(k) );
          }
        } else if( d.n==TB_STTS || d.n==TB_CTTS || d.n==TB_STSS ) {
          if( !(t.tflags&1) || !t.has_stbl || sp<4 || (d.n==TB_CTTS && !t.tm.hascto) || (d.n==TB_STSS && !t.tm.hassync) ) FAIL("bad table");
          qword cnt = BE32(st+sp-4), nst = t.npc ? t.cfirst[size_t(t.npc-1)]+t.spc[size_t(t.npc-1)] : 0;
          n = (d.n==TB_STSS ? 4 : 8)*cnt;
          if( pass ) {
            tb.clear(); byte e[8]; qword ne = 0;
            if( d.n==TB_STSS ) {
              for( qword k=0; k<nst; k++ ) if( t.sync[size_t(k)] ) { if( ++ne>cnt ) break; PutBE32( e, uint(k+1) ); tb.insert( tb.end(), e, e+4 ); }
            } else
              for( qword k=0, j; k<nst && ne<=cnt; k=j ) {
                qword v = d.n==TB_STTS ? t.dur[size_t(k)] : qword(t.cto[size_t(k)]);
                for( j=k+1; j<nst && (d.n==TB_STTS ? t.dur[size_t(j)] : qword(t.cto[size_t(j)]))==v; j++ );
                PutBE32( e, uint(j-k) ); PutBE32( e+4, uint(v) ); tb.insert( tb.end(), e, e+8 ); ne++;
              }
            if( ne!=cnt ) FAIL("table entry count mismatch");
          }
        } else if( d.n==TB_PSET ) {
          if( t.xform!=XF_ANNEXB || sp<2 ) FAIL("bad table");
          n = BE16(st+sp-2);
          const byte* h = t.xhdr.data(); qword hl = t.xhdr.size(), c = t.pcur;
          if( hl-c<4 || h[c] || h[c+1] || h[c+2] || h[c+3]!=1 || n>hl-c-4 ) FAIL("bad parameter set ref");
          if( pass ) tb.assign( h+c+4, h+c+4+n );
          t.pcur = c+4+n;
        } else if( d.n==TB_SDTP ) {
          if( !(t.tflags&1) || !t.tm.hassdtp || !t.has_stbl ) FAIL("bad table");
          n = t.npc ? t.cfirst[size_t(t.npc-1)]+t.spc[size_t(t.npc-1)] : 0;
          if( pass ) tb = t.sdtp;
        } else if( d.n==TB_STSC ) {
          if( !(t.cflags&1) || sp<4 ) FAIL("bad table");
          qword cnt = BE32(st+sp-4);
          n = 12*cnt;
          if( pass ) {
            tb.clear(); byte e[12]; qword ne = 0; const std::vector<qword>& v = cspc[size_t(d.t)];
            for( size_t c=0, j; c<v.size() && ne<=cnt; c=j ) {
              for( j=c+1; j<v.size() && v[j]==v[c] && !((t.cflags&4) && j+1==v.size()); j++ );
              PutBE32( e, uint(c+1) ); PutBE32( e+4, uint(v[c]) ); PutBE32( e+8, uint(t.csdi) ); tb.insert( tb.end(), e, e+12 ); ne++;
            }
            if( ne!=cnt ) FAIL("stsc entry count mismatch");
          }
        } else if( d.n==TB_TFDT ) {
          if( !(t.tflags&8) || sp<4 ) FAIL("bad table");
          int v = st[sp-4];
          size_t k = std::lower_bound( t.trun.begin(), t.trun.end(), sp ) - t.trun.begin();
          if( k>=t.spc.size()-t.npc ) FAIL("bad tfdt ref");
          qword val = qword(t.tbase) + t.dts[size_t(t.cfirst[size_t(t.npc+k)])];
          n = v==1 ? 8 : 4;
          if( pass ) { tb.resize( size_t(n) ); if( v==1 ) PutBE64( &tb[0], val ); else PutBE32( &tb[0], uint(val) ); }
        } else if( d.n==TB_TRUN ) {
          if( ntrun[d.t]>=t.trun.size() ) FAIL("bad table");
          size_t k = size_t(ntrun[d.t]++), c = size_t(t.npc+k);
          TrunHdr h; ParseTrunHdr( st+t.trun[k], M.stlen-t.trun[k], h );
          if( c>=t.spc.size() || t.spc[c]!=h.count ) FAIL("bad trun");
          bool dsz = (h.flags&0x200) && t.szmode>=2, ddur = (h.flags&0x100) && (t.tflags&2), dcto = (h.flags&0x800) && (t.tflags&4);
          if( M.ver<7 ) ddur = dcto = false;
          if( !(dsz||ddur||dcto) ) FAIL("bad trun");
          qword rs = h.recsize-4*(int(dsz)+int(ddur)+int(dcto));
          n = qword(h.recsize)*h.count; ns = rs*h.count;
          if( ns>M.stlen-sp ) FAIL("struct overrun");
          if( pass ) {
            tb.resize( size_t(n) );
            byte* q = tb.data(); const byte* r = st+sp;
            for( qword j=0; j<h.count; j++ ) {
              qword si = t.cfirst[c]+j;
              for( uint fb=0x100; fb<=0x800; fb<<=1 ) {
                if( !(h.flags&fb) ) continue;
                bool der = fb==0x100 ? ddur : fb==0x200 ? dsz : fb==0x800 ? dcto : false;
                if( !der ) { memcpy( q, r, 4 ); r += 4; }
                else if( fb==0x100 ) { if( t.dur[size_t(si)]>>32 ) FAIL("duration overflow"); PutBE32( q, uint(t.dur[size_t(si)]) ); }
                else if( fb==0x200 ) PutBE32( q, t.Size(si) );
                else PutBE32( q, uint(t.cto[size_t(si)]) );
                q += 4;
              }
            }
          }
        } else FAIL("bad table");
        if( pass ) out.put( tb.data(), n );
        pos+=n; sp+=ns;
      } else {
        for( qword j=0; j<(d.code==OP_CRUN ? d.n : 1); j++ ) {
          size_t ti = size_t(d.t);
          if( d.code==OP_CRUN ) {                 // predicted: group track with the smallest (next-b, index)
            ti = nt; long long bk = 0;
            for( size_t u=0; u<nt; u++ ) {
              const DTrack& g = M.trk[u];
              if( !(g.cflags&2) || g.xform==XF_BLOB || next[u]>=g.spc.size() ) continue;
              long long key = (long long)next[u]-g.cb;
              if( ti==nt || key<bk ) ti = u, bk = key;
            }
            if( ti==nt ) FAIL("no chunk to predict");
          }
          DTrack& t = M.trk[ti];
          qword c = d.code==OP_CHUNK ? d.c : next[ti];
          if( c>=t.spc.size() ) FAIL("bad chunk index");
          next[ti] = c+1;
          qword n = t.csz[size_t(c)];
          if( pass==0 ) t.cpos[size_t(c)] = pos;
          else out.put( S[ti]->data()+t.cso[size_t(c)], n );
          pos+=n;
        }
      }
    }
    if( sp!=M.stlen ) FAIL("struct not consumed");
    if( pass && (out.err || (out.ref && out.pos!=out.ref->size())) ) FAIL( out.ref ? "output differs" : "write error" );
  }
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
    if( t.xform==XF_HDR ) {                   // codec config / WAV header: also the source of a cut DSI
      if( t.hdrlen>S[i].size() ) { *why = "bad header stream"; return false; }
      t.priv.assign( S[i].begin(), S[i].begin()+size_t(t.hdrlen) ); R[i].assign( S[i].begin()+size_t(t.hdrlen), S[i].end() );
      P[i] = &R[i]; continue;
    }
    bool ok = t.xform==XF_ANNEXB ? UnAnnexB( t, S[i], R[i] ) : t.xform==XF_OGG ? UnOgg( t, S[i], R[i] ) : UnADTS( t, S[i], R[i] );
    if( !ok ) { *why = t.xform==XF_ANNEXB ? "bad annexb stream" : t.xform==XF_OGG ? "bad ogg stream" : "bad adts stream"; return false; }
    if( t.xform==XF_ANNEXB ) t.xhdr.assign( S[i].begin(), S[i].begin()+size_t(t.hdrlen) );
    P[i] = &R[i];
  }
  return Restore( M, P, out, why );
}

} // namespace mp4
