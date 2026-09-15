# fpaq0mw — bytewise coding of a bitwise model by journaled speculative execution

This document describes what the program in this repository does and how the
pieces fit together. It was written from the sources of iteration `024`
(`024-track_inc`), the state of the experiment recorded at the bottom of
`log.txt`.

Contents

1. [What the program is](#1-what-the-program-is)
2. [Source map](#2-source-map)
3. [The bitwise model](#3-the-bitwise-model)
4. [From bit probabilities to a byte distribution](#4-from-bit-probabilities-to-a-byte-distribution)
5. [Journaled speculative execution](#5-journaled-speculative-execution)
6. [The build pipeline: instrumenting the compiler's output](#6-the-build-pipeline-instrumenting-the-compilers-output)
7. [The range coder](#7-the-range-coder)
8. [Main loop and file format](#8-main-loop-and-file-format)
9. [Numeric formats](#9-numeric-formats)
10. [Performance history](#10-performance-history)
11. [Limitations and caveats](#11-limitations-and-caveats)

---

## 1. What the program is

`fpaq0mw` is an experimental lossless compressor in the *fpaq0* family: an
order-0 model that predicts a byte one bit at a time, with the 0..7 bits
already seen of the current byte as the only context.

The standard way to use such a model is to run a **binary** arithmetic coder
eight times per byte: predict bit, code bit, update model, repeat. This program
does something different. For every input byte it

1. derives the full **256-symbol probability distribution** that the bitwise
   model implies for the next byte,
2. codes the byte with a single **multi-symbol** range-coder step against that
   distribution, and
3. only then updates the model with the eight bits of the byte actually coded.

Step 1 is the interesting part. The probability of a byte `d` under a bitwise
model is the product of the eight conditional bit probabilities along `d`'s
path through the binary tree of prefixes. But a real model does not stand
still between bits: after each bit it *updates itself* (counters, mixer
weights, its notion of the current context), and the prediction for the next
bit is made from that updated state. To evaluate all 256 paths one therefore
has to run the model's actual *predict-and-update* step down every branch of
the tree and roll it back again on the way up.

Rather than hand-writing a "predict without side effects" version of the
model, the program treats the compiled model step as a black box and makes it
reversible mechanically:

* the model step (`encode_sim`) is compiled to assembly language,
* a script (`track.pl`) inserts a call to a small journaling routine in front
  of **every memory store** in that assembly,
* the journaling routine records the address and the old contents of each
  store, and
* `NEST()` / `UNDO()` mark a point in the journal and restore everything
  written since that point.

This is, in effect, a software transactional memory for a single function,
implemented by binary instrumentation of the compiler's output. (The comment
lines `//#include "tsx.inc"` at the top of `fpaq0mw.cpp` and the `tsx.inc`
entries in `log.txt` show that Intel's hardware transactional memory (RTM) was
tried for the same purpose before this software journal; those files are not
part of this snapshot.)

Because the full tree walk costs up to 254 simulated model steps per byte, the
walk is pruned: sub-trees whose prefix has already become too improbable are
not explored and their leaves get the minimum frequency.

The model used here is deliberately simple so that the machinery, not the
model, is what is being tested. For this particular model the per-node state
is disjoint between the tree nodes, so the journal is only strictly needed to
roll back the context variable; the point of the design is that it works
unchanged for models where that is not true (hashed or shared counters, SSE
stages, mixers with shared weights, and so on). The by-product of the walk,
the code length of *every* possible next byte, is also exactly what an
optimal-parsing encoder (for example an LZ parser) needs from a model.

The compression gain over plain bitwise coding of the same model is
negligible (see [§10](#10-performance-history)); the experiment is about the
technique and its cost.

## 2. Source map

| File | Role |
| --- | --- |
| `fpaq0mw.cpp` | Includes everything else; `main()` with the per-byte encode/decode loop. Compiled **twice**: once with `-DSIM_FUNC` (only the model step, to assembly) and once normally (the program). |
| `model.inc` | `Predictor`: the bitwise order-0 model (two counters and a mixer per context). |
| `sh_mixer.inc` | `iMixer`: a one-weight interpolating mixer that combines the two counters. |
| `sh_v1m.inc` | `Rangecoder`: 32-bit-low / 64-bit-range range coder with explicit carry handling. |
| `coder.inc` | `Coder`: glues predictor and range coder; `encode_sim()` = one predict-and-update step, exported with C linkage in the `SIM_FUNC` build. |
| `log2lut.inc` | `log2LUT`: integer log2 table in 16.16 fixed point. |
| `track.inc` | The journal (`trk`, `NEST`, `UNDO`), the `track1/2/4/8` journaling stubs (assembly), the pruned tree walk `TEST_ENCODE` / `test_encode()`, the `unlog` table. |
| `track.pl` | Build step: inserts the journaling calls into the compiler-generated assembly. |
| `build.sh`, `build.bat` | The three-step build on Linux / Windows. |
| `test.sh`, `test.bat`, `timetest.cpp` | Round-trip and timing test that appends to `log.txt`. |
| `log.txt`, `log1.txt`, `log.pl` | The experiment log (size and timings per change) and the script that tabulates it. |
| `legacy/` | The original Windows-only scripts (`g.bat` for MinGW GCC, `c.bat` for Intel ICX via LLVM `llc`, `1.pl`/`1a.pl`, `t.bat`, `icx64.cfg`) kept for reference. |

## 3. The bitwise model

### 3.1 Context

`Predictor::cxt` is the classic fpaq0 context: the bits of the current byte
seen so far, with a leading 1. It starts at 1 for every byte, becomes
`cxt*2+bit` after each bit, and is reset to 1 when it reaches 256 (after the
eighth bit). The 255 reachable values 1..255 are the internal nodes of a
binary tree whose 256 leaves are the byte values.

### 3.2 Two counters per context

Each context owns two 12-bit probabilities of a 1 bit (`SCALE = 4096`),
updated as shift-based exponential moving averages at different rates:

```
p1 += (SCALE - p1) >> 4   on a 1        p1 -= p1 >> 4   on a 0      (fast, 1/16)
p2 += (SCALE - p2) >> 8   on a 1        p2 -= p2 >> 8   on a 0      (slow, 1/256)
```

Because of the integer shifts the counters saturate short of the limits:
`p1` stays in [15, 4081] and `p2` in [255, 3841], so no probability is ever
0 or 1.

### 3.3 The mixer

`iMixer` (one 16-bit weight `w` per context, `SCALE = 32768`) does not add
stretched inputs like a logistic mixer. It **interpolates** between the two
counters:

```
P = p1 + ((p2 - p1) * w) >> 15          w = 0 -> p1,   w = 32768 -> p2
```

`w` starts at 32768, so a fresh context predicts with the slow counter.

Its update is a closed-form step towards the weight that would have predicted
the bit perfectly. Working in a 15-bit probability-of-zero space
(`q = 32768 - 8*p`), with target `T = 100` if the bit was 1 and `32668` if it
was 0 (i.e. about 0.3 % / 99.7 %), the ideal weight is the position of `T` on
the line from `q1` to `q2`:

```
w* = (T - q1) / (q2 - q1)                (as a fraction, scaled by 32768)
w  = clamp( (5/6) * w + (1/6) * w*, 0, 32768 )       (wr = 32768/6)
```

The step is skipped when the two counters are within `Limit = 100` (of 32768)
of each other, where the division would explode and mixing cannot help
anyway. Since the map from the 12-bit probability-of-one space to the 15-bit
probability-of-zero space is affine, the interpolation weight found in one
space is the right weight in the other; that is why `Update` and `Mixup` can
work in different spaces.

### 3.4 One model step

`Predictor::update(y)` runs, in this order: mixer update (using the old
counter values), counter updates, context shift. `Coder::encode_sim(y)` is
"predict, then update" and returns the prediction:

```cpp
uint encode_sim( int y ) {
  uint p = predictor.P();     // probability of a 1, 12-bit
  predictor.update(y);
  return p;
}
```

One step writes exactly four words of state: `p1[cxt]`, `p2[cxt]`,
`mix[cxt].w` and `cxt`. Those four stores are what the journal has to capture.

## 4. From bit probabilities to a byte distribution

### 4.1 Costs in the log domain

Multiplying eight 12-bit probabilities is done as a sum of logarithms.
`log2LUT` tabulates `LOG2(i) ≈ 2^16 · log2(i)` for `i` in [0, 32768] with a
16-bit fractional part, computed by repeated squaring in integer arithmetic
(the table values carry a constant bias of 31..32 units which cancels in the
differences taken below).

`test_encode()` fills `clen[0x200]`, indexed by tree node (`1` = root,
children `2n` and `2n+1`, leaves `0x100 + byte`). For a node the value is the
accumulated `Σ LOG2(p_i)` over the bits on the path to it, i.e. 2^16 times the
log-probability numerator; the cost of the path in bits is
`12·depth - clen/2^16`. The array is initialised to `0xFFFFF` everywhere
(meaning "not evaluated" / cost about 16 bits) with `clen[1] = 0`.

### 4.2 The tree walk

`TEST_ENCODE<Model, bit, ctx, K>` is a template instantiated once per
(depth, bit) so that the whole walk unrolls into straight-line code. At an
internal node `ctx` with a chosen `bit` (child `cty = 2*ctx + bit`):

```
NEST()                                   // remember the journal position
p = encode_sim(&E, bit)                  // real predict + update, journaled
if bit == 0:                             // p is the same for both children,
    clen[2*ctx+0] = clen[ctx] + LOG2(SCALE - p)     // so fill both at once
    clen[2*ctx+1] = clen[ctx] + LOG2(p)
if clen[cty] > 0x90000 * depth:          // pruning test, see below
    TEST_ENCODE<0, cty, K-1>             // descend into both children
    TEST_ENCODE<1, cty, K-1>
UNDO()                                   // roll the model back
```

`K` counts down from 7 at the root; `depth = 8 - K`. The `bit == 1`
instantiation still has to call `encode_sim` so that the model is in the
right state for its sub-tree, even though the two `clen` entries were already
filled by its sibling.

At the last level (`K == 0`, the eighth bit) no update is needed, because
nothing is predicted after it inside this byte, so the specialisation only
reads `E.predictor.P()` and fills the two leaves. This removes 128 of the 254
simulated steps and all their journaling (the log calls it "probability reuse
on last bit").

### 4.3 Pruning

A sub-tree is entered only if its prefix is still probable enough:

```
clen[cty] > 0x90000 * depth     <=>    Σ log2(p_i) > 9 * depth
                                <=>    cost so far < 3 bits per bit
                                <=>    P(prefix) > 8^-depth
```

Prefixes that fail keep `0xFFFFF` in all their leaves, which becomes the
minimum frequency 1 in the next step. The threshold is a trade-off recorded in
`log.txt`: with `0x80000` (4 bits/bit) the distribution is exact and
compression is 446 945 bytes at 1.96 s; `0x90000` gives 446 962 bytes at
1.77 s; `0xA0000` (2 bits/bit) already loses 3.6 %; `0xC0000` prunes
everything and the output is the raw file.

### 4.4 Frequencies

For each byte value `d`:

```
r       = LOG2(SCALE)*8 - clen[0x100+d]      // 2^16 * cost of d in bits
r       = min(r, 0xFFFFF)                    // cap at ~16 bits
freq[d] = unlog[r]                           // = floor(2^16 * 2^(-r/2^16)), at least 1
```

`unlog` is a 2^20-entry table built once with `pow()` at start-up
(`PSCALE = 2^16`). The frequencies therefore sum to about 2^16 (plus rounding
and the 1s of pruned symbols), and `total` is their exact sum, computed on the
fly and identical in encoder and decoder.

## 5. Journaled speculative execution

### 5.1 The journal

```cpp
struct Cell { void* ptr; uint msk; uint val; };   // 16 bytes
Cell trk[1<<20];        // the journal
uint trkptr;            // next free cell
uint nest_trkptr[0x200], nesting;   // stack of saved journal positions
```

`NEST()` pushes `trkptr`. `UNDO()` pops it and walks the cells written since
then **backwards**, restoring `*ptr = (*ptr & ~msk) | val` for each, so that
several stores to the same location unwind in the right order. The mask lets a
1-, 2- or 4-byte store be recorded as a 32-bit read of its address; 8-byte
stores are recorded as two cells.

Only the current root-to-node path is ever live in the journal (at most seven
steps of four stores), so the capacity is far larger than needed.

### 5.2 Which stores are journaled

There are two versions of the model step in the executable:

* the **instrumented** `encode_sim` — the C function exported by `coder.inc`
  in the `SIM_FUNC` build, compiled to assembly and patched by `track.pl`.
  `test_encode` calls this one through the `extern "C"` declaration in
  `track.inc`;
* the **plain** `Coder::encode_sim` member function, inlined into `main()`
  from the normal build, used for the real update after the byte is coded.

Only the first version journals its stores; the real update is meant to stick.
Both come from the same source, compiled with the same compiler and flags, so
they agree on the layout of `Coder`.

### 5.3 The journaling stubs

`track1`, `track2`, `track4`, `track8` (in `track.inc`) each append one cell
(two for `track8`) for the address passed in the first-argument register
(`rcx` on Windows, `rdi` on Linux). They are written in assembly using only
`mov`, `movzx`, `lea`, `push`, `pop` and `ret`, so they preserve every
register **and EFLAGS**. That matters: the compiler is free to keep a
comparison result live across a store (the GCC output for this very model has
`cmp` / `mov [mem],reg` / `cmovge`), and a stub that clobbered the flags would
corrupt the model. The original C stubs used
`__attribute__((no_caller_saved_registers))`, which saves registers but not
flags, and needed a `pushf`/`popf` pair around every call; `log.txt` records
that dropping the pair ("drop pushf") took the run from 7.5 s to 3.2 s, which
is why the stubs are now flag-neutral by construction.

## 6. The build pipeline: instrumenting the compiler's output

```
fpaq0mw.cpp --(CXX -S -masm=intel -DSIM_FUNC)--> coder.s
coder.s     --(perl track.pl)-------------------> coder1.s
fpaq0mw.cpp + coder1.s --(CXX)------------------> fpaq0mw[.exe]
```

`track.pl` reads the Intel-syntax assembly and, for every plain store

```
mov  {byte|word|dword|qword} ptr <mem>, <src>
```

inserts, in front of it:

```
push ARG                 ; ARG = rcx (Win64) or rdi (SysV x86-64)
lea  ARG, <mem>
call trackN              ; N = width of the store in bytes
pop  ARG
```

The address is computed with `lea` from the same operand the store uses, so
the stub sees the exact location before it is overwritten. The ABI is
auto-detected from the assembly (COFF `.seh_proc`/`.def` directives mean
Win64, ELF `.type name, @function` means SysV) and can be forced with
`--abi=`. On Windows the script also appends the `.def` declarations the
original scripts emitted.

Stores relative to `rsp` are deliberately **not** instrumented: they are the
function's own stack frame, which is dead when `UNDO()` runs — and `UNDO()`'s
own frame would then occupy the same addresses, so "restoring" them would be
harmful. Any other memory-writing instruction the script does not know how to
handle (read-modify-write ALU ops such as `add [mem], reg`, SSE/AVX stores,
`xchg`, string ops) is reported and fails the build, because a store the
journal does not see cannot be undone; `--lax` downgrades that to a warning.
For the model in this snapshot GCC and Clang both produce exactly four plain
stores (three `word`, one `dword`) and nothing else, matching the four state
words of [§3.4](#34-one-model-step).

Why assembly-level instrumentation rather than a compiler option: it needs no
support from the compiler, applies to already optimised code (the stores that
survive optimisation are exactly the ones that matter), and it leaves the
model source untouched.

## 7. The range coder

`sh_v1m.inc` is a byte-oriented range coder with a 32-bit `low`, a separate
carry bit, and a 64-bit `range` (so that it can start at exactly 2^32).

**Interval selection.** For a symbol with cumulative frequency `cum`,
frequency `freq` and total `tot`, the sub-interval is computed from the top
down:

```
tmp  = range - floor((tot - cum)        * range / tot)     // start
rnew = range - floor((tot - cum - freq) * range / tot)     // end
encoder: low += tmp        decoder: code -= tmp
range = rnew - tmp
```

Written this way the 256 sub-intervals tile `[0, range)` exactly: the first
symbol starts at 0 and the last one ends at `range`, with no rounding gap.

**Renormalisation.** While `range < 2^24` the coder shifts out a byte:
`range <<= 8`, and the encoder runs `ShiftLow()` while the decoder pulls in
the next input byte.

**Carry propagation.** `low` and `Carry` overlay a 64-bit `lowc`, so
`lowc += tmp` deposits any carry out of bit 31 into `Carry` for free.
`ShiftLow()` is the usual cached-byte scheme: the top byte of `low` is held
back in `Cache`; if the byte to be shifted out is `0xFF` and no carry is
pending it is only counted (`FFNum++`), because a later carry could still
turn it into `0x00` and bump `Cache`. When a non-`0xFF` byte or a carry
arrives, `Cache + Carry` and then `FFNum` bytes of `0xFF + Carry` are written
out.

**Flush.** `rc_Quit()` widens `low` with as many trailing `0xFF` bytes as
still fit below `low + range` and then omits them from the output; the decoder
reads `0xFF` for every byte past the end of the file (`getc` returns EOF, which
`byte()` truncates to `0xFF`), so the truncated tail decodes identically. The
decoder pre-loads four bytes into `code`, matching the 32-bit `low`.

**Decoding a symbol.** `rc_GetFreq(tot) = floor(code * tot / range)` gives a
value in `[cum, cum + freq)` of the coded symbol; `main()` finds it with a
linear scan over the cumulative frequencies. (The `Coder` struct also keeps a
binary `encode_p`/`decode_p` interface from earlier fpaq0 versions; the
bytewise path does not use it.)

## 8. Main loop and file format

The output is a 4-byte native-endian length followed by the range coder
stream. Per byte, encoder and decoder run the same code except for where the
symbol comes from:

```
test_encode(E, clen)                       // pruned tree walk, journaled
freq[d] = unlog[min(LOG2(SCALE)*8 - clen[0x100+d], 0xFFFFF)] for all d
total   = Σ freq[d]

encoder: c = next input byte               decoder: v = rc.rc_GetFreq(total)
         low = Σ_{j<c} freq[j]                      scan for c with cum[c] <= v < cum[c]+freq[c]
                                                    emit c
rc.rc_Process(low, freq[c], total)
for j = 7..0: E.encode_sim((c >> j) & 1)   // real, non-journaled update
```

After the last byte the encoder calls `flush()`. The decoder knows the count
from the header, so no end-of-stream symbol is coded.

## 9. Numeric formats

| Quantity | Format | Range / notes |
| --- | --- | --- |
| Counter `p1`, `p2` | 12-bit probability of 1 (`SCALE = 4096`) | [15, 4081] / [255, 3841] |
| Mixer weight `w` | 16-bit, 15 fractional bits (`32768` = 1.0) | [0, 32768], starts at 32768 |
| Mixer update space | 15-bit probability of 0 | `q = 32768 - 8p`, targets 100 / 32668 |
| `LOG2(i)` | 16.16 fixed-point log2 | table for `i` ≤ 32768, bias ≈ −31.5, cancels |
| `clen[node]` | 2^16 · Σ log2(p) along the path | init `0xFFFFF`, root 0 |
| Pruning threshold | `0x90000 · depth` = 9.0 per bit | i.e. cost < 3 bits per bit |
| `r` (byte cost) | 2^16 · bits | capped at `0xFFFFF` ≈ 16 bits |
| `freq[d]` | 2^16 · P(d) (`PSCALE = 65536`) | at least 1; `unlog` table of 2^20 entries |
| Range coder | `low` 32 bit + carry, `range` 64 bit | renormalise below 2^24, bytewise |
| Journal cell | `{ptr, msk, val}` = 16 bytes | 2^20 cells, nest stack of 512 |

## 10. Performance history

`log.txt` is the author's lab notebook: after each change, `t.bat` appended
the encode and decode wall time and the compressed size of `book1` (Calgary
corpus, 768 771 bytes); `log.pl` tabulates it into `log1.txt`. The key
milestones:

| Stage (comment in log) | Size | Enc | Dec |
| --- | ---: | ---: | ---: |
| Plain bitwise coding of the same model | 447 071 | 0.08 s | 0.09 s |
| First bytewise version (full tree, C stubs) | 447 350 | 27.0 s | 29.8 s |
| `014-nesting`: NEST/UNDO journal | 447 351 | 18.4 s | 18.5 s |
| `PSCALE = 1<<16` | 446 945 | 18.9 s | 18.8 s |
| probability reuse on last bit | 447 083 | 13.7 s | 14.0 s |
| `017`: template recursion, inlining | 447 083 | 13.0 s | 13.0 s |
| `bit0: encode_sim -> E.predictor.P()` | 447 083 | 8.6 s | 8.6 s |
| LOG-scale `clen`, `022`: return p from `encode_sim` | 446 945 | 7.5 s | 7.6 s |
| drop `pushf` | 446 945 | 3.2 s | 3.3 s |
| `023`: pruning at `0x80000·depth` | 446 945 | 1.96 s | 1.97 s |
| `024`: pruning at `0x90000·depth` (this snapshot) | 446 962 | 1.77 s | 1.79 s |

The numbers were measured on the author's Windows machine with Intel ICX or
MinGW GCC; the comment lines are the author's. The Linux build of this
repository produces the identical 446 962-byte output for `book1` and decodes
it back bit-exactly (2.5 s each way on the container used for the port).

So the bytewise reformulation costs roughly 20× the time of bitwise coding
and gains about 0.02 % in size (the exact distribution codes 447 071 →
446 945; pruning gives back 17 bytes). The value of the experiment is the
machinery of [§5](#5-journaled-speculative-execution) and [§6](#6-the-build-pipeline-instrumenting-the-compilers-output),
not this model.

## 11. Limitations and caveats

* **Only plain `mov` stores are journaled.** A model whose compiled step
  contains read-modify-write instructions, SSE/AVX stores or string
  operations makes `track.pl` fail the build (by design). Such a model needs
  either source changes that steer the compiler towards plain stores, or new
  stubs plus script support for the instruction forms involved.
* **Calls out of the model step are invisible** unless the callee is compiled
  in the same `SIM_FUNC` translation unit (then it is instrumented too).
  Library calls such as `memset`/`memcpy` are not.
* **Stack stores are skipped** on the assumption that the model keeps no state
  in the step's own frame. With frame-pointer-based code (`[rbp-…]`) the script
  cannot tell stack from heap and journals them; build with
  `-fomit-frame-pointer` as the scripts do.
* **Journal capacity** is 2^20 cells and 512 nesting levels; nothing checks
  for overflow. A model step with more than ~150 000 stores per nesting level
  would overrun it.
* **Both copies of the model must match**: the instrumented `encode_sim` and
  the inlined `Coder::encode_sim` are compiled separately, so they must be
  built with the same compiler and the same flags (the build scripts enforce
  this by using one `CXX`/`CXXFLAGS` for both steps).
* **The `unlog` table is built with floating-point `pow()`.** Encoder and
  decoder of the same binary always agree, but a different libm could in
  principle round one table entry differently, which would make streams
  incompatible between builds. In practice the tables from GCC and Clang on
  Linux and from MinGW on Windows produced identical output in this port.
* **Pruning makes the coded distribution an approximation** of the model's;
  this is harmless for correctness (the decoder computes the same
  approximation) but it costs a little compression, tunable via the
  `0x90000` constant in `track.inc`.
* **Decoding is a linear scan** over 256 cumulative frequencies; a binary
  search would be the obvious improvement if the coder itself ever mattered.
* **Files are limited to 4 GiB** by the 4-byte length header, and there is no
  error checking on the input (a truncated stream decodes silently as `0xFF`
  bytes).
* **x86-64 only**, GNU assembler syntax only: GCC or Clang on Linux, MinGW-w64
  GCC or Clang on Windows. The MSVC/Intel toolchain path of the original
  `c.bat` (ICX → LLVM bitcode → `llc` → MASM-style listing) is not ported.
