// iftree_rans.cpp -- the rANS twin of iftree_bench.cpp. Standalone probe, NOT
// part of the build. Needs rans_tree_{T,U}.h from iftree_rans_gen.py.
//
//   python3 iftree_rans_gen.py && clang++ -O3 -march=native -o ir iftree_rans.cpp
//   ./ir ../../enwik8 [reps]
//
// Same coder as rans.inc (32-bit state, byte renorm, SCALElog=15, KLOG=8,
// refill before the step, LIFO substreams read downwards) and the same lane
// assignment as model1.inc (bit i -> lane i%16, so byte m of a group is lanes
// m*8+j). The counter is rans_shapes.cpp's synthesised 256-state FSM2 shape.
// Only the DECODER's model walk varies:
//   A  model1.inc's wavefront: NB=2 interleaved byte chains, indexed cty[lctx]
//   T  the 255-node if-tree with the coder step inline at every node
//   U  the if-tree deciding bits only, from a precomputed s[], with the eight
//      coder updates run after the byte
//
// Byte-sequential and interleaved walks are equivalent, which is what lets the
// tree exist at all: at bit j the context is in [2^j, 2^(j+1)), so two bytes
// can only collide at the SAME depth, and at that depth byte 0 writes before
// byte 1 reads in both orders.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
typedef uint32_t uint; typedef uint8_t byte; typedef uint16_t word;
enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, KLOG=8, RANSL=SCALE<<KLOG, XSH=KLOG+8 };
enum { RCNUM=16, NB=RCNUM/8, BLKSIZE=1<<16, PAD=64, ROW=1<<17 };
#define UNROLL _Pragma("clang loop unroll(full)")
#define NOINLINE __attribute__((noinline))
static inline uint load32( const byte* p ){ uint w; memcpy(&w,p,4); return w; }
static inline void store16( byte* p, uint w ){ word v=word(w); memcpy(p,&v,2); }

static uint FSM2[256*2];
static void fsm_init(){
  for( int st=0; st<256; st++ ) for( int b=0; b<2; b++ ){
    int n0=st&15, n1=st>>4; if(b) n1++; else n0++;
    if( n0>15 || n1>15 ){ n0=(n0+1)>>1; n1=(n1+1)>>1; }
    int p=((2*n0+1)*SCALE)/(2*(n0+n1)+2); if(p<1)p=1; if(p>SCALE-1)p=SCALE-1;
    FSM2[st*2+b]=(uint(n0|(n1<<4))<<16)|uint(p);
  }
}
struct Counter { uint v;
  inline uint Get() const { return v; }
  static inline uint P( uint st ){ return st & 0xFFFF; }
  inline void Update( uint st, uint bit ){ v = FSM2[(st>>16)*2+bit]; }
  void Reset(){ v = (0u<<16)|(SCALE/2); } };
static Counter cty_[0x200] __attribute__((aligned(64)));
static void model_reset(){ for(int i=0;i<0x200;i++) cty_[i].Reset(); }

static word pbit[BLKSIZE*8];
static byte rows[RCNUM*ROW];
static void encode_block( const byte* in, uint blksize, uint* lens ){
  Counter* __restrict cty = cty_;
  uint lctx=1;
  for( uint i=0;i<blksize;i++ ){ uint c=in[i];
    for( uint j=0;j<8;j++ ){ uint st=cty[lctx].Get(), p=Counter::P(st), bit=(c>>(7-j))&1;
      pbit[i*8+j]=(bit<<15)+p; cty[lctx].Update(st,bit); lctx=2*lctx+bit; }
    lctx=1; }
  uint x[RCNUM], ptr[RCNUM];
  for( uint k=0;k<RCNUM;k++ ){ x[k]=RANSL; ptr[k]=k*ROW+PAD; }
  for( uint i=blksize*8; i>0; ){
    --i; uint k=i%RCNUM; uint b=pbit[i], p=b&mSCALE; b>>=15;
    uint f = b ? SCALE-p : p, c = b ? p : 0; uint xm = f<<XSH;
    while( x[k]>=xm ){ rows[ptr[k]++]=byte(x[k]); x[k]>>=8; }
    uint q=x[k]/f; x[k]=(q<<SCALElog)+(x[k]-q*f)+c;
  }
  for( uint k=0;k<RCNUM;k++ ){
    for( int t=0;t<4;t++ ){ rows[ptr[k]++]=byte(x[k]); x[k]>>=8; }
    lens[k]=ptr[k]-(k*ROW+PAD); }
}

