// paq8hpc - the paq8hp model from cmix, driven by the sh_v1m rangecoder.
//
// I/O is Lib3's coroutine layer, the same shape ddsdet and dxt5comp use: the
// codec is a Coroutine, its byte streams are two of that coroutine's pins, and
// CoroFileProc drives it.  When a window runs dry or fills up deep inside
// rc_Process, the pin's yield suspends the whole call stack and returns to the
// driver for a refill or a flush; the codec never sees a FILE*.
//
// The stdio build this replaces (getc/putc straight onto two handles) produced
// the same bytes, and so does this one -- the pins carry every byte of the
// stream, header included.

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <new>

//---------------------------------------------------------------- Lib3
// common.inc supplies the typedefs, if_e0/if_e1, INLINE/NOINLINE/ALIGN and the
// X64 probe that coro3b.inc's setjmp shim selects on -- the same set this file
// used to carry an inline copy of.  Include path: -I./Lib3, because coro3b.inc
// reaches its own headers by bare name.
#include "common.inc"
#include "coro3b.inc"
#include "coro_fp.inc"

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(__MINGW32__)
  #define __cdecl
#endif

//---------------------------------------------------------------- allocator

qword g_memory = 0;

// paq8hp.hpp no longer allocates anything: the model is one object whose every
// table is a fixed-size member, and every constructor writes what it needs.  So
// this no longer has to hand back zeroed storage, and the baseline's
// memset-everything (5.5 GB at level 11) is gone.
//
// The two switches below exist to prove that: -DPAQ_ZERO_NEW=1 restores the
// memset, -DPAQ_POISON_NEW=1 fills with 0xA5 instead.  All three builds must
// produce byte-identical archives; if one differs, something is being read
// before it is written.
#ifndef PAQ_ZERO_NEW
  #define PAQ_ZERO_NEW 0
#endif
#ifndef PAQ_POISON_NEW
  #define PAQ_POISON_NEW 0
#endif

#if PAQ_POISON_NEW && defined(PAQ_LAZY_ZERO)
  #error "PAQ_POISON_NEW and PAQ_LAZY_ZERO are contradictory: the poison fill is exactly what lazy-zero assumes absent"
#endif

// Set by the allocator when a block came straight from the OS and is therefore
// known zero.  PAQ_LAZY_ZERO reads it to skip the ContextMap/BH ctor memsets;
// there is exactly one big allocation in the program (`new Model`), so a single
// global flag describes it.
int g_zeroed = 0;

