# ccsds — a lossless BMP coder on CCSDS 123.0-R-1

A single binary that compresses and decompresses Windows BMP files with the
CCSDS 123 predictor and entropy coder, the standard written for lossless
multispectral and hyperspectral imaging.

```
ccsds c [options] input.bmp output.ccsds
ccsds d [options] input.ccsds output.bmp
```

A BMP is presented to the predictor as a very short hyperspectral cube: one
band per colour plane, so the standard's inter-band prediction decorrelates
blue, green and red the same way it would decorrelate adjacent spectral
channels. The mapped prediction residuals then go through the standard's
entropy coder, and a small container header carries the palette and the sub-byte
row padding that the coded cube has no room for.

1, 4, 8, 24 and 32 bit BMPs are supported, uncompressed or RLE, with the usual
40 byte info header.

## Building

```
make            # -> ./ccsds
make check      # round trips the test corpus through every coder setting
make fuzz       # feeds the decoder damaged streams
make sancheck   # both suites over an address/UB sanitizer build
```

The whole coder is one translation unit: `src/ccsds_bmp.cpp` includes the two
`.inc` fragments and the five CCSDS 123 implementation files, so `make` runs a
single compiler invocation. No dependencies beyond a C++ compiler and libm.
Python 3 is needed for the tests only.

It builds with g++ or clang++, on Linux or Windows, and it does not care which
`-D` flags you pass it — there is no combination that compiles but misbehaves.
That was worth arranging, because the reference implementation had two:

    clang++ -O2 -DNDEBUG -DWIN32 -Isrc -o ccsds.exe src/ccsds_bmp.cpp

## What it does

| image | source | BMP | PNG (zlib -9) | ccsds | coder chosen |
| --- | --- | ---: | ---: | ---: | --- |
| `probe24` | 256x192, 24 bpp, continuous tone | 147510 | 80507 | **25842** | block, 64 |
| `chroma24` | 256x192, 24 bpp, independent chroma | 147510 | 103453 | **27025** | block, 64 |
| `probe8` | 256x192, 8 bpp grey, continuous tone | 50230 | 31356 | **13232** | sample |
| `photo24` | 128x96, 24 bpp | 36918 | 13406 | **5875** | block, 64 |
| `alpha32` | 33x24, 32 bpp | 3222 | 2059 | **904** | block, 8 |
| `noise24` | 40x30, 24 bpp uniform noise | 3654 | 3698 | **3690** | block, 32 |
| `pal8` | 53x37, 8 bpp palette | 3150 | 1273 | **1236** | block, 8 |
| `grey8` | 64x64, 8 bpp, `x^y` pattern | 5174 | 1611 | 2647 | block, 64 |
| `pal4` | 53x29, 4 bpp | 930 | 197 | 313 | block, 8 |
| `mono1` | 37x23, 1 bpp | 246 | 108 | 176 | block, 64 |

The PNG column is the same pixels through zlib at level 9 with per-row filters,
as a familiar reference point rather than a benchmark. The pattern is what the
standard's design predicts: on continuous-tone images a predictive coder with
learned weights is far ahead — three times smaller than PNG on `probe24` — and
on synthetic or palette art, where the redundancy is repeated byte strings
rather than a smooth surface, a match-based coder wins. CCSDS 123 has no
concept of a repeated string.

## Options

Compression parameters travel in the stream, so decompression takes none.

### Entropy coder

    --auto             code with every setting below and keep the smallest
                       result (the default)
    --sample           sample-adaptive coder only
    --block            block-adaptive coder only
    -k, --k N          accumulator initialisation constant, 0..D-2 (default 3)
    --u-max N          unary length limit, 8..32 (default 16)
    --y-star N         rescaling counter size, 4..9 (default 6)
    --y0 N             initial count exponent, 1..8 (default 1)
    -B, --block-size N block size 8, 16, 32 or 64 (default: try each)
    --ref-interval N   reference sample interval, 1..4096 (default 256)
    --restricted       restricted code set, block coder with D<=4 only

`--auto` is worth its cost: prediction runs once whichever coder is used, and
coding is the cheap half, so trying all five settings adds a fraction of the
runtime. It also pays — no single setting wins on every image. The block coder
at 64 samples takes the colour photographs by ten percent or more, the sample
coder takes the smooth greyscales, and the eight-sample blocks take the palette
art.

