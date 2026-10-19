# h264p: a plan for faster compression

h264p compresses at about 40 KB of H.264 per second (`test/002/04.h264`, 484 KB,
11.3 s on a 2.1 GHz Xeon; `02.h264`, 28 KB, 1.0 s), and decompresses at about the
same speed (04: 10.5 s): the model is symmetric, and both sides reconstruct the
pictures. This plan starts from where the time actually goes, answers seven
questions (allocation, template branches, `restrict`, SIMD lookups, memory
latency, layout, vectorization), and orders the work by what it buys.
Section 4 has what has been done since, measured: compression 3.2x and
decompression 2.6x faster over the corpus, to the same output but for a
cheaper end_of_slice_flag (3 bytes more over the 96 test streams).

## 1. Where the time goes

Measured with callgrind (instruction counts, `-O3 -g` build of the current
source) and wall-clock timings; g++ 13.3, 4 cores, AVX-512 available.

| | 04.h264 (P/B, typical) | 02.h264 (intra-heavy, high rate) |
|---|---:|---:|
| instructions | 117.8 G | 1.70 G |
| values coded | 3.28 M | 0.21 M |
| instructions per value | ~36,000 | ~8,100 |
| L1 data miss rate | 0.67% | 0.9% |
| LL misses (276 MB L3 here) | 0.68 M | 0.27 M |

By component (share of instructions):

| component | 04 | 02 |
|---|---:|---:|
| template search (`Recon::tm_search`, `tm_miss`, `tm_bi`) | 45.9% | 23.2% |
| motion compensation (`mc_luma`, `mc_chroma`, `inter`) | 18.5% | 1.3% |
| other pixel work (intra prediction, proxies, textures, IDCT) | 10.5% | 5.5% |
| deblocking | 2.4% | 0.2% |
| **pixel work, total** | **77.3%** | **30.2%** |
| CM arithmetic (counters, APMs, mixers, coder: cm/fam/rc.inc) | 13.8% | 45.8% |
| context assembly (`model.inc`) | 4.2% | 10.7% |
| context indices (generated `MOD/*_p.inc`) | 1.2% | 4.8% |
| syntax walk, CABAC/CAVLC (three passes in `c`) | 2.2% | 6.4% |
| libc (memset, memcpy), driver, value log | 1.4% | 2.0% |

The code is mostly compute-bound: fewer than 1% of data accesses miss the
L1, and on this machine all 226 MB of model tables fit in the L3 (2.5 has
what the misses cost).

**The template search.** It runs 1.45 M times on 04 (on 02, 4,700 times),
evaluating 19.9 vectors per search: 78% at sub-sample positions (bilinear), and
6.5% repeats of a vector already evaluated in the same search. One loop, the
bilinear SAD (`recon.inc:1776`), is 18% of all instructions; with its row loop
and the edge path, the template SADs are about 37%. Most of the rows are the
left template's, two samples wide. Who calls it on 04 (inclusive instructions):

| caller | calls | instructions | share |
|---|---:|---:|---:|
| `block()`: the intra proxy refined before each 4x4/8x8 luma block of an I_NxN macroblock (`model.inc:2303`) | 92 K | 17.2 G | 14.6% |
| `ref()`: the best of up to 8 references, each a full search (`model.inc:1681`) | 383 K | 12.0 G | 10.2% |
| `skip()`, and `tm_best()` from it: every P/B macroblock | 714 K | 12.9 G | 11.0% |
| `proxy_mv()`: the intra proxy's vector | 30 K | 5.3 G | 4.5% |
| `mvd()`: the vector the mvd is coded against | 235 K | 3.1 G | 2.6% |

**Motion compensation.** `tm_miss()` (the skip context) predicts its template
in 4x2 and 2x4 pieces, one `mc_luma()` call per piece and list: 6.2 M calls on
04, about 1,000 instructions each, most of it setup (`recon.inc:1697`).
`inter()` predicts every P/B macroblock for the skip context (0.5 M calls).

**The CM arithmetic.** Each decision reads ten counters (6-byte `cm_cnt2`,
rows of `nd` nodes), makes 22 stretched inputs, mixes them in three 24-input
mixers with 32-bit weights and 64-bit sums, then a final mixer and two APMs,
and updates all of it. That is a few hundred instructions per decision. On 02
it is 46% of the time.

