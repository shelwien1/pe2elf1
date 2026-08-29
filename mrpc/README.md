# mrpc on OpenCL

`mrpc` is a lossless RGB/RGBA image compressor on MRP's scheme: it segments the
image over a quadtree, fits a linear predictor per (class, colour component),
picks a probability model per pixel by local activity, and optimises all of
that against measured code length. It is a two-pass coder, and the second pass
is where the hours go.

The encoder's optimisation loop runs on an OpenCL device. The codec is a
library — `mrpc_lib.h`, in-memory, C linkage — and `mrpc.cpp` is a command line
on top of it that knows about BMP and files so the library does not have to.

## The library

```c
#include "mrpc_lib.h"

mrpc_opts opt = {0};
opt.use_opencl = 1;
opt.platform = opt.device = -1;

mrpc_ctx* c = mrpc_init(&opt);

mrpc_image img = {0};
img.width = w; img.height = h; img.ncomp = 3; img.stride = stride;
img.data = raster;

mrpc_blob out;
mrpc_compress(c, &img, hdr, hdrlen, 0, 0, &out);   /* raster -> blob */
...
mrpc_free(out.data);
mrpc_quit(c);
```

and back:

```c
mrpc_image img; mrpc_blob head, tail;
mrpc_decompress(c, blob, bloblen, &img, &head, &tail);
/* img.data, head.data, tail.data are yours; mrpc_free each */
```

and, instead of the stream, a picture of what it would have cost:

```c
mrpc_image plot; double bits;
mrpc_plot(c, &img, &plot, &bits);
/* same geometry; every component byte is that component's code length
   in 4.4 fixed point.  `bits` is the exact total before the rounding. */
```

* **One class does both directions.** `Codec` in `mrpc_lib.cpp` is the whole
  codec: `Init`, `Compress`, `Decompress`, `Quit`. `mrpc_ctx` is one of them.
* **Explicit lifetime, everywhere.** No constructors, no destructors, nothing
  that runs on its own: every type in the library has `Init` and `Quit`, and
  each is called by the thing that owns it. What used to be function statics —
  the group histogram, the class list, the sweep buffers — belongs to the
  context now, because a leak per call is a leak in a library. `t_lib` runs 60
  round trips through one context and watches the process not grow.
* **The device belongs to the context**, not to the image: `mrpc_init` opens it
  and builds the kernels, so a hundred images through one context compile
  nothing after the first.
* **The plot costs the encoder nothing.** `CodeImage` is a template on what
  the pass is for — encode, decode, cost, plot — and the dispatcher underneath
  it is the only place that is a run-time value, so the three modes that are
  not the encoder do not put a branch a symbol in the encoder. `mrpc_plot`
  runs the same search and the same coder as `mrpc_compress`; only the
  instantiation differs.
* **head and tail** are arbitrary bytes carried through the stream with an
  order-1 model — a file format's header and trailer riding along with the
  raster. `img` may be null, which compresses those alone: what a frontend does
  with a file it could not parse.
* The library **never exits, never prints to stdout, and touches no file**
  except the kernel cache, and only when asked. It is not thread safe: one call
  on one context at a time.

`make` builds `libmrpc.a` and the `mrpc` frontend against it; `make test`
builds `t_lib` — which is C, not C++, so that the header has to be what it
claims — and runs it.

### Shared

```sh
make libmrpc.so   # ELF, built -fvisibility=hidden
make exports      # what came out of it
make dll          # mrpc.dll and libmrpc.dll.a, cross-built
```

The ten functions in the header carry `MRPC_API`, and nothing else in the
library does, so `make exports` lists those ten and no more. On ELF the
attribute is `visibility("default")` and unconditional — it costs a static
build nothing and it is the thing that survives `-fvisibility=hidden`, which is
what leaves a shared library with no surface at all. Windows cannot do that,
because export and import are different keywords: define `MRPC_BUILD_DLL` when
building the DLL and `MRPC_DLL` when using one, and with neither the header
describes the static library. The exports are undecorated `__cdecl`, so
`GetProcAddress` finds them by the names in the header.