### Predictor

    -p, --pred-bands N spectral bands used for prediction (default: all but one)
    --reduced          reduced prediction mode (default: full)
    --column-sum       column oriented local sum (default: neighbour oriented)
    --reg-size N       register size, 4..64 (default 32)
    --w-resolution N   weight resolution, 4..19 (default 14)
    --w-interval N     weight update interval, a power of 2 in 16..2048 (default 128)
    --w-initial N      weight update scaling exponent, initial, -6..9 (default -2)
    --w-final N        weight update scaling exponent, final, -6..9 (default 5)

The defaults were measured rather than copied: `--w-interval 128` and the
`-2..5` exponent range came out about half a percent ahead of the more usual
`32` and `-1..3` across the corpus, and the weight resolution saturates at 13.

### Colour

    --rct              decorrelate 24/32 bpp planes with the reversible
                       YCoCg-R transform before prediction
    --no-rct           code the colour planes as they are (the default)

`--rct` is off by default because it lost on every image tried, by two to three
percent. That is not a surprise: CCSDS 123 already learns its inter-band
weights per image, so a fixed transform in front of it has little left to
remove, and the transform's chroma planes need a ninth bit, which widens the
dynamic range of the whole cube. The option is there because the trade-off
depends on the image and the measurement was made on a synthetic corpus.

Permuting the colour planes to put green first — so that blue and red are both
predicted from it — was measured too, and came out level. The planes are coded
in the order the bytes sit in a BMP pixel.

## How a BMP becomes a cube

| BMP | cube | D | notes |
| --- | --- | --- | --- |
| 24 bpp | `w` x `h` x 3 | 8 | one band per plane, in file order: blue, green, red |
| 32 bpp | `w` x `h` x 4 | 8 | alpha is the fourth band |
| 8 bpp | `w` x `h` x 1 | 8 | palette indices; the palette rides in the container |
| 4 bpp | `w` x `h` x 1 | 4 | unpacked to one sample per pixel |
| 1 bpp | `w` x `h` x 1 | 2 | unpacked; D starts at 2 in the standard |

Sub-byte rows are unpacked rather than coded as packed bytes, so the predictor
sees pixels instead of bit soup. A row whose width does not fill its last byte
leaves padding bits over; they are not pixels, but they are part of the file, so
the container carries them — one byte per row, and only when some of them are
not zero, which is almost never.

## The container

The CCSDS header already carries the cube's geometry, its dynamic range and
every predictor and coder parameter, so the container only has to say how the
BMP was turned into a cube.

```
0    "CCB1"
4    u8    bits per pixel
5    u8    flags: 1 unpacked sub-byte source, 2 palette,
                  4 row padding table, 8 colour transform
6    u16   width           (little endian)
8    u16   height
10   u16   palette entries
12   palette, 3 bytes per entry, blue green red
     row padding bits, one byte per row, when flag 4 is set
     the CCSDS 123.0-R-1 stream
```

## Losslessness

`make check` round trips the corpus in `test/bmp` through fourteen coder
settings and asserts two things per run.

The decoded BMP depicts the same image as the source: same geometry, same
palette, same pixels. It is not compared byte for byte with the source, because
the coder rebuilds the file header rather than carrying the original's — an RLE
source comes back stored plainly, a short declared palette comes back full, and
the resolution fields come back zero.

Then that decoded BMP, coded and decoded again, comes back byte for byte
identical. Once a BMP has been through the coder it is a fixed point, padding
bits and all, which is the strongest statement available given the header is
rebuilt.

`make fuzz` feeds the decoder 851 truncated and corrupted streams and asserts
it never crashes, reads out of bounds or leaks; under `make sancheck` the
sanitizers do the checking.

## Layout

    src/ccsds_bmp.cpp    the coder: options, BMP <-> cube, container
    src/bmp.inc          BMP reading and writing (from the BMF codec, verbatim)
    src/bmf_glue.inc     the handful of symbols bmp.inc expects from its host
    src/ccsds123/        the CCSDS 123.0-R-1 predictor and entropy coder
    test/                the corpus generator, the round trip suite, the fuzzer

`ccsds_bmp.cpp` includes the `.inc` fragments at the top, then the CCSDS 123
headers it was written against, and the five `.c` files at the bottom — the
library underneath the coder rather than part of it. Each of those is a
standalone translation unit that includes what it needs, so the order does not
matter and they still compile separately if you would rather link them. The
Makefile passes `-MMD`, so editing any included file rebuilds.

