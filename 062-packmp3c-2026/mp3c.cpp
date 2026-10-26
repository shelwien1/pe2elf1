
#include "common.inc"

// mp3rw/frame_dump_io.inc serializes the raw bytes between &GrbLen and
// &Garbage, so the dump format depends on the exact layout of mp3frame.
// Lib/common.inc used to impose this globally; Lib3/common.inc deliberately
// does not, so the program has to ask for it itself.  Without it the frame
// fields are read back into the wrong offsets and the model walks off into
// garbage indices.
#pragma pack(1)

#define float error
#define double error

qword c_alloc = 0;

void* operator new( size_t N ) {
  void* p = malloc(N);
  c_alloc += N;
  return p;
}

#include "coro3b.inc"
#include "coro_fp2.inc"

#include "mp3hdr.inc"
#include "mp3mod.inc"

ALIGN(4096)
static union {
  CoroFileProc< mp3mod<0> > C;
  CoroFileProc< mp3mod<1> > D;
};

int main( int argc, char** argv ) {

//printf( "sizeof(C)=%i sizeof(D)=%i\n", sizeof(C), sizeof(D) );
  if( argc<4 ) return 1;

  uint f_DEC = (argv[1][0]=='d'); 

  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  if( f_DEC==0 ) {
    C.Init();
//printf( "size=%I64i c_alloc=%I64i\n", C.m_rDelta[0].M_datasize*8+sizeof(C), c_alloc );
    C.processfile( f, g );

//printf( "c_alloc=%I64i\n", c_alloc );

    c_alloc /= 1<<15;
//     void* buf = malloc(c_alloc);      
//     memset( buf, 0xCC, c_alloc );     
//     fwrite( buf, 1,c_alloc, g );      
    
  } else {
    D.Init();
    D.processfile( f, g );
  }

}
