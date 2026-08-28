# `sh_v1xN` decoder — the batched-coder split, and ISPC as the vector backend

Third document in the series, after `rc_vectorized_design_v1.md` (encoder) and
`rc_decoder_opencl_plan_v1.md` (decoder). The premise this one starts from:

> **The rangecoder is universal.** It codes for whatever model calls it, so nothing in the
> coder may depend on the model's structure. The striped-model results of the decoder plan
> (§4 there) live on the model side of that line — a model is free to stripe itself, and the
> coder never knows — but a *coder* speed path may not require it. What is left to optimize
> is the coder's own per-bit work: the code/range update and the renormalization.

Those can be batched and vectorized without touching the model, because of a property the
interleave already guarantees. This document is the design for that split, the measurements
of an exact implementation of it, and an evaluation of Intel ISPC as the way to build the
vector half — including what OpenCL cannot do here and why. As with the previous documents,
everything is measured, this time with a harness whose every variant is verified
bit-identical against the reference (§6.1); §10 has the conditions.

The verdict, up front, because it is not the one hoped for: **the split wins up to 2.2x in a
hostile regime and loses ~0.8x in the real decoder's regime.** The batch as built costs a
fixed ~8–10 clk/bit; the fused coder half it replaces costs 5 (predictable data) to 28
(hostile data); the real decoder sits near the predictable end. §7 locates the crossover and
§8 lists what would have to shrink to move it. The ISPC findings are independent of that
verdict and mostly positive — a per-group C-ABI call into an ISPC kernel is nearly free
(§6.3), which is what makes the whole approach buildable at all, and the encoder-side ISPC
port (§9) is worth doing regardless.

---

## 1. What may be vectorized in a universal decoder

The decoder's step, from `rc.inc`, per bit:

```
renorm:  n = (range<2^24)+(range<2^16);  code = (code<<8n)+bytes;  range <<= 8n
         rpre = (range>>15)*p
         bit  = (code >= rpre)
         range = bit ? range-rpre : rpre;   code -= bit ? rpre : 0
```

`p` comes from the model, `bit` goes back to it, and the model is serial — that chain was
measured and mapped in the decoder plan and none of it changes here. What the interleave
adds: bit *i* goes to lane *i%RCNUM*, so a lane is touched once per RCNUM bits, and at the
moment bit *i* is decided, lane *i%RCNUM*'s state has been final for RCNUM−1 bits. All the
per-lane work — the update, the renorm, the refill — is off the model's chain *in principle*;
in the fused decoder it still executes between model steps, costing chain slots, branch
predictor state, and the `imul` that sits directly on the chain (`p → rpre → bit`).

Two exact reformulations move all of it into a once-per-group batch:

* **The threshold form of the decision.** For integers, `code >= (range>>15)*p` is
  equivalent to `p <= floor(code/(range>>15))`. So a per-lane threshold
  `T = floor(code/ru)`, `ru = range>>15`, computed *when the lane's state settles*, turns
  the per-bit decision into one compare `p <= T[lane]` — no multiply on the chain, nothing
  data-dependent but `p` itself. The division this costs is off-chain, vectorized, 16 lanes
  at once. (`rc_vectorized_design_v1.md` §4 lists `RC_DECDIV` — divide instead of compare —
  as a scalar negative result at `dec 61.77 -> 56.60`; that put the divide *on* the chain,
  per bit. This puts it off the chain, per group. Same identity, opposite placement.)
* **The batch.** The scalar loop records `{p;bit}` per bit — the decoder's own little
  `pbit[]`, one group long. Every RCNUM bits, a 16-wide vector pass consumes it: rebuild
  `rpre = ru*p`, update range/code, counted renorm, refill, then `ru` and `T` for each
  lane's next use. The exactness of the whole arrangement is not argued but asserted: every
  variant in §6 must end bit-identical to the fused reference, and does.

The model interface survives untouched. `rc_Process(rcidx, p, bit)` is called in `i%RCNUM`
order — that is the format — so `RangecoderN` can hide the split entirely: buffer the
`{p;bit}` pairs, run the batch every RCNUM calls. A universal coder, same API, different
internals.

## 2. The granularity problem, or why not OpenCL

The batch has a hard deadline: lane *j*'s new `T` is needed RCNUM bits after its last use.
At RCNUM=16 that is *the very next group*. Whatever executes the batch must respond in tens
of clocks.

OpenCL's dispatch is microseconds. The encoder's own measurements
(`rc_config.inc`): 281–932 µs per 64 KB block for a whole kernel, and a bare
`clEnqueueNDRangeKernel`+flush round trip is ~5–50 µs against a 16-bit group budget of
~100–300 clk (~40–100 ns). Three orders of magnitude. Per-block dispatch works for the
encoder because `pbit[]` makes a block self-contained and the traffic one-way; the decoder's
batch is a round trip per group, and no amount of pipelining fixes a 100x-per-item overhead.
OpenCL stays what it is here: the encoder's whole-block backend.

