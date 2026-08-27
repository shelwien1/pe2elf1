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
| `rc_kernel.c` | the coding kernel, built at run time |
| `rc_kernel.cl` | ... with its functions generated into macros |
| `rc_kernel.inc` | ... wrapped up as C string literals |
| `mk_kernel.sh` | runs those two passes — `rc_macro.pl`, `defines.pl`, `txt2inc.pl` |
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
set up. `rc_kernel.c` is the same coder as one work-item per lane, and Intel's
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

| | host | device | |
| --- | --- | --- | --- |
| `book1` 768,771 | 18.8 MB/s | 74.3 MB/s | 3.95x |
| 10 MB text | 18.6 | 76.3 | 4.11x |
| 10 MB random | 13.7 | 75.5 | 5.53x |

Output byte-identical in every one. The pipeline is what the second of those
columns rests on: at `RC_CL_NBLK` 1 the same build does 47 MB/s, and 0 of 153
blocks are finished when collected against 141 at the default 8. It plateaus
around 4 to 12; each slot is about 2 MB of host memory and as much again on the
device.

The work-group size is the other trade, and the earlier version of this table
got it wrong by never measuring the interesting end of it. A group is
vectorised across its work-items *and* scheduled on one compute unit, so the
reasoning was "cores beat vector width, use one group per compute unit". The
real answer is further along the same line: use no vectorisation across
work-items at all.

| work-group | | txt 10 MB | random 10 MB |
| --- | --- | --- | --- |
| 1 | one work-item per group | **72.9 MB/s** | **75.2** |
| 2 | | 71.3 | 70.0 |
| 4 | one group per compute unit | 61.6 | 61.6 |
| 8 | | 52.7 | 53.4 |
| 16 | 16-way vector, one core | 31.1 | 31.6 |

A lane's output is a byte store into its own row, RCNUM rows `OUTSTRIDE` apart,
so widening the group turns every store into a scatter and the scatters cost
more than the width pays. The lanes still run in parallel at a work-group of 1
— 16 groups over 4 cores — they just run as scalar code. Intel's compile-time
remark still says "successfully vectorized (16)", because that is the vectorised
variant being compiled; at a local size of 1 the runtime does not use it.

So the default is 1 on a CPU device. Elsewhere there is no measurement to go on
and it is left to the runtime, whose preferred multiple is the whole point of
the SIMD width on a GPU. `RC_CL_LWS` overrides either.

Raising RCNUM does not help: it costs output (+0.1% at 32, +0.3% at 64, from
the extra flushes) and measures slower, because the work per launch is the same
and only the number of lanes changes.

Two things measured and rejected. Partitioning the CPU device to leave a core
for the host thread (`clCreateSubDevices` by counts, 3 units of 4) cost more in
coding throughput than it bought in overlap: 32 MB/s against 45. And reading
each lane's substream back separately is RCNUM enqueues per block of a few
kilobytes each — all overhead — so `Collect` maps the slot's buffer once and
copies out of it instead.

### The shape of the kernel

Two things about how the kernel is written, both taken from `sh_v1xN.inc` and
both worth more than they look.

**The byte window is stored unconditionally.** `ShiftLow` emits 0, 1 or 2 bytes
per step, and how many differs between lanes. Written as a loop that is the one
place the coder diverges, and every lane pays for the widest. Instead both
bytes go out every time and only the cursor advances by `nsh`; at `nsh<2` the
next store overwrites the second. The last one can leave a byte of rubbish just
past the substream, inside the row's padding and past the length the host is
told, which is why nothing reads it. Worth about 6%.

`sh_v1xN.inc` writes *backwards*, which is what lets it do this in a single
16-bit store — the little-endian store lands the two bytes in the right order
when the cursor runs down. Here that would reverse each substream against the
host coder and the decoder, so it is two byte stores going forwards instead.
The branch is what cost, not the store width: a single `vstore2` (legal
unaligned, `uchar2` needs only 8-bit alignment) measured slower than the pair.

**The state is scalars, not a struct.** A struct in private memory ought to be
scalarised before the runtime vectorises across work-items, and on this runtime
it is — the kernel vectorised either way. It is still worth about 6% to not
leave it to the compiler, which is why the bodies are macros over six named
locals, the same six values `sh_v1xN.inc` keeps as separate `ALIGN(VECSIZE)`
arrays. It also means the kernel reads as one lane's worth of straight-line
scalar work, which is what the work-group of 1 above wants it to be.

Together with the work-group change, that is 56.3 MB/s to 76.3 on 10 MB of
text, and 55.3 to 75.5 on 10 MB of random.

### The kernel is OpenCL, and the macros are generated

`rc_kernel.c` is the kernel, as ordinary OpenCL C that an editor can
highlight. Three perl passes turn it into what `rc_cl.cpp` compiles:

```
rc_kernel.c  --rc_macro.pl-->  each function becomes #define / #enddef
               --defines.pl -->  ... a real multi-line macro
               --txt2inc.pl -->  ... C string literals: RC_CL_SRC
```

`mk_kernel.sh` runs the chain; `build.sh` and the batch files run it when
`rc_kernel.c` is newer, and warn instead if perl is missing. Both generated
files are committed, so perl is only wanted by whoever edits the kernel.

The macro pass is not decoration. The coder has to compile to plain private
scalars with no pointer to an aggregate for the vectoriser to give up on, which
is worth about 6% — so the bodies have to be macros, and generating them beats
maintaining a screen of trailing backslashes by hand.

What that costs is that the coder functions are macro bodies, however much they
look like functions: they take only their real arguments and reach the lane
state — `low`, `range`, `rpre`, `ffnum`, `nout`, `o` — by name. It is the same
arrangement as `sh_v1xN_macro.inc`, for the same reason. Two consequences worth
knowing before editing:

* **A macro body cannot hold `#if`.** So a value with variants — the low
  accumulator, the optional renorm tail — has each variant written out as its
  own function, and an `#if` afterwards picks one with a plain
  `#define LOW_ADDC(cv) low_addc_word8(cv)`. The unpicked ones are still
  defined, as macros nothing expands, which is why the split variants can name
  `lowl`/`lowh` while the other pair names `low`.
* **What cannot be a function.** `LOW_DECL` declares the lane state, so it has
  to land in the caller's scope rather than inside a generated `{ }` body;
  `LOW_B0`, `LOW_B1` and `LOW_GET` are expressions, and a `{ }` body is not
  one. Those four stay ordinary `#define`s. Everything else with code in it is
  generated — there is not a backslash continuation left in `rc_kernel.c`.
* **`txt2inc.pl` runs `-raw`**, because its default doubles `%` for `printf`
  and this string is not a format: a `k%RCNUM` would otherwise reach the device
  compiler as `k%%RCNUM`. Everything above the `[[...]]` marker is preamble and
  stays out of the string.

Three small changes to the scripts, all additive. `rc_macro.pl` now requires
the return type and function name to be identifiers — as written, `([^\s]+)`
also matches a bare `(`, so a line like `if( a || ((b)<c) ) {` looked like a
function and became a macro definition. `defines.pl` now only opens a block for
a `#define` with nothing after its parameter list, which is what `rc_macro.pl`
emits; it was treating every `#define` that way, so an ordinary
`#define LOWBITS (LOWBYTES*8)` collected the rest of the file. And `txt2inc.pl`
grew `-raw` and an optional declaration in the marker.

### Types

OpenCL C has no `<stdint.h>` and does not need one: `char`, `short`, `int` and
`long` are exactly 8, 16, 32 and 64 bits wide *by definition* there, not "at
least". So `uint` already is `uint32_t`. The kernel typedefs the stdint
spellings anyway and uses them, so it reads like the host code beside it.

### Why a wide work-group was slow: the byte scatter

`ocl2elf.py` carves the native AVX-512 object out of what `-k` caches, so the
built kernel can be disassembled and counted:

| | instructions | conditional branches | scalar byte stores | vector extracts |
| --- | --- | --- | --- | --- |
| byte stores, work-group 4 | 2596 | 488 | 63 | 330 |
| dword accumulator, work-group 16 | 859 | 46 | 0 | 26 |

488 branches, for a coder whose arithmetic is maybe forty instructions. They
are not divergence in the coder — there is one `kortestw` in the whole kernel.
They are the output.

**A byte store to a per-lane address cannot be vectorised.** There is no byte
scatter in AVX-512 — no `vpscatterb`, in any subset — and no other SIMD ISA has
one. So when the runtime vectorises across work-items, every `o[nout] = b`
becomes: extract the lane's address out of the vector register, scalar store,
test the lane's mask bit, branch. Once per lane, per byte, at every expansion
of `rc_shiftlow`. In the disassembly that is `vextracti32x4` / `vpextrq` →
`mov BYTE PTR [rax]` → `test dl, 0x20` → `jz`, sixteen deep.

That is the work-group answer too. The chain is per *lane*, so its cost scales
with the work-group while the useful work does not — every measurement got
worse as the group got wider, and a work-group of 1 was fastest.

And it is why `RC_LOWSPLIT` could not help. The kernel was not using 32-byte
vectors: every arithmetic instruction in the dump was on `zmm`, the 32-bit ones
included. The vectoriser picks one width for the kernel, and the widest thing
in it was a *pointer* — `o` was a per-lane `__global uchar*`, 64 bits, so the
store became `vpscatterqd`, scatter by 64-bit index, which needs two `zmm` for
sixteen lanes. Everything else got dragged along: `vpmovzxdq` to widen the
32-bit lanes into 64-bit indices, `vextracti64x4` to split what would not fit.
Splitting `low` into halves buys nothing when the register width is set by
something else, and it adds the widening back.

