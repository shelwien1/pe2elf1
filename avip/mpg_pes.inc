// MPEG-1/2 systems: PES packets, shared by program streams (ps::) and transport streams (ts::),
// and what an elementary stream holds
namespace mpg {

// stream ids whose PES packets have nothing between the length field and the data (H.222.0
// 2.4.3.7): program stream map, padding, private_stream_2, ECM, EMM, DSM-CC, H.222.1 type E,
// program stream directory
static bool PesNoHdr( int sid ) {
  return sid==0xBC || sid==0xBE || sid==0xBF || sid==0xF0 || sid==0xF1 || sid==0xF2 || sid==0xF8 || sid==0xFF;
}

// 33-bit timestamp field: 4-bit prefix, bits 32..30, marker, 29..15, marker, 14..0, marker
static bool GetTs( const byte* p, int pre, qword& v ) {
  if( (p[0]>>4)!=pre || !(p[0]&1) || !(p[2]&1) || !(p[4]&1) ) return false;
  v = (qword(p[0]>>1)&7)<<30 | qword(BE16(p+1)>>1)<<15 | qword(BE16(p+3)>>1);
  return true;
}

static void PutTs( byte* p, int pre, qword v ) {
  uint a = (uint(v>>15)&0x7FFF)<<1 | 1, b = (uint(v)&0x7FFF)<<1 | 1;
  p[0] = byte( pre<<4 | (uint(v>>29)&0x0E) | 1 ); p[1] = byte(a>>8); p[2] = byte(a); p[3] = byte(b>>8); p[4] = byte(b);
}

// the PES header after the length field at p (MPEG-2 syntax, or MPEG-1's: stuffing, STD buffer,
// timestamps): its length, and its PTS / DTS where those fields are canonical (ts: 1 PTS, 3 PTS
// and DTS; offsets from p)
struct PesHdr { qword len, ptso, dtso, pts, dts; int ts; };

static bool ParsePesHdr( int sid, const byte* p, qword avail, PesHdr& h ) {
  h.len = h.ptso = h.dtso = h.pts = h.dts = 0; h.ts = 0;
  if( PesNoHdr( sid ) ) return true;
  if( avail>=3 && (p[0]&0xC0)==0x80 ) {                   // MPEG-2
    qword n = 3+qword(p[2]); int pd = p[1]>>6;
    if( n>avail ) return false;
    if( pd==2 && n>=8 && GetTs( p+3, 2, h.pts ) ) h.ts = 1, h.ptso = 3;
    if( pd==3 && n>=13 && GetTs( p+3, 3, h.pts ) && GetTs( p+8, 1, h.dts ) ) h.ts = 3, h.ptso = 3, h.dtso = 8;
    h.len = n; return true;
  }
  qword i = 0;                                            // MPEG-1
  while( i<avail && i<16 && p[i]==0xFF ) i++;
  if( i<avail && (p[i]&0xC0)==0x40 ) { if( avail-i<2 ) return false; i += 2; }
  if( i>=avail ) return false;
  if( (p[i]>>4)==2 ) {
    if( avail-i<5 ) return false;
    if( GetTs( p+i, 2, h.pts ) ) h.ts = 1, h.ptso = i;
    i += 5;
  } else if( (p[i]>>4)==3 ) {
    if( avail-i<10 ) return false;
    if( GetTs( p+i, 3, h.pts ) && GetTs( p+i+5, 1, h.dts ) ) h.ts = 3, h.ptso = i, h.dtso = i+5;
    i += 10;
  } else if( p[i]==0x0F ) i++;
  else return false;
  h.len = i; return true;
}

// The PES headers of a track: templates (the 2-byte length field and the header after it, with
// the timestamps blanked; lenmode 0: the length is the packet's own and blanked too, 1: as in
// the template), per PES the template index, and the PTS / DTS as zigzag differences to the
// previous ones of the track (mod 2^33). Metainfo: vint n, n x {vint size, bytes, u8 ts |
// lenmode<<2, [vint PTS offset], [vint DTS offset]} (offsets from the length field), then
// columns of the indices, PTS and DTS differences.
struct PesTmpl { Buf b; int ts, lenmode; qword ptso, dtso; };

static qword TsDiff( qword a, qword b ) {                 // zigzag of a-b in [-2^32, 2^32) mod 2^33
  qword d = (a-b) & ((qword(1)<<33)-1);
  return d>=(qword(1)<<32) ? ZigZag( (long long)d-(1LL<<33) ) : ZigZag( (long long)d );
}

struct PesModel {
  std::vector<PesTmpl> tm; std::vector<qword> ti, pr, dr;
  qword lp, ld;                                           // encoder: the last PTS / DTS
  PesModel() : lp(0), ld(0) {}
  // encoder: PES with the length field and header q (hl bytes; parsed: h), own length: actual
  void Add( const byte* q, qword hl, const PesHdr& h, bool actual ) {
    PesTmpl t; t.b.assign( q, q+hl ); t.ts = h.ts; t.lenmode = actual ? 0 : 1;
    t.ptso = h.ts ? h.ptso+2 : 0; t.dtso = h.ts==3 ? h.dtso+2 : 0;
    if( actual ) t.b[0] = t.b[1] = 0;
    if( h.ts ) memset( &t.b[size_t(t.ptso)], 0, 5 ), pr.push_back( TsDiff( h.pts, lp ) ), lp = h.pts;
    if( h.ts==3 ) memset( &t.b[size_t(t.dtso)], 0, 5 ), dr.push_back( TsDiff( h.dts, ld ) ), ld = h.dts;
    size_t k = 0;
    while( k<tm.size() && !(tm[k].b==t.b && tm[k].ts==t.ts && tm[k].lenmode==t.lenmode && tm[k].ptso==t.ptso && tm[k].dtso==t.dtso) ) k++;
    if( k==tm.size() ) tm.push_back( t );
    ti.push_back( k );
  }
};

static void PutPesModel( Writer& w, const PesModel& m ) {
  w.vint( m.tm.size() );
  for( size_t k=0; k<m.tm.size(); k++ ) {
    const PesTmpl& t = m.tm[k];
    w.vint( t.b.size() ); w.bytes( t.b.data(), t.b.size() ); w.u8( uint( t.ts | t.lenmode<<2 ) );
    if( t.ts ) w.vint( t.ptso );
    if( t.ts==3 ) w.vint( t.dtso );
  }
  PutCol( w, m.ti ); PutCol( w, m.pr ); PutCol( w, m.dr );
}

// npes: PES packets of the track
static bool GetPesModel( Reader& r, PesModel& m, qword npes ) {
  qword nt = r.vint(); if( r.err || nt>npes || nt>MAXCOUNT ) return false;
  m.tm.resize( size_t(nt) );
  qword np = 0, nd = 0;
  for( size_t k=0; k<m.tm.size(); k++ ) {
    PesTmpl& t = m.tm[k]; qword n = r.vint();
    const byte* p = n<2 || n>2+258 ? 0 : r.bytes( n ); if( !p ) return false;
    t.b.assign( p, p+n ); uint f = r.u8(); t.ts = int(f&3); t.lenmode = int(f>>2); t.ptso = t.dtso = 0;
    if( r.err || f>7 || t.ts==2 ) return false;
    if( t.ts ) { t.ptso = r.vint(); if( t.ptso<2 || t.ptso+5>n ) return false; }
    if( t.ts==3 ) { t.dtso = r.vint(); if( t.dtso<2 || t.dtso+5>n ) return false; }
  }
  if( r.err || !GetCol( r, m.ti, npes ) ) return false;
  for( size_t k=0; k<m.ti.size(); k++ ) {
    if( m.ti[k]>=nt ) return false;
    np += m.tm[size_t(m.ti[k])].ts!=0; nd += m.tm[size_t(m.ti[k])].ts==3;
  }
  return GetCol( r, m.pr, np ) && GetCol( r, m.dr, nd );
}

struct PesCursor { qword k, ip, id, lp, ld; PesCursor() : k(0), ip(0), id(0), lp(0), ld(0) {} };

// decoder: the size of the next PES's length field and header (its template), or -1
static long long PesHdrSize( const PesModel& m, const PesCursor& c ) {
  return c.k<m.ti.size() ? (long long)m.tm[size_t(m.ti[size_t(c.k)])].b.size() : -1;
}

// decoder: the timestamps of the next PES's template (0 none, 1 PTS, 3 PTS and DTS), or -1
static int PesTsKind( const PesModel& m, const PesCursor& c ) {
  return c.k<m.ti.size() ? m.tm[size_t(m.ti[size_t(c.k)])].ts : -1;
}

// decoder: the length field and header of the next PES (cursor ip/id into the timestamp
// columns, last PTS / DTS), the packet holding after bytes after the header

static bool PesHeaderNext( const PesModel& m, PesCursor& c, qword after, Buf& o ) {
  if( c.k>=m.ti.size() ) return false;
  const PesTmpl& t = m.tm[size_t(m.ti[size_t(c.k++)])];
  o = t.b;
  qword pl = o.size()-2+after;
  if( t.lenmode==0 ) { if( pl>0xFFFF ) return false; o[0] = byte(pl>>8); o[1] = byte(pl); }
  if( t.ts ) {
    if( c.ip>=m.pr.size() ) return false;
    c.lp = (c.lp+qword(UnZigZag( m.pr[size_t(c.ip++)] ))) & ((qword(1)<<33)-1); PutTs( &o[size_t(t.ptso)], t.ts==3 ? 3 : 2, c.lp );
  }
  if( t.ts==3 ) {
    if( c.id>=m.dr.size() ) return false;
    c.ld = (c.ld+qword(UnZigZag( m.dr[size_t(c.id++)] ))) & ((qword(1)<<33)-1); PutTs( &o[size_t(t.dtso)], 1, c.ld );
  }
  return true;
}

// what an elementary stream of stream id sid (DVD private_stream_1: sub-stream id sub, -1 for
// none) holds, by its first bytes: the extension of its track stream. n: the stream's size; it
// reads no more than its first ES_HEAD bytes (256 KB of video, audio frames from the first 4 KB)
static const qword ES_HEAD = (qword(1)<<18) + (qword(1)<<14);
static std::string EsKind( int sid, int sub, const byte* p, qword n ) {
  if( sid==0xBD && sub>=0 ) return sub>=0x80 && sub<=0x87 ? "ac3" : sub>=0x88 && sub<=0x8F ? "dts" : sub>=0xA0 && sub<=0xA7 ? "pcm" :
                                   sub>=0x20 && sub<=0x3F ? "sub" : "bin";
  if( sid>=0xE0 && sid<=0xEF ) {                         // video: by its start codes in the first 256 KB
    qword m = n<(qword(1)<<18) ? n : qword(1)<<18, tot = 0, vop = 0, pic = 0, slice = 0, seq = 0, ext = 0, hevc = 0, avc = 0;
    for( qword i=Next001( p, 0, m ); i+5<=m; i=Next001( p, i+3, m ) ) {
      byte c = p[i+3]; tot++;
      vop += c==0xB6;                                     // MPEG-4 VOP (reserved in MPEG-1/2, not a NAL header)
      pic += c==0x00; slice += c>=0x01 && c<=0xAF; seq += c==0xB3 || c==0xB8; ext += c==0xB5;
      hevc += (c&0x81)==0 && p[i+4]==1 && ((c>>1)&0x3F)<=40;
      avc += !(c&0x80) && (c&0x1F)>=1 && (c&0x1F)<=12;
    }
    if( vop ) return "m4v";
    if( tot>=4 && hevc==tot ) return "hevc";
    if( seq || (pic && slice>=pic) ) return ext ? "m2v" : "m1v";
    if( tot && 2*avc>=tot ) return "h264";
    if( tot ) return "bin";
  }
  // audio: the first frame header, within the first 4 KB, that another one follows
  for( qword j=0; j<4096 && j+8<=n; j++ ) {
    const byte* q = p+j;
    if( q[0]==0x0B && q[1]==0x77 && (sid==0xBD || sid==0xFD) ) {
      qword u = UnitLen( PT_AC3, 0, q, n-j );
      if( u && (j+u+2>n || (q[u]==0x0B && q[u+1]==0x77)) ) return (q[5]>>3)>10 ? "eac3" : "ac3";
    }
    if( q[0]!=0xFF || (q[1]&0xE0)!=0xE0 || !((sid>=0xC0 && sid<=0xDF) || sid==0xBD || sid==0xFA) ) continue;
    if( (q[1]&0xF6)==0xF0 ) {                             // ADTS
      qword u = UnitLen( PT_ADTS, 0, q, n-j );
      if( u && (j+u+2>n || (q[u]==0xFF && (q[u+1]&0xF6)==0xF0)) ) return "aac";
      continue;
    }
    qword u = UnitLen( PT_MPA, 0, q, n-j );
    if( u && (j+u+2>n || (q[u]==0xFF && (q[u+1]&0xE0)==0xE0)) ) {
      int layer = (q[1]>>1)&3;
      return layer==3 ? "mp1" : layer==2 ? "mp2" : "mp3";
    }
  }
  return "bin";
}

} // namespace mpg
