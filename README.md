# coder — sh_v1xN framework on the sh_v1xN_s.cpp rangecoder

A port of the `069-lowl+lowh` coder onto the scalar rangecoder from
`sh_v1xN_s.cpp`. The framework, the model and the compressed format are the
originals; the SIMD rangecoder and the perl passes that generated it are gone,
replaced by RCNUM plain instances of the scalar coder — and the encoder's
carryless coding pass optionally runs as an OpenCL kernel, where the device
compiler does the vectorising the perl pass used to set up.

```
./build.sh
./coder c book1 1 FSM0.txt     # encode
./coder d 1     2 FSM0.txt     # decode
cmp book1 2

./coder -l                     # what devices are there
./coder -V c book1 1 FSM0.txt  # encode on one, with the timings
./coder -C c book1 1 FSM0.txt  # the reference path, no device
```

## What changed, and what did not

**Gone: the perl preprocessor.** `Lib3/rc_macro.pl` turned `sh_v1xN.inc` into
`sh_v1xN_macro.inc` — every method rewritten as a `#define` — and `defines.pl`
split that into `_D.inc` / `_U.inc` pairs that `model0.inc` included in the
middle of `do_process`. The point was to inline the RCNUM-lane arrays
(`range[RCNUM]`, `lowc[RCNUM]`, …) as *locals*, so the compiler would put one
lane in each element of a ymm register. `rc_vec.inc` holds RCNUM ordinary coder
objects instead, so there is nothing to generate: `c.bat` just compiles.

**Gone: the vector rangecoder.** `sh_v1xN.inc` and `sh_v1xN_c.inc` are replaced
by `rc.inc`, and the `cl`-write / `tmpbase` / reverse-pointer machinery they
needed goes with them (the original wrote each substream backwards through a
descending pointer; here `RC_IO` writes forward).

**Kept: everything else.** The coroutine framework and `coder.cpp` main loop,
the FSM counter model and `Predictor`, the probability cache, the carryless →
carry-propagating fallback, and the compressed format.

## Layout

| file | what |
|---|---|
| `rc.inc` | the `Rangecoder` class out of `sh_v1xN_s.cpp` |
| `rc_io.inc` | `RC_IO` (per-lane byte cursor) and `RCio` (block I/O, substream size headers) |
| `rc_vec.inc` | `RangecoderN` — the RCNUM-lane wrapper, based on `sh_v1xN.inc` |
| `rc_config.inc` | block geometry and the rangecoder knobs |
| `rc_cl.h`, `rc_cl.cpp` | device selection, buffers, launches — the whole OpenCL host side |
| `rc_kernel.inc` | the coding kernel, as a string, built at run time |
| `model.inc` | includes `rc.inc` twice, once per carry mode |
| `model0.inc` / `model1.inc` | encoder / decoder |
| `predict.inc`, `counter.inc`, `FSM.cpp` | the model, unchanged |
| `coder.cpp` | the framework, unchanged apart from portability |

### `rc.inc`, included twice

`model.inc` does:

```c
#define RC_CARRYLESS 1
#define Rangecoder Rangecoder_CL
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#define RC_CARRYLESS 0
#define Rangecoder Rangecoder_CY
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS
```

`RC_CARRYLESS` only reaches `ShiftLow` and `rc_Quit`, so the two classes differ
only in how the encoder flushes; the decode path is identical, which is why one
decoder handles output from either.

`f_DEC` became a template parameter rather than the member it is in
`sh_v1xN_s.cpp`, because `model0.cpp` and `model1.cpp` are separate translation
units specialised on it. The other deviation: a lost carry is reported through
`carry_lost()` instead of being fatal — in a single-pass stream demo there is
nothing to fall back on, here there is.

### `RangecoderN` — the fake vector

`sh_v1xN.inc` kept one array per field so a lane could live in each SIMD
element. `rc_vec.inc` keeps RCNUM independent scalar coders. Same interface,
same lane assignment, same substream layout — only the vectorisation is gone.

### The carry fallback

