# paq8hpc speed: every idea in paq8hpc_speed.md, built and measured

All 20 proposals in `paq8hpc_speed.md` — the five implemented flags, the two
"next in line" items, the five §7 null results, the decode fast path, and the
five §10 untested items — were implemented as macro gates in
`paq8hp_speed.hpp` / `paq8hpc_speed.cpp` / `sh_v1m_speed.inc`, verified
byte-identical, and measured. A build with no defines is byte-for-byte the
uploaded baseline.

**Result: 1.54x at L4, 1.66x at L8, 1.79x at L11 on full book1, byte-identical
output.** The doc's own reference implementation reported 1.40x / 1.57x.

```
g++ -O3 -march=native -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 \
    -DPAQ_GETSIMD -DPAQ_APMPF paq8hpc_speed.cpp -o paq8hpc
```

## 0. Headline

Full book1 (768,771 B), min-of-3 end-to-end wall clock, pristine baseline vs the
five flags above. Output compared byte-for-byte at every level, plus
cross-decode in both directions.

| level | model | baseline | final | speedup | output |
|------:|------:|---------:|------:|--------:|-------:|
| 4  | 67 MB   | 27.74 s | 18.04 s | **1.54x** | 194,348 B identical |
| 8  | 592 MB  | 31.74 s | 19.11 s | **1.66x** | 193,109 B identical |
| 11 | 4512 MB | 44.45 s | 24.88 s | **1.79x** | 193,108 B identical |

Attribution, leave-one-out from the final config at L8/128 KB, n=15 shuffled
interleave. In-block control read −0.4% / 7-of-15 and +1.2% / 8-of-15 in the two
blocks, which is the noise band each row has to beat:

| removed from final | median | wins |
|--------------------|-------:|-----:|
| `PAQ_PREFETCH`            | −26.2% | 0/15 |
| `PAQ_AVX2`+`PAQ_DOT2` (back to SSE2) | −9.3% | 0/15 |
| `PAQ_GETSIMD`             |  −4.4% | 2/15 |
| `PAQ_APMPF`               |  −3.7% | 3/15 |
| `PAQ_DOT2` alone          |  −2.0% | 3/15 |

Cumulative ladder, same protocol (n=11, control +1.3% / 8-of-11):

| build | L4 64 KB | L8 128 KB |
|-------|---------:|----------:|
| baseline                | +0.0%  | +0.0%  |
| `+PREFETCH`             | +22.4% | +37.7% |
| `+AVX2`                 | +32.2% | +46.1% |
| `+DOT2`                 | +39.3% | +51.5% |
| `+GETSIMD`              | +45.4% | +58.0% |
| `+APMPF` (final)        | +52.2% | +64.3% |

## 1. Measurement protocol — the doc's §1 is not strong enough here

The doc warns about drift. It is worse than described, and this changes several
verdicts, so the method had to change first.

A single binary, run 12 times back to back at L8/128 KB, reported:

```
34.70 36.62 37.77 27.86 32.07 33.99 35.63 34.70 34.95 32.96 34.79 33.95 KB/s
```

a 35% spread. Worse, the doc's own procedure — interleave N binaries in fixed
slot order, report best-of-5 and win rate — is *biased*, not merely noisy. Five
**byte-identical copies** of one binary in five slots reported:

```
   p_z1  +0.0%  (ref)      p_z3  +4.0%  6/9 wins
   p_z2  -1.1%  3/9        p_z4  +2.3%  7/9 wins
                           p_z5  -0.5%  4/9
```

So "+4%, 7-of-9 wins" is what *no change at all* looks like. Every effect the
doc reports in its ±4% band, and every effect this work initially measured
there, is unresolvable by that method. Two fixes:

- **In-block control.** Every A/B carries an extra slot holding a byte-identical
  copy of the reference. Whatever it reports is that block's noise, measured
  under exactly the conditions the candidates saw. A candidate counts only if it
  clearly beats the control. (`ab.sh`)
- **Shuffled slot order** each round, since fixed order penalises the first
  slot systematically.

And for anything whose mechanism is instruction count rather than latency,
wall clock was abandoned entirely in favour of **callgrind `Ir`**, which is
exact and reproducible to the instruction (`icount.sh`). Note `-march=native`
emits AVX-512 that valgrind cannot decode — the Ir builds use `-mavx2`.
Cachegrind is *not* usable for the prefetch items: it models neither the
hardware prefetchers nor miss cost, so a software prefetch appears as pure
added instructions with none of its benefit.

Regression set for byte-identity (`verify.sh`), run on all 27 builds: 7
(input, level) pairs — including L0, L11 and both published doc vectors —
each checked for encode md5, own round-trip, and **cross-decode of the pristine
baseline's archive**. Plus full book1 at L4/L8/L11 encode-identity and
cross-decode in both directions.

