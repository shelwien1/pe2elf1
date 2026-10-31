# ac3pack

ac3pack is a lossless compressor for AC-3 and E-AC-3 elementary streams (`.ac3`, `.eac3`: Dolby
Digital and Dolby Digital Plus). It parses each syncframe down to its exponents and mantissas with
the syntax of [ac3dec](../ac3dec) and [ac3tsv](../ac3tsv), and codes the elements again with
context-mixing binary arithmetic coding, in the style of m4vpack and h264p. Decompression restores
the original stream byte for byte. The 43 streams of the [corpus](../samples) shrink to
71.23% of their size at the default level, while `xz -9e` gets 84.70%.

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
[enc] 10588668 > 4587520   53.9%  19113 frames   0.54 MB/s  48.86M RAM
```

and when it is done:

```
$ ac3pack c 7_pt_1.eac3 7_pt_1.a3p
[enc] audio: E-AC-3 (bsid 16), 48000 Hz, 3/2+LFE, 930 kbps, 1 block a frame
[enc] 19632652 > 9147827  100.0%  35438 frames  done in 37.58s  57.94M RAM at most
7_pt_1.eac3: 19632652 -> 9147827 bytes (46.59%, -9); 35438 frames (AC-3 0, E-AC-3 35438, 17719 of them dependent), 35438 packed; 0 bytes between frames
$ ac3pack d 7_pt_1.a3p 7_pt_1.eac3
[dec] audio: E-AC-3 (bsid 16), 48000 Hz, 3/2+LFE, 930 kbps, 1 block a frame
[dec] 19632652 > 9147827  100.0%  35438 frames  done in 40.73s  56.43M RAM at most
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
| `-9` | per decision ten counters, three mixers and a final one, two APMs | 71.23% | 0.48 MB/s | 0.47 MB/s | 105 MB |
| `-5` | ten counters, one mixer | 71.47% | 0.65 MB/s | 0.66 MB/s | 97 MB |
| `-1` | four counters (keeping their probabilities stretched), one mixer | 72.74% | 1.23 MB/s | 1.22 MB/s | 112 MB |

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
  right surround's left), the deltas before, the strategy, the channel, the bin.
- **mag: the mantissas** (`model_mant.inc`). A mantissa is a level of its quantiser (`q` in
  ac3tsv's text): bap 1-5 are the symmetric quantisers of 3, 5, 7, 11 and 15 levels, coded as their
  magnitude ("above 0? above 1? ..."), bap 6-15 two's complement of 5-16 bits, coded as the
  magnitude's length (a tree of 3-5 decisions), its two bits below the top one and the rest in a
  plane of single counters; then the sign. The contexts are the coefficients the neighbours give,
  each rescaled to this bin's quantiser step and exponent (in quarter levels): the bins before it in
  the block, the last block's coefficient at the bin and around it, the partner channel's at the
  bin and around it; with the bap, the exponent, the exponents' shape around the bin, the strategy,
  the block switch flag.
- **aht: E-AC-3's adaptive hybrid transform** (`model_mant.inc`): the vector indices (hebap 1-7,
  the top bits as a tree), the gains of the gain adaptive quantiser, its mantissas (each bin's six,
  one for each block) and the large mantissas after an escape, as mag's magnitudes; by the hebap,
  the gain, the block, the magnitudes of the bin's mantissa in the block before and of the block's
  at the bin before.
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
| 9 | `ac3pack`, the format version 1 and the level |
| rest | range coded data: a sequence of chunks, each a kind (a frame as records, bytes between frames, a frame kept as bytes, or the end) followed by its content, and after the end the CRC-32 of the stream |

On threads:

| Bytes | Content |
|---|---|
| 5 | `ac3pm` |
| 1 | the format version 1 |
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
| `aamss_pv` | 3361792 | 89.59% | 89.75% | 90.25% | 97.55% |
| `cg_mj6` | 3516800 | 70.72% | 71.11% | 74.89% | 97.24% |
| `cg_mj8` | 602112 | 70.97% | 71.18% | 72.17% | 88.98% |
| `cg_mj9` | 2143232 | 78.32% | 78.75% | 79.64% | 96.59% |
| `cg_mjed` | 2564352 | 77.41% | 77.71% | 78.86% | 96.62% |
| `sd_nced` | 6997760 | 90.92% | 91.17% | 91.42% | 98.61% |
| `ss_sp3` | 1344896 | 70.01% | 70.29% | 71.66% | 88.36% |
| `ss_sp5` | 2087680 | 78.87% | 79.15% | 79.70% | 95.37% |
| `ff_ac3_21_192k` | 480000 | 80.48% | 80.69% | 81.50% | 98.63% |
| `ff_ac3_21b_256k` | 640000 | 81.28% | 81.50% | 82.16% | 98.79% |
| `ff_ac3_22_320k` | 800000 | 81.23% | 81.45% | 82.11% | 98.89% |
| `ff_ac3_30_256k` | 640000 | 79.46% | 79.69% | 80.51% | 98.94% |
| `ff_ac3_31_320k` | 800000 | 80.37% | 80.62% | 81.31% | 98.96% |
| `ff_ac3_32_st_96k` | 240192 | 71.23% | 71.48% | 72.47% | 87.64% |
| `ff_ac3_441_st_192k` | 480652 | 70.96% | 71.21% | 71.95% | 89.71% |
| `ff_ac3_50_384k` | 960000 | 80.79% | 81.03% | 81.76% | 99.08% |
| `ff_ac3_51_448k` | 1120000 | 81.55% | 81.79% | 82.47% | 98.98% |
| `ff_ac3_51_640k` | 1600000 | 84.51% | 84.73% | 85.28% | 99.55% |
| `ff_ac3_mono_96k` | 240000 | 67.80% | 68.00% | 68.74% | 84.58% |
| `ff_ac3_st_cpl_96k` | 240000 | 65.53% | 65.80% | 66.88% | 84.57% |
| `ff_ac3_st_xbsi_256k` | 640000 | 68.62% | 68.89% | 69.85% | 85.45% |
| `ff_eac3_32_st_128k` | 320256 | 69.71% | 69.98% | 70.70% | 87.40% |
| `ff_eac3_441_st_192k` | 480652 | 71.85% | 72.10% | 72.63% | 89.56% |
| `ff_eac3_51_3072k` | 7680000 | 72.52% | 72.62% | 74.24% | 90.79% |
| `ff_eac3_51_448k` | 1120000 | 82.79% | 83.03% | 83.64% | 98.93% |
| `ff_eac3_51_6144k` | 15360000 | 40.57% | 40.63% | 42.60% | 49.70% |
| `ff_eac3_mono_64k` | 160000 | 66.11% | 66.28% | 66.77% | 82.58% |
| `ff_eac3_st_1536k` | 3840000 | 58.92% | 58.92% | 60.14% | 64.40% |
| `ff_eac3_st_96k` | 240000 | 67.27% | 67.55% | 68.21% | 83.92% |
| `00007` | 1464320 | 87.94% | 88.08% | 88.48% | 96.46% |
| `7_pt_1` | 19632652 | 46.59% | 46.93% | 48.34% | 59.32% |
| `buggy_aht_5.1_384` | 7014912 | 85.77% | 86.06% | 86.63% | 97.73% |
| `casablanca_aht_mono_64` | 6013664 | 82.13% | 82.37% | 83.21% | 94.80% |
| `csi_miami_5.1_256_spx` | 667490 | 84.24% | 84.44% | 85.31% | 97.88% |
| `csi_miami_stereo_128_spx` | 333664 | 84.02% | 84.20% | 84.91% | 97.33% |
| `dolby_Holi_en_2ch_128kbps_ddp` | 1362432 | 80.13% | 80.31% | 80.93% | 95.55% |
| `dolby_Holi_en_514ch_448kbps_ddp` | 4766720 | 77.53% | 77.85% | 78.60% | 94.03% |
| `dolby_Holi_en_6ch_256kbps_ddp` | 2724864 | 84.09% | 84.30% | 84.85% | 97.94% |
| `dolby_Holi_en_6ch_640kbps_ddp_joc` | 6809600 | 79.03% | 79.41% | 80.22% | 94.74% |
| `matrix2_commentary1_stereo_192` | 2191872 | 80.12% | 80.42% | 82.44% | 97.30% |
| `matrix2_english_5.1_640` | 7306240 | 87.15% | 87.56% | 87.93% | 98.60% |
| `rio_bravo_mono_64_spx` | 2097408 | 81.43% | 81.60% | 82.39% | 93.96% |
| `serenity_english_5.1_1536` | 13108224 | 84.64% | 84.83% | 87.47% | 98.35% |