ISPC is the right shape for this. It is a compiler, not a runtime: SPMD C compiled
ahead-of-time to an object file with a plain C ABI. Calling an ISPC kernel is calling a
function — measured below at **under 1 clk/bit of overhead at RCNUM=16** against inlined
intrinsics (§6.3). No ICD, no JIT, no context, no `-k` cache, no enqueue. One 26 MB static
binary (`github.com/ispc/ispc/releases`, Linux/Windows/macOS; Ubuntu also packages 1.22),
`--target=avx512skx-x16` maps one gang to one zmm exactly like one OpenCL sub-group, and
`gcc`/`cl` link the object directly.

## 3. The latency problem, or why RCNUM=64

The first measurement (§6.2, NL=16 rows) showed the batch costing ~14 clk/bit against ~4 of
vector arithmetic. The reason is structural: at RCNUM=16 the scalar phase and the batch
strictly alternate — group *k*'s batch produces the `T` that group *k+1*'s first decision
consumes — so the batch's full *latency* (a gather, two `vdivpd`, the convert chains:
~150–200 clk) sits on the serial path, however few µops it retires. This is, precisely, the
mechanism behind the design doc's oldest negative result: "batch renorm, tried 4+ times,
always worse". Batching per se was never the problem; batching *with the result due
immediately* was.

The format already sells the fix. At RCNUM=64, group *k* uses lanes `16k..16k+15 (mod 64)`,
so a batch's results are not consumed until **four groups later** — three full scalar phases
(~200 clk) for the out-of-order window to hide the batch latency in, with the batch still
16 wide, one quarter of the lane file per group. No new mechanism, no format change: RCNUM
is a knob the format always had. Measured cost of RCNUM=64 against 16 on this box:
**+0.29%** on 30 MB of text, **+0.17%** on incompressible data, decode speed unchanged
(19.6–20.1 MB/s both, same-session pinned A/B — an apparent +23% on the first measurement
did not reproduce and was VM drift; the design doc's RCNUM table saturates by 32 on the
5.1 GHz box, consistent).

Measured effect of the slack (§6.2): the split gains 2.8–3.5 clk/bit going NL=16 → NL=64.
Real, but bounded — the batch is ~50 vector µops competing for the same ports as the scalar
phase, so hiding latency does not make it free.

## 4. The communication problem

Scalar and vector code exchanging data through memory every 16 bits hits store-to-load
forwarding hazards from both directions: 16 scalar stores of `{p;bit}` immediately re-read
as one vector, a vector store of `T` immediately re-read by scalar code. Both were measured
and both have fixes:

* `{p;bit}` crosses in four u64 registers, packed by shift/or in the scalar loop (off the
  model chain) and rebuilt with one `set_epi64x` + widen on the vector side. For the ISPC
  variant the four u64s pass **by value** as arguments — no memory at all in that direction.
* `T` returns through memory, but at NL=64 the values read were stored four groups ago, so
  the hazard is gone by construction. (At NL=16 a gpr return path — clamp to 0x8000, which
  preserves every `p<=T` outcome since p≤32767, `vpmovdw`, four extracts — measured ~2
  clk/bit better than memory; at NL=64 it no longer matters. A C-ABI call can't return
  registers anyway, so ISPC is memory-bound in this direction on principle.)
* The refill gather moves off the batch's own chain with a one-dword-per-lane **lookahead**:
  the batch consumes bytes from a `look[16]` register loaded at the *end of the previous*
  batch, and issues the gather for the next one after advancing `pos` — the gather's ~25 clk
  then overlaps the following scalar phase. (A dword covers the worst case, n≤2.)
  AVX-512 has no 16-bit gather — the dword gather plus one `vpshufb` is the whole trick.

## 5. What the ISPC batch compiles to

The shipped kernel (`misc/bench/rc_splitdec.ispc`, `rc_batch2`) compiles at
`--target=avx512skx-x16 -O2` to **80 instructions, one gather, two `vdivpd`** — the same
class of code as the hand-written intrinsics, and the measured gap agrees (§6.3). ISPC's
per-work-item indexed accesses become gathers exactly like Intel's OpenCL CPU vectoriser's
(it even warns at compile time, which the OpenCL path never did) and the same mitigations
apply. The exact `floor(code/ru)` uses double division — both operands exactly
representable, quotient < 2^23, and since the true quotient is at least 2^-17 away from the
next integer while the rounding error is < 2^-30, one conditional `+1` after a
`code - T*ru >= ru` test makes it exact; that argument is in the source and the fingerprint
check enforces it.

