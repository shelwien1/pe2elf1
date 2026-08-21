// -------------------------------------------------------------
// mrpc_cl.inc -- the OpenCL host side.
//
// What runs on the device is the encoder's optimisation loop, which is
// where all of the time goes and none of the bitstream is: the class
// search, the coefficient sweep and the group histogram.  The rangecoder,
// the quadtree bookkeeping and the whole decoder stay on the host -- they
// are serial by construction, and a decoder that needs a GPU to read a
// file is not a decoder.
//
// Nothing here is required.  With -C, or with no usable device, every
// entry point below reports failure and the caller runs the reference
// code path, which is still there and still what the results are checked
// against.
// -------------------------------------------------------------

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#include "mrpc_kernels.inc"

// -------------------------------------------------------------
// Windows has no import library to link the ICD loader against, and an
// executable that imports OpenCL.dll statically will not start at all on
// a machine that has no OpenCL installed -- which would make -C
// unreachable exactly where it is needed.  So on Windows the loader is
// opened by hand, and failing to find it is one more reason to run on the
// host.  Everywhere else these are the real symbols.
// -------------------------------------------------------------
#if defined(_WIN32)||defined(_MSC_VER)

extern "C" {
typedef int(__stdcall* CL_PROC)();
__declspec(dllimport) void* __stdcall LoadLibraryA(const char*);
__declspec(dllimport) CL_PROC __stdcall GetProcAddress(void*, const char*);
}

// every entry point this file uses; the queue constructors are the only
// optional ones, and one of the two has to be there
#define CL_EACH(F)                                                         \
        F(clGetPlatformIDs) F(clGetPlatformInfo) F(clGetDeviceIDs)         \
        F(clGetDeviceInfo) F(clCreateContext) F(clCreateProgramWithSource) \
        F(clBuildProgram) F(clGetProgramBuildInfo) F(clCreateKernel)       \
        F(clCreateBuffer) F(clSetKernelArg) F(clEnqueueNDRangeKernel)      \
        F(clEnqueueReadBuffer) F(clEnqueueWriteBuffer) F(clWaitForEvents)  \
        F(clGetEventProfilingInfo) F(clReleaseEvent)
#define CL_EACH_OPT(F) F(clCreateCommandQueueWithProperties) F(clCreateCommandQueue)

#define CL_TDEF(n) typedef decltype(&::n) t_##n;
CL_EACH(CL_TDEF)
CL_EACH_OPT(CL_TDEF)
#undef CL_TDEF
#define CL_PTR(n) static t_##n pfn_##n = 0;
CL_EACH(CL_PTR)
CL_EACH_OPT(CL_PTR)
#undef CL_PTR

