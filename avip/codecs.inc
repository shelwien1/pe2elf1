// codec mapping shared by the containers: VfW (BITMAPINFOHEADER) and ACM (WAVEFORMATEX)
// formats, used by AVI strf, MKV V_MS/VFW/FOURCC and A_MS/ACM, and native codec IDs that
// map to the same handling. Gives the frame class kind, the self-delimiting unit type,
// the stream extension and the transform (codec header / WAV header prepended, ADTS).

struct CodecSetup {
  int ck; uint ckp;                 // frame class kind (CK_*) and parameter
  int pt, pp;                       // unit type (PT_*) and parameter, PT_NONE if not self-delimiting
  std::string ext, xext;            // stream extension raw / with the transform
  int xform; Buf xhdr; byte adts[7];
  CodecSetup() : ck(CK_RAW), ckp(0), pt(PT_NONE), pp(0), xform(XF_NONE) { memset( adts, 0, 7 ); }
};

static std::string FccStr( uint f ) { std::string s; for( int i=0; i<4; i++ ) s += char(f>>(8*i)); return s; }

static bool FccIn( uint f, const char* list ) {      // case-insensitive, list of 4-char codes
  char u[5]; for( int i=0; i<4; i++ ) { char c = char(f>>(8*i)); u[i] = c>='a'&&c<='z' ? char(c-32) : c; } u[4] = 0;
  for( const char* p=list; *p; p+=4 ) if( !memcmp( p, u, 4 ) ) return true;
  return false;
}