The carryless coder drops carry propagation from the hot path entirely: no
`Cache`, no FF run, the top byte goes straight out and a carry only raises a
flag. That is only safe because `RC_LOWBYTES` is 8 — a carry then has 4 bytes
of headroom to travel through inside the register before it can escape into
already-emitted output, making it a ~2⁻³³ event per step.

When one does escape, `model0.inc` re-codes the whole block with the
carry-propagating twin. That is what the probability cache buys: `pbit[]` holds
`{p;bit}` for every bit of the block, so the second pass re-runs the coder and
nothing else — the model is never touched twice.

Lowering `RC_LOWBYTES` is the cheap way to exercise that path. On `book1`:

| `RC_LOWBYTES` | blocks re-coded | output |
|---|---|---|
| 4 | 12 / 12 | 456441 |
| 5 | 12 / 12 | 456441 |
| 6 | 2 / 12 | 456441 |
| 7 | 0 / 12 | 456441 |
| 8 | 0 / 12 | 456441 |

Identical output either way, which is the invariant: the two coders hand `put()`
the same byte sequence, just offset by one step.

## On the device

The encoder's carryless coding pass, and nothing else.

That pass was already the parallel one: bit *i* of a block goes to lane
*i % RCNUM* and no lane reads another's state, which is what `sh_v1xN.inc`'s
hand-written SIMD version was built on and what the perl macro pass existed to
set up. `rc_kernel.inc` is the same coder as one work-item per lane, and Intel's
CPU runtime reports `Kernel "rc_encode" was successfully vectorized (16)` — the
vector rangecoder, from scalar source.

**The output is byte-identical to the host coder.** It is the same integer
arithmetic in the same order, so anything else would be a bug, and `t.sh`
compares a device encode against `-C` byte for byte rather than by size.

What stays on the host:

* **The carry-propagating twin.** It is a serial `Cache`/FF-run state machine
  with a data-dependent inner loop, and it only runs on the blocks the fast
  path had to hand back. The device reports each lane's escaped-carry count
  along with its length, and a non-zero one re-codes that block on the host
  exactly as before — the fallback did not change, it just has a second caller.
* **The decoder**, entirely. A decoder that needs a device to read a file is
  not a decoder.
* **The model.** It is serial by construction: each bit's probability depends
  on the ones before it.

`RC_FORCE_CARRY=1` takes the device out of the build, since only the carryless
coder has a kernel.

### What it costs and buys

A 4-core CPU device at 2.1 GHz — so the ceiling is four cores plus the
runtime's vectorization, not a GPU. Best of three runs of five iterations each,
host and device alternating:

| file | host | device | | output |
| --- | --- | --- | --- | --- |
| `book1` 768,771 | 18.94 MB/s | 42.44 MB/s | 2.24x | 456,441, identical |
| 10,000,000 (text + random) | 14.14 MB/s | 43.24 MB/s | 3.06x | identical |

Per launch, on the 10 MB file: 153 blocks, 900 µs of device time each, of which
the `pbit` upload is about 12%.

The work-group size is the whole trade, because a group is vectorised across
its work-items *and* scheduled on one compute unit. At RCNUM=16 on this device:

| work-group | groups | | |
| --- | --- | --- | --- |
| 16 | 1 | 16-way vector, one core | 1.7x |
| 8 | 2 | | 2.2x |
| 4 | 4 | 4-way vector, four cores | **3.0x** |
| 2 | 8 | | 3.0x |
| runtime's choice | | | 1.7x |

Cores win over vector width, because the lanes' byte writes are a scatter —
RCNUM rows `OUTSTRIDE` apart — and widening that does not help. So the default
is one work-group per compute unit, rounded to something that divides RCNUM;
`RC_CL_LWS` overrides it.

Raising RCNUM does not help either: it costs output (+0.1% at 32, +0.3% at 64,
from the extra flushes) and measures slower, because the work per launch is the
same and only the number of lanes changes.

### What is left on the table

