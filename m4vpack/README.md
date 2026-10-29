# m4vpack

m4vpack is a lossless compressor for MPEG-4 Part 2 video elementary streams (`.m4v`: XviD, DivX
4/5/6, libavcodec `mpeg4`, ...). It parses the stream down to the macroblock and coefficient
level and codes the syntax elements again with context-modelled binary arithmetic coding, in the
style of H.264's CABAC. Decompression restores the original stream byte for byte. The streams in
the tests below shrink to 66-85% of their size, while `xz -9e` gets about 90-100%.

## Usage

```
m4vpack c|d [options] input output
```

- `c` compresses, `d` decompresses.
- `-v` (with `c`) decompresses the output afterwards and compares it with the input.
- `-s` (with `c`) shows the input and output size of each kind of syntax element.
- `-q` turns off the statistics.
- `-t N` (with `c`) compresses on N threads (1 to 64): the stream in parts, a model for each.
  `d` takes the number of threads from the stream.
- `-c N` (with `c -t`) sets the size of a part, at least: N MB (1 to 1024), or N KB with a `k`.
  The default is 4 MB.

`input` and `output` can be `-` for stdin and stdout, except with `-v`, which reads both files
again.

```
aq_cm.m4v: 3875193 -> 2956263 bytes (76.29%); 1258 VOPs (I 18, P 666, B 574, S 0), 1258 packed
verify: OK
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | not an m4vpack file, or damaged data (`d`) |
| 5 | `-v` found a difference, out of memory, or an internal error |

## How it works

The stream is cut at start codes (`00 00 01 xx`). m4vpack parses each VOP into the same records
that [m4vtsv](../m4vtsv) writes as text: the VOP header, one record per macroblock (type, coded
block pattern, dquant, motion vector differences as coded, coefficient events), the video packet
headers and the bits after the last macroblock. It writes these records back as bits right away
and compares the result with the VOP. If they match, the records are arithmetic coded. If they
don't, or the VOP uses a feature the parser doesn't handle, the VOP is stored as bytes. Headers
(VOS, VO, VOL), user data and anything else that isn't a VOP are stored as bytes too. So any input
round-trips, including damaged streams and files that aren't video at all.

One set of functions codes and decodes every record. Each function takes the values when
compressing and returns them when decompressing, so the two directions can't drift apart (the
same idea as h264p's symmetric syntax coder).

### Streaming

The compressor and the decompressor are coroutines, built on Lib3's stack-copying coroutines as in
h264p. Each takes its input and hands over its output a 64 KB buffer at a time, so the same code
runs on files, pipes or memory, and any number of runs can go at once. The input buffer grows
with the longest chunk (to 64 MiB), and the macroblock tables grow with the largest picture.
Nothing else depends on the stream's size.

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

### Models

The coding follows H.264's CABAC, using the correspondences worked out in
[h264-counterparts.md](../m4vtsv/h264-counterparts.md). Every syntax element is turned into binary
decisions. Each decision has an adaptive probability chosen by the context, usually the
neighbouring macroblocks or blocks. Where CABAC has a single context per decision, m4vpack combines
several: their predictions are mixed in the logistic domain with weights trained online (as in
lpaq), using two weight sets chosen by different contexts. Coefficient decisions then go through
an APM (secondary estimation).

| Syntax | MPEG-4 Part 2 coding | m4vpack |
|---|---|---|
| macroblock type | not_coded, MCBPC, modb, mb_type VLCs | binary decisions by the left, above, above-left, above-right and co-located macroblocks (last anchor VOP), the run of skipped or direct macroblocks before, and whether the predicted vector is zero |
| coded block pattern | in MCBPC and CBPY, or 6 bits in B-VOPs | per 8x8 block, by the left and above blocks, the size of the motion vector differences, whether the motion is sub-pel, the co-located macroblock, the blocks coded so far, the quantiser |
| motion vector difference | VLC of the f_code-scaled magnitude, sign, residual bits | zero flag by the neighbours' magnitudes (as CABAC's mvd), sign, magnitude in VLC code units in unary, residual bits; also by the other component, the co-located macroblock, the spread of the prediction candidates, and the difference that would repeat the co-located vector |
| intra DC difference | size VLC, bits | size by the previous and neighbouring blocks' sizes and counts and whether the block has AC coefficients, then bits (the top ones by the bits above them) and sign (by the neighbours' and the previous one's) |
| coefficients | 3-D VLC (last, run, level), escapes | CABAC's residual coding: significance map with "last" flags in scan order, levels in reverse order (greater than 1, unary, then size and bits), signs, the escape mode where it isn't the usual one |

For the vector contexts, m4vpack reconstructs the motion vectors as a decoder does: the median
prediction of P-VOPs with its video packet rules, field and 4MV vectors, and the B-VOP predictors
(a check against m4vdec's decoded vectors found no differences). In B-VOPs the co-located vector is
scaled by the display times, as in direct mode.

The coefficient contexts are the scan position and block class (inter, B, intra, intra with AC
prediction; luma or chroma), the neighbouring blocks' coefficient counts (CAVLC's nC), the
coefficients found so far, the left and above blocks' coefficients at the same position, the
frequency neighbours already coded (as in HEVC), the quantiser, the motion vector size (for an
intra block, the size of its DC difference) and the co-located block in the last anchor VOP. A Cr
block also sees the Cb block of its macroblock.

Some contexts come from h264p's model:
- **Histories:** the significance flags before the current one (the last 8 by scan position, and
  the last 16 hashed into a table of 2^18), and the magnitudes coded before a level and their sum.
- **Signs:** a block's coefficients come in sign patterns, as an edge leaves them. So the signs go
  by the signs coded before them in the block, and by those of the frequency neighbours to the
  left and above (zigzag order codes them first), weighted by their magnitudes. They also go by the
  signs of the left and above blocks and, for Cr, the Cb block at the same position.

Probabilities adapt at a rate of 1/n, with n counting the decisions seen up to 511. All probability
computations that affect the coded data use integers only, so files are portable between
platforms.

### Ideas from h264p

These were tried against the 12 streams of the tuning set (24.8 MB), which went from 75.60% to
73.78% of their size:

| Change | Size | |
|---|---:|---:|
| the last 8 significance flags as a context, by scan position | 75.40% | −0.20 |
| the last 16 hashed, as one more input | 75.28% | −0.12 |
| the magnitudes coded before a level (the last 3), by position | 75.20% | −0.08 |
| the signs coded before a sign (the last 3) | 75.09% | −0.10 |
| the signs of the frequency neighbours left and above | 74.85% | −0.24 |
| those signs weighted by their magnitudes, with the one above left | 74.71% | −0.15 |
| Cb's coefficient (magnitude, sign) at the same position for Cr | 74.59% | −0.12 |
| the size of an intra block's DC difference where an inter block has its vector size | 74.56% | −0.03 |
| a second weight set and an APM for the signs | 74.51% | −0.05 |
| the exact pattern of 5 frequency neighbours for significance | 74.43% | −0.09 |
| the neighbours right and below, and the magnitude history, for the unary part of levels | 74.36% | −0.07 |
| the coded block pattern by the quantiser and the blocks coded so far | 74.27% | −0.09 |
| each neighbour's vector size, a second weight set for vector differences | 74.24% | −0.03 |
| the left and above blocks' signs as coded (a bug left them all 0 before) | 73.91% | −0.33 |
| intra DC: the neighbours' counts and signs, the top bits by the bits above them | 73.85% | −0.06 |
| the sum of the magnitudes coded before a level; a second weight set for its unary part | 73.75% | −0.10 |
| the hashed tables at 2^18 entries instead of 2^20 (faster) | 73.78% | +0.03 |

No gain: coding a block's count first, then the significance knowing how many are left (+0.21%
with the same contexts: h264p's choice for 4x4 blocks loses on 8x8 ones); a mixer learning rate
that is higher while a weight set is new, weights starting at 1/n, counters aiming short of 0 and
1 (all within ±0.02%); a fast second probability in each counter (tried earlier).

Not done: h264p's largest gains come from a decoder in the loop. It reconstructs the pictures, so
it can predict signs by how well a block continues its surroundings, and use the texture of the
prediction and an intra estimate from the reference. For m4vpack that would mean running m4vdec's
reconstruction inside the model. Signs are still 11-17% of the output.

### File format

| Bytes | Content |
|---|---|
| 8 | `m4vpack` and the format version 3 |
| rest | arithmetic coded data: a sequence of chunks, each a kind (VOP as records, bytes, or end) followed by its content, and after the end the CRC-32 of the stream |

On threads:

| Bytes | Content |
|---|---|
| 5 | `m4vpm` |
| 1 | the format version 3 |
| 1 | the threads |
| 4 | the part size in KB, little endian |
| rest | MTwrap's container of the workers' streams |

A worker's stream holds a 48-byte record for each of its parts: the record's size, the part's size,
where the headers copied into it are, the part's place in the stream, and a CRC-32 of these fields.
The compressed part follows, as a stream of the first kind. An empty record ends the worker's
stream. `d` checks the CRC of each part and each record, and that no part is lost, added or moved.

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
(XviD 1.3) or its own `mpeg4` encoder.

| Stream | Size | xz -9e | m4vpack |
|---|---:|---:|---:|
| aq_cm.m4v | 3875193 | 97.2% | 76.3% |
| aria_cm.m4v | 5097686 | 98.9% | 79.0% |
| ispure03.m4v | 5864104 | 95.8% | 73.0% |
| sdova_ed.m4v | 4717732 | 89.6% | 65.7% |
| susi_small1_divx.m4v | 90887 | 100.1% | 83.4% |
| susi_small2_divx.m4v | 1556817 | 100.0% | 85.1% |
| xvid.m4v | 257813 | 97.7% | 74.4% |
| lavc_ilace.m4v (interlaced) | 1015896 | 90.9% | 68.6% |
| xvid_gmc.m4v (GMC) | 833291 | 99.8% | 76.9% |
| xvid_qpel_4mv.m4v (quarter-pel, 4MV) | 383624 | 95.5% | 74.0% |

Where the bits go in `aq_cm.m4v` (`-s`; the packed sizes are the model's estimates):

```
  bytes                                  input     packed
  headers, raw chunks                    11752       2620  22.30%
    raw chunks                                        315
    VOP and video packet headers, tails              2305
  macroblock types, cbp, dquant         357692     148871  41.62%
    types, flags, dquant                            66279
    coded block patterns                            82592
  motion vectors                        191806     152213  79.36%
  intra DC                              256787     205539  80.04%
  coefficients                         3057155    2447228  80.05%
    positions                                      945607
    levels                                        1049501
    signs                                          450410
    escape modes                                     1710
  total                                3875193    2956471  76.29%
