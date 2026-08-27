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

### Blocks in flight

Block *n+1*'s model pass depends only on the input, not on how block *n* was
coded, so the host can be modelling ahead while the device codes. `RC_CL_NBLK`
slots make that possible: one `{p;bit}` cache and one set of substream rows
each, filled in order and collected in order, so the output is still written
block by block in sequence. A block is only ever written after every block
submitted before it, whatever happened to either.

Two things had to be right for the pipeline to be real rather than notional,
and both are easy to get wrong quietly:

* **`clFlush` after submitting.** Without it a runtime is free to sit on the
  queue until something blocks on it, which is the pipeline not happening. The
  `-V` line counts how many blocks were already finished when they were
  collected — without the flush it was 0 of 153.
* **A second queue for the readbacks.** They are enqueued at collect time,
  behind everything already queued, so waiting for them on the submit queue
  waits for every block still in flight. That one `clFinish` cost the whole
  pipeline: every block was done when collected and it was still no faster.

### What it costs and buys

A 4-core CPU device at 2.1 GHz — so the ceiling is four cores plus the
runtime's vectorization, not a GPU. Encoding a 10 MB file (text and random),
host and device alternating, best of the repeats:

| | host | device | | already done when collected |
| --- | --- | --- | --- | --- |
| `RC_CL_NBLK` 1 (no pipeline) | 14.5 MB/s | 47.0 MB/s | 3.25x | 0 of 153 |
| 4 | 14.5 | 57.6 | 3.98x | 122 of 153 |
| 8 | 14.4 | 60.9 | **4.22x** | 141 of 153 |
| 16 | 14.3 | 62.5 | 4.39x | 148 of 153 |

Output byte-identical in every one. It plateaus around 8, which is the default;
each slot past that is about 2 MB of host memory and as much again on the
device, for a couple of percent. On `book1`, which is twelve blocks rather than
153, the default is 19.7 MB/s against 60.3 — 3.06x, the pipeline having less to
fill.

The work-group size is the other trade, because a group is vectorised across
its work-items *and* scheduled on one compute unit. At RCNUM=16 on this device,
before the pipeline existed:

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

Raising RCNUM does not help: it costs output (+0.1% at 32, +0.3% at 64, from
the extra flushes) and measures slower, because the work per launch is the same
and only the number of lanes changes.

Two things measured and rejected. Partitioning the CPU device to leave a core
for the host thread (`clCreateSubDevices` by counts, 3 units of 4) cost more in
coding throughput than it bought in overlap: 32 MB/s against 45. And reading
each lane's substream back separately is RCNUM enqueues per block of a few
kilobytes each — all overhead — so `Collect` maps the slot's buffer once and
copies out of it instead.

### Opening the loader by hand

Windows has no import library to link the ICD loader against, and an executable
that imports `OpenCL.dll` statically will not start at all on a machine without
OpenCL — which would put `-C` out of reach exactly where it is needed. So there
the loader is opened by hand: two X-macro lists name every `cl*` entry point,
one generating the function pointers and one redirecting the calls onto them.

Everywhere else the real symbols are linked and that whole block is skipped —
so it is not compiled by the build the testing runs on, and a name added to one
list but not the other builds clean here and fails on Windows only. It has done
exactly that, twice over: `clFlush`, `clGetEventInfo`, `clEnqueueMapBuffer` and
`clEnqueueUnmapMemObject` all went in with the pipeline and none reached the
lists.

`RC_CL_DYNAMIC=1` forces the same arrangement onto Linux over `dlopen`, so
`./build.sh -DRC_CL_DYNAMIC=1` compiles it, links it — a missing redirect is an
undefined reference — and runs it against a real device. Keeping that
configuration in the `t.sh` sweep is what makes the lists checkable without a
Windows box.

### What is left on the table

* **The scattered writes.** Interleaving the lanes' output and de-interleaving
  it on the host is the `clset` experiment in the original `RC.txt`; it is what
  the 7 scatters in the vectorizer's report are.
* **`pbit` is copied to the device every block.** A `CL_MEM_USE_HOST_PTR`
  buffer would be free on a CPU device, but the map that makes it correct is a
  blocking command on an in-order queue, which is the pipeline undone again —
  it wants a queue per slot.
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
| `RC_LOWSPLIT` | 0 | hold `low` as two uints instead of one qword (stream-neutral) |
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
| `RC_CL_DYNAMIC` | 1 on Windows | open the ICD loader at run time instead of linking it |
| `RC_CODBYTES` | 4 | width of the code register |
| `RC_FF_TRIM` | 32 | most bytes the flush may leave for the decoder's 0xFF padding |
| `RC_FF_PADSIZE` | derived | that padding — must cover the trim plus `RC_LOWBYTES` |
| `CORO_FAKE` | 0 | 1 = straight-through instead of the setjmp coroutine |

