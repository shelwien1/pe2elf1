# paq8hpc speed optimization

Single-thread only: SIMD is in scope up to **AVX2** (nothing wider — see §4 for
the measured reason), multithreading is not. The build must work with **MinGW
gcc on Windows** as well as Linux gcc/clang; every OS-specific mechanism is
gated per-OS in the reference source. Working assumption, confirmed by
instrumentation: **~99 % of time is in probability estimation and model
update** — the rangecoder and I/O are not worth touching (§8 quarantines them
with reasons).

Everything in sections 2–6 is **output-preserving** — verified byte-identical to
the uploaded `paq8hpc_.cpp` on the standing regression set (book1[0:65536] L4 =
20321 B, md5 `5b20ec75933c5f38b209d9948e432ed8`; book1[0:131072] L6 = 37970;
300 B / L0; an L8 reference; decode round-trips and cross-decode of baseline
archives). Additionally verified cross-OS: the MinGW builds (run under wine)
produce archives byte-identical to the Linux builds, both directions, including
a Windows all-flags build decoding a Windows no-flags archive.

Reference implementation: `paq8hpc_speed.cpp` — the uploaded file plus a few
hundred macro-gated lines; a build with no defines is the exact baseline:

```
-DPAQ_PREFETCH    ContextMap + APM line prefetch                     (§3)
-DPAQ_AVX2        256-bit dot_product/train                          (§4)
-DPAQ_HUGEPAGES   Linux: 2M-aligned mmap + MADV_HUGEPAGE;
                  Windows: VirtualAlloc MEM_LARGE_PAGES w/ fallback  (§5)
-DPAQ_LAZY_ZERO   skip ctor memsets over OS-zeroed storage           (§6)
-DPAQ_WXPF        mixer weight-row prefetch (measured null here, §7)
```

## 0. Headline

Interleaved A/B, best-of-5, gcc 13.3 `-O3 -march=native`, 1-core container
(Xeon 2.8 GHz, 32K/1M/33M caches), book1 prefixes. Baseline = uploaded source,
final = `-DPAQ_PREFETCH -DPAQ_AVX2 -DPAQ_HUGEPAGES -DPAQ_LAZY_ZERO`.
Pairwise win rate 10/10 at L4 and L8, 3/3 at L9.

| case            | model  | baseline KB/s | final KB/s | speedup |
|-----------------|--------|--------------:|-----------:|---------|
| 64 KB, level 4  | 67 MB  | 28.3          | 39.7       | 1.40x   |
| 128 KB, level 8 | 592 MB | 22.9          | 36.0       | 1.57x   |
| 128 KB, level 9 | 1.15 GB| 20.5          | 31.5       | 1.54x   |

Attribution (each measured in isolation, interleaved): prefetch +32–38 %,
AVX2 kernels +4–8 %, hugepages unprovable in this container (§5), lazy-zero /
APM-prefetch / mixer-row-prefetch within noise individually, PGO +0–2 %,
AVX-512 ±0 over AVX2 before its removal.

## 1. Measurement protocol

Same rules as the ddsdet work, they all bit here again:

- Interleave A/B/A/B and report pairwise win rate or best-of-N; this container
  drifts (same binary spanned 17.3–22.9 KB/s inside one 5-run block, and the
  §0 absolute numbers moved ~10 % between sessions while every A/B verdict
  reproduced).
- **The first run after a build is never comparable** — cold binary/page cache
  gave 10.8 KB/s where warm interleaved runs gave 20.5.
- Wall clock cannot resolve <2 %; the "within noise" verdicts below mean exactly
  that, not "zero".
- No callgrind/perf in this container; the coarse breakdown below is gprof
  (`-pg -O2`), which inflates small hot functions via mcount — use it for shape,
  not percentages.

## 2. Where the time goes

Instrumented counts (debug build, book1 64 KB, level 4; levels >= 4 all have the
same shape because WordModel/SparseModel/RecordModel are active):

- **Mixer width `nx` = 456 every bit** (the full `Mixer<456, …, 6>`), and
  `Mixer::update()` runs **6 trains of 456 shorts per bit**; `p()` runs 6 dot
  products of 456 shorts. ≈ 5.5 K madd-shorts per bit, 44 K per byte — that is
  the entire SIMD surface.
