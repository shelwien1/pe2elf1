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