* **The model pass and the coding pass could overlap.** Block *n+1*'s model
  pass depends only on the input, not on how block *n* was coded, so the
  kernel could run while the host models the next block. That is the single
  biggest win still available — it would hide most of the device time — and it
  needs a second `pbit` buffer and a second set of substream rows.
* **The scattered writes.** Interleaving the lanes' output and de-interleaving
  it on the host is the `clset` experiment in the original `RC.txt`; it is what
  the 7 scatters in the vectorizer's report are.
* **`pbit` is copied to the device every block.** It is 12% of the launch. A
  `CL_MEM_USE_HOST_PTR` buffer with map/unmap would be free on a CPU device.
* **No kernel binary cache**, so the device compiler costs about 0.25 s per
  run. The program is built before the timing loop, so it does not land in a
  measurement, but it is still a quarter second.

### On a discrete GPU

Untried. The launch is RCNUM work-items — sixteen — which is nothing for a GPU,
and each block is an upload, a launch and a blocking readback. Making this suit
a graphics card means many blocks in flight at once, which is the pipelining
above and then some.

## Compressed format

Unchanged from `sh_v1xN_io.inc`. Per block:

```
RCNUM x  2-byte little-endian substream length
RCNUM x  substream payload, in lane order
```

The block length is not in the header — it is coded *inside* the rangecoder
data, as one flag bit (`blksize != BLKSIZE`) plus 16 bits when the block is
short. `BLKSIZE` itself does not fit in 16 bits, hence the flag. A short block
ends the stream, so a file that is an exact multiple of `BLKSIZE` is followed by
an empty block.

Bit *i* of a block is coded by lane *i % RCNUM*, so the lanes are independent
and the decoder can walk the bits in order while pulling each from its own
substream.

## Configuration

Everything lives in `rc_config.inc` and can be overridden from the command line:

```
./build.sh -DRC_RCNUM=8 -DRC_LOWBYTES=6 -DRC_TRACE_FALLBACK=1
```

| macro | default | what |
|---|---|---|
| `RC_BLKSIZE` | `1<<16` | bytes per block |
| `RC_RCNUM` | 16 | substreams per block |
| `RC_LOWBYTES` | 8 | width of the low accumulator, 4..8 — the carry headroom |
| `RC_LOWSPLIT` | 1 | hold `low` as two uints instead of one qword (stream-neutral) |
| `RC_RANGE64` | 0 | 1 = `range` starts at `1<<32` instead of `0xFFFFFFFF` |
| `RC_RENORM_TAIL` | 0 | renorm tail loop; dead for a binary coder |
| `RC_FORCE_CARRY` | 0 | 1 = always use the carry twin, never the fast path |
| `RC_TRACE_FALLBACK` | 0 | 1 = report every re-coded block |
| `RC_STATS` | 0 | per-substream carry counts to stderr |
| `RC_CARRYLESS_WARN` | 0 | the demo's "lost carries" warning |
| `RC_IO_CHECK` | 1 | bounds-check encoder writes |
| `RC_STRICT_BLKSIZE` | 0 | reject an unsafe `BLKSIZE`/`RCNUM` pair at compile time |
| `RC_OPENCL` | auto | 1 = build the device path (`build.sh` sets it when `CL/cl.h` is there) |
| `RC_CL_LWS` | 0 | work-group size; 0 = one group per compute unit |
| `CORO_FAKE` | 0 | 1 = straight-through instead of the setjmp coroutine |

`BLKSIZE` and `RCNUM` are enums, not macros, because `rc_io.inc` and
`rc_vec.inc` take template parameters of those names — hence the `RC_` prefix on
the overrides.

### The `BLKSIZE` / `RCNUM` limit

A substream length is two bytes, so a lane cannot emit more than 65535 bytes per
block. Worst case is `SCALElog` = 15 bits per coded bit (`p` is clamped to
`[1, SCALE-1]`), i.e. `blkmax = (0xFFFF - RC_LOWBYTES - 96) * RCNUM / SCALElog`
— 4362 at `RCNUM=1`, 69793 at `RCNUM=16`.

