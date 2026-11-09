# dtsdpack

dtsdpack is a lossless compressor for the core of a DTS stream, given the lossless audio it was
coded from. A DTS-HD Master Audio stream carries a lossy DTS core and a lossless extension (XLL);
decoded, the extension gives back the encoder's input exactly. dtsdpack codes the core against that
audio: nearly every quantization index, scale factor, ADPCM predictor, high-frequency vector, LFE
sample and bit allocation of the core is what the DTS encoder makes of that audio, so what is left
to code is where the encoder chose otherwise. Decompression, with the same WAV, restores the core
stream byte for byte.

The core of `s.2.dts` (4 minutes 25 seconds of 5.1 at 1509 kbps; 50008260 bytes) compresses to
396620 bytes (0.79%); without its audio, dtsdpack's model gets 71.76% and `xz -9e` 88.21%.

The core substream's extensions are coded too: XCH (6.1's back centre) and XXCH (7.1's other
channels) as more channels, X96 (DTS 96/24's 24-48 kHz band and the core's residuals) as subbands
of its own. A 96 kHz stream's core is the lower half of a 64-band analysis of its audio, so a WAV at
twice the core's rate is analysed that way: DTS-HD MA 96 kHz cores go to 1.6-6.9%, where nothing
lined up before.

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
s2.dtsc: 50008260 -> 396620 bytes (0.79%, -9); 24855 frames, 24855 packed; 0 bytes between frames; 5 channels and the LFE against the WAV (offset 512, correlation 1.0000)
  bytes                             input     packed
  bytes kept as bytes                   0          4    0.00%
  headers, container              1059444        161    0.02%
  side information                2521040     129606    5.14%
  bit allocations                 2174812     202747    9.32%
  scale factors                   3512740      34389    0.98%
  subband samples                40631706      31539    0.08%
  bits after the subframes         108518          1    0.00%
  total                          50008260     398447    0.80%
  the audio's indexes: 99.89% of the samples; the RMS's scale factors: 98.76%
  the encoder's searches on the audio: 84.90% of the ADPCM predictors; HF VQ: 78.10% of the groups, 91.65% of the vectors in them
  the binding: core channel 0 = WAV 2; core channel 1 = WAV 0; core channel 2 = WAV 1; core channel 3 = WAV 4; core channel 4 = WAV 5; LFE = WAV 3
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
| `s.2.dts` (5.1, 1509 kbps, 4:25) | 50008260 | 396620 (0.79%) | 71.76% | 88.21% |
| its first 3 MB (6.8 s) | 1275608 | 12558 (0.98%) | | |
| 3 MB from its middle (5.7 s) | 1068372 | 13750 (1.29%) | | |
| FFmpeg's `master_audio_7.1_24bit.dts` (12.5 s) | 2358064 | 25975 (1.10%) | | |

Compressing the whole of `s.2.dts`'s core takes 30 seconds (1.7 MB of core a second) and 39 MB of
memory, decompressing it 33 seconds and 32 MB, on one thread of a 2.1 GHz Xeon. The last three
rows were the tuning set (below); the first, the whole of `s.2.dts`, wasn't. A 96 kHz stream's
64-band analysis costs more: `96-24.dts`'s core (1.9 MB) compresses in 5.2 seconds, 3 of them c's
search for the alignment, and decompresses in 2.0.

FFmpeg's small DTS-HD MA samples (`xll_*`, 7 frames each) round-trip too:

| Sample | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `xll_51_24_48_768` | 7168 | 722 (10.07%) | the WAV's 5.1 |
| `xll_71_24_48_768_0` | 7168 | 561 (7.83%) | a downmix of its 7.1: every channel at -3 dB, each surround its side and rear surround |
| `xll_71_24_48_768_1` | 7168 | 694 (9.68%) | the same, the rear surrounds' sign inverted |
| `xll_xch_61_24_48_768` | 7168 | 1037 (14.47%) | its 6.1's 5.1, the back centre in the surrounds at about -3 dB; and in the XCH extension, the back centre |
| `xll_71_24_96_768` | 7168 | 496 (6.92%) | the 96 kHz WAV's 64-band analysis's lower half, a downmix as the 48 kHz one's |
| `xll_51_16_192_768_1` | 7168 | 3517 (49.07%) | coded from a version of the 192 kHz WAV dtsdpack hasn't: nothing lines up |