`mrpc.dll` imports `KERNEL32` and `msvcrt` and nothing else — no OpenCL, which
it loads at run time, and no MinGW runtime. Which CRT it holds is exactly why
`mrpc_free` exists: memory the library allocated has to go back to the
allocator that gave it, and across a DLL boundary the caller's `free` may not
be that one.

### 8bpp, grey and paletted

One component per pixel works and is lossless. The frontend takes an 8bpp BMP
and hands the index field over as a one-component raster; the palette needs no
special handling at all, because `bfOffBits` already points past it, so it
rides along in the head blob and comes back bit-identical without the codec
ever seeing it. `bmpn.py` makes both kinds out of a 24 or 32bpp file, which is
where the files below came from.

What the codec does with them is a different question, and the answer depends
on what the index means. Against `xz -9`, on the raster:

| | mrpc | `xz -9` | |
| --- | --- | --- | --- |
| `PIA13882_crop256_gray` 256x256 | **46,382** | 47,384 | mrpc 2% better |
| `PIA13882_crop256_pal` (256 colours) | 46,885 | **43,580** | xz 7% better |
| `t24_gray` 320x240 | **55,564** | 59,600 | mrpc 7% better |
| `t24_pal` (256 colours) | 48,334 | **30,796** | xz 36% better |

A grey index is a luminance: neighbouring pixels have neighbouring values, the
predictor works, and the codec is ahead. A palette index is a label, and
neighbouring pixels have neighbouring *labels* only by accident — there is
nothing for a predictor to find, while the repetition in a label map is exactly
what an LZ eats.

Nothing about this path expands anything to RGB: an 8bpp file goes in as one
component and is coded with the 20 same-component taps and no cross-component
ones, which is what MRP itself did. `-x` is the one thing that touches the
index field, and it only re-indexes it to the values that occur — order
preserved, and the palette in the head blob never looked at. `t24` is graphic art with flat regions, which is the worst
case for this and the best case for `xz`; the satellite photo is textured, and
the gap is small.

I thought palette *ordering* was the lever — sort the palette by luma and the
indices become monotone in brightness — and it is not: re-indexing both files
that way made mrpc **worse**, 48,581 against 46,885 and 52,525 against 48,334.
Luma is a poor one-dimensional embedding of a three-dimensional palette, and
median cut's own order already groups colours that occur together. If paletted
images matter, the thing to model is the index field as a label map, not the
palette's order.

### 1 and 4bpp, packed

Below a byte a BMP row is packed, and the codec reads bytes, so the frontend
widens a row on the way in and packs it again on the way out. What it widens is
the **whole row** — `stride*8/bpp` pixels, not `width` — because BMP pads every
row out to four bytes and those trailing bits are file content like any other.
Widening the row whole makes the map from row to raster onto, so the round trip
is exact by construction rather than by a separate blob spliced back in; the
padding arrives as up to seven extra columns of whatever it held, which is a
constant in every file anyone writes and costs nothing to code. Nothing else
changes: it is the same one-component path 8bpp uses, and the palette still
rides through in the head blob untouched.

The two depths behave very differently, and neither the way I expected:

| | | mrpc | `xz -9` | |
| --- | --- | --- | --- | --- |
| `DLRAW.bmp` | 816x1200 4bpp | **274,491** | 286,496 | mrpc 4% better |
| `f05_200.bmp` | 1728x2339 1bpp | **24,992** | 35,388 | mrpc 29% better |

A bilevel scan is the case where a predictive coder ought to be at its worst —
two values, so the residual is in {-1,0,1} and there is no magnitude to model —
and it is instead where the margin is largest, at 0.099 bits per pixel. What
carries it is the class map: a scanned page is enormous flat white with thin
structure on it, the quadtree isolates the text, and the per-class predictor
plus activity model amounts to a context model over the causal neighbourhood.
The 4bpp file is closer, and closer for the reason the 8bpp table already
gives — it is paletted graphic art, which is what `xz` is best at.