## 2. Confirmed wins (all five in the recommended config)

### `PAQ_PREFETCH` — +37.7% at L8, the one big lever (§3)
Exactly as the doc describes: all `cn` probe addresses issued at the top of
`mix1` at bpos 0/2/5. Measured +37.7% at L8 (11/11) and +22.4% at L4 (11/11)
added to the baseline, and −26.2% (0/15) when removed from the final config.
The doc's absolute numbers reproduce closely (31.81 vs their 31.3 at L8).

### `PAQ_AVX2` — +6% (§4)
The doc's mechanics are right and its warnings are worth repeating: row stride
is 912 B so `loadu` is mandatory, and the `nx ≡ 8 (mod 16)` tail must be a
128-bit step rather than padding `nx`. Ir: **−13.13%** instructions.

### `PAQ_DOT2` — +2.0% (§10.1, was untested)
Two weight rows per `dot_product`/`train` pass. Ir: **−16.94%** vs baseline,
i.e. 3.8 points beyond AVX2 alone — the largest instruction-count reduction
measured. Wall clock +2.0% (3/15 against control 8/15), which is what a
compute-side win is worth in a latency-bound program.

Safety note the doc does not raise: `train2` writes two rows in one interleaved
pass, so it would be wrong if two mixer contexts ever selected the same row.
They cannot — `Mixer::set()` advances `base` by `range` on every call, so the
six `cxt[]` values live in disjoint intervals. Documented at the call site.

### `PAQ_GETSIMD` — +4.4% (§10.5, predicted ~0)
SIMD checksum compare in `ContextMap::E::get()`: one 16-byte load covers
`chk[0..6]`, `movemask & 0x3fff`, lowest set lane is the entry the scalar loop
would have returned first. **The doc predicts ~0 ("after §3's prefetch the line
is in L1 and the 7-entry scan is short"); measured +4.4% (2/15 vs control
8/15), the second-largest win in the set.** Ir is only −1.36%, so the gain is
not instruction count — it is the seven data-dependent branches per probe,
201 probes per byte. The doc's reasoning about locality was right and about
branch cost was missing.

For the record, the doc's §10.5 aside is correct: qwen's masking arithmetic for
this is wrong. `0x7f7f` is not the right mask. It is `0x3fff` on the
`movemask_epi8` of an `epi16` compare (each matching lane sets two bits;
lanes 0–6 are bits 0–13), and the lane index is `ctz(mask)>>1`.

### `PAQ_APMPF` — +3.7% (§3 APM variant, doc measured a wash)
All six APM contexts are functions of state final before `contextModel.p()`, so
the hashes hoist above it and both lines of each 33-entry region prefetch under
the whole model call. **The doc kept this only because it "costs nothing";
measured +6.0% (11/15) isolated and −3.7% on removal from the final config.**
It is a real win, not a wash — plausibly because the doc measured it as a
6-probes/bit item against 25/bit, which understates it: those six probes sit on
the serial dependency chain at the *end* of the bit, with nothing after them to
hide behind.

## 3. Confirmed null or negative — measured, not inherited

| item | doc's claim | measured here |
|------|-------------|---------------|
| `PAQ_WXPF` mixer row prefetch (§7) | null | **null**: −0.3%, 7/15 vs control 6/15 |
| `PAQ_PF2` speculative bpos-1/4 probes (§3) | "small marginal gains" | **null**: −2.3%, 6/15 |
| `PAQ_RCMPF` rcm/cm hoist (§3) | "small marginal gains" | **null**: +0.4%, 8/15 |
| `PAQ_AVX512` (§4/§7) | no gain, removed | **confirmed**: −0.7% vs baseline alone, and −6.5% when swapped for AVX2 inside the full stack. (Both on the pre-control harness, so read the sign, not the magnitude — but it never won.) No Ir figure: valgrind cannot decode AVX-512. |
| `PAQ_N16` raise N to 464 (§4) | legal, unmeasured | **negative**: Ir −12.59% vs AVX2's −13.13%; −1.3% wall. Aligned loads do not pay for the wider `wx`. |
| `PAQ_CXTFL_T` cxtfl templating (§10.3) | "expect ~0, clean and free" | **negative**: −0.7%, and Ir −2.03% — *fewer* instructions but slower, i.e. the second `mix1` instantiation costs more in I-cache than the branch cost |
| `PAQ_RESTRICT` (§10.4) | hygiene, ~0 | **exactly zero**: Ir −0.00% (4 instructions in 2.9 G) |
| `-fno-exceptions -fno-rtti` (§10.4) | hygiene, ~0 | **null**: Ir −0.15% |
| `-flto` (§10.4) | mostly moot, single TU | **null**: Ir −2.05%, wall inside noise |
| `-Ofast` (§7) | pure no-op, no FP in hot path | **confirmed exactly**: Ir −0.00% |
| PGO (§7) | +0–2% | **null**: +0.6%, 6/11 vs control 5/11 |
| `FIXED_LEVEL=n` (§10.2) | kills 11 template bodies' I-cache | **null on speed**: −0.5%, 5/11. Binary does shrink 436 KB → 94 KB. |
| `PAQ_DECFAST` (§8, qwen §3.1) | correct, sub-1%, not built | **built and exact**; effect below noise (+0.7%, 6/11). Ir is flat: it trades a 64-bit divide for a multiply-shift, same instruction count, and one divide per bit against 44 K madds per byte is invisible. The algebra in §8 checks out — see `sh_v1m_speed.inc` for the derivation. |