That worst case means a model that is maximally wrong about every bit, so the
usual geometries are nowhere near it: a lane runs about `BLKSIZE/RCNUM` bytes
even on incompressible input. The one config that trips on ordinary random data
is `RCNUM=1` at `BLKSIZE=64K`, where a whole block lands in one lane and clears
65535 by a byte.

A lane can never overrun its buffer — it is sized to the smaller of the worst
case and the header cap, and `model0.inc` reports the overflow and stops rather
than emitting a stream that will not decode. `-DRC_STRICT_BLKSIZE=1` turns that
into a compile-time error instead. (This is the "worst-case model can output 15
bytes per input byte" note in the original `RC.txt`; the honest fix would be for
the encoder to snapshot the model, shrink the block and retry, which the
probability cache already makes cheap — but that changes block segmentation, so
it is left out.)

## Building

**Linux / gcc:** `./build.sh [extra -D flags]`. It builds the OpenCL path when
`CL/cl.h` is present; `OPENCL=0 ./build.sh` leaves it out and links against
nothing extra.

The device path needs an ICD loader, headers, and a runtime for whatever it
should run on. For a CPU device that is Intel's, from the oneAPI apt
repository:

```sh
sudo apt-get install ocl-icd-libopencl1 opencl-headers clinfo
# Intel's CPU runtime is not in the distro repositories:
curl -fsSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
  | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg >/dev/null
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
  | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt-get update && sudo apt-get install intel-oneapi-runtime-opencl
clinfo -l        # Platform #0: Intel(R) OpenCL -- Device #0: ... CPU
```

**Windows:** `g.bat` (MinGW), `gc.bat` (clang-cl), `c.bat` (ICL/ICX). `c.bat` no
longer runs perl. These build without OpenCL; add `-DRC_OPENCL=1` and
`rc_cl.cpp` to turn it on. There is no import library for the ICD loader on
Windows and an executable that imports `OpenCL.dll` statically will not start
on a machine without it — which would put `-C` out of reach exactly where it is
needed — so `rc_cl.cpp` opens the loader by hand there, as `mrpc_cl.inc` does.
That path compiles but has not been run here.

`Lib3/file_api.inc` picks `file_api_win.inc` on `_WIN32` and `file_api_std.inc`
elsewhere; force the portable one with `-DFILE_API_STD`.

The host coder is scalar now, so the `target("avx2,…")` attributes on
`do_process` are gone.

## Testing

```
./t.sh "" "-DRC_RCNUM=8" "-DRC_LOWBYTES=5" "-DRC_FORCE_CARRY=1"
```

Each config is built and round-trips a set of files — empty, 1 byte, exactly
`BLKSIZE`, `BLKSIZE+1`, `2*BLKSIZE`, all-`0xFF`, all-zero, 300KB random, `book1`
— against both FSM tables, comparing byte for byte. Where there is a device, it
also encodes each file on it and checks that output against the host encode
byte for byte, and fails if the device quietly fell back — which is the failure
mode a size comparison would hide.

## Usage

```
coder [options] c|d input output FSM_file [n_iter] [test_output]

  -l        list the OpenCL platforms and devices, and exit
  -d <n>    use device <n>, numbered as -l prints it
  -p <n>    only look at platform <n> (and number -d within it)
  -T <t>    pick by type instead: cpu, gpu, acc
  -C        do not use OpenCL at all -- the reference code path
  -V        report the device and what its kernel cost
```

With no `-d` or `-T` the first GPU is used, or the first CPU device if there is
no GPU. Naming a device that is not there, or anything going wrong with one
later, falls back to the host coder with a note on stderr: the block format and
the fallback do not care where a block was coded.

`FSM_file` is the counter state machine `Predictor::Init` loads (`FSM0.txt`,
`nzcc.txt`); both ends need the same one. `n_iter` repeats the run for timing.
With `test_output` the coder encodes, decodes back and appends a line to
`log.txt`:

```
./coder c book1 1 FSM0.txt 10 2
```
