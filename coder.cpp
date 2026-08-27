
#define INC_FLEN
#include "common.inc"
#include "rc_config.inc"
#include "file_api.inc"

#include "rc_cl.h"

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

// coder [options] c|d input_file output_file FSM_file [n_iter] [test_output_file]
//
//   coder c book1 1 FSM0.txt          -- encode
//   coder d 1     2 FSM0.txt          -- decode
//   coder c book1 1 FSM0.txt 10 2     -- encode x10, decode, append log.txt
//
// The FSM file is the counter state machine loaded by Predictor::Init; both
// ends need the same one.
//
// The options are all about the OpenCL path, which only the encoder's
// carryless coding pass uses -- see rc_cl.cpp. They are stripped out of argv
// before anything else looks at it, so the positional arguments are where they
// always were.
static void usage( void ) {
  printf( "coder [options] c|d input output FSM_file [n_iter] [test_output]\n"
          "\n"
          "  -l        list the OpenCL platforms and devices, and exit\n"
          "  -d <n>    use device <n>, numbered as -l prints it\n"
          "  -p <n>    only look at platform <n> (and number -d within it)\n"
          "  -T <t>    pick by type instead: cpu, gpu, acc\n"
          "  -C        do not use OpenCL at all -- the reference code path\n"
          "  -V        report the device and what its kernel cost\n" );
}

// Pull the options out of argv, leaving the positional arguments compacted at
// the front. Called again on the recursive test-mode invocation, where there
// are none left and g_clopt already says what was asked for.
static int parse_opts( int argc, char** argv ) {
  int n = 1;
  for( int i=1; i<argc; i++ ) {
    char* a = argv[i];
    if( a[0]!='-' || a[1]==0 ) { argv[n++] = a; continue; }
    char o = a[1];
    // only -d, -p and -T take a value, attached or as the next argument
    const char* v = 0;
    if( o=='d' || o=='p' || o=='T' ) {
      v = a[2] ? a+2 : ((i+1<argc) ? argv[++i] : 0);
      if( !v ) { fprintf( stderr, "coder: -%c wants a value\n", o ); usage(); exit(1); }
    }
    switch( o ) {
      case 'C': g_clopt.use = 0; break;
      case 'V': g_clopt.verbose = 1; break;
      case 'l': g_clopt.use = -1; break;              // handled in main
      case 'd': g_clopt.dev  = atoi(v); break;
      case 'p': g_clopt.plat = atoi(v); break;
      case 'T': g_clopt.type = (v[0]=='c') ? 1 : (v[0]=='g') ? 2 : 3; break;
      default:  fprintf( stderr, "coder: unknown option -%c\n", o ); usage(); exit(1);
    }
  }
  return n;
}

int main( int argc, char** argv ) {
  uint i,r,c;

  argc = parse_opts( argc, argv );

  if( g_clopt.use<0 ) { CL_ListDevices(stdout); return 0; }

  if( argc<5 ) { usage(); return 1; }

  // test mode: encode, then decode back, then log both timings
  if( argc>6 ) {
    char* argv_c[] = { argv[0], (char*)"c", argv[2],argv[3],argv[4],argv[5] };
    char* argv_d[] = { argv[0], (char*)"d", argv[3],argv[6],argv[4],(char*)"1" };
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

  // The device is the encoder's, and it is opened once: C_init runs per
  // processfile, and n_iter runs that as many times as it is asked to.
  CL_Enable( f_DEC==0 );

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

  CL_Report( stderr );
  CL_Quit();

  f.close();
  g.close();

  if( f_DEC==0 ) sprintf( c_res, "%6.2lfMB/s %5.3lfs", double(mx_sp)/100, double(mn_tm)/1E7 );
  if( f_DEC==1 ) sprintf( d_res, "%6.2lfMB/s %6.3lfs", double(mx_sp)/100, double(mn_tm)/1E7 );

  return 0;
}
