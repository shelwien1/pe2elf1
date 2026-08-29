# What to try next, and why

A plan, written off `misc/model0_process_skl.asm` (the annotated AVX2 encode
listing), `misc/avx2_profile.md` (the measured encode budget) and the source.
Everything below is either a measurement to make or an experiment to run;
nothing here is a claim that something is faster.

Numbers are from this box at **3.290 GHz** (`misc/clk.c` -- not the 2.8 GHz
`/proc/cpuinfo` reports, and not the 3.235 GHz of `avx2_profile.md`, which was
a different container instance). clang 18.1.3, `-O3 -flto -march=skylake`,
20 MB of enwik8. Absolute MB/s does not carry across instances; ratios do.

## Results -- all seven items, tested

Everything below the line was written before any of it was tried. This section
is what happened. Same box, 3.290 GHz, 20 MB of enwik8, medians of best-of-5
over 6 round-robin runs with a copy of the baseline binary in the rotation.
Every variant produces a **byte-identical stream**; the compressed size never
moved, on any target, in any configuration.

| | encode | decode |
|---|---|---|
| **`-march=skylake` (AVX2)** | | |
| baseline (before this round) | 72.72 | 39.83 |
| defaults now | **78.90 (+8.5%)** | **47.31 (+18.8%)** |
| defaults + `RC_THREADS=1` | **128.49 (+76.7%)** | 47.33 (+18.8%) |
| *control* | +1.15% | -1.21% |
| **`-march=native` (AVX-512)** | | |
| baseline | 80.82 | 39.88 |
| defaults now | 81.79 (+1.2%) | **46.95 (+17.8%)** |
| defaults + `RC_THREADS=1` | **121.57 (+50.4%)** | 46.95 (+17.7%) |
| *control* | +0.22% | +1.24% |

Item by item:

| # | what | result |
|---|---|---|
| 1 | fuse `pp` into the counter step | **TAKEN** -- `RC_FUSE_PP_ENC/DEC`, decode +18% on both targets, encode +2.7% on AVX2 |
| 2 | decode: eager child load | lost, -3.4% decode -- `RC_EAGER_CTY`, default 0 |
| 3 | decode: shrink the loop body | lost at every unroll factor, -7.6% to -30% |
| 4 | AVX-512 at 512-bit | lost, -3.2% encode, and it halves the sweep loop |
| 5 | interleave model pass and sweep | **TAKEN** -- `RC_CHUNK=2048`, encode +3% on both targets |
| 6 | model pass on a second thread | **TAKEN** -- `RC_THREADS=1`, encode +64%, decode untouched |
| 7 | `-fno-pie` | marginal: encode +0.4%, decode +2.5% on AVX2; a packaging call |

What the plan got right and wrong:

- **Right about where to look.** Decode had never been profiled and was the
  bigger half; it is now 18% faster and the *encoder* is the slow side again.
- **Right about the mechanism for item 1**, and right that the old
  "pp fused into FSM" result was explained by the address shape rather than
  the table size. Wrong about the size of the encode half of it: the plan said
  ~+9% from "two fewer instructions per bit", and the real answer is that clang
  micro-fuses `add pp(,%state,2),%p`, so the load costs no instruction slot and
  the encode gain is 2.7% from the load port, not the front end.
- **Wrong about item 5 being a re-test of a known loss.** It is a different
  change: both passes now work the low end of `pbit`, so the chunk is rewritten
  in place. The old experiment's stated mechanism (FSM+pp evicted from L2) was
  never real -- 256 states, 2 KB of tables.
- **Wrong about item 2 and item 4, in the same way.** Both shorten the
  instruction stream and both lose. Item 2's own failure is what identified the
  reason: once item 1 took the dependent load off the chain, decode stopped
  being chain-bound, so the lever it was aiming at had already moved.
- **Right about item 6 being the largest, and about the ceiling.** 1/0.57 =
  1.75x predicted, 1.77x measured on AVX2.

One bug came out of it: `RC_CHUNK` as first written broke the carry fallback,
which re-codes a whole block out of `pbit[]` that the chunked pass no longer
holds. A lost carry is a ~2^-33 event, so no test would have found it; forcing
the fallback segfaults. The model is now snapshotted per block and the fallback
rewinds. See the commit.

---

## 0. Where the time actually is

|  | MB/s | cyc/byte | cyc per group of 16 bits |
|---|---|---|---|
| encode | 73.92 | 44.5 | **89.0** |
| decode | 39.77 | 82.7 | **165.4** |

**Decode costs 1.86x what encode does and has never been profiled.** Every
measurement in `avx2_profile.md`, every knob in `rc_config.inc` and the whole
annotated listing are about encode. That asymmetry is the largest single fact
in this document.

The two sides are in completely different regimes:

| | static insns per group | bytes | regime |
|---|---|---|---|
| encode: `model_pass` loop | 108 per **8** bits (216/group) | 470 | ~4 uops/cycle, **rename-bound** |
| encode: sweep loop | 145 per 16 bits | 908 | ~4.1 uops/cycle, **rename-bound** |
| decode: group loop | 404 per 16 bits | 1756 | **<2.4 IPC -- latency-bound** |

Encode is at the 4-wide rename ceiling on both loops, which is why every win
there has been "fewer uops" and why two recent attempts that *shortened* the
loop still lost (`avx2_profile.md` 9c). Decode is nowhere near it: 404 static
instructions against 165.4 cycles is 2.44, and most of that body is the
rarely-taken refill and never executes, so the real figure is well under 2.
It runs a serial per-bit chain and spends most of its cycles waiting.

### The one fact that changes the old conclusions

`FSM0.txt` has **256 states**, max next-state 254. The declared arrays are
`fsm FSM[32768]` (128 KB) and `word pp[32768]` (64 KB), but the live parts are:

    FSM   256 x 4 B  = 1 KB
    pp    256 x 2 B  = 512 B
    cty   256 x 2 B  = 512 B
                       ------
                       2 KB, i.e. 1/16 of L1d

The model's whole working set fits L1 sixteen times over. Two conclusions in
`avx2_profile.md` were written against the declared sizes and are therefore
unsupported:

- **"`pp` fused into `FSM` costs 3.42 cyc/grp because it doubles the table"**
  (section 8). It cannot have been footprint. The likely cause is the one
  section 9 later found and fixed for the plain FSM: a fat entry makes the
  address `base + s*8 + bit*2`, two scaled terms, which clang cannot fold into
  one addressing mode -- exactly the thing that cost six `lea`+`add` pairs per
  byte before `counter.inc` was rewritten to a single scaled index.
- **"chunking loses because tmpbuf evicts FSM+pp from L2"** (section 8). With
  a 2 KB working set nothing evicts them from L1, let alone L2.

Both experiments deserve a retry, and the first one is now the headline item.

---

## 1. Fuse `pp` into the counter step -- keeping ONE scaled index

*Helps both sides, for different reasons. Do this first.*

Per coded bit the model does three dependent L1 loads:

    state = cty[ctx].state          // 2 B
    p     = pp[state]               // 2 B   <- depends on the load above
    next  = FSM[state*2 + bit]      // 2 B   <- depends on the load above

Measured in `model_pass`: **3 loads, 2 stores and 3 `lea` per bit**, 13.5
instructions per bit total.

**The change.** Make the counter a dword holding `{state, p}` and the FSM
entry a dword holding `{next_state, pp[next_state]}`:

    word2 CTY[512];        // {state, p} -- 512 entries, see item 2
    word2 FSM2[256*2];     // FSM2[s*2+bit] = { FSM[s*2+bit], pp[FSM[s*2+bit]] }

Per bit that becomes:

    sp   = CTY[ctx]                 // 4 B -- gives state AND p in one load
    ...
    CTY[ctx] = FSM2[state*2 + bit]  // 4 B -- value already carries the next p

Two loads instead of three, and the `pp` load and its address arithmetic
disappear. `FSM2[s*2+bit]` is still `base + (s*2+bit)*4`, a single scaled
index with scale 4 -- **this is the whole point**; a `struct{word s[2]; word p;}`
would reintroduce the two-term address that section 9 removed. Tables become
2 KB + 1 KB (from 1 KB + 512 B): still nothing.

**Expected size, encode.** ~2 instructions per bit out of 13.5 (~15%) off a
rename-bound loop that is 57% of encode: 51.4 -> ~44 cyc/grp, i.e.
**89.0 -> ~82, about +9%**. An estimate, not a measurement.

**Expected size, decode.** Larger, and for a different reason -- see item 2.

**Falsified if** `model_pass` does not lose ~2 instructions per bit. Check the
loop's static instruction count before believing any timing.

**Watch for:** the packing order. `pbit[] = (bit<<15)+p` wants `p` cheaply, the
FSM index wants `state` cheaply. Try `{p:16, state:16}` and `{state:16, p:16}`
and count instructions; one of them will need a shift the other does not.
`p < 2^15` and `state < 2^8`, so a `{p:15, state:9}` bitfield in one dword is
also available if the shift falls the wrong way.

---

## 2. Decode: break the pointer chase with an eager child load

*The big decode item. Depends on item 1.*

Decode's per-bit serial chain is:

    cty[ctx] load (~4-5) -> pp[state] load (~4-5) -> imul (3) -> cmp/setae (1)
      -> bit -> ctx = ctx*2+bit (1) -> next cty[ctx] load ...

which is ~13-15 cycles of latency against a measured **10.34 cyc/bit** -- so
the loop is running close to its dependency height, with the rangecoder's
own work (per-lane, independent) hiding underneath. Item 1 removes the second
load from that chain outright.

