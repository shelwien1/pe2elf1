// ocltest - a small OpenCL smoke test for the Intel CPU Runtime for OpenCL
// Applications (the "intel-oneapi-runtime-opencl" package).
//
// It enumerates the ICD platforms, picks a CPU device, builds kernels.cl at
// run time, and executes three kernels whose results are all checked against a
// plain C++ reference computed on the host:
//
//   saxpy       float math                     -> compared with a tolerance
//   hash_chain  integer math, compute bound    -> compared bit-exact
//   reduce_sum  local memory + barriers        -> compared bit-exact
//
// Kernel wall time is taken from OpenCL profiling events. Exit status is 0 when
// every check passes, 1 otherwise.
//
// Build: g++ -O2 -std=c++17 ocltest.cpp -lOpenCL -o ocltest

#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

const char* cl_error_string(cl_int e)
{
    switch (e) {
#define E(x) case x: return #x;
    E(CL_SUCCESS) E(CL_DEVICE_NOT_FOUND) E(CL_DEVICE_NOT_AVAILABLE)
    E(CL_COMPILER_NOT_AVAILABLE) E(CL_MEM_OBJECT_ALLOCATION_FAILURE)
    E(CL_OUT_OF_RESOURCES) E(CL_OUT_OF_HOST_MEMORY) E(CL_PROFILING_INFO_NOT_AVAILABLE)
    E(CL_MEM_COPY_OVERLAP) E(CL_IMAGE_FORMAT_MISMATCH) E(CL_BUILD_PROGRAM_FAILURE)
    E(CL_MAP_FAILURE) E(CL_INVALID_VALUE) E(CL_INVALID_DEVICE_TYPE)
    E(CL_INVALID_PLATFORM) E(CL_INVALID_DEVICE) E(CL_INVALID_CONTEXT)
    E(CL_INVALID_QUEUE_PROPERTIES) E(CL_INVALID_COMMAND_QUEUE) E(CL_INVALID_HOST_PTR)
    E(CL_INVALID_MEM_OBJECT) E(CL_INVALID_BINARY) E(CL_INVALID_BUILD_OPTIONS)
    E(CL_INVALID_PROGRAM) E(CL_INVALID_PROGRAM_EXECUTABLE) E(CL_INVALID_KERNEL_NAME)
    E(CL_INVALID_KERNEL_DEFINITION) E(CL_INVALID_KERNEL) E(CL_INVALID_ARG_INDEX)
    E(CL_INVALID_ARG_VALUE) E(CL_INVALID_ARG_SIZE) E(CL_INVALID_KERNEL_ARGS)
    E(CL_INVALID_WORK_DIMENSION) E(CL_INVALID_WORK_GROUP_SIZE)
    E(CL_INVALID_WORK_ITEM_SIZE) E(CL_INVALID_GLOBAL_OFFSET)
    E(CL_INVALID_EVENT_WAIT_LIST) E(CL_INVALID_EVENT) E(CL_INVALID_OPERATION)
    E(CL_INVALID_BUFFER_SIZE) E(CL_INVALID_GLOBAL_WORK_SIZE)
#undef E
    default: return "CL_UNKNOWN_ERROR";
    }
}

[[noreturn]] void fail(const char* what, cl_int err, const char* file, int line)
{
    std::fprintf(stderr, "%s:%d: %s failed: %s (%d)\n", file, line, what,
                 cl_error_string(err), err);
    std::exit(1);
}