- **`ContextMap::E::get()` runs 201 times per byte** (58 contexts across the
  four ContextMap owners x 3 probes at bpos 0/2/5, plus the `cpi[3]==2`
  nibble-completion probes). Each probe is one hashed, effectively random
  64-byte `E` entry in tables of up to 2 x 2 GiB at L11.
- L0–3 for contrast: nx = 48, 14 probes/byte — the word/sparse/record models are
  ~85 % of both budgets.

gprof shape at L4: WordModel::mix 33 %, mix2 17 %, Mixer::p 11 %,
RecordModel 9 %, ContextModel::p (contains the trains) 6 %, SparseModel 6 %,
cm.mix1 5 %, Predictor::update + APMs ~11 %. The rangecoder does not appear in
the top tier at all, which matches the prior: model ≈ 99 %, coder + I/O ≈ noise.

The controlling fact: **the program is latency-bound on the hashed probes, not
ALU-bound**, at every level >= 4. Even the 67 MB L4 model is 2x the 33 MB L3, and
the bucket index `(cxt[i]+cc)&Sz` is hash-uniform, so probes miss regardless of
input size. The probes are independent across `i` but the out-of-order window
cannot run far ahead through `get()`'s data-dependent search/LRU branches, so
they serialize. That is why baseline speed barely drops from L4 (28 KB/s) to
L8 (23) — the latency was already exposed — and why the fix below pays at L4 too.
Any profile that puts `dot_product` at 40–55 % and calls the program
compute-bound (several of the reviewed docs do) is describing a different
program; here the mixer is ~15 % *after* the stalls are removed.

## 3. Prefetch the hashed probes — the one big lever (+32–38 %)

`E` is 64 bytes at `alignas(64)`, so one prefetch covers the whole entry, and the
probe addresses for all `cn` contexts are known before the walk starts. Top of
`mix1`:

```c++
if( bpos==0||bpos==2||bpos==5 )
  for( int i = 0; i<cn; ++i )
    PAQ_PF(&t[(cxt[i]+cc)&Sz]);   // __builtin_prefetch(p,1) / _mm_prefetch
```

(bpos 1/3/6 and 4/7 only re-slice `cp0` inside the already-resident line;
bpos>1 with `runp[i][0]==0` skips the probe — prefetching those anyway is
harmless and keeps the guard trivial.)

Measured alone, interleaved best-of:

| case      | base | +prefetch |
|-----------|-----:|----------:|
| 64 KB L4  | 24.8 | 34.2      |
| 128 KB L8 | 23.7 | 31.3      |

The i=0 probe gets no cover, i>=1 overlap the state updates and mix2 work of the
preceding contexts; with 46 contexts in WordModel's map the average cover is
long. Byte-identical trivially (prefetch has no architectural effect) — verified
anyway on the full set.

**Shape matters.** Four of the reviewed docs propose prefetch, all in weaker
forms: one-iteration-ahead inside the walk (grok/kimi/qwen) gives a single
`nex()`+`mix2` of cover instead of the whole preceding walk; prefetching only at
`bpos==0` from `ContextModel::p` (gem) misses two thirds of the probes — the
bpos-2 and bpos-5 re-probes hit fresh hashed lines too. All-addresses-upfront at
all three probe positions is what produces the number above.

**APM variant, measured a wash here, kept in the source:** all six APM contexts
in `Predictor::update` are functions of state that is final *before* the
`contextModel.p()` call (`c0/b1/x5/w5/fails/failz` are updated above it), so the
hashes can be hoisted and both lines of each 33-entry region prefetched under the
entire model call. Legal (pure reordering of pure computation, verified
identical), but on top of the ContextMap prefetch it bought 3/4 marginal wins
within noise — 6 probes/bit against 25/bit was never going to dominate. Costs
nothing; drop it if you want the diff minimal.

