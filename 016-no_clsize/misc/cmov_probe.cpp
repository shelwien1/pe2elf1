// Standalone probe -- NOT part of the build.  See misc/cmov_probe.md.
//
// Two questions about CMOV with a memory source:
//   1. Does it read memory when the condition is false?  (If so the line
//      gets cached, which a timed read afterwards can see.)
//   2. Does its result depend on the source it did not select?  (If so,
//      `sel(p, 0, false) / x` waits for p's line to arrive before dividing.)
//
//   linux:    clang++ -O2 -o cmov_probe cmov_probe.cpp     (or g++)
//   windows:  cl /O2 cmov_probe.cpp
//   ./cmov_probe [repeats=7]
//
// Four forms of  int f(const int* p, int q, int cond):
//   sel        the C++ below, compiled -- whatever the compiler makes of it
//   cmov_mem   hand-assembled  test cond; mov eax,q; cmovne eax,[p]; ret
//   cmov_reg   hand-assembled  mov ecx,[p]; test cond; mov eax,q; cmovne eax,ecx; ret
//   branch     hand-assembled  test cond; jne L; mov eax,q; ret; L: mov eax,[p]; ret
//   sel_addr   the C++ below: select the ADDRESS, then one load
//   cmov_addr  hand-assembled  local=q; lea rax,&local; test; cmovne rax,p; mov eax,[rax]
// The branch form is the control: with cond false it never touches p.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#define NOINLINE __declspec(noinline)
#else
#include <sys/mman.h>
#include <x86intrin.h>
#define NOINLINE __attribute__((noinline))
#endif
#ifndef __clang__
#define __builtin_unpredictable(x) (x)     // clang-only builtin; gcc and MSVC see the plain ternary
#endif
typedef uint8_t byte; typedef uint32_t uint; typedef uint64_t qword;
typedef int (*t_sel)( const int* p, int q, int cond );

NOINLINE int sel( const int* __restrict p, int q, int cond ) {
  int t = *p;                                   // load must be unconditional
  return __builtin_unpredictable(cond) ? t : q;
}
// Select the ADDRESS, then load once: with cond false the load hits a local
// on the stack and never touches p.  Branchless if the compiler keeps the
// select on the pointer -- it may not fold it into a load of each side,
// because *p is not known to be safe to speculate.
NOINLINE int sel_addr( const int* __restrict p, int q, int cond ) {
  int local_var = q;
  return *( __builtin_unpredictable(cond) ? p : &local_var );
}

