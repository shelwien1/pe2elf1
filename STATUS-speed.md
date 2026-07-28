# xadpcm speed work — status log

Implementing `xadpcm_speed_v4.md`. Rule for this pass: **take everything that is
bit-exact; take a ratio trade only if it costs a few bytes at most.** That makes
Tier 0 and Tier 1 in scope wholesale, and puts most of Tier 2 out of scope by
construction — §3.1/§3.2/§3.3 each cost kilobytes, not bytes. Each is measured
and reported below rather than assumed.

## Method

* **Correctness:** `./md5s.sh` — md5 of the compressed output over 24
  (file, mode) combinations: both real files × {default, `-s`, `-ss`}, all 14
  synthetic wavs, their concatenation × {default, `-ss`}, and a two-input
  archive × {default, `-ss`}. Every Tier 0/1 commit must leave all 24 md5s
  untouched. `./verify.sh` additionally round-trips everything.
* **Speed:** `./bench.sh [binary] [reps]` — `perf` is not available in this
  container, so it reports **min-of-N user CPU seconds** (N = 5). Minimum, not
  mean: the noise here is additive (shared box), so the fastest run is the best
  estimate of the true cost. Spread across runs is under 2%. Four measurements:
  encode/decode × MS (`wavs2`, 2.5 MB) / IMA (`Player_Death_Music_ima.wav`,
  600 KB); `TOTAL` is their sum and is what the table below tracks.
