// MPEG transport streams: .ts (188-byte packets), M2TS / Blu-ray .m2ts (192: a 4-byte arrival
// timestamp before each packet)
//
// Track streams hold the PES payloads of each PID that carries PES packets (video, audio,
// private streams), one frame per PES. Regenerated on restore:
//   - PES packets: start code, stream id, the header from the track's PES model (mpg_pes.inc:
//     templates, PTS / DTS differences), the payload: the track's next frame
//   - the packets that carry them: header (PUSI at a PES start, PID, priority, continuity
//     counter + 1); adaptation field: the first packet's from a template given per PES (its
//     PCR from the track's PCR model), stuffing in the packet where the PES ends; payload
//     slices of the PES
//   - PCRs: predicted from the last PCR and its step, from the PES's DTS (muxers that write
//     PCR = DTS - delay), or from the packet position at a constant rate (CBR), residuals kept
//   - null packets (a template), packets of other PIDs that repeat the PID's last packet with
//     the continuity counter + 1 (PAT, PMT, ...)
//   - M2TS arrival timestamps: the last one plus its last step modulo a modulus, residuals kept
// The struct keeps the rest: PSI and other packets as they are, the header and adaptation
// field of PES packets that don't follow the model, bytes outside the packet grid.
//
// meta: u8 packet size (188, 192); 192: vint timestamp modulus, vint packets, column of
//   zigzag timestamp residuals; u8 null template present, [its 188 bytes]
//   vint ntracks; per track: common head (frames: PES payloads), vint PID, u8 stream id,
//   PES model, AF part, column per PES of the first packet's template (0 none, else index+1)
//   vint PCR-only PIDs (adaptation-field-only packets with a PCR, no PES: Blu-ray, AVCHD);
//   per PID: vint PID, AF part
//   AF part: u8 bits 0..3 the continuity counter before the first packet, bit 4 priority,
//   bits 5..6 PCR model (0 last + step, 1 DTS (else PTS) - last offset, 2 position: vint
//   rate in bit/s); adaptation field templates: vint n, n x {u8 size, the fields after the
//   length byte without stuffing, PCR as 0}; vint PCRs, column of zigzag PCR residuals
// ops: 0x00..0x3F  packets of track #code up to the end of its current (or next) PES
//      0x40..0x7F n  n packets of track #(code-0x40)
//      0x80..0xBF  as 0xF6 t 0 with t = code-0x80
//      0xF0 n      n bytes from the struct
//      0xF1 n      n packets from the struct
//      0xF2 t      as 0x00+t, for any track
//      0xF3 t n    as 0x40+t, for any track
//      0xF4 t      packet of track t with its header and adaptation field from the struct
//      0xF5 t a    packet of track t with adaptation field template a
//      0xF6 t a    packet of track t (t >= ntracks: PCR-only PID t-ntracks) with adaptation
//                  field template a and no payload
//      0xF7 n      n null packets
//      0xF8 i      the last struct packet of the i-th PID of struct packets, counter + 1
//      0xFF        the rest of the struct, end
// A track's packets take its PES bytes in turn; a packet that finds the current PES done
// starts the next one (and takes its first-packet template). Track packets carry stuffing
// where the rest of the PES is shorter than their payload space.
namespace ts {

enum { OP_RUNN=0x40, OP_AFONLY0=0x80, OP_RAW=0xF0, OP_RAWPKT=0xF1, OP_RUN=0xF2, OP_RUNNX=0xF3, OP_XPKT=0xF4, OP_AFPKT=0xF5,
       OP_AFONLY=0xF6, OP_NULL=0xF7, OP_REP=0xF8, OP_END=0xFF };

static const qword PCRMAX = (qword(1)<<33)*300;
static const qword NULLRUN = qword(1)<<20;                  // longest run of null packets in one op

// 188 or 192 when packets with their sync bytes start within the first 188 bytes (at least 2,
// up to 8 checked; a file of one packet: 1), else 0
static int PacketSize( const byte* d, qword n ) {
  for( qword s=0; s<188 && s<n; s++ )
    for( int ps=188; ps<=192; ps+=4 ) {
      qword off = ps==192 ? 4 : 0, np = (n-s)/qword(ps), k = 0;
      if( np>8 ) np = 8;
      while( k<np && d[s+off+k*ps]==0x47 ) k++;
      if( np && k==np && (np>=2 || (s==0 && n==qword(ps))) ) return ps;
    }
  return 0;
}

// PCR: 33-bit base, 6 reserved bits (1s), 9-bit extension < 300; v = base*300 + extension
static bool GetPcr( const byte* p, qword& v ) {
  qword base = qword(p[0])<<25 | qword(p[1])<<17 | qword(p[2])<<9 | qword(p[3])<<1 | qword(p[4]>>7);
  uint ext = uint(p[4]&1)<<8 | p[5];
  if( (p[4]&0x7E)!=0x7E || ext>=300 ) return false;
  v = base*300+ext; return true;
}

static void PutPcr( byte* p, qword v ) {
  qword base = v/300; uint ext = uint(v%300);
  p[0] = byte(base>>25); p[1] = byte(base>>17); p[2] = byte(base>>9); p[3] = byte(base>>1);
  p[4] = byte( (base&1)<<7 | 0x7E | ext>>8 ); p[5] = byte(ext);
}

// size of an adaptation field's fields (flags and what they announce, no stuffing), a: the L
// bytes after its length byte; -1 if they don't fit
static long long AfCore( const byte* a, qword L ) {
  if( !L ) return 0;
  uint f = a[0]; qword c = 1;
  if( f&0x10 ) c += 6;
  if( f&0x08 ) c += 6;
  if( f&0x04 ) c += 1;
  if( f&0x02 ) { if( c>=L ) return -1; c += 1+qword(a[c]); }
  if( f&0x01 ) { if( c>=L ) return -1; c += 1+qword(a[c]); }
  return c<=L ? (long long)c : -1;
}

// header and adaptation field of a packet at o (returns their size): PID pid, PUSI pusi,
// priority prio, counter cc, the fields core (af: present), stuffing so that the payload
// takes r bytes when fewer than its space; afonly: no payload, the field fills the packet
static size_t PktHead( byte* o, int pid, bool pusi, int prio, int cc, bool af, const Buf& core, qword r, bool afonly ) {
  qword c = af ? 1+qword(core.size()) : 0, T = c;
  if( afonly ) T = 184;
  else if( r<184-c ) T = 184-r;
  o[0] = 0x47; o[1] = byte( (pusi ? 0x40 : 0) | (prio ? 0x20 : 0) | (pid>>8&0x1F) ); o[2] = byte(pid);
  o[3] = byte( (T ? 0x20 : 0) | (afonly ? 0 : 0x10) | (cc&15) );
  if( T ) {
    o[4] = byte(T-1);
    size_t n = 5;
    if( T>=2 ) {
      if( core.size() ) { memcpy( o+5, core.data(), core.size() ); n += core.size(); }
      else o[n++] = 0;
      while( n<4+T ) o[n++] = 0xFF;
    }
  }
  return size_t(4+T);
}

// PCR models (mode, rate) at packet position pos, ts: the DTS (else PTS) of the track's PES
struct PcrState { qword last, step, off, lpos; PcrState() : last(0), step(0), off(0), lpos(0) {} };

static qword PcrPred( const PcrState& s, int mode, qword rate, qword pos, qword ts ) {
  if( mode==0 ) return s.last+s.step;
  if( mode==1 ) return ts*300-s.off;
  qword dp = pos-s.lpos;
  return dp<(qword(1)<<34) && rate ? s.last+(dp*216000000+rate/2)/rate : s.last;
}

static void PcrUpdate( PcrState& s, qword v, qword pos, qword ts ) { s.step = v-s.last; s.last = v; s.off = ts*300-v; s.lpos = pos; }

// M2TS arrival timestamps: the last one plus its last step, modulo m
static qword AtsPred( qword last, qword step, qword m ) { return (last%m+step)%m; }
static qword AtsStep( qword v, qword last, qword m ) { return (v%m+m-last%m)%m; }

static int UnitType( const std::string& e ) {
  return e=="m1v" || e=="m2v" ? PT_M2V : e=="m4v" ? PT_VOP : e=="h264" ? PT_AU264 : e=="ac3" ? PT_AC3 :
         e=="mp1" || e=="mp2" || e=="mp3" ? PT_MPA : e=="aac" ? PT_ADTS : PT_NONE;
}

//--- decoder -----------------------------------------------------------------

struct Tmpl { Buf core; bool pcr; };

struct DTrack {
  TrackHead h; int pid, sid, cc0, prio, pmode; qword rate;
  mpg::PesModel pm; std::vector<Tmpl> af; std::vector<qword> fa, pcr;
  std::vector<uint> sz, xsz;
};

struct DOp { int code; qword t, n; };

struct Meta {
  const byte* st; qword stlen;
  int ps; qword atsm; std::vector<qword> ats; bool hasnull; Buf null;
  std::vector<DOp> ops; std::vector<DTrack> trk, pcp;         // PES tracks, PCR-only PIDs
};

static bool GetAfPart( Reader& r, DTrack& t ) {
  uint f = r.u8();
  if( r.err || f>=0x60 ) return false;
  t.cc0 = int(f&15); t.prio = int(f>>4&1); t.pmode = int(f>>5); t.rate = 0;
  if( t.pmode==2 ) { t.rate = r.vint(); if( r.err || !t.rate || t.rate>(qword(1)<<40) ) return false; }
  qword na = r.vint(); if( r.err || na>4096 ) return false;
  t.af.resize( size_t(na) );
  for( size_t k=0; k<t.af.size(); k++ ) {
    uint n = r.u8(); const byte* p = n<=183 ? r.bytes( n ) : 0;
    if( !p ) return false;
    t.af[k].core.assign( p, p+n ); t.af[k].pcr = n && (p[0]&0x10);
    if( t.af[k].pcr && n<7 ) return false;
  }
  qword np = r.vint();
  return !r.err && GetCol( r, t.pcr, np );
}

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_TS ) return false;
  Reader r( m.data()+1, m.size()-1 );
  M.ps = int(r.u8()); M.atsm = 0; M.ats.clear();
  if( r.err || (M.ps!=188 && M.ps!=192) ) return false;
  if( M.ps==192 ) {
    M.atsm = r.vint(); qword n = r.vint();
    if( r.err || !M.atsm || M.atsm>(qword(1)<<32) || !GetCol( r, M.ats, n ) ) return false;
  }
  uint hn = r.u8(); M.hasnull = hn==1; M.null.clear();
  if( r.err || hn>1 ) return false;
  if( M.hasnull ) { const byte* p = r.bytes( 188 ); if( !p ) return false; M.null.assign( p, p+188 ); }
  qword nt = r.vint(); if( r.err || nt>8192 ) return false;
  M.trk.resize( size_t(nt) );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i];
    if( !GetHead( r, t.h ) || t.h.xform!=XF_NONE || t.h.szmode==SZ_TABLES || t.h.szmode==SZ_XFORM ) return false;
    qword pid = r.vint(); t.sid = int(r.u8());
    if( r.err || pid>=0x1FFF || t.sid<0xBC ) return false;
    t.pid = int(pid);
    if( !mpg::GetPesModel( r, t.pm, t.h.nf ) || !GetAfPart( r, t ) || !GetCol( r, t.fa, t.h.nf ) ) return false;
    for( size_t k=0; k<t.fa.size(); k++ ) if( t.fa[k]>t.af.size() ) return false;
  }
  qword nq = r.vint(); if( r.err || nq>8192 ) return false;
  M.pcp.resize( size_t(nq) );
  for( size_t i=0; i<M.pcp.size(); i++ ) {
    DTrack& t = M.pcp[i]; qword pid = r.vint();
    if( r.err || pid>=0x1FFF ) return false;
    t.pid = int(pid); t.sid = 0;
    if( !GetAfPart( r, t ) ) return false;
  }
  M.ops.clear();
  for(;;) {
    DOp d; d.code = int(r.u8()); d.t = d.n = 0;
    if( r.err ) return false;
    if( d.code<OP_RUNN ) d.t = qword(d.code), d.code = OP_RUN;
    else if( d.code<OP_AFONLY0 ) d.t = qword(d.code-OP_RUNN), d.n = r.vint(), d.code = OP_RUNNX;
    else if( d.code<0xC0 ) d.t = qword(d.code-OP_AFONLY0), d.code = OP_AFONLY;
    else if( d.code==OP_RUN || d.code==OP_XPKT ) d.t = r.vint();
    else if( d.code==OP_RUNNX || d.code==OP_AFPKT || d.code==OP_AFONLY ) d.t = r.vint(), d.n = r.vint();
    else if( d.code==OP_RAW || d.code==OP_RAWPKT || d.code==OP_NULL || d.code==OP_REP ) d.n = r.vint();
    else if( d.code!=OP_END ) return false;
    if( r.err ) return false;
    if( (d.code==OP_RUN || d.code==OP_RUNNX || d.code==OP_XPKT || d.code==OP_AFPKT) && d.t>=M.trk.size() ) return false;
    if( d.code==OP_AFONLY && d.t>=M.trk.size()+M.pcp.size() ) return false;
    if( d.code==OP_AFPKT && d.n>=M.trk[size_t(d.t)].af.size() ) return false;
    if( d.code==OP_AFONLY && d.n>=(d.t<M.trk.size() ? M.trk[size_t(d.t)] : M.pcp[size_t(d.t-M.trk.size())]).af.size() ) return false;
    if( d.code==OP_NULL && (!M.hasnull || d.n>NULLRUN) ) return false;
    M.ops.push_back( d );
    if( d.code==OP_END ) break;
  }
  M.st = r.p; M.stlen = qword(r.e-r.p);
  return true;
}

