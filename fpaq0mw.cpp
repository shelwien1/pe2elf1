
//#include "tsx.inc"
//#include "tsx1.inc"
enum{ PSCALE=1<<16 };

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#define assert(x)

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) 
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// __max/__min are MSVC/MinGW CRT macros; provide them elsewhere (glibc etc.)
#ifndef __max
 #define __max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef __min
 #define __min(a,b) (((a)<(b))?(a):(b))
#endif


#include "sh_mixer.inc"

#include "model.inc"

#include "sh_v1m.inc"

#include "coder.inc"

#ifndef SIM_FUNC

uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

#include "log2lut.inc"
#include "track.inc"

//////////////////////////// main ////////////////////////////

Coder E;

int main(int argc, char** argv) {

  // Chech arguments: fpaq0 c/d input output
  if (argc!=4 || (argv[1][0]!='c' && argv[1][0]!='d')) {
    printf("To compress:   fpaq0m c input output\n"
           "To decompress: fpaq0m d input output\n");
    exit(1);
  }

  uint f_DEC = (argv[1][0]!='c');

  // Open files
  FILE *in=fopen(argv[2], "rb");  if( in==0 ) perror(argv[2]), exit(1);
  FILE *out=fopen(argv[3], "wb"); if( out==0 ) perror(argv[3]), exit(1);
  int c,d;
  uint i,j,f_len,bit,p,r;
  int code,low,total=0,freq[1+256];

  Track_Init();

  if( f_DEC==0 ) {
    f_len=flen(in); fwrite( &f_len,1,4,out );
    E.rc_Init(COMPRESS,out);
  } else {
    f_len=0; fread( &f_len,1,4,in );
    E.rc_Init(DECOMPRESS,in);
  }

  for( i=0; i<f_len; i++ ) { 

    uint clen[0x200]; 
    test_encode( E, clen );

    for( d=0,low=0; d<256; d++ ) {
      r = LOG2(SCALE)*8 - clen[0x100+d];

      if( r>0xFFFFF ) r=0xFFFFF;
      freq[d]=unlog[r]; 
      low += freq[d];
    } freq[256] = total = low;

    if( f_DEC==0 ) {

      c = byte(getc(in));
      for( j=0,low=0; j<c; j++ ) low+=freq[j];

    } else {

      code = E.rc.rc_GetFreq(total);
      for( c=0,low=0; low+freq[c]<=code; c++ ) low+=freq[c];
      putc( byte(c), out );

    }

    E.rc.rc_Process(low,freq[c],total);

    // actually update model with current symbol
    for( int j=7; j>=0; --j ) E.encode_sim( (c>>j)&1 );

  } // for i

  if( f_DEC==0 ) E.flush();

  return 0;
}

#endif
