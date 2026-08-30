// Standalone probe -- NOT part of the build. Needs tree.h from iftree_gen.py.
// See misc/dec_vectorize.md section 9.
// Same question with N independent lanes interleaved, as the real decoder has.
// Lanes share the model (cty) and carry their own coder state and substream --
// the codebase's arrangement. The if-tree's per-node branch sites are now
// shared by every lane, so each site sees N interleaved outcome streams.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint32_t uint; typedef uint64_t qword; typedef uint8_t byte;
enum { SCALElog=15, SCALE=1<<SCALElog, sTOP=1u<<24, RATE=5, MAXL=16 };
static inline qword rdtsc(){ return __builtin_ia32_rdtsc(); }
static uint cty[512];
static void model_init(){ for(int i=0;i<512;i++) cty[i]=SCALE/2; }
#define upd(c,bit) { uint _p=cty[c];                                    \
    cty[c] = (bit) ? _p - (_p>>RATE) : _p + ((SCALE-_p)>>RATE);         \
    if(cty[c]<1) cty[c]=1; if(cty[c]>SCALE-1) cty[c]=SCALE-1; }

static byte*  ebuf[MAXL]; static size_t epos[MAXL];
static qword  e_low[MAXL]; static uint e_range[MAXL], e_cache[MAXL], e_cacheSize[MAXL];
static void e_init(int L,byte* b){ ebuf[L]=b; epos[L]=0; e_low[L]=0; e_range[L]=0xFFFFFFFFu;
  e_cache[L]=0; e_cacheSize[L]=1; }
static void shiftLow(int L){
  if( uint(e_low[L]>>32)!=0 || uint(e_low[L])<0xFF000000u ){
    do { ebuf[L][epos[L]++] = byte(e_cache[L] + uint(e_low[L]>>32)); e_cache[L]=0xFF; }
    while(--e_cacheSize[L]);
    e_cache[L] = byte(e_low[L]>>24);
  }
  e_cacheSize[L]++; e_low[L] = uint(e_low[L])<<8;
}
static void e_bit(int L, uint c, uint bit){
  uint rpre=(e_range[L]>>SCALElog)*cty[c];
  if(bit){ e_low[L]+=rpre; e_range[L]-=rpre; } else e_range[L]=rpre;
  upd(c,bit)
  while(e_range[L]<sTOP){ shiftLow(L); e_range[L]<<=8; }
}
static const byte* dbuf_[MAXL]; static size_t dpos_[MAXL];
static uint d_code_[MAXL], d_range_[MAXL];
static void d_init(int L,const byte* b){ dbuf_[L]=b; dpos_[L]=0; d_code_[L]=0;
  d_range_[L]=0xFFFFFFFFu; dpos_[L]++; for(int i=0;i<4;i++) d_code_[L]=(d_code_[L]<<8)|b[dpos_[L]++]; }

static const byte* dbuf; static size_t dpos;      // tree.h reads these
#define RENORM while( range<sTOP ){ code=(code<<8)|dbuf[dpos++]; range<<=8; }
static uint decA(int L){
  uint code=d_code_[L], range=d_range_[L], ctx=1; dbuf=dbuf_[L]; dpos=dpos_[L];
  for(int k=0;k<8;k++){
    uint rpre=(range>>SCALElog)*cty[ctx];
    uint bit=(code>=rpre);
    if(bit){ range-=rpre; code-=rpre; } else range=rpre;
    upd(ctx,bit) ctx=ctx*2+bit; RENORM
  }
  d_code_[L]=code; d_range_[L]=range; dpos_[L]=dpos; return ctx&0xFF;
}
static uint decB(int L){
  uint code=d_code_[L], range=d_range_[L], rpre, sym=0; dbuf=dbuf_[L]; dpos=dpos_[L];
#include "tree.h"
  d_code_[L]=code; d_range_[L]=range; dpos_[L]=dpos; return sym&0xFF;
}
int main(int argc,char**argv){
  const char* path=argc>1?argv[1]:"enwik8";
  size_t N=argc>2?strtoul(argv[2],0,10):(8u<<20);
  int NL=argc>3?atoi(argv[3]):1;
  FILE* f=fopen(path,"rb"); if(!f){printf("no file\n");return 1;}
  byte* src=(byte*)malloc(N); N=fread(src,1,N,f); fclose(f);
  size_t chunk=N/NL; N=chunk*NL;
  byte* enc[MAXL]; byte* out[MAXL];
  for(int L=0;L<NL;L++){ enc[L]=(byte*)malloc(chunk*2+4096); out[L]=(byte*)malloc(chunk); }
  model_init(); for(int L=0;L<NL;L++) e_init(L,enc[L]);
  for(size_t i=0;i<chunk;i++) for(int L=0;L<NL;L++){
    uint c=1,s=src[L*chunk+i];
    for(int k=7;k>=0;k--){ uint b=(s>>k)&1; e_bit(L,c,b); c=c*2+b; } }
  for(int L=0;L<NL;L++) for(int i=0;i<5;i++) shiftLow(L);
  size_t tot=0; for(int L=0;L<NL;L++) tot+=epos[L];
  const int R=9; double va[R],vb[R];
  for(int r=0;r<R;r++){
    model_init(); for(int L=0;L<NL;L++) d_init(L,enc[L]);
    qword t0=rdtsc();
    for(size_t i=0;i<chunk;i++) for(int L=0;L<NL;L++) out[L][i]=(byte)decA(L);
    qword t1=rdtsc(); va[r]=double(t1-t0)/N;
    model_init(); for(int L=0;L<NL;L++) d_init(L,enc[L]);
    t0=rdtsc();
    for(size_t i=0;i<chunk;i++) for(int L=0;L<NL;L++) out[L][i]=(byte)decB(L);
    t1=rdtsc(); vb[r]=double(t1-t0)/N;
  }
  for(int i=0;i<R;i++)for(int j=i+1;j<R;j++){
    if(va[j]<va[i]){double t=va[i];va[i]=va[j];va[j]=t;}
    if(vb[j]<vb[i]){double t=vb[i];vb[i]=vb[j];vb[j]=t;} }
  int bad=0; for(int L=0;L<NL;L++) if(memcmp(out[L],src+L*chunk,chunk)) bad++;
  printf("lanes %2d  bpc %.4f  A %7.2f  B %7.2f  ->  B is %.2fx %s   %s\n",
    NL, 8.0*tot/N, va[R/2], vb[R/2],
    vb[R/2]>va[R/2]?vb[R/2]/va[R/2]:va[R/2]/vb[R/2],
    vb[R/2]>va[R/2]?"SLOWER":"faster", bad?"WRONG":"ok");
  return 0;
}
