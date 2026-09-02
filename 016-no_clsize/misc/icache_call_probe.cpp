// Standalone probe -- NOT part of the build.  See misc/icache_lut.md.
//
// A lookup table as CODE: entry i is `mov eax, imm32; ret` and a lookup is an
// indirect call to entry i.  The value really does come out of the
// instruction cache.  How does that compare with a data load, and can it be
// faster when the stubs are in L1i?
//
//   linux:    clang++ -O2 -o icache_call_probe icache_call_probe.cpp
//   windows:  cl /O2 icache_call_probe.cpp
//   ./icache_call_probe [repeats=5]
//
// Every lookup is chained -- the value fetched is the next index -- so the
// number is latency.  Two orders: a random single cycle over the N entries,
// where the call target is unpredictable, and sequential (i -> i+1), where it
// is not.  Two strides: 8 bytes (N=2048 is 16 KB, inside L1i) and 64 bytes
// (one entry a line, 128 KB, four L1i's).  The data chase is the same chain
// through a plain array at the same strides, so at stride 64 and N=2048 it
// is the L2 number and at stride 8 the L1d number.  Everything is warmed once
// before timing; this is the steady-state cost of a lookup, not a cold one.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#else
#include <sys/mman.h>
#include <x86intrin.h>
#endif
typedef uint8_t byte; typedef uint32_t uint; typedef uint64_t qword;
enum { MAXN=2048, MAXSTRIDE=64, CODESZ=MAXN*MAXSTRIDE, CHAIN=1<<16 };