**Build flags.** These changes give bit-identical output and are within the
±5% timing noise on 04:
- `-march=x86-64-v3` or `-march=native`: about 0-8%. The mixer loops vectorize
  only with AVX2 or AVX-512, because of their 64-bit arithmetic.
- PGO.
- clang 18.

## 2. The seven questions

### 2.1 Dynamic allocation

There is none on the hot path. Per decision and per macroblock the code works on
fixed arrays: `MbInfo`/`MbCol` in vectors sized per picture, and stack buffers in
recon.

| where | what | verdict |
|---|---|---|
| model tables (`model.inc:85`, `tc_map`) | 15 per-family structs, 226 MB, sizes compile-time constants in the release build, mapped once | make them static arrays (each family is under 100 MB; the largest, `sig`, is 58 MB). BSS is zero-filled and committed on touch like the mapping, and static addresses let the compiler fold the base addresses and row sizes (see 2.2). One model per process, as now. Keep the mapping in the tuning build, whose sizes are load-time values. |
| `tc_fam::Z`, `ZM` (hint counters, `fam.inc:82`) | `std::vector`, assigned on first use | static per family; the size is a compile-time constant |
| `rhash_` (raw match model, `model.inc:459`) | `std::vector`, 256 KB, assigned at the first raw byte | static array |
| `info_`, `hist_[30]`, `aq_*`, `pxv_` | vectors sized by the picture, resized when it grows | keep: at the largest picture (139,264 macroblocks) `info_` alone would be 164 MB and `hist_` 2 GB; they are not reallocated per macroblock or per picture |
| recon's pictures (`RPic`) | per picture size, reused through `spare` | keep |
| `hprev_` (`model.inc:758`) | a copy of the kind's whole history per picture (496 B per macroblock: 4 MB per 1080p picture) | swap two buffers instead of copying |
| `begin_mb()` (`model.inc:781`) | memset of the 1,180-byte `MbInfo` per macroblock | clear only the fields that are read before being written |
| recon per picture | `db.assign` (224 B per macroblock), `make_maps`' temporary vector; per slice, `build_lists`' small vectors | keep, or reuse members; negligible |
| driver per NAL unit | `unescape`/`escape` and `find_trailer` copy each NAL unit 4-5 times | compare in place; negligible |

So allocation is not where the time goes (libc is 1.1-1.3% in total). Static
tables still matter, as part of 2.2.

### 2.2 Runtime branches that could be template parameters

- **Family geometry.** `tc_fam` keeps every table's hashed-or-direct choice,
  hash bits, `nd`, `nm`, APM sizes and sign contexts as members set by `wire()`.
  In the release build they are all constants. With a traits class per family
  (`tc_fam<f_DEC, H_sig_traits>`) and static tables:
  - `select()` (`fam.inc:201`) loses its 14 branches and computes the rows in
    straight-line code;
  - `* nd` becomes constant multiplies or LEAs;
  - families with `nd == 1` (sig, sgn, mvg, qpg) drop the node arithmetic.
- **Inputs that are always zero.** Where a call site has no outside prediction
  (`prp < 0`, `prpa == nullptr`) or no hint (`hs == 0`), inputs 7 and 23 of the
  mixers are exactly 0. Their products and weight updates are then exact
  no-ops, so a `bit<PRP, HINT>()` specialization that drops them is
  bit-exact. `bitm()` likewise.
- **The syntax walk.** `SliceCoder` tests `W` (reader or writer) in 71 places
  and `cabac_` throughout. A `SliceCoder<W, CABAC>` would remove those tests.
  It would also let `c`'s model pass use a writer that skips producing bits:
  `compress()` discards that pass's bitstream (`h264p.cpp`), so it does a
  CABAC encode that nothing reads. The walk is 2-6% in total, so this is a
  small but free item.
- **The channel.** `Chan` is virtual, but called once per syntax element (a
  whole residual block per call), so devirtualizing buys little.
- **`tm_search`.** Its cost lambda tests `top`, `left`, `tr`, `ninner` per
  evaluation, and the SAD tests `inside` and `fx | fy` per row. Two
  specializations would remove these tests: one per template shape (thin
  2-sample templates for vectors to code, 16-sample ones for the proxy), and
  one that holds when the whole search window is inside the picture (checked
  once per search, true for 96% of rows).
