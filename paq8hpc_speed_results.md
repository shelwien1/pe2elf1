# paq8hpc speed: every idea in paq8hpc_speed.md, built and measured

All 20 proposals in `paq8hpc_speed.md` — the five implemented flags, the two
"next in line" items, the five §7 null results, the decode fast path, and the
five §10 untested items — were implemented as macro gates in
`paq8hp_speed.hpp` / `paq8hpc_speed.cpp` / `sh_v1m_speed.inc`, verified
byte-identical, and measured. A build with no defines is byte-for-byte the
uploaded baseline.

**Result: 1.54x at L4, 1.66x at L8, 1.79x at L11 on full book1, byte-identical
output.** The doc's own reference implementation reported 1.40x / 1.57x. Also
built with MinGW and verified under wine: Windows archives are byte-identical to
Linux ones and each OS decodes the other's output (§6).

**Read §7 before choosing flags.** Measurements on real hardware with newer
compilers reach **1.71x** with four more macros, and flip three of the verdicts
in §3 — this container's ±4% noise floor (§1) cannot resolve effects that size,
and where the two disagree, §7 wins.

Portable set — wins on both hosts tested, enable unconditionally:

```
-DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF
```

Best measured, on real hardware with clang (§7) — the extra four are
target-dependent, A/B them together on your machine:

```
clang -O3 -march=native -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD \
      -DPAQ_APMPF -DPAQ_RESTRICT -DPAQ_N16 -DPAQ_WXPF -DPAQ_CXTFL_T \
      paq8hpc_speed.cpp -o paq8hpc
```

Then try adding **`-DPAQ_BPOS_T`** (§8): it cuts 6.43% of all instructions —
three times what `PAQ_CXTFL_T` saves — but like `CXTFL_T` it needs a quiet
machine to show up in wall clock, and it did not on this host.

`P_SCALE` is a real parameter now: `-DPAQ_P_BITS=n` with `-DPAQ_LOGISTIC` works
for n in 8..16, all within 1% of the 12-bit reference and 11..16 within 0.04%
(§12).

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

Cross-OS byte-identity (`verify_win.sh`) is covered in §6.

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

## 3. Null or negative *in this container* — but see §7

Four rows below are overturned by the real-hardware measurements in §7:
`PAQ_WXPF`, `PAQ_N16` and `PAQ_CXTFL_T` are wins there (the first two only in
combination), and `PAQ_RESTRICT` is a tie rather than a zero. They are left as
measured here, with the flip noted inline, because the *reason* each one differs
is the useful part. Everything else in the table holds on both hosts.

| item | doc's claim | measured here |
|------|-------------|---------------|
| `PAQ_WXPF` mixer row prefetch (§7) | null | **null here**: −0.3%, 7/15 vs control 6/15. **Flipped in §7** — a win on a smaller-L3 CPU, exactly as the original doc predicted. |
| `PAQ_PF2` speculative bpos-1/4 probes (§3) | "small marginal gains" | **null**: −2.3%, 6/15. Confirmed dead in §7 too. |
| `PAQ_RCMPF` rcm/cm hoist (§3) | "small marginal gains" | **null**: +0.4%, 8/15. Confirmed dead in §7 too. |
| `PAQ_AVX512` (§4/§7) | no gain, removed | **confirmed**: −0.7% vs baseline alone, and −6.5% when swapped for AVX2 inside the full stack. (Both on the pre-control harness, so read the sign, not the magnitude — but it never won.) No Ir figure: valgrind cannot decode AVX-512. |
| `PAQ_N16` raise N to 464 (§4) | legal, unmeasured | **negative here**: Ir −12.59% vs AVX2's −13.13%; −1.3% wall. **Flipped in §7** when paired with `WXPF` rather than tested alone. |
| `PAQ_CXTFL_T` cxtfl templating (§10.3) | "expect ~0, clean and free" | **negative under gcc 13.3**: −0.7%, though Ir is −2.03% — *fewer* instructions but slower. **Flipped in §7 under clang** (+2.5%): the win was always in the instruction count, gcc's layout was eating it. |
| `PAQ_RESTRICT` (§10.4) | hygiene, ~0 | **exactly zero**: Ir −0.00% (4 instructions in 2.9 G). §7 agrees: 0.17%, i.e. free but idle. |
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
  `AdjustTokenPrivileges` return-value trap) and is exercised under wine — see
  §6.

`PAQ_POISON_NEW` + `PAQ_LAZY_ZERO` is rejected at compile time with `#error`,
per §6.

## 5. Not pursued

`dot2`'s sibling ideas that would change the stream (§9's (a)–(k)) were not
built; that section's analysis was spot-checked and holds — in particular (a)'s
ceil/floor error and (e)'s `squash()` returning 0 for `d < -2047`, which makes
the `p<1` clamp load-bearing.

## 6. Cross-OS: MinGW build, verified under wine

MinGW gcc 13.2 and wine 9.0 were installed in this container, so unlike the
earlier revision of this file the Windows side is now built *and run*, not just
reasoned about.

```
x86_64-w64-mingw32-g++ -O3 -mavx2 -static -DPAQ_PREFETCH -DPAQ_AVX2 \
    -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF paq8hpc_speed.cpp -o paq8hpc.exe
```

Four Windows binaries compile warning-clean and were verified with
`verify_win.sh`, which checks the leg that per-platform testing cannot: an
archive written on one OS decoding on the other.

| build | flags | cross-OS result |
|-------|-------|-----------------|
| `w_base.exe`  | none                       | ok, 5 cases |
| `w_final.exe` | the five recommended       | ok, 5 cases |
| `w_hl.exe`    | + `HUGEPAGES` `LAZY_ZERO`  | ok, 5 cases |
| `w_dec4.exe`  | + `DECFAST` `FIXED_LEVEL=4`| ok, 1 case  |

"ok" means all four of these held for every case: Windows encode output is
**byte-identical to the Linux baseline's archive**, Windows round-trips its own
output, **Windows decodes the Linux archive**, and **Linux decodes the Windows
archive**. The doc's published vector reproduces on Windows too: b64k L4 →
20321 B, md5 `5b20ec75933c5f38b209d9948e432ed8`.

**The Windows allocator path is genuinely taken, not silently falling back.**
Traced with `-DPAQ_VERBOSE_ALLOC` under wine at L8:

```
no gates:            alloc 591.7MB -> 00007f5bcec40080 zeroed=0   (_aligned_malloc)
HUGEPAGES+LAZY_ZERO: alloc 591.7MB -> 00007f199fa50000 zeroed=1   (VirtualAlloc)
```

The 64 KB-granular pointer and `zeroed=1` confirm `VirtualAlloc`. The
`MEM_LARGE_PAGES` attempt fails under wine (no `SeLockMemoryPrivilege`, which is
also the default on real Windows), so it exercises exactly the fallback §5
specifies — and because plain `VirtualAlloc` still returns zeroed pages,
`PAQ_LAZY_ZERO` stays valid on that fallback. That is the property worth
checking, and `w_hl.exe` passing byte-identity is the proof.

Windows measurements (under wine, so treat as order-of-magnitude — wine adds
per-call overhead and this is not a native Windows run):

| measurement | baseline | final |
|-------------|---------:|------:|
| b128k L8 throughput, best-of-3 | 20.58 KB/s | **35.96 KB/s** (1.75x) |
| L9 startup, 300-byte input, min-of-3 | 3.689 s | **0.152 s** with `LAZY_ZERO` |

