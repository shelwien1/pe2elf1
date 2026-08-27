// Fused vs split decoder step -- exact, not a shape benchmark.
//
// One "decoder": NL interleaved coder lanes, bit i in lane i%NL, a serial
// FSM bit-tree model with ctx reset every 8 bits. All variants of one NL
// implement identical arithmetic and must finish with identical lane and
// model state; that equality is asserted, so the threshold reformulation
//     bit = (code >= (range>>15)*p)   <=>   bit = (p <= code/(range>>15))
// is verified end to end, not argued.
//
// The split: the scalar loop keeps the model chain and a p<=T[lane] compare;
// a 16-wide vector batch consumes the group's {p;bit} pairs and does the
// range/code update, renorm, refill and the T=code/ru division. At NL=16 the
// batch's result is needed by the very next group, so its full latency sits
// on the serial path. At NL=64 a group's batch is not consumed until four
// groups later, and the out-of-order window pipelines it behind the
// following scalar phases -- that is the whole point of the NL=64 variants.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
#include "rc_splitdec_ispc.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
static double now(){ timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }

enum { MAXL=64, STRIDE=1<<20, NG=2000000, NST=32768 };
static double CLK=2.8e9;

alignas(64) static u8  strm[MAXL*STRIDE+64];
alignas(64) static u16 fsm2[NST*2];
alignas(64) static u16 ppt[NST];
alignas(64) static u16 cst[256];
alignas(64) static u32 range[MAXL], code_[MAXL], pos[MAXL], ru[MAXL], T[MAXL];
alignas(64) static u32 look_[MAXL];
alignas(64) static u16 Tc[MAXL];
static u32 ctx;

static void init_tables( int hot ) {
  if( hot ) {
    // L1-resident model, skewed p, essentially no refill: brackets the real
    // decoder from the easy side (real data at 0.59 ratio refills at ~0.074
    // bytes/bit, this mode at ~0.000)
    for( u32 i=0; i<NST; i++ )
      ppt[i] = (i&1) ? (1+((i*2654435761u)>>20 & 0x1FFF))
                     : (32767-((i*2654435761u)>>20 & 0x1FFF));
    for( u32 i=0; i<NST*2; i++ ) fsm2[i] = (i*40503u)&0x3FF;
  } else {
    // max-entropy model, uniform p: brackets from the harsh side (~0.090
    // bytes/bit refill, mispredict-heavy renorm, L2-sized model walk)
    for( u32 i=0; i<NST; i++ )   ppt[i] = ((i*2654435761u)>>17 & 0x7FFF) | 1;
    for( u32 i=0; i<NST*2; i++ ) fsm2[i] = (i*40503u)&0x7FFF;
  }
  for( u32 i=0; i<MAXL*STRIDE+64; i++ ) strm[i]=(u8)(i*2654435761u>>24);
}
static void reset_state( u32 NL ) {
  for( u32 i=0; i<256; i++ ) cst[i]=0;
  ctx=1;
  for( u32 j=0; j<NL; j++ ) {
    range[j]=0xFFFFFFFFu;
    u32 c=0; for( u32 k=0; k<4; k++ ) c=(c<<8)+strm[j*STRIDE+k];
    code_[j]=c; pos[j]=4;
    ru[j]=range[j]>>15;
    T[j]=code_[j]/ru[j];
    Tc[j]=(u16)(T[j]<0x8000u?T[j]:0x8000u);
    look_[j]=(u32)strm[j*STRIDE+4] | ((u32)strm[j*STRIDE+5]<<8)
           | ((u32)strm[j*STRIDE+6]<<16) | ((u32)strm[j*STRIDE+7]<<24);
  }
}
static u64 fingerprint( u32 NL ) {
  u64 h=1469598103934665603ull;
  #define H(x) h=(h^(x))*1099511628211ull
  for( u32 j=0; j<NL; j++ ) { H(range[j]); H(code_[j]); H(pos[j]); }
  for( u32 i=0; i<256; i++ ) H(cst[i]);
  H(ctx);
  #undef H
  return h;
}

// ---------------- fused ----------------
template< u32 NL, int BRANCHLESS >
static void run_fused( void ) {
  for( u32 g=0; g<NG; g++ ) {
    u32 base=(g*16)%NL;
    for( u32 j=0; j<16; j++ ) {
      u32 L=base+j;
      if( (j&7)==0 ) ctx=1;
      u32 st=cst[ctx]; u32 p=ppt[st];
      u32 r=range[L], c=code_[L];
      u32 rp=(r>>15)*p;
      u32 bit=(c>=rp);
      r = bit ? r-rp : rp;
      c = bit ? c-rp : c;
      u32 n=(r<0x01000000u)+(r<0x00010000u);
      u32 q=pos[L];
      if( BRANCHLESS ) {
        u32 w=(u32(strm[L*STRIDE+q])<<8)+strm[L*STRIDE+q+1];
        c=(c<<(n*8))+(w>>((2-n)*8));
        q=(q+n)&(STRIDE-1);
      } else {
        for( u32 k=n; k!=0; k-- ) { c=(c<<8)+strm[L*STRIDE+q]; q=(q+1)&(STRIDE-1); }
      }
      r<<=n*8;
      range[L]=r; code_[L]=c; pos[L]=q;
      cst[ctx]=fsm2[st*2+bit];
      ctx=ctx*2+bit;
    }
  }
}