Widening is the frontend's business, so `mrpc_lib.h` is unchanged and the
stream format is unchanged: what the library sees is a one-component raster of
`stride*8/bpp` by `height`, and a decoder tells it apart from an 8bpp file by
the header that comes back in the head blob. Since the stream did not change,
the version byte did not either — which does mean a packed file written by this
build and decoded by an earlier one comes back with the widened raster in place
of the packed one, silently. Earlier builds could not encode these depths as
images at all (they fell through to the whole-file blob, which this build still
decodes), so the only way to meet that is to mix the two. A `p` plot of a packed file gets a
fresh 8bpp header of the widened geometry, since a code length in 4.4 fixed
point does not fit in four bits, let alone one.

### The stream is self-describing now

It has to be: a library that takes a raster cannot recover the geometry by
parsing a BMP header the way the old container did. Width, height, component
count, stride and the head and tail lengths are coded into the stream, in
variable-length fields. That costs three to five bytes against the old
container, and **files written by earlier builds do not decode** — the frontend
writes the library's blob and nothing else.

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

**The first loop's class search** (`batch_block`). The first optimisation loop
cuts the image into `BASE_BSIZE` blocks with no quadtree under them, so a
block is one argmin over classes and there are sixteen times as many of them
as there are quadtree blocks. One launch each never paid for itself and they
were left on the host, where they were the largest single item in the encode:
28 GMAC an iteration on one core. They now go a row of blocks at a time —
every block in the row, every class, one launch — and one work-item is one
(block, class): it predicts the block and charges for it in a single raster
pass, keeping the four error rows the activity measure reaches in private
memory, so nothing leaves the work-item but one float. The decisions stay on
the host in raster order, so the move-to-front state the class costs come from
is what it always was; what is approximated is the halo, which is committed
above the row and one pass stale within it. Splitting it into a predict pass
and a cost pass over global cubes measured 2.3x slower: the halo cells have no
prediction to make but sit in the same vector as the ones that do, and the
cubes are gigabytes of traffic an iteration.

**The group quantiser** (`group_hist`, `dp_prefix`, `dp_stage`, `dp_back`,
`pmodel_cost`). The histogram is one work-item per (class, component, group) —
no atomics, no local memory. The threshold search after it is a 514-state,
16-stage shortest path, one per (class, component). Run as one work-item per
problem that is 252 of them — four cores' worth at best, and it was the
largest item on the device for a small image. It does not have to be serial:
within a stage every candidate threshold reads the previous stage's costs and
writes only its own, so a stage is fully parallel and the launch boundary is
the barrier. As sixteen staged launches over (threshold, component, class) it
is thirty thousand work-items instead of 252, and bit-identical: the serial
tie-break — scan upward from `th0 == th1`, take a candidate only on a strict
improvement — is reproduced by each work-item for its own threshold. Measured
on a 256x256 image: 1.79 s to 0.42 s, byte-identical output.

**What is a constant and what is an argument.** The kernels take the image's
component count, the row stride of its bordered raster and the size of a plane
as arguments, so one built program encodes any image. What stays on the `-D`
line is the codec's own geometry — tap counts, table strides, the padding, the
block size the cube is cut into — because those are the strides the device
compiler folds into addresses: passing *them* as arguments measured 4x slower,
since it can no longer prove that neighbouring work-items touch neighbouring
memory. The tap loop runs to `NTMAX` rather than to the image's actual tap
count for the same reason, with zero coefficients past the end; on a
3-component image that is about 6% more prediction work, and it buys a program
a 4-component image can use too. Measured against building per image: no
difference in wall clock on either, and identical output.

Everything else stays on the host: the rangecoder, the quadtree bookkeeping,
and the Gauss-Jordan solve.

## Results

Intel CPU Runtime for OpenCL, 4 cores at 2.1 GHz -- so the ceiling here is four
cores plus the runtime's 16-way vectorization, not a GPU.

