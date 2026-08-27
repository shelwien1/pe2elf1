// Dependency-shape benchmark for a 16-lane vectorised decoder step.
// Not a decoder: same chain, same instruction mix, arbitrary data.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
typedef uint32_t u32;
static double now(){ timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
enum { LANES=16, NBYTE=3*1000*1000, NST=32768 };
alignas(64) static u32 cellS[LANES*256];   // {state}
alignas(64) static u32 cellSP[LANES*256];  // {p<<16 | state}
alignas(64) static u32 ppA[NST];           // p as dwords            (128 KB)
alignas(64) static u32 fsmD[NST*2];        // successor state, [st*2+bit] (256 KB)
alignas(64) static u32 fsmP[NST*2];        // {p<<16 | state} successor   (256 KB)
alignas(64) static uint8_t strm[LANES][1<<20];
static double CLK=2.8e9;
static const u32 sTOP=0x01000000u, gTOP=0x00010000u;
#define REPORT(nm,t) printf("%-52s %6.2f clk/bit  %8.1f Mbit/s\n",nm,(t)*CLK/(double(NBYTE)*8*LANES),(double(NBYTE)*8*LANES)/(t)/1e6)

int main(int argc,char**argv){
  if(argc>1) CLK=atof(argv[1])*1e9;
  for(u32 i=0;i<LANES*256;i++){ cellS[i]=i*7919u&0x7FFF; cellSP[i]=((i*2654435761u)&0x7FFF0000u)|0x10000|(i*7919u&0x7FFF); }
  for(u32 i=0;i<NST;i++) ppA[i]=1+((i*2654435761u)>>17);
  for(u32 i=0;i<NST*2;i++){ u32 s=(i*40503u)&0x7FFF; fsmD[i]=s; fsmP[i]=(ppA[s]<<16)|s; }
  for(u32 l=0;l<LANES;l++) for(u32 i=0;i<(1<<20);i++) strm[l][i]=(uint8_t)(i*31+l);

  __m512i lanebase=_mm512_setr_epi32(0,256,512,768,1024,1280,1536,1792,2048,2304,2560,2816,3072,3328,3584,3840);
  __m512i strmbase=_mm512_setr_epi32(0,1<<20,2<<20,3<<20,4<<20,5<<20,6<<20,7<<20,8<<20,9<<20,10<<20,11<<20,12<<20,13<<20,14<<20,15<<20);
  __m512i one=_mm512_set1_epi32(1), c32=_mm512_set1_epi32(32);
  __m512i mSTATE=_mm512_set1_epi32(0x7FFF), mP=_mm512_set1_epi32(0x7FFE);

  for(int V=0;V<6;V++){
    __m512i ctx=one,acc=_mm512_setzero_si512();
    __m512i range=_mm512_set1_epi32(0xFFFFFFFFu),code=_mm512_set1_epi32(0x12345678u),pos=_mm512_set1_epi32(8);
    __m512i qi[8],qv[8]; __m512i sink=_mm512_setzero_si512();
    double t=now();
    for(u32 b=0;b<NBYTE;b++){ ctx=one;
      for(int j=0;j<8;j++){
        // ---- renorm + branchless refill (gather, on the chain) ----
        __mmask16 m1=_mm512_cmplt_epu32_mask(range,_mm512_set1_epi32(sTOP));
        __mmask16 m2=_mm512_cmplt_epu32_mask(range,_mm512_set1_epi32(gTOP));
        __m512i n=_mm512_add_epi32(_mm512_maskz_set1_epi32(m1,1),_mm512_maskz_set1_epi32(m2,1));
        __m512i sh=_mm512_slli_epi32(n,3);
        __m512i v;
        if(V==5){ // interleaved container: one contiguous load + a permute to
                  // hand each lane its own bytes -- no gather
          v=_mm512_loadu_si512((const void*)(strm[0]+(b&0xFFFF)*4));
          v=_mm512_permutexvar_epi32(_mm512_and_si512(_mm512_add_epi32(pos,n),_mm512_set1_epi32(15)),v);
        } else
        v=_mm512_i32gather_epi32(_mm512_add_epi32(pos,strmbase),(const int*)strm,1);
        code=_mm512_add_epi32(_mm512_sllv_epi32(code,sh),_mm512_srlv_epi32(v,_mm512_sub_epi32(c32,sh)));
        range=_mm512_sllv_epi32(range,sh); pos=_mm512_add_epi32(pos,n);
        // ---- model read ----
        __m512i idx=_mm512_add_epi32(ctx,lanebase), st, p;
        if(V>=2){ __m512i cell=_mm512_i32gather_epi32(idx,cellSP,4);
                  st=_mm512_and_si512(cell,mSTATE); p=_mm512_srli_epi32(cell,16); }
        else    { st=_mm512_and_si512(_mm512_i32gather_epi32(idx,cellS,4),mSTATE);
                  p=_mm512_i32gather_epi32(st,ppA,4); }
        p=_mm512_or_si512(_mm512_and_si512(p,mP),one);
        // ---- coder ----
        __m512i rpre=_mm512_mullo_epi32(_mm512_srli_epi32(range,15),p);
        __mmask16 m=_mm512_cmpge_epu32_mask(code,rpre);
        range=_mm512_mask_sub_epi32(rpre,m,range,rpre);
        code =_mm512_mask_sub_epi32(code,m,code,rpre);
        // ---- model write-back (off the chain) ----
        if(V){ __m512i k=_mm512_add_epi32(_mm512_add_epi32(st,st),_mm512_maskz_set1_epi32(m,1));
               if(V==1)      _mm512_i32scatter_epi32(cellS ,idx,_mm512_i32gather_epi32(k,fsmD,4),4);
               else if(V==2) _mm512_i32scatter_epi32(cellSP,idx,_mm512_i32gather_epi32(k,fsmP,4),4);
               else if(V==3) sink=_mm512_add_epi32(sink,_mm512_i32gather_epi32(k,fsmP,4));
               else if(V==4){ qi[j]=idx; qv[j]=_mm512_i32gather_epi32(k,fsmP,4); }
               else { // arithmetic counter: p += (bit ? -p : (SCALE-p)) >> 5
                 __m512i d=_mm512_mask_sub_epi32(_mm512_sub_epi32(_mm512_set1_epi32(32768),p),m,_mm512_setzero_si512(),p);
                 _mm512_i32scatter_epi32(cellSP,idx,_mm512_add_epi32(p,_mm512_srai_epi32(d,5)),4); } }
        ctx=_mm512_mask_add_epi32(_mm512_add_epi32(ctx,ctx),m,_mm512_add_epi32(ctx,ctx),one);
      }
      if(V==4) for(int k2=0;k2<8;k2++) _mm512_i32scatter_epi32(cellSP,qi[k2],qv[k2],4);
      pos=_mm512_and_si512(pos,_mm512_set1_epi32((1<<20)-8));
      acc=_mm512_add_epi32(acc,ctx);
    }
    t=now()-t;
    REPORT(V==0?"chain only, separate pp[] (no model write-back)"
          :V==1?"realistic: cell={state},   2 gathers on chain"
          :V==2?"realistic: cell={state,p}, 1 gather on chain"
          :V==3?"  ... same, model scatter removed (FSM gather kept)"
          :V==4?"  ... same, 8 scatters batched at the byte boundary"
                :"target: contiguous refill + arithmetic counter (1 gather)",t);
    if(_mm512_reduce_add_epi32(_mm512_add_epi32(acc,sink))==123)puts("");
  }
  return 0;
}
