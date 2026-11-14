# flacdump

Lossless extraction / restoration of FLAC files.

```
flacdump c [options] input.flac out_prefix     -> out_prefix.wav + out_prefix.meta [+ out_prefix.N.ext]
flacdump d [options] inp_prefix restored.flac  <- inp_prefix.wav + inp_prefix.meta [+ inp_prefix.N.ext]
```

`c` decodes the FLAC stream into a standard `.wav` file (the PCM) and writes a
`.meta` file holding everything else needed to rebuild the original `.flac`
**bit-exactly**: container layout, metadata blocks, and every encoder decision
(block sizes, channel assignments, subframe types, predictor orders and
coefficients, rice partitions/parameters, ...). Embedded files (cover art in
PICTURE blocks, base64 data in VORBIS_COMMENT fields such as
`METADATA_BLOCK_PICTURE=`, `COVERART=` or other binary tags, JPEG/PNG images
inside ID3v2/APE tags (including base64 text in ID3v2/APE tag values) or other raw regions) are
written to separate files `out_prefix.1.jpg`, `out_prefix.2.png`,
`out_prefix.3.bin`, ... so they
can be handled by format-specific tools; the `.meta` only keeps their position,
size, type and a CRC. `d` rebuilds the identical file from all of them.

Options:

| option | meaning |
|--------|---------|
| `-v`   | print a breakdown of the `.meta` size (`-vv` adds model statistics) |
| `-n`   | (`c`) skip the built-in verification (by default `c` re-runs the restore in memory and compares it with the input) |
| `-i`   | (`c`) keep embedded files inside the `.meta` instead of writing `prefix.N.ext` files |
| `-x`   | (`c`) also write `prefix.side`, a plain uncompressed serialization of the side info (frame headers, predictor and rice parameters, metadata blocks) - only for comparisons with generic compressors |

The typical use is to store/transmit the PCM with a stronger audio compressor
and restore the original FLAC later, so the `.meta` file should be as small as
possible.

## Build

C++20, no STL, no exceptions, no RTTI; only the C runtime is used.

```
make            # g++ -std=c++20 -fno-exceptions -fno-rtti -ffp-contract=off -O2
```

`make test` runs `tests/regress.sh`: crafted inputs that once crashed, hung or
did not round-trip (garbage and truncated frames in the FLAC, corrupt `.meta`
files); valid ones must round-trip, corrupt ones must fail cleanly.
`tests/roundtrip.sh file.flac ...` checks the round trip of any FLAC files.

The program is one translation unit: `flacdump.cpp` includes the `*.inc` files
next to it (one per topic, and one per class with member functions) in
dependency order, so only `flacdump.cpp` is passed to the compiler.
`REFACTORING.md` describes the layout and how it was arrived at.

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
VORBIS_COMMENT, foreign metadata. Example with a large cover picture
(24-bit/48 kHz, 86 MB FLAC, 1.35 MB JPEG in a PICTURE block): `.meta` = 436 bytes
(of which ~360 are the 678-byte VORBIS_COMMENT), plus `prefix.1.jpg`.

### Integrated coding vs. a generic compressor

`-x` writes the side information in a plain form (raw frame headers; per
subframe the type, order, wasted bits, precision, shift, 16-bit coefficients,
partition order, rice parameters; the metadata blocks as they are). Compressed
with xz/bzip2 this is what a "dump the fields, then xz" approach would need:

| file                                   | plain side info | xz -9e  | bzip2 -9 | flacdump .meta |
|----------------------------------------|----------------:|--------:|---------:|---------------:|
| 24 bit, all verbatim (picture removed) | 142,579         | 4,456   | 7,608    | 395            |
| libFLAC 1.4.2 16 bit                   | 140,360         | 69,948  | 65,156   | 42             |
| libFLAC 1.2.1 `-8`                     | 194,560         | 93,940  | 84,072   | 84             |
| libFLAC 1.3.1, 24 bit                  | 142,361         | 101,784 | 97,901   | 138            |
| libFLAC 1.3.3, 24 bit (unknown build)  | 87,214          | 46,508  | 45,372   | 5,019          |
| ffmpeg `-compression_level 8`          | 36,752          | 18,044  | 18,177   | 65             |
| ffmpeg cholesky LPC (not replicated)   | 34,479          | 16,116  | 16,340   | 8,839          |
| flac `-A hann;welch;gauss(0.3)`        | 35,614          | 14,716  | 13,951   | 6,506          |