`BLKSIZE` and `RCNUM` are enums, not macros, because `rc_io.inc` and
`rc_vec.inc` take template parameters of those names — hence the `RC_` prefix on
the overrides. `RC_CL_NBLK` is a constant for the same reason: it is an array
bound in three places. Change it in `rc_config.inc`.

### `RC_LOWSPLIT`

`sh_v1xN_s.cpp` defaults this on: in the vector coder two uint lanes map onto
one ymm each where a qword lane needs two. There is no vector coder here — the
host coder is scalar, and the kernel holds `low` as one `ulong` whatever this
says — so the reason is gone. Measured, best of five alternating rounds:

| | split=1 | split=0 | |
| --- | --- | --- | --- |
| carryless, `RC_LOWBYTES=8`, `book1` | 19.75 MB/s | 19.84 MB/s | -0.5% |
| carryless, `RC_LOWBYTES=8`, 10 MB | 14.37 | 14.56 | -1.3% |
| carry twin, `RC_LOWBYTES=4`, `book1` | 19.59 | 20.08 | -2.4% |
| decoder, any | — | — | none |

Output byte-identical in every pair, which is the point of calling it
stream-neutral. The decoder is untouched by construction: `low_Add`, `low_Top`
and `low_Shift8` are all under `if( f_DEC==0 )`. The carry twin at
`RC_LOWBYTES=4` loses most, because `LOWBITS` is 32 there and the split version
shifts a `lowh` that the mask then clears anyway. So the default is 0 here.

### The zero prefix

The encoder's first `RC_LOWBYTES - RC_CODBYTES` bytes are zero: additions land
in the bottom 32 bits of `low` while `ShiftLow` emits the top byte, so the top
has not been reached yet. That lag is the point of the wide accumulator, and
the prefix is not part of the stream.

`sh_v1xN_s.cpp` drops it with a counter tested inside `put()`. Here the row
carries it instead — `StartEncode` leaves `RC_SKIP` bytes in front of the
payload, `put()` stores every byte it is handed, and `rc_Write` starts each
lane at `beg`. Same bytes on the wire; one fewer test in the coder's only hot
store. The kernel lays its rows out the same way, so `Collect` copies from the
row start and `rc_Write` reads host-coded and device-coded rows identically.

A lane can stop *inside* the prefix and owe the stream nothing — 16 block-length
header bits spread over 64 lanes leaves most of them with no work at all — so
both `rc_len()` and the kernel's `outlen` floor at zero rather than wrapping.

It measured as roughly free on the host and about 3% on the device:

| | before | after | |
| --- | --- | --- | --- |
| host, `book1` | 18.99 MB/s | 18.78 | -1.1% |
| host, 10 MB random | 13.96 | 13.86 | -0.7% |
| device, `book1` | 60.77 | 62.22 | +2.4% |
| device, 10 MB random | 57.05 | 58.76 | +3.0% |

Output byte-identical throughout. No host gain was on offer: `if_e0` had
already pinned the branch as not-taken, and what is left is struct layout —
`RC_IO` lost a member, which moves every element of `RangecoderN`'s array.

### The trailing 0xFF run

The decoder reads `0xFF` past the end of a substream, because `rc_Read` pads it
with them, and that is what lets `rc_Quit` leave the tail of its flush out. The
carry-propagating coder's flush drops a whole trailing `0xFF` run — and that
run is as long as the data makes it: 6384 bytes on a repeating `{0xFF,0x00}`.
`sh_v1xN_c.inc` and `sh_v1xN_s.cpp` drop it unconditionally, and a block whose
flush leaves more behind than the padding covers then decodes to the wrong
bytes, silently, with both ends exiting 0.

So the trim is capped at `RC_FF_TRIM` and the padding is derived from it. The
carryless coder is unaffected — it commits its `0xFF`s during `ShiftLow` rather
than trimming them — which is why the default configuration never showed it;
but the carry coder is the fallback there too. `test/ff0`, `test/ff7` and
`test/ffmix` in `t.sh` are the regression.

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
./t.sh "" "-DRC_RCNUM=8" "-DRC_LOWBYTES=5" "-DRC_FORCE_CARRY=1" "-DRC_CL_DYNAMIC=1"
```

Each config is built and round-trips a set of files — empty, 1 byte, exactly
`BLKSIZE`, `BLKSIZE+1`, `2*BLKSIZE`, all-`0xFF`, all-zero, 300KB random,
`ff0`/`ff7`/`ffmix` for the flush's trailing `0xFF` run, `book1` — against both
FSM tables, comparing byte for byte. Where there is a device, it also encodes
each file on it and checks that output against the host encode byte for byte,
and fails if the device quietly fell back — which is the failure mode a size
comparison would hide.

Worth keeping `-DRC_CL_DYNAMIC=1` in the sweep: it is the only configuration
that compiles the hand-rolled ICD loader outside Windows, and a missing entry
in its lists is invisible everywhere else.

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