Both effects reproduce on Windows with the same sign and rough magnitude as
Linux, including the lazy-zero startup collapse.

Caveat worth stating: `-mavx2` is the portable floor used here, not
`-march=native`, and wine is not Windows. This establishes that the code
compiles, runs, and is bit-exact across OSes — it is not a Windows performance
benchmark.

## 7. Real hardware, newer compilers: three verdicts flip

Everything above §7 was measured in the container described in §1, whose ±4%
noise floor cannot resolve a 2–4% effect. The numbers below were contributed by
the author on **real Windows hardware with gcc 16.1 and clang**, encoding and
decoding full book1 (768,771 B), and they resolve exactly the band this
container could not. **Where they disagree with §2/§3, they win** — a quiet
machine measuring a 2% effect beats a noisy one calling it null.

Reported as single runs, cumulative unless noted, seconds:

| build | encode | decode |
|-------|-------:|-------:|
| gcc, no macros (baseline)                  | 29.043 | 28.531 |
| gcc, the five of §0                        | 18.331 | 17.961 |
| clang, the five of §0                      | 17.721 | 18.847 |
| `+PAQ_RESTRICT`                            | 17.691 | 17.717 |
| `+PAQ_PF2`                                 | 18.057 | 18.881 |
| `+PAQ_RCMPF`                               | 17.821 | 18.432 |
| `+PAQ_N16`                                 | 17.779 | 17.604 |
| `+PAQ_N16 +PAQ_WXPF`                       | 17.458 | 17.403 |
| `+PAQ_N16 +PAQ_WXPF +PAQ_CXTFL_T`          | **17.028** | **16.986** |
| same, minus `PAQ_DOT2`                     | 17.425 | 17.972 |

**1.71x over the no-macro baseline**, versus the 1.66x this container measured
for the five-macro set at L8. The best configuration on that hardware is all
nine:

```
clang -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF \
      -DPAQ_RESTRICT -DPAQ_N16 -DPAQ_WXPF -DPAQ_CXTFL_T
```

Three §3 verdicts flip, and the reasons are mechanical rather than statistical:

- **`PAQ_N16` + `PAQ_WXPF` is an interaction neither of them shows alone.**
  `N16` by itself is a wash (17.779 vs 17.691) — matching this container's −1.3%
  — and `WXPF` was null here. Together: 17.458. The mechanism is visible in the
  source: `N16` makes the `wx` row stride 928 B, a 32-byte multiple, which is
  what lets the AVX2 kernel use `_mm256_load_si256` and makes a row a
  line-aligned span; prefetching *that* is worth more than prefetching a span
  that straddles. §3 tested the two separately, and paired `N16` against `DOT2`
  instead — the wrong pairing, and the reason the interaction was missed.
- **`PAQ_WXPF` flipping is what the original `paq8hpc_speed.md` §7 predicted**:
  "the verdict may flip on CPUs whose L3 is smaller than `wx` + the ContextMap
  hot set; re-measure there." This container has a 33 MB L3 against a 9.6 MB
  `wx`, so a row miss cost only an L3 hit. That prediction was right and the
  gate was correctly kept.
- **`PAQ_CXTFL_T` is compiler-dependent.** −0.7% under gcc 13.3 here, +2.5%
  under clang there. §3 blamed I-cache pressure from the second `mix1`
  instantiation, but instruction count *fell* 2.03% — so the win was always on
  paper and only gcc's layout was eating it. Re-measure per compiler.