#define CL_CHECK(call)                                                        \
    do {                                                                      \
        cl_int err_ = (call);                                                  \
        if (err_ != CL_SUCCESS) fail(#call, err_, __FILE__, __LINE__);         \
    } while (0)

// For the clCreate* family, which report through an out parameter.
cl_int g_err = CL_SUCCESS;
#define CL_CHECKED(expr)                                                      \
    ([&] {                                                                    \
        auto v_ = (expr);                                                     \
        if (g_err != CL_SUCCESS) fail(#expr, g_err, __FILE__, __LINE__);       \
        return v_;                                                            \
    }())

std::string device_string(cl_device_id dev, cl_device_info info)
{
    size_t n = 0;
    CL_CHECK(clGetDeviceInfo(dev, info, 0, nullptr, &n));
    std::string s(n, '\0');
    CL_CHECK(clGetDeviceInfo(dev, info, n, s.data(), nullptr));
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

std::string platform_string(cl_platform_id p, cl_platform_info info)
{
    size_t n = 0;
    CL_CHECK(clGetPlatformInfo(p, info, 0, nullptr, &n));
    std::string s(n, '\0');
    CL_CHECK(clGetPlatformInfo(p, info, n, s.data(), nullptr));
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

template <typename T>
T device_value(cl_device_id dev, cl_device_info info)
{
    T v{};
    CL_CHECK(clGetDeviceInfo(dev, info, sizeof(T), &v, nullptr));
    return v;
}

// Milliseconds spent between CL_PROFILING_COMMAND_START and _END.
double event_ms(cl_event ev)
{
    cl_ulong t0 = 0, t1 = 0;
    CL_CHECK(clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(t0), &t0, nullptr));
    CL_CHECK(clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(t1), &t1, nullptr));
    return double(t1 - t0) / 1.0e6;
}

std::string exe_dir()
{
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    buf[n] = '\0';
    char* slash = std::strrchr(buf, '/');
    if (slash) *slash = '\0';
    return buf;
}

// Kernel source path: argv[1], then $OCLTEST_KERNELS, then next to the binary,
// then the working directory.
std::string read_kernel_source(const char* argv1)
{
    std::vector<std::string> candidates;
    if (argv1) candidates.emplace_back(argv1);
    if (const char* env = std::getenv("OCLTEST_KERNELS")) candidates.emplace_back(env);
    candidates.push_back(exe_dir() + "/kernels.cl");
    candidates.emplace_back("kernels.cl");

    for (const std::string& path : candidates) {
        std::ifstream f(path);
        if (!f) continue;
        std::ostringstream ss;
        ss << f.rdbuf();
        std::printf("kernel source : %s\n", path.c_str());
        return ss.str();
    }

    std::fprintf(stderr, "cannot find kernels.cl (tried %zu paths); "
                         "pass the path as argv[1]\n", candidates.size());
    std::exit(1);
}

// Host references, deliberately written the same way the kernels are.
uint64_t host_hash_chain(uint32_t seed)
{
    uint64_t x = uint64_t(seed) * 0x2545F4914F6CDD1DULL + 0x9E3779B97F4A7C15ULL;
    for (int r = 0; r < 64; r++) {
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        x *= 0x2545F4914F6CDD1DULL;
    }
    return x;
}

// Picks the first CPU device, falling back to any device if the CPU runtime is
// not installed. Returns false when no device exists at all.
bool pick_device(cl_platform_id* out_platform, cl_device_id* out_device)
{
    cl_uint num_platforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        std::fprintf(stderr, "no OpenCL platforms found (%s)\n", cl_error_string(err));
        return false;
    }
    std::vector<cl_platform_id> platforms(num_platforms);
    CL_CHECK(clGetPlatformIDs(num_platforms, platforms.data(), nullptr));

    std::printf("OpenCL platforms: %u\n", num_platforms);

    bool have_pick = false;
    for (cl_platform_id p : platforms) {
        std::printf("  platform  : %s (%s, %s)\n",
                    platform_string(p, CL_PLATFORM_NAME).c_str(),
                    platform_string(p, CL_PLATFORM_VENDOR).c_str(),
                    platform_string(p, CL_PLATFORM_VERSION).c_str());

        cl_uint num_devices = 0;
        if (clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices) != CL_SUCCESS)
            continue;
        std::vector<cl_device_id> devices(num_devices);
        CL_CHECK(clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr));

        for (cl_device_id d : devices) {
            cl_device_type type = device_value<cl_device_type>(d, CL_DEVICE_TYPE);
            const char* kind = (type & CL_DEVICE_TYPE_CPU)         ? "CPU"
                             : (type & CL_DEVICE_TYPE_GPU)         ? "GPU"
                             : (type & CL_DEVICE_TYPE_ACCELERATOR) ? "ACCELERATOR"
                                                                   : "OTHER";
            std::printf("    device  : [%s] %s\n", kind,
                        device_string(d, CL_DEVICE_NAME).c_str());

            // Prefer a CPU device; otherwise keep the first device seen.
            if ((type & CL_DEVICE_TYPE_CPU) || !have_pick) {
                if (!have_pick || (type & CL_DEVICE_TYPE_CPU)) {
                    bool already_cpu = have_pick &&
                        (device_value<cl_device_type>(*out_device, CL_DEVICE_TYPE) & CL_DEVICE_TYPE_CPU);
                    if (!already_cpu) {
                        *out_platform = p;
                        *out_device   = d;
                        have_pick     = true;
                    }
                }
            }
        }
    }

    if (!have_pick) std::fprintf(stderr, "no OpenCL devices found\n");
    return have_pick;
}

