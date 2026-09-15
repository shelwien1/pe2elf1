# Bytewise coding of a bitwise model, by journaled speculative execution

This document describes what the programs in this repository do and how the
pieces fit together. It was written from the sources of iteration `024`
(`024-track_inc`), the state of the experiment recorded at the bottom of
`log.txt`, and extended when the Tangelo model was ported onto the same
machinery.

Contents

1. [What the programs are](#1-what-the-programs-are)
2. [Source map](#2-source-map)
3. [The models](#3-the-models)
4. [From bit probabilities to a byte distribution](#4-from-bit-probabilities-to-a-byte-distribution)
5. [Journaled speculative execution](#5-journaled-speculative-execution)
6. [The build pipeline: instrumenting the compiler's output](#6-the-build-pipeline-instrumenting-the-compilers-output)
7. [The range coder](#7-the-range-coder)
8. [Main loop and file format](#8-main-loop-and-file-format)
9. [Numeric formats](#9-numeric-formats)
10. [Performance](#10-performance)
11. [Limitations and caveats](#11-limitations-and-caveats)

---

## 1. What the programs are

Two lossless compressors that code a whole **byte** per arithmetic-coder step,
using a model that predicts one **bit** at a time. They share every line of the
machinery and differ only in the model:

| Program | Model | `book1` |
| --- | --- | ---: |
| `fpaq0mw` | order-0: two counters and a mixer per partial-byte context | 446 962 |
| `tangelo_w` | Tangelo: a paq/lpaq-class model, about 361 MB of state | 197 483 |

The usual way to use a bitwise model is to run a **binary** arithmetic coder
eight times per byte: predict bit, code bit, update model, repeat. These
programs do something different. For every input byte they

1. derive the full **256-symbol probability distribution** that the bitwise
   model implies for the next byte,
2. code the byte with a single **multi-symbol** range-coder step against that
   distribution, and
3. only then update the model with the eight bits of the byte actually coded.

Step 1 is the interesting part. The probability of a byte `d` under a bitwise
model is the product of the eight conditional bit probabilities along `d`'s path
through the binary tree of prefixes. But a real model does not stand still
between bits: after each bit it *updates itself* (counters, mixer weights, hash
slots, its notion of the current context), and the prediction for the next bit is
made from that updated state. To evaluate all 256 paths one therefore has to run
the model's actual *predict-and-update* step down every branch of the tree and
roll it back again on the way up.

Rather than hand-writing a "predict without side effects" version of the model,
these programs treat the compiled model step as a black box and make it
reversible mechanically:

* the model step (`encode_sim`) is compiled to assembly language,
* a script (`track.pl`) inserts a call to a small journaling routine in front of
  **every memory write** in that assembly,
* the journaling routine records the address and the old contents of each write,
  and
* `NEST()` / `UNDO()` mark a point in the journal and restore everything written
  since that point.

This is, in effect, a software transactional memory for a single function,
implemented by binary instrumentation of the compiler's output. (The comment
lines `//#include "tsx.inc"` at the top of `fpaq0mw.cpp` and the `tsx.inc`
entries in `log.txt` show that Intel's hardware transactional memory (RTM) was
tried for the same purpose before this software journal; those files are not
part of this snapshot.)

Because the full tree walk costs up to 254 simulated model steps per byte, the
walk is pruned: sub-trees whose prefix has already become too improbable are not
explored and their leaves get the minimum frequency.

`fpaq0mw`'s model is deliberately simple, so that the machinery rather than the
model is what is being tested; its per-node state is disjoint between tree nodes,
so the journal is only strictly needed there to roll back the context variable.
Tangelo is the other case, and the one the technique is for: hash tables with
eviction, shared mixer weights, a match model with its own cursor, a randomised
state decay. Nothing about it is written to be reversible, and it does not need
to be. The by-product of the walk - the code length of *every* possible next
byte - is also exactly what an optimal-parsing encoder (for example an LZ parser)
needs from a model.

The compression gain over plain bitwise coding of the same model is negligible,
and at these thresholds slightly negative (see [§10](#10-performance)); the
experiment is about the technique and its cost.

## 2. Source map

Shared machinery:

| File | Role |
| --- | --- |
| `main.inc` | The driver: per-byte walk, frequency table, one coding step, real update. Model-agnostic; both programs include it. |
| `track.inc` | The journal (`trk`, `NEST`, `UNDO`), the `track1` … `track64` journaling stubs (assembly), the pruned tree walk `TEST_ENCODE` / `test_encode()`, the `unlog` table. |
| `track.pl` | Build step: inserts the journaling calls into the compiler-generated assembly. |
| `-DTRACK_VERIFY=n` | Not a file: the self-check build described in §5.5. |
| `log2lut.inc` | `log2LUT`: integer log2 table in 16.16 fixed point. |
| `sh_v1m.inc` | `Rangecoder`: 32-bit-low / 64-bit-range range coder with explicit carry handling. |
| `build.sh`, `build.bat` | The three-step build on Linux / Windows, for each program. |
| `test.sh`, `test.bat`, `timetest.cpp` | Round-trip and timing test that appends to `log.txt`. |
| `log.txt`, `log1.txt`, `log.pl` | The experiment log (size and timings per change) and the script that tabulates it. |

`fpaq0mw`:

| File | Role |
| --- | --- |
| `fpaq0mw.cpp` | Includes the model, the coder and `main.inc`. Compiled **twice**: once with `-DSIM_FUNC` (only the model step, to assembly) and once normally (the program). |
| `model.inc` | `Predictor`: the bitwise order-0 model (two counters and a mixer per context). |
| `sh_mixer.inc` | `iMixer`: a one-weight interpolating mixer that combines the two counters. |
| `coder.inc` | `Coder`: glues predictor and range coder; `P()` / `encode_sim()` are what the walk calls. |

`tangelo_w`:

| File | Role |
| --- | --- |
| `tangelo_w.cpp` | Same shape as `fpaq0mw.cpp`, plus the single definition of the model's shared globals. |
| `tangelo/tangelo.inc` | Includes the model's parts, in order; lists the five edits made to the original. |
| `tangelo/common.inc` | Probability scale, `MEM`, the `extern` declarations, locale-free character classes. |
| `tangelo/{window,random,ilog,idiv,hash,table_fsm,stsq}.inc` | History buffer, the random generator, the `ilog`/`1/(2i+3)`/`squash`/`stretch` tables, the hash, the 256-state FSM table. |
| `tangelo/{mixer_train,mixer1,mixer}.inc` | SSE dot product and weight update; the one-layer and two-layer mixers. |
| `tangelo/{counter,CM_small,CM,CM_match,APM}.inc` | `StateMap`, the small stationary maps, the 22-context `ContextMap`, the match model, the APM/SSE stage. |
| `tangelo/CM_main.inc` | `Model::predictNext()`: the contexts, the mixing, the APM chain. |
| `coder_tangelo.inc` | `Coder`: adapts Tangelo's inverted step to what the walk expects. |

`legacy/` keeps the original Windows-only scripts (`g.bat` for MinGW GCC, `c.bat`
for Intel ICX via LLVM `llc`, `1.pl`/`1a.pl`, `t.bat`, `icx64.cfg`) and
`tangelo_orig.cpp`, the single-file Tangelo this port started from.

## 3. The models

### 3.1 fpaq0mw: two counters and a mixer

**Context.** `Predictor::cxt` is the classic fpaq0 context: the bits of the
current byte seen so far, with a leading 1. It starts at 1 for every byte,
becomes `cxt*2+bit` after each bit, and is reset to 1 when it reaches 256 (after
the eighth bit). The 255 reachable values 1..255 are the internal nodes of a
binary tree whose 256 leaves are the byte values.

**Two counters per context.** Each context owns two 12-bit probabilities of a 1
bit (`SCALE = 4096`), updated as shift-based exponential moving averages at
different rates:

```
p1 += (SCALE - p1) >> 4   on a 1        p1 -= p1 >> 4   on a 0      (fast, 1/16)
p2 += (SCALE - p2) >> 8   on a 1        p2 -= p2 >> 8   on a 0      (slow, 1/256)
```

Because of the integer shifts the counters saturate short of the limits: `p1`
stays in [15, 4081] and `p2` in [255, 3841], so no probability is ever 0 or 1.

**The mixer.** `iMixer` (one 16-bit weight `w` per context, `SCALE = 32768`) does
not add stretched inputs like a logistic mixer. It **interpolates** between the
two counters:

```
P = p1 + ((p2 - p1) * w) >> 15          w = 0 -> p1,   w = 32768 -> p2
```

`w` starts at 32768, so a fresh context predicts with the slow counter.

Its update is a closed-form step towards the weight that would have predicted the
bit perfectly. Working in a 15-bit probability-of-zero space (`q = 32768 - 8p`),
with target `T = 100` if the bit was 1 and `32668` if it was 0 (i.e. about 0.3 % /
99.7 %), the ideal weight is the position of `T` on the line from `q1` to `q2`:

```
w* = (T - q1) / (q2 - q1)                (as a fraction, scaled by 32768)
w  = clamp( (5/6) * w + (1/6) * w*, 0, 32768 )       (wr = 32768/6)
```

The step is skipped when the two counters are within `Limit = 100` (of 32768) of
each other, where the division would explode and mixing cannot help anyway. Since
the map from the 12-bit probability-of-one space to the 15-bit
probability-of-zero space is affine, the interpolation weight found in one space
is the right weight in the other; that is why `Update` and `Mixup` can work in
different spaces.

**One model step.** `Predictor::update(y)` runs, in this order: mixer update
(using the old counter values), counter updates, context shift.
`Coder::encode_sim(y)` is "predict, then update" and returns the prediction:

```cpp
uint encode_sim( int y ) {
  uint p = predictor.P();     // probability of a 1, 12-bit
  predictor.update(y);
  return p;
}
```

One step writes exactly four words of state: `p1[cxt]`, `p2[cxt]`,
`mix[cxt].w` and `cxt`. Those four stores are what the journal has to capture.

### 3.2 Tangelo: a paq-class model

`tangelo/` holds a model of the kind this technique was built for: some hundreds
of times the state and the arithmetic of §3.1, and roughly the compression of the
paq8/lpaq family. Its step, `Model::predictNext()`, folds the bit just coded (the
global `y`) into the state and returns the probability of the next bit, also on a
12-bit scale.

**Contexts.** Once per byte (`bpos == 0`) it computes 22 context values and hands
them to the ContextMap: orders 0 to 4, taken straight from the last four bytes
`c4`; hashed orders 5, 6 and 14 from a rolling chain `cxt[]`; three sparse masks
of `c4` (`0xf8f8c0ff`, `0x00e0e0e0`, `0xffc0ff80`); three combinations of the
current match length with parts of `c4`; a character-class history (each byte
mapped to one of eight classes - letter, punctuation, space, `0xFF`, small,
medium, other - three bits per byte) and a hash of two such histories; four
indirect contexts built from `t1[]` and `t2[]`, which remember which bytes
followed a given byte and a given byte pair; and a word model, hashing the
current run of letters and the previous word.

**ContextMap** (`tangelo/CM.inc`) is most of the model and nearly all of the
memory: 2^22 buckets of 64 bytes, each holding seven 16-bit checksums and seven
slots of seven bit-history bytes, so one bucket covers one context's byte with a
nibble-indexed tree inside it. Per context and per bit it advances a bit-history
state through `State_table` (a 256-state nonstationary counter FSM, with a
randomised decay of high states - that is what `Random` is for), turns the state
into a probability with a per-slot `StateMap` (an adaptive probability/count
pair, stepped by `1/(2n+3)`), and separately runs a run model over the `runp`
bytes that predicts a repeat of the byte last seen in this context, weighted by
`ilog(run length)`.

**MatchModel** (`tangelo/CM_match.inc`) hashes the recent bytes into a table of
positions, and when the history repeats it predicts the byte that followed last
time, with a confidence rising in the match length. Two small stationary maps,
keyed on the position and on the distance since the last line break, ride along
with it.

**Mixing.** Everything above pushes stretched probabilities into a 73-input mixer
with 1160 weight sets, selected by five contexts (`c1`, `c0`, a summary of how
many contexts are active plus a few flags, `c2`, and the match length); a second
layer (`Mixer1<5,1,1>`) combines those five outputs. The result passes through
three APM/SSE stages keyed on `c0`, on `c0` with `c1`, and on a hash of `bpos`,
`c1` and `c2`, blended 5:15:12.

| | |
| --- | ---: |
| `sizeof(Model)` | 360.8 MB |
| ContextMap hash table | 256.0 MB (4 194 304 buckets x 64 B) |
| match model + history buffer | 96.3 MB |
| three APMs | 8.3 MB |
| mixer weights | 181.5 KB |

**The step is inverted** relative to §3.1, which is the one thing the port has to
reconcile. fpaq0mw's model predicts and is then told the bit; Tangelo's is told
the bit and then predicts. `coder_tangelo.inc` keeps the pending prediction in a
member, so the walk still sees "give me the prediction you are about to use, then
consume this bit":

```cpp
uint P()                 { return p; }                                  // the 8th-bit leaf
uint encode_sim( int b ) { uint p0=p; y=b; p=clamp(M.predictNext()); return p0; }
```

`p` starts at `hSCALE`, which is what Tangelo's own encoder seeds it with, and is
clamped into [1, SCALE-1] because the coder takes both `LOG2(p)` and
`LOG2(SCALE-p)` and cannot code a zero-width interval. The clamp applies to the
pending value only, so the model's own feedback path still sees its unclamped
`pr0`. (Tangelo's own coder instead does `p += p < 2048` before every use, which
nudges every probability below a half.)

**Where the byte boundary falls.** The walk calls the step for bits 0..6 and
reads `P()` for bit 7 (§4.2), so inside a simulated byte `c0` never reaches 256:
the byte-flush path - appending to the history buffer, shifting `c4`, and the
whole `bpos == 0` context rebuild - runs only in the real update. What the
simulation does touch is `bpos`, which it takes from 0 up to 7, and with it the
`if (bpos == 7) cn = 0;` in `ContextMap::mix`. Those are ordinary memory writes,
journaled and rolled back like everything else.

## 4. From bit probabilities to a byte distribution

### 4.1 Costs in the log domain

Multiplying eight 12-bit probabilities is done as a sum of logarithms.
`log2LUT` tabulates `LOG2(i) ≈ 2^16 · log2(i)` for `i` in [0, 32768] with a
16-bit fractional part, computed by repeated squaring in integer arithmetic
(the table values carry a constant bias of 31..32 units which cancels in the
differences taken below).

`test_encode()` fills `clen[0x200]`, indexed by tree node (`1` = root, children
`2n` and `2n+1`, leaves `0x100 + byte`). For a node the value is the accumulated
`Σ LOG2(p_i)` over the bits on the path to it, i.e. 2^16 times the
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
if clen[cty] > PRUNE_LOG * depth:        // pruning test, see below
    TEST_ENCODE<0, cty, K-1>             // descend into both children
    TEST_ENCODE<1, cty, K-1>
UNDO()                                   // roll the model back
```

`K` counts down from 7 at the root; `depth = 8 - K`. The `bit == 1`
instantiation still has to call `encode_sim` so that the model is in the right
state for its sub-tree, even though the two `clen` entries were already filled by
its sibling.

At the last level (`K == 0`, the eighth bit) no update is needed, because nothing
is predicted after it inside this byte, so the specialisation only reads `E.P()`
and fills the two leaves. This removes 128 of the 254 simulated steps and all of
their journaling (the log calls it "probability reuse on last bit").

### 4.3 Pruning

A sub-tree is entered only if its prefix is still probable enough:

```
clen[cty] > PRUNE_LOG * depth   <=>   Σ log2(p_i) > (PRUNE_LOG/2^16) * depth
```

With the fpaq0mw default of `0x90000` that is 9.0 per bit, i.e. `P(prefix)` is
still above `8^-depth`, a cost of under 3 bits per bit. Prefixes that fail keep
`0xFFFFF` in all their leaves, which becomes the minimum frequency 1 in the next
step.

The threshold is a speed/size trade-off, and the right value depends on how sharp
the model is; it is a macro (`PRUNE_LOG`), defaulted per program and overridable
from the build. `log.txt` records the fpaq0mw sweep: `0x80000` (4 bits/bit) makes
the distribution effectively exact at 446 945 bytes and 1.96 s, `0x90000` gives
446 962 at 1.77 s, `0xA0000` (2 bits/bit) already loses 3.6 %, and `0xC0000`
prunes everything so the output is the raw file. Tangelo predicts far more
sharply, so the same threshold cuts deeper into the distribution that matters and
its default is `0x80000` instead - see the table in §10.

### 4.4 Frequencies

For each byte value `d`:

```
r       = LOG2(SCALE)*8 - clen[0x100+d]      // 2^16 * cost of d in bits
r       = min(r, 0xFFFFF)                    // cap at ~16 bits
freq[d] = unlog[r]                           // = floor(2^16 * 2^(-r/2^16)), at least 1
```

`unlog` is a 2^20-entry table built once with `pow()` at start-up
(`PSCALE = 2^16`). The frequencies therefore sum to about 2^16 (plus rounding and
the 1s of pruned symbols), and `total` is their exact sum, computed on the fly and
identical in encoder and decoder.

## 5. Journaled speculative execution

### 5.1 The journal

```cpp
struct Cell { void* ptr; uint msk; uint val; };   // 16 bytes
Cell trk[1<<20];        // the journal
uint trkptr;            // next free cell
uint nest_trkptr[0x200], nesting;   // stack of saved journal positions
```

`NEST()` pushes `trkptr`. `UNDO()` pops it and walks the cells written since
then **backwards**, so that several writes to the same location unwind in the
right order. `msk` records the width: a 1-, 2- or 4-byte write is saved as a
32-bit read of its address, masked, and restored at its own width. (Restoring
every cell with one 32-bit read-modify-write would be simpler, and correct - the
extra bytes are written back unchanged - but it reads and writes up to 3 bytes
past a narrow cell, which is a fault at the end of a mapping, and it measured
9 % slower.) Wider writes are recorded as several cells.

Only the current root-to-node path is ever live in the journal, so the capacity is
far larger than needed: fpaq0mw uses at most seven steps of four writes, and
Tangelo a few thousand cells. `NEST()` checks the remaining capacity once per
step, since a journal that silently ran off the end would corrupt memory rather
than restore it.

### 5.2 Which writes are journaled

Only the instrumented copy of the model step journals (§5.4), and it journals
every write it makes to memory that is not its own stack frame:

* **Plain stores** - `mov SIZE PTR <mem>, <src>`, for SIZE from `byte` to
  `qword`.
* **Vector stores** - `movdqa`/`movdqu`/`movaps`/`movq`/`vextract…`/`vpmovwb`
  and friends with a memory destination, up to `zmmword`. The Tangelo build
  produces 16-byte stores from the SSE mixer update in `tangelo/mixer_train.inc`
  and 32-byte ones from loops the auto-vectorizer widened.
* **Read-modify-writes** - `add`/`inc`/`and`/`xadd`/… with a memory destination.
  These are exactly as journalable as a plain store, because `UNDO()` only needs
  the destination's contents from before the instruction ran, not a description
  of what it did to them. Tangelo's `++ptr` in the match model compiles to
  `inc DWORD PTR`, so this is not hypothetical.
* **Not the stack.** A write relative to `rsp` is skipped whatever the
  instruction: it belongs to the step's own frame, which is dead by the time
  `UNDO()` runs - and `UNDO()`'s own frame would then sit at the same addresses,
  so restoring it would be actively harmful.
* **Nothing else.** Any other memory-writing instruction fails the build. A write
  the journal does not see cannot be undone, and the resulting failure is silent
  (§11), so the default is to refuse rather than to guess. Four families are
  named explicitly, because each looks journalable and is not:
  - scatters and `rep stos`: one instruction, many addresses, no `lea` that
    reproduces them;
  - `maskmovdqu` and `movdir64b`: the address is implicit, so there is no memory
    operand in the text to `lea` at all - and without this they would be *skipped
    in silence* rather than refused, which is the one outcome the script exists
    to prevent;
  - `bts`/`btr`/`btc` with a register bit offset: these address a bit, and the
    write lands at `<mem> + (reg DIV opsize_bits) * opsize_bytes`, outside the
    operand `lea` would capture. With an immediate offset the bit index is taken
    modulo the operand size, so that form is journaled normally;
  - a destination carrying a segment prefix, since `lea` computes the effective
    address and ignores the segment, and `xchg`/`xadd`/`cmpxchg` with the memory
    operand second, which the memory-destination parse does not cover;
  - an indirect `call` *or* `jmp` - a tail call is a call, and a jump table is
    told apart by its `.L` target;
  - stores under an AVX-512 write mask: the *restore* would be a harmless no-op
    for the masked-off lanes, but the *save* is not. A suppressed lane may
    address unmapped memory without faulting, so reading the full width to
    journal it faults exactly where the store would not.

The instrumentation writes `push ARG` at `[rsp-8]`, so the 128-byte **red zone**
below `rsp` must not be in use. The instrumented compile gets `-mno-red-zone` for
that reason, and `track.pl` stops if it sees a negative `rsp` displacement anyway.
This is not theoretical either: GCC put a spilled accumulator at `-8[rsp]` in the
Tangelo step.

**Calls out of the step** were the remaining hazard. Anything the step calls that
was compiled into the same translation unit is instrumented too, because
`track.pl` processes the whole file. A call to a *library* function is not, and
its writes would escape in silence. Three things reach for one:

* `memset()` in the ContextMap's bucket eviction, and the `<ctype.h>` calls in
  the byte classifier - both rewritten in the model source, the latter also
  making the output independent of the process locale;
* the compiler, which turns ordinary loops into library calls: Clang compiles the
  mixer's tail-zeroing `while (nx & 7) tx[nx++] = 0;` into `call memset@PLT`.
  The instrumented compile therefore gets `-fno-builtin`.

`track.pl` refuses any call whose target this file does not define (and any
indirect call), so the question is settled at build time rather than by
inspection. With `-fno-builtin` the instrumented Tangelo step contains no calls
at all under GCC, Clang and MinGW alike.

The same argument applies to **static initialisers**: a constructor in the
instrumented translation unit would be journaled too, filling the journal before
`main()` with entries nothing ever undoes. That is why the model's tables are
defined only in the main translation unit, and why `track.pl` refuses an input
containing an `.init_array` entry.

### 5.3 The journaling stubs

`track1`, `track2` and `track4` (in `track.inc`) append one cell each; `track8`,
`track16`, `track32` and `track64` append 2, 4, 8 and 16 cells, one per 4-byte
word of the destination, since `Cell::val` is 32 bits. All of them take the
address in the first-argument register (`rcx` on Windows, `rdi` on Linux).

They are written in assembly using only `mov`, `movzx`, `lea`, `push`, `pop` and
`ret`, so they preserve every register **and EFLAGS**. That matters twice over:
the compiler keeps comparison results live across stores (the GCC output for
fpaq0mw has `cmp` / `mov [mem],reg` / `cmovge`), and a read-modify-write such as
`inc [mem]` sets flags that a following branch reads. A stub that clobbered flags
would corrupt the model in both cases. The wide stubs are unrolled rather than
looped for the same reason: a loop counter needs a flag-setting instruction.

The original C stubs used `__attribute__((no_caller_saved_registers))`, which
saves registers but not flags, and needed a `pushf`/`popf` pair around every
call; `log.txt` records that dropping the pair ("drop pushf") took fpaq0mw from
7.5 s to 3.2 s, which is why the stubs are now flag-neutral by construction.

### 5.4 Two copies of the model, and keeping them apart

The program contains the model step twice: the instrumented copy, compiled with
`-DSIM_FUNC` and patched by `track.pl`, which the walk calls through the
`extern "C" encode_sim`; and the ordinary copy, inlined into `main()`, which
performs the real update after the byte is coded. Both come from the same source
with the same flags, so they agree on the layout of every object - and they must
operate on the *same* objects, which is why the Tangelo model's mutable globals
(`y`, `bpos`, `rnd`, `ilog`, `pt`, `stretch`) are only declared in
`tangelo/common.inc` and are defined once, in `tangelo_w.cpp`. If any of them
existed twice, the simulation would quietly drift away from the model it is
supposed to be simulating.

The subtler version of the same trap is C++ linkage. A member function the
compiler declines to inline is emitted as a **weak COMDAT symbol** - and the
normal build emits the same symbol from the same source. The linker keeps exactly
one of the two, and if it keeps the uninstrumented one, the walk runs with no
journaling at all: every speculative branch then leaks into the real model,
`UNDO()` restores nothing, and the model is destroyed a little more with every
byte. Nothing crashes. Both sides of the codec corrupt the model identically, so
the round trip still succeeds; it just compresses terribly. This is what happened
first during the Tangelo port - `Model::predictNext()` was too big for GCC to
inline - and it turned `book1`'s first 64 KB into 130 088 bytes instead of
20 739.

Three things now prevent it:

1. The functions a compiler might leave out of line - `Model::predictNext()`,
   `ContextMap::mix()`, `MatchModel::matchModel()`, `E::get()`, `Mixer::p()` and
   `Coder::encode_sim()` - are marked `INLINE` (`always_inline`), so the
   instrumented translation unit defines exactly one symbol: `encode_sim`. GCC
   needed three of those and Clang a different three.
2. `track.pl` refuses an input that exports anything besides that one entry
   point. The test is on the symbol set rather than on a directive, because the
   two ABIs spell the same C++ COMDAT differently: `.weak` on ELF, but `.globl`
   inside a `.linkonce` section on COFF - so a guard written against the ELF
   spelling would have left the entire Windows build unprotected. (MinGW's
   `.refptr.*` address thunks are exempt; they are COMDAT by design.)
3. `main.inc` checks, after the first byte, that the walk journaled and rolled
   back something, and stops if it did not.

### 5.5 Proving it, rather than hoping

Neither half of the journal's contract is testable by round-tripping: encoder and
decoder run the same code, so they corrupt the model identically and still agree.
`-DTRACK_VERIFY=n` therefore builds a binary that checks both directly.

**The journal restores everything.** For the first `n` input bytes, every byte of
model state is copied aside, the walk runs, and the state is compared byte for
byte. The ranges compared are the whole `Coder` object plus anything a program
lists in `TRACK_VERIFY_RANGES` - for tangelo_w that is `y`, `bpos` and the random
generator. A single write escaping the instrumentation shows up as a named offset
at the byte where it happened.

**The simulation reproduces the real step.** After coding each byte, the eight
real per-bit predictions are turned back into code lengths and summed; the total
must equal the walk's `clen[0x100+c]` for that symbol exactly, for every byte of
the file (unless the symbol's prefix was pruned). That catches the subtler
failure where the journal restores memory correctly but the simulated and real
paths do not correspond - a global that exists twice, say, or a model whose step
is not a pure function of its state and the bit.

Both programs pass, on all three toolchains - GCC 13, Clang 18 and MinGW-w64
(run under wine). `fpaq0mw` is checked over all of `book1`; `tangelo_w` with all
360.8 MB of its model state compared byte for byte, and its predictions checked
over every byte of `book1`.

One path needs help to be reached: at 4 194 304 buckets the ContextMap never
evicts anything in a short run, so the eviction branch of `E::get` - the one that
overwrites a bucket's checksum and clears seven bytes of bit history - would go
untested. Building with `-DMEM=16384` shrinks the table 512-fold; with eviction
running constantly and the state compared after every one of 65 536 input bytes,
the journal still restores every byte.

The check is not vacuous. Deleting a single one of the 146 `call trackN`
sequences from the instrumented assembly by hand - one 4-byte write in the mixer
- and rebuilding gives, on the very first input byte:

```
track: UNDO() did not restore Coder+378369996 at input byte 0: 00 -> 88
  a memory write in the model step is escaping the journal
```

Which is also the answer to "how would you know?": that write is one of tens of
thousands per byte, it corrupts the model by a single mixer weight, and without
this check its only symptom would have been a slightly larger output file.

**What it does not cover.** The prediction half catches anything that makes the
simulated step diverge from the real one - a write escaping the journal, a global
that exists twice, the two translation units disagreeing about the layout of
`Coder` - because in each case the walk predicts one thing and the model then
does another. It cannot catch a fault that moves both in step. Entering the walk
somewhere other than a byte boundary is the example: the walk would explore bits
1..7 of the byte instead of 0..6, the real update would consume the same eight
bits from the same state, their code lengths would agree exactly, and the output
would simply be 4.6 % larger. So `main.inc` asserts that invariant directly,
before every walk, in the normal build:

```
track: the speculative walk entered mid-byte at input byte 0 -
  the byte distribution it produces is meaningless.
```

## 6. The build pipeline: instrumenting the compiler's output

Per program (`fpaq0mw`, `tangelo_w`):

```
<prog>.cpp  --(CXX -S -masm=intel -mno-red-zone -DSIM_FUNC)-->  coder-<prog>.s
coder-<prog>.s  --(perl track.pl)---------------------------->  coder1-<prog>.s
<prog>.cpp + coder1-<prog>.s  --(CXX)------------------------>  <prog>[.exe]
```

`track.pl` reads the Intel-syntax assembly and, in front of every write listed in
§5.2, inserts:

```
push ARG                 ; ARG = rcx (Win64) or rdi (SysV x86-64)
lea  ARG, <mem>
call trackN              ; N = width of the write in bytes
pop  ARG
```

The address is computed with `lea` from the same operand the write uses, so the
stub sees the exact location before it is overwritten; the `push` does not
disturb that even when the operand is based on `ARG` itself. The ABI is
auto-detected from the assembly (COFF `.seh_proc`/`.def` directives mean Win64,
ELF `.type name, @function` means SysV) and can be forced with `--abi=`. On
Windows the script also appends the `.def` declarations the original scripts
emitted. `-v` lists every write it instruments or skips.

For fpaq0mw, GCC and Clang both produce exactly four plain stores (three `word`,
one `dword`) and nothing else, matching the four state words of §3.1. For
tangelo_w the same compile produces about 146 writes in six different widths,
plus about 67 stack writes that are skipped.

Why assembly-level instrumentation rather than a compiler option: it needs no
support from the compiler, it applies to already optimised code (the writes that
survive optimisation are exactly the ones that matter), and it leaves the model
source almost untouched - the Tangelo port needed five edits to the model itself,
all of them listed at the top of `tangelo/tangelo.inc`.

## 7. The range coder

`sh_v1m.inc` is a byte-oriented range coder with a 32-bit `low`, a separate carry
bit, and a 64-bit `range` (so that it can start at exactly 2^32).

**Interval selection.** For a symbol with cumulative frequency `cum`, frequency
`freq` and total `tot`, the sub-interval is computed from the top down:

```
tmp  = range - floor((tot - cum)        * range / tot)     // start
rnew = range - floor((tot - cum - freq) * range / tot)     // end
encoder: low += tmp        decoder: code -= tmp
range = rnew - tmp
```

Written this way the 256 sub-intervals tile `[0, range)` exactly: the first symbol
starts at 0 and the last one ends at `range`, with no rounding gap.

**Renormalisation.** While `range < 2^24` the coder shifts out a byte:
`range <<= 8`, and the encoder runs `ShiftLow()` while the decoder pulls in the
next input byte.

**Carry propagation.** `low` and `Carry` overlay a 64-bit `lowc`, so
`lowc += tmp` deposits any carry out of bit 31 into `Carry` for free.
`ShiftLow()` is the usual cached-byte scheme: the top byte of `low` is held back
in `Cache`; if the byte to be shifted out is `0xFF` and no carry is pending it is
only counted (`FFNum++`), because a later carry could still turn it into `0x00`
and bump `Cache`. When a non-`0xFF` byte or a carry arrives, `Cache + Carry` and
then `FFNum` bytes of `0xFF + Carry` are written out.

**Flush.** `rc_Quit()` widens `low` with as many trailing `0xFF` bytes as still
fit below `low + range` and then omits them from the output; the decoder reads
`0xFF` for every byte past the end of the file (`getc` returns EOF, which
`byte()` truncates to `0xFF`), so the truncated tail decodes identically. The
decoder pre-loads four bytes into `code`, matching the 32-bit `low`.

**Decoding a symbol.** `rc_GetFreq(tot) = floor(code * tot / range)` gives a
value in `[cum, cum + freq)` of the coded symbol; `main()` finds it with a linear
scan over the cumulative frequencies. (The fpaq0mw `Coder` also keeps a binary
`encode_p`/`decode_p` interface from earlier versions; the bytewise path does not
use it.)

## 8. Main loop and file format

The output is a 4-byte native-endian length followed by the range coder stream.
Per byte, encoder and decoder run the same code except for where the symbol comes
from:

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

After the last byte the encoder calls `flush()`. The decoder knows the count from
the header, so no end-of-stream symbol is coded.

## 9. Numeric formats

| Quantity | Format | Range / notes |
| --- | --- | --- |
| Model output `p` | 12-bit probability of a 1 (`SCALE = 4096`) | both models; clamped to [1, 4095] for the coder |
| fpaq0mw counters `p1`, `p2` | 12-bit probability of 1 | [15, 4081] / [255, 3841] |
| fpaq0mw mixer weight `w` | 16-bit, 15 fractional bits | [0, 32768], starts at 32768 |
| Tangelo mixer weights | 16-bit, dot product `>>8` per pair, output `>>5` then squashed | trained with error `((y<<12)-pr)*7` |
| `LOG2(i)` | 16.16 fixed-point log2 | table for `i` ≤ 32768, bias ≈ −31.5, cancels |
| `clen[node]` | 2^16 · Σ log2(p) along the path | init `0xFFFFF`, root 0 |
| `PRUNE_LOG` | 16.16 log2 per bit | `0x90000` = 3 bits/bit (fpaq0mw), `0x80000` = 4 bits/bit (tangelo_w) |
| `r` (byte cost) | 2^16 · bits | capped at `0xFFFFF` ≈ 16 bits |
| `freq[d]` | 2^16 · P(d) (`PSCALE = 65536`) | at least 1; `unlog` table of 2^20 entries |
| Range coder | `low` 32 bit + carry, `range` 64 bit | renormalise below 2^24, bytewise |
| Journal cell | `{ptr, msk, val}` = 16 bytes | 2^20 cells, nest stack of 512 |

## 10. Performance

### 10.1 fpaq0mw

`log.txt` is the author's lab notebook: after each change, `t.bat` appended the
encode and decode wall time and the compressed size of `book1` (Calgary corpus,
768 771 bytes); `log.pl` tabulates it into `log1.txt`. The key milestones:

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
| `024`: pruning at `0x90000·depth` | 446 962 | 1.77 s | 1.79 s |

Those were measured on the author's Windows machine with Intel ICX or MinGW GCC;
the comment lines are the author's.

### 10.2 tangelo_w

Measured here, on `book1` and on its first 64 KB, with GCC 13 `-O3
-march=native` on a 2.8 GHz Xeon. The reference row is `legacy/tangelo_orig.cpp`,
the same model with its own binary arithmetic coder, coding bit by bit.

| | `book1` size | enc | 64 KB size | enc |
| --- | ---: | ---: | ---: | ---: |
| Tangelo, bitwise (reference) | 197 022 | 5.0 s | 20 706 | 0.68 s |
| tangelo_w, no pruning | | | 20 734 | 14.3 s |
| tangelo_w, `PRUNE_LOG=0x80000` (default) | 197 483 | 97.2 s | 20 739 | 10.2 s |
| tangelo_w, `PRUNE_LOG=0x90000` | | | 20 804 | 7.5 s |
| tangelo_w, `PRUNE_LOG=0xA0000` | | | 21 307 | 5.2 s |
| tangelo_w, `PRUNE_LOG=0xB0000` | | | 25 160 | 3.7 s |

Two things are worth reading off that table. First, the bytewise reformulation
does not pay for itself in size: even with the walk unpruned, coding the byte in
one step costs 28 bytes more than coding its eight bits separately, because the
256 frequencies are quantised to a 2^16 scale with a floor of 1 while the binary
coder uses the model's 12-bit probability directly. Second, the pruning threshold
that was right for fpaq0mw is not right here: at `0x90000` Tangelo loses 0.34 %,
where fpaq0mw lost 0.004 %, because a sharp model puts real probability mass on
prefixes that a 3-bit-per-bit cut-off throws away.

The cost is the point of the exercise. Measured over 64 KB, `tangelo_w` rolls
back 2.47 billion journal cells on text and 6.20 billion on random data (which
prunes almost not at all) - around 38 000 cells per input byte, or 600 KB of
journal traffic for every byte of output. What it never runs short of is
capacity: peak live journal use is 2 527 cells of the 1 048 576 available, 0.24 %,
because only the current root-to-leaf path is ever live.

| | text (64 KB of book1) | random (64 KB) |
| --- | ---: | ---: |
| cells rolled back | 2 469 607 148 | 6 195 434 587 |
| peak live cells | 2 527 | 2 383 |
| of `N_Cells` | 0.24 % | 0.23 % |

## 11. Limitations and caveats

* **Only fixed-width writes are journaled.** A model whose compiled step
  contains scatter stores or string operations makes `track.pl` fail the build
  (by design). Such a model needs either source changes that steer the compiler
  towards ordinary writes, or new stubs plus script support for the instruction
  forms involved.
* **Calls out of the model step are invisible** unless the callee is compiled in
  the same `SIM_FUNC` translation unit (then it is instrumented too). `track.pl`
  now refuses any call it cannot see the target of, so this fails the build
  rather than corrupting the model - but a new model may need source changes, or
  more `-fno-…` flags, to stop the compiler reaching for a library routine.
* **Weak symbols are a silent trap**, and the reason for the three guards in
  §5.4. Any future model whose step is too large to inline will hit it.
* **Stack writes are skipped** on the assumption that the model keeps no state in
  the step's own frame, and they are recognised by `rsp` appearing among the
  address expression's registers. Where `rbp` is also a frame pointer an
  `rbp`-based write is ambiguous - frame slot or model pointer - so `track.pl`
  refuses that combination rather than guess. It is not hypothetical in either
  direction: GCC sets `rbp` up here purely to realign the stack while still
  addressing every local off `rsp`, and MinGW has no frame pointer and uses
  `rbp` as the base for ContextMap writes. Build with `-fomit-frame-pointer`, as
  the scripts do.
* **Journal capacity** is 2^20 cells and 512 nesting levels. `NEST()` checks both
  once per step and exits rather than overrunning, but there is no per-write
  check, because that is the hot path: a single model step writing more than the
  64 K-cell margin would still overrun. `trk[]` is allocated with that margin
  past `N_Cells` so that such an overrun stays inside the array rather than
  landing on the very variables the check reads (`nest_trkptr` follows `trk`
  immediately in bss). Measured peak use is 2 527 cells.
* **Both copies of the model must match**: the instrumented `encode_sim` and the
  inlined one are compiled separately, so they must be built with the same
  compiler and the same flags (the build scripts enforce this by using one
  `CXX`/`CXXFLAGS` for both steps; only `-mno-red-zone` differs, which changes no
  arithmetic).
* **The `unlog` table is built with floating-point `pow()`.** Encoder and decoder
  of the same binary always agree, but a different libm could in principle round
  one table entry differently, which would make streams incompatible between
  builds. In practice the tables from GCC and Clang on Linux and from MinGW on
  Windows produced identical output in this port.
* **Pruning makes the coded distribution an approximation** of the model's; this
  is harmless for correctness (the decoder computes the same approximation) but
  it costs compression, tunably (§4.3).
* **Decoding is a linear scan** over 256 cumulative frequencies; a binary search
  would be the obvious improvement if the coder itself ever mattered.
* **Files are limited to 4 GiB** by the 4-byte length header, and there is no
  error checking on the input (a truncated stream decodes silently as `0xFF`
  bytes).
* **tangelo_w needs about 380 MB of address space** for its model, in BSS. It
  relies on that memory starting out zeroed, which is true of BSS but was *not*
  true of the `new byte[]` the original `tangelo_orig.cpp` used.
* **x86-64 only**, GNU assembler syntax only: GCC or Clang on Linux, MinGW-w64
  GCC or Clang on Windows. The MSVC/Intel toolchain path of the original `c.bat`
  (ICX → LLVM bitcode → `llc` → MASM-style listing) is not ported.
