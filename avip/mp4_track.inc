// MP4: box parser, encoder track setup, cover art
namespace mp4 {

//--- encoder -----------------------------------------------------------------

struct Box {
  qword pos, hdr, size; uint type;
  qword pay() const { return pos+hdr; }
  qword plen() const { return size-hdr; }
  qword end() const { return pos+size; }
};

static bool GetBox( const byte* d, qword p, qword end, Box& b ) {
  if( p>end || end-p<8 ) return false;
  qword sz = BE32(d+p); b.type = BE32(d+p+4); b.hdr = 8;
  if( sz==1 ) { if( end-p<16 ) return false; sz = BE64(d+p+8); b.hdr = 16; }
  else if( sz==0 ) sz = end-p;
  if( sz<b.hdr || sz>end-p ) return false;
  b.pos = p; b.size = sz;
  return true;
}

static std::vector<Box> Children( const byte* d, qword s, qword e ) {
  std::vector<Box> r; Box b;
  while( s<e && GetBox(d,s,e,b) ) r.push_back(b), s=b.end();
  return r;
}

static bool FindChild( const byte* d, const Box& par, uint type, Box& out, qword skip=0 ) {
  if( par.plen()<skip ) return false;
  std::vector<Box> c = Children( d, par.pay()+skip, par.end() );
  for( size_t i=0; i<c.size(); i++ ) if( c[i].type==type ) { out=c[i]; return true; }
  return false;
}

static std::string FourccStr( uint t ) {
  std::string s;
  for( int i=24; i>=0; i-=8 ) {
    char c = char(t>>i);
    s += ((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')) ? c : '_';
  }
  return s;
}

struct TrunInfo { Box b; TrunHdr h; };

struct Track {
  int idx; uint tid; bool ok;
  uint fourcc; int lensize;               // NAL length size from avcC/hvcC, 0 if none
  int aacp;                               // AacFrameLen param from esds, -1 if none
  bool has_stbl; Box stsz, stsc, stco; bool stz2, co64; qword nch_stbl;
  qword npc, nstp; bool trunc, strunc;    // present stbl chunks / samples (< total if truncated); stbl truncated
  bool blob; std::string name;            // blob: cut-out struct region (cover art), not a track
  SizeInfo ss; std::vector<uint> fsz;     // stbl sample sizes, fragment sample sizes
  std::vector<TrunInfo> trun;
  std::vector<qword> spc, coff, csz, cfirst;
  int szmode, ptype, pparam; std::vector<uint> ucnt; std::vector<qword> esc;
  int xform; Buf xhdr; byte adts[7];        // transform: param set header / ADTS template
  const byte* vh[3]; qword vl[3]; VorbisInfo vi;   // Ogg: Vorbis header packets (in esds)
  qword dsipos, dsilen;                   // Ogg: esds DSI, cut from the struct when canonical; header transform: the codec config
  qword xdpos, xdlen;                     // video codec config (esds DSI / glbl) file position
  uint vfcc;                              // video: fourcc for FourccSetup (little endian), 0 if none
  uint pcm[4];                            // PCM / G.711: WAV format tag, channels, rate, bits (tag 0 if none)
  int cpt, cpp, fck; uint fckp;           // codec: unit type / param, frame class kind / param
  std::string ext, xext;                  // stream suffix without / with the transform
  IdxLen xa, xb;                          // transform side info
  // timing: per-sample durations / composition offsets / sync flags (stbl samples, then truns)
  Box stts, ctts, stss; bool hstts, hctts, hstss, cstts, cctts, cstss;   // present / regenerable (canonical)
  std::vector<qword> sdur; std::vector<long long> scto; std::vector<byte> ssync;
  bool fcto;                              // a trun has composition offsets
  struct Tfdt { qword pos; int ver; qword val; size_t trun; };
  std::vector<Tfdt> tfdt;
  bool tmok; int tflags; long long tbase; TimeModel tm; FlagMap fm; std::string tinfo;
  bool cstsc, cslast; qword sdi, ts;      // stsc regenerable (canonical, one sdi), last chunk split; mdhd timescale
  PosLen psets;                           // avcC/hvcC parameter set NAL payloads (file positions)
  Box sdtp; bool hsdtp; Buf sdv;          // sdtp box, values per stbl sample
  int cflags, cmode, crule; size_t cmaster; long long ca, cO, cb; std::vector<qword> ccol;
  qword nsamp, bytes;
  uint Size( qword i ) const { return i<ss.count ? ss.get(i) : fsz[size_t(i-ss.count)]; }
  bool AddChunk( qword off, qword n, qword size, qword fs ) {
    if( off>fs || size>fs-off ) return false;
    coff.push_back(off); csz.push_back(size); cfirst.push_back(nsamp); spc.push_back(n);
    nsamp+=n; bytes+=size; return true;
  }
};

struct Item {
  qword off, size, rem; int kind; int trk; qword idx;   // kind 0=chunk, else TB_*; rem = bytes removed from struct
  bool operator<( const Item& b ) const {
    if( off!=b.off ) return off<b.off;
    if( size!=b.size ) return size<b.size;
    if( kind!=b.kind ) return kind<b.kind;
    if( trk!=b.trk ) return trk<b.trk;
    return idx<b.idx;
  }
};

//--- codec configs -> transform setup ---

// MPEG-4 descriptor header: tag + 1..4 byte size
static bool DescHdr( const byte* p, qword n, qword& o, uint& tag, qword& len ) {
  if( o>=n ) return false;
  tag = p[o++]; len = 0;
  for( int i=0;; i++ ) {
    if( i>=4 || o>=n ) return false;
    byte c = p[o++]; len = (len<<7)|(c&0x7F);
    if( !(c&0x80) ) break;
  }
  return len<=n-o;
}

// esds payload -> objectTypeIndication + DecoderSpecificInfo
static bool ParseEsds( const byte* p, qword n, uint& oti, const byte*& dsi, qword& dsilen ) {
  qword o = 4, len; uint tag;
  dsi = 0; dsilen = 0;
  if( !DescHdr(p,n,o,tag,len) || tag!=3 || len<3 ) return false;
  qword e = o+len;
  uint fl = p[o+2]; o += 3;
  if( fl&0x80 ) o+=2;
  if( fl&0x40 ) { if( o>=e ) return false; o += 1+p[o]; }
  if( fl&0x20 ) o+=2;
  while( o<e ) {
    if( !DescHdr(p,e,o,tag,len) ) return false;
    if( tag!=4 ) { o+=len; continue; }
    if( len<13 ) return false;
    oti = p[o];
    qword de = o+len; o += 13;
    while( o<de ) {
      if( !DescHdr(p,de,o,tag,len) ) break;
      if( tag==5 ) { dsi = p+o; dsilen = len; break; }
      o += len;
    }
    return true;
  }
  return false;
}

// mp4a/enca: AacFrameLen param for size dedup; mp4a only: ADTS transform or mp3 naming
static void SetupAudio( const byte* d, const Box& e, Track& t ) {
  if( e.plen()<28 ) return;
  uint ver = BE16( d+e.pay()+8 );                   // QuickTime sound description version
  qword skip = ver==1 ? 44 : ver==2 ? 64 : 28;
  Box esds, wave;
  if( !FindChild(d,e,FCC_BE('e','s','d','s'),esds,skip) ) {
    if( !FindChild(d,e,FCC_BE('w','a','v','e'),wave,skip) || !FindChild(d,wave,FCC_BE('e','s','d','s'),esds) ) return;
  }
  uint oti=0; const byte* dsi; qword dl;
  if( !ParseEsds( d+esds.pay(), esds.plen(), oti, dsi, dl ) ) return;
  bool clear = e.type==FCC_BE('m','p','4','a');       // enca: encrypted, sizes only
  if( oti==0xDD ) {                                 // Vorbis: DSI = Xiph-laced header packets
    if( clear && dsi && SplitXiphHeaders( dsi, dl, t.vh, t.vl ) && VorbisParse( t.vh, t.vl, t.vi ) ) {
      t.xform = XF_OGG; t.xext = "ogg";
      if( XiphCanonical( t.vh, t.vl, dsi, dl ) ) t.dsipos = qword(dsi-d), t.dsilen = dl;
    }
    return;
  }
  if( dsi ) t.aacp = AacParamFromAsc( dsi, dl );
  if( !clear ) return;
  if( oti==0x69 || oti==0x6B ) { t.ext = "mp3"; return; }
  int ok = 0;
  if( dsi && (oti==0x40 || (oti>=0x66 && oti<=0x68)) ) {
    BitRd b( dsi, dl );
    uint aot = b.aot(), sfi = b.get(4), freq = sfi==15 ? b.get(24) : 0, ch = b.get(4);
    if( aot==5 || aot==29 ) { if( b.get(4)==15 ) b.get(24); aot = b.aot(); }   // explicit SBR/PS: core config
    uint flf = b.get(1);                                                      // GASpecificConfig frameLengthFlag
    if( b.err ) return;
    if( aot>=32 && aot<=34 ) { t.ext = "mp3"; return; }                     // MPEG-1/2 layer 1..3
    if( sfi==15 ) for( sfi=0; sfi<13 && aac_rates[sfi]!=freq; sfi++ );       // ADTS: exact rates only
    ok = aot>=1 && !flf && AdtsTpl( t.adts, oti!=0x40, aot-1, sfi, ch );   // ADTS can't signal 960-sample frames
  } else if( !dsi && oti>=0x66 && oti<=0x68 ) {                              // MPEG-2 AAC, no config
    uint ch = BE16(d+e.pay()+16), freq = BE32(d+e.pay()+24)>>16, sfi;
    for( sfi=0; sfi<13 && aac_rates[sfi]!=freq; sfi++ );
    if( sfi<13 ) t.aacp = int(oti-0x66) | int(sfi)<<2;
    ok = AdtsTpl( t.adts, 1, oti-0x66, sfi, ch );
  }
  if( ok ) t.xform = XF_ADTS, t.xext = "aac";
}

// other video (fourcc / esds object type -> FourccSetup) and PCM-like audio (-> WAV header)
static void SetupCodec( const byte* d, const Box& e, Track& t ) {
  uint fc = e.type, lc = (fc>>24)|((fc>>8)&0xFF00)|((fc<<8)&0xFF0000)|(fc<<24);
  const byte* p = d+e.pay(); qword pl = e.plen();
  auto is = [&]( const char* f ) { return fc==FCC_BE(f[0],f[1],f[2],f[3]); };
  auto pre = [&]( const char* f, int n ) { return (fc>>(32-8*n))==(FCC_BE(f[0],f[1],f[2],f[3])>>(32-8*n)); };
  bool aud = is("sowt") || is("twos") || is("lpcm") || is("raw ") || is("ulaw") || is("alaw") || is("in24") || is("in32") ||
             is("fl32") || is("fl64");
  if( !aud && pl>=78 && BE16(p+24)>0 && BE16(p+26)>0 ) {        // visual sample entry: width, height
    Box x; const byte* xd = 0; qword xl = 0; uint oti = 0;
    if( FindChild(d,e,FCC_BE('e','s','d','s'),x,78) && ParseEsds( d+x.pay(), x.plen(), oti, xd, xl ) ) {
      if( is("mp4v") ) lc = oti==0x20 ? FCC_LE('M','P','4','V') : (oti>=0x60 && oti<=0x65) || oti==0x6A ? FCC_LE('M','P','G','2') :
                            oti==0x6C ? FCC_LE('M','J','P','G') : 0;
    } else if( FindChild(d,e,FCC_BE('g','l','b','l'),x,78) ) xd = d+x.pay(), xl = x.plen();
    else if( is("mp4v") ) lc = 0;
    if( is("jpeg") || is("mjpa") ) lc = FCC_LE('M','J','P','G');
    else if( is("m2v1") || is("mp1v") || is("mp2v") || pre("xdv ",3) || pre("xd5 ",3) || pre("hdv ",3) || pre("mx  ",2) )
      lc = FCC_LE('M','P','G','2');
    t.vfcc = lc;
    if( xd ) t.xdpos = qword(xd-d), t.xdlen = xl;
    return;
  }
  if( !aud || pl<28 ) return;
  uint ver = BE16(p+8), ch = BE16(p+16), bits = BE16(p+18), rate = BE32(p+24)>>16, tag = 0;
  auto v2 = [&]() {                                  // version 2 sound description: float64 rate, 32-bit fields
    qword rb = BE64(p+32); double r; memcpy( &r, &rb, 8 );
    ch = BE32(p+40); bits = BE32(p+48);
    rate = r>=1 && r<4e9 && double(uint(r))==r ? uint(r) : 0;
  };
  if( is("sowt") ) tag = bits==16 ? 1 : 0;
  else if( is("raw ") ) tag = bits==8 ? 1 : 0;
  else if( is("ulaw") || is("alaw") ) tag = is("ulaw") ? 7 : 6, bits = 8;
  else if( is("lpcm") ) {
    if( ver==2 && pl>=64 ) {
      uint fl = BE32(p+52); v2();                   // flags: 1 float, 2 big endian, 4 signed integer
      if( !(fl&2) && ( (fl&1) ? bits==32 || bits==64 : bits==8 ? !(fl&4) : (fl&4)!=0 ) ) tag = fl&1 ? 3 : 1;
    }
  } else if( !is("twos") ) {                         // in24 in32 fl32 fl64: little endian if wave/enda says so
    Box w, en; qword skip = ver==1 ? 44 : ver==2 ? 64 : 28;
    if( FindChild(d,e,FCC_BE('w','a','v','e'),w,skip) && FindChild(d,w,FCC_BE('e','n','d','a'),en) && en.plen()>=2 && BE16(d+en.pay()) ) {
      if( ver==2 && pl>=64 ) v2();
      bits = is("in24") ? 24 : is("fl64") ? 64 : 32; tag = (fc>>24)=='f' ? 3 : 1;
    }
  }
  t.pcm[0] = tag; t.pcm[1] = ch; t.pcm[2] = rate; t.pcm[3] = bits;
}

// codec setup that needs the sample data (first frame): size units, frame classes, transforms
static void FinishCodec( const byte* d, Track& t, bool usexf ) {
  if( t.blob || t.xform!=XF_NONE || !t.nsamp ) return;
  CodecSetup c;
  if( t.vfcc ) {
    const byte* first = 0; qword firstn = 0;
    for( size_t k=0; k<t.coff.size() && !first; k++ ) {
      qword o = t.coff[k];
      for( qword j=t.cfirst[k]; j<t.cfirst[k]+t.spc[k]; o+=t.Size(j), j++ ) if( t.Size(j) ) { first = d+o; firstn = t.Size(j); break; }
    }
    FourccSetup( t.vfcc, t.xdlen ? d+t.xdpos : 0, t.xdlen, first, firstn, c );
    t.fck = c.ck; t.fckp = c.ckp; t.cpt = c.pt; t.cpp = c.pp;
    if( c.xext.size() ) t.ext = c.xext;
    if( c.xform==XF_HDR && usexf ) t.xform = XF_HDR, t.xhdr.swap( c.xhdr ), t.xext = t.ext, t.dsipos = t.xdpos, t.dsilen = t.xdlen;
  } else if( t.pcm[0] && usexf ) {
    PcmSetup( t.pcm[0], t.pcm[1], t.pcm[2], t.pcm[3], t.bytes, c );
    if( c.xform==XF_HDR ) t.xform = XF_HDR, t.xhdr.swap( c.xhdr ), t.xext = "wav";
  }
}

static bool SetupTrack( const byte* d, qword fs, const Box& trak, Track& t ) {
  Box tkhd, mdia, minf, stbl, stsd;
  t.tid = 0; t.fourcc = FCC_BE('d','a','t','a'); t.lensize = 0; t.aacp = -1; t.has_stbl = false; t.nch_stbl = 0;
  t.npc = t.nstp = 0; t.trunc = t.strunc = t.blob = false;
  t.nsamp = 0; t.bytes = 0; t.stz2 = t.co64 = false;
  t.xform = XF_NONE; t.dsipos = t.dsilen = 0; t.ext.clear(); t.xext.clear();
  t.hstts = t.hctts = t.hstss = t.cstts = t.cctts = t.cstss = t.fcto = false; t.tmok = false; t.tflags = 0; t.tbase = 0;
  t.cstsc = false; t.sdi = 0; t.ts = 0; t.cflags = t.cmode = t.crule = 0; t.cmaster = 0; t.ca = t.cO = t.cb = 0;
  t.hsdtp = false; t.psets.clear();
  t.xdpos = t.xdlen = 0; t.vfcc = 0; memset( t.pcm, 0, sizeof(t.pcm) ); t.cpt = PT_NONE; t.cpp = 0; t.fck = CK_RAW; t.fckp = 0;
  if( FindChild(d,trak,FCC_BE('t','k','h','d'),tkhd) && tkhd.plen()>=24 )
    t.tid = BE32( d+tkhd.pay()+(d[tkhd.pay()]==1 ? 20 : 12) );
  if( !FindChild(d,trak,FCC_BE('m','d','i','a'),mdia) ) return false;
  { Box mdhd;
    if( FindChild(d,mdia,FCC_BE('m','d','h','d'),mdhd) && mdhd.plen()>=24 )
      t.ts = d[mdhd.pay()]==1 ? (mdhd.plen()>=32 ? BE32(d+mdhd.pay()+20) : 0) : BE32(d+mdhd.pay()+12); }
  if( !FindChild(d,mdia,FCC_BE('m','i','n','f'),minf) ) return false;
  if( !FindChild(d,minf,FCC_BE('s','t','b','l'),stbl) ) return false;

  // codec info from the first sample entry
  if( FindChild(d,stbl,FCC_BE('s','t','s','d'),stsd) && stsd.plen()>=16 ) {
    std::vector<Box> se = Children( d, stsd.pay()+8, stsd.end() );
    if( se.size() ) {
      const Box& e = se[0]; t.fourcc = e.type;
      uint fc = e.type;
      bool avc = fc==FCC_BE('a','v','c','1') || fc==FCC_BE('a','v','c','2') || fc==FCC_BE('a','v','c','3') || fc==FCC_BE('a','v','c','4') ||
                 fc==FCC_BE('d','v','a','1') || fc==FCC_BE('d','v','a','v');
      bool hvc = fc==FCC_BE('h','v','c','1') || fc==FCC_BE('h','e','v','1') || fc==FCC_BE('d','v','h','1') || fc==FCC_BE('d','v','h','e');
      if( e.plen()>=78 ) {                      // visual sample entry: children after 78 bytes
        Box cfg; t.psets.clear();
        if( FindChild(d,e,FCC_BE('a','v','c','C'),cfg,78) && cfg.plen()>=5 ) {
          t.lensize = (d[cfg.pay()+4]&3)+1;
          if( avc && AvcHdr( d+cfg.pay(), cfg.plen(), t.xhdr, &t.psets ) ) t.xform = XF_ANNEXB, t.xext = "h264";
        } else if( FindChild(d,e,FCC_BE('h','v','c','C'),cfg,78) && cfg.plen()>=22 ) {
          t.lensize = (d[cfg.pay()+21]&3)+1;
          if( hvc && HvcHdr( d+cfg.pay(), cfg.plen(), t.xhdr, &t.psets ) ) t.xform = XF_ANNEXB, t.xext = "hevc";
        }
      }
      if( t.xform==XF_ANNEXB ) {                // payload offsets -> file positions
        Box cfg;
        if( FindChild(d,e,FCC_BE('a','v','c','C'),cfg,78) || FindChild(d,e,FCC_BE('h','v','c','C'),cfg,78) )
          for( size_t k=0; k<t.psets.size(); k++ ) t.psets[k].first += cfg.pay();
      } else t.psets.clear();
      if( fc==FCC_BE('m','p','4','a') || fc==FCC_BE('e','n','c','a') ) SetupAudio( d, e, t );
      else if( fc==FCC_BE('a','c','-','3') ) t.ext = "ac3", t.cpt = PT_AC3;
      else if( fc==FCC_BE('e','c','-','3') ) t.ext = "eac3", t.cpt = PT_AC3;
      else if( fc==FCC_BE('.','m','p','3') || fc==FCC_BE('m','s',0,0x55) ) t.ext = "mp3", t.cpt = PT_MPA;
      else if( fc==FCC_BE('.','m','p','2') || fc==FCC_BE('m','s',0,0x50) ) t.ext = "mp2", t.cpt = PT_MPA;
      else if( fc==FCC_BE('.','m','p','1') ) t.ext = "mp1", t.cpt = PT_MPA;
      else if( !avc && !hvc && t.xform==XF_NONE ) SetupCodec( d, e, t );
    }
  }
  if( t.ext.empty() ) t.ext = FourccStr( t.fourcc );

  bool hs = FindChild(d,stbl,FCC_BE('s','t','s','z'),t.stsz);
  if( !hs ) { hs = FindChild(d,stbl,FCC_BE('s','t','z','2'),t.stsz); t.stz2=hs; }
  bool hc = FindChild(d,stbl,FCC_BE('s','t','c','o'),t.stco);
  if( !hc ) { hc = FindChild(d,stbl,FCC_BE('c','o','6','4'),t.stco); t.co64=hc; }
  bool hp = FindChild(d,stbl,FCC_BE('s','t','s','c'),t.stsc);
  if( !hs && !hc && !hp ) return true;          // no sample tables; fragments may follow
  if( !hs || !hc || !hp ) return false;
  t.has_stbl = true;

  const byte* sc = d+t.stco.pay(); qword sl = t.stco.plen();
  if( sl<8 ) return false;
  qword nch = BE32(sc+4), es = t.co64 ? 8 : 4;
  if( 8+es*nch>sl ) return false;
  std::vector<uint> spc;
  if( !ParseSizes( d+t.stsz.pay(), t.stsz.plen(), t.stz2, ~qword(0)>>1, t.ss ) ) return false;
  if( !ParseStsc( d+t.stsc.pay(), t.stsc.plen(), nch, spc ) ) return false;
  t.nch_stbl = nch;
  qword nst = 0;
  for( size_t c=0; c<nch; c++ ) nst += spc[c];
  if( nst!=t.ss.count ) return false;
  for( size_t c=0; c<nch; c++ ) {
    qword off = t.co64 ? BE64(sc+8+8*c) : BE32(sc+8+4*c);
    qword n = spc[c], s=0, si=t.nsamp;
    if( t.ss.csize ) s = n*t.ss.csize; else for( qword k=0; k<n; k++ ) s += t.ss.v[size_t(si+k)];
    if( t.AddChunk( off, n, s, fs ) ) continue;
    // chunk runs past EOF: truncated file. Keep the whole samples that fit, drop the
    // rest (their bytes stay in the struct); the decoder derives the same cut from nsamp.
    qword m=0, b=0;
    if( off<fs ) while( m<n && t.ss.get(si+m)<=fs-off-b ) b += t.ss.get(si+m), m++;
    if( m ) t.AddChunk( off, m, b, fs );
    while( t.spc.size() && t.spc.back()==0 ) t.coff.pop_back(), t.csz.pop_back(), t.cfirst.pop_back(), t.spc.pop_back();
    t.trunc = t.strunc = true;
    break;
  }
  t.npc = t.coff.size(); t.nstp = t.nsamp;
  // timing tables -> per-sample columns; canonical = what the decoder regenerates
  qword nst2 = t.ss.count;
  t.hstts = FindChild(d,stbl,FCC_BE('s','t','t','s'),t.stts);
  t.hctts = FindChild(d,stbl,FCC_BE('c','t','t','s'),t.ctts);
  t.hstss = FindChild(d,stbl,FCC_BE('s','t','s','s'),t.stss);
  if( t.hstts && t.stts.plen()>=8 ) {
    const byte* p = d+t.stts.pay(); qword n = BE32(p+4), tot = 0;
    bool ok = 8+8*n==t.stts.plen(), canon = true;
    for( qword i=0; i<n && ok; i++ ) {
      qword c = BE32(p+8+8*i), v = BE32(p+12+8*i);
      if( c==0 || (i && v==BE32(p+4+8*i)) ) canon = false;
      if( tot+c>nst2 ) { ok = false; break; }
      t.sdur.insert( t.sdur.end(), size_t(c), v ); tot += c;
    }
    if( !ok || tot!=nst2 ) t.sdur.clear(), t.hstts = false; else t.cstts = canon;
  } else t.hstts = false;
  if( t.hctts && t.ctts.plen()>=8 ) {
    const byte* p = d+t.ctts.pay(); qword n = BE32(p+4), tot = 0;
    bool ok = 8+8*n==t.ctts.plen(), canon = true;
    for( qword i=0; i<n && ok; i++ ) {
      qword c = BE32(p+8+8*i); long long v = (long long)int32_t(BE32(p+12+8*i));
      if( c==0 || (i && BE32(p+12+8*i)==BE32(p+4+8*i)) ) canon = false;
      if( tot+c>nst2 ) { ok = false; break; }
      t.scto.insert( t.scto.end(), size_t(c), v ); tot += c;
    }
    if( !ok || tot!=nst2 ) t.scto.clear(), t.hctts = false; else t.cctts = canon;
  } else t.hctts = false;
  if( t.hstss && t.stss.plen()>=8 ) {
    const byte* p = d+t.stss.pay(); qword n = BE32(p+4), prev = 0;
    bool ok = 8+4*n==t.stss.plen();
    t.ssync.assign( size_t(nst2), 0 );
    for( qword i=0; i<n && ok; i++ ) {
      qword v = BE32(p+8+4*i);
      if( v<=prev || v>nst2 ) ok = false; else t.ssync[size_t(v-1)] = 1, prev = v;
    }
    if( !ok ) t.ssync.clear(), t.hstss = false; else t.cstss = true;
  } else t.hstss = false;
  { // stsc canonical: first chunks 1,..., increasing, adjacent entries differ, one sample description
    const byte* p = d+t.stsc.pay(); qword n = BE32(p+4);
    bool ok = 8+12*n==t.stsc.plen() && !t.trunc;
    t.cslast = false;
    for( qword i=0; i<n && ok; i++ ) {
      qword fc = BE32(p+8+12*i), sp = BE32(p+12+12*i), sd = BE32(p+16+12*i);
      if( i==0 ) ok = fc==1, t.sdi = sd;
      else {
        ok = fc>BE32(p+12*i-4) && sd==t.sdi;
        if( sp==BE32(p+12*i) ) { if( i+1==n && fc==nch ) t.cslast = true; else ok = false; }
      }
      if( fc>nch ) ok = false;
    }
    t.cstsc = ok;
  }
  t.hsdtp = FindChild(d,stbl,FCC_BE('s','d','t','p'),t.sdtp) && t.sdtp.plen()==4+nst2 && !t.trunc;
  if( t.hsdtp ) t.sdv.assign( d+t.sdtp.pay()+4, d+t.sdtp.end() );
  if( !t.hstts ) t.sdur.assign( size_t(nst2), 0 );   // no model then (tmok needs stts)
  if( !t.hctts ) t.scto.assign( size_t(nst2), 0 );
  return true;
}

// iTunes cover art: moov/udta/meta/ilst/covr/data (also moov/meta/...) -> blob tracks
static void AddCovers( const byte* d, const Box& moov, std::vector<Track>& T ) {
  std::vector<Box> metas; Box udta, meta;
  if( FindChild(d,moov,FCC_BE('u','d','t','a'),udta) && FindChild(d,udta,FCC_BE('m','e','t','a'),meta) ) metas.push_back(meta);
  if( FindChild(d,moov,FCC_BE('m','e','t','a'),meta) ) metas.push_back(meta);
  for( size_t i=0; i<metas.size(); i++ ) {
    Box ilst;                                       // meta is a FullBox in ISO files, plain in some QT ones
    if( !FindChild(d,metas[i],FCC_BE('i','l','s','t'),ilst,4) && !FindChild(d,metas[i],FCC_BE('i','l','s','t'),ilst,0) ) continue;
    std::vector<Box> il = Children( d, ilst.pay(), ilst.end() );
    for( size_t j=0; j<il.size(); j++ ) {
      if( il[j].type!=FCC_BE('c','o','v','r') ) continue;
      std::vector<Box> cv = Children( d, il[j].pay(), il[j].end() );
      for( size_t k=0; k<cv.size(); k++ ) {
        if( cv[k].type!=FCC_BE('d','a','t','a') || cv[k].plen()<=12 ) continue;
        qword o = cv[k].pay()+8, L = cv[k].plen()-8; const byte* p = d+o;   // skip type + locale
        const char* ext = 0;
        if( p[0]==0xFF && p[1]==0xD8 && p[2]==0xFF ) ext = "jpg";
        else if( !memcmp(p,"\x89PNG",4) ) ext = "png";
        else if( !memcmp(p,"GIF8",4) ) ext = "gif";
        else if( p[0]=='B' && p[1]=='M' ) ext = "bmp";
        else if( !memcmp(p,"RIFF",4) && !memcmp(p+8,"WEBP",4) ) ext = "webp";
        if( !ext ) continue;
        Track t;
        t.idx = -1; t.tid = 0; t.ok = true; t.blob = true; t.trunc = t.strunc = false;
        t.fourcc = FCC_BE('c','o','v','r'); t.lensize = 0; t.aacp = -1;
        t.has_stbl = t.stz2 = t.co64 = false; t.nch_stbl = t.npc = t.nstp = 0;
        t.szmode = t.ptype = t.pparam = 0; t.xform = XF_NONE; t.dsipos = t.dsilen = 0;
        t.hstts = t.hctts = t.hstss = t.cstts = t.cctts = t.cstss = t.fcto = false; t.tmok = false; t.tflags = 0; t.tbase = 0;
        t.cstsc = false; t.sdi = 0; t.ts = 0; t.cflags = t.cmode = t.crule = 0; t.cmaster = 0; t.ca = t.cO = t.cb = 0;
        t.hsdtp = false;
        t.xdpos = t.xdlen = 0; t.vfcc = 0; memset( t.pcm, 0, sizeof(t.pcm) ); t.cpt = PT_NONE; t.cpp = 0; t.fck = CK_RAW; t.fckp = 0;
        t.nsamp = 0; t.bytes = 0;
        t.AddChunk( o, 0, L, ~qword(0) );
        t.name = ext;
        T.push_back( t );
      }
    }
  }
}

struct Trex { uint tid, size, dur; };

// moof/traf/trun: one chunk per trun, appended to its track

} // namespace mp4