**Not implemented, next in line:** the bpos-2 and bpos-5 probes can be issued a
full bit earlier by prefetching both branch targets `(cxt[i]+2*c0)&Sz` and
`(cxt[i]+2*c0+1)&Sz` at bpos 1 / bpos 4 — an entire bit of mixer ALU as cover,
2x line traffic. Also: `ContextModel::p()` currently lets the three
`RunContextMap::set()` BH walks (MEM/4..MEM/2 tables) run before `cm`'s prefetch
loop; hoisting one prefetch pass over rcm buckets + cm entries above all the
`set()` walks is the same trick once more. Expect small marginal gains — the
main serialization is already gone.

## 4. AVX2 kernels (+4–8 %) — and why the ceiling is AVX2

`dot_product`/`train` are the whole SIMD surface (§2). The SSE2 pair widens to
AVX2 with the result **provably identical**:

- `dot_product`: `madd` pairs the same adjacent shorts at any lane width, the
  `>>8` is per-32-bit-lane, and the final value is a mod-2^32 sum of the same
  int32 terms — wrapping addition is associative/commutative, so lane count and
  reduction order cannot change it.
- `train`: elementwise saturating ops; lane width is invisible as long as each
  element is processed exactly once.

Mechanics that matter (three of the reviewed docs get these wrong, see §9):

- **Rows are 16-byte aligned only.** `wx` row stride is N x 2 = 912 B (N = 456,
  a multiple of 8 but not 16), so `_mm256_load_si256` faults on odd contexts
  regardless of any `alignas(32)` on the array — alignment of the base cannot
  fix a stride. Use `loadu` (measured fine: with row starts cycling through
  0/16/32/48 mod 64, about a quarter of the 32 B loads split a cache line, and
  the +4.4 % below already includes that cost).
- **Tail: nx ≡ 8 (mod 16).** Handled with one 128-bit step in both kernels.
  **Do not "fix" it by padding `nx` to 16 in `Mixer::p()`**: `tx` is
  `short tx[456]` exactly, and `wx` rows are contiguous — writing `tx[456..463]`
  / training `w[456..463]` overflows into the next context's row (the same class
  of quiet corruption as the nin-overrun trap in ddsdet).
- The clean alternative, proposed correctly in one reviewed doc: raise
  `N` itself to `(n+15)&-16` = 464. Then rows are 928 B (32-byte multiple),
  aligned loads become legal, the pad lanes are zeros (dot contribution 0;
  train of a 0 input writes 0), and the stream is unchanged. Legal but
  unmeasured here; against a one-instruction tail and loadu that already wins,
  the typedef churn wasn't justified.

Measured, interleaved (64 KB L4): SSE2 23.2, AVX2 24.2 (5/5 wins, +4.4 %).
On top of prefetch at L8, AVX2 added +7–8 % (31.3 → 33.7) — the ALU share grows
once the stalls are gone.

**AVX-512 was built, verified identical, measured, and removed: no gain over
AVX2** (23.5 vs 24.2 at L4, losing 4/5 interleaved). 456 shorts is 14 512-bit
iterations — too short to amortize, with frequency licensing on top. VNNI
(`dpwssd`) is unusable regardless: it fuses madd+accumulate with no room for the
`>>8` between them, so it cannot reproduce the stream. The reference source now
carries AVX2/SSE2/SSE/scalar only.

## 5. Hugepages — built for both OSes, byte-identical, unproven in this container

The case: 201 probes/byte, hash-uniform over 0.5–4.5 GB of tables. A 4K-page
dTLB covers ~6–8 MB, so at L8+ essentially **every probe is also a page walk**;
2M pages raise coverage ~500x and should convert one miss class into none. This
is a known win for paq-family codecs.

`malloc1` gains a path for blocks >= 8 MB (the model is a single `new Model`, so
one call covers every big table; `free1` is already a no-op):

- **Linux**: anonymous `mmap` over-allocated by 2 MB, pointer aligned up to a
  2 MB boundary, `madvise(MADV_HUGEPAGE)`.
- **Windows (MinGW/MSVC)**: try `SeLockMemoryPrivilege` via
  `AdjustTokenPrivileges` (note: its TRUE return is not success — check
  `GetLastError()==ERROR_SUCCESS`), then
  `VirtualAlloc(MEM_COMMIT|MEM_RESERVE|MEM_LARGE_PAGES)` rounded up to
  `GetLargePageMinimum()`; on any failure fall back to plain `VirtualAlloc`,
  which still yields zeroed pages so `PAQ_LAZY_ZERO` stays valid. The privilege
  is off by default — grant "Lock pages in memory" (secpol.msc) to the user and
  re-logon, or the build silently runs on 4K pages.

