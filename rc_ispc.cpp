
// -------------------------------------------------------------
//  rc_ispc.cpp -- the ISPC host side.
//
//  Same contract as rc_cl.cpp, same CL_* names, so model0.inc cannot tell
//  which backend it is driving. What is different is everything the contract
//  used to paper over: there is no device to open, no kernel to build at run
//  time, no queue to flush and no way for a launch to fail asynchronously --
//  rc_encode is a function in this executable, compiled by ispc at build time
//  from the same generated rc_kernel.cl the OpenCL path embeds.
//
//  The pipelining that OpenCL got from command queues comes from one worker
//  thread: CL_Submit hands a block to it and returns, so the model runs ahead
//  exactly as before; CL_Collect waits for that block and copies its rows
//  out. RC_ISPC_THREAD=0 removes the thread and runs the kernel inside
//  CL_Collect, straight into the caller's rows -- the reference arrangement,
//  and the fallback for a platform without <thread>.
//
//  With RC_ISPC=0 this file is empty; rc_cl.cpp's stubs or the OpenCL path
//  serve the same names instead.
// -------------------------------------------------------------

#define INC_FLEN
#include "common.inc"
#include "rc_config.inc"
#include "rc_cl.h"

#if RC_ISPC

#include <string.h>
#include "rc_kernel_ispc.h"

// same wall clock as rc_cl.cpp's
#include <time.h>
static double tnow( void ) {
#ifdef _WIN32
  return double(clock())/CLOCKS_PER_SEC;
#else
  timespec t; clock_gettime( CLOCK_MONOTONIC, &t );
  return double(t.tv_sec) + double(t.tv_nsec)*1e-9;
#endif
}

#ifndef RC_ISPC_THREAD
#define RC_ISPC_THREAD 1
#endif

#if RC_ISPC_THREAD
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

// what CL_Submit hands over; dst is the slot's internal rows, so pbit and the
// result arrays are the only caller memory the worker touches, and those are
// contract-pinned until CL_Collect
struct IspcJob {
  const word* pbit;
  uint nbits, blksize;
  uint *lens, *carries;
  int  queued;   // guarded by mx in the threaded build
};

static struct RcIspc {

  uint stride=0, cap=0;
  int  enabled=0, active=0;

  // one internal row set per slot, kernel-written, copied out at collect.
  // Allocated at CL_Init, when the geometry is known.
  byte* ibuf = 0;

  IspcJob job[RC_CL_NBLK];

  double kernsec = 0;
  uint   nblk = 0;

#if RC_ISPC_THREAD
  std::thread th;
  std::mutex mx;
  std::condition_variable cv_go, cv_done;
  uint q_head=0, q_tail=0, q_n=0;   // slots in submit order
  uint q_ring[RC_CL_NBLK];
  int  th_run=0;
#endif

  byte* rows( uint slot ) { return ibuf + (size_t)slot*RCNUM*stride; }

  void Run( uint slot ) {
    IspcJob& j = job[slot];
    double t0 = tnow();
    ispc::rc_encode( j.pbit, j.nbits, j.blksize, rows(slot),
                     j.lens, j.carries, stride, cap );
    kernsec += tnow()-t0;
    nblk++;
  }

#if RC_ISPC_THREAD
  void Worker( void ) {
    for(;;) {
      uint slot;
      { std::unique_lock<std::mutex> l(mx);
        cv_go.wait( l, [&]{ return q_n!=0 || !th_run; } );
        if( q_n==0 ) return;
        slot = q_ring[q_head]; q_head=(q_head+1)%RC_CL_NBLK;
      }
      Run( slot );
      { std::unique_lock<std::mutex> l(mx);
        job[slot].queued = 0; q_n--; }
      cv_done.notify_all();
    }
  }
#endif

  int Init( void ) {
    if( g_clopt.use==0 ) return active=0;
    if( !enabled ) return 0;
    if( active ) return 1;
    if( stride==0 ) return 0;
    ibuf = (byte*)malloc( (size_t)RC_CL_NBLK*RCNUM*stride );
    if( ibuf==0 ) return 0;
    for( uint i=0; i<RC_CL_NBLK; i++ ) job[i].queued = 0;
#if RC_ISPC_THREAD
    th_run = 1;
    th = std::thread( [this]{ Worker(); } );
#endif
    return active = 1;
  }

  int Submit( uint slot, const word* pbit, uint nbits, uint blksize,
              uint* lens, uint* carries ) {
    if( !active ) return 0;
    IspcJob& j = job[slot];
    j.pbit=pbit; j.nbits=nbits; j.blksize=blksize; j.lens=lens; j.carries=carries;
#if RC_ISPC_THREAD
    { std::unique_lock<std::mutex> l(mx);
      j.queued = 1;
      q_ring[q_tail] = slot; q_tail=(q_tail+1)%RC_CL_NBLK; q_n++; }
    cv_go.notify_one();
#else
    j.queued = 1;   // ran lazily, in Collect
#endif
    return 1;
  }

  int Collect( uint slot, byte* dst, uint dststride ) {
    if( !active ) return 0;
    IspcJob& j = job[slot];
#if RC_ISPC_THREAD
    { std::unique_lock<std::mutex> l(mx);
      cv_done.wait( l, [&]{ return job[slot].queued==0; } );
    }
#else
    if( j.queued ) { Run( slot ); j.queued = 0; }
#endif
    // Only the bytes each lane produced, prefix included -- the same span the
    // OpenCL path reads back. A lane that overflowed reports len > cap and
    // the caller raises rc_overflow; the copy is clamped to the row.
    for( uint i=0; i<RCNUM; i++ ) {
      uint n = RC_SKIP + Min( j.lens[i], cap );
      memcpy( dst + (size_t)i*dststride, rows(slot) + (size_t)i*stride, n );
    }
    return 1;
  }

  void Quit( void ) {
#if RC_ISPC_THREAD
    if( active ) {
      { std::unique_lock<std::mutex> l(mx); th_run = 0; }
      cv_go.notify_all();
      th.join();
    }
#endif
    if( ibuf ) { free(ibuf); ibuf=0; }
    active = 0;
  }

} g_ispc;

void CL_ListDevices( FILE* f ) {
  // shaped like the OpenCL listing so t.sh's device probe matches
  fprintf( f, "ISPC backend, compiled in (no runtime to enumerate):\n" );
  fprintf( f, "  -d 0  [CPU] ispc rc_encode, RCNUM=%d lanes%s\n",
           int(RCNUM), RC_ISPC_THREAD ? ", worker thread" : ", synchronous" );
}

void CL_Geometry( uint stride, uint cap ) { g_ispc.stride=stride; g_ispc.cap=cap; }
void CL_Enable( int on ) { g_ispc.enabled = on; }
int  CL_Active( void )   { return g_ispc.active; }
int  CL_Init( void )     { return g_ispc.Init(); }

int CL_Submit( uint slot, const word* pbit, uint nbits, uint blksize,
               uint* lens, uint* carries ) {
  return g_ispc.Submit( slot, pbit, nbits, blksize, lens, carries );
}
int CL_Collect( uint slot, byte* rows, uint stride ) {
  return g_ispc.Collect( slot, rows, stride );
}

void CL_Report( FILE* f ) {
  if( g_ispc.nblk==0 ) return;
  fprintf( f, "ispc: %u blocks, %.3fs in rc_encode, %.0fus/block\n",
           g_ispc.nblk, g_ispc.kernsec, 1e6*g_ispc.kernsec/g_ispc.nblk );
}

void CL_Quit( void ) { g_ispc.Quit(); }

#endif // RC_ISPC
