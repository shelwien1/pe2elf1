
#include "common.inc"

#include "coro3b.inc"
//#include "coro2_pin.inc"

#include "coro_fp12.inc"
#include "coro_fp21.inc"

#include "mp3hdr.inc"
#include "mp3det.inc"

ALIGN(4096)
static union {
  CoroFileProc12< mp3det<0> > mp3w;
  CoroFileProc21< mp3det<1> > mp3r;
};

int main( int argc, char** argv ) {

  if( argc<5 ) return 1;

  uint f_DEC = (argv[1][0]=='d'); 

  if( f_DEC==0 ) {

    FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
    FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;
    FILE* h = fopen(argv[4],"wb"); if( h==0 ) return 4;

    mp3w.Init();
    mp3w.processfile( f, g, h );

  } else {

    FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
    FILE* g = fopen(argv[3],"rb"); if( g==0 ) return 3;
    FILE* h = fopen(argv[4],"wb"); if( h==0 ) return 4;

    mp3r.Init();
    mp3r.processfile( f, g, h );

  }

}
