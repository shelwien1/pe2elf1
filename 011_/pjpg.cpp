
#include "common.inc"

#include "coro3b.inc"
#include "coro_fp2.inc"

#include "cinfo.inc"

#include "pjpg0j.inc"
#include "pjpg1.inc"

CoroFileProc< pjpg > C;

int main( int argc, char **argv ) {

  if( argc<2 ) return 1;

//  uint f_DEC = (argv[1][0]=='d'); // decoding?
  FILE* f = fopen( argv[1], "rb" ); if( f==0 ) return 1;
//  FILE* g = fopen( argv[3], "wb" ); if( g==0 ) return 1;

//  init_lentag();

  C.processfile( f, 0 );

  return 0;
}


