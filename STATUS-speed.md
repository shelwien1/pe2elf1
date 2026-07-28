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

Best configuration is `CXX=clang++ ARCH=-march=native ./mk.sh release`
(18.562 s). The default portable gcc build is 22.069 s = 1.77×. Every one of
those binaries produces the same 24 md5s.

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