`PAQ_DOT2` and `PAQ_N16` **do** stack on that hardware. The two overlap in
mechanism — row reuse vs aligned loads — and did not stack under gcc 13.3 here,
so it was worth checking directly: dropping `DOT2` from the winning nine gives
17.425 encode / 17.972 decode, i.e. **+2.3% encode and +5.8% decode worse**.
Keep both. `DOT2` remains the largest instruction-count lever either way
(Ir −16.94% vs `AVX2`'s −13.13%), and the decode side is where it shows most.

Two items stay null on both machines, which is now a much stronger statement
than §3 alone could make: **`PAQ_PF2`** (18.057, worse than 17.691) and
**`PAQ_RCMPF`** (17.821, worse). Both were "next in line" in the original doc's
§3; both are measured dead on two very different hosts.

`PAQ_RESTRICT` at 17.691 vs 17.721 is a 0.17% difference — still consistent with
this container's Ir of −0.00% (4 instructions in 2.9 G), i.e. nothing. It is
free, so the recommendation keeps it, but it is not doing work.

**Practical guidance.** `PREFETCH`, `AVX2`, `DOT2`, `GETSIMD`, `APMPF` are wins
on both hosts — enable unconditionally. `N16`+`WXPF`+`CXTFL_T` are worth
3–4% on real hardware but were neutral-to-negative here, so they are the set to
A/B on your own target, together rather than individually. Never enable `PF2` or
`RCMPF`.

## 8. Second-pass audit: what the first pass missed

Re-reading `paq8hpc_speed.md` against the gate list turned up two items I had
not actually tested, and re-reading the *source* turned up one idea the doc never
raises. All are now implemented and verified byte-identical.

### `PAQ_BPOS_T` — bit-position templating (not in the doc; biggest Ir win yet)

`bpos` is constant for an entire `mix1` call, but the per-context loop re-tests it
up to five times and feeds it to two *variable* shifts — and that loop runs `cn`
times, up to 46 for WordModel's map, 58 contexts per bit across the four owners.
Dispatching on `bpos` once, so the body sees it as a compile-time constant,
collapses the four-way chain to the arm actually taken, turns `>>(8-bpos)` and
`>>(7-bpos)` into constant shifts, and resolves the `bpos==7` reset and both
prefetch guards at compile time. `RunContextMap::p` gets the same treatment
(three calls per bit, two variable shifts each).

**Exact instruction count: −6.43%** on top of the five recommended macros — more
than three times what `PAQ_CXTFL_T` saves (−2.03%), and the largest reduction of
any gate except the SIMD kernels themselves. Wall clock here: −0.6% (7/15)
against a control of −3.2%, i.e. unresolvable, which is what a pure
instruction-count win looks like on a latency-bound host.

**This is the top candidate for the real hardware in §7.** `CXTFL_T` is the
precedent: it also only reduced instructions, measured negative here, and
converted to +2.5% under clang there. `BPOS_T` is the same shape and three times
the size. It costs 8 instantiations of `mix1t` (16 combined with `CXTFL_T`), so
the I-cache tradeoff is the thing to watch.

### A gcc 13.3 `-O3` miscompile, found while building the above

The first version of `PAQ_BPOS_T` cached the value in a local:
`const int bp = (BP<0) ? bpos : BP;`. That **silently changed the archive in the
no-macro build** — book1[0:65536] at L0 went 23173 → 32656 bytes, diverging from
bit 16 on. The two forms are semantically identical, and the evidence says the
compiler is at fault, not the code:

- `-O0`, `-O1`, `-O2` all produce the correct 23173 from both forms.
- `-O3 -fno-unswitch-loops` produces 23173. Eleven other `-O3`-only passes make
  no difference; **loop unswitching alone** accounts for it.
- **clang 18 `-O3` produces 23173 from both forms.**
- `bpos` provably never changes inside the loop — checked with volatile reads at
  loop entry and per iteration, zero hits.
- `-Warray-bounds=2 -Wstringop-overflow=4` report nothing in the model.

The mechanism: reading the global directly leaves it a potential alias of the
`U8*` stores in the loop, so gcc cannot treat it as loop-invariant and cannot
unswitch. Caching it in a local removes that barrier, unswitching fires, and the
unswitched loop is wrong.

The fix is to never introduce the variable — `PAQ_BP` is a macro expanding to a
fresh read of the global when `BP<0`, textually what the baseline does, and to a
literal when `BP>=0` (where the tests fold and there is nothing to unswitch). The
warning is in the source at the macro definition, because this is a live trap for
anyone touching that loop: **any edit that makes `bpos` provably loop-invariant
in `mix1t` can silently change the output under gcc -O3.** Worth knowing that
the shipped configuration is unaffected — no recommended gate caches `bpos`, and
all of them verify byte-identical — but the next person to optimise this loop
will walk straight into it.

### The two doc items I had skipped

- **`PAQ_HOT`** — `__attribute__((hot))` is listed in the doc's §10.4 next to
  `__restrict`, and the first pass tested only `__restrict`. Measured:
  **Ir +0.00%, exactly zero** (it only biases block layout), wall clock inside
  the control band. Null, as the doc expected.
- **`PAQ_IOUNLOCKED`** — the doc's §7 closes `fgetc_unlocked` *by argument*
  ("nothing at the byte pipe is visible") rather than by measurement, so the gate
  exists to check it. glibc's `getc`/`putc` do take the stream lock per byte, and
  this program is single-threaded and owns both handles. Measured:
  **Ir +0.13% — slightly more instructions, not fewer.** The doc's conclusion was
  right, and now it is measured rather than argued.
- **`PAQ_BHMOVE`** — mmax §7's `int`-switch replacement for BH's `memmove`, which
  the doc calls the correct form but assigns to "measured class: noise" without a
  number. Now numbered: **Ir −0.18%**, wall clock inside noise. Noise confirmed.

### Build flags: four candidates eliminated by inspection, no measurement needed

Worth recording because three of them are commonly recommended and all are
no-ops on this toolchain:

| flag | why it is pointless here |
|------|--------------------------|
| `-fomit-frame-pointer` | already enabled (`-Q --help=optimizers` confirms); Ubuntu 24.04's frame-pointer default does not apply to this compiler's `-O3` |
| `-fno-stack-protector` | stack protector already off by default |
| `-D_FORTIFY_SOURCE=0` | `_FORTIFY_SOURCE` is 3 by default, but gcc proves every `memmove`/`memset` size safe — **zero `__*_chk` calls in the generated code**, so there is nothing to switch off |
| `-mprefer-vector-width=256` | only one `zmm` register appears in the whole `-march=native` output, so there is no 512-bit frequency-licensing exposure to avoid |

`-static` remains untried: 126 `memmove@PLT`/`memset@PLT` call sites exist,
though the hot ones (`E::get`'s constant-size `memset`) are already inlined.

## 9. `PAQ_LOGISTIC` — accurate squash/stretch tables ported from xad_logistic.inc

**The only gate here that changes the bitstream.** Off by default, and the
compression case for it did not survive measurement — but the port is exact and
the tables are strictly better, so it is kept and documented.

### What the baseline does wrong

`squash()` is a 33-node piecewise-linear approximation of the logistic on a
128-wide grid, and `stretch()` is built by inverting *that*, so it inherits the
error. Linear interpolation across a 128-wide step of a curve whose second
derivative peaks near `4096*0.0962/256^2` costs about `(128^2/8)*that ≈ 12`
units; measured, the worst case is **13.27 at d=318**, out of 4096.

The ported construction computes stretch exactly —
`stretch(p) = S*ln(p/(N-p)) = S*ln2*(log2 p - log2(N-p))` from an integer
`65536*log2` table built by repeated squaring — then finds squash as its inverse
by binary search plus one linear interpolation, so it rounds the real crossing
rather than snapping to a tabulated stretch. Max deviation from the true
logistic: **13.27 → 0.58, a 22.8x improvement.** No floating point is involved;
the one irrational needed is `ln2` as a Q48 literal, so the tables are a
property of the source rather than of whichever libm the build links — which
matters because encoder and decoder must agree on them exactly.

The tables differ from the baseline's in **3539 of 4096 stretch entries (max 32)
and 3162 of 4095 squash entries (max 13)**, so this is not a rounding tweak.

### Validation (`test_logistic.cpp`, run before touching the model)

The strongest check available: paq8hp's constants (`P_ONE` 4096, `ST_SCALE` 256,
stretch range ±2047, p range 1..4095) coincide with xadpcm's shipped ones, so a
faithful port must reproduce that file's published `XAD_SQSTT_HASH`. It does —
**FNV-1a over (SQT,STT) = `7BCBA716`**, bit for bit, both standalone and inside
paq8hpc, which pins all 8192 entries at once. Also checked: `LOG2` within 1 unit
of `65536*log2(i)`; stretch within 1 of the float reference; both tables
monotone; ranges safe at every use site including `squash(d)*16` fitting the u16
in `APM`'s ctor (max 65520); rebuilds identical.

**One assertion I got wrong, worth recording.** I first asserted that squash is
an exact left inverse of stretch — `squash(stretch(p)) == p`. That is
*mathematically impossible*, not a porting defect: `d(stretch)/dp = S*N/(p(N-p))`
is **0.25 at the midpoint**, so up to 5 distinct `p` share each integer `d` and no
left inverse can exist. The correct statement for a many-to-one map is the
pseudo-inverse pair, and both hold with **zero violations**:

```
stretch(squash(stretch(p))) == stretch(p)      for all p
squash(stretch(squash(d)))  == squash(d)       for all d
```

Round-trip error also improves: `stretch(squash(d))` mean 10.09 → 5.88 (max
127 → 88, the residual being the saturated tail where consecutive `p` are ~95
stretch units apart and no table can do better), and `squash(stretch(p))` mean
0.885 → 0.625, max 3 → 2.

### Compression: consistent, and negligible

Round-trip verified on all 13 cases plus full book1 at L4/L8/L11, and the MinGW
build is cross-OS byte-identical to the Linux one with the gate on.

| input | level | baseline | ported | delta |
|-------|------:|---------:|-------:|------:|
| regression set (7 cases, total) | — | 132,458 | 132,438 | **−0.015%** |
| book1[0:262144] | 6 | 71,701 | 71,694 | −0.010% |
| book1[0:262144] | 9 | 71,699 | 71,694 | −0.007% |
| 256K of ELF | 6 | 38,028 | 38,025 | −0.008% |
| 256K of ELF | 9 | 38,022 | 38,021 | −0.003% |
| 256K of C++ source | 6 | 23,437 | 23,435 | −0.009% |
| 256K of C++ source | 9 | 23,437 | 23,435 | −0.009% |
| **full book1** | 4 | 194,348 | 194,354 | **+0.003%** (+6 B) |
| **full book1** | 8 | 193,109 | 193,107 | −0.001% (−2 B) |
| **full book1** | 11 | 193,108 | 193,108 | 0.000% |

Eight of eleven cases improve, one is exact, and **full book1 at L4 gets 6 bytes
worse**. The effect is about −0.01% where it exists, i.e. single-digit bytes, and
it shrinks toward zero as the input grows.

**Why a 22.8x more accurate logistic buys essentially nothing:** the mixer's
weights are trained against whatever `squash` they are given, and a smooth
monotone distortion of it is almost entirely absorbed by the weight vector — and
then by the six-stage APM chain, which is itself a learned correction of the
final probability. Removing the distortion mostly relocates the weights rather
than improving the prediction. That also explains the size trend: the shorter
the input, the less adaptation has happened, and the more the raw table accuracy
shows — which is exactly the direction the table above runs.

### Speed: neutral, as expected

Ir **−0.13%** (squash becomes a load instead of six arithmetic ops), wall clock
ambiguous — −3.8% median but 5/15 wins against a control of +0.2%/8-of-15, and a
*higher* best-of. The 8 KB `sqt` table replaces a 132-byte one, so it adds L1
pressure that roughly cancels the arithmetic saved. Call it neutral.

### Verdict

Not enabled by default: it changes the bitstream — archives are mutually
undecodable across the gate — and buys ~0.01%. That trade only makes sense for a
new format, not for a drop-in. It is the right construction if the tables are
ever revised for other reasons (a different `ST_SCALE`, or wanting the
libm-independence guarantee), and `test_logistic.cpp` plus the built-in FNV
checksum make it safe to change. `-DPAQ_LOGISTIC_HASH=0` disables the startup
check while deliberately altering the tables.

## 10. History window: 1 GiB, and there is no match model

Asked to make sure the match-model window is 1 GB so enwik9 fits whole. Three
findings, in order of importance.

**1. There is no match model in this build.** The predictor runs
`ContextMap` (order-N, 7 contexts), three `RunContextMap`s, `WordModel`,
`SparseModel` and `RecordModel`. No `MatchModel`, no match hash table, no match
pointer or length. So `Buf` *is* the whole match/history window — there is
nothing else holding history — and if the intent is to exploit enwik9-scale
repetition, that needs a match model added, not a bigger buffer.

**2. The window already holds enwik9, and is now machine-checked.**
`Buf<1u<<PAQ_BUFBITS>` with `PAQ_BUFBITS` defaulting to 30 is
**1,073,741,824 bytes**, clearing enwik9's 10^9 by 73.7 MB (7.4%). It was already
1 GiB; what was missing is that nothing enforced it. A `static_assert` now does:

```
static_assert( (1ull<<PAQ_BUFBITS) >= 1000000000ull, ... );
```

Verified to fire on `-DPAQ_BUFBITS=22` — the 4 MiB shrink one reviewed doc
proposed and labelled bit-exact (§9(g)) — and a second assert bounds
`PAQ_BUFBITS` to [1,31], since `SZ` is a `U32` template argument and 32 would
wrap the size itself to zero. `-DPAQ_ALLOW_SMALL_BUF` is the deliberate escape
hatch.

`test_window.cpp` includes the real header and inspects the actual
`paq8hp::buf`, so it cannot drift from what ships. It checks the size and
headroom; that masking is the identity for **all 10^9 positions** (exhaustive,
not sampled); that the first colliding position is exactly 2^30, i.e. past
enwik9 rather than inside it; and then writes all 10^9 bytes and confirms
`buf(i)` back-references resolve correctly at distances up to 999,999,999 —
including that byte 0 is still live when `pos` sits at the end of enwik9. Also
records the limit ordering: **window 1 GiB < `int pos` 2 GiB < 4-byte `f_len`
header 4 GiB**, so the window is the binding constraint and enwik9 uses 46.6% of
`int pos`.

**3. What the window is actually earning today: ~128 bytes of it.** The only
reads of `buf` in the whole model are `buf(2)`, `buf(3)` in `ContextModel::p` and
`buf[nl1+col]` in `WordModel` — the previous-but-one line at the same column.
Measured by shrinking the window and comparing archives:

| window | book1[0:131072] L6 | vs 1 GiB |
|--------|-------------------:|----------|
| 1 GiB … 128 B | 37,970 | **byte-identical** |
| 64 B | 37,970 | differs |

A **128-byte** window reproduces the 1 GiB output exactly; it only diverges at
64. That matches the structure: the longest (previous line + current line) span
in that input is 137 bytes. So for text, the current model set uses ~10^2 bytes
of a 2^30-byte window — the 1 GiB is ~8 million times more than any present
model reads.

There is one case where the size does matter today. With **no newlines** in the
input, `nl1` never leaves its −2 initial value, so `buf[nl1+col]` indexes the top
of the buffer, whose value depends on whether the input has wrapped around to
overwrite it. Measured on a 128 KB newline-free file: a 1 MiB window matches
1 GiB, a 64 KiB one does not. A 1 GiB window keeps that context a deterministic
zero for any input up to 2^30 — enwik9 included.

**Net:** the window requirement is satisfied and now enforced, but it is
insurance for a match model that does not exist yet rather than something the
current models are using.

## 11. Named scales (pure refactor, byte-identical)

The probability and logistic scales were spelled as literals throughout. They
are now `constexpr` constants (not enums — every remaining `enum` constant-holder
in the model and driver was converted too, including the template-scope ones in
`Mixer`, `BH` and `ContextMap`):

```
P_BITS 12   P_SCALE 4096   P_MAX 4095   P_MIN 1   P_HALF 2048
ST_SCALE 256   ST_MAX 2047   ST_MIN -2047
SQ_BITS 7   SQ_STEP 128   SQ_MASK 127   SQ_NODES 33   SQ_MID 16   APM_NODES 33
PR_BITS 16   PR_ONE 65536   PR_MAX 65535   PR_SHIFT 4
N_STATES 256   ILOG_SIZE 65536   Q8_BITS/Q8_ONE/Q8_HALF (logistic builder)
```

Each is derived from its parent rather than restated, so `P_SCALE` is the single
knob: `P_MAX = P_SCALE-1`, `P_HALF = P_SCALE/2`, `ST_MAX = P_HALF-1`,
`SQ_NODES = 2*P_HALF/SQ_STEP+1`, `PR_SHIFT = PR_BITS-P_BITS`. Derived shifts that
were opaque now read as arithmetic: APM's `>>11` is
`>>(PR_BITS+SQ_BITS-P_BITS)`, and its `*16` is `<<PR_SHIFT`.

**The reason this needed care rather than sed: the same numeral means different
things in different places.** Renaming by numeral would have been a silent
stream change. The ones deliberately left as literals, with the reason:

| site | literal | why it is *not* the probability scale |
|------|--------:|----------------------------------------|
| `malloc1`'s alignment guard | 4096 | a **page size**; now `PAQ_PAGE_SIZE`, with a comment saying so |
| `ContextModel::p`'s last `m.set` | 2048 | a mixer **context count**, not a probability |
| `hash(29, failz&2047)` | 2047 | a mask on the `failz` bit history |
| `llog`'s tier offsets | 256, 128 | tier bases, unrelated to `ST_SCALE`/`SQ_STEP` |
| `StateMap`'s `n0/n1 *= 128` | 128 | a determinism boost, a tuning constant |
| partial-byte arithmetic | 256, 8 | the **byte alphabet**, left alone on purpose |
| `Ilog::t` | 65536 | the **U16 domain**; named `ILOG_SIZE`, kept distinct from `PR_ONE` even though numerically equal |

The rangecoder also stops carrying a model constant: `rc_Decide` takes `totFreq`
as a parameter instead of hardcoding 4096, and the driver passes
`PSCALE = paq8hp::P_SCALE`. The doc's §8 concern about the divide folding is
checked directly rather than assumed — **the DECFAST build contains zero
`divq`/`idivq`** (the non-DECFAST build has 12), so constant propagation through
inlining keeps the shift and `PAQ_DECFAST` still removes the last division from
the decode path.

Verified byte-identical on the full regression set for the no-defines build, the
recommended five, and an all-fourteen-gates build; `PAQ_LOGISTIC` reproduces its
own pre-rename numbers exactly (−0.015% on the set); both standalone tests pass;
and the MinGW build still produces the published vector (20321 B, md5
`5b20ec75…`).

## 12. `P_SCALE` is a real parameter: P_BITS 8..16 all work

The first pass at this concluded "structurally yes, practically no" and wrote off
a dozen constants as un-derivable tuning. That was wrong, and the correction came
from the right observation: **probability estimation here is fixed-point, so those
constants are not opaque weights — each is a coefficient fitted against a value
carrying a specific number of fractional bits.** Reinterpret the scale and they
follow `P_BITS` after all.

`-DPAQ_P_BITS=n` (with `-DPAQ_LOGISTIC`, which computes the tables instead of
using the literal ones) now works across the whole legal range:

| P_BITS | size | v1 as-found | v2 reinterpreted | v3 + decoupled |
|-------:|-----:|------------:|-----------------:|---------------:|
| 8  | 38,322 | — | — | **+0.94%** |
| 9  | 38,068 | +95.3% | +5.2% | **+0.27%** |
| 10 | 38,004 | +131.3% | +3.0% | **+0.11%** |
| 11 | 37,977 | +220.3% | +0.79% | **+0.034%** |
| **12** | **37,964** | 0% | 0% | **0%** |
| 13 | 37,965 | +29.5% | +0.09% | **+0.003%** |
| 14 | 37,963 | +106.5% | +2.65% | **−0.003%** |
| 15 | 37,964 | +16.9% | +17.8% | **+0.000%** |
| 16 | 37,966 | +78.1% | +52.7% | **+0.005%** |

(book1[0:131072] L6, every setting round-tripping.) P_BITS 11–16 land within
0.04% of the 12-bit reference; 14 is marginally *better*. Three changes got it
there.

### 1. Reinterpret the P-domain constants (the big one)

Rather than rescale every literal, bring the **value** to the scale its
coefficient was fitted at and leave the coefficient alone:

```c++
constexpr int P_REF_BITS = 12;                 // what every coefficient was fitted at
inline    int p_to_ref  (int v);               // value  -> reference scale
constexpr int p_from_ref(int v);               // constant -> current scale (exact)
inline    int p_scaled  (int v,int num,int den);  // v*num/den at the reference scale
```

All three are the identity at `P_BITS == 12`, so the shipped output is unchanged.
`p_scaled` folds the rescaling into the division the expression already had, so
precision is lost once instead of twice — which matters because `train()`'s
`_mm256_mulhi_epi16` hardcodes a `>>16`, so the mixer error must arrive
pre-scaled and every dropped bit is lost learning signal. Sites fixed:
`Mixer::update`/`update2`'s `(y<<12)-pr` error, `mix2t`'s three P-domain mixer
inputs, `SmallStationaryContextMap`'s `*cp>>4` (a PR→P conversion, so `>>PR_SHIFT`)
and `(y<<16)`, and `Predictor`'s `pr>=1820` / `pr>=848` fail thresholds (via
`p_from_ref`, which scales the *threshold* and so stays exact).

