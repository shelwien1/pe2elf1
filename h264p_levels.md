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
Xeon VM the other documents use (section 8 has how).

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

- **`template <int f_DEC, int L> struct Model`**, L the level, with a
  traits struct per level: `Lv<9>`, `Lv<1>`, ... of `constexpr` switches
  (`recon`, `search`, `proxy`, `signpred`, `skippx`, `deblock`, `cabpred`)
  and the coder's shape (`counters`, `mixers`, `apms`). The switches of the
  prototype (`X_NOREC`, `X_NOSEARCH`, `X_NOPROXY`, `X_NOSIGNP`,
  `X_NOSKIPPX`, `X_NODEBLOCK`, `X_NOCAB`, `X_CNT`, `X_MIX1`, `X_NOAPM`)
  become these, read at compile time per instance.
- **The decoder as a member type**: `Recon` for levels with it, a stub
  without (its queries `constexpr` "not known", `active` false), chosen
  by `std::conditional`, so that a level without the decoder has none of
  its code or its pictures. (The build is C++11: no
  `if constexpr`; the stub's constant answers let the compiler drop the
  dead branches.)
- **The families as `tc_fam<f_DEC, NC, NMIX, NAPM>`**: the counters read
  and updated in NC / 4 vectors, one mixer without the final one, the APMs
  compiled out. Each level has its own `IDX/` set (`IDX/l1/*.idx`)
  generated into its own `MOD/` set, with its own contexts, table sizes and
  parameters; `mk.sh` and `opt.pl` work per level.
- **The format**: the level is a byte of the header after the version;
  `d` reads it and runs `Model<1, L>`; `c -L` runs `Model<0, L>`. `-9`'s
  output stays as it is now.
- **Code size**: two instances per level. `-9`'s are 420 KB (compressing)
  and 380 KB (decompressing) of the 1.4 MB binary, with 210 KB of the
  reconstruction's own; a level without the decoder needs much less, and
  compile time grows with each instance.

## 7. One thread

- The coder's thread (`TcDefer`), the thread that parses ahead and the
  finishing thread (deblocking, padding, maps behind the decoding) go;
  their single-thread paths, built today with `H264P_NO_THREADS`, become
  the only ones. The output is the same.
- Memory per process: `-9` peaks at 153-157 MB on the large streams (87
  MB on 02), `-1` as prototyped at 37-41 MB (25 MB on 02).

## 8. How this was measured

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
