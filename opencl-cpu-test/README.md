# OpenCL on the CPU — Intel runtime + test program

A working install of the **Intel CPU Runtime for OpenCL Applications** and a
small C++ program that runs kernels on it and verifies the results.

No GPU is involved: the Intel runtime exposes the host CPU as an OpenCL device
and JIT-compiles kernels into vectorized (SSE/AVX/AVX-512) machine code.

## 1. Install the runtime

The runtime is distributed through Intel's oneAPI apt repository as
`intel-oneapi-runtime-opencl` (see Intel's
[CPU Runtime for OpenCL Applications with SYCL support](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-cpu-runtime-for-opencl-applications-with-sycl-support.html)).
It is an ICD, so an ICD loader (`ocl-icd-libopencl1`) has to be installed
alongside it for applications to find it.

```sh
sudo ./install-intel-opencl-cpu.sh
```

which is the scripted form of:

```sh
curl -fsSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  | sudo gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
  | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt-get update
sudo apt-get install -y intel-oneapi-runtime-opencl ocl-icd-libopencl1 opencl-headers clinfo
```

The package drops `/etc/OpenCL/vendors/intel64.icd` pointing at
`/opt/intel/oneapi/redist/lib/libintelocl.so`, and registers
`/usr/lib/x86_64-linux-gnu/libOpenCL.so` through `update-alternatives`. Check it
with `clinfo --list`:

```
Platform #0: Intel(R) OpenCL
 `-- Device #0: Intel(R) Xeon(R) Processor @ 2.80GHz
```

Other distributions: Intel also publishes the same runtime as a standalone
tarball on the [intel/llvm releases](https://github.com/intel/llvm/releases)
page (`oclcpuexp-*.tar.gz`); unpack it and write an `.icd` file naming its
`libintelocl.so`.

## 2. Build and run the test

```sh
make          # g++ -O2 -std=c++17 ocltest.cpp -lOpenCL -o ocltest
./ocltest     # or: make run
```

`ocltest` enumerates the platforms, picks a CPU device (falling back to any
device if no CPU runtime is present), builds `kernels.cl` at run time, and runs
three kernels. **Every result is checked against a plain C++ reference on the
host**, and the exit status is non-zero if any check fails.

| kernel | what it exercises | check |
| --- | --- | --- |
| `saxpy` | float math, one work-item per element, memory bound | relative error < 1e-5 |
| `hash_chain` | 64 rounds of xorshift64\* per element, integer, compute bound | bit-exact |
| `reduce_sum` | `__local` memory, `barrier()`, work-group tree reduction | bit-exact |

Kernel times come from OpenCL profiling events
(`CL_QUEUE_PROFILING_ENABLE`), so they measure device execution rather than
enqueue overhead. The program also prints the build log, which is where the
Intel compiler reports its vectorization decisions.

The kernel source is looked up in `argv[1]`, then `$OCLTEST_KERNELS`, then next
to the binary, then the working directory — so the binary can be run from
anywhere.

### Sample output

```
OpenCL platforms: 1
  platform  : Intel(R) OpenCL (Intel(R) Corporation, OpenCL 3.0 LINUX)
    device  : [CPU] Intel(R) Xeon(R) Processor @ 2.80GHz

using device  : Intel(R) Xeon(R) Processor @ 2.80GHz
  vendor      : Intel(R) Corporation
  version     : OpenCL 3.0 (Build 0)
  driver      : 2026.21.7.0.24_160000
  compute u.  : 4 @ 2800 MHz
  global mem  : 15.70 GiB
  local mem   : 256 KiB
  max wg size : 8192
  pref. float : float1
kernel source : /path/to/opencl-cpu-test/kernels.cl

--- build log ---
...
Kernel "saxpy" was successfully vectorized (16)
Kernel "hash_chain" was successfully vectorized (16)
Kernel "reduce_sum" was successfully vectorized (16)
Done.

problem size  : 4194304 elements

[saxpy]          6.326 ms    7.96 GB/s  max rel err 0  ok
[hash_chain]    45.643 ms   29.41 Gop/s  ok (bit-exact)
[reduce_sum]     1.527 ms  64 groups x 256  sum 0xa90b56d1f65af5ff  ok (bit-exact)

all checks passed
```

The "vectorized (16)" lines are the runtime packing 16 work-items per SIMD lane
group — the CPU-runtime equivalent of a warp — on a machine with AVX-512.

## Troubleshooting

* **`no OpenCL platforms found`** — the loader found no `.icd` file. Check
  `ls /etc/OpenCL/vendors/` and that `libintelocl.so` at the path inside it
  exists.
* **`cannot open shared object file: libOpenCL.so.1`** — the ICD loader is
  missing: `sudo apt-get install ocl-icd-libopencl1`.
* **A GPU device is picked instead** — `ocltest` prefers CPU devices; if a GPU
  is still chosen the CPU runtime is not registered.
* **`CL_DEVICE_NOT_FOUND` from the Intel platform** — the runtime requires
  SSE4.2 or newer; check `grep sse4_2 /proc/cpuinfo`.

## Files

| file | |
| --- | --- |
| `install-intel-opencl-cpu.sh` | installs the runtime, loader and headers |
| `ocltest.cpp` | host program: device selection, build, dispatch, verification |
| `kernels.cl` | the three OpenCL C kernels |
| `Makefile` | `make`, `make run`, `make clean` |
