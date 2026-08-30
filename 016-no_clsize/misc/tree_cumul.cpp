// Standalone probe -- NOT part of the build. Build with:
//   clang++ -O2 -march=native -o tree_cumul tree_cumul.cpp
// See misc/dec_vectorize.md section 7.
// Can the 255 node comparisons be had WITHOUT a per-node update+renorm?
//
// If yes, the whole tree is one cumulative-frequency lookup: multiply the p's
// down each root-to-leaf path, prefix-sum to 256 thresholds, and find where
// code falls. No per-node state, no per-node renorm -- exactly what makes the
// idea cheap.
//
// The obstacle is that the real coder computes (range>>SCALElog)*p against a
// RENORMALISED range at every level, and that truncation is path-dependent.
// This measures how often the two disagree.
#include <stdio.h>
#include <stdint.h>
typedef uint32_t uint; typedef uint64_t qword;
enum { SCALElog=15, SCALE=1<<SCALElog, sTOP=1u<<24, gTOP=1u<<16 };
static uint cty[512]; static uint8_t strm[1<<20];

// ground truth: the serial walk the decoder actually performs
static uint walk( uint code, uint range, uint& pos, uint* bits ) {
  uint ctx=1;
  for(int k=0;k<8;k++){
    uint p=cty[ctx];
    uint rpre=(range>>SCALElog)*p;
    uint bit=(code>=rpre);
    bits[k]=bit;
    if(bit){ range-=rpre; code-=rpre; } else range=rpre;
    while(range<sTOP){ code=(code<<8)|strm[pos++]; range<<=8; }
    ctx=ctx*2+bit;
  }
  return ctx&0xFF;
}
// the cheap version: leaf intervals from products of p, no per-level truncation
static uint cumul( uint code, uint range, uint* bits ) {
  // width of each leaf as a fraction of range, in 2^-15 units per level
  double w[256], acc=0;
  for(int sym=0;sym<256;sym++){
    double f=1.0; uint c=1;
    for(int k=0;k<8;k++){
      uint bit=(sym>>(7-k))&1;
      double p=double(cty[c])/SCALE;
      f *= bit ? (1.0-p) : p;
      c=c*2+bit;
    }
    w[sym]=f;
  }
  double x=double(code)/double(range);
  for(int sym=0;sym<256;sym++){
    if( x < acc+w[sym] ){ for(int k=0;k<8;k++) bits[k]=(sym>>(7-k))&1; return sym; }
    acc+=w[sym];
  }
  return 255;
}
int main(){
  uint64_t s=999331ull;
  for(uint i=0;i<(1<<20);i++){ s^=s<<13;s^=s>>7;s^=s<<17; strm[i]=uint8_t(s); }
  long trials=0, symdiff=0, bitdiff=0, firstdiff[9]={0};
  for(int rep=0; rep<20000; rep++){
    for(int i=0;i<512;i++){ s^=s<<13;s^=s>>7;s^=s<<17; cty[i]=1+uint(s%(SCALE-1)); }
    s^=s<<13;s^=s>>7;s^=s<<17; uint range=sTOP+uint(s%(0xFFFFFFFFu-sTOP));
    s^=s<<13;s^=s>>7;s^=s<<17; uint code=uint(s%range);
    uint pos=0, b1[8], b2[8];
    uint sym1=walk(code,range,pos,b1);
    uint sym2=cumul(code,range,b2);
    trials++;
    if(sym1!=sym2) symdiff++;
    int fd=8;
    for(int k=0;k<8;k++) if(b1[k]!=b2[k]){ bitdiff++; if(fd==8) fd=k; }
    firstdiff[fd]++;
  }
  printf("bytes tried            : %ld\n",trials);
  printf("symbol mismatches      : %ld  (%.2f%%)\n",symdiff,100.0*symdiff/trials);
  printf("bit mismatches         : %ld  (%.2f%% of bits)\n",bitdiff,100.0*bitdiff/(trials*8));
  printf("first divergence at bit: ");
  for(int k=0;k<8;k++) printf("%d:%.1f%% ",k,100.0*firstdiff[k]/trials);
  printf("| never:%.1f%%\n",100.0*firstdiff[8]/trials);
  return 0;
}
