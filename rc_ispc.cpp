
// -------------------------------------------------------------
//  rc_ispc.cpp -- the host side of the device path, which is one function
//  call: rc_encode is compiled by ispc at build time from the generated
//  rc_kernel.ispc, and DEV_Encode runs it synchronously into the caller's
//  substream rows. Model a block, code a block -- the original sh_v1xN
//  arrangement. Nothing is queued, nothing can fail asynchronously, and a
//  build without RC_ISPC is just the stubs at the bottom.
// -------------------------------------------------------------

#define INC_FLEN
#include "common.inc"
#include "rc_config.inc"
#include "rc_dev.h"

DEVOpts g_devopt = {1, 0};

#if RC_ISPC

#include "rc_kernel_ispc.h"

#include <time.h>
static double tnow( void ) {
#ifdef _WIN32
  return double(clock())/CLOCKS_PER_SEC;
#else
  timespec t; clock_gettime( CLOCK_MONOTONIC, &t );
  return double(t.tv_sec) + double(t.tv_nsec)*1e-9;
#endif
}

static double g_kernsec = 0;
static uint   g_nblk    = 0;

void DEV_ListDevices( FILE* f ) {
  fprintf( f, "ispc coding kernel, compiled in (no runtime to enumerate):\n" );
#define DEV_STR1(x) #x
#define DEV_STR(x) DEV_STR1(x)
  fprintf( f, "  -d 0  [CPU] rc_encode, RCNUM=%d lanes, %s\n",
           int(RCNUM), DEV_STR(RC_ISPC_TARGET) );
}

int DEV_Encode( const word* pbit, uint nbits, uint blksize,
                byte* rows, uint stride, uint cap,
                uint* lens, uint* carries ) {
  if( g_devopt.use==0 ) return 0;
  double t0 = tnow();
  ispc::rc_encode( pbit, nbits, blksize, rows, lens, carries, stride, cap );
  g_kernsec += tnow()-t0;
  g_nblk++;
  return 1;
}

void DEV_Report( FILE* f ) {
  if( g_nblk==0 ) return;
  fprintf( f, "ispc: %u blocks, %.3fs in rc_encode, %.0fus/block\n",
           g_nblk, g_kernsec, 1e6*g_kernsec/g_nblk );
}

#else // RC_ISPC==0: everything on the host coder

void DEV_ListDevices( FILE* f ) {
  fprintf( f, "this build has no device path (rebuild with ispc on the path)\n" );
}
int DEV_Encode( const word*, uint, uint, byte*, uint, uint, uint*, uint* ) {
  return 0;
}
void DEV_Report( FILE* ) {}

#endif // RC_ISPC
