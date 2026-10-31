enum { XF_NONE=0, XF_ANNEXB=1, XF_ADTS=2, XF_BLOB=3, XF_HDR=4, XF_OGG=5 };   // stream transforms

static void PutADTS( byte* h, const byte* tpl, uint fl ) {
  h[0]=tpl[0]; h[1]=tpl[1]; h[2]=tpl[2];
  h[3] = byte( (tpl[3]&0xFC) | (fl>>11) );
  h[4] = byte( fl>>3 );
  h[5] = byte( ((fl&7)<<5) | 0x1F );       // buffer fullness 0x7FF (VBR)
  h[6] = 0xFC;                             // 1 raw data block
}

struct BitRd {
  const byte* p; qword n, pos; bool err;
  BitRd( const byte* _p, qword _n ) : p(_p), n(_n), pos(0), err(false) {}
  uint get( int k ) {
    uint x=0;
    while( k-- ) {
      if( pos>=8*n ) { err=true; return 0; }
      x = (x<<1) | ((p[pos>>3]>>(7-(pos&7)))&1); pos++;
    }
    return x;
  }
  uint aot() { uint a = get(5); return a==31 ? 32+get(6) : a; }
};

static bool AdtsTpl( byte* h, int mpeg2, uint prof, uint sfi, uint ch ) {
  if( prof>3 || sfi>12 || ch>7 ) return false;
  h[0] = 0xFF; h[1] = byte( 0xF1 | (mpeg2<<3) );
  h[2] = byte( (prof<<6) | (sfi<<2) | (ch>>2) );
  h[3] = byte( (ch&3)<<6 );
  h[4] = h[5] = h[6] = 0;
  return true;
}
