# dff2dsf

A standalone decoder for DSDIFF (`.dff`) files, including the DST lossless
compression that the Philips encoder (`DstEncUi`) produces, writing plain DSF
(`.dsf`).

```
usage: dff2dsf d input.dff output.dsf

  d   decode: DST coded or raw DSD in a .dff, written as a .dsf
```

Only decoding is implemented; `d` is the command so that an encoder can be added
later without changing the interface.

## Building

```
make
```

C++20, no dependencies. Built with `-fno-exceptions -fno-rtti`; no STL streams
and no STL containers are used, only stdio and explicit allocation.

## What it does

DSDIFF and DSF hold the same 1-bit DSD samples, but arrange them differently, so
decoding is a matter of undoing DST and then rearranging:

| | DSDIFF (.dff) | DSF (.dsf) |
|---|---|---|
| chunk sizes | 64-bit big-endian | 64-bit little-endian |
| sample data | byte-interleaved per channel | planar, 4096-byte blocks per channel |
| bit order | MSB first | LSB first (`bits per sample` = 1) |
| compression | raw DSD or DST | raw DSD only |

DST (Direct Stream Transfer, ISO/IEC 14496-3 Part 3 Subpart 10) codes each 1/75 s
frame independently: a 128-tap sign-based FIR predicts the next DSD bit from the
previous ones, and the prediction is corrected by an arithmetic-coded residual.
Filter coefficient sets and probability tables are transmitted per frame, either
verbatim or predicted with fixed predictors plus Golomb-coded residuals.

The last DSF block is padded to the fixed 4096 bytes per channel with `0x69`
(DSD silence); the exact length is carried by the sample count in the header, so
players trim the padding.

## Verifying against FFmpeg

FFmpeg decodes both formats, so decoding the source `.dff` and the produced
`.dsf` to PCM must give identical output:

```sh
ffmpeg -v error -i input.dff -c:a pcm_s24le -f md5 -
./dff2dsf d input.dff output.dsf
ffmpeg -v error -i output.dsf -c:a pcm_s24le -f md5 -
```

`tests/verify.sh input.dff` does exactly this.

Checked bit-exact against FFmpeg (master, `74705b3`) on a 4:15 DSD64 stereo file
produced by `DstEncUi - Version '4.0.3'` (19119 DST frames, 75 MB in, 180 MB
out), and on a synthetic uncompressed-DSD `.dff`. The decoder also runs clean
under ASan/UBSan, including 180 runs over randomly corrupted input.

Decoding the 4:15 test file takes about 27 s on one core. DST frames are
independent, so this parallelises directly across frames if it ever matters.

## Source layout

| file | contents |
|---|---|
| `src/main.cpp` | command line, decode loop |
| `src/dsdiff.cpp` | DSDIFF container parsing and frame iteration |
| `src/dst.cpp` | DST decoder |
| `src/dsf.cpp` | DSF container writing, deinterleave and bit reversal |
| `src/bits.h` | MSB-first bit reader, JPEG-LS Golomb code |
| `src/common.h` | byte order helpers, buffered file access |

## Licensing

`src/dst.cpp`, `src/dst.h` and `src/bits.h` are derived from FFmpeg
(`libavcodec/dstdec.c`, `get_bits.h`, `golomb.h`), which is **LGPL-2.1-or-later**
— see `LICENSE.ffmpeg`. That license governs those files and therefore the
combined binary, regardless of the MIT `LICENSE` covering the rest of this
repository. Writing an independent DST implementation from the ISO specification
would be the way to make the whole thing MIT.