#define LANE_INIT() \
  for( uint k=0;k<RCNUM;k++ ){ tp[k]=off[k]+lens[k]-1; rx[k]=load32(base+tp[k]-3); tp[k]-=4; }

NOINLINE static void decode_A( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint lctx[NB], st[NB], bt[NB], p;
    UNROLL for( uint m=0;m<NB;m++ ) lctx[m]=1;
    UNROLL for( uint j=0;j<8;j++ ){
      UNROLL for( uint m=0;m<NB;m++ ){
        const uint k=m*8+j;
        st[m]=cty[lctx[m]].Get(); p=Counter::P(st[m]);
        if( __builtin_expect_with_probability( rx[k]<RANSL, 0, 0.99 ) ){
          uint dsh=(rx[k]<(RANSL>>8))?16:8;
          rx[k]<<=dsh; rx[k]|=load32(base+tp[k]-3)>>(32-dsh); tp[k]-=dsh>>3; }
        uint s=rx[k]&mSCALE, a=p*(rx[k]>>SCALElog);
        bt[m]=(s>=p); rx[k]=bt[m]?(rx[k]-p-a):(a+s);
        cty[lctx[m]].Update(st[m],bt[m]); lctx[m]=2*lctx[m]+bt[m];
      } }
    store16( out+i/8, (lctx[0]&0xFF)|((lctx[1]&0xFF)<<8) );
  }
}

NOINLINE static void decode_T( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint _sym, c0, c1;
    { const uint _base=0;
#include "rans_tree_T.h"
      c0=_sym&0xFF; }
    { const uint _base=8;
#include "rans_tree_T.h"
      c1=_sym&0xFF; }
    store16( out+i/8, c0|(c1<<8) );
  }
}

NOINLINE static void decode_U( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM], sv[RCNUM], pv[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint _sym, c[NB];
    UNROLL for( uint m=0;m<NB;m++ ){
      // pre: renormalise the eight lanes of this byte and take their s.
      // Legal because each lane owns one of the group's bit slots.
      UNROLL for( uint j=0;j<8;j++ ){ const uint k=m*8+j;
        if( __builtin_expect_with_probability( rx[k]<RANSL, 0, 0.99 ) ){
          uint dsh=(rx[k]<(RANSL>>8))?16:8;
          rx[k]<<=dsh; rx[k]|=load32(base+tp[k]-3)>>(32-dsh); tp[k]-=dsh>>3; }
        sv[k]=rx[k]&mSCALE; }
      { const uint _base=m*8;
#include "rans_tree_U.h"
        c[m]=_sym&0xFF; }
      UNROLL for( uint j=0;j<8;j++ ){ const uint k=m*8+j;
        const uint b=(c[m]>>(7-j))&1, p=pv[k], a=p*(rx[k]>>SCALElog);
        rx[k] = b ? (rx[k]-p-a) : (a+sv[k]); }
    }
    store16( out+i/8, c[0]|(c[1]<<8) );
  }
}

// S: A's indexed walk, but BYTE-SEQUENTIAL like the trees -- byte 0's eight
// bits, then byte 1's. The diagnostic that separates the tree from the walk
// order: if S lands near T, the trees' loss is the wavefront's ILP and not the
// tree; if S lands near A, it is the tree.
NOINLINE static void decode_S( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint c[NB];
    UNROLL for( uint m=0;m<NB;m++ ){
      uint lctx=1;
      UNROLL for( uint j=0;j<8;j++ ){
        const uint k=m*8+j;
        uint st=cty[lctx].Get(), p=Counter::P(st);
        if( __builtin_expect_with_probability( rx[k]<RANSL, 0, 0.99 ) ){
          uint dsh=(rx[k]<(RANSL>>8))?16:8;
          rx[k]<<=dsh; rx[k]|=load32(base+tp[k]-3)>>(32-dsh); tp[k]-=dsh>>3; }
        uint s=rx[k]&mSCALE, a=p*(rx[k]>>SCALElog);
        uint b=(s>=p); rx[k]=b?(rx[k]-p-a):(a+s);
        cty[lctx].Update(st,b); lctx=2*lctx+b; }
      c[m]=lctx&0xFF; }
    store16( out+i/8, c[0]|(c[1]<<8) );
  }
}

