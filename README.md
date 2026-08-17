# dff2dsf

A standalone codec for DSDIFF (`.dff`) files with DST lossless compression — the
format the Philips encoder (`DstEncUi`) produces — and plain DSF (`.dsf`).

```
usage: dff2dsf d input.dff output.dsf
       dff2dsf c input.dsf output.dff [options]

  d   decode: DST coded or raw DSD in a .dff, written as a .dsf
  c   compress: DSD in a .dsf, written as a DST coded .dff

  --threads N   encode N frames at a time (default 1, 'auto' for one
                per core); the output does not depend on the count

options, which select the optional chunks of a written .dff:
  --disable-DSTI   frame index, 12 bytes per frame, for seeking
  --disable-COMT   comment naming the encoder, with a timestamp
  --disable-ABSS   absolute start time, in PROP/SND
  --disable-LSCO   loudspeaker configuration, in PROP/SND
  --enable-DSTC    per-frame CRC over the DSD, 16 bytes per frame
```

## Building

```
make                     # or, with no build system at all:
c++ -O2 -std=c++20 -mavx2 -pthread src/dff2dsf.cpp -o dff2dsf

make windows             # cross build with mingw-w64
```

`-mavx2` (or any `-march` that implies it, such as `-march=native`) selects the
vector kernels; without it the scalar ones are compiled and encoding is about
four times slower. Nothing is chosen at run time, so nothing has to be detected
and no compiler runtime library is involved.

The whole program is a single translation unit, and `src/dff2dsf.cpp` is the
only file that includes anything at all: the C and C++ library headers, then
Lib3, then one header per module in dependency order. Nothing to link, and one
place that says what the program depends on.

## I/O

Both conversions run as coroutines on **Lib3** (`src/Lib3/`), the same library
and the same shape `dxt5comp` uses: `CoroFileProc` reads the input file into a
64 KB buffer and writes the output one out, and the conversion itself is
straight-line code that pulls and pushes bytes through the coroutine's two pins.
A pin that runs dry or fills up yields — which suspends the whole call stack,
parser and codec and frame loop together — until the driver has refilled or
flushed it, and then resumes exactly where it stopped. `Stream` in `common.h` is
the thin layer between the two: it copies whole runs out of the pin's window
rather than going a byte at a time through `get()`/`put()`.

The stack is saved and restored by copying it, which is the reason nothing on
the way down to a `get()` or a `put()` may be large. Every working buffer here
is static or a member already; measured, the deepest yield copies 377 bytes
decoding, 457 encoding, and 2665 with `--threads 4`, against the 64 KB the
library allows. `make frames` checks the constraint with
`-Wframe-larger-than=65536`: the only function over it is Lib3's own 256 KB
stack pad, which is what that budget is measured from.

Two things a stream cannot do: seek back to a header once its contents are
known, and read the output back. So the writers do that afterwards, with plain
stdio on the file the driver has just finished writing — `DsfWriter::patch()`
fills in the DSF file size, sample count and data size; `DffWriter::patch()`
walks the `DSTF` chunks to build the `DSTI` index, appends `COMT`, and fills in
the `FRM8` and `DST ` sizes and the frame count. Everything else, in both
directions, goes through the pins.

## Memory

Every buffer is sized at build time for the largest stream the program accepts —
DSD512, six channels, so a frame of 37632 bytes per channel — rather than for the
stream in hand. Nothing is sized, grown or allocated at run time as a result:
decoding and single threaded encoding allocate nothing whatsoever. That costs
5.8 MB of BSS against 62 KB of code, and at DSD64 most of it is never touched —
peak RSS is 10.4 MB either direction.

Two things resisted that. The `DSTI` index is 12 bytes per frame with no bound on
the frame count, so it is not kept at all: `patch()` recovers it by walking the
`DSTF` chunks the coroutine has just written, which is one 12 byte read per frame
against pages still in cache, and reproduces the remembered index byte for byte
over the test file's 19119 frames. And `--threads N` needs an encoder and a slot
buffer per worker, which is a runtime choice by definition — those are the
program's only two allocations, one block each, however many threads are asked
for.

