
#include "common.inc"

#include "coro3b.inc"
#include "coro_fp2.inc"

#include "cinfo.inc"

#include "pjpg0j.inc"
#include "pjpg1.inc"

CoroFileProc< pjpg > C;

int main( int argc, char **argv ) {

  if( argc<2 ) { fprintf( stderr, "usage: %s <file.jpg>\n", argv[0] ); return 2; }

  FILE* f = fopen( argv[1], "rb" );
  if( f==0 ) { fprintf( stderr, "%s: cannot open\n", argv[1] ); return 2; }

  uint r = C.processfile( f, 0 );

  fclose( f );

  // 0 = parsed cleanly (warnings still allow 0), 1 = fatal parse error.
  return (r==PJPG_ERR) ? 1 : 0;
}