* Builds are `./mk.sh release` (Const) unless stated. Baseline flags are the
  tree's own: `-O2 -fstrict-aliasing -fomit-frame-pointer -fno-exceptions
  -fno-rtti`, no `-march`.
* Machine: Intel Xeon @ 2.80 GHz, 4 cores, AVX2 + AVX-512F available.

## Results

| # | item | TOTAL (s) | vs base | md5 |
|---|---|---|---|---|
| — | baseline (rev after the refactor) | 39.067 | 1.00× | — |
| 1 | §1.5 clz + §1.9 hoisting + §1.11 stats gate | 36.948 | 1.06× | identical |
| 2 | §1.1 linearize history + §1.2 SIMD kernels + §1.3 | 26.533 | **1.47×** | identical |
| 2b | …the same, built `ARCH=-march=haswell` | 25.492 | **1.53×** | identical |
| 3 | §1.7 counter arena, 64B rows, huge pages | 25.226 | 1.55× | identical |
| 4 | §1.4 SIMD mixer + stride 32, §1.10 STC | 22.803 | **1.71×** | identical |
| 5 | §1.6 no divisions, §1.12 dead leftovers | 22.4 ¹ | 1.74× | identical |

| 6 | §1.8 prefetch schedule | 21.7 | 1.80× | identical |
| 7 | §1.16 build: `-O3`; **clang++**; `ARCH=-march=native` | **18.562** | **2.10×** | identical |

¹ within the noise band of run 4 on the whole-suite number; measured properly by
alternating A/B on the path it touches — see below.

| 8 | §1.15 RC carry loop; §2.2 tried and reverted | — | — | identical |
| 9 | §2.1 size `Pred` from its actual stage lengths | — | — | identical |
| 10 | §1.13 IMA state-transition tables | — | — | identical |
| 11 | review pass: AVX-512, §1.3 specialisation, lazy match arena | — | — | identical |
| **final** | **clang++ `-O3 -march=native`** | **16.877** | **2.31×** | identical |
| final | gcc `-O3`, portable (mk.sh default) | 19.891 | 1.96× | identical |
| final | 20 × 20 KB inputs (the directory case) | 9.25 | **1.79×** | identical |

Runs 8–10 are individually below the ±2% whole-suite noise floor and were
measured by alternating A/B on the paths they touch; see the log. Every binary
in this table produces the same 24 md5s.

---

## Log

### 1 — §1.5, §1.9, §1.11 (the "afternoon" group)

**§1.5 bit-scan loops → clz.** Three loops, ~8 calls per sample-channel:
`Pred::adapt` and `Pred::adaptx` each ran `for(u64 e = en+n*fl; e>1; e>>=1) s++`
(up to 64 iterations), and `ms_dlog` ran `for(u32 x=d; x>>=1;) e++`. Replaced
with `63-__builtin_clzll(x|1)` and `31-__builtin_clz(d)`. The `|1` guards
`clzll(0)`, which is undefined; the argument is never actually 0 here (`n` and
`fl` are both `pclamp`ed to ≥1, and `adaptx` only runs under `if(n4)`), but the
guard is free and survives a widened clamp. `ms_dlog`'s `d` is floored at 16 by
its own first line, so no guard is needed there.

**§1.9 per-symbol hoisting.** Three changes in `code_symbol`/`mbit`:

* The 24 `cs[n++] = row+nd` stack stores were rebuilt on every bit — 88 stores
  per 4-bit symbol that differ only by `nd`. `cs[]` is now built once per symbol
  as row *bases* and `mbit` takes `nd` and indexes `cs[i][nd]`.
* `tMS.row(srow)` / `tML.row(lrow)` genuinely change per bit (the row depends on
  the expected bit and on whether the match is still alive), but only three rows
  per model are reachable inside one symbol. Those three are resolved once and
  selected in the loop.
* `if(sl)` in `mbit` was dead — one call site, always passing `rQs+nd`. Deleted.

**§1.11 compile out the accounting.** `stat[scat] += CLT[pf][b]` ran on every
coded bit and kept the 16 KB `CLT` table hot in L1 for the benefit of `-v`
alone. Now behind `XAD_STATS` (default 0), which also drops `CLT` itself and its
`log2()` initialisation. `-v` without it reports the byte totals it still has
and says to rebuild with `-DXAD_STATS=1` for the per-stream split.

Result: **39.067 → 36.948 s, 1.06×**, all 24 md5s identical.

### 2 — §1.1 linearize the NLMS history, §1.2 SIMD kernels, §1.3 specialise on length

The big one. Three items that only make sense together.

**§1.1.** The history was a 1024-entry ring read as `h[(t-1-i)&HM]`: a load, an
AND and a wrapping index on each of ~1500 taps per sample-channel, and
unvectorizable because consecutive `i` walk descending, wrapping addresses. It
is now a **newest-first sliding window** — `h[0]` newest, `h[i]` *i* instants
older — so the weights (lag-ascending) and the history are parallel and both
loops are contiguous ascending runs. `push` writes downward (`*--h`); when the
cursor reaches the bottom the live part is copied back to the top of the slack.

Two things fell out that are worth recording:

* `dot`/`dotx` and `adapt`/`adaptx` **collapse into one pair each**. The only
  difference between them was `h[(t-1-i)]` versus `h[(t-i)]`, which was never
  about the kernel — it was about whether the current instant's value had been
  pushed yet. With a sliding window the push itself says that: `set_cross`
  pushes before `predict` reads, the own-signal push happens after.
* `HSLACK` = 256 makes a history `PRED_NMAX+264` entries = 3.0 KB, which is
  *less* than the 4 KB ring it replaces. The copy costs `n8*4/HSLACK` ≈ 6
  bytes/sample against several hundred taps. So the footprint went **down**
  while the loops became vectorizable — this is most of what §2.1 was for.

**§1.2.** `xad_simd.inc`: `xad_dot64` and `xad_adapt` in AVX2 and SSE4.1
intrinsics, `_mm256_mul_epi32` throughout, every product kept at full 64 bits.
Bit-exactness is structural, not empirical — the dot accumulates into `i64` and
two's-complement addition is associative under overflow, so lane-splitting
reproduces the scalar order exactly; adapt is elementwise. AVX2 has no 64-bit
*arithmetic* right shift, so adapt uses the xor–sub identity
`sra(x,s) = ((x >>ᵤ s) ^ b) - b`, `b = 1<<(63-s)`.

**Dispatch.** Compiled for AVX2 (`ARCH=-march=haswell`, which is what `g.bat`
has always used) the kernels are called directly and inline into `Pred`.
Otherwise they carry `target` attributes and a one-time `__builtin_cpu_supports`
check picks between them through a function pointer: one portable binary, full
speed on hardware that has AVX2, at the cost of one indirect call per stage.
That cost is measurable and small — **26.533 s dispatched vs 25.492 s
inlined, 4%**.

**§1.3.** Already in place from the refactor: `Pred<VAR>`'s stage lengths are
`IDXC` constants, so in the Const build every kernel call has a compile-time
`n`. Added here: `n1p..n4p`, the lengths rounded up to a whole vector. `dot`
runs to `n#p` and relies on `w[n..n#p)` being zero — `adapt` writes only
`[0,n)`, `reset` clears the array and the lengths never change, so those lanes
are zero for the object's life and contribute exactly nothing. That removes
dot's scalar tail without changing a bit. **adapt keeps its tail** (≤7 taps) on
purpose: it *writes*, and letting it run into the padding would make those
lanes nonzero, which `dot` would then pick up.

**A real bug this found.** `b = (i64)1 << (63-sh)` is undefined behaviour at
`sh = 0` — shifting a signed 1 into the sign bit. Corrected to
`(i64)((u64)1 << (63-sh))`. Real audio never reaches `sh = 0` (the clamp is
`PRED_SHMAX` = 47), so no test on actual data could have found it.

**§4.4 kernel differential test.** `ktest.cpp` + `./ktest.sh`: 200k `dot` and
200k `adapt` cases per ISA level, values drawn to include 0, `INT32_MIN`,
`INT32_MAX` and full-range randoms, `sh` swept over the whole legal `[0,63]`
with both ends forced often, `n` covering every tail length, and `dot` also
called from an unaligned cursor (which is how `Pred` always calls it). Run at
four build levels — portable/dispatched, `-msse4.1`, `-mavx2`, `-march=native`
— because the inlined and the dispatched builds are different code generations
of the same intrinsics. **0 failures, 1.6 M cases.**

### 3 — §1.7 cache and TLB