Integers are spelled as fixed-width types throughout — `int32_t`, `uint32_t`,
`uint64_t` and the rest — with `size_t` kept for sizes and offsets into memory,
and `int` surviving only where the language fixes it, in `main`'s signature.
64-bit values are printed through `PRIu64` rather than `%llu`, which is both the
matching spelling and the one the Windows runtime actually wants.

Pointers are declared through a small set of `__restrict` qualified types in
`common.h` — `ByteP`, `CByteP`, `WordP`, `CLutP` and so on — rather than by
repeating the qualifier at each declaration. Nothing here is ever aliased: the
frame going in, the frame coming out, the history, the per-sample codes, the
bitplanes and the lookup tables are all distinct objects, and saying so lets the
compiler keep values in registers across stores. The types are used only where
that promise actually holds; plain types mark the places it does not. It buys no
measurable speed on this workload — the hot loops already copy into locals — but
it makes the aliasing rules the code relies on part of its signatures, and the
output is byte for byte the same with GCC, Clang, mingw, scalar and vector
builds alike, which is what a wrong `restrict` would break.

Under Valgrind, counting every allocation the process makes including the C and
C++ runtimes' own:

| | allocations | peak heap | peak RSS |
|---|---|---|---|
| print usage | 1 | 72 KB | |
| decode | 5 | 72 KB | 10.4 MB |
| encode | 10 | 74 KB | 10.4 MB |
| encode `--threads 4` | 20 | 15.8 MB | 18.3 MB |

The first rows are libstdc++'s startup pool and stdio's own per-file structures
and buffers; none of them are this program's. The threaded run adds its two
blocks — four encoders and six slots, 15.1 MB of the 15.8 — and libstdc++'s
per-thread bookkeeping.

C++20, and Lib3 for the I/O. Built with `-fno-exceptions -fno-rtti`; no STL
streams and no STL containers are used, only stdio and explicit allocation. The
one place the standard library does the work is threading, which is `<thread>`,
`<mutex>` and `<condition_variable>` — hence `-pthread`. Compiles
clean at `-Wall -Wextra -Wpedantic` with GCC, Clang and mingw-w64; the only
platform specific pieces are 64-bit file offsets and reading the clock, both in
`common.h` and keyed on the C runtime rather than the OS, since MinGW and MSVC
share a runtime but agree on little else. The Windows build was checked by running it: under Wine it encodes
the same input to byte-identical DST data and round trips bit-exactly, with the
AVX2 paths active.

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
none, so they are off by default here too; `--enable-DSTC` writes them. The
DSDIFF 1.5 specification puts one after each frame, holding a four byte CRC over
the *uncompressed* DSD that frame codes — so it checks the decoder as much as
the file:

    CRC(x) = (x^32 · I(x)) mod2long G(x),   G(x) = x^32 + x^31 + x^4 + 1

with `I(x)` the frame's interleaved DSD bits, most significant bit of the first
byte first. That is an ordinary MSB-first CRC with generator `0x80000011`, zero
initial value, no reflection and no final inversion. They cost 16 bytes per
frame, 0.4% of a typical file. This decoder checks them whenever a file carries
them, whoever wrote it, and reports how many frames matched.

The `--disable-` options remove the last two rows of that table, and `ABSS` and
`LSCO` from within `PROP`; each also has an `--enable-` form, so a default can
be restated rather than remembered. Everything else — the `FRM8` form, `FVER`,
`FS`, `CHNL`, `CMPR` and the sound data — is what a decoder needs, and is always
written. The options change the container only: the DST frames come out byte for
byte the same whatever is switched on or off, which is what makes them safe to
use. FFmpeg decodes every combination to identical samples, and each round trips
bit-exactly.

The CRCs were checked two ways: against a bitwise long division written straight
from the specification's own recipe rather than from the table-driven code, and
by corrupting files — a flipped bit in a `DSTC` value, and one in the middle of
a frame's coded data that still decodes without complaint, are both caught.

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
   correlation measurably costs accuracy on DSD. A `[1 2 1]/4` smoothing pass
   over the solution takes off the part of the fit that is estimation noise
   rather than signal; it wins on most frames but not all, so the smoothed and
   unsmoothed designs are both kept.