### 2. Decouple the logistic domain from `P_HALF`

`ST_MAX = P_HALF-1` tied the logistic range to the probability precision. It is a
property of `ST_SCALE`: `stretch` reaches `±ST_SCALE*ln(P_MAX)` ≈ `±8*ST_SCALE`.
Sizing it from `P_HALF` broke both ends of the sweep, and the measurements show
exactly that — the loss tracks a geometric quantity, not a tuning error:

| P_BITS | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| % of p range the clamp ate | 53.9 | 23.9 | 3.6 | 0.07 | 0 | 0 | 0 | 0 |
| APM nodes reachable of 33 | 33 | 33 | 33 | 33 | 19 | 10.7 | 6.2 | 3.8 |
| loss at stage v2 | +5.2% | +3.0% | +0.8% | 0 | +0.09% | +2.6% | +17.8% | +52.7% |

Below 12 the clamp destroys resolution; above 12 the APM's 33-node grid spreads
over `±P_HALF` while `stretch` still only reaches `±2839`, so at P_BITS=16 fewer
than 4 nodes are ever visited. Now:

```c++
constexpr int ST_BITS  = ilog2c(ST_SCALE);   // 8
constexpr int ST_LIMIT = 1<<(ST_BITS+3);     // 2048 = +-8 nats, independent of P_BITS
constexpr int ST_MAX   = ST_LIMIT-1;
constexpr int SQ_BITS  = ST_BITS-1;          // 7 -- the grid belongs to the logistic domain
```