So two things were wrong, and both had to go:

* **The store granularity.** `RC_CL_WORDOUT` buffers each lane's output in an
  accumulator and lets a whole dword go at once — `vpscatterdd`, one masked
  instruction covering every lane, no branch. The partial dword is stored every
  time and rewritten until it is full, which is what keeps the accumulator to
  four bytes: there is never a fifth byte to hold, only a spill to carry.
* **The address width.** The row is addressed as a uniform base and a 32-bit
  index rather than a per-lane pointer, so the scatter takes 32-bit indices and
  sixteen of them fit in one register. Nothing in the output stage is 64 bits
  wide any more.

Measured, 10 MB of text and 10 MB of random:

| | byte stores | dword accumulator |
| --- | --- | --- |
| work-group 16, `RC_LOWSPLIT=1` | 36.3 / 34.6 MB/s | **72.3 / 70.5** |
| work-group 16, `RC_LOWSPLIT=0` | 36.3 / 34.6 | 69.2 / 70.9 |
| work-group 1 | 71.6 / 70.4 | 57.7 / 58.1 |

Exactly 2x at a full-width work-group, which is where the problem was, and
about 20% *slower* at a work-group of 1, where there is no vectorisation to fix
and the accumulator's shifts replace a store that was only ever a store. Hence
`RC_CL_WORDOUT=-1`, the default: decide by the work-group actually chosen.
`RC_LOWSPLIT=1` also finally earns its keep — 72.3 against 69.2 — now that the
register width is not being set by a pointer.

### Where the last 46 branches are

Tracked down on the fixed kernel at a work-group of 16, and then confirmed
against a decompilation of it:

| | count | on the hot path? |
| --- | --- | --- |
| `pbit[k]`, scalarised — its guard and 30 lane tests | 31 | **yes, every iteration** |
| the scalar remainder path, and the two guards that dispatch to it | 13 | no — 2 run once, 11 never |
| main loop back-edge | 1 | necessary |
| `rc_quit`'s flush loop | 1 | once per block |

The runtime's kernel opens with

```
cmp r8,0x10      ; r8 = the local work-group size
jae 548          ; >= 16  -> the vectorised loop
test r8b,0xf     ; work-group % 16
je  536          ; == 0   -> skip the scalar tail entirely
```

so what looked like a prologue is a *complete second copy of the coder*, scalar,
for the work-items left over when the work-group is not a multiple of the vector
width — its own header bits, its own `k += RCNUM` loop, its own fully unrolled
minimal flush, its own `outlen`/`outcarry` stores. At a work-group of 16 it is
dead code and only the two dispatch tests execute, once per launch.

The 31 are the same disease as the byte scatter, on the load side. A
work-group can be launched partly filled, so the runtime's vectorised kernel
always carries a lane mask, and the load is therefore masked — and a masked
16-bit load has no instruction behind it, because AVX-512 has no 16-bit gather
any more than it has a byte scatter. So it becomes sixteen of extract the
address, `vpbroadcastw`, `vpblendd`, test the lane's mask bit, branch.

They can be removed, and none of the three ways is worth it. Widening `pbit` to
32 bits takes the kernel to 17 branches and 5 extracts — but the array is then 2 MB a block
instead of 1 MB, over the bus and through the kernel's own reads, and at the
geometry that matters it costs more than the branches do:

| | 16-bit `pbit` | 32-bit `pbit` |
| --- | --- | --- |
| RCNUM=16, work-group 16 | 74.9 MB/s, 865 us | **80.8, 755 us** |
| RCNUM=64, work-group 16 | **112.0 MB/s, 417 us** | 100.3, 510 us |

At RCNUM=16 the kernel is not yet bandwidth-bound and the branches dominate;
at RCNUM=64 it is, and they do not. The fast geometry is the one worth keeping,
so `pbit` stays 16-bit and the branches stay.

The other two ways are worse outright. Splitting the loop to make the trip count
uniform: the mask is on the work-item, not on `k`, so it survives, and the
second loop is another copy of the coder — 81 branches. Loading unconditionally
past the end, with slack in the buffer, and putting only `rc_process` under the
mask: the compiler predicates the whole coder, 1398 instructions against 898,
nine of the per-lane loads survive anyway, and it measures no faster.

### More lanes, now that lanes are cheap

Raising RCNUM used to measure slower, because every extra lane was another
scalarised store chain. With the store fixed it does what it looks like it
should, and a work-group of 16 with RCNUM=64 is four full-width groups over
four cores:

| RCNUM | work-group | | kernel | output vs RCNUM=16 |
| --- | --- | --- | --- | --- |
| 16 | 1 | 71.6 / 70.4 MB/s | 974 us | — |
| 16 | 16 | 74.9 / 73.7 | 865 us | — |
| 32 | 16 | 99.0 / 92.7 | 627 us | +0.10% |
| 64 | 16 | **112.0 / 103.5** | 417 us | +0.29% |
| 64 | 32 | 103.5 / 101.7 | 504 us | +0.29% |

1.7x over the best that was reachable before, for 0.29% of output — the extra
flushes of 64 substreams rather than 16. The defaults are left alone because
that is a trade to make deliberately, not one to inherit: `-DRC_RCNUM=64
-DRC_CL_LWS=16` takes it.

### `-k`, the kernel binary cache### `-k`, the kernel binary cache

Building the kernel costs about 0.2 s here and over 2 s on a cold runtime,
every run, for a program that only changes when its source or the geometry
does. `-k` writes the built binary to `coder_kernel.bin` — `-k<file>` for
somewhere else — and reuses it: 0.00 s instead.

The difficulty is that a stale binary does not fail, it quietly codes with the
wrong geometry. So the file records everything the binary was built from, and
any mismatch rebuilds:

* the device — name, vendor, driver version, OpenCL version, compared verbatim
* the `-D` options, which carry RCNUM, the block size, the row stride,
  `RC_LOWBYTES`, `RC_LOWSPLIT` and the rest
* the length and a hash of the kernel source
* a hash of the binary itself

That last one is not belt and braces. Flipping 64 bytes in the middle of the
stored binary, this driver accepted it without complaint and coded with the
result — a wrong stream from a cache file that had been sitting on disk. With
the hash it rebuilds instead. Truncated, empty and garbage files were already
caught by the length and magic checks; corruption in place was not.

`-k` takes its filename attached only (`-kFILE`, not `-k FILE`), because a bare
`-k` must not swallow the input filename after it.

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
| `RC_LOWSPLIT` | 0 | hold `low` as two uints instead of one qword (stream-neutral); host and kernel both |
| `RC_RANGE64` | 0 | 1 = `range` starts at `1<<32` instead of `0xFFFFFFFF` |
| `RC_RENORM_TAIL` | 0 | renorm tail loop; dead for a binary coder |
| `RC_FORCE_CARRY` | 0 | 1 = always use the carry twin, never the fast path |
| `RC_TRACE_FALLBACK` | 0 | 1 = report every re-coded block |
| `RC_STATS` | 0 | per-substream carry counts to stderr |
| `RC_CARRYLESS_WARN` | 0 | the demo's "lost carries" warning |
| `RC_IO_CHECK` | 1 | bounds-check encoder writes |
| `RC_STRICT_BLKSIZE` | 0 | reject an unsafe `BLKSIZE`/`RCNUM` pair at compile time |
| `RC_OPENCL` | auto | 1 = build the device path (`build.sh` sets it when `CL/cl.h` is there) |
| `RC_CL_LWS` | 0 | work-group size; 0 = let the runtime choose |
| `RC_CL_WORDOUT` | -1 | kernel writes dwords not bytes; -1 = whenever the work-group is > 1 |
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
host coder is scalar and the kernel is one lane per work-item — so the reason
is gone. Measured:

| | split=1 | split=0 | |
| --- | --- | --- | --- |
| host, carryless, `RC_LOWBYTES=8`, `book1` | 19.75 MB/s | 19.84 MB/s | -0.5% |
| host, carryless, `RC_LOWBYTES=8`, 10 MB | 14.37 | 14.56 | -1.3% |
| host, carry twin, `RC_LOWBYTES=4`, `book1` | 19.59 | 20.08 | -2.4% |
| host, decoder, any | — | — | none |
| device, 10 MB text | 68.8 | 78.4 | -12.2% |
| device, 10 MB random | 69.0 | 78.2 | -11.8% |

Output byte-identical in every pair, which is the point of calling it
stream-neutral. The decoder is untouched by construction: `low_Add`, `low_Top`
and `low_Shift8` are all under `if( f_DEC==0 )`. The carry twin at
`RC_LOWBYTES=4` loses most on the host, because `LOWBITS` is 32 there and the
split version shifts a `lowh` that the mask then clears anyway. So the default
is 0.

The kernel carries the same knob, and loses more by it — 12% — for the same
reason and one more: at `RC_LOWSPLIT=1` a shift of the pair by 0, 8 or 16 bits
needs a select, because a 32-bit shift by `32-0` is a shift by 32, which OpenCL
takes modulo 32 and turns into no shift at all. It is there because a device
with no native 64-bit integer has no choice, and that is most GPUs — where the
numbers above should invert.

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
  -k [file] cache the built kernel binary and reuse it next run
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
