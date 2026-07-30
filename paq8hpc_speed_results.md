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

## 9. Reproducing

```
./build.sh FINAL -DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_DOT2 -DPAQ_GETSIMD -DPAQ_APMPF
./verify.sh /tmp/.../p_FINAL FINAL          # byte-identity, 7 cases x 3 checks
./ab.sh   vfy/b128k 8 15 p_FINAL p_CANDIDATE  # wall clock, in-block control
./icount.sh vfy/b16k 4 c_base c_CANDIDATE     # exact instruction counts
./verify_win.sh w_final.exe win_final         # cross-OS, needs mingw + wine
```

Cross-OS prerequisites, both installed from the distro:
`apt-get install g++-mingw-w64-x86-64 wine64` (the wine binary lands at
`/usr/lib/wine/wine64`, with no `wine` wrapper on the PATH).
