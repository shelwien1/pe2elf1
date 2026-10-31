enum { CK_RAW=0, CK_H264=1, CK_M4V=2, CK_MJPEG=3, CK_M2V=4, CK_AUDIO=5, CK_HEVC=6, CK_MSM=7, CK_WMV2=8, CK_VC1=9 };
enum { VC1_FINTERP=1, VC1_RANGERED=2, VC1_BFRAMES=4, VC1_ADV=8, VC1_INTERLACE=16 };   // CK_VC1 parameter bits
enum { FC_KEY=1, FC_HDR=2, FC_EMPTY=4 };

// VC-1 picture type from a picture header at p (MSB first): 1 = I, 0 = other / unknown
static int VC1Intra( uint prm, const byte* p, qword n ) {
  qword q = 0; int nb = 0; uint acc = 0;
  #define VBIT( b ) { if( !nb ) { if( q>=n ) return 0; acc = p[q++]; nb = 8; } b = (acc>>(--nb))&1; }
  int b;
  if( prm&VC1_ADV ) {                       // advanced profile: [FCM] PTYPE / FPTYPE
    if( prm&VC1_INTERLACE ) {
      VBIT(b);
      if( b ) { VBIT(b); if( b ) { int t = 0; for( int i=0; i<3; i++ ) { VBIT(b); t = t*2+b; } return t<2; } }
    }
    int u = 0; for( ; u<4; u++ ) { VBIT(b); if( !b ) break; }
    return u==2;                            // 0 = P, 10 = B, 110 = I, 1110 = BI, 1111 = skipped
  }
  if( prm&VC1_FINTERP ) VBIT(b);            // simple/main: [INTERPFRM] FRMCNT(2) [RANGEREDFRM] PTYPE
  VBIT(b); VBIT(b);
  if( prm&VC1_RANGERED ) VBIT(b);
  VBIT(b); if( b ) return 0;                // 1 = P
  if( prm&VC1_BFRAMES ) { VBIT(b); return b; }   // 01 = I, 00 = B/BI
  return 1;
  #undef VBIT
}

// key: IDR / I-VOP / I-picture / IRAP (always for intra-only codecs); hdr: first H.264
// slice intra, VOL / sequence header present; empty: no data
static int FrameClass( int ck, const byte* p, qword n, uint prm ) {
  if( n==0 ) return FC_EMPTY;
  int f = 0;
  switch( ck ) {
  case CK_H264: {
    bool first = true;
    for( qword sc=Next001(p,0,n); sc+4<n; sc=Next001(p,sc+3,n) ) {
      int ty = p[sc+3]&31;
      if( ty==5 ) f |= FC_KEY;
      if( (ty==1 || ty==5) && first ) {
        first = false;
        uint acc = 0; int nb = 0, z, k = 0; qword q = sc+4; uint v[2] = {0,0};
        for( int e=0; e<2; e++ ) {          // first_mb_in_slice, slice_type (ue)
          for( z=0; ; z++ ) {
            if( !nb ) { if( q>=n ) return f; acc = p[q++]; nb = 8; }
            int b = (acc>>(--nb))&1;
            if( b ) break;
            if( z>30 ) return f;
          }
          uint x = 0;
          for( int i=0; i<z; i++ ) { if( !nb ) { if( q>=n ) return f; acc = p[q++]; nb = 8; } x = (x<<1)|((acc>>(--nb))&1); }
          v[k++] = (1u<<z)-1+x;
        }
        if( v[1]%5==2 || v[1]%5==4 ) f |= FC_HDR;
      }
    }
  } break;
  case CK_HEVC:
    for( qword sc=Next001(p,0,n); sc+5<n; sc=Next001(p,sc+3,n) ) {
      int ty = (p[sc+3]>>1)&63;
      if( ty>=16 && ty<=23 ) f |= FC_KEY;
      if( ty>=32 && ty<=34 ) f |= FC_HDR;
    }
    break;
  case CK_M4V: {
    bool vop = false;
    for( qword sc=Next001(p,0,n); sc+4<n; sc=Next001(p,sc+3,n) ) {
      byte c = p[sc+3];
      if( c>=0x20 && c<=0x2F ) f |= FC_HDR;
      if( c==0xB6 && !vop ) { vop = true; if( !(p[sc+4]>>6) ) f |= FC_KEY; }
    }
  } break;
  case CK_M2V: {
    bool pic = false;
    for( qword sc=Next001(p,0,n); sc+5<n; sc=Next001(p,sc+3,n) ) {
      byte c = p[sc+3];
      if( c==0xB3 ) f |= FC_HDR;
      if( c==0x00 && !pic ) { pic = true; if( ((p[sc+5]>>3)&7)==1 ) f |= FC_KEY; }
    }
  } break;
  case CK_MSM: if( !(p[0]>>6) ) f |= FC_KEY; break;      // MS-MPEG4 v2/v3, WMV1: 2-bit picture type, 0 = I
  case CK_WMV2: if( !(p[0]>>7) ) f |= FC_KEY; break;     // WMV2: 1-bit picture type
  case CK_VC1:                                            // WMV3 / WVC1: optional start codes
    if( (prm&VC1_ADV) && n>=4 && p[0]==0 && p[1]==0 && p[2]==1 ) {
      for( qword sc=Next001(p,0,n); sc+4<n; sc=Next001(p,sc+3,n) ) {
        byte c = p[sc+3];
        if( c==0x0F ) f |= FC_HDR;
        if( c==0x0D ) { if( VC1Intra( prm, p+sc+4, n-sc-4 ) ) f |= FC_KEY; break; }
      }
    } else if( VC1Intra( prm, p, n ) ) f |= FC_KEY;
    break;
  default: f = FC_KEY;
  }
  return f;
}