// P<D>: U's shape with the tree cut off at depth D -- the well-predicted top
// of the context tree as branches, the rest as the indexed walk.
template<int D, int WHICH>
NOINLINE static void decode_P( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM], sv[RCNUM], pv[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint c[NB];
    UNROLL for( uint m=0;m<NB;m++ ){
      UNROLL for( uint j=0;j<8;j++ ){ const uint k=m*8+j;
        if( __builtin_expect_with_probability( rx[k]<RANSL, 0, 0.99 ) ){
          uint dsh=(rx[k]<(RANSL>>8))?16:8;
          rx[k]<<=dsh; rx[k]|=load32(base+tp[k]-3)>>(32-dsh); tp[k]-=dsh>>3; }
        sv[k]=rx[k]&mSCALE; }
      uint _lctx;
      { const uint _base=m*8;
        if( WHICH==2 ) {
#include "rans_tree_P2.h"
        } else if( WHICH==3 ) {
#include "rans_tree_P3.h"
        } else {
#include "rans_tree_P4.h"
        } }
      for( uint j=D;j<8;j++ ){ const uint k=m*8+j;
        uint st=cty[_lctx].Get(), p=Counter::P(st);
        uint b=(sv[k]>=p); pv[k]=p;
        cty[_lctx].Update(st,b); _lctx=2*_lctx+b; }
      c[m]=_lctx&0xFF;
      UNROLL for( uint j=0;j<8;j++ ){ const uint k=m*8+j;
        const uint b=(c[m]>>(7-j))&1, p=pv[k], a=p*(rx[k]>>SCALElog);
        rx[k] = b ? (rx[k]-p-a) : (a+sv[k]); }
    }
    store16( out+i/8, c[0]|(c[1]<<8) );
  }
}

// SD / TD: S and T with the branch made to WAIT for the multiply, the way the
// range coder's `code >= (range>>SCALElog)*p` does. The asm is empty, so the
// only thing it adds is the dependency. If the multiply's POSITION is what
// decides whether the tree pays, then putting it on the branch path should
// hurt the flat walk more than the tree -- the tree can speculate past the
// branch and the walk cannot -- and TD/SD should be better than T/S.
NOINLINE static void decode_SD( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint c[NB];
    UNROLL for( uint m=0;m<NB;m++ ){
      uint lctx=1;
      UNROLL for( uint j=0;j<8;j++ ){
        const uint k=m*8+j;
        uint st=cty[lctx].Get(), p=Counter::P(st);
        if( __builtin_expect_with_probability( rx[k]<RANSL, 0, 0.99 ) ){
          uint dsh=(rx[k]<(RANSL>>8))?16:8;
          rx[k]<<=dsh; rx[k]|=load32(base+tp[k]-3)>>(32-dsh); tp[k]-=dsh>>3; }
        uint s=rx[k]&mSCALE, a=p*(rx[k]>>SCALElog);
        uint sd=s; __asm__("" : "+r"(sd) : "r"(a));
        uint b=(sd>=p); rx[k]=b?(rx[k]-p-a):(a+s);
        cty[lctx].Update(st,b); lctx=2*lctx+b; }
      c[m]=lctx&0xFF; }
    store16( out+i/8, c[0]|(c[1]<<8) );
  }
}
NOINLINE static void decode_TD( const byte* __restrict base, const uint* off, const uint* lens, uint blksize, byte* __restrict out ){
  Counter* __restrict cty = cty_;
  uint rx[RCNUM], tp[RCNUM];
  LANE_INIT();
  uint nb1=blksize*8;
  for( uint i=0;i<nb1;i+=RCNUM ){
    uint _sym, c0, c1;
    { const uint _base=0;
#include "rans_tree_TD.h"
      c0=_sym&0xFF; }
    { const uint _base=8;
#include "rans_tree_TD.h"
      c1=_sym&0xFF; }
    store16( out+i/8, c0|(c1<<8) );
  }
}

