// bmg -- a lossless Grey/RGB/RGBA BMP compressor.
//
//   bmg c input.bmp output.bmg
//   bmg d input.bmg output.bmp
//
// The model follows ALGORITHM_v2.md -- BMF 2.01's second reading -- rebuilt on
// bcdr5's range coder, EMA counter and IDX parameter framework.  What is taken
// from it, and what is not, is written down in BMG-FORMAT.md.
//
// The container is byte-exact: whatever goes in comes back out, header bytes,
// row padding, RLE run structure and trailing bytes included.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
#endif

#include "bmg_rc.inc"
#include "sh_mapping.inc"
#include "MOD/bmg-P0_h.inc"
#include "bmg_ctr.inc"
#include "bmg_model.inc"
#include "bmg_bmp.inc"

// ---------------------------------------------------------------- estimates

// Order-0 entropy of the MED residuals of one plane, in bits.  Section 6.3's
// tool: cheap enough to run over the whole image several times, and every
// encoder-side choice in bmg is settled by comparing two of these.
static double med_cost1( const byte* pl, int W, int H ) {
  uint hist[256]; memset( hist, 0, sizeof(hist) );
  for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
    int Wv = x ? pl[(qword)y*W+x-1] : (y ? pl[(qword)(y-1)*W+x] : 128);
    int Nv = y ? pl[(qword)(y-1)*W+x] : Wv;
    int NWv = (x&&y) ? pl[(qword)(y-1)*W+x-1] : Nv;
    hist[ ImgModel::zigzag( pl[(qword)y*W+x] - ImgModel::med(Wv,Nv,NWv) ) ]++;
  }
  return estimate_cost( hist, 256, (qword)W*H );
}

// The same for three planes coded the way the model will code them: the green
// plane on its own, then blue and red as the section 7.1 value-domain
// difference against it.  The estimate has to match the coder it is choosing
// for, which is why this subtracts the reference plane's *value* rather than
// its residual.
static double med_cost3( const byte* b, const byte* g, const byte* r, int W, int H ) {
  const byte* pl[3] = { g, b, r };
  double tot = 0;
  for( int k=0; k<3; k++ ) {
    uint hist[256]; memset( hist, 0, sizeof(hist) );
    const byte* q = pl[k];
    for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
      qword i = (qword)y*W+x;
      int d  = k ? ((q[i]  - g[i]) & 255) : q[i];
      int dW = x ? (k ? ((q[i-1]-g[i-1])&255) : q[i-1])
                 : (y ? (k ? ((q[i-W]-g[i-W])&255) : q[i-W]) : 128);
      int dN = y ? (k ? ((q[i-W]-g[i-W])&255) : q[i-W]) : dW;
      int dNW = (x&&y) ? (k ? ((q[i-W-1]-g[i-W-1])&255) : q[i-W-1]) : dN;
      hist[ ImgModel::zigzag( d - ImgModel::med(dW,dN,dNW) ) ]++;
    }
    tot += estimate_cost( hist, 256, (qword)W*H );
  }
  return tot;
}

// ---------------------------------------------------------------- codec

// Coding a rank in [0,k-1] with the constraint respected bit by bit, so a
// candidate count that is not a power of two costs no more than it should.
struct RankModel {
  Counter t[8*256];
  void init( void ) { ctr_fill( t, 8*256, hSCALE ); }
  template<int f_DEC>
  uint code( uint rank, uint k ) {
    if( k <= 1 ) return 0;
    uint hi = k-1, clen = (uint)ilog2i(hi)+1, z = 0;
    for( int i=(int)clen-1; i>=0; i-- ) {
      if( z + (1u<<i) > hi ) continue;              // that branch cannot hold a value
      uint cx = ((uint)i & 7)*256 + ((((1u<<clen)+z) >> (i+1)) & 255);
      uint b = f_DEC ? 0 : ((rank>>i)&1);
      b = bit_code<f_DEC>( t[cx], b, P0_FL_wr, P0_FL_mw );
      z |= b<<i;
    }
    return z;
  }
};