static byte* alloc_exec( size_t n ) {
#ifdef _WIN32
  return (byte*)VirtualAlloc( 0, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE );
#else
  void* p = mmap( 0, n, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  return p==MAP_FAILED ? 0 : (byte*)p;
#endif
}
static void set_rw( byte* p, size_t n ) {
#ifdef _WIN32
  DWORD o; VirtualProtect( p, n, PAGE_READWRITE, &o );
#else
  mprotect( p, n, PROT_READ|PROT_WRITE );
#endif
}
static void set_rx( byte* p, size_t n ) {
#ifdef _WIN32
  DWORD o; VirtualProtect( p, n, PAGE_EXECUTE_READ, &o ); FlushInstructionCache( GetCurrentProcess(), p, n );
#else
  mprotect( p, n, PROT_READ|PROT_EXEC );
#endif
}
static inline qword tsc_begin( void ) { _mm_lfence(); return __rdtsc(); }
static inline qword tsc_end( void ) { unsigned a; qword t = __rdtscp(&a); _mm_lfence(); return t; }
static volatile qword sink;

static uint rng_s = 0x9E3779B9u;
static uint rng( void ) { rng_s ^= rng_s<<13; rng_s ^= rng_s>>17; rng_s ^= rng_s<<5; return rng_s; }
// next[i] for a random single cycle (Sattolo) or the sequential one
static void make_order( uint* nxt, uint n, int sequential ) {
  static uint perm[MAXN];
  for( uint i=0; i<n; i++ ) perm[i]=i;
  if( !sequential ) for( uint i=n-1; i>0; i-- ) { uint j=rng()%i; uint t=perm[i]; perm[i]=perm[j]; perm[j]=t; }
  for( uint i=0; i<n; i++ ) nxt[perm[i]] = perm[(i+1)%n];
}
// the table as code: B8 imm32 C3, padded with int3
static void build_code( byte* code, const uint* nxt, uint n, uint stride ) {
  memset( code, 0xCC, (size_t)n*stride );
  for( uint i=0; i<n; i++ ) { byte* p = code + (size_t)i*stride; p[0]=0xB8; memcpy(p+1,&nxt[i],4); p[5]=0xC3; }
}
typedef uint (*t_stub)( void );
static double chase_code( const byte* code, uint stride, uint chain ) {
  uint idx=0; qword t0=tsc_begin();
  for( uint i=0; i<chain; i++ ) { t_stub f = (t_stub)(uintptr_t)(code + (size_t)idx*stride); idx = f(); }
  qword t1=tsc_end(); sink+=idx; return double(t1-t0)/chain;
}
// Non-repeating order: the fetched value is mixed with an LCG before it is
// used as the next index, so the target sequence never repeats and nothing can
// learn it -- which is the FSM's situation: the state is data-driven. The mix
// is identical on the code and data sides, so it costs both the same.
static double chase_code_scr( const byte* code, uint stride, uint n, uint chain ) {
  uint idx=0, r=12345; qword t0=tsc_begin();
  for( uint i=0; i<chain; i++ ) { t_stub f = (t_stub)(uintptr_t)(code + (size_t)idx*stride);
    r = r*1664525u+1013904223u; idx = (f() ^ (r>>20)) & (n-1); }
  qword t1=tsc_end(); sink+=idx; return double(t1-t0)/chain;
}
static double chase_data_scr( const byte* data, uint stride, uint n, uint chain ) {
  uint idx=0, r=12345; qword t0=tsc_begin();
  for( uint i=0; i<chain; i++ ) { uint v = *(const volatile uint*)(data + (size_t)idx*stride);
    r = r*1664525u+1013904223u; idx = (v ^ (r>>20)) & (n-1); }
  qword t1=tsc_end(); sink+=idx; return double(t1-t0)/chain;
}
static double chase_data( const byte* data, uint stride, uint chain ) {
  uint idx=0; qword t0=tsc_begin();
  for( uint i=0; i<chain; i++ ) idx = *(const volatile uint*)(data + (size_t)idx*stride);
  qword t1=tsc_end(); sink+=idx; return double(t1-t0)/chain;
}

int main( int argc, char** argv ) {
  int reps = argc>1 ? atoi(argv[1]) : 5;
  byte* code = alloc_exec( CODESZ ); byte* data = (byte*)malloc( CODESZ );
  if( !code || !data ) { printf("alloc failed\n"); return 1; }
  static uint nxt[MAXN];
  { auto w0=std::chrono::steady_clock::now(); qword t0=__rdtsc();
    while( std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count()<0.2 ){}
    qword t1=__rdtsc(); double s=std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count();
    printf("TSC %.2f GHz; %d lookups a measurement, min of %d; ticks per lookup\n\n",(t1-t0)/s/1e9,CHAIN,reps); }

  printf("  ticks per lookup.  cycle = a fixed order the predictor can learn; scrambled = never repeats.\n\n");
  printf("                    ------- table as CODE (call mov eax,imm; ret) -------   ---- table as DATA (load) ----\n");
  printf("     N  footprint   cycle/8B  seq/8B  cycle/64B  seq/64B  scr/8B  scr/64B   cycle/8B cycle/64B scr/8B scr/64B\n");
  const uint sizes[] = { 16, 64, 256, 1024, 2048 };
  for( uint si=0; si<5; si++ ) {
    uint n = sizes[si];
    double best[12]; for( int k=0;k<12;k++ ) best[k]=1e9;
    for( int rep=0; rep<reps; rep++ ) {
      for( int seq=0; seq<2; seq++ ) for( uint stride=8; stride<=64; stride*=8 ) {
        make_order( nxt, n, seq );
        set_rw( code, CODESZ ); build_code( code, nxt, n, stride ); set_rx( code, CODESZ );
        chase_code( code, stride, CHAIN );                       // warm: everything resident
        double v = chase_code( code, stride, CHAIN );
        int slot = seq + (stride==64?2:0);                       // 0 c8, 1 s8, 2 c64, 3 s64
        if( v<best[slot] ) best[slot]=v;
        if( !seq ) { chase_code_scr( code, stride, n, CHAIN );
          v = chase_code_scr( code, stride, n, CHAIN );
          slot = 4 + (stride==64?1:0);                           // 4 scr8, 5 scr64
          if( v<best[slot] ) best[slot]=v; }
      }
      for( uint stride=8; stride<=64; stride*=8 ) {
        make_order( nxt, n, 0 );
        for( uint i=0; i<n; i++ ) *(uint*)(data + (size_t)i*stride) = nxt[i];
        chase_data( data, stride, CHAIN );
        double v = chase_data( data, stride, CHAIN );
        int slot = 6 + (stride==64?1:0);                         // 6 d-c8, 7 d-c64
        if( v<best[slot] ) best[slot]=v;
        chase_data_scr( data, stride, n, CHAIN );
        v = chase_data_scr( data, stride, n, CHAIN );
        slot = 8 + (stride==64?1:0);                             // 8 d-scr8, 9 d-scr64
        if( v<best[slot] ) best[slot]=v;
      }
    }
    printf("  %4u %4uKB %4uKB  %7.1f %7.1f  %8.1f  %7.1f %7.1f  %7.1f   %7.1f  %7.1f  %6.1f  %6.1f\n",
           n, n*8/1024, n*64/1024, best[0], best[1], best[2], best[3], best[4], best[5],
           best[6], best[7], best[8], best[9]);
  }
  printf("\n  8B: N=2048 is 16 KB, inside L1i / L1d.  64B: one entry a line, 128 KB at N=2048 -- outside\n"
         "  L1i, so the data column there is the L2 number.  The scrambled columns are the FSM's case.\n");
  return int(sink&1);
}