All 24 counter tables were separate `std::vector`s at 16-byte alignment. At
`nsym = 16` a row is `sizeof(Ctr)*16` = 64 bytes — exactly one cache line — so
three rows in four straddled two lines and cost two misses where one would do.
Within a symbol the node index walks 1 → {2,3} → {4..7} → {8..15}, all inside
one row, so an aligned table costs **one line per symbol, not one per bit**.

They now sub-allocate from one `Arena` at 64-byte granularity, which also drops
the page count over ~27 MB of essentially random access and lets the whole thing
be backed by huge pages (`mmap` + `MADV_HUGEPAGE` on Linux, silent fallback to
`aligned_alloc` elsewhere): ~7 000 4 KB pages become ~14 2 MB ones.

Sizing it needed care. The layout is 26 expressions in `bind_tables()`, and
duplicating them to compute a total would be two things to keep in step. Instead
the arena has a **dry mode**: the first `reset_stats` runs the whole binding pass
with `take()` handing out nothing and only accumulating, sizes the arena from
that, and runs it again for real. The sizes depend only on `amaxsym`/`amaxnib`,
fixed for the object's life, so every later reset re-binds the same layout —
which is also §2.3, since the refill is now a walk over contiguous memory rather
than 24 `vector::assign` calls with per-element construction.

Not done, deliberately (the doc says so and it is right): the tables are *not*
merged into one indexed slab. They have different index spaces and row counts —
`tP` is `nsym` rows, `tP4` is `nsym⁴`, `tHJ` is `2^hj_bits` — so no shared row
index exists.

**25.226 s, 1.55×.**

### 4 — §1.4 SIMD mixer, §1.10 fold the `pr()` clamp

**§1.10.** `Ctr::pr()` clamps `p>>CTR_SH` to `[P_MIN,P_MAX]`, but `p` is `u16`
and `CTR_SH` is 4, so the value is in `[0,4095]`: the upper clamp is dead and
the lower one only maps 0 → 1. `STC[i] = STT[i ? i : 1]` folds both into the
stretch lookup, and `mbit` reads `STC[cs[i][nd].p >> CTR_SH]` — two compares and
a select gone, 26 times per coded bit.

**§1.4.** Weight-row stride padded 28 → 32 and the array 64B-aligned out of the
same arena: at stride 28 a row was 112 B straddling two or three lines, at 32
it is exactly two. `nx` is 26 on every `mbit` call, so `st[26..32)` is zeroed
once and both loops run to the constant 32 — a zero input contributes 0 to the
sum and its weight update is `(0*el)>>20 = 0`, so the wider loop is bit-exact
and unrolls with no remainder. `mix` reuses `xad_dot`; `upd` gets its own kernel
(`xad_mixupd`), which is `xad_adapt` plus the symmetric clamp as
`min_epi32`/`max_epi32`.

**Where this departs from the plan.** §1.4 says `el = err*lr` "reaches ±2^28 and
therefore cannot be a `mul_epi32` operand directly", and budgets four multiplies
per 8 lanes for an exact hi/lo split. It fits: `|err| ≤ 4095` (because `pr_`
comes from `squash()`, clamped to `[1,4095]`) and `lr ≤ 65535` (PMUL), so
`|el| ≤ 268 364 025` — a factor of eight inside `i32`. And
`i64(st)·i64(err)·i64(lr) == i64(st)·i64(err*lr)` exactly, since `err*lr` is
itself exact. So **one** multiply per lane, not four. Both bounds come from the
arithmetic contract, so the file carries a `static_assert` rather than a comment.

**A real bug, caught by the md5 gate.** The first attempt folded
`STC[p] = STT[p<P_MIN ? P_MIN : p]` into the loop that *builds* `STT` — so
`STC[0]` read `STT[1]` one iteration before it was written, and every archive
grew by ~6% (wavs2 1 248 335 → 1 323 176). It is a second pass now, and the
comment says why. Worth recording as the case for the md5 matrix: the codec
still round-tripped perfectly, so only the size check caught it.

**22.803 s, 1.71×.**

### 5 — §1.6 remove the integer divisions, §1.12 drop the dead leftovers

Six 64-bit `idiv`s per sample-channel on the MS path (`ms_quantize` ×4,
`ms_conf` ×2), all sharing one runtime divisor the compiler cannot
strength-reduce.

* **`ms_quantize`** takes a reciprocal. The reciprocal is `((1<<32)-1)/d`, not
  `(1<<32)/d`: the domain starts at **1**, not 16 — `ms_delta_update` floors at
  16 but the per-block header path clamps only at 1 — and the plain form
  truncates to 0 exactly there. Better still, the reciprocal is now carried
  *next to* iDelta (`Ch::dl` / `Ch::rcp`, refreshed by `set_delta()` wherever
  delta moves), so the four calls share it and the inner loop has **no division
  at all**: six per sample-channel become one per nibble.