- **`tc_bit()`.** It tests `tc_verbose` per bit, for the `-v` statistics; a
  compile-time switch would remove the test.

### 2.3 `restrict`

The mixers and APMs already use `__restrict`; the families' table pointers
do not. Where aliasing actually costs is recon's pixel kernels. They write through
`uint8_t*`, and a char store may alias anything, so the compiler reloads
coefficients, sources and `this->` members after every store and does not
vectorize. Affected: `mc_luma`, `mc_chroma`, `inter`'s weighted prediction,
`pred_nxn`, `pred_l16`, `pred_c8`, `idct4_add`, `idct8_add`, `add4`,
`proxy_build`, the deblocking filter.

Define the pointer types once, next to `u8`/`i16` in `cmbase.inc`:

```cpp
typedef uint8_t* __restrict pix;        // a picture or block buffer that nothing else points into
typedef const uint8_t* __restrict cpix;
typedef int* __restrict pint;
typedef const int* __restrict cpint;
typedef cm_cnt2* __restrict pcnt2;      // the families' tables
```

and use them in the kernel signatures and in `tc_fam`'s table members. GCC,
clang and MSVC all take `__restrict`. On its own this is worth a few percent,
but it is what lets the kernels in 3.1 vectorize.

### 2.4 SIMD for the hash table lookups

There is no lookup to vectorize: no table searches. A direct table is indexed by
the context. A hashed one takes its row from `(index * 0x9E3779B1) >> (32 -
bits)` (`tc_hrow`) with no tag and no probing, so contexts that collide share
the row. A lookup is one load. What SIMD can do here:

- **Computing the rows.** `select()` makes 17 row indices per value, 14 of
  them possibly hashed (per decision in the `nd == 1` families). With compile-time geometry (2.2) that is
  one 16-lane multiply and variable shift (AVX-512), or two with AVX2,
  instead of 14 scalar multiply/shift/branch sequences. Worth it only after
  2.2.
- **Gathering the counters.** Not a win: the loads are latency- or
  throughput-bound, and a gather is no faster than ten scalar loads.
- **Tagged rows ("vector rows").** Slots with 8- or 16-bit check bytes, found
  by a SIMD compare within a cache line, would cut collisions, so the big
  hashed tables could shrink at equal compression: `sig`'s (three of 2^21
  rows, 58 MB in all), and `cbf`'s, `mvs`'s and `lvl`'s. That is a model change, worth testing
  where memory latency matters (2.5).

### 2.5 Memory latency and interleaving lookups

`select()` already prefetches the rows of every table of a value, and `bit()`'s
ten counter loads are independent, so out-of-order execution overlaps them.
What remains exposed is the first decision of each value, and every decision
of the `nd == 1` families (sig: 0.48 M decisions on 04): the prefetch is
issued right before the read.

How much this costs, on 04:
- **DRAM.** Little. callgrind counts 0.68 M last-level misses with this
  machine's 276 MB L3. Simulating a desktop's 32 MB L3 gives 0.85 M: still
  under 1% of the run. A 484 KB stream touches far fewer rows than the 226 MB
  of tables hold. Longer streams touch more, so this grows with the input.
- **L1.** More. 65% of the 0.17 G L1 read misses (about 112 M, some 34 per
  coded value) are the CM's table rows: ten counter rows, two APMs in two
  arrays each, three mixers' weights and counts. They mostly hit L2/L3, but
  the CM core's share of the time is probably well above its 13.8% of the
  instructions. The layout changes (2.6) cut the lines per decision; the
  prefetching below hides the rest.

Two ways to hide it:

- **The compressor knows every value in advance.** The dry run has put the
  whole NAL unit in the value log, so the contexts of the next decisions can
  be computed ahead and their rows prefetched several decisions early. Done
  properly, this is the producer/consumer split (3.4), where the CM thread
  prefetches from a queue.
- **The decompressor** can prefetch the rows of the next decision for both
  outcomes of the current one, where the next context depends on it.
  Significance depends on the last bits through `hb`, `h3` and `rem`; levels on
  `g1` and `e1`.

