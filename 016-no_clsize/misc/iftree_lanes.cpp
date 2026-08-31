// Standalone probe -- NOT part of the build. Needs tree_lanes.h from
// iftree_lanes_gen.py.  See misc/iftree_lanes.md.
//
//   python3 iftree_lanes_gen.py && clang++ -O2 -march=native -o il iftree_lanes.cpp
//   ./il <enwik8> <megabytes>
//
// The codebase's lane arrangement: byte m's bit j is coded by lane m*8+j, so a
// byte's eight bits come from eight INDEPENDENT coder instances whose
// code/range are all known when the byte starts.  The only serial edge across
// a byte is the model context.  Three decoders over the same stream:
//
//   A  the loop, one byte at a time            (model1.inc's tail form)
//   W  the loop, NB bytes interleaved          (model1.inc's wavefront)
//   B  the generated if-tree, one byte at a time
//
// A and B code identically -- same counter update order, same stream.  W has a
// different update order, so it gets its own stream; it is here because it is
// what the real decoder actually runs, and the tree has to beat THAT.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint32_t uint; typedef uint64_t qword; typedef uint8_t byte;
enum { SCALElog=15, SCALE=1<<SCALElog, sTOP=1u<<24, RATE=5, NL=16, NB=NL/8 };
static inline qword rdtsc(){ return __builtin_ia32_rdtsc(); }

static uint cty[512];
static void model_init(){ for(int i=0;i<512;i++) cty[i]=SCALE/2; }
#define upd(c,bit) { uint _p=cty[c];                                    \
    cty[c] = (bit) ? _p - (_p>>RATE) : _p + ((SCALE-_p)>>RATE);         \
    if(cty[c]<1) cty[c]=1; if(cty[c]>SCALE-1) cty[c]=SCALE-1; }

//--- encoder: one carry-propagating instance per lane ------------------------
static byte* ebuf[NL]; static size_t epos[NL];
static qword elow[NL]; static uint erng[NL], ecache[NL], ecsz[NL];
static void e_init(){ for(int L=0;L<NL;L++){ epos[L]=0; elow[L]=0; erng[L]=0xFFFFFFFFu;
                                             ecache[L]=0; ecsz[L]=1; } }
static void shiftLow(int L){
  if( uint(elow[L]>>32)!=0 || uint(elow[L])<0xFF000000u ){
    do { ebuf[L][epos[L]++] = byte(ecache[L] + uint(elow[L]>>32)); ecache[L]=0xFF; }
    while(--ecsz[L]);
    ecache[L] = byte(elow[L]>>24);
  }
  ecsz[L]++; elow[L] = uint(elow[L])<<8;
}
static void e_bit(int L, uint c, uint bit){
  uint rp=(erng[L]>>SCALElog)*cty[c];
  if(bit){ elow[L]+=rp; erng[L]-=rp; } else erng[L]=rp;
  upd(c,bit)
  while(erng[L]<sTOP){ shiftLow(L); erng[L]<<=8; }
}
static void e_flush(){ for(int L=0;L<NL;L++) for(int i=0;i<5;i++) shiftLow(L); }

//--- decoder state -----------------------------------------------------------
static const byte* dsrc[NL]; static size_t dpos[NL];
static uint cod[NL], rng[NL];
static void d_init(){ for(int L=0;L<NL;L++){ dpos[L]=0; rng[L]=0xFFFFFFFFu; cod[L]=0;
    dpos[L]++; for(int i=0;i<4;i++) cod[L]=(cod[L]<<8)|dsrc[L][dpos[L]++]; } }
#define RENORM(L) while( rng[L]<sTOP ){ cod[L]=(cod[L]<<8)|dsrc[L][dpos[L]++]; rng[L]<<=8; }