A generic compressor cannot know that most of these values are functions of
the PCM, so even for encoders whose analysis is not replicated the context
model stays well ahead. For the tag text alone the CM coder also beats raw
LZMA2 (e.g. 678-byte VORBIS_COMMENT: ~360 bytes vs 471 for `xz --format=raw`,
which additionally has no container overhead).

## How it works

### Container

* Bytes before `fLaC` (ID3v2 tags etc.) and after the last frame (ID3v1, APE,
  junk) are kept; JPEG/PNG images found inside them (validated by parsing the
  JPEG marker / PNG chunk structure, >= 256 bytes) become external files, the
  rest is coded with the CM byte model.
* ID3v2 (2.2/2.3/2.4) and APEv2 tags in those regions are parsed to find the
  exact values:
  * ID3 text and comment frames (`T***`, `TXXX`, `COMM`) in ISO-8859-1,
    UTF-8 or UTF-16 (with BOM, or big endian), and APE text items: values that
    are base64 - also line broken - are decoded exactly as described for
    VORBIS_COMMENT below (pictures/blobs become files, small blobs are coded as
    binary when cheaper). UTF-16 text is narrowed for decoding and widened
    again on restore.
  * ID3 `GEOB` objects: base64 text after a short binary header (as in Serato
    tags) is decoded; otherwise objects of 256 bytes or more become files.
  * ID3 `APIC`/`PIC` pictures and APE binary items (`filename\0data`): the
    data becomes a file, whatever the image format.

  Frames that are compressed, encrypted or unsynchronised are left alone.
  Outside parsed tags, runs of base64 text are detected heuristically.
* PICTURE blocks: the picture data becomes an external file (extension from
  its magic bytes: jpg, png, gif, bmp, webp, tif, otherwise bin); the block's
  other fields stay in the `.meta`.
* VORBIS_COMMENT values that are base64 (standard alphabet, with or without
  `=` padding, optionally wrapped into fixed-length lines with LF or CR LF;
  they must re-encode to exactly the same text) are decoded to binary:
  * a FLAC PICTURE structure (`METADATA_BLOCK_PICTURE=`): the image becomes an
    external file, the other fields are coded like a PICTURE block;
  * an image (`COVERART=` etc.): external file;
  * any other binary (e.g. Serato `SERATO_*` tags, embedded PDFs, text): an
    external file if it is at least 256 bytes (extension from magic bytes:
    pdf, zip, gz, xz, 7z, ogg, flac, txt, otherwise bin), otherwise - and with
    `-i` - it is coded as binary inside the `.meta` when that is cheaper than
    coding the base64 text (random data and hex strings stay text).
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
check of the PCM, so `d` refuses a `.wav` that does not belong to the `.meta`.
External files are referenced by number (`prefix.N.ext`, numbered in file order)
and checked by size and CRC-16 when restoring. The format is
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
* Work buffers are static arrays sized for the format limits (8 channels,
  65535-sample blocks, 16 MB metadata blocks): about 121 MiB of BSS that is
  only paged in as it is used. Heap memory is used only for data without a
  format bound: the input FLAC, the `.meta`, per-frame bookkeeping, the list of
  metadata blocks and (in `d`) raw byte regions.
* base64 values that would decode to more than 16 MB are kept as text.
* At most 100000 metadata blocks per file.
* WAV files above 4 GB use 0xFFFFFFFF size fields (sample count is then taken
  from the file size).
