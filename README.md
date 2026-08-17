# dff2dsf

A standalone codec for DSDIFF (`.dff`) files with DST lossless compression — the
format the Philips encoder (`DstEncUi`) produces — and plain DSF (`.dsf`).

```
usage: dff2dsf d input.dff output.dsf
       dff2dsf c input.dsf output.dff

  d   decode: DST coded or raw DSD in a .dff, written as a .dsf
  c   compress: DSD in a .dsf, written as a DST coded .dff
```

## Building

```
make
```

C++20, no dependencies. Built with `-fno-exceptions -fno-rtti`; no STL streams
and no STL containers are used, only stdio and explicit allocation.

## What it does

DSDIFF and DSF hold the same 1-bit DSD samples, but arrange them differently, so
converting is a matter of undoing (or applying) DST and rearranging:

| | DSDIFF (.dff) | DSF (.dsf) |
|---|---|---|
| chunk sizes | 64-bit big-endian | 64-bit little-endian |
| sample data | byte-interleaved per channel | planar, 4096-byte blocks per channel |
| bit order | MSB first | LSB first (`bits per sample` = 1) |
| compression | raw DSD or DST | raw DSD only |

DST (Direct Stream Transfer, ISO/IEC 14496-3 Part 3 Subpart 10) codes each
1/75 s frame independently: a 128-tap sign-based FIR predicts the next DSD bit
from the previous ones, and the prediction is corrected by an arithmetic-coded
residual. Filter coefficient sets and probability tables are sent per frame,
either verbatim or predicted with fixed predictors plus Golomb-coded residuals.

The last DSF block is padded to the fixed 4096 bytes per channel with `0x69`
(DSD silence); the exact length is carried by the sample count in the header, so
players trim the padding.

The `.dff` this encoder writes is laid out like the reference encoder's, chunk
for chunk:

| chunk | contents |
|---|---|
| `FVER` | format version 1.5.0.0 |
| `PROP`/`SND ` | `FS  `, `CHNL`, `CMPR`, `ABSS`, `LSCO` |
| `DST ` | `FRTE` frame count and rate, then one `DSTF` per frame |
| `DSTI` | frame index: data offset and length for each frame, for seeking |
| `COMT` | file history comment naming the encoder |

Per-frame CRCs (`DSTC`) are allowed by DSDIFF but the reference file carries
none, so this encoder emits none either — adding them would make the output less
like the original, not more.

## How the encoder designs its filter

The filter is the whole game: with the reference encoder's own coefficients
substituted into this encoder, output lands within 0.01% of the reference
encoder's own files, so the arithmetic coder, probability tables and table
coding here are already as good as its. Everything else follows from that.

Per frame, for all channels together:

1. **Autocorrelate** the ±1 mapped DSD bits. Since ±1 products are just bit
   comparisons, the whole correlation is popcounts of shifted XORs — 128 lags
   over 37632 samples costs a few hundred thousand word operations.
2. **Levinson-Durbin** for a 128-tap predictor, quantised so the largest tap
   fills the 9-bit coefficient field. No white noise correction: slackening the
   correlation measurably costs accuracy on DSD.
3. **Refine it against the objective DST actually scores.** This matters more
   than anything else here. Levinson-Durbin minimises the squared error of the
   prediction, but the decoder only ever takes its *sign*, so the filter is a
   binary classifier and the objective is a likelihood, not a least square. The
   refinement is a maximum-likelihood (logistic) gradient step: each sample is
   weighted by the probability the model gets it wrong, which the probability
   bin it landed in already measures. Weights are quantised to bitplanes so the
   weighted correlation is again just popcounts. Twelve steps with a decaying
   step size takes the frame from the Levinson solution to roughly 4% cheaper.
4. **Measure the probability table** from the refined prediction: for each
   |prediction| bin, how often the predicted bit was actually wrong. The table
   length is chosen to trade modelling accuracy against the cost of sending it.
5. **Arithmetic code** the "prediction was wrong" flags with those probabilities.

Steps 3 to 5 predict bit for bit exactly as the decoder does, sharing the filter
lookup table in `dst.cpp`.

Things that were tried and measurably lost, for the record: per-channel filters
and per-channel probability tables (both cost more to send than they save),
warm-starting the refinement from the previous frame's filter, smoothing the
gradient, weighting expensive errors more heavily, and nudging coefficients
towards cheaper codes. Coefficient quantisation turns out to cost nothing.

## Results

On a 4:15 DSD64 stereo file produced by `DstEncUi - Version '4.0.3'` (19119
frames, 180 MB of raw DSD):