## 4. `PAQ_HUGEPAGES` and `PAQ_LAZY_ZERO`: a real bug, then real numbers

The doc reports both as unprovable-in-container washes. Neither was actually
being exercised, for a reason that is a defect in the uploaded baseline:

> **`Predictor` is an over-aligned type, so `new Model` never called
> `malloc1`.** `ContextMap::t`, `BH::t` and `Mixer::wx` are `alignas(64)`, which
> makes `Predictor` over-aligned, which makes `new Model` resolve to the C++17
> **aligned** `operator new(size_t, align_val_t)`. The baseline overrides only
> the plain form. So the single 4.5 GB allocation — "the only allocation in the
> program", per the source comment — went straight to the library allocator.
> `g_memory` never saw it, and any allocator-level gate hung off `malloc1`
> silently did nothing at all.

`paq8hpc_speed.cpp` now overrides the aligned forms too. Verified with an
allocation trace: before, the only call reaching `malloc1` at L9 was the 0.0 MB
`Coder`; after, `alloc 1151.7MB ... zeroed=1`. This is why the doc's §5/§6
measured a wash — there was nothing to measure. With the fix, at L11 on a
300-byte input:

| build | startup+total wall | peak RSS |
|-------|-------------------:|---------:|
| final                     |  9.339 s | 4515 MB |
| final `+LAZY_ZERO`        | **0.086 s** | **64 MB** |
| final `+HUGEPAGES`        | 18.091 s | 4513 MB |
| final `+HUGEPAGES+LAZY_ZERO` | 0.088 s | 71 MB |

- **`PAQ_LAZY_ZERO` is a 109x startup win and 70x RSS win** at L11 — the ctor
  memsets really are pure waste over OS-zeroed storage, exactly as §6 argues.
  But it is **not** free on throughput: it moves page population from one fast
  sequential memset into random demand faults during compression, and those cost
  more per page. Measured −9.4% (0/11) on the KB/s metric at L8/128 KB, and
  end-to-end on full book1: L8 18.94 s → 19.84 s (−4.8%), L11 28.27 s →
  27.89 s (+1.4%, break-even — it saves 9.2 s of memset and spends ~8.9 s in
  faults). **Verdict: off by default, correct choice for short inputs at high
  levels.** The crossover is where model size stops dominating input size.
- **`PAQ_HUGEPAGES` is a measurable loss here, not a wash**: 18.09 s vs 9.34 s.
  `AnonHugePages: 0 kB` in `smaps_rollup` mid-run confirms the container never
  grants a page (THP is in `madvise` mode), so `MADV_HUGEPAGE` buys nothing
  while making the kernel work harder on every fault. Steady-state −0.3%.
  **Off by default.** The doc's advice stands: verify `AnonHugePages` on the
  target before believing any number. The Windows `VirtualAlloc` +
  `MEM_LARGE_PAGES` path is implemented as §5 specifies (including the
  `AdjustTokenPrivileges` return-value trap) but is compile-only here — no
  MinGW in this container, so unlike the doc this work has **no cross-OS
  verification**.

`PAQ_POISON_NEW` + `PAQ_LAZY_ZERO` is rejected at compile time with `#error`,
per §6.

## 5. Not pursued

`dot2`'s sibling ideas that would change the stream (§9's (a)–(k)) were not
built; that section's analysis was spot-checked and holds — in particular (a)'s
ceil/floor error and (e)'s `squash()` returning 0 for `d < -2047`, which makes
the `p<1` clamp load-bearing.

## 6. Reproducing

```
./build.sh FINAL -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF
./verify.sh /tmp/.../p_FINAL FINAL          # byte-identity, 7 cases x 3 checks
./ab.sh   vfy/b128k 8 15 p_FINAL p_CANDIDATE  # wall clock, in-block control
./icount.sh vfy/b16k 4 c_base c_CANDIDATE     # exact instruction counts
```