The one ISPC *negative* result: putting the **whole loop** in ISPC — model as `uniform`
(scalar) code, lanes `varying`, one call per run, the "sync per block" endgame — is the
worst split variant measured, 7–8 clk/bit behind calling a per-group ISPC batch from C++.
ISPC's uniform codegen for a serial two-load chain simply loses to g++'s scalar output. The
right division of labor is the boring one: **the model stays in C++, ISPC gets the batch.**

## 6. Measurements

### 6.1 The harness

`misc/bench/rc_splitdec.cpp` + `rc_splitdec.ispc`. Not a shape benchmark this time: one
synthetic decoder (NL interleaved lanes, serial FSM bit-tree model, ctx reset per byte, real
coder arithmetic), and **every variant of one NL must finish with a bit-identical
fingerprint** over all lane state, the model's counters and the context — one decision
different anywhere diverges everything after it. All variants pass, in both modes, which is
also the end-to-end proof of the threshold form. Two statistics modes bracket the workload:

* **cold**: uniform p, FSM walking 32768 states — refill 0.090 bytes/bit, renorm branches
  effectively random, model working set beyond L1;
* **hot**: skewed p, 1024 states — refill ~0.000 bytes/bit, everything predicted, L1-clean.

For calibration, real data on the real decoder: 30 MB of text at ratio 0.59 refills at
0.074 bytes/bit; incompressible input at 0.125. The *volume* sits between the modes and
nearer cold; the *predictability*, which is what the fused coder's cost tracks, is nearer
hot (n is 0 for ~92% of bits on text, in runs).

### 6.2 The matrix

gcc 13.3, pinned, best of 4, 32M bits per run, clk at 2.8 GHz nominal; ±3% within the
session:

| variant | cold | hot |
|---|---|---|
| NL=16 fused, branchy refill *(the current decoder's shape)* | 32.8–33.5 | 9.2–9.4 |
| NL=16 fused, branchless counted refill | 32.7–34.3 | 13.0–13.5 |
| NL=16 split, intrinsics batch | 18.7–21.1 | 15.3–15.5 |
| NL=16 split, ISPC batch per group | 19.4–20.9 | 15.8–16.6 |
| NL=16 split, whole loop in ISPC (uniform model) | 27.6–27.8 | 22.4–22.6 |
| NL=64 fused, branchy refill | 32.9–33.1 | 10.0–10.2 |
| **NL=64 split, intrinsics batch, pipelined** | **15.0–15.2** | **12.5–12.9** |
| NL=64 split, ISPC batch per group | 18.0–18.1 | 15.5 |
| floor: model+decision only, no batch | 4.6–4.8 | 4.1–4.3 |

Derived, the two numbers the whole question reduces to:

|  | cold | hot |
|---|---|---|
| what the fused coder half costs (fused − floor) | **~28** | **~5–6** |
| what the split's batch costs (best split − floor − ~0.5 decision) | **~10** | **~8** |

The fused coder's cost is wildly data-dependent — mispredicted renorm branches and refill
work — while the batch's is nearly flat, because it is branchless and its latency is either
exposed (NL=16) or hidden (NL=64) but its µops always retire. **Split wins 2.2x where the
fused coder is miserable; loses 0.8x where the fused coder is comfortable.**

### 6.3 The ISPC findings in isolation

* Per-group C-ABI call vs inlined intrinsics, NL=16: **+0.5–0.7 clk/bit** — effectively
  free, because at NL=16 both are latency-bound and a `call` adds none that matters. This is
  the load-bearing ISPC result: batching at 16-bit granularity through a compiled-language
  boundary costs nothing.
* Same at NL=64: **+2.8–3.0 clk/bit** — once latency is hidden, the C-ABI's forced
  state round-trip through memory (the callee cannot keep zmm state across calls, cannot
  return `T` in a register) becomes the visible cost. To collect all of NL=64's win the
  batch must be inlineable — intrinsics, or ISPC's `-emit-llvm` LTO'd into the caller, which
  is unproven here.
* Whole-loop-in-ISPC: +7–8 clk/bit over per-group calls (both modes). Uniform codegen, see §5.
* Branchless counted refill in the *fused* decoder: a wash cold, **3.4–3.7 clk/bit worse
  hot** — on predictable data the branchy loop is nearly free while branchless does its work
  unconditionally. Consistent with the decoder plan (§4.4 there): branchless pays off only
  once interleaving destroys predictability, never on its own.

### 6.4 Real-coder anchors, same session

RCNUM=16 vs 64 on the real (fused, scalar) decoder, pinned A/B: 19.6–20.1 MB/s both
(17.0–17.4 clk/bit on this box today), csize +0.29% text / +0.17% random for 64. Random
input decodes at 18.1–18.3 MB/s (18.2–18.4 clk/bit) — barely different from text, which is
the point of §7.

## 7. Where the real decoder sits

The real decoder's own numbers place it: its coder half was measured at ~4.1 clk/bit (gcc,
probes 0−1 of the decoder plan), and even on incompressible input the whole decoder moves by
less than 10% (§6.4) — the FSM converges toward mid-scale p on random data, refill grows to
only ~1 byte per 8 bits, and the renorm branch stays largely predictable. The real decoder
never enters the cold regime. It lives at the hot end of the table, where the fused coder
half costs ~4–6 clk/bit and the batch costs ~8–10.

