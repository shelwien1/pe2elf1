
#define INC_LOG2I
#include "common.inc"
#include "coro3b.inc"

#include "coro_fp.inc"

FILE* PF;

#include "zipdump.inc"

CoroFileProc< zipdump > M;

int main( int argc, char** argv ) {

  if( argc<4 ) return 1;

  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  strcpy( M.password, &argv[1][1] );
  M.mode = (argv[1][0]=='d');

  PF = fopen(argv[4],M.mode?"wb":"rb"); if( PF==0 ) return 3;

  //2ab047bf 567539f8 36ab563e
  uint k0,k1,k2, p0,p1,p2;
  k0=0; sscanf(argv[5],"%X",&k0);
  k1=0; sscanf(argv[6],"%X",&k1);
  k2=0; sscanf(argv[7],"%X",&k2);
//  printf( "2ab047bf 567539f8 36ab563e\n" );
  printf( "key: %08X %08X %08X\n", k0, k1, k2 );
  M.k0 = k0;
  M.k1 = k1;
  M.k2 = k2;

#if 0
  if( M.mode==0 ) {
  p0=0; sscanf(argv[7],"%X",&p0);
  p1=0; sscanf(argv[8],"%X",&p1);
  p2=0; sscanf(argv[9],"%X",&p2);
//  printf( "9470EF62 60DA6268 9A65DFC7\n" );
  printf( "pad: %08X %08X %08X\n", p0, p1, p2 );
  M.pad0 = p0;
  M.pad1 = p1;
  M.pad2 = p2;
  }
#endif

  M.processfile( f, g );

  fclose(f);
  fclose(g);

  return 0;
}