3. **Refine it against the objective DST actually scores**, from each of those
   two starting points, keeping whichever result codes cheaper. This matters
   more than anything else here. Levinson-Durbin minimises the squared error of the
   prediction, but the decoder only ever takes its *sign*, so the filter is a
   binary classifier and the objective is a likelihood, not a least square. The
   refinement is a maximum-likelihood (logistic) gradient step: each sample is
   weighted by the probability the model gets it wrong, which the probability
   bin it landed in already measures. Weights are quantised to bitplanes so the
   weighted correlation is again just popcounts. Twelve steps with a decaying
   step size takes the frame from the Levinson solution to roughly 4% cheaper.
   The refinement is a local search, so the two starts land in different basins
   and following both is worth another 0.12%, at the cost of doubling encode
   time.
4. **Measure the probability table** from the refined prediction: for each
   |prediction| bin, how often the predicted bit was actually wrong. The table
   length is chosen to trade modelling accuracy against the cost of sending it.
5. **Arithmetic code** the "prediction was wrong" flags with those probabilities.

Steps 3 to 5 predict bit for bit exactly as the decoder does, sharing the filter
lookup table in `dst.h`.

`ALGORITHM.md` describes both directions in full — the frame syntax, the table
coding, the arithmetic coder, the filter design and the refinement — at the level
of detail you would need to write another implementation.
`docs/philips-encoder.md` compares this against the reference encoder's own
algorithm, read out of a decompilation of its DLL: where the two agree, where
they differ, and which of its ideas were worth taking. The coefficient smoothing
and the probability scale above both came from there.

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
| dff2dsf | 78,038,932 | 78,507,698 | 2.3049 |

0.192% smaller on the same 19119 frames. Both measures agree because both files
carry the same chunks, including the 229 KB `DSTI` index; comparing whole files
against an encoder that omits the index would be meaningless, so the DST frame
data is the figure to watch.

Per frame this encoder wins on loud material and still loses a little on quiet
material: on the first 200 frames, which are a quiet intro, it is 0.6% behind.
Comparing frame by frame there shows the gap is two-sided rather than
systematic, which is what following two starting points is for.

The refinement is what buys the last 0.4%: without it the Levinson-Durbin filter
alone lands about 4% behind, and a cheaper sign-error (perceptron) refinement
stops around 0.2% behind.

## Speed

Decoding the test file takes about 24 s on one core. Encoding is dominated by
the twelve refinement passes over every frame, and three inner loops account for
nearly all of it. Each has an AVX2 implementation, chosen at compile time by
whether the target has AVX2; both produce byte-identical output, which is how
the vector code is tested - build it both ways and compare.

| inner loop | scalar | AVX2 | how |
|---|---|---|---|
| correlation of weights with the bit sequence | 10.2 s | 1.3 s | four words per step, popcount by nibble table shuffle |
| packing gradient weights into bitplanes | 3.2 s | 0.3 s | byte compare plus movemask, 32 samples at a time |
| the prediction itself | 2.8 s | 1.8 s | four taps per byte shuffle, see below |
| autocorrelation | 0.25 s | 0.08 s | same popcount kernel as the correlation |

Times are for 400 frames. Together they take the test file from 14m17s to
3m35s, a 4x speedup with the output byte-identical. Refining from two starting
points then spends half of that back, for 0.12% smaller output: the file now
encodes in 6m50s. The same holds across
targets: `-mavx2`, `-march=native` and a scalar build all produce the same
bytes, and `-ffp-contract=off` in the default flags keeps that true by stopping
the filter design arithmetic from contracting into FMAs on targets that have
them.

The decoder has no vector kernel to hide behind, so what it costs is what its
inner loop is made of, and several things in there were arguments rather than
constants. The channel count is a template parameter now, which unrolls the
per-channel loop and lets everything a channel reaches through its element
mapping - its filter table, its probability table and that table's length - be
gathered once instead of re-indexed per sample. The leading stretch where a
channel may code at even odds is a second parameter, so the tail after it is
instantiated with the test known false: the decoder codes a hundred thousand
samples per frame and only the first hundred-odd can take that branch. The same
two splits are applied to the encoder's coding loop and to the .dsf
deinterleave, the predictor order in the table search - one, two or three terms -
is a parameter so the innermost dot product unrolls, and the frame CRC and the
correlation kernels' word-aligned case go the same way.

