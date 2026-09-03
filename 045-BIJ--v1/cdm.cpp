// CDM r045 -- the r044 model over sh_bit's bijective bitwise rangecoder.
//
//   encode:  file -> CDM<0> ---bits---> midbuf -> BitWrap<0>::bit2byte -> archive
//   decode:  archive -> BitWrap<1>::byte2bit -> midbuf ---bits---> CDM<1> -> file
//
// Same two-stage CoroutinePair shape as sh_bitc.cpp: each stage reads pin[0] and
// writes pin[1], bits travel one per byte through the mid-buffer, and the
// bit/byte packing at the end of the encode chain is BitstringWrap's bijection.

#define INC_FLEN
#include "common.inc"
#include "file_api.inc"

#include "coro3b.inc"

qword g_prog_in = 0; // for CoroFileProc

#include "coro2pair.inc"

#include "coro_fhp2.inc"

#include "bitwrap.inc"

#include "cdm.inc"

// ---- bit wrap stage (sh_bitc.cpp, unchanged) ------------------------------------
// mode 0: bits in on pin[0] (midbuf), bytes out on pin[1] (file)
// mode 1: bytes in on pin[0] (file), bits out on pin[1] (midbuf)
template< int mode >
struct BitWrap : BitstringWrap<Coroutine> {
  void do_process( void ) {
    mode ? byte2bit() : bit2byte();
    yield(this,0);
  }
};

// The mid-buffer carries one bit per byte, so it holds MIDBUFSIZE codestream
// bits per hop.  CDM's codestream runs about one bit per input bit, and the
// pair refills the buffer as often as it needs to, so the default is fine.
typedef CoroFileProc< CoroutinePair< CDM<0>, BitWrap<0> > > t_encproc;
typedef CoroFileProc< CoroutinePair< BitWrap<1>, CDM<1> > > t_decproc;

// Separate objects rather than r044's union: the self-test alternates encode and
// decode thousands of times, and CDM's Init() rebuilds a 1 MB price table.
ALIGN(4096) static t_encproc C;
ALIGN(4096) static t_decproc D;

#include "tests.inc"

int main( int argc, char** argv ) {

  if( (argc>=2) && (argv[1][0]=='t') ) return run_tests();

  if( argc<4 ) { fprintf( stderr, "usage: %s c|d in out   |   %s t\n", argv[0],argv[0] ); return 1; }

  uint f_DEC = (argv[1][0]=='d');

  filehandle f( argv[2],0 ); if( f==0 ) return 2;
  filehandle g( argv[3],1 ); if( g==0 ) return 3;

  if( f_DEC==0 ) {
    C.M1.Init();
    C.processfile( f, g );
  } else {
    D.M2.Init();
    D.processfile( f, g );
  }

  g.close();
  f.close();

  return 0;
}
