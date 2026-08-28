// Does depth-4 speculation shorten the model chain?  Coder removed from both
// sides: the bit is a compare of p against a per-depth value that does NOT
// depend on the model -- which is exactly the decoder's situation, since the
// eight bits of a byte go to eight different coder lanes whose (range,code)
// were fixed sixteen bits earlier.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
typedef uint32_t u32;
static double now(){ timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
enum { NBYTE=20*1000*1000, NST=32768 };
alignas(64) static u32 cell[256];
alignas(64) static u32 fsmP[NST*2];
alignas(64) static u32 thr[16];
alignas(64) static u32 scratch[16];
static double CLK=2.8e9;
#define REP(nm,t) printf("%-50s %6.2f clk/bit\n",nm,(t)*CLK/(double(NBYTE)*8))

int main(int argc,char**argv){
  if(argc>1) CLK=atof(argv[1])*1e9;
  for(u32 i=0;i<256;i++) cell[i]=((i*2654435761u)&0x7FFE0000u)|0x10000|((i*7919u)&0x7FFF);
  for(u32 i=0;i<NST*2;i++){ u32 s=(i*40503u)&0x7FFF; fsmP[i]=(((s*2654435761u)&0x7FFE0000u)|0x10000)|s; }
  for(u32 i=0;i<16;i++) thr[i]=0x30000000u+i*0x01234567u;

  { u32 acc=0; double t=now();
    for(u32 b=0;b<NBYTE;b++){ u32 ctx=1;
      for(u32 j=0;j<8;j++){
        u32 cl=cell[ctx], p=cl>>16, st=cl&0x7FFF;
        u32 bit = ((p*0x8001u) >= thr[j]);
        cell[ctx]=fsmP[st*2+bit];
        ctx=ctx*2+bit;
      }
      acc+=ctx;
    }
    t=now()-t; REP("serial: eight dependent model steps per byte",t); if(acc==7)puts(""); }

  const __m512i DEPTH=_mm512_setr_epi32(0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3);
  const __m512i MUL=_mm512_set1_epi32(0x8001u);
  { u32 acc=0; double t=now();
    __m512i t4a=_mm512_permutexvar_epi32(DEPTH,_mm512_setr_epi32(thr[0],thr[1],thr[2],thr[3],0,0,0,0,0,0,0,0,0,0,0,0));
    __m512i t4b=_mm512_permutexvar_epi32(DEPTH,_mm512_setr_epi32(thr[4],thr[5],thr[6],thr[7],0,0,0,0,0,0,0,0,0,0,0,0));
    for(u32 b=0;b<NBYTE;b++){ u32 root=1;
      for(u32 r=0;r<2;r++){
        // the 15 cells of the depth-4 subtree rooted at `root`: four contiguous
        // runs of 1,2,4,8 landing at vector positions 1, 2-3, 4-7, 8-15
        __m512i cl=_mm512_maskz_loadu_epi32(0x0002,(const void*)(cell+root-1));
        cl=_mm512_mask_loadu_epi32(cl,0x000C,(const void*)(cell+2*root-2));
        cl=_mm512_mask_loadu_epi32(cl,0x00F0,(const void*)(cell+4*root-4));
        cl=_mm512_mask_loadu_epi32(cl,0xFF00,(const void*)(cell+8*root-8));
        __m512i p=_mm512_srli_epi32(cl,16);
        __mmask16 mk=_mm512_cmpge_epu32_mask(_mm512_mullo_epi32(p,MUL),r?t4b:t4a);
        _mm512_store_si512((void*)scratch,cl);
        u32 k=1;
        for(u32 t2=0;t2<4;t2++) k=2*k+((u32(mk)>>k)&1);     // the whole walk
        // commit the four taken cells (off the chain)
        u32 kk=k, ct=root*8+(k-8);
        for(u32 t2=0;t2<4;t2++){ u32 st=scratch[kk]&0x7FFF;
          cell[ct]=fsmP[st*2+(kk&1)]; kk>>=1; ct>>=1; }
        root=root*8+(k-8); root=(root&127)|1;
      }
      acc+=root;
    }
    t=now()-t; REP("speculative: two depth-4 rounds per byte",t); if(acc==7)puts(""); }
  return 0;
}