static byte* alloc_exec( size_t n ) {
#ifdef _WIN32
  return (byte*)VirtualAlloc( 0, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE );
#else
  void* p = mmap( 0, n, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  return p==MAP_FAILED ? 0 : (byte*)p;
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

// The stubs, per ABI.  SysV: p=rdi q=esi cond=edx.  Win64: p=rcx q=edx cond=r8d.
#ifdef _WIN32
static const byte S_cmov_mem[] = { 0x45,0x85,0xC0, 0x89,0xD0, 0x0F,0x45,0x01, 0xC3 };
static const byte S_cmov_reg[] = { 0x44,0x8B,0x09, 0x45,0x85,0xC0, 0x89,0xD0, 0x41,0x0F,0x45,0xC1, 0xC3 };
static const byte S_branch[]   = { 0x45,0x85,0xC0, 0x75,0x03, 0x89,0xD0, 0xC3, 0x8B,0x01, 0xC3 };
// mov [rsp+8],edx; lea rax,[rsp+8]; test r8d,r8d; cmovne rax,rcx; mov eax,[rax]; ret
static const byte S_cmov_addr[]= { 0x89,0x54,0x24,0x08, 0x48,0x8D,0x44,0x24,0x08, 0x45,0x85,0xC0,
                                   0x48,0x0F,0x45,0xC1, 0x8B,0x00, 0xC3 };
#else
static const byte S_cmov_mem[] = { 0x85,0xD2, 0x89,0xF0, 0x0F,0x45,0x07, 0xC3 };
static const byte S_cmov_reg[] = { 0x8B,0x0F, 0x85,0xD2, 0x89,0xF0, 0x0F,0x45,0xC1, 0xC3 };
static const byte S_branch[]   = { 0x85,0xD2, 0x75,0x03, 0x89,0xF0, 0xC3, 0x8B,0x07, 0xC3 };
// mov [rsp-8],esi; lea rax,[rsp-8]; test edx,edx; cmovne rax,rdi; mov eax,[rax]; ret
static const byte S_cmov_addr[]= { 0x89,0x74,0x24,0xF8, 0x48,0x8D,0x44,0x24,0xF8, 0x85,0xD2,
                                   0x48,0x0F,0x45,0xC7, 0x8B,0x00, 0xC3 };
#endif

enum { NL=1024, STRIDE=4096 };      // lines 4 KB apart: no streamer/spatial prefetch across them
static byte* region; static uint order[NL];
static uint rng_s = 0x9E3779B9u;
static uint rng( void ) { rng_s ^= rng_s<<13; rng_s ^= rng_s>>17; rng_s ^= rng_s<<5; return rng_s; }
static const int* line( uint i ) { return (const int*)(region + (size_t)order[i]*STRIDE); }
static void flush_all( void ) { for( uint i=0; i<NL; i++ ) _mm_clflush( line(i) ); _mm_mfence(); }
static void warm_all( void ) { qword s=0; for( uint i=0; i<NL; i++ ) s += *(volatile const int*)line(i); sink+=s; }

// Q1: after f(line, 7, cond) on a flushed line, how long does a read of it take?
static double q1( t_sel f, int cond, int reps ) {
  double best = 1e9;
  for( int r=0; r<reps; r++ ) {
    double tot = 0;
    for( uint i=0; i<NL; i++ ) {
      const int* p = line(i);
      _mm_clflush( p ); _mm_mfence();
      if( f ) sink += f( p, 7, cond );
      qword t0 = tsc_begin(); int v = *(volatile const int*)p; qword t1 = tsc_end();
      sink += v; tot += double(t1-t0);
    }
    if( tot/NL < best ) best = tot/NL;
  }
  return best;
}
// Q2a: the DEPENDENT chain.  Each line holds the index of the next line, and
// so does next[]; the next address comes out of f's RESULT:
//     idx = f( line(idx), next[idx], cond )
// With cond false the result is q = next[idx], which is known before the call.
// If CMOV's output does not depend on the load it did not select, the chain
// runs at call+cmov speed and the cold loads complete in the background; if it
// does, every step waits for its line to arrive.  cond true is the reference:
// the result IS the line's contents.
static uint nxt[NL];
static double q2_chain( t_sel f, int cond, int cold, int reps ) {
  double best = 1e9;
  for( int r=0; r<reps; r++ ) {
    if( cold ) flush_all(); else warm_all();
    uint idx = 0;
    qword t0 = tsc_begin();
    for( uint i=0; i<NL; i++ ) idx = (uint)f( line(idx), (int)nxt[idx], cond );
    qword t1 = tsc_end(); sink += idx;
    if( double(t1-t0)/NL < best ) best = double(t1-t0)/NL;
  }
  return best;
}
// Q2b: the INDEPENDENT stream -- the literal  x = x / (f(line,0,cond)+1)  over
// lines whose addresses do not depend on x.  Kept because it is the natural
// way to write the test and it shows nothing: the addresses are known up
// front, so the out-of-order engine issues the loads for many iterations
// ahead and memory-level parallelism hides their latency behind the divides.
static double q2_stream( t_sel f, int cond, int cold, int reps ) {
  double best = 1e9;
  for( int r=0; r<reps; r++ ) {
    if( cold ) flush_all(); else warm_all();
    uint x = 0xFFFFFFFFu;
    qword t0 = tsc_begin();
    for( uint i=0; i<NL; i++ ) x = x / (uint)(f( line(i), 0, cond ) + 1);
    qword t1 = tsc_end(); sink += x;
    if( double(t1-t0)/NL < best ) best = double(t1-t0)/NL;
  }
  return best;
}

int main( int argc, char** argv ) {
  int reps = argc>1 ? atoi(argv[1]) : 7;
  region = (byte*)malloc( (size_t)NL*STRIDE + 64 ); memset( region, 0, (size_t)NL*STRIDE + 64 );   // *p == 0 everywhere
  for( uint i=0; i<NL; i++ ) order[i]=i;
  for( uint i=NL-1; i>0; i-- ) { uint j=rng()%(i+1); uint t=order[i]; order[i]=order[j]; order[j]=t; }
  byte* code = alloc_exec( 4096 ); if( !code ) { printf("alloc failed\n"); return 1; }
  memset( code, 0xCC, 4096 );
  memcpy( code+0,    S_cmov_mem, sizeof S_cmov_mem );
  memcpy( code+256,  S_cmov_reg, sizeof S_cmov_reg );
  memcpy( code+512,  S_branch,   sizeof S_branch );
  memcpy( code+768,  S_cmov_addr,sizeof S_cmov_addr );
  set_rx( code, 4096 );
  enum { NF=6 };
  t_sel F[NF] = { sel, (t_sel)(uintptr_t)(code+0), (t_sel)(uintptr_t)(code+256), (t_sel)(uintptr_t)(code+512),
                  sel_addr, (t_sel)(uintptr_t)(code+768) };
  const char* nm[NF] = { "sel (compiled)", "cmov_mem", "cmov_reg", "branch", "sel_addr (compiled)", "cmov_addr" };
  // sanity: every form selects correctly
  { int v = 42; for( int k=0; k<NF; k++ ) if( F[k](&v,7,0)!=7 || F[k](&v,7,1)!=42 ) { printf("form %s is wrong\n",nm[k]); return 1; } }
  { auto w0=std::chrono::steady_clock::now(); qword t0=__rdtsc();
    while( std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count()<0.2 ){}
    qword t1=__rdtsc(); double s=std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count();
    printf("TSC %.2f GHz; %d lines 4 KB apart, random order; min of %d; ticks\n\n",(t1-t0)/s/1e9,NL,reps); }

  printf("Q1  read latency of a flushed line AFTER calling f(line, 7, cond) on it\n");
  printf("      %-20s %8.1f   (no call: the cold read)\n", "none", q1( 0, 0, reps ));
  for( int k=0; k<NF; k++ )
    printf("      %-20s %8.1f   cond=false      %8.1f   cond=true\n", nm[k], q1(F[k],0,reps), q1(F[k],1,reps));

  // a random single cycle over the lines, stored both in next[] and IN the lines
  { static uint perm[NL]; for( uint i=0;i<NL;i++ ) perm[i]=i;
    for( uint i=NL-1;i>0;i-- ){ uint j=rng()%i; uint t=perm[i]; perm[i]=perm[j]; perm[j]=t; }
    for( uint i=0;i<NL;i++ ){ nxt[perm[i]]=perm[(i+1)%NL]; *(uint*)line(perm[i]) = perm[(i+1)%NL]; } }

  printf("\nQ2a ticks per step of  idx = f(line(idx), next[idx], cond)  -- the next ADDRESS comes\n");
  printf("    out of f's result, so the chain waits on exactly what the result depends on\n");
  printf("      %-20s   cold,cond=false   warm,cond=false   cold,cond=true   warm,cond=true\n", "");
  for( int k=0; k<NF; k++ )
    printf("      %-20s %10.1f %16.1f %16.1f %16.1f\n", nm[k],
           q2_chain(F[k],0,1,reps), q2_chain(F[k],0,0,reps), q2_chain(F[k],1,1,reps), q2_chain(F[k],1,0,reps));
  printf("\nQ2b ticks per  x = x / (f(line(i), 0, cond) + 1)  -- addresses independent of x\n");
  printf("      %-20s   cold,cond=false   warm,cond=false   cold,cond=true   warm,cond=true\n", "");
  for( int k=0; k<NF; k++ )
    printf("      %-20s %10.1f %16.1f %16.1f %16.1f\n", nm[k],
           q2_stream(F[k],0,1,reps), q2_stream(F[k],0,0,reps), q2_stream(F[k],1,1,reps), q2_stream(F[k],1,0,reps));
  printf("\n  Q1:  a warm number under cond=false means the instruction read the line.\n"
         "  Q2a: cold,false up at cold,true means the result waited for the line it did not select.\n"
         "  Q2b: flat everywhere is memory-level parallelism, not independence -- see Q2a.\n");
  return int(sink&1);
}