### 2.6 Data layout

| structure | now | better |
|---|---|---|
| counter rows (`cm_cnt2`, 6 B) | `nd` × 6 B: 78 B for `nd` = 13, across 2-3 lines | align rows to 64 B and put nodes 0-2 (zero? one? two?, the decisions every value makes) in the first line; or split them into a dense table of their own |
| APM (`cm_apm`) | values (`i16 t[]`) and counts (`u8 c[]`) in separate arrays: two lines per access, four per decision | interleave them, `{i16 t; u8 c;}`, or two values and two counts per pair |
| mixer weights | 24 × `i32` = 96 B per set, its count byte in another array: 2-3 lines per mixer, 3 mixers per decision | the count in the same line; with 16-bit weights (3.3), 48 B, one line per set |
| `MbInfo` (1,180 B) | memset per macroblock; neighbours read the header, vectors and `coef` | hot/cold split; or no memset (2.1) |
| model history | `hprev_` copied per picture | double buffer |

The layout changes that keep the arithmetic (counters, APMs, counts) are
bit-exact.

### 2.7 Auto-vectorization

- **Mixers.** Their 64-bit products and sums vectorize only with
  `-march=x86-64-v3` (`vpmuldq`) or AVX-512 (`vpmullq`, `vpsraq`); the baseline
  x86-64 build runs them scalar. In fixed point that fits in 16/32-bit lanes,
  each 24-input mixer is a few `pmaddwd`s on SSE2 (3.3).
- **Template SADs.** The rows are short (16, 8, 4 or 2 samples), stop early on
  a budget, and choose between paths per row, so compilers do little with them.
  Written for SIMD, a 16-sample row is a handful of instructions
  (`pmaddubsw` for the bilinear weights, `psadbw`) instead of about 130
  scalar ones (3.1).
