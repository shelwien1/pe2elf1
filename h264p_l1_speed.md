# h264p `-1`: where its time went, and what made it faster

`-1` as first built (its own four contexts a family, its parameters
tuned, no decoder, no CABAC states, `h264p_levels.md` sections 4 and 5)
compressed the 7 corpus streams (`test/002`, 3.87 MB of H.264) at 422
KB/s and decompressed them at 528, in one thread on the 2.1 GHz Xeon VM
of the other documents (`h264p_levels.md` section 4; its tuning had made
it 6% faster both ways, its tables going from 34 MB to 13). The target
was 400-500 KB/s. Sections 1-5 profile that `-1` and list what could
make it faster: first what keeps the format (section 2), then what
changes the model (section 3), each with what it would save.

Section 6 has what was built of it and what each item gave: all of
section 2 but the stretch (2.7, which gave nothing), and 3.1 and 3.3,
which make `-1` 0.53% larger (skip runs, 3.2, cost more than they would
save: section 6.7). `-1` now compresses the corpus at 660 KB/s
and decompresses it at 742, 45% and 28% faster than before (454 and 578
measured in the same runs: the VM's speed varies from run to run and
from day to day); the
other levels code as before and got faster too, most of them compressing
(section 6.8). Section 7 profiles `-1` again.

## 1. Where the time went

Sampled with call stacks (section 5), all 7 streams, compressing (`c`,
52,558 samples) and decompressing (`d`, 43,112):

| part | `c` | `d` | what it is |
|---|---:|---:|---|
| **coder** | **38.5%** | **50.2%** | the families' context mixing |
| counters: read, stretch | 10.9 | 13.3 | four counters a decision, their probabilities stretched (an AVX2 gather) |
| mixer | 10.3 | 14.5 | one 16-input mixer: mix, squash, update |
| rows: select, prefetch | 6.0 | 7.4 | the rows of the counters and the mixer weights for each value |
| context indices | 3.8 | 4.9 | the generated index builders (`H1_*_make`) |
| range coder | 2.8 | 3.6 | |
| counters: update | 2.1 | 3.9 | |
| binarization, inputs, the rest | 2.6 | 2.5 | |
| **dry run** (`c` only) | **30.5** | | parse each NAL unit, re-encode it, compare |
| parse: CABAC engine, bit input | 5.9 | | |
| parse: syntax walk | 5.2 | | |
| parse: log of the values | 2.6 | | `LogChan`, which the model pass reads |
| re-encode: CABAC engine, bit output | 8.3 | | |
| re-encode: syntax walk | 5.9 | | |
| re-encode: log replayed | 2.2 | | |
| **model contexts** | **24.0** | **30.2** | the variables of the families' indices |
| coefficients (`block`) | 3.9 | 3.6 | |
| the macroblock's start (`begin_mb`) | 3.0 | 3.5 | a third of it a `memset` |
| skip variables (`skp_vars`, `mb_near`) | 2.4 | 4.0 | a 3x3 window of the skip map and two more neighbours, a bounds check each |
| motion vector prediction | 3.2 | 4.3 | `mv_nb`, `mv_pred`, `mv_direct`, `set_mv` |
| the history copied at each P picture (`slice_begin`) | 1.3 | 1.5 | `hprev_`, which `-1` never reads |
| the macroblock's end, mvd, the rest | 10.2 | 13.3 | |
| **the walk** | **6.5** | **19.2** | the syntax walked once more with the model's values |
| syntax | 4.9 | 7.8 | |
| bins, bit output | 1.6 | 11.4 | `d` writes the stream here; `c` only walks the bins (`Chan::nobins`): the calls, and the headers' bits |
| **other** | **0.5** | **0.5** | NAL units, I/O, the tables mapped, libc |

The coder by family (`c`/`d`, % of all the time): skip flags 8.0/10.0,
significance 4.4/5.6, coefficient counts 4.0/5.3, cbp 3.6/4.7, levels
3.5/4.7, sub_mb_type, ref_idx and mvd 3.4/4.6, mb_type 3.4/4.6, signs
2.3/3.2, intra modes 2.3/3.3, mb_qp_delta 1.7/1.9, the rest 1.9/2.2.
Compressing 00 (3.8 M macroblocks, 95% of them skipped), the skip flags
take 12.0%.

