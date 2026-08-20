# mrpc on OpenCL

`mrpc` is a lossless RGB/RGBA BMP compressor on MRP's scheme: it segments the
image over a quadtree, fits a linear predictor per (class, colour component),
picks a probability model per pixel by local activity, and optimises all of
that against measured code length. It is a two-pass coder, and the second pass
is where the hours go.

This port moves the encoder's optimisation loop to an OpenCL device and adds
the command line for choosing one. The bitstream format is unchanged, the
decoder is untouched, and `-C` still runs the original code path byte for byte.

## Where the time was

`gprof`, encoding a 705x800x4 image with `MAX_ITER=2`:

| | share | what it is |
| --- | --- | --- |
| `CalcCost` | 51% | 25 million calls — the code length of a block under a class |
| `FitPredictor` | 18% | least squares, once per class per iteration |
| `SetPrdBuf` | 13% | every pixel of a block predicted under every class |
| `OptimizeCoef` | 11% | the 11x3 candidate sweep of the coefficient search |
| `VbsClass` | 5% | the quadtree recursion itself |

Over a full encode that is `OptimizeClass` ~70%, `OptimizeGroup` ~14%,
`OptimizePredictor` ~9%. Those three are what moved.

## What runs on the device

The decoder does not use OpenCL at all, and neither does anything that writes
to the bitstream. A decoder that needs a device to read a file is not a
decoder.

**The class search** (`predict_cube`, `pixel_cost`). The host asks "what does
this block cost under class *c*?" 341 times per 32x32 block — once per quadtree
node per class — and each answer is a fresh pass over the block's pixels. Every
one of those passes computes the same per-pixel numbers; only the rectangle
they are summed over differs. So the device computes the cube of per-pixel
costs once, for every class at once, and the quadtree reads rectangle sums out
of a prefix sum of it. That is a fifth of the arithmetic before any of it is
made parallel, and it subsumes `SetPrdBuf` as well.

**The coefficient sweep** (`coef_scan`, `coef_apply`). The 33 candidates are
the reduction and the class's pixels are the parallel part. Every class is
swept in the same launch: two classes never touch the same pixel or the same
coefficient, so the class loop can be turned inside out without changing the
sequence of shifts any one class sees. `prd` and `errB` stay on the device for
the length of the pass.

**The group quantiser** (`group_hist`, `group_dp`, `pmodel_cost`). The
histogram is one work-item per (class, component, group) — no atomics, no
local memory. The threshold search after it is a 514-state, 16-stage shortest
path, and there is one per (class, component): 252 of them, all independent,
and together they were 0.69 s of every iteration.

Everything else stays on the host: the rangecoder, the quadtree bookkeeping,
the Gauss-Jordan solve, and the first optimisation loop's `BASE_BSIZE` class
search — 8x8 pixels is about forty microseconds of arithmetic, which does not
pay for two launches and a readback (measured: a shade slower than just doing
it).

## Results

Intel CPU Runtime for OpenCL, 4 cores at 2.8 GHz — so the ceiling here is four
cores plus the runtime's 16-way vectorization, not a GPU.

| image | host (`-C`) | device | | output, host | device | |
| --- | --- | --- | --- | --- | --- | --- |
| `t24.bmp` 320x240x3 | 14.4 s | 6.8 s | 2.1x | 98,862 | 99,001 | +0.14% |
| `x_ep.bmp` 705x800x4 | 163.3 s | 48.7 s | 3.4x | 321,332 | 322,454 | +0.35% |

At a fixed iteration count (`-DMAX_ITER=8 -DEXTRA_ITER=99`, so both runs do the
same number of passes) `x_ep.bmp` is 108.0 s against 46.0 s, 321,949 against
322,454 bytes.

Device time on that encode, by kernel:

```
predict_cube    3.55 s     5750 launches    618 us each
pixel_cost      2.15 s     5750 launches    374 us each
coef_scan       5.64 s     1580 launches   3572 us each
group_dp        3.08 s       18 launches 171319 us each
group_hist      0.73 s       18 launches  40563 us each
pmodel_cost     0.34 s       10 launches  33785 us each
coef_apply      0.35 s     4052 launches     85 us each
```

The 33 s that is not on the device is the first loop's class search, the
predictor fit, and the full-image `PredictRegion`/`CalcCost` between passes.

### Why the output is not identical

Within a few tenths of a percent, in either direction, and the file still
decodes to the original bit for bit — `t.sh` checks exactly that.

