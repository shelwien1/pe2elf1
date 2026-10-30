# m4vpack

m4vpack is a lossless compressor for MPEG-4 Part 2 video elementary streams (`.m4v`: XviD, DivX
4/5/6, libavcodec `mpeg4`, ...). It parses the stream down to the macroblock and coefficient
level and codes the syntax elements again with context-mixing binary arithmetic coding, in the
style of H.264's CABAC. At its strongest level it also decodes the pictures as it goes, so that
its contexts can look at the samples. Decompression restores the original stream byte for byte.
The streams in the tests below shrink to 60-83% of their size at the default level, while
`xz -9e` gets about 90-100%.

## Usage

```
m4vpack c|d [options] input output
```

- `c` compresses, `d` decompresses.
- `-9`, `-7`, `-5`, `-3`, `-1` (with `c`) choose the model's level, from the strongest (`-9`, the
  default) to the fastest (`-1`). `d` takes the level from the file.
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

Where stderr is a terminal (or with `-p`), a call shows its progress as h264p does: first what the
video is, then a line put over itself every half second with the bytes of the stream and of the
compressed stream so far, how far through its input it is, the VOPs, the speed and the memory the
process has, and at the end how long it took and the most memory it had. `-v`'s check shows a line
of its own (`[ver]`).

```
$ m4vpack c aq_cm.m4v aq_cm.m4p
[enc] video: 512x384, progressive, 30.01 fps, XviD build 47
[enc] 2424493 > 1703936   62.6%  722 VOPs   0.23 MB/s  32.04M RAM
```

and when it is done:

```
$ m4vpack c aq_cm.m4v aq_cm.m4p
[enc] video: 512x384, progressive, 30.01 fps, XviD build 47
[enc] 3875193 > 2739700  100.0%  1258 VOPs  done in 16.87s  32.09M RAM at most
aq_cm.m4v: 3875193 -> 2739700 bytes (70.70%, -9); 1258 VOPs (I 18, P 666, B 574, S 0), 1258 packed
$ m4vpack d aq_cm.m4p aq_cm.m4v
[dec] video: 512x384, progressive, 30.01 fps, XviD build 47
[dec] 3875193 > 2739700  100.0%  1258 VOPs  done in 15.89s  30.73M RAM at most
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | not an m4vpack file, or damaged data (`d`) |
| 5 | `-v` found a difference, out of memory, a level this build lacks, or an internal error |

## Levels

| Level | Model | Tuning set | `c` | `d` | Memory |
|---|---|---:|---:|---:|---:|
| `-9` | the decoder in the loop; per decision ten counters, three mixers and a final one, two APMs | 68.71% | 0.25 MiB/s | 0.26 MiB/s | 32 MiB |
| `-7` | the same mixing without the decoder | 71.24% | 0.39 MiB/s | 0.39 MiB/s | 22 MiB |
| `-5` | ten counters, one mixer | 71.80% | 0.56 MiB/s | 0.55 MiB/s | 20 MiB |
| `-3` | six counters, one mixer | 72.18% | 0.71 MiB/s | 0.71 MiB/s | 20 MiB |
| `-1` | four counters (keeping their probabilities stretched), one mixer | 73.00% | 0.93 MiB/s | 1.01 MiB/s | 22 MiB |

The tuning set is the first 256 KB of each of the 12 streams the levels were tuned on (2.8 MiB);
its size compressed is given as a share of its size. `c` and `d` are the MiB of stream per second
(wall time) on `aq_cm.m4v` (3.7 MiB), on one thread of a 2.8 GHz Xeon, and the memory is the most
the process had (its resident set). The decoder in the loop takes `-9` 1.6 times as long as
`-7`; each level below is 1.2-1.45 times as fast as the one above it. Format 3's single model ran at
about 1 MiB/s on the same machine.

## How it works

The stream is cut at start codes (`00 00 01 xx`). m4vpack parses each VOP into the same records
that [m4vtsv](../m4vtsv) writes as text: the VOP header, one record per macroblock (type, coded
block pattern, dquant, motion vector differences as coded, coefficient events), the video packet
headers and the bits after the last macroblock. It writes these records back as bits right away
and compares the result with the VOP. If they match, the records are arithmetic coded. If they
don't, or the VOP uses a feature the parser doesn't handle, the VOP is stored as bytes. Headers
(VOS, VO, VOL), user data and anything else that isn't a VOP are stored as bytes too. So any input
round-trips, including damaged streams and files that aren't video at all.

One set of functions codes and decodes every record: the model (`model.inc`), whose elements go as
its families do, each in a file of its own (the headers `model_hdr.inc`, the residual
`model_res.inc`, the vectors `model_mv.inc`, the macroblocks `model_mb.inc`). Each function takes
the values when compressing and returns them when decompressing, so the two directions can't drift
apart (the same idea as h264p's symmetric syntax coder). The bits are handled the same way: one walk
per record (`syntax.inc`) both reads it from the bits and writes it back, so the parser and the
writer can't disagree either, and the model asks the same helpers which fields a record has.

### The model

Every syntax element is turned into binary decisions, following H.264's CABAC through the
correspondences worked out in [h264-counterparts.md](../m4vtsv/h264-counterparts.md). Each decision
goes to one of 13 families (`fam.inc`, from h264p, after oggcomp): headers, raw bytes, skipping,
macroblock types and flags, coded block patterns, dquant, vector differences and their signs, intra
DC differences, significance, levels, signs and escape modes. A family predicts a decision from up
to ten adaptive counters, each chosen by a context index, mixes their predictions in the logistic
domain with up to three weight sets (and a final mixer over those), and refines the result with up
to two APMs. Where the model has a guess at a number (a hint: the magnitude that would repeat the
co-located vector, the intra estimate's level), each decision of the number's binarisation also
sees what the hint's own binarisation decides there.

`famvars.inc` lists the variables the model gives each family. A level's context indices over those
variables, and every parameter of its families (adaptation rates, mixer learning rates, table sizes,
how each variable is bucketed), are declared in `IDX/l<L>/<family>.idx` (the format:
`IDX/IDX-FORMAT.md`, from h264p). `IDX/idx2inc.pl` turns them into `MOD/l<L>/`, which the build
includes; `levels.inc` and `lv<L>.inc` put each level's families together.

The model gives the variables as they are: counts, runs and magnitudes capped where few values lie
above the cap, values of a wide range on half octaves. So every split of a variable is a threshold
of an index in the `.idx` files, which tuning moves. What shapes a variable itself (the signs the
search covers, the template's size, the search ranges, the history lengths) is a named constant in
the source, documented where it is defined: changing one changes the dumps the tuner works from, so
it is not one of the parameters the tuner can try (as in h264p).

The variables, in short (`famvars.inc` has them all):

| Element | Contexts |
|---|---|
| macroblock type | the left, above, above-left, above-right and co-located (last anchor VOP) macroblocks, the run of skipped or direct macroblocks, whether the predicted vector is zero |
| coded block pattern | per 8x8 block: the left and above blocks, the size of the vector differences, whether the motion is sub-pel, the co-located macroblock, the blocks coded so far, the quantiser |
| vector difference | the neighbours' magnitudes (as CABAC's mvd), the other component, the co-located macroblock, the spread of the median's candidates, the difference that would repeat the co-located vector (its magnitude the hint) |
| intra DC difference | the previous and neighbouring blocks' sizes, counts and signs, whether the block has AC coefficients |
| coefficients | CABAC's residual coding (significance with "last" flags in scan order, levels in reverse order, signs, the escape mode where it isn't the usual one), by position and block class, the neighbouring blocks' counts (CAVLC's nC), the coefficients coded so far (histories of significance flags and magnitudes, as h264p), the left and above blocks' coefficients at the same position, the frequency neighbours already coded (as HEVC), the quantiser, the vector size, the co-located block, and for Cr the Cb block; signs by the signs coded before them, the frequency neighbours' (weighted by their magnitudes) and the neighbouring blocks' |

The vectors the contexts use are the decoder's own: one motion state (`motion.inc`, m4vdec's code)
makes them at every level from the differences coded, with FFmpeg's median prediction and its video
packet rules (down to the stored vector it zeroes at a packet's start), field and 4MV vectors, GMC's
warped vector, the B-VOP predictors, and direct mode scaled by the display times as FFmpeg keeps
them (16 bits). The decoder in the loop of `-9` moves its predictions by the same vectors.

### The decoder in the loop (level -9)

h264p's largest gains come from reconstructing the pictures inside the model, and so do
m4vpack's. Level -9 runs m4vdec's decoder (a port of FFmpeg's MPEG-4 Part 2 decoder) inside the
model (`recon.inc`), driven by the records instead of the bits: as a macroblock is coded, its
prediction is made in the picture once its type and vectors are known, and each block is
reconstructed as soon as its coefficients are. So when a block is coded, the samples above and to
the left of it are decoded, and its own prediction is in place. Everything the decoder does is
there: DC/AC prediction, H.263 and MPEG quantisation with custom matrices, the simple and XviD
IDCTs (chosen as FFmpeg chooses), half and quarter sample motion compensation, 4MV, field motion and
field DCT, B-VOP direct mode, GMC with 1-3 warp points, video packets, and the encoder bug
workarounds (DivX, XviD and libavcodec builds, read from the user data). Its pictures are
bit-exact with m4vdec's on 45 test streams (9,758 frames).

The contexts it gives, and what each is worth: the tuning set's size with the context switched off
in the tuned model, against 68.71% with all of them. (A model tuned with a context loses a little
more without it than one tuned without it: `-7`, the same model without the decoder and tuned on
its own, gets 71.24%.)

| Context | Elements | Without it |
|---|---|---:|
| **sign prediction**: a block's first 8 signs (in scan order) are searched for the combination whose block continues the decoded samples above and left of it best: the sum over its top row and left column of the distance from 2 R1 − R2, and from R1 (R1, R2 the two decoded rows or columns beyond it; MPEG-4 has no deblocking, so the extrapolation is noisy where the samples are); the other signs go first, then these, with the sign each of the two searches prefers, by how much, and their rank | signs | 69.50% (+0.79) |
| **the intra estimate**: an intra macroblock's blocks as the reference has them where its neighbours' motion (refined by matching the decoded samples above and left of it, the template) puts it, transformed and quantised as the encoder would, the intra prediction taken off: the DC difference's hint and sign, the level at each position (the levels' hint), its sign, the AC coefficients per block for the coded block pattern; trusted by the template's match, then by how far the macroblock's blocks so far were from it | intra DC, coefficients, coded block pattern | 69.32% (+0.61) |
| **an inter block's prediction**: its texture, and how far its edges are from the decoded samples next to them, against the quantiser | coded block pattern, coefficients | 69.14% (+0.43) |
| intra blocks with AC prediction in their own scan (vertical or horizontal, by the DC prediction), which the decoder knows before their coefficients | coefficients | 69.01% (+0.31) |
| the prediction's spectrum: its transform at each position against the quantiser, and which way its texture runs | coefficients | 68.97% (+0.26) |
| skipping: how much the area changed since the reference (the decoded samples around the macroblock against the reference's), how well the reference's macroblock (in B-VOPs the direct prediction) continues them | skip, macroblock type | 68.79% (+0.08) |
| the vector the template matches best (a search around the vector's prediction, the co-located vector and none, then half samples) | vector differences, their signs | 68.78% (+0.08) |
| the levels an inter block's edges ask for: their miss projected onto each coefficient's pattern | coefficients | 68.78% (+0.07) |
| all of them | | 71.40% (+2.70) |

Signs, which neither MPEG-4 nor CABAC models, gain most: on the samples, the sign a search prefers
is right for 64-80% of the signs it covers (by stream and by which of the two).

### Tuning

Every parameter of a level is tuned against the decisions its families code, as in h264p.
`./mk.sh dump L DIR streams...` writes what each family of level -L codes (its variables and
decisions), `./mk.sh famtune L` builds `famtune`, which codes a family's dumps again, and
`IDX/ftune.py` hill-climbs each family's parameters in a copy of it, a bit at a time, against its
dumps: a trial costs a pass over one family's decisions rather than a compression of the streams.
`IDX/import.pl` folds the winners into the `.idx` files. `./mk.sh check L streams...` confirms that
the tuning build (every parameter patchable), the shipping build (every parameter a literal) and the
build without AVX2 write the same files.

The levels were tuned on the first 256 KB of each of the 12 streams of the tuning set: `-9` for
four passes over each family, then `-7` from `-9`'s values (two passes over the families whose
contexts differ). When the model's variables became raw (every split a threshold in the `.idx`
files), both were tuned again, the two largest families (sig and lvl) for about a pass and the
others for two. `-5`, `-3` and `-1` were then cut from `-7`: `-5` keeps its ten counters, `-3` and
`-1` the six and four whose loss (each switched off in turn) costs each of `-7`'s families most;
then each was tuned for a pass.

### Streaming

The compressor and the decompressor are coroutines, built on Lib3's stack-copying coroutines as in
h264p. Each takes its input and hands over its output a 64 KB buffer at a time, so the same code
runs on files, pipes or memory, and any number of runs can go at once. The input buffer grows
with the longest chunk (to 64 MiB), and the macroblock tables and level -9's pictures grow with
the largest picture. Nothing else depends on the stream's size.

### Threads

With `-t N`, a splitter coroutine cuts the stream into parts, and MTwrap (in its parts mode) deals
them in turn to N worker threads. Each worker compresses a part with a model of its own, so the
parts compress a little worse than the whole stream. A part ends:
- before the first I-VOP at least the part size after its start, or before the run of headers
  (VOS, VO, VOL, user data, GOV) right before that I-VOP;
- where no I-VOP comes, before the first VOP that isn't a B-VOP at least 4 times the part size
  after its start, since a B-VOP's syntax depends on the VOP before it;
- where no VOP comes, at any start code past 6 times the part size, or anywhere past 8 times.

VOPs that aren't coded (the placeholders that packed B-frames leave) are never cut points. A part
gets a copy of the headers the parser reads (the last VOL, and the last DivX and XviD user data)
if it doesn't define them itself before its first VOP. So each part parses as it does in the
whole stream. `d` removes the copies.

[m4vsplit](../m4vsplit) splits a stream the same way for other tools, into parts that are valid
streams.

### File format

| Bytes | Content |
|---|---|
| 9 | `m4vpack`, the format version 4 and the level |
| rest | arithmetic coded data: a sequence of chunks, each a kind (VOP as records, bytes, or end) followed by its content, and after the end the CRC-32 of the stream |

On threads:

| Bytes | Content |
|---|---|
| 5 | `m4vpm` |
| 1 | the format version 4 |
| 1 | the threads |
| 4 | the part size in KB, little endian |
| rest | MTwrap's container of the workers' streams |

A worker's stream holds a 48-byte record for each of its parts: the record's size, the part's size,
where the headers copied into it are, the part's place in the stream, and a CRC-32 of these fields.
The compressed part follows, as a stream of the first kind (with its level). An empty record ends
the worker's stream. `d` checks the CRC of each part and each record, and that no part is lost,
added or moved.

MTwrap writes the workers' blocks in the order they finish, so two runs on threads can write
different bytes. The files have the same size and decompress to the same stream.

### Kept as bytes

Everything m4vtsv keeps raw is stored as bytes here, using order 0 and order 1 contexts:
- **Unsupported features:** data partitioning (except in B-VOPs, which don't use it),
  non-rectangular shapes, static sprites, sprite brightness change, newpred, reduced resolution,
  scalability, and the studio profile.
- **Damaged VOPs:** VOPs that don't parse or don't reproduce.
- **Size limits:** VOPs longer than 64 MiB, and VOPs with more than 1 MiB after the last macroblock.

## Results

Sizes relative to the input. The first six are the avip samples: four XviD streams (builds 41-47,
three with packed B-frames) and two DivX streams. The others were made with FFmpeg, using libxvid
(XviD 1.3) or its own `mpeg4` encoder. Format 3 is the previous m4vpack (one model, no levels).

| Stream | Size | xz -9e | format 3 | `-9` | `-7` | `-5` | `-3` | `-1` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| aq_cm.m4v | 3875193 | 97.2% | 76.3% | 70.7% | 74.8% | 75.5% | 75.9% | 76.7% |
| aria_cm.m4v | 5097686 | 98.9% | 79.0% | 73.8% | 78.0% | 78.7% | 79.0% | 80.0% |
| ispure03.m4v | 5864104 | 95.8% | 73.0% | 67.5% | 71.6% | 72.3% | 72.6% | 73.3% |
| sdova_ed.m4v | 4717732 | 89.6% | 65.7% | 60.1% | 65.0% | 65.7% | 65.9% | 66.5% |
| susi_small1_divx.m4v | 90887 | 100.1% | 83.4% | 77.4% | 81.9% | 82.3% | 82.5% | 83.1% |
| susi_small2_divx.m4v | 1556817 | 100.0% | 85.1% | 83.1% | 84.6% | 85.1% | 85.2% | 85.5% |
| xvid.m4v | 257813 | 97.7% | 74.4% | 68.9% | 70.2% | 71.1% | 71.6% | 72.6% |
| lavc_ilace.m4v (interlaced) | 1015896 | 90.9% | 68.6% | 62.9% | 64.5% | 65.4% | 65.8% | 67.1% |
| xvid_gmc.m4v (GMC) | 833291 | 99.8% | 76.9% | 70.7% | 73.7% | 74.3% | 75.0% | 75.8% |
| xvid_qpel_4mv.m4v (quarter-pel, 4MV) | 383624 | 95.5% | 74.0% | 68.9% | 70.3% | 70.8% | 71.3% | 72.1% |

Where the bits go in `aq_cm.m4v` at `-9` (`-s`; the packed sizes are the model's estimates):

```
  bytes                                  input     packed
  headers, raw chunks                    11752       1633  13.90%
    raw chunks                                        202
    VOP and video packet headers, tails              1431
  macroblock types, cbp, dquant         357692     130322  36.43%
    types, flags, dquant                            55598
    coded block patterns                            74724
  motion vectors                        191806     141983  74.02%
  intra DC                              256787     195482  76.13%
  coefficients                         3057155    2270169  74.26%
    positions                                      921642
    levels                                        1020004
    signs                                          328352
    escape modes                                      171
  total                                3875193    2739590  70.70%