`sqt` is indexed `d+ST_LIMIT` over `2*ST_LIMIT` entries and APM indexes
`(pr+ST_LIMIT)>>SQ_BITS`. At `ST_SCALE=256` every one of these is the shipped
value, so output is unchanged. This also supersedes the earlier `SQ_BITS = P_BITS-5`
fix — deriving it was right, but from the wrong parameter: the squash grid is a
property of the logistic domain, not the probability domain.

### 3. Two type/precision bugs found on the way

- **`sqt` must be unsigned.** It holds probabilities in `[0,P_MAX]`, and at
  `P_BITS=16` `short(65535)` is `-1`. Now `U16`, with a `static_assert` that the
  element type can hold `P_MAX`.
- **The `PAQ_LOGISTIC` port had UB**: `(long long)d<<Q8_BITS` left-shifts a
  negative value for half its range, undefined before C++20. xad_logistic.inc
  relies on being built as C++20 and says so; this file sets no `-std` and gcc 13
  defaults to gnu++17. Found with `-fsanitize=undefined`, fixed to `d*Q8_ONE`.
  The non-`PAQ_LOGISTIC` path was and stays UBSan-clean.

And the table checksum earned its keep: restructuring the FNV loop into two passes
changed the hash, the guard fired at startup and aborted rather than silently
shipping different tables. Restored to the interleaved order, bounded by the
shorter table so it cannot read out of range once the two sizes differ.

### What is still pinned

`P_BITS == 12` remains **required without `-DPAQ_LOGISTIC`**, because the baseline
`squash()` carries 33 literal 12-bit probabilities — asserted at compile time. And
`PR_BITS = 16` caps `P_BITS`, since `StateMap`/`APM` store at `PR_BITS` and shift
down by `PR_SHIFT`. Within those, `P_SCALE` is now a genuine parameter rather than
a number that happens to appear in 40 places.

## 13. Uniform linear mixing

Every fixed-weight weighted average of probabilities is now written the same way:
weights in `P_SCALE` units summing to `P_SCALE`, rounding `+P_HALF`.

```c++
inline int pmix(int w0,int v0, int w1,int v1);                          // 2 terms
inline int pmix(int w0,int v0, int w1,int v1, int w2,int v2, int w3,int v3);

const int APM1_w    = P_SCALE/8;      // 512   a1's correction, 1/8 : 7/8
const int MIXA_pt_w =  6*P_SCALE/32;  // 768   final blend when fails&255
const int MIXA_pu_w =  1*P_SCALE/32;  // 128
const int MIXA_pv_w = 11*P_SCALE/32;  // 1408
const int MIXA_pz_w = 14*P_SCALE/32;  // 1792
const int MIXB_pt_w =  4*P_SCALE/32;  // 512   and when it is clear
const int MIXB_pu_w =  5*P_SCALE/32;  // 640
const int MIXB_pv_w = 12*P_SCALE/32;  // 1536
const int MIXB_pz_w = 11*P_SCALE/32;  // 1408
```