The coder spends about 75 ns on a decision compressing and 80
decompressing (its share of the measured times, over the corpus's
47.9 M decisions and mantissa bits). `-1`'s tables are 13.6 MB (34 MB
before its tuning, which left many contexts smaller), nearly all of
them under 1 MB a family (the significance's counter B is 7.1 MB, the
levels' counter C 2.1 MB), so the counters' cost is less their memory
than the gather that stretches them: the instruction after it holds
3.6% of all the samples.

By instruction counts (callgrind, compressing 04: 6.6 G instructions,
13,600 a byte), the slice coder's `macroblock()` and `bin()` lead with
6.1% and 11% (the CABAC engine inlined into it), then the model's
`block()` 5.2%, `cabac_residual()` 3.9%, the walk's `loop()` 2.5%, the
range coder's `rc_Renorm()` 2.4% and the log's `block()` 2.3%.

## 2. Faster, coding the same

### 2.1 The CABAC engines and the bit I/O

`CabacDecoder` and `CabacEncoder` (`cabac.inc`) are the standard's
flowcharts as written: renormalization one bit at a time, each bit
through `BitReader::bit()` or `BitWriter::bit()` (a `push_back` per
byte), outstanding bits counted one by one. With `SliceCoder::bin()`
around them, they take about 15% of compression (the dry run decodes
every bin, then encodes it again) and 10% of decompression (the output's
bins). A table-driven engine of the kind x264 and FFmpeg use renormalizes
with one shift by a table of the range's leading zeros, reads and writes
bytes (the decoder refilled 16 bits at a time, the encoder's carry kept
as a count of outstanding bytes), and codes bypass bins in groups; such
engines are two to three times as fast, and they code the same bits,
which the round trip tests and the dry run itself check.

Saves about 9% compressing and 6% decompressing.

### 2.2 Verify while parsing

The dry run walks each NAL unit's syntax twice: once parsing it into the
log, once re-encoding the log, the result compared with the input. A
mirror encoder fed each bin as it is decoded (the same context states,
so the same intervals) would produce the bytes during the parse, compared
with the input as they come; a mismatch falls back as now (the NAL unit
stored verbatim). What the second walk costs beyond the encoding goes:
its syntax walk and the log's replay, 8.1% of compression.

Saves about 8% compressing; 2.1 then makes the encoding the fused parse
keeps cheap.

### 2.3 No history copy at a level without the reference lists

At each new P picture `slice_begin()` copies the picture's history of
macroblocks (`hprev_.assign()`, an `MbCol` per macroblock) for
`ref_hist()`, which answers nothing while the reference lists are not
kept (`rec_.mact` false): at `-1` the copy is never read. Gating it with
`LV::refs || LV::recon` keeps every output.

Saves 1.3% compressing and 1.5% decompressing.

### 2.4 The log of values

The dry run writes each value into `LogChan`'s vectors, and the model
pass reads them back: 4.8% of compression, of it 0.7% growing the
vectors (`push_back`, the copies of reallocation). Buffers sized per NAL
unit from the previous one, written and read without per-value calls,
would halve it.

Saves about 2% compressing.

### 2.5 The skip variables

`skp_vars()` counts the skipped macroblocks in a 3x3 window of the skip
map (`c9`) and two more neighbours (`cd`), through `mb_near()`'s bounds
checks, eleven calls a macroblock (the second window, `k9`, needs the
reference lists); with the rest of the skip variables, 2.4% of compression
and 4% of decompression, most of it in skip-heavy streams. A skip map
with a border of one macroblock (no bounds checks), or the window's sum
kept as the macroblock moves along its row (three new values for each),
would cut most of it.

Saves about 1.5% compressing and 2.5% decompressing.

### 2.6 Smaller per-macroblock costs

- `begin_mb()` clears the macroblock's record with three `memset`s
  (0.9% and 1.1%); much of it is overwritten for every macroblock
  anyway. About 0.5%.
- `SliceCoder` decides at each bin, at run time, whether it reads or
  writes (`W`) and whether it codes the bins (`nobins_`); a template on
  the two would drop the branches and let the engine inline. About 1%.

### 2.7 The counters' stretch

The counters' probabilities are stretched by an AVX2 gather from
`cm_st` (`bit_x()`), four counters' P and Q in one gather, on the path
from the counters to the coded bit; 11-13% sits there. Two candidates,
both coding the same: eight scalar loads instead of the gather, or the
stretched probabilities kept beside the counter's, computed at its
update (after the bit, off that path), at the cost of a larger counter.
To be measured; perhaps 3%.

### 2.8 Together

2.1-2.6: about 22% of compression time and 11% of decompression, which
would make `-1` about 540 KB/s compressing and 595 decompressing, with
2.7 still open.

## 3. Faster, changing the model

Prices from the dumps of `IDX/ftune.py` (`h264p_levels.md` section 7.5),
on the tuning corpus, where `-1` codes 1,152,117 bytes: the lines named
set to zeros in the family's replay.

### 3.1 No motion vector prediction

`-1` reads the predicted vector in five contexts: the mvd's `sp` (the
neighbours' spread around the prediction), `zd` (the mvd were the vector
zero) and `pd` (col's vector against the prediction), the mvd signs'
`ns` and `ox`. Without them the mvds take 3,415 bytes more and their
signs 630, 0.35% of the corpus; and the model needs no vectors: the
prediction, the direct modes and the vectors stored (3.2% of compression
and 4.3% of decompression), and part of the mvd's variables, could go,
gated by a mask of what needs the vectors (as `refs` is).

Saves about 3.5% compressing and 4.5% decompressing, for 0.35%. The
other trades of the kind cost `-1` more for their speed: the reference
lists, 0.84% for 9%.

### 3.2 Skip runs

The skip flags are 8% of compression and 10% of decompression, 12% of
compressing 00, and with the end_of_slice flag (its own counter, also
one decision a macroblock) 18.4 M of the corpus's values, at 0.12 bits
each. Coding a run of skipped macroblocks as one value, as CAVLC's
mb_skip_run does, would replace most of those decisions; and the
model's work for a skipped macroblock (its start and end, the skip
variables) could be done for the run at once. A change of the skip
family's format and of the model's bookkeeping; it saves most in the
streams that are slowest per byte (00 and 03, where macroblocks, not
bytes, cost).

