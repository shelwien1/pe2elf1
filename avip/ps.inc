// MPEG program streams: MPEG-1 system streams, MPEG-2 program streams (.mpg, VCD, DVD .vob)
//
// Track streams hold the PES payloads of each elementary stream, by stream id, and for
// private_stream_1 (DVD) by sub-stream id: MPEG-1/2 video (.m1v/.m2v), MPEG-4 Part 2 (.m4v),
// H.264, MPEG audio, AC-3, DTS, LPCM, subpictures. Regenerated on restore:
//   - PES packets: start code, stream id, length and sub-stream id; the header from the
//     track's PES model (mpg_pes.inc: templates, PTS / DTS differences); the payload: the
//     track's next frame. Muxers write packets of a fixed size (DVD 2048 bytes, VCD 2324),
//     a pack header counted in, so the payload size of a packet that ends at the next
//     multiple of that size from the last pack header is derived; the others are stored
//   - pack headers: the SCR as a difference to the last SCR plus its step, mux rate, stuffing
//   - padding packets of 0xFF bytes, and their length where they fill the packet
//   - for DVD AC-3 sub-streams, the frame count and first access unit pointer after the
//     sub-stream id, from the AC-3 frames that start in the packet
// The struct keeps the rest: system headers, program stream maps, navigation packets
// (private_stream_2), the other bytes after a DVD sub-stream id (DTS, LPCM header), end
// codes, and whatever doesn't parse.
//
// meta: vint ntracks; per track: common head (frames: its PES packets; sizes: their
//   payloads after the sub-stream bytes; szmode 1: from the layout, see below), u8 stream id,
//   [private_stream_1: u8 sub-stream id, u8 bytes kept after it, u8 their model (0 in the
//   struct, 1 AC-3 frame count and first access unit pointer: then the PES packets that
//   have them in the struct, as an index list)], PES model (mpg_pes.inc), [szmode 1: column
//   per PES of 0 (fills its packet) or payload size + 1]
// pack model: u8 kind (0: none modeled, 1 MPEG-1, 2 MPEG-2); with a kind: vint packet size
//   (0: none), vint packs, vint step mode m, columns of zigzag(SCR - predicted SCR) (MPEG-2:
//   27 MHz units; predicted: the last SCR plus, m 0: the last step, else m-1), mux rates,
//   MPEG-2: stuffing lengths. A PES or padding packet starting at
//   pos fills its packet when it ends at p + k * packet size, p the position of the last
//   modeled pack header (none: never), k the smallest with p + k * packet size > pos.
// ops: 0x00..0x77  PES packet of track #code
//      0x78..0xEF  pack header, then PES packet of track #(code-0x78)
//      0xF0 n      n bytes from the struct
//      0xF1 t      PES packet of track t
//      0xF2        pack header (the pack model's next one)
//      0xF3 n      padding packet of n bytes of 0xFF
//      0xF4        padding packet of 0xFF bytes that fills the packet
//      0xFF        the rest of the struct, end
// The struct holds, for each PES packet of a private_stream_1 track in turn, the bytes kept
// after its sub-stream id.
namespace ps {

enum { OP_PACKPES=0x78, OP_RAW=0xF0, OP_PES=0xF1, OP_PACK=0xF2, OP_PAD=0xF3, OP_PADF=0xF4, OP_END=0xFF };

// pack header: kind (1 MPEG-1, 2 MPEG-2), length, SCR (MPEG-2: 27 MHz units), mux rate, stuffing
struct Pack { int kind; qword len, scr, mux; uint stuff; };

static bool ParsePack( const byte* p, qword avail, Pack& k ) {
  if( avail<12 ) return false;
  if( (p[4]&0xC0)==0x40 ) {                                 // MPEG-2
    if( avail<14 ) return false;
    k.kind = 2; k.stuff = p[13]&7; k.len = 14+k.stuff;
    if( k.len>avail ) return false;
    qword base = (qword(p[4]>>3)&7)<<30 | qword(p[4]&3)<<28 | qword(p[5])<<20 | qword(p[6]>>3)<<15 | qword(p[6]&3)<<13 |
                 qword(p[7])<<5 | qword(p[8]>>3);
    k.scr = base*300 + (qword(p[8]&3)<<7 | qword(p[9]>>1));
    k.mux = qword(p[10])<<14 | qword(p[11])<<6 | qword(p[12]>>2);
    return true;
  }
  if( (p[4]&0xF0)!=0x20 || !mpg::GetTs( p+4, 2, k.scr ) ) return false;   // MPEG-1
  k.kind = 1; k.len = 12; k.stuff = 0;
  k.mux = qword(p[9]&0x7F)<<15 | qword(p[10])<<7 | qword(p[11]>>1);
  return true;
}

// canonical bytes (markers and reserved bits set, stuffing 0xFF), k.len of them
static void PutPack( const Pack& k, byte* o ) {
  o[0] = 0; o[1] = 0; o[2] = 1; o[3] = 0xBA;
  if( k.kind==2 ) {
    qword b = k.scr/300, e = k.scr%300;
    o[4] = byte( 0x44 | (b>>27&0x38) | (b>>28&3) ); o[5] = byte(b>>20);
    o[6] = byte( (b>>12&0xF8) | 4 | (b>>13&3) );    o[7] = byte(b>>5);
    o[8] = byte( (b<<3&0xF8) | 4 | (e>>7&3) );      o[9] = byte( (e<<1&0xFE) | 1 );
    o[10] = byte(k.mux>>14); o[11] = byte(k.mux>>6); o[12] = byte( (k.mux<<2&0xFC) | 3 );
    o[13] = byte( 0xF8 | k.stuff );
    for( uint i=0; i<k.stuff; i++ ) o[14+i] = 0xFF;
  } else {
    mpg::PutTs( o+4, 2, k.scr );
    o[9] = byte( 0x80 | (k.mux>>15&0x7F) ); o[10] = byte(k.mux>>7); o[11] = byte( (k.mux<<1&0xFE) | 1 );
  }
}

// bytes kept after a DVD private_stream_1 sub-stream id: AC-3 / DTS frame count and first
// access unit (3), LPCM also its header (6)
static int SubExtra( int sub ) { return sub>=0x80 && sub<=0x8F ? 3 : sub>=0xA0 && sub<=0xAF ? 6 : 0; }

// length of the system unit at p (a pack header, or a packet with a length field: 0xBB..0xFF), 0 if none
static qword UnitAt( const byte* d, qword p, qword n ) {
  if( n-p<6 || d[p] || d[p+1] || d[p+2]!=1 || d[p+3]<0xBA ) return 0;
  if( d[p+3]==0xBA ) { Pack k; return ParsePack( d+p, n-p, k ) ? k.len : 0; }
  qword L = 6+qword(BE16(d+p+4));
  return L<=n-p ? L : 0;
}

// what follows a unit ending at e: zeros, then a system start code (>= 0xB9), or the end
static bool NextOk( const byte* d, qword e, qword n ) {
  qword q = e; while( q<n && !d[q] ) q++;
  return q==n || (q>=e+2 && q+1<n && d[q]==1 && d[q+1]>=0xB9);
}

// a program stream: system units that follow each other, at least 4 (or to the end), from one
// within the first 4 KB, among them a pack header or a PES packet with a header
static bool Detect( const byte* d, qword n ) {
  for( qword p=Next001( d, 0, n ); p<n && p<4096; p=Next001( d, p+1, n ) ) {
    qword q = p; int k = 0; bool av = false;
    while( k<4 ) {
      qword L = UnitAt( d, q, n );
      if( !L || !NextOk( d, q+L, n ) ) break;
      int c = d[q+3]; mpg::PesHdr h;
      if( c==0xBA || ((c==0xBD || (c>=0xC0 && c<=0xEF)) && mpg::ParsePesHdr( c, d+q+6, L-6, h )) ) av = true;
      q += L; k++;
      while( q<n && !d[q] && !(q+2<n && !d[q+1] && d[q+2]==1) ) q++;
      if( q>=n ) break;
    }
    if( av && (k>=4 || (k && q>=n)) ) return true;
  }
  return false;
}

// start offsets of the AC-3 frames of stream s: from the first sync word that a frame or the
// end follows, while frames follow each other
static void Ac3Starts( const Buf& s, std::vector<qword>& v ) {
  v.clear(); qword n = s.size(), q = 0;
  for( ; q+6<=n; q++ ) {
    if( s[q]!=0x0B || s[q+1]!=0x77 ) continue;
    qword u = UnitLen( PT_AC3, 0, s.data()+q, n-q );
    if( u && (q+u==n || (q+u+2<=n && s[q+u]==0x0B && s[q+u+1]==0x77)) ) break;
  }
  while( q+6<=n ) {
    qword u = UnitLen( PT_AC3, 0, s.data()+q, n-q );
    if( !u ) break;
    v.push_back( q ); q += u;
  }
}

// the 3 bytes after an AC-3 sub-stream id for a payload at [o,o+z) of the stream: the frames
// that start in it, 1 + the offset of the first; i: cursor into the frame starts
static void Ac3Bytes( const std::vector<qword>& v, size_t& i, qword o, qword z, byte* b ) {
  while( i<v.size() && v[i]<o ) i++;
  size_t j = i; while( j<v.size() && v[j]<o+z ) j++;
  qword n = j-i, p = n ? v[i]-o+1 : 0;
  b[0] = byte( n>255 ? 255 : n ); b[1] = byte( p>0xFFFF ? 0xFF : p>>8 ); b[2] = byte( p>0xFFFF ? 0xFF : p );
}

static int UnitType( const std::string& e ) {
  return e=="m1v" || e=="m2v" ? PT_M2V : e=="m4v" ? PT_VOP : e=="h264" ? PT_AU264 : e=="ac3" ? PT_AC3 :
         e=="mp1" || e=="mp2" || e=="mp3" ? PT_MPA : e=="aac" ? PT_ADTS : PT_NONE;
}

//--- decoder -----------------------------------------------------------------

struct DTrack {
  TrackHead h; int sid, sub, xb, xm; std::vector<qword> xexc;   // xm 1: AC-3 bytes derived but at xexc
  mpg::PesModel pm; std::vector<qword> fill;                // szmode 1: per PES 0 (fills its packet) or size+1
  std::vector<uint> sz, xsz;
};

struct DOp { int code; qword t, n; };

struct Meta {
  const byte* st; qword stlen;
  std::vector<DOp> ops; std::vector<DTrack> trk;
  int pk; qword psize, npk, sm; std::vector<qword> scr, mux, stf;
};

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_PS ) return false;
  Reader r( m.data()+1, m.size()-1 );
  qword nt = r.vint(); if( r.err || nt>1024 ) return false;
  M.trk.resize( size_t(nt) );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i];
    if( !GetHead( r, t.h ) || t.h.xform!=XF_NONE || t.h.szmode==SZ_XFORM ) return false;
    t.sid = int(r.u8()); t.sub = -1; t.xb = 0; t.xm = 0; t.xexc.clear();
    if( t.sid==0xBD ) t.sub = int(r.u8()), t.xb = int(r.u8()), t.xm = int(r.u8());
    if( r.err || t.sid<0xBD || mpg::PesNoHdr( t.sid ) || t.xb>8 || t.xm>1 || (t.xm && t.xb!=3) ) return false;
    if( t.xm && !GetIdxCol( r, t.xexc, t.h.nf ) ) return false;
    if( !mpg::GetPesModel( r, t.pm, t.h.nf ) ) return false;
    t.fill.clear();
    if( t.h.szmode==SZ_TABLES ) {
      if( !GetCol( r, t.fill, t.h.nf ) ) return false;
      for( size_t k=0; k<t.fill.size(); k++ ) if( t.fill[k]>0x10000 ) return false;
    }
  }
  M.pk = int(r.u8()); M.psize = M.npk = M.sm = 0; M.scr.clear(); M.mux.clear(); M.stf.clear();
  if( r.err || M.pk>2 ) return false;
  if( M.pk ) {
    M.psize = r.vint(); M.npk = r.vint(); M.sm = r.vint();
    if( r.err || M.psize>0x100000 || !GetCol( r, M.scr, M.npk ) || !GetCol( r, M.mux, M.npk ) ) return false;
    if( M.pk==2 && !GetCol( r, M.stf, M.npk ) ) return false;
    for( size_t k=0; k<M.mux.size(); k++ ) if( M.mux[k]>=(qword(1)<<22) || (M.pk==2 && M.stf[k]>7) ) return false;
  }
  M.ops.clear();
  for(;;) {
    DOp d; d.code = int(r.u8()); d.t = d.n = 0;
    if( r.err ) return false;
    if( d.code<OP_RAW ) { d.n = d.code>=OP_PACKPES; d.t = qword( d.code-(d.n ? OP_PACKPES : 0) ); d.code = OP_PES; }
    else if( d.code==OP_PES ) d.t = r.vint();
    else if( d.code==OP_RAW || d.code==OP_PAD ) d.n = r.vint();
    else if( d.code!=OP_PACK && d.code!=OP_PADF && d.code!=OP_END ) return false;
    if( r.err || (d.code==OP_PES && d.t>=M.trk.size()) || (d.code==OP_PAD && d.n>0xFFFF) ) return false;
    M.ops.push_back( d );
    if( d.code==OP_END ) break;
  }
  M.st = r.p; M.stlen = qword(r.e-r.p);
  return true;
}

