#include <stdlib.h>

#define INC_FLEN
#include "common.inc"
#include "rc_config.inc"
#include "file_api.inc"



#include "misc/timer.h"
#include "misc/valloc.inc"
#include "misc/model.h"

// inpbufsize / outbufsize come from rc_config.inc, so the model can assert
// against them (model1.inc needs outbufsize >= BLKSIZE).
#if CORO_FAKE==0
ALIGN(4096) byte _inpbuf[inpbufsize];
ALIGN(4096) byte _outbuf[outbufsize];
byte* __restrict inpbuf=_inpbuf;
byte* __restrict outbuf=_outbuf;
#else
byte* __restrict inpbuf=0;
byte* __restrict outbuf=0;
#endif

uint processfile_init() {
  if( inpbuf==0 ) inpbuf=(byte*)VAlloc(inpbufsize); if( inpbuf==0 ) return 5;
  if( outbuf==0 ) outbuf=(byte*)VAlloc(outbufsize); if( outbuf==0 ) return 5;
  return 0;
}

template< int f_DEC >
NOINLINE
uint processfile( void* M, filehandle f, filehandle g ) {
  uint r,l;

  if( (r = C_init<f_DEC>(M,0)) ) { printf( "C_init(0) error %i\n", r ); return 1; };

  C_addout<f_DEC>( M, outbuf, outbufsize );

  goto r1;

  while(1) {
    r = C_process<f_DEC>(M);
    if( r==1 ) {
r1:
      l = f.read( inpbuf, inpbufsize );
      C_addinp<f_DEC>( M, inpbuf, l );
    } else { // r0=quit, r3=error
      l = C_getoutsize<f_DEC>(M);
      if( l>0 ) g.writ( outbuf, l );
      if( r!=2 ) break;
      C_addout<f_DEC>( M, outbuf, outbufsize );
    } // if
  } // while
  C_quit<f_DEC>(M);

  return 0;
}

char c_res[256];
char d_res[256];
char t_res[256];

// coder [options] c|d in out FSM_file [n_iter] [test_output_file] [n_iter_dec]
//
//   coder c book1 1 FSM0.txt          -- encode
//   coder d 1     2 FSM0.txt          -- decode
//   coder c book1 1 FSM0.txt 10 2     -- encode x10, decode, append log.txt
//   coder c book1 1 FSM0.txt 10 2 10  -- ... and decode x10 as well
//
// The FSM file is the counter state machine loaded by Predictor::Init; both
// ends need the same one.
//

static void usage( void ) {
  printf( "coder c|d in out FSM_file [n_iter] [test_output] [n_iter_dec]\n" );
}

// Pull the options out of argv, leaving the positional arguments compacted at
// the front. Called again on the recursive test-mode invocation, where there
// are none left.
static int parse_opts( int argc, char** argv ) {
  int n = 1;
  for( int i=1; i<argc; i++ ) {
    char* a = argv[i];
    if( a[0]!='-' || a[1]==0 ) { argv[n++] = a; continue; }
    switch( a[1] ) {
      default:  fprintf( stderr, "coder: unknown option -%c\n", a[1] ); usage(); exit(1);
    }
  }
  return n;
}

int main( int argc, char** argv ) {

  uint i,r,c;

  argc = parse_opts( argc, argv );

  if( argc<5 ) { usage(); return 1; }

  // test mode: encode, then decode back, then log both timings
  if( argc>6 ) {
    char* argv_c[] = { argv[0], (char*)"c", argv[2],argv[3],argv[4],argv[5] };
    char* argv_d[] = { argv[0], (char*)"d", argv[3],argv[6],argv[4],(argc<=7?(char*)"1":argv[7]) };
    uint r = 0;
    if( r==0 ) r = main( DIM(argv_c), argv_c );
    if( r==0 ) main( DIM(argv_c), argv_d );
    if( r==0 ) if( FILE* g=fopen( "log.txt", "ab" ) ) {
      FILE* G = fopen( argv[3], "rb" ); if( G==0 ) return 2;
      uint f_len=flen(G);
      sprintf(t_res,"%03i,%03i,%03i,%03i", (f_len/int(1E9))%1000,(f_len/int(1E6))%1000,(f_len/int(1E3))%1000,f_len%1000);
      char* p = t_res; while((p[0]=='0')||(p[0]==',')) ++p; p-=(*p==0);
      #ifndef __DIRNAM__
      #define __DIRNAM__1 __FILE__
      #else
      #define Q1(x) #x
      #define Q(x) Q1(x)
      #define __DIRNAM__1 Q(__DIRNAM__)
      #endif
      fprintf(g, "%s%s %-10s // %s RCNUM=%i LB=%i\n", c_res,d_res, p, __DIRNAM__1, int(RCNUM), int(RC_LOWBYTES) );
      fclose( G );
      fclose( g );
    }
    return 0;
  }

  uint f_DEC = (argv[1][0]=='d');

  uint M_size = Max(C_get_object_size<0>(),C_get_object_size<1>());
  void* M = VAlloc(M_size); if( M==0 ) return 5;
  if( (r = C_init<0>(M,argv[4])) ) { printf( "C_init error %i\n", r ); return 1; };

  filehandle f(argv[2],0); if( f==0 ) return 2;
  filehandle g(argv[3],1); if( g==0 ) return 3;

  if( processfile_init() ) return 6;

  qword f_len = f.size();
  uint n_iter = argc>5?atoi(argv[5]):1;

  uint mx_sp = 0; qword cpu_time_used, mn_tm=-1ULL;

  for( i=0; i<n_iter; i++ ) {

    if( i>0 ) { f.seek(0); g.seek(0); }

    qword st = GetTickCount1();

    if( f_DEC==0 ) {
      processfile<0>( M, f, g );
    } else {
      processfile<1>( M, f, g );
    }

    qword ed = GetTickCount1();

    cpu_time_used = Max<qword>(1,ed-st);
    uint speed = (qword(f_DEC?g.size():f_len)*10000000*100) / cpu_time_used / (1<<20);
    mx_sp = Max<uint>(mx_sp,speed);
    mn_tm = Min<qword>(mn_tm,cpu_time_used);
    printf("\r%s %3i.%02iMB/s %i.%03is   #%i", f_DEC?"dec":"enc", mx_sp/100, mx_sp%100, int(cpu_time_used/10000000),int((cpu_time_used/10000)%1000), i+1 );
  }

  printf( "\n" );


  f.close();
  g.close();

  if( f_DEC==0 ) sprintf( c_res, "%6.2lfMB/s %5.3lfs", double(mx_sp)/100, double(mn_tm)/1E7 );
  if( f_DEC==1 ) sprintf( d_res, "%6.2lfMB/s %6.3lfs", double(mx_sp)/100, double(mn_tm)/1E7 );

  return 0;
}
