/* bench.c - three small matrix products in plain C (no intrinsics) for
   comparing -march builds: written so that gcc and clang auto-vectorize them.

     f  float matmul       C = A*B, 384x384, i-k-j loop order (FMA, vector width)
     i  int8 matmul        C = A*Bt, u8 x s8 -> int32, 256x256x1024 (dot product:
                           VNNI vpdpbusd where the target has it)
     b  bit-matrix matmul  C[i][j] = popcount(A[i] & B[j]), 768x768x2048 bits
                           (portable SWAR popcount; compilers recognize it)

   usage: bench [kernels]   e.g. "bench fib" (default), "bench i"
   Prints, per kernel, the best time of several runs and a checksum. The data
   are small integers, so every build must print the same checksums. */
#define _POSIX_C_SOURCE 199309L  /* clock_gettime (MinGW: link with -lpthread) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

static double Now( void ) {
  struct timespec t; clock_gettime( CLOCK_MONOTONIC, &t );
  return t.tv_sec + t.tv_nsec*1e-9;
}

static uint64_t seed = 88172645463325252ull;  /* xorshift64 */
static uint32_t Rnd( void ) { seed ^= seed<<13; seed ^= seed>>7; seed ^= seed<<17; return (uint32_t)(seed>>32); }

/* ---- f: float matmul */
enum { FN = 384 };
static _Alignas(64) float FA[FN*FN], FB[FN*FN], FC[FN*FN];

NOINLINE static void MatMulF( int n, const float* restrict a, const float* restrict b, float* restrict c ) {
  for( int i=0; i<n; i++ ) {
    float* restrict ci = c + (size_t)i*n;
    for( int j=0; j<n; j++ ) ci[j] = 0;
    for( int k=0; k<n; k++ ) {
      float aik = a[(size_t)i*n+k];
      const float* restrict bk = b + (size_t)k*n;
      for( int j=0; j<n; j++ ) ci[j] += aik*bk[j];
    }
  }
}

/* ---- i: int8 matmul, B stored transposed so the inner loop is a dot product */
enum { IM = 256, IK = 1024 };
static _Alignas(64) uint8_t IA[IM*IK];
static _Alignas(64) int8_t  IB[IM*IK];
static _Alignas(64) int32_t IC[IM*IM];

NOINLINE static void MatMulI8( int m, int kk, const uint8_t* restrict a, const int8_t* restrict bt, int32_t* restrict c ) {
  for( int i=0; i<m; i++ )
    for( int j=0; j<m; j++ ) {
      const uint8_t* ai = a + (size_t)i*kk;
      const int8_t* bj = bt + (size_t)j*kk;
      int32_t s = 0;
      for( int k=0; k<kk; k++ ) s += (int32_t)ai[k] * (int32_t)bj[k];
      c[(size_t)i*m+j] = s;
    }
}

/* ---- b: bit-matrix product */
enum { BN = 768, BW = 32 };  /* 32 x 64 = 2048 bits per row */
static _Alignas(64) uint64_t BA[BN*BW], BB[BN*BW];
static _Alignas(64) int32_t BC[BN*BN];

static inline int Popcount64( uint64_t x ) {
  x = x - ((x>>1) & 0x5555555555555555ull);
  x = (x & 0x3333333333333333ull) + ((x>>2) & 0x3333333333333333ull);
  x = (x + (x>>4)) & 0x0F0F0F0F0F0F0F0Full;
  return (int)((x*0x0101010101010101ull)>>56);
}

NOINLINE static void MatMulBits( int n, int w, const uint64_t* restrict a, const uint64_t* restrict b, int32_t* restrict c ) {
  for( int i=0; i<n; i++ )
    for( int j=0; j<n; j++ ) {
      const uint64_t* ai = a + (size_t)i*w;
      const uint64_t* bj = b + (size_t)j*w;
      int32_t s = 0;
      for( int k=0; k<w; k++ ) s += Popcount64( ai[k] & bj[k] );
      c[(size_t)i*n+j] = s;
    }
}

/* ---- driver */
static int runs = 7;

static void Report( const char* name, double best, double ops, const char* unit, unsigned long long sum ) {
  printf( "%-5s %9.3f ms %9.2f %s  sum=%016llx\n", name, best*1e3, ops/best*1e-9, unit, sum );
}

static void BenchF( void ) {
  for( int i=0; i<FN*FN; i++ ) { FA[i] = (float)((int)(Rnd()%17)-8); FB[i] = (float)((int)(Rnd()%17)-8); }
  double best = 1e9;
  for( int r=0; r<runs; r++ ) { double t = Now(); MatMulF( FN, FA, FB, FC ); t = Now()-t; if( t<best ) best = t; }
  unsigned long long s = 0;
  for( int i=0; i<FN*FN; i++ ) s = s*31 + (unsigned long long)(long long)FC[i];
  Report( "float", best, 2.0*FN*FN*FN, "GFLOP/s", s );
}

static void BenchI( void ) {
  for( int i=0; i<IM*IK; i++ ) { IA[i] = (uint8_t)Rnd(); IB[i] = (int8_t)Rnd(); }
  double best = 1e9;
  for( int r=0; r<runs; r++ ) { double t = Now(); MatMulI8( IM, IK, IA, IB, IC ); t = Now()-t; if( t<best ) best = t; }
  unsigned long long s = 0;
  for( int i=0; i<IM*IM; i++ ) s = s*31 + (unsigned long long)(uint32_t)IC[i];
  Report( "int8", best, 2.0*IM*IM*IK, "GOP/s", s );
}

static void BenchB( void ) {
  for( int i=0; i<BN*BW; i++ ) {
    BA[i] = (uint64_t)Rnd()<<40 ^ (uint64_t)Rnd()<<20 ^ Rnd();
    BB[i] = (uint64_t)Rnd()<<40 ^ (uint64_t)Rnd()<<20 ^ Rnd();
  }
  double best = 1e9;
  for( int r=0; r<runs; r++ ) { double t = Now(); MatMulBits( BN, BW, BA, BB, BC ); t = Now()-t; if( t<best ) best = t; }
  unsigned long long s = 0;
  for( int i=0; i<BN*BN; i++ ) s = s*31 + (unsigned long long)(uint32_t)BC[i];
  Report( "bits", best, (double)BN*BN*BW, "Gword/s", s );  /* 64-bit AND+popcount per second */
}

int main( int argc, char** argv ) {
  const char* k = argc>1 ? argv[1] : "fib";
  if( argc>2 ) runs = atoi( argv[2] );
  setvbuf( stdout, 0, _IONBF, 0 );  /* print each result before a later kernel can crash */
  for( ; *k; k++ )
    switch( *k ) {
      case 'f': BenchF(); break;
      case 'i': BenchI(); break;
      case 'b': BenchB(); break;
      default: fprintf( stderr, "usage: bench [fib] [runs]\n" ); return 1;
    }
  return 0;
}