static bool Restore( Meta& M, const std::vector<const Buf*>& S, Sink& out, const char** why ) {
  #define FAIL(s) { *why = s; return false; }
  struct St { Buf pes; qword po, k, so, fa, ip, ts; int cc; mpg::PesCursor cur; PcrState pcr; };
  size_t nt = M.trk.size(), na = nt+M.pcp.size();
  std::vector<St> X( na ); std::vector<DTrack*> A( na );
  for( size_t i=0; i<na; i++ ) {
    DTrack& t = *(A[i] = i<nt ? &M.trk[i] : &M.pcp[i-nt]); St& x = X[i];
    if( i<nt && !HeadSizes( t.h, *S[i], t.xsz, t.sz, why ) ) return false;
    x.po = x.k = x.so = x.fa = x.ip = x.ts = 0; x.cc = t.cc0;
  }
  const byte* st = M.st;
  qword pos = 0, sp = 0, ka = 0, alast = 0, astep = 0;
  std::vector<int> rpid; std::vector<Buf> rlast;            // struct packets: PIDs by first appearance, last packet
  Buf hb;
  auto ats = [&]() -> bool {                              // M2TS: the packet's timestamp
    if( M.ps!=192 ) return true;
    if( ka>=M.ats.size() ) FAIL("timestamp overrun");
    qword v = AtsPred( alast, astep, M.atsm )+qword( UnZigZag( M.ats[size_t(ka++)] ) );
    if( v>0xFFFFFFFFu ) FAIL("bad timestamp");
    byte b[4]; PutBE32( b, uint(v) ); out.put( b, 4 ); pos += 4;
    astep = AtsStep( v, alast, M.atsm ); alast = v;
    return true;
  };
  auto startpes = [&]( size_t ti ) -> bool {
    DTrack& t = M.trk[ti]; St& x = X[ti]; const Buf& s = *S[ti];
    if( x.k>=t.h.nf ) FAIL("PES overrun");
    qword z = t.sz[size_t(x.k)];
    if( z>s.size()-x.so ) FAIL("stream too short");
    int tk = mpg::PesTsKind( t.pm, x.cur );
    if( !mpg::PesHeaderNext( t.pm, x.cur, z, hb ) ) FAIL("bad PES header");
    if( tk==3 ) x.ts = x.cur.ld; else if( tk==1 ) x.ts = x.cur.lp;
    x.pes.resize( 4 ); x.pes[0] = 0; x.pes[1] = 0; x.pes[2] = 1; x.pes[3] = byte(t.sid);
    x.pes.insert( x.pes.end(), hb.begin(), hb.end() );
    x.pes.insert( x.pes.end(), s.begin()+size_t(x.so), s.begin()+size_t(x.so+z) );
    x.so += z; x.po = 0; x.fa = t.fa[size_t(x.k)]; x.k++;
    return true;
  };
  // a packet of track ti from the model: AF template a (-1: none, or the PES's first-packet
  // template when it starts one), afonly: no payload
  auto modeled = [&]( size_t ti, int a, bool afonly ) -> bool {
    DTrack& t = *A[ti]; St& x = X[ti];
    bool first = false;
    if( !afonly && x.po>=x.pes.size() ) { if( !startpes( ti ) ) return false; first = true; }
    if( first && a<0 ) a = int(x.fa)-1;
    Buf core;
    if( a>=0 ) {
      const Tmpl& tp = t.af[size_t(a)]; core = tp.core;
      if( tp.pcr ) {
        if( x.ip>=t.pcr.size() ) FAIL("PCR overrun");
        qword v = PcrPred( x.pcr, t.pmode, t.rate, pos, x.ts )+qword( UnZigZag( t.pcr[size_t(x.ip++)] ) );
        if( v>=PCRMAX ) FAIL("bad PCR");
        PutPcr( &core[1], v ); PcrUpdate( x.pcr, v, pos, x.ts );
      }
    }
    int cc = afonly ? x.cc : (x.cc+1)&15;
    x.cc = cc;
    qword r = afonly ? 0 : x.pes.size()-x.po;
    byte h[192]; size_t n = PktHead( h, t.pid, first, t.prio, cc, a>=0, core, r, afonly );
    qword pl = afonly ? 0 : 188-qword(n);
    if( n>188 || pl>r || (!afonly && !pl) ) FAIL("bad packet");
    if( !ats() ) return false;
    out.put( h, n ); out.put( x.pes.data()+x.po, pl ); x.po += pl; pos += 188;
    return true;
  };
  for( size_t i=0; i<M.ops.size(); i++ ) {
    const DOp& d = M.ops[i];
    switch( d.code ) {
    case OP_RAW: case OP_END: {
      qword n = d.code==OP_RAW ? d.n : M.stlen-sp;
      if( n>M.stlen-sp ) FAIL("struct overrun");
      out.put( st+sp, n ); sp += n; pos += n;
    } break;
    case OP_RAWPKT:
      for( qword j=0; j<d.n; j++ ) {
        if( 188>M.stlen-sp ) FAIL("struct overrun");
        const byte* p = st+sp; int pid = int(p[1]&0x1F)<<8 | p[2];
        if( !ats() ) return false;
        out.put( p, 188 ); sp += 188; pos += 188;
        size_t k = 0; while( k<rpid.size() && rpid[k]!=pid ) k++;
        if( k==rpid.size() ) rpid.push_back( pid ), rlast.push_back( Buf() );
        rlast[k].assign( p, p+188 );
      }
      break;
    case OP_REP: {
      if( d.n>=rpid.size() ) FAIL("bad repeat");
      Buf& b = rlast[size_t(d.n)];
      b[3] = byte( (b[3]&0xF0) | ((b[3]+1)&15) );
      if( !ats() ) return false;
      out.put( b.data(), 188 ); pos += 188;
    } break;
    case OP_NULL:
      for( qword j=0; j<d.n; j++ ) { if( !ats() ) return false; out.put( M.null.data(), 188 ); pos += 188; }
      break;
    case OP_RUN: {
      size_t ti = size_t(d.t);
      do { if( !modeled( ti, -1, false ) ) return false; } while( X[ti].po<X[ti].pes.size() );
    } break;
    case OP_RUNNX:
      for( qword j=0; j<d.n; j++ ) if( !modeled( size_t(d.t), -1, false ) ) return false;
      break;
    case OP_AFPKT: case OP_AFONLY:
      if( !modeled( size_t(d.t), int(d.n), d.code==OP_AFONLY ) ) return false;
      break;
    case OP_XPKT: {
      St& x = X[size_t(d.t)];
      if( 4>M.stlen-sp ) FAIL("struct overrun");
      const byte* h = st+sp; int afc = h[3]>>4&3;
      qword T = 0;
      if( !(afc&1) ) FAIL("bad packet");
      if( afc&2 ) { if( 5>M.stlen-sp ) FAIL("struct overrun"); T = 1+qword(h[4]); if( T>184 ) FAIL("bad packet"); }
      if( 4+T>M.stlen-sp ) FAIL("struct overrun");
      qword n = 184-T;
      if( n && x.po>=x.pes.size() && !startpes( size_t(d.t) ) ) return false;
      if( n>x.pes.size()-x.po ) FAIL("bad packet");
      x.cc = h[3]&15;
      if( !ats() ) return false;
      out.put( h, 4+T ); sp += 4+T; out.put( x.pes.data()+x.po, n ); x.po += n; pos += 188;
    } break;
    default: FAIL("bad op");
    }
  }
  if( sp!=M.stlen ) FAIL("struct not consumed");
  if( ka!=M.ats.size() ) FAIL("timestamps not consumed");
  for( size_t i=0; i<na; i++ ) {
    const St& x = X[i];
    if( x.ip!=A[i]->pcr.size() || (i<nt && (x.k!=M.trk[i].h.nf || x.po!=x.pes.size() || x.so!=S[i]->size())) ) FAIL("track not consumed");
  }
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

struct Pes { qword size; int ts; qword pts, dts; size_t pj, pe; uint hl; };   // PES bytes, timestamps (mpg::PesHdr ts);
                                                                             // its packets [pj,pe) of the PID's, header bytes

struct Track {
  int pid, sid, prio, cc0, pmode; qword rate;
  std::vector<Pes> pes; std::vector<uint> fsz; qword sn;   // PES, payload sizes, stream bytes
  const std::vector<size_t>* pk;                            // the PID's packets
  mpg::PesModel pm; std::vector<Tmpl> af; std::vector<qword> fa, pcr;
  std::vector<qword> pv, pp, pt;                            // PCRs: value, packet position, PES timestamp
  std::string ext; TrackHead h;
};

enum { U_BYTES, U_PKT };
// unit: bytes outside the grid, or a packet; for packets: track (-1 none), role (0 other,
// 1 payload of a PES of the track, 2 adaptation field only)
struct Unit { int kind; qword pos, len; int pid, trk, role; };

// packet decisions
enum { P_MODEL, P_AFPKT, P_AFONLY, P_XPKT, P_RAW, P_REP, P_NULL, P_BYTES };
struct Dec { int kind, trk; qword a; bool end; };          // a: template / repeat index / bytes; end: the PES ends here

// the payload of PES k of track t (its packets' payloads after the PES header), appended to o;
// U, d, off: the units, the file, the packet offset in a unit
static void PesPayload( const Track& t, size_t k, const std::vector<Unit>& U, const byte* d, qword off, Buf& o ) {
  const Pes& q = t.pes[k]; const std::vector<size_t>& P = *t.pk; qword skip = q.hl;
  for( size_t m=q.pj; m<q.pe; m++ ) {
    const byte* b = d+U[P[m]].pos+off; int bfc = b[3]>>4&3;
    if( !(bfc&1) ) continue;
    qword T0 = bfc&2 ? 1+qword(b[4]) : 0, L = 184-T0;        // (T0<=184: checked when the PES was taken)
    if( skip>=L ) { skip -= L; continue; }
    o.insert( o.end(), b+4+T0+skip, b+188 ); skip = 0;
  }
}

static void PutAfPart( Writer& w, const Track& t ) {
  w.u8( uint( t.cc0 | t.prio<<4 | t.pmode<<5 ) );
  if( t.pmode==2 ) w.vint( t.rate );
  w.vint( t.af.size() );
  for( size_t k=0; k<t.af.size(); k++ ) w.u8( uint(t.af[k].core.size()) ), w.bytes( t.af[k].core.data(), t.af[k].core.size() );
  w.vint( t.pcr.size() ); PutCol( w, t.pcr );
}

static int FindTmpl( std::vector<Tmpl>& v, const Buf& core ) {
  for( size_t k=0; k<v.size(); k++ ) if( v[k].core==core ) return int(k);
  if( v.size()>=4096 ) return -1;
  Tmpl t; t.core = core; t.pcr = core.size() && (core[0]&0x10); v.push_back( t );
  return int(v.size())-1;
}

// level 2: all models; 1: PES payloads only (track packets with their headers in the struct).
// dry: avip's info pass: no streams (their sizes in *dry), no restore check
static bool Encode1( const Buf& in, int level, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names,
                     std::vector<qword>* dry, const char** why, std::string& log ) {
  const byte* d = in.data(); qword fs = in.size();
  int ps = PacketSize( d, fs ); qword off = ps==192 ? 4 : 0;
  if( !ps ) { *why = "no packets"; return false; }
  // packet grid; anything off it: bytes up to the next position where packets resume
  std::vector<Unit> U;
  for( qword p=0; p<fs; ) {
    if( fs-p>=qword(ps) && d[p+off]==0x47 ) {
      Unit u; u.kind = U_PKT; u.pos = p; u.len = qword(ps); u.pid = int(d[p+off+1]&0x1F)<<8 | d[p+off+2]; u.trk = -1; u.role = 0;
      U.push_back( u ); p += qword(ps); continue;
    }
    qword q = p+1;
    while( q<fs && !(fs-q>=qword(ps) && d[q+off]==0x47 && (fs-q<2*qword(ps) || d[q+qword(ps)+off]==0x47)) ) q++;
    if( fs-q<qword(ps) ) q = fs;
    if( U.size() && U.back().kind==U_BYTES ) U.back().len += q-p;
    else { Unit u; u.kind = U_BYTES; u.pos = p; u.len = q-p; u.pid = -1; u.trk = -1; u.role = 0; U.push_back( u ); }
    p = q;
  }
  // PES per PID: packets from a PUSI to the next; tracks: PIDs with valid PES
  std::map<int,std::vector<size_t> > bypid;
  for( size_t k=0; k<U.size(); k++ ) if( U[k].kind==U_PKT ) bypid[U[k].pid].push_back( k );
  std::vector<Track> T; Buf pb;
  for( std::map<int,std::vector<size_t> >::iterator it=bypid.begin(); it!=bypid.end(); ++it ) {
    int pid = it->first; const std::vector<size_t>& P = it->second;
    if( pid==0x1FFF ) continue;
    Track t; t.pid = pid; t.sid = -1; t.prio = 0; t.cc0 = 0; t.pmode = 0; t.rate = 0; t.sn = 0; t.pk = &P;
    for( size_t j=0; j<P.size(); ) {
      const byte* a = d+U[P[j]].pos+off;
      int afc = a[3]>>4&3;
      if( !(a[1]&0x40) || !(afc&1) ) { j++; continue; }
      // a PES: this packet and the following payload packets up to the next PUSI
      size_t e = j+1; bool bad = (a[1]&0x80) || (a[3]&0xC0);
      std::vector<size_t> mem( 1, P[j] );
      for( ; e<P.size(); e++ ) {
        const byte* b = d+U[P[e]].pos+off; int bfc = b[3]>>4&3;
        if( (b[1]&0x40) && (bfc&1) ) break;
        if( b[1]&0x80 ) bad = true;
        if( bfc&1 ) { if( b[3]&0xC0 ) bad = true; mem.push_back( P[e] ); }
      }
      pb.clear();
      for( size_t m=0; m<mem.size() && !bad; m++ ) {
        const byte* b = d+U[mem[m]].pos+off; int bfc = b[3]>>4&3;
        qword T0 = bfc&2 ? 1+qword(b[4]) : 0;
        if( T0>184 ) { bad = true; break; }
        pb.insert( pb.end(), b+4+T0, b+188 );
      }
      mpg::PesHdr h;
      if( !bad && pb.size()>=6 && !pb[0] && !pb[1] && pb[2]==1 && pb[3]>=0xBC && (t.sid<0 || t.sid==pb[3]) &&
          mpg::ParsePesHdr( pb[3], pb.data()+6, pb.size()-6, h ) ) {
        if( t.sid<0 ) t.sid = pb[3], t.prio = (a[1]>>5)&1, t.cc0 = (a[3]-1)&15;
        bool actual = BE16(pb.data()+4)==pb.size()-6;
        t.pm.Add( pb.data()+4, 2+h.len, h, actual );
        Pes q; q.size = pb.size(); q.ts = h.ts; q.pts = h.pts; q.dts = h.dts; q.pj = j; q.pe = e; q.hl = uint(6+h.len);
        t.pes.push_back( q ); t.fsz.push_back( uint(pb.size()-6-h.len) ); t.sn += pb.size()-6-h.len;
        for( size_t m=0; m<mem.size(); m++ ) U[mem[m]].role = 1, U[mem[m]].trk = int(T.size());
      }
      j = e;
    }
    if( t.sid<0 ) continue;
    for( size_t j=0; j<P.size(); j++ ) {                   // adaptation-field-only packets of the track's PID
      const byte* a = d+U[P[j]].pos+off;
      if( U[P[j]].role==0 && (a[3]>>4&3)==2 && !(a[1]&0xC0) && !(a[3]&0xC0) ) U[P[j]].role = 2, U[P[j]].trk = int(T.size());
    }
    T.push_back( Track() ); T.back() = t;
  }
  // PCR-only PIDs: no PES, adaptation-field-only packets with a PCR
  std::vector<Track> Q;
  for( std::map<int,std::vector<size_t> >::iterator it=bypid.begin(); it!=bypid.end() && level>1; ++it ) {
    const std::vector<size_t>& P = it->second; bool pes = false; long long f0 = -1;
    for( size_t j=0; j<P.size(); j++ ) {
      const byte* a = d+U[P[j]].pos+off;
      if( U[P[j]].role ) pes = true;
      if( f0<0 && (a[3]&0xF0)==0x20 && !(a[1]&0xC0) && a[4] && (a[5]&0x10) ) f0 = (long long)j;
    }
    if( pes || f0<0 || it->first==0x1FFF ) continue;
    Track t; const byte* a = d+U[P[size_t(f0)]].pos+off;
    t.pid = it->first; t.sid = 0; t.prio = (a[1]>>5)&1; t.cc0 = a[3]&15; t.pmode = 0; t.rate = 0; t.sn = 0; t.pk = &P;
    for( size_t j=0; j<P.size(); j++ ) {
      const byte* b = d+U[P[j]].pos+off;
      if( (b[3]&0xF0)==0x20 && !(b[1]&0xC0) ) U[P[j]].role = 2, U[P[j]].trk = int(T.size()+Q.size());
    }
    Q.push_back( t );
  }
  auto TR = [&]( size_t i ) -> Track& { return i<T.size() ? T[i] : Q[i-T.size()]; };
  // track streams (the PES payloads, put together from the packets), codecs, size models
  auto frames = [&]( const Track& t ) {
    FrameSrc r; r.sz = t.fsz.data(); r.gf = [&U,&t,d,off]( qword k, Buf& o ) { PesPayload( t, size_t(k), U, d, off, o ); };
    return r;
  };
  streams.clear(); names.clear(); if( dry ) dry->clear();
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i]; FrameSrc fr = frames( t ); qword nf = t.fsz.size();
    { Buf hb; HeadBytes( fr, nf, mpg::ES_HEAD, hb ); t.ext = mpg::EsKind( t.sid, -1, hb.data(), t.sn ); }
    t.h.ext = t.ext; t.h.xform = XF_NONE; t.h.nf = nf; t.h.szmode = SZ_EXPLICIT; t.h.col.assign( t.fsz.begin(), t.fsz.end() );
    int pt = UnitType( t.ext );
    if( level>1 && pt!=PT_NONE ) ChooseUnits( pt, 0, fr, nf, t.h );
    if( dry ) dry->push_back( t.sn );
    else { streams.push_back( Buf() ); XOut o( &streams.back() ); PutFrames( fr, nf, o ); }
  }
  // the packets in order, as the decoder will see them
  struct St { qword po, size, k, ts; int cc; };
  std::vector<St> X( T.size()+Q.size() );
  for( size_t i=0; i<X.size(); i++ ) X[i].po = X[i].size = X[i].k = X[i].ts = 0, X[i].cc = TR( i ).cc0;
  std::vector<Dec> D; D.reserve( U.size() );
  std::vector<int> rpid; std::vector<const byte*> rlast;
  Buf nulltp; bool hasnull = false;
  std::vector<qword> atsv;
  qword nmod = 0, nafp = 0, nafo = 0, nx = 0, nraw = 0, nrep = 0, nnull = 0, nbytes = 0;
  for( size_t k=0; k<U.size(); k++ ) {
    const Unit& u = U[k]; Dec dc; dc.trk = -1; dc.a = 0; dc.end = false;
    if( u.kind==U_BYTES ) { dc.kind = P_BYTES; dc.a = u.len; D.push_back( dc ); nbytes += u.len; continue; }
    const byte* a = d+u.pos+off;
    if( ps==192 ) atsv.push_back( BE32( d+u.pos ) );
    int afc = a[3]>>4&3; qword T0 = afc&2 ? 1+qword(a[4]) : 0;
    if( u.role==1 ) {
      Track& t = T[size_t(u.trk)]; St& x = X[size_t(u.trk)];
      qword n = 184-T0; bool first = false;
      if( n && x.po>=x.size ) {                           // starts the next PES
        const Pes& q = t.pes[size_t(x.k)];
        x.size = q.size; x.po = 0; x.k++; first = true;
        if( q.ts==3 ) x.ts = q.dts; else if( q.ts==1 ) x.ts = q.pts;
        t.fa.push_back( 0 );
      }
      qword r = x.size-x.po; int cc = (x.cc+1)&15;
      // the fields of its adaptation field as a template (PCR canonical, blanked)
      int tix = -1; long long c = -1; qword pcr = 0;
      if( afc&2 && T0>=1 ) {
        c = AfCore( a+5, T0-1 );
        if( c>0 && (a[5]&0x10) && !GetPcr( a+6, pcr ) ) c = -1;
      }
      bool done = false;
      if( level>1 && n && n<=r ) {
        byte h[192]; Buf none;
        if( PktHead( h, t.pid, first, t.prio, cc, false, none, r, false )==4+T0 && !memcmp( h, a, size_t(4+T0) ) ) {
          dc.kind = P_MODEL; done = true; nmod++;
        } else if( c>=0 ) {
          Buf core( a+5, a+5+c ); if( c>0 && (a[5]&0x10) ) memset( &core[1], 0, 6 );
          tix = FindTmpl( t.af, core );
          if( tix>=0 ) {
            Buf cf( a+5, a+5+c );
            if( PktHead( h, t.pid, first, t.prio, cc, true, cf, r, false )==4+T0 && !memcmp( h, a, size_t(4+T0) ) ) {
              done = true;
              if( first ) dc.kind = P_MODEL, t.fa.back() = qword(tix)+1, nmod++; else dc.kind = P_AFPKT, dc.a = qword(tix), nafp++;
              if( t.af[size_t(tix)].pcr ) t.pv.push_back( pcr ), t.pp.push_back( u.pos ), t.pt.push_back( x.ts );
            }
          }
        }
      }
      if( !done ) { dc.kind = P_XPKT; nx++; }
      dc.trk = u.trk; x.cc = a[3]&15; x.po += n;
      dc.end = x.po>=x.size;
      D.push_back( dc ); continue;
    }
    if( u.role==2 && level>1 ) {
      Track& t = TR( size_t(u.trk) ); St& x = X[size_t(u.trk)];
      long long c = T0==184 ? AfCore( a+5, T0-1 ) : -1; qword pcr = 0;
      if( c>0 && (a[5]&0x10) && !GetPcr( a+6, pcr ) ) c = -1;
      if( c>=0 ) {
        Buf core( a+5, a+5+c ); if( c>0 && (a[5]&0x10) ) memset( &core[1], 0, 6 );
        int tix = FindTmpl( t.af, core );
        byte h[192]; Buf cf( a+5, a+5+c );
        if( tix>=0 && PktHead( h, t.pid, false, t.prio, x.cc, true, cf, 0, true )==188 && !memcmp( h, a, 188 ) ) {
          dc.kind = P_AFONLY; dc.trk = u.trk; dc.a = qword(tix); D.push_back( dc ); nafo++;
          if( t.af[size_t(tix)].pcr ) t.pv.push_back( pcr ), t.pp.push_back( u.pos ), t.pt.push_back( x.ts );
          continue;
        }
      }
    }
    // other packets: null packets, repeats, as they are
    if( level>1 && u.pid==0x1FFF ) {
      if( !hasnull ) nulltp.assign( a, a+188 ), hasnull = true;
      if( !memcmp( nulltp.data(), a, 188 ) ) { dc.kind = P_NULL; D.push_back( dc ); nnull++; continue; }
    }
    size_t ri = 0; while( ri<rpid.size() && rpid[ri]!=u.pid ) ri++;
    if( level>1 && ri<rpid.size() && a[3]==((rlast[ri][3]&0xF0) | ((rlast[ri][3]+1)&15)) && !memcmp( a, rlast[ri], 3 ) &&
        !memcmp( a+4, rlast[ri]+4, 184 ) ) {
      dc.kind = P_REP; dc.a = qword(ri); rlast[ri] = a; D.push_back( dc ); nrep++; continue;
    }
    if( ri==rpid.size() ) rpid.push_back( u.pid ), rlast.push_back( a );
    rlast[ri] = a;
    dc.kind = P_RAW; D.push_back( dc ); nraw++;
  }
  // PCR models: the cheapest of last + step, DTS - offset, position at a constant rate
  for( size_t i=0; i<T.size()+Q.size(); i++ ) {
    Track& t = TR( i ); size_t best = 0;
    long double rt = 0;
    if( t.pv.size()>=2 && t.pv.back()>t.pv[0] ) rt = (long double)(t.pp.back()-t.pp[0])*216000000.0L/(long double)(t.pv.back()-t.pv[0]);
    qword rate = rt>=1 && rt<1e12 ? qword( rt+0.5L ) : 0;
    for( int m=0; m<3; m++ ) {
      if( m==2 && !rate ) continue;
      std::vector<qword> c( t.pv.size() ); PcrState s;
      for( size_t k=0; k<t.pv.size(); k++ ) {
        c[k] = ZigZag( (long long)( t.pv[k]-PcrPred( s, m, rate, t.pp[k], t.pt[k] ) ) );
        PcrUpdate( s, t.pv[k], t.pp[k], t.pt[k] );
      }
      Writer w; PutCol( w, c ); size_t z = w.b.size()+(m==2 ? VintSize( rate ) : 0);
      if( !m || z<best ) best = z, t.pmode = m, t.rate = m==2 ? rate : 0, t.pcr.swap( c );
    }
  }
  // ops: runs of modeled packets of a track (to the end of a PES, or counted), the others one by one
  Writer op; Buf st; st.reserve( size_t(fs>>6) );
  auto trkop = [&]( int base, int ext, size_t t ) { if( t<0x40 ) op.u8( uint(base+int(t)) ); else op.u8( uint(ext) ), op.vint( t ); };
  for( size_t k=0; k<D.size(); ) {
    const Dec& dc = D[k];
    if( dc.kind==P_MODEL ) {
      size_t j = k;
      while( j<D.size() && D[j].kind==P_MODEL && D[j].trk==dc.trk ) {
        size_t e = j; while( e<D.size() && D[e].kind==P_MODEL && D[e].trk==dc.trk && !D[e].end ) e++;
        if( e<D.size() && D[e].kind==P_MODEL && D[e].trk==dc.trk ) { trkop( 0, OP_RUN, size_t(dc.trk) ); j = e+1; continue; }
        if( dc.trk<0x40 ) op.u8( uint(OP_RUNN+dc.trk) ); else op.u8( OP_RUNNX ), op.vint( qword(dc.trk) );
        op.vint( e-j ); j = e;
        break;
      }
      k = j; continue;
    }
    if( dc.kind==P_RAW ) {
      size_t j = k; while( j<D.size() && D[j].kind==P_RAW ) j++;
      op.u8( OP_RAWPKT ); op.vint( j-k );
      k = j; continue;
    }
    if( dc.kind==P_NULL ) {
      size_t j = k; while( j<D.size() && D[j].kind==P_NULL && j-k<NULLRUN ) j++;
      op.u8( OP_NULL ); op.vint( j-k ); k = j; continue;
    }
    if( dc.kind==P_BYTES ) op.u8( OP_RAW ), op.vint( dc.a );
    else if( dc.kind==P_REP ) op.u8( OP_REP ), op.vint( dc.a );
    else if( dc.kind==P_XPKT ) op.u8( OP_XPKT ), op.vint( qword(dc.trk) );
    else if( dc.kind==P_AFONLY && dc.trk<0x40 && !dc.a ) op.u8( uint(OP_AFONLY0+dc.trk) );
    else op.u8( dc.kind==P_AFPKT ? OP_AFPKT : OP_AFONLY ), op.vint( qword(dc.trk) ), op.vint( dc.a );
    k++;
  }
  op.u8( OP_END );
  // struct: bytes off the grid, packets as they are, headers + adaptation fields of explicit packets
  for( size_t k=0; k<D.size(); k++ ) {                     // (a decision per unit)
    const Unit& u = U[k]; const byte* a = d+u.pos+off;
    if( D[k].kind==P_BYTES ) st.insert( st.end(), d+u.pos, d+u.pos+u.len );
    else if( D[k].kind==P_RAW ) st.insert( st.end(), a, a+188 );
    else if( D[k].kind==P_XPKT ) { qword T0 = (a[3]&0x20) ? 1+qword(a[4]) : 0; st.insert( st.end(), a, a+4+T0 ); }
  }
  // meta
  Writer w; PutMetaHdr( w, FMT_TS );
  w.u8( uint(ps) );
  size_t zats = 0;
  if( ps==192 ) {                                         // timestamp modulus: the cheapest of 2^30, 2^30-1, 2^32
    static const qword mods[3] = { qword(1)<<30, (qword(1)<<30)-1, qword(1)<<32 };
    size_t best = 0; std::vector<qword> bc; qword bm = 0;
    for( int m=0; m<3; m++ ) {
      std::vector<qword> c( atsv.size() ); qword l = 0, s = 0;
      for( size_t k=0; k<atsv.size(); k++ ) {
        c[k] = ZigZag( (long long)atsv[k]-(long long)AtsPred( l, s, mods[m] ) ); s = AtsStep( atsv[k], l, mods[m] ); l = atsv[k];
      }
      Writer t; PutCol( t, c );
      if( !m || t.b.size()<best ) best = t.b.size(), bc.swap( c ), bm = mods[m];
    }
    w.vint( bm ); w.vint( bc.size() ); PutCol( w, bc ); zats = w.b.size()-2;
  }
  w.u8( hasnull ? 1 : 0 ); if( hasnull ) w.bytes( nulltp.data(), 188 );
  w.vint( T.size() );
  size_t zhead = 0, zpes = 0, zaf = 0;
  for( size_t i=0; i<T.size(); i++ ) {
    Track& t = T[i];
    size_t z0 = w.b.size();
    PutHead( w, t.h ); w.vint( qword(t.pid) ); w.u8( uint(t.sid) );
    size_t z1 = w.b.size();
    mpg::PutPesModel( w, t.pm );
    size_t z2 = w.b.size();
    PutAfPart( w, t ); PutCol( w, t.fa );
    zhead += z1-z0; zpes += z2-z1; zaf += w.b.size()-z2;
    names.push_back( TrackSuffix( i, t.ext ) );
    if( verbose ) {
      static const char* pmn[3] = { "step", "dts", "rate" };
      char l[240];
      snprintf( l, sizeof(l), "pid %04X %02X: %-4s %8llu PES %11llu bytes  sizes=%s templates=%llu af=%llu pcr=%llu/%s -> %s\n", t.pid, t.sid,
        t.ext.c_str(), (unsigned long long)t.fsz.size(), (unsigned long long)t.sn, t.h.szmode==SZ_UNITS ? ptname[t.h.ptype] : "explicit",
        (unsigned long long)t.pm.tm.size(), (unsigned long long)t.af.size(), (unsigned long long)t.pcr.size(), pmn[t.pmode], names[i].c_str() );
      log += l;
    }
  }
  w.vint( Q.size() );
  for( size_t i=0; i<Q.size(); i++ ) {
    const Track& t = Q[i]; size_t z0 = w.b.size();
    w.vint( qword(t.pid) ); PutAfPart( w, t );
    zaf += w.b.size()-z0;
    if( verbose ) {
      static const char* pmn[3] = { "step", "dts", "rate" };
      char l[160];
      snprintf( l, sizeof(l), "pid %04X: PCR only, af=%llu pcr=%llu/%s\n", t.pid, (unsigned long long)t.af.size(),
        (unsigned long long)t.pcr.size(), pmn[t.pmode] );
      log += l;
    }
  }
  if( verbose ) {
    char l[300];
    snprintf( l, sizeof(l), "layout: %d-byte packets: %llu modeled, %llu with an AF template, %llu AF only, %llu explicit, %llu as they are, "
      "%llu repeats, %llu null; %llu bytes off the grid\n", ps, (unsigned long long)nmod, (unsigned long long)nafp, (unsigned long long)nafo,
      (unsigned long long)nx, (unsigned long long)nraw, (unsigned long long)nrep, (unsigned long long)nnull, (unsigned long long)nbytes );
    log += l;
    snprintf( l, sizeof(l), "meta: timestamps %llu, heads %llu, PES models %llu, AF/PCR %llu, ops %llu, struct %llu\n", (unsigned long long)zats,
      (unsigned long long)zhead, (unsigned long long)zpes, (unsigned long long)zaf, (unsigned long long)op.b.size(), (unsigned long long)st.size() );
    log += l;
  }
  w.bytes( op.b.data(), op.b.size() ); w.bytes( st.data(), st.size() );
  meta.swap( w.b );
  if( dry ) return true;
  Meta M; Sink chk( 0, &in ); *why = "meta parse";
  return ParseMeta( meta, M ) && Decode( M, streams, chk, why );
}

static bool Encode( const Buf& in, Buf& meta, std::vector<Buf>& streams, std::vector<std::string>& names, std::vector<qword>* dry ) {
  const char* why = ""; std::string log;
  for( int level=2; level>=1; level-- ) {
    log.clear();
    if( Encode1( in, level, meta, streams, names, dry, &why, log ) && names.size() ) {
      if( verbose ) fputs( log.c_str(), stdout );
      return true;
    }
    if( names.size() ) fprintf( stderr, "warning: verify failed (%s)%s\n", why, level>1 ? ", retrying with fewer models" : "" );
  }
  meta.clear(); streams.clear(); names.clear(); if( dry ) dry->clear();
  return false;
}

} // namespace ts
