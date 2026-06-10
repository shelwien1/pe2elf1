
#define INC_LOG2I
#include "common.inc"
#include "coro3b.inc"

#include "coro_fp.inc"

FILE* PF;

#include "zipdump.inc"

CoroFileProc< zipdump > M;

int main( int argc, char** argv ) {

  if( argc<4 ) return 1;

  // 'g' mode: emit a CBMC key-recovery model instead of (de)crypting.
  //   zipcl g <archive> <model.cpp> <nunk> <k0> <k1> <k2> [nfiles] [usecheck]
  if( argv[1][0]=='g' ) {
    if( argc<8 ) return 1;
    FILE* f  = fopen(argv[2],"rb"); if( f==0 )  return 2;
    FILE* gf = fopen(argv[3],"wb"); if( gf==0 ) return 3;
    M.gen = 1; M.gf = gf; M.mode = 1; M.gcount = 0;
    M.nunk = atoi(argv[4]);
    uint k0=0,k1=0,k2=0;
    sscanf(argv[5],"%X",&k0); sscanf(argv[6],"%X",&k1); sscanf(argv[7],"%X",&k2);
    M.k0=M.genk0=k0; M.k1=M.genk1=k1; M.k2=M.genk2=k2;
    M.gmax       = (argc>8) ? atoi(argv[8]) : 8;
    M.g_usecheck = (argc>9) ? atoi(argv[9]) : 1;
    M.g_reverse  = (argc>10) ? atoi(argv[10]) : 0;
    M.g_defl     = (argc>11) ? atoi(argv[11]) : 0;
    M.password[0]=0;
    PF = fopen("/dev/null","wb"); if( PF==0 ) return 3;
    FILE* dn = fopen("/dev/null","wb"); if( dn==0 ) return 3;
    if( M.g_reverse ) M.gen_header_rev(); else M.gen_header();
    M.processfile( f, dn );
    M.gen_footer();   // same footer (match label) for both directions
    fclose(gf); fclose(f); fclose(dn);
    fprintf( stderr, "generated %s: %u file(s), %u unknown bit(s), key %08X %08X %08X\n",
             argv[3], M.gcount, M.nunk, k0, k1, k2 );
    return 0;
  }

  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  M.gen = 0;
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