int main(int argc,char**argv){
  const char* fn = argc>1 ? argv[1] : "../../enwik8";
  size_t n = (argc>2 ? atoi(argv[2]) : 8) * (size_t)1024*1024;
  n &= ~(size_t)(2*NB-1);
  FILE* f=fopen(fn,"rb"); if(!f){ printf("no %s\n",fn); return 1; }
  byte* d=(byte*)malloc(n); n=fread(d,1,n,f); fclose(f); n &= ~(size_t)(2*NB-1);
  for(int L=0;L<NL;L++) ebuf[L]=(byte*)malloc(n/NB+1024);
  byte* out=(byte*)malloc(n);
  printf("%s, %zu bytes, %d lanes (byte m bit j -> lane m*8+j)\n\n",fn,n,NL);

  // ---- stream 1: byte-at-a-time order (decoders A and B) ----
  model_init(); e_init();
  for(size_t i=0;i<n;i++){ uint m=i%NB, ctx=1, c=d[i];
    for(uint j=0;j<8;j++){ uint b=(c>>(7-j))&1; e_bit(m*8+j,ctx,b); ctx=ctx*2+b; } }
  e_flush();
  size_t tot1=0; for(int L=0;L<NL;L++) tot1+=epos[L];

  static byte* s1[NL]; for(int L=0;L<NL;L++){ s1[L]=(byte*)malloc(epos[L]+64);
    memcpy(s1[L],ebuf[L],epos[L]); memset(s1[L]+epos[L],0xFF,64); }

  // ---- stream 2: wavefront order (decoder W) ----
  model_init(); e_init();
  for(size_t i=0;i<n;i+=NB){ uint ctx[NB];
    for(uint m=0;m<NB;m++) ctx[m]=1;
    for(uint j=0;j<8;j++) for(uint m=0;m<NB;m++){
      uint b=(d[i+m]>>(7-j))&1; e_bit(m*8+j,ctx[m],b); ctx[m]=ctx[m]*2+b; } }
  e_flush();
  size_t tot2=0; for(int L=0;L<NL;L++) tot2+=epos[L];
  static byte* s2[NL]; for(int L=0;L<NL;L++){ s2[L]=(byte*)malloc(epos[L]+64);
    memcpy(s2[L],ebuf[L],epos[L]); memset(s2[L]+epos[L],0xFF,64); }
  printf("  coded: byte-order %zu bytes (%.4f bpc), wavefront %zu (%.4f bpc)\n\n",
         tot1,8.0*tot1/n,tot2,8.0*tot2/n);

  qword best[3]={~0ULL,~0ULL,~0ULL}; const char* nm[3]={"A loop, byte at a time",
                                                        "W loop, wavefront","B if-tree"};
  for(int rep=0;rep<5;rep++){
    for(int which=0;which<3;which++){
      byte** S = (which==1)? s2 : s1;
      for(int L=0;L<NL;L++) dsrc[L]=S[L];
      model_init(); d_init();
      qword t0=rdtsc();
      if(which==0){
        for(size_t i=0;i<n;i++){ uint m=i%NB, B=m*8, ctx=1;
          for(uint j=0;j<8;j++){ uint rp=(rng[B+j]>>SCALElog)*cty[ctx];
            uint b=(cod[B+j]>=rp);
            if(b){ rng[B+j]-=rp; cod[B+j]-=rp; } else rng[B+j]=rp;
            upd(ctx,b) ctx=ctx*2+b; RENORM(B+j) }
          out[i]=byte(ctx); }
      } else if(which==1){
        for(size_t i=0;i<n;i+=NB){ uint ctx[NB];
          for(uint m=0;m<NB;m++) ctx[m]=1;
          for(uint j=0;j<8;j++) for(uint m=0;m<NB;m++){ uint B=m*8;
            uint rp=(rng[B+j]>>SCALElog)*cty[ctx[m]];
            uint b=(cod[B+j]>=rp);
            if(b){ rng[B+j]-=rp; cod[B+j]-=rp; } else rng[B+j]=rp;
            upd(ctx[m],b) ctx[m]=ctx[m]*2+b; RENORM(B+j) }
          for(uint m=0;m<NB;m++) out[i+m]=byte(ctx[m]); }
      } else {
        for(size_t i=0;i<n;i++){ uint B=(i%NB)*8, rp, sym;
#include "tree_lanes.h"
          for(uint j=0;j<8;j++){ RENORM(B+j) }
          out[i]=byte(sym); }
      }
      qword t1=rdtsc();
      if(memcmp(out,d,n)!=0){ printf("  %s: MISMATCH\n",nm[which]); }
      if(t1-t0<best[which]) best[which]=t1-t0;
    }
  }
  printf("  %-24s %8.2f cycles/byte\n",nm[0],double(best[0])/n);
  printf("  %-24s %8.2f cycles/byte\n",nm[1],double(best[1])/n);
  printf("  %-24s %8.2f cycles/byte   %.2fx vs A, %.2fx vs W\n",nm[2],double(best[2])/n,
         double(best[0])/best[2], double(best[1])/best[2]);
  return 0;
}
