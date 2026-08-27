
// -------------------------------------------------------------
//  rc_cl.cpp -- the OpenCL host side.
//
//  What runs on the device is one thing: the encoder's carryless coding pass
//  over a block's cached {p;bit} pairs, RCNUM lanes as RCNUM work-items. That
//  is the pass sh_v1xN.inc vectorised by hand, and the only one in this coder
//  whose lanes are independent by construction.
//
//  The carry-propagating fallback stays on the host -- it is a serial
//  Cache/FF-run state machine, and it only runs on blocks the fast path had to
//  hand back. So does the decoder, and so does the model: a decoder that needs
//  a device to read a file is not a decoder.
//
//  With RC_OPENCL=0 this file is the stubs at the bottom and links against
//  nothing.
// -------------------------------------------------------------

#define INC_FLEN
#include "common.inc"
#include "rc_config.inc"
#include "rc_cl.h"

CLOpts g_clopt = {1, -1, -1, 0, 0};

#if RC_OPENCL

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

#include "rc_kernel.inc"

// -------------------------------------------------------------
// Windows has no import library to link the ICD loader against, and an
// executable that imports OpenCL.dll statically will not start at all on a
// machine that has no OpenCL installed -- which would put -C out of reach
// exactly where it is needed. So there the loader is opened by hand, and not
// finding it is one more reason to run on the host. Everywhere else these are
// the real symbols. (This is mrpc_cl.inc's arrangement, and untested here.)
// -------------------------------------------------------------
#if defined(_WIN32)||defined(_MSC_VER)

extern "C" {
typedef int(__stdcall* CL_PROC)();
__declspec(dllimport) void* __stdcall LoadLibraryA(const char*);
__declspec(dllimport) CL_PROC __stdcall GetProcAddress(void*, const char*);
}

#define CL_EACH(F)                                                          \
        F(clGetPlatformIDs) F(clGetPlatformInfo) F(clGetDeviceIDs)          \
        F(clGetDeviceInfo) F(clCreateContext) F(clCreateProgramWithSource)  \
        F(clBuildProgram) F(clGetProgramBuildInfo) F(clCreateKernel)        \
        F(clGetKernelWorkGroupInfo) F(clCreateBuffer) F(clSetKernelArg)     \
        F(clEnqueueNDRangeKernel) F(clEnqueueReadBuffer)                    \
        F(clEnqueueWriteBuffer) F(clWaitForEvents) F(clFinish)              \
        F(clGetEventProfilingInfo) F(clReleaseEvent) F(clReleaseProgram)    \
        F(clReleaseKernel) F(clReleaseMemObject) F(clReleaseCommandQueue)   \
        F(clReleaseContext)
#define CL_EACH_OPT(F) F(clCreateCommandQueueWithProperties) F(clCreateCommandQueue)

#define CL_TDEF(n) typedef decltype(&::n) t_##n;
CL_EACH(CL_TDEF)
CL_EACH_OPT(CL_TDEF)
#undef CL_TDEF
#define CL_PTR(n) static t_##n pfn_##n = 0;
CL_EACH(CL_PTR)
CL_EACH_OPT(CL_PTR)
#undef CL_PTR

static int CLLoadICD( void ) {
  if( pfn_clGetPlatformIDs ) return 1;
  void* h = LoadLibraryA("OpenCL.dll");
  if( !h ) return 0;
  int ok = 1;
#define CL_GET(n) pfn_##n = (t_##n)GetProcAddress(h,#n); if( !pfn_##n ) ok = 0;
  CL_EACH(CL_GET)
#undef CL_GET
#define CL_GETOPT(n) pfn_##n = (t_##n)GetProcAddress(h,#n);
  CL_EACH_OPT(CL_GETOPT)
#undef CL_GETOPT
  if( !pfn_clCreateCommandQueueWithProperties && !pfn_clCreateCommandQueue ) ok = 0;
  return ok;
}