* **`ms_conf` and the IMA confidence** are `floor(num/den)` capped at
  `NCONF-1`, and `c >= k` is exactly `num >= k*den`, so counting how many of the
  `NCONF-1` thresholds the numerator clears gives the same answer with no
  division. At the shipped `NCONF = 2` that is **one comparison**. The plan
  proposed an 89×4 reciprocal table for the IMA side; a table lookup where one
  comparison does is the wrong trade, so this went the other way.
* **`ms_predict`'s `/256`** is a signed truncating division, not a shift.
  Branch-free exact form: `(x + ((x>>63)&255)) >> 8`.
* **§1.12 dead leftovers.** `q2`/`q3` never read their remainder: the IMA path
  wrote `lo2` twice into a variable nobody reads, the MS path computed `e2`/`e3`
  and dropped them. They now call leftover-free variants. The other half of
  §1.12 — sharing `step>>i` across the four IMA quantizer calls — was left
  alone: with `BPS` a compile-time constant those loops fully unroll and GCC
  already CSEs the shifts, so hand-fusing would add a four-accumulator loop for
  nothing.

**Measurement note.** On the whole-suite number this looked like *zero* change
(22.803 → 22.813 s). It is not: the box's noise band is ±2% (three consecutive
runs of the same binary gave 22.085 / 22.924 / 22.592). Re-measured by building
both revisions and **alternating** them, 7 reps each, on the MS encode path this
actually touches:

```
  MS encode  prev  9.118 s      MS encode  prev  8.914 s
  MS encode  new   8.607 s      MS encode  new   8.557 s
```

**≈4–5% on MS encode**, which is what the plan predicted for the divisions.
Every later item is measured this way rather than off the suite total.

**§5.5 exhaustive verification.** The quantizer primitives moved to
`xad_msq.inc` — no IDX dependency — so `ktest.cpp` can include them, and the
*reference* implementation is kept there rather than deleted, because "bit-exact"
has to be bit-exact against something. The test brute-forces
`ms_quantize_r`/`ms_quantize_nq` against it: **exhaustively for `d ∈ [1,64]`
over every `|diff| ≤ 16d+8`** (every quotient cell and both sides of the ±8
clamp), then for larger `d` every power of two and its neighbours plus a dense
arithmetic sample, against diffs straddling `k·d` and `k·d ± d/2` — where an
off-by-one in the reciprocal would show — plus the extreme magnitudes a
**non-standard coefficient table** reaches. The plan bounds `|P| < 2^17`, which
holds for the standard table; a wav may carry its own with `|c|` up to 32767,
putting `|P|` near 8.4e6, so the sweep runs out to 2^24. **912 073 cases, 0
failures.**

### 6 — §1.8 prefetch schedule

The 24 context rows split by *when their index becomes known*. Twelve are
functions of `c.prev*`, the scale bucket and the other channel — all final
before the predictor runs, because channel *k−1* was coded earlier this instant
(and for *k = 0*, the previous instant). `ib`, `xch` and `xch2` are now computed
at the top of the per-channel body instead of after the quantizers (a pure
reordering of pure functions — nothing that writes moved), and
`prefetch_early()` issues the twelve there. The ~760 taps of the two NLMS
cascades then sit between the prefetch and the use: several hundred cycles of
cover for a working set that misses essentially every time.

The eleven q-dependent rows have nothing left to hide behind — `q^` only exists
once the cascades have run — but they are still prefetched at the head of
`code_symbol` rather than at first use, which buys the whole mixer setup.

The address arithmetic is deliberately repeated rather than threaded through
`code_symbol`'s signature: a dozen integer multiply-adds against the memory
latency they are hiding.

Measured with both revisions at the *same* `-O3` (the first attempt compared an
`-O2` build against an `-O3` one and conflated the two): **11.108 / 11.596 s
without, 10.794 / 10.683 s with — 4–6%.**

### 7 — §1.16 build

Three of the four recommendations hold; one does not.

**`-O3`:** adopted, though on its own it is inside the noise band against `-O2`.
Harmless and the plan asks for it.

**`-march=native` / `-march=haswell`:** ~5%, and the mechanism is specific —
it is not autovectorization, it is that `__AVX2__` lets `xad_simd.inc` call the
kernels directly instead of through the CPUID-dispatched function pointer, so
they inline into `Pred`. `g.bat` has always used `-march=haswell`, so the
Windows build already had this.

**clang.** Not in the plan; worth it. **clang++ 18 is 6–9% faster than g++ 13**
on this codec at the same flags, alternating A/B:

```
  g++   -O3                10.654 s      clang++ -O3                10.030 s
  g++   -O3 -march=native   9.737 s      clang++ -O3 -march=native   8.883 s
```

`CXX=clang++` already worked — mk.sh honours it — and is now documented there.
Two clang-only warnings were real and are fixed: a `/*` inside a block comment
in three files, and `#pragma GCC diagnostic ignored "-Wsubobject-linkage"`,
which clang does not have. A third was in `Lib3/coro3b.inc`, whose
`#if defined(_MSC_VER) || defined(__clang__)` guard on three MSVC pragmas is
wrong for clang on a non-Windows target; narrowed to `__clang__ && _WIN32`,
which is what clang-cl is.