| Group | Streams | Size | `-9` | `-5` | `-1` | `xz -9e` |
|---|---:|---:|---:|---:|---:|---:|
| AC-3 test files | 8 | 22618624 | 81.97% | 82.26% | 83.37% | 96.66% |
| AC-3 by FFmpeg | 13 | 8880844 | 79.05% | 79.28% | 80.01% | 96.48% |
| E-AC-3 by FFmpeg | 8 | 29200908 | 54.20% | 54.28% | 55.94% | 65.86% |
| E-AC-3 by Dolby | 14 | 75494062 | 73.67% | 73.97% | 75.20% | 87.02% |
| all | 43 | 136194438 | 71.23% | 71.47% | 72.74% | 84.70% |

Where the bytes go at `-9` (`-s`, added up over each group): each kind's share of the group's
input, and what it compresses to, as a share of the input too (the packed sizes are the model's
estimates):

| | AC-3 test files | AC-3 by FFmpeg | E-AC-3 by FFmpeg | E-AC-3 by Dolby |
|---|---:|---:|---:|---:|
| header, CRCs, trailer | 1.0% -> 0.0% | 1.0% -> 0.0% | 0.5% -> 0.0% | 2.6% -> 0.2% |
| side information | 1.6% -> 0.2% | 4.1% -> 1.1% | 0.4% -> 0.1% | 2.6% -> 0.9% |
| exponents | 11.2% -> 7.0% | 18.5% -> 12.7% | 9.2% -> 7.3% | 27.4% -> 14.6% |
| mantissas | 84.0% -> 74.6% | 75.4% -> 65.3% | 67.8% -> 46.8% | 50.8% -> 43.6% |
| of them the signs | (15.1%) | (15.5%) | (5.3%) | (9.7%) |
| AHT | - | - | - | 13.7% -> 12.9% |
| skip fields | 0.3% -> 0.2% | 0.1% -> 0.0% | - | 2.1% -> 1.2% |
| bits after the last block | 1.9% -> 0.0% | 1.0% -> 0.0% | 22.1% -> 0.0% | 0.9% -> 0.2% |
| bytes kept as bytes | - | - | - | 0.0% -> 0.0% |
| total | 100.0% -> 82.0% | 100.0% -> 79.0% | 100.0% -> 54.2% | 100.0% -> 73.7% |

The mantissas are most of every stream and stay most of the output: they come to 86-89% of their
bytes (69% in FFmpeg's E-AC-3, whose high rates leave them far below their range), and a fifth to a
quarter of what is left (a ninth in FFmpeg's E-AC-3) is their signs, a bit each, which nothing here
predicts. AHT's data comes to 94%. The exponents come to 53-80% of their bytes, the side
information to 10-35%, and the headers, the CRCs and the bits after the last block to almost
nothing, but in Dolby's streams (8% and 21%: their headers change more, and the 16-bit word their
encoders write before `auxdatae` looks random). So the streams come to 41%
(`ff_eac3_51_6144k`, whose frames are 39% unused) to 91% (`sd_nced`, DVD stereo at 448 kbps).
ac3model.md's estimate from single tables of the same contexts (~87%, ~85%, ~58% and ~82% for the
four groups) is 4-8 points above what mixing them gets.

`-5` writes 0.3% more than `-9`, and `-1` 2.1% more, at 2.6 times `-9`'s speed (`-5` at 1.4 times).

On threads, `7_pt_1.eac3` (19.6 MB, the largest stream), with the default 4 MB parts, on four
cores of the same machine:

| | Size | `c` | `d` |
|---|---:|---:|---:|
| one thread | 9147827 (46.59%) | 37.6 s, 58 MB | 40.7 s, 56 MB |
| `-t 2` (5 parts) | 9150650 (46.61%) | 22.3 s, 94 MB | 19.4 s, 87 MB |
| `-t 4` (5 parts) | 9150704 (46.61%) | 13.4 s, 178 MB | 13.7 s, 160 MB |

Each part's model starts afresh: the parts cost 0.03% of the size.

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
- **Threads:** the 43 streams round-trip with `-t 4 -c 1` (153 parts in all, which cost 0.1% at
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
