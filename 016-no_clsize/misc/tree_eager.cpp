// Standalone probe -- NOT part of the build. Build with:
//   clang++ -O2 -march=native -o tree_eager tree_eager.cpp
// See misc/dec_vectorize.md section 7.
// Speculate only the COUNTER LOADS, D levels ahead; keep the state serial.
// From node c the next D levels' counters are D contiguous runs -- cty[c],
// cty[2c..2c+1], cty[4c..4c+3] ... -- every address known at c, so none of the
// loads is on the dependency chain. No extra coder arithmetic at all: the
// state update stays 8 per byte. D=1 is RC_EAGER_CTY.
//
// What replaces the load on the chain is SELECTING the right counter from the
// ones already in hand, so the question is whether a select is cheaper than an
// L1 hit, and by how much as D grows.
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
typedef uint32_t uint;
enum { SCALElog=15, sTOP=1u<<24, gTOP=1u<<16 };
static inline uint64_t rdtsc(){ return __rdtsc(); }
static uint cty[512]; static uint feed[1<<16];

#define STEP(p) { uint rpre=(range>>SCALElog)*(p); uint b=(code>=rpre);      \
    range = b?range-rpre:rpre; code -= b?rpre:0;                             \
    uint d=(range<sTOP)?(range<gTOP)?16:8:0;                                 \
    range<<=d; code=(code<<d)|(feed[(fp+k)&0xFFFF]>>(31-d));                 \
    ctx=ctx*2+b; k++; }

// D=0: plain, one dependent load per bit
static uint dec0( uint code, uint range, uint fp ){
  uint ctx=1,k=0;
  for(int i=0;i<8;i++){ uint p=cty[ctx]; STEP(p) }
  return ctx&0xFF;
}
// D=1: both children as one 8-byte load, a bit early -- RC_EAGER_CTY
static uint dec1( uint code, uint range, uint fp ){
  uint ctx=1,k=0;
  uint64_t kids=*(const uint64_t*)&cty[2*1];
  uint p=cty[1];
  for(int i=0;i<8;i++){
    STEP(p)
    uint b=ctx&1;
    p = b ? uint(kids>>32) : uint(kids);
    if(i<7) kids=*(const uint64_t*)&cty[2*ctx];
  }
  return ctx&0xFF;
}
// D=2: four grandchildren as one 16-byte load, two levels early
static uint dec2( uint code, uint range, uint fp ){
  uint ctx=1,k=0;
  uint p=cty[1];
  __m128i quad=_mm_loadu_si128((const __m128i*)&cty[4*1]);
  uint64_t kids=*(const uint64_t*)&cty[2*1];
  for(int i=0;i<8;i++){
    STEP(p)
    uint b=ctx&1;
    p = b ? uint(kids>>32) : uint(kids);
    if(i<7){
      alignas(16) uint q[4]; _mm_store_si128((__m128i*)q,quad);
      kids = (uint64_t(q[2*b+1])<<32) | q[2*b];
      if(i<6) quad=_mm_loadu_si128((const __m128i*)&cty[4*ctx]);
    }
  }
  return ctx&0xFF;
}
int main(){
  uint64_t s=12345678901234567ull;
  for(int i=0;i<512;i++){ s^=s<<13;s^=s>>7;s^=s<<17; cty[i]=1+uint(s%32766); }
  for(uint i=0;i<(1<<16);i++){ s^=s<<13;s^=s>>7;s^=s<<17; feed[i]=uint(s); }
  const long R=3000000;
  #define RUN(f,name) { uint acc=0; uint64_t t0=rdtsc(); \
      for(long r=0;r<R;r++) acc+=f(0x12345678,0xFFFFFFFFu,r); \
      uint64_t t1=rdtsc(); double c=double(t1-t0)/R; \
      printf("%-34s %7.2f cycles/byte  %5.2f /bit  (acc %u)\n",name,c,c/8,acc); }
  RUN(dec0,"D=0  load per bit, on the chain")
  RUN(dec1,"D=1  both children, one load  ")
  RUN(dec2,"D=2  four grandchildren       ")
  return 0;
}