// ---------------- scalar half of the split ----------------
// T from Tc[] (u16, written by the batch >=0 groups ago), pb packed into
// four u64s by shift/or -- neither direction adds a load to the model chain
// beyond the Tc read, and nothing store-forwards from the same group at
// NL=64.
static inline void group_scalar( u32 base, u64* pk ) {
  u64 a0=0,a1=0,a2=0,a3=0;
  for( u32 j=0; j<16; j++ ) {
    if( (j&7)==0 ) ctx=1;
    u32 st=cst[ctx]; u32 p=ppt[st];
    u32 bit=(p<=T[base+j]);
    u64 v=(u64)((p<<1)|bit) << ((j&3)*16);
    switch( j>>2 ) { case 0: a0|=v; break; case 1: a1|=v; break;
                     case 2: a2|=v; break; default: a3|=v; }
    cst[ctx]=fsm2[st*2+bit];
    ctx=ctx*2+bit;
  }
  pk[0]=a0; pk[1]=a1; pk[2]=a2; pk[3]=a3;
}

// ---------------- the intrinsics batch ----------------
static const __m512i SBASE16 = _mm512_setr_epi32(0,STRIDE,2*STRIDE,3*STRIDE,
  4*STRIDE,5*STRIDE,6*STRIDE,7*STRIDE,8*STRIDE,9*STRIDE,10*STRIDE,11*STRIDE,
  12*STRIDE,13*STRIDE,14*STRIDE,15*STRIDE);
static inline void batch_intrin( u32 base, const u64* pk ) {
  const __m512i one=_mm512_set1_epi32(1);
  __m512i sbase=_mm512_add_epi32(SBASE16,_mm512_set1_epi32(base*STRIDE));
  __m512i rv=_mm512_load_si512((void*)(range+base)), cv=_mm512_load_si512((void*)(code_+base));
  __m512i qv=_mm512_load_si512((void*)(pos+base)),   uv=_mm512_load_si512((void*)(ru+base));
  __m512i lookv=_mm512_load_si512((void*)(look_+base));
  __m256i pb16=_mm256_set_epi64x(pk[3],pk[2],pk[1],pk[0]);
  __m512i pbv=_mm512_cvtepu16_epi32(pb16);
  __m512i pv=_mm512_srli_epi32(pbv,1);
  __mmask16 bm=_mm512_test_epi32_mask(pbv,one);
  __m512i rp=_mm512_mullo_epi32(uv,pv);
  rv=_mm512_mask_sub_epi32(rp,bm,rv,rp);
  cv=_mm512_mask_sub_epi32(cv,bm,cv,rp);
  __mmask16 m1=_mm512_cmplt_epu32_mask(rv,_mm512_set1_epi32(0x01000000));
  __mmask16 m2=_mm512_cmplt_epu32_mask(rv,_mm512_set1_epi32(0x00010000));
  __m512i n=_mm512_add_epi32(_mm512_maskz_set1_epi32(m1,1),_mm512_maskz_set1_epi32(m2,1));
  __m512i sh=_mm512_slli_epi32(n,3);
  const __m512i BSW=_mm512_broadcast_i32x4(_mm_setr_epi8(1,0,-1,-1,5,4,-1,-1,9,8,-1,-1,13,12,-1,-1));
  __m512i w=_mm512_shuffle_epi8(lookv,BSW);
  cv=_mm512_add_epi32(_mm512_sllv_epi32(cv,sh),
                      _mm512_srlv_epi32(w,_mm512_sub_epi32(_mm512_set1_epi32(16),sh)));
  rv=_mm512_sllv_epi32(rv,sh);
  qv=_mm512_and_si512(_mm512_add_epi32(qv,n),_mm512_set1_epi32(STRIDE-1));
  lookv=_mm512_i32gather_epi32(_mm512_add_epi32(qv,sbase),(const int*)strm,1);
  uv=_mm512_srli_epi32(rv,15);
  __m512d clo=_mm512_cvtepu32_pd(_mm512_castsi512_si256(cv));
  __m512d chi=_mm512_cvtepu32_pd(_mm512_extracti64x4_epi64(cv,1));
  __m512d ulo=_mm512_cvtepu32_pd(_mm512_castsi512_si256(uv));
  __m512d uhi=_mm512_cvtepu32_pd(_mm512_extracti64x4_epi64(uv,1));
  __m256i tlo=_mm512_cvttpd_epu32(_mm512_div_pd(clo,ulo));
  __m256i thi=_mm512_cvttpd_epu32(_mm512_div_pd(chi,uhi));
  __m512i tv=_mm512_inserti64x4(_mm512_castsi256_si512(tlo),thi,1);
  __m512i dd=_mm512_sub_epi32(cv,_mm512_mullo_epi32(tv,uv));
  __mmask16 fix=_mm512_cmpge_epu32_mask(dd,uv);
  tv=_mm512_mask_add_epi32(tv,fix,tv,one);
  _mm512_store_si512((void*)(T+base),tv);
  _mm512_store_si512((void*)(range+base),rv); _mm512_store_si512((void*)(code_+base),cv);
  _mm512_store_si512((void*)(pos+base),qv);   _mm512_store_si512((void*)(ru+base),uv);
  _mm512_store_si512((void*)(look_+base),lookv);
}