struct Codec {
  byte* buf;          // the whole BMP, encode input / decode output
  uint  fsize;
  BmpInfo info;
  ImgModel im;
  PrologueModel pm;
  ByteModel  bm;      int bm_on;
  RleModel   rm;
  RankModel  km;
  Counter    flag[8];
  Counter    twt[256], tdcm[256];

  byte* pix;          // W*H*bytes_per_pixel, top-down
  byte* pal;          // ncol*3, BGR
  int   expand, NPX;
  int   im_on;
  int   own_buf;      // the decoder allocates buf; the encoder is handed it

  int hub;
  Codec() { buf=0; pix=0; pal=0; expand=0; bm_on=0; cn=0; hub=1;
            tsc0=0; tsc1=0; im_on=0; own_buf=0; }
  ~Codec() {
    if( im_on ) im.free_();
    if( bm_on ) bm.free_();
    delete[] pix; delete[] pal; delete[] tsc0; delete[] tsc1;
    if( own_buf ) delete[] buf;
    pm.free_();
  }

  void need_bm( void ) { if( !bm_on ) { bm.alloc(); bm_on=1; } }

  // ---------------------------------------------------------- palette

  int gray_monotone( void ) {
    if( info.ncol <= 0 ) return 0;
    int prev = -1;
    for( int i=0; i<info.ncol; i++ ) {
      int b = pal[3*i], g = pal[3*i+1], r = pal[3*i+2];
      if( b!=g || g!=r ) return 0;
      if( b < prev ) return 0;
      prev = b;
    }
    return 1;
  }

  // ---------------------------------------------------------- planes