Measured, min of nine runs each with the builds interleaved: decoding the test
file goes from 25.1 s to 23.9 s, about 5%, and callgrind puts the instruction
count 8.1% lower, which is the part that does not depend on the machine being
quiet. Encoding does not move - 8.5 to 8.6 s whichever of the four combinations
is built, a spread with no ordering in it - and callgrind agrees, 1.8% fewer
instructions against a workload that spends nearly all of them in the three
vector kernels above. Those had no branches to remove, which is the whole
result: this pays where the hot loop is scalar and the count is small, and
nowhere else.

Frames are the other axis. Each one starts the decoder's history from the same
fixed pattern, so nothing carries over from the frame before and frames can be
encoded in any order — `--threads N` does N at once. Reading and writing stay in
the calling thread and stay in order: frames move through a small ring of slots,
that thread fills every free slot and then blocks until the oldest frame has been
coded and writes it, while workers take the next uncoded frame in turn. The ring
is two slots longer than the worker count so reading and writing overlap
encoding. Nothing else is shared — each worker has its own encoder, since every
buffer in one is rewritten per frame anyway.

| threads | test file | speedup |
|---|---|---|
| 1 (default) | 6m50s | |
| 4 | 1m53s | 3.6x |

The output does not depend on the thread count: on the full file every thread
count produces the same 78,038,932 bytes of DST data, and on a clip the files are
byte for byte identical, including from the Windows build. That is the property
worth keeping, and it is what the tests check.

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
./dff2dsf c out.dsf out3.dff --threads 4                # must equal out2.dff
```

All of these hold on the test file against FFmpeg master (`74705b3`): decoding
is bit-exact with FFmpeg, FFmpeg decodes this encoder's DST stream to identical
samples, and compress/decompress round trips bit for bit. The whole 4:15 file was
checked that way on four threads too, matching the reference md5 and producing
the same 78,038,932 bytes as one thread. The decoder was also checked against a
synthetic uncompressed-DSD `.dff`, and both directions run clean under ASan/UBSan
including runs over randomly corrupted input — with `--param asan-stack=0`, since
ASan's stack red-zones and a coroutine that saves its stack by copying it cannot
both be right about that memory; the threaded encoder is clean under
ThreadSanitizer as well, including the awkward cases — fewer frames than threads,
a truncated input, and a failing write, none of which may leave a worker waiting.

## Source layout

One header per module, each holding a class with its methods defined inline —
there are no separate declaration and implementation files, and no out-of-line
definitions. None of them include anything; `src/dff2dsf.cpp` includes them all,
in order, so they are read in that order too.

| file | contents |
|---|---|
| `src/dff2dsf.cpp` | command line, decode and encode loops |
| `src/dsdiff.h` | DSDIFF container parsing and frame iteration |
| `src/dst.h` | DST decoder |
| `src/dstenc.h` | DST encoder: filter design, refinement, arithmetic coding |
| `src/dsf.h` | DSF writing: deinterleave and bit reversal |
| `src/dsfread.h` | DSF reading: the same in reverse |
| `src/dffwrite.h` | DSDIFF writing |
| `src/encpool.h` | encoding frames on several threads |
| `src/bits.h`, `src/bitwrite.h` | bit reader and writer, JPEG-LS Golomb code |
| `src/crc.h` | the DSDIFF frame CRC carried in `DSTC` |
| `src/Lib3/` | the coroutine library the I/O runs on, as supplied |
| `src/common.h` | byte order, buffered file access, the platform bits |

## Licensing

`src/dst.h` and `src/bits.h` are derived from FFmpeg
(`libavcodec/dstdec.c`, `get_bits.h`, `golomb.h`), which is **LGPL-2.1-or-later**
— see `LICENSE.ffmpeg`. That license governs those files and therefore the
combined binary, regardless of the MIT `LICENSE` covering the rest of this
repository. The encoder is original work, but it uses the decoder's filter
evaluation, so it links against LGPL code either way. Writing an independent DST
decoder from the ISO specification would be the way to make the whole thing MIT.