So, projected onto the real decoder: floor ≈ 13, split ≈ 13 + 0.5 + 8–10 ≈ **21–23 clk/bit
against the fused 17–18** — a ~0.8x regression on text, and about break-even at best on
hostile input. The split, as built, does not pay for itself in the regime the decoder
actually operates in. That is the honest conclusion, and it is the same shape as the
decoder plan's §3 (speculation): the fused chain is shorter than it looks, and a fixed
vector overhead has to be very small before it beats a data-dependent scalar cost that is
usually near its minimum.

What it would take to flip: the batch must come down from ~8–10 to under ~5 clk/bit
(~80 clk/group). §8 has the candidates; they are plausible collectively and unproven
individually. Until one of them lands, the split is a documented mechanism with exact
semantics and a measured cost model, not a recommendation.

## 8. Headroom, ranked

1. **Register-resident lane file.** NL=64 = 20 zmm for range/code/pos/ru/look — AVX-512 has
   32. Unroll the group loop by 4, keep everything but `T` out of memory: removes ~11
   load/store µops per group and the last forwarding risks. Estimate 1–2 clk/bit.
   Intrinsics or inlined-ISPC only (a C-ABI callee can't hold state).
2. **Division by reciprocal.** `vrcp14pd` + one Newton step + the same exact fixup, instead
   of two `vdivpd`: ~15–25 clk of latency and most of the divider pressure gone. Estimate
   1–2 clk/bit at NL=16, less at 64 (latency already hidden).
3. **Deeper slack.** RCNUM=128 doubles the hiding window for +~0.6% size (extrapolated);
   only worth it if 1–2 leave the batch latency-bound, which at NL=64 it largely is not.
4. **Byte-swapped substreams.** The container storing each lane's bytes in refill order
   would drop the `vpshufb` and one shift — trivial, but it is a format change and worth
   well under 1 clk/bit; the interleaved-container idea from the encoder doc's §6.3 remains
   the only refill change with real weight (it removes the gather).
5. **ISPC inlining.** `ispc --emit-llvm` + LTO into the C++ caller would merge 1 with the
   ISPC source form. Unproven toolchain path; if it works, the intrinsics variant becomes
   unnecessary.

## 9. The encoder is the actual ISPC customer

Everything this document measured about ISPC — free calls, OpenCL-class codegen, no
runtime — applies with no caveats to the path that already works: the encoder's carryless
coding pass. `rc_kernel.c` is SPMD over lanes with a two-way handshake per *block*, not per
group; it runs today through the OpenCL CPU runtime, i.e. through an ICD loader, a JIT, a
binary cache (`-k`), two command queues and a `clFlush` discipline, all to reach the same
AVX-512 units ISPC reaches with a function call. Porting it:

* The preprocessing chain is already language-agnostic text: `rc_macro.pl` and `defines.pl`
  don't parse C, and the kernel's OpenCL-isms are a small dictionary — `__kernel void` →
  `export void`, `__global uint*` → `uniform uint32* uniform`, `get_local_id(0)` →
  `programIndex`, per-lane locals → `varying`, `stdint` names → a typedef prelude
  (`rc_splitdec.ispc` shows the pattern). One `#if RC_ISPC` block in the shared source,
  one extra `mk_kernel.sh` output, and `ispc -o rc_kernel_ispc.o -h` at build time.
  `txt2inc.pl` and the `-k` cache have no ISPC equivalent because nothing is compiled at
  run time — that entire subsystem just disappears on this path.
* `intel_reqd_sub_group_size(16)` becomes `--target=avx512skx-x16` (and a `-x8` build for
  AVX2 hosts, selected at startup — something the OpenCL path got from the driver and the
  ISPC path must do itself; ISPC's multi-target mode emits both with a dispatcher).
* The block-read extension query, `PickLWS`, device enumeration, the fallback-and-drain
  machinery for a dying device — all deleted; a function call cannot fail asynchronously.
  `RC_CL_NBLK` pipelining becomes either nothing (call is synchronous and fast) or a worker
  thread if overlap with the model pass still pays; that is a measurement, not a design.
* Expected performance: the OpenCL CPU runtime is the same SPMD-on-SIMD compilation model —
  same gathers, same scatters, same sub-group width — minus which there is only overhead to
  lose. The risk is codegen quality variance (the whole-loop uniform result in §5 shows ISPC
  is not uniformly excellent), so the port is validated the way the OpenCL kernel was:
  byte-identical output against `-C`, and the instruction census against the known-good
  disassembly.

This is the concrete recommendation of the document: **do the encoder port**; keep the
decoder split on the shelf with its cost model until §8 moves the batch under ~5 clk/bit.

### 9.1 The port, done and measured

The port above is now implemented in this tree. One generated file serves both backends —
`rc_kernel.cl` is embedded for the OpenCL JIT *and* compiled by ispc with `-DRC_ISPC=1`,
so `mk_kernel.sh` did not change at all; the source grew an ISPC entry point (a `foreach`
over the RCNUM lanes around the same coder macros), fixed-width typedefs, and two
portability repairs that OpenCL C accepts identically (explicit `(uint64_t)` casts, since
an ISPC `UL` literal stays 32 bits, and ternaries where `bool+bool` appeared). The host
side is `rc_ispc.cpp`: the same `CL_*` names, a worker thread where OpenCL had command
queues, `RC_ISPC_THREAD=0` for the synchronous fallback. `build.sh` prefers ISPC when an
`ispc` binary is present (`ISPC=0` refuses it), maps the `RC_*` overrides to the kernel's
`-D` names, and builds `avx512skx-x16` plus `avx2-i32x16` with ispc's dispatcher so one
binary runs on both.

Correctness: eight configurations (`RCNUM` 8/16/64, `RC_LOWSPLIT`, `RC_LOWBYTES` 5/6/8,
`RC_RANGE64`, both output paths) through the full `t.sh` corpus — 768 checks, every device
encode byte-identical to `-C`. The OpenCL and no-backend builds still pass beside it.

Same-session pinned A/B on 30 MB of text:

| encoder | MB/s | kernel µs/block |
|---|---|---|
| host coder (`-C`) | 18.0 | — |
| OpenCL backend, all cores | 85.7 | 581 |
| OpenCL backend, 2 cores | 90.9 | — |
| **ISPC backend, worker thread, 2 cores** | **88.9–91.7** | **483–519** |
| ISPC backend, all cores | 92.7 | — |
| ISPC backend, synchronous, 1 core | 40.7 | 878 |
| ISPC backend, worker thread, 1 core | 44.2 | — |

Equal to OpenCL within this box's noise, with a kernel ~15% faster per block and the
startup JIT — measured at **2.81 s** here, the entire reason `-k` existed — gone, along
with the ICD loader, the binary cache, both command queues and the asynchronous-failure
machinery. Two codegen notes: the ispc kernel has **zero gathers** — inside `foreach` the
`pbit[k]` read is provably unit-stride, which replaces the Intel sub-group block-read
extension outright — and 38 branches against the OpenCL build's 46. And on this backend
`RC_LOWSPLIT=1` wins (~5%, 468 vs 549 µs/block): the 64-bit low accumulator costs two zmm
per varying value, so `build.sh` defaults the *kernel* to the split form — stream-neutral,
so the host keeps its own preference and the output does not move.

### 9.2 The decoder split, in the real codec

The split of §1–§6 is now real code too, behind `RC_DECSPLIT`: `rc_decode_batch` in
`rc_kernel.c` (always present in the kernel object), the scalar side in `rc_vec.inc`, one
`rc_Bulk` call in `model1.inc` where the model enters strict `i%RCNUM` order. The block's
header bits run fused — their lane sequence revisits lane 0 immediately, which the group
rotation cannot express — and `rc_Bulk` converts the lanes to split state after them; a
partial group at block end is dropped, which is exact because none of its lanes is read
again. No format change, no interface change: `rc_Process(rcidx,p,bit)` as before, and the
fused decoder remains the reference and the fallback for any configuration the split does
not cover (`RCNUM%16!=0`, `RC_RANGE64`, no ispc).

Verified through the full `t.sh` corpus at `RC_DECSPLIT=1` × {default, `RCNUM=64`,
`LOWBYTES=5`, `LOWSPLIT=1`} — 768 checks, byte-exact.

Measured, same-session pinned, real codec, 30 MB:

| decoder | text MB/s | random MB/s |
|---|---|---|
| fused (default) | 19.8–20.0 | 18.5 |
| split, RCNUM=16 | 14.1 | 13.8 |
| split, RCNUM=64 | 14.3 | — |

**0.71–0.75x — the cost model of §7 held** (it projected ~0.8x), and even the harness's
one hopeful case does not transfer: real random input is not hostile enough (§6.4), and
RCNUM=64's pipelining win is eaten by the per-call C-ABI state round-trip exactly as §6.3
measured. So the knob defaults to off, the fused decoder stays what the codec runs, and
`RC_DECSPLIT=1` is one flag away on a machine whose memory subsystem prices the batch
differently — the box here is a 2.8 GHz VM, and §8's headroom list is untouched.

### 9.3 OpenCL, removed

With the ISPC backend equal-or-better everywhere it was measured, the OpenCL path is gone
from the tree: `rc_cl.cpp`, the embedded `rc_kernel.inc`, the `txt2inc.pl` step, the `-k`
binary cache and the `-d/-p/-T` device options, the ICD machinery, the `RC_CL_*` knobs.
The kernel source generates to `rc_kernel.ispc` (the `.cl` name retired with the JIT), the
host API is `DEV_*` in `rc_dev.h`, and a build without ispc is simply the host coder —
`-C` always, nothing to fall back from. What this document's earlier sections say about
the OpenCL backend describes a tree that ends at the commit this section arrived in.

### 9.4 Simplified to the original's shape

Two structural follow-ups, on request, both after the numbers above:

* **The encoder is synchronous again** — model a block, code a block, the original
  sh_v1xN arrangement. The worker thread, the slot ring, `RC_DEV_NBLK`, the `NSLOT`
  dimension of `RCio` and the Submit/Collect API are gone; `DEV_Encode` is one call that
  codes straight into the substream rows, and `rc_ispc.cpp` is ~60 lines. What the thread
  was buying on the 4-vCPU reference box was real — 91 MB/s down to 42 single-core — but
  it was bought with machinery; on the user's AVX-512 box the synchronous original hit the
  same ~135 MB/s the threaded ISPC build did, which is the number that decided this.
* **One explicit ispc target, no dispatcher**: `ISPC_TARGET=avx512skx-x16` (default) or
  `avx2-i32x16` in `build.sh`, `set ispctarg=` in `gc.bat`, one object on the link line.
  Both targets pass the corpus byte-identically.

And one more negative result, from re-reading the original rather than trusting memory:
`sh_v1xN.inc`'s own decode split (its `RC_DECDIV` double-quotient decision plus batched
renorm) keeps the multiply and the range update *scalar per bit* on the lane arrays, with
no `{p;bit}` log at all — only the code-fold, renorm, refill and division are batched.
Rebuilt that shape through the ispc call boundary and measured it in the real codec:
**10.2–10.7 MB/s against shape A's 12.1 and the fused 17.0** (same session). What made it
right for the original — everything inlined into one loop clang could schedule across —
is exactly what a C-ABI batch call takes away: the scalar-written `range[]`/`rpm[]`
arrays are re-read by the batch a few instructions later and hit the store-forwarding
hazard that shape A's by-value qwords were designed around, and the extra per-bit scalar
work buys nothing back. Shape A stays; the original's decode split does not survive
translation to a function boundary. (Incidentally the original's `_val` double compare
and this document's floor+fixup threshold are the same exactness argument in two forms —
the 2^-17 gap between attainable quotients and the next integer.)

### 9.5 The decode gap, settled by disassembly

The user's cross-check that started this: on the same AVX2-built machine, the original
sh_v1xN decodes at 42.7 MB/s and this port at 28.6 — and the claim that the original's
decoder auto-vectorizes (SoA arrays, `[rcidx]` everywhere). Settled empirically against
the real 069 tree, rebuilt on the reference box with the same clang and flags:

**Where the vector code is.** 069's `Model<1>::do_process` does contain vector code —
139 xmm/ymm instructions (`vpinsrb`, `vpshufb`, `vpor` …) — and every one of them sits in
the **per-block setup**: clang SLP-vectorizes the sixteen lanes' pointer/`code`
initialization. The per-bit hot loop — sixteen unrolled bodies of ~26 instructions, one
`imul` each — contains **zero** vector instructions. So the auto-vectorization is real
and the hot decode path is scalar; both statements are true, and the decode speed lives
in the scalar body's shape. (069's `model1.inc` uses `Rangecoder1` from `sh_v1xN_c.inc`,
a plain class — the macro-generated coder serves `model0`; the SIMD `Rangecoder` decoder
sits under `#if 0`.)