Two things differ from the host path. The larger one is in the class search: a
pixel's activity, which picks its probability model, is a weighted sum of the
errors of its causal neighbours, and inside a block those neighbours belong to
whichever sub-block the recursion has just decided on. The cube has to fix a
class for the whole block to compute anything, so it answers as if the
neighbours had the class being tried. At the top of the quadtree that is
exactly what the host does; further down it differs wherever two adjacent
sub-blocks end up with different classes. The smaller one is arithmetic:
sums that the host makes in one order, a parallel reduction makes in another.

Neither is a bias. On `x_ep.bmp` at a fixed iteration count, against the host's
321,949 bytes: with the class search left on the host it is 321,908, with the
coefficient sweep left on the host it is 321,953, with both on the device it is
322,454. Either one alone lands on the host's answer to within tens of bytes,
including one that is *smaller*; together they perturb the search into a
slightly different local optimum. The group search is not in that list because
it is bit-exact — its histogram bins and its dynamic program are in double on
the device precisely because in float they were not, and that cost 0.7% of the
output on a small image.

`-C` gives the reference result, and is what to compare against when measuring
anything else.

## Command line

```
mrpc [options] <mode> <input> <output>
mrpc -l

  <mode>    'c' compress, 'd' decompress

  -l        list the OpenCL platforms and devices, and exit
  -d <n>    use device <n>, numbered as -l prints it
  -p <n>    only look at platform <n> (and number -d within it)
  -T <t>    pick by type instead: cpu, gpu, acc
  -C        do not use OpenCL at all -- the reference code path
  -V        report what the device compiler had to say, and the device
            time per kernel at the end
```

With no `-d` or `-T` the first GPU is used, or the first CPU device if there is
no GPU. Naming a device that is not there stops with status 8; not finding one
when none was asked for falls back to the host with a note on stderr. So does
anything that goes wrong with the device later — the state is consistent at
every block boundary, and the host path is still there.

```sh
mrpc -l                          # what is available
mrpc -d 1 c in.bmp out.mrp       # on device 1
mrpc -T cpu -V c in.bmp out.mrp  # on a CPU device, with the timings
mrpc -C c in.bmp out.mrp         # reference
mrpc d out.mrp back.bmp          # decode: host, always
```

## Build

```sh
make            # mrpc, with OpenCL
make mrpc-nocl  # without: no OpenCL headers or loader needed
make devices    # build and list
```

Needs an ICD loader and headers — on Debian/Ubuntu `ocl-icd-libopencl1` and
`opencl-headers` — plus a runtime for whatever it should run on. For the CPU
that is Intel's, from the oneAPI apt repository:

```sh
sudo apt-get install intel-oneapi-runtime-opencl ocl-icd-libopencl1 opencl-headers
```

### Windows

```sh
make windows    # mrpc.exe, cross-built with MinGW-w64
```

Needs `g++-mingw-w64-x86-64` and the Khronos headers (`CL_HEADERS`, staged
into `winsdk/` — handing the cross compiler the whole of `/usr/include`
hands it glibc's headers too). `WINARCH` defaults to `haswell`, matching
`gc.bat`, so the executable wants AVX2.

There is no import library for the ICD loader on Windows, and an executable
that imports `OpenCL.dll` statically will not start at all on a machine
without it — which would put `-C` out of reach exactly where it is needed.
So the loader is opened by hand at startup and a missing one is just one
more reason to run on the host. `mrpc.exe` imports nothing but `KERNEL32`
and `msvcrt`: no OpenCL, no runtime DLLs.

`gc.bat` is the original Windows/clang build; it carries a commented line
showing what to add for OpenCL if you build there natively.

Compile-time switches, on top of the ones mrpc already had: `MRP_OPENCL` turns
the whole thing on, and `MRP_CL_CLASS`, `MRP_CL_COEF`, `MRP_CL_GROUP` (all 1)
take one search off the device without taking the device out of the build,
which is how the table above was measured.

## Testing

```sh
./t.sh img.bmp [img.bmp ...]
```

Encodes each image on the device and on the host, decodes both, checks each
against the input, and reports what the two cost. It fails if the device
quietly fell back to the host, which is the failure mode a size comparison
would otherwise hide.

The host path is byte-identical to the unported mrpc — checked on `t24.bmp`
and on 24- and 32-bit images with sizes that are not multiples of the block
size, which is where the clipped blocks at the right and bottom edges get
exercised.

## Files

| | |
| --- | --- |
| `mrpc.cpp` | the codec; the device paths sit next to the host ones they mirror |
| `mrpc_cl.inc` | device selection, buffers, launches — the whole OpenCL host side |
| `mrpc_kernels.inc` | the kernels, as a string, built at run time |
| `sh_common.inc`, `sh_v2f.inc` | the shared front end and rangecoder |
| `Makefile`, `gc.bat` | Linux and Windows builds |
| `t.sh` | round-trip and reference check |
