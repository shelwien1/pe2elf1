# h264p: one thread, and lighter model levels

The aim: h264p runs in one thread. Several threads are used by running
several h264p processes side by side, each on a large piece of video
stream, not by splitting the work of one. Each must do 400-500 KB of
H.264 per second, compressing and decompressing, and can lose some
compression for it. The current model stays as level `-9`, the strongest;
lighter levels become further instances of the model template, as
compression and decompression already are (`Model<0>`, `Model<1>`).

This document measures where one thread's time goes in the current model,
what each part buys in compression, and what the lighter levels would
cost. Everything was measured on the 7 corpus streams (`test/002`, 3.87 MB
of H.264), single-threaded (the `H264P_NO_THREADS` build), on the 2.1 GHz
Xeon VM the other documents use (section 9 has how).

## 1. Speed now, in one thread

KB of H.264 per second of CPU time:

| stream | H.264 | `-9` compressed | `c` | `d` |
|---|---:|---:|---:|---:|
| 00 (low rate, nearly all skipped) | 815,140 | 69.6% | 72 | 82 |
| 01 | 1,095,103 | 73.4% | 104 | 116 |
| 02 (intra, high rate) | 28,143 | 93.3% | 130 | 140 |
| 03 (much intra) | 1,181,369 | 65.6% | 71 | 79 |
| 04 (P/B) | 484,238 | 79.2% | 106 | 120 |
| 05 | 31,669 | 72.6% | 119 | 126 |
| 06 | 230,978 | 73.8% | 205 | 205 |
| **all** | 3,866,640 | 71.1% | **87** | **97** |

(Runs vary by about 5%: other runs of the same build gave 91-92 and
98-101 KB/s.) The target is 4.5-5.5x away. The threads of today's build
make `-9` 1.4x faster compressing and 1.06x decompressing; they are not
counted here.

Speed in KB/s depends on the bit rate. The coder makes about 12 binary
decisions per byte of H.264 (46.4 M on the corpus, plus 1.5 M mantissa
bits), but a low-rate stream spends much of its time per macroblock, not
per byte: 00 has 3.8 M macroblocks in 1,867 pictures, 95% of them skipped,
and 11.3 M decisions for its 815 KB, a third of them skip flags. At
400-500 KB/s a thread has 170-210 ns per decision for all of its work;
`-9` takes 860 ns decompressing and 960 compressing.

## 2. Where the time goes

Sampled with call stacks, all 7 streams together (so weighted by time):