Plain `const`, not `constexpr`, so a weight can be retuned here without the value
being baked into unrelated constant expressions; `static_assert`s check that each
set sums to `P_SCALE`. So

```c++
pu = (a1.p(pr,k1,3) + 7*pr + 4)>>3;          // was
pu = pmix(APM1_w, a1.p(pr,k1,3), P_SCALE-APM1_w, pr);   // now
```

**Exactness** is not merely argued: each old numerator is the new one divided by
`P_SCALE/2^k`, and a standalone check sweeps all four inputs densely across the
whole probability range and reports **0 mismatches** against the original shift
forms. Byte-identity holds for the no-defines, recommended, and all-gates builds.
`long long` accumulation because one term reaches `P_MAX*P_SCALE = 2^32` at
`P_BITS=16`; these run once per coded bit against ~5500 madds, so the width is
free.

### What the audit found, and what was deliberately left alone

Searching for sum-of-products-then-shift across the whole model turned up exactly
**three** fixed-weight probability mixes — the two-term `pu` blend (in both the
`PAQ_APMPF` and baseline branches) and the two four-term final blends. Everything
else falls into three categories that are *not* fixed-weight mixing:

- **Dynamic-weight interpolation** — `squash()`'s and `APM::p`'s
  `(t[i]*(SQ_STEP-w) + t[i+1]*w)`. The weight is data, not a constant, so there is
  no constant to name.
- **Exponential moving averages** — `v += (target-v)>>rate`, in `APM::p`,
  `StateMap::p` and `SmallStationaryContextMap::mix`. These *are* linear mixes,
  with weight `2^-rate`. Where the rate is constant (SSCM) the rate is now named
  (`SCM_RATE_EARLY/LATE`, `SCM_SWITCH_POS`) and the rounding term is derived from
  it rather than restated, but they stay in shift form: SSCM's `mix` runs ten
  times per bit, and a `P_SCALE`-weighted form in the PR domain needs 64-bit
  arithmetic to hold `PR_MAX*PR_ONE`. The rest have a runtime rate.
- **Single-value scalings** — `st/4`, `st*9/32`, `(dp*9)>>9`, `z>>9`,
  `(z*15)>>13`, `stretch(...)*mulc/32`, `b*c`. One operand, so nothing to
  weight against.

**The logistic domain contains no fixed-weight linear mixing at all** — every
logistic-domain expression in the model is a scaling of a single stretch value.
The mixing there is the `Mixer` dot product itself, whose weights are learned
rather than constant.

Because the weights are `P_SCALE`-derived they also follow `PAQ_P_BITS`:
re-checked at P_BITS 10/12/14/16, still +0.105% / 0 / −0.003% / +0.005%.

## 14. Adaptation rates, given the same treatment

Every adaptive counter in the model updates as `v += (target-v)>>rate`, which is
a linear mix of the stored value and the target with weight `2^-rate`. So the
rate *is* the weight, in log form — the same uniformity §13 gave the fixed-weight
mixes, except a shift is the natural spelling when the weight is a power of two
and the code is in the hot path. One shape now covers all three users:

```c++
inline int ema( int v, int target, int rate ) { return v + ((target-v)>>rate); }
```

