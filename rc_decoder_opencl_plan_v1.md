# `sh_v1xN` decoder — plan for an OpenCL / vector speed path

Companion to `rc_vectorized_design_v1.md`, which covers the encoder. That document's
§6.1 opens with "measure the decoder's model ceiling first" and notes the probe had never
been run. It has now been run, together with eight others, and the answer changes what the
plan should be. Everything below is built on measurements taken in this tree; §10 says
exactly how, and which numbers are load-bearing and which are not.

---

## 0. Summary

The encoder's device path works because `pbit[]` decouples the model from the coder: pass 1
produces one `{p;bit}` word per bit for a whole block, and the coding pass is then a pure
function of that array with `RCNUM` independent lanes. **The decoder has no such split and
cannot have one** — `bit` is the coder's *output* and the model's *input*, so there is no
phase to hand to a device.

That is not a difficulty to engineer around; it is a proof that the encoder's shape does not
transfer. The measured consequence:

| question | measured answer |
|---|---|
| What would offloading the whole coder buy? | **1.33x** (gcc) / **1.38x** (clang) — and it is not expressible |
| What would offloading the whole model buy? | 1.40x / 1.38x — likewise |
| What does the loop scaffolding cost? | 1.7 / 1.0 clk/bit out of 16.7 / 13.8 |
| Does speculative decoding shorten the chain? | **No** — measured 1.5–2.2x *worse* than the serial form (§3) |
| Is there device-scale parallelism available at all? | Only after a format change (§4) |
| What does that format change cost in compression? | **Nothing** — it *gains* 0.14–0.47% (§4.2) |
| What does it buy on the host today? | **+11–19% (gcc) / +29–43% (clang)**, size −0.30% (§4.3) |
| What would a 16-lane vector decoder cost, naively ported? | 16.7–17.1 clk/bit — i.e. **no faster than scalar** (§5.1) |
| What would it cost with the right layout? | **3.8–4.4 clk/bit → ≈3–4x** (§5.3) |

So the plan is not "port the decode loop to OpenCL". It is:

1. **Branchless counted refill** (§4.4) — a prerequisite, not an optimisation. With the
   current branchy `ShiftCode` any attempt to interleave streams *loses* 33%.
2. **Model striping** (§4) — `S` independent order-0 models, byte *i* using model *i%S*,
   with the host decode loop restructured to carry `S` bytes at once. This is the only thing
   in the whole analysis that creates parallelism, it is free in compression, and it costs
   the encoder's kernel nothing.
3. **Only then** the vector/OpenCL kernel (§5, §6), and only with the layout changes in §5.4 —
   a straight port of today's model loses to the scalar code it replaces.

Steps 1 and 2 are worth +29–43% (clang) on the host on their own and are the entry ticket for step 3.
Step 3 needs one further change that is *not* free (§5.4, the arithmetic counter) and whose
compression cost has to be measured before it is committed to.

---

## 1. Why the encoder's recipe does not transfer

### 1.1 The structural fact

`model0.inc` runs the model over a whole block first:

```cpp
for( i=0; i<blksize; i++ ) {
  c = get0();
  for( j=0; j<8; j++ ) {
    P_S(state); p=pp[state];
    bit = (c>>(7-j))&1;                 // <-- known, it is the input byte
    S.pbit[i*8+j] = (bit<<15)+p;
    P_update( state, bit );
  }
}
```

`bit` comes from the plaintext, so the model pass needs nothing from the coder. `proc_block`
then walks `pbit[]` with `RCNUM` independent coders and never touches the model. That is the
whole device path.

`model1.inc` cannot do this:

```cpp
for( i=0; i<blksize*8; i++ ) {
  j = i % RCNUM;
  P_S(state); p=pp[state];
  rc.rc_Process( j, p, bit );           // <-- bit is produced here
  P_update( state, bit );               // <-- and consumed here
  if_e0( i%8==7 ) { P_byte(stats,c); put0(c); }
}
```

To decode bit *i* you need `p`, which needs the model updated with bits `0..i-1`. The
`RCNUM` interleave breaks the *coder's* serial chain — lane *j*'s `range` feeds its own next
bit, sixteen bits later — but it does nothing for the model's, which stays fully serial.
`P_Init` is called once, outside the block loop, so the model also carries across blocks:
block *n+1* is not decodable until block *n* is finished. There is no block-level parallelism
in the format either.

### 1.2 The null probe

`rc_vectorized_design_v1.md` §6.1 asks for the decoder equivalent of the encoder's
`volatile q=p+bit` probe. Three variants of `model1.inc`'s inner loop, each keeping the loop,
the block structure and the output side identical:

* `PROBE=0` — the real step.
* `PROBE=1` — the model chain, with `bit = (p>>7)&1` in place of the coder. *The coder is free.*
* `PROBE=2` — the coder, with a constant `p` and no model. *The model is free.*

| probe | gcc MB/s | clang MB/s | gcc clk/bit | clang clk/bit |
|---|---|---|---|---|
| 0 — full decode | 19.99 | 24.20 | 16.70 | 13.79 |
| 1 — coder free | 26.62 | 33.27 | 12.54 | 10.03 |
| 2 — model free | 27.90 | 33.31 | 11.96 | 10.02 |

Pinned to one core, best of five iterations, two to three runs each; these are the most
stable numbers in this document (§10).

### 1.3 What that rules out

**A device that only does the coder is capped at 1.33–1.38x**, and that cap assumes the
coder becomes *free* — infinite speed, zero transfer, zero latency. The encoder's actual
device win is 3.95–5.53x, and it is achievable precisely because the coding pass is a whole
block's worth of work that can be handed over once. The decoder has no such unit: the
handover would be per bit.

Note also how *little* the two halves cost when separated (12.5 and 12.0 clk/bit) compared
to together (16.7). They overlap heavily — but they overlap as a *series*, `p` into the
coder and `bit` back into the model, so neither can be removed without the other stalling.
The 1.7 clk/bit of loop scaffolding (`PROBE=5`, §2) is the only part that is not chain.

**Conclusion: any decoder speed work has to shorten or duplicate the fused chain.** The rest
of this document is about which of those two is available.

---

## 2. Where the decoder's time goes

Six more probes, same harness, decomposing the chain. These were taken on the 100 MB file
unpinned at three iterations, so they carry the unpinned noise floor of §10 — read the large
effects, not the small ones. Baseline under those conditions: 19.75 / 23.52 MB/s.

| probe | what | gcc | clang | gcc clk/bit | clang clk/bit |
|---|---|---|---|---|---|
| 5 | neither model nor coder: loop + output only | 200.54 | 324.37 | 1.67 | 1.03 |
| 7 | dependent `pp[]` chain only, no counter load | 66.99 | 64.99 | 4.99 | 5.14 |
| 6 | model chain without the `pp[]` lookup | 30.18 | 39.86 | 11.06 | 8.38 |
| 1 | model chain, complete | 26.09 | 32.09 | 12.79 | 10.40 |
| 0 | full step | 19.75 | 23.52 | 16.90 | 14.19 |

Reading:

* **The loop is free.** 1.0–1.7 clk/bit of the 13.8–16.7 is `i%RCNUM`, the `i%8==7` test,
  `put0` and the loop itself. Nothing to win there; the doc's §6.2 "port the decoder to the
  macro form" is aimed at this and has at most ~10% in it.
* **`pp[]` is nearly free too.** Removing the lookup from the model chain (6 vs 1) buys 1.7–2.0
  clk/bit, not the ~5 clk a dependent load "should" cost — the state distribution is
  concentrated enough that it is an L1 hit almost always. This matters in §5: what is nearly
  free scalar is *not* free vector, where it is a second gather.
* **The counter load is the model chain.** Probe 7 shows a dependent chain through `pp[]`
  alone runs at ~5 clk/bit; probe 6 shows the counter load alone costs ~8–11. `cty[ctx]`, a
  512-byte array, is the expensive load — because the address depends on the previous bit and
  the array was written a few instructions earlier.

The intra-byte chain, then, is:

```
ctx  ->  load cty[ctx].state  ->  load pp[state]  ->  imul  ->  cmp  ->  bit  ->  ctx
```

with the counter write-back (`FSM[state].s[bit]`, then a store) hanging off it. `ctx` resets
to 1 every byte, and within a byte the eight cells visited are all distinct — so nothing read
in a byte was written in that byte. That is worth stating precisely, because it is what makes
§3 possible and what kills one of §6.1's suggestions (§9).

---

## 3. Speculative decoding

The obvious idea, and the one that is worth the most careful treatment because it looks like
it should work.

### 3.1 The shape that is actually available

Speculation needs a small, enumerable set of possible futures. The decoder has exactly one:
**the bit-tree context.** At the start of a byte `ctx==1`; after *d* bits it is one of 2^*d*
nodes. Evaluate all of them, then pick the path once the bits resolve.

The crucial enabling fact is on the coder side, and it is a gift from the `RCNUM` interleave:

> Bit *i* goes to lane *i%RCNUM*. With `RCNUM=16`, the eight bits of a byte go to eight
> **different** lanes, each of which last coded sixteen bits — two bytes — earlier. So at the
> moment a byte starts, every `(range, code)` pair it will use is already final.

