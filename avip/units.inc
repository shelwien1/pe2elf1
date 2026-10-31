// self-delimiting units: frame sizes derived from the frame data. Start-code based units
// (H.264 access units, MPEG-4 VOPs, MPEG-1/2 pictures, JPEG images) end where the next
// unit begins, so their length depends on the following bytes; encoders check every
// derived size in stream context, like the decoder.

enum { PT_NONE=0, PT_NAL=1, PT_OBU=2, PT_MPA=3, PT_AC3=4, PT_AAC=5, PT_ADTS=6, PT_AU264=7, PT_VOP=8, PT_JPEG=9, PT_M2V=10 };
static const char* ptname[] = { "table", "nal", "obu", "mpa", "ac3", "aac", "adts", "h264au", "vop", "jpeg", "m2v" };

// position of the next 00 00 00 01 at or after s, or n
static qword NextSC( const byte* p, qword s, qword n ) {
  qword i = s;
  while( i+4<=n ) {
    byte c = p[i+3];
    if( c>1 ) i+=4;                        // no match can cover p[i+3]
    else if( c==0 ) i++;
    else { if( p[i]==0 && p[i+1]==0 && p[i+2]==0 ) return i; i+=4; }
  }
  return n;
}

// next 00 00 01 at or after s, or n
static qword Next001( const byte* p, qword s, qword n ) {
  qword i = s;
  while( i+3<=n ) {
    byte c = p[i+2];
    if( c>1 ) i += 3;
    else if( c==0 ) i++;
    else { if( p[i]==0 && p[i+1]==0 ) return i; i += 3; }
  }
  return n;
}