Each caller folds its own rounding into `target`, which is what lets them share
it — and makes an asymmetry explicit that was previously buried: **`StateMap` and
`APM` round only the `y==1` target** (`sm_target(rate) = PR_MAX + (2^rate - 1)`,
and APM's `g = (y<<PR_BITS)+(y<<rate)-y*2`), taking a plain `0` for `y==0`, while
`SmallStationaryContextMap` rounds symmetrically. That is the baseline's
behaviour, now visible rather than implied.

The rates and their schedules, plain `const` like the mixing weights, each
schedule named next to the rates it switches between:

| | |
|---|---|
| `SM_RATE_0/1/2` = 7/8/9 | `SM_SWITCH_1` = 512K, `SM_SWITCH_2` = 1M |
| `APM_RATE_BASE` = 6, `APM1_RATE` = 3, `APM2_RATE_ADD` = 1 | `APM_SWITCH_1` = 3,670,016, `APM_SWITCH_2` = 14,680,064 |
| `SCM_RATE_EARLY/LATE` = 9/10 | `SCM_SWITCH_POS` = 4,000,000 |
| `MIX_LR_NUM/DEN` = 7/1, `MIX2_LR_NUM/DEN` = 3/2 | `MIX_W_INIT` = 512, `MIX2_W_INIT` = 0x7fff |
| `CM_DECAY_FROM/BIAS/SHIFT/STEP` = 204/452/3/4 | |
| `RCM_RUN_MAX` = 255, `CM_RUN_MAX` = 254, `CM_RUN_STEP` = 2 | `SM_PRIOR_ADD` = 1 |

Also named on the way: `PR_HALF` (SSCM's initial 32768) and the two mixer weight
initialisers, which were template arguments spelled as bare numbers.

**Checked, not assumed.** `ema()` is swept against all four original update forms
across the PR range at every rate 3..10 — **0 mismatches** — and the three shipped
configurations plus `PAQ_LOGISTIC` are byte-identical. Instruction count went
*down* 0.36% (`U16(ema(...))` generates marginally better code than the `+=`
form), so the uniformity is free. `PAQ_P_BITS` still works: 10/12/16 →
+0.105% / 0 / +0.005%.

One method note: an assertion in the rewrite script caught me miscounting a
call site — `p_scaled(...pr[i],7,1)` occurs three times, not two (train2, the
odd-tail train, and the non-DOT2 path). The script writes only after every
anchor matches, so the file was left untouched; the `verify.sh` run that followed
was therefore testing the *old* binary and passed vacuously. Worth stating because
it is exactly the failure mode a green check can hide.

## 15. The IDX port

`IDX-FORMAT.md` describes a small language for declaring the two things a
context-mixing compressor is made of — context indices and tunable parameters —
and `IDX/idx2inc.pl` turns a `.idx` declaration plus a `.inc` template into C++.
Sections 11–14 above had already collected every choice-shaped number in the
model behind a name; this section moves those names into IDX, which is what
turns "named" into "reachable by an optimizer".

### What went in, and what deliberately did not

`IDX/paq8-G0.idx` declares **62 `Number` parameters** and **4 `Index`
declarations** (46 in the first pass; section 16 replaced the six shared APM
parameters with 22 per-instance ones):

| group | parameters |
|---|---|
| probability-domain mixing weights | `A1_w`, `MIXA_pt/pu/pv`, `MIXB_pt/pu/pv` |
| StateMap | 3 rates, 2 switch positions, the Laplace prior |
| APM (×6 instances) | per-instance rate, 2 switch positions, node count — see section 16 |
| SSCM | 2 rates, 1 switch position |
| Mixer | 2 learning-rate pairs, 2 weight initialisers, 5 logistic output scalers, 4 input scalers |
| ContextMap | 4 decay parameters, 3 run-length caps |
| Predictor | `FAIL_TH`, `FAILZ_TH` |

The line that is worth defending is what stayed **out**: `P_BITS`, `P_SCALE`,
`PR_BITS`, `ST_SCALE` and everything derived from them. Those are the
arithmetic's *resolution* — fixed by the rangecoder's `totFreq` and by the width
the counters are stored at — not a tuning choice, and section 12 already carries
the static_asserts that say what they may be. Handing them to `opt.pl` would be
handing it the format rather than the model. IDX holds what an optimizer is
allowed to move; that is a different set, and conflating the two is how a sweep
ends up producing archives the decoder cannot read.

### Three things the port had to solve

**The four-weight blends could not keep their `static_assert`.** `pmix` requires
its weights to sum to `P_SCALE`, and the previous code asserted exactly that. In
the tuning build the weights are runtime reads, so the assertion cannot even be
written — and in *any* build `opt.pl` moving one weight breaks the sum. So only
three weights of each blend are declared and the fourth is the remainder:

```c++
const int MIXA_pv_w = PWGT(G0_MIXA_pv);
const int MIXA_pz_w = P_SCALE - MIXA_pt_w - MIXA_pu_w - MIXA_pv_w;
```

The invariant is now structural rather than asserted, and it reproduces the
shipped 14/32 and 11/32 exactly. Weights are declared in units of `P_SCALE/256`,
which keeps the arithmetic exact at any `P_BITS >= 8` and gives the optimizer
1/256 resolution instead of the 1/32 the original shifts had. (That is why the
`P_BITS` lower bound in section 12 tightened from 1 to 8 — the measured survey
only ever ran 8..16 anyway.)

**Two parameters are template arguments.** `Mixer<n,m,s,w>` takes its initial
weight as a non-type parameter, and in the tuning build `G0_MIX_W_INIT` is a read
from a patchable object, not a constant expression. Section 8's `IDXP` trick — a
non-type parameter of *reference* type — is what makes `Mixer<456, MIXER_ROWS, 6,
MIX_W_INIT>` well-formed and identically spelled in both builds. The `s = 1, w =
0` defaults went with it (a reference parameter cannot take one); both
instantiations always passed all four arguments anyway.

**`Mixer::wx` is a fixed-size member**, so its row count has to be a literal in
both builds and cannot be the sum of the IDX `_Volume`s. It stays written out as
`MIXER_ROWS`, guarded from both sides: a `static_assert` that the six context
ranges sum to it where the Volumes fold, and a bounds check in `Mixer::set` where
they do not. That is also why all six index mask lines are **frozen** (`!`,
section 10) — a mask in `.idx` is a bit *selector*, so widening one multiplies
the Volume, and there is no room in `wx` for that. The structure is declared and
reviewable; the search space stays on the parameters.

### The mixer context selectors

Four of the six `m.set()` calls in `ContextModel::p` are exact packings of a few
small variables, so they are declarations rather than arithmetic:

```
Index MX0                          # was: 256*order + (w4&240) + (c2>>4)
 ADD 7: order
!mx0_w4:  w4,  &11110000
!mx0_c2:  c2,  &11110000
```

The other two are not, and were left as code: one is a two-branch expression over
`bpos`, the other adds two fields that can carry into each other. An `Index` for
either would misdescribe what it computes, which is worse than leaving it
spelled out. The four that did port now get their `range` argument from
`G0_MX*_Volume` instead of from a literal `256*7` sitting next to an expression
that has to agree with it by inspection.

### `sh_mapping.inc` was missing the freeze macros

Exactly as `IDX-FORMAT.md` section 10 documents: `pdesc`/`pmask`/`pmask2` were
there, `mdesc`/`mmask`/`mmask2` were not, so a `!` line emitted a call to an
undeclared macro. Added, with the two properties every one of them needs — two
NUL-separated strings, because the constructors do `S += strlen(S)+1` to reach
the pattern, and the pattern as a separate literal after the `"\x00"`, because
`\x` escapes are greedy in hex digits.

### Verification

`IDX-FORMAT.md` section 12 states the contract: *"the two builds must agree, and
that is testable."* Here it is testable as `verify.sh`, which already checks
byte-identity against the pristine baseline. Every seeded pattern spells the
value the code shipped with, so both builds must reproduce the baseline archives
exactly — and do:

| build | `verify.sh` |
|---|---|
| `./mk.sh` (tuning: `Debug 1, Const 0`, `USE_NEW 1`) | ok — 7 cases × {encode, round-trip, cross-decode} + 2 doc vectors |
| `./mk.sh release` (`Const 1`, `USE_NEW 0`) | ok — same |
| tuning + the 14 speed gates | ok — same |
| release + the 14 speed gates | ok — same |
| MinGW release under wine | ok — 5 cases: win==linux bytes, win round-trip, win←linux, linux←win |

The marker counts confirm the freeze worked: the tuning binary carries one
`!MAP!` marker per `Number` (**46** at this point, 62 after section 16) and **6**
`map.` markers
(the frozen index masks, invisible to `opt.pl`); the release binary carries
**zero** of either.

And the loop itself runs. `IDX/opt.pl`, retargeted at `paq8hpc` (its `$level` is
a memory level, not a tuning parameter — a run at one level does not transfer to
another), finds all 46 maps and starts hill-climbing:

```
46 maps, 260 bits total
=== start 26261
!!! 0543D7: G0_APM1_RATE_ !!!    ->  26257
!!! 054228: G0_APM1_w_ !!!       ->  26232
```

`import.pl` then folds `export.!!!` back onto the source, and the diff is
**exactly the two lines opt.pl moved** — alignment, comments and the frozen mask
lines all preserved. Those two values are not committed: a 2-file, 80 KB corpus
is precisely the overfit `opt.pl`'s own header warns about, and this model has
separate word/sparse/record submodels that such a corpus never exercises.

### Files

| file | what it is |
|---|---|
| `IDX/paq8-G0.idx` | the declaration — 46 parameters, 4 indices |
| `IDX/paq8-G0.inc` | the template; no `MakeTables`, so the declarations stay in `_h.inc` |
| `IDX/idx2inc.pl`, `opt.pl`, `import.pl` | generator, optimizer, fold-back (from the 028 tree; `opt.pl` retargeted) |
| `sh_mapping.inc` | the IDX runtime, plus the three freeze macros |
| `MOD/paq8-G0_h.inc`, `_p.inc` | **generated, and committed** — a build input, not an artefact, so the tree builds without perl (§12) |
| `mk.sh` | the two builds, from one source, by one `sed` |

## 16. One instance, one set of parameters

Two follow-ups to the port, both about the same thing: a parameter shared by
things that are not the same thing is a parameter the optimizer cannot actually
move.

### The six APMs had one rate schedule between them

`Predictor::update` computed `rate = APM_RATE_BASE + (pos>APM_SWITCH_1) +
(pos>APM_SWITCH_2)` once per bit and handed it to five of the six APMs, with a2
adding a constant on top. Written that way it looks like a property of the
model. It is not — the six sit at different points of the correction chain and
index on completely different contexts:

| APM | contexts | corrects | old rate |
|---|---|---|---|
| a1 | 256 | the raw model output | fixed 3 |
| a2 | 32768 | the raw model output | shared + 1 |
| a3 | 32768 | the raw model output | shared |
| a4 | 131072 | a1's output | shared |
| a5 | 65536 | a2's output | shared |
| a6 | 65536 | a4's output | shared |

A 256-row table and a 131072-row table do not want the same adaptation rate, and
they certainly do not want the same *switch positions* — the whole point of the
schedule is "slow down once the counts have settled", and a 131072-row table
settles 512× later than a 256-row one. So each now owns `A<i>_RATE`, `A<i>_SW1`,
`A<i>_SW2`. Seeds reproduce the shared arithmetic exactly: `A2_RATE` is 7
because a2's rate was base+1, the other four are 6, and every switch position is
the value the shared pair held.

a1 keeps a single fixed rate, because it never had a schedule. Cloning a knob is
not the same as inventing one, and giving a1 a schedule seeded to never fire
would be a behaviour change dressed up as a rename. `APM1_w` — how much of a1's
output survives the blend with the raw prediction — is renamed `A1_w`, since it
belongs to a1 as much as its rate does.

`APM_RATE_DEF` is gone rather than cloned. It was `APM::p`'s default argument and
every one of the six call sites passed a rate explicitly, so it was a knob
nothing read: pure noise in `opt.pl`'s search space.

The mixers turned out to be clean already — `m.update()` is only ever called on
the main mixer and `update2()` only on the submixer, so `MIX_LR_*` and
`MIX2_LR_*` were genuinely disjoint. Three names were lying, though:
`MIX_Z_SHIFT`, `MIX_ZB_MUL` and `MIX_ZB_SHIFT` execute *only* in the `S==1`
branch, which is the submixer's `p()`. Renamed `MIX2_*`.

### The APM node count is now a parameter (§2 of the request)

`APM` reused squash's grid outright — 33 nodes at `SQ_STEP` spacing, addressed
with a shift pair — which welded the shape of an *adaptive table* to the shape of
a *fixed function approximation*. They are unrelated questions: squash's grid is
fixed by how well 33 points approximate a logistic, APM's by how finely a
correction table should partition its input, and only the second is a tuning
question.

Following `d5c_mix.inc`'s `AP_N_*`, APM now carries its own count and addresses
the range as a fixed-point fraction of it, independent of the grid:

```c++
const int sn = (pr+ST_LIMIT)*NIV;          // was: w = pr & SQ_MASK
const int w  = sn&(ST_RANGE-1);            //      index = (pr+ST_LIMIT)>>SQ_BITS
index = (sn>>ST_RANGE_BITS)+cxt*CELLS;
return (t[index]*(ST_RANGE-w)+t[index+1]*w)>>(PR_BITS+ST_RANGE_BITS-P_BITS);
```

**Declared as the half-count, not the node count.** `A<i>_NH` gives `2*NH`
intervals and `2*NH+1` nodes, so the grid is symmetric and there is always a node
*exactly* at `p == P_HALF`. A free node count would let the optimizer pick an
even one, which puts the midpoint — by far the most visited cell — on an
interpolation boundary. 33 is `NH == 16`.

**The tuning build cannot size a member array from an IDX value.** Same wall as
`Mixer::wx`, different answer: rather than freezing the parameter, the tuning
build reserves the widest grid the clamp admits (`APM_CELLS_MAX = 65`) and treats
the live count as the row's *content* rather than its *extent*, while the
shipping build sizes it exactly. Rows stay disjoint either way, so the two builds
differ in layout only:

| build | cells/context | all six APMs |
|---|---|---|
| shipping | 33 | 21,643,800 B |
| tuning | 65 | 42,631,728 B |

That +21 MB is a tuning-build cost only, and it buys a parameter that would
otherwise have had to be frozen.

### Checked, not assumed

`test_apm.cpp` checks the byte-identity claim directly rather than inferring it
from `verify.sh` passing — the two forms must be **exactly** equal at `NIV==32`,
not approximately, not for typical inputs:

- the interval index agrees for **all 4095** stretch values
- the new fraction is exactly 32× the old one, which is what makes the wider
  shift cancel
- the interpolated result agrees over **14,743,040** `(t0,t1,w)` triples and
  626,535 end-to-end evaluations
- the midpoint lands on a node with zero weight for **every** `NH` in
  `[1,32]` — and an odd interval count demonstrably does not, which is the
  reason for halving
- the interval stays in `[0,NIV-1]` for all **131,040** `(NH, stretch)` pairs,
  so the `index+1` read is in bounds by construction at any node count
- no `int` overflow at `APM_NH_MAX`: max `s*NIV` = 262,080, max weighted sum =
  536,862,720

All 13 checks pass. `verify.sh` then confirms byte-identity end to end:

| build | `verify.sh` |
|---|---|
| release (`Const 1`, `USE_NEW 0`) | ok |
| tuning (`Debug 1, Const 0`, `USE_NEW 1`) | ok |
| release + 14 speed gates | ok |
| tuning + 14 speed gates | ok |
| MinGW release under wine | ok — 5 cases, all four cross-OS checks |

`PAQ_LOGISTIC` still builds at `P_BITS` 8/10/12/16. The tuning binary now
carries **62** `!MAP!` markers, with all six APMs independently visible, and
`import.pl` round-trips the new base-1 `NH` lines — pattern replaced, base and
alignment intact.

### What still shares, and why it was left alone

Stating the boundary rather than leaving it silent:

- **`SM_RATE_*` / `SM_SWITCH_*`** are shared by every `StateMap` — hundreds of
  them, inside every `ContextMap`. They are driven by the globals `sm_shft` /
  `sm_add`, updated once per *byte* rather than per instance. That is a real
  structure, not an accident of how the code was written, and unpicking it means
  a per-instance rate read on the hottest path in the program.
- **`SCM_RATE_*` / `SCM_SWITCH_POS`** are shared by the ten
  `SmallStationaryContextMap`s. These *are* cloneable the same way the APMs
  were — ten instances, a template already carrying a per-instance `mulc`. Not
  done here because the request named mixers and APMs, and quietly widening it
  is the kind of scope drift that makes a diff hard to review.
- **`MIX_MUL_DEN`** is shared by the main mixer's three input groups, but that
  is one mixer, not three instances, and a per-group denominator is redundant
  with widening the per-group numerator (5/4 and 10/8 are the same number).

## 17. Reproducing

```
./build.sh FINAL -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF
./verify.sh /tmp/.../p_FINAL FINAL          # byte-identity, 7 cases x 3 checks
./ab.sh   vfy/b128k 8 15 p_FINAL p_CANDIDATE  # wall clock, in-block control
./icount.sh vfy/b16k 4 c_base c_CANDIDATE     # exact instruction counts
./verify_win.sh w_final.exe win_final         # cross-OS, needs mingw + wine
./verify_rt.sh  p_LOG LOGISTIC                # round-trip + size, for PAQ_LOGISTIC
g++ -O2 -o t test_logistic.cpp -lm && ./t     # squash/stretch table checks
g++ -O2 -o w test_window.cpp && ./w           # 1 GiB window holds enwik9
g++ -O2 -o a test_apm.cpp && ./a              # APM grid: old shift pair == new form
```

The IDX builds, and the tuning loop:

```
./mk.sh                                       # tuning:  Debug 1, Const 0, USE_NEW 1
./mk.sh release                               # shipping: Const 1, USE_NEW 0
./verify.sh ./paq8hpc_idx idx                 # must pass under BOTH -- that is the contract
OUT=./p OPT="-O3 -march=native -DPAQ_AVX2" ./mk.sh release   # OUT/OPT/CXX are overridable

printf 'file1\nfile2\n' > opt.lst
./mk.sh && perl IDX/opt.pl opt.lst             # hill-climb; writes export.!!!
cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done
```

`MOD/` is committed, generated in release mode, so the tree builds with a plain
`g++ paq8hpc_speed.cpp` and no perl. It is a build *input*: a stale `MOD/`
compiles fine and codes differently, which is why `mk.sh` regenerates it
whenever perl is present.

Cross-OS prerequisites, both installed from the distro:
`apt-get install g++-mingw-w64-x86-64 wine64` (the wine binary lands at
`/usr/lib/wine/wine64`, with no `wine` wrapper on the PATH).