- **6-tap interpolation (`mc_luma`).** Generic: 32-bit intermediates, a switch
  per sample on the quarter-sample position, a copy with clipping at the edges.
  Per-position kernels with 16-bit lanes (as ffmpeg's `h264qpel`) vectorize
  well.
- **Counter updates.** Each needs `cm_rec[t]`, a table lookup, so they do not
  vectorize without gathers. Leave them scalar, made branchless (3.3).
- **Context indices.** The `MOD/*_p.inc` builders are table lookups with
  clamps: scalar, but they can be split (3.3).

## 3. The plan

**Bit-exact** means the compressed output does not change: check by comparing
the outputs of every test stream with those of the previous build (md5), as
well as by the round trips. **Model change** means the format changes: measure
the corpus and the 95-stream suite, and re-tune if it costs anything.

### 3.0 Measurement first

- [ ] `tools/bench.sh`: compress and decompress `test/002` and the small
  streams. Report MB/s per stream, and compare the md5 of each compressed
  output with a reference build's, failing on a difference unless told to
  expect one.
- [ ] Per-stage time in `-v`: walk, contexts, pixels, CM. Use counters around
  the families' `code()`/`bit()` and recon's entry points (rdtsc, or
  `steady_clock` per call batch).
- [ ] Keep `mk.sh`'s rule that tuning and release builds code identically,
  and add it to the checks. It caught a real bug.

### 3.1 Pixel kernels (bit-exact). Target on 04: about 2x

1. **Template SAD in SIMD** (`recon.inc:1762`). This is the biggest single
   item: 37% of 04, 23% of 02.
   - Rows of 8 or 16 samples: bilinear with `pmaddubsw` on interleaved
     (a, a+1) and (c, c+1) byte pairs, weights (4-fx)(4-fy) and the like as
     bytes; `+8 >> 4`, pack, then `psadbw` against the current picture.
     Integer positions are plain `psadbw`.
   - The left template (2 x 16): gather its 16 rows into vectors (or make a
     transposed copy per search, since the template is fixed) and do 16 rows
     at once, instead of 16 two-sample loops.
   - Check once per search that the window is inside the picture (96% of
     rows), and keep the clipped path for the rest.
   - Keep the budget test per row (or per two rows).
   - Expected: template search from 46% of 04 to about 12%.
2. **`tm_miss` in strips.** P_Skip, and every partition whose 4x4 blocks
   share a vector, can predict the 16x2 top strip and the 2x16 left strip in
   one `mc_luma` call each, instead of 8 calls per list (6.2 M calls on 04 to
   about 1 M). Also add a no-clip fast path.
3. **`mc_luma` and `mc_chroma` per position**, SIMD, 16-bit intermediates, no
   per-sample switch; the clipped copy only at the edges. Also do `inter()`'s
   averaging and weighting in SIMD.
4. **Intra proxy refinement, incremental** (`model.inc:2303`; 14.6% of 04,
   most of 02's template search). Within one I_NxN macroblock, the 16 searches
   share the outer template, and the inner template only grows by one block.
   Cache the outer SAD per vector for the macroblock, and the inner SAD of the
   blocks already in. The search sees the same costs, so this is exact.
5. **Candidates.** Drop duplicates (mvp, neighbours and zero often coincide),
   and keep the vectors already evaluated (6.5%).
6. **The other kernels.** Deblocking (2.4%); `proxy_mode` and `proxy_cmode`;
   `ac_energy`, `mb_texture`, `pred_texture`; the forward quantizers.
7. **`restrict`** on all of them (2.3).

Expected, all of 3.1: 04's pixel work from 77% of its instructions to about
23%, about 2.2x on P/B streams; about 1.3x on intra-heavy ones (02).

### 3.2 Context assembly (bit-exact)

1. **`block()`'s significance loop.**
   - In intra blocks it counts the estimated nonzero levels from each
     position to the end afresh at every position (`model.inc:2481`):
     O(n²), up to 2,000 `est()` calls per 8x8 block. Use a suffix count made
     once per block.
   - Likewise, make `q4`/`est`/`tcf` arrays per block, and keep the 8- and
     3-position significance histories as running shifts.
2. **Generated index builders in two stages.** `idx2inc.pl` could emit a
   per-value part (the variables constant over a value or block: `cg`, `md`,
   `nc`, `q6`, `pg`, `em`, ...) and a per-decision part (`pos`, `rem`, `hb`,
   ...). The index is a mixed-radix number, so a prefix computed once is
   exact if the constant variables come first in each pattern. Reordering
   the variables within a pattern renumbers the contexts but does not change
   what they distinguish, so it codes identically too (hashed tables aside).
   `H_sig_make` alone is 2.6% of 02.
3. **Per-macroblock bookkeeping.** `begin_mb()`'s memset, `finish_mb()`'s
   copies into the history (1.8% of 04), and the per-picture `hprev_` copy.

### 3.3 The CM core

Bit-exact:
1. **Family traits and static tables** (2.1, 2.2): straight-line `select()`,
   constant geometry, absolute addresses.
2. **`bit<PRP, HINT>()` / `bitm<...>()` specializations** without the
   always-zero inputs.
3. **Branchless counters.** `P()`/`Q()` test `t == 0` and clamp. Fold that
   into a select, or into a 16-bit table of the stretched values by `p >> 4`
   with the clamp built in.
4. **APM, counter and mixer layout** (2.6).
5. **Vectorize** the mixers' current 64-bit arithmetic with an AVX2/AVX-512
   path. Use function multiversioning or a dispatch on `cpuid`, so that one
   binary still runs on baseline x86-64.

Model change, measure first:
6. **16-bit fixed-point mixers.** Inputs are stretched probabilities in
   ±2047 and fit in 16 bits; weights 16-bit with a shift (as lpaq/paq8), or
   32-bit with `pmuldq`. The dot product is then 3 `pmaddwd` (SSE2) or 2
   (AVX2), and the update one `pmulhw`-based pass. Expected: the mixers from
   about half the CM time to a fifth. It changes rounding, so measure the
   suite. paq-style mixers lose little to nothing at this precision.
7. **Fewer tables where they buy least.** Each counter table is a load and an
   update per decision. h264p_cm.md lists what counters seven to ten bought
   (19 KB on the corpus); a family-by-family check of what each table is worth
   would tell where to drop one.

Expected: the CM core 1.5-2x faster. That saves 5-7% of 04's time but 15-23%
of 02's, and after 3.1 the CM core is a much larger share of what remains.

### 3.4 Compression only: a producer and a consumer

In `c`, the model pass knows every value before it codes it, from the value
log. Nothing in the contexts depends on the CM state:
- the pixel work depends only on the values and the reconstruction;
- `cab0`/`cab1` read the walk's CABAC states, which follow from the values;
- hints and outside predictions are computed from contexts.

The compressor can therefore split the work:

- **Thread P** runs `Model<ENC>` and recon, but instead of coding through
  the families it records each coding call into a ring buffer: the family,
  the `tcx` indices, node or value, sign context, `prp`/`prpa`, hint. In the
  encoder a coding call's result is the value itself, so P never waits.
- **Thread C** replays the records into the families and the range coder,
  prefetching the rows of the record 8-16 ahead. That hides nearly all the
  memory latency (2.5).
- **The decoder keeps the inline path.** The context code is the same source
  for both, with the record/replay layer inside the families, so the contexts
  are identical by construction.
- Records are about 90 B. Use batches per NAL unit or per macroblock row and
  a bounded ring (64 MB holds 0.7 M records), not the whole stream.
- A third thread can do the dry run (parse and verify) of the next NAL
  units: 1.5% of 04's instructions, though `c` takes 8% longer than `d` on
  04.

Speed-up: compression time becomes the larger of the two parts instead of
their sum. After 3.1-3.3, on 04 that is about 1.2x (P, mostly pixel work, is
the larger part); on 02, where the two are about even, close to 2x.

Going further means making P parallel. The reconstruction and every pixel
context of a picture follow from the values and the reference pictures alone,
so they can run ahead, frame by frame or by macroblock rows. Then only the
cheap per-value model state (history, AQ fits, run counters) stays sequential.
That is a larger refactor; do it if 3.1-3.4 still leave compression too slow.

### 3.5 Options that trade compression for speed (model changes)

Measure each against the suite. Ship one only as a mode recorded in the
stream header, or not at all.

- **`ref()` ranking by whole-sample search only**, skipping the half and
  quarter steps for the up-to-8 references (10% of 04).
- **Intra proxy refined per 8x8 instead of per 4x4 block**, or once per
  macroblock.
- **Segment-parallel mode.** Independent models for segments that start at
  IDR pictures, coded in parallel: near-linear scaling with cores, at the
  cost of each model's warm-up and 226 MB per thread.
- **Smaller or tagged tables** for desktop caches (2.4, 2.5).

### 3.6 Order and expected result

| step | bit-exact | 04 (P/B) | 02 (intra) |
|---|---|---:|---:|
| 3.0 bench and stage timers | yes | — | — |
| 3.1 pixel kernels | yes | ~2.2x | ~1.3x |
| 3.2 context assembly | yes | ~1.05x | ~1.05x |
| 3.3 1-5 CM core | yes | ~1.1x | ~1.2x |
| 3.3 6 16-bit mixers | no | ~1.08x | ~1.15x |
| 3.4 producer/consumer (c only) | yes | ~1.2x | ~1.9x |
| **total for `c`** | | **~3.3x** | **~3.7x** |

These are estimates from the instruction counts (Amdahl, assuming about 4x
on the SIMD kernels and 1.4x on the bit-exact CM changes), to be replaced by
measurements as each step lands. Decompression gets 3.1-3.3: about 2.7x on 04
and 1.9x on 02.

## 4. Progress

What has landed, measured (wall clock, this machine, the original build and
the current one run one after the other; its timings vary by 5-10% from run
to run and from day to day). Every step codes exactly as before it (the
output of every test stream is the same, and the round trips pass) but the
last, which changes how end_of_slice_flag is coded.

| stream | bytes | `c` before | `c` now | `d` before | `d` now |
|---|---:|---:|---:|---:|---:|
| 00 (low rate, nearly all skipped) | 815,140 | 30.95 s | 9.56 s | 30.03 s | 11.20 s |
| 01 | 1,095,103 | 28.15 s | 8.20 s | 27.54 s | 10.83 s |
| 02 (intra, high rate) | 28,143 | 0.33 s | 0.24 s | 0.29 s | 0.23 s |
| 03 (much intra) | 1,181,369 | 40.11 s | 12.44 s | 39.95 s | 15.86 s |
| 04 (P/B) | 484,238 | 12.28 s | 3.56 s | 12.49 s | 4.45 s |
| 05 | 31,669 | 0.47 s | 0.28 s | 0.45 s | 0.30 s |
| 06 | 230,978 | 2.15 s | 1.04 s | 2.15 s | 1.23 s |
| **total** | 3,866,640 | **114.4 s** | **35.3 s (3.2x)** | **112.9 s** | **44.1 s (2.6x)** |

The steps, in order (the gains on the streams named, paired runs):

- **SIMD template SAD and mixers** (3.1.1, 3.3.5): the bilinear SAD, then
  the mixers' sums with exactly the results of the 64-bit loops (each weight
  split into 15-bit halves for `pmaddwd`, the error into 16-bit halves for
  the update). `tm_miss` predicts its strips in runs of equal motion (3.1.2);
  `mc_luma` computes the half samples a position needs once. 04: 1.3x.
- **The intra proxy's searches share their costs** (3.1.4), whole sums only,
  so each search sees the costs it saw before. 04: 1.1x.
- **AVX2 as the target** (x86-64-v3; `PARCH=` builds the plain C++, which
  codes identically). Reference pictures get a padded copy of their luma
  and that transposed: no clipping near the edges, and the two-sample-wide
  left templates become two rows of the transposed copy (the six-tap and
  bilinear interpolations are symmetric). The thin templates' SAD is whole
  and branch-free, since their vectors change sub-sample position from one
  evaluation to the next and the branches mispredicted. AVX2 in `mc_luma`,
  `mc_chroma`, `inter()`'s weighting, `tm_bi`, the texture and energy sums;
  the three mixers' inputs packed once; the next decision's rows
  prefetched. 04: 1.5x.
- **The coder in a second thread** (3.4): the compressor records its
  families' coding calls in a ring for a thread that codes them (with the
  rows of the records a few ahead prefetched) and writes the output. 04 and
  06: 1.2x; intra streams, where the coder is the larger part, little.
  Decompression cannot: what it decodes steers its model.