**The coder never has to be speculated at all.** `rpre = (range>>15)*p` and `bit = code>=rpre`
can be evaluated for every candidate node from state that is already known; only `p` is
speculative. That is much better than the general speculative-decoding situation, and it is
why this is worth trying rather than dismissing.

The layout cooperates too. The depth-*d* subtree rooted at `ctx` occupies cells
`ctx`, `2ctx..2ctx+1`, `4ctx..4ctx+3`, `8ctx..8ctx+7` — **four contiguous runs**, not a
scatter. A depth-4 subtree is 15 cells and fits one AVX-512 dword vector, loaded with four
masked loads. So a round is:

```
4 masked loads  ->  p = cell>>16  ->  vpmulld against a per-depth range  ->  vpcmpud
     ->  a 16-bit mask holding the decoded bit of every node in the subtree
     ->  walk it:  k=1;  four times:  k = 2*k + ((mask>>k)&1)
```

The walk is the only serial part, and it is `shrx`/`and`/`lea` — no loads. Two rounds per
byte at *d*=4.

### 3.2 What it should cost

Per byte, against a chain of eight steps at ~14 clk each: two rounds of (four L1 masked
loads, one permute, one `vpmulld`, one compare, one mask extract) ≈ 2×22 clk, plus 8 walk
steps at ~3 clk = 24 clk. About 68 clk/byte = 8.5 clk/bit against 14.19 — a projected 1.7x,
with no format change and no compression change. Worth measuring.

### 3.3 Measured: it loses