```

`-9` writes 2.4-8.5% less than format 3, 7.3% less on this stream. Coefficients make up most of a
stream (79% of this one), and their signs change most: 27% fewer bytes than format 3's here (328352
against 450410), from the sign prediction; the positions and levels take 2.5% and 2.8% fewer, the
macroblock headers 12% fewer. The decoder in the loop is worth 5.4-7.5% of the output against `-7`
on the avip samples (but 1.8% on `susi_small2_divx.m4v`), and 2-4% on the FFmpeg streams. `-1`, the
fastest level, is within a point of format 3 on the avip samples (worse on five of the six) and 1-2
points better on the FFmpeg streams.

On threads, with five of the avip samples concatenated (21 MB, four cores):

| | Size | `c` | `d` |
|---|---:|---:|---:|
| one thread | 14580473 (69.06%) | 104.0 s, 38 MiB | 107.6 s, 37 MiB |
| `-t 2` | 14607143 (69.19%) | 58.3 s, 96 MiB | 61.2 s, 82 MiB |
| `-t 4` | 14607245 (69.19%) | 38.4 s, 174 MiB | 42.1 s, 149 MiB |

At `-9` (the default), with the default 4 MB parts: the stream has 5 parts, which cost 0.18% of
the size, each part's model starting afresh.

## Build

```
./mk.sh release
```

builds every level into `./m4vpack` (`-O3`, with AVX2; `PARCH= ./mk.sh release` without AVX2,
which writes the same files). Or by hand:

```
g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -ILib3 -pthread -o m4vpack m4vpack.cpp
x86_64-w64-mingw32-g++ -O3 -s -static -std=c++11 -ILib3 -o m4vpack.exe m4vpack.cpp
```

`-DM4V_ONLY=L` builds level -L alone (much faster to compile). `MOD/` is generated from `IDX/` by
`./mk.sh mod` (it needs perl) and is kept in the repository, so a plain build needs nothing but a
C++11 compiler.

The source is C++11 that builds without exceptions or RTTI and has no dependencies besides the C
library. `m4vpack.cpp` includes the parts in order; each class (but the small counters of
`cm.inc`), and each function over 100 lines, is in a file of its own named after it (`m4vcoro.inc`,
`code_block.inc`, ...), which the file of its part includes where it belongs. `Lib3/` (the
coroutines), `MTwrap_v3/` (the threads), the counters, mixers and families (`cmbase.inc`,
`buf.inc`, `cm.inc` and `cm_*.inc`, `fam.inc` and `tc_*.inc`, `famcfg.inc`), `mapping.inc`,
`masking.inc` and the tools in `IDX/` come from h264p. MTwrap includes `win_shim.inc` by name,
hence `-ILib3`. On POSIX, `win_shim.inc` emulates the few Windows calls it makes. The arithmetic
that decides what is coded is integer arithmetic (the one table computed with `exp()` rounds with a
wide margin), so files are portable between platforms.

## Verification

These 54 streams all compress and decompress to identical bytes at every level, with `-v` passing:
- the 6 avip samples;
- the 40 synthetic, FFmpeg-made and workaround test streams used for m4vdec and m4vtsv
  (I/P/B/S-VOPs, GMC with 1-3 points, quarter-pel, 4MV, interlacing, MPEG quantisation with custom
  matrices, video packets, packed B-frames, N-VOPs, odd frame sizes, the DivX, XviD and libavcodec
  bugs m4vdec works around);
- a stream with junk inserted, an empty file, 300 KB of random bytes, and short or truncated
  files.

On threads, all of them also round-trip with `-t 2 -c 1k`, `-t 3 -c 16k`, `-t 4 -c 64k`, `-t 7 -c 1`
and `-t 64 -c 4k` (each at another level, `-9` to `-1`). The tiny part sizes force cuts at P-VOPs,
at bare start codes, and inside VOPs.

Other checks:
- **Builds:** at each level, the tuning build (every parameter patchable), the shipping build (every
  parameter folded) and the build without AVX2 write the same files (`./mk.sh check`).
- **The decoder in the loop:** its pictures are bit-exact with m4vdec's on 45 test streams (9,758
  frames).
- **Fuzzing,** at every level, under AddressSanitizer and UBSan (ASan's checks of the heap and the
  globals: the coroutines copy their stacks, which its stack checks take for errors):
  - 200 damaged streams (bit flips, truncation, inserted garbage and fake start codes) all
    round-tripped exactly with `-v`;
  - 400 damaged compressed files (bit flips, truncation, garbage, runs of bytes, a changed level)
    each decoded to the original stream (its CRC holds) or were rejected with exit code 4, without
    crashes or hangs;
  - on threads, 120 damaged streams round-tripped with `-t 3 -c 4k -v`, and 300 damaged files of
    parts (the same damage, plus changed header fields and moved blocks) ended with exit code 0 or
    4.

  MemorySanitizer found nothing either: round trips of 8 streams at every level and on threads,
  and 42 damaged files. Decoding checks every decoded value before writing it, and stops when it
  reads past the end of the input.
- **Chunk splitting:** a build with a 64 KiB input buffer and 7-byte reads round-trips every stream.
- **Pipes:** `c` and `d` give the same results through stdin and stdout, also on threads.
- **Portability:** the Windows build (mingw, run under wine) writes the same compressed files as the
  Linux build at every level, and each decompresses the other's, files of parts too. clang and gcc
  builds write the same files.

## License

The parsing code (`syntax.inc`, `parse_vol.inc`), the motion state (`motion.inc`, `mo_sprite.inc`)
and the reconstruction (`recon.inc`, `rc_vop_begin.inc`) are derived from FFmpeg's libavcodec
through m4vdec, so m4vpack is licensed under the GNU Lesser General Public License, version 2.1 or
later, not under this repository's MIT license (the contexts drawn from the reconstruction,
`reconctx.inc`, are m4vpack's own). `Lib3/`, `MTwrap_v3/` and the parts from h264p are h264p's.
