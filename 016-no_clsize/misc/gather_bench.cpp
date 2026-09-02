// What a gather costs on this box, in isolation: ticks per 8-lane dword
// vpgatherdd from L1-hot data, against eight scalar loads building the same
// vector -- independent iterations (throughput) and a dependent chain (the
// gathered lane feeds the next indices: latency).
//
//   clang++ -O2 -march=native gather_bench.cpp -o gather_bench && ./gather_bench
//
// Cascade Lake VM, clang 18, 2.8 GHz TSC (misc/dec_renorm.md's box):
//
//   gather: 25.1 ticks/iter throughput, 37.1 dependent | 8 scalar loads: 1.1 throughput, 19.0 dependent
//
// 25 ticks of THROUGHPUT for one ymm gather is the Downfall (GDS) microcode
// mitigation, which serialises every gather on Skylake-generation cores; an
// unmitigated Skylake-X does one every ~5 cycles. The guest kernel reports
// "gather_data_sampling: Not affected", and the timing says otherwise. It is
// why RC_DEC_VRENORM does not gather (rc_config.inc): two of these a group
// cost 59 cycles on a 139-cycle decode group. (The 1.1-tick scalar figure is
// the compiler seeing through the +1 index stride and loading contiguously;
// the dependent one is the honest number.)
// cycles per 8-lane dword gather from L1-hot data, against 8 scalar loads
// building the same vector. Independent iterations (throughput), and a
// dependent chain (latency: the gathered lane 0 feeds the next indices).
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
static uint32_t tab[1024];
__attribute__((noinline)) uint64_t run_gather( int n, __m256i idx ) {
  __m256i acc = _mm256_setzero_si256();
  uint64_t t0 = __rdtsc();
  for( int i=0; i<n; i++ ) {
    __m256i v = _mm256_i32gather_epi32( (const int*)tab, idx, 4 );
    acc = _mm256_add_epi32( acc, v );
    idx = _mm256_add_epi32( idx, _mm256_set1_epi32(1) );
    idx = _mm256_and_si256( idx, _mm256_set1_epi32(1023) );
  }
  uint64_t t1 = __rdtsc();
  volatile int sink = _mm256_extract_epi32( acc, 0 ); (void)sink;
  return t1-t0;
}
__attribute__((noinline)) uint64_t run_gather_dep( int n, __m256i idx ) {
  uint64_t t0 = __rdtsc();
  for( int i=0; i<n; i++ ) {
    __m256i v = _mm256_i32gather_epi32( (const int*)tab, idx, 4 );
    idx = _mm256_and_si256( _mm256_add_epi32( idx, v ), _mm256_set1_epi32(1023) );
  }
  uint64_t t1 = __rdtsc();
  volatile int sink = _mm256_extract_epi32( idx, 0 ); (void)sink;
  return t1-t0;
}
__attribute__((noinline)) uint64_t run_scalar( int n, __m256i idx ) {
  __m256i acc = _mm256_setzero_si256();
  uint64_t t0 = __rdtsc();
  for( int i=0; i<n; i++ ) {
    alignas(32) uint32_t ix[8]; _mm256_store_si256( (__m256i*)ix, idx );
    __m256i v = _mm256_setr_epi32( tab[ix[0]],tab[ix[1]],tab[ix[2]],tab[ix[3]],tab[ix[4]],tab[ix[5]],tab[ix[6]],tab[ix[7]] );
    acc = _mm256_add_epi32( acc, v );
    idx = _mm256_add_epi32( idx, _mm256_set1_epi32(1) );
    idx = _mm256_and_si256( idx, _mm256_set1_epi32(1023) );
  }
  uint64_t t1 = __rdtsc();
  volatile int sink = _mm256_extract_epi32( acc, 0 ); (void)sink;
  return t1-t0;
}
__attribute__((noinline)) uint64_t run_scalar_dep( int n, __m256i idx ) {
  uint64_t t0 = __rdtsc();
  for( int i=0; i<n; i++ ) {
    alignas(32) uint32_t ix[8]; _mm256_store_si256( (__m256i*)ix, idx );
    __m256i v = _mm256_setr_epi32( tab[ix[0]],tab[ix[1]],tab[ix[2]],tab[ix[3]],tab[ix[4]],tab[ix[5]],tab[ix[6]],tab[ix[7]] );
    idx = _mm256_and_si256( _mm256_add_epi32( idx, v ), _mm256_set1_epi32(1023) );
  }
  uint64_t t1 = __rdtsc();
  volatile int sink = _mm256_extract_epi32( idx, 0 ); (void)sink;
  return t1-t0;
}
int main() {
  for( int i=0; i<1024; i++ ) tab[i] = (i*7919u)&1023u;
  __m256i idx = _mm256_setr_epi32( 3,100,211,377,500,640,777,900 );
  const int n = 1<<22;
  for( int rep=0; rep<3; rep++ ) {
    printf( "gather: %.1f ticks/iter throughput, %.1f dependent | 8 scalar loads: %.1f throughput, %.1f dependent\n",
      double(run_gather(n,idx))/n, double(run_gather_dep(n,idx))/n,
      double(run_scalar(n,idx))/n, double(run_scalar_dep(n,idx))/n );
  }
  return 0;
}