template< u32 NL >
static void run_split_intrin( void ) {
  u64 pk[4];
  for( u32 g=0; g<NG; g++ ) {
    u32 base=(g*16)%NL;
    group_scalar( base, pk );
    batch_intrin( base, pk );
  }
}

template< u32 NL >
static void run_split_ispc( void ) {
  u64 pk[4];
  for( u32 g=0; g<NG; g++ ) {
    u32 base=(g*16)%NL;
    group_scalar( base, pk );
    ispc::rc_batch2( range+base, code_+base, pos+base, ru+base, T+base,
                     look_+base, Tc+base, strm+base*STRIDE, STRIDE,
                     pk[0], pk[1], pk[2], pk[3] );
  }
}

static void run_block_ispc( void ) {
  ispc::rc_block( range, code_, pos, ru, T, strm, STRIDE, cst, fsm2, ppt, &ctx, NG );
}

static void run_floor( void ) {
  u64 pk[4];
  for( u32 g=0; g<NG; g++ ) group_scalar( 0, pk );
}

typedef void (*t_run)( void );
static const struct { const char* name; t_run run; u32 nl; } VAR[] = {
  { "NL=16 fused, branchy refill (current shape)", run_fused<16,0>,       16 },
  { "NL=16 fused, branchless counted refill     ", run_fused<16,1>,       16 },
  { "NL=16 split, intrinsics batch              ", run_split_intrin<16>,  16 },
  { "NL=16 split, ispc batch per group          ", run_split_ispc<16>,    16 },
  { "NL=16 split, whole loop in ispc            ", run_block_ispc,        16 },
  { "NL=64 fused, branchy refill                ", run_fused<64,0>,       64 },
  { "NL=64 fused, branchless counted refill     ", run_fused<64,1>,       64 },
  { "NL=64 split, intrinsics batch (pipelined)  ", run_split_intrin<64>,  64 },
  { "NL=64 split, ispc batch per group          ", run_split_ispc<64>,    64 },
  { "floor: model+decision only, no batch       ", run_floor,             0  },
};

int main( int argc, char** argv ) {
  if( argc>1 ) CLK=atof(argv[1])*1e9;
  int hot = (argc>2) && (argv[2][0]=='h');
  init_tables( hot );
  { reset_state(16); u64 nrf=0;
    for( u32 g=0; g<NG/16; g++ ) for( u32 j=0; j<16; j++ ) {
      if( (j&7)==0 ) ctx=1;
      u32 st=cst[ctx]; u32 p=ppt[st];
      u32 r=range[j], c=code_[j];
      u32 rp=(r>>15)*p; u32 bit=(c>=rp);
      r = bit ? r-rp : rp; c = bit ? c-rp : c;
      u32 n=(r<0x01000000u)+(r<0x00010000u); nrf+=n;
      u32 q=pos[j];
      u32 w=(u32(strm[j*STRIDE+q])<<8)+strm[j*STRIDE+q+1];
      c=(c<<(n*8))+(w>>((2-n)*8)); q=(q+n)&(STRIDE-1);
      r<<=n*8;
      range[j]=r; code_[j]=c; pos[j]=q;
      cst[ctx]=fsm2[st*2+bit]; ctx=ctx*2+bit;
    }
    printf( "mode=%s  refill: %.3f bytes/bit\n", hot?"hot(skewed)":"cold(max-entropy)",
            double(nrf)/(double(NG/16)*16) );
  }
  u64 ref16=0, ref64=0; int r16=0, r64=0;
  double bits = double(NG)*16;
  for( u32 v=0; v<sizeof(VAR)/sizeof(VAR[0]); v++ ) {
    double best=1e9; u64 fp=0;
    for( int rep=0; rep<4; rep++ ) {
      reset_state( VAR[v].nl ? VAR[v].nl : 16 );
      double t=now(); VAR[v].run(); t=now()-t;
      if( t<best ) best=t;
      fp=fingerprint( VAR[v].nl ? VAR[v].nl : 16 );
    }
    const char* eq="";
    if( VAR[v].nl==16 )      { if(!r16){ref16=fp;r16=1;eq=" [ref]";} else eq=(fp==ref16)?" [identical]":" [STATE MISMATCH]"; }
    else if( VAR[v].nl==64 ) { if(!r64){ref64=fp;r64=1;eq=" [ref]";} else eq=(fp==ref64)?" [identical]":" [STATE MISMATCH]"; }
    printf( "%s %6.2f clk/bit%s\n", VAR[v].name, best*CLK/bits, eq );
  }
  return 0;
}