**The three ingredients of that shape, now all in the port:**

1. `RC_UNROLL` — `#pragma unroll(RCNUM)` on the group loop, lane index a compile-time
   constant in every body, byte-boundary test decided at compile time.
2. `RC_DECSOA` — the lane state as four compact arrays, the original's structure.
3. **The branchy renorm** — the decisive one. `Rangecoder1` refills inside two
   predicted-not-taken tests, so the ~91% of bits with no refill pay nothing; the port
   had the counted branchless form everywhere, which is right for the vectorized encoder
   and wrong for the scalar decoder — the exact measurement the original had already made
   (`RC.txt`, 054: ShiftCodeN 62.91 → 51.66) and the port had inherited backwards.

Head to head on the reference box — same clang, `-march=haswell`, same input, external
timing, pinned: **069 original 33.9–35.8 MB/s, port before 28.6, port now 35.2.**
Parity. (gcc trails at 26.4 on the same source; the original was always a clang/icx
coder.) Full corpus green, `RC_DECSOA=0` keeps the old path.

This also reframes the split's standing one more time: the fused decoder gained another
~25% while the split did not, so the split's deficit widened again — every improvement to
the scalar loop is an improvement to the thing the split has to beat.

And the same 069 cross-check exposes the encoder's ISA dependence. At ISA parity on the
user's machine, ispc-AVX2 encodes at 46 MB/s against the original's AVX2 60; on this box
the AVX2-x8 kernel runs 1380 µs/block against AVX-512's 495, and 069's whole
single-thread encoder does 52.5 next to the ispc-AVX-512 sync path's 59. The kernel's
output is the dword scatter — `vpscatterdd` — and AVX2 has no scatter of any width, so
ispc scalarises every lane store, the pathology the OpenCL byte path had on AVX-512. The
original's AVX2 encoder never paid it because its output design (reverse-order 16-bit
stores, later the clset row) was AVX2-native. On AVX-512 the ispc kernel wins even
through the downclock (80 vs 60 on the user's box); an AVX2 build that must match the
original needs an AVX2-shaped output path.

Decompiling both ispc objects (the user's Hex-Rays dumps) makes the AVX2 pathology exact.
The AVX-512 kernel: 16 scatter instructions and 13 branches in the whole object. The AVX2
kernel: **80 scalar stores, 136 extracts, 203 branches** — every `rc_emit` site is a
movemask, a branch per lane, and an extract-address/extract-value/store pair per set bit,
executed *per coded bit*, because the emit stores the partial dword unconditionally and
the capacity mask is almost always all-true.

The obvious fix — store only when the dword completes, ~5x fewer emit events — was
implemented, verified byte-identical, and measured **slower on both targets** (AVX-512
493 → 615 µs/block, AVX2 1371 → 1538): with the all-true mask, the branch chains are
perfectly predicted and cost only their instruction count, while the sparse
data-dependent mask trades those predictable instructions for mispredicts, and adds mask
arithmetic to the AVX-512 scatter for nothing. Reverted; the negative result is the
point: on AVX2 the emit's cost is *count*, not prediction, and only removing per-lane
indexed stores entirely can cut it. That means a unit-stride row store — lane-linear
addressing that ispc provably compiles to one vector store, the pbit load already proves
it — plus a compaction pass: the clset design (071), which lost to the direct scatter on
AVX-512 and remains the one credible AVX2 route, as a target-specific variant. Effort is
a kernel rework plus host compaction; on a machine with AVX-512 the straight answer
remains `ISPC_TARGET=avx512skx-x16` (80 vs 60 through the downclock).

### 9.6 ISPC dropped: the coder generated from `rc.inc`, the 069 way

Sections 9.4–9.5 left the port in an unstable place: the decoder reshaped to the
original's scalar form and at parity, the encoder split between an ISPC kernel that is
excellent on AVX-512 and pathological on AVX2, and a second copy of the coder
(`rc_kernel.c`) to keep in sync with `rc.inc` by hand. The resolution is the one 069 had
all along: no separate vector language, no separate vector source — make clang
auto-vectorize the one coder, by generating the lane-array macro form from `rc.inc`
itself. ISPC is gone (`rc_ispc.cpp`, `rc_kernel.ispc`, `rc_kernel.c`, `rc_dev.h`
deleted), and the build needs only a C preprocessor and perl.

**The pipeline** (`mk_kernel.sh`, run by `build.sh`/`gc.bat` for the exact `-D`
configuration being built):

    cc -E -P -x c++ -DRC_VECOUT=1 -DRC_CARRYLESS=1 [config -Ds] \
       -imacros rc_config.inc  rc.inc            >  rc_kernel0.c
    perl rc_soa.pl   rc_kernel0.c                # -> rc_kernel1.c
    perl rc_macro.pl rc_kernel1.c                # -> rc_kernel1_macro.c
    perl defines.pl  rc_kernel1_macro.c          # -> rc_vecD.inc

The C preprocessor resolves the `#if` forest of `rc.inc` for one configuration
(`-imacros` supplies the config's macros without pasting its text, so `RCNUM`,
`SCALElog` etc. stay symbolic and bind at the include site). `rc_soa.pl` is the new
step: fields become `ALIGN(VECSIZE) T name[RCNUM];` arrays, every method grows a
`uint rcidx` first argument, every field mention becomes `name[rcidx]`, byte I/O
becomes cursor arrays over one buffer (`tmpptr[RCNUM]` + `tmpbase`), value-returning
helpers are dropped (a macro cannot return a value; their bodies are inlined in the
`RC_VECOUT` variants inside `rc.inc`), and duplicate overload names get `_2` suffixes.
The existing `rc_macro.pl` + `defines.pl` chain — unchanged, the same scripts 069
shipped — then turns those functions into multi-line macros and the declarations into
plain statements, so `#include "rc_vecD.inc"` inside `do_process` makes the whole
coder state **function locals**: the alias-freedom clang needs, per §9.4's ablation.

`rc.inc` carries small `#if RC_VECOUT` variant bodies for the few methods whose scalar
form can't survive macro-ization (ShiftLow's output store becomes one unconditional
little-endian word store plus `tmpptr += n`; Renorm and Quit inline their helper
calls). Everything else — and both scalar coders — is the same text.

**Three coders, one source.** The generated `rc_vecD.inc` is the carryless fast coder;
when a block's `FFNum[]` shows escaped carries, `model0` re-encodes it with the
carry-propagating twin — the plain `RangecoderN<…,Rangecoder_CY>` template
instantiation of the very same `rc.inc`. The `-C` switch selects the scalar carryless
reference the same way. Nothing is duplicated; a change to `rc.inc` reaches all three
on the next build. `-DRC_RANGE64=1` (no vector form) and `-DRC_VEC=0` keep the pure
scalar build.

**Measured** (reference box, clang, `-march=haswell`, external pinned timing, 069
rebuilt in the same session):

| coder | encode MB/s | decode MB/s |
|---|---|---|
| 069 original | 52.49 | 35.63 |
| this, generated coder | 49.84 | 35.81 |

Decode at parity, encode at 95%. The encoder's pbit sweep genuinely auto-vectorizes —
`model0::do_process` disassembles to 295 ymm-register lines — leaving a residual ~5%
attributed at first to the one structural difference: 069 emits each lane's bytes as
reversed single-word stores into its private tail, where this format's forward
substreams must put the *top* byte first, against the store's little-endian order.

The forward emit had spelled that swap out as two byte extracts, two masks, and an or
— five ALU ops plus the store against 069's single shift plus raw word store — and an
interim fix rewrote it as a bswap16 idiom clang folds into the store (`shr; movbe`),
worth +12% encode on its own. But the right answer was the one 069 actually
implements, and it needs no swap at all: **the substreams run backwards.** `put1` and
`get1` both decrement, `ShiftLowN` stores `cl` — low aligned so its top byte is `cl`'s
top byte — with a plain little-endian store ending at `tmpptr+1` and commits 0..2
staged bytes by the counted decrement, `rc_Write` memcpys the used region ascending
(so the file holds each lane's chunk last-emitted-first), and the decoder starts at
the chunk's top and walks down, over-reading into the 0xFF pad *below* it. With the
cursor running down, the LE store lands the bytes in stream order by itself:

    qword cl = lowc;                       // LOWBYTES==8: zero shifts
    (qword&)tmpbase[tmpptr-6] = cl;        // top byte at tmpptr+1
    tmpptr -= n;

(one shift where low is narrower or split; whole `lowh` as-is for split LOWBYTES==8).
Adopted end-to-end: the generated coder, `RC_IO`'s scalar cursors, `RCio`'s chunk
placement, both models' cursor init — so the lane chunks in the container are now
byte-reversed relative to the earlier forward builds, the one deliberate format
change in this whole port, and the encoder–scalar–twin byte-identity checks all
moved over with it.

Same-session three-way, interleaved and pinned (the box ran globally slower that day
— 069 itself re-timed at 40.0–40.4 enc / 28.8 dec — so the ratios are the signal):
encode **reversed 49.3–49.9 MB/s** vs bswap-form 47.2–47.9 vs pre-fix 42.1, decode at
parity throughout (29.3/29.7/28.8). `model0::do_process` shrank from 1879 to 1210
instructions. The port now encodes ~23% ahead of 069 on the same box with decode at
parity — the "layout cost" ran the other way: the forward layout was costing the
port, and 069's reversed layout is simply the better fit for a little-endian store.

Vector-vs-scalar `-C` output stayed byte-identical on the first build after the flip,
and the full `t.sh` matrix — grown to cover every emit variant (default, `RCNUM=8`,
`RCNUM=64`, `RC_LOWSPLIT=1`, `RC_LOWBYTES=4/5/6/7` split and non-split,
`RC_FORCE_CARRY=1`, `RC_VEC=0`) — is green, 528 checks.

With that, §8's ledger closes: the batched split, OpenCL, and ISPC are all removed
from the tree, each for the measured reason recorded above, and the codec is back to
one source file and a four-line generation step — 069's architecture, reversed
substreams included, carried over to the carryless format.