`misc/bench/rc_specdec_shape.cpp` compares the two chains directly, with the coder removed
from *both* sides (the bit is a compare of `p` against a per-depth value that does not depend
on the model — which, per §3.1, is exactly the decoder's situation):

| | serial | speculative |
|---|---|---|
| gcc, morning session | 4.9 | 10.9 |
| gcc, a slower session hours later | 5.5–5.6 | 9.5–9.6 |
| clang | 5.3 | 8.1 |

Speculation is **1.5–2.2x worse**, in every session and under both compilers.

The reason is the first column, not the second. That benchmark folds `p` into the
counter cell, so the serial chain is *one* L1 load plus an `imul` and a compare — ~5 clk/bit.
A chain that short has nothing left for speculation to hide: the fixed cost of a round
(the four masked loads, the permute, the mask extraction) already exceeds four steps of it,
and the mask walk adds its own serial cost on top of a chain that was ~5.

This is the general shape of the result, and it is worth stating as a rule:

> **Speculation pays in proportion to the chain it removes.** The decoder's per-bit chain is
> two L1 loads and a multiply. There is not enough of it to be worth 3.75x the arithmetic and
> a serial mask walk.

### 3.4 What survives

* **Speculate across bytes, not within them.** The version that works is §4 — decode *S*
  bytes at once with *S* independent models. That is not speculation (nothing is discarded)
  and it is measured to work.
* **Depth-2 speculation** would have a smaller fixed cost and might break even. It is not
  worth the complexity given that §4 does better for less.
* **Cross-block speculation** — guess the model state at a block boundary, decode block *n+1*
  in parallel, verify when block *n* finishes — is not viable as a guess (the FSM counters
  oscillate rather than settle, so exact equality across a 64 KB boundary is unlikely) but
  becomes trivial if the state is *checkpointed*: 255 counters × 2 B = 510 B written every
  *K* blocks. At *K*=16 that is 0.05% of the input and gives `nblocks/K` independently
  decodable units. Listed as a fallback in §7 if §4's format change is rejected, since it is
  the only other way to get block-level independence.

---

## 4. What does create parallelism: model striping

### 4.1 The change

Byte *i* uses model *i%S*. `S` independent copies of the order-0 counter array; nothing else
changes. Encoder and decoder do it identically, so it round-trips by construction.

In `model0.inc`, one line at the top of the byte loop:

```cpp
for( i=0; i<blksize; i++ ) {
  cty = &stats[i&(RC_STRIPE-1)][0] - 1;      // <-- added
  c = get0();
  ...
```

and the same in `model1.inc` at each byte boundary. `stats` becomes `Counter[S][0x100]`;
`P_Init` initialises all of it. `ctx` needs no saving or restoring — it resets to 1 every
byte anyway, which is exactly why byte-granular striping is the cheap granularity.

**The encoder's OpenCL kernel is untouched.** Striping lives entirely in the model pass that
produces `pbit[]`; `rc_kernel.c` reads `pbit` and knows nothing about counters. That is worth
saying out loud, because it means this format change costs the existing device path nothing —
not a recompile of the kernel, not a re-measurement.

Implemented behind `RC_STRIPE` (a power of two — the selection is a mask). Round-trip
verified at *S* = 1, 2, 4, 8, 16, 32, 64 on `book1` and a 100 MB file, and on the full
`t.sh` corpus — host *and* device encodes — for the default build, `S=2`,
`S=16/ILV=8`, and `S=4/ILV=2` with `RC_FOLDP=1`. The corpus run caught a real bug in the
first version of the prototype: the interleaved loop indexed the model by position within
the group instead of by byte number, which coincides only when `RC_DECILV==RC_STRIPE`, so
`ILV<STRIPE` mis-decoded. The fix (`stats[(byte+t)%S]`) was A/B-measured as free. That is
what the corpus is for; the plan's own §10 rule — verify byte-for-byte, never trust "it
looks right" — applies to the prototype too.

### 4.2 Compression cost: none — it is a gain

Exact output sizes:

| S | book1 (768,771 B) | Δ | 100,007,936 B of text | Δ |
|---|---|---|---|---|
| 1 | 456,441 | — | 59,373,986 | — |
| 2 | 455,125 | −0.29% | 59,198,493 | −0.30% |
| **4** | **454,374** | **−0.45%** | **59,092,170** | **−0.47%** |
| 8 | 455,481 | −0.21% | 59,218,390 | −0.26% |
| 16 | 456,318 | −0.03% | 59,290,694 | −0.14% |
| 32 | 457,258 | +0.18% | 59,347,872 | −0.04% |
| 64 | 458,672 | +0.49% | 59,397,483 | +0.04% |

Striping *improves* compression up to *S*=4 and is still ahead of a single model at *S*=16.
An FSM counter has a fixed adaptation rate, so *S* of them over interleaved byte positions
behave as a mild ensemble: each sees 1/*S* of the data, which at 100 MB is still 6 MB per
stripe — orders of magnitude past convergence — while the ensemble averages out some of the
counters' own oscillation. Only past *S*=32 does dilution start to win.

Two caveats, both important:

* This is an **order-0** model. A higher-order model would divide its context statistics *S*
  ways and would pay properly. If the project ever moves past order-0, this number must be
  re-measured, not assumed.
* `S`=16 on `book1` (768 KB) is already at −0.03%, i.e. break-even. Small files are where
  dilution shows first; a format that stripes should not use a large *S* on small inputs.

### 4.3 Host speedup, measured

With striping plus the branchless refill of §4.4, and the decode loop restructured to carry
*S* bytes at once (`RC_DECILV`), on 30,015,488 B pinned to one core, best of four:

| config | csize | gcc | clang |
|---|---|---|---|
| pristine | 17,819,829 | 19.83 | 23.45 |
| restructured byte loop + branchless refill (S=1) | 17,819,829 | 22.46 (+13%) | 26.24 (+12%) |
| **S = ILV = 2** | 17,767,111 (−0.30%) | **23.66 (+19%)** | **33.57 (+43%)** |
| S = ILV = 4 | 17,735,253 (−0.47%) | 23.47 (+18%) | 31.89 (+36%) |

Round-trip verified at every entry. Note the second row is not the refill alone: it also
replaces the bit loop's `i%8` bookkeeping with a per-byte loop, and the two are not
separable in this configuration (the refill alone is *slower*, §4.4).

A second session hours later — after the VM had slowed by ~12% on identical binaries (§10) —
reproduced the shape with smaller margins: pristine 20.97 → 27.05 at S=ILV=2 (+29%) under
clang, 17.12 → 19.06 (+11%) under gcc, same csize, round-trip verified. The honest headline
is therefore the range **+11–19% (gcc) / +29–43% (clang)**: the ratio itself moves with the
machine's state, and a reader on quiet hardware should re-measure rather than quote either
endpoint.

### 4.4 The branchless refill is a prerequisite, not an optimisation

`ShiftCode` currently refills byte at a time:

```cpp
for( ; n!=0; n-- ) code = (code<<8) + get();
```

`n` is 0, 1 or 2 and data-dependent. With one stream the branch predictor handles it. With
*S* streams interleaved, *S* unpredictable branch streams alias in the predictor and the
mispredictions dominate:

| | gcc ILV=1 | ILV=2 | ILV=4 | clang ILV=1 | ILV=2 | ILV=4 |
|---|---|---|---|---|---|---|
| branchy `ShiftCode` | 22.02 | **14.70** | 13.63 | 29.73 | 31.15 | 26.51 |
| branchless refill | 19.62 | **24.20** | 23.66 | 27.12 | 32.58 | 31.79 |

Interleaving with the branchy refill *loses 33%* under gcc. With the branchless one it gains.
The replacement is four lines:

```cpp
// n is 0..2 for the binary coder, so the two bytes the shift can need are
// always the two at ptr: read both, shift in the ones that count, advance by n.
// Reading a byte or two past the substream is safe -- rc_Read pads each
// substream with FF_PADSIZE bytes of 0xFF.
uint v = (uint(ptr[0])<<8) + ptr[1];
code = (code<<(n*8)) + (v>>((2-n)*8));
ptr += n;
```

Note it is *slower* on its own at ILV=1 under both compilers (19.62 vs 22.02, 27.12 vs 29.73)
— which is exactly what `RC.txt`'s `054` measurement found when `ShiftCodeN` was tried and
rejected. That measurement was right and is still right; it was answering a different
question. `rc_vectorized_design_v1.md` §6.2 asks for it to be re-measured "in the current
framework" — the answer is that it only pays once something else needs it to, and the table
above is that need: at ILV≥2 the branchless form wins by 30–65% (gcc).

### 4.5 Why scalar interleaving saturates at *S*=2–4

`ILV=8` falls back to 19.58 / 24.22. Two contributions, separated by experiment:

* **Coder lane collision.** Byte *b* uses lanes `(8b)%16 .. (8b+7)%16`, so bytes *b* and *b*+2
  share lanes. At ILV=4 and above the interleaved bytes serialise through the coder again.
  Raising `RCNUM` to 64 at ILV=8 recovers part of it (23.60 → 27.37, +16%) — the effect is
  real. The design rule is `RCNUM >= 8*S` if lane disjointness is wanted.
* **The rest is register and L1 pressure**, and it is the larger part: at ILV=4, `RCNUM`=32
  makes no difference at all (32.24 vs 31.87). `rc[rcidx]` is an indexed access into an array
  of coder objects — with *S* streams, *S* of those per step, all through memory.

That second point is the whole argument for the vector form: **with the streams in SIMD lanes,
`range` and `code` live in registers and the indexing disappears.** The scalar interleave is
not the destination; it is the proof that the parallelism exists.

---

## 5. What a 16-lane vector decoder actually costs

`misc/bench/rc_vecdec_shape.cpp` runs the exact dependency chain and instruction mix of a
16-lane striped decoder step, in seven layouts. Arbitrary data, so the numbers are shape, not
correctness. gcc, AVX-512F/BW/DQ/VL, pinned, ±3% within a session:

| layout | gathers on chain | total gather/scatter per bit | clk/bit |
|---|---|---|---|
| chain only, separate `pp[]`, model write-back removed | 2 | 3 G | 9.0–9.4 |
| realistic, cell = `{state}` | 2 | 4 G + 1 S | 16.7–17.1 |
| realistic, cell = `{state,p}` | 1 | 3 G + 1 S | 11.6–11.7 |
| ... model scatter removed | 1 | 3 G | 9.2–9.4 |
| ... scatters batched at the byte boundary | 1 | 3 G + 8 S/byte | 11.6–11.8 |
| **target: contiguous refill + arithmetic counter** | **1** | **1 G + 1 S** | **4.4** |
| ... target, but the refill left as a gather | 1 | 2 G + 1 S | 7.3–7.5 |

Reference: the real scalar decoder on the same box is 13.79 (clang) / 16.70 (gcc) clk/bit.
An earlier build of the benchmark (before the last row existed) measured every row 15–25%
lower — 13.5–14.0 for the naive port, 3.82 for the target — with the same ordering (except
the batched-scatter row, which trades places with the row above it within noise — §5.2) and
similar ratios. All seven variants share one compiled loop, so adding one changes the code
of the rest; the rows within one build are directly comparable, the absolute clk/bit
carries that build-to-build spread (§10).

### 5.1 The naive port loses

A direct translation of today's `model1.inc` inner loop to 16 work-items is the second row:
**16.7–17.1 clk/bit, the same as the scalar code it replaces.** Every part of the step that
is an indexed memory access becomes a gather or a scatter, and there are four of them:

1. `code` refill — each lane reads from its own substream at its own offset → **gather**.
2. `cty[ctx].state` — `ctx` is per-lane and data-dependent → **gather**.
3. `pp[state]` — dependent on the previous gather → **gather, on the chain**.
4. `FSM[state].s[bit]` then the store back → **gather + scatter**.

This is the same wall the encoder hit (`rc_vectorized_design_v1.md` §6.3), except the encoder
only had the *output* scatter to deal with — its model side was `pbit[]`, a contiguous stream.
The decoder's model side is a random-access table per bit, which is the worst possible input
to AVX-512.

### 5.2 Which of them actually costs

Removing the model scatter takes 11.6 → 9.3. Batching the eight scatters of a byte at the
byte boundary is a wash: it measured ~1 clk/bit *better* in one build of the benchmark
(10.8 → 9.9) and dead even in another (11.6 vs 11.7) — inside the build-to-build spread, so
rescheduling the scatter buys nothing reliable. No single access dominates — it is the
count. The way out is to reduce the count, not to reschedule.

### 5.3 The target shape

The target row — one gather and one scatter per bit — reaches **4.4 clk/bit** in the current
build of the benchmark, 3.82 in the earlier one: **≈3–4x** the scalar decoder. That is the
same order as the encoder's measured device win (3.95–5.53x), and it is the number the plan
is aimed at.

### 5.4 What has to change to get there

Three changes, and the first two are the ones that need judgement:

* **Fold `p` into the counter cell** (`{word state; word p;}`), so reading a counter gives the
  probability without the second lookup. Takes one gather off the chain: 16.7–17.1 → 11.6
  clk/bit vector (1.45x; the earlier build measured the same change as 1.25x). *On the
  scalar host it is a small loss* (~4%: 29.5–30.2 → 28.5–28.7 clang, unpinned, at the edge
  of this box's noise) — the `pp[]` load is nearly free scalar (§2) and folding doubles the counter array
  and adds a lookup to the write-back. **This is a vector-only optimisation**, and a good
  illustration of why the device port has to be designed against the vector cost model.
  No compression impact, no format impact.
* **Replace the FSM/`pp` table pair with an arithmetic counter** stored in the cell, e.g.
  `p += (bit ? -p : (SCALE-p)) >> rate`. This is what removes gathers 3 and 4 outright — the
  update becomes pure arithmetic on data already in registers. It is the single largest term
  in getting from 11.6 to 4.4. **It is also a compression change**, and the only item in this
  plan whose cost is not yet measured. The FSM machines (`nzcc.txt`, `FSM0.txt`) are tuned;
  a shift-counter will not match them for free. Measure before committing (§7, item 5).
* **Make the refill contiguous.** In the current container each lane's substream is a separate
  run of bytes, so a vector refill is a gather. `rc_vectorized_design_v1.md` §6.3 already
  proposes the interleaved-write layout for the encoder — each 16-bit group's output packed
  contiguously, the decoder computing every `n_j` before it needs any byte. That is the same
  change, and the decoder is where it pays: one contiguous load per group instead of sixteen
  scattered ones. Measured directly by the benchmark's last two rows — the target with the
  refill left as a gather runs 7.3–7.5 against 4.4, so the contiguous refill is worth
  **~3 clk/bit** of the target's budget.

---

## 6. The OpenCL shape

### 6.1 The kernel owns the whole block

There is no `pbit` and therefore no phase boundary. The kernel takes the substream rows and
the model state and returns the decoded block; nothing crosses the host/device line per bit.
This is a bigger kernel than `rc_kernel.c` — it contains the model — and it is the reason the
generation chain (`rc_macro.pl` → `defines.pl` → `txt2inc.pl`) matters more here, not less.

### 6.2 Geometry

`S` striped streams map to `S` work-items, one work-group, `intel_reqd_sub_group_size(16)`,
`S`=16 to match the AVX-512 dword width — the same geometry the encoder settled on, arrived at
from the opposite direction. `RCNUM` should be `8*S` if lane disjointness is wanted (§4.5),
but in the vector form the coder state is in registers, so the `RCNUM` interleave is no longer
buying anything the striping does not already buy: **`RCNUM=1` per stream is the natural
endpoint**, with `S` substreams instead of `RCNUM` and one length header each. That collapses
two mechanisms into one and should be evaluated as part of item 6 in §7 rather than assumed.

### 6.3 What Intel's vectoriser will do to it

Established during the encoder work and unchanged: **every per-work-item indexed memory access
becomes a gather or a scatter, with no contiguity analysis** — `outlen[id]` with an all-ones
mask still compiled to `vpscatterqd`. So §5.4's three changes are not "nice to have" in the
OpenCL version; they are the only way to control what the vectoriser emits. Anything left as
`array[per_work_item_index]` is a gather, full stop.

Corollary: write the kernel so the *only* indexed accesses are the counter read and the
counter write. Everything else — the refill, the coder state, the context — must be in
work-item-private scalars that the vectoriser can keep in vector registers.

### 6.4 What does not carry over from the encoder path

* **`RC_CL_NBLK` pipelining.** The encoder can have eight blocks in flight because the model
  pass for block *n+1* does not need block *n*'s coding. The decoder cannot start block *n+1*
  until block *n*'s model state exists. Multi-block scheduling needs either block-granular
  striping or the checkpoint idea of §3.4 — otherwise the slot ring is dead weight.
* **The carryless/twin split.** Decode-side there is no carry to lose; `Rangecoder_CY<1>` is
  used because `RC_CARRYLESS` only changes `ShiftLow` and `rc_Quit`, both encode-only. Nothing
  to port.
* **The `-k` binary cache, `ocl2elf.py`, the instruction census.** All of it carries over
  directly and should be used from day one — the decoder kernel is harder to get right and the
  disassembly is how the gather count gets verified against §5's table.

### 6.5 Not a GPU

Eight dependent steps per byte, one L1-latency load each. A GPU has neither the single-thread
latency nor a way to fill thousands of lanes: `S` would have to be in the thousands, which
§4.2 says costs real compression (already +0.49% at *S*=64 on `book1`) and would need
thousands of substream headers per block. The target is the same Intel CPU device the encoder
uses. Say so in the code, so nobody re-litigates it.

---

## 7. The plan, in order

| # | item | cost | risk | expected |
|---|---|---|---|---|
| 1 | Branchless counted refill in `ShiftCode`, behind a knob | hours | none — bit-exact | slightly negative alone; the +33% swing appears once interleaved (§4.4) |
| 2 | `RC_STRIPE`: *S* independent models, byte *i* → model *i%S* | a day | format fork | free / −0.3% size |
| 3 | `RC_DECILV`: decode *S* bytes at once on the host | a day | none beyond 2 | +11–19% gcc / +29–43% clang combined |
| 4 | Fold `p` into the counter cell, behind a knob, **off by default on the host** | hours | none | −4% scalar, 1.25x vector — for item 6 |
| 5 | **Measure** an arithmetic counter against the FSM machines | days | compression | decides whether §5.3's target is reachable |
| 6 | The vector decoder as AVX-512 intrinsics first, OpenCL second | weeks | high | ≈3–4x if 5 lands, ~1.0x if it does not |
| 7 | Interleaved container layout (shared with encoder §6.3) | weeks | format fork | ~3 clk/bit of item 6's budget (measured, §5.4) |

Items 1–3 are independent of everything else and are worth doing on their own merits. Item 5
is the gate: **if the arithmetic counter costs more than a few tenths of a percent, item 6 is
not worth starting**, because §5.1 says the FSM-table version of the vector decoder is no
faster than the scalar one it replaces.

Note the ordering of item 6 — intrinsics before OpenCL, the reverse of what was done for the
encoder. The reason is §5: the encoder's kernel had one hard problem (the output scatter) and
a known-good scalar reference to check against, so writing it in OpenCL first was fine. The
decoder's kernel has four gather/scatter sites and its performance depends on which of them
survive; that needs a form where the emitted instructions are under direct control. Port to
OpenCL once the shape is proven and the census matches.

---

## 8. Format and compatibility

Striping (item 2) is a format fork: a striped stream does not decode with an unstriped
decoder or vice versa, and *S* has to be agreed. Options, in increasing order of effort:

* Bump whatever version marker the container has and treat *S* as a build-time constant, as
  `RCNUM` and `BLKSIZE` already are. Simplest, consistent with how the rest of the geometry
  is handled, and the honest choice for a research coder.
* Carry *S* in the stream. One byte per file. Worth it only if files are expected to travel
  between builds.

The interleaved container (item 7) is a second fork and should be landed together with the
encoder-side change, not separately.

Items 1, 4 and 6 are format-neutral. Item 5 is a model change and therefore also a fork; if
items 2 and 5 both land they should land as one version bump.

---

## 9. Negative results, and one retired item

Recorded so they are not re-tried:

| idea | measured | verdict |
|---|---|---|
| Offload the coder to a device, encoder-style | ceiling 1.33–1.38x, and not expressible (§1) | dead |
| Depth-4 speculative decoding over the bit tree | ~10 vs ~5 clk/bit — 1.5–2.2x worse (§3.3) | dead as a chain-shortener |
| Load `FSM[state]` as one dword and select `s[bit]` (§6.1 bullet 1 of the design doc) | model chain 12.79 → 11.38 clk/bit (gcc; clang's *regresses*, 10.40 → 11.62), and the **full step** 16.90 → 22.40 (gcc), 14.19 → 14.61 (clang) | dead — at best it helps the gcc probe, and it hurts the decoder |
| **Deferred counter update** (§6.1 bullet 3 of the design doc) | see below | retired |
| Fold `p` into the cell, on the host | 29.5–30.2 → 28.5–28.7 MB/s (clang, unpinned — at the edge of noise) | vector-only (§5.4) |
| Batching the vector model scatters at the byte boundary | ~1 clk/bit better in one build, dead even in another (§5.2) | no reliable effect; obsoleted by the arithmetic counter |

The deferred counter update deserves its own note, because the design doc rates it highly and
the measurement is decisive. Within a byte the bit tree visits eight *distinct* cells and never
reads one twice, so **deferring all eight write-backs to the byte boundary is bit-identical to
updating in place** — no statistics lag at all, and the free version of the idea. Implemented
and verified byte-for-byte against the source file. Speed: 19.75 → 20.10 (gcc), 23.52 → 19.30
(clang). Nothing. The write-back is simply not on the intra-byte chain, which §2 explains: the
cell being written is never the cell about to be read. The *cross-byte* variant — byte *k*+1
reading state as of byte *k*−1 — would change statistics and is a different, more expensive
proposal; given that the free version buys zero, there is no reason to pay for the other one.

One more, from the same family: adding a second and third independent model chain to the real
decode step costs 19.75 → 17.74 → 16.85 (gcc) and 23.52 → 19.84 → 18.98 (clang), i.e. about
1.5–1.9 clk/bit per extra chain against a 16.9 clk/bit step (the first chain costs more
than the second). That is the ILP evidence that §4 is
built on — extra streams are cheap — but note it is *not* a speedup on its own, and the naive
projection from it (~1.6x at *S*=2) overshot the measured +19–43% by a wide margin. The
scalar interleave collects a fraction of the available ILP; §4.5 says why, and §5 is where the
rest of it is.

---

## 10. Measurement notes

**Box.** 4-vCPU Intel Xeon @ 2.80 GHz VM, AVX-512F/BW/CD/DQ/VL (no VBMI), 15 GB. gcc 13.3.0 and
clang 18.1.3, `./build.sh` with `OPENCL=0`. Intel OpenCL CPU runtime present. This is *not* the
5.1 GHz host of `rc_vectorized_design_v1.md`; absolute MB/s here is roughly a third of that
document's, so **compare ratios, never absolutes, across the two**.

**Corpus.** `big30` = 30,015,488 B (458 × BLKSIZE) and `big` = 100,007,936 B (1526 × BLKSIZE),
both `book1` repeated. Sizes are exact multiples of `BLKSIZE` so every probe does the same
number of blocks. `clk/bit = 2.8e9 / (MB/s × 2^20 × 8)`.

**Noise.** This box is a shared VM and its noise floor is the main hazard here. Three
separate effects, all observed on *identical binaries* and all material:

* *Run-to-run, unpinned*: swings up to ±25%. The same `RC_FOLDP=0` binary measured 19.72 and
  30.15 MB/s in two unpinned runs an hour apart. This one bit me: an early reading of §5.4's
  fold-`p` change came out as +43% and was that swing, not the change. An unpinned single
  reading proves nothing here.
* *Run-to-run, pinned* (`taskset -c 2`, five or more iterations): ±2–3%. Every number in
  §1.2, §4.3 and §5 is pinned and best-of-4-or-more; the tables in §2 and §4.4 are unpinned
  at three iterations — read directions and large effects only.
* *Machine drift across hours*: the whole box slowed ~12% between the morning and afternoon
  sessions — pristine clang went 23.45 → 20.97 on the same binary, pinned, and every
  benchmark row in §5 shifted with it. Worse, ratios move too, not just absolutes: striping's
  win read +43% in the morning session and +29% in the afternoon one (§4.3). Hence the rules
  used throughout: **a baseline is only valid for the session it was measured in** — re-run
  the pristine build alongside every comparison — and a ratio that two sessions disagree on
  is quoted as a range, not a number.

**Harness.** The probes are `#if RC_PROBE==n` variants of `model1.inc`'s inner loop, keeping
the loop, the block structure and the output side byte-for-byte identical and swapping only
the step; they are shipped as `misc/bench/rc_decoder_probes.patch`. Blocks are counted rather
than terminated on the short-block flag (`RC_PROBE_BLOCKS`), since the probes that remove the
coder cannot decode the block header. `PROBE=12` (the deferred write-back) is the one probe
that produces correct output, and it is checked with `cmp` against the source file.

The striping, branchless-refill and interleave work is real code, not a probe — shipped as
`misc/bench/rc_decoder_proto.patch` — and `RC_STRIPE`, `RC_DECREFILL`, `RC_DECILV` and
`RC_FOLDP` round-trip byte-for-byte at every value measured; the configurations named in
§4.1 also pass the full `t.sh` corpus, device encodes included.

The two shape benchmarks are in `misc/bench/` and are the source of §3.3 and §5 — arbitrary
data through the real dependency chain and instruction mix. The plan's numbers are gcc's,
pinned; clang compiles both benchmarks and preserves every verdict (speculation still loses,
the target row still wins), but its absolute rows differ by 10–20%, so do not mix compilers
within a table.
