# ac3pack

ac3pack is a lossless compressor for AC-3 and E-AC-3 elementary streams (`.ac3`, `.eac3`: Dolby
Digital and Dolby Digital Plus). It parses each syncframe down to its exponents and mantissas with
the syntax of [ac3dec](../ac3dec) and [ac3tsv](../ac3tsv), and codes the elements again with
context-mixing binary arithmetic coding, in the style of m4vpack and h264p. Decompression restores
the original stream byte for byte. The 43 streams of the [corpus](../samples) shrink to
70.29% of their size at the default level, while `xz -9e` gets 84.70%.

## Usage

```
ac3pack c|d [options] input output
```

- `c` compresses, `d` decompresses.
- `-9`, `-5`, `-1` (with `c`) choose the model's level, from the strongest (`-9`, the default) to
  the fastest (`-1`). `d` takes the level from the file.
- `-v` (with `c`) decompresses the output afterwards and compares it with the input.
- `-s` (with `c`) shows the input and output size of each kind of syntax element.
- `-q` turns off the statistics and the progress line.
- `-p` shows the progress line even where stderr is not a terminal.
- `-t N` (with `c`) compresses on N threads (1 to 64): the stream in parts, a model for each.
  `d` takes the number of threads from the stream.
- `-c N` (with `c -t`) sets the size of a part, at least: N MB (1 to 1024), or N KB with a `k`.
  The default is 4 MB.

`input` and `output` can be `-` for stdin and stdout, except with `-v`, which reads both files
again.