| image | host (`-C`) | device | | output, host | device | |
| --- | --- | --- | --- | --- | --- | --- |
| `t24.bmp` 320x240x3 | 13.3 s | 4.3 s | 3.1x | 98,868 | 99,245 | +0.38% |
| `PIA13882_crop256.bmp` 256x256x3 | 11.6 s | 4.1 s | 2.8x | 121,182 | 121,423 | +0.20% |
| `20000171A.bmp` 4096x512x4 | 516.3 s | 101.5 s | 5.1x | 2,754,981 | 2,751,292 | -0.13% |

The large image is where the parallel form pays: the encode is long enough for
the searches to dominate everything around them, and the first loop's class
search -- the batched one -- is 28 GMAC an iteration that used to be one core's
work.

Those output sizes predate the residual correction and the per-image
coefficient clamp, which are worth **2.26 % on the mean** of the reference
corpus (`MODEL-IMPROVEMENTS.md` §14); the times are within a few percent of
what the codec does now, since neither change touches the search. The
device-against-host comparison is what this table is for, and that is
unaffected.

Device time on that encode, by kernel:

```
predict_cube   20.22 s    34816 launches    581 us each
batch_block    18.55 s      384 launches  48320 us each
pixel_cost     10.59 s    34816 launches    304 us each
coef_scan       8.09 s     2686 launches   3012 us each
group_hist      1.70 s       23 launches  73738 us each
coef_apply      1.43 s    20666 launches     69 us each
dp_stage        1.37 s      345 launches   3961 us each
pmodel_cost     0.90 s       17 launches  52719 us each
dp_prefix       0.03 s       23 launches   1306 us each
dp_back         0.00 s       23 launches     49 us each
```

63 s of the 98 s that encode spends after the header, against 19% before the
first loop's class search and the threshold DP moved across. The launch count
went the other way -- 458,752 launches for the first loop's blocks if each one
is its own launch, 384 if a row of blocks is -- which is the whole point: a
launch is about 10 us of nothing, and there were more of them than there was
arithmetic to cover them.

What is left on the host is the predictor fit (`FitPredictor`, 2 s an
iteration), the full-image `PredictRegion`/`CalcCost` between passes, and the
rangecoder.

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

The batched first-loop search adds a third of the same kind: a block is
measured against the error planes as its row of blocks began, so the row above
it is committed but its neighbours to the left and right are one pass stale.

None of the three is a bias, and on a large image they are not even a loss --
`20000171A.bmp` comes out 0.13% *smaller* on the device than on the host,
because the perturbed search lands in a different local optimum and that one
happens to be better. The group search is not in the list at all because it is
bit-exact: its histogram bins and its dynamic program are in double on the
device precisely because in float they were not, and that cost 0.7% of the
output on a small image. The staged form of the DP is bit-exact against the
serial one as well, tie-break included.

`-C` gives the reference result, and is what to compare against when measuring
anything else.

### On a discrete GPU

This was written and tuned against a CPU device, and three things about it suit
that and not a graphics card:

* The second loop's class search syncs per block. Block *n+1*'s activity halo
  is block *n*'s committed errors, so each 32x32 block is a halo upload, two launches and a
  blocking readback of the cost cube, in that order, 550 times per pass. On a
  CPU device those are zero-copy and about 9 us; across PCIe they are the whole
  cost, and the kernels themselves — ten to sixty thousand work-items — do not
  cover the latency.
* The threshold DP is thirty thousand work-items now rather than 252, but every
  one of them is double precision, and on a consumer card that runs at the 1/64
  fp64 rate. It stays on the host on anything that is not a CPU device, which
  also keeps a launch from being long enough to trip Windows' two-second driver
  timeout and take the driver down with it.
* `batch_block` keeps a 208-short error tile per work-item in private memory.
  On a CPU device that is stack; on a GPU it is registers, and at that size it
  will spill to local memory, which is slower but still local. A GPU port would
  want the block's tile in `__local` shared by a work-group of classes instead.

The first loop's class search is no longer on that list: it syncs once per row
of blocks rather than once per block, which is 64 launches an iteration on the
image above instead of 32,768, and each one is 48 ms of arithmetic. That much
does cover PCIe.