| | DST data | whole file | ratio |
|---|---|---|---|
| Philips `DstEncUi` 4.0.3 | 78,188,710 | 78,657,280 | 2.3005 |
| dff2dsf | 78,133,734 | 78,602,392 | 2.3021 |

0.070% smaller on the same 19119 frames. Both measures agree because both files
carry the same chunks, including the 229 KB `DSTI` index; comparing whole files
against an encoder that omits the index would be meaningless, so the DST frame
data is the figure to watch.

Per frame this encoder wins on loud material and loses a little on quiet
material: on the first 200 frames, which are a quiet intro, it is 0.8% behind.

The refinement is what buys the last 0.4%: without it the Levinson-Durbin filter
alone lands about 4% behind, and a cheaper sign-error (perceptron) refinement
stops around 0.2% behind.

## Speed

Decoding the test file takes about 27 s on one core. Encoding is dominated by
the twelve refinement passes over every frame, and three inner loops account for
nearly all of it. Each has an AVX2 implementation, selected at run time through
`__builtin_cpu_supports`, with the scalar version kept as the fallback; both
produce byte-identical output, which is how the vector code is tested.

| inner loop | scalar | AVX2 | how |
|---|---|---|---|
| correlation of weights with the bit sequence | 10.2 s | 1.3 s | four words per step, popcount by nibble table shuffle |
| packing gradient weights into bitplanes | 3.2 s | 0.3 s | byte compare plus movemask, 32 samples at a time |
| the prediction itself | 2.8 s | 1.8 s | four taps per byte shuffle, see below |
| autocorrelation | 0.25 s | 0.08 s | same popcount kernel as the correlation |

Times are for 400 frames. Together they take the test file from 14m17s to
3m35s, a 4x speedup with the output byte-identical.

Two details made the vector code possible. The bitplane packing looked like it
needed a bit transpose; reversing the bytes first turns it into exactly what
`vpmovmskb` produces. And the prediction is sixteen table lookups per sample,
which vectorises only if the tables are small enough for a byte shuffle: four
taps index sixteen entries, but their partial sums do not fit in a byte, so each
coefficient is split into high and low nibbles and the halves are accumulated
apart and recombined as `16 * high + low`. Gathering from the byte-indexed
tables instead - the obvious approach - was tried and ran twice as slow as the
scalar code on this hardware.

## Verifying against FFmpeg

FFmpeg decodes both formats, and it is the reference for whether the DST this
encoder writes is really the standard format rather than something only this
decoder understands. `tests/verify.sh input.dff` checks the whole loop:

```sh
ffmpeg -v error -i input.dff  -c:a pcm_s24le -f md5 -   # reference samples
./dff2dsf d input.dff out.dsf
ffmpeg -v error -i out.dsf    -c:a pcm_s24le -f md5 -   # must match
./dff2dsf c out.dsf out2.dff
ffmpeg -v error -i out2.dff   -c:a pcm_s24le -f md5 -   # must match
./dff2dsf d out2.dff out2.dsf                           # must equal out.dsf
```

All of these hold on the test file against FFmpeg master (`74705b3`): decoding
is bit-exact with FFmpeg, FFmpeg decodes this encoder's DST stream to identical
samples, and compress/decompress round trips bit for bit. The decoder was also
checked against a synthetic uncompressed-DSD `.dff`, and both directions run
clean under ASan/UBSan including runs over randomly corrupted input.

## Source layout

| file | contents |
|---|---|
| `src/main.cpp` | command line, decode and encode loops |
| `src/dsdiff.cpp` | DSDIFF container parsing and frame iteration |
| `src/dst.cpp` | DST decoder |
| `src/dstenc.cpp` | DST encoder: filter design, refinement, arithmetic coding |
| `src/dsf.cpp` | DSF writing: deinterleave and bit reversal |
| `src/dsfread.cpp` | DSF reading: the same in reverse |
| `src/dffwrite.cpp` | DSDIFF writing |
| `src/bits.h`, `src/bitwrite.h` | bit reader and writer, JPEG-LS Golomb code |
| `src/common.h` | byte order helpers, buffered file access |

## Licensing

`src/dst.cpp`, `src/dst.h` and `src/bits.h` are derived from FFmpeg
(`libavcodec/dstdec.c`, `get_bits.h`, `golomb.h`), which is **LGPL-2.1-or-later**
— see `LICENSE.ffmpeg`. That license governs those files and therefore the
combined binary, regardless of the MIT `LICENSE` covering the rest of this
repository. The encoder is original work, but it uses the decoder's filter
evaluation, so it links against LGPL code either way. Writing an independent DST
decoder from the ISO specification would be the way to make the whole thing MIT.