| part | `c` | `d` | what it is |
|---|---:|---:|---|
| **coder** | **31.3%** | **34.7%** | the context mixing of each decision |
| counters | 10.1 | 10.5 | ten counters a decision: read, stretch, update |
| mixers | 7.9 | 9.5 | three 24-input mixers and the final one |
| rows: addresses, prefetch | 7.2 | 7.9 | hashing the context rows, fetching about 23 lines a decision |
| context indices | 2.6 | 2.8 | the generated index builders (`H_*_make`) |
| APMs | 2.4 | 2.8 | two |
| range coder, binarization | 1.1 | 1.2 | |
| **pixel contexts** | **19.5** | **20.9** | contexts from the reconstructed pictures |
| skip prediction | 9.0 | 9.8 | each P/B macroblock predicted as if skipped (its motion compensation included), its misses, template miss, texture, `bi_diff` |
| intra proxy, textures, edge misses | 7.5 | 7.9 | the intra estimate from the reference: modes by SATD, levels, energies, DIMD; prediction textures; edge misses |
| sign prediction | 1.9 | 2.0 | |
| maps of the last picture | 1.1 | 1.1 | energy and change by macroblock |
| **template searches** | **11.0** | **11.6** | |
| skip (best match) | 3.6 | 3.8 | `tm_best`, every P/B macroblock |
| intra proxy, blocks inside | 3.3 | 3.4 | before each 4x4/8x8 block of an I_NxN macroblock |
| intra proxy, macroblock | 2.3 | 2.4 | `proxy_mv` |
| mvd, ref_idx | 1.8 | 2.0 | |
| **reconstruction** | **10.1** | **10.7** | the decoder in the loop |
| intra prediction, residual, bookkeeping | 4.6 | 5.0 | |
| deblocking | 3.1 | 3.2 | |
| padded and transposed copies | 1.7 | 1.7 | for the searches |
| inter prediction | 0.8 | 0.8 | (a skipped macroblock's is made in the skip prediction) |
| **model contexts** | **15.4** | **16.3** | from the syntax |
| skip, the macroblock's start | 5.0 | 5.2 | neighbours, histories, records |
| coefficients | 3.4 | 3.6 | |
| motion vector prediction, direct | 3.1 | 3.3 | |
| motion, cbp, intra modes, qp, mb_type | 2.5 | 2.8 | |
| per slice and picture, records, CABAC states as predictions | 1.1 | 1.1 | |
| **syntax** | **12.7** | **5.8** | |
| parse and verify (compression's dry run) | 6.9 | | CABAC decoding, re-encoding to compare |
| the walk | 5.8 | 5.8 | the syntax, CABAC/CAVLC coding of the bins |

The decoder in the loop (pixel contexts, searches, reconstruction) is 41%
of compression and 43% of decompression. By stream (`d`):

| stream | coder | pixel contexts | searches | reconstruction | model | syntax |
|---|---:|---:|---:|---:|---:|---:|
| 00 | 32.0 | 23.8 | 8.4 | 12.0 | 18.4 | 5.3 |
| 01 | 36.9 | 19.3 | 13.7 | 8.8 | 15.3 | 6.1 |
| 02 | 75.7 | 6.8 | 2.0 | 2.0 | 7.6 | 5.9 |
| 03 | 30.6 | 22.8 | 12.9 | 12.3 | 16.1 | 5.4 |
| 04 | 37.7 | 17.0 | 12.1 | 9.3 | 16.1 | 7.8 |
| 05 | 60.6 | 9.2 | 6.8 | 7.2 | 11.9 | 4.3 |
| 06 | 55.8 | 12.9 | 8.3 | 6.7 | 12.5 | 3.8 |

The coder by family (`c`/`d`): skip and end 5.4/5.9, significance
4.7/5.2, count 3.5/3.9, sub_mb_type, ref_idx and mvd 3.3/3.7, mb_type
3.2/3.5, levels 2.8/3.1, cbp 2.6/3.0, intra modes 2.1/2.3, mb_qp_delta
1.8/2.0, signs 1.7/1.8.

## 3. What the parts buy

Each part switched off in a copy of the current source. Sizes are of the 7
streams (`-9`: 2,750,189 bytes); speed-ups are against `-9` in the same
run.

One change at a time (single runs, about 5% noise):

| without | size | `c` | `d` |
|---|---:|---:|---:|
| the APMs | +0.12% | 1.10x | 1.08x |
| the CABAC states as predictions | +0.10% | 1.00x | 0.96x |
| counters 9-10 | +0.42% | 0.99x | 0.97x |
| deblocking (the contexts see unfiltered pictures) | +0.64% | 0.98x | 1.00x |
| mixers 2-3 and the final mixer | +1.24% | 1.08x | 1.11x |
| the template searches | +1.50% | 1.13x | 1.17x |
| sign prediction | +1.59% | 1.03x | 1.03x |
| the intra proxy | +5.52% | 1.18x | 1.13x |
| counters 5-10 (most pixel contexts are there: without the decoder they cost 0.9%) | +5.63% | 1.12x | 1.12x |

Combinations (interleaved runs, best of two):

| | without | size | `c` KB/s | `d` KB/s | `c` | `d` |
|---|---|---:|---:|---:|---:|---:|
| a | APMs | +0.12% | 92 | 101 | 1.06x | 1.04x |
| b | APMs, mixers 2-3 | +1.62% | 101 | 113 | 1.17x | 1.17x |
| c | APMs, searches | +1.62% | 110 | 122 | 1.27x | 1.27x |
| d | APMs, mixers 2-3, searches, sign prediction | +4.78% | 130 | 135 | 1.50x | 1.40x |
| e | as d, and intra proxy, deblocking, CABAC states | +10.10% | 144 | 167 | 1.57x | 1.71x |
| f | as e, and the skip prediction's pixel work | +11.58% | 168 | 194 | 1.84x | 1.98x |
| g | the decoder | +15.71% | 206 | 247 | 2.37x | 2.55x |
| h | the decoder, APMs | +15.88% | 225 | 279 | 2.59x | 2.90x |
| i | the decoder, APMs, mixers 2-3 | +16.84% | 281 | 338 | 3.23x | 3.51x |
| j | the decoder, APMs, mixers 2-3, counters 5-10 | +17.31% | 306 | 401 | 3.54x | 4.17x |
| k | as j, and the CABAC states | +17.46% | 314 | 415 | 3.63x | 4.30x |

(Rows e and f come from a second run, where `-9` ran at 91 and 98 KB/s:
their KB/s are from that run.)

With the decoder kept, nothing passes 200 KB/s: the reconstruction itself
and the contexts that need it cost more than the target allows. Without
it, the coder decides the rest: from g to k, its simplification costs 1.5%
more and buys 1.5x compressing, 1.7x decompressing.

Where k loses against `-9`, by element:

| element | `-9` | k | |
|---|---:|---:|---:|
| skip, end | 222,882 | 279,953 | +25.6% |
| mb_type | 264,027 | 304,208 | +15.2% |
| cbp, transform_size_8x8_flag | 158,668 | 174,408 | +9.9% |
| intra modes | 241,396 | 263,987 | +9.4% |
| sub_mb_type, ref_idx | 47,857 | 49,759 | +4.0% |
| mvd | 342,297 | 372,377 | +8.8% |
| mvd signs | 41,668 | 52,929 | +27.0% |
| mb_qp_delta | 186,648 | 214,184 | +14.8% |
| coefficient counts | 417,150 | 472,392 | +13.2% |
| significance | 352,712 | 401,656 | +13.9% |
| levels | 261,753 | 320,206 | +22.3% |
| signs | 207,480 | 318,284 | +53.4% |
| headers, NAL units, bytes | 5,251 | 5,587 | +6.4% |
| **all** | 2,750,189 | 3,230,275 | +17.5% |

Against H.264's own CABAC, `-9` saves 28.9% and k 16.5%.

## 4. Levels

| level | has | size vs `-9` | `c` KB/s | `d` KB/s | memory |
|---|---|---:|---:|---:|---:|
| `-9` | everything (now) | 0 | 87 | 97 | 155 MB |
| `-7` | no APMs, no template searches (c) | +1.6% | 110 | 122 | |
| `-5` | the decoder only for maps, textures and edge misses; one mixer, no APMs, no deblocking (f) | +11.6% | 168 | 194 | |
| `-3` | no decoder; `-9`'s coder without its APMs (h) | +15.9% | 225 | 279 | |
| `-1` | no decoder; four counters, one mixer, no APMs, no CABAC states (k) | +17.5% | 314 | 415 | 40 MB |
| `-1`, finished | as k, with section 5 done | about +16% | about 400 | about 500 | |

Memory is the peak per process on the large streams. `-1`'s
compressed size and speed by stream (prototype k):

| stream | size vs `-9` | `c` KB/s | `d` KB/s |
|---|---:|---:|---:|
| 00 | +15.4% | 256 | 350 |
| 01 | +21.8% | 347 | 454 |
| 02 | +1.9% | 299 | 376 |
| 03 | +19.1% | 294 | 383 |
| 04 | +12.6% | 342 | 465 |
| 05 | +10.8% | 427 | 508 |
| 06 | +10.4% | 592 | 717 |

Only `-1` comes near the target: as prototyped it decompresses the corpus
at 415 KB/s (350-717 by stream) and compresses it at 314. `-9` and `-1`
are the two that matter; `-5` and `-3` fill the gap if wanted, and `-7` is
nearly `-9` at 1.25x. Each level is two more instances to build and test.

## 5. What `-1` still needs (estimates)

The prototype switches parts off but still computes much of what they
need. In it, compression's time is 40% coder, 37% syntax (parsing 10%, the
dry run's re-encoding 12%, the model pass's walk with its CABAC coding
15%) and 22% model contexts; decompression's 52% coder, 28% model contexts
and 18% syntax.

- **One CABAC encoding when compressing.** The dry run parses each NAL
  unit and re-encodes it to check that it comes back the same; the model
  pass encodes it again and drops the result (only its context states are
  used, for the CABAC-state predictions `-1` does without). With the
  model pass doing the check instead, the dry run only parses: about −12%
  of `-1`'s compression time. (A NAL unit that fails the check, rare,
  would then have the compressor start again with it stored as it is.)