// end of the packet that a packet starting at pos fills (pstart: the last pack header, -1 none)
static qword FillEnd( long long pstart, qword psize, qword pos ) {
  if( pstart<0 || !psize || pos<qword(pstart) ) return 0;
  return qword(pstart) + ((pos-qword(pstart))/psize+1)*psize;
}

static bool Restore( Meta& M, const std::vector<const Buf*>& S, Sink& out, const char** why ) {
  #define FAIL(s) { *why = s; return false; }
  size_t nt = M.trk.size();
  std::vector<qword> next( nt, 0 ), so( nt, 0 );            // next PES, stream offset
  std::vector<mpg::PesCursor> cur( nt );
  std::vector<std::vector<qword> > fst( nt ); std::vector<size_t> fi( nt, 0 ), xi( nt, 0 );   // AC-3 frame starts, cursors
  for( size_t i=0; i<nt; i++ ) {
    DTrack& t = M.trk[i];
    if( t.h.szmode!=SZ_TABLES && !HeadSizes( t.h, *S[i], t.xsz, t.sz, why ) ) return false;
    if( t.xm ) Ac3Starts( *S[i], fst[i] );
  }
  const byte* st = M.st;
  qword pos = 0, sp = 0, kp = 0, ls = 0, lst = 0; long long pstart = -1;
  byte ff[256]; memset( ff, 0xFF, sizeof(ff) );
  Buf hb;
  for( size_t i=0; i<M.ops.size(); i++ ) {
    const DOp& d = M.ops[i];
    if( d.code==OP_RAW || d.code==OP_END ) {
      qword n = d.code==OP_RAW ? d.n : M.stlen-sp;
      if( n>M.stlen-sp ) FAIL("struct overrun");
      out.put( st+sp, n ); sp += n; pos += n;
      continue;
    }
    if( d.code==OP_PACK || (d.code==OP_PES && d.n) ) {
      if( !M.pk || kp>=M.npk ) FAIL("pack overrun");
      Pack k; k.kind = M.pk; k.scr = ls+(M.sm ? M.sm-1 : lst)+qword( UnZigZag( M.scr[size_t(kp)] ) ); k.mux = M.mux[size_t(kp)];
      k.stuff = M.pk==2 ? uint(M.stf[size_t(kp)]) : 0; k.len = M.pk==2 ? 14+k.stuff : 12;
      if( k.scr>=(qword(1)<<33)*(M.pk==2 ? 300 : 1) ) FAIL("bad SCR");
      lst = k.scr-ls; ls = k.scr; kp++;
      byte b[24]; PutPack( k, b ); out.put( b, k.len );
      pstart = (long long)pos; pos += k.len;
      if( d.code==OP_PACK ) continue;
    }
    if( d.code==OP_PAD || d.code==OP_PADF ) {
      qword n = d.n;
      if( d.code==OP_PADF ) {
        qword e = FillEnd( pstart, M.psize, pos );
        if( e<pos+6 || e-pos-6>0xFFFF ) FAIL("bad packet fill");
        n = e-pos-6;
      }
      byte h[6] = { 0, 0, 1, 0xBE, byte(n>>8), byte(n) };
      out.put( h, 6 );
      for( qword j=0; j<n; ) { qword c = std::min( n-j, qword(sizeof(ff)) ); out.put( ff, c ); j += c; }
      pos += 6+n;
      continue;
    }
    size_t ti = size_t(d.t); DTrack& t = M.trk[ti]; const Buf& s = *S[ti];
    if( next[ti]>=t.h.nf ) FAIL("PES overrun");
    qword k = next[ti]++, ex = t.sub>=0 ? 1+qword(t.xb) : 0, z;
    if( t.h.szmode!=SZ_TABLES ) z = t.sz[size_t(k)];
    else if( t.fill[size_t(k)] ) z = t.fill[size_t(k)]-1;
    else {                                                // fills the packet
      long long hl = mpg::PesHdrSize( t.pm, cur[ti] );
      qword e = FillEnd( pstart, M.psize, pos );
      if( hl<0 || e<pos+4+qword(hl)+ex ) FAIL("bad packet fill");
      z = e-(pos+4+qword(hl)+ex);
    }
    if( z>s.size()-so[ti] ) FAIL("stream too short");
    if( !mpg::PesHeaderNext( t.pm, cur[ti], ex+z, hb ) ) FAIL("bad PES header");
    byte h[4] = { 0, 0, 1, byte(t.sid) };
    out.put( h, 4 ); out.put( hb.data(), hb.size() );
    if( t.sub>=0 ) {
      byte sb[12]; sb[0] = byte(t.sub);
      while( xi[ti]<t.xexc.size() && t.xexc[xi[ti]]<k ) xi[ti]++;
      if( t.xm && !(xi[ti]<t.xexc.size() && t.xexc[xi[ti]]==k) ) Ac3Bytes( fst[ti], fi[ti], so[ti], z, sb+1 );
      else {
        if( qword(t.xb)>M.stlen-sp ) FAIL("struct overrun");
        memcpy( sb+1, st+sp, size_t(t.xb) ); sp += qword(t.xb);
      }
      out.put( sb, 1+qword(t.xb) );
    }
    out.put( s.data()+so[ti], z ); so[ti] += z;
    pos += 4+hb.size()+ex+z;
  }
  if( sp!=M.stlen ) FAIL("struct not consumed");
  if( kp!=M.npk ) FAIL("pack count");
  for( size_t i=0; i<nt; i++ ) if( next[i]!=M.trk[i].h.nf || so[i]!=S[i]->size() ) FAIL("track not consumed");
  if( out.err || (out.ref && out.pos!=out.ref->size()) ) FAIL( out.ref ? "output differs" : "write error" );
  return true;
  #undef FAIL
}