Perhaps 15% on 00 and 5-8% on the corpus.

### 3.3 Cheaper contexts

Other variables computed per decision, priced the same way: the skip
window `c9` (above) 0.10%, the significance's last eight positions `hb`
0.14%, the same position in the neighbouring blocks `sn` 0.03% (two
`coef_at()` lookups a position). Only `sn` costs about what it saves;
the rest are worth keeping, and 2.5 makes `c9` cheap.

## 4. Order

1. 2.3, the history copy: one condition, 1.3-1.5%.
2. 2.1 and 2.2, the dry run: the most of it, coding the same.
3. 2.4-2.6, the small ones.
4. 3.1, if `-1` may lose 0.35%, and 3.2, the larger change, for the
   skip-heavy streams.

## 5. How this was measured

- Time by part: a sampling profiler (`SIGPROF` every 200 µs of wall time,
  recording each sample's call stack through frame pointers), on a
  build of `-1` alone (`-DH264P_ONLY=1`) with the Makefile's flags and
  `-g -fno-omit-frame-pointer`, pinned to one core with nothing else
  running; each sample attributed by its stack (inline frames included)
  to the dry run (below `dry_run()`), the coder (below a `tc_fam`
  method, the family read from its template argument), the model's
  contexts (below a `Model` method, the innermost), the walk, or the
  rest.
- Instructions: callgrind, compressing 04 with the same build.
- The speed in KB/s: one thread on one core, interleaved best of two
  runs, as in `h264p_levels.md`.
- The prices of section 3: `famtune` of `-1`'s tuning build replaying
  `-1`'s dumps of the tuning corpus, with the patterns of the lines named
  set to zeros.
## 6. What was built

In the order of section 4. Each step was timed against the build before
it, the two run in turn on the corpus, best of two or three runs (runs
vary by 2-5%, so the steps' gains do not add up exactly); each step but
3.1 and 3.3 codes every stream at every level to the same bytes as
before, which `test/h264p.sh` and MD5s of all 480 outputs checked:

| | as built | `c` KB/s | `d` KB/s |
|---|---|---:|---:|
| 2.3 | the history copy gated by `refs` or `recon` | within the noise | within the noise |
| 2.1 | table-driven CABAC engines | 445 -> 488 | 562 -> 614 |
| 2.2 | the model pass checks a slice, not a write-back | 464 -> 545 | - |
| 2.4, 2.5, 2.6 | the log's buffers kept, the skip window summed, no clearing per macroblock | 523 -> 574 | 609 -> 627 |
| 2.6 | the walk a template on its mode | 562 -> 606 | 632 -> 643 |
| 2.7 | the stretch by eight loads: no gain, not kept | 562 -> 547 | 634 -> 636 |
| 3.1, 3.3 | no motion vectors, no `sn` (`-1` 0.53% larger) | 559 -> 623 | 645 -> 714 |
| 3.2 | skip runs: simulated, larger, not built | | |
| **all** | | **454 -> 660** | **578 -> 742** |

(The last row: the build before section 2 and the one after 3.1 in turn,
best of four runs, section 6.8.)

### 6.1 The CABAC engines (2.1)

A context is one byte, pStateIdx * 2 + valMPS, its transitions and
rangeTabLPS indexed by it, and a decision takes the LPS's values by
selection, not by a branch on the bin; a renormalization is one shift by
the range's leading zeros. The decoder keeps codIOffset above the input it
has read ahead (32 bits at a time); its position, the bits taken into
codIOffset, is exact, and goes back to the `BitReader` when a terminating
bin is 1 (the I_PCM samples follow it), and it fails exactly where the
bit-serial one read past the end. The encoder keeps codILow with the bits
above it not yet written, writes 32 of them at a time as bytes, and adds a
carry back into the bytes written (it never reaches before the first: the
bit `PutBit()` leaves out is always 0). Both agreed with the bit-serial
engines on 129 M random decisions, bypass and terminating bins, with I_PCM
breaks between runs, and on where truncated input fails. On 20 M bins of
mixed probabilities, alone, decoding takes 7.5 ns a bin instead of 9.5,
encoding 5.7 instead of 12.8.

### 6.2 The check of a slice (2.2)

As section 2.2 had it, a mirror encoder fed the bins as they are decoded,
the check would have been weaker than the write-back: for a run of CABAC
data that ends in a 1 bit, an encoder fed the decoded bins always writes
the input's own bits back, so it would check nothing of what the walk does
when it writes from the values, which is what decompression does. Built
instead:

- the parse records where the slice data ends and, with CABAC, its bins
  and where each run of CABAC data starts and ends (`BinRec`); where each
  run ends in a 1 bit (and the trailer is found against the input itself),
  the slice would come out as it is, if the walk writes the same values,
  bits and bins as it read;
- the model pass, which walks the write direction anyway, checks that:
  it compares each bin it would write with the one recorded, and the bits
  it writes with those outside the CABAC data (it codes no bins: at `-3`,
  `-7` and `-9`, whose model reads the CABAC states, only the contexts'
  states follow the bins, which is all the states depend on);
- should that ever fail, the model has coded the slice already, so
  compression starts again, the dry run writing back every NAL unit as
  before: the output is the same in every case. That needs the input read
  again and the output written again, so files only; through pipes the
  dry run writes back as before (`test/h264p.sh` now compresses each
  stream through pipes too and compares). The restart was tried by forcing
  a mismatch at chosen slices, and by corrupting the recorded bins and the
  input's bits as the check sees them: each was caught, and the output
  stayed the same.

A run of CABAC data that does not end in a 1 bit, or a trailer not found,
still goes through the write-back, as do the NAL units that are not slices.
It saved 17%, more than the 8% estimated: the estimate left the
write-back's encoding to the check, but without the bins (as `-1` runs it)
the check only compares them.

### 6.3 The smaller ones (2.4-2.6)

- The log: one buffer that only grows (the NAL unit's item is kept for the
  next, with its log, bin record and RBSP), written and read without
  `push_back`'s checks, a block of coefficients and the I_PCM samples at
  once.
- The skip window: summed without a bounds check per neighbour (one for
  the macroblock, its nine bytes added where it is inside the picture;
  MBAFF as before).
- `begin_mb()`: the coefficient rows the last macroblock at the address
  wrote stay (`cmask` says which rows are written), `store_coef()` clears a
  row as it starts writing it, and the three places that read the
  macroblock's own rows directly read them through `cmask`; the next
  macroblock's record is prefetched. (Prefetching its history too, and the
  clearing in vector stores instead of `rep stos`, gave nothing measurable
  and were not kept.)
- `SliceCoderT<kMode>`: 0 parses, 1 writes, 2 writes without the bins.

### 6.4 The stretch (2.7)

Eight scalar loads from `cm_st` instead of the gather code the same and
gave nothing (562 KB/s compressing with the gather, 547 with the loads;
decompressing the same), so the gather stays. The other candidate, the
stretched probabilities kept beside the counter's, was not tried: what the
counters cost is their loads, not the gather, and it would make a counter
10 bytes instead of 6.

### 6.5 No motion vectors (3.1)

A new part of the model, `mvp`: the motion vectors as the decoder has
them, predicted (`mv_pred()`, the direct and P_Skip motion) and kept in
each macroblock's record and history. `-1` is the one level without it
(the decoder and the reference lists need the vectors, which `Model`
asserts); the variables that need it were found as the other parts' were,
comparing dumps of every family's variables with and without it (on P, B,
hierarchical, spatial and temporal direct streams, and every partition):
the mvd's `pd`, `sp`, `zd` and the mvd signs' `zs`, `ds`, `ns`, and no
other. Section 3.1 had missed `zs` and `ds`, which also read the
prediction: without the six lines, retuned against their dumps, `-1` codes
the tuning corpus 0.50% larger (mvds +3,281 bytes, their signs +2,397),
test/002 0.53%; and it is 11% faster both ways, more than the 3.5% and
4.5% estimated, the vectors' share having grown as the rest shrank.

