// Standalone probe -- NOT part of the build.  See misc/icache_lut.md.
//
// Does executing a block of memory as CODE make it fast to read as DATA?
//
//   linux:    clang++ -O2 -o icache_probe icache_probe.cpp     (or g++)
//   windows:  cl /O2 icache_probe.cpp                           (or clang-cl)
//   ./icache_probe [flush_MB=256] [repeats=5]
//
// A 4K-aligned 128 KB block is filled with a data pattern and threaded with
// an instruction sequence that touches every cache line: a 2-byte short jump
// straddling each line boundary (bytes 63..64 of every line, `EB 3E`, target
// the next boundary), ending in `ret`.  Each line keeps 62 bytes of data.
// Executing it once pulls every line through the front end.
//
// Reads are timed as a pointer chase -- each line's data holds the index of
// the next line in a random single cycle -- so every load depends on the one
// before it and the tick count is the latency of wherever the line lives,
// not the bandwidth of a prefetcher streaming it.  Timed over the whole
// block and over its first and last 16 KB separately: 128 KB is four L1i's,
// so only the LAST lines executed can still be in L1i, and that tail is where
// an "L1i serves loads" effect would have to show up.
//
// States measured, each from a cold start:
//   cold          after reading a 256 MB block (and, separately, clflush)
//   after exec    the block executed as code, nothing read as data
//   after data    the block chased once as data
// plus a second chase of the 16 KB tail after a first, which is the L1d hit
// time on this machine, and a chase after clflush, which is DRAM.
//
// The Windows branch (VirtualAlloc/VirtualProtect/FlushInstructionCache,
// <intrin.h>) is written to the documented API but was not compile-checked
// on the Linux box this was developed on -- no cl or mingw there.
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
#include <unistd.h>
#include <x86intrin.h>
#endif

typedef uint8_t byte; typedef uint32_t uint; typedef uint64_t qword;
enum { LINE=64, BLOCK=128*1024, NLINE=BLOCK/LINE, REGION=16*1024/LINE, NEXT_OFS=8 };