**Status: could not be validated here.** THP is in `madvise` mode but the
container never granted a page — `AnonHugePages: 0 kB` in `smaps_rollup`
mid-run — and the measured effect was correspondingly a wash. That is evidence
about the container, not the technique. On real hardware check `AnonHugePages`
(Linux) or that `VirtualAlloc` with `MEM_LARGE_PAGES` actually succeeded
(Windows) before believing any number.

## 6. Startup: ctor memsets vs demand-zero (wash here, matters at L11)

With storage from anonymous mmap / `VirtualAlloc` the pages are already zero, so
`ContextMap`/`BH`'s `memset(t,0,sizeof(t))` is redundant — `PAQ_LAZY_ZERO` skips
them behind a `g_zeroed` flag set by the allocator path. Everything that writes
nonzero patterns (SSCM's 32768 fill, APM/StateMap/Mixer/Ilog init) is untouched.

Correctness rests on the same two facts as the 004 allocator work: the OS
zero-page guarantee, and the BH `PAD` slack — the stream's documented dependence
on an out-of-bounds read seeing zero — staying zero, which demand-zero
preserves. One interaction to respect: **`PAQ_POISON_NEW` + `PAQ_LAZY_ZERO` is a
contradiction**, the poison fill is precisely what lazy-zero assumes absent; the
0xA5 proof must be run against the memset build only.

Measured init-only (empty input): L8 both ~0.28 s, L9 both ~0.52 s — on this
container the process/sandbox overhead swamps a 0.5–1 GB memset, so the gain was
not demonstrable. The arithmetic still holds at L11 (two 2 GiB maps + the rest
≈ 4.5 GB touched at ctor time) and lazy-zero additionally keeps RSS at
touched-pages rather than table-size for short inputs.

## 7. Marginal and null results — measured, don't re-propose

- **AVX-512 over AVX2: 0** (§4). Removed.
- **Mixer weight-row prefetch (`PAQ_WXPF`)**, proposed by four reviewed docs:
  prefetch the selected 912 B `wx` row in `Mixer::set()` so it lands before
  `p()` dots it and `update()` trains it. Byte-identical, measured at L8 on top
  of the full flag set: 32.16 vs 32.19 best-of-5, 2W/2L/1T — **null**. Expected
  in hindsight: `wx` is 9.6 MB (456 x 11008 x 2) and lives in a 33 MB L3, so a
  row miss costs an L3 hit, and the fetch-ahead distance from `set()` to `p()`
  is a few dozen instructions. Kept as a gate because the verdict may flip on
  CPUs whose L3 is smaller than `wx` + the ContextMap hot set; re-measure there.
- **PGO** (`-fprofile-generate` on L4+L8+decode, `-fprofile-use`): +0–2 %,
  4/4 tiny wins at L4, wash at L8. Free if the build system tolerates it, not
  worth process complexity otherwise.
- **APM hoist+prefetch, lazy-zero: individually within noise** on top of §3
  (both kept — they cost nothing and are exact).
- **I/O layer: already settled** — the coroutine→stdio port measured identical
  speed; at 5.5 K madds/bit nothing at the byte pipe is visible. This closes
  the reviewed docs' `IOBUFSIZE`, `fgetc_unlocked`, `posix_fadvise`, and
  progress-printf items in one line: the printf fires every 64 KB ≈ 3 s of
  work here, and 64 KB stdio buffers mean one syscall per ~3 s. Nothing to win.
- `-Ofast`/`-ffast-math`: nothing to gain — grep confirms the hot path has no
  float/double at all (FP appears only in the final KB/s printf), so "fast
  math" optimizes zero instructions. Two reviewed docs call it a pure win;
  it is a pure no-op, and a latent hazard if FP ever appears in table init.

## 8. Rangecoder and I/O: quarantined by the 99 % rule

Confirmed by both the instrumented counts and gprof: the model is ~99 % of the
time, so even a free 2x on the coder is <1 % wall. Several reviewed docs put
their biggest expected win here; for the record, what's actually true in
`sh_v1m` as embedded:

- The encoder's divisions are **already shifts**: `rc_Process` is called with
  literal `totFreq = 4096`, `mulRdiv` inlines at `-O3`, and `/4096` on the
  compile-time constant becomes `>>12`. Suggestions to hand-specialize it
  (kimi §4.1, mmax §2 item 1) change nothing.
- The only real division is decode-side: `rc_GetFreq` → `muldivR` divides by
  the live 64-bit `range` once per bit. The one *correct* removal among the
  reviewed docs is qwen §3.1's binary fast path: with `q = (R*p)>>12`,
  `split = R-q`, the decode decision `GetFreq(4096) >= px[1]` is exactly
  `code >= split` (the floor/ceil algebra works out: `R - floor(R*(4096-p)/4096)
  = ceil(R*p/4096)` and the comparison against a floor flips to a plain
  integer comparison). Verified on paper against the actual `rc_Process`
  identities; **not built** — it's a decode-only sub-1 % item by the profile.
- The same algebra is where two docs break the stream — see §9 (a), (b).

## 9. Errors in the reviewed proposals (checked against source or measured)

Seven external optimization docs were reviewed (gem/gpt/grok/kimi/mmax/qwen/
zai). Valid overlapping ideas are integrated above with credit where the form
was right. These items are **wrong as written** — each verified against the
actual source, the floor semantics, or a measurement:

**(a) mmax §2 "algebraically the two terms collapse" — breaks the stream.**
`tmp = range - mulRdiv(totFreq-cumFreq, totFreq)` is
`R - floor(R*(T-c)/T) = ceil(R*c/T)`, not `floor(R*c/T)`. The proposed
`code_off = (R*cumFreq)>>12` is off by one whenever `R*c mod 4096 != 0`, i.e.
almost every bit — encoder and decoder desynchronize. The doc labels it
bit-exact and its "single most worthwhile change"; per §8 the *ceiling* of a
correct version is <1 % anyway.

**(b) gem's reciprocal `muldivR`** — `(product * ((1<<48)/range)) >> 48` is not
`product/range`; the truncated reciprocal is off by up to 1 in the quotient.
Decode desync.

**(c) gem/grok/kimi AVX2 kernels use `_mm256_load_si256` and prescribe
"bump `alignas` to 32".** Faults at the second dot product: row stride 912 B
means every odd `wx` row is 16-mod-32 regardless of base alignment (§4). Only
zai (loadu) and qwen (raise N to 464) handle it.

**(d) zai's cheat-sheet `while (nx & 15) tx[nx++] = 0;` with the struct
unchanged** — writes `tx[456..463]` out of bounds and trains 8 shorts of the
next context's `wx` row: silent state corruption, stream change. (zai's body
text does mention the N-bump requirement; the copy-paste block at the end is
the trap.) grok's "N is already a multiple of 8 so the factor-of-two width is
free" fails the same arithmetic: 456 mod 16 = 8.

