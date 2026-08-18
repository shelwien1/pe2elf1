// bmg_bmp.inc -- the BMP front end and the two byte models that carry
// everything which is not a pixel.
//
// Section 3 of ALGORITHM_v2.md describes a front end that decodes RLE and then
// throws the run structure away, so a decoded RLE file is pixel-identical to
// its input and byte-different from it.  bmg does not do that: it codes the
// pixels with the image model and then codes the *ops* against those pixels,
// where a run's length and every literal byte are already known, so the
// structure costs only the encoder's choices.  A .bmg round-trips to the byte.

// ---------------------------------------------------------------- header

static INLINE uint rd16( const byte* p ) { return p[0] | (p[1]<<8); }
static INLINE uint rd32( const byte* p ) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((uint)p[3]<<24); }

struct BmpInfo {
  uint fileSize, dataOffset, dibSize;
  int  width, sheight, W, H, topdown;
  int  bpp, comp;
  uint clrUsed;
  int  ncol;            // palette entries the file carries
  uint palOff;
  uint stride;          // padded row size, uncompressed only
  uint used;            // used bytes per row
  int  ok;

  // Parses the 14+40 bytes bmg needs and answers whether the image model can
  // take this file.  Anything it refuses goes to the generic model, so this is
  // allowed to be strict.
  void parse( const byte* d, uint n ) {
    ok = 0;
    if( n < 54 ) return;
    if( d[0]!='B' || d[1]!='M' ) return;
    fileSize   = rd32(d+2);
    dataOffset = rd32(d+10);
    dibSize    = rd32(d+14);
    if( dibSize < 40 ) return;
    if( n < 14 + dibSize ) return;
    width   = (int)rd32(d+18);
    sheight = (int)rd32(d+22);
    if( rd16(d+26) != 1 ) return;
    bpp     = rd16(d+28);
    comp    = rd32(d+30);
    clrUsed = rd32(d+46);
    if( fileSize != n ) return;
    if( width <= 0 || width > 65535 ) return;
    topdown = (sheight < 0);
    H = topdown ? -sheight : sheight;
    W = width;
    if( H <= 0 || H > 65535 ) return;
    if( bpp!=8 && bpp!=24 && bpp!=32 ) return;
    if( comp!=0 && !(comp==1 && bpp==8) ) return;
    if( comp==1 && topdown ) return;                   // RLE is bottom-up only
    if( (qword)W*H > 400000000ull ) return;
    ncol = 0; palOff = 14 + dibSize;
    if( bpp == 8 ) {
      uint want = clrUsed ? clrUsed : 256;
      if( want > 256 ) return;
      if( palOff + 4*want <= dataOffset && palOff + 4*want <= n ) ncol = (int)want;
      else ncol = 0;
    }
    if( dataOffset < 14+dibSize || dataOffset > n ) return;
    used   = ((uint)W*bpp) >> 3;
    stride = ((((uint)W*bpp) + 31) >> 5) << 2;
    if( comp==0 ) {
      if( (qword)dataOffset + (qword)stride*H > n ) return;
    }
    ok = 1;
  }
};

// ---------------------------------------------------------------- RLE8

enum { RLE_RUN=0, RLE_ABS, RLE_EOL, RLE_EOB, RLE_DELTA, RLE_STOP };

// Straight decode, used by the encoder to recover the pixels and to prove the
// stream is one bmg can reproduce.  img is W*H, top-down; RLE rows run
// bottom-up.  Returns 0 on anything malformed -- the file then goes generic.
static int rle8_decode( const byte* s, uint len, byte* img, int W, int H, uint* consumed ) {
  memset( img, 0, (qword)W*H );
  uint i = 0;  int x = 0, row = 0;
  for(;;) {
    if( i+2 > len ) { *consumed = i; return 1; }        // truncated: stop here
    uint n = s[i], val = s[i+1];
    if( n ) {
      if( row >= H || x + (int)n > W ) return 0;
      byte* dst = img + (qword)(H-1-row)*W + x;
      memset( dst, val, n );
      x += n; i += 2;
    } else if( val == 0 ) { x = 0; row++; i += 2; }
    else if( val == 1 ) { i += 2; *consumed = i; return 1; }
    else if( val == 2 ) {
      if( i+4 > len ) return 0;
      x += s[i+2]; row += s[i+3]; i += 4;
      if( x > W || row > H ) return 0;
    } else {
      uint m = val, pad = (m+1) & ~1u;
      if( i+2+pad > len ) return 0;
      if( row >= H || x + (int)m > W ) return 0;
      memcpy( img + (qword)(H-1-row)*W + x, s+i+2, m );
      x += m; i += 2 + pad;
    }
  }
}

// ---------------------------------------------------------------- byte models

// The prologue: everything before the pixels.  Coded as a delta against the
// byte four back, contexted on the position modulo four and on the previous
// delta at the same offset -- which is what a palette is, and a grey ramp
// collapses to four perfectly predictable contexts.
struct PrologueModel {
  Counter* t;
  byte     prevd[4];
  static const int ROWS = 4*256 + 4;
  void alloc( void ) { t = new Counter[(qword)ROWS*256]; ctr_fill( t, (qword)ROWS*256, hSCALE ); memset(prevd,0,4); }
  void free_( void ) { delete[] t; t = 0; }

