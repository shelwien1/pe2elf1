# h264p: one thread, and lighter model levels

The aim: h264p runs in one thread. Several threads are used by running
several h264p processes side by side, each on a large piece of video
stream, not by splitting the work of one. (`c -t N` now does that in one
process: the stream in parts that start at IDR access units, each part
compressed by a model of its own on one of N threads; README, Threads.) Each must do 400-500 KB of
H.264 per second, compressing and decompressing, and can lose some
compression for it. The model of before stays as level `-9`, the
strongest; lighter levels are further instances of the model template, as
compression and decompression are (`Model<0, Lv9>`, `Model<1, Lv9>`).

Sections 1-3 measure where one thread's time went in the model that is
now `-9`, what each part buys in compression, and what lighter levels
would cost; sections 4-8 planned the levels. They are built: h264p runs in
one thread, `c -9`, `-7`, `-5`, `-3` and `-1` pick the level, and `-1`
compresses the corpus at 660 KB/s and decompresses it at 742, 18.1% larger
than `-9` (section 4; as first built, 422 and 528, `h264p_l1_speed.md`
has how it got faster). Everything was measured on the 7 corpus streams
(`test/002`, 3.87 MB of H.264), in one thread, on the 2.1 GHz Xeon VM the
other documents use (section 9 has how).

## 1. The speed of the model before, in one thread

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

As built (the plan, from the prototypes of section 3, in brackets), on the
corpus in one thread:

| level | has | size | vs `-9` | `c` KB/s | `d` KB/s | memory |
|---|---|---:|---:|---:|---:|---:|
| `-9` | everything | 2,717,777 | 0 | 106 | 108 | 63 MB |
| `-7` | no APMs, no template searches (c) | 2,761,576 | +1.61% (+1.6%) | 131 (110) | 141 (122) | 61 MB |
| `-5` | the decoder only for maps, textures and edge misses, unfiltered; one mixer, no APMs, no CABAC states (f) | 3,028,386 | +11.43% (+11.6%) | 222 (168) | 226 (194) | 58 MB |
| `-3` | no decoder but its reference lists; `-9`'s coder without its APMs (h) | 3,127,949 | +15.09% (+15.9%) | 323 (225) | 349 (279) | 37 MB |
| `-1` | no decoder, no motion vectors; its own four counters a family, one mixer, no APMs, no CABAC states (k) | 3,210,963 | +18.15% (about +16%) | 660 (about 400) | 742 (about 500) | 28 MB |