### 6.6 `sn` (3.3)

The significance's line reading `sn` (two `coef_at()` lookups a decision)
went, as priced: retuned, the significance codes the tuning corpus 80
bytes larger (0.01%).

### 6.7 Skip runs (3.2): tried in simulation, not built

The skip flags carry 6% of what `-1` makes of 00 and 19% of 03 (0.04 and
0.22 bits a decision, a skip flag or an end of slice flag, two a
macroblock); they take 10.5% of compressing and 12.3% of decompressing
(section 7), and the model's work at the start and end of each macroblock
and for the skip variables 8.3% and 9.0% more (for every macroblock,
skipped or not). A run code has to model them as well as the
per-macroblock contexts do, and these are surest where the runs are long:
in 00, a decision inside a run a copy would cover costs 0.009 bits.

Simulated in the compressor: at a macroblock where the last picture of
the kind has a run of K >= 2 skipped macroblocks from there, one decision
says whether this picture's next K are all skipped too (an adaptive
probability by K's class, whether the macroblocks above the run were all
skipped in this picture, and the last such decision's outcome); if so,
none of them codes a skip flag; if not, none is tried again within the
run. It would spare half the skip decisions of 00 and 01 and a fifth of
03's and 04's, but cost more than they did:

| stream | skip decisions | spared | they cost | the copy decisions cost |
|---|---:|---:|---:|---:|
| 00 | 3,790,320 | 1,950,980 | 2,227 bytes | 5,376 bytes |
| 01 | 1,678,320 | 750,542 | 1,738 | 4,339 |
| 03 | 3,153,600 | 680,624 | 8,919 | 20,096 |
| 04 | 413,250 | 95,312 | 1,202 | 2,662 |