  template<int f_DEC>
  uint code( byte* buf, uint i, uint val ) {
    uint row, d;
    if( i < 4 ) { row = 4*256 + i; d = val; }
    else        { row = (i&3)*256 + prevd[i&3]; d = (val - buf[i-4]) & 255; }
    d = byte_code<f_DEC>( t + (qword)row*256, d, P0_PR_wr, P0_PR_mw );
    if( i < 4 ) val = d;
    else { val = (buf[i-4] + d) & 255; prevd[i&3] = byte(d); }
    return val;
  }
};

// The generic model: order-1 and order-2 mixed, for the trailing bytes, the
// row padding, and any file the image model refuses outright.
struct ByteModel {
  Counter* o1;
  Counter* o2;
  Mixer<3> mx;
  uint c1, c2;
  static const int O2LOG = 16;
  void alloc( void ) {
    o1 = new Counter[256*256];   ctr_fill( o1, 256*256, hSCALE );
    o2 = new Counter[(qword)(1<<O2LOG)*256]; ctr_fill( o2, (qword)(1<<O2LOG)*256, hSCALE );
    mx.alloc( 256*8, P0_MX_W0 );
    c1 = c2 = 0;
  }
  void free_( void ) { delete[] o1; delete[] o2; mx.free_(); }

  template<int f_DEC>
  uint code( uint val, uint aux ) {
    Counter* p1 = o1 + (qword)c1*256;
    uint h = ((c1 | (c2<<8) | (aux<<16)) * 2654435761u) >> (32-O2LOG);
    Counter* p2 = o2 + (qword)h*256;
    uint ctx = 1;
    for( int i=7; i>=0; i-- ) {
      mx.st[0] = stretch15( p1[ctx].P );
      mx.st[1] = stretch15( p2[ctx].P );
      mx.st[2] = 256;
      mx.select( (int)(ctx&255)*8 + (7-i) );
      int pr = mx.mix();
      uint p15 = uint(pr) << 3;
      if( p15 < 8 ) p15 = 8;
      if( p15 > SCALE-8 ) p15 = SCALE-8;
      uint b = f_DEC ? 0 : ((val>>i)&1);
      b = rc.rc_BProcess( p15, b );
      mx.update( b, P0_MX_LR );
      p1[ctx].Update( b, P0_BY_wr, P0_BY_mw );
      p2[ctx].Update( b, P0_BY_wr, P0_BY_mw );
      ctx += ctx + b;
    }
    val = ctx & 255;
    c2 = c1; c1 = val;
    return val;
  }
};

// ---------------------------------------------------------------- RLE8 structure

// Codes the *ops* of an RLE8 stream against pixels both sides already have.
// A run's length is nearly always the whole run the image offers and its value
// is always the pixel under the cursor, so what is left on the wire is the
// encoder's choices and nothing else.
struct RleModel {
  Counter opc[16*5];
  Counter runeq[8], abseq[8], valeq[8], absmatch[8], padz[4];
  Counter trun[256], tabs[256], tval[256], tlit[256], tdx[256], tdy[256], tpad[256];

  void init( void ) {
    ctr_fill( opc, 16*5, hSCALE );
    ctr_fill( runeq, 8, hSCALE ); ctr_fill( abseq, 8, hSCALE );
    ctr_fill( valeq, 8, hSCALE ); ctr_fill( absmatch, 8, hSCALE );
    ctr_fill( padz, 4, hSCALE );
    ctr_fill( trun,256,hSCALE ); ctr_fill( tabs,256,hSCALE ); ctr_fill( tval,256,hSCALE );
    ctr_fill( tlit,256,hSCALE ); ctr_fill( tdx,256,hSCALE ); ctr_fill( tdy,256,hSCALE );
    ctr_fill( tpad,256,hSCALE );
  }

  static int runof( const byte* img, int W, int H, int row, int x ) {
    if( row < 0 || row >= H || x >= W ) return 0;
    const byte* r = img + (qword)(H-1-row)*W;
    byte c = r[x];
    int n = 1, lim = W - x; if( lim > 255 ) lim = 255;
    while( n < lim && r[x+n] == c ) n++;
    return n;
  }