```

Coefficients make up most of a stream (79% of this one). Their signs are 11-17% of the output on
the samples. The sign contexts save 2-8% of their bits, more on the XviD streams than on the DivX
ones, but most of a sign can't be told from the syntax. Macroblock headers shrink the most: to 42%
here, and to 48% in `sdova_ed.m4v`, where they are a third of the input.

`c` and `d` each run at about 1 MB of stream per second on one thread (built with `-O3`), and use
about 9 MiB. The model of format 3 takes 1.5-1.9 times as long as that of format 2, for 2.4%
smaller files.

On threads, with five of the avip samples concatenated (21 MB, four cores):

| | Size | `c` | `d` |
|---|---:|---:|---:|
| one thread | 15689039 (74.32%) | 21.8 s, 9 MiB | 20.7 s, 8 MiB |
| `-t 2` | 15700471 (74.37%) | 13.0 s, 45 MiB | 13.3 s, 32 MiB |
| `-t 4` | 15700573 (74.37%) | 9.0 s, 73 MiB | 9.1 s, 53 MiB |

With the default 4 MB parts the stream has 5 parts, which cost 0.07% of the size.

## Build

```
x86_64-w64-mingw32-g++ -O3 -s -static -ILib3 -o m4vpack.exe m4vpack.cpp
g++ -O3 -ILib3 -pthread -o m4vpack m4vpack.cpp
```

`-O3` runs about 15% faster than `-O2` here, and writes the same files.

The source is C++11 that builds without exceptions or RTTI, and has no dependencies besides the C
library. `Lib3/` (the coroutines) and `MTwrap_v3/` (the threads) are copied unchanged from h264p.
MTwrap includes `win_shim.inc` by name, hence `-ILib3`. On POSIX, `win_shim.inc` emulates the few
Windows calls it makes.

## Verification

These streams all compress and decompress to identical bytes, with `-v` passing:
- the 6 avip samples;
- the 39 synthetic and workaround test streams used for m4vdec and m4vtsv (I/P/B/S-VOPs, GMC,
  quarter-pel, 4MV, interlacing, MPEG quantisation, video packets, packed B-frames, N-VOPs, odd
  frame sizes);
- a stream with junk inserted, an empty file, 300 KB of random bytes, and short or truncated
  files.

On threads, all of them also round-trip with `-t 2 -c 1k`, `-t 3 -c 16k`, `-t 4 -c 64k`, `-t 7 -c 1`
and `-t 64 -c 4k`. The tiny part sizes force cuts at P-VOPs, at bare start codes, and inside VOPs.

Other checks:
- **Fuzzing,** under AddressSanitizer and UBSan (and again after the model changes of format 3,
  where UBSan found an overflowing history that is now masked):
  - 200 damaged streams (bit flips, truncation, inserted garbage and fake start codes) all
    round-tripped exactly with `-v`;
  - 300 damaged compressed files (bit flips, truncation, garbage, runs of bytes) were all rejected
    with exit code 4, without crashes or hangs;
  - 400 damaged files of parts (the same damage, plus changed header fields and moved blocks) all
    ended with exit code 0 or 4 and no sanitizer report.

  Decoding checks every decoded value before writing it, and stops when it reads past the end of
  the input.
- **Chunk splitting:** a build with a 64 KiB input buffer and 7-byte reads round-trips everything.
- **Pipes:** `c` and `d` give the same results through stdin and stdout, also on threads.
- **Portability:** the Windows build (mingw, run under wine) writes the same compressed files as
  the Linux build, and each decompresses the other's, files of parts too. clang and gcc builds
  write the same files.
- **Builds:** no warnings from m4vpack's own code with `-Wall -Wextra` (Lib3 and MTwrap have some).

## License

The parsing code is derived from FFmpeg's libavcodec through m4vdec, so `m4vpack.cpp` is licensed
under the GNU Lesser General Public License, version 2.1 or later, not under this repository's MIT
license. `Lib3/` and `MTwrap_v3/` are h264p's.