**(e) mmax §5 "the clamps are unreachable, delete them".** `squash()` returns
**0** for `d < -2047` (verified in source), so `p = M->p()` can be 0; without
`if_e0(p<1) p=1` that encodes bit 1 with frequency 0 — a broken, undecodable
archive on the first strongly-predicted-zero bit. (The `p>4095` arm is indeed
dead — squash caps at 4095 — but deleting one never-taken predicted branch
buys nothing.)

**(f) mmax §4 `llog` rewrite** — `(top&7)*16 + ilog(shifted)` cannot reproduce
the baseline's `128 + ilog(x>>8)` / `256 + ilog(x>>16)` tiers (for x >= 2^24,
`top&7` is 0..7 → offset 0..112, not 256). Wrong values into RecordModel →
stream change. Also `llog` isn't hot.

**(g) mmax §8 shrink `Buf<1u<<30>` to 4 MiB** — `operator[]` masks with
`&(SZ-1)`; match/word positions are absolute, so every reference farther back
than the new size reads different bytes. Stream change, mislabeled bit-exact.
(The 1 GiB is demand-zero .bss: it costs address space and RSS-on-touch, not
load time.)

**(h) mmax §9 fold `ContextMap::set`'s two multiplies into one 64-bit
multiply** — there is a `<<16|>>16` rotate between the multiplies; no single
multiplicative constant reproduces mul-rot-mul. Different `cxt[]` → stream
change. mmax §10's xorshift swap for `Random` likewise changes the `rnd()`
sequence that drives the `ns -= 4` state decay: stream change, in a doc whose
premise is "every recommendation is bit-exact".