Before the downmixes were found, the 7.1 and 6.1 ones went to 36-38%: only 3 of their core's
channels lined up with a WAV channel, and those at the wrong gain in the 7.1 ones. With its XCH
extension kept as bytes (about 140 of each frame's 1024), the 6.1 one went to 26.79%; before the
64-band analysis, the 96 kHz one to 42.16%.

Streams of FFmpeg's samples site (`samples.ffmpeg.org/A-codecs/DTS/dts/`, some 5 MB pieces of
longer ones) and FATE's `dts_es.dts`, their cores against FFmpeg's decodes (lossless for DTS-HD MA,
lossy for the others: the decoder's output, which the core's indexes are mostly the re-analysis
of):

| Stream | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `96-24.dts` (DTS 96/24, 5.1, 20 s) | 1888194 | 124477 (6.59%) | the 96 kHz decode's 64-band analysis's lower half (95.99% of its indexes); its X96 extension, 12% of each frame, the upper half and the residuals (99.97%) |
| `Master Audio 5.0 96khz.dts` | 1768548 | 29017 (1.64%) | the 96 kHz WAV's 64-band analysis's lower half (99.57%) |
| `Master Audio 7.1.dts` | 1945604 | 28696 (1.47%) | a downmix of its 7.1 |
| `3-1.dts` (DTS-HD MA 3.1) | 2782596 | 15574 (0.56%) | its 3.1 |
| `ES 6.1 16bit.dts` (DTS-ES, XCH) | 5233800 | 133906 (2.56%) | 6.1 by XCH, decoded |
| FATE's `dts_es.dts` (DTS-ES, XCH) | 503000 | 15302 (3.04%) | the same |
| `open bitrate.dts` (stereo, open rate) | 5242416 | 1617464 (30.85%) | decoded, but only part of it lines up (66% of the indexes) |

No stream at hand has XXCH in its core substream: FATE's XXCH samples (`xxch_71_24_48_2046`,
`xbr_xxch_71_24_48_3840`, `x96_xxch_71_24_96_3840`, 7 frames each) have it in their extension
substreams. Their XXCH frames moved into their core frames after the subframes (EXT_AUDIO set,
EXT_AUDIO_ID 6), the extension substream dropped, make core-substream XXCH streams that FFmpeg
decodes as 7.1; dtsdpack round-trips them against those decodes (34.59%, 37.53%, 41.60%: their
signals near full scale make their cores line up poorly with any decode, as their original cores
with theirs, 35%).

## How it works

### The stream