#define CL_REDIR(n) n
#define clGetPlatformIDs          pfn_clGetPlatformIDs
#define clGetPlatformInfo         pfn_clGetPlatformInfo
#define clGetDeviceIDs            pfn_clGetDeviceIDs
#define clGetDeviceInfo           pfn_clGetDeviceInfo
#define clCreateContext           pfn_clCreateContext
#define clCreateProgramWithSource pfn_clCreateProgramWithSource
#define clBuildProgram            pfn_clBuildProgram
#define clGetProgramBuildInfo     pfn_clGetProgramBuildInfo
#define clCreateKernel            pfn_clCreateKernel
#define clGetKernelWorkGroupInfo  pfn_clGetKernelWorkGroupInfo
#define clCreateBuffer            pfn_clCreateBuffer
#define clSetKernelArg            pfn_clSetKernelArg
#define clEnqueueNDRangeKernel    pfn_clEnqueueNDRangeKernel
#define clEnqueueReadBuffer       pfn_clEnqueueReadBuffer
#define clEnqueueWriteBuffer      pfn_clEnqueueWriteBuffer
#define clWaitForEvents           pfn_clWaitForEvents
#define clFinish                  pfn_clFinish
#define clEnqueueMapBuffer        pfn_clEnqueueMapBuffer
#define clEnqueueUnmapMemObject   pfn_clEnqueueUnmapMemObject
#define clGetEventProfilingInfo   pfn_clGetEventProfilingInfo
#define clGetEventInfo            pfn_clGetEventInfo
#define clReleaseEvent            pfn_clReleaseEvent
#define clReleaseProgram          pfn_clReleaseProgram
#define clReleaseKernel           pfn_clReleaseKernel
#define clReleaseMemObject        pfn_clReleaseMemObject
#define clReleaseCommandQueue     pfn_clReleaseCommandQueue
#define clReleaseContext          pfn_clReleaseContext

#else
static int CLLoadICD( void ) { return 1; }
#endif

#include <time.h>
static double tnow( void ) {
#ifdef _WIN32
  return double(clock())/CLOCKS_PER_SEC;
#else
  timespec t; clock_gettime( CLOCK_MONOTONIC, &t );
  return double(t.tv_sec) + double(t.tv_nsec)*1e-9;
#endif
}

