# dtsdpack

dtsdpack is a lossless compressor for the core of a DTS stream, given the lossless audio it was
coded from. A DTS-HD Master Audio stream carries a lossy DTS core and a lossless extension (XLL);
decoded, the extension gives back the encoder's input exactly. dtsdpack codes the core against that
audio: nearly every quantization index, scale factor, ADPCM predictor, high-frequency vector, LFE
sample and bit allocation of the core is what the DTS encoder makes of that audio, so what is left
to code is where the encoder chose otherwise. Decompression, with the same WAV, restores the core
stream byte for byte.

The core of `s.2.dts` (4 minutes 25 seconds of 5.1 at 1509 kbps; 50008260 bytes) compresses to
435729 bytes (0.87%); without its audio, dtsdpack's model gets 71.52% and `xz -9e` 88.21%.

The core comes from [dtssplit](../dtssplit) (`prefix.dtsc`) or from any DTS stream; the WAV is the
stream decoded, for instance by FFmpeg:

```
ffmpeg -i in.dts -c:a pcm_s24le in_full.wav
dtssplit c in.dts in
dtsdpack c in.dtsc in_full.wav in.dtsd
```

## Usage

```
dtsdpack c [options] input.dtsc input.wav output.dtsd
dtsdpack d [options] input.dtsd input.wav output.dtsc
```

- `c` compresses the core stream against the WAV, `d` decompresses it with the same WAV.
- `-v` (with `c`) decompresses the output afterwards and compares it with the input.
- `-s` (with `c`) shows the input and output size of each kind of syntax element, and how much of
  the core the audio predicted.
- `-q` turns off the statistics and the progress line.
- `-p` shows the progress line even where stderr is not a terminal.
- `-9` (with `c`) is the model's level, the only one this version has; `d` takes it from the file.

The core stream's input and output can be `-` for stdin and stdout, except with `-v`, which reads
the files again; the WAV is read once from its start, so it can be a pipe too (not both of them
from stdin). The WAV can be any PCM WAV of 16, 24 or 32 bits (RIFF or RF64): FFmpeg's 24-bit
decode, or its 32-bit one (a 24-bit decode in 32 bits).

A WAV that doesn't line up with the core, or isn't a WAV, isn't an error: the core is then coded
by the model alone (as about 72% of its size), and the summary line says why. `d` checks that its
WAV is the one `c` had: its format, its size and the CRC of its first 64 KB are in the header.

```
$ dtsdpack c -s s2.dtsc s2_full.wav s2.dtsd
s2.dtsc: 50008260 -> 435729 bytes (0.87%, -9); 24855 frames, 24855 packed; 0 bytes between frames; 5 channels and the LFE against the WAV (offset 512, correlation 1.0000)
  bytes                             input     packed
  bytes kept as bytes                   0          4    0.00%
  headers, container              1059444        162    0.02%
  side information                2521040     138225    5.48%
  bit allocations                 2174812     232560   10.69%
  scale factors                   3512740      35102    1.00%
  subband samples                40631706      31549    0.08%
  bits after the subframes         108518          1    0.00%
  total                          50008260     437602    0.88%
  the audio's indexes: 99.89% of the samples; the RMS's scale factors: 98.76%
  the encoder's searches on the audio: 84.90% of the ADPCM predictors; HF VQ: 78.10% of the groups, 91.65% of the vectors in them
```

(`-s`'s byte counts are the coder's estimate, a little above what it writes.) Where stderr is a
terminal, or with `-p`, a call shows what the audio is and a progress line, as
[ac3pack](../ac3pack) does.

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | not a dtsdpack file, damaged data, or not the WAV it was compressed with (`d`) |
| 5 | `-v` found a difference, out of memory, a level this build lacks, or an internal error |

## Results

| Stream | Core | dtsdpack | Without the WAV | `xz -9e` |
|---|---:|---:|---:|---:|
| `s.2.dts` (5.1, 1509 kbps, 4:25) | 50008260 | 435729 (0.87%) | 71.52% | 88.21% |
| its first 3 MB (6.8 s) | 1275608 | 13624 (1.07%) | | |
| 3 MB from its middle (5.7 s) | 1068372 | 14885 (1.39%) | | |
| FFmpeg's `master_audio_7.1_24bit.dts` (12.5 s) | 2358064 | 28910 (1.23%) | | |