// length of one unit at p (avail bytes available), 0 on failure
static qword UnitLen( int type, int param, const byte* p, qword avail ) {
  qword u = 0;
  switch( type ) {
  case PT_NAL: {
    if( avail<qword(param) ) return 0;
    qword L=0; for( int i=0; i<param; i++ ) L = (L<<8)|p[i];
    u = param+L;
  } break;
  case PT_OBU: {
    if( avail<1 ) return 0;
    uint h = p[0];
    if( (h&0x80) || !(h&2) ) return 0;    // forbidden bit / no size field
    qword o = 1+((h>>2)&1), L=0;
    for( int i=0;; i++ ) {
      if( i>=8 || o>=avail ) return 0;
      byte c = p[o++]; L |= qword(c&0x7F)<<(7*i);
      if( !(c&0x80) ) break;
    }
    u = o+L;
  } break;
  case PT_MPA: {
    static const short br[2][3][16] = {
      { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},   // v1 L1
        {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0},      // v1 L2
        {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0} },     // v1 L3
      { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},      // v2 L1
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},           // v2 L2
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0} } };       // v2 L3
    static const int sr[3] = { 44100, 48000, 32000 };
    if( avail<4 ) return 0;
    if( p[0]!=0xFF || (p[1]&0xE0)!=0xE0 ) return 0;
    int ver = (p[1]>>3)&3, lay = (p[1]>>1)&3, bi = p[2]>>4, si = (p[2]>>2)&3, pad = (p[2]>>1)&1;
    if( ver==1 || lay==0 || bi==0 || bi==15 || si==3 ) return 0;
    int v2 = ver!=3, L = 3-lay;
    qword b = br[v2][L][bi]*1000, f = sr[si] >> (ver==3 ? 0 : ver==2 ? 1 : 2);
    if( L==0 ) u = (12*b/f+pad)*4;
    else if( L==2 && v2 ) u = 72*b/f+pad;
    else u = 144*b/f+pad;
  } break;
  case PT_AC3: {
    if( avail<6 ) return 0;
    if( p[0]!=0x0B || p[1]!=0x77 ) return 0;
    int bsid = p[5]>>3;
    if( bsid<=10 ) {
      static const short rate[19] = { 32,40,48,56,64,80,96,112,128,160,192,224,256,320,384,448,512,576,640 };
      int fs = p[4]>>6, fc = p[4]&63;
      if( fs==3 || fc>=38 ) return 0;
      qword r = rate[fc>>1];
      u = 2*( fs==0 ? 2*r : fs==1 ? r*960/441+(fc&1) : 3*r );
    } else if( bsid<=16 ) {
      u = 2*( qword(((p[2]&7)<<8)|p[3]) + 1 );
    } else return 0;
  } break;
  case PT_AAC:
    u = AacFrameLen( param, p, avail );
    if( u==0 ) return 0;
    break;
  case PT_ADTS:
    if( avail<7 || p[0]!=0xFF || (p[1]&0xF6)!=0xF0 ) return 0;
    u = (qword(p[3]&3)<<11) | (qword(p[4])<<3) | (p[5]>>5);
    if( u<7 ) return 0;
    break;
  case PT_AU264: {                       // Annex B access unit
    if( avail<4 || p[0] || p[1] || !(p[2]==1 || (p[2]==0 && p[3]==1)) ) return 0;
    bool vcl = false; qword pos = 0;
    for(;;) {
      qword sc = Next001( p, pos, avail );
      if( sc+3>=avail ) { u = avail; break; }
      qword st = sc>pos && p[sc-1]==0 ? sc-1 : sc, h = sc+3;
      int ty = p[h]&31;
      if( vcl && st>0 && (ty==9 || ty==7 || ty==8 || ty==6 || (ty>=14 && ty<=18) ||
                         ((ty==1 || ty==5) && h+1<avail && (p[h+1]&0x80))) ) { u = st; break; }
      if( ty==1 || ty==5 ) vcl = true;
      pos = h;
    }
  } break;
  case PT_VOP: {                         // MPEG-4 part 2: headers + one VOP
    if( avail<4 || p[0] || p[1] || p[2]!=1 ) return 0;
    bool vop = false; qword pos = 0;
    for(;;) {
      qword sc = Next001( p, pos, avail );
      if( sc+3>=avail ) { u = avail; break; }
      byte c = p[sc+3];
      if( vop && sc>0 && (c==0xB6 || c==0xB0 || c==0xB3 || c==0xB5 || c<=0x2F) ) { u = sc; break; }
      if( c==0xB6 ) vop = true;
      pos = sc+3;
    }
  } break;
  case PT_M2V: {                         // MPEG-1/2: headers + one picture
    if( avail<4 || p[0] || p[1] || p[2]!=1 ) return 0;
    bool pic = false; qword pos = 0;
    for(;;) {
      qword sc = Next001( p, pos, avail );
      if( sc+3>=avail ) { u = avail; break; }
      byte c = p[sc+3];
      if( pic && sc>0 && (c==0xB3 || c==0xB8 || c==0x00) ) { u = sc; break; }
      if( c==0x00 ) pic = true;
      if( c==0xB7 && pic ) { u = sc+4; break; }    // sequence end code belongs to this picture
      pos = sc+3;
    }
  } break;
  case PT_JPEG: {                        // SOI..EOI, plus padding up to the next SOI
    if( avail<4 || p[0]!=0xFF || p[1]!=0xD8 ) return 0;
    qword pos = 2;
    for(;;) {
      while( pos+1<avail && p[pos]==0xFF && p[pos+1]==0xFF ) pos++;
      if( pos+2>avail || p[pos]!=0xFF ) return 0;
      byte m = p[pos+1]; pos += 2;
      if( m==0xD9 ) break;
      if( (m>=0xD0 && m<=0xD7) || m==0x01 ) continue;
      if( pos+2>avail ) return 0;
      qword L = BE16(p+pos);
      if( L<2 || L>avail-pos ) return 0;
      pos += L;
      if( m==0xDA ) {                    // entropy-coded data up to the next non-RST marker
        while( pos+1<avail && !(p[pos]==0xFF && p[pos+1]!=0 && !(p[pos+1]>=0xD0 && p[pos+1]<=0xD7)) ) pos++;
        if( pos+1>=avail ) return 0;
      }
    }
    qword e = pos;
    while( e+3<=avail && !(p[e]==0xFF && p[e+1]==0xD8 && p[e+2]==0xFF) ) e++;
    u = e+3<=avail ? e : avail;
  } break;
  default: return 0;
  }
  return u<=avail ? u : 0;
}

// number of units exactly covering [p,p+len), or -1
static long long CountUnits( int type, int param, const byte* p, qword len ) {
  qword pos=0; long long n=0;
  while( pos<len ) {
    qword u = UnitLen( type, param, p+pos, len-pos );
    if( u==0 ) return -1;
    pos+=u; n++;
  }
  return n;
}

// total length of n units starting at p, 0 on failure (n>0)
static qword UnitsLen( int type, int param, const byte* p, qword avail, qword n ) {
  qword pos=0;
  for( qword i=0; i<n; i++ ) {
    qword u = UnitLen( type, param, p+pos, avail-pos );
    if( u==0 ) return 0;
    pos+=u;
  }
  return pos;
}