static byte* alloc_exec( size_t n ) {
#ifdef _WIN32
  return (byte*)VirtualAlloc( 0, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE );
#else
  void* p = mmap( 0, n, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  return p==MAP_FAILED ? 0 : (byte*)p;
#endif
}
static int make_exec( byte* p, size_t n ) {   // RW -> RX; data reads still allowed
#ifdef _WIN32
  DWORD old; if( !VirtualProtect( p, n, PAGE_EXECUTE_READ, &old ) ) return 0;
  FlushInstructionCache( GetCurrentProcess(), p, n ); return 1;
#else
  return mprotect( p, n, PROT_READ|PROT_EXEC )==0;
#endif
}
static inline qword tsc_begin( void ) { _mm_lfence(); return __rdtsc(); }
static inline qword tsc_end( void ) { unsigned a; qword t = __rdtscp(&a); _mm_lfence(); return t; }

static byte* blk;                 // the 128 KB block
static byte* flushbuf; static size_t flushsz;
static volatile qword sink;

// A random single cycle over lines [lo, lo+n): Sattolo's shuffle.
static uint rng_s = 0x9E3779B9u;
static uint rng( void ) { rng_s ^= rng_s<<13; rng_s ^= rng_s>>17; rng_s ^= rng_s<<5; return rng_s; }
static void link_cycle( uint lo, uint n ) {
  static uint perm[NLINE];
  for( uint i=0; i<n; i++ ) perm[i] = lo+i;
  for( uint i=n-1; i>0; i-- ) { uint j = rng()%i; uint t=perm[i]; perm[i]=perm[j]; perm[j]=t; }
  for( uint i=0; i<n; i++ ) *(uint*)(blk + (size_t)perm[i]*LINE + NEXT_OFS) = perm[(i+1)%n];
}
// Chase n links from line `start`; returns ticks per load.
static double chase( uint start, uint n ) {
  uint cur = start; qword t0 = tsc_begin();
  for( uint i=0; i<n; i++ ) cur = *(volatile uint*)(blk + (size_t)cur*LINE + NEXT_OFS);
  qword t1 = tsc_end(); sink += cur;
  return double(t1-t0)/n;
}
static void flush_by_read( void ) {      // stream the big block through the caches
  qword s=0; for( size_t i=0; i<flushsz; i+=8 ) s += *(volatile qword*)(flushbuf+i); sink += s;
}
static void flush_by_clflush( void ) {
  for( size_t i=0; i<BLOCK; i+=LINE ) _mm_clflush( blk+i );
  _mm_mfence();
}
static void execute_block( void ) { ((void(*)(void))(blk+LINE-1))(); }   // first jmp is at byte 63

int main( int argc, char** argv ) {
  size_t flush_mb = argc>1 ? atoi(argv[1]) : 256;
  int reps = argc>2 ? atoi(argv[2]) : 5;
  flushsz = flush_mb<<20;

  blk = alloc_exec( BLOCK ); if( !blk ) { printf("alloc failed\n"); return 1; }
  flushbuf = (byte*)malloc( flushsz ); if( !flushbuf ) { printf("flush alloc failed\n"); return 1; }
  for( size_t i=0; i<flushsz; i++ ) flushbuf[i] = byte(i*7+3);        // touch it all

  // data pattern, then the code threaded through it
  for( size_t i=0; i<BLOCK; i++ ) blk[i] = byte(i*13+1);
  for( uint l=0; l+1<NLINE; l++ ) { blk[(size_t)l*LINE+63] = 0xEB; blk[(size_t)l*LINE+64] = 0x3E; }
  blk[(size_t)(NLINE-1)*LINE+63] = 0xC3;                                 // ret
  // three chains: whole block, first 16 KB, last 16 KB -- each its own cycle,
  // all stored in the line's data bytes (offset 8, never touching 63..64)
  link_cycle( 0, NLINE );                        // whole
  static uint nx_whole[NLINE]; for( uint i=0;i<NLINE;i++ ) nx_whole[i]=*(uint*)(blk+(size_t)i*LINE+NEXT_OFS);
  link_cycle( 0, REGION );                       // head 16 KB (overwrites those lines' links)
  static uint nx_head[REGION];  for( uint i=0;i<REGION;i++ ) nx_head[i]=*(uint*)(blk+(size_t)i*LINE+NEXT_OFS);
  link_cycle( NLINE-REGION, REGION );            // tail 16 KB
  static uint nx_tail[REGION];  for( uint i=0;i<REGION;i++ ) nx_tail[i]=*(uint*)(blk+(size_t)(NLINE-REGION+i)*LINE+NEXT_OFS);
  // restore the whole-block cycle; head/tail cycles are re-linked on demand
  auto set_links = [&]( int which ){
    for( uint i=0;i<NLINE;i++ ) *(uint*)(blk+(size_t)i*LINE+NEXT_OFS)=nx_whole[i];
    if( which==1 ) for( uint i=0;i<REGION;i++ ) *(uint*)(blk+(size_t)i*LINE+NEXT_OFS)=nx_head[i];
    if( which==2 ) for( uint i=0;i<REGION;i++ ) *(uint*)(blk+(size_t)(NLINE-REGION+i)*LINE+NEXT_OFS)=nx_tail[i];
  };
  // The links have to be in place BEFORE the page goes RX, so pick one layout
  // per experiment and re-map for it.  Simpler: keep the page RW+X where the
  // platform allows and RX otherwise; writes to an RX page fault, so we
  // re-protect around each relink.
  auto relink = [&]( int which ){
#ifdef _WIN32
    DWORD old; VirtualProtect( blk, BLOCK, PAGE_READWRITE, &old ); set_links(which);
    VirtualProtect( blk, BLOCK, PAGE_EXECUTE_READ, &old ); FlushInstructionCache( GetCurrentProcess(), blk, BLOCK );
#else
    mprotect( blk, BLOCK, PROT_READ|PROT_WRITE ); set_links(which); mprotect( blk, BLOCK, PROT_READ|PROT_EXEC );
#endif
  };
  if( !make_exec( blk, BLOCK ) ) { printf("make_exec failed\n"); return 1; }

  // TSC rate, so ticks can be read as ns
  { auto w0 = std::chrono::steady_clock::now(); qword t0 = __rdtsc();
    while( std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count() < 0.2 ) {}
    qword t1 = __rdtsc(); double s = std::chrono::duration<double>(std::chrono::steady_clock::now()-w0).count();
    printf("TSC %.2f GHz; block %d KB (%d lines), regions %d KB, flush %zu MB, %d repeats, min of each\n\n",
           (t1-t0)/s/1e9, BLOCK/1024, NLINE, REGION*LINE/1024, flush_mb, reps);
  }

  struct Row { const char* name; double whole, head, tail; };
  auto measure = [&]( const char* name, int prep ) -> Row {
    // prep: 0 = nothing (cold), 1 = execute the code, 2 = chase once as data,
    //       3 = chase the region twice (L1d reference), 4 = clflush then nothing
    Row r = { name, 1e9, 1e9, 1e9 };
    for( int rep=0; rep<reps; rep++ ) {
      double v[3];
      for( int which=0; which<3; which++ ) {
        relink( which );
        uint start = which==2 ? NLINE-REGION : 0, n = which==0 ? NLINE : REGION;
        flush_by_read(); flush_by_clflush();
        if( prep==1 ) execute_block();
        if( prep==2 ) { chase( start, n ); }
        if( prep==3 ) { chase( start, n ); chase( start, n ); }
        v[which] = chase( start, n );
      }
      if( v[0]<r.whole ) r.whole=v[0];
      if( v[1]<r.head  ) r.head =v[1];
      if( v[2]<r.tail  ) r.tail =v[2];
    }
    return r;
  };

  Row rows[5];
  rows[0] = measure( "cold (256MB read + clflush)", 0 );
  rows[1] = measure( "after EXECUTING the block  ", 1 );
  rows[2] = measure( "after reading it as data   ", 2 );
  rows[3] = measure( "read twice (L1d reference) ", 3 );
  rows[4] = measure( "cold again (control)       ", 0 );

  printf("  ticks per dependent load          whole 128KB   first 16KB   last 16KB\n");
  for( int i=0; i<5; i++ )
    printf("  %-32s %8.1f     %8.1f     %8.1f\n", rows[i].name, rows[i].whole, rows[i].head, rows[i].tail);
  printf("\n  If executing the block let loads read L1i, 'after EXECUTING' / last 16KB\n"
         "  would match the L1d reference. If it only fills L2, it sits between\n"
         "  L1d and cold on every column, tail included.\n");
  return int(sink&1);
}
