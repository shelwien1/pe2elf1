// paq8hpc - the paq8hp model from cmix, driven by the sh_v1m rangecoder.
//
// This build has no Lib3 dependency: the Coroutine / coro3_pin / CoroFileProc
// layer is gone and the coder talks to stdio directly (getc/putc).  Output is
// bit-identical to the coroutine build.

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

//---------------------------------------------------------------- was common.inc

typedef unsigned short     word;
typedef unsigned int       uint;
typedef unsigned char      byte;
typedef unsigned long long qword;

#ifdef __GNUC__
  #define if_e0(x) if(__builtin_expect((x),0))
  #define if_e1(x) if(__builtin_expect((x),1))
#else
  #define if_e0(x) if(x)
  #define if_e1(x) if(x)
#endif

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

void* __cdecl malloc1( size_t size, char c='m' ) {
  void* p = malloc(size);
  if( p ) {
    g_memory += size;
    if( PAQ_POISON_NEW ) memset(p,0xA5,size); else
    if( PAQ_ZERO_NEW   ) memset(p,0,size);
  }
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

#include "paq8hp.hpp"

#include "sh_v1m.inc"

//---------------------------------------------------------------- coder

template< int f_DEC, int LEVEL >
struct Coder : Rangecoder<f_DEC> {

  typedef Rangecoder<f_DEC> RC;
  using RC::get;
  using RC::put;
  using RC::nget;
  using RC::nput;
  using RC::rc_SetFiles;
  using RC::rc_Init;
  using RC::rc_Process;
  using RC::rc_GetFreq;
  using RC::rc_Quit;

  typedef paq8hp::Predictor<LEVEL> Model;

  uint f_len;

  Model* M;

  // returns 0 on success, 3 if the model could not be allocated
  int process( FILE* f, FILE* g ) {

    rc_SetFiles( f, g );

    // The model is one object with every table a fixed-size member, so this is
    // the only allocation in the program.  It is on the heap rather than static
    // because a global would put EVERY instantiated level in .bss at once (the
    // dispatcher covers 0..11, which is ~11 GB of it); this way only the level
    // actually asked for is committed.  malloc1 zeroes it, but nothing relies
    // on that - see -DPAQ_POISON_NEW.
    M = new Model; if( M==0 ) return 3;

    rc_Init();

    uint c,z,i,k,p,bit,px[3];

    for( i=0; i<f_len; i++ ) {

//if( i==16 ) printf( "Allocated %.0lfMB of memory\n", double(g_memory)/(1<<20) );
if_e0( (i&0xFFFF)==0 ) { printf( "%u -> %u\r", uint(nget), uint(nput) ); fflush(stdout); }

      if( f_DEC==0 ) c=get();

      for( k=7,z=0; k!=-1; k-- ) {
        bit = (c>>k)&1;

        p = M->p();
        if_e0( p<1 ) p=1;
        if_e0( p>4095 ) p=4095;

        px[0]=0; px[1]=4096-p; px[2]=4096;

        if( f_DEC==1 ) bit = (rc_GetFreq(4096)>=px[1]);

        rc_Process( px[bit], px[1+bit]-px[bit], 4096 );

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

};

//---------------------------------------------------------------- level dispatch

// The model tables are sized from the template argument, so the level has to be
// a compile-time constant.  MAXLEVEL is 11 because at 12 the (U32)MEM()*16 the
// two big ContextMaps are built from wraps to 0.
enum{ MAXLEVEL = 11 };

template< int f_DEC, int LEVEL >
struct Dispatch {
  static int run( uint level, FILE* f, FILE* g, uint f_len ) {
    if( level==LEVEL ) {
      //static Coder<f_DEC,LEVEL> C;
      auto PC = new Coder<f_DEC,LEVEL>;
      g_memory = sizeof(typename Coder<f_DEC,LEVEL>::Model);
      if( PC==0 ) { printf( "Failed to allocate %iMB\n", int(g_memory>>20) ); exit(1); }
      //printf( "Allocated %.0lfMB of memory\n", double(g_memory)/(1<<20) );
      auto& C = *PC;
      C.f_len = f_len;
      printf( "level %i, model %.0lfMB\n", LEVEL, double(sizeof(typename Coder<f_DEC,LEVEL>::Model))/(1<<20) );
      return C.process(f,g);
    }
    return Dispatch<f_DEC,LEVEL-1>::run( level, f, g, f_len );
  }
};

template< int f_DEC >
struct Dispatch<f_DEC,-1> {
  static int run( uint level, FILE*, FILE*, uint ) {
    printf( "level %u not supported (0..%i)\n", level, int(MAXLEVEL) );
    return 5;
  }
};


uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

enum{ IOBUFSIZE = 1<<16 };
static byte iobuf_f[IOBUFSIZE];
static byte iobuf_g[IOBUFSIZE];

int main( int argc, char** argv ) {
  double cpu_time_used;
  clock_t st, ed;
  int r;

  if( argc<4 ) {
    printf( "paq8hpc c <input> <output> [level=11]\n"
            "        d <input> <output>\n" );
    return 1;
  }

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  setvbuf( f, (char*)iobuf_f, _IOFBF, IOBUFSIZE );
  setvbuf( g, (char*)iobuf_g, _IOFBF, IOBUFSIZE );

  uint level = 11;
  uint f_len = 0;
  if( f_DEC==0 ) {
    if( argc>4 ) level = atoi(argv[4]);
    f_len = flen(f);
    fwrite( &f_len,1,4,g );
    fwrite( &level,1,1,g );
  } else {
    if( fread( &f_len,1,4,f )!=4 ) return 4;
    level=0; if( fread( &level,1,1,f )!=1 ) return 4;
  }

  st = clock();

  if( f_DEC==0 ) {
    r = Dispatch<0,MAXLEVEL>::run( level, f, g, f_len );
  } else {
    r = Dispatch<1,MAXLEVEL>::run( level, f, g, f_len );
  }

  ed = clock();
  cpu_time_used = ((double) (ed - st)) / CLOCKS_PER_SEC;
  double speed = (double)(f_len)/cpu_time_used/(1<<10);
  printf("%.2f KB/s\n", speed);

  fclose(g);
  fclose(f);

  if( r ) printf( "error %i\n", r );

  return r;
}