// The 2.0 queue constructor where there is one; the profiling queue -V asks
// for is the only property either takes.
static cl_command_queue CLMakeQueue( cl_context ctx, cl_device_id d, int prof, cl_int* e ) {
  const cl_queue_properties qp[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
#if defined(_WIN32)||defined(_MSC_VER)
  if( !pfn_clCreateCommandQueueWithProperties )
    return pfn_clCreateCommandQueue( ctx, d, prof ? CL_QUEUE_PROFILING_ENABLE : 0, e );
#endif
  return clCreateCommandQueueWithProperties( ctx, d, prof ? qp : 0, e );
}

static const char* CLErrStr( cl_int e ) {
  switch( e ) {
#define E(x) case x: return #x;
    E(CL_SUCCESS) E(CL_DEVICE_NOT_FOUND) E(CL_DEVICE_NOT_AVAILABLE) E(CL_COMPILER_NOT_AVAILABLE)
    E(CL_MEM_OBJECT_ALLOCATION_FAILURE) E(CL_OUT_OF_RESOURCES) E(CL_OUT_OF_HOST_MEMORY)
    E(CL_BUILD_PROGRAM_FAILURE) E(CL_INVALID_VALUE) E(CL_INVALID_DEVICE) E(CL_INVALID_CONTEXT)
    E(CL_INVALID_COMMAND_QUEUE) E(CL_INVALID_MEM_OBJECT) E(CL_INVALID_BINARY) E(CL_INVALID_PROGRAM)
    E(CL_INVALID_PROGRAM_EXECUTABLE) E(CL_INVALID_KERNEL_NAME) E(CL_INVALID_KERNEL)
    E(CL_INVALID_ARG_INDEX) E(CL_INVALID_ARG_VALUE) E(CL_INVALID_ARG_SIZE) E(CL_INVALID_KERNEL_ARGS)
    E(CL_INVALID_WORK_DIMENSION) E(CL_INVALID_WORK_GROUP_SIZE) E(CL_INVALID_WORK_ITEM_SIZE)
    E(CL_INVALID_GLOBAL_WORK_SIZE) E(CL_INVALID_BUFFER_SIZE) E(CL_INVALID_OPERATION)
#undef E
    default: return "CL_ERROR";
  }
}

static char* CLInfoStr( cl_device_id d, cl_device_info i, char* buf, size_t n ) {
  size_t got = 0;
  buf[0] = 0;
  if( clGetDeviceInfo(d,i,n-1,buf,&got)!=CL_SUCCESS ) return buf;
  buf[(got<n)?got:n-1] = 0;
  return buf;
}

static const char* CLTypeStr( cl_device_type t ) {
  if( t&CL_DEVICE_TYPE_CPU ) return "CPU";
  if( t&CL_DEVICE_TYPE_GPU ) return "GPU";
  if( t&CL_DEVICE_TYPE_ACCELERATOR ) return "ACC";
  return "OTHER";
}

// Every (platform, device) pair, in the order -d counts them. -p narrows the
// enumeration, so with -p the index is within that platform.
struct CLDevList {
  static const int MAXD = 64;
  cl_platform_id plat[MAXD];
  cl_device_id   dev[MAXD];
  int n;

  void Enumerate( int want_plat ) {
    n = 0;
    cl_uint np = 0;
    if( !CLLoadICD() || clGetPlatformIDs(0,0,&np)!=CL_SUCCESS || np==0 ) return;
    cl_platform_id* ps = new cl_platform_id[np];
    clGetPlatformIDs( np, ps, 0 );
    for( cl_uint i=0; i<np; i++ ) {
      if( want_plat>=0 && int(i)!=want_plat ) continue;
      cl_uint nd = 0;
      if( clGetDeviceIDs(ps[i],CL_DEVICE_TYPE_ALL,0,0,&nd)!=CL_SUCCESS || nd==0 ) continue;
      cl_device_id* ds = new cl_device_id[nd];
      clGetDeviceIDs( ps[i], CL_DEVICE_TYPE_ALL, nd, ds, 0 );
      for( cl_uint j=0; j<nd && n<MAXD; j++ ) { plat[n]=ps[i]; dev[n]=ds[j]; n++; }
      delete[] ds;
    }
    delete[] ps;
  }
};

void CL_ListDevices( FILE* f ) {
  cl_uint np = 0;
  if( !CLLoadICD() || clGetPlatformIDs(0,0,&np)!=CL_SUCCESS || np==0 ) {
    fprintf( f, "no OpenCL platforms found\n"
                "  (an ICD loader and a runtime have to be installed; on Debian/Ubuntu\n"
                "   that is ocl-icd-libopencl1 plus a driver -- intel-oneapi-runtime-opencl\n"
                "   for the CPU, or the vendor package for a GPU)\n" );
    return;
  }
  cl_platform_id* ps = new cl_platform_id[np];
  clGetPlatformIDs( np, ps, 0 );
  char buf[1024];
  int idx = 0;
  fprintf( f, "OpenCL devices (-d takes the index, -p the platform):\n" );
  for( cl_uint i=0; i<np; i++ ) {
    size_t got = 0; buf[0] = 0;
    clGetPlatformInfo( ps[i], CL_PLATFORM_NAME, sizeof(buf)-1, buf, &got );
    buf[(got<sizeof(buf))?got:sizeof(buf)-1] = 0;
    fprintf( f, "\n  platform %d: %s\n", int(i), buf );
    cl_uint nd = 0;
    if( clGetDeviceIDs(ps[i],CL_DEVICE_TYPE_ALL,0,0,&nd)!=CL_SUCCESS || nd==0 ) {
      fprintf( f, "    (no devices)\n" );
      continue;
    }
    cl_device_id* ds = new cl_device_id[nd];
    clGetDeviceIDs( ps[i], CL_DEVICE_TYPE_ALL, nd, ds, 0 );
    for( cl_uint j=0; j<nd; j++, idx++ ) {
      cl_device_type t = 0;
      cl_uint cu = 0, mhz = 0, vw = 0;
      cl_ulong gm = 0;
      size_t wg = 0;
      clGetDeviceInfo( ds[j], CL_DEVICE_TYPE, sizeof(t), &t, 0 );
      clGetDeviceInfo( ds[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, 0 );
      clGetDeviceInfo( ds[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(mhz), &mhz, 0 );
      clGetDeviceInfo( ds[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(gm), &gm, 0 );
      clGetDeviceInfo( ds[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wg), &wg, 0 );
      clGetDeviceInfo( ds[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(vw), &vw, 0 );
      fprintf( f, "    -d %-2d [%s] %s\n", idx, CLTypeStr(t),
               CLInfoStr(ds[j],CL_DEVICE_NAME,buf,sizeof(buf)) );
      fprintf( f, "          %u units @ %u MHz, %.1f GiB global, max wg %u, int vec %u\n",
               cu, mhz, double(gm)/(1024.0*1024.0*1024.0), unsigned(wg), vw );
      fprintf( f, "          %s / %s\n",
               CLInfoStr(ds[j],CL_DEVICE_VERSION,buf,sizeof(buf)),
               CLInfoStr(ds[j],CL_DRIVER_VERSION,buf,sizeof(buf)) );
    }
    delete[] ds;
  }
  delete[] ps;
}

// -------------------------------------------------------------
struct RcCL {
  int enabled;      // the encoder asked for it
  int active;       // and it is live
  int tried;        // Init has run
  int verbose;

  cl_device_id   device;
  cl_platform_id platform;
  cl_context     ctx;
  // Two queues. Submits go on q, in order, so the launches run in the order
  // the blocks were modelled. The payload readbacks go on q2, because they are
  // enqueued at collect time -- behind everything already queued on q -- and
  // waiting for them on q would wait for every block still in flight, which is
  // the pipeline undone.
  cl_command_queue q, q2;
  cl_program     prog;

  // One kernel object and one set of buffers per slot: the arguments differ
  // per launch, and a slot's are live from its submit until its collect.
  cl_kernel k_enc[RC_CL_NBLK];
  cl_mem d_pbit[RC_CL_NBLK], d_out[RC_CL_NBLK], d_len[RC_CL_NBLK], d_carry[RC_CL_NBLK];
  cl_event ev_done[RC_CL_NBLK];   // the last read of that slot's launch
  cl_event ev_kern[RC_CL_NBLK];   // -V only
  uint*    h_len[RC_CL_NBLK];     // where that slot's lengths are landing

  // The substream buffer rows the kernel writes, matching RCio::tmpbuf.
  // Filled in at Init from what the model passes to CL_EncodeBlock.
  uint stride, cap;

  size_t lws;       // work-group size, see PickLWS

  double kms;    // device time in the kernel
  double sms;    // wall clock the host spends in CL_Submit
  double cms;    // ... and in CL_Collect, which is where it waits
  long   nlaunch;
  long   nready;   // collects that found the block already coded -- how much
                   // of the device time the pipeline actually hid
  double buildsec;

  int Fail( const char* what, cl_int e ) {
    fprintf( stderr, "\ncoder: opencl: %s: %s\n", what, CLErrStr(e) );
    if( active ) fprintf( stderr, "coder: opencl: falling back to the host coder\n" );
    active = 0;
    return 0;
  }

  int Open( void ) {
    CLDevList l;
    l.Enumerate( g_clopt.plat );
    if( l.n==0 ) {
      fprintf( stderr, "coder: opencl: no devices (-l lists them, -C disables this)\n" );
      return 0;
    }
    int pick = -1;
    if( g_clopt.dev>=0 ) {
      if( g_clopt.dev>=l.n ) {
        fprintf( stderr, "coder: opencl: -d %d: only %d device%s (-l lists them)\n",
                 g_clopt.dev, l.n, l.n==1?"":"s" );
        return 0;
      }
      pick = g_clopt.dev;
    } else {
      static const cl_device_type order[4][3] = {
        { CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_ACCELERATOR, CL_DEVICE_TYPE_CPU },
        { CL_DEVICE_TYPE_CPU, 0, 0 },
        { CL_DEVICE_TYPE_GPU, 0, 0 },
        { CL_DEVICE_TYPE_ACCELERATOR, 0, 0 } };
      for( int r=0; r<3 && pick<0; r++ ) {
        cl_device_type want = order[g_clopt.type][r];
        if( !want ) break;
        for( int i=0; i<l.n; i++ ) {
          cl_device_type t = 0;
          clGetDeviceInfo( l.dev[i], CL_DEVICE_TYPE, sizeof(t), &t, 0 );
          if( t&want ) { pick = i; break; }
        }
      }
      if( pick<0 ) {
        fprintf( stderr, "coder: opencl: no device of that type (-l lists them)\n" );
        return 0;
      }
    }
    platform = l.plat[pick];
    device   = l.dev[pick];

    cl_int e = CL_SUCCESS;

    cl_context_properties props[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
    ctx = clCreateContext( props, 1, &device, 0, 0, &e );
    if( e!=CL_SUCCESS ) return Fail( "clCreateContext", e );
    q = CLMakeQueue( ctx, device, verbose, &e );
    if( e!=CL_SUCCESS ) return Fail( "clCreateCommandQueue", e );
    q2 = CLMakeQueue( ctx, device, 0, &e );
    if( e!=CL_SUCCESS ) return Fail( "clCreateCommandQueue", e );
    return 1;
  }

  void Describe( char* buf, size_t n ) const {
    char d[512], p[512];
    size_t got = 0; p[0] = 0;
    clGetPlatformInfo( platform, CL_PLATFORM_NAME, sizeof(p)-1, p, &got );
    p[(got<sizeof(p))?got:sizeof(p)-1] = 0;
    cl_device_type t = 0;
    clGetDeviceInfo( device, CL_DEVICE_TYPE, sizeof(t), &t, 0 );
    snprintf( buf, n, "[%s] %s (%s)", CLTypeStr(t),
              CLInfoStr(device,CL_DEVICE_NAME,d,sizeof(d)), p );
  }

  int Program( void ) {
    // Everything the kernel needs is a compile-time constant on this side
    // too, so there is nothing to pass per launch except the block itself.
    char opts[1024];
    snprintf( opts, sizeof(opts),
              "-cl-std=CL1.2"
              " -D RCNUM=%d -D SCALElog=%d -D hSCALE=%d"
              " -D LOWBYTES=%d -D CODBYTES=%d -D BLKFULL=%u"
              " -D OUTSTRIDE=%u -D OUTCAP=%u"
              " -D RC_RANGE64=%d -D RC_RENORM_TAIL=%d",
              int(RCNUM), int(SCALElog), int(hSCALE),
              int(RC_LOWBYTES), 4, unsigned(BLKSIZE),
              unsigned(stride), unsigned(cap),
              int(RC_RANGE64), int(RC_RENORM_TAIL) );

    cl_int e = CL_SUCCESS;
    double t0 = tnow();
    prog = clCreateProgramWithSource( ctx, 1, &RC_CL_SRC, 0, &e );
    if( e!=CL_SUCCESS ) return Fail( "clCreateProgramWithSource", e );
    e = clBuildProgram( prog, 1, &device, opts, 0, 0 );
    buildsec = tnow()-t0;

    if( e!=CL_SUCCESS || verbose ) {
      size_t n = 0;
      clGetProgramBuildInfo( prog, device, CL_PROGRAM_BUILD_LOG, 0, 0, &n );
      if( n>1 ) {
        char* log = new char[n+1];
        clGetProgramBuildInfo( prog, device, CL_PROGRAM_BUILD_LOG, n, log, 0 );
        log[n] = 0;
        fprintf( stderr, "coder: opencl: build log:\n%s\n", log );
        delete[] log;
      }
    }
    if( e!=CL_SUCCESS ) return Fail( "clBuildProgram", e );

    for( uint i=0; i<RC_CL_NBLK; i++ ) {
      k_enc[i] = clCreateKernel( prog, "rc_encode", &e );
      if( e!=CL_SUCCESS ) return Fail( "rc_encode", e );
    }
    return 1;
  }

  // A work-group is vectorised across its work-items and scheduled on one
  // compute unit, so the group size decides how the RCNUM lanes are split
  // between vector width and cores. Measured on a 4-core CPU device at
  // RCNUM=16, encoding a 7 MB file: one group of 16 is 1.7x the host coder,
  // four groups of 4 is 3.0x. Cores win, because the lanes' byte writes are a
  // scatter -- 16 rows OUTSTRIDE apart -- and widening that does not help.
  //
  // So the default is one group per compute unit, rounded down to something
  // that divides RCNUM. RC_CL_LWS overrides it; 0 here would mean letting the
  // runtime choose, which it does worse.
  void PickLWS( void ) {
    lws = RC_CL_LWS;
    if( lws!=0 ) return;
    cl_uint cu = 1;
    clGetDeviceInfo( device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, 0 );
    if( cu<1 ) cu = 1;
    lws = RCNUM/cu;
    if( lws<1 ) lws = 1;
    while( lws>1 && (RCNUM%lws)!=0 ) lws--;
    size_t mx = 0;
    if( clGetKernelWorkGroupInfo( k_enc[0], device, CL_KERNEL_WORK_GROUP_SIZE,
                                  sizeof(mx), &mx, 0 )==CL_SUCCESS && mx>0 )
      while( lws>mx ) lws >>= 1;
  }

  int Buffers( void ) {
    cl_int e = CL_SUCCESS;
    for( uint i=0; i<RC_CL_NBLK; i++ ) {
      d_pbit[i] = clCreateBuffer( ctx, CL_MEM_READ_ONLY, size_t(BLKSIZE)*8*sizeof(word), 0, &e );
      if( e!=CL_SUCCESS ) return Fail( "buffer pbit", e );
      d_out[i] = clCreateBuffer( ctx, CL_MEM_WRITE_ONLY, size_t(RCNUM)*stride, 0, &e );
      if( e!=CL_SUCCESS ) return Fail( "buffer out", e );
      d_len[i] = clCreateBuffer( ctx, CL_MEM_WRITE_ONLY, size_t(RCNUM)*sizeof(uint), 0, &e );
      if( e!=CL_SUCCESS ) return Fail( "buffer len", e );
      d_carry[i] = clCreateBuffer( ctx, CL_MEM_WRITE_ONLY, size_t(RCNUM)*sizeof(uint), 0, &e );
      if( e!=CL_SUCCESS ) return Fail( "buffer carry", e );
    }
    return 1;
  }

  int Init( uint stride_, uint cap_ ) {
    if( tried ) return active;
    tried = 1;
    verbose = g_clopt.verbose;
    stride = stride_; cap = cap_;
    if( !enabled || !g_clopt.use ) return 0;
#if RC_FORCE_CARRY
    return 0;                       // no kernel for the carry twin
#endif
    if( !Open() ) return 0;
    active = 1;                       // so Fail() reports the fallback
    if( !Program() ) return 0;
    if( !Buffers() ) return 0;
    PickLWS();
    if( verbose ) {
      char d[768]; Describe( d, sizeof(d) );
      fprintf( stderr, "coder: opencl: %s\n", d );
      fprintf( stderr, "coder: opencl: kernel built in %.2fs, %d lanes per launch, "
                       "work-group %d, %d block%s in flight\n",
               buildsec, int(RCNUM), int(lws),
               int(RC_CL_NBLK), RC_CL_NBLK==1?"":"s" );
    }
    return active;
  }

  // Queue the whole of one block: the {p;bit} upload, the launch, and the
  // lengths and carry counts coming back. Nothing here blocks, so the host is
  // free to model the next block while this one codes.
  int Submit( uint slot, const word* pbit, uint nbits, uint blksize,
              uint* lens, uint* carries ) {
    if( !active ) return 0;
    double w0 = tnow();
    cl_int e;

    // pbit is the only thing that crosses per block: nbits packed pairs, two
    // bytes each. Sending only the used prefix rather than the whole array is
    // what keeps that near the size of the block on a short one.
    e = clEnqueueWriteBuffer( q, d_pbit[slot], CL_FALSE, 0,
                              size_t(nbits)*sizeof(word), pbit, 0, 0, 0 );
    if( e!=CL_SUCCESS ) return Fail( "write pbit", e );

    cl_kernel k = k_enc[slot];
    uint a = 0;
    if( (e=clSetKernelArg(k, a++, sizeof(cl_mem), &d_pbit[slot] ))!=CL_SUCCESS ) return Fail( "arg pbit", e );
    if( (e=clSetKernelArg(k, a++, sizeof(uint),   &nbits        ))!=CL_SUCCESS ) return Fail( "arg nbits", e );
    if( (e=clSetKernelArg(k, a++, sizeof(uint),   &blksize      ))!=CL_SUCCESS ) return Fail( "arg blksize", e );
    if( (e=clSetKernelArg(k, a++, sizeof(cl_mem), &d_out[slot]  ))!=CL_SUCCESS ) return Fail( "arg out", e );
    if( (e=clSetKernelArg(k, a++, sizeof(cl_mem), &d_len[slot]  ))!=CL_SUCCESS ) return Fail( "arg len", e );
    if( (e=clSetKernelArg(k, a++, sizeof(cl_mem), &d_carry[slot]))!=CL_SUCCESS ) return Fail( "arg carry", e );

    const size_t gws = RCNUM;
    e = clEnqueueNDRangeKernel( q, k, 1, 0, &gws, lws?&lws:0, 0, 0,
                                verbose ? &ev_kern[slot] : 0 );
    if( e!=CL_SUCCESS ) return Fail( "rc_encode", e );
    nlaunch++;

    e = clEnqueueReadBuffer( q, d_len[slot], CL_FALSE, 0, size_t(RCNUM)*sizeof(uint),
                             lens, 0, 0, 0 );
    if( e!=CL_SUCCESS ) return Fail( "read len", e );
    e = clEnqueueReadBuffer( q, d_carry[slot], CL_FALSE, 0, size_t(RCNUM)*sizeof(uint),
                             carries, 0, 0, &ev_done[slot] );
    if( e!=CL_SUCCESS ) return Fail( "read carry", e );

    // Without this the runtime is free to sit on the queue until something
    // blocks on it, which is the whole pipeline not happening.
    if( (e=clFlush(q))!=CL_SUCCESS ) return Fail( "clFlush", e );

    h_len[slot] = lens;
    sms += (tnow()-w0)*1000.0;
    return 1;
  }

  // Wait for a slot's launch and bring its substreams back. Only the bytes
  // each lane actually produced are read, so the readback is the size of the
  // compressed block and not of the buffer. A lane that overflowed is left for
  // the caller to notice: its payload is truncated and unusable either way.
  int Collect( uint slot, byte* rows, uint stride_ ) {
    if( !active ) return 0;
    double w0 = tnow();
    cl_int e;

    if( ev_done[slot] ) {
      cl_int st = 0;
      if( clGetEventInfo( ev_done[slot], CL_EVENT_COMMAND_EXECUTION_STATUS,
                          sizeof(st), &st, 0 )==CL_SUCCESS && st==CL_COMPLETE )
        nready++;
      e = clWaitForEvents( 1, &ev_done[slot] );
      clReleaseEvent( ev_done[slot] );
      ev_done[slot] = 0;
      if( e!=CL_SUCCESS ) { if( ev_kern[slot] ) { clReleaseEvent(ev_kern[slot]); ev_kern[slot]=0; } return Fail( "wait", e ); }
    }
    if( verbose && ev_kern[slot] ) {
      cl_ulong t0=0, t1=0;
      clGetEventProfilingInfo( ev_kern[slot], CL_PROFILING_COMMAND_START, sizeof(t0), &t0, 0 );
      clGetEventProfilingInfo( ev_kern[slot], CL_PROFILING_COMMAND_END,   sizeof(t1), &t1, 0 );
      kms += double(t1-t0)/1e6;
      clReleaseEvent( ev_kern[slot] );
      ev_kern[slot] = 0;
    }

    const uint* lens = h_len[slot];
    uint novf = 0;
    for( uint i=0; i<RCNUM; i++ ) novf += (lens[i]>cap);
    if( novf==0 ) {
      // One map rather than RCNUM reads. Only a few kilobytes of each row are
      // used, so a read per lane is all enqueue overhead -- and there are
      // RCNUM of them per block. Mapping the whole thing once costs two
      // commands, and on a CPU device the map itself is free.
      void* p = clEnqueueMapBuffer( q2, d_out[slot], CL_TRUE, CL_MAP_READ, 0,
                                    size_t(RCNUM)*stride_, 0, 0, 0, &e );
      if( e!=CL_SUCCESS ) return Fail( "map out", e );
      for( uint i=0; i<RCNUM; i++ )
        if( lens[i] ) memcpy( rows+size_t(i)*stride_, (byte*)p+size_t(i)*stride_, lens[i] );
      e = clEnqueueUnmapMemObject( q2, d_out[slot], p, 0, 0, 0 );
      if( e!=CL_SUCCESS ) return Fail( "unmap out", e );
      if( (e=clFinish(q2))!=CL_SUCCESS ) return Fail( "clFinish", e );
    }

    cms += (tnow()-w0)*1000.0;
    return 1;
  }

  void Report( FILE* f ) {
    if( !verbose || !nlaunch ) return;
    fprintf( f, "coder: opencl: rc_encode %8.2f ms device over %ld launches, "
                "%.1f us each\n", kms, nlaunch, kms*1000.0/double(nlaunch) );
    fprintf( f, "coder: opencl: host %8.2f ms queueing, %8.2f ms collecting; "
                "%ld of %ld blocks were already done when collected\n",
             sms, cms, nready, nlaunch );
  }

  void Quit( void ) {
    if( !tried ) return;
    for( uint i=0; i<RC_CL_NBLK; i++ ) {
      if( ev_done[i] ) { clWaitForEvents(1,&ev_done[i]); clReleaseEvent(ev_done[i]); ev_done[i]=0; }
      if( ev_kern[i] ) { clReleaseEvent(ev_kern[i]); ev_kern[i]=0; }
      if( d_pbit[i] )  clReleaseMemObject( d_pbit[i] );
      if( d_out[i] )   clReleaseMemObject( d_out[i] );
      if( d_len[i] )   clReleaseMemObject( d_len[i] );
      if( d_carry[i] ) clReleaseMemObject( d_carry[i] );
      if( k_enc[i] )   clReleaseKernel( k_enc[i] );
      d_pbit[i]=d_out[i]=d_len[i]=d_carry[i]=0; k_enc[i]=0;
    }
    if( prog ) clReleaseProgram( prog );
    if( q )    clReleaseCommandQueue( q );
    if( q2 )   clReleaseCommandQueue( q2 );
    if( ctx )  clReleaseContext( ctx );
    prog=0; q=0; q2=0; ctx=0;
    active = 0; tried = 0;
  }
};

static RcCL g_cl;

void CL_Enable( int on ) { g_cl.enabled = on; }
int  CL_Active( void )   { return g_cl.active; }
void CL_Report( FILE* f ){ g_cl.Report(f); }
void CL_Quit( void )     { g_cl.Quit(); }

// The row geometry of RCio::tmpbuf. rc_cl.cpp cannot see the template, so the
// caller hands it over on the first call; it is a compile-time constant there.
static uint g_stride = 0, g_cap = 0;
void CL_Geometry( uint stride, uint cap ) { g_stride = stride; g_cap = cap; }

int CL_Init( void ) { return g_cl.Init( g_stride, g_cap ); }

int CL_Submit( uint slot, const word* pbit, uint nbits, uint blksize,
               uint* lens, uint* carries ) {
  return g_cl.Submit( slot, pbit, nbits, blksize, lens, carries );
}

int CL_Collect( uint slot, byte* rows, uint stride ) {
  return g_cl.Collect( slot, rows, stride );
}

#else  // RC_OPENCL==0

void CL_ListDevices( FILE* f ) {
  fprintf( f, "this build has no OpenCL support (rebuild with RC_OPENCL=1)\n" );
}
void CL_Enable( int ) {}
int  CL_Active( void ) { return 0; }
int  CL_Init( void ) { return 0; }
void CL_Geometry( uint, uint ) {}
int  CL_Submit( uint, const word*, uint, uint, uint*, uint* ) { return 0; }
int  CL_Collect( uint, byte*, uint ) { return 0; }
void CL_Report( FILE* ) {}
void CL_Quit( void ) {}

#endif