void print_device_info(cl_device_id dev)
{
    auto cus       = device_value<cl_uint>(dev, CL_DEVICE_MAX_COMPUTE_UNITS);
    auto mhz       = device_value<cl_uint>(dev, CL_DEVICE_MAX_CLOCK_FREQUENCY);
    auto gmem      = device_value<cl_ulong>(dev, CL_DEVICE_GLOBAL_MEM_SIZE);
    auto lmem      = device_value<cl_ulong>(dev, CL_DEVICE_LOCAL_MEM_SIZE);
    auto max_wg    = device_value<size_t>(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE);
    auto vec_float = device_value<cl_uint>(dev, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);

    std::printf("\nusing device  : %s\n", device_string(dev, CL_DEVICE_NAME).c_str());
    std::printf("  vendor      : %s\n", device_string(dev, CL_DEVICE_VENDOR).c_str());
    std::printf("  version     : %s\n", device_string(dev, CL_DEVICE_VERSION).c_str());
    std::printf("  driver      : %s\n", device_string(dev, CL_DRIVER_VERSION).c_str());
    std::printf("  compute u.  : %u @ %u MHz\n", cus, mhz);
    std::printf("  global mem  : %.2f GiB\n", double(gmem) / (1024.0 * 1024.0 * 1024.0));
    std::printf("  local mem   : %llu KiB\n", (unsigned long long)(lmem / 1024));
    std::printf("  max wg size : %zu\n", max_wg);
    std::printf("  pref. float : float%u\n", vec_float);
}

} // namespace