// -------- big-block allocator: OS pages, optionally 2M-backed --------------
// Both gates need the same thing - storage that comes from the OS rather than
// from the malloc free lists - so they share one path.  PAQ_HUGEPAGES adds the
// large-page request on top.  free1 is already a no-op, so nothing has to
// remember how a block was obtained.
#if defined(PAQ_HUGEPAGES) || defined(PAQ_LAZY_ZERO)
  #define PAQ_OSALLOC 1
  static constexpr size_t PAQ_PAGE_SIZE = 4096;
  static constexpr size_t PAQ_OSALLOC_MIN = 8u<<20;

  #if defined(_WIN32)
    #include <windows.h>

    static int paq_win_lockmem( void ) {
      HANDLE tok;
      TOKEN_PRIVILEGES tp;
      if( !OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&tok) ) return 0;
      int ok = 0;
      if( LookupPrivilegeValue(NULL,SE_LOCK_MEMORY_NAME,&tp.Privileges[0].Luid) ) {
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        // AdjustTokenPrivileges returns TRUE even when it did not grant the
        // privilege; only GetLastError() tells the truth.
        AdjustTokenPrivileges( tok, FALSE, &tp, 0, NULL, NULL );
        ok = (GetLastError()==ERROR_SUCCESS);
      }
      CloseHandle(tok);
      return ok;
    }

    static void* paq_osalloc( size_t size ) {
      #if defined(PAQ_HUGEPAGES)
      static int tried = 0, priv = 0;
      if( tried==0 ) tried = 1, priv = paq_win_lockmem();
      if( priv ) {
        SIZE_T lp = GetLargePageMinimum();
        if( lp ) {
          size_t rounded = (size+lp-1) & ~(size_t)(lp-1);
          void* p = VirtualAlloc( NULL, rounded, MEM_COMMIT|MEM_RESERVE|MEM_LARGE_PAGES, PAGE_READWRITE );
          if( p ) return p;
        }
      }
      #endif
      // plain VirtualAlloc still hands back zeroed pages, so PAQ_LAZY_ZERO
      // stays valid on the fallback
      return VirtualAlloc( NULL, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE );
    }
  #else
    #include <sys/mman.h>

    static void* paq_osalloc( size_t size ) {
      #if defined(PAQ_HUGEPAGES)
      constexpr size_t HP = 2u<<20;
      size_t over = size+HP;
      char* raw = (char*)mmap( 0, over, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
      if( raw!=(char*)MAP_FAILED ) {
        char* al = (char*)(((size_t)raw+HP-1) & ~(size_t)(HP-1));
        #if defined(MADV_HUGEPAGE)
        madvise( al, size, MADV_HUGEPAGE );
        #endif
        return al;
      }
      #endif
      void* p = mmap( 0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
      return (p==MAP_FAILED) ? 0 : p;
    }
  #endif
#endif

// `align` is the alignment C++ asks for, 0 when it does not care.  This matters
// here: Predictor has alignas(64) members (ContextMap::t, BH::t, Mixer::wx), so
// it is an over-aligned type and `new Model` resolves to the C++17 aligned
// operator new - NOT the plain one.  The baseline overrode only the plain form,
// so the single 4.5 GB model allocation went straight to the library allocator
// and never reached malloc1 at all: g_memory never saw it, and any allocator
// gate hung off malloc1 silently did nothing.  Both forms are overridden below.
void* __cdecl malloc1( size_t size, size_t align = 0, char c='m' ) {
  void* p = 0;
#if defined(PAQ_OSALLOC)
  // mmap/VirtualAlloc hand back page-granular storage, which satisfies every
  // alignment this program asks for (64).  NB this 4096 is a page size and has
  // nothing to do with paq8hp::P_SCALE, which is also 4096.
  if( size>=PAQ_OSALLOC_MIN && align<=PAQ_PAGE_SIZE ) {
    p = paq_osalloc(size);
    if( p ) g_zeroed = 1;
  }
#endif
  if( p==0 ) {
    if( align<=2*sizeof(void*) ) {
      p = malloc(size);
    } else {
#if defined(_WIN32)
      p = _aligned_malloc(size, align);
#else
      // aligned_alloc requires a size that is a multiple of the alignment
      p = aligned_alloc(align, (size+align-1) & ~(align-1));
#endif
    }
    if( p ) g_zeroed = 0;
  }
  if( p ) {
    g_memory += size;
    if( PAQ_POISON_NEW ) memset(p,0xA5,size); else
    if( PAQ_ZERO_NEW   ) memset(p,0,size);
  }
#if defined(PAQ_VERBOSE_ALLOC)
  printf( "alloc %.1fMB -> %p zeroed=%i\n", double(size)/(1<<20), p, g_zeroed );
#endif
//  printf( "!%calloc: %7I64i -> %I64X ! %I64iM \n", c, size, p, (g_memory+(1<<20)-1)>>20 );
  return p;
}

void __cdecl free1( void* ptr ) {
//  printf( "! free: p=%I64X !\n", ptr );
}

void* __cdecl operator new( size_t n ) { return malloc1(n); }
void __cdecl operator delete( void* p ) { free1(p); }

void* __cdecl operator new[]( size_t n ) { return malloc1(n); }
void __cdecl operator delete[]( void* p ) { free1(p); }

// the over-aligned forms `new Model` actually uses - see malloc1
void* __cdecl operator new( size_t n, std::align_val_t a ) { return malloc1(n,size_t(a)); }
void __cdecl operator delete( void* p, std::align_val_t ) { free1(p); }

void* __cdecl operator new[]( size_t n, std::align_val_t a ) { return malloc1(n,size_t(a)); }
void __cdecl operator delete[]( void* p, std::align_val_t ) { free1(p); }

#include "paq8hp.hpp"

#include "sh_v1m.inc"

//---------------------------------------------------------------- coder

// The coder's totFreq is the model's probability scale; naming it here keeps the
// rangecoder itself free of any model constant.
static constexpr uint PSCALE     = paq8hp::P_SCALE;   // 4096
static constexpr uint PSCALE_MIN = paq8hp::P_MIN;     // 1, p==0 is undecodable
static constexpr uint PSCALE_MAX = paq8hp::P_MAX;     // 4095

template< int f_DEC, int LEVEL >
struct Coder : Rangecoder<f_DEC> {

  typedef Rangecoder<f_DEC> RC;
  using RC::get;
  using RC::put;
  using RC::f_quit;
  using RC::nget;
  using RC::nput;
  using RC::rc_SetFiles;
  using RC::rc_Init;
  using RC::rc_Process;
  using RC::rc_GetFreq;
  using RC::rc_Decide;
  using RC::rc_Quit;

  typedef paq8hp::Predictor<LEVEL> Model;

  // Set by the driver before the coroutine starts.  On the way in it is the
  // plaintext length, which only the frontend can know (it stats the file); on
  // the way out do_process reads it back off the stream.
  uint f_len;
  uint level;
  int  rcode;

  Model* M;

  // The stream header, through the pins like everything else.  It used to be an
  // fwrite/fread on the FILE* before the coder ran, which is the one place a
  // coroutine port can silently change the output: the bytes have to keep going
  // out in the same order, so they go out the same way.  Little-endian by
  // construction, matching what fwrite of a uint did on x86.
  void put_hdr( void ) {
    put( f_len     &255 ); put( (f_len>> 8)&255 );
    put( (f_len>>16)&255 ); put( (f_len>>24)&255 );
    put( level&255 );
  }
  int get_hdr( void ) {
    uint b, i; f_len = 0;
    for( i=0; i<4; i++ ) { b=get(); if( b==uint(-1) ) return 4; f_len |= (b&255)<<(8*i); }
    b = get(); if( b==uint(-1) ) return 4;
    level = b&255;
    return 0;
  }

  // returns 0 on success, 3 if the model could not be allocated
  int process( void ) {

    rc_SetFiles();

    // The model is one object with every table a fixed-size member, so this is
    // the only allocation in the program.  It is on the heap rather than static
    // because a global would put EVERY instantiated level in .bss at once (the
    // dispatcher covers 0..11, which is ~11 GB of it); this way only the level
    // actually asked for is committed.  malloc1 zeroes it, but nothing relies
    // on that - see -DPAQ_POISON_NEW.
    M = new Model; if( M==0 ) return 3;

    if( f_DEC==0 ) put_hdr();

    rc_Init();

    uint c,z,i,k,p,bit,px[3];

    for( i=0; i<f_len; i++ ) {

//if( i==16 ) printf( "Allocated %.0lfMB of memory\n", double(g_memory)/(1<<20) );
if_e0( (i&0xFFFF)==0 ) { printf( "%u -> %u\r", uint(nget), uint(nput) ); fflush(stdout); }

      if( f_DEC==0 ) c=get();

      for( k=7,z=0; k!=-1; k-- ) {
        bit = (c>>k)&1;

        p = M->p();
        if_e0( p<PSCALE_MIN ) p=PSCALE_MIN;
        if_e0( p>PSCALE_MAX ) p=PSCALE_MAX;

        px[0]=0; px[1]=PSCALE-p; px[2]=PSCALE;

#if defined(PAQ_DECFAST)
        if( f_DEC==1 ) bit = rc_Decide(p,PSCALE);
#else
        if( f_DEC==1 ) bit = (rc_GetFreq(PSCALE)>=px[1]);
#endif

        rc_Process( px[bit], px[1+bit]-px[bit], PSCALE );

        M->Perceive(bit);
        z = z*2 + bit;
      }

      if( f_DEC==1 ) put(z);
    }

    rc_Quit();

    delete M;

    printf( "%u -> %u\n", uint(nget), uint(nput) );

    return 0;
  }

  // The coroutine entry point.  CoroFileProc::coro_call lands here on the first
  // call and never returns to it: process() runs to completion, yielding out of
  // get()/put() whenever a window needs servicing, and the final yield(0) is
  // what tells the driver the run is over.
  void do_process( void ) {
    rcode = 0;
    // The decoder's header is the first thing on its input, so it is read here
    // rather than by the frontend -- the frontend has no stream to read from.
    if( f_DEC==1 ) rcode = get_hdr();
    if( rcode==0 ) rcode = process();
    yield( this, 0 );
  }

};

//---------------------------------------------------------------- level dispatch

// The model tables are sized from the template argument, so the level has to be
// a compile-time constant.  MAXLEVEL is 11 because at 12 the (U32)MEM()*16 the
// two big ContextMaps are built from wraps to 0.
#if defined(FIXED_LEVEL)
constexpr int MAXLEVEL = FIXED_LEVEL, MINLEVEL = FIXED_LEVEL;
#else
constexpr int MAXLEVEL = 11, MINLEVEL = 0;
#endif

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

// The carrier.  CoroFileProc is Lib3's two-pin driver, the same one dxt5comp
// wraps its codec in: it owns the two 64 KB windows, pumps pin[0] from a FILE*
// and drains pin[1] through LazyOut, and does not know or care what the model
// between them is.
template< int f_DEC, int LEVEL >
using Carrier = CoroFileProc< Coder<f_DEC,LEVEL> >;

template< int f_DEC, int LEVEL >
struct Dispatch {
  static int run( uint level, FILE* f, const char* outp, uint f_len ) {
    if( level==LEVEL ) {
      // The carrier is heap-allocated for the reason it always was -- one
      // instantiation per level and only the requested one committed -- and it
      // now also carries the coroutine's 64 KB stack and the two windows.
      auto PC = new Carrier<f_DEC,LEVEL>;
      g_memory = sizeof(typename Coder<f_DEC,LEVEL>::Model);
      if( PC==0 ) { printf( "Failed to allocate %iMB\n", int(g_memory>>20) ); exit(1); }
      auto& C = *PC;
      C.f_len = f_len;
      C.level = level;
      C.rcode = 0;
      C.out.path = outp;
      printf( "level %i, model %.0lfMB\n", LEVEL, double(sizeof(typename Coder<f_DEC,LEVEL>::Model))/(1<<20) );
      C.processfile( f, 0 );
      int r = C.rcode;
      // LazyOut creates the file on the first byte written, which is what keeps
      // a run that dies early from leaving a zero-length archive behind.  A run
      // that SUCCEEDS with nothing to write is a different case and has to
      // produce the empty file: decoding an empty plaintext is a legitimate
      // round trip, and the stdio build made it by opening the handle up front.
      if( r==0 ) C.out.w();
      if( !C.out.close() ) r = r ? r : 3;
      return r;
    }
    // -DFIXED_LEVEL=n compiles this one instantiation only; the recursion stops
    // immediately instead of dragging the other eleven model bodies into the
    // binary.
    if( LEVEL>int(MINLEVEL) ) return Dispatch<f_DEC,(LEVEL>int(MINLEVEL) ? LEVEL-1 : -1)>::run( level, f, outp, f_len );
    printf( "level %u not supported (%i..%i)\n", level, int(MINLEVEL), int(MAXLEVEL) );
    return 5;
  }
};

template< int f_DEC >
struct Dispatch<f_DEC,-1> {
  static int run( uint level, FILE*, const char*, uint ) {
    printf( "level %u not supported (%i..%i)\n", level, int(MINLEVEL), int(MAXLEVEL) );
    return 5;
  }
};

int main( int argc, char** argv ) {
  double cpu_time_used;
  clock_t st, ed;
  int r;

  if( argc<4 ) {
    printf( "paq8hpc c <input> <output> [level=11]\n"
            "        d <input> <output>\n" );
    return 1;
  }

  printf( "kernel %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", PAQ_KERNEL
#if defined(PAQ_N16)
          , " n16"
#else
          , ""
#endif
#if defined(PAQ_DOT2)
          , " dot2"
#else
          , ""
#endif
#if defined(PAQ_PREFETCH)
          , " pf"
#else
          , ""
#endif
#if defined(PAQ_PF2)
          , " pf2"
#else
          , ""
#endif
#if defined(PAQ_RCMPF)
          , " rcmpf"
#else
          , ""
#endif
#if defined(PAQ_APMPF)
          , " apmpf"
#else
          , ""
#endif
#if defined(PAQ_WXPF)
          , " wxpf"
#else
          , ""
#endif
#if defined(PAQ_CXTFL_T)
          , " cxtfl"
#else
          , ""
#endif
#if defined(PAQ_GETSIMD)
          , " getsimd"
#else
          , ""
#endif
#if defined(PAQ_RESTRICT)
          , " restrict"
#else
          , ""
#endif
#if defined(PAQ_HUGEPAGES)
          , " hugepages"
#else
          , ""
#endif
#if defined(PAQ_LAZY_ZERO)
          , " lazyzero"
#else
          , ""
#endif
#if defined(PAQ_DECFAST)
          , " decfast"
#else
          , ""
#endif
        );

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;

  uint level = 11;
  uint f_len = 0;
  if( f_DEC==0 ) {
    // Only the encoder can know the plaintext length, and only from outside the
    // stream; it is handed to the coroutine, which writes it into the header.
    if( argc>4 ) level = atoi(argv[4]);
    f_len = flen(f);
  } else {
    // The decoder's header is the first five bytes of its input, so it is read
    // through the pins by do_process rather than here.  The level therefore is
    // not known until the coroutine has started -- but the level selects the
    // Coder instantiation, which has to be chosen before it can start.  So the
    // frontend peeks the one byte it needs and rewinds; the coroutine still
    // reads the whole header itself, and the stream is unchanged.
    byte hdr[5];
    if( fread( hdr,1,5,f )!=5 ) return 4;
    f_len = hdr[0]|(hdr[1]<<8)|(hdr[2]<<16)|(uint(hdr[3])<<24);  // for the speed line
    level = hdr[4];
    rewind( f );
  }

  st = clock();

  const char* outp = argv[3];
  if( f_DEC==0 ) {
    r = Dispatch<0,MAXLEVEL>::run( level, f, outp, f_len );
  } else {
    r = Dispatch<1,MAXLEVEL>::run( level, f, outp, f_len );
  }

  ed = clock();
  cpu_time_used = ((double) (ed - st)) / CLOCKS_PER_SEC;
  double speed = (double)(f_len)/cpu_time_used/(1<<10);
  printf("%.2f KB/s\n", speed);

  fclose(f);

  if( r ) printf( "error %i\n", r );

  return r;
}