**PGO: rejected, measured.** The plan expects 10–20%. On top of `-O3` it is a
small **regression** (10.459 / 10.513 s against 10.179 / 10.309 s for plain
`-O3 -march=native`). That is consistent rather than surprising: PGO's value
here would have been in branchy scalar code, and the branchy scalar code is what
§1.1/§1.2/§1.6 replaced. Not adopted; the reasoning is recorded in mk.sh so it
is not re-tried blind.

**`-ffast-math`: not used**, as the plan insists. The only floating point is
`init_tables`, and that is exactly where it is dangerous — `SQT`/`STT` come from
`exp`/`log` and a one-ULP shift at a rounding boundary changes a table entry and
therefore the output.

**The md5 identity test is now cross-compiler**, which is stronger than the
plan's `-O0/-O2/-Ofast` version: g++ at `-O2`/`-O3`/`-O3 -march=native`/PGO and
clang++ at `-O2`/`-O3`/`-O3 -march=native` all produce the same 24 md5s.

**`INLINE` (plan's last bullet):** the `#ifdef 13` the plan flags does not exist
in this tree — `INLINE` comes from `Lib3/common.inc`, correctly guarded on
`__GNUC__`, and `nm` confirms `mbit` has no out-of-line copies.

### 8 — §1.15 range coder I/O, §2.2 interleave the cascades

**§1.15.** Two of its three items were already gone: the output vector and its
per-byte `push_back` capacity check disappeared when the coder became a
coroutine layer (`shift_low` writes through a pin cursor), and the decoder's
"bounds check" is the pin's window test, which is the protocol rather than an
overhead to pad away. What was left is the carry loop: `cachesz` now goes
through a local so the loop does not reload the member, and the pending-carry
case is marked unlikely (it is one output byte per 10+ coded bits). Effect is
inside the noise; kept because it is strictly less work.

**§2.2 — tried, measured, reverted.** `pd` and `pdB` really are independent, so
hoisting both `predict()` calls adjacent lets the scheduler overlap two
reduction chains. Measured: **neutral under clang, ~1.8% worse under gcc**
(10.683/10.568 s → 10.856/10.762 s). Register pressure is the plausible reason —
both cascades' `p1..p4` then live across the whole pair. Reverted, with the
numbers recorded in the source so it is not re-tried blind. A real interleave
would have to fuse the dot loops themselves, which is a different and much
larger change; after §1.2 the NLMS block is no longer where the time is.

### 9 — §2.1 shrink `Pred`

`w1..w4` were `[PRED_NMAX]` and the histories `[PRED_NMAX+HSLACK+8]` regardless
of the stage they serve: 20.6 KB per `Pred`, and there are four (two cascades ×
two channels), so the predictor alone was **82 KB** — out of L1 for no benefit.
They now come from the same arena as the counter tables, sized from the actual
stage lengths: **29 KB**, which fits alongside the hot counter rows.
`PRED_NMAX` remains the clamp bound on the IDX numbers; it is no longer the
allocation. `reset()` also stops clearing 82 KB per segment.

The allocation rides the existing dry/real binding pass, so it is sized by the
same code that lays it out. **~3% under both compilers** (gcc 21.596 → 20.924 s,
clang 9.197 → 8.862 s on the encode pair).

### 10 — §1.13 IMA state-transition tables

`ima_apply`'s magnitude and `index_update`'s next index depend only on
`(index, code)` for a fixed `bps`, so both become a lookup and the
`code_magnitude` loop plus the `index_update` switch leave the apply path. The
sign is folded into the stored magnitude. `clip16` stays — `pcm + mag`
genuinely leaves i16 range and the saturation is part of the reconstruction.

Measured on the IMA path alone (it touches nothing else, and MS never builds or
reads the tables): **3.829 → 3.815 s, ~0.4%** — which is the plan's own
"a small win", and honestly is at the edge of what this box can resolve. Kept:
non-negative, and it removes work rather than adding a trade.

No MS equivalent exists, as the plan says: `ms_apply` and `ms_delta_update` are
functions of iDelta, a runtime value over `[1, 2 796 202]`.

---

## Tier 2 — not taken, and why

The brief was to take anything that does not cost more than a few bytes. Every
Tier 2 item costs kilobytes, so all three are out on the stated criterion. They
are listed with what they would cost so the decision is reviewable rather than
implicit:

* **§3.1 drop marginal context models.** `tHJ` is 16.8 MB for a measured
  −1 771 B on the plan's corpus, `tP4` 4.2 MB for −981 B, `tQPX` 0.28 MB for
  −1 868 B. Dropping `tHJ` alone would take the arena from ~27 MB to ~10 MB and
  change the TLB picture qualitatively — by far the best speed-per-byte trade
  available here — but it is ~1.8 KB on wavs2, not "a few bytes".
* **§3.2 the second cascade.** `pdB` is ~34% of the NLMS taps for a measured
  −0.43%, i.e. thousands of bytes on these files.
* **§3.3 match model sizing.** Halving the short model's `MATCH_TB` costs
  collisions, hence bytes. Note this one is now *reachable from IDX without a
  code change*: the refactor split `MATCH_HB2`/`MATCH_TB2` out, so the two
  models can be sized independently by `opt.pl`.

The plan's own note applies to all three: re-measure on the full corpus before
committing, which is a tuning run rather than a code change.

## Summary against the plan's targets

The plan predicted **1.8–2.4× bit-exact**, with the Amdahl cap at ~3.4×.
Delivered **2.13×** (clang, `-march=native`) and **1.77×** portable gcc, with
every one of the 24 output md5s unchanged from the pre-optimization baseline
under seven different compiler/flag combinations.

Where the results diverged from the plan, in both directions:

| plan says | measured |
|---|---|
| §1.4 `err*lr` needs an exact hi/lo split, 4 mul/8 lanes | it fits `i32`; 1 mul/lane, with a `static_assert` on the bound |
| §1.6 IMA wants an 89×4 reciprocal table | a threshold count is exact and cheaper at `NCONF = 2` |
| §1.16 PGO worth 10–20% | small **regression** on top of `-O3` |
| §2.2 interleaving the cascades helps | neutral (clang), 1.8% worse (gcc) |
| §1.13 "a small win" | ~0.4%, at the edge of measurable |
| — (not in plan) | **clang++ is 6–9% faster than g++** at equal flags |
| §1.1 `HSLACK` costs footprint | it *reduced* it: 3.0 KB/history against a 4 KB ring |

Two real bugs were found by writing the verification the plan asked for rather
than by the optimization itself: the `(i64)1 << 63` UB in the AVX2 shift
identity (§1.2), and an `STC` table built one iteration before its input
existed (§1.10) — the latter grew every archive ~6% while still round-tripping
perfectly, so only the md5 matrix caught it.

---

# Review pass

A read-through of the result against the plan, after the fact. Three real
defects, two of them pre-existing and neither caused by the optimization work;
one plan premise that turns out to be wrong by an order of magnitude; and one
plan item I had wrongly written off.

## Defects found

### R1 — `g++ -Os` produces a binary that segfaults

Not from the speed work: bisected across every commit back to the coroutine
port itself. Every other configuration is correct **and** byte-identical —
g++ `-O0/-O1/-O2/-O3/-Ofast`, clang++ `-O0..-O3/-Os/-Oz/-Ofast`, with and
without `-march=native`, AVX2 and AVX-512 paths, 18 configurations checked.

The failure is inside Lib3's `yield()`: the **first** yield is correct
(`p = &Menc`), the **second** receives `p =` the *value of* `Menc.stkptrH` — a
stack address — and the stack-copy `memcpy` then walks off the end of the
mapping. So `__builtin_setjmp`/`__builtin_longjmp` plus the manual
save-and-restore of the coroutine's stack is not surviving GCC's `-Os` register
allocation. That is the hazard GCC documents for those builtins (a
language-runtime facility with constraints, not a general mechanism), and
`coro3b.inc` already records one earlier `-O2/-Ofast` miscompile of a previous
implementation of the same thing. A bare twenty-line Lib3 coroutine survives
`-Os`; it takes a deeper call chain to expose.

Not fixable from this side without rewriting the coroutine, so the build now
**refuses** rather than shipping a crashing binary:

```c
#if defined(__GNUC__) && !defined(__clang__) && defined(__OPTIMIZE_SIZE__)
#error "g++ -Os miscompiles the Lib3 coroutine ..."
#endif
```

**This is in Lib3, so the 021/ddsdet tree has it too.**

### R2 — half a gigabyte of hash table, zero-filled up front

`vector<u32> tab` in each match model is `2^26 × 4 B` at the shipped
`MATCH_TB`, so **512 MB across the two**, and `assign()` eagerly writes every
byte of it at startup. On a 20 KB input that is 0.14 s of *user* time against
**0.8–3.8 s of system time** — the run spent 85–95% of its wall clock having the
kernel fault in half a gigabyte it would barely look at. This is exactly the
directory-of-short-files case §2.3 was worried about, and it dwarfs what §2.3
addressed.

Fixed by giving the match models their own mmap'd arena: the kernel's
demand-zero pages provide the same guaranteed zeros, but only for pages actually
probed. Two details matter:

* **No `MADV_HUGEPAGE` on this arena.** The counter arena wants huge pages
  (dense, 27 MB); this one must not have them, because a first touch would fault
  in 2 MB where a random probe needs 4 KB.
* **Adaptive pre-fault.** Lazy is not always right: measured, a 20 KB input is
  39% faster lazy (20 runs, 16.55 s → 10.15 s) while a 2.5 MB input is 8% faster
  eager (8.66 s vs 9.34 s), because a long run touches every page anyway and one
  streaming fill beats ~131 000 random demand faults. So `Codec::init` now takes
  the payload size and pre-faults above `PREFAULT_MIN`. Both ends keep their
  win.

Net: **20 short files 16.55 s → 9.25 s (1.79×)**, long files unchanged, peak RSS
on a short input 544 MB → 265 MB.

### R3 — a silent-corruption path in the arena

Table sizes are fixed after `Codec::init`, and the arena is sized by a dry pass
on the first `reset_stats`. If `init` were ever called twice with a wider
geometry the second `bind_tables` would lay a larger set into the block sized
for the first — silent heap corruption. Not reachable today (one `init` per
carrier per run), but it is one comparison to make it an abort with a message
instead, so `Arena::check()` now runs after every bind.

## A plan premise that is wrong

§1.7 sizes the working set at "**~75 MB** of essentially random access: ~27 MB
counters + ~48 MB match-model history and hash tables", and the whole dTLB
argument rests on that. The counter figure is right; the match figure is off by
more than 10×. Measured, reported by `-v`:

```
  model arenas: counters 27.4 MB, match 512.2 MB
```

So the real working set is **540 MB**, not 75 MB, and the match tables are 95%
of it. That does not invalidate what §1.7 asked for — the counter arena still
benefits from huge pages and 64B rows — but it does mean the *dominant* TLB
pressure is somewhere §1.7 never looked, and that §3.3 (match model sizing) is a
much bigger lever than the plan's ordering suggests. Note the shipped
`MATCH_TB` pattern evaluates to 30 and is then `pclamp`ed to 26, so most of that
parameter's search space saturates at the ceiling — worth knowing before the
next tuning run.

## A plan item I had wrongly written off

### AVX-512 (§1.2, second half) — now implemented, 9–10%

The plan calls for `_mm512_mul_epi32` + `_mm512_srai_epi64` +
`_mm512_reduce_add_epi64`. I had not implemented it, and — worse — the header
comment in `xad_simd.inc` claimed the dispatcher chose between AVX2 **and
AVX-512F**, which was simply false. Implemented and measured:

```
  avx2    9.398 s      avx512  8.373 s
  avx2    9.180 s      avx512  8.399 s
```

**9–10% faster, bit-identical.** `_mm512_sra_epi64` is a true 64-bit arithmetic
shift, so `adapt` drops the xor–sub identity entirely. It is now the default
where the hardware has it, with an opt-out (`-DXAD_NO_AVX512`, or `XAD_AVX512=0`
for the dispatched build) because 512-bit downclocking is a real effect on some
server parts and this file cannot know the target. `ktest.cpp` covers the new
kernels.

I had speculated in an earlier note that AVX-512 would lose to downclocking.
That was a guess presented alongside measurements, which is the one thing this
log is supposed not to do; it is corrected here and in the source.

### §1.3 — kernel specialisation, now actually done

`Pred`'s stage lengths were already compile-time constants, but the kernels took
`n8` as a runtime argument and were not forced to inline, so the trip count did
not fold. On the compile-time ISA paths the kernels are now `always_inline`,
which is what §1.3 asks for: a constant bound, full unroll, no loop control.
Worth **~1%** (8.094 → 8.018 s) — small, and consistent.

## Also fixed

* `xad_simd.inc`'s header described two kernels and an AVX-512 dispatch that did
  not exist; `xad_pred.inc` still claimed storage was "sized for PRED_NMAX"
  after §2.1 changed exactly that. Both corrected.
* `APM` was the last `std::vector` on the coding path, so it was allocated **and
  filled twice** at startup — once in the arena's dry sizing pass. Moved to the
  arena.
* Four build configurations (g++/clang++ × portable/native) now compile with
  **zero warnings**; the clang-only ones were a duplicate `inline` specifier
  introduced by the §1.3 change.

---

# Coverage audit: every numbered item in xadpcm_speed_v4.md

Checked item by item against the source, not against memory.

| item | state | where |
|---|---|---|
| §0 measure first | **adapted** — `perf` is not in this container, so min-of-N + alternating A/B | `bench.sh`, `md5s.sh` |
| §1.1 linearize the NLMS history | **done** | `xad_pred.inc` `Hist` |
| §1.2 SIMD dot/adapt, AVX2 | **done** | `xad_simd.inc` |
| §1.2 …AVX-512F variant | **done** (review pass, R-section) | `xad_simd.inc` |
| §1.2 CPUID dispatch, one binary | **done** | `xad_simd.inc` selection block |
| §1.3 template kernels on stage length | **done** (review pass) | `XAD_T_*` = `INLINE` on compile-time paths |
| §1.4 SIMD the mixer | **done**, with the operand split shown unnecessary | `xad_mix.inc`, `xad_mixupd` |
| §1.4 stride 28→32, 64B-align | **done** | `MIX_STRIDE`, `Arena` |
| §1.5 bit-scan loops → clz | **done** (3 sites) | `xad_pred.inc`, `xad_ms.inc` |
| §1.6 reciprocal for `ms_quantize` | **done**, carried on `Ch` so the inner loop has none | `xad_msq.inc` |
| §1.6 `ms_conf` comparison chain | **done** | `xad_ms.inc` |
| §1.6 IMA 89×4 reciprocal table | **superseded** — threshold count is exact and cheaper at NCONF=2 | `ima_conf` |
| §1.6 `ms_predict` `/256` branch-free | **done** | `xad_ms.inc` |
| §1.7 64B-align every CtrTab | **done** | `Arena::take` |
| §1.7 one arena | **done** | `Codec::cta` |
| §1.7 huge pages | **done** (counters only — see R2 for why not the match arena) | `Arena::reserve` |
| §1.7 do *not* merge tables into one slab | **respected** | — |
| §1.8 prefetch schedule | **done**, both halves | `prefetch_early`, `code_symbol` |
| §1.9 hoist `cs[]` out of the bit loop | **done** | `xad_codec_bits.inc` |
| §1.9 three match rows per model | **done** | `msr[]`/`mlr[]` |
| §1.9 delete dead `if(sl)` | **done** | `mbit` |
| §1.10 fold `pr()` into the stretch table | **done** | `STC` |
| §1.11 compile out the accounting | **done** | `XAD_STATS`, `XSTAT` |
| §1.12 drop dead `q2`/`q3` leftovers | **done** | `ima_quantize_nl`, `ms_quantize_nq` |
| §1.12 fuse the four quantizer loops | **declined**, measured reasoning: `BPS` is compile-time so they unroll and GCC already CSEs `step>>i` | — |
| §1.13 IMA state-transition tables | **done** (~0.4%, the plan's own "small win") | `ima_mag`/`ima_nxt` |
| §1.14 `template<bool ENC>` | **already done** by the earlier refactor (`MD`) | `Codec<MD>` |
| §1.14 `template<int BPS>` | **already done** by the earlier refactor | `code_data_ima<BPS,XST>` |
| §1.15 reserve the output vector | **moot** — the coroutine port removed the vector; the RC writes a pin cursor | `xad_rc.inc` |
| §1.15 hoist `cachesz`, hint the carry | **done** | `shift_low` |
| §1.15 decoder bounds check | **moot** — it is the pin's window test, i.e. the protocol | — |
| §1.15 leave renorm a loop | **respected** | — |
| §1.16 `-O3` | **done** (inside noise vs `-O2`, adopted anyway) | `mk.sh` |
| §1.16 `-march=native` | **documented knob**, ~5% | `mk.sh` `ARCH` |
| §1.16 PGO 10–20% | **rejected, measured** — small regression on top of `-O3` | `mk.sh` comment |
| §1.16 no `-ffast-math` | **respected** | — |
| §1.16 verify `INLINE` is applied | **checked** — the plan's `#ifdef 13` does not exist here; `nm` shows no out-of-line `mbit` | — |
| §2.1 shrink `Pred` | **done**, 82 KB → 29 KB | `Hist::alloc`, `Pred::alloc` |
| §2.2 interleave the two cascades | **tried, reverted**, numbers in-source (neutral clang, −1.8% gcc) | `xad_codec_ima.inc` note |
| §2.3 `reset_stats` contiguous fill | **done** via the arena | `bind_tables` |
| §3.1 drop `tHJ`/`tP4`/`tQPX` | **declined** — costs ~1.8 KB on wavs2, brief allows "a few bytes" | — |
| §3.2 disable the B cascade | **declined** — −0.43%, thousands of bytes | — |
| §3.3 halve the short model's `MATCH_TB` | **declined** — costs collisions; now reachable from IDX without a code change | — |
| §3.4 do not touch the contract | **respected** | — |
| §4.1 md5 before/after every commit | **done** | `md5s.sh`, 24 combinations |
| §4.2 round-trip matrix | **done** | `verify.sh` |
| §4.3 `-O0/-O2/-Ofast` identity | **done and extended** — 18 configurations, two compilers, both ISA paths |
| §4.3 ASan/UBSan | **done**, with the caveat that ASan's stack instrumentation cannot coexist with a stack-copying coroutine (`--param asan-stack=0`) | — |
| §4.3 geometry fuzz | **partial** — 14 synthetic geometries cover every walk instantiation, but it is a fixed corpus, not a fuzzer |
| §4.4 kernel differential test | **done**, extended to AVX-512 | `ktest.cpp` |
| §5 brute-force the reciprocal domain | **done**, 912 073 cases | `ktest.cpp` |

**Two items are not fully closed**, both deliberately:

* **§4.3 geometry fuzz.** `gen_testwavs.py` produces a fixed 14-file corpus that
  reaches every payload-walk instantiation, plus odd block sizes and a
  non-standard MS coefficient table. That is coverage, not fuzzing — nothing
  generates *random* geometries or malformed headers looking for a crash. The
  container parser does get malformed input in `verify.sh`, but only one case.
* **§1.12's loop fusion**, declined with a reason rather than measured. If it
  were to be revisited, the measurement is cheap; the argument against is that
  the compiler already does it.

Everything else in the document is either implemented, superseded by something
measured to be better, or declined against the stated
"no more than a few bytes" rule with its price recorded.
