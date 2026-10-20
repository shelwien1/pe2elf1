# h264p `-1`: where its time goes now, and what could still speed it up

`-1` as built (its own four contexts a family, its parameters tuned, no
decoder, no CABAC states, `h264p_levels.md` sections 4 and 5) compresses
the 7 corpus streams (`test/002`, 3.87 MB of H.264) at 444 KB/s and
decompresses them at 557, in one thread on the 2.1 GHz Xeon VM of the
other documents (its tuning made it 6% faster both ways: its tables
went from 34 MB to 13). The target was 400-500 KB/s. This document
profiles `-1` again and lists what could make it faster: first what
keeps the format (section 2), then what changes the model (section 3),
each with what it would save. Nothing here is built yet.

## 1. Where the time goes

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
would make `-1` about 570 KB/s compressing and 625 decompressing, with
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