static std::string ExtSanitize( const std::string& s, bool dash ) {
  std::string r;
  for( size_t i=0; i<s.size() && r.size()<40; i++ ) {
    char c = s[i];
    r += ((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||(dash && c=='-')) ? c : '_';
  }
  return r.empty() ? std::string("data") : r;
}

// sequence-level headers present (VOL / SPS / VPS..PPS / sequence header)?
static bool SeqHdr( int ck, const byte* p, qword n ) {
  for( qword sc=Next001(p,0,n); sc+4<n; sc=Next001(p,sc+3,n) ) {
    byte c = p[sc+3];
    if( ck==CK_M4V && c>=0x20 && c<=0x2F ) return true;
    if( ck==CK_H264 && (c&31)==7 ) return true;
    if( ck==CK_HEVC && ((c>>1)&63)==33 ) return true;
    if( ck==CK_M2V && c==0xB3 ) return true;
    if( ck==CK_VC1 && c==0x0F ) return true;
  }
  return false;
}

// VC-1 sequence flags from extradata: WMV3 STRUCT_C (32 bits), or an advanced profile
// sequence header (00 00 01 0F) anywhere in it (WVC1/WMVA)
static uint VC1Param( uint c, const byte* x, qword xl ) {
  if( c!=FCC_LE('W','M','V','3') && c!=FCC_LE('w','m','v','3') ) {
    for( qword i=0; i+10<=xl; i++ )
      if( x[i]==0 && x[i+1]==0 && x[i+2]==1 && x[i+3]==0x0F ) {
        // PROFILE 2, LEVEL 3, COLORDIFF 2, FRMRTQ 3, BITRTQ 5, POSTPROCFLAG 1, MAX_CODED_WIDTH 12,
        // MAX_CODED_HEIGHT 12, PULLDOWN 1, INTERLACE 1 -> bit 41
        const byte* q = x+i+4;
        return VC1_ADV | ( ((q[5]>>6)&1) ? VC1_INTERLACE : 0 );
      }
    return VC1_ADV;
  }
  if( xl<4 ) return 0;
  uint s = (uint(x[0])<<24)|(uint(x[1])<<16)|(uint(x[2])<<8)|x[3];
  if( (s>>30)==3 ) return VC1_ADV;
  return ( (s>>1)&1 ? VC1_FINTERP : 0 ) | ( (s>>7)&1 ? VC1_RANGERED : 0 ) | ( (s>>4)&7 ? VC1_BFRAMES : 0 );
}

// codec headers x (start-code ES syntax) prepended when the first frame lacks them
static void CodecHdr( CodecSetup& c, const byte* x, qword xl, const byte* first, qword firstn ) {
  if( c.ck==CK_RAW || c.ck==CK_MJPEG || xl<5 ) return;
  bool sc = x[0]==0 && x[1]==0 && (x[2]==1 || (x[2]==0 && x[3]==1));
  if( sc && first && firstn && !SeqHdr( c.ck, first, firstn ) ) c.xhdr.assign( x, x+xl ), c.xform = XF_HDR;
}

// video by fourcc; x/xl: codec extradata; first: first non-empty frame (or null)
static void FourccSetup( uint c, const byte* x, qword xl, const byte* first, qword firstn, CodecSetup& s ) {
  s.ext = c==1 ? "rle8" : c==2 ? "rle4" : c<256 ? "dib" : ExtSanitize( FccStr(c), false );
  if( FccIn( c, "XVIDDIVXDX50FMP4MP4V3IV2M4S2MP4SRMP4DIV6SEDGWV1FUMP4BLZ0DXGMM4CCDIVF" ) ) s.ck = CK_M4V, s.pt = PT_VOP, s.xext = "m4v";
  else if( FccIn( c, "MP42MP43DIV2DIV3DIV4DIV5MPG3AP41COL1DVX3WMV1" ) ) s.ck = CK_MSM;
  else if( FccIn( c, "WMV2" ) ) s.ck = CK_WMV2;
  else if( FccIn( c, "WMV3WVC1WMVA" ) ) {
    s.ck = CK_VC1; s.ckp = VC1Param( c, x, xl );
    if( (s.ckp&VC1_ADV) && first && firstn>=4 && Next001( first, 0, firstn )==0 ) s.xext = "vc1";   // SMPTE 421M Annex E
  }
  else if( FccIn( c, "H264X264AVC1VSSHDAVCAVC H26LX26L" ) ) s.ck = CK_H264, s.pt = PT_AU264, s.xext = "h264";
  else if( FccIn( c, "HEVCH265X265HVC1HEV1" ) ) s.ck = CK_HEVC, s.xext = "hevc";
  else if( FccIn( c, "MJPGAVRNLJPGJPGLDMB1AVDJMJPA" ) ) s.ck = CK_MJPEG, s.pt = PT_JPEG, s.xext = "mjpeg";
  else if( FccIn( c, "MPG1MPG2MPEGPIM1PIM2MMESMX3PMX5PHDV1HDV2HDV3HDV5HDV6HDV7HDV8HDV9" ) ) s.ck = CK_M2V, s.pt = PT_M2V, s.xext = "m2v";
  CodecHdr( s, x, xl, first, firstn );
}

// BITMAPINFOHEADER f (fl bytes, extradata after biSize)
static void VfwSetup( const byte* f, qword fl, const byte* first, qword firstn, CodecSetup& s ) {
  if( fl<40 ) return;
  qword bs = LE32(f); if( bs<40 || bs>fl ) bs = 40;
  FourccSetup( LE32(f+16), f+bs, fl-bs, first, firstn, s );
}

// WAV file header for a WAVEFORMATEX f (fl bytes) and dl data bytes
static void WavHeader( const byte* f, qword fl, qword dl, Buf& h ) {
  qword wl = fl + (fl&1);
  h.assign( size_t( 20+wl+8 ), 0 );
  memcpy( &h[0], "RIFF", 4 ); memcpy( &h[8], "WAVEfmt ", 8 ); PutLE32( &h[16], uint(fl) );
  memcpy( &h[20], f, size_t(fl) );
  memcpy( &h[size_t(20+wl)], "data", 4 );
  PutLE32( &h[size_t(24+wl)], dl>0xFFFFFFFFu ? 0xFFFFFFFFu : uint(dl) );
  qword rl = h.size()-8+dl; PutLE32( &h[4], rl>0xFFFFFFFFu ? 0xFFFFFFFFu : uint(rl) );
}

// ADTS from an AudioSpecificConfig if it allows (else a raw AAC unit parser)
static void AacSetup( const byte* asc, qword al, CodecSetup& s ) {
  int ap = AacParamFromAsc( asc, al );
  BitRd r( asc, al );
  uint aot = r.aot(), sfi = r.get(4), freq = sfi==15 ? r.get(24) : 0, ch = r.get(4);
  if( aot==5 || aot==29 ) { if( r.get(4)==15 ) r.get(24); aot = r.aot(); }
  uint flf = r.get(1);
  if( sfi==15 ) for( sfi=0; sfi<13 && aac_rates[sfi]!=freq; sfi++ );
  if( !r.err && aot>=1 && !flf && AdtsTpl( s.adts, 0, aot-1, sfi, ch ) ) s.xform = XF_ADTS, s.xext = "aac";
  if( s.xform!=XF_ADTS && ap>=0 ) s.pt = PT_AAC, s.pp = ap;
}

// WAVEFORMATEX f (fl bytes); bytes: total audio data (WAV header)
static void AcmSetup( const byte* f, qword fl, qword bytes, CodecSetup& s ) {
  if( fl<14 ) return;
  uint tag = LE16(f); uint cb = fl>=18 ? LE16(f+16) : 0;
  char b[16]; snprintf( b, sizeof(b), "a%04x", tag ); s.ext = b;
  s.ck = CK_AUDIO;
  if( tag==0x55 ) s.pt = PT_MPA, s.ext = "mp3";
  else if( tag==0x50 ) s.pt = PT_MPA, s.ext = "mp2";
  else if( tag==0x2000 ) s.pt = PT_AC3, s.ext = "ac3";
  else if( tag==0x2001 ) s.ext = "dts";
  else if( tag==0x1600 ) s.pt = PT_ADTS, s.ext = "aac";
  else if( tag==0xFF || tag==0x1610 || tag==0x706D || tag==0x4143 ) {
    const byte* asc = 0; qword al = 0;
    if( tag==0x1610 && fl>=30 && cb>=12 ) {            // HEAACWAVEINFO: wPayloadType 0 = raw, 1 = ADTS
      uint pt = LE16(f+18);
      if( pt==1 ) { s.pt = PT_ADTS; s.ext = "aac"; }
      else if( pt==0 && fl>30 ) asc = f+30, al = std::min( fl, qword(18+cb) )-30;
    } else if( fl>18 && cb>=2 ) asc = f+18, al = std::min( fl, qword(18+cb) )-18;
    if( asc && al>=2 ) AacSetup( asc, al, s );
  } else if( tag==1 || tag==3 || tag==2 || tag==0x11 || tag==6 || tag==7 || tag==0x31 || tag==0xFFFE || tag==0x22 || tag==0x40 || tag==0x45 ) {
    WavHeader( f, fl, bytes, s.xhdr ); s.xform = XF_HDR; s.xext = "wav";   // PCM-like
  }
}

// PCM from channel count, rate and sample format (fmt 1 integer, 3 float): WAV header
static void PcmSetup( uint fmt, uint ch, uint rate, uint bits, qword bytes, CodecSetup& s ) {
  if( !ch || !rate || !bits || bits%8 || ch>64 || bits>64 ) return;
  byte w[18]; uint ba = ch*bits/8;
  PutLE32( w, fmt | ch<<16 ); PutLE32( w+4, rate ); PutLE32( w+8, rate*ba ); PutLE32( w+12, ba | bits<<16 ); w[16] = w[17] = 0;
  WavHeader( w, 16, bytes, s.xhdr ); s.xform = XF_HDR; s.xext = "wav"; s.ck = CK_AUDIO;
}
