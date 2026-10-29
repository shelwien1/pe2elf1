# m4vpack

m4vpack is a lossless compressor for MPEG-4 Part 2 video elementary streams (`.m4v`: XviD, DivX
4/5/6, libavcodec `mpeg4`, ...). It parses the stream down to the macroblock and coefficient
level and codes the syntax elements again with context-modelled binary arithmetic coding, in the
style of H.264's CABAC. Decompression restores the original stream byte for byte. The streams in
the tests below shrink to 67-86% of their size, while `xz -9e` gets about 90-100%.

## Usage

```
m4vpack c|d [options] input output
```

- `c` compresses, `d` decompresses.
- `-v` (with `c`) decompresses the output in memory afterwards and compares it with the input.
- `-s` (with `c`) shows the input and output size of each kind of syntax element.
- `-q` turns off the statistics.

```
aq_cm.m4v: 3875193 -> 3026965 bytes (78.11%); 1258 VOPs (I 18, P 666, B 574, S 0), 1258 packed
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
| 5 | `-v` found a difference, or an internal error |

The output of `c` has to be a regular file, since the header is written last. `-v` reads the input
a second time.

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
| coded block pattern | in MCBPC and CBPY, or 6 bits in B-VOPs | per 8x8 block, by the left and above blocks, the size of the motion vector differences, whether the motion is sub-pel, the co-located macroblock |
| motion vector difference | VLC of the f_code-scaled magnitude, sign, residual bits | zero flag by the neighbours' magnitudes (as CABAC's mvd), sign, magnitude in VLC code units in unary, residual bits; also by the other component, the co-located macroblock, the spread of the prediction candidates, and the difference that would repeat the co-located vector |
| intra DC difference | size VLC, bits | size by the previous and neighbouring blocks' sizes and whether the block has AC coefficients, then bits and sign |
| coefficients | 3-D VLC (last, run, level), escapes | CABAC's residual coding: significance map with "last" flags in scan order, levels in reverse order (greater than 1, unary, then size and bits), signs, the escape mode where it isn't the usual one |

For the vector contexts, m4vpack reconstructs the motion vectors as a decoder does: the median
prediction of P-VOPs with its video packet rules, field and 4MV vectors, and the B-VOP predictors
(a check against m4vdec's decoded vectors found no differences). In B-VOPs the co-located vector is
scaled by the display times, as in direct mode.

The coefficient contexts are the scan position and block class (inter, B, intra, intra with AC
prediction; luma or chroma), the neighbouring blocks' coefficient counts (CAVLC's nC), the
coefficients found so far, the left and above blocks' coefficients at the same position, the
frequency neighbours already coded (as in HEVC), the quantiser, the motion vector size and the
co-located block in the last anchor VOP.

Probabilities adapt at a rate of 1/n, with n counting the decisions seen up to 511. All probability
computations that affect the coded data use integers only, so files are portable between
platforms.

### File format

| Bytes | Content |
|---|---|
| 8 | `m4vpack` and the format version 1 |
| 8 | size of the original stream, little endian |
| 4 | CRC-32 of the original stream |
| rest | arithmetic coded data: a sequence of chunks, each a kind (VOP as records, bytes, or end) followed by its content |

`d` checks the size and the CRC.

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
| aq_cm.m4v | 3875193 | 97.2% | 78.1% |
| aria_cm.m4v | 5097686 | 98.9% | 80.5% |
| ispure03.m4v | 5864104 | 95.8% | 74.5% |
| sdova_ed.m4v | 4717732 | 89.6% | 66.7% |
| susi_small1_divx.m4v | 90887 | 100.1% | 83.7% |
| susi_small2_divx.m4v | 1556817 | 100.0% | 85.6% |
| xvid.m4v | 257813 | 97.7% | 79.0% |
| lavc_ilace.m4v (interlaced) | 1015896 | 90.9% | 73.0% |
| xvid_gmc.m4v (GMC) | 833291 | 99.8% | 80.3% |
| xvid_qpel_4mv.m4v (quarter-pel, 4MV) | 383624 | 95.5% | 77.9% |

Where the bits go in `aq_cm.m4v` (`-s`; the packed sizes are the model's estimates):

```
  bytes                                  input     packed
  headers, raw chunks                    11752       2616  22.26%
    raw chunks                                        315
    VOP and video packet headers, tails              2301
  macroblock types, cbp, dquant         357692     150345  42.03%
    types, flags, dquant                            66279
    coded block patterns                            84066
  motion vectors                        191806     152749  79.64%
  intra DC                              256787     210596  82.01%
  coefficients                         3057155    2510868  82.13%
    positions                                      964495
    levels                                        1062589
    signs                                          482075
    escape modes                                     1710
  total                                3875193    3027174  78.12%
```

Coefficients make up most of a stream (79% of this one). Their signs are 11-18% of the output on
the samples, and the syntax alone can't predict them. Macroblock headers shrink the most: to 42%
here, and to 48% in `sdova_ed.m4v`, where they are a third of the input.

`c` and `d` each run at about 1.2-1.3 MB of stream per second (one core), and use about 11 MiB.

## Build

```
x86_64-w64-mingw32-g++ -O2 -s -static -o m4vpack.exe m4vpack.cpp
g++ -O2 -o m4vpack m4vpack.cpp
```

The source is plain C that compiles as C or C++ and has no dependencies besides the C library
(`-lm` when built as C). Like m4vdec and m4vtsv, it doesn't use malloc. All buffers are static,
about 150 MiB of `.bss`, and only the part the data needs gets touched.

## Verification

These streams all compress and decompress to identical bytes, with `-v` passing:
- the 6 avip samples;
- the 39 synthetic and workaround test streams used for m4vdec and m4vtsv (I/P/B/S-VOPs, GMC,
  quarter-pel, 4MV, interlacing, MPEG quantisation, video packets, packed B-frames, N-VOPs, odd
  frame sizes);
- a stream with junk inserted, an empty file, 300 KB of random bytes, and short or truncated
  files.

Other checks:
- **Fuzzing,** under AddressSanitizer and UBSan: 200 damaged streams (bit flips, truncation,
  inserted garbage and fake start codes) all round-tripped exactly with `-v`. 300 damaged
  compressed files (bit flips, truncation, garbage, overwritten headers) were all rejected with exit
  code 4, without crashes or hangs. Decoding checks every decoded value before writing it, and stops
  at the size the header gives or when it reads past the end of the input.
- **Chunk splitting:** a build with a 64 KiB input buffer and 7-byte reads round-trips everything.
- **Portability:** the Windows build (mingw, run under wine) writes the same compressed files as
  the Linux build, and each decompresses the other's.
- **Builds:** compiles without warnings as C99 (`-pedantic`), C++, and with mingw.

## License

The parsing code is derived from FFmpeg's libavcodec through m4vdec, so `m4vpack.cpp` is licensed
under the GNU Lesser General Public License, version 2.1 or later, not under this repository's MIT
license.