  void planes_from_pix( void ) {
    int W = info.W, H = info.H;
    if( info.bpp == 8 && !expand ) {
      for( int y=0; y<H; y++ ) memcpy( im.v[0] + (qword)y*im.stride, pix + (qword)y*W, W );
    } else if( info.bpp == 8 ) {
      for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
        int c = pix[(qword)y*W+x];
        qword o = (qword)y*im.stride + x;
        im.v[0][o] = pal[3*c]; im.v[1][o] = pal[3*c+1]; im.v[2][o] = pal[3*c+2];
      }
    } else {
      int np = info.bpp>>3;
      for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
        const byte* s = pix + ((qword)y*W+x)*np;
        qword o = (qword)y*im.stride + x;
        for( int p=0; p<np; p++ ) im.v[p][o] = s[p];
      }
    }
  }

  void pix_from_planes( void ) {
    int W = info.W, H = info.H;
    if( info.bpp == 8 && !expand ) {
      for( int y=0; y<H; y++ ) memcpy( pix + (qword)y*W, im.a[0] + (qword)y*im.stride, W );
    } else if( info.bpp != 8 ) {
      int np = info.bpp>>3;
      for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
        byte* d = pix + ((qword)y*W+x)*np;
        qword o = (qword)y*im.stride + x;
        for( int p=0; p<np; p++ ) d[p] = im.a[p][o];
      }
    }
    // the expanded case is finished by the disambiguation pass
  }

  // ---------------------------------------------------------- pixels <-> file

  void rows_from_file( void ) {                  // encode, uncompressed
    int W = info.W, H = info.H, bpp8 = info.bpp>>3;
    for( int r=0; r<H; r++ ) {
      int y = info.topdown ? r : H-1-r;
      memcpy( pix + (qword)y*W*bpp8, buf + info.dataOffset + (qword)r*info.stride, (qword)W*bpp8 );
    }
  }
  void rows_to_file( void ) {                    // decode, uncompressed
    int W = info.W, H = info.H, bpp8 = info.bpp>>3;
    for( int r=0; r<H; r++ ) {
      int y = info.topdown ? r : H-1-r;
      memcpy( buf + info.dataOffset + (qword)r*info.stride, pix + (qword)y*W*bpp8, (qword)W*bpp8 );
    }
  }

  // ---------------------------------------------------------- the stream

  template<int f_DEC>
  int body( void ) {
    ctr_fill( flag, 8, hSCALE );
    // ---- mode
    uint mode = f_DEC ? 0 : (uint)(info.ok ? 1 : 0);
    mode = bit_code<f_DEC>( flag[0], mode, P0_FL_wr, P0_FL_mw );

    if( mode == 0 ) {
      need_bm();
      for( uint i=0; i<fsize; i++ ) {
        uint b = bm.code<f_DEC>( f_DEC?0:buf[i], 0 );
        if( f_DEC ) buf[i] = byte(b);
      }
      return 1;
    }

    // ---- prologue
    ctr_fill( twt, 256, hSCALE ); ctr_fill( tdcm, 256, hSCALE );
    pm.alloc();
    for( uint i=0; i<14; i++ ) {
      uint b = pm.code<f_DEC>( buf, i, f_DEC?0:buf[i] );
      if( f_DEC ) buf[i] = byte(b);
    }
    uint dofs = f_DEC ? rd32(buf+10) : info.dataOffset;
    if( f_DEC ) {
      if( dofs < 14 || dofs > fsize ) return 0;
    }
    for( uint i=14; i<dofs; i++ ) {
      uint b = pm.code<f_DEC>( buf, i, f_DEC?0:buf[i] );
      if( f_DEC ) buf[i] = byte(b);
    }
    pm.free_();

    if( f_DEC ) {
      info.parse( buf, fsize );
      if( !info.ok ) return 0;
      delete[] pal; pal = 0;
      if( info.ncol > 0 ) {
        pal = new byte[3*info.ncol];
        for( int i=0; i<info.ncol; i++ ) {
          pal[3*i  ] = buf[info.palOff+4*i  ];
          pal[3*i+1] = buf[info.palOff+4*i+1];
          pal[3*i+2] = buf[info.palOff+4*i+2];
        }
      }
    }

    int W = info.W, H = info.H;
    int bpp8 = info.bpp>>3;
    if( !pix ) pix = new byte[(qword)W*H*bpp8];

    // ---- expansion flag (8-bit paletted images only)
    if( info.bpp==8 && info.ncol>0 && !gray_monotone() ) {
      uint b = f_DEC ? 0 : (uint)(expand ? 0 : 1);
      b = bit_code<f_DEC>( flag[1], b, P0_FL_wr, P0_FL_mw );
      expand = (b==0);
    } else expand = 0;

    NPX = expand ? 3 : (info.bpp==8 ? 1 : bpp8);
    im.alloc( W, H, NPX );  im_on = 1;
    if( !f_DEC ) planes_from_pix();

    // ---- section 6.3: which plane leads, and what the others subtract of it
    if( NPX >= 3 ) {
      if( !f_DEC ) {
        tsc0 = new byte[W+2]; tsc1 = new byte[W+2];
        memset( tsc0, 128, W+2 ); memset( tsc1, 128, W+2 );
        double bestc = 1e300;
        int bh = 1, bw[4][3];
        memset( bw, 0, sizeof(bw) );
        for( int hb=0; hb<3; hb++ ) {
          im.set_hub( hb );
          double tot = 0;  int cw[4][3];  memset( cw, 0, sizeof(cw) );
          for( int k=0; k<NPX; k++ ) {
            int q = im.order[k];
            tot += search_plane( q, im.refA[q], im.refB[q], cw[q][0], cw[q][1], cw[q][2] );
          }
          if( tot < bestc ) { bestc = tot; bh = hb; memcpy( bw, cw, sizeof(bw) ); }
        }
        hub = bh;
        im.set_hub( hub );
#ifdef BMG_STATS
        fprintf(stderr,"  hub=%d est=%.0f B  weights:", bh, bestc/8);
        for(int q=0;q<NPX;q++) fprintf(stderr," p%d(%d,%d,dc%d)",q,bw[q][0],bw[q][1],bw[q][2]);
        fprintf(stderr,"\n");
#endif
        for( int q=0; q<NPX; q++ ) { im.tw0[q]=bw[q][0]; im.tw1[q]=bw[q][1]; im.tdc[q]=bw[q][2]; }
        delete[] tsc0; delete[] tsc1; tsc0=tsc1=0;
      }
      uint hb = f_DEC ? 0 : (uint)hub;
      uint b0 = bit_code<f_DEC>( flag[2], (hb>>1)&1, P0_FL_wr, P0_FL_mw );
      uint b1 = bit_code<f_DEC>( flag[3], hb&1,      P0_FL_wr, P0_FL_mw );
      if( f_DEC ) { hub = (int)(b0*2+b1); if( hub > 2 ) hub = 1; im.set_hub( hub ); }
      for( int k=0; k<NPX; k++ ) {
        int q = im.order[k];
        if( im.refA[q] < 0 ) continue;
        uint w0 = byte_code<f_DEC>( twt, f_DEC?0:(uint)(im.tw0[q]+64), P0_FL_wr, P0_FL_mw );
        uint w1 = 0;
        if( im.refB[q] >= 0 ) w1 = byte_code<f_DEC>( twt, f_DEC?0:(uint)(im.tw1[q]+64), P0_FL_wr, P0_FL_mw );
        uint dc = byte_code<f_DEC>( tdcm, f_DEC?0:(uint)im.tdc[q], P0_FL_wr, P0_FL_mw );
        if( f_DEC ) { im.tw0[q]=(int)w0-64; im.tw1[q]=(int)w1-64; im.tdc[q]=(int)dc; }
      }
    }

    im.run<f_DEC>();
    if( f_DEC ) pix_from_planes();

    // ---- the expansion inverse
    if( expand ) {
      km.init();
      build_colour_index();
      for( int y=0; y<H; y++ ) for( int x=0; x<W; x++ ) {
        qword o = (qword)y*im.stride + x;
        int k, lo = colour_run( im.a[0][o], im.a[1][o], im.a[2][o], &k );
        uint rank = 0;
        if( !f_DEC ) {
          int want = pix[(qword)y*W+x];
          for( int j=0; j<k; j++ ) if( (int)(csort[lo+j] & 255u) == want ) { rank = (uint)j; break; }
        }
        rank = km.code<f_DEC>( rank, (uint)k );
        if( f_DEC ) pix[(qword)y*W+x] =
          byte( k ? (csort[lo + (rank<(uint)k ? rank : 0)] & 255u) : 0 );
      }
    }

    // ---- the pixel bytes back into the file
    if( info.comp != 1 ) {
      if( f_DEC ) rows_to_file();
      need_bm();
      uint padn = info.stride - info.used;
      if( padn ) for( int r=0; r<H; r++ ) {
        byte* q = buf + info.dataOffset + (qword)r*info.stride + info.used;
        for( uint j=0; j<padn; j++ ) {
          uint b = bm.code<f_DEC>( f_DEC?0:q[j], 200+j );
          if( f_DEC ) q[j] = byte(b);
        }
      }
      uint end = info.dataOffset + info.stride*(uint)H;
      for( uint i=end; i<fsize; i++ ) {
        uint b = bm.code<f_DEC>( f_DEC?0:buf[i], 1 );
        if( f_DEC ) buf[i] = byte(b);
      }
    } else {
      rm.init( W, H );
      uint cap = fsize - info.dataOffset;
      uint used = rm.walk<f_DEC>( buf + info.dataOffset, cap, pix, W, H );
#ifdef BMG_STATS
      fprintf(stderr,"  rle: %llu run %llu abs %llu eol %llu eob %llu delta   len-miss run=%llu abs=%llu  stream=%u\n",
              (unsigned long long)rm.nop[0],(unsigned long long)rm.nop[1],(unsigned long long)rm.nop[2],
              (unsigned long long)rm.nop[3],(unsigned long long)rm.nop[4],
              (unsigned long long)rm.nlen[0],(unsigned long long)rm.nlen[1], used);
      fprintf(stderr,"  rle: greedy hits %llu, misses %llu\n",
              (unsigned long long)rm.nlen[2],(unsigned long long)rm.nlen[3]);
#endif
      rm.free_();
      need_bm();
      for( uint i=info.dataOffset+used; i<fsize; i++ ) {
        uint b = bm.code<f_DEC>( f_DEC?0:buf[i], 1 );
        if( f_DEC ) buf[i] = byte(b);
      }
    }
    return 1;
  }

  // ---------------------------------------------------------- transform search

  // Order-0 cost, in bits, of coding plane p's MED residuals after the
  // section 7.1 transform against (r0,r1) at (w0,w1,dc).  Reads im.v[], which
  // still holds actual sample values -- run() is what turns them into
  // transformed ones.
  double tcost( int p, int r0, int r1, int w0, int w1, int dc ) {
    int W = im.W, H = im.H, st = im.stride;
    uint hist[256]; memset( hist, 0, sizeof(hist) );
    byte* d0 = tsc0; byte* d1 = tsc1;
    const byte* vp = im.v[p];
    const byte* v0 = (r0>=0) ? im.v[r0] : 0;
    const byte* v1 = (r1>=0) ? im.v[r1] : 0;

    // A large image is sampled in four contiguous bands rather than read whole:
    // this runs a few hundred times per image and it is an estimate, so what it
    // has to get right is the *ordering* of a few dozen candidates.  Contiguous
    // bands rather than every n-th row, because the MED residual of a row needs
    // the row above it to mean anything.
    int nband = 1, bh = H;
    if( (qword)W*H > 200000 ) {
      nband = 4;
      bh = (int)( 200000 / (4*(qword)W) );
      if( bh < 8 ) bh = 8;
      if( bh > H/4 ) bh = H/4;
    }
    qword n = 0;
    for( int b=0; b<nband; b++ ) {
      int y0 = (nband==1) ? 0 : b*(H/nband);
      for( int yy=0; yy<bh && y0+yy<H; yy++ ) {
        int y = y0+yy;
        const byte* rp = vp + (qword)y*st;
        const byte* q0 = v0 ? v0 + (qword)y*st : 0;
        const byte* q1 = v1 ? v1 + (qword)y*st : 0;
        for( int x=0; x<W; x++ ) {
          int bl = dc;
          if( q0 ) { int s = w0*q0[x]; if( q1 ) s += w1*q1[x]; bl += rsh(s,7); }
          d0[x] = byte( (rp[x] - bl) & 255 );
        }
        for( int x=0; x<W; x++ ) {
          int Wv = x ? d0[x-1] : (yy ? d1[0] : 128);
          int Nv = yy ? d1[x]  : Wv;
          int NWv = (x&&yy) ? d1[x-1] : Nv;
          hist[ ImgModel::zigzag( d0[x] - ImgModel::med(Wv,Nv,NWv) ) ]++;
        }
        byte* t = d1; d1 = d0; d0 = t;
        n += W;
      }
    }
    return estimate_cost( hist, 256, n );
  }

  // DC that centres the transformed plane on 128, so the modulo-256 wrap of
  // the transform lands where the data is not.  Taken on a subsample: it moves
  // the cost only through the wrap, so a rough answer is the right answer.
  int tdc_for( int p, int r0, int r1, int w0, int w1 ) {
    int W = im.W, H = im.H, st = im.stride;
    qword sum = 0, n = 0;
    for( int y=0; y<H; y+=4 ) for( int x=0; x<W; x+=4 ) {
      qword o = (qword)y*st + x;
      int bl = 0;
      if( r0>=0 ) { int s = w0*im.v[r0][o]; if( r1>=0 ) s += w1*im.v[r1][o]; bl = rsh(s,7); }
      sum += (qword)(uint)((im.v[p][o] - bl) & 255); n++;
    }
    if( !n ) return 0;
    return (int)(((sum + n/2)/n - 128) & 255);
  }

  double search_plane( int p, int r0, int r1, int& bw0, int& bw1, int& bdc ) {
    if( r0 < 0 ) { bw0=bw1=0; bdc=0; return tcost( p, -1, -1, 0, 0, 0 ); }
    static const int coarse[] = { -64, -32, 0, 32, 64, 96, 112, 128, 144, 160, 191 };
    int w0 = 128, w1 = 0;
    double best = 1e300;
    for( unsigned c=0; c<sizeof(coarse)/sizeof(coarse[0]); c++ ) {
      int dc = tdc_for( p, r0, r1, coarse[c], 0 );
      double v = tcost( p, r0, r1, coarse[c], 0, dc );
      if( v < best ) { best = v; w0 = coarse[c]; }
    }
    // coordinate descent, halving the step -- section 6.3 step 3
    for( int round=0; round<(r1>=0?2:1); round++ ) {
      for( int stage=0; stage<(r1>=0?2:1); stage++ ) {
        int* wv = stage ? &w1 : &w0;
        for( int step=16; step>=1; step>>=1 ) {
          for(;;) {
            int improved = 0;
            for( int s=-1; s<=1; s+=2 ) {
              int t = *wv + s*step;
              if( t < -64 || t > 191 ) continue;
              int sv = *wv; *wv = t;
              int dc = tdc_for( p, r0, r1, w0, w1 );
              double v = tcost( p, r0, r1, w0, w1, dc );
              if( v < best ) { best = v; improved = 1; break; }
              *wv = sv;
            }
            if( !improved ) break;
          }
        }
      }
    }
    bw0 = w0; bw1 = (r1>=0) ? w1 : 0;
    bdc = tdc_for( p, r0, r1, bw0, bw1 );
    return best;
  }

  byte *tsc0, *tsc1;

  // ---------------------------------------------------------- colour index

  // The inverse of the palette.  A palette with two entries of the same colour
  // is legal and has to round-trip, so a colour does not name an index on its
  // own; what it names is a *run* of indices, and which one of them was meant
  // is coded as a rank.
  //
  // This was a hash table first, and the hash was the problem rather than the
  // idea: 256 palette colours in 4096 slots collided five times more often than
  // random on a colour cube, and every collision widened a rank that should
  // have been empty -- 279 bytes of disambiguation on an image that has no
  // duplicate colours at all.  Sorting the palette and binary-searching it is
  // exact, costs 256 entries, and cannot collide, so a rank is coded only where
  // the palette really is ambiguous.
  uint csort[256];   // colour in bits 8..31, palette index in bits 0..7
  int  cn;
  void build_colour_index( void ) {
    if( cn ) return;
    cn = info.ncol;
    for( int i=0; i<cn; i++ )
      csort[i] = ((uint)pal[3*i] | ((uint)pal[3*i+1]<<8) | ((uint)pal[3*i+2]<<16))*256u + (uint)i;
    for( int i=1; i<cn; i++ ) {                 // insertion sort; cn <= 256
      uint v = csort[i]; int j = i-1;
      while( j>=0 && csort[j] > v ) { csort[j+1] = csort[j]; j--; }
      csort[j+1] = v;
    }
  }
  // First entry with this colour, and how many entries share it.
  int colour_run( int b, int g, int r, int* count ) {
    uint key = ((uint)b | ((uint)g<<8) | ((uint)r<<16))*256u;
    int lo = 0, hi = cn;
    while( lo < hi ) { int m = (lo+hi)>>1; if( csort[m] < key ) lo = m+1; else hi = m; }
    int k = 0;
    while( lo+k < cn && (csort[lo+k] & ~255u) == key ) k++;
    *count = k;
    return lo;
  }

  // ---------------------------------------------------------- drivers

  int compress( FILE* fo ) {
    info.parse( buf, fsize );
    if( info.ok && info.ncol > 0 ) {
      pal = new byte[3*info.ncol];
      for( int i=0; i<info.ncol; i++ ) {
        pal[3*i  ] = buf[info.palOff+4*i  ];
        pal[3*i+1] = buf[info.palOff+4*i+1];
        pal[3*i+2] = buf[info.palOff+4*i+2];
      }
    }
    if( info.ok ) {
      int W = info.W, H = info.H, bpp8 = info.bpp>>3;
      pix = new byte[(qword)W*H*bpp8];
      if( info.comp != 1 ) rows_from_file();
      else {
        uint cons = 0;
        if( !rle8_decode( buf+info.dataOffset, fsize-info.dataOffset, pix, W, H, &cons ) )
          info.ok = 0;
      }
    }
    if( info.ok && info.bpp==8 && info.ncol>0 && !gray_monotone() ) {
      // every index must name a palette entry, or the expansion has no inverse
      int okx = 1;
      for( qword i=0; i<(qword)info.W*info.H; i++ ) if( pix[i] >= info.ncol ) { okx=0; break; }
      if( okx ) {
        int W = info.W, H = info.H;
        byte* pb = new byte[(qword)W*H]; byte* pg = new byte[(qword)W*H]; byte* pr = new byte[(qword)W*H];
        for( qword i=0; i<(qword)W*H; i++ ) {
          int c = pix[i]; pb[i]=pal[3*c]; pg[i]=pal[3*c+1]; pr[i]=pal[3*c+2];
        }
        double ci = med_cost1( pix, W, H );
        double cr = med_cost3( pb, pg, pr, W, H );
        // the disambiguation channel is part of the expanded cost
        double extra = 0;
        {
          build_colour_index();
          for( qword i=0; i<(qword)W*H; i++ ) {
            int k; colour_run( pb[i], pg[i], pr[i], &k );
            if( k > 1 ) extra += log((double)k) * 1.4426950408889634;
          }
          expand = ( cr + extra < ci );
#ifdef BMG_STATS
          fprintf( stderr, "  palette: indices %.0f B, expanded %.0f B (+%.0f disambiguation) -> %s\n",
                   ci/8, cr/8, extra/8, expand ? "expand" : "indices" );
#endif
        }
        delete[] pb; delete[] pg; delete[] pr;
      }
    }

    qword cap = (qword)fsize + 0x20000;
    byte* out = new byte[cap];
    rc.init( out, cap, 0 );
    int good = body<0>();
    rc.flush();
    qword n = rc.mpos;

    byte hdr[9];
    hdr[0]='B'; hdr[1]='M'; hdr[2]='G'; hdr[3]='1';
    hdr[4]=fsize&255; hdr[5]=(fsize>>8)&255; hdr[6]=(fsize>>16)&255; hdr[7]=(fsize>>24)&255;

    if( !good || n >= (qword)fsize ) {          // section 4.2: the raw fallback
      hdr[8] = 2;
      fwrite( hdr, 1, 9, fo );
      fwrite( buf, 1, fsize, fo );
      delete[] out;
      return 9 + fsize;
    }
    hdr[8] = 1;
    fwrite( hdr, 1, 9, fo );
    fwrite( out, 1, n, fo );
    delete[] out;
    return (int)(9 + n);
  }

  int decompress( const byte* src, uint slen, FILE* fo ) {
    if( slen < 9 || src[0]!='B'||src[1]!='M'||src[2]!='G'||src[3]!='1' ) {
      fprintf( stderr, "not a bmg stream\n" ); return 0;
    }
    fsize = src[4] | (src[5]<<8) | (src[6]<<16) | ((uint)src[7]<<24);
    int mode = src[8];
    buf = new byte[ fsize ? fsize : 1 ];  own_buf = 1;
    memset( buf, 0, fsize );
    if( mode == 2 ) {
      if( slen < 9+fsize ) { fprintf(stderr,"truncated\n"); return 0; }
      memcpy( buf, src+9, fsize );
    } else {
      rc.init( (byte*)src+9, slen-9, 1 );
      if( !body<1>() ) { fprintf( stderr, "corrupt bmg stream\n" ); return 0; }
    }
    fwrite( buf, 1, fsize, fo );
    return 1;
  }
};