The first load can come off it too. A byte walks a root-to-leaf path in a
256-node binary tree: at bit *j* the context is `ctx`, and at bit *j+1* it is
either `2*ctx` or `2*ctx+1` -- **two adjacent entries**. With dword counters
that is one 8-byte load, and its address is known one bit early:

    // at bit j, having just computed ctx for bit j:
    qword kids = *(qword*)&CTY[2*ctx];      // issued now, used at bit j+1
    // at bit j+1:
    sp = bit ? uint(kids>>32) : uint(kids); // 1 cycle, no load on the chain

The chain becomes `select (1) -> imul (3) -> cmp (1) -> bit` ~= 5-6 cycles per
bit instead of ~10. Go one level further (four grandchildren = one 16-byte
load, two bits ahead) if the two-level version measures well.

**Safety.** Within a byte the eight contexts visited are distinct nodes on one
root-to-leaf path, and the update writes the node just *left*, never a
descendant -- so the speculative read can never miss a write. Across bytes
`P_byte` resets `ctx` to 1. The one edge is bit 7, where `2*ctx` reaches 510:
pad `CTY` to 512 entries (2 KB) and the load is always in range. No branch, no
check.

**Expected size.** If decode is really chain-bound, 165.4 -> ~110 cyc/grp,
i.e. **+50%**. This is the largest single number in this document and also the
least certain one -- measure item 0's split first.

**Falsified if** the decode-with-constant-`p` probe (below) shows the model
chain is not the binding constraint.

**Measure the split first.** Build a decoder whose `p` is the constant
`hSCALE` instead of `pp[state]`, with the model update deleted. It decodes
garbage, but the block length header is coded at `hSCALE` explicitly so
`blksize` still comes out right and the loop still runs the right number of
iterations. The gap between that and the real decoder is the model chain's
share -- the decode equivalent of section 7's "no store" probes.

---

## 3. Decode: get the refill out of the loop body

*Cheap, independent of 1 and 2.*

