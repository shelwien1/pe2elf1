# dtsdpack

dtsdpack is a lossless compressor for the core of a DTS stream, given the lossless audio it was
coded from. A DTS-HD Master Audio stream carries a lossy DTS core and a lossless extension (XLL);
decoded, the extension gives back the encoder's input exactly. dtsdpack codes the core against that
audio: nearly every quantization index, scale factor, ADPCM predictor, high-frequency vector, LFE
sample and bit allocation of the core is what the DTS encoder makes of that audio, so what is left
to code is where the encoder chose otherwise. Decompression, with the same WAV, restores the core
stream byte for byte.

The core of `s.2.dts` (4 minutes 25 seconds of 5.1 at 1509 kbps; 50008260 bytes) compresses to
117423 bytes (0.23%); without its audio, dtsdpack's model gets 71.76% and `xz -9e` 88.21%.

The core substream's extensions are coded too: XCH (6.1's back centre) and XXCH (7.1's other
channels) as more channels, X96 (DTS 96/24's 24-48 kHz band and the core's residuals) as subbands
of its own. A 96 kHz stream's core is the lower half of a 64-band analysis of its audio, so a WAV at
twice the core's rate is analysed that way: DTS-HD MA 96 kHz cores go to 0.7-5.8%, where nothing
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
s2.dtsc: 50008260 -> 117423 bytes (0.23%, -9); 24855 frames, 24855 packed; 0 bytes between frames; 5 channels and the LFE against the WAV (offset 512, correlation 1.0000)
  bytes                             input     packed
  bytes kept as bytes                   0          4    0.00%
  headers, container              1059444        154    0.01%
  side information                2521040      53998    2.14%
  bit allocations                 2174812       9973    0.46%
  scale factors                   3512740      26274    0.75%
  subband samples                40631706      28884    0.07%
  bits after the subframes         108518          1    0.00%
  total                          50008260     119287    0.24%
  the audio's indexes: 99.90% of the samples; the audio's scale factors: 99.07%
  the bit allocations' staircases: 95.53% of the ABITS, right in 99.27% of them
  the encoders' allocation table: 91.27% of the ABITS, right in 99.29% of them
  the encoder's searches on the audio: 99.76% of the ADPCM predictors; HF VQ: 92.13% of the groups, 92.63% of the vectors in them
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
| `s.2.dts` (5.1, 1509 kbps, 4:25) | 50008260 | 117423 (0.23%) | 71.76% | 88.21% |
| its first 3 MB (6.8 s) | 1275608 | 4689 (0.37%) | | |
| 3 MB from its middle (5.7 s) | 1068372 | 6651 (0.62%) | | |
| FFmpeg's `master_audio_7.1_24bit.dts` (12.5 s) | 2358064 | 4253 (0.18%) | | |