  // dst is the RLE byte stream: read on encode, written on decode.
  // Returns the number of stream bytes the walk covered.
  template<int f_DEC>
  uint walk( byte* dst, uint cap, const byte* img, int W, int H ) {
    uint i = 0;  int x = 0, row = 0;
    for(;;) {
      int mr = runof( img, W, H, row, x );
      int oc = ilog2i( (uint)mr + 1 ); if( oc > 7 ) oc = 7;
      oc = oc*2 + (x==0);

      int op = RLE_STOP;
      if( !f_DEC ) {
        if( i+2 > cap ) op = RLE_STOP;
        else {
          uint n = dst[i], val = dst[i+1];
          if( n ) op = RLE_RUN;
          else if( val==0 ) op = RLE_EOL;
          else if( val==1 ) op = RLE_EOB;
          else if( val==2 ) op = RLE_DELTA;
          else op = RLE_ABS;
        }
      }
      int j;
      for( j=0; j<5; j++ ) {
        uint bit = f_DEC ? 0 : (op==j ? 0u : 1u);
        bit = bit_code<f_DEC>( opc[oc*5+j], bit, P0_RL_wr, P0_RL_mw );
        if( bit==0 ) break;
      }
      if( f_DEC ) op = j;

      if( op == RLE_STOP ) return i;

      if( op == RLE_RUN ) {
        int pn = mr ? mr : 1;
        uint n = f_DEC ? 0 : dst[i];
        uint bit = f_DEC ? 0 : (n==(uint)pn ? 0u : 1u);
        bit = bit_code<f_DEC>( runeq[oc>>1], bit, P0_RL_wr, P0_RL_mw );
        if( bit ) n = byte_code<f_DEC>( trun, f_DEC?0:n, P0_RL_wr, P0_RL_mw );
        else if( f_DEC ) n = (uint)pn;
        uint pv = (row>=0 && row<H && x<W) ? img[(qword)(H-1-row)*W + x] : 0;
        uint val = f_DEC ? 0 : dst[i+1];
        bit = f_DEC ? 0 : (val==pv ? 0u : 1u);
        bit = bit_code<f_DEC>( valeq[oc>>1], bit, P0_RL_wr, P0_RL_mw );
        if( bit ) val = byte_code<f_DEC>( tval, f_DEC?0:val, P0_RL_wr, P0_RL_mw );
        else if( f_DEC ) val = pv;
        if( f_DEC ) { if( i+2 > cap ) return i; dst[i]=byte(n); dst[i+1]=byte(val); }
        x += (int)n; i += 2;
      } else if( op == RLE_ABS ) {
        int ml = W - x; if( ml > 255 ) ml = 255; if( ml < 3 ) ml = 3;
        uint n = f_DEC ? 0 : dst[i+1];
        uint bit = f_DEC ? 0 : (n==(uint)ml ? 0u : 1u);
        bit = bit_code<f_DEC>( abseq[oc>>1], bit, P0_RL_wr, P0_RL_mw );
        if( bit ) n = byte_code<f_DEC>( tabs, f_DEC?0:n, P0_RL_wr, P0_RL_mw );
        else if( f_DEC ) n = (uint)ml;
        if( n < 3 ) n = 3;
        const byte* src = img + (qword)(H-1-row)*W + x;
        int same = 1;
        if( !f_DEC ) { for( uint q=0; q<n; q++ ) if( dst[i+2+q]!=src[q] ) { same=0; break; } }
        bit = f_DEC ? 0 : (same ? 0u : 1u);
        bit = bit_code<f_DEC>( absmatch[oc>>1], bit, P0_RL_wr, P0_RL_mw );
        same = (bit==0);
        uint pad = (n+1) & ~1u;
        if( f_DEC && i+2+pad > cap ) return i;
        if( f_DEC ) { dst[i]=0; dst[i+1]=byte(n); }
        if( same ) { if( f_DEC ) memcpy( dst+i+2, src, n ); }
        else for( uint q=0; q<n; q++ ) {
          uint b = byte_code<f_DEC>( tlit, f_DEC?0:dst[i+2+q], P0_RL_wr, P0_RL_mw );
          if( f_DEC ) dst[i+2+q] = byte(b);
        }
        if( pad != n ) {
          uint pb = f_DEC ? 0 : dst[i+2+n];
          bit = f_DEC ? 0 : (pb==0 ? 0u : 1u);
          bit = bit_code<f_DEC>( padz[0], bit, P0_RL_wr, P0_RL_mw );
          if( bit ) pb = byte_code<f_DEC>( tpad, f_DEC?0:pb, P0_RL_wr, P0_RL_mw );
          else pb = 0;
          if( f_DEC ) dst[i+2+n] = byte(pb);
        }
        x += (int)n; i += 2 + pad;
      } else if( op == RLE_EOL ) {
        if( f_DEC ) { if( i+2 > cap ) return i; dst[i]=0; dst[i+1]=0; }
        x = 0; row++; i += 2;
      } else if( op == RLE_EOB ) {
        if( f_DEC ) { if( i+2 > cap ) return i; dst[i]=0; dst[i+1]=1; }
        return i+2;
      } else {  // RLE_DELTA
        uint dx = byte_code<f_DEC>( tdx, f_DEC?0:dst[i+2], P0_RL_wr, P0_RL_mw );
        uint dy = byte_code<f_DEC>( tdy, f_DEC?0:dst[i+3], P0_RL_wr, P0_RL_mw );
        if( f_DEC ) { if( i+4 > cap ) return i; dst[i]=0; dst[i+1]=2; dst[i+2]=byte(dx); dst[i+3]=byte(dy); }
        x += (int)dx; row += (int)dy; i += 4;
      }
      if( x > W ) x = W;
      if( row > H ) row = H;
    }
  }
};