0.5% of 00, 0.3% of 01 and 04, 1.2% of 03 larger, for about 4% of the
time (the skip family's share of the decisions spared). Not built: the
decisions a run code spares are the cheapest, and a run model as sure as
the per-macroblock contexts would need much of their work at each
macroblock.

### 6.8 Every level

2.1, 2.2, 2.4 and 2.6 are the engines', the dry run's, the log's and the
walk's, which every level has, so every level is faster, compressing
most: the build before section 2 and the one after 3.1 on the corpus, in
turn, two runs of two rounds, the best of the four:

| level | `c` KB/s | `d` KB/s | size |
|---|---:|---:|---|
| `-9` | 96 -> 106 (+10%) | 108 -> 108 | the same |
| `-7` | 123 -> 131 (+6%) | 128 -> 141 (+10%) | the same |
| `-5` | 191 -> 222 (+16%) | 217 -> 226 (+4%) | the same |
| `-3` | 257 -> 323 (+26%) | 316 -> 349 (+11%) | the same |
| `-1` | 454 -> 660 (+45%) | 578 -> 742 (+28%) | +0.53% |

Peak memory, compressing 03, is as it was within a megabyte, `-1`'s 28
MB instead of 30 (decompressing, 24 instead of 25).

Since, `-3`, `-7` and `-9` no longer run the CABAC encoder in the model
pass either (they had, for the contexts' states their model reads, which
depend on the bins alone): the walk keeps only the states
(`SliceCoderT<3>`, `Chan::states`), and checks the bins as `-1` and `-5`
do. The output is the same; compressing 04 at `-3` takes 1.5% fewer
instructions (9.27 G to 9.13 G), the times moving within their noise.

## 7. Where the time goes now

As section 1, on the build after section 6 (37,389 samples compressing,
33,052 decompressing):

| part | `c` | `d` | |
|---|---:|---:|---|
| **coder** | **53.8%** | **62.6%** | |
| counters: read, stretch | 16.2 | 18.1 | |
| mixer and its inputs | 15.0 | 18.7 | |
| rows: select, prefetch | 7.7 | 8.9 | |
| context indices | 4.3 | 5.7 | |
| range coder | 4.2 | 4.4 | |
| counters: update | 3.2 | 4.7 | |
| binarization, the rest | 3.2 | 2.0 | |
| **the parse** (`c` only) | **16.1** | | was the dry run's 30.5 |
| CABAC engine, bit input | 5.7 | | |
| syntax walk | 7.4 | | |
| log of the values | 2.6 | | |
| trailer, the rest | 0.4 | | |
| **model contexts** | **22.9** | **24.2** | |
| coefficients (`block`, `store_coef`, `coef_at`) | 5.9 | 5.6 | |
| the macroblock's start and end | 6.1 | 6.9 | `begin_mb`, `finish_mb` |
| skip variables | 2.2 | 2.1 | `skp_vars`, `win9` |
| the rest | 8.7 | 9.6 | |
| **the walk** | **6.5** | **12.6** | |
| syntax | 5.0 | 6.8 | |
| bins, bit output | 1.5 | 5.8 | `c` checks its bins against the parse's |
| **other** | **0.7** | **0.7** | |

The coder by family (`c`/`d`, % of all the time): skip flags 10.5/12.3,
coded block flags 5.8/6.9, significance 5.5/6.9, levels 5.5/6.8, mb_type
4.8/5.2, cbp 4.8/5.9, sub_mb_type, ref_idx and mvd 4.7/5.5, intra modes
3.4/4.1, signs 3.1/3.9, mb_qp_delta 2.2/2.4, the rest 3.5/2.7.

Half of compressing and nearly two thirds of decompressing is now the
coder, whose time goes with the number of decisions; outside it are the
parse (a sixth of compressing), the model's work per macroblock and the
walk. What would make `-1` faster next is cheaper decisions, or fewer of
them where they cost (section 6.7: not the skip flags).