Where stderr is a terminal (or with `-p`), a call shows its progress as m4vpack does: first what the
audio is (as the frames of the stream's first substream have it, and again where that changes),
then a line put over itself every half second with the bytes of the stream and of the compressed
stream so far, how far through its input it is, the frames, the speed and the memory the process
has, and at the end how long it took and the most memory it had. `-v`'s check shows a line of its
own (`[ver]`).

```
$ ac3pack c 7_pt_1.eac3 7_pt_1.a3p
[enc] audio: E-AC-3 (bsid 16), 48000 Hz, 3/2+LFE, 930 kbps, 1 block a frame
[enc] 10514432 > 4521984   53.6%  18979 frames   0.52 MB/s  75.12M RAM
```

and when it is done:

```
$ ac3pack c 7_pt_1.eac3 7_pt_1.a3p
[enc] audio: E-AC-3 (bsid 16), 48000 Hz, 3/2+LFE, 930 kbps, 1 block a frame
[enc] 19632652 > 9077922  100.0%  35438 frames  done in 40.12s  85.73M RAM at most
7_pt_1.eac3: 19632652 -> 9077922 bytes (46.24%, -9); 35438 frames (AC-3 0, E-AC-3 35438, 17719 of them dependent), 35438 packed; 0 bytes between frames
$ ac3pack d 7_pt_1.a3p 7_pt_1.eac3
[dec] audio: E-AC-3 (bsid 16), 48000 Hz, 3/2+LFE, 930 kbps, 1 block a frame
[dec] 19632652 > 9077922  100.0%  35438 frames  done in 40.84s  83.85M RAM at most
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | not an ac3pack file, or damaged data (`d`) |
| 5 | `-v` found a difference, out of memory, a level this build lacks, or an internal error |

## Levels

| Level | Model | Corpus | `c` | `d` | Memory |
|---|---|---:|---:|---:|---:|
| `-9` | per decision ten counters, three mixers and a final one, two APMs | 70.29% | 0.37 MB/s | 0.34 MB/s | 136 MB |
| `-5` | ten counters, one mixer | 70.53% | 0.53 MB/s | 0.56 MB/s | 111 MB |
| `-1` | four counters (keeping their probabilities stretched), one mixer | 71.33% | 0.91 MB/s | 0.97 MB/s | 124 MB |

The corpus is the 43 streams of [samples/](../samples) (136 MB); its size compressed is given as
a share of its size. `c` and `d` are the MB of stream per second on `sd_nced.ac3` (7.0 MB, AC-3
stereo at 448 kbps), on one thread of a 2.8 GHz Xeon, and the memory is the most the process had
(its resident set).

## How it works

The stream is cut into syncframes and the bytes between them, as [ac3tsv](../ac3tsv) cuts it: a
frame is a sync word and a header that parses, where the next frame follows it, the input ends
after it, or its CRCs hold. ac3pack parses each frame into the record ac3tsv writes as text (the
header, E-AC-3's audfrm, each block's side information, exponents and mantissas, AHT's data, the
bits after the last block), writes the record back as bits right away and compares the result with
the frame. If they match, the record is arithmetic coded; if they don't, or the frame doesn't parse
(enhanced coupling, damaged data), the frame is stored as bytes, and so is everything between
frames. So any input round-trips, including damaged streams and files that aren't AC-3 at all.

The record leaves out what follows from it, which costs nothing: the sync word, the CRCs where they
hold (a flag each, and the word only where it doesn't), the frame size, where the blocks' parts
are, the exponent groups, the coupling and SPX bands, and the bit allocation, which the walk
computes from the exponents and the parameters as a decoder does. The bit allocation is what every
mantissa is coded under: its bap tells the mantissa's quantiser.

One walk of a frame (`syntax.inc`, ac3dec's and ac3tsv's) reads it from the bits, writes it back,
and at record level hands each element to an IO that has the record's values: ac3tsv's text, or
here the model (`model.inc`), which codes each value as the walk hands it over when compressing and
decodes it into the record when decompressing. The two directions are one walk and one model, so
they can't drift apart.

### The model

Each element is turned into binary decisions, and each decision goes to one of six families
(`fam.inc`, m4vpack's and h264p's, after oggcomp). A family predicts a decision from up to ten
adaptive counters, each chosen by a context index, mixes their predictions in the logistic domain,
refines the result with APMs, and range codes the decision (`rc.inc`, oggcomp's coder as h264p has
it). `famvars.inc` lists each family's variables; `IDX/l<L>/<family>.idx` declares which of them
each index combines, and where it splits their values, for each level (the IDX format:
[IDX/IDX-FORMAT.md](IDX/IDX-FORMAT.md)); `IDX/idx2inc.pl` generates `MOD/l<L>/` from them.

Each kind of substream has its own history: AC-3, and E-AC-3 by `strmtyp` and `substreamid`, so
that a 7.1 stream's independent and dependent substreams, which alternate frame by frame, are each
coded against their own last frame. A history keeps the last two values of each header field at
each place in the frame, the last block's side information, and the last block's exponents, bit
allocation and mantissas.

- **hdr: the header and the trailer** (`model_hdr.inc`). Each field is a flag "the same as the
  last frame's", then "the same as the one before" (`convsync` and the frame sizes at 44.1 kHz
  alternate), then its difference from the last value; by the field, its place in the frame,
  whether the fields before it kept their values. The CRC flags, the container's kinds and lengths
  go here too.
- **sid: the side information** (`model_hdr.inc`): the block switch and dither flags, `dynrng`, the
  strategies, the coupling and SPX coordinates, the bit allocation parameters, the SNR offsets,
  the delta bit allocation, the skip fields' lengths. Each field at each place in the block against
  the last block's value there, and against the field's place before in the block (another
  channel's coordinates, offsets, bandwidth).
- **exp: the exponents** (`model_exp.inc`). Each delta (-2..2) as a tree of up to three
  decisions, by the last block's exponent at its bins (the least of them, against the exponent
  before the delta), the partner channel's in the block (the first full bandwidth channel, or a
  right surround's left), the deltas before, the strategy, the channel, the bin, how many deltas
  are left to the channel's last mantissa (encoders' low-pass lifts the last ones), and how many
  blocks after this one reuse the exponents (E-AC-3 sends every block's strategy first).
- **mag: the mantissas** (`model_mant.inc`). A mantissa is a level of its quantiser (`q` in
  ac3tsv's text): bap 1-5 are the symmetric quantisers of 3, 5, 7, 11 and 15 levels, coded as their
  magnitude ("above 0? above 1? ..."), bap 6-15 two's complement of 5-16 bits, coded as the
  magnitude's length (a tree of 3-5 decisions), its two bits below the top one and the rest in a
  plane of single counters; then the sign. The contexts are the coefficients the neighbours give,
  each rescaled to this bin's quantiser step and exponent (in quarter levels): the bins before it in
  the block, the last block's coefficient at the bin and around it, the partner channel's at the
  bin and around it; with the bap, the exponent, the exponents' shape around the bin, the strategy,
  the block switch flag. Then the mantissa's tile: its exponent group over the blocks that reuse its
  exponents. An encoder sets the exponent by the tile's largest coefficient, so that one comes out
  big (|m| at least 1/2), unless A/52's deltas of at most 2 lowered it or it was capped; so the
  contexts have whether the exponent may have been lowered, whether one of the tile's mantissas was
  big yet, how many came before and how many are still to come. A mantissa that comes last in its
  tile with none big yet is big: 99.8% of them in the corpus, by every encoder. And a tonal
  predictor's coefficient: a steady tone makes each bin's coefficients over the blocks a sampled
  sinusoid (x = a x1 - x2, a = 2 cos of its phase advance a block), which a per-bin predictor (integer
  arithmetic, a its running estimate) follows; its estimate and how close it came lately. The sign
  is a decision of the family too, mixed like the others, by the signs of the bins before, the
  last block's at the bin, the partner channel's, the predictor's, and whether the bin's band is
  rematrixed (2/0).
- **aht: E-AC-3's adaptive hybrid transform** (`model_mant.inc`): the vector indices (hebap 1-7,
  the top bits as a tree), the gains of the gain adaptive quantiser, its mantissas (each bin's six,
  one for each block) and the large mantissas after an escape, as mag's magnitudes; by the hebap,
  the gain, the block, the magnitudes of the bin's mantissa in the block before and of the block's
  at the bin before, and that of the last frame's at the bin and block.
- **raw: bytes** (`model_hdr.inc`): bytes kept as bytes, `addbsi`, `mixdata`, `blkstrtinfo`, the skip
  fields (in Dolby's Atmos streams, EMDF payloads: against the last skip field of the block) and the
  bits after the last block (a flag for all zero, else the bytes from the first that isn't, against
  the last frame's from its end: Dolby's streams have a 16-bit word there).

### Tuning

Every parameter of a level is tuned against the decisions its families code, as in m4vpack and
h264p. `./mk.sh dump L DIR streams...` writes what each family of level -L codes (its variables
and decisions), `./mk.sh famtune L` builds `famtune`, which codes a family's dumps again, and
`IDX/ftune.py` hill-climbs each family's parameters in a copy of it, a bit at a time, against its
dumps: a trial costs a pass over one family's decisions rather than a compression of the streams.
`IDX/import.pl` folds the winners into the `.idx` files. `./mk.sh check L streams...` confirms that
the tuning build (every parameter patchable), the shipping build (every parameter a literal) and the
build without AVX2 write the same files.

Level -9 was tuned on the first 256 KB of each of 21 streams of the corpus (5.3 MB: four of the
DVD files, seven of the FFmpeg encodes, ten of the downloaded streams), for a pass over the hdr and
sid families and part of one (20 minutes) over exp, mag and aht; raw, whose bytes are 0.6% of the
output, was left as it was. That took the families' output on the tuning set from 3947856 to
3942871 bytes (0.13%), and the corpus's at `-9` from 97034734 to 97007561 bytes (0.03%; each
stream by -0.18% to +0.04%): the first declarations were close. One parameter went back: the pass
had set the depth of mag's tree over a mantissa's top bits (`mt`) to 0, which cost FFmpeg's 1536
kbps stereo stream, not in the tuning set, 5%. `-5` and `-1` were then cut from `-9`
(`IDX/derive.py`): `-5` keeps its ten counters, mixer 1 and the mantissa plane, `-1` four of the
counters, each with `-9`'s parameters. Each `.idx` says where it was cut from.

Version 2 adds what the design notes `ac3_cm_design.md` suggested and a statistics build confirmed
on the corpus: the tiles (a mantissa that comes last in its tile with none big yet was big 99.8% of
the time, but the old contexts already knew most of that where tiles were one bin and one block),
the tonal predictor, the sign as a decision of the mag family, the exponents' distance to the last
mantissa and their reuse, the deltas that would leave 0..24 not coded, and AHT's last frame. Each
new line went where it paid on the tuning set (`./mk.sh famtune` against dumps of it), and two
passes of `IDX/ftune.py` (`FTUNE_PATS` keeping them to the new lines and the three families' counter
and mixer parameters; 83 minutes) tuned them. The pass set mag's `mt` to 0 again, which cost
`ff_eac3_st_1536k` 4.9%, and it went back to 6. The signs' variables stay out of mag's largest
indices: each context has a row for each of the family's 92 decisions, so they multiplied those
indices' memory for 0.06% of the output. `-1` keeps mag's counters a (the tile), b, c and k. The
families' output on the tuning set went from 3968072 to 3920612 bytes (1.20%), and the corpus's at
`-9` from 97007561 to 95737227 bytes (1.31%), each stream by -0.4% to -3.9% at every level. It costs
time: `-9` compresses 0.37 MB/s rather than 0.48 (136 MB rather than 105), `-1` 0.91 rather than
1.23. Version 1's files are not read (`d` says which version a file is).

### Streaming

The compressor and the decompressor are coroutines, built on Lib3's stack-copying coroutines as in
m4vpack. Each takes its input and hands over its output a 64 KB buffer at a time, so the same code
runs on files, pipes or memory, and any number of runs can go at once. The reader holds a frame and
the next one's header ahead (bytes between frames come in pieces of up to 16 MiB), and the model's
memory is its tables (mapped, so that only the rows the stream reaches take memory) and a history
of 2 MB for the kinds of substreams; nothing depends on the stream's size.

### Threads

With `-t N`, a splitter coroutine cuts the stream into parts, and MTwrap (in its parts mode) deals
them in turn to N worker threads. Each worker compresses a part with a model of its own, so the
parts compress a little worse than the whole stream. A part ends before the first frame of the
stream's first substream (an AC-3 frame, or E-AC-3's independent substream 0) at least the part size
after its start, so that a frame and the dependent substreams' frames after it stay together; where
none comes, before any frame past 4 times the part size; where no frame comes, anywhere past 8
times. The splitter finds frames as the reader does. AC-3 needs nothing of the frames before a
frame, so each part is coded as it would be in the whole stream, but by a model that starts afresh.

### File format

| Bytes | Content |
|---|---|
| 9 | `ac3pack`, the format version 2 and the level |
| rest | range coded data: a sequence of chunks, each a kind (a frame as records, bytes between frames, a frame kept as bytes, or the end) followed by its content, and after the end the CRC-32 of the stream |

On threads:

| Bytes | Content |
|---|---|
| 5 | `ac3pm` |
| 1 | the format version 2 |
| 1 | the threads |
| 4 | the part size in KB, little endian |
| rest | MTwrap's container of the workers' streams |

A worker's stream holds a 24-byte record for each of its parts: the part's size, its place in the
stream, and a CRC-32 of these fields. The compressed part follows, as a stream of the first kind
(with its level), which ends where its decoder stops reading (the coder's decoder reads exactly the
bytes its coder wrote). A record of size 0 ends the worker's stream. `d` checks the CRC of each part
and each record, and that no part is lost, added or moved. Each worker writes its stream as it codes
it, 64 KB at a time, and MTwrap puts the blocks in its container in the order they come, so `d`'s
workers find their input about when they need it. Two runs on threads can write different bytes;
the files have the same size and decompress to the same stream.

### Kept as bytes

As in ac3tsv: frames that don't parse (enhanced coupling, which no stream of the corpus has, and
damaged frames) or don't come back the same when written again, and everything that isn't a frame:
the bytes between frames, cut frames at a stream's start or end. They are coded with order 1 and 2
contexts.

## Results

Sizes relative to the input, for the 43 streams of [samples/](../samples), in the groups of
[ac3model.md](../ac3model.md) (`E-AC-3 by Dolby`: the samples.ffmpeg.org streams and the Dolby
kit's).

| Stream | Size | `-9` | `-5` | `-1` | `xz -9e` |
|---|---:|---:|---:|---:|---:|
| `aamss_pv` | 3361792 | 88.56% | 88.78% | 89.54% | 97.55% |
| `cg_mj6` | 3516800 | 69.18% | 69.82% | 73.96% | 97.24% |
| `cg_mj8` | 602112 | 69.96% | 70.29% | 71.51% | 88.98% |
| `cg_mj9` | 2143232 | 76.70% | 77.13% | 78.40% | 96.59% |
| `cg_mjed` | 2564352 | 76.30% | 76.68% | 78.03% | 96.62% |
| `sd_nced` | 6997760 | 90.12% | 90.34% | 90.73% | 98.61% |
| `ss_sp3` | 1344896 | 68.89% | 69.34% | 70.99% | 88.36% |
| `ss_sp5` | 2087680 | 77.80% | 78.10% | 78.81% | 95.37% |
| `ff_ac3_21_192k` | 480000 | 78.02% | 78.37% | 79.28% | 98.63% |
| `ff_ac3_21b_256k` | 640000 | 78.69% | 79.03% | 79.70% | 98.79% |
| `ff_ac3_22_320k` | 800000 | 78.50% | 78.83% | 79.56% | 98.89% |
| `ff_ac3_30_256k` | 640000 | 76.83% | 77.21% | 78.06% | 98.94% |
| `ff_ac3_31_320k` | 800000 | 77.63% | 78.00% | 78.81% | 98.96% |
| `ff_ac3_32_st_96k` | 240192 | 70.17% | 70.66% | 71.90% | 87.64% |
| `ff_ac3_441_st_192k` | 480652 | 69.90% | 70.28% | 71.44% | 89.71% |
| `ff_ac3_50_384k` | 960000 | 78.02% | 78.38% | 79.14% | 99.08% |
| `ff_ac3_51_448k` | 1120000 | 78.73% | 79.06% | 79.82% | 98.98% |
| `ff_ac3_51_640k` | 1600000 | 82.00% | 82.30% | 82.94% | 99.55% |
| `ff_ac3_mono_96k` | 240000 | 66.31% | 66.69% | 67.91% | 84.58% |
| `ff_ac3_st_cpl_96k` | 240000 | 64.14% | 64.69% | 65.93% | 84.57% |
| `ff_ac3_st_xbsi_256k` | 640000 | 66.86% | 67.39% | 68.77% | 85.45% |
| `ff_eac3_32_st_128k` | 320256 | 68.58% | 69.06% | 70.08% | 87.40% |
| `ff_eac3_441_st_192k` | 480652 | 70.69% | 71.10% | 72.06% | 89.56% |
| `ff_eac3_51_3072k` | 7680000 | 71.24% | 71.34% | 72.06% | 90.79% |
| `ff_eac3_51_448k` | 1120000 | 79.85% | 80.17% | 80.83% | 98.93% |
| `ff_eac3_51_6144k` | 15360000 | 40.40% | 40.42% | 40.93% | 49.70% |
| `ff_eac3_mono_64k` | 160000 | 64.95% | 65.29% | 65.84% | 82.58% |
| `ff_eac3_st_1536k` | 3840000 | 57.95% | 58.01% | 59.05% | 64.40% |
| `ff_eac3_st_96k` | 240000 | 65.79% | 66.37% | 67.18% | 83.92% |
| `00007` | 1464320 | 86.83% | 87.00% | 87.45% | 96.46% |
| `7_pt_1` | 19632652 | 46.24% | 46.52% | 46.89% | 59.32% |
| `buggy_aht_5.1_384` | 7014912 | 85.04% | 85.30% | 85.96% | 97.73% |
| `casablanca_aht_mono_64` | 6013664 | 81.61% | 81.80% | 82.75% | 94.80% |
| `csi_miami_5.1_256_spx` | 667490 | 83.68% | 83.88% | 84.81% | 97.88% |
| `csi_miami_stereo_128_spx` | 333664 | 83.56% | 83.80% | 84.57% | 97.33% |
| `dolby_Holi_en_2ch_128kbps_ddp` | 1362432 | 79.64% | 79.84% | 80.51% | 95.55% |
| `dolby_Holi_en_514ch_448kbps_ddp` | 4766720 | 76.24% | 76.57% | 77.52% | 94.03% |
| `dolby_Holi_en_6ch_256kbps_ddp` | 2724864 | 83.56% | 83.74% | 84.31% | 97.94% |
| `dolby_Holi_en_6ch_640kbps_ddp_joc` | 6809600 | 77.77% | 78.10% | 79.23% | 94.74% |
| `matrix2_commentary1_stereo_192` | 2191872 | 78.06% | 78.44% | 81.42% | 97.30% |
| `matrix2_english_5.1_640` | 7306240 | 85.22% | 85.58% | 86.09% | 98.60% |
| `rio_bravo_mono_64_spx` | 2097408 | 81.02% | 81.14% | 81.94% | 93.96% |
| `serenity_english_5.1_1536` | 13108224 | 84.19% | 84.34% | 84.77% | 98.35% |

| Group | Streams | Size | `-9` | `-5` | `-1` | `xz -9e` |
|---|---:|---:|---:|---:|---:|---:|
| AC-3 test files | 8 | 22618624 | 80.86% | 81.21% | 82.55% | 96.66% |
| AC-3 by FFmpeg | 13 | 8880844 | 76.65% | 77.02% | 77.87% | 96.48% |
| E-AC-3 by FFmpeg | 8 | 29200908 | 53.48% | 53.56% | 54.21% | 65.86% |
| E-AC-3 by Dolby | 14 | 75494062 | 72.89% | 73.14% | 73.82% | 87.02% |
| all | 43 | 136194438 | 70.29% | 70.53% | 71.33% | 84.70% |

Where the bytes go at `-9` (`-s`, added up over each group): each kind's share of the group's
input, and what it compresses to, as a share of the input too (the packed sizes are the model's
estimates):

| | AC-3 test files | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby |
|---|---:|---:|---:|---:|
| header, CRCs, trailer | 1.0% -> 0.0% | 1.0% -> 0.0% | 0.5% -> 0.0% | 2.6% -> 0.2% |
| side information | 1.6% -> 0.2% | 4.1% -> 1.1% | 0.4% -> 0.1% | 2.6% -> 0.9% |
| exponents | 11.2% -> 6.9% | 18.5% -> 12.5% | 9.2% -> 7.3% | 27.4% -> 14.5% |
| mantissas | 84.0% -> 73.6% | 75.4% -> 63.0% | 67.8% -> 46.1% | 50.8% -> 43.0% |
| of them the signs | (14.6%) | (14.3%) | (5.1%) | (9.5%) |
| AHT | - | - | - | 13.7% -> 12.8% |
| skip fields | 0.3% -> 0.2% | 0.1% -> 0.0% | - | 2.1% -> 1.2% |
| bits after the last block | 1.9% -> 0.0% | 1.0% -> 0.0% | 22.1% -> 0.0% | 0.9% -> 0.2% |
| bytes kept as bytes | - | - | - | 0.0% -> 0.0% |
| total | 100.0% -> 80.9% | 100.0% -> 76.6% | 100.0% -> 53.5% | 100.0% -> 72.9% |

The mantissas are most of every stream and stay most of the output: they come to 84-88% of their
bytes (68% in FFmpeg's E-AC-3, whose high rates leave them far below their range), and 20-23% of
what is left (11% in FFmpeg's E-AC-3) is their signs. Those were a bit each in version 1; the
partner channel's sign, the last block's and the tonal predictor's now save 2-8% of them. AHT's
data comes to 93%. The exponents come to 53-79% of their bytes, the side information to 10-35%, and
the headers, the CRCs and the bits after the last block to almost nothing, but in Dolby's streams
(8% and 21%: their headers change more, and the 16-bit word their encoders write before `auxdatae`
looks random). So the streams come to 40% (`ff_eac3_51_6144k`, whose frames are 39% unused) to 90%
(`sd_nced`, DVD stereo at 448 kbps). ac3model.md's estimate from single tables of the same contexts
(~87%, ~85%, ~58% and ~82% for the four groups) is 5-9 points above what mixing them gets.

`-5` writes 0.3% more than `-9`, and `-1` 1.5% more, at 2.5 times `-9`'s speed (`-5` at 1.4 times).

On threads, `7_pt_1.eac3` (19.6 MB, the largest stream), with the default 4 MB parts, on four
cores of the same machine:

| | Size | `c` | `d` |
|---|---:|---:|---:|
| one thread | 9077922 (46.24%) | 40.1 s, 86 MB | 40.8 s, 84 MB |
| `-t 2` (5 parts) | 9078525 (46.24%) | 25.8 s, 143 MB | 26.6 s, 136 MB |
| `-t 4` (5 parts) | 9078579 (46.24%) | 15.6 s, 270 MB | 16.6 s, 252 MB |

Each part's model starts afresh: the parts cost 0.01% of the size.

## Build

```
./mk.sh release
```

builds every level into `./ac3pack` (`-O3`, with AVX2; `PARCH= ./mk.sh release` without AVX2,
which writes the same files). Or by hand:

```
g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -ILib3 -pthread -o ac3pack ac3pack.cpp
x86_64-w64-mingw32-g++ -O3 -s -static -std=c++11 -ILib3 -o ac3pack.exe ac3pack.cpp
```

`-DAC3P_ONLY=L` builds level -L alone (much faster to compile). `MOD/` is generated from `IDX/` by
`./mk.sh mod` (it needs perl) and is kept in the repository, so a plain build needs nothing but a
C++11 compiler.

The source is C++11 that builds without exceptions or RTTI and has no dependencies besides the C
library. It is classes with their methods defined in their bodies (the walk reads and writes a
frame, the model codes its records, a run is a coroutine, a call runs c or d), and the free
functions that serve them. `ac3pack.cpp` includes the parts in order; each class over 100 lines,
and each method over 100 lines, is in a file of its own named after it (`ac3coro.inc`,
`ac3part.inc`, `model_hdr.inc`, ...): a class's where its part includes it, a method's in its
class's body. `syntax.inc` is ac3tsv's syntax section as it is (ac3dec's, without the decoder).
`Lib3/` (the coroutines), `MTwrap_v3/` (the threads), the counters, mixers and families
(`cmbase.inc`, `buf.inc`, `cm.inc` and `cm_*.inc`, `fam.inc` and `tc_*.inc`, `famcfg.inc`), the range
coder (`rc.inc`), `mapping.inc`, `masking.inc`, the tools in `IDX/` and the drivers (`coio.inc`,
`call.inc`, `progress.inc`, the threads' `mt.inc` and the parts) come from m4vpack (and through it
from h264p). The arithmetic that decides what is coded is integer arithmetic, so files are portable
between platforms.

## Verification

- **Round trips:** the 43 streams of the corpus all compress and decompress to identical bytes at
  every level. All their frames are coded as records; the only bytes kept as
  bytes are the cut frames that start the csi_miami streams (352 and 866 bytes) and the one that
  ends `casablanca_aht_mono_64` (224 bytes), as in ac3tsv.
- **Threads:** the 43 streams round-trip with `-t 4 -c 1` (153 parts in all, which cost 0.2% at
  `-5`), some with `-t 3 -c 64k` and `-t 3 -c 4k` (the smallest parts cut between any two frames),
  and through pipes.
- **Edge cases:** an empty file, 300 KB of random bytes (100.15%: kept as bytes), a binary, input
  and output through stdin and stdout; a cut, a damaged and a foreign compressed file are rejected
  with exit code 4.
- **Builds:** the tuning build (every parameter patchable), the shipping build (every parameter
  folded) and the build without AVX2 write the same files (`./mk.sh check`); so do gcc and clang
  builds, and each decompresses the other's.
- **Small buffers:** a build with a 64 KiB input buffer and 7-byte reads (`-DAC3P_SMALL_BUFFERS`)
  round-trips every stream.
- **Fuzzing,** under AddressSanitizer and UBSan (ASan's checks of the heap and the globals: the
  coroutines copy their stacks, which its stack checks take for errors):
  - 180 damaged streams (bit flips, truncation, inserted garbage, deleted and repeated bytes, fake
    sync words) all round-tripped exactly with `-v`, at `-9`, at `-1` and on threads
    (`-5 -t 3 -c 4k`);
  - 460 damaged compressed files (the same damage, and changed header bytes: the level, the
    version, the threads), 200 of them files of parts, each decoded to the original stream (its
    CRC holds) or was rejected with exit code 4, without crashes or hangs.

  Decoding checks every decoded value against what the syntax allows before the walk goes on, writes
  a frame only after the walk has written its bits back, stops when it reads past the end of the
  input, and fails on bytes after the end of the stream.

## License

The syntax (`syntax.inc`) is ac3dec's, derived from FFmpeg's libavcodec, so ac3pack is licensed
under the GNU Lesser General Public License, version 2.1 or later, not under this repository's MIT
license. `Lib3/`, `MTwrap_v3/` and the parts from m4vpack and h264p are theirs.