The core stream is cut into core frames and the bytes between them, as dtssplit and
[dts2tsv](../dts2tsv) cut it. Each frame is parsed into its record (`syntax.inc`: dts2tsv's walk of
the core, down to every subband sample's quantization index), written back as bits right away and
compared with the frame: if they match, the record is coded; if not, or the frame doesn't parse,
the frame is kept as bytes, and so is everything between frames, so any input round-trips. The same
walk that reads a frame's bits and writes them is, at record level, what codes and decodes the
record: the model (`model.inc`) is its IO, so the two directions can't drift apart.

The extension the frame's header names (EXT_AUDIO_ID), after the core's subframes, is found as
FFmpeg finds it: its sync word at a 4-byte boundary, searched for back from the frame's end; XCH's
and X96's frame size the distance to the end, XXCH's frame header's CRC right. Then it is parsed
into the record as FFmpeg parses it:

- **XCH** (6.1's back centre): its coding header and subframes are the core's syntax for one more
  channel (without an LFE, in the core's subsubframes).
- **XXCH** (7.1's other channels, one channel set as FFmpeg has it): a frame header (the speaker
  masks' size, the core's mask), a channel set header (its channels' speaker mask, the downmix
  coefficients of those the core's channels have embedded), then the core's syntax for its
  channels, a joint channel numbered from its own; the headers' reserved bits and CRCs, and the bits
  after its subframes, are bits of the record.
- **X96** (DTS 96/24): its own syntax, from FFmpeg's `parse_x96_frame()`: each channel's subbands
  from a first one (27 in the streams seen: the core's residuals) to 64, one scale factor each, a bit
  allocation that is 0 for none (the decoder's noise), 1 for VQ (a 16-sample vector each two
  subsubframes) or the quantizer's ABITS + 1, coded as differences by the quantization index books;
  joint intensity for whole channels' upper bands.

XCH's and XXCH's channels are channels after the core's, which the model codes as it codes the
core's, against the WAV; X96's subbands the model codes against the WAV's 64-band analysis (below).
What is left is the bits from the subframes to the extension's sync word (the zeros to the next
4-byte boundary; in X96's stream, a 16-bit word before it too) and the bits after it. Where the
extension doesn't parse, its bytes are the bits after the subframes again.

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
  first guess (none yet at this energy: the nearest energy's), then the last one, then the
  difference. Where the ABITS seen at the energies either side are the same, it holds 98% of the
  time, where neither is 56%: a context, with the energy, its change, the neighbours' and the
  loudest subband's.

A WAV at twice the core's rate (96 kHz for a 48 kHz core) is analysed by the 64-band filterbank
instead: the same transform of 64 bands, by FFmpeg's 1024-tap X96 prototype, each block 64 of the
WAV's samples. Its lower 32 bands are the core's subband samples, as a DTS 96/24 stream's and
DTS-HD MA 96 kHz streams' cores are (99.2-99.6% of the indexes; the spec's encoder decimates the
audio first, but these encoders didn't). X96's subbands are what TS 102 114's encoder makes of it:
the upper 32 bands, and from X96's first subband up the lower ones less the core's decoded
samples (the residuals). On them, X96's elements as the core's, with histories of their own:
99.97% of `96-24.dts`'s indexes. Its decoded WAV's joint subbands are their source's times the
joint scale factor, so where the source has a level the scale factor is the projection of the one
on the other (100% of those); a subband without bits is the decoder's noise, uniform to half the
scale factor, so its scale factor is 2.1 times the peak (94%). Each kind of subband's rule, and the
RMS rule, are counted as the stream goes, and the one right more often is the prediction: a
lossless WAV may make the RMS rule the better. The core's scale factors get the same choice, with a
second rule (the first entry above 0.99 times the RMS: `96-24.dts`'s core's, 67% where the RMS
rule's is 49%).

c finds how the core lines up with the WAV before it starts (`call.inc`'s `find_alignment()`, in
doubles; the header keeps the result). The offset: from the core's first 4 MB, the 64 blocks with
the most energy, decoded to their plain subband samples, against the WAV's analysis at offsets
-1024..1536 for each of its channels (times 2 for the 64-band analysis); the best for the core's
channel 0 gives it. Then each core channel, XCH's and XXCH's too (`find_mixes()`): a core may be a downmix of the WAV, as a DTS-HD MA 7.1 stream's is (its
lossless extension's primary channel set is the 5.1 the core was coded from, each surround the side
and the rear surround, every channel scaled: coefficients of DTS's downmix table, FFmpeg's
`ff_dca_dmixtable`, Q15). So over
up to 256 of the frames read first, a weighted least squares fit (each decoded subband sample by 1 /
its step squared: the finer, the surer) of the core channel against every WAV channel and every
pair of them gives its best single channel and its best pair, each with its gains as fitted and as
the downmix table's entries either side (signed); of those, the one that makes the most of the
channel's indexes exactly is its binding (a simpler one unless another makes 0.5% more), if it
makes a fifth of them. The LFE gets the remaining WAV channel whose decimation correlates best with
the first 1024 LFE samples. `-s` shows the binding:

```
  the binding: core channel 0 = 0.707092 WAV 2; core channel 1 = 0.707092 WAV 0; ...; core channel 3 = 0.707092 WAV 4 + 0.707092 WAV 6; ...; LFE = WAV 3
```

### The model

Each element is binarised, and each decision goes to one of eight families (`fam.inc`, ac3pack's,
m4vpack's and h264p's): counters chosen by context indices, mixed in the logistic domain, refined
by APMs, then range coded (`rc.inc`). `famvars.inc` lists the variables the model gives each
family; `IDX/l9/<family>.idx` declares level -9's indices over them and their tunable parameters,
which `IDX/idx2inc.pl` turns into `MOD/l9/` (`./mk.sh mod`; the format:
[IDX/IDX-FORMAT.md](IDX/IDX-FORMAT.md)).

- **hdr**: the frame's header, the coding header and the subframes' headers, each field against its
  last values (the extension's headers too: XCH's and X96's frame size against the one its sync
  word's distance to the frame's end makes); the container's kinds and lengths.
- **sid**: a subband's PMODE (against the gain above), TMODE, joint scale factors, and where the
  WAV isn't bound, its predictor, its HF VQ vector and the LFE samples.
- **abt**: the bit allocations.
- **scl**: the scale factors, against the RMS's (the LFE's too).
- **hit**: whether an index that isn't the audio's is the other side's, and an LFE sample's whether
  it is the audio's.
- **smp**: the indexes otherwise (and all of them where the WAV isn't bound): magnitude and sign.
- **vqr**: the ranks of the predictors and the HF VQ vectors.
- **raw**: bytes kept as bytes, the bits after the subframes, those before the extension's sync word,
  and XXCH's other bits.

The parameters started as ac3pack's tuned level -9's (its sid, mag and raw families'), then were
tuned as ac3pack's are: `./mk.sh dump 9 DIR stream wav ...` writes what each family codes,
`./mk.sh famtune 9` builds `famtune`, which codes a family's dumps again, and `IDX/ftune.py`
hill-climbs each family's parameters against them, a bit at a time; `IDX/import.pl` folds the
winners into the `.idx` files. The dumps were of three streams: `s.2.dts`'s first 3 MB and 3 MB of
its middle, and FFmpeg's `master_audio_7.1_24bit.dts`. Two runs of three passes each (the second
after abt's last contexts, on new dumps) took each family's output on them down by: abt 6.8%, sid
9.3%, hit 8.9%, hdr 7.1%, scl 4.1%, raw 3.3%, smp 1.5%, vqr 1.3%; and the whole of `s.2.dts`'s
core, which wasn't in them, by 4.7% (415945 to 396547 bytes). `./mk.sh check 9 stream wav ...`
confirms that the tuning build, the shipping build and the build without AVX2 write the same
files.

### The format

"dtsdpack", the format's version (4), the level, the WAV's binding (the offset; each core channel's
WAV channel, the second of its mix and their gains, Q23; the LFE's channel; the WAV's samples to
the core's, 1 or 2; the WAV's format, size and the CRC-32 of its first 64 KB), then the
range coded chunks (a frame as records, bytes between frames, a frame kept as bytes), the end, and
the CRC-32 of the core stream, which `d` checks.

## What it doesn't do

- A core channel is bound to at most two of the WAV's channels: one that mixes more (a stereo core
  downmixed from 5.1) is bound to its best two, or isn't. A 192 kHz stream's core isn't bound at
  all (nothing the 32- or 64-band analysis makes of its WAV lines up).
- The LFE is predicted at 64x only (LFF 2); a 128x LFE (LFF 1) is coded by the model alone.
- XXCH has been tried only on the streams made from FATE's (above), and only one channel set is
  parsed, as in FFmpeg; X96 and XXCH in the extension substream aren't in the core (dtssplit keeps
  the extension substream).
- A decoded WAV can't say what X96's joint scale factors are where their source decodes to
  silence: those are coded against their last values alone (most of `96-24.dts`'s X96 bytes).
- One thread, one level.

## Build

```
./mk.sh release
```

or by hand:

```
g++ -O3 -march=x86-64-v3 -ffp-contract=off -std=c++11 -fno-exceptions -fno-rtti -ILib3 -pthread -o dtsdpack dtsdpack.cpp
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
