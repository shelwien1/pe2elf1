# coder — sh_v1xN framework on the sh_v1xN_s.cpp rangecoder

A port of the `069-lowl+lowh` coder onto the scalar rangecoder from
`sh_v1xN_s.cpp`. The framework, the model and the compressed format are the
originals; the SIMD rangecoder and the perl passes that generated it are gone,
replaced by RCNUM plain instances of the scalar coder.

```
./build.sh
./coder c book1 1 FSM0.txt     # encode
./coder d 1     2 FSM0.txt     # decode
cmp book1 2
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

**Linux / gcc:** `./build.sh [extra -D flags]`

**Windows:** `g.bat` (MinGW), `gc.bat` (clang-cl), `c.bat` (ICL/ICX). `c.bat` no
longer runs perl. `Lib3/file_api.inc` picks `file_api_win.inc` on `_WIN32` and
`file_api_std.inc` elsewhere; force the portable one with `-DFILE_API_STD`.

The coder is scalar now, so the `target("avx2,…")` attributes on `do_process`
are gone.

## Testing

```
./t.sh "" "-DRC_RCNUM=8" "-DRC_LOWBYTES=5" "-DRC_FORCE_CARRY=1"
```

Each config is built and round-trips a set of files — empty, 1 byte, exactly
`BLKSIZE`, `BLKSIZE+1`, `2*BLKSIZE`, all-`0xFF`, all-zero, 300KB random, `book1`
— against both FSM tables, comparing byte for byte.

## Usage

```
coder c|d input output FSM_file [n_iter] [test_output]
```

`FSM_file` is the counter state machine `Predictor::Init` loads (`FSM0.txt`,
`nzcc.txt`); both ends need the same one. `n_iter` repeats the run for timing.
With `test_output` the coder encodes, decodes back and appends a line to
`log.txt`:

```
./coder c book1 1 FSM0.txt 10 2
```