// ---------------------------------------------------------------- main

static Codec cd;

static void usage( void ) {
  printf( "bmg -- lossless Grey/RGB/RGBA BMP compressor\n"
          "\n"
          "  bmg c input.bmp output.bmg\n"
          "  bmg d input.bmg output.bmp\n" );
  exit(1);
}

int main( int argc, char** argv ) {
  if( argc != 4 || argv[1][1] ) usage();
  int mode = argv[1][0];
  if( mode!='c' && mode!='d' ) usage();

  FILE* f = fopen( argv[2], "rb" );
  if( !f ) { fprintf( stderr, "cannot open %s\n", argv[2] ); return 1; }
  fseek( f, 0, SEEK_END );  long L = ftell(f);  fseek( f, 0, SEEK_SET );
  if( L < 0 ) { fprintf(stderr,"cannot size %s\n",argv[2]); return 1; }
  byte* in = new byte[ L ? L : 1 ];
  if( (long)fread( in, 1, L, f ) != L ) { fprintf(stderr,"short read\n"); return 1; }
  fclose(f);

  FILE* g = fopen( argv[3], "wb" );
  if( !g ) { fprintf( stderr, "cannot create %s\n", argv[3] ); return 2; }

  init_logistic();
  init_partitions();
  clock_t t0 = clock();
  int rv = 0;

  if( mode == 'c' ) {
    cd.buf = in; cd.fsize = (uint)L;
    int n = cd.compress( g );
    double sec = double(clock()-t0)/CLOCKS_PER_SEC;
#ifdef BMG_STATS
    { double tot=0; qword N=0;
      fprintf(stderr,"  match hit rate %.3f\n", st_mhit[0]+st_mhit[1] ? (double)st_mhit[1]/(st_mhit[0]+st_mhit[1]) : 0.0);
      for(int i=0;i<3;i++) tot+=st_bits[i];
      for(int c=0;c<4;c++) N+=st_n[c];
      fprintf(stderr,"  ladder %.0f B  esc-level %.0f B  esc-path %.0f B  total %.0f B\n",
              st_bits[0]/8, st_bits[1]/8, st_bits[2]/8, tot/8);
      for(int c=0;c<4;c++) if(st_n[c]) {
        double e=estimate_cost((uint*)0,0,0); (void)e;
        double s2=(double)st_n[c]*log((double)st_n[c]);
        for(int i=0;i<256;i++) if(st_hist[c][i]) s2-=(double)st_hist[c][i]*log((double)st_hist[c][i]);
        fprintf(stderr,"  pclass %d: n=%llu  order0(residual)=%.0f B (%.3f bpp)\n",
                c,(unsigned long long)st_n[c], s2*1.4426950408889634/8,
                s2*1.4426950408889634/st_n[c]);
      }
    }
#endif
    if( cd.info.ok )
      fprintf( stderr, "%s: %dx%d %dbpp%s%s -> %d bytes (%.4f bpp), %.2fs\n",
               argv[2], cd.info.W, cd.info.H, cd.info.bpp,
               cd.info.comp==1 ? " rle8" : "", cd.expand ? " expanded" : "",
               n, 8.0*n/((double)cd.info.W*cd.info.H), sec );
    else
      fprintf( stderr, "%s: %ld -> %d bytes (generic), %.2fs\n", argv[2], L, n, sec );
  } else {
    rv = cd.decompress( in, (uint)L, g ) ? 0 : 3;
    fprintf( stderr, "%s -> %s, %.2fs\n", argv[2], argv[3], double(clock()-t0)/CLOCKS_PER_SEC );
  }

  fclose(g);
  if( mode != 'c' ) delete[] in;      // the encoder handed `in` to the codec
  return rv;
}