Compressing the whole of `s.2.dts`'s core takes 29 seconds (1.7 MB of core a second) and 50 MB of
memory, decompressing it 30 seconds and 43 MB, on one thread of a 2.1 GHz Xeon. The rows were all
in the tuning sets (below; the whole of `s.2.dts` since the HF VQ groups' scores were learned), and
`s.2.dts` in what the HF VQ groups' scores and the ADPCM predictors' codewords were learned from.
A 96 kHz stream's 64-band analysis costs more: `96-24.dts`'s core (1.9 MB) compresses and
decompresses in 3.2 seconds. The decoded WAVs' searches cost too; every stream below goes at about
500 KB of core a second or more both ways, the 7-frame samples (7 KB, 9-21 ms each way, a third of
that the kernel mapping the pages they touch) at 330 KB a second or more: the searches run on
every value of a stream's first 2048, which is all of theirs.

FFmpeg's small DTS-HD MA samples (`xll_*`, 7 frames each) round-trip too:

| Sample | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `xll_51_24_48_768` | 7168 | 554 (7.73%) | the WAV's 5.1 |
| `xll_71_24_48_768_0` | 7168 | 449 (6.26%) | a downmix of its 7.1: every channel at -3 dB, each surround its side and rear surround |
| `xll_71_24_48_768_1` | 7168 | 499 (6.96%) | the same, the rear surrounds' sign inverted |
| `xll_xch_61_24_48_768` | 7168 | 936 (13.06%) | its 6.1's 5.1, the back centre in the surrounds at about -3 dB; and in the XCH extension, the back centre |
| `xll_71_24_96_768` | 7168 | 392 (5.47%) | the 96 kHz WAV's 64-band analysis's lower half, a downmix as the 48 kHz one's |
| `xll_51_16_192_768_1` | 7168 | 1711 (23.87%) | the 64-band analysis's lower half of the 192 kHz WAV decimated by 2, about |
| `xll_51_16_192_768_0` | 7168 | 1760 (24.55%) | the same (the same WAV, another core) |

Before the downmixes were found, the 7.1 and 6.1 ones went to 36-38%: only 3 of their core's
channels lined up with a WAV channel, and those at the wrong gain in the 7.1 ones. With its XCH
extension kept as bytes (about 140 of each frame's 1024), the 6.1 one went to 26.79%; before the
64-band analysis, the 96 kHz one to 42.16%. The 192 kHz ones went to 48% until their WAV was
decimated by 2 (`wav.inc`'s `WAV_DEC2`): the 96 kHz signal's 64-band analysis then lines up with
their cores, as a 96 kHz stream's does with its WAV's, 7 samples later. A lowpass with its centre
0.58 of an input sample earlier lines up best (by a least squares fit of the decimation filter to
the cores' samples, and by their sizes); the samples' content ends below 15 kHz, where any flat
lowpass does as well, so the cutoff, 40 kHz, is a guess. The fit is near, not exact: their
cores' indexes are the audio's 82% of the time (the other samples' 95-99%), as in the best fit
70% of the centre channel's (finely coded: 7 bits a sample) lie within half a step of it (a 48
kHz sample's 91%), and no longer filter, nor a decimation by 4 to the 32-band analysis, does
better.

Streams of FFmpeg's samples site (`samples.ffmpeg.org/A-codecs/DTS/dts/`, some 5 MB pieces of
longer ones) and FATE's `dts_es.dts`, their cores against FFmpeg's decodes (lossless for DTS-HD MA,
lossy for the others: the decoder's output, which the core's indexes are mostly the re-analysis
of, and whose values the scale factor and bit allocation searches find their scale factors and bit
allocations in):

| Stream | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `96-24.dts` (DTS 96/24, 5.1, 20 s) | 1888194 | 79515 (4.21%) | the 96 kHz decode's 64-band analysis's lower half (95.98% of its indexes); its X96 extension, 12% of each frame, the upper half and the residuals (99.94%) |
| `Master Audio 5.0 96khz.dts` | 1768548 | 4497 (0.25%) | the 96 kHz WAV's 64-band analysis's lower half (99.82%) |
| `Master Audio 7.1.dts` | 1945604 | 6987 (0.36%) | a downmix of its 7.1 |
| `3-1.dts` (DTS-HD MA 3.1) | 2782596 | 4811 (0.17%) | its 3.1 |
| `ES 6.1 16bit.dts` (DTS-ES, XCH) | 5233800 | 7106 (0.14%) | 6.1 by XCH, decoded (1.39% before the bit allocation search) |
| FATE's `dts_es.dts` (DTS-ES, XCH) | 503000 | 2419 (0.48%) | the same (1.95%) |
| `open bitrate.dts` (stereo, open rate) | 5242416 | 3891 (0.07%) | decoded (99.99% of the indexes, 99.95% of the scale factors, 98.25% of the bit allocations, 99.98% of the HF VQ groups, 97.86% of the ADPCM predictors; 30.85% before the binding's fit stopped trusting silent subbands at fine steps, below, 2.19% before the scale factor search, and 0.85% before the bit allocation search) |

No stream at hand has XXCH in its core substream: FATE's XXCH samples (`xxch_71_24_48_2046`,
`xbr_xxch_71_24_48_3840`, `x96_xxch_71_24_96_3840`, 7 frames each) have it in their extension
substreams. Their XXCH frames moved into their core frames after the subframes (EXT_AUDIO set,
EXT_AUDIO_ID 6), the extension substream dropped, make core-substream XXCH streams that FFmpeg
decodes as 7.1; dtsdpack round-trips them against those decodes (16.87%, 18.57%, 15.20%; 21.64%
and 22.63% before the binding stopped taking a gain a clipped decode's fit makes, below). Their
signal is a silent frame, then white noise near full scale; the decoder undoes XXCH's downmix
pre-scaling (every core channel times 1.41), so the decodes clip, and only partly line up with
any core (their own cores against their core-only decodes, which don't clip, make 18.91%).

FATE's other DTS samples (`dcadec-suite`'s; 7 frames each, the 96 kHz ones 14) round-trip too,
against FFmpeg's decodes of the whole streams. Their signal is the same noise near full scale, and
their decodes clip too (0.3-1.2% of the samples):

| Stream | Core | dtsdpack |
|---|---:|---:|
| `core_51_24_48_768_0`, `_1` (5.1) | 7168 | 2074 (28.93%), 2098 (29.27%) |
| `xch_61_24_48_768` (6.1 by XCH) | 7168 | 2092 (29.19%) |
| `x96_51_24_96_1509` (X96) | 14084 | 3600 (25.56%) |
| `x96_xch_61_24_96_3840` | 14084 | 2886 (20.49%) |
| `xbr_51_24_48_3840`, `xbr_xch_61_24_48_3840` (XBR: its residuals in the decode) | 14084 | 2646 (18.79%), 3618 (25.69%) |
| `xll_x96_51_24_96_1509` (DTS-HD MA, X96) | 14084 | 1956 (13.89%) |

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
4-byte boundary; in X96's stream, a 16-bit word before it too) and the bits after it. Those are
coded as the bytes of the frame they end in, against the last frame's at the same places from the
end: the encoders' patterns go by the frame's 32-bit words (`open bitrate.dts`'s frames end in 1s
to a 32-bit boundary, or 2 short of it, or none, then a 0 word; `ES 6.1 16bit.dts`'s XCH sync word
comes after one of a few hundred 16-bit words, 0x5A5A the most), which made `Master Audio 7.1.dts`
10% smaller. Whether they are all 0, and how many of their last bytes are, are coded by their
bits to the next 32-bit boundary, the fill's odds going by them (`open bitrate.dts` 14% smaller).
Where the frame has auxiliary data (AUXF), the bits after the subframes are its byte count, 0s to
a 32-bit boundary, its sync word and the data, which an encoder repeats frame after frame: those
bytes go against the last frame's from that boundary, where the data is, not from the end
(`Master Audio 7.1.dts`'s sit 24 or 28 bytes from it: 3.8% smaller). Even so, such a frame's
bits cost a bit or two, more where the fill's size changes. So a tail of 8 bytes or more the same
as one of the last 32 of its kind and size that weren't all 0 (the auxiliary data's are a few,
one by the fill's size) is that one, coded as which, the last first (`Master Audio 7.1.dts` 4.0%
smaller, `Master Audio 5.0 96khz.dts` 2.0%). `ES 6.1 16bit.dts`'s encoder leaves the bits before
the XCH sync word as an earlier frame had them at the same place: those words are what its frame
buffers held (of a ring of 8: 850 of its 2600 frames' are what one of the 8 frames before has
there, 568 of them the frame 8 before; another 316 a copy of the XCH data started a few bytes
early), and the bits before those bytes, the rest of the core's last byte, are 0. So those bits
are first offered as each of the last 8 frames' bytes at their place, the bits before them 0 (8
back, then 1 to 7; the frames' bytes kept as they are coded), a flag each where they aren't 0 or
one already offered (its core 11.5% smaller; the other streams' no different). Where the extension doesn't
parse, its bytes are the bits after the subframes again.

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
  RMS (log2, 16 buckets each), and by the subband's recent misses and its channel's (how many of its
  last 32 missed: 0, 1, 2-3, more), from adaptive counts straight to the range coder (15-bit
  probabilities: the families' are 12). The channel's say 32 times sooner when the WAV has stopped
  being what the encoder had, as in the frames after a cut stream's start (its decoder's state not
  the encoder's), where nearly all of a channel's indexes miss by a step for a while, and in its
  last frame (with them, `3-1.dts`'s core 6.8% smaller, 29% of that in its first six frames and 40%
  in its last, `s.2.dts`'s first 3 MB 6.2%, FATE's XXCH samples 4-5%, the whole `s.2.dts` 0.24%;
  four of the 7-frame samples 9-14 bytes bigger, their counts split). A lossless WAV's go by the
  channel's level at the sample (the root of all its subband samples' squares) instead of its
  loudest subband's: the encoder's arithmetic errs by about all its signal there (held out, 3.6%
  fewer bits for the decisions of `s.2.dts`'s first 3 MB; its core 1.2% smaller). A decoded WAV's
  misses are the lattice's, not the arithmetic's, and keep the loudest subband's. An index that is
  neither, the audio's 4 or more, is coded by whether it lies within 4 of the audio's, then its
  side (that one's, or the other) and its distance in unary; else as its magnitude, from the top
  bit down, as any index is without the WAV (the core 2.1% smaller in all: the 7.1 DTS-HD MA
  streams' and `3-1.dts`'s 11%, `96-24.dts`'s 2.6%, `s.2.dts`'s 0.5%). A miss beside a smaller
  index costs less as its magnitude (`96-24.dts`'s, where X96's residuals are in the core's
  values, 1.6 KB more otherwise). The misses beyond the rounding boundary are mostly where the
  WAV ends: a piece cut from a longer stream has its last frame's analysis look past it at the
  audio that came next (`s.2.dts`'s first 3 MB's last frame, 718 of its 734).
- **The WAV's end**: block t's window ends at sample 32 t + 512, so the core's last frame looks 512
  samples past the WAV's end (a decoded WAV as long as the core's frames are), at what its encoder
  had there: 0s there missed three quarters of that frame's indexes, about 2 KB of `s.2.dts`'s first
  3 MB (with the next 1024 samples of the whole `s.2.dts`'s WAV after it, it went from 5764 bytes to
  3767). So past its end each channel goes on as the linear prediction of order 64 its last 4096
  samples make (their autocorrelation under a half parabola, the newest weighted most, a 10000th of
  white noise in it; Levinson's recursion in the soft float, the LFE fit's), run on for 2048
  samples; or as 0s, where the encoder's input ended there: where the subbands' analysis first
  reaches past the end, compression takes whichever makes more of that subframe's indexes (those not
  ADPCM's or VQ's) and codes it in a bit. The pieces cut from longer streams went on (`s.2.dts`'s
  first 3 MB 7.2% smaller, `master audio 7.1 24bit.dts` 5.1%, FATE's `dts_es.dts` 8.4%, the others
  0.5-4%); the 7-frame samples, whole programs, ended (as before, within 2 bytes: the LFE, whose
  windows reach on further, has the continuation before the bit). But no continuation is the audio
  a cut stream went on with: where a window lies past the end by more than 192 samples of its 512,
  most of its indexes miss either way. So a second bit says whether the continuation made fewer
  than half of those indexes (of that subframe's), and where it did, such an index is coded without
  the audio, and a scale factor's values leave those samples out (`s.2.dts`'s first 3 MB 5.3%
  smaller, FATE's `dts_es.dts` 6.6%, `master audio 7.1 24bit.dts` 4.2%, `ES 6.1 16bit.dts` 3.7%,
  the other cut ones 0.5-2.7%; the 7-frame samples within a byte). Their last frame is still most
  of what a frame costs without the audio: 1.0-1.5 KB of each, 15-42% of their cores' bytes.
- **The scale factors**: the table's entry nearest (linearly) the RMS of the values it scales (an
  ADPCM subband's prediction residual): 98.9% of them. The tables' first entries repeat values (1,
  1, 2, 2, 2, 2, 3, ...), and among those the encoder's choice is not the rule's (the HF VQ
  subbands, quiet, are there: 90% of theirs): the model knows when the prediction is one of them.
  `s.2.dts`'s encoder puts its fronts' and surrounds' top subband (31, a VQ one) 1 to 4 entries
  above the rule's a third and a quarter of the time (its centre's 5%), in stretches, and the
  likelier the louder the subband below (no fixed offset, leak from below or other rule makes
  them): the channel's class and the subband below's energy are contexts (its core 0.9% smaller).
  It is the encoder's arithmetic: its top subband has its bottom one's signal in it, at about
  2^-20 (-120 dB). The subband is near silence there (its RMS 1 to 3 of the decoder's units, the
  scale factor table's first entries), so that leak, nothing to the other subbands, is about as
  loud as its own. Of the other subbands' values, the bottom one's alone go with what is left of
  the top one's values once the encoder's VQ vector is taken from them (their regression's
  coefficient -3.7e-7, the others' nothing near); no rounding of the filterbank's prototype or
  cosines, 32-bit floats or block floating point makes it, and couplings of the other VQ
  subbands with their mirrors (30 with 1, 29 with 2, 28 with 3) make next to nothing. Taking
  2^-20 of the bottom subband's values from the top one's (not an exact copy of the leak) makes
  the RMS rule right 73.6% of the time there, from 65.9%, and the HF VQ search's first 51% of
  the vectors, from 46%. So a lossless WAV's top subband is that, or m 2^-23 of the bottom one
  taken from it for m even from 0 to 16, where that m has made the RMS rule and the VQ search's
  group right clearly more often than 8 (`s.2.dts`'s core 3.8% smaller: its fronts' and
  surrounds' top subband's VQ vectors 38.7 KB to 33.2, their scale factors 14.8 to 13.6; 3 MB of
  its middle 2.7%; the other lossless WAVs' within 0.2%, a decoded WAV's none). What is left of
  the leak goes with how loud the bottom subband is against the top one: their powers' ratio, in
  doublings, is a context of the top subband's scale factor (for the subband below's energy) and
  of its VQ vector's ranks (for its scale factor), a lossless WAV's (`s.2.dts`'s core 0.5%
  smaller). The next subband down, 30, has subband 1's signal in it the same way, weaker and the
  other way: 3 2^-23 of subband 1's values added to its own make its scale factors and VQ vectors
  right a little more often (0.6 and 1.0 points; subbands 29 and 28 with 2 and 3 nothing), so it
  is taken so (m from -2 to 6, the same way), with the same contexts (`s.2.dts`'s core 0.5%
  smaller). Both at the core's rate only: a 96 kHz WAV's 64-band analysis's subbands 30 and 31
  lie mid-band (taking the leaks from them made `Master Audio 5.0 96khz.dts`'s core 9 bytes
  bigger). The rest of the top subband's leak is noise, about 2^-40 of the bottom subband's
  power: the louder the bottom subband against the top one, the more often the top one's VQ
  vector misses (93% of `s.2.dts`'s in their group at a power ratio of 2^30, 55% at 2^36, 25%
  at 2^39), no coefficient of the bottom subband's values (fixed, by block, by channel, or
  delayed) nor any filter of them taking it out. Its power is in the encoder's RMS, though, so
  the top subband's RMS rule adds 2^-40 of the bottom one's mean square (its scale factor right
  81.6% of `s.2.dts`'s time, from 79.2%; its core 0.1% smaller), and the HF VQ groups' scores
  don't learn from the vectors it swamps (below).
  A decoded WAV isn't the encoder's input, and its RMS makes the encoder's scale factors less often
  (58% of `open bitrate.dts`'s, 0.7% of its HF VQ subbands'); but its values are the decoder's,
  the indexes dequantized at the step and scale factor, or a VQ vector scaled. So the scale factor
  search tries the entries from 3 below the RMS rule's to 5 above, rebuilds the values as the
  decoder would at each (the indexes nearest them in the quantizer's range, dequantized as it does;
  the VQ vector the two-stage search finds, or the entry most nearly a multiple of the values,
  scaled as it does), and takes the one whose reconstruction lies nearest, where its error is a
  quarter of the next best's at most (its ratio a context): 99.95% of `open bitrate.dts`'s scale
  factors. A decoded VQ subband's scale factor lies 1 to 12 entries above its RMS rule's (the
  vectors' entries are small), so its search goes to 12 above, and where it isn't sure, its best
  is the prediction all the same.
- **The ADPCM predictors**: Levinson-Durbin on the autocorrelation of the subframe's subband samples
  and the 4 before them (unwindowed: with a sample fewer or more at either end, the entry nearest is
  the encoder's 14-24% of the time), then a search of the code book in two stages: the nearest of
  64 codewords, each standing for a block of 64 entries (64 g to 64 g + 63), then the block's entry
  nearest the coefficients. The codewords aren't the decoder's: `tools/pvqblocks.py` learns them
  (`pvqblocks.inc`) from five DTS-HD MA streams' predictors (`./mk.sh pvqdump`), by learning vector
  quantization from the medians of their blocks' entries. The entry nearest of all is the
  encoder's predictor 88-91% of the time, the two stages' 99.6-99.8% (98.7% of `s.2.dts`'s with the
  codewords learned from the other four's alone). (Its misses gave it away: where the nearest of
  all isn't the encoder's, the two are in the same block of 64 1.6% of the time, and the encoder's
  is its block's nearest 96%.) The predictor is coded as its block's rank, then its rank in the
  block (`vqr`, each as a bucket and the bits below its top): both are first 99.76% of `s.2.dts`'s
  time, where its rank among all was first 89.5% (its core 11.7% smaller, FFmpeg's
  `master_audio_7.1_24bit.dts`'s 16.1%, `3-1.dts`'s 7.8%). Where the nearest of all has been the
  predictor clearly more often than the two stages (another encoder's), the blocks go by their
  nearest entries instead. The encoder's ADPCM runs closed, so the 4 samples before the subframe are
  the decoder's, not the audio's (they differ by the quantization's noise): its LPC, its PMODE's
  gain and the residual its scale factor and ABITS go by start from them (84.9% before; `s.2.dts`'s
  core 4.8% smaller, 3 MB of its middle 11%). A decoded WAV's are the audio's own, as its values
  are the decoder's. Whether a subband predicts (PMODE) follows the gain of the entry the encoder would
  choose: from 2^(6/4) over the subframe it does, 98% of the time, but only where the subband isn't
  quiet: the encoders seen never predict one whose RMS rule's scale factor is under 100 (the 7-bit
  table's 35th entry), at any gain, so such a subband's PMODE goes as a gain of none (`s.2.dts`'s
  core 1.6% smaller). A decoded WAV's values
  less the decoder's prediction are its dequantized indexes, so where the scale factor search's
  rule is taken, the first of the order's first 1024 whose prediction leaves values that fit a
  quantizer's lattice (by the bit allocation search, to a 64th of a step squared a value) is put
  first (its block, and it in its block): `open bitrate.dts`'s predictor is then the first 97.86% of the time, from 78% (96.68% by
  the first 16, which miss 2.4% of its predictors and 8.4% of `ES 6.1 16bit.dts`'s: with 1024,
  their cores 4.6% and 3.1% smaller; all 4096 find more that fit by chance). (They are tried in
  turn until one fits, the order's next taken from a heap; where none has fitted in a quarter of
  the tries, as in `96-24.dts`'s core, the first 4.) A subband that doesn't predict has its values as they are on
  a lattice, so PMODE's context there is how they fit the one the bit allocation search finds in
  them (to a 64th of a step squared a value, to 16 64ths, worse): `open bitrate.dts`'s subbands
  that fit to a 64th don't predict 465734 times in 465747, those that fit to 16 64ths do 92% of
  the time (its PMODEs 1712 bytes to 467). The search is the one ABITS would make, kept for it.
- **The high-frequency VQ vectors**: a two-stage search, a group of 32 entries, then the entry of
  the group nearest the samples (by 16 / the scale factor); coded as the group's rank and the
  entry's (92% and 93% of `s.2.dts`'s first). The DTS-HD Master Audio encoders' group isn't the
  one whose centroid is nearest, for any centroids made of the entries: nearest the mean of the
  entries each scaled to unit energy, times 0.9 their mean norm (`HF_GC` in `anal.inc`), is their
  group in 61.3-89.0% of the vectors of the five such streams here (81.2% of `s.2.dts`'s; the
  entries' plain mean less). Their silences (the samples all 0: 4.5% of `s.2.dts`'s vectors, 30.6%
  of `Master Audio 7.1.dts`'s) take group 0's entry 0, which those centroids put far down; their
  quietest vectors (`Master Audio 5.0 96khz.dts`'s first third, at the least scale factor) go to
  the groups of small entries; most other misses are near ties. So the groups go by a score
  learned from those five streams' vectors (an averaged perceptron's: `hfgroups.inc`, by
  `tools/hfgroups.py` from `./mk.sh hfdump`'s vectors; as a distance, |16 x / sc - W_g|^2 + e_g),
  and the model learns on as it codes: where its first group isn't the encoder's, the vector is
  added to that group's weights and taken from the first's (1/512 of it, the step halving as the
  steps double from 1024, to 1/16384). Each of the five left out of the learning, its first group
  is the encoder's in 90.8-97.1% of its vectors (`s.2.dts`'s 91.5%) and its core is 5.1-18.3%
  smaller (`s.2.dts`'s 7.6%, `Master Audio 5.0 96khz.dts`'s 18.3%); learned from all five, as
  here, 91.9-97.3% (`s.2.dts`'s core 8.3% smaller, `Master Audio 5.0 96khz.dts`'s 21.5%,
  `3-1.dts`'s 10.2%). FFmpeg's `xll_*` samples, in none of the learning, go from 45-62% to 86-94%
  (their cores 1.4-3.2% smaller). Where the top subbands' leak is 2^32 times their power or more
  (above), the scores don't learn from the vector: its group is the noise's as much as the
  scores' (`s.2.dts`'s core 0.3% smaller, the other streams the same). A score fitted to
  `s.2.dts`'s own vectors makes about 91% of
  them, no more: the rest isn't a function of the samples as the analysis has them. Where the
  encoder's search and the model's differ, it is nearly always a near tie, so how far the first
  beats the second is a context, by doublings from 1/256 of the first's distance (by the
  centroids: 40% of `s.2.dts`'s first 3 MB's groups within 1/256 were the first, 54% within 1/64,
  83% from 1/32 to 1/16, 99% from 1/8; from 1/16 before, its core 1.4% smaller). A decoded WAV's
  values are the vector's,
  scaled, whatever the encoder searched, so where the entry nearest of all lies within 1/64 of the
  values' energy, the groups go by their nearest entries instead (each at 16 times the nearest of
  all at most): 99.98% of `open bitrate.dts`'s groups first, where the centroids' order makes 94%.
  A lossless WAV's subbands keep the scores' order.
- **The LFE**: the WAV's LFE channel decimated by a filter near the decoder's 512-tap interpolation
  prototype, then quantized by the scale factor nearest its RMS. dtsdpack decimates by the
  prototype and corrects it with a 13-tap filter at the LFE's rate that a least squares fit learns
  as the stream goes (forgetting, ridged; solved by LDL' in a small deterministic soft float). Each
  value counts by 1 over its step squared (its error in steps is what the coder pays for), and the
  fit forgets 1/32 of the past each subframe (1/128 before, unweighted: `s.2.dts`'s first 3 MB
  2.7% smaller, FFmpeg's `master_audio_7.1_24bit.dts` 1.8%, `s.2.dts` 0.1%, the XXCH samples 0.3%
  bigger). A
  decoded WAV's LFE is the decoder's interpolation of the LFE values, 8 of them in each of its
  samples, so a second fit has the last 7 values too, with which it sees the next alone; the one
  whose predictions have been the values more often lately is taken (`ES 6.1 16bit.dts` 10%
  smaller; some lossless WAVs' streams gain too, `Master Audio 7.1.dts` 3.5%). The LFE
  scale factor comes after the samples in the bitstream; the model codes it with the first of
  them (the subframe's values as predicted, for the ones not yet coded), and the samples against
  the predictions at its step. That step is the 7-bit table's entry times 0.56 (the quantizer's
  0.035, times 16) exactly, as the encoder quantizes by it; FFmpeg's decoder rounds it to an
  integer, so a decoded WAV's LFE (where the scale factor search shows the WAV decoded) has its
  values times the rounded one. A quiet LFE's steps come to one unit or a few, which rounding
  puts up to 79% off (a lossless WAV's were rounded too before: FFmpeg's
  `master_audio_7.1_24bit.dts`'s LFE took 2248 bytes, now 262, its core 30% smaller; `s.2.dts`'s
  core 2.4%, `Master Audio 7.1.dts`'s 1.0%).
- **The transients**: a decoded WAV's subband with a transient (TMODE) doesn't fit one scale
  factor's lattice. So, its ABITS known, where the bit allocation search's fit is within a 64th of
  a step squared a value, it has none; else the subsubframe from which two scale factors' lattices
  (each the best of 3 below its part's RMS rule's to 5 above) fit 4 times better than one, is its
  TMODE's prediction, the ratio a context: `ES 6.1 16bit.dts` 9.9% smaller. A lossless WAV's
  encoders make a transient only where a subsubframe's energy is 8 times the one before's at least
  (not once below, in `s.2.dts`'s 3.5 million), never in a subband that predicts and once in one
  from 20 up, and the likelier the larger the rise and the ABITS (1% at ABITS 1, 81% from 13):
  those make a context, and the largest rise's subsubframe the first guess (`s.2.dts`'s TMODEs were
  8.8 KB, 7.1 of them its 9354 transients; its core 2.9% smaller, the 7-frame samples' 4-7%). And
  a lossless WAV's ABITS, which come before its TMODEs, say whether its encoder took a transient's
  mean square from the rise on (below): where the guesses at the two differ, which one the ABITS
  is makes the class (`s.2.dts`'s TMODEs 2.1 KB to 1.5 KB, 1.4 KB with sid tuned for it).
- **The bit allocations**: an encoder's ABITS rise with a subband's energy, at thresholds that its
  rate control moves from frame to frame. The ABITS a subband had the last time it had its energy
  (as the scale factor index its RMS makes; an ADPCM subband's residual's), in any channel, is its
  ABITS 85% of the time in `s.2.dts` (its last one 63%). But the thresholds fall inside the scale
  factors' buckets (where the ABITS seen at the energy above differs, the ABITS is one more than
  that 9% of the time in the bucket's lowest quarter and 43% in its highest), and a frame's misses
  in a channel go the same way 82% of the time: the frame has a level. So each subband (and whether
  it predicts) has a staircase: for each ABITS, the fine energies it was last seen at (in 128ths of
  a doubling of the values' mean square), less the level then, as intervals that rise with the
  energy, the newest winning; its guess at an energy is the ABITS of the interval the energy lies
  in, or of the nearer of the two either side. The level is the rate control's: at the subframe's
  first ABITS (its PMODEs and predictors all known), the one at which the staircases' ABITS for all
  its subbands would cost what the last subframes' did (a sample's log2 of its quantizer's levels;
  an average, each subframe's weight half the next one's), then, as the subframe's ABITS come, the
  shift from it (to 4 doublings either side) at which the staircases would have said the most of
  them, less a 16th of one for each 128th of a doubling it lies from the centre (the rate control's
  centre is worth more than a few ABITS' say: `s.2.dts`'s core 0.4% smaller). But the encoders seen
  fill their frames to within 16 bits (each of `s.2.dts`'s frames ends its subframe 20 to 35 bits
  before its 16088), and the bits a subband's samples take at an ABITS are the audio's: its values
  quantized at the RMS rule's scale factor, by the code the frame's header gives the ABITS, take the
  stream's bits exactly (in each of `s.2.dts`'s 24855 frames). So once a subframe has been seen, the
  core's centre is where the staircases' ABITS would take the bits the frame has for its samples:
  its bits from the subframe's first ABITS on, less what the samples of the last subframes left of
  theirs (the side information after the ABITS, the bits short of the 16: an average, each
  subframe's weight half the next one's) (`s.2.dts`'s core 3.1% smaller, the other lossless WAVs'
  0.4-2.8%; the nearer of the levels either side, 0.4% more; the rate control as it was stays for
  the subframes before, and for XCH's). An ADPCM subband's staircase is the subband's, as the
  other's (its residual's energy meets the same thresholds: 0.8% smaller), and once the subframe's
  ABITS are all known, the staircases have each of them at its last level, not the level it came at
  (1.7% smaller). Its guess is the first guess; the second, the ABITS across the edge of its
  energies the energy lies nearer (a miss's 98% of the time; the last ABITS, the second guess
  before, 39%); then the difference. How near the energy lies to that edge, and on which side, is a
  context, and so are the ABITS seen at the energy and either side, the energy, its change, the
  neighbours' and the loudest subband's. The staircases guess 95.5% of `s.2.dts`'s ABITS (the rest
  are silent subbands', whose ABITS an encoder fills a frame's bits with by a pattern of its own:
  there the ABITS seen at the energy is the first guess) and 98.4% of those right: its bit
  allocations went from 193 KB to 53 KB, its core from 386893 bytes to 249228 (246945 with abt tuned
  again, below), and the other lossless WAVs' streams 12-45%; the second guess took them to 46 KB
  (its core 3.1% smaller), and the energies in 128ths of a doubling instead of 32nds, with the
  margins to the edge in 8 buckets either side instead of 6, to 41 KB (its core 1.9% smaller, the
  other lossless WAVs' 0.5-2.6%), and the frame's bits and the rest above to 29 KB. Without the rate
  control, each subframe's level starting from the last one's, it was 278950. (The level is the
  frame's, not each channel's: the shifts fitted to each channel alone correlate at 0.99 between
  channels.) And since the encoders fill their frames, the bits left say which way a guess near an
  edge of its energies goes: as a subframe's ABITS come, what its samples may still take (the
  frame's bits from the first ABITS on, less the side information after them, as the centre has it),
  less what the ABITS so far and the staircases' guesses for the rest would take, toward the edge in
  steps of the subband's bits across it, is a context (bg). In `s.2.dts`'s first 3 MB, a guess right
  by its lower edge misses 66% of the time where the subframe's guesses would take a step or more
  too many, 14% where they would leave one or more (34% in all; by its upper edge, 79% and 16% of
  42%); at the subframe's last ABITS what is left is within 7 bits (the steps there 12-28). Its bit
  allocations went from 28.8 KB to 26.5 KB, its core 1.6% smaller (with abt tuned for it, below; the
  other lossless WAVs' 0.3-1.6%; a decoded WAV's ABITS go without it, which made theirs 0.1-0.9%
  bigger). And the lossless WAVs' encoders allocate by one table. Fitted afterwards, with a level
  for each subframe, a staircase for each subband (its edges where the fewest ABITS lie on their
  wrong side) misses 0.6% of `s.2.dts`'s ABITS, where the intervals as they come miss 1.6%; and
  those staircases are one, shifted. A subband's ABITS is a where its fine energy, less the level
  and its band's offset, is 0, 3, 5, 6, 8, 10, 12, 13 or 17 steps of a quarter of a doubling of its
  RMS (1.5 dB) or more for ABITS 2 to 10, 4 steps more for each ABITS above (its quantizer's levels
  doubled), and the next ABITS's less (else 1); the bands' offsets are the stream's: bands 5 to 19
  the same, 20 to 27 2 dB more each, and the bottom five 12, 10, 8, 5 and 3 steps lower in two of
  the five lossless WAVs (`s.2.dts` and `master audio 7.1 24bit.dts`), as the others in the rest. A
  subband with a transient goes by the mean square of its values from the transient on (where its
  second scale factor is): where the rise is 4 doublings or more, nearly always (`s.2.dts`'s 4184
  such subbands: by the subframe's mean square, the table right in 43% of them, by theirs in 94%),
  from 3 to 4 the more often the louder those values (where the rise is 3.9 doublings or more, those
  values' ABITS by the table 9 or less, an eighth of a doubling less for each ABITS above, to 3: 94%
  of the 15860 such right, 84% by the subframe's mean square alone). So each band's offset is the
  one most of its last ABITS say at their subframes' levels (each one's 1023/1024 of the next's; its
  scores start again where two in a row lie past them, as after the silence `Master Audio 7.1.dts`
  starts with), and the core's level is where the table's ABITS would take the frame's bits (as the
  staircases' centre is), plus how far the last subframes' levels lay from that (an average, each
  subframe's weight 31/32 of the next one's), then the shift (within 64 128ths) at which the table
  makes the most of the subframe's ABITS so far, less a 16th of one for each 128th, as the
  staircases'. The table's guess is the first one where it has missed less than the staircases' (of
  the last 1024 or so where both had one), and what the other one says is a context (ag); so is a
  transient's guess at the mean square not taken, the second guess where it differs. The table says
  91.27% of `s.2.dts`'s ABITS, right in 99.29% of them: its bit allocations went from 26.5 KB to
  15.9 KB by the table, to 10.5 KB by the transients' mean squares (10.0 KB with abt tuned for them,
  below), and its core 12.4% smaller in all; the other lossless WAVs' 13-17%. A decoded WAV's values
  are the decoder's, on the lattice of the step its ABITS and scale factor make, so the bit
  allocation search tries each ABITS from 4 below the first guess to 4 above (3 before: `open
  bitrate.dts`'s core 2.8% smaller, FATE's `dts_es.dts`'s 0.5%, `ES 6.1 16bit.dts`'s 0.3%; 5 either
  side finds more that fit by chance; PMODE's fit is the same search's), with each scale factor from
  3 below the RMS rule's to 5 above, and takes the pair whose lattice the values lie nearest, in its
  steps (the indexes nearest them in the quantizer's range, dequantized as the decoder does: a finer
  lattice fits as nearly, but in more of its steps, and a coarser one misses the values between its
  points), where an index it makes isn't 0. Its ABITS is then the first guess, and its margin over
  the next pair, and whether it is the first guess without it, a context: `open bitrate.dts`'s ABITS
  were that first guess's 81% of the time (in its first 400 frames), and are the search's 98.25%
  (`ES 6.1 16bit.dts`'s 97.90%; `96-24.dts`'s core's 83.02%, its values having X96's residuals in
  them). What the staircase or the table guesses is a context of the search's guess: where they
  agree, the search is right 99.99% of the time (`ES 6.1 16bit.dts`'s), where not 80%, and its
  misses are mostly what the staircase says, or the last ABITS where it says that (`open
  bitrate.dts`'s core 1.1% smaller, `dts_es.dts`'s 0.6%; the staircase's as the second guess, in the
  last ABITS's place, makes `dts_es.dts`'s 5% smaller but `ES 6.1 16bit.dts`'s 2.6% bigger).
  The pairs go by their distance at the two largest values, and each one's whole distance is
  summed, the largest values first, only while it could still come under the second best's: the same
  pair as trying them all, at a fraction of the cost.

A WAV at twice the core's rate (96 kHz for a 48 kHz core) is analysed by the 64-band filterbank
instead: the same transform of 64 bands, by FFmpeg's 1024-tap X96 prototype, each block 64 of the
WAV's samples. Its lower 32 bands are the core's subband samples, as a DTS 96/24 stream's and
DTS-HD MA 96 kHz streams' cores are (99.3-99.8% of the indexes; the spec's encoder decimates the
audio first, but these encoders didn't). The DTS-HD MA encoders' 64-band analysis is 1 - 1.4e-4
times this one, though: at FFmpeg's prototype's gain `Master Audio 5.0 96khz.dts`'s bottom four
subbands' values round to the encoder's index 98.34% of the time, and at that one's 99.65% (99.73%
by the 32-band analysis in `s.2.dts`'s first 3 MB, whose gain is the encoder's to 5e-6), the fine steps' indexes
the most: a lossless WAV without X96 is analysed so (its core 38.9% smaller, the 96 kHz `xll`
sample's 1.8%; a decoded WAV's analysis keeps the decoder's gain). X96's subbands are what TS
102 114's encoder makes of it:
the upper 32 bands, and from X96's first subband up the lower ones less the core's decoded
samples (the residuals). On them, X96's elements as the core's, with histories of their own:
99.94% of `96-24.dts`'s indexes (99.97% before an index far past the WAV's end went without the audio, below). Its decoded WAV's joint subbands are their source's times the
joint scale factor, so where the source has a level the scale factor is the projection of the one
on the other (100% of those); a subband without bits is the decoder's noise, uniform to half the
scale factor, so its scale factor is 2.1 times the peak (94%). Each kind of subband's rule, the
RMS rule and the scale factor search (for X96's subbands with bits or VQ vectors: their vectors
16 samples) are counted as the stream goes, and the one right more often is the prediction: a
lossless WAV makes the RMS rule the better. The core's scale factors get the same choice, among the
RMS rule, the search, and the first entry above 0.99 times the RMS (`96-24.dts`'s core's, 67% where
the RMS rule's is 49%; the search's, 89%). The searches cost: each is tried in its first 2048
values, then wherever its rule is the one taken (the bit allocation search's, while it is right at
least three quarters as often as the first guess without it), and otherwise in one value in 64, so
a lossless WAV's stream loses no time to them.

c finds how the core lines up with the WAV before it starts (`call.inc`'s `find_alignment()`, in
doubles; the header keeps the result). The offset: from the core's first 4 MB, the 64 blocks with
the most energy, decoded to their plain subband samples, against the WAV's analysis for each of
its channels at the decoders' usual delays (0, 256, 512 and 1024 samples, times 2 for the 64-band
analysis): one that correlates with the core's channel 0 at 0.999 or more is the offset; one at
0.99 or more is where the best of the offsets within 32 of it is (the 3 best by the 16 blocks with
the most energy in channel 0, then by all of them), where that lies inside them. Else at every
offset of -1024..1536 (times 2), by those 16 blocks, then the 64 best of those by all the blocks:
the best for channel 0 gives it. (A WAV at neither the core's rate nor twice it isn't searched:
nothing lines up.) Then each core channel, XCH's and XXCH's too (`find_mixes()`): a core may be a downmix of the WAV, as a DTS-HD MA 7.1 stream's is (its
lossless extension's primary channel set is the 5.1 the core was coded from, each surround the side
and the rear surround, every channel scaled: coefficients of DTS's downmix table, FFmpeg's
`ff_dca_dmixtable`, Q15). So over
up to 256 of the frames read first, a weighted least squares fit (each decoded subband sample by 1 /
its uncertainty squared: its quantization's, step^2 / 12, and the WAV's own, half a unit, 128 for a
16-bit WAV: the finer, the surer) of the core channel against every WAV channel and every pair of
them gives its best single channel and its best pair, each with its gains as fitted and as the
downmix table's entries either side (signed); with the channel the alignment found as it is and
by the table's gains of an unweighted fit, the candidates. Of those, the one that makes the most of
the channel's indexes exactly is its binding (a simpler one unless another makes 0.5% more; not one
whose gains are all under 1/64), if it makes a fifth of them. (A clipped decode's distortion swamps
a channel's quiet subbands at fine steps: their fit's gains came out at 0.00007 for channels that
are their WAV channels as they are, and as most of those channels' indexes are 0, such a gain made
more of them exactly than the right one: FATE's 5.1 and 6.1 samples' cores came out 36% and 18%
bigger, the XXCH ones' 19% and 14%.) (Against a decoded WAV, two kinds of samples break a fit by steps alone: a
silent subband the encoder gave a fine step, as `open bitrate.dts`'s has ABITS 24 at the smallest
scale factor, all its indexes 0, where the WAV has its rounding; and silence beside a sudden loud
onset, where the decoder's filterbank, FILTS 0's not quite perfect reconstruction, leaves a few
percent of the onset in the WAV. Their fine steps made them outweigh everything else, and the
fit's gains came out at 0.017 for a channel that is its WAV channel as it is.) The encoder mixed
in the PCM, before its analysis: each sample's sum of the scaled channels rounded to the nearest
integer (not each product rounded, nor the sum truncated). So a lossless WAV's mixed or scaled
core channel, at the core's rate, is analysed as that rounded PCM, not as its channels' analyses
mixed (`Master Audio 7.1.dts` 14.7% smaller: its indexes the audio's 99.64% of the time, from
99.51%; the 7-frame 7.1 samples 2%; the XXCH samples' decodes, a decoder's mix, until the scale
factor search shows them decoded, 0.4-0.7% bigger). The LFE gets the remaining WAV channel whose decimation correlates best with
the first 1024 LFE samples that aren't 0 (`Master Audio 7.1.dts`'s first 1024 are all 0). `-s` shows the binding:

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
- **vqr**: the ranks of the predictors (their blocks', then theirs in the block) and of the HF VQ
  vectors (their groups', then theirs in the group).
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
core, which wasn't in them, by 4.7% (415945 to 396547 bytes). abt was tuned again for its contexts
added since (three passes, on those and two decoded streams: FATE's `dts_es.dts` and 400 frames of
`open bitrate.dts`), 0.9% off its output on them, and as much on streams that weren't; and again for
the staircases (two passes, on those five and `Master Audio 5.0 96khz.dts`, `Master Audio 7.1.dts`
and `3-1.dts`), 7.1% off its output on them, and 0.9% off the whole of `s.2.dts`'s core (then
for the second guess, two passes more, 0.8% off its output); and again after the frame's bits
set the level (three passes, on the lossless WAVs' six and four decoded streams: `open
bitrate.dts`, `ES 6.1 16bit.dts`, FATE's `dts_es.dts` and `96-24.dts`), 4.9% off its output on
them: the decoded streams 0.4-3.4% smaller, the lossless WAVs' within 0.2% (tuned on the lossless
six alone, it took 1.6% off their output but made the decoded ones 0.3-4.5% bigger). Tuning
the other families again on the five's dumps took 0.2% off theirs but made the streams outside
them bigger (`Master Audio 7.1.dts` by 1.9%), so they stayed as they were; so did sid and vqr
after the transients' and the margins' contexts (on the eight, and `96-24.dts`): 5.3% and 0.7%
off their output on them, but `96-24.dts`'s core 1.4% bigger (sid; it wasn't in that set) and the
whole of `s.2.dts`'s 0.2% (vqr), the short streams' faster adaptation not the long one's. After the
HF VQ groups' learned scores, vqr was tuned again (a pass, on the whole of `s.2.dts` and the ten
streams above with a WAV each, `ES 6.1 16bit.dts` and FATE's samples among them, each of the ten's
dumps three times, so that the long stream weighs as much as the short ones): 1.3% off its output
on them, every stream at hand smaller (`s.2.dts`'s core 0.7%; tuned on the whole of `s.2.dts` with
the ten once each, it took 0.75% off that but made `open bitrate.dts` 0.7% bigger, and with
`s.2.dts`'s two 3 MB pieces for it, 0.1%, the short streams 0.1-0.6% smaller). So were scl and abt
after the 64-band analysis's gain (a pass each, on the same dumps): 4.2% and 0.9% off their output
on them, the whole of `s.2.dts`'s core 0.7% smaller and `96-24.dts`'s 0.9%, `ES 6.1 16bit.dts`'s
0.8%, but the short streams and `open bitrate.dts` up to 0.7% bigger (`s.2.dts`'s two 3 MB pieces
0.2% and 0.3%, `open bitrate.dts` 0.7%, the 192 kHz `xll` sample 0.6%), its parameters the long
streams'. Then hit and sid, after the top subband's leak noise (a pass each, on new dumps of the
same streams): 2.1% and 2.2% off their output on them, `96-24.dts`'s core 0.8% smaller,
`open bitrate.dts`'s 1.2%, `ES 6.1 16bit.dts`'s 0.9%, `s.2.dts`'s 0.1%, the rest within 10 bytes
(the 192 kHz `xll` sample's 0.3% bigger). Then raw and hdr (a pass each, on those dumps): 14.5% and
12.0% off their output on them, `open bitrate.dts`'s core 4.4% smaller, `ES 6.1 16bit.dts`'s
2.0%, `96-24.dts`'s 0.4%, `Master Audio 7.1.dts`'s 0.5%, the `xll` samples' up to 2.2%; a few
streams 1-4 bytes bigger. With the predictors' two ranks, vqr again (two passes, on new dumps of
the same set): 0.26% off its output on them, `s.2.dts`'s core 0.07% smaller, the others' up to
0.45%. With bg, abt again (a pass, on new dumps), and sid, scl and hit a little (a part of a pass,
on the dumps before): 1.43% off abt's output on them. Then sid, scl, hit and vqr (a pass, on those
dumps; hit's nearly): 0.01-0.16% off their output on them; and with the allocation table and the
transients' mean squares, abt and sid (two passes, on new dumps): 2.3% and 0.1% off theirs. After
the near misses, the WAV's end, the staircase's agreement and the channel's misses, smp, abt, sid
and hit (on new dumps; two passes each but hit's one): 1.0%, 0.3%, 0.01% and 0.4% off their output
on them, FATE's XXCH samples 0.5-1.4% smaller and its `dcadec-suite` ones 0.4-1.0% (one 2 bytes
bigger), the others within 0.4% (`s.2.dts`'s core 9 bytes bigger).
`./mk.sh check 9 stream wav ...`
confirms that the tuning build, the shipping build and the build without AVX2 write the same
files.

### The format

"dtsdpack", the format's version (37), the level, the WAV's binding (the offset; each core channel's
WAV channel, the second of its mix and their gains, Q23; the LFE's channel; the WAV's samples to
the core's, 1, 2 or 4 (decimated by 2, then 2); the WAV's format, size and the CRC-32 of its first
64 KB), then the
range coded chunks (a frame as records, bytes between frames, a frame kept as bytes), the end, and
the CRC-32 of the core stream, which `d` checks.

## What it doesn't do

- A core channel is bound to at most two of the WAV's channels: one that mixes more (a stereo core
  downmixed from 5.1) is bound to its best two, or isn't. A 192 kHz stream's WAV, decimated by 2
  by a lowpass fitted to FFmpeg's two samples, lines up with its core only nearly (above); a
  176.4 kHz one's is taken the same way, untried.
- The LFE is predicted at 64x only (LFF 2); a 128x LFE (LFF 1) is coded by the model alone.
- XXCH has been tried only on the streams made from FATE's (above), and only one channel set is
  parsed, as in FFmpeg; X96 and XXCH in the extension substream aren't in the core (dtssplit keeps
  the extension substream).
- A decoded WAV can't say what X96's joint scale factors are where their source decodes to
  silence: those are coded against a quarter of the sum of their last values and the subband
  below's (each correlates 0.4 with them; against their last values alone, as before, 7% more
  bytes), most of `96-24.dts`'s X96 bytes still.
- The scale factor search can't see through the decoder's noise: X96's subbands without bits keep
  the peak rule (94%), and a clipped decode (as the full decodes of FATE's XXCH samples are) has
  values that aren't the decoder's reconstruction, where it isn't sure.
- The bit allocation search fits one scale factor to the subframe: a subband with a transient (two
  of them; its TMODE comes after its ABITS) doesn't fit, and a DTS 96/24 stream's core subbands
  hold X96's residuals too (83% of `96-24.dts`'s).
- A stream cut from a longer one costs most at its ends. Its last frame holds the audio that went
  on past its WAV: 1.0-1.5 KB of each here, about what a frame costs without the audio (a decoded
  WAV's could be solved for, the decoder's synthesis being known, but the system is ill-conditioned
  for the frame's later blocks). And in its first frames the WAV isn't the encoder's input until
  each channel's decoder state catches up: noise of about a step, white, in all its subbands
  (`3-1.dts`'s first six frames 1.6 KB). A whole program has neither.
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
generated from FFmpeg's `libavcodec/dcadata.c` by `tools/mktables.py`, `IDX/l9/*.idx`'s first
declarations by `tools/mkidx.py`, and `hfgroups.inc` (the HF VQ groups' scores) by
`tools/hfgroups.py` (numpy) from the vectors `./mk.sh hfdump DIR stream wav ...` writes: those of
`s.2.dts`, `Master Audio 5.0 96khz.dts`, `Master Audio 7.1.dts`, `master_audio_7.1_24bit.dts` and
`3-1.dts`, in that order, make it as it is; and `pvqblocks.inc` (the ADPCM predictor search's
codewords) by `tools/pvqblocks.py` (numpy) from the predictors `./mk.sh pvqdump DIR stream wav ...`
writes: those of `s.2.dts`, `master_audio_7.1_24bit.dts`, `Master Audio 5.0 96khz.dts`, `Master
Audio 7.1.dts` and `3-1.dts`, in that order.

## License

The tables in `tables.inc` (the QMF and LFE prototypes, the quantizer and scale factor tables, the
ADPCM and HF VQ code books) are FFmpeg's, so dtsdpack is licensed under the GNU Lesser General
Public License, version 2.1 or later. The framework (`fam.inc`, `cm*.inc`, `rc.inc`, `Lib3/`, `IDX/`)
is ac3pack's.