Compressing the whole of `s.2.dts`'s core takes 29 seconds (1.7 MB of core a second) and 52 MB of
memory, decompressing it 31 seconds and 45 MB, on one thread of a 2.1 GHz Xeon.

FFmpeg's small DTS-HD MA samples (`xll_*`, 7 frames each) round-trip too. Where their core's
channels are the WAV's channels (`xll_51_24_48_768`) the core goes to 8.9%; where only some are (the
7.1 and 6.1 ones: 3 of the core's 5 channels line up with a WAV channel, the surrounds presumably
being mixes) only those are bound (33-37%); where the WAV's rate is 96 or 192 kHz (the core is coded
from a 48 kHz version of it) nothing is (40-48%).

## How it works

### The stream

The core stream is cut into core frames and the bytes between them, as dtssplit and
[dts2tsv](../dts2tsv) cut it. Each frame is parsed into its record (`syntax.inc`: dts2tsv's walk of
the core, down to every subband sample's quantization index), written back as bits right away and
compared with the frame: if they match, the record is coded; if not, or the frame doesn't parse,
the frame is kept as bytes, and so is everything between frames, so any input round-trips. The same
walk that reads a frame's bits and writes them is, at record level, what codes and decodes the
record: the model (`model.inc`) is its IO, so the two directions can't drift apart.

### The audio

The encoder's input is the WAV. `anal.inc` computes what the encoder computed from it, in integers
(64- and 128-bit), so that compression and decompression agree everywhere:

- **The subband samples**: the encoder's 32-band analysis filterbank (FFmpeg's `dcaenc`'s
  transform, with the core's prototype filter: FILTS 0 the non-perfect one), the window of block t
  ending at sample 32 t + 512 of the WAV, in the decoder's units (its gain, 2). Quantized by the
  core's own step sizes, scale factors and ADPCM predictions (round to nearest, halves away from
  zero), they give 99.9% of the core's indexes. Where an index isn't the audio's, it is nearly always
  the one on the other side of the rounding boundary, and the nearer the boundary, the likelier: the
  encoder's arithmetic errs by about its signal's level. So whether an index is the audio's is coded
  by how near the boundary the audio's value lies, in steps and in the channel's loudest subband's
  RMS (log2, 16 buckets each), and by the subband's recent misses, from adaptive counts straight to
  the range coder (15-bit probabilities: the families' are 12).
- **The scale factors**: the table's entry nearest (linearly) the RMS of the values it scales (an
  ADPCM subband's prediction residual): 98.8% of them. The tables' first entries repeat values (1,
  1, 2, 2, 2, 2, 3, ...), and among those the encoder's choice is not the rule's (the HF VQ
  subbands, quiet, are there: 90% of theirs): the model knows when the prediction is one of them.
- **The ADPCM predictors**: Levinson-Durbin on the autocorrelation of the subframe's subband samples
  and the 4 before them, then the code book's entry nearest the coefficients. The predictor is coded
  as its rank in that order (`vqr`, as a bucket and the bits below its top): it is the first 85% of
  the time. Whether a subband predicts (PMODE) follows the gain of the entry the encoder would
  choose: above about 2^(5.5/4) over the subframe it does, nearly always.
- **The high-frequency VQ vectors**: a two-stage search, the group of 32 entries whose mean is
  nearest the samples (by 16 / the scale factor), then the entry of the group nearest; coded as the
  group's rank and the entry's (78% and 92% first).
- **The LFE**: the WAV's LFE channel decimated by a filter near the decoder's 512-tap interpolation
  prototype, then quantized by the scale factor nearest its RMS. dtsdpack decimates by the
  prototype and corrects it with a 13-tap filter at the LFE's rate that a least squares fit learns
  as the stream goes (forgetting, ridged; solved by LDL' in a small deterministic soft float). The
  LFE scale factor comes after the samples in the bitstream; the model codes it with the first of
  them, and the samples against the predictions at its step.
- **The bit allocations**: mostly a function of the subband and its energy (as the scale factor
  index its RMS makes; an ADPCM subband's residual's) that drifts slowly. The ABITS a subband had the
  last time it had this energy, in any channel, is its ABITS 85% of the time (its last one 63%): the
  first guess, then the last one, then the difference; with the energy, its change, the neighbours'
  and the loudest subband's as contexts.

c finds how the core lines up with the WAV before it starts (`call.inc`'s `find_alignment()`, in
doubles; the header keeps the result): from the core's first 4 MB, the 64 blocks with the most
energy, decoded to their plain subband samples, against the WAV's analysis at offsets -1024..1536
for each of its channels; the best for the core's channel 0 gives the offset, and each other core
channel gets the WAV channel that correlates best there (0.9 at least, else none). The LFE gets the
remaining WAV channel whose decimation correlates best with the first 1024 LFE samples.

### The model

Each element is binarised, and each decision goes to one of eight families (`fam.inc`, ac3pack's,
m4vpack's and h264p's): counters chosen by context indices, mixed in the logistic domain, refined
by APMs, then range coded (`rc.inc`). `famvars.inc` lists the variables the model gives each
family; `IDX/l9/<family>.idx` declares level -9's indices over them and their tunable parameters,
which `IDX/idx2inc.pl` turns into `MOD/l9/` (`./mk.sh mod`; the format:
[IDX/IDX-FORMAT.md](IDX/IDX-FORMAT.md)).

- **hdr**: the frame's header, the coding header and the subframes' headers, each field against its
  last values; the container's kinds and lengths.
- **sid**: a subband's PMODE (against the gain above), TMODE, joint scale factors, and where the
  WAV isn't bound, its predictor, its HF VQ vector and the LFE samples.
- **abt**: the bit allocations.
- **scl**: the scale factors, against the RMS's (the LFE's too).
- **hit**: whether an index that isn't the audio's is the other side's, and an LFE sample's whether
  it is the audio's.
- **smp**: the indexes otherwise (and all of them where the WAV isn't bound): magnitude and sign.
- **vqr**: the ranks of the predictors and the HF VQ vectors.
- **raw**: bytes kept as bytes, and the bits after the subframes.

The parameters are ac3pack's tuned level -9's (its sid, mag and raw families'), not yet tuned for
these families; the tools are there (`./mk.sh dump`, `./mk.sh famtune`, `IDX/ftune.py`, as in
ac3pack). `./mk.sh check 9 stream wav ...` confirms that the tuning build, the shipping build and
the build without AVX2 write the same files.

### The format

"dtsdpack", the format's version (1), the level, the WAV's binding (the offset, each core channel's
WAV channel and the LFE's, the WAV's format, size and the CRC-32 of its first 64 KB), then the
range coded chunks (a frame as records, bytes between frames, a frame kept as bytes), the end, and
the CRC-32 of the core stream, which `d` checks.

## What it doesn't do

- Core channels that aren't one of the WAV's (a 7.1 stream's core surrounds, presumably mixes of
  the WAV's) aren't bound: those are coded by the model alone. So are cores coded from a resampled
  WAV (96 and 192 kHz streams).
- The LFE is predicted at 64x only (LFF 2); a 128x LFE (LFF 1) is coded by the model alone.
- Extensions in the core substream (XCH, XXCH, X96) are bytes after the subframes, kept as they are.
- One thread, one level.

## Build

```
./mk.sh release
```

or by hand:

```
g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -fno-exceptions -fno-rtti -ILib3 -o dtsdpack dtsdpack.cpp
```

AVX2 or not, the files are the same. It needs GCC's or Clang's `__int128`. `MOD/` is generated
from `IDX/` (perl) and kept in the repository, so a plain build needs no perl; `tables.inc` is
generated from FFmpeg's `libavcodec/dcadata.c` by `tools/mktables.py`, and `IDX/l9/*.idx`'s first
declarations by `tools/mkidx.py`.

## License

The tables in `tables.inc` (the QMF and LFE prototypes, the quantizer and scale factor tables, the
ADPCM and HF VQ code books) are FFmpeg's, so dtsdpack is licensed under the GNU Lesser General
Public License, version 2.1 or later. The framework (`fam.inc`, `cm*.inc`, `rc.inc`, `Lib3/`, `IDX/`)
is ac3pack's.