The decode group loop is **404 instructions in 1756 bytes** for 16 bits,
against the encode sweep's 145 in 908 for the same 16 bits -- and 16 of those
instructions are `jbe`, one predicted-not-taken renorm test per lane. Most of
the body is sixteen inlined copies of the rare byte-refill path
(`rc.inc`'s `if_e0(range<sTOP) { ... if_e0(range<gTOP) { ... } }`), which
~91% of bits skip.

Push the taken side out of line -- a `NOINLINE` cold helper, or
`__attribute__((cold))` on the refill -- so the loop body shrinks toward the
~100-instruction hot path it actually executes. Straight-line fetch of 1756
bytes per group is not free even when the branches predict.

**Expected size.** Unknown; front-end effects are hard to predict without
counters. Worth measuring because it is a one-line change.

**Falsified if** the hot-path instruction count does not drop, or if the extra
call overhead on the 9% path outweighs it. Count both.

---

## 4. AVX-512 at 512-bit vector width

*Encode only, one flag, untried.*

`-march=skylake-avx512` implies `-mprefer-vector-width=256`, so the sweep runs
`RCNUM=16` as **two** ymm per lane array. That is what puts 19 ymm into a
16-register file and produces the 27 uops of spill traffic the annotated
listing accounts for. At 512 bits it is **one zmm per array**: six lane arrays
plus two staging arrays plus three constants = 11 of 32 registers. The spills
go away and the arithmetic instruction count roughly halves.

    ARCH=native OPT='-O3 -mprefer-vector-width=512' ./build.sh

**Against it:** 512-bit downclocking on this part, and the annotated listing's
own warning that a shorter stream does not reliably pay (9c). But the failed
shortenings there were a handful of instructions out of 145; this removes most
of the arithmetic and all of the spill traffic at once, which is a different
order of change.

**Falsified if** the sweep loop does not roughly halve in instruction count --
check that first, since `-mprefer-vector-width` is a hint the vectorizer can
decline. Then check the achieved clock with `misc/clk.c` running alongside.

---

## 5. Re-run the chunking sweep, with the footprint story corrected

Section 8 recorded "chunked model/sweep, 2048" at +10.27 cyc/grp and blamed
`tmpbuf` evicting `FSM`+`pp` from L2. Section 0 above shows that cannot be
the mechanism. Whatever the real one is, the motivation for chunking is
unchanged and worth re-testing: `pbit` is `BLKSIZE*8*2` = **1 MB written and
1 MB read per 64 KB block**, i.e. 32 bytes of L2/L3 traffic per input byte,
for a value that is produced and consumed in the same order minutes apart.

Sweep the chunk size rather than testing one point: 256, 512, 1024, 2048,
4096, 8192 input bytes (2 KB … 64 KB of `pbit`). L1d is 32 KB and also holds
the model's 2 KB and the sweep's 16 live output lines, so if there is a knee
it is around 1-2 KB of input per chunk, well below the 2048 already tried.

**Falsified if** the curve is monotonically bad. Then the cost is the chunk
loop overhead and the sweep's lost run length, and chunking is dead for good.

---

## 6. Two-thread software pipeline for encode

*The largest structural win available, and the largest change.*

`model_pass` and the coding sweep are already separated by a buffer -- that is
what `pbit` is. They are 57% and 43% of encode. Running the model pass for
block *k+1* while the sweep codes block *k* is a two-stage pipeline whose
ceiling is `1/0.57` = **1.75x on encode**.

What it needs: a second `pbit` buffer (2 MB total), a handoff at the block
boundary, and one thread. The model state (`cty`) is carried across blocks, so
blocks cannot be reordered -- but the pipeline does not reorder them, it only
overlaps two adjacent stages, which is exactly the dependency the split
already respects.

What it costs: the codebase is a single-threaded coroutine design
(`Lib3/coro3b.inc`, `__builtin_setjmp` with stack copying). A thread inside
that is not free to add. And decode gets nothing -- it cannot be split at all,
which makes the encode/decode asymmetry *worse*, not better.

**Do this last**, and only if items 1-5 have been taken. It is the only item
here that changes the program's shape rather than its instruction stream.

---

## 7. `-fno-pie`

Already measured: **0.85 cyc/grp** on the current default build
(`avx2_profile.md` section 10). One flag, no risk, no source change, and it is
a packaging decision (no ASLR for the executable) rather than an experiment.
Listed so it is not forgotten; it is worth about 1%.

---

## Do not repeat these

All measured, all in `avx2_profile.md` sections 8 and 9c:

| | |
|---|---|
| branch past all-zero commit groups (`RC_SCATTER_SKIP`) | +12.07 cyc/grp |
| commit deferred one group | +5.52 |
| chunked model/sweep at 2048 | +10.27 (but see item 5) |
| `pp` fused into `FSM`, fat-struct shape | +3.42 (but see item 1) |
| prefetching `pbit` | noise |
| `-mprefer-vector-width=128` | much worse |
| `RC_SHIFT_SAT=1` (`lowl>>(32-sh)`) | slower -- clang will not fold the poison |
| `RC_FOLD_RPRE=1` (rpre applied in `rc_Process`) | -2.55%, and it *shortened* the loop |
| reading `pbit` through a signed `short*` | -2.1%, ditto |
| BMI2 `rorx` for the bit extract | codegen identical, no-op |
| clang 23.1.0 instead of 18.1.3 | -7% encode; see section 9d |

Also considered and rejected on paper, so nobody spends a day on them:

- **Packing four `pbit` pairs into one 64-bit store.** Saves 12 store uops per
  group, costs ~24 shift/or uops. The model pass is rename-bound with the
  store port at 62%, so trading uops for port pressure is the wrong direction.
- **Storing a state index in `pbit` instead of `p`** (1 byte per bit, halving
  the buffer). The sweep would then need `pp[state]` for 16 lanes at once,
  i.e. a gather. `vpgatherdd` costs more than the entire commit.
- **`vpmuludq` pairs instead of `vpmulld`.** 3-4 uops against 2. The sweep is
  not latency-bound, so trading uops for latency is also the wrong direction.

---

## How to measure anything here

From `avx2_profile.md` section 9b, learned the hard way: **best-of-N on this
box has a ~2% spread, which is the size of most of these effects.**

1. Round-robin the variants, N iterations each, and take **medians of
   best-of-N across runs** -- never a single run, never best-of-N alone.
2. **Put a control in the rotation**: a byte-identical copy of the baseline
   binary. It should read 0.0% +/- 0.5%. If it does not, the box is busy and
   the run is worthless. Both recent negative results were only trustworthy
   because the control read -0.03% and +0.53%.
3. **Count instructions before you believe a timing.** Every item above has a
   "falsified if" that is a static instruction count, and it is cheaper to
   check than a benchmark. Two of the last three experiments shortened the
   loop and lost anyway -- codegen changed the way it was supposed to, and the
   machine disagreed. Static counts tell you the change happened; only the
   clock tells you it helped.
4. `t.sh` after every change: roundtrip, and byte-identical against the `-C`
   scalar reference. Items 1-6 are all meant to be stream-neutral. If the
   stream moves, the experiment is over regardless of the timing.

    ARCH=skylake ./t.sh                 # correctness, both directions
    ./coder c ../enwik8 /dev/null ../FSM0.txt 6   # the encode timing loop
    ./coder d t.enc t.dec ../FSM0.txt 6           # the decode timing loop
    cc -O2 misc/clk.c -o clk && ./clk             # the clock the figures are against