static int CLLoadICD(void) {
  if( pfn_clGetPlatformIDs )
    return 1;
  void* h = LoadLibraryA("OpenCL.dll");
  if( !h )
    return 0;
  int ok = 1;
#define CL_GET(n)                                       \
        pfn_##n = (t_##n)GetProcAddress(h, #n);         \
        if( !pfn_##n )                                  \
          ok = 0;
  CL_EACH(CL_GET)
#undef CL_GET
#define CL_GETOPT(n) pfn_##n = (t_##n)GetProcAddress(h, #n);
  CL_EACH_OPT(CL_GETOPT)
#undef CL_GETOPT
  if( !pfn_clCreateCommandQueueWithProperties&&!pfn_clCreateCommandQueue )
    ok = 0;
  return ok;
}

#define clGetPlatformIDs             pfn_clGetPlatformIDs
#define clGetPlatformInfo            pfn_clGetPlatformInfo
#define clGetDeviceIDs               pfn_clGetDeviceIDs
#define clGetDeviceInfo              pfn_clGetDeviceInfo
#define clCreateContext              pfn_clCreateContext
#define clCreateProgramWithSource    pfn_clCreateProgramWithSource
#define clBuildProgram               pfn_clBuildProgram
#define clGetProgramBuildInfo        pfn_clGetProgramBuildInfo
#define clCreateKernel               pfn_clCreateKernel
#define clCreateBuffer               pfn_clCreateBuffer
#define clSetKernelArg               pfn_clSetKernelArg
#define clEnqueueNDRangeKernel       pfn_clEnqueueNDRangeKernel
#define clEnqueueReadBuffer          pfn_clEnqueueReadBuffer
#define clEnqueueWriteBuffer         pfn_clEnqueueWriteBuffer
#define clWaitForEvents              pfn_clWaitForEvents
#define clGetEventProfilingInfo      pfn_clGetEventProfilingInfo
#define clReleaseEvent               pfn_clReleaseEvent

#else
static int CLLoadICD(void) {
  return 1;
}
#endif

// The 2.0 constructor where there is one, the 1.2 one where there is not:
// the profiling queue -V asks for is the only property either takes.
static cl_command_queue CLMakeQueue(cl_context ctx, cl_device_id d, int prof, cl_int* e) {
#if defined(_WIN32)||defined(_MSC_VER)
  if( !pfn_clCreateCommandQueueWithProperties ) {
    return pfn_clCreateCommandQueue(ctx, d, prof ? CL_QUEUE_PROFILING_ENABLE : 0, e);
  }
  const cl_queue_properties qp[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
  return pfn_clCreateCommandQueueWithProperties(ctx, d, prof ? qp : 0, e);
#else
  const cl_queue_properties qp[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
  return clCreateCommandQueueWithProperties(ctx, d, prof ? qp : 0, e);
#endif
}

// the 11x3 candidate grid of OptimizeCoef, as the kernel sees it
static const int CL_SR = 11, CL_SSR = 3, CL_NCAND = CL_SR*CL_SSR;
// classes batched per work-item in predict_cube; must match CLGRP there
static const int CL_CLGRP = 8;

// the activity neighbourhood, as a tile: how far it reaches from a pixel
static const int HT_T = UPEL_DIST, HT_L = UPEL_DIST, HT_R = 2;
static const int TWMAX = MAX_BSIZE+HT_L+HT_R;
static const int THMAX = MAX_BSIZE+HT_T;

// The cube kernels want the block size as a constant -- with it passed in
// as an argument the device compiler cannot prove that neighbouring
// work-items touch neighbouring memory, and every load becomes a gather,
// which measured 4x slower.  So the program is built per block size, and
// the size is an index.  Only MAX_BSIZE is built: the first optimisation
// loop's BASE_BSIZE blocks stay on the host, because 8x8 pixels is 40
// microseconds of arithmetic behind two launches and a readback, and
// measuring it that way came out a shade slower than just doing it.
static const int CL_NBS = 1;
static const int CL_BSZ[2] = {MAX_BSIZE, BASE_BSIZE};
static const int CL_TW[2] = {MAX_BSIZE+HT_L+HT_R, BASE_BSIZE+HT_L+HT_R};
static const int CL_TH[2] = {MAX_BSIZE+HT_T, BASE_BSIZE+HT_T};

struct CLOpts {
  int use;     // 0 = -C, run everything on the host
  int plat;    // -p, -1 = any
  int dev;     // -d, -1 = pick one
  int type;    // -T: 0 any (prefer GPU), 1 cpu, 2 gpu, 3 accelerator
  int verbose; // -V
};

static CLOpts g_clopt = {1, -1, -1, 0, 0};

static const char* CLErrStr(cl_int e) {
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
    default:
      return "CL_ERROR";
  }
}

static char* CLInfoStr(cl_device_id d, cl_device_info i, char* buf, size_t n) {
  size_t got = 0;
  buf[0] = 0;
  if( clGetDeviceInfo(d, i, n-1, buf, &got)!=CL_SUCCESS )
    return buf;
  buf[(got<n) ? got : n-1] = 0;
  return buf;
}

static const char* CLTypeStr(cl_device_type t) {
  if( t&CL_DEVICE_TYPE_CPU )
    return "CPU";
  if( t&CL_DEVICE_TYPE_GPU )
    return "GPU";
  if( t&CL_DEVICE_TYPE_ACCELERATOR )
    return "ACC";
  return "OTHER";
}

// Every (platform, device) pair, in the order -d counts them.  -p narrows
// the enumeration, so with -p the index is within that platform.
struct CLDevList {
  static const int MAXD = 64;
  cl_platform_id plat[MAXD];
  cl_device_id dev[MAXD];
  int n;

  void Enumerate(int want_plat) {
    n = 0;
    cl_uint np = 0;
    if( !CLLoadICD()||clGetPlatformIDs(0, 0, &np)!=CL_SUCCESS||np==0 )
      return;
    cl_platform_id* ps = new cl_platform_id[np];
    clGetPlatformIDs(np, ps, 0);
    for( cl_uint i = 0; i<np; i++ ) {
      if( want_plat>=0&&int(i)!=want_plat )
        continue;
      cl_uint nd = 0;
      if( clGetDeviceIDs(ps[i], CL_DEVICE_TYPE_ALL, 0, 0, &nd)!=CL_SUCCESS||nd==0 )
        continue;
      cl_device_id* ds = new cl_device_id[nd];
      clGetDeviceIDs(ps[i], CL_DEVICE_TYPE_ALL, nd, ds, 0);
      for( cl_uint j = 0; j<nd&&n<MAXD; j++ ) {
        plat[n] = ps[i];
        dev[n] = ds[j];
        n++;
      }
      delete[] ds;
    }
    delete[] ps;
  }
};

static void CLListDevices(FILE* f) {
  cl_uint np = 0;
  if( !CLLoadICD()||clGetPlatformIDs(0, 0, &np)!=CL_SUCCESS||np==0 ) {
#if defined(_WIN32)||defined(_MSC_VER)
    fprintf(f, "no OpenCL platforms found\n"
               "  (OpenCL.dll and a runtime have to be installed; a GPU driver brings\n"
               "   both, and Intel's CPU runtime -- 'Intel CPU Runtime for OpenCL\n"
               "   Applications' -- adds a device that does not need one)\n");
#else
    fprintf(f, "no OpenCL platforms found\n"
               "  (an ICD loader and a runtime have to be installed; on Debian/Ubuntu\n"
               "   that is ocl-icd-libopencl1 plus a driver -- intel-oneapi-runtime-opencl\n"
               "   for the CPU, mesa-opencl-icd or the vendor package for a GPU)\n");
#endif
    return;
  }
  cl_platform_id* ps = new cl_platform_id[np];
  clGetPlatformIDs(np, ps, 0);
  char buf[1024];
  int idx = 0;
  fprintf(f, "OpenCL devices (-d takes the index, -p the platform):\n");
  for( cl_uint i = 0; i<np; i++ ) {
    size_t got = 0;
    buf[0] = 0;
    clGetPlatformInfo(ps[i], CL_PLATFORM_NAME, sizeof(buf)-1, buf, &got);
    buf[(got<sizeof(buf)) ? got : sizeof(buf)-1] = 0;
    fprintf(f, "\n  platform %d: %s\n", int(i), buf);
    cl_uint nd = 0;
    if( clGetDeviceIDs(ps[i], CL_DEVICE_TYPE_ALL, 0, 0, &nd)!=CL_SUCCESS||nd==0 ) {
      fprintf(f, "    (no devices)\n");
      continue;
    }
    cl_device_id* ds = new cl_device_id[nd];
    clGetDeviceIDs(ps[i], CL_DEVICE_TYPE_ALL, nd, ds, 0);
    for( cl_uint j = 0; j<nd; j++, idx++ ) {
      cl_device_type t = 0;
      cl_uint cu = 0, mhz = 0;
      cl_ulong gm = 0, lm = 0;
      size_t wg = 0;
      clGetDeviceInfo(ds[j], CL_DEVICE_TYPE, sizeof(t), &t, 0);
      clGetDeviceInfo(ds[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, 0);
      clGetDeviceInfo(ds[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(mhz), &mhz, 0);
      clGetDeviceInfo(ds[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(gm), &gm, 0);
      clGetDeviceInfo(ds[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(lm), &lm, 0);
      clGetDeviceInfo(ds[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(wg), &wg, 0);
      fprintf(f, "    -d %-2d [%s] %s\n", idx, CLTypeStr(t),
              CLInfoStr(ds[j], CL_DEVICE_NAME, buf, sizeof(buf)));
      fprintf(f, "          %u units @ %u MHz, %.1f GiB global, %llu KiB local, max wg %u\n",
              cu, mhz, double(gm)/(1024.0*1024.0*1024.0),
              (unsigned long long)(lm/1024), unsigned(wg));
      fprintf(f, "          %s / %s\n", CLInfoStr(ds[j], CL_DEVICE_VERSION, buf, sizeof(buf)),
              CLInfoStr(ds[j], CL_DRIVER_VERSION, buf, sizeof(buf)));
    }
    delete[] ds;
  }
  delete[] ps;
}

// -------------------------------------------------------------
#define CLCK(call, what)                        \
        do {                                    \
          cl_int e_ = (call);                   \
          if( e_!=CL_SUCCESS )                  \
            return Fail(what, e_);              \
        } while( 0 )

struct MrpcCL {
  int active;
  int verbose;
  cl_platform_id platform;
  cl_device_id device;
  cl_context ctx;
  cl_command_queue q;
  cl_program prog[2];
  cl_kernel k_predict[2], k_cost[2];
  cl_kernel k_scan, k_apply, k_hist, k_pmod, k_dp;
  size_t scan_workers, pm_chunks;
  cl_ulong lmem;
  // -V accounting: device time per kernel, and how often it was asked
  double kms[8];
  long kn[8];
  static const char* KName(int i) {
    static const char* n[8] = {"predict_cube", "pixel_cost", "coef_scan", "coef_apply", "group_hist", "pmodel_cost", "group_dp", ""};
    return n[i];
  }

  uint W, H, nc, BS, num_class;
  size_t bn; // plane size, elements
  int ntpad;

  cl_mem d_planes, d_org, d_prd, d_err, d_grp, d_cpo, d_cpq;
  cl_mem d_coef, d_tapoff, d_uq, d_costtab, d_pmoff;
  cl_mem d_udy, d_udx, d_uw, d_cA, d_cB, d_cbeg;
  cl_mem d_halo, d_prdcube, d_errcube, d_pixcost, d_partials, d_hist, d_pmcost;
  cl_mem d_cum, d_dp, d_tre, d_th, d_thcost;

  // host staging
  short* halo;
  float* pixcost;
  float* partials;
  double* pmcost;
  double* psum; // [class][(bh+1)*(bw+1)], prefix sums of pixcost
  int psw, psh;

  int Fail(const char* what, cl_int e) {
    fprintf(stderr, "\nmrpc: opencl: %s: %s\n", what, CLErrStr(e));
    if( active ) {
      fprintf(stderr, "mrpc: opencl: falling back to the host code path\n");
      active = 0;
    }
    return 0;
  }

  // enqueue, and when -V is on wait for it and charge the device time
  cl_int Run(cl_kernel k, int dim, const size_t* g, const size_t* l, int slot) {
    cl_event ev = 0;
    cl_int e = clEnqueueNDRangeKernel(q, k, dim, 0, g, l, 0, 0, verbose ? &ev : 0);
    if( e==CL_SUCCESS&&verbose ) {
      clWaitForEvents(1, &ev);
      cl_ulong t0 = 0, t1 = 0;
      clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(t0), &t0, 0);
      clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(t1), &t1, 0);
      kms[slot] += double(t1-t0)/1e6;
      kn[slot]++;
      clReleaseEvent(ev);
    }
    return e;
  }

  void Report(FILE* f) const {
    if( !verbose )
      return;
    fprintf(f, "mrpc: opencl: device time\n");
    for( int i = 0; i<7; i++ )
      if( kn[i] )
        fprintf(f, "  %-14s %8.2f s  %9ld launches  %6.1f us each\n",
                KName(i), kms[i]/1000.0, kn[i], kms[i]*1000.0/double(kn[i]));
  }

  cl_mem Buf(cl_mem_flags f, size_t n, cl_int* e) {
    return clCreateBuffer(ctx, f, n ? n : 1, 0, e);
  }

  // --- device selection -------------------------------------------
  int Open(const CLOpts &o) {
    memset(this, 0, sizeof(*this));
    verbose = o.verbose;
    CLDevList l;
    l.Enumerate(o.plat);
    if( l.n==0 ) {
      fprintf(stderr, "mrpc: opencl: no devices (-l lists them, -C disables this)\n");
      return 0;
    }
    int pick = -1;
    if( o.dev>=0 ) {
      if( o.dev>=l.n ) {
        fprintf(stderr, "mrpc: opencl: -d %d: only %d device%s (-l lists them)\n",
                o.dev, l.n, l.n==1 ? "" : "s");
        return 0;
      }
      pick = o.dev;
    } else {
      // no device named: the first of the wanted type, and with no type
      // wanted a GPU if there is one, since that is the point
      static const cl_device_type order[4][3] = {
        {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_ACCELERATOR, CL_DEVICE_TYPE_CPU},
        {CL_DEVICE_TYPE_CPU, 0, 0},
        {CL_DEVICE_TYPE_GPU, 0, 0},
        {CL_DEVICE_TYPE_ACCELERATOR, 0, 0}};
      for( int r = 0; r<3&&pick<0; r++ ) {
        cl_device_type want = order[o.type][r];
        if( !want )
          break;
        for( int i = 0; i<l.n; i++ ) {
          cl_device_type t = 0;
          clGetDeviceInfo(l.dev[i], CL_DEVICE_TYPE, sizeof(t), &t, 0);
          if( t&want ) {
            pick = i;
            break;
          }
        }
      }
      if( pick<0 ) {
        fprintf(stderr, "mrpc: opencl: no device of that type (-l lists them)\n");
        return 0;
      }
    }
    platform = l.plat[pick];
    device = l.dev[pick];

    cl_int e = CL_SUCCESS;
    cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    ctx = clCreateContext(props, 1, &device, 0, 0, &e);
    if( e!=CL_SUCCESS )
      return Fail("clCreateContext", e);
    q = CLMakeQueue(ctx, device, verbose, &e);
    if( e!=CL_SUCCESS )
      return Fail("clCreateCommandQueue", e);
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(lmem), &lmem, 0);
    return 1;
  }

  void Describe(char* buf, size_t n) const {
    char d[512], p[512];
    size_t got = 0;
    p[0] = 0;
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(p)-1, p, &got);
    p[(got<sizeof(p)) ? got : sizeof(p)-1] = 0;
    cl_device_type t = 0;
    clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(t), &t, 0);
    snprintf(buf, n, "[%s] %s (%s)", CLTypeStr(t),
             CLInfoStr(device, CL_DEVICE_NAME, d, sizeof(d)), p);
  }

  // --- build ------------------------------------------------------
  // The image geometry is a compile-time constant for the kernels: nc and
  // the row stride decide every address they compute, and a device
  // compiler that knows them vectorizes the tap loops.
  int Build(uint W_, uint H_, uint nc_, uint BS_, size_t bn_, int ntmax_, int numclass_, int nfreq) {
    W = W_;
    H = H_;
    nc = nc_;
    BS = BS_;
    bn = bn_;
    ntpad = (ntmax_+3)&~3;
    if( ntpad>NTMAX )
      ntpad = NTMAX; // it indexes a coefficient row, which is NTMAX long
    num_class = uint(numclass_);

    char opts[2048];
    snprintf(opts, sizeof(opts),
             "-cl-std=CL1.2 -cl-fast-relaxed-math"
             " -D NC=%u -D MAXC=%d -D BSW=%u -D BN=%llu -D NTMAX=%d -D NTPAD=%d"
             " -D PADT=%d -D PADL=%d -D HT_T=%d -D HT_L=%d -D HT_R=%d"
             " -D COEF_PREC=%d -D PM_ACC=%d -D NUM_SUBPM=%d -D MAXPRD=%d"
             " -D PMSIZE=%d -D CSTRIDE=%d -D MRP_GROUP=%d -D MAX_UPARA=%d"
             " -D NUM_UPELS=%d -D XUPEL=%d -D SR=%d -D SSR=%d -D NCAND=%d -D COEF_MAX=%d",
             nc, MAXC, BS, (unsigned long long)bn, NTMAX, ntpad,
             PADT, PADL, HT_T, HT_L, HT_R, COEF_PREC, PM_ACC, NUM_SUBPM, MAXPRD,
             PMSIZE, CSTRIDE, MRP_GROUP, MAX_UPARA, NUM_UPELS, XUPEL,
             CL_SR, CL_SSR, CL_NCAND, COEF_MAX);

    cl_int e = CL_SUCCESS;
    for( int b = 0; b<CL_NBS; b++ ) {
      char o2[2200];
      snprintf(o2, sizeof(o2), "%s -D BSZ=%d -D TW=%d -D TH=%d", opts,
               CL_BSZ[b], CL_TW[b], CL_TH[b]);
      prog[b] = clCreateProgramWithSource(ctx, 1, &MRPC_CL_SRC, 0, &e);
      if( e!=CL_SUCCESS )
        return Fail("clCreateProgramWithSource", e);
      e = clBuildProgram(prog[b], 1, &device, o2, 0, 0);
      if( e!=CL_SUCCESS||(verbose&&b==0) ) {
        size_t n = 0;
        clGetProgramBuildInfo(prog[b], device, CL_PROGRAM_BUILD_LOG, 0, 0, &n);
        if( n>1 ) {
          char* log = new char[n+1];
          clGetProgramBuildInfo(prog[b], device, CL_PROGRAM_BUILD_LOG, n, log, 0);
          log[n] = 0;
          fprintf(stderr, "mrpc: opencl: build log:\n%s\n", log);
          delete[] log;
        }
      }
      if( e!=CL_SUCCESS )
        return Fail("clBuildProgram", e);
      k_predict[b] = clCreateKernel(prog[b], "predict_cube", &e);
      if( e!=CL_SUCCESS )
        return Fail("predict_cube", e);
      k_cost[b] = clCreateKernel(prog[b], "pixel_cost", &e);
      if( e!=CL_SUCCESS )
        return Fail("pixel_cost", e);
    }
    if( CL_NBS<2 ) {
      k_predict[1] = k_predict[0];
      k_cost[1] = k_cost[0];
    }

    struct {
      const char* n;
      cl_kernel* k;
    } ks[] = {{"coef_scan", &k_scan}, {"coef_apply", &k_apply}};
    for( int i = 0; i<2; i++ ) {
      *ks[i].k = clCreateKernel(prog[0], ks[i].n, &e);
      if( e!=CL_SUCCESS )
        return Fail(ks[i].n, e);
    }
    // The group search needs doubles from end to end -- see group_hist --
    // so on a device without them it stays on the host entirely.
    cl_device_fp_config fp64 = 0;
    clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fp64), &fp64, 0);
    k_dp = k_hist = k_pmod = 0;
    if( fp64 ) {
      struct {
        const char* n;
        cl_kernel* k;
      } gk[] = {{"group_hist", &k_hist}, {"pmodel_cost", &k_pmod}, {"group_dp", &k_dp}};
      for( int i = 0; i<3; i++ ) {
        *gk[i].k = clCreateKernel(prog[0], gk[i].n, &e);
        if( e!=CL_SUCCESS )
          return Fail(gk[i].n, e);
      }
      // ... except the threshold DP, which is 252 work-items that each
      // run a long serial double-precision loop.  That shape fills a CPU
      // device and starves a graphics one: a few hundred threads on a
      // part built for tens of thousands, at the 1/64 fp64 rate a
      // consumer card runs doubles at, and long enough per launch to
      // trip Windows' two-second driver timeout.  It stays on the host
      // for anything that is not a CPU device.
      cl_device_type dt = 0;
      clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(dt), &dt, 0);
      if( !(dt&CL_DEVICE_TYPE_CPU) ) {
        k_dp = 0;
        if( verbose )
          fprintf(stderr, "mrpc: opencl: not a CPU device;"
                          " the threshold DP stays on the host\n");
      }
    } else
      fprintf(stderr, "mrpc: opencl: no fp64 on this device;"
                      " the group search stays on the host\n");

    // How finely a sweep is cut up.  Every worker costs 33 floats of
    // readback, and every class in the launch has its own set of them, so
    // this trades the tail of a slice against the size of the transfer.
    cl_uint cu = 4;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, 0);
    scan_workers = cu*4;
    if( scan_workers<8 )
      scan_workers = 8;
    if( scan_workers>64 )
      scan_workers = 64;
    // pmodel_cost cuts the image into one chunk per compute unit; both
    // this buffer and the host's have to be sized from that and not from
    // a number that happened to be larger than a CPU's core count
    pm_chunks = cu;

    // --- buffers
    const size_t npix = size_t(W)*H;
    const size_t cubepix = size_t(MRP_MAXCLASS)*MAX_BSIZE*MAX_BSIZE;
    const size_t cubetile = size_t(MRP_MAXCLASS)*THMAX*TWMAX;
    cl_int ee[32];
    int ne = 0;
    d_planes = Buf(CL_MEM_READ_ONLY, bn*nc+size_t(MAX_BSIZE+8)*BS, &ee[ne++]);
    d_org = Buf(CL_MEM_READ_ONLY, bn*nc, &ee[ne++]);
    d_prd = Buf(CL_MEM_READ_WRITE, npix*nc*sizeof(short), &ee[ne++]);
    d_err = Buf(CL_MEM_READ_WRITE, bn*nc*sizeof(short), &ee[ne++]);
    d_grp = Buf(CL_MEM_READ_ONLY, npix*nc, &ee[ne++]);
    d_cpo = Buf(CL_MEM_READ_ONLY, npix*sizeof(uint), &ee[ne++]);
    d_cpq = Buf(CL_MEM_READ_ONLY, npix*sizeof(uint), &ee[ne++]);
    d_coef = Buf(CL_MEM_READ_ONLY, size_t(MRP_MAXCLASS)*MAXC*NTMAX*sizeof(int), &ee[ne++]);
    d_tapoff = Buf(CL_MEM_READ_ONLY, size_t(MAXC)*ntpad*sizeof(int), &ee[ne++]);
    d_uq = Buf(CL_MEM_READ_ONLY, size_t(MRP_MAXCLASS)*MAXC*(MAX_UPARA+1), &ee[ne++]);
    d_costtab = Buf(CL_MEM_READ_ONLY, size_t(nfreq)*CSTRIDE*sizeof(float), &ee[ne++]);
    d_pmoff = Buf(CL_MEM_READ_ONLY, size_t(MAXC)*MRP_GROUP*sizeof(int), &ee[ne++]);
    d_udy = Buf(CL_MEM_READ_ONLY, NUM_UPELS*sizeof(int), &ee[ne++]);
    d_udx = Buf(CL_MEM_READ_ONLY, NUM_UPELS*sizeof(int), &ee[ne++]);
    d_uw = Buf(CL_MEM_READ_ONLY, NUM_UPELS*sizeof(int), &ee[ne++]);
    d_cA = Buf(CL_MEM_READ_ONLY, CL_NCAND*sizeof(int), &ee[ne++]);
    d_cB = Buf(CL_MEM_READ_ONLY, CL_NCAND*sizeof(int), &ee[ne++]);
    d_cbeg = Buf(CL_MEM_READ_ONLY, (MRP_MAXCLASS+1)*sizeof(uint), &ee[ne++]);
    d_halo = Buf(CL_MEM_READ_ONLY, size_t(nc)*THMAX*TWMAX*sizeof(short), &ee[ne++]);
    d_prdcube = Buf(CL_MEM_READ_WRITE, cubepix*nc*sizeof(short), &ee[ne++]);
    d_errcube = Buf(CL_MEM_READ_WRITE, cubetile*nc*sizeof(short), &ee[ne++]);
    d_pixcost = Buf(CL_MEM_WRITE_ONLY, cubepix*sizeof(float), &ee[ne++]);
    d_partials = Buf(CL_MEM_WRITE_ONLY,
                     size_t(MRP_MAXCLASS)*scan_workers*CL_NCAND*sizeof(float), &ee[ne++]);
    d_pmcost = Buf(CL_MEM_WRITE_ONLY,
                   size_t(MAXC)*MRP_GROUP*NUM_PMODEL*pm_chunks*sizeof(double), &ee[ne++]);
    d_hist = Buf(CL_MEM_READ_WRITE,
                 size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*(MAX_UPARA+2)*sizeof(double), &ee[ne++]);
    const size_t ndp = size_t(MRP_MAXCLASS)*MAXC*(MAX_UPARA+2);
    if( k_dp ) {
      d_cum = Buf(CL_MEM_READ_WRITE, ndp*MRP_GROUP*sizeof(double), &ee[ne++]);
      d_dp = Buf(CL_MEM_READ_WRITE, ndp*sizeof(double), &ee[ne++]);
      d_tre = Buf(CL_MEM_READ_WRITE, ndp*MRP_GROUP*sizeof(short), &ee[ne++]);
      d_th = Buf(CL_MEM_WRITE_ONLY, size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*sizeof(int), &ee[ne++]);
      d_thcost = Buf(CL_MEM_READ_ONLY, (MAX_UPARA+2)*sizeof(double), &ee[ne++]);
    }
    for( int i = 0; i<ne; i++ )
      if( ee[i]!=CL_SUCCESS )
        return Fail("clCreateBuffer", ee[i]);

    halo = new short[size_t(nc)*THMAX*TWMAX];
    pixcost = new float[cubepix];
    partials = new float[size_t(MRP_MAXCLASS)*scan_workers*CL_NCAND];
    pmcost = new double[size_t(MAXC)*MRP_GROUP*NUM_PMODEL*pm_chunks];
    psum = new double[size_t(MRP_MAXCLASS)*(MAX_BSIZE+1)*(MAX_BSIZE+1)];

    active = 1;
    return 1;
  }

  int Write(cl_mem b, const void* p, size_t n, const char* what, size_t off = 0) {
    if( n )
      CLCK(clEnqueueWriteBuffer(q, b, CL_TRUE, off, n, p, 0, 0, 0), what);
    return 1;
  }

  // --- per-block class search --------------------------------------
  // Returns 0 if anything went wrong, and the caller runs the host path.
  int BlockCube(int b, int tly, int tlx, int bh, int bw, int ncls) {
    const int bsz = CL_BSZ[b], tw = CL_TW[b], th = CL_TH[b];
    CLCK(clEnqueueWriteBuffer(q, d_halo, CL_FALSE, 0, size_t(nc)*th*tw*sizeof(short),
                              halo, 0, 0, 0), "write halo");

    int a = 0;
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_planes), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_coef), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_tapoff), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_halo), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_prdcube), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(cl_mem), &d_errcube), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(int), &tly), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(int), &tlx), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(int), &bh), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(int), &bw), "arg");
    CLCK(clSetKernelArg(k_predict[b], a++, sizeof(int), &ncls), "arg");
    size_t g1[3] = {size_t(tw), size_t(th), size_t((ncls+CL_CLGRP-1)/CL_CLGRP)};
    CLCK(Run(k_predict[b], 3, g1, 0, 0), "predict_cube");

    a = 0;
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_planes), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_prdcube), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_errcube), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_uq), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_costtab), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_pmoff), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_udy), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_udx), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_uw), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(cl_mem), &d_pixcost), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(int), &tly), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(int), &tlx), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(int), &bh), "arg");
    CLCK(clSetKernelArg(k_cost[b], a++, sizeof(int), &bw), "arg");
    size_t g2[3] = {size_t(bsz), size_t(bsz), size_t(ncls)};
    CLCK(Run(k_cost[b], 3, g2, 0, 1), "pixel_cost");

    CLCK(clEnqueueReadBuffer(q, d_pixcost, CL_TRUE, 0,
                             size_t(ncls)*bsz*bsz*sizeof(float),
                             pixcost, 0, 0, 0), "read pixcost");

    // 2D prefix sums, one per class: every rectangle the quadtree asks
    // about is then four loads.  The cube is MAX_BSIZE square whatever
    // the block is; only the block's own corner of it is summed.
    psh = bh+1;
    psw = bw+1;
    for( int cl = 0; cl<ncls; cl++ ) {
      const float* s = pixcost+size_t(cl)*bsz*bsz;
      double* d = psum+size_t(cl)*psh*psw;
      for( int x = 0; x<psw; x++ )
        d[x] = 0.0;
      for( int y = 0; y<bh; y++ ) {
        const float* sr = s+size_t(y)*bsz;
        const double* p0 = d+size_t(y)*psw;
        double* p1 = d+size_t(y+1)*psw;
        double row = 0.0;
        p1[0] = 0.0;
        for( int x = 0; x<bw; x++ ) {
          row += double(sr[x]);
          p1[x+1] = p0[x+1]+row;
        }
      }
    }
    return 1;
  }

  // the cost of a rectangle of the current block under class cl
  double CubeCost(int cl, int y0, int x0, int y1, int x1) const {
    const double* p = psum+size_t(cl)*psh*psw;
    return p[y1*psw+x1]-p[y0*psw+x1]-p[y1*psw+x0]+p[y0*psw+x0];
  }

  // --- the coefficient sweep ---------------------------------------
  // One launch sweeps every class, and out[] comes back as
  // [class][candidate].
  int CoefScan(int ncls, int k, int o1, int o2, double* out) {
    int a = 0, w = int(scan_workers);
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_org), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_prd), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_grp), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_cpo), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_cpq), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_costtab), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_pmoff), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_cA), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_cB), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_cbeg), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(int), &k), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(int), &o1), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(int), &o2), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(int), &w), "arg");
    CLCK(clSetKernelArg(k_scan, a++, sizeof(cl_mem), &d_partials), "arg");
    size_t g[2] = {scan_workers, size_t(ncls)};
    CLCK(Run(k_scan, 2, g, 0, 2), "coef_scan");
    CLCK(clEnqueueReadBuffer(q, d_partials, CL_TRUE, 0,
                             size_t(ncls)*scan_workers*CL_NCAND*sizeof(float),
                             partials, 0, 0, 0), "read partials");
    for( int cl = 0; cl<ncls; cl++ ) {
      const float* p = partials+size_t(cl)*scan_workers*CL_NCAND;
      double* o = out+size_t(cl)*CL_NCAND;
      for( int t = 0; t<CL_NCAND; t++ )
        o[t] = 0.0;
      for( size_t w2 = 0; w2<scan_workers; w2++ )
        for( int t = 0; t<CL_NCAND; t++ )
          o[t] += double(p[w2*CL_NCAND+t]);
    }
    return 1;
  }

  int CoefApply(uint cbeg, uint cend, int k, int o1, int o2, int di, int dj) {
    if( cend<=cbeg )
      return 1;
    int a = 0;
    CLCK(clSetKernelArg(k_apply, a++, sizeof(cl_mem), &d_org), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(cl_mem), &d_prd), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(cl_mem), &d_err), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(cl_mem), &d_cpo), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(cl_mem), &d_cpq), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(uint), &cbeg), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(uint), &cend), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(int), &k), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(int), &o1), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(int), &o2), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(int), &di), "arg");
    CLCK(clSetKernelArg(k_apply, a++, sizeof(int), &dj), "arg");
    size_t g = cend-cbeg;
    CLCK(Run(k_apply, 1, &g, 0, 3), "coef_apply");
    return 1;
  }

  // --- the group histogram ------------------------------------------
  int GroupHist(int ncls, double* out) {
    if( !k_hist )
      return 0;
    int a = 0;
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_org), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_prd), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_err), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_cpo), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_cpq), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_costtab), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_pmoff), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_udy), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_udx), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_uw), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_cbeg), "arg");
    CLCK(clSetKernelArg(k_hist, a++, sizeof(cl_mem), &d_hist), "arg");
    size_t g[3] = {MRP_GROUP, size_t(nc), size_t(ncls)};
    CLCK(Run(k_hist, 3, g, 0, 4), "group_hist");
    CLCK(clEnqueueReadBuffer(q, d_hist, CL_TRUE, 0,
                             size_t(ncls)*nc*MRP_GROUP*(MAX_UPARA+2)*sizeof(double),
                             out, 0, 0, 0), "read hist");
    return 1;
  }

  // The threshold DP for every (class, component) at once.  thcost is
  // the side cost of a threshold, and th comes back as the host's
  // th[class][component][group].
  int GroupDPCL(int ncls, const double* thcost, int* th) {
    if( !k_dp )
      return 0;
    if( !Write(d_thcost, thcost, (MAX_UPARA+2)*sizeof(double), "thcost") )
      return 0;
    int a = 0;
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_hist), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_cum), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_thcost), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_dp), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_tre), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(cl_mem), &d_th), "arg");
    CLCK(clSetKernelArg(k_dp, a++, sizeof(int), &ncls), "arg");
    size_t g[2] = {size_t(ncls), size_t(nc)};
    CLCK(Run(k_dp, 2, g, 0, 6), "group_dp");
    CLCK(clEnqueueReadBuffer(q, d_th, CL_TRUE, 0,
                             size_t(MRP_MAXCLASS)*MAXC*MRP_GROUP*sizeof(int),
                             th, 0, 0, 0), "read th");
    return 1;
  }

  // out[k][gr][i]: what component k's group gr would cost under shape i
  int PmodelCost(int npix_, int num_pm, double* out) {
    if( !k_pmod )
      return 0;
    int a = 0;
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_org), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_prd), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_grp), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_cpo), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_cpq), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_costtab), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(int), &npix_), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(int), &num_pm), "arg");
    CLCK(clSetKernelArg(k_pmod, a++, sizeof(cl_mem), &d_pmcost), "arg");
    size_t g[3] = {size_t(num_pm), size_t(nc), pm_chunks};
    CLCK(Run(k_pmod, 3, g, 0, 5), "pmodel_cost");
    const size_t n = size_t(nc)*MRP_GROUP*num_pm*pm_chunks;
    CLCK(clEnqueueReadBuffer(q, d_pmcost, CL_TRUE, 0, n*sizeof(double),
                             pmcost, 0, 0, 0), "read pmcost");
    for( uint k = 0; k<nc; k++ )
      for( int g2 = 0; g2<MRP_GROUP; g2++ )
        for( int i = 0; i<num_pm; i++ ) {
          const double* p = pmcost+(size_t(k*MRP_GROUP+g2)*num_pm+i)*pm_chunks;
          double s = 0.0;
          for( size_t ch = 0; ch<pm_chunks; ch++ )
            s += p[ch];
          out[(size_t(k)*MRP_GROUP+g2)*NUM_PMODEL+i] = s;
        }
    return 1;
  }
};

static MrpcCL g_cl;