`-9` is the model of sections 1-3 (2,750,189 bytes there, 7 more with the
level byte each stream's header has now) with its parameters tuned again
and the model fixes of format 6 (`h264p_review.md`), 1.18% smaller; every level's parameters were tuned so, against dumps of
its families' decisions (section 7.5), which also left their tables
smaller (`-9`'s 28 MB instead of 226) and every level faster. Memory is
the peak resident size, compressing stream 03 (decompressing it takes
4-6 MB less). (Since, `-1`'s counters keep their probabilities stretched:
it decompresses 3-5% faster than the table has it and peaks at 30 MB,
`h264p_l1_speed.md` section 6.4.) `-1` by stream:

| stream | size vs `-9` | `c` KB/s | `d` KB/s |
|---|---:|---:|---:|
| 00 | +16.8% | 537 | 582 |
| 01 | +22.9% | 747 | 811 |
| 02 | +2.3% | 680 | 807 |
| 03 | +18.9% | 596 | 713 |
| 04 | +13.6% | 807 | 905 |
| 05 | +11.7% | 859 | 1001 |
| 06 | +10.7% | 1103 | 1158 |
| **all** | +18.1% | **660** | **742** |

`-1` is above the target on the corpus and on each stream: 660 KB/s
compressing and 742 decompressing (as first built, 422 and 528, 0.53%
smaller: `h264p_l1_speed.md` has what made it faster, and where its time
goes now). Its slowest are 00 and 03, where macroblocks, not bytes, cost
(00: 3.8 M macroblocks in 815 KB, 95% skipped).
The other levels fill the range between: `-7` is `-9` at 1.2-1.3 times
the speed for 1.6%, `-5` twice as fast for 11.4%, `-3` three times for
15.1%.
`-3` has the decoder's reference lists and pictures' motion without their
samples (`ReconRefs`, section 6), worth 0.8% to it at 8% of its speed;
`-1` could have them for the same 0.8% at 9% of its speed as first built,
and with them the motion vectors it has not since (0.5% for 11%,
`h264p_l1_speed.md` section 6.5).

## 5. What `-1` got

The plan's items (estimates, from the prototype), and what came of each:

- **One CABAC encoding when compressing** (−12%). Done another way: the
  dry run still parses each NAL unit and re-encodes it to check that it
  comes back the same, but the model pass, whose bits are dropped, no
  longer codes the CABAC bins at a level that reads no CABAC states
  (`Chan::nobins`): the walk only walks them. The one thing the walk took
  from its writer, the count of I_PCM alignment bits in a CABAC slice,
  comes from the dry run's log instead. No restart is needed, and the
  output is the same: `-1` compresses at 414 KB/s instead of 369 (+12%).
  (Since, the dry run no longer writes a slice back at all: the model pass
  checks it, `h264p_l1_speed.md` sections 2.2 and 6.2; and no level's
  model pass codes the bins, those whose model reads the CABAC states
  keeping only these, `Chan::states`.)
- **The level's own contexts** (−8%). `-1`'s families are its own (four
  counters, one mixer, the tables of that shape and nothing else);
  its decoder (`ReconOff`) answers every query about pictures as a
  constant, so the code that needs them folds away at compile time; the
  variables it does not read are not computed where that costs (the
  significance loop's neighbour coefficients). The last gave nothing measurable: what cost was
  gone with the decoder.
- **A 12-input mixer** (−4%). The mixers take 2 inputs per counter and 4
  more, in lanes of 8: 16 for four counters, one AVX2 vector, instead of
  24 (W0 is then 1/16, where the prototype kept 1/24).
- **Smaller tables.** `-1`'s tables are its own, 13 MB of address space
  (34 MB before its tuning; `-9`'s are 28 MB, 226 MB before theirs); it
  peaks at 30 MB compressing stream 03, 25 MB decompressing.
- **Compression back** (1-2%): 1.1%, from its contexts and its
  parameters (test/002: 3,229,830 bytes before, 3,193,962 after); the
  contexts at about the same speed, the tuned parameters 6% faster both
  ways (they leave many contexts smaller: the tables went from 34 MB to
  13).
  - its own four contexts a family. `-1` had kept `-9`'s first four
    indices (a-d), less their decoder variables. Instead, one of a-d
    swapped at a time for one of the six `-1` left out (e, h-l: several
    are syntax contexts, not the decoder's), in every family at once, each
    family taking its best swap (the families code apart, so their sizes
    add up), then a second swap from there: twelve families changed, the
    tuning corpus 0.42% smaller, test/002 0.47% (mvd signs 7.2%, qp delta
    signs 9.7%). A third round would gain less than the second's 0.04%.
  - its parameters tuned for its shape (they were `-9`'s, tuned for ten
    counters, three mixers and two APMs): `IDX/ftune.py` against dumps of
    the tuning corpus (section 7.5), every family to its last gain but
    the significance (two passes): the tuning corpus 0.74% smaller
    (8,644 bytes; skip flags 1.6%, mb_type 1.1%, significance 0.9%, qp
    delta signs 5.4%), test/002 0.64%, each stream 0.4-0.9%.
  - six counters a family instead of four (`-9`'s indices e and h, less
    their decoder variables): 0.22% smaller, 7% slower compressing. Not
    taken (tried before the two above).
  - the reference lists without the decoder (`ReconRefs`): 0.84% smaller,
    9% slower. Taken for `-3`, not for `-1` (section 4).
- **Skip runs** (for 00 and 03): not done; it changes the format of the
  skip family and the model's bookkeeping, and the corpus reaches the
  target without. (Simulated later: the skip flags it would spare are the
  cheapest, and it would make 00 0.5% larger and 03 1.2%,
  `h264p_l1_speed.md` section 6.7.)
- **No motion vectors** (later, `h264p_l1_speed.md` section 3.1): `-1`
  has not the part `mvp`, the motion vectors predicted and kept as the
  decoder has them, nor the six contexts of the mvds and their signs that
  read them, nor the significance's `sn`: 0.53% larger, 11% faster both
  ways.

## 6. The template

As built:

- **`template <int f_DEC, class LV> struct Model`**, `LV` the level's
  class (`Lv9`, `Lv7`, `Lv5`, `Lv3`, `Lv1` in `lv<L>.inc`): its switches
  as compile-time constants, `refs` (the reference lists and the
  pictures' motion), `recon` (the decoder in the loop), `search`,
  `proxy`, `signpred`, `skippx`, `deblock`, `cabpred`, `mvp` (the motion
  vectors as the decoder has them, which `refs` and `recon` need; later,
  for `-1`), and its families' classes. The prototype's switches
  (`X_NOREC`, `X_NOSEARCH`, `X_NOPROXY`, `X_NOSIGNP`, `X_NOSKIPPX`,
  `X_NODEBLOCK`, `X_NOCAB`) became these; its coder's (`X_CNT`, `X_MIX1`,
  `X_NOAPM`) the families' shapes.
- **The decoder as a member type**, by the switches: `ReconT<K>`, `K`
  what the level has of it: `Recon` (`ReconT<2>`) for levels with it;
  `ReconRefs` (`ReconT<1>`), the pictures' motion, POCs and reference
  lists without their samples, for `refs` alone; `ReconOff`
  (`ReconT<0>`) without either. `MbDecoder<K>` is the decoder, on `Dpb`
  (the pictures, their marking, POCs, lists and weights); `ReconT<K>` on
  it the model's queries of the samples. What a level has not is a
  constant, as while no picture is active: a query about the pictures'
  motion asks `motion()` (`K >= 1 && active`), one about their samples
  `act()` (`K == 2 && active`), and both fold to false where the level
  has them not, so a level without the decoder has none of its pixel code
  or its pictures' samples, and one without either none of its pictures.
  The searches and the intra proxy are reached through helpers that are
  a constant "not known" where the level has them not; sign prediction,
  the skip prediction's pixels and the CABAC states as predictions are
  gated at their one place each; deblocking is a flag of the decoder.
- **The families as `tc_fam<f_DEC, C>`**, `C` the family's class at the
  level: an even count of counters up to ten (read and updated four to an
  AVX2 vector), one mixer or three and the final one, no APM or two; the
  mixers' inputs two per counter and four more, in lanes of 8.
- **The coder** is the same for every level: `tc_rc<f_DEC>`, the range
  coder with the statistics of `-v`, the base of `Model<f_DEC, L>` (as
  built, a static `tc_rc<f_DEC>::rc` out of the model; since then the
  model's own, so that models can run on threads side by side).
- **The format** (version 5; 6 since the model fixes of `h264p_review.md`):
  the level is a byte of the header after the version; `d` reads it and
  runs `Model<1, L>`; `c -L` runs `Model<0, L>`.  What `-9` coded was as
  before, byte for byte after the header.
- **Code size**: the binary with all five levels is 5.4 MB (the
  Makefile's `-O3`; 1.4 MB before, with `-9` alone) and compiles in about
  3 minutes, one translation unit; a level alone (`-DH264P_ONLY=L`, as
  `mk.sh`'s tuning and check builds have it) in under one.

## 7. Each level's own IDX and MOD

Built as this section describes, with these differences: `famcfg.inc`
takes an even count of counters (2 to 10), has their constants as arrays
(`lim`, `tg`, `hc` the row bits, `kc` the rows) made by an X-macro of the
shape, and defines every member in every class (0 where the shape has not
the part), since `tc_fam`'s code names them in the branches the compiler
drops; counter C has a frozen row limit like the other nine; the tables
reach `tc_fam`
through the class's `tables()`, and a `static_assert` checks that the
template's mixer rows are as wide as the family's inputs. The masks of
7.3 have a part `refs` beside `rec` (the variables that need only the
reference lists: skp's `c1` and `k9`, mbt's `c1`, mvs's `rt`).
`IDX/l7`, `l5`, `l3` and `l1` were cut from `IDX/l9` by a script (each
level's shape, less the lines of variables its parts cannot make) and
are each level's own from there. `mk.sh` has `release`, `mod`, `tuning
L` and `check L`; `./mk.sh tuning 1` has 5,030 tunable bits in 525
patterns (the cut copy of 7.5 less its decoder lines; 3,904 in 415 since
the knobs its families never read are frozen), and `./mk.sh check
1` finds the tuning and the shipping build of `-1` coding the corpus
alike. Since `h264p_review.md` (D4, W4): a level's 15 families share one
template, `IDX/l<L>/family.inc` (`%F%` in it the family's name), which
states the level's shape once and ends with the family's class (it
defines the `FC_` macros of 7.2 and includes `famcfg.inc`), so `lv<L>.inc`
only includes the generated files and names the classes; and each
family's variables are listed once, with what each needs of the model's
parts, in `famvars.inc` (`H_<fam>_VARS`), which makes the structs, the
masks of 7.3 and the index builders' locals.

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
| `IDX/l9/<fam>.idx`, `family.inc` | `-9`: today's 15 families, moved, and the template they share |
| `IDX/l1/<fam>.idx`, `family.inc` | `-1`: a copy of `IDX/l9` to start with |
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
  (`IDX/l1/family.inc`) declares the tables of its shape and builds the
  indices they take (its `MakeIndex` list); `bind()` hands them to
  `tc_fam` under the same `#if`s as the members, so a template with fewer
  tables than the shape asks for does not compile.
- **Per family.** The shape is defined at each include (`famcfg.inc`
  undefines it at its end, as `config.hpp` does `CP_SSE`), so a level's
  families can differ: `-1` can keep more counters where the bits are
  (coefficient counts, significance and mvd are 40% of `-9`'s output)
  than in its small families. (As built, the level's template defines
  it for all of its families; a family that should differ gets a
  template of its own, `IDX/l<L>/<fam>.inc`, which `idx2inc.pl` takes
  before `family.inc`.)

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

The model can leave a variable the level does not read unfilled, and
does where filling it costs work of its own: sig's `ft`, `hb`, `h3` and
`sn`, per coefficient (`block()` in `model.inc`). The other variables are
cheap, or their work goes with a part the level has not (the switches,
below), and are filled whatever the level reads:

```cpp
#define H_USES(F, f) ((L::F::Uses >> (offsetof(H_##F##_v, f) / sizeof(int))) & 1)

if (H_USES(sig, sn))  // the same position in the blocks A and B
  S.sn = ...;
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
- The switches are tied to the variables at compile time. Each field
  of a family's `V` says which parts it needs (`famvars.inc`'s
  `H_skp_VARS`: skp's `td`, `en`, `sm`, `tm`, `tb`, `bd` and `tx` need
  the decoder, `NEED_REC`), a mask per part is made of that, and the
  model asserts them:

  ```cpp
  static_assert(L::recon || !(L::skp::Uses & H_skp_needs<NEED_REC>()), "a level without the decoder reads ...");
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

`lv1.inc` includes `MOD/l1`'s headers (each family's `_h.inc` and
`_p.inc`, which ends with the family's class, `famcfg.inc`'s), and then
defines the level's own:

```cpp
struct Lv1 {
  static const int id = 1;  // the header's level byte
  static const bool recon = false, search = false, proxy = false, signpred = false,
                    skippx = false, deblock = false, cabpred = false;
#define LV_FAM(F) typedef C1_##F F;  // nal, hdr, skp, ... pcm
  H264P_FAMS(LV_FAM)
#undef LV_FAM
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

As built, the levels were tuned another way, a family at a time against
dumps of its decisions:

```sh
./mk.sh dump 1 dump stream...           # what each family codes, a directory per stream
./mk.sh famtune 1                       # ../famtune, the families of -1 in the tuning form
python3 IDX/ftune.py ../famtune dump    # the hill-climb, four families at a time
cd IDX/l1 && for f in *.idx; do perl ../import.pl $f ../../export.\!\!\! > t && mv t $f; done
```

- **Why it can.** What a family codes, and what it costs, depends on
  the family's parameters and on three things they do not change: its
  decisions, the variables its indices read, and what reaches it from
  outside (the CABAC states' prediction, the hint's state). A build with
  `-DH264P_DUMP` writes the three down as it compresses (`fam.inc`): the
  variables at each `make()` (all of them, `H_USES` forced, so that any
  index can be tried), each `select()`, each decision with its outside
  prediction and hint state, a mantissa bit by its place in the number
  (so that the tree's depth `mt` can change), each sign.
  `IDX/famtune.cpp` codes one family's dumps again through the same
  `tc_fam` and prints the size: the model's own for the family, to a
  thousandth of a byte (checked at `-1` and `-9`, against `-v`'s
  accounting by family).
- **What it saves.** `opt.pl` compresses the corpus for each bit it
  tries, every family's work and the model's around them, to see one
  family's size change. A trial of `ftune.py` codes that family's
  decisions alone: at `-1`, 0.01 s (NAL units) to 1 s (skip flags) of
  CPU, where compressing the tuning corpus takes 4 s. The families do
  not interact, so they are tuned side by side, and the stream dumps of
  a trial are coded side by side too.
- **Its own measure.** `ftune.py` keeps a flip that makes the family's
  size smaller by any amount (`opt.pl` sees whole bytes of the
  corpus), and ends a family's climb with a pass that gains less than
  0.01% of the family's size (or a byte).
- **The dumps.** Each record is a letter and its numbers as zigzag
  varints: a level's dumps of the tuning corpus (1.37 MB of H.264, 10
  streams) take 325-337 MB, and replaying the largest family's (the
  significance's) takes 1-1.7 s of CPU at any level. A level's dumps
  are written in 30 s (`-9`'s in a minute), `famtune` compiles in 8 s.
- **A run cut short.** The export is rewritten at each gain, and
  `FTUNE_FROM=export` starts a run from one, so that a run stopped (the
  machine restarted under `-5` and `-7`) resumes where it was.
- **The cost.** The significance family has 1,035 tunable bits at `-1`
  and 2,200-2,400 at the other levels, far more than any other, and its
  climb is each level's last: `-1` took 75 minutes on four cores, the
  larger levels 2-4 hours, of which the significance's second pass alone
  is half an hour or more. The levels after `-1` were given two passes a
  family.
- **What it gave.** test/002 0.64-1.31% smaller at each level (`-9`
  1.16%, `-7` 1.17%, `-5` 1.31%, `-3` 1.04%, `-1` 0.64%, after its own
  contexts' 0.47%), the tuning corpus 0.75-1.77% (`-3` 1.13%, `-5`
  1.48%, `-7` and `-9` 1.77%): the lighter levels had `-9`'s
  parameters, made for `-9`'s shape, and `-9`'s own were `opt.pl`'s,
  from whole-corpus runs that saw only whole bytes. The
  tie rule (a flip that changes nothing is kept if it clears a bit) and
  the gains left many contexts smaller: the levels' tables went from
  226, 186, 93, 47 and 34 MB to 28, 22, 28, 17 and 13, and every level is
  faster for it (`-9` 103 KB/s compressing instead of 93, `-7` 125
  instead of 109, `-5` 205 instead of 188; runs vary by about 5%).

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

Done: the coder's thread (`TcDefer`), the thread that parsed ahead and the
finishing thread (deblocking, padding, maps behind the decoding) are gone;
their single-thread paths, built before with `H264P_NO_THREADS`, are the
only ones, and the output was the same. (The finishing went on row by row,
two rows behind the decoding, until `h264p_review.md` (D3) had it measured:
finishing each picture at its end codes the same in the same time, and is
what it does now.) Memory per process is in section 4.

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
- Sections 4 and 5 (the built levels): CPU time in one thread on one core,
  each stream compressed and decompressed by every level in turn, twice in
  alternating order, the better of the two runs (section 4's table: two
  such runs, each with the build of before `h264p_l1_speed.md` section 6
  in turn, the best of the four); sizes of the shipping build; memory the
  peak resident size (`wait4`). Every level round-trips
  the 98 streams of `test/h264p.sh`. The masks of section 7.3 come from
  coding the corpus with `-9` and each part switched off in turn,
  recording each context variable's range.
- Section 7: the tunable bits are what `mk.sh`'s tuning build reports
  for `-9`, and the same count over the lines the cut copy keeps; a
  variable counts as read where a line naming it has a 1 in its pattern;
  the compile time is one run of `mk.sh`'s `g++` command for the
  shipping build.