**(i) kimi §6.1's unrolled `memmove` replacement** is broken as written: byte
assignments (`p[24] = p[20]`) where 4-byte entries must move, with duplicated
lines between cases. mmax §7's `int*` switch version is the correct form —
but `BH` moves <= 24 B inside one cache line and is not in the profile's top
tier; measured class: noise.

**(j) gpt #15 (cheaper hash functions) and #28 (reorder model execution)** both
change the stream while the doc's frame is "preserve the model and bitstream":
the hash values *are* the table addressing, and model order defines the
`m.add()` input ordering that mixer weights and SubMixer set boundaries index.

**(k) Misdiagnosed profiles.** gem ("mixer is the ALU consumer"), zai
("dot_product 40–55 %", `wx` "8.0 MB, 464 x 8960" — actual M = 128*86 = 11008,
9.6 MB), gpt/mmax (rangecoder 8 %, "the biggest single change is the
rangecoder") — all contradicted by the instrumented counts in §2. The docs'
per-item speedup tables (e.g. "AVX2 mixer 1.8–3x", "overall 2–2.5x from SIMD")
inherit the error; measured reality is §0.

## 10. From the reviews, untested but plausible — in order

Ideas from the seven docs that are exact, not yet measured here, and not
already closed by §7:

1. **qwen §3.3 `dot2`**: interleave two rows per pass so `tx` loads are reused
   and two miss streams overlap. Exact (independent accumulators, same per-lane
   ops). The mixer is ~15 % post-prefetch, so the ceiling is a few percent.
2. **Fixed-level build** (qwen §8): compile one `Dispatch` instantiation
   (`-DFIXED_LEVEL=n`) — kills the other 11 template bodies' I-cache footprint.
3. **`cxtfl` templating** (qwen §5.1): turns a well-predicted per-context
   branch into two instantiations. Expect ~0 by §1's noise floor, but it's
   clean and free.
4. **`__restrict`/`hot` attributes, `-fno-exceptions -fno-rtti`, LTO** (all
   docs): single-TU program, so LTO is mostly moot; the rest is hygiene.
   Expect within noise; harmless.
5. **E::get SIMD checksum compare** (gem/gpt/qwen): after §3's prefetch the
   line is in L1 and the 7-entry scan is short with an LRU fast path;
   expect ~0. qwen's own masking arithmetic for it is wrong (`& 0x7f7f`
   keeps the wrong movemask bits — entries 0–6 of 16-bit lanes are
   `mask & 0x1555` after the even-bit reduction), one more reason to leave it.

## 11. Recommended configuration

Linux:
```
g++ -O3 -march=native -DPAQ_PREFETCH -DPAQ_AVX2 \
    [-DPAQ_HUGEPAGES after verifying AnonHugePages on the target] \
    paq8hpc_speed.cpp -o paq8hpc
```

Windows (MinGW gcc — this exact command cross-verified byte-identical to the
Linux build, encode and decode):
```
x86_64-w64-mingw32-g++ -O3 -mavx2 -static -DPAQ_PREFETCH -DPAQ_AVX2 \
    [-DPAQ_HUGEPAGES -DPAQ_LAZY_ZERO] paq8hpc_speed.cpp -o paq8hpc.exe
```
(`-march=native` works with native MinGW too; `-mavx2` is the portable floor.
MSVC: `/O2 /arch:AVX2` — the prefetches go through `_mm_prefetch` there via the
`PAQ_PF` macro.)

Expected on this class of machine: ~1.4x at L4, ~1.55x at L8–L9 over the
uploaded baseline, byte-identical output. Next candidates in order of expected
return: hugepages on hardware that grants them; the speculative bpos-1/bpos-4
two-target prefetch (§3); the rcm/cm prefetch hoist above the `set()` walks
(§3); `dot2` (§10).
