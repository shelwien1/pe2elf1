
#include <stdarg.h>   // for pjpg0::pf(), the indenting printf

#include "common.inc"

#include "coro3b.inc"
#include "coro_fp2.inc"

#include "cinfo.inc"

#include "pjpg0j.inc"
#include "pjpg1.inc"

// One parser instance per nesting level.  A JPEG may carry a JPEG thumbnail in
// its Exif APP1 or JFXX APP0 segment, and that thumbnail may carry one of its
// own, so the levels are used recursively: level 0 parses the file, and when it
// meets a JPEG thumbnail it feeds those bytes to level 1 instead of skipping
// them.  PjpgLevels bounds the recursion; the deepest level skips.
const int PjpgLevels = 5;

struct pjpg1 {
  pjpg pjpg_[PjpgLevels];

  // CoroFileProc drives level 0 only; every deeper level is driven by the level
  // above it, so the frontend's view of this object is just level 0's.
  volatile uint& f_quit = pjpg_[0].f_quit;
  byte*&         outptr = pjpg_[0].outptr;
  byte*&         outbeg = pjpg_[0].outbeg;

  uint f_entropy = 1;      // decode entropy-coded scans

  void coro_init( void ) {
    uint i;
    for( i=0; i<PjpgLevels; i++ ) {
      pjpg_[i].coro_init();
      pjpg_[i].level = i;
      pjpg_[i].stream_base = 0;
      pjpg_[i].sub   = (i+1<PjpgLevels) ? &pjpg_[i+1] : 0;
      pjpg_[i].f_entropy = f_entropy;
    }
  }

  template <typename T> uint coro_call( T* ) { return pjpg_[0].coro_call( &pjpg_[0] ); }

  // feed(), not addinp(): it keeps level 0's byte counter in step with the file,
  // so every segment pjpg reports knows where in the file it came from.
  void addinp( byte* p, uint n ) { pjpg_[0].feed(p,n); }
  void addout( byte* p, uint n ) { pjpg_[0].addout(p,n); }

  // Worst outcome across every level, so a thumbnail that fails to parse is
  // visible in the exit status of the run that found it.
  uint worst_err( void ) {
    uint i,e; e=0;
    for( i=0; i<PjpgLevels; i++ ) if( pjpg_[i].err_code ) { e=pjpg_[i].err_code; break; }
    return e;
  }
};

CoroFileProc< pjpg1 > C;

int main( int argc, char **argv ) {

  // -s: structure only, skip the entropy-coded scans.  Decoding every Huffman
  // or arithmetic symbol costs about 50x the time of skipping the data, so the
  // fast structural pass stays available.
  int argi = 1;
  if( (argc>2) && (argv[1][0]=='-') && (argv[1][1]=='s') ) { C.f_entropy = 0; argi = 2; }

  if( argc<argi+1 ) { fprintf( stderr, "usage: %s [-s] <file.jpg>\n", argv[0] ); return 2; }

  FILE* f = fopen( argv[argi], "rb" );
  if( f==0 ) { fprintf( stderr, "%s: cannot open\n", argv[argi] ); return 2; }

  uint r = C.processfile( f, 0 );

  fclose( f );

  // 0 = parsed cleanly (warnings still allow 0), 1 = fatal parse error.
  return ((r==PJPG_ERR) || C.worst_err()) ? 1 : 0;
}