So `-T cpu` may well beat `-d <gpu>` on the same machine. Making the quadtree
class search suit a GPU means not synchronising per block -- the same move
`batch_block` makes for the first loop, but with a quadtree under each block to
keep consistent, which is a design change and not a tuning one.

## Command line

```
mrpc [options] <mode> <input> <output>
mrpc -l

  <mode>    'c' compress, 'd' decompress, 'p' plot

  -l        list the OpenCL platforms and devices, and exit
  -d <n>    use device <n>, numbered as -l prints it
  -p <n>    only look at platform <n> (and number -d within it)
  -T <t>    pick by type instead: cpu, gpu, acc
  -C        do not use OpenCL at all -- the reference code path
  -k        cache the compiled kernels in the working directory
  -n <n>    use <n> predictor classes (2..63) instead of the number the
            image size suggests
  -t        encode the image both ways round and keep the smaller file
  -x        also try re-indexing each plane to the values it uses
  -V        report what the device compiler had to say, and the device
            time per kernel at the end
```

`p` encodes the image exactly as `c` does — same search, same model, same
stream — throws the stream away, and writes a BMP of the same geometry
instead.  Every component byte is the code length of that component in 4.4
fixed point: sixteenths of a bit, saturating at 15.9375.  So the picture is a
map of where the file's bits went, channel for channel, and it is a real map
rather than an estimate — the exact figure it prints agrees to the byte with
what the encoder's own model trial measured.

```
$ mrpc -C p t24_0.bmp plot.bmp
mrpc: 61440 symbols, 116972 bits = 14621 B raster, 14647 B exact, mean 1.907 b/symbol, 4 saturated
```

The raster reads slightly low because a symbol costing less than a
thirty-second of a bit rounds to a zero byte, and on a flat plane most of
them do; the exact figure is what the coder charged.  The difference between
that and the file `c` writes is the class map, the predictors and the
thresholds.  A one-component plot gets a grey ramp for a palette, since a
code length is not a palette index and the plot is meant to be looked at.
What the plot file carries is the input's header and a raster the same shape;
anything that trailed the pixel data in the input does not come along.  `-t`
applies: the plot is of whichever orientation won, mapped back the right way
up.

### Planes that skip levels

MRP's probability model is a generalized Gaussian over the integers and its
resolution is one level.  That is fine while a plane uses its levels.  It is
not fine when the plane is four bits widened to eight — every sixteenth
residual is then the only one that can occur, and the model has no way to say
so, so it spreads its mass over all of them and pays log2 of the spacing on
every symbol.  Measured, on a 128x128 RGB tile quantised to four bits a
component:

| | as it stands | levels closed up |
| --- | --- | --- |
| `v = k*16`, low nibble always zero | 34,992 | **10,660** |
| `v = k*17`, four bits replicated | 35,334 | **10,660** |

Both forms occur in the wild and they are equally bad, which is the first
thing to say about the obvious fix.  A mask of the always-zero and always-one
bits catches `k*16` — the low nibble is constant — and does nothing at all for
`k*17`, which has no constant bit anywhere.  The other half of the mask, the
constant *high* bits, is catchable and measured harmful: see below.  What closes
both is the **gcd of the gaps** between the values a plane uses.  Every
component now goes through `v -> (v - lo%g) / g` on the way in and its inverse
on the way out, with `lo%g` and `g` in the stream.  It is a change of units,
so the predictor is unchanged in shape and the gain is exactly `log2 g`.  On
the reference corpus every plane has `g == 1`, the map is the identity, and
the whole thing costs the one symbol in `params` that says so — a byte on the
file, and not one image of the twenty-four moves otherwise.

Only the scale, never the offset — which is where the always-one bits of a
mask would have gone.  Subtracting `lo` outright is not free: 128 is written
into the borders and into the working buffer before the first pixel is read,
and a prediction is clamped to [0,255], so moving a plane's values away from
the middle costs something.  On planes with nothing to gain from it, it
measured **+2.8 %** on `piag_0` (which starts at 22) and **+12.9 %** on
`t24p_1`.  So `lo` contributes only the remainder that makes the division
exact, and a plane whose top bit is always set is left where it is.

