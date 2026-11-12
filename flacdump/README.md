# flacdump

Lossless extraction / restoration of FLAC files.

```
flacdump c [options] input.flac out_prefix     -> out_prefix.wav + out_prefix.meta
flacdump d [options] inp_prefix restored.flac  <- inp_prefix.wav + inp_prefix.meta
```

`c` decodes the FLAC stream into a standard `.wav` file (the PCM) and writes a
`.meta` file holding everything else needed to rebuild the original `.flac`
**bit-exactly**: container layout, metadata blocks, and every encoder decision
(block sizes, channel assignments, subframe types, predictor orders and
coefficients, rice partitions/parameters, ...). `d` rebuilds the identical
file from the two.

Options:

| option | meaning |
|--------|---------|
| `-v`   | print a breakdown of the `.meta` size (`-vv` adds model statistics) |
| `-n`   | (`c`) skip the built-in verification (by default `c` re-runs the restore in memory and compares it with the input) |

The typical use is to store/transmit the PCM with a stronger audio compressor
and restore the original FLAC later, so the `.meta` file should be as small as
possible.

## Build

C++20, no STL, no exceptions, no RTTI; only the C runtime is used.

```
make            # g++ -std=c++20 -fno-exceptions -fno-rtti -ffp-contract=off -O2
```

`-ffp-contract=off` matters: the model replays floating point computations of
the original encoders and both directions must compute bit-identical values.
GCC (ISO mode) and clang builds, `-O2` or `-O3 -march=native`, produce
identical `.meta` files and can decode each other's output. On 32-bit x86, SSE2
math is required.

## How small is the .meta?

Results on a test corpus (real-world files from archive.org encoded by libFLAC
1.2.1 ... 1.4.3, plus files made with flac 1.4.3 at all presets and ffmpeg):

| encoder / settings                   | FLAC size | .meta |
|--------------------------------------|----------:|------:|
| libFLAC 1.4.3 `-5`, 16 bit, 40 s     | 4.4 MB    | 33 B  |
| libFLAC 1.4.3 `-8`, 16 bit, 40 s     | 4.4 MB    | 152 B |
| libFLAC 1.4.2, 24 bit, 3 min         | 33.6 MB   | 137 B |
| libFLAC 1.3.1, 24 bit, 3.3 min       | 35.3 MB   | 136 B |
| libFLAC 1.2.1 `-8`, 16 bit, 3.4 min  | 18.4 MB   | 82 B  |
| ffmpeg flac, compression 5 / 8 / 12  | 4.4 MB    | 61 / 63 / 72 B |
| 6 channels, mono, 8/12/20/32 bit     | 1-12 MB   | 29-300 B |

For comparison, storing the same decisions without prediction costs about
0.2-0.3% of the FLAC size (tens of kilobytes for an album track).

A good part of what remains is genuine information: tag text in
VORBIS_COMMENT, cover art, foreign metadata.

## How it works

### Container

* Bytes before `fLaC` (ID3v2 tags etc.) and after the last frame (ID3v1, junk)
  are kept verbatim.
* Metadata blocks are modelled: STREAMINFO fields are predicted from the
  restored audio (min/max block and frame sizes, total samples, MD5),
  SEEKTABLE points are coded as frame indices (usually predicted from the
  seek point spacing), PADDING as a length, VORBIS_COMMENT with a vendor string
  dictionary and an order-3 context-mixing text model, other blocks with the
  CM model (or stored if that does not help).
* The audio region is split into segments: frames that can be re-encoded
  bit-exactly from (PCM + decisions), decodable frames that can't (stored raw,
  PCM still goes to the WAV), and undecodable junk (stored raw). Every modelled
  frame is re-encoded and compared during `c`.

### Encoder decisions

All decisions are coded with an adaptive binary arithmetic coder. The key to a
small `.meta` is that most decisions can be *predicted from the PCM* by
replaying the original encoder's analysis, so usually only a "prediction was
right" flag (a small fraction of a bit) is coded:

* **LPC coefficients**: libFLAC's analysis is reproduced exactly: apodization
  windows (`tukey`, `partial_tukey`, `punchout_tukey`, `subdivide_tukey`
  including its quirks), autocorrelation in the flavours used by different
  builds (1.4 double ascending/descending, 1.3 float SSE "old"/"new", 1.2 x87,
  fma-contracted arm64 builds), Levinson-Durbin and coefficient quantization.
  Window values are computed with correctly rounded `cosf`, with libFLAC 1.2/1.3.3
  double `cos`, and with an emulation of glibc's `cosf`. ffmpeg's flacenc
  analysis (welch window, autocorrelation, Levinson, quantizer) is replicated
  too. Candidates are tried in most-recently-used order and the index is coded.
* **LPC order**: libFLAC's order guess (`FLAC__lpc_compute_best_order`), its
  exhaustive search (`-e`), or ffmpeg's order selection methods (EST, 4LEVEL,
  LOG, SEARCH). Which one, the encoder's maximum order and other parameters are
  learned from the stream itself.
* **Channel assignment, subframe type, window choice**: a simulation of
  libFLAC's `process_subframe_` cost estimates for the L/R/M/S signals predicts
  the stereo decorrelation and the winning subframe; ffmpeg's stereo estimator
  is used as a second predictor.
* **Partition order and rice parameters**: libFLAC's estimators (both the
  pre-1.4.3 and 1.4.3+ versions, including the old "suggested parameter" for
  partition order 0 and the ln(2)^2 error scaling of <= 1.3.1) and ffmpeg's.
* Wasted bits, fixed predictor order, frame header fields, frame numbers,
  block sizes: predicted from the signal / previous frames.

Version-dependent behaviour is detected from the vendor string as a hint and
then learned from the data (votes on which variant reproduced the actual
choices), so re-tagged or unknown encoders still work - they just cost more.

Anything that is not predicted is coded explicitly (e.g. coefficient deltas
against the best guess), so arbitrary valid FLAC streams are supported; files
from unknown encoders simply produce a larger `.meta`.

## .meta format

`'F' 0xD1` followed by a single arithmetic-coded stream: container layout and
metadata blocks, then one segment per frame/junk run, then the fields that
depend on the restored audio (STREAMINFO sizes/MD5, seek tables) and a 16-bit
check of the PCM, so `d` refuses a `.wav` that does not belong to the `.meta`. The format is
defined by the model code; there is no compatibility guarantee between
different versions of the tool.

## WAV output

PCM WAV (`WAVE_FORMAT_EXTENSIBLE` for >2 channels or bit depths that are not a
multiple of 8; samples are left-justified in their container like `flac -d`
does). 8-bit samples are unsigned as required by WAV. `d` only needs the PCM
samples, so the WAV may be rewritten by other tools as long as the samples stay
the same.

## Limitations

* Native FLAC only (Ogg FLAC is rejected).
* Frames whose header bits are non-canonical, have reserved bits set or non-zero
  padding are stored raw (still lossless).
* Frames with CRC errors are kept as raw bytes and their audio is not in the WAV.
* The whole input FLAC is loaded into memory during `c`.
* WAV files above 4 GB use 0xFFFFFFFF size fields (sample count is then taken
  from the file size).
