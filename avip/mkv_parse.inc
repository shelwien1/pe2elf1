// MKV: encoder track setup and element parser
namespace mkv {

//--- encoder -----------------------------------------------------------------

struct Elem { qword pos, hdr, size, end; uint id; int idl, szl; bool unk, trunc; };

// element header at p, bounded by e. Elements running past e get end=e, trunc=true;
// unknown-size elements get end=e, unk=true.
static bool GetElem( const byte* d, qword p, qword e, Elem& x ) {
  if( p>=e ) return false;
  x.pos = p; x.idl = ReadID( d+p, e-p, x.id );
  if( !x.idl ) return false;
  x.szl = ReadVint( d+p+x.idl, e-p-x.idl, x.size, &x.unk );
  if( !x.szl ) return false;
  x.hdr = x.idl+x.szl; x.trunc = false;
  if( x.unk ) x.end = e;
  else if( x.size>e-p-x.hdr ) x.end = e, x.trunc = true;
  else x.end = p+x.hdr+x.size;
  return true;
}

static qword ReadUint( const byte* p, qword n ) { qword v=0; for( qword i=0; i<n && i<8; i++ ) v = (v<<8)|p[i]; return v; }

static double ReadFloat( const byte* p, qword n ) {
  if( n==4 ) { uint u = BE32(p); float f; memcpy( &f, &u, 4 ); return f; }
  if( n==8 ) { qword u = BE64(p); double f; memcpy( &f, &u, 8 ); return f; }
  return 0;
}

struct Track {
  int idx; qword num; bool ok, blob, enc;
  std::string codec, name; const byte* priv; qword privlen; double rate; uint chans;
  int lensize, aacp, xform; Buf xhdr; byte adts[7];
  int fck; uint fckp;                              // frame class kind (CK_*) and parameter, 0 = none
  int cpt, cpp, ccls; uint bits;                   // unit type of the codec (PT_*); codec class for FinishCodec
                                                   // (1 VfW, 2 ACM, 3 MPEG-4/1/2 video, 4 PCM int, 5 PCM float); bit depth
  const byte* vh[3]; qword vl[3]; VorbisInfo vi;   // Ogg: Vorbis header packets (in CodecPrivate)
  long long disc;                                  // DiscardPadding (ns) of the last block
  bool privcut;                                    // CodecPrivate cut from the struct (OP_PRIV)
  std::string ext, xext;                           // stream suffix without / with the transform
  IdxLen xa, xb;                                   // transform side info
  std::vector<qword> foff; std::vector<uint> fsz;  // frames: file offset, size (as stored in the blocks)
  Buf strip;                                       // header stripping (ContentCompAlgo 3): prefix removed from each frame
  Buf sbuf; std::vector<qword> soff; std::vector<uint> ssz;   // with stripping: frames with the prefix restored
  int szmode, ptype, pparam; std::vector<uint> ucnt; std::vector<qword> esc;
  Buf tvb; std::vector<uint> btc, bnf; Buf bfl, breg;   // block header columns, frames per block
  std::vector<qword> bcts, gmode;                  // per block: cluster timestamp base; group column
  qword nbe, nfe; bool haslast; long long lastabs; // layout state: blocks, frames emitted
  int kc[3], dkind; Buf bflx;                      // block op kinds (SB/GB/other), default kind; coded flags
  int tsm, tck, tcp, tk; qword tm, tc;             // timestamp model (see metainfo format)
  std::vector<qword> tsks, tsms, tst;              // piecewise model: scales (k, m), state per block
  int kmode; long long kbias; int seg;             // interleave key; segment
  std::vector<qword> tcol, cpos, lcnt; std::vector<uint> r16v;   // ts column, codec positions, lace counts, residuals
  int km, kp; std::vector<byte> kf;                // key prediction model (see KeyPred), per frame
  std::vector<std::pair<qword,byte> > ksb;         // SimpleBlocks: first frame, key bit
  Buf stream; bool conv;                           // output stream, converted
  int nsb, ngb, nlb; std::string tsinfo;           // statistics
  qword nsamp, bytes, nblk;
  // frame data for analysis and the stream (prefix restored)
  const byte* FB( const byte* d ) const { return strip.size() ? sbuf.data() : d; }
  const std::vector<qword>& FO() const { return strip.size() ? soff : foff; }
  const std::vector<uint>& FZ() const { return strip.size() ? ssz : fsz; }
  uint Size( qword i ) const { return FZ()[size_t(i)]; }
  void Unstrip( const byte* d ) {
    if( strip.empty() ) return;
    qword L = strip.size(), tot = 0;
    for( size_t j=0; j<fsz.size(); j++ ) tot += fsz[j]+L;
    sbuf.clear(); sbuf.reserve( size_t(tot) ); soff.resize( fsz.size() ); ssz.resize( fsz.size() );
    for( size_t j=0; j<fsz.size(); j++ ) {
      soff[j] = sbuf.size(); ssz[j] = uint(fsz[j]+L);
      sbuf.insert( sbuf.end(), strip.begin(), strip.end() ); sbuf.insert( sbuf.end(), d+foff[j], d+foff[j]+fsz[j] );
    }
    bytes = tot;
  }
};

enum { K_BLOCK, K_BLOB, K_PRIV, K_CLUSTER, K_CEND, K_CTS, K_MARK, K_SEG, K_POS, K_CUE, K_VOID, K_SEEK, K_CRC };

struct Blk {                                       // layout item in file order
  qword pos, end; int kind, trk;
  qword szpos, hend, lpos; int szl, nf; bool regen;   // block: size vint, header end, lace header; POS: header end
  qword gpos, gend; int gidl, gszl;                   // enclosing BlockGroup (gend==0: none)
  qword val, seg;                                     // CTS: timestamp; POS: value, segment data start; CEND: cluster pos
  qword ctrack, ccpos, crel, cdur; int cfl;           // CUE: CueTrack, CueClusterPosition, rel. position, duration, flags
};

static Blk Item( int kind, qword pos, qword end, int trk ) {
  Blk b; memset( &b, 0, sizeof(b) );
  b.kind = kind; b.pos = pos; b.end = end; b.trk = trk;
  return b;
}

// zero-length items sharing a position: cluster end, segment start, mark, then the rest
static int Prio( int k ) { return k==K_CEND ? 0 : k==K_SEG ? 1 : k==K_MARK ? 2 : k==K_CLUSTER ? 3 : 4; }   // K_VOID after its K_MARK

static std::string Sanitize( const std::string& s ) {
  std::string r;
  for( size_t i=0; i<s.size() && r.size()<40; i++ ) {
    char c = s[i];
    r += ((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-') ? c : '_';
  }
  return r.empty() ? std::string("data") : r;
}

// MPEG-4 descriptor header: tag + 1..4 byte size

static void SetupCodec( Track& t ) {
  const std::string& c = t.codec; const byte* p = t.priv; qword n = t.privlen;
  t.ext = Sanitize( c ); t.fck = 0; t.fckp = 0;
  if( t.enc ) return;                               // ContentEncodings: frames aren't plain codec data
  if( c.compare(0,11,"V_MPEG4/ISO")==0 && c!="V_MPEG4/ISO/AVC" ) t.fck = CK_M4V, t.cpt = PT_VOP, t.ext = "m4v", t.ccls = 3;
  else if( c=="V_MPEG1" || c=="V_MPEG2" ) t.fck = CK_M2V, t.cpt = PT_M2V, t.ext = "m2v", t.ccls = 3;
  else if( c=="V_MJPEG" ) t.fck = CK_MJPEG, t.cpt = PT_JPEG;
  else if( c=="V_MS/VFW/FOURCC" ) t.ccls = 1;
  else if( c=="A_MS/ACM" ) t.ccls = 2;
  else if( c=="A_PCM/INT/LIT" ) t.ccls = 4;
  else if( c=="A_PCM/FLOAT/IEEE" ) t.ccls = 5;
  if( c=="V_MPEG4/ISO/AVC" && n>=5 ) {
    t.lensize = (p[4]&3)+1;
    if( AvcHdr( p, n, t.xhdr ) ) t.xform = XF_ANNEXB, t.xext = "h264";
  } else if( c=="V_MPEGH/ISO/HEVC" && n>=23 ) {
    t.lensize = (p[21]&3)+1;
    if( HvcHdr( p, n, t.xhdr ) ) t.xform = XF_ANNEXB, t.xext = "hevc";
  } else if( c=="A_AAC" && n>=2 ) {
    t.aacp = AacParamFromAsc( p, n );
    BitRd b( p, n );
    uint aot = b.aot(), sfi = b.get(4), freq = sfi==15 ? b.get(24) : 0, ch = b.get(4);
    if( aot==5 || aot==29 ) { if( b.get(4)==15 ) b.get(24); aot = b.aot(); }
    uint flf = b.get(1);
    if( b.err ) return;
    if( sfi==15 ) for( sfi=0; sfi<13 && aac_rates[sfi]!=freq; sfi++ );
    if( aot>=1 && !flf && AdtsTpl( t.adts, 0, aot-1, sfi, ch ) ) t.xform = XF_ADTS, t.xext = "aac";
  } else if( c.compare(0,6,"A_AAC/")==0 ) {          // legacy IDs: A_AAC/MPEG{2,4}/{MAIN,LC,SSR,LTP}[/SBR]
    int prof = c.find("/MAIN")!=std::string::npos ? 0 : c.find("/LC")!=std::string::npos ? 1 :
               c.find("/SSR")!=std::string::npos ? 2 : c.find("/LTP")!=std::string::npos ? 3 : -1;
    uint sfi; for( sfi=0; sfi<13 && aac_rates[sfi]!=uint(t.rate+0.5); sfi++ );
    if( prof<0 || sfi>12 ) return;
    t.aacp = prof | int(sfi)<<2;
    if( AdtsTpl( t.adts, c.compare(6,5,"MPEG2")==0, uint(prof), sfi, t.chans ) ) t.xform = XF_ADTS, t.xext = "aac";
  } else if( c=="A_MPEG/L3" ) t.ext = "mp3";
  else if( c=="A_MPEG/L2" ) t.ext = "mp2";
  else if( c=="A_MPEG/L1" ) t.ext = "mp1";
  else if( c=="A_AC3" || c.compare(0,6,"A_AC3/")==0 ) t.ext = "ac3";
  else if( c=="A_EAC3" ) t.ext = "eac3";
  else if( c=="A_DTS" || c.compare(0,6,"A_DTS/")==0 ) t.ext = "dts";
  else if( c=="V_MJPEG" ) t.ext = "mjpeg";
  else if( c=="A_FLAC" && n>=8 && !memcmp(p,"fLaC",4) ) t.xform = XF_HDR, t.xhdr.assign( p, p+n ), t.xext = "flac";
  else if( c=="A_VORBIS" && SplitXiphHeaders( p, n, t.vh, t.vl ) && VorbisParse( t.vh, t.vl, t.vi ) ) t.xform = XF_OGG, t.xext = "ogg";
}

struct Parser {
  const byte* d; qword fs;
  std::vector<Track>& T; std::vector<Blk>& B;
  std::vector<std::pair<qword,int> > tmap;           // current segment: track number -> T index
  int ntrk, natt, nseg; qword segst, tcs;            // tcs: TimecodeScale
  std::vector<Elem> L1;                              // level-1 elements (statistics)
  Parser( const byte* _d, qword _fs, std::vector<Track>& _T, std::vector<Blk>& _B ) : d(_d), fs(_fs), T(_T), B(_B), ntrk(0), natt(0), nseg(0), segst(0), tcs(1000000) {}

  static void InitTrack( Track& t ) {
    t.idx = -1; t.num = 0; t.ok = true; t.blob = t.enc = false; t.priv = 0; t.privlen = 0; t.rate = 0; t.chans = 0;
    t.lensize = 0; t.aacp = -1; t.xform = XF_NONE; t.szmode = t.ptype = t.pparam = 0; t.disc = 0; t.privcut = false;
    t.nbe = t.nfe = 0; t.haslast = false; t.lastabs = 0; t.nsb = t.ngb = t.nlb = 0; t.dkind = 0;
    t.tsm = t.tck = t.tcp = t.tk = 0; t.tm = t.tc = 0; t.conv = false; t.kmode = 0; t.kbias = 0; t.seg = 0;
    t.nsamp = t.bytes = t.nblk = 0; t.fck = 0; t.fckp = 0; t.km = t.kp = 0; t.cpt = PT_NONE; t.cpp = 0; t.ccls = 0; t.bits = 0;
  }

  void Tracks( qword s, qword e ) {
    for( Elem x; s<e && GetElem(d,s,e,x) && !x.unk; s = x.end ) {
      if( x.id!=ID_TRACKENTRY ) continue;
      Track t; InitTrack( t ); t.idx = ntrk++; t.seg = nseg>0 ? nseg-1 : 0;
      bool hasnum = false;
      for( qword p=x.pos+x.hdr; ; ) {
        Elem y; if( !GetElem(d,p,x.end,y) || y.unk || y.trunc ) break;
        const byte* v = d+y.pos+y.hdr;
        if( y.id==ID_TRACKNUMBER ) t.num = ReadUint( v, y.size ), hasnum = true;
        else if( y.id==ID_CODECID ) t.codec.assign( (const char*)v, size_t(y.size) ), t.codec = t.codec.c_str();
        else if( y.id==ID_CODECPRIVATE ) t.priv = v, t.privlen = y.size;
        else if( y.id==ID_CONTENTENCODINGS ) t.enc = !StripPrefix( y, t.strip );
        else if( y.id==ID_AUDIO )
          for( qword q=y.pos+y.hdr; ; ) {
            Elem z; if( !GetElem(d,q,y.end,z) || z.unk || z.trunc ) break;
            if( z.id==ID_SAMPLINGFREQ ) t.rate = ReadFloat( d+z.pos+z.hdr, z.size );
            else if( z.id==ID_CHANNELS ) t.chans = uint( ReadUint( d+z.pos+z.hdr, z.size ) );
            else if( z.id==0x6264 ) t.bits = uint( ReadUint( d+z.pos+z.hdr, z.size ) );   // BitDepth
            q = z.end;
          }
        p = y.end;
      }
      if( x.trunc || !hasnum ) t.ok = false;
      for( size_t i=0; i<tmap.size(); i++ ) if( tmap[i].first==t.num ) T[size_t(tmap[i].second)].ok = t.ok = false;  // ambiguous
      SetupCodec( t );
      if( t.xform==XF_OGG && XiphCanonical( t.vh, t.vl, t.priv, t.privlen ) )      // rebuilt from the .ogg on restore
        B.push_back( Item( K_PRIV, qword(t.priv-d), qword(t.priv-d)+t.privlen, int(T.size()) ) );
      tmap.push_back( std::make_pair( t.num, int(T.size()) ) );
      T.push_back( t );
    }
  }

  // ContentEncodings that only strip a fixed prefix from each frame (one encoding: scope frames,
  // compression, ContentCompAlgo 3, ContentCompSettings = the prefix)
  bool StripPrefix( const Elem& y, Buf& pre ) {
    pre.clear();
    int ne = 0; bool ok = false;
    for( qword q=y.pos+y.hdr; ; ) {
      Elem z; if( !GetElem(d,q,y.end,z) || z.unk || z.trunc ) break;
      q = z.end;
      if( z.id!=0x6240 ) continue;                  // ContentEncoding
      ne++; qword scope = 1, type = 0, algo = 0; bool comp = false; Buf set;
      for( qword r=z.pos+z.hdr; ; ) {
        Elem u; if( !GetElem(d,r,z.end,u) || u.unk || u.trunc ) break;
        r = u.end; const byte* v = d+u.pos+u.hdr;
        if( u.id==0x5032 ) scope = ReadUint( v, u.size );
        else if( u.id==0x5033 ) type = ReadUint( v, u.size );
        else if( u.id==0x5035 ) type = 1;
        else if( u.id==0x5034 ) {
          comp = true;
          for( qword s=u.pos+u.hdr; ; ) {
            Elem w; if( !GetElem(d,s,u.end,w) || w.unk || w.trunc ) break;
            s = w.end;
            if( w.id==0x4254 ) algo = ReadUint( d+w.pos+w.hdr, w.size );
            else if( w.id==0x4255 ) set.assign( d+w.pos+w.hdr, d+w.end );
          }
        }
      }
      ok = scope==1 && type==0 && comp && algo==3 && set.size() && set.size()<=255;
      if( ok ) pre.swap( set );
    }
    if( ne!=1 || !ok ) pre.clear();
    return ne==1 && ok;
  }

  void Attachments( qword s, qword e ) {
    for( Elem x; s<e && GetElem(d,s,e,x) && !x.unk && !x.trunc; s = x.end ) {
      if( x.id!=ID_ATTACHEDFILE ) continue;
      std::string fname, mime; Elem data = Elem(); bool hd = false;
      for( qword p=x.pos+x.hdr; ; ) {
        Elem y; if( !GetElem(d,p,x.end,y) || y.unk || y.trunc ) break;
        const char* v = (const char*)d+y.pos+y.hdr;
        if( y.id==ID_FILENAME ) fname.assign( v, size_t(y.size) );
        else if( y.id==ID_FILEMIMETYPE ) mime.assign( v, size_t(y.size) );
        else if( y.id==ID_FILEDATA && !hd ) data = y, hd = true;
        p = y.end;
      }
      if( !hd || data.size==0 ) continue;
      const byte* q = d+data.pos+data.hdr; qword n = data.size;
      std::string ext;
      if( n>=3 && q[0]==0xFF && q[1]==0xD8 && q[2]==0xFF ) ext = "jpg";
      else if( n>=4 && !memcmp(q,"\x89PNG",4) ) ext = "png";
      else if( n>=4 && !memcmp(q,"GIF8",4) ) ext = "gif";
      else if( n>=12 && !memcmp(q,"RIFF",4) && !memcmp(q+8,"WEBP",4) ) ext = "webp";
      else {
        size_t dot = fname.rfind('.');
        ext = dot!=std::string::npos && dot+1<fname.size() ? Sanitize( fname.substr(dot+1) ) : Sanitize( mime );
      }
      Track t; InitTrack( t ); t.blob = true; t.codec = mime;
      if( ext.empty() ) ext = "bin";
      natt++; t.name = ext;
      t.foff.push_back( data.pos+data.hdr ); t.fsz.push_back( 0 ); t.bytes = n;
      B.push_back( Item( K_BLOB, data.pos+data.hdr, data.end, int(T.size()) ) ); T.push_back( t );
    }
  }

  // position elements of SeekHead / Cues: regenerated from the marks when they point at one
  void Pos( const Elem& y ) {
    if( y.size<1 || y.size>8 ) return;
    Blk k = Item( K_POS, y.pos, y.end, -1 );
    k.hend = y.pos+y.hdr; k.val = ReadUintBE( d+y.pos+y.hdr, y.size ); k.seg = segst;
    B.push_back( k );
  }

  // Seek in the form OP_SEEK regenerates: SeekID (1..4 bytes), SeekPosition (1..8), short sizes
  void Seek( const Elem& x ) {
    Elem c[2]; int nc = 0;
    for( qword p=x.pos+x.hdr; p<x.end; ) {
      Elem y; if( nc==2 || !GetElem(d,p,x.end,y) || y.unk || y.trunc || y.szl!=1 ) return;
      c[nc++] = y; p = y.end;
    }
    if( nc!=2 || c[0].id!=0x53AB || c[1].id!=ID_SEEKPOSITION || x.szl!=1 || c[0].size<1 || c[0].size>4 || c[1].size<1 || c[1].size>8 ) return;
    Blk b = Item( K_SEEK, x.pos, x.end, -1 );
    b.val = ReadUintBE( d+c[1].pos+c[1].hdr, c[1].size ); b.seg = segst;
    b.ctrack = ReadUintBE( d+c[0].pos+c[0].hdr, c[0].size ); b.cfl = int(c[0].size-1) | int(c[1].size-1)<<2;
    B.push_back( b );
  }

  void SeekHead( qword s, qword e ) {
    for( qword p=s; p<e; ) {
      Elem x; if( !GetElem(d,p,e,x) || x.unk || x.trunc ) break;
      if( x.id==ID_SEEK ) Seek( x );
      p = x.end;
    }
    for( Elem x; s<e && GetElem(d,s,e,x) && !x.unk && !x.trunc; s = x.end )
      if( x.id==ID_SEEK )
        for( qword p=x.pos+x.hdr; ; ) {
          Elem y; if( !GetElem(d,p,x.end,y) || y.unk || y.trunc ) break;
          if( y.id==ID_SEEKPOSITION ) Pos( y );
          p = y.end;
        }
  }

  // CuePoint in the form OP_CUE regenerates: CueTime, CueTrackPositions{ CueTrack,
  // CueClusterPosition, [CueRelativePosition], [CueDuration] }, all minimal lengths
  void CuePoint( const Elem& x ) {
    if( x.szl!=VintLen(x.size) ) return;
    Elem c[2]; int nc = 0;
    for( qword p=x.pos+x.hdr; p<x.end; ) {
      Elem y; if( nc==2 || !GetElem(d,p,x.end,y) || y.unk || y.trunc ) return;
      c[nc++] = y; p = y.end;
    }
    if( nc!=2 || c[0].id!=0xB3 || c[1].id!=ID_CUETRACKPOSITIONS || c[1].szl!=VintLen(c[1].size) ) return;
    static const uint ids[4] = { 0xF7, ID_CUECLUSTERPOS, 0xF0, 0xB2 };
    qword v[4] = {0,0,0,0}; int k = 0, fl = 0;
    if( c[0].size<1 || c[0].size>8 || c[0].szl!=1 ) return;
    qword tm = ReadUintBE( d+c[0].pos+c[0].hdr, c[0].size );
    if( int(c[0].size)!=UMinLen(tm) ) return;
    for( qword p=c[1].pos+c[1].hdr; p<c[1].end; ) {
      Elem y; if( !GetElem(d,p,c[1].end,y) || y.unk || y.trunc || y.szl!=1 || y.size<1 || y.size>8 ) return;
      while( k<4 && ids[k]!=y.id ) { if( k<2 ) return; k++; }
      if( k==4 ) return;
      v[k] = ReadUintBE( d+y.pos+y.hdr, y.size );
      if( int(y.size)!=UMinLen(v[k]) ) return;
      if( k>=2 ) fl |= 1<<(k-2);
      k++; p = y.end;
    }
    if( k<2 ) return;
    Blk b = Item( K_CUE, x.pos, x.end, -1 );
    b.val = tm; b.seg = segst; b.ctrack = v[0]; b.ccpos = v[1]; b.crel = v[2]; b.cdur = v[3]; b.cfl = fl;
    B.push_back( b );
  }

  void Cues( qword s, qword e ) {
    for( qword p=s; p<e; ) {
      Elem x; if( !GetElem(d,p,e,x) || x.unk || x.trunc ) break;
      if( x.id==ID_CUEPOINT ) CuePoint( x );
      p = x.end;
    }
    for( Elem x; s<e && GetElem(d,s,e,x) && !x.unk && !x.trunc; s = x.end )
      if( x.id==ID_CUEPOINT )
        for( qword p=x.pos+x.hdr; ; ) {
          Elem y; if( !GetElem(d,p,x.end,y) || y.unk || y.trunc ) break;
          if( y.id==ID_CUETRACKPOSITIONS )
            for( qword q=y.pos+y.hdr; ; ) {
              Elem z; if( !GetElem(d,q,y.end,z) || z.unk || z.trunc ) break;
              if( z.id==ID_CUECLUSTERPOS ) Pos( z );
              q = z.end;
            }
          p = y.end;
        }
  }

  void Block( const Elem& x, const Elem* g ) {
    qword ds = x.pos+x.hdr, dl = x.size;
    BlkHdr h; std::vector<qword> ls;
    if( !ParseBlkHdr( d+ds, dl, h, &ls ) || h.hlen>dl ) return;
    int ti = -1;
    for( size_t i=0; i<tmap.size(); i++ ) if( tmap[i].first==h.track ) { ti = tmap[i].second; break; }
    if( ti<0 || !T[size_t(ti)].ok ) return;
    Track& t = T[size_t(ti)];
    qword data = dl-h.hlen, sum = 0;
    std::vector<qword> fz;
    if( h.lacing==0 ) fz.push_back( data );
    else if( h.lacing==2 ) {
      if( data%qword(h.nf) ) return;
      fz.assign( size_t(h.nf), data/qword(h.nf) );
    } else {
      for( size_t i=0; i<ls.size(); i++ ) { sum += ls[i]; if( sum>data ) return; }
      fz = ls; fz.push_back( data-sum );
    }
    std::vector<uint> fu( fz.size() );
    for( size_t i=0; i<fz.size(); i++ ) { if( fz[i]>0xFFFFFFFFu ) return; fu[i] = uint(fz[i]); }
    // track number vint: one encoding per track (blocks using another one stay in the struct)
    const byte* tv = d+ds;
    if( t.tvb.empty() ) t.tvb.assign( tv, tv+h.tvl );
    else if( t.tvb.size()!=size_t(h.tvl) || memcmp( t.tvb.data(), tv, size_t(h.tvl) ) ) return;
    qword lp = ds+h.tvl+3; bool regen = false;
    if( h.lacing ) {
      Buf lb; MakeLace( h.lacing, fu.data(), h.nf, lb );
      regen = lb.size()==ds+h.hlen-lp && !memcmp( lb.data(), d+lp, lb.size() );
      t.breg.push_back( regen );
    }
    t.btc.push_back( BE16(tv+h.tvl) ); t.bfl.push_back( tv[h.tvl+2] ); t.bnf.push_back( uint(h.nf) );
    qword o = ds+h.hlen;
    for( size_t i=0; i<fz.size(); i++ ) t.foff.push_back( o ), t.fsz.push_back( fu[i] ), o += fz[i];
    t.nsamp += fz.size(); t.bytes += data; t.nblk++;
    t.disc = 0;
    if( g )
      for( qword q=g->pos+g->hdr; ; ) {
        Elem y; if( !GetElem(d,q,g->end,y) || y.unk || y.trunc ) break;
        if( y.id==ID_DISCARDPADDING && y.size>=1 && y.size<=8 ) {
          int sh = int(64-8*y.size);
          t.disc = (long long)( ReadUint( d+y.pos+y.hdr, y.size )<<sh ) >> sh;
        }
        q = y.end;
      }
    Blk k = Item( K_BLOCK, x.pos, x.end, ti );
    k.szpos = x.pos+x.idl; k.szl = x.szl; k.hend = ds+h.hlen; k.lpos = lp; k.nf = h.nf; k.regen = regen;
    if( g ) k.gpos = g->pos, k.gend = g->end, k.gidl = g->idl, k.gszl = g->szl;
    B.push_back( k );
  }

  // returns the cluster end (for unknown-size clusters: the first level-1 element)
  qword Cluster( qword s, qword e, bool unk ) {
    qword p = s;
    while( p<e ) {
      Elem x; if( !GetElem(d,p,e,x) ) break;
      if( unk && IsLevel1(x.id) ) return p;
      if( x.trunc || x.unk ) break;
      if( x.id==ID_SIMPLEBLOCK ) Block( x, 0 );
      else if( x.id==ID_BLOCKGROUP ) {
        for( qword q=x.pos+x.hdr; ; ) {
          Elem y; if( !GetElem(d,q,x.end,y) || y.unk || y.trunc ) break;
          if( y.id==ID_BLOCK ) { Block( y, &x ); break; }
          q = y.end;
        }
      } else if( x.id==ID_TIMESTAMP && x.idl==1 && x.size<=8 ) {
        Blk c = Item( K_CTS, x.pos, x.end, -1 ); c.val = ReadUintBE( d+x.pos+x.hdr, x.size ); c.hend = x.pos+x.hdr;
        B.push_back( c );
      }
      p = x.end;
    }
    return unk ? p : e;
  }

  // returns where the segment ended (an unknown-size segment ends at the next EBML header)
  qword Segment( qword s, qword e, bool unk ) {
    tmap.clear();
    qword p = s;
    while( p<e ) {
      Elem x; if( !GetElem(d,p,e,x) ) break;
      if( unk && (x.id==ID_EBML || x.id==ID_SEGMENT) ) return p;
      qword ce = x.end;
      L1.push_back( x );
      if( x.id==ID_CLUSTER && !x.unk && !x.trunc && x.idl==4 && x.szl==VintLen(x.size) ) {
        B.push_back( Item( K_CLUSTER, x.pos, x.pos+x.hdr, -1 ) );            // size regenerated
        Blk c = Item( K_CEND, x.end, x.end, -1 ); c.val = x.pos; B.push_back( c );
      } else B.push_back( Item( K_MARK, x.pos, x.pos, -1 ) );
      if( !x.unk && !x.trunc && x.size>=6 && d[x.pos+x.hdr]==0xBF && d[x.pos+x.hdr+1]==0x84 &&    // valid CRC-32 first child
          LE32( d+x.pos+x.hdr+2 )==Crc32( d+x.pos+x.hdr+6, x.size-6 ) ) {
        Blk c = Item( K_CRC, x.pos+x.hdr, x.pos+x.hdr+6, -1 ); c.val = x.size-6; c.gpos = x.pos; B.push_back( c );
      }
      if( x.id==0xEC && !x.unk && !x.trunc && x.szl<=8 ) {                       // all-zero Void
        qword z = x.pos+x.hdr; while( z<x.end && !d[z] ) z++;
        if( z==x.end ) { Blk v = Item( K_VOID, x.pos, x.end, -1 ); v.val = x.size; v.szl = x.szl; B.push_back( v ); }
      }
      if( x.id==ID_TRACKS && !x.unk ) Tracks( x.pos+x.hdr, x.end );
      else if( x.id==ID_ATTACHMENTS && !x.unk ) Attachments( x.pos+x.hdr, x.end );
      else if( x.id==ID_SEEKHEAD && !x.unk ) SeekHead( x.pos+x.hdr, x.end );
      else if( x.id==ID_INFO && !x.unk )
        for( qword p=x.pos+x.hdr; ; ) {
          Elem y; if( !GetElem(d,p,x.end,y) || y.unk || y.trunc ) break;
          if( y.id==0x2AD7B1 && y.size>=1 && y.size<=8 ) { qword v = ReadUintBE( d+y.pos+y.hdr, y.size ); if( v ) tcs = v; }
          p = y.end;
        }
      else if( x.id==ID_CUES && !x.unk ) Cues( x.pos+x.hdr, x.end );
      else if( x.id==ID_CLUSTER ) ce = Cluster( x.pos+x.hdr, x.end, x.unk );
      else if( x.unk ) break;
      if( ce<=p ) break;
      p = ce;
    }
    return unk ? p : e;
  }

  void File() {
    qword p = 0;
    while( p<fs ) {
      Elem x; if( !GetElem(d,p,fs,x) ) break;
      qword e = x.end;
      if( x.id==ID_SEGMENT ) {
        segst = x.pos+x.hdr; B.push_back( Item( K_SEG, segst, segst, -1 ) ); nseg++;
        e = Segment( x.pos+x.hdr, x.end, x.unk );
      }
      if( e<=p ) break;
      p = e;
    }
  }
};

// choose frame-size representation: derive from frame data if it's self-delimiting
// codec setups that need the frames: VfW / ACM CodecPrivate, codec headers to prepend,
// WAV headers (data size)
static void FinishCodec( const byte* d, Track& t ) {
  if( !t.ccls || t.enc || t.blob ) return;
  const std::vector<uint>& fz = t.FZ();
  size_t k = 0; while( k<fz.size() && !fz[k] ) k++;
  const byte* first = k<fz.size() ? t.FB(d)+t.FO()[k] : 0; qword firstn = k<fz.size() ? fz[k] : 0;
  CodecSetup c;
  if( t.ccls==1 ) VfwSetup( t.priv, t.privlen, first, firstn, c );
  else if( t.ccls==2 ) AcmSetup( t.priv, t.privlen, t.bytes, c );
  else if( t.ccls==3 ) { c.ck = t.fck; c.pt = t.cpt; c.xext = t.ext; if( t.priv ) CodecHdr( c, t.priv, t.privlen, first, firstn ); }
  else PcmSetup( t.ccls==5 ? 3 : 1, t.chans ? t.chans : 1, t.rate>0 ? uint(t.rate+0.5) : 8000, t.bits, t.bytes, c );   // spec defaults
  if( t.ccls<=2 ) t.fck = c.ck==CK_AUDIO || c.ck==CK_RAW ? 0 : c.ck, t.fckp = c.ckp, t.cpt = c.pt, t.cpp = c.pp, t.ext = c.ext;
  if( c.xform==XF_ADTS ) t.xform = XF_ADTS, memcpy( t.adts, c.adts, 7 ), t.xext = c.xext;
  else if( c.xform==XF_HDR ) t.xform = XF_HDR, t.xhdr.swap( c.xhdr ), t.xext = c.xext.size() ? c.xext : t.ext;
  if( t.xform==XF_NONE && c.xext.size() && c.ck!=CK_RAW && c.ck!=CK_AUDIO ) t.ext = c.xext;   // already an elementary stream
  if( c.pt==PT_AAC && t.aacp<0 ) t.aacp = c.pp;
}

static void ChooseSizeMode( const byte* fd, Track& t ) {
  t.szmode = 0; t.ptype = PT_NONE; t.pparam = 0;
  const byte* d = t.FB( fd ); const std::vector<qword>& foff = t.FO(); const std::vector<uint>& fsz = t.FZ();
  qword n = t.nsamp;
  if( n==0 ) return;
  if( t.xform==XF_ADTS || t.xform==XF_OGG ) { t.szmode = 3; return; }   // sizes carried by the transform
  if( t.cpt==PT_VOP || t.cpt==PT_M2V || t.cpt==PT_JPEG || t.cpt==PT_AU264 || t.cpt==PT_ADTS ) {   // start-code units: checked in stream context
    Buf raw; raw.reserve( size_t(t.bytes) );
    if( t.strip.size() ) raw = t.sbuf; else for( size_t j=0; j<n; j++ ) raw.insert( raw.end(), d+foff[j], d+foff[j]+fsz[j] );
    TrackHead h;
    if( ChooseUnits( t.cpt, t.cpp, d, foff, fsz, raw, h ) ) {
      t.szmode = 2; t.ptype = h.ptype; t.pparam = h.pparam;
      t.ucnt.assign( h.col.begin(), h.col.end() ); t.esc = h.esc;
    }
    return;
  }
  int cand[8][2]; int nc=0;
  if( t.cpt==PT_MPA || t.cpt==PT_AC3 ) cand[nc][0]=t.cpt, cand[nc++][1]=t.cpp;
  if( t.aacp>=0 ) cand[nc][0]=PT_AAC, cand[nc++][1]=t.aacp;
  if( t.lensize ) cand[nc][0]=PT_NAL, cand[nc++][1]=t.lensize;
  cand[nc][0]=PT_OBU, cand[nc++][1]=0;
  cand[nc][0]=PT_MPA, cand[nc++][1]=0;
  cand[nc][0]=PT_AC3, cand[nc++][1]=0;
  qword best = n/8+1; int bi=-1;
  std::vector<uint> cnt( (size_t)n ), bcnt;
  for( int k=0; k<nc; k++ ) {
    qword e=0;
    for( qword j=0; j<n && e<best; j++ ) {
      long long u = CountUnits( cand[k][0], cand[k][1], d+foff[size_t(j)], fsz[size_t(j)] );
      if( u<0 || u>=0xFFFFFFFFll ) cnt[size_t(j)]=0, e++; else cnt[size_t(j)]=uint(u+1);
    }
    if( e<best ) { best=e; bi=k; bcnt.swap(cnt); cnt.resize(size_t(n)); if( e==0 ) break; }
  }
  if( bi<0 ) return;
  t.szmode = 2; t.ptype = cand[bi][0]; t.pparam = cand[bi][1];
  t.ucnt.swap( bcnt ); t.esc.clear();
  for( size_t j=0; j<n; j++ ) if( t.ucnt[j]==0 ) t.esc.push_back( fsz[j] );
}

// size vint at f (length l) with value bits stored relative to sub
static void PutResid( Buf& st, const byte* f, int l, qword sub ) {
  qword v; ReadVint( f, l, v );
  byte b[8]; PutVint( b, l, (v-sub)&VintMask(l) );
  st.insert( st.end(), b, b+l );
}

static void ToAnnexB( Track& t, const Buf& s, Buf& c ) {
  ::ToAnnexB( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.lensize, t.xhdr, c, t.xa, t.xb );
}

static void ToADTS( Track& t, const Buf& s, Buf& c ) {
  t.xb.clear(); ::ToADTS( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.adts, c, t.xa );
}

// Vorbis packets -> Ogg Vorbis file (header packets from the codec config).
// trim: samples to cut from the end (lowers the final granulepos)
static void ToOgg( Track& t, const Buf& s, Buf& c, uint serial, qword trim ) {
  ::ToOgg( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.vh, t.vl, t.vi, serial, trim, c );
}

// timestamp column: plain deltas, or the model with the smallest encoding.
// Model candidates: x = frame index or codec position (t.cpos); scale = least-squares
// slope of absolute time over x, and its continued-fraction convergents (exact rates).

} // namespace mkv