int main(int argc, char** argv)
{
    cl_platform_id platform = nullptr;
    cl_device_id   device   = nullptr;
    if (!pick_device(&platform, &device)) {
        std::fprintf(stderr,
                     "\nIs the Intel CPU runtime installed and registered?\n"
                     "  ls /etc/OpenCL/vendors/   (expects intel64.icd)\n");
        return 1;
    }
    print_device_info(device);

    const std::string source = read_kernel_source(argc > 1 ? argv[1] : nullptr);

    // --- context, queue, program ------------------------------------------
    cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0
    };
    cl_context context = CL_CHECKED(
        clCreateContext(props, 1, &device, nullptr, nullptr, &g_err));

    const cl_queue_properties qprops[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };
    cl_command_queue queue = CL_CHECKED(
        clCreateCommandQueueWithProperties(context, device, qprops, &g_err));

    const char* src_ptr = source.c_str();
    size_t      src_len = source.size();
    cl_program program  = CL_CHECKED(
        clCreateProgramWithSource(context, 1, &src_ptr, &src_len, &g_err));

    cl_int build_err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", nullptr, nullptr);
    {
        size_t log_len = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_len);
        if (log_len > 1) {
            std::string log(log_len, '\0');
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_len, log.data(), nullptr);
            std::printf("\n--- build log ---\n%s\n", log.c_str());
        }
    }
    if (build_err != CL_SUCCESS) {
        std::fprintf(stderr, "clBuildProgram failed: %s\n", cl_error_string(build_err));
        return 1;
    }

    cl_kernel k_saxpy  = CL_CHECKED(clCreateKernel(program, "saxpy", &g_err));
    cl_kernel k_hash   = CL_CHECKED(clCreateKernel(program, "hash_chain", &g_err));
    cl_kernel k_reduce = CL_CHECKED(clCreateKernel(program, "reduce_sum", &g_err));

    const size_t N = 1u << 22; // 4 Mi elements
    std::printf("\nproblem size  : %zu elements\n", N);

    // --- host inputs -------------------------------------------------------
    std::vector<float>    a(N), b(N), c_gpu(N);
    std::vector<uint32_t> seed(N);
    for (size_t i = 0; i < N; i++) {
        a[i]    = float(i % 1000) * 0.5f;
        b[i]    = float((i * 7) % 977) * 0.25f;
        seed[i] = uint32_t(i * 2654435761u + 12345u);
    }
    const float s = 3.25f;

    cl_mem d_a = CL_CHECKED(clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                           N * sizeof(float), a.data(), &g_err));
    cl_mem d_b = CL_CHECKED(clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                           N * sizeof(float), b.data(), &g_err));
    cl_mem d_c = CL_CHECKED(clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                           N * sizeof(float), nullptr, &g_err));
    cl_mem d_seed = CL_CHECKED(clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                              N * sizeof(uint32_t), seed.data(), &g_err));
    cl_mem d_hash = CL_CHECKED(clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              N * sizeof(uint64_t), nullptr, &g_err));

    int failures = 0;

    // --- 1. saxpy ----------------------------------------------------------
    {
        CL_CHECK(clSetKernelArg(k_saxpy, 0, sizeof(d_a), &d_a));
        CL_CHECK(clSetKernelArg(k_saxpy, 1, sizeof(d_b), &d_b));
        CL_CHECK(clSetKernelArg(k_saxpy, 2, sizeof(s), &s));
        CL_CHECK(clSetKernelArg(k_saxpy, 3, sizeof(d_c), &d_c));

        cl_event ev = nullptr;
        CL_CHECK(clEnqueueNDRangeKernel(queue, k_saxpy, 1, nullptr, &N, nullptr,
                                        0, nullptr, &ev));
        CL_CHECK(clWaitForEvents(1, &ev));
        const double ms = event_ms(ev);
        CL_CHECK(clReleaseEvent(ev));

        CL_CHECK(clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, N * sizeof(float),
                                     c_gpu.data(), 0, nullptr, nullptr));

        double  max_rel = 0.0;
        size_t  bad     = 0;
        for (size_t i = 0; i < N; i++) {
            const float want = a[i] * s + b[i];
            const float got  = c_gpu[i];
            const double rel = std::fabs(want - got) / (std::fabs(want) + 1e-6f);
            if (rel > max_rel) max_rel = rel;
            if (rel > 1e-5) bad++;
        }
        const double gbps = (3.0 * N * sizeof(float)) / (ms * 1e-3) / 1e9;
        std::printf("\n[saxpy]       %8.3f ms  %6.2f GB/s  max rel err %.3g  %s\n",
                    ms, gbps, max_rel, bad ? "FAIL" : "ok");
        if (bad) {
            std::printf("              %zu of %zu elements outside tolerance\n", bad, N);
            failures++;
        }
    }

    // --- 2. hash_chain -----------------------------------------------------
    std::vector<uint64_t> hash_gpu(N);
    {
        CL_CHECK(clSetKernelArg(k_hash, 0, sizeof(d_seed), &d_seed));
        CL_CHECK(clSetKernelArg(k_hash, 1, sizeof(d_hash), &d_hash));

        cl_event ev = nullptr;
        CL_CHECK(clEnqueueNDRangeKernel(queue, k_hash, 1, nullptr, &N, nullptr,
                                        0, nullptr, &ev));
        CL_CHECK(clWaitForEvents(1, &ev));
        const double ms = event_ms(ev);
        CL_CHECK(clReleaseEvent(ev));

        CL_CHECK(clEnqueueReadBuffer(queue, d_hash, CL_TRUE, 0, N * sizeof(uint64_t),
                                     hash_gpu.data(), 0, nullptr, nullptr));

        size_t bad = 0;
        for (size_t i = 0; i < N; i++)
            if (hash_gpu[i] != host_hash_chain(seed[i])) bad++;

        // 64 rounds x 5 integer ops per element, roughly.
        const double gops = (double(N) * 64.0 * 5.0) / (ms * 1e-3) / 1e9;
        std::printf("[hash_chain]  %8.3f ms  %6.2f Gop/s  %s\n",
                    ms, gops, bad ? "FAIL" : "ok (bit-exact)");
        if (bad) {
            std::printf("              %zu of %zu elements differ from the host result\n", bad, N);
            failures++;
        }
    }

    // --- 3. reduce_sum -----------------------------------------------------
    {
        size_t kernel_wg = 0;
        CL_CHECK(clGetKernelWorkGroupInfo(k_reduce, device, CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof(kernel_wg), &kernel_wg, nullptr));
        const auto lmem = device_value<cl_ulong>(device, CL_DEVICE_LOCAL_MEM_SIZE);

        size_t local = 256;
        while (local > 1 && (local > kernel_wg || local * sizeof(uint64_t) > lmem))
            local /= 2;

        const size_t groups = 64;
        const size_t global = groups * local;
        const cl_ulong n    = N;

        cl_mem d_partials = CL_CHECKED(clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                                      groups * sizeof(uint64_t), nullptr, &g_err));

        CL_CHECK(clSetKernelArg(k_reduce, 0, sizeof(d_hash), &d_hash));
        CL_CHECK(clSetKernelArg(k_reduce, 1, sizeof(n), &n));
        CL_CHECK(clSetKernelArg(k_reduce, 2, local * sizeof(uint64_t), nullptr)); // __local
        CL_CHECK(clSetKernelArg(k_reduce, 3, sizeof(d_partials), &d_partials));

        cl_event ev = nullptr;
        CL_CHECK(clEnqueueNDRangeKernel(queue, k_reduce, 1, nullptr, &global, &local,
                                        0, nullptr, &ev));
        CL_CHECK(clWaitForEvents(1, &ev));
        const double ms = event_ms(ev);
        CL_CHECK(clReleaseEvent(ev));

        std::vector<uint64_t> partials(groups);
        CL_CHECK(clEnqueueReadBuffer(queue, d_partials, CL_TRUE, 0, groups * sizeof(uint64_t),
                                     partials.data(), 0, nullptr, nullptr));
        CL_CHECK(clReleaseMemObject(d_partials));

        uint64_t got = 0;
        for (uint64_t p : partials) got += p;

        uint64_t want = 0;
        for (size_t i = 0; i < N; i++) want += hash_gpu[i];

        const bool bad = (got != want);
        std::printf("[reduce_sum]  %8.3f ms  %zu groups x %zu  sum 0x%016llx  %s\n",
                    ms, groups, local, (unsigned long long)got,
                    bad ? "FAIL" : "ok (bit-exact)");
        if (bad) {
            std::printf("              host sum is 0x%016llx\n", (unsigned long long)want);
            failures++;
        }
    }

    // --- cleanup -----------------------------------------------------------
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_c);
    clReleaseMemObject(d_seed);
    clReleaseMemObject(d_hash);
    clReleaseKernel(k_saxpy);
    clReleaseKernel(k_hash);
    clReleaseKernel(k_reduce);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    std::printf("\n%s\n", failures ? "FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