typedef void (*t_dec)( const byte*, const uint*, const uint*, uint, byte* );
static double now(){ return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int main( int argc, char** argv ){
  if( argc<2 ){ fprintf(stderr,"iftree_rans <file> [reps]\n"); return 1; }
  int reps = argc>2?atoi(argv[2]):3;
  fsm_init();
  FILE* f=fopen(argv[1],"rb"); if(!f) return 2;
  fseek(f,0,SEEK_END); size_t n=ftell(f); fseek(f,0,SEEK_SET);
  size_t n2=n+(n&1);
  byte* in=(byte*)calloc(n2+16,1); if( fread(in,1,n,f)!=n ) return 3; fclose(f);
  uint nblk=uint((n2+BLKSIZE-1)/BLKSIZE);
  enum { GUARD=1<<17 };
  byte* enc=(byte*)calloc( GUARD+n2+n2/4+(size_t)nblk*RCNUM*(PAD+8)+1024, 1 );
  uint* off=(uint*)malloc((size_t)nblk*RCNUM*4);
  uint* len=(uint*)malloc((size_t)nblk*RCNUM*4);
  uint* bsz=(uint*)malloc((size_t)nblk*4);
  model_reset(); size_t o=GUARD;
  for( uint b=0;b<nblk;b++ ){
    uint blksize=uint((n2-(size_t)b*BLKSIZE)<BLKSIZE?(n2-(size_t)b*BLKSIZE):BLKSIZE);
    bsz[b]=blksize;
    encode_block(in+(size_t)b*BLKSIZE, blksize, len+b*RCNUM);
    for( uint k=0;k<RCNUM;k++ ){ memset(enc+o,0,PAD); o+=PAD; off[b*RCNUM+k]=uint(o);
      memcpy(enc+o, rows+k*ROW+PAD, len[b*RCNUM+k]); o+=len[b*RCNUM+k]; } }
  printf("input %zu B, %u blocks, coded %zu B (%.4f bpc)\n", n, nblk,
         o-GUARD-(size_t)nblk*RCNUM*PAD, 8.0*(o-GUARD-(size_t)nblk*RCNUM*PAD)/n);
  struct { const char* name; t_dec fn; } D[] = {
    { "A  wavefront, indexed cty[lctx] (model1.inc's shape)", decode_A },
    { "S  indexed walk but byte-sequential (the ILP control)  ", decode_S },
    { "T  255-node if-tree, coder step inline at each node ", decode_T },
    { "U  255-node if-tree, bits only, updates after the byte", decode_U },
    { "SD S, but the bit made to wait for the multiply       ", decode_SD },
    { "TD T, but the bit made to wait for the multiply       ", decode_TD },
    { "P2 if-tree for 2 levels, then the indexed walk        ", decode_P<2,2> },
    { "P3 if-tree for 3 levels, then the indexed walk        ", decode_P<3,3> },
    { "P4 if-tree for 4 levels, then the indexed walk        ", decode_P<4,4> },
  };
  byte* out=(byte*)malloc(n2+16);
  for( auto& d : D ){
    double best=1e9; int ok=0;
    for( int r=0;r<reps;r++ ){
      model_reset(); memset(out,0,n2);
      double t1=now();
      for( uint b=0;b<nblk;b++ ) d.fn( enc, off+b*RCNUM, len+b*RCNUM, bsz[b], out+(size_t)b*BLKSIZE );
      double dt=now()-t1; if(dt<best) best=dt;
      ok = memcmp(out,in,n2)==0; }
    printf("%-56s %s  %7.2f MB/s  %6.3f ns/bit\n", d.name, ok?"OK  ":"FAIL", n/best/1e6, best*1e9/(n*8.0));
  }
  return 0;
}