- **Skipped macroblocks** (most of 00): the skip motion derived once, the
  predictions by each list kept for `bi_diff` and `inter()`, no boundary
  strengths inside a flat inter macroblock, no residual pass without
  residual, the model's per-macroblock lookups (place, list 1's history,
  skipped neighbours) made cheap. 00: 1.35x, 04: 1.15x.
- **SATD in AVX2** for the intra mode estimates. 03: 1.1x.
- **The template search's walk shared** through a template over its cost
  function: the thin templates' cost on templates loaded once per search,
  the bilinear weights from a table; candidates seen already and the
  diamond's step back are skipped (both known not to win). `d` of 04: 3-7%.
- **The intra proxy's 16-sample templates**: the template above and the one
  above right summed as rows of 32, each reference row interpolated once
  for the two prediction rows it serves; the blocks decoded so far summed
  whole without a call each. The proxy's searches 21-31% faster; `d` of 03:
  2.5%.
- **The 4x4 and 8x8 intra predictions by mode**, a loop each instead of a
  switch on the mode per sample (the proxy's mode estimate predicts every
  block in all nine modes). `d` of 03: 2%.
- **The next macroblock's reference samples prefetched** (luma and chroma
  at the vector zero, in the first reference of each list): a skipped
  macroblock's prediction, early in its coding, stalled on them. `d` of 03:
  2.8%.