- **The level's own contexts.** The prototype still builds all 17 row
  indices of `-9` (indices are 9% of its compression time and 11% of its
  decompression) and fills every context variable `-9` uses (the model
  contexts, 22% and 28%). With families of four counters and one mixer
  selector, about −8% both ways.
- **A 12-input mixer** instead of 24 inputs of which 12 are zero: about
  −4%, more decompressing.
- **Smaller tables.** The prototype maps `-9`'s 226 MB of tables (its
  peak resident size is 37-41 MB); `-1`'s own tables, sized for its
  contexts, would miss less, and bound the memory of each process.
- **Compression back**: `-1`'s four contexts per family chosen for `-1`
  (the prototype keeps `-9`'s first four), and its parameters tuned by
  `IDX/opt.pl` for its own configuration: perhaps 1-2% of the 17.5%.

Together: about 400 KB/s compressing and 500 decompressing over the
corpus. The slowest streams stay slower (00 and 03, where macroblocks,
not bytes, cost: 00's 256 and 350 KB/s would become about 340 and 410).
For them, a further model change would help: skipped macroblocks coded as
runs (as CAVLC's mb_skip_run is), one value per run instead of a decision
per macroblock (00: 3.6 M skip flags in 58,536 runs), and the model's
per-macroblock bookkeeping of a run done at once.

## 6. The template

- **`template <int f_DEC, class L> struct Model`**, `L` the level's class
  (`Lv9`, `Lv1`, ...; section 7): its switches as compile-time constants
  (`recon`, `search`, `proxy`, `signpred`, `skippx`, `deblock`,
  `cabpred`) and its families' classes. The prototype's switches
  (`X_NOREC`, `X_NOSEARCH`, `X_NOPROXY`, `X_NOSIGNP`, `X_NOSKIPPX`,
  `X_NODEBLOCK`, `X_NOCAB`) become the first; its coder's (`X_CNT`,
  `X_MIX1`, `X_NOAPM`) become the families' shapes.
- **The decoder as a member type**: `Recon` for levels with it, a stub
  without (its queries `constexpr` "not known", `active` false), chosen
  by `std::conditional`, so that a level without the decoder has none of
  its code or its pictures. (The build is C++11: no
  `if constexpr`; the stub's constant answers let the compiler drop the
  dead branches.)
- **The families as `tc_fam<f_DEC, C>`**, `C` the family's class at the
  level: its shape (counters, mixers, APMs) sizes the family and chooses
  its code (the counters read and updated in vectors of their number, one
  mixer without the final one, the APMs compiled out), and its parameters
  are the class's constants.
- **The format**: the level is a byte of the header after the version
  (a new format version); `d` reads it and runs `Model<1, L>`; `c -L`
  runs `Model<0, L>`. What `-9` codes stays as it is now.
- **Code size**: two instances per level. `-9`'s are 420 KB (compressing)
  and 380 KB (decompressing) of the 1.4 MB binary, with 210 KB of the
  reconstruction's own; a level without the decoder needs much less.
  Compile time grows with each instance: today's two take about 30 s
  (`mk.sh`'s `g++ -O2`, one translation unit, this VM).

## 7. Each level's own IDX and MOD

Today there is one profile. `IDX/<fam>.idx` and its template
`IDX/<fam>.inc` are generated into `MOD/<fam>_h.inc` (the parameters,
the threshold tables, the Volumes, the tables' struct `H_<fam>_T`) and
`MOD/<fam>_p.inc` (the index builder `H_<fam>_make`), and `model.inc`
uses them by name: `H_WIRE(F)` hands 94 of a family's tables, Volumes
and parameters to `tc_fam::wire()`, which clamps the parameters and
keeps them in members. Even the shipping build reads them from there as
it codes.

With levels, each level has its own copy of the IDX sources, generated
into its own `MOD/`. The constants of each family at each level are
wrapped into a class, as `config.hpp` wraps the `C0_`, `C1_` and `S0_`
constants into the bundles `CP_C0`, `CP_C1` and `CP_S0` of
`Counter<CP>`, and the classes are the template parameters of the
instances: `tc_fam<f_DEC, C1_skp>`, `Model<f_DEC, Lv1>`. A level's
profile (its contexts, its tables' rows, its rates, its shape) is then
edited and tuned without touching another level's, and its shipping
build has its constants folded into its own code.

### 7.1 Layout, names, generation

| path | what |
|---|---|
| `IDX/idx2inc.pl`, `opt.pl`, `import.pl`, `IDX-FORMAT.md` | the tools, shared |
| `IDX/l9/<fam>.idx`, `<fam>.inc` | `-9`: today's 15 families, moved |
| `IDX/l1/<fam>.idx`, `<fam>.inc` | `-1`: a copy of `IDX/l9` to start with |
| `MOD/l9/`, `MOD/l1/` | generated from them, kept in the repository in the shipping form as `MOD/` is now |
| `famcfg.inc` | the class of one family at one level (7.2) |
| `lv9.inc`, `lv1.inc` | a level: its `MOD/` headers, its families' classes, its own class (7.4) |

(`l7`, `l5` and `l3` likewise, for the levels between, if they are made.)

**Names.** Each level's `.idx` has the level in its `Prefix`: `Prefix
H9_skp` in `IDX/l9/skp.idx`, `Prefix H1_skp` in `IDX/l1/skp.idx`. Every
generated name is then the level's own (`H1_skp_rA`, `H1_skp_a_Volume`,
`H1_skp_T`, `H1_skp_make`), so all the levels' headers compile into the
one translation unit h264p is. So is every name `opt.pl` finds in a
tuning binary and writes into `export.!!!` (`H1_skp_rA_`), and since
`import.pl` matches an export's lines by `Prefix` and name, an export
goes back only into the level it came from. This is IDX-FORMAT.md §7's
"put the family in the `Prefix`", one step further. (A namespace per
level would keep today's names; but then two levels' patterns and
exports would have the same names, and an export of `-9`'s tuning would
import into `IDX/l1` without a word.)

**Shared.** What the levels have in common stays in `model.inc`: each
family's variables (`struct H_skp_v`), which the model computes and each
level reads a part of (7.3); the node counts (`H_skp_ND`, `_MND`,
`_NG`), which follow from the code that numbers the decisions; `tcx`.
The templates name these directly (`void %M%make( const H_skp_v & D,
tcx & X )`, `* H_skp_ND` in the `Table()` lines) rather than through
`%M%`: the one edit to the templates before they are copied.

**Generation.** `mk.sh` loops over the levels as it loops over the
families now:

- `./mk.sh mod`: every level's `MOD/` in the shipping form (after an
  edit of `IDX/`, as now);
- `./mk.sh release`: the shipping binary, all levels;
- `./mk.sh tuning 1`: `MOD/l1` in the tuning form, and a binary of `-1`
  alone (7.5);
- `./mk.sh check 1`, new: the corpus coded by the tuning and by the
  shipping build of `-1`, compared. Both builds must code identically
  (IDX-FORMAT.md §1), level by level, and 7.3 adds a way for them not
  to.

### 7.2 A family's class

`famcfg.inc`, modelled on `config.hpp`, is included once per level and
family, after the family's generated headers, with the class's name, the
prefix and the shape defined before it:

```cpp
#include "MOD/l1/skp_h.inc"
#include "MOD/l1/skp_p.inc"
#define FC_NAME C1_skp   // the class
#define FC_PFX  H1_skp_  // the prefix of the generated names
#define FC_FAM  skp      // the family: its variables, its node counts
#define FC_NC   4        // counters: A, B, C, D (10: A-E, H-L)
#define FC_NMIX 1        // mixers: 1, or 3 and the final one
#define FC_NAPM 0        // APMs: 0, 1 or 2
#include "famcfg.inc"
```

```cpp
// famcfg.inc
#define FC_CAT2(a, b) a##b
#define FC_CAT(a, b)  FC_CAT2(a, b)
#define FCX(n)        FC_CAT(FC_PFX, n)                // H1_skp_rA
#define FCF(n)        FC_CAT(FC_CAT(H_, FC_FAM), n)    // H_skp_v, H_skp_ND
#define FC_USE(f)     | 1ull << offsetof(V, f) / sizeof(int)

struct FC_NAME {
  typedef FCF(_v) V;                            // the variables, as the model fills them
  typedef FCX(T) T;                             // the level's tables
  static INLINE void make(const V& D, tcx& X) { FCX(make)(D, X); }
  static const int ND = FCF(_ND), MND = FCF(_MND), NG = FCF(_NG);
  static const int NC = FC_NC, NMIX = FC_NMIX, NAPM = FC_NAPM;
  static const int NIN = 2 * NC + 4;            // the mixers' inputs: 24 in -9, 12 in -1
  static const u64 Uses = 0 FCX(USES)(FC_USE);  // the variables the indices read (7.3)
  static const int lim[NC], tg[NC];             // the counters' limits and targets
  static const int rT, mwT, mt, rG, mwG;        // the mantissa's counters and tree, the signs'
  static const int lr, mb, lrm, mbm;            // the mixers' rates
#if FC_NAPM
  static const int rS1, rS2, qs, qf, bw;        // the APMs'
#endif
  static const int hA, hB, hD, hM, hT, hN;      // a table's rows: 0 direct, else hashed into 2^h
  static void bind(T* t, tc_tabs& b);           // the tables, in the order tc_fam takes them
};

#define set const int FC_NAME::
set lim[FC_NC] = {tc_clamp(FCX(rA), 1, CM_TMAX), tc_clamp(FCX(rB), 1, CM_TMAX),
                  tc_clamp(FCX(rC), 1, CM_TMAX), tc_clamp(FCX(rD), 1, CM_TMAX)
#if FC_NC > 4
                  , tc_clamp(FCX(rE), 1, CM_TMAX)  // ... to rL
#endif
};
set tg[FC_NC] = {tc_clamp(FCX(mwA) * 64, 0, CM_CONE / 4) /* ... */};
set lr = tc_clamp(FCX(lr), 1, 64);
set mt = tc_clamp(FCX(mt), 0, TC_MTREE);
#if FC_NAPM
set rS1 = cm_apm::limit(tc_clamp(FCX(rS1), 1, 15));
#endif
set hA = FCX(a_Volume) > (1LL << tc_hbits(FCX(hA))) ? tc_hbits(FCX(hA)) : 0;
// ...
#undef set
// bind(): t->FCX(A) to t->FCX(D), t->FCX(W), ..., under the same #if's;
// then, as config.hpp ends, #undef of FCX ... and of FC_NAME to FC_NAPM
```

- **The formulas are `wire()`'s.** Each clamp is the one
  `tc_fam::wire()` applies now (IDX-FORMAT.md §5: the consumer clamps);
  the class is where it moves. `wire()` keeps binding the tables
  (`f_skp.wire(t_skp)`), and `H_WIRE` goes.
- **One spelling, two builds.** In the shipping build `FCX(rA)` is a
  literal. With `tc_clamp` and `cm_apm::limit` made `constexpr` (C++11
  allows one `return` statement, which `limit` has to be rewritten
  into), every member is a constant expression, and
  `tc_fam`'s reads of `C::lim[k]`, `C::lr`, `C::hA` fold into its code:
  which tables are hashed, the counters' limits and targets, the APMs'
  widths, the mixers' rates, the node count, all read from the family's
  members as it codes today. In the tuning build `FCX(rA)` is
  `H1_skp_rA_.value * (1)`, a read of the object `opt.pl` patches, and
  the members are set as the program starts, from the patterns as
  patched. The template argument is a type, the same in both builds, so
  IDX-FORMAT.md §8's reference parameters (`IDXP`) are not needed. A
  cut-down copy of the pattern, compiled with g++ 13.3 `-std=c++11 -O2
  -Wall -Wextra`: a function adding `lim[k] * tg[k]`, `lr` and `hA` is
  one constant in the shipping form and loads of the class's members in
  the tuning form, with no warning in either.
- **Order.** In the tuning build the members are initialized
  dynamically, in the order of their definitions, as `config.hpp` warns.
  h264p is one translation unit, the mapping objects come first (in the
  `MOD/` headers, included before), and in `famcfg.inc` each definition
  comes after anything it reads.
- **What may shape code.** Only what is constant in both builds: the
  node counts, the shape (`NC`, `NMIX`, `NAPM`, `NIN`) and `Uses`. They
  size `tc_fam`'s members (`cm_mix<C::NIN>`, the counters' vectors, no
  APM objects in a family without APMs) and choose its code. The rest
  are values: folded in the shipping build, loaded in the tuning build,
  which is slower and codes the same.
- **The tables** stay in the generated struct (`H1_skp_T`: arrays in the
  shipping build, pointers mapped by `H1_skp_Init()` in the tuning one),
  mapped by `tc_table<>()` as now. Each level's template
  (`IDX/l1/skp.inc`) declares the tables of its shape and builds the
  indices they take (its `MakeIndex` list); `bind()` hands them to
  `tc_fam` under the same `#if`s as the members, so a template with fewer
  tables than the shape asks for does not compile.
- **Per family.** The shape is defined at each include (`famcfg.inc`
  undefines it at its end, as `config.hpp` does `CP_SSE`), so a level's
  families can differ: `-1` can keep more counters where the bits are
  (coefficient counts, significance and mvd are 40% of `-9`'s output)
  than in its small families.

### 7.3 What a level reads, and what it computes

Today the model fills every variable of a family before the family's
index builder runs. With levels, the `.idx` decides what is computed.
`idx2inc.pl` writes one more line into `_h.inc`, the variables the
family's indices read:

```cpp
// -1's skp: the copy cut to -1's shape (7.6), its pixel lines gone
#define H1_skp_USES(X) X(st) X(a3) X(b3) X(c9) X(eq) X(lf) X(af) X(cd) X(cnt) X(c1) X(col) X(run) X(k9)
```

which the class turns into `Uses`, a bit per field of `V` (above; a name
that is not a field of `V` does not compile). Every line of h264p's
`.idx` files reads one variable by name (none uses `ADD`, whose
expression the generator would have to parse), so the list is exact. A
variable is in it, in the shipping build, when a line reading it has a 1
in its pattern (a line of zeros adds nothing to its index, and the
generator already leaves it out); in the tuning build, when a live line
reads it, or a frozen one with a 1, since `opt.pl` can set any live bit.
Both are constants.

The model fills a variable only where the level reads it:

```cpp
#define H_USES(F, f) ((L::F::Uses >> (offsetof(H_##F##_v, f) / sizeof(int))) & 1)

if (H_USES(skp, c9)) {  // the skipped ones in the 3x3 window
  D.c9 = 0;
  ...
}
```

- A variable that several families read is made if one of them reads
  it: the maps' `td` and `en` and the skip prediction's `sm`, `tm`,
  `tb`, `bd` and `tx`, which skp and mbt both read.
- Only work whose one product is the variable is skipped. Where it also
  keeps state for the model, the state must not depend on it:
  `skip_miss()` keeps the skip motion for `skip()`, which makes it again
  when `skip_miss()` has not run (`smv_ok_`), so `skip_miss()` can go.
- The tuning build reads more than the shipping build (its live lines
  of zeros), so it computes more; the two code the same as long as the
  skipped work has no other product. That is what `./mk.sh check` tests.
- The switches are tied to the variables at compile time. Next to each
  family's `V` is a mask per switch of the fields it feeds (skp: `td`,
  `en`, `sm`, `tm`, `tb`, `bd` and `tx` need the decoder), and the model
  asserts them:

  ```cpp
  static_assert(L::recon || !(L::skp::Uses & H_skp_rec), "a level without the decoder reads a pixel context of skp");
  ```

  A level without the decoder whose `.idx` reads a pixel context does
  not compile; the line has to go from its `.idx`, or be frozen at zero.
  (In the cut-down copy above, adding `tm` to the list stops the build
  with that message, and a misspelt variable with "`V` has no member
  named ...".)

For `-9` this changes nothing: its indices read all but 3 of the 228
variables it fills (`ipm.blk`, `mvs.pi`, `sig.fld`, cheap ones). For
`-1` it is section 5's "level's own contexts": a context deleted from
`IDX/l1` is no longer computed by `-1`. A copy cut to `-1`'s shape
(counters A-D, mixer 1, the mantissa and the signs) still reads 157 of
the 225 variables `-9` reads, some of them from the pictures (skp's `td`
and `en`), so `IDX/l1`'s contexts have to be chosen for `-1` before it
is fast.

### 7.4 A level's class

`lv1.inc` includes `MOD/l1`'s headers (the 15 `_h.inc`, then the 15
`_p.inc`), makes the 15 families' classes through `famcfg.inc`, and
then the level's own:

```cpp
struct Lv1 {
  static const int id = 1;  // the header's level byte
  static const bool recon = false, search = false, proxy = false, signpred = false,
                    skippx = false, deblock = false, cabpred = false;
  typedef C1_nal nal;
  typedef C1_hdr hdr;
  typedef C1_skp skp;
  // ... to pcm
};
```

`Model<f_DEC, L>` reaches all of a level through `L`:

```cpp
template <int f_DEC, class L>
struct Model {
  tc_fam<f_DEC, typename L::skp> f_skp;  // and the other 14
  typename L::skp::T* t_skp;
  ...
  int skp_bit(const H_skp_v& D, int node, int v, int pr = -1) {
    tcx X;
    L::skp::make(D, X);
    f_skp.select(X);
    ...
```

Only the running level's tables are mapped (`init()` maps them, as
now), so a process's memory is its level's (section 4: 40 MB for `-1`
as prototyped), whatever else the binary holds. One thing moves out of `Model`: the
range coder, which `tc_bit<f_DEC>()` reaches as `Model<f_DEC>::rc` and
which is the same for every level, becomes one per direction.

Knobs of the model outside the families that should differ by level
(the searches' ranges, the pixel contexts' thresholds: constants in the
code today) can come under IDX the same way: `IDX/l1/lv.idx` (`Prefix
H1_lv`, numbers only) wrapped into `Lv1`, as `config0.hpp` has the SSE
cells' bundle beside the counters'.

### 7.5 Tuning one level

```sh
./mk.sh tuning 1                  # MOD/l1 in the tuning form, a binary of -1 alone
perl IDX/opt.pl opt.lst ./h264p   # it finds H1_ patterns only
cd IDX/l1 && for f in *.idx; do perl ../import.pl $f ../../export.\!\!\! > t && mv t $f; done
cd ../.. && ./mk.sh mod && ./mk.sh check 1
```

- **One level per tuning binary.** `opt.pl` patches every `!MAP!`
  pattern it finds and measures `h264p c`. Another level's patterns
  would cost a run over the corpus per bit and change nothing, and `c`
  with no level given has to run the level being tuned. A binary of one
  level compiles no slower than today's.
- **Cost.** The copy cut to `-1`'s shape has 5,588 tunable bits in 574
  patterns, `-9` 12,229 in 1,293 (`mk.sh`'s count). A pass of `opt.pl`
  codes the corpus once per bit, and `-1` codes it 3.6 times as fast: a
  pass over `-1` costs about an eighth of one over `-9`.
- **Values from another level.** A copy starts with the values of the
  level it was copied from. To give a level another's later values,
  rename the prefix in the export (`sed 's/( H9_/( H1_/' export.!!!`)
  before importing it; an export made before the move needs the same
  (`H_` to `H9_`).
- **By hand, and by `opt.pl`.** A level's structure is edited by hand:
  its contexts (the `Index` lines), its tables' rows (the frozen
  `hA`-`hN`, 2^24 rows in `-9`; set lower for `-1`, they are section 5's
  smaller tables), its shape (the `FC_` macros and its templates'
  tables). The bits of the patterns are `opt.pl`'s.

### 7.6 Getting there

Each step checked on the 96 test streams:

1. `-9` moved: `IDX/<fam>.*` to `IDX/l9/`, `MOD/` to `MOD/l9/`, `Prefix
   H_` to `H9_`, the templates naming the shared variables and node
   counts, `mk.sh` by level. Only names change: the output is the same,
   byte for byte.
2. `famcfg.inc`, `lv9.inc`, `tc_fam<f_DEC, C>`, `Model<f_DEC, Lv9>`:
   `H_WIRE` goes, `wire()` takes the tables alone instead of 94
   arguments, and the clamps move into the classes. The same output;
   the shipping build now has the parameters folded (its speed to be
   measured).
3. `Uses`, the gates and the asserts: the same output (`-9` reads what
   it fills).
4. `cp -r IDX/l9 IDX/l1`, `Prefix H1_`, `lv1.inc` with `-9`'s shape and
   switches, and the level byte in the header: a second instance, coding
   as `-9` does but for that byte. This checks the plumbing.
5. `-1` as section 4 has it: its switches off; `FC_NC 4`, `FC_NMIX 1`,
   `FC_NAPM 0`, its templates' tables and `MakeIndex` lists cut to
   match; the `Index` blocks e, h-l, s, f and w and their numbers
   deleted from `IDX/l1`, and the lines that read pixel variables. To
   compare with the prototype k: +17.46%, 314 and 415 KB/s.
6. `-1`'s own: its contexts chosen, its rows, an `opt.pl` run (section
   5).

## 8. One thread

- The coder's thread (`TcDefer`), the thread that parses ahead and the
  finishing thread (deblocking, padding, maps behind the decoding) go;
  their single-thread paths, built today with `H264P_NO_THREADS`, become
  the only ones. The output is the same.
- Memory per process: `-9` peaks at 153-157 MB on the large streams (87
  MB on 02), `-1` as prototyped at 37-41 MB (25 MB on 02).

## 9. How this was measured

- Time per part: a sampling profiler (`SIGPROF` every 200 µs) recording
  each sample's call stack through frame pointers, on a build with
  `-fno-omit-frame-pointer` and without threads; each sample attributed by
  its stack (inline frames included) to the first part it belongs to, the
  coder's samples to a family by the model's line that called it. All 7
  streams, compressed and decompressed, the samples added up.
- Trade-offs: compile-time switches in a copy of the current source (the
  ones named in section 6); each variant compressed and decompressed every
  stream back to the same bytes. CPU time single-threaded on one core; the
  combinations interleaved (each stream: every variant in turn, twice, in
  alternating order, the better of the two runs).
- Decisions counted by a counter in `bit_x` and `bitm`; memory as the
  peak resident size.
- Section 7: the tunable bits are what `mk.sh`'s tuning build reports
  for `-9`, and the same count over the lines the cut copy keeps; a
  variable counts as read where a line naming it has a 1 in its pattern;
  the compile time is one run of `mk.sh`'s `g++` command for the
  shipping build.
