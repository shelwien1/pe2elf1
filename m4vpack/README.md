# m4vpack

m4vpack is a lossless compressor for MPEG-4 Part 2 video elementary streams (`.m4v`: XviD, DivX
4/5/6, libavcodec `mpeg4`, ...). It parses the stream down to the macroblock and coefficient
level and codes the syntax elements again with context-mixing binary arithmetic coding, in the
style of H.264's CABAC. At its strongest level it also decodes the pictures as it goes, so that
its contexts can look at the samples. Decompression restores the original stream byte for byte.
The streams in the tests below shrink to «RANGE9» of their size at the default level, while
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
«EXAMPLE»
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
| `-9` | the decoder in the loop; per decision ten counters, three mixers and a final one, two APMs | «» | «» | «» | «» |
| `-7` | the same mixing without the decoder | «» | «» | «» | «» |
| `-5` | ten counters, one mixer | «» | «» | «» | «» |
| `-3` | six counters, one mixer | «» | «» | «» | «» |
| `-1` | four counters (keeping their probabilities stretched), one mixer | «» | «» | «» | «» |

«LEVELS NOTE»

## How it works

The stream is cut at start codes (`00 00 01 xx`). m4vpack parses each VOP into the same records
that [m4vtsv](../m4vtsv) writes as text: the VOP header, one record per macroblock (type, coded
block pattern, dquant, motion vector differences as coded, coefficient events), the video packet
headers and the bits after the last macroblock. It writes these records back as bits right away
and compares the result with the VOP. If they match, the records are arithmetic coded. If they
don't, or the VOP uses a feature the parser doesn't handle, the VOP is stored as bytes. Headers
(VOS, VO, VOL), user data and anything else that isn't a VOP are stored as bytes too. So any input
round-trips, including damaged streams and files that aren't video at all.

One set of functions (`model.inc`) codes and decodes every record. Each function takes the values
when compressing and returns them when decompressing, so the two directions can't drift apart (the
same idea as h264p's symmetric syntax coder).

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
how each variable is bucketed), are declared in `IDX/l<L>/<family>.idx` (the format: `IDX/IDX-FORMAT.md`,
from h264p). `IDX/idx2inc.pl` turns them into `MOD/l<L>/`, which the build includes; `levels.inc`
and `lv<L>.inc` put each level's families together.

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

The vectors the contexts use are reconstructed as a decoder does (median prediction with its video
packet rules, field and 4MV vectors, B-VOP predictors, direct mode scaling by the display times).

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
in the tuned model, against 68.76% with all of them.

| Context | Elements | Without it |
|---|---|---:|
| **sign prediction**: a block's first 8 signs (in scan order) are searched for the combination whose block continues the decoded samples above and left of it best: the sum over its top row and left column of the distance from 2 R1 − R2, and from R1 (R1, R2 the two decoded rows or columns beyond it; MPEG-4 has no deblocking, so the extrapolation is noisy where the samples are); the other signs go first, then these, with the sign each of the two searches prefers, by how much, and their rank | signs | 69.95% (+1.20) |
| **the intra estimate**: an intra macroblock's blocks as the reference has them where its neighbours' motion (refined by matching the decoded samples above and left of it, the template) puts it, transformed and quantised as the encoder would, the intra prediction taken off: the DC difference's hint and sign, the level at each position (the levels' hint), its sign, the AC coefficients per block for the coded block pattern; trusted by the template's match, then by how far the macroblock's blocks so far were from it | intra DC, coefficients, coded block pattern | 69.62% (+0.87) |
| **an inter block's prediction**: its texture, and how far its edges are from the decoded samples next to them, against the quantiser | coded block pattern, coefficients | 69.56% (+0.80) |
| intra blocks with AC prediction in their own scan (vertical or horizontal, by the DC prediction), which the decoder knows before their coefficients | coefficients | 69.11% (+0.35) |
| the prediction's spectrum: its transform at each position against the quantiser, and which way its texture runs | coefficients | 69.09% (+0.34) |
| the vector the template matches best (a search around the vector's prediction, the co-located vector and none, then half samples) | vector differences, their signs | 68.92% (+0.17) |
| skipping: how much the area changed since the reference (the decoded samples around the macroblock against the reference's), how well the reference's macroblock (in B-VOPs the direct prediction) continues them | skip, macroblock type | 68.92% (+0.16) |
| the levels an inter block's edges ask for: their miss projected onto each coefficient's pattern | coefficients | 68.83% (+0.07) |
| all of them | | 72.80% (+4.05) |

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
contexts differ), and `-5`, `-3` and `-1` cut from `-7` (their counters' indices the ones whose loss
costs `-7` most) and tuned for two passes each.

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

«RESULTS TABLE»

Where the bits go in `aq_cm.m4v` at `-9` (`-s`; the packed sizes are the model's estimates):

```
«STATS»
```

«RESULTS NOTE»

On threads, with five of the avip samples concatenated (21 MB, four cores):

«MT TABLE»

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
library. `Lib3/` (the coroutines), `MTwrap_v3/` (the threads), `cm.inc`, `fam.inc`, `famcfg.inc`,
`mapping.inc` and the tools in `IDX/` come from h264p. MTwrap includes `win_shim.inc` by name, hence
`-ILib3`. On POSIX, `win_shim.inc` emulates the few Windows calls it makes. The arithmetic that
decides what is coded is integer arithmetic (the one table computed with `exp()` rounds with a
wide margin), so files are portable between platforms.

## Verification

«VERIFICATION»

## License

The parsing code (`syntax.inc`) and the reconstruction (`recon.inc`) are derived from FFmpeg's
libavcodec through m4vdec, so m4vpack is licensed under the GNU Lesser General Public License,
version 2.1 or later, not under this repository's MIT license. `Lib3/`, `MTwrap_v3/` and the parts
from h264p are h264p's.