`-x` goes further: re-index each plane to the **values it actually uses**,
rank order, 256 bits in the stream.  That closes up a plane whose levels are
near a lattice without being on one, which is most quantised imagery, and it
is worth a great deal there — but it is not a change of units, it warps what
a linear predictor sees, and nothing measurable separates the images it helps
from the ones it hurts.  Forced on, it wins on 15 of the 24 corpus tiles by
up to 31.5 % and loses on 7 by up to 13.7 %.  So it is a trial: two more
encodes, keep the smaller, **-7.33 % on the mean and -7.03 % on the total**,
and it cannot lose on any image.

| | `-x` | |
| --- | --- | --- |
| `pia_*` quantised space imagery | −26.6 % to −31.5 % | 100 of 256 levels used |
| `big*` 32bpp | −1.6 % to −9.1 % | |
| `t24_1`, `t24_2`, `t24_3` graphic art | −6.1 % to −12.5 % | 64 levels in one plane |
| `t24_0`, `t24p_1` | +13.7 %, +9.0 % forced | the trial declines them |

The two trials compose: `-t -x` is four encodes.

`-t` runs the whole search twice, once on the image and once on its
transpose, and keeps whichever came out smaller.  Nothing about the taps, the
activity neighbourhood or the quadtree is symmetric, so a picture with
vertical structure is not the same picture to this codec as one with
horizontal structure.  On the reference corpus the transpose is smaller on
ten of twenty-four tiles, by up to 4.8 %, and larger on the rest, by up to
39 %; as a trial it is worth **0.41 % on the mean** and cannot lose on any
image.  It costs two encodes — measured at 2.2-2.4x — and nothing at all to
decode, which is why it is not the default.  `MODEL-IMPROVEMENTS.md` §14 has
the per-image numbers.

`-n` sets the class count directly.  The default comes from the image size,
which is demonstrably not where the right answer lives -- see `TUNING.md` --
so on an image worth the trouble, sweep it.

`-k` caches the compiled kernels in the working directory as
`<device name>.!cl` — `NVIDIA_GeForce_RTX_3090.!cl` — which saves the second or
three the device compiler costs on every run. One file serves every image on
that device: the kernels do not depend on the image (see below). The header
records hashes of the kernel source, the build options, the device and driver
versions, and the binary itself; anything that does not match means compiling
from source and overwriting the file. A directory it cannot write to is not an
error, just no cache.

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
| `mrpc_lib.h` | the library's whole surface: image in, blob out, C linkage |
| `mrpc_lib.cpp` | the codec; the device paths sit next to the host ones they mirror |
| `mrpc_cl.inc` | device selection, buffers, launches — the whole OpenCL host side |
| `mrpc_kernels.inc` | the kernels, as a string, built at run time |
| `sh_common.inc`, `sh_v2f.inc` | the shared helpers and the rangecoder, which codes to memory as well as to a file |
| `mrpc.cpp` | the frontend: options, BMP, files, and nothing else |
| `Makefile`, `gc.bat` | Linux and Windows builds |
| `t.sh` | round-trip and reference check, through the command line |
| `t_lib.c` | the same through the C API, in C — and against `libmrpc.a`, `libmrpc.so` or `mrpc.dll` unchanged |
| `bmpn.py` | 1, 4 and 8bpp grey and paletted BMPs out of a 24/32bpp one, for testing the one-component path and the packed depths |
| `ALGORITHM.md` | how the codec works, end to end |
| `ENTROPY.md` | where the bits go: the rangecoder, the probability model, and what a PAQ-style component would be worth |
| `TUNING.md` | the class count and the shape of the model: what was measured, what moved, and what did not |
| `MODEL-IMPROVEMENTS.md` | where the model could compress better, read against Shkarin's BMF |
| `tools/entropy_probe.patch` | the measurement harness those numbers come from |
| `tools/border_probe.patch` | every border rule, costed with the model held fixed |
| `tools/nlms_probe.patch`, `tools/resid_fit.py` | a learned correction on the predictor, and why there is nothing for it to learn |