- **Less work around skipped macroblocks**: the snapshot of the last
  picture's macroblocks, which only P slices read, taken for P pictures
  only; spatial direct derivation an 8x8 block at a time where
  `direct_8x8_inference` makes its four 4x4 blocks alike; the macroblock's
  left samples gathered once for all the searches. 00: 2-4%.
- **A decision's ten counters in AVX2**: loaded four to a vector, their
  probabilities stretched by a gather and packed for the mixers, their
  updates at once (the fast probability through the slow one's arithmetic
  with the rate 8192/65536), written back in the order of the arrays. The
  mixers' inputs are in the counters' order now: all weights start alike,
  so the order changes nothing coded. A decision takes 22% fewer
  instructions; `d` 3-5% (more instructions saved than time: the decision
  waits on its rows).
- **The deblocking filter in AVX2**: a luma edge's 16 lines at once (across
  columns through a 16x8 transpose), a chroma edge's lines of both planes
  at once. `d` of 03 and 01: 2-3.5%.
- **The proxy's distance from the decoded luma** (`proxy_acc`, asked before
  every residual block of an intra macroblock) a masked row at a time. `d`
  of 01: 2.8%.
- **The compressor parses ahead in a third thread**: a NAL unit's work that
  needs no model (its parsing, writing it back to see that it comes out
  the same, the padding detector, 6-8% of the model's thread) runs up to
  three NAL units ahead on a context of its own; the model's context takes
  the same parameter sets and padding as it reaches each NAL unit. `c`:
  4-12%.
- **end_of_slice_flag by a counter of its own** after a slice of its kind
  that was the whole picture (there 0 all but for sure); elsewhere the skip
  family's decision as before. It took a full decision per macroblock, as
  much time as the skip flag: `d` of 00 5.5%, of 01 4.5%. The 96 test
  streams take 3 bytes more; the format's version is 2.
- **Pictures finished in a thread of their own, behind the decoding**:
  deblocking, the maps the model reads and a reference picture's padded and
  transposed luma (7% of `d` on 00, 4% on 04) take each macroblock row once
  the decoding has come in order two rows past it (decoding reads the row
  above its own unfiltered, and deblocking a row changes the one above it).
  The decoding only publishes how far it has come, and the thread looks
  about once a row (after a look that found nothing it waits twice as long,
  after one that found two rows or more half as long): waking it for each
  row cost more than it saved (a wake-up from one vCPU to another is dear
  here). At a picture's end the decoding finishes what the thread has not
  begun. A decoding that leaves the order (slice groups, slices out of
  order or repeated) has what was let go finished and lets go no more, so
  what comes out never depends on timing; built without threads, the same
  rows are finished at the same points. `d` of 00 5%, of 01 4%, of 04 2-4%,
  of 06 as before; `c` of 00 7% (04's coder thread sets its pace). It
  costs 6-11% more CPU time, the thread's share included.
- **A macroblock's coefficient rows cleared only where written**: each
  record notes the rows of coefficients it wrote, the next macroblock in
  its place clears those instead of all 816 bytes, the neighbours' and the
  colocated block's rows not written read as zero without a load, and the
  history copies the rows written then or before. On 00, 0.5% fewer
  instructions and 8.5% fewer L1 data misses; the time it saves is within
  the noise.

What did not pay:

- AVX2 six-tap interpolation took 70% of `mc_luma`'s instructions away but
  hardly any time: the scalar code ran at a high IPC. (Kept: it costs
  nothing.)
- Huge pages for the model tables (`MADV_HUGEPAGE`): a tenth of the page
  faults, 0.05-0.1 s less a run on small streams, but runs started after
  other processes had fragmented memory stalled up to 0.7 s in compaction.
- The searches cut down within the 0.1% budget (3.5): `tm_best` in whole
  samples, `ref()` on four references in whole samples, the mvd's search
  and the proxy's in halves, two diamond steps instead of four. On the 96
  test streams these cost -44 to +931 bytes, but once the searches were
  vectorized they bought 0-2.5% of CPU time, so the format stays as it is.
- The template walk's neighbours costed all at once and the best taken
  without a branch, or the thin cost forced inline: no change measured.
- Counters kept as their difference from 1/2 (no select on a fresh
  counter): as many instructions as the selects they replaced.
- Prefetching the next macroblock's model records and colocated motion,
  or only the next node's APM and mixer rows: no change measured.
- The coder's decision out of line, one copy instead of 35 (a third of the
  binary): no change measured.

Where the time goes now (decompressing, one thread):

- 00: the coder about 30% (the skip flag alone 12%), the skipped
  macroblock's own work (direct motion, motion vector prediction, the
  neighbours' counts, bookkeeping) about 15%, the template searches 8%, the
  predictions and `tm_miss` 10%.
- 01: the coder about 35%, the template searches 13%.
- Compressing, the coder's thread waits for the model's: with the parsing
  in a third, the three take about 70% of one thread's time on the large
  streams.

What is left in the plan: the generated index builders (3.2.2), the rest of
the coder's arithmetic (3.3; it waits on its rows more than it computes
now), and making the model's side parallel (3.4, last paragraph).