`src/ccsds123` is Luca Fossati's reference implementation for the European Space
Agency, under the European Space Agency Public License v2.0; the licence text is
at the head of every file. It is included with its changes marked: search for
`MODIFIED` to see all of them. They fall into three groups.

**Entry points.** `predict()` and `unpredict()` read and write raw sample files,
and `encode()` and `decode()` own whole files. Each grew an in-memory or
already-open-stream twin (`predict_samples`, `unpredict_samples`,
`encode_to_buffer`, `encode_to_stream`, `decode_from_stream`) so a BMP does not
have to be spilled to a raw file on the way through, and so the container can
put its own header in front of the coded image. The original entry points are
still there, in terms of the new ones. `decoder.h` also used to declare
`encoder_config_t` a second time, identically to `entropy_encoder.h`, which
meant no translation unit could include both — a combined coder has to.

**Correctness.** Five defects that a round trip trips over:

- The neighbour oriented local sum reaches to `(x+1, y-1)`, which does not exist
  when the image is one column wide. It read past the end of the row and landed
  on the very sample being predicted — the encoder saw the original value there
  and the decoder a not yet reconstructed zero, so a 1 pixel wide image did not
  survive a round trip.
- A run of five or more zero blocks ending on a segment boundary was sent as the
  ROS code, which the decoder sizes as the smaller of the rest of the segment
  and the rest of the reference sample interval. This encoder never breaks a run
  at a reference interval — the test for it compares a counter already reduced
  modulo the interval against the interval itself, so it never fires. The two
  disagreed whenever the reference sample interval was not a multiple of the 64
  block segment. The explicit count says the same thing in a few more bits and
  any conformant decoder reads it back unchanged.
- The second extension option pairs samples, so the last block of an image with
  an odd sample count is one short. Both halves of the final pair were stored
  anyway, one sample past the end of the residual buffer.
- The coded stream buffer was sized at the uncompressed size of the residuals,
  on the assumption that coding never expands. It can: a sample-adaptive escape
  costs `u_max + D` bits, so the worst case is six bytes per sample, not two.
- `mod_star` shifted a negative value in both directions and formed 2^63 in a
  signed `long long` at a 64 bit register size. Reducing modulo 2^R with
  unsigned arithmetic picks the same representative without either.

**Portability.** The reference implementation had settings you had to get right
before it would work, and getting them wrong failed in ways that did not look
like a build problem:

- `NO_COMPUTE_LOCAL` chose between computing each local difference where it is
  needed and building four whole cubes of them up front. Only `predictor.c` ever
  had the choice: `unpredict.c` calls the compute-them-here forms unconditionally,
  so the precomputing branch could encode but never decode, and a build that did
  not define the flag did not compile at all. The branch is gone, along with every
  `#ifdef` that selected it.
- Without `NDEBUG`, two bounds checks compared the coded length against the
  uncompressed size of the residuals, on the same assumption that coding never
  expands that undersized the buffer. A debug build refused to encode an
  incompressible image. They are gone; `encode_to_buffer` checks the length that
  matters once, against the buffer it actually allocated. A block of `printf()`s
  that dumped the decoded header to stdout in the same builds is gone too.
- `utils.h` defined a function-like macro named `log2` for every `WIN32` build.
  `log2` has been in the C runtime since C99, so it was only ever needed by Visual
  C++ before 2013 — and in a single translation unit the header is reached before
  `<math.h>`, where the macro rewrites the C library's own declaration and the
  compile fails inside `math.h`. It is now guarded down to the compilers that
  need it.
- `_CRT_SECURE_NO_WARNINGS` was defined unguarded in all five files, which warns
  once per file if the build already defines it on the command line.

**Robustness.** A coded stream is untrusted input, and the decoder used to trust
it: none of the header reads were checked, a zero band interleaving depth
reached a division by zero, a damaged geometry field could ask for 2^48 samples
before anything validated it, and the end of the stream was signalled by a
return value that a release build never looked at — so `k = compression_id - 1`
became 0xFFFFFFFE and got used as a shift. Those are all checked now, the
decoder is told the sample count the caller is expecting before it allocates
anything, and truncation is an error rather than a half decoded image. Three
allocations that were leaked on every call or on an error path are freed.