static std::string TrackName( const Meta& M, size_t i ) { return TrackSuffix( i, M.trk[i].h.ext ); }

static bool Decode( Meta& M, const std::vector<Buf>& S, Sink& out, const char** why ) {
  std::vector<const Buf*> P( M.trk.size() );
  for( size_t i=0; i<M.trk.size(); i++ ) P[i] = &S[i];
  return Restore( M, P, out, why );
}

//--- encoder -----------------------------------------------------------------

struct Track {
  int sid, sub, xb, xm;                                     // stream id, sub-stream id (-1: none), bytes kept after it, their model
  std::vector<qword> xo, xexc;                              // per PES: where the kept bytes are; derived but not matching
  std::vector<qword> foff; std::vector<uint> fsz;           // payloads in the file
  std::vector<qword> fill;                                  // per PES: 0 (fills its packet) or size+1
  mpg::PesModel pm; std::string ext; TrackHead h;
};

enum { U_RAW, U_PACK, U_PES, U_PAD };
struct Unit { int kind, trk; qword pos, len; Pack pk; qword xo; };   // xo: the kept sub-stream bytes

static size_t HeadCost( const TrackHead& h ) { Writer w; PutHead( w, h ); return w.b.size(); }

// level 2: all models; 1: PES payloads only (sizes explicit or units, packs and padding as bytes)
static bool Encode1( const Buf& in, int level, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names,
                     const char** why, std::string& log ) {
  const byte* d = in.data(); qword fs = in.size();
  std::vector<Unit> U; std::vector<Track> T; std::map<int,int> key;
  auto raw = [&]( qword p, qword n ) {
    if( !n ) return;
    if( U.size() && U.back().kind==U_RAW && U.back().pos+U.back().len==p ) U.back().len += n;
    else { Unit u; u.kind = U_RAW; u.trk = -1; u.pos = p; u.len = n; u.xo = 0; U.push_back( u ); }
  };
  for( qword p=0; p<fs; ) {
    if( fs-p>=4 && !d[p] && !d[p+1] && d[p+2]==1 && d[p+3]>=0xB9 ) {
      int c = d[p+3];
      Unit u; u.trk = -1; u.pos = p; u.xo = 0;
      if( c==0xBA && ParsePack( d+p, fs-p, u.pk ) && NextOk( d, p+u.pk.len, fs ) ) {
        u.kind = U_PACK; u.len = u.pk.len; U.push_back( u ); p += u.len; continue;
      }
      if( c>=0xBB && fs-p>=6 && 6+qword(BE16(d+p+4))<=fs-p && NextOk( d, p+6+qword(BE16(d+p+4)), fs ) ) {
        qword len = BE16(d+p+4); u.len = 6+len;
        bool pes = c==0xBD || (c>=0xC0 && c<=0xEF) || c==0xFA || c==0xFD;
        mpg::PesHdr h;
        if( pes && mpg::ParsePesHdr( c, d+p+6, len, h ) ) {
          qword po = p+6+h.len, pl = len-h.len; int sub = -1, xb = 0;
          if( c==0xBD ) { if( pl ) sub = d[po], xb = SubExtra( sub ); if( !pl || pl<1+qword(xb) ) sub = -2; }
          if( sub!=-2 ) {
            int kk = c<<9 | (sub>=0 ? 256|sub : 0);
            std::map<int,int>::iterator it = key.find( kk );
            if( it==key.end() ) {
              Track t; t.sid = c; t.sub = sub; t.xb = xb; t.xm = 0; T.push_back( t );
              it = key.insert( std::make_pair( kk, int(T.size())-1 ) ).first;
            }
            Track& t = T[size_t(it->second)];
            qword ex = sub>=0 ? 1+qword(xb) : 0;
            t.pm.Add( d+p+4, 2+h.len, h, true );
            t.foff.push_back( po+ex ); t.fsz.push_back( uint(pl-ex) ); t.xo.push_back( po+1 );
            u.kind = U_PES; u.trk = it->second; u.xo = po+1; U.push_back( u ); p += u.len; continue;
          }
        }
        if( level>1 && c==0xBE ) {                        // padding of 0xFF bytes
          qword i = 0; while( i<len && d[p+6+i]==0xFF ) i++;
          if( i==len ) { u.kind = U_PAD; U.push_back( u ); p += u.len; continue; }
        }
        raw( p, u.len ); p += u.len; continue;
      }
    }
    qword q = Next001( d, p+1, fs );                      // anything else: as is, up to the next start code
    raw( p, q-p ); p = q;
  }
  // pack model: packs of the first pack's kind with canonical bytes; the others stay as bytes
  int pkind = 0; std::vector<qword> vscr, cscr, cmux, cstf;
  for( size_t k=0; k<U.size(); k++ ) {
    Unit& u = U[k];
    if( u.kind!=U_PACK ) continue;
    if( !pkind ) pkind = u.pk.kind;
    byte b[24]; PutPack( u.pk, b );
    if( level<2 || u.pk.kind!=pkind || memcmp( b, d+u.pos, size_t(u.len) ) ) { u.kind = U_RAW; continue; }
    vscr.push_back( u.pk.scr ); cmux.push_back( u.pk.mux ); cstf.push_back( u.pk.stuff );
  }
  if( vscr.empty() ) pkind = 0;
  // SCR prediction: the last step, or the most common step
  qword sm = 0;
  {
    std::map<qword,qword> hist; qword bc = 0, step = 0;
    for( size_t k=1; k<vscr.size(); k++ ) hist[vscr[k]-vscr[k-1]]++;
    for( std::map<qword,qword>::iterator it=hist.begin(); it!=hist.end(); ++it ) if( it->second>bc ) bc = it->second, step = it->first;
    std::vector<qword> c0( vscr.size() ), c1( vscr.size() ); qword ls = 0, lst = 0;
    for( size_t k=0; k<vscr.size(); k++ ) {
      c0[k] = ZigZag( (long long)( vscr[k]-(ls+lst) ) ); c1[k] = ZigZag( (long long)( vscr[k]-(ls+step) ) );
      lst = vscr[k]-ls; ls = vscr[k];
    }
    Writer w0, w1; PutCol( w0, c0 ); PutCol( w1, c1 );
    if( w1.b.size()+VintSize( step+1 )<w0.b.size()+1 ) sm = step+1, cscr.swap( c1 ); else cscr.swap( c0 );
  }
  // packet size: the most common distance between packet starts (pack headers, other units
  // not right after one); which packets fill their packet
  qword psize = 0;
  if( pkind ) {
    std::map<qword,qword> hist; qword lp = 0, bc = 0; bool any = false;
    for( size_t k=0; k<U.size(); k++ ) {
      if( k && U[k-1].kind==U_PACK ) continue;
      if( any ) hist[U[k].pos-lp]++;
      lp = U[k].pos; any = true;
    }
    for( std::map<qword,qword>::iterator it=hist.begin(); it!=hist.end(); ++it ) if( it->second>bc ) bc = it->second, psize = it->first;
  }
  std::vector<byte> fills( U.size(), 0 );
  {
    long long lp = -1;
    for( size_t k=0; k<U.size(); k++ ) {
      const Unit& u = U[k];
      if( u.kind==U_PACK ) lp = (long long)u.pos;
      else if( (u.kind==U_PES || u.kind==U_PAD) && u.pos+u.len==FillEnd( lp, psize, u.pos ) ) fills[k] = 1;
      if( u.kind==U_PES ) T[size_t(u.trk)].fill.push_back( fills[k] ? 0 : qword(T[size_t(u.trk)].fsz[T[size_t(u.trk)].fill.size()])+1 );
    }
  }
  // tracks: streams, codecs, size models
  streams.clear(); names.clear();
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; Buf s;
    for( size_t k=0; k<t.fsz.size(); k++ ) s.insert( s.end(), d+t.foff[k], d+t.foff[k]+t.fsz[k] );
    t.ext = mpg::EsKind( t.sid, t.sub, s.data(), s.size() );
    t.h.ext = t.ext; t.h.xform = XF_NONE; t.h.nf = t.fsz.size(); t.h.szmode = SZ_EXPLICIT; t.h.col.assign( t.fsz.begin(), t.fsz.end() );
    size_t best = HeadCost( t.h );
    int pt = UnitType( t.ext );
    TrackHead hu = t.h;
    if( pt!=PT_NONE && ChooseUnits( pt, 0, d, t.foff, t.fsz, s, hu ) && HeadCost( hu )<best ) t.h = hu, best = HeadCost( hu );
    if( level>1 && psize ) {
      TrackHead hf = t.h; hf.szmode = SZ_TABLES; hf.col.clear(); hf.esc.clear();
      Writer w; PutCol( w, t.fill );
      if( HeadCost( hf )+w.b.size()<best ) t.h = hf;
    }
    if( level>1 && t.ext=="ac3" && t.xb==3 ) {            // DVD AC-3: frame count and first access unit derived
      std::vector<qword> fs; Ac3Starts( s, fs ); size_t fi = 0; qword o = 0;
      std::vector<qword> exc;
      for( size_t k=0; k<t.fsz.size(); k++ ) {
        byte b[3]; Ac3Bytes( fs, fi, o, t.fsz[k], b );
        if( memcmp( b, d+t.xo[k], 3 ) ) exc.push_back( k );
        o += t.fsz[k];
      }
      if( 2*exc.size()<t.fsz.size() ) t.xm = 1, t.xexc.swap( exc );
    }
    names.push_back( TrackSuffix( i, t.ext ) ); streams.push_back( Buf() ); streams.back().swap( s );
  }
  // ops + struct
  Writer op; Buf st; st.reserve( size_t(fs>>5) );
  qword rawb = 0, npes = 0, npad = 0, npk = 0, nfill = 0;
  std::vector<size_t> pk( T.size(), 0 ), pxi( T.size(), 0 );   // per track: PES index, exception cursor
  auto pes = [&]( const Unit& u, int base ) {
    if( u.trk<0x78 ) op.u8( uint(base+u.trk) ); else op.u8( OP_PES ), op.vint( qword(u.trk) );
    const Track& t = T[size_t(u.trk)]; size_t k = pk[size_t(u.trk)]++; size_t& x = pxi[size_t(u.trk)];
    while( x<t.xexc.size() && t.xexc[x]<k ) x++;
    if( t.sub>=0 && (!t.xm || (x<t.xexc.size() && t.xexc[x]==k)) ) st.insert( st.end(), d+u.xo, d+u.xo+t.xb );
    npes++;
  };
  for( size_t k=0; k<U.size(); k++ ) {
    const Unit& u = U[k];
    if( u.kind==U_RAW ) {                                 // (merges raw units that a pack fell back into)
      qword n = u.len; size_t j = k;
      while( j+1<U.size() && U[j+1].kind==U_RAW ) n += U[++j].len;
      op.u8( OP_RAW ); op.vint( n ); st.insert( st.end(), d+u.pos, d+u.pos+n ); rawb += n; k = j;
    } else if( u.kind==U_PACK ) {
      npk++;
      if( k+1<U.size() && U[k+1].kind==U_PES && U[k+1].trk<0x78 ) pes( U[++k], OP_PACKPES );
      else op.u8( OP_PACK );
    } else if( u.kind==U_PAD ) {
      if( fills[k] ) op.u8( OP_PADF ); else op.u8( OP_PAD ), op.vint( u.len-6 );
      npad++;
    } else pes( u, 0 );
  }
  op.u8( OP_END );
  // meta
  Writer w; PutMetaHdr( w, FMT_PS );
  w.vint( T.size() );
  size_t zhead = 0, zpes = 0, zfill = 0;
  for( size_t i=0; i<T.size(); i++ ) {
    const Track& t = T[i];
    size_t z0 = w.b.size();
    PutHead( w, t.h ); w.u8( uint(t.sid) );
    if( t.sid==0xBD ) {
      w.u8( uint(t.sub) ); w.u8( uint(t.xb) ); w.u8( uint(t.xm) );
      if( t.xm ) PutIdxCol( w, t.xexc );
    }
    size_t z1 = w.b.size();
    mpg::PutPesModel( w, t.pm );
    size_t z2 = w.b.size();
    if( t.h.szmode==SZ_TABLES ) PutCol( w, t.fill );
    zhead += z1-z0; zpes += z2-z1; zfill += w.b.size()-z2;
    if( verbose ) {
      qword nf = 0; for( size_t k=0; k<t.fill.size(); k++ ) nf += !t.fill[k];
      nfill += t.h.szmode==SZ_TABLES ? nf : 0;
      char id[16]; if( t.sub>=0 ) snprintf( id, sizeof(id), "%02X.%02X", t.sid, t.sub ); else snprintf( id, sizeof(id), "%02X", t.sid );
      char l[240];
      snprintf( l, sizeof(l), "stream %-5s %-4s %8llu PES %11llu bytes  sizes=%s templates=%llu -> %s\n", id, t.ext.c_str(),
        (unsigned long long)t.fsz.size(), (unsigned long long)streams[i].size(),
        t.h.szmode==SZ_UNITS ? ptname[t.h.ptype] : t.h.szmode==SZ_TABLES ? "packet fill" : "explicit",
        (unsigned long long)t.pm.tm.size(), names[i].c_str() );
      log += l;
    }
  }
  size_t z3 = w.b.size();
  w.u8( uint(pkind) );
  if( pkind ) {
    w.vint( psize ); w.vint( cscr.size() ); w.vint( sm );
    PutCol( w, cscr ); PutCol( w, cmux ); if( pkind==2 ) PutCol( w, cstf );
  }
  size_t zpk = w.b.size()-z3;
  if( verbose ) {
    char l[240];
    snprintf( l, sizeof(l), "layout: %llu PES packets (%llu fill their packet), %llu packs (MPEG-%d), packet size %llu, %llu padding; "
      "raw %llu bytes; struct %llu, ops %llu\n", (unsigned long long)npes, (unsigned long long)nfill, (unsigned long long)npk, pkind,
      (unsigned long long)psize, (unsigned long long)npad, (unsigned long long)rawb, (unsigned long long)st.size(), (unsigned long long)op.b.size() );
    log += l;
    snprintf( l, sizeof(l), "meta: heads %llu, PES models %llu, packet fill sizes %llu, packs %llu, ops %llu, struct %llu\n",
      (unsigned long long)zhead, (unsigned long long)zpes, (unsigned long long)zfill, (unsigned long long)zpk,
      (unsigned long long)op.b.size(), (unsigned long long)st.size() );
    log += l;
  }
  w.bytes( op.b.data(), op.b.size() ); w.bytes( st.data(), st.size() );
  meta.swap( w.b );
  Meta M; Sink chk( 0, &in ); *why = "meta parse";
  return ParseMeta( meta, M ) && Decode( M, streams, chk, why );
}

static bool Encode( const Buf& in, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names ) {
  const char* why = ""; std::string log;
  for( int level=2; level>=1; level-- ) {
    log.clear();
    if( Encode1( in, level, meta, streams, names, &why, log ) && streams.size() ) {
      if( verbose ) fputs( log.c_str(), stdout );
      return true;
    }
    if( streams.size() ) fprintf( stderr, "warning: verify failed (%s)%s\n", why, level>1 ? ", retrying with fewer models" : "" );
  }
  meta.clear(); streams.clear(); names.clear();
  return false;
}

} // namespace ps
