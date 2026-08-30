// Standalone probe -- NOT part of the build (build.sh names its sources
// explicitly). Build with:  clang++ -O2 -march=native -o tree_spec tree_spec.cpp
//
// Answers: is it worth speculating the context tree so the counter loads
// come off the per-bit dependency chain? See misc/dec_vectorize.md section 7.
// Speculate D bits at a time: evaluate the 2^D-1 node subtree, then walk it.
// D=1 is what the decoder does now (one node, one load on the chain); D=4 is
// the nibble; D=8 is the whole symbol.
//
//   nodes per byte = (8/D)*(2^D - 1)      loads on the chain = 8/D
//     D=1   8 nodes,  8 loads        D=4   30 nodes, 2 loads
//     D=2  12 nodes,  4 loads        D=8  255 nodes, 1 load
//
// The counters for a subtree rooted at c are NOT contiguous for D>1 -- level
// j holds 2^j of them at 2^j*c .. -- so a depth-D root needs D contiguous
// runs, which is still D loads with addresses known at the root. That is the
// point: they do not depend on each other.
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
typedef uint32_t uint;
enum { SCALElog=15, sTOP=1u<<24, gTOP=1u<<16 };
static inline uint64_t rdtsc(){ return __rdtsc(); }
static uint cty[512]; static uint feed[1<<16];

template<int D>
static uint dec( uint code, uint range, uint fp ) {
  uint ctx=1;
  for( int g=0; g<8/D; g++ ){
    // --- speculate the depth-D subtree rooted at ctx ---
    uint sc[1<<D], sr[1<<D], bit[1<<D];
    sc[0]=code; sr[0]=range;
    uint base=ctx;
    for( int j=0;j<D;j++ ){
      const int n=1<<j;
      for( int i=n-1;i>=0;i-- ){          // in place, high to low
        uint r=sr[i], c=sc[i];
        uint p=cty[(base<<j)+i];
        uint rpre=(r>>SCALElog)*p;
        uint b=(c>=rpre); bit[(1<<j)-1+i]=b;
        uint r0=rpre,c0=c, r1=r-rpre,c1=c-rpre;
        uint d0=(r0<sTOP)?(r0<gTOP)?16:8:0;
        uint d1=(r1<sTOP)?(r1<gTOP)?16:8:0;
        uint f=feed[(fp+j)&0xFFFF];
        sr[2*i]  =r0<<d0; sc[2*i]  =(c0<<d0)|(f>>(31-d0));
        sr[2*i+1]=r1<<d1; sc[2*i+1]=(c1<<d1)|(f>>(31-d1));
      }
    }
    // --- walk it ---
    uint idx=0;
    for( int j=0;j<D;j++ ){ uint b=bit[(1<<j)-1+idx]; idx=2*idx+b; ctx=ctx*2+b; }
    code=sc[idx]; range=sr[idx];
  }
  return ctx&0xFF;
}
int main(){
  uint64_t s=12345678901234567ull;
  for(int i=0;i<512;i++){ s^=s<<13;s^=s>>7;s^=s<<17; cty[i]=1+uint(s%32766); }
  for(uint i=0;i<(1<<16);i++){ s^=s<<13;s^=s>>7;s^=s<<17; feed[i]=uint(s); }
  const long R=2000000;
  printf("  D   nodes/byte   loads/byte    cycles/byte   /bit\n");
  #define RUN(D) { uint acc=0; uint64_t t0=rdtsc(); \
      for(long r=0;r<R;r++) acc+=dec<D>(0x12345678,0xFFFFFFFFu,r); \
      uint64_t t1=rdtsc(); double c=double(t1-t0)/R; \
      printf("%3d %10d %11d %14.2f %6.2f   (acc %u)\n", \
             D,(8/D)*((1<<D)-1),8/D,c,c/8,acc); }
  RUN(1) RUN(2) RUN(4) RUN(8)
  return 0;
}
