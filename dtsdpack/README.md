# dtsdpack

dtsdpack is a lossless compressor for the core of a DTS stream, given the lossless audio it was
coded from. A DTS-HD Master Audio stream carries a lossy DTS core and a lossless extension (XLL);
decoded, the extension gives back the encoder's input exactly. dtsdpack codes the core against that
audio: nearly every quantization index, scale factor, ADPCM predictor, high-frequency vector, LFE
sample and bit allocation of the core is what the DTS encoder makes of that audio, so what is left
to code is where the encoder chose otherwise. Decompression, with the same WAV, restores the core
stream byte for byte.

The core of `s.2.dts` (4 minutes 25 seconds of 5.1 at 1509 kbps; 50008260 bytes) compresses to
76356 bytes (0.15%); without its audio, dtsdpack's model gets 62.44% and `xz -9e` 88.21%.

The core substream's extensions are coded too: XCH (6.1's back centre) and XXCH (7.1's other
channels) as more channels, X96 (DTS 96/24's 24-48 kHz band and the core's residuals) as subbands
of its own. A 96 kHz stream's core is the lower half of a 64-band analysis of its audio, so a WAV at
twice the core's rate is analysed that way: DTS-HD MA 96 kHz cores go to 0.7-5.8%, where nothing
lined up before.

The core comes from [dtssplit](../dtssplit) (`prefix.dtsc`) or from any DTS stream, its bits in
16-bit words big-endian (the core's own), little-endian, or 14 bits a word as a DTS CD's are (DTS
in a WAV file: below); the WAV is the stream decoded, for instance by FFmpeg:

```
ffmpeg -i in.dts -c:a pcm_s24le in_full.wav
dtssplit c in.dts in
dtsdpack c in.dtsc in_full.wav in.dtsd
```

## Usage

```
dtsdpack c [options] input.dtsc input.wav [heights.wav] output.dtsd
dtsdpack d [options] input.dtsd input.wav [heights.wav] output.dtsc
```

- `c` compresses the core stream against the WAV, `d` decompresses it with the same WAV.
- `heights.wav` (since format 155) is a DTS:X stream's heights, dtssplit's `prefix.x.wav` (Lh, Rh,
  Lhr and Rhr), for a WAV of its 7.1 bed with them out: as the WAV is read, each is added into its
  L, R, Lsr or Rsr (FL, FR, BL, BR) at -3 dB, times 23170/32768 rounded, which makes the stream's
  7.1 decode again (below). A WAV that has them in, as FFmpeg's 7.1 decode has, takes none. Without
  one, a 7.1.4 WAV (12 channels, FL, FR, FC, LFE, BL, BR, SL, SR, TFL, TFR, TBL, TBR: FFmpeg's
  master branch's decode of a DTS:X stream, the bed less the heights) has its own last four added
  back the same way. `d` takes the same one.
- `-v` (with `c`) decompresses the output afterwards and compares it with the input.
- `-s` (with `c`) shows the input and output size of each kind of syntax element, and how much of
  the core the audio predicted.
- `-q` turns off the statistics and the progress line.
- `-p` shows the progress line even where stderr is not a terminal.
- `-9` (with `c`) is the model's level, and `-8` the same model with eight families' parameters
  tuned on a DTS:X encoder's stream (the Cymatics one, below). Without either, `c` takes `-8`
  where the binding mixes two WAV channels, one with a gain before the mix (that encoder's
  downmix of the back surrounds), else `-9`; `d` takes the level from the file.

The core stream's input and output can be `-` for stdin and stdout, except with `-v`, which reads
the files again; the WAVs are read once from their start, so they can be pipes too (one input from
stdin at most). But `c` lines a stream that starts quiet for longer than 21.8 s (at 48 kHz; 10.9 s
for a 96 or 192 kHz WAV) up with its WAV only where it can read on and back (below): files, not
pipes. The WAV can be any PCM WAV of 16, 24 or 32 bits (RIFF or RF64): FFmpeg's 24-bit decode, or
its 32-bit one (a 24-bit decode in 32 bits).

A WAV that doesn't line up with the core, or isn't a WAV, isn't an error: the core is then coded by
the model alone (`s.2.dts`'s as 67.9% of its size), and the summary line says why. `d` checks that
its WAV is the one `c` had: its format, its size and the CRC of its first 64 KB are in the header
(and the heights WAV's, where one was added).

```
$ dtsdpack c -s s2.dtsc s2_full.wav s2.dtsd
s2.dtsc: 50008260 -> 76356 bytes (0.15%, -9); 24855 frames, 24855 packed; 0 bytes between frames; 5 channels and the LFE against the WAV (offset 512, correlation 1.0000)
  bytes                             input     packed
  bytes kept as bytes                   0          3    0.00%
  headers, container              1059444         32    0.00%
  side information                2521040      36249    1.44%
  bit allocations                 2174812       4467    0.21%
  scale factors                   3512740      15273    0.43%
  subband samples                40631706      20299    0.05%
  bits after the subframes         108518          0    0.00%
  total                          50008260      76323    0.15%
  the audio's indexes: 99.92% of the samples; the audio's scale factors: 99.09%
  the bit allocations' staircases: 95.53% of the ABITS, right in 99.57% of them
  the encoders' allocation table: 94.32% of the ABITS, right in 99.57% of them
  the encoder's searches on the audio: 99.76% of the ADPCM predictors; HF VQ: 92.36% of the groups, 93.04% of the vectors in them
  the binding: core channel 0 = WAV 2; core channel 1 = WAV 0; core channel 2 = WAV 1; core channel 3 = WAV 4; core channel 4 = WAV 5; LFE = WAV 3
```

(`-s`'s byte counts are the coder's estimate of its decisions' costs: for `s.2.dts`, 33 bytes short
of what it writes, 211 before format 103's coder took a decision's share of the range exactly (in
`rc.inc`); before format 102 they took a probability by its top 12 bits, which put its subband
samples at 22802 bytes.) Where stderr is a terminal, or with `-p`, a call shows what the audio is
and a progress line, as [ac3pack](../ac3pack) does.

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
| `s.2.dts` (5.1, 1509 kbps, 4:25) | 50008260 | 76356 (0.15%) | 62.34% | 88.21% |
| its first 3 MB (6.8 s) | 1275608 | 3470 (0.27%) | | |
| 3 MB from its middle (5.7 s) | 1068372 | 4615 (0.43%) | | |
| FFmpeg's `master_audio_7.1_24bit.dts` (12.5 s) | 2358064 | 2585 (0.11%) | | |

Compressing the whole of `s.2.dts`'s core takes 78 seconds (0.6 MB of core a second; 71 in format
95, 38 before the HF VQ's sampling of format 93, below) and 42 MB of memory, decompressing it 80
seconds and 34 MB, on one thread of a 2.1 GHz Xeon (a virtual machine; each figure here the median
of runs one at a time; from one set of runs to the next, 10% either way, and the host's own speed
wanders by a third; on an earlier 2.3 GHz host, before format 84, it took 22 seconds each way). The
rows were all in the tuning sets (below; the whole of `s.2.dts` since the HF VQ groups' scores were
learned), and `s.2.dts` in what the HF VQ groups' scores and the ADPCM predictors' codewords were
learned from. A 96 kHz stream's 64-band analysis and X96's searches cost more: `96-24.dts`'s core
(1.9 MB) compresses in 4.6 seconds (413 KB of core a second; 4.2 before format 104's calibrations)
and decompresses in 4.3 (435 KB). The decoded WAVs' searches cost too; every other stream below goes
at 550 KB of core a second or more both ways (FFmpeg's encodes, where its encoder is made again,
less: below; `padded.dts`, whose searches run on every value since format 149, 439 KB a second
compressing, 445 decompressing). The short samples below (7 frames, the 96 kHz and XBR ones 14; 7
and 14 KB) take 10 to 41 ms each way at best, 322 to 806 KB a second (FATE's XCH and X96 ones the
slowest, compressing; the best of 25 runs: this host's speed varies by a quarter from run to run,
and by their medians those two compress at 262 and 260 KB a second, decompress at 302): some
milliseconds of a run are the same for any stream (starting, the WAV's header and start, the
alignment, the WAV's continuation past its end, the kernel mapping the 2400 pages the model
touches), and the searches run on every value of a stream's first 2048 (the bit allocation search's
first 512; since format 148 every 8th of them where it has been right 24 times fewer than the first
guess, as on FATE's samples, X96's from its 256th where no more often), which is all of theirs.

FFmpeg's small DTS-HD MA samples (`xll_*`, 7 frames each) round-trip too:

| Sample | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `xll_51_24_48_768` | 7168 | 282 (3.93%) | the WAV's 5.1 |
| `xll_71_24_48_768_0` | 7168 | 263 (3.67%) | a downmix of its 7.1: every channel at -3 dB, each surround its side and rear surround |
| `xll_71_24_48_768_1` | 7168 | 285 (3.98%) | the same, the rear surrounds' sign inverted |
| `xll_xch_61_24_48_768` | 7168 | 543 (7.58%) | its 6.1's 5.1, the back centre in the surrounds at about -3 dB; and in the XCH extension, the back centre |
| `xll_71_24_96_768` | 7168 | 239 (3.33%) | the 96 kHz WAV's 64-band analysis's lower half, a downmix as the 48 kHz one's |
| `xll_51_16_192_768_1` | 7168 | 216 (3.01%) | the 64-band analysis's lower half of the 192 kHz WAV's lower frequency band, as DTS-HD MA's XLL makes it (below) |
| `xll_51_16_192_768_0` | 7168 | 192 (2.68%) | the same (the same WAV, another core) |

Before the downmixes were found, the 7.1 and 6.1 ones went to 36-38%: only 3 of their core's
channels lined up with a WAV channel, and those at the wrong gain in the 7.1 ones. With its XCH
extension kept as bytes (about 140 of each frame's 1024), the 6.1 one went to 26.79%; before the
64-band analysis, the 96 kHz one to 42.16%. The 192 kHz ones went to 48% until their WAV was
decimated by 2, the 96 kHz signal's 64-band analysis then lining up with their cores as a 96 kHz
stream's does with its WAV's; by a lowpass (format 32, a sinc at 40 kHz, its centre where a least
squares fit put it), to 23%, their indexes the audio's 83% of the time. But DTS-HD MA's XLL codes a
192 kHz stream's audio as two frequency bands at 96 kHz, which its decoder puts together by lifting
steps (FFmpeg's `assemble_freq_bands()`, by `ff_dca_xll_band_coeff`), and the encoder's core is
the lower band's 64-band analysis's lower half. So the WAV goes back into those bands exactly
(format 67, `wav.inc`): its even samples are the upper band, its odd ones from the 15th the lower
band from 7 before its first, and the decoder's steps are undone in reverse, from the stream's
start as its decoder runs them, at the stream's own resolution (the samples' common trailing 0
bits: a 16-bit stream's decoded to 24 bits, 8; the binding keeps it; at 24 bits they make 1470
bytes of the first; read on past a quiet start, 4 MB at a time, until 2^16 samples that aren't 0
have come, to 256 MB: where the first 4 MB were silent it was 0, and at 0 the first sample comes
out 4.3 times bigger; with 300 silent frames before it, 1616 bytes then, 852 after; 629 in
format 131, 609 in format 132), the lower band saturating at 24 bits as the encoders' does. Their
indexes are then the audio's 99.5% of the time, their scale factors 99.8% (from 66%), and their
cores 78% and 80% smaller.

Streams of FFmpeg's samples site (`samples.ffmpeg.org/A-codecs/DTS/dts/`, some 5 MB pieces of
longer ones) and FATE's `dts_es.dts`, their cores against FFmpeg's decodes (lossless for DTS-HD MA,
lossy for the others: the decoder's output, which the core's indexes are mostly the re-analysis
of, and whose values the scale factor and bit allocation searches find their scale factors and bit
allocations in):

| Stream | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `96-24.dts` (DTS 96/24, 5.1, 20 s) | 1888194 | 28229 (1.50%) | the 96 kHz decode's 64-band analysis's lower half (99.79% of its indexes, 95.98% before X96's residuals were taken from its top subbands, below); its X96 extension, 12% of each frame, the upper half and the residuals (99.94%) |
| `Master Audio 5.0 96khz.dts` | 1768548 | 3464 (0.20%) | the 96 kHz WAV's 64-band analysis's lower half (99.82%) |
| `Master Audio 7.1.dts` | 1945604 | 4102 (0.21%) | a downmix of its 7.1 |
| `3-1.dts` (DTS-HD MA 3.1) | 2782596 | 3779 (0.14%) | its 3.1 |
| `ES 6.1 16bit.dts` (DTS-ES, XCH) | 5233800 | 1929 (0.04%) | 6.1 by XCH, decoded (1.39% before the bit allocation search) |
| FATE's `dts_es.dts` (DTS-ES, XCH) | 503000 | 1503 (0.30%) | the same (1.95%) |
| `open bitrate.dts` (stereo, open rate) | 5242416 | 613 (0.01%) | decoded (99.99% of the indexes, 99.96% of the scale factors, 99.82% of the bit allocations, 99.99% of the HF VQ groups, 99.82% of the ADPCM predictors; 30.85% before the binding's fit stopped trusting silent subbands at fine steps, below, 2.19% before the scale factor search, and 0.85% before the bit allocation search) |
| `padded.dts` (5.1, 1509 kbps, its frames padded to 2048 bytes, 72 s) | 13756416 | 133144 (0.97%) | decoded (99.15% of the indexes, 99.09% before format 151; 97.19% of the scale factors, 69.52% before format 149; the bit allocation search taken in 62.8% of the ABITS, right in 95.5%, before format 150 in 35.0%, right in 85.4%: below); its first 340 frames, 5% of them, under -66 dBFS and many of their steps under the 24-bit WAV's LSB, took 61% of its indexes' bytes before format 151 |

A DTS:X stream, `Nigel_Stanford_Cymatics_DTS-X.dts` (5:27; a 5.1 core at 1509 kbps beside DTS-HD
MA's XLL, whose 7.1 bed FFmpeg decodes, and the DTS:X objects' data, which
[dtssplit](../dtssplit) keeps apart), against that decode:

| Stream | Core | dtsdpack | The core is |
|---|---:|---:|---|
| `Nigel_Stanford_Cymatics_DTS-X.dts` | 61758340 | 60631 (0.10%) | the bed's 5.1 downmix, every channel at -3 dB, each surround its side and back surround, the back's samples first times 32767/32768, rounded (at level -8, below; 60720 before format 175's signs, 60907 before format 140's LFE fixed filters at its downmix's gain and -8's hit and smp retuned, 61161 before format 139's LFE hit flags by the predictions' agreement, 61287 before format 131's retunes, 61830 before format 130's -8, 61943 before format 129's retunes, 62024 before format 128's sid retune, 67327 before format 127's LFE error fits, 76800 before format 125's transients, 89909 before format 124's allocation table for its encoder, 92266 before format 123's LFE predictions by class, 179247 before format 122's gains before the mix, below; 184951 before format 117's changes for this encoder's LFE) |

Its surrounds' subband samples were 80 KB of its 180, their bottom subband 36 KB, missed 9% of the
time, the more the louder: what seemed something more in the core than in the bed (white, in time
and across the two surrounds) was the encoder's gain. It put the WAV's channels 4 and 5 (FFmpeg's
back left and right) into the core's surrounds times 32767/32768, a Q15 unity's, each sample rounded
to the nearest integer, then mixed them at the downmix table's 0.707092 with channels 6 and 7. A
sample under 2^14 comes out of that as it was, so where those channels are quiet the table's gain
alone makes the same indexes, and where they are loud it misses. Format 122's binding has each mixed
WAV channel's gain before the mix (below): the core's indexes are the audio's 99.90% of the time
(99.67% before), its subband samples 15 KB of 92; the surrounds' miss as rarely as the fronts'
(0.021-0.026% of them in frames 8000-13999; the centre's 0.039%, its excess in the subbands next to
12 kHz and in its bottom one at ABITS 20-21, hundreds of steps out: the encoder's arithmetic against
loud bass). The bit allocations were 27 KB, the staircases' first guesses taken and missing 1.8% of
them, over all the channels and subbands, the centre's in runs of subframes (neither the last ABITS
as a guess there, nor a shift of each channel's own in the subframe, helped). Yet its encoder
allocates by the DTS-HD MA encoders' table (below): of 8.2 million pairs of a subframe's channels in
a band, 102 have ABITS out of order with their energies and 102 the same ABITS further apart than
its step in the table; with a level for each subframe and an offset for each band, fitted
afterwards, it makes all but 0.10% of them. But its bottom five bands lie 18, 15, 12, 9 and 6 dB
below the rest, where `s.2.dts`'s encoder's lie 18, 15, 12, 7.5 and 4.5 dB below: the table missed
its bands 3 and 4, so the staircases' guesses were taken. Since format 124 that is a third shape:
the table's guesses taken for 95.7% of its ABITS, right in 98.75% of them, its bit allocations 16 KB
and its core 78723 bytes. And at a loud onset its level rises 8 to 23 doublings from a subframe to
the next (37 times), where the budget's level could move 8: it now reaches 32 where the bits meet
the frame's only past 8, the table right in 99.13%, the bit allocations 14 KB (the first channel of
a subframe, the centre, guessed before the subframe's ABITS have moved the level, missed most: 2.2%
of its first guesses, the surrounds' 0.7-0.9%). But the budget left out a transient's second scale
factor, so that the level lay low in subframes with many transients, and this encoder takes
transients from subband 20 up too, where none were expected: since format 125, 5.9 KB, the table
right in 99.55% (below). The HF VQ indexes are 27 KB, their misses in the top subband: 15-16% of the
centre's and the surrounds' groups and vectors there, which hold little but the encoder's FFT noise,
4.5-6.7% of the fronts'; the model's noise there is as strong as it should be (half as strong or
twice made those frames 0.4% and 1.0% bigger). Its LFE, 17 KB before format 123, 14 KB before format
127 and 8.9 KB now, is a fixed filter of the WAV's LFE, rounded up, but at its quietest: one of 64
taps at 6 kHz fitted to half of those frames makes the louder three quarters of the other half's
values to 0.37 steps RMS (81% of them exactly; with 64 ms of the past, 0.32 and 90%), the quietest
quarter, whose steps are about a twentieth of the WAV's unit, to 2 steps; the other channels' low
end explains none of it. The model's best prediction was the value 85.2% of the time over the
stream, one of its twelve 98.2%: format 123 takes the one best for the best's sign and size (below),
88.6%; its fit with 31 ms of the past, or whether its two best agree as a context, made 10-40 bytes
of difference. The fits err as a line by sign: the values are some 0.7% bigger than their
predictions, and those near 0 a sixth of a step nearer it; format 127 takes each prediction less a
fit of that (below), 93.8%.

The 7.1 bed has the DTS:X stream's heights in it, each in the channel under it at -3 dB (L, R, Lsr
and Rsr with Lh, Rh, Lhr and Rhr times 23170/32768, rounded: [dtssplit](../dtssplit) takes them out
of its WAV again), and without them it packs smaller: FLAC (`-compression_level 5`) takes FFmpeg's
7.1 decode to 185462834 bytes, the bed less the heights to 169242780 (8.7% smaller) and the heights
themselves, dtssplit's `prefix.x.wav`, to 98128482, so a store of the audio would keep the bed
without them. Against that bed the core is 27924292 bytes (45.2%), its L, R and surrounds missing
the heights; since format 155 the heights' WAV can follow the WAV (`dtsdpack c cym.dtsc bed.wav
cym.x.wav cym.dtsd`), each height added back into its channel as the WAV is read, and the core is
60730 bytes, the decode's 60720 and the heights' WAV's size and CRC. FFmpeg's releases (to 9.1)
decode such a stream's 7.1 bed, its master branch 7.1.4 (its `parse_x_heights()`): the bed less the
heights, as dtssplit's WAV has its channels, then the heights; dtsdpack adds a 7.1.4 WAV's own
heights back the same way, and the core against one made so from the bed less the heights and
`prefix.x.wav` is 60720 bytes (14653732 before). (A WAV that has the heights in already, as the
releases' 7.1 decode does, would have them twice: 25224008 bytes.) dtssplit's own `prefix.wav` can't
be the WAV, heights or not: its channels coded against the core hold their residuals (here all but
the rear surrounds, which hold the bed's audio less the heights, exactly), which only the core makes
audio again.

FATE's `dtsx_7.1.4_24bit.dts`, two frames of another DTS:X stream (7.1.4), is a cut, it seems: as
with eight frames cut from the middle of the Cymatics stream, FFmpeg decodes its first frame as the
core alone (the back surrounds silent, the core's channels at their own level), the rest as the
bed, the core's channels at -3 dB of it. Its core (4024 bytes) lined up with nothing until the
binding left out the blocks whose windows pass the WAV's end (below): 3274 bytes before, 2406
after (2432 in format 132, below); the Cymatics cut's core (16096 bytes) 12528 before, 3835 after
(3831 in format 122).

No stream at hand has XXCH in its core substream: FATE's XXCH samples (`xxch_71_24_48_2046`,
`xbr_xxch_71_24_48_3840`, `x96_xxch_71_24_96_3840`, 7 frames each) have it in their extension
substreams. Their XXCH frames moved into their core frames after the subframes (EXT_AUDIO set,
EXT_AUDIO_ID 6), the extension substream dropped, make core-substream XXCH streams that FFmpeg
decodes as 7.1; dtsdpack round-trips them against those decodes (11.73%, 10.95%, 8.92%; 15.80%,
17.58% and 14.44% before a twin channel's fields went by the earlier one's, below; the first two
21.64% and 22.63% before the binding stopped taking a gain a clipped decode's fit makes). Their
signal is a silent frame, then white noise near full scale; the decoder undoes XXCH's downmix
pre-scaling (every core channel times 1.41), so the decodes clip, and only partly line up with
any core (their own cores against their core-only decodes, which don't clip, make 18.91%).

FATE's other DTS samples (`dcadec-suite`'s; 7 frames each, the 96 kHz ones 14) round-trip too,
against FFmpeg's decodes of the whole streams. Their signal is the same noise near full scale, and
their decodes clip too (0.3-1.2% of the samples):

| Stream | Core | dtsdpack |
|---|---:|---:|
| `core_51_24_48_768_0`, `_1` (5.1) | 7168 | 1363 (19.02%), 1339 (18.68%) |
| `xch_61_24_48_768` (6.1 by XCH) | 7168 | 1394 (19.45%) |
| `x96_51_24_96_1509` (X96) | 14084 | 2617 (18.58%) |
| `x96_xch_61_24_96_3840` | 14084 | 1927 (13.68%) |
| `xbr_51_24_48_3840`, `xbr_xch_61_24_48_3840` (XBR: its residuals in the decode) | 14084 | 1575 (11.18%), 1986 (14.10%) |
| `xll_x96_51_24_96_1509` (DTS-HD MA, X96) | 14084 | 1291 (9.17%) |

Streams FFmpeg's encoder made (`ffmpeg -c:a dca -strict -2`), against FFmpeg's decodes: their
choices are FFmpeg's own (`libavcodec/dcaenc.c`). Its bit allocations go by each subband's peak
against a masking curve of its own, at the noise level its search finds for the frame's bits; since
format 135 the model makes them again from the WAV as FFmpeg's encoder computes them (`ffenc.inc`,
below), and they are the core's 97.1-98.7% of the time (the model's staircases', 80-89%). Its scale
factors are the least whose range at the subband's ABITS holds the subband's peak, 3 to 14 entries
under the RMS rule's at its fine steps, which since format 133 the scale factor search goes down to,
and since format 135 each one's first guess is the likeliest of four, that rule on the decode's peak
among them (since format 137 the likeliest at that rule's size and the model's distance from it,
below); its code books for each frame's indexes (each channel's for each ABITS: the DTS encoders'
are the same for the whole stream) those that code the values the cheapest, which since format 134
the model codes after the values, by that rule, and since format 135 its bit allocations' books too
(since format 138 the first frame's too, by a flag where they are the rule's: before, the frames
after went by the frames before, and the first frame without FFmpeg's encoder made again); and its
decodes' LFE, which their decimation makes 89-91% of, the model binds since format 134 by its
decimation corrected as a decoded WAV's, and predicts since format 135 by FFmpeg's decoder's
interpolation undone exactly, its scale factors since format 136 by the lattice the WAV's LFE fits
(below). Their bit allocations took 83.3 KB in format 134 and take 5.7 KB, their scale factors 24.4
KB, 8.4 KB in format 136 and 1.7 KB, their LFE 6.0 KB, 0.3 KB in format 135 and 70 bytes; what is
left is mostly the indexes that the decodes don't give back (the first 3 MB's 7.1 KB of 8.8, `open
bitrate.dts`'s encode's 9.8 KB of 10.9), at steps under the WAV's unit, which its rounding hides
(since format 142 their hit flags by how near the boundary they lie against that unit: below).
Making FFmpeg's encoder's choices again takes some time, less since format 137: these cores compress
at 472-1292 KB a second and decompress at 548-1352 in format 142 (format 141 at 494-1304 and
558-1385 in the same runs, the 7-frame samples above at 339 and 380 at the least), the slowest the
768 kbps one, whose frames are half the bytes for the same work (earlier, on the host as it was,
511-1331 and 590-1453, format 137 at 516-1365 and 591-1405 in the same runs, where the 7-frame
samples above go at 449 and 565 at the least; on the host as it was when those went at 484 and 591,
format 134, which doesn't make them, at 566-1320 and 597-1447, and format 136's arithmetic at
432-1156 and 506-1221):

| FFmpeg's encode | Core | dtsdpack | format 141 | format 137 | format 134 |
|---|---:|---:|---:|---:|---:|
| `s.2.dts`'s first 3 MB as decoded (5.1, 1509 kbps, 6.8 s) | 1275608 | 8840 (0.69%) | 9255 | 9386 | 26484 |
| 3 MB from its middle (5.7 s) | 1068372 | 1376 (0.13%) | 1405 | 1514 | 10051 |
| its first 3 MB at 768 kbps | 649216 | 1304 (0.20%) | 1326 | 1390 | 11263 |
| `open bitrate.dts`'s first 20 s as decoded (stereo, 768 kbps) | 1920000 | 10891 (0.57%) | 11188 | 11403 | 23362 |
| a minute of 5.1 shaped noise, its surrounds silent to 30 s (above) | 10597500 | 4588 (0.04%) | 4590 | 4667 | 65358 |

Against the WAV that was FFmpeg's encoder's input, not a decode, the model makes nearly all of an
encode again since format 138: each index as the encoder quantizes the value its transform makes
from that WAV, the LFE by its decimation, as FFmpeg 6.1's encoder does it (its history the input's
last channel's, a sample late), and the scale factors by its rule (below). What is left is the first
frames' (the switches are learned) and a few bits a frame:

| FFmpeg's encode, against its input | Core | dtsdpack | format 137 |
|---|---:|---:|---:|
| `s.2.dts`'s first 3 MB as decoded (5.1, 1509 kbps, 6.8 s) | 1275608 | 173 (0.014%) | 25838 |
| 3 MB from its middle (5.7 s) | 1068372 | 176 (0.016%) | 7495 |
| its first 3 MB at 768 kbps | 649216 | 170 (0.026%) | 11052 |
| `open bitrate.dts`'s first 20 s as decoded (stereo, 768 kbps) | 1920000 | 169 (0.009%) | 9340 |
| 10 s of that one's decode as 16 bits, encoded at FFmpeg's default (1411 kbps) | 1767192 | 136 (0.008%) | 162307 |

FFmpeg's default 16-bit decode of that last one takes 380173 bytes, 445344 in format 137: where
FFmpeg's quantization of the decode's values has been the core's index more often than the model's
prediction where the two differ, it is the prediction (`-s`: "the audio's indexes: 55.45% of the
samples", 39.37% before); its decodes of the 24-bit WAVs' encodes, 16-bit too, take 0.02% less, and
since format 142 1.8% less again (1102817 bytes to 1083123, the five above), their hit flags by how
near the boundary the decode's values lie against the WAV's LSB (below).

Its 24-bit decode took 432519 bytes in format 142, 51.6 KB more than the 16-bit one, which is the
same decode rounded: rounding takes the encode's noise away wherever it lies under half a 16-bit
step, and gives back the encoder's input there (94.6% of the samples, every one in the blocks below
-38 dBFS). So since format 143, where the WAV has more than 16 bits, FFmpeg's transform is made again
from the WAV rounded to 16 bits as well, and an index goes by its quantization of that where it has
been the core's twice as often as that of the WAV as it is, plus 64, the counts halved at
4096 as the signal goes quiet and loud (halved at 2^16, 10% bigger; counted by the subframe's level
too, 0.16% smaller); it is made no more where it has been right less than a quarter as often
(FFmpeg's 24-bit decodes of the 24-bit WAVs' encodes the same bytes and as fast): 334657 bytes, 22.6%
less, and faster to compress (1400 KB a second from 1252), there being fewer misses to code.
And since format 144, where the counts halved at 2^16 say the rounded one as a rule, the transform
not taken gives the other candidate, the alternative, where the two differ: 322932 bytes, 3.5% less
again (by the quickly halved counts alone, FFmpeg's decodes of the 24-bit WAVs' encodes up to 19
bytes bigger); 321616 since format 168's retune of the decoded WAVs' families (below).

A stream whose words are packed otherwise (below) was all bytes between frames until format 145,
kept as bytes; now its frames are unpacked and coded as the core's own are. FFmpeg's samples of DTS
in a WAV file, against their decodes by FFmpeg at 24 bits:

| Sample | Its words | Stream | dtsdpack | format 145 | Before |
|---|---|---:|---:|---:|---:|
| `dtswavsample14.wav` (5.1, 44.1 kHz, 1953 frames, the last cut) | 14 bits each, little-endian | 8000000 | 40778 (0.51%) | 65338 | 7266433 (90.83%) |
| `Fire__VooDoo_Studio_30_sec.wav` (a DTS CD's, 5.1, 44.1 kHz) | the same | 5156908 | 7431 (0.14%) | 16791 | 4680858 (90.77%) |
| `dtswavsample16.wav` (5.1, 48 kHz, each frame padded to 2048 bytes) | 16 bits, little-endian | 47966252 | 37210 (0.08%) | 188826 | 44772325 (93.34%) |

And a stream packed any of the four ways compresses to the same bytes as in the core's own words
(six of the sets' streams, packed so by a script: s.2's head, `96-24.dts`, `ES 6.1 16bit.dts`,
open bitrate's FFmpeg encode, FATE's XCH sample and a cut one).

## How it works

### The stream

The core stream is cut into core frames and the bytes between them, as dtssplit and
[dts2tsv](../dts2tsv) cut it. Each frame is parsed into its record (`syntax.inc`: dts2tsv's walk of
the core, down to every subband sample's quantization index), written back as bits right away and
compared with the frame: if they match, the record is coded; if not, or the frame doesn't parse,
the frame is kept as bytes, and so is everything between frames, so any input round-trips. The same
walk that reads a frame's bits and writes them is, at record level, what codes and decodes the
record: the model (`model.inc`) is its IO, so the two directions can't drift apart.

A stream may have its 16-bit words little-endian (DTS in a WAV at 48 kHz, its bytes swapped), or 14
bits of it in each word, the word's top 2 bits copies of its bit 13 so that played as PCM the
stream is no louder than 14 bits (a DTS CD's, DTS in a WAV at 44.1 kHz: little-endian there,
big-endian too), as FFmpeg reads them all (`pack.inc`). The stream's packing is its first frame's;
a frame packed so is unpacked into the core's own bytes, which are coded, and packed back as it
was. Where packing those bytes again doesn't make the frame's (a word's top bits not those copies,
or the bits past the frame's in its last 14-bit word, or the byte after an odd-sized frame's in its
last 16-bit word, not 0), the frame is kept as bytes, its blocks counted as a frame's.

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
end: the encoders' patterns go by the frame's 32-bit words (`open bitrate.dts`'s frames end in 1s to
a 32-bit boundary, or 2 short of it, or none, then a 0 word; `ES 6.1 16bit.dts`'s XCH sync word
comes after one of a few hundred 16-bit words, 0x5A5A the most), which made `Master Audio 7.1.dts`
10% smaller. Whether they are all 0, and how many of their last bytes are, are coded by their bits
to the next 32-bit boundary, the fill's odds going by them (`open bitrate.dts` 14% smaller), and the
bits after the subframes by whether the last ones ended in more than 31 0s too: `open bitrate.dts`'s
are 1s, 0 to 10 of them, then 0s, and how many 1s goes by how many 0s the last frame's had (5.3%
smaller). Where the frame has auxiliary data (AUXF), the bits after the subframes are its byte
count, 0s to a 32-bit boundary, its sync word and the data, which an encoder repeats frame after
frame: those bytes go against the last frame's from that boundary, where the data is, not from the
end (`Master Audio 7.1.dts`'s sit 24 or 28 bytes from it: 3.8% smaller). Even so, such a frame's
bits cost a bit or two, more where the fill's size changes. So a tail of 8 bytes or more the same as
one of the last 32 of its kind and size that weren't all 0 (the auxiliary data's are a few, one by
the fill's size) is that one, coded as which, the last first (`Master Audio 7.1.dts` 4.0% smaller,
`Master Audio 5.0 96khz.dts` 2.0%). Where the fill's size changes, the tail is offered as each of
the last 2 with one of its first 4 runs of 0s longer or shorter by their sizes' difference, a flag
each (a field of their own; after a stream's first 4 tries, only while that has been the tail in an
eighth of them): the 7-frame `xll` samples up to 5.2% smaller, `Master Audio 5.0 96khz.dts` 0.8%,
FATE's `core_51_24_48_768_1` 1.1%. `ES 6.1 16bit.dts`'s encoder leaves the bits before the XCH sync
word as an earlier frame had them at the same place: those words are what its frame buffers held (of
a ring of 8: 850 of its 2600 frames' are what one of the 8 frames before has there, 568 of them the
frame 8 before; another 316 a copy of the XCH data started a few bytes early), and the bits before
those bytes, the rest of the core's last byte, are 0. So those bits are first offered as each of the
last 8 frames' bytes at their place, the bits before them 0 (8 back, then 1 to 7; the frames' bytes
kept as they are coded), a flag each where they aren't 0 or one already offered (its core 11.5%
smaller; the other streams' no different). Where the extension doesn't parse, its bytes are the bits
after the subframes again.

Those buffers make the rest of those bits too (format 65). The encoders write a frame over the one 8
frames before, and only its own bits: the core's, 0s to the next 16-bit boundary where an extension
follows, the extension's from its sync word; then, after the last of them, the words to the frame's
end as 0s. So the bits before the sync word are 0s to that boundary, then what the frame 8 before
had there (`ES 6.1 16bit.dts`'s in 2592 frames of its 2600, `96-24.dts`'s, before X96's, in 930 of
938: but for the first 8 frames, all of them). The bits after the subframes are what it had there up
to the next 32-bit boundary, then 0s; or, where the last frame's subframes ran past that boundary,
all 0s (every frame's of `open bitrate.dts`, `3-1.dts` and `master_audio_7.1_24bit.dts` but their
first 8: `open bitrate.dts`'s 1s are the end of the frame 8 before's DSYNC). And where the
subframes end at a 4-byte boundary, the sync word comes a whole word later (`ES 6.1 16bit.dts`'s in
99 frames of 99; FATE's `dts_es.dts`'s in none of 7). So the bits before the sync word are offered
as each frame before's with the bits to the 16-bit boundary 0, then with those of the core's last
byte; XGAP goes by whether the subframes end at a 4-byte boundary (there, as the last one there, at
first a word; else none); and where there is a frame 8 before and no auxiliary data, the zero flag
of the bits after the subframes goes by which of those the rule makes them (all 0s; the frame 8
before's, which are 0s; or not), and where it makes them the frame 8 before's, a flag says whether
they are (a field of its own). `open bitrate.dts`'s bits after the subframes go from 971 bytes to
10, its core 42% smaller; `ES 6.1 16bit.dts`'s before the sync word and XGAP from 1064 to 46, its
core 26%; `3-1.dts`'s after the subframes 149 to 12 (3.0%), `master_audio_7.1_24bit.dts`'s 140 to 11
(3.7%); `96-24.dts`'s before X96's sync word 106 to 6 (0.3%); FATE's `dts_es.dts` is 2 bytes bigger.

### The audio

The encoder's input is the WAV. `anal.inc` computes what the encoder computed from it, in integers
(64- and 128-bit), so that compression and decompression agree everywhere:

- **The subband samples**: the encoder's 32-band analysis filterbank (FFmpeg's `dcaenc`'s transform,
  with the core's prototype filter: FILTS 0 the non-perfect one), the window of block t ending at
  sample 32 t + 512 of the WAV, in the decoder's units (its gain, 2). Quantized by the core's own
  step sizes, scale factors and ADPCM predictions (round to nearest, halves away from zero), they
  give 99.9% of the core's indexes. Where an index isn't the audio's, it is nearly always the one on
  the other side of the rounding boundary, and the nearer the boundary, the likelier: the encoder's
  arithmetic errs by about its signal's level. So whether an index is the audio's is coded by how
  near the boundary the audio's value lies, in steps and in the channel's loudest subband's RMS
  (log2, 16 buckets each), and by the subband's recent misses (of its last 8: 0, 1-3, 4-6, more;
  0, 1, 2, more before format 74) and its channel's (of its last 32: 0, 1-2, 3-7, more; 0, 1, 2-3,
  more before), from adaptive counts straight to the range coder (18-bit probabilities since format
  103, 15 before: the families' are 12). The channel's say 32 times sooner when the WAV has stopped
  being what the encoder had, as in the frames after a cut stream's start (its decoder's state not
  the encoder's), where nearly all of a channel's indexes miss by a step for a while, and in its
  last frame (with them, `3-1.dts`'s core 6.8% smaller, 29% of that in its first six frames and 40%
  in its last, `s.2.dts`'s first 3 MB 6.2%, FATE's XXCH samples 4-5%, the whole `s.2.dts` 0.24%;
  four of the 7-frame samples 9-14 bytes bigger, their counts split). A lossless WAV's go by the
  channel's level at the sample (the root of all its subband samples' squares) instead of its
  loudest subband's: the encoder's arithmetic errs by about all its signal there (held out, 3.6%
  fewer bits for the decisions of `s.2.dts`'s first 3 MB; its core 1.2% smaller). The encoder's
  arithmetic errs more in some subbands than in others, the same ones through a stream (`s.2.dts`'s
  encoder's: those beside the multiples of 3 kHz, 15 and 16 most, then 23, 24, 7 and 8, where an
  index this near the boundary misses 3 to 15 times as often as in subband 5; no direction to it,
  from the subbands beside it, its mirror, its neighbours in time or its own value): the nearness is
  against the level times the subband's error's scale, learned as the stream goes in quarter
  doublings from 2^-2.5 (2^-2.25 in formats 69 to 72: most streams' end up from 2^-0.75 to
  2^-3.75, a decoded WAV's near 2^-0.75; from 1, a short stream's were still on their way down at
  its end: `s.2.dts`'s first 1.2 MB 0.3% smaller), a step where the subband's misses since its last
  have strayed from those its counts expected by more than 1.25 times their root, and only where
  the channel's last 32 missed 3 times or fewer (`s.2.dts`'s core 3.8% smaller, its subband samples
  28.9 KB to 24.4; its first 3 MB 2.0%, `3-1.dts` 1.1%, the other lossless WAVs' 0.4-0.7%,
  `96-24.dts`'s 0.3%, the 7-frame samples within 4 bytes; 0.75% slower). Since format 79, where the
  WAV is at the core's rate, each subband's starts where the lossless WAVs' end on average: 2^-1 to
  2^-2 next to the multiples of 6 kHz (subbands 7-8, 15-16 and 23-24), 2^-3 to 2^-4 elsewhere, the
  same in `s.2.dts`'s, `Master Audio 7.1.dts`'s, `master_audio_7.1_24bit.dts`'s and `3-1.dts`'s,
  three encoders'; back to 2^-2.5 at the least where the scale factor search's rule first holds, a
  decoded WAV's (the short streams 0.1% smaller, `master_audio_7.1_24bit.dts`'s core 0.7%).
  Those subbands are where the encoder's FFT errs most (format 82): its cosine modulation runs
  through a 16-point FFT, subbands 2k and 31 - 2k its bin k, whose rounding errs by the values on a
  bin's path, and those are bin 0's (subbands 0 and 31, where the signal mostly is) for the most
  stages in bin 8 (subbands 15 and 16), then in 4 and 12 (7, 8, 23, 24), then in 2, 6, 10 and 14
  (3, 4, 11, 12, 19, 20, 27, 28): 72% of `s.2.dts`'s indexes' bytes were theirs. Where the bottom
  subband has 90% of the level, a miss's boundary lies within a few 2^-23 of the bottom subband's
  value of the audio's value (subbands 1 to 4 within 1, 11, 12, 19 and 20 within 3, 7 and 8 within
  4, 23 and 24 within 6, 15 and 16 within 12), within less where subband 1 (an odd bin) has it, and
  part of it is a bias: subband 7's misses lie 0 to 3 of those below their boundary, 8's 0 to 4
  above (none from any other subband, from the samples before or after, or where subband 1 or 2
  has the level). So a lossless WAV's subband sample is the analysis's plus m 2^-25 of the bottom
  subband's at the same sample, m from -12 to 12 for each subband: the m that would have made the
  audio's indexes clearly more often (by 8 of the indexes some m could change, the counts halved
  each 16384), from where the encoders' lie at the core's rate (subband 7's -7, 8's 5, 15's -4,
  16's 4, 23's -1, 24's 2, 3's -1, 4's 1, the same signs in `s.2.dts`'s, `Master Audio 7.1.dts`'s,
  `master_audio_7.1_24bit.dts`'s and `3-1.dts`'s), otherwise from 0. And at the core's rate the
  nearness is against the level along the subband's FFT path, as the HF VQ margins are (below): the
  root of a fifth of the sum of the 16 bins' energies, of those that share its bin's first stage,
  its first two, its first three, and its own (by stage 1, 2, 4 and 8 times, as the values grow,
  0.1% bigger; 1, 4, 16 and 64 times, 0.6%). `s.2.dts`'s core 0.94% smaller (the bias 0.65%, the
  nearness 0.14%, the margins 0.1%), its first 3 MB 1.0%, `3-1.dts`'s 0.6%, `Master Audio 7.1.dts`'s
  0.2%, five of the 7-frame samples 1 to 5 bytes bigger; a lossless WAV 5% slower to compress. A
  decoded WAV's misses are the lattice's, not the arithmetic's, and keep the loudest subband's; but
  not for their hit cells since format 142: a decoder rounds its output to the WAV's resolution,
  which errs by the same through the stream, more than its arithmetic does by the level, and
  against the level a 16-bit decode's quiet subbands' error scales ran down to their floor, 2^-8,
  and stayed there (subbands 22-23 of FFmpeg's 16-bit decode of the first 3 MB above, 2-3 and 16-18
  of its middle's, 19-21 of its encode of `open bitrate.dts`'s). So a decoded WAV's hit cells' nearness is
  against 2^21 of its LSBs (16 bits: 256 of the core's units), its scales then ending at 2^-4 to
  2^0.25, and its families' (nl) against the level floored at 2^19: FFmpeg's 16-bit decodes of its
  encodes (above) 1.8% smaller, its 24-bit ones 2.4%, `96-24.dts`'s core 11 bytes, FATE's XXCH and
  XBR samples' 2-10; the decodes that do better against the level, four of FATE's 7-frame samples',
  `dts_es.dts`'s, `ES 6.1 16bit.dts`'s and `open bitrate.dts`'s (all 24-bit), 2-18 bytes bigger.
  Against the level plus 2^K LSBs, the 16-bit decodes were smallest at K = 20-21, the 24-bit ones
  at 21-23; the level at the sample added, as a lossless WAV's (above), took at most 23 bytes off
  the 24-bit ones; the X96 indexes the same either way. Each
  nearness is a doubling's bucket, and within one the miss rate changes several times over; so
  (format 71) a flag's
  probability is the geometric mean of the rates of the cells either side of where its two
  nearnesses lie, each by how near (in 16ths of a doubling, its log2's fraction by the 4 bits after
  the top one; log2 and 2^x by tables made in integers alone), and the flag counts in each of them
  by its weight. And a cell's counts are halved at 128 flags, a row's at 2^14, where they were at
  2^18: the encoder's arithmetic errs by more or less than the level says as the signal goes.
  `s.2.dts`'s core 1.1% smaller (0.4% of that the halving; its subband samples 2.3%, and its scale
  factors 2.4%, their nearness by the subbands' error scales, which learn from these probabilities);
  the other lossless WAVs 0.5-2.8%, FATE's samples 1.0-3.0%, the other decoded WAVs 0.05-0.4%; 2%
  slower. And (format 72) a cell's counts are kept twice, halved at 64 flags and at 2^18, its rate
  the geometric mean of the two; after a subband's error scale has taken 16 steps, a step takes its
  misses 2 times the root of those expected astray, not 1.25, so that a long stream's scales stray
  less (`s.2.dts`'s core 0.13% smaller, `96-24.dts`'s 0.11%, FATE's samples 0.06-0.25%; four
  lossless WAVs and `open bitrate.dts` 1-3 bytes bigger). And (format 73) the faster counts halved
  at 16 flags, with a prior of one flag at the row's rate rather than two, the rows at 2^12, the
  error scales from 2^-2.5 and surer after 8 steps: FATE's samples 0.5-1.6% smaller, the lossless
  WAVs up to 0.7% (`s.2.dts`'s core 0.06%; `master_audio_7.1_24bit.dts` 2 bytes bigger),
  `96-24.dts`'s 0.04% bigger. And (format 80) where a channel coded before missed at the same
  subband's same sample (the core's channels' samples of a subsubframe go channel by channel), the
  channels miss together where the WAV isn't what the encoder had or a decoder's values aren't the
  core's: in 3 MB from `s.2.dts`'s middle, cut from it, a sample missed 0.11% of the time where no
  channel before had, 20% where one had, 85% where two; in a lossless WAV as it goes, where they are
  the channels' near ties, which the nearness already has, 0.09% and 0.5%. So such a sample's hit
  flag has cells of its own, counted always and taken while they have coded those flags in fewer
  bits than the others, by 4 bits and a 32nd (the others then not counting them): the short streams
  0.2% smaller, `master_audio_7.1_24bit.dts`'s core 1.8%, the 7-frame XLL samples with XCH and X96
  2-3%, `s.2.dts`'s the same. And (format 104) the flag's probability is then calibrated: by its
  log2 in halves, the misses each half's flags were expected to make and those they made, the
  probability times their ratio (each with a prior of 2), in two tables, one halved at 4 misses (32
  until format 106) and one at 4096, and of the fast one's, the slow one's and their mean, the one
  whose flags have cost the least so far: the cells' rates, from counts that start empty, aren't
  how often their flags miss (the regression set 0.16% smaller, with the families' cap's fourth
  bin, below, 12 bytes of that; FATE's samples 0.4-1.8%, the 7-frame XLL ones 0.4-1.0%, `s.2.dts`'s
  core 0.10%; `dts_es.dts`'s 0.17% bigger; one table, halved at 1024 misses, 0.08% smaller; since
  format 106, with the cells' prior 2 rows' worth, not 1, the regression set 0.06% smaller again,
  FATE's samples up to 1.1%, the calibration set 0.11%; since format 107 by whole log2s, the same
  tables for subbands with ADPCM prediction and without, the fast one's prior a miss: the short
  streams 22 bytes smaller, `s.2.dts`'s 5). And (format 108) what is calibrated is the cells' rates
  mixed, not their mean: its log2 the sum of the fast counts' rates' log2 (their mean's over the
  cells, by how near), the slow ones', the rows' and 1, each times a weight learned as the flags go
  (from 1/2, 1/2, 0 and 0, by the gradient of each flag's cost), the weight the sum of one for the
  mean's whole log2 and one for that in the subband: the regression set's short streams 0.13%
  smaller (`96-24.dts`'s core 0.18%, FATE's XBR and XXCH sample 0.6%), `s.2.dts`'s 3 bytes bigger
  (the subbands' weights 37 bytes of it, the rows' rates 15). Some decoded WAVs' bottom subband
  isn't the core's values but a filter of them, as if the decode had been high-passed
  (`96-24.dts`'s and `ES 6.1 16bit.dts`'s, whose indexes there were the audio's 94.5% and 99.7% of
  the time): the decode less the core's value is fitted, by least squares (the LFE's fit, below:
  every channel's together, refitted each 64 samples), on the core's last 16 values there, the
  decode's present one, its last 8 and its next 12 in the subframe (0 past it: `ES 6.1 16bit.dts`'s
  core 2.1% smaller, `96-24.dts`'s 0.3%), and the decode less the fit's is the audio's value where
  its index has been the core's more often than the decode's own (98.9% and 99.9% of those indexes,
  the cores 2.3% and 3.4% smaller; a lossless WAV's gains nothing). An index that is neither, the
  audio's 4 or more, is coded by whether it lies within 4 of the audio's, then its side (that one's,
  or the other) and its distance in unary; else as its magnitude, from the top bit down, as any
  index is without the WAV (the core 2.1% smaller in all: the 7.1 DTS-HD MA streams' and `3-1.dts`'s
  11%, `96-24.dts`'s 2.6%, `s.2.dts`'s 0.5%). A miss beside a smaller index costs less as its
  magnitude (`96-24.dts`'s, where X96's residuals are in the core's values, 1.6 KB more otherwise).
  Since format 109 one past 4 of the audio's goes as its distance past 4 and its side, where such
  misses' distances have so far taken fewer bits than their magnitudes would have (two for each bit
  of their lengths), else as its magnitude: where a cut stream's WAV ends, its last frame decoded
  alone (below), its misses lie nearer the audio's index than 0 (`Master Audio 7.1.dts`'s core
  2.0% smaller, `Master Audio 5.0 96khz.dts`'s 1.8%, `mid.dtsc` 1.6%, the regression set's short
  streams 0.6%), where FATE's XXCH samples' don't (18-76% bigger by their distances alone).
  The misses beyond the rounding boundary are mostly where the WAV ends: a piece cut from a longer
  stream has its last frame's analysis look past it at the audio that came next (`s.2.dts`'s first 3
  MB's last frame, 718 of its 734). And where the binding wasn't what the encoder had: the
  Cymatics stream's core (above) is its 7.1 bed's downmix, but its surrounds seemed to have
  something more (the core's surrounds less the bed's downmix at about -71 dB at the lowest
  frequencies, no channel of the WAV in it), which was a gain on one of each surround's channels
  before the mix, 32767/32768 with each sample rounded (format 122's gains before the mix, below).
  The surrounds' bottom subband, finely quantized, missed 9% of the time, 70% where that is loud,
  there by an offset that changes slowly. So (format 117) a subband's index is
  also the audio's with the decoded values' running difference from it added (each sample taking
  half of the mean), but for half a step of it, taken where it has been the index more often by 16
  (the counts halved at 256): 20% of those misses where loud, the stream's core 1455 bytes smaller;
  the other streams the same (by 0, the short streams 92 bytes bigger: the noise at `3-1.dts`'s
  start enough to switch).
  Since format 151 a decoded WAV's index where its step lies under the LSB goes by the decoder's
  rounding: the decoder rounds its subband samples to the LSB, so the WAV says the rounded value and
  no more of the index (the audio's value less the decoded one spreads the same, 0.05 LSB in
  `padded.dts`'s quiet start, whichever of the indexes that decode to it is the core's), and the
  audio's index, one of them, is no likelier than the rest. Of those, the one in the place most
  often the core's (counted from the one nearest 0, by how many there are and the value: 0, 1 or
  more) is the first guess where it has been the core's more often than the audio's by 16, the next
  most often the second, nearness 0 (`padded.dts`'s 241632 such indexes the audio's 75.1% of the
  time, the place's 78.9%). Where that isn't taken, the audio's value soft-thresholded toward 0 by c
  N / R (N the noise's mean square: the audio's subband samples less the decoder's, where those lie
  near 0; R the subframe's root mean square), where that has been the index more often than the
  audio's by 16: c 48, nearness 0 and the audio's the second, as where the WAV's rounding loses a
  quiet signal (FATE's core and XCH samples 1.6-3.2% smaller, the XXCH streams made from its samples
  (above) 1.7-3.8%; `x96_xch_61_24_96_3840` 6.0% bigger, its WAV bound only in part); where FFmpeg's
  encoder is made again, c 1.4, about a Laplacian prior's sqrt(2) (its 16-bit decodes' noise is
  their rounding's, apart from the signal), with the threshold's own nearness and other side: those
  decodes 1083008 bytes to 1073569 (nearness 0 made the f1 encode's 0.6% bigger). `padded.dts`'s
  core 2.6% smaller, `dtswavsample16.wav`'s 0.7%, the DTS CD's sample's 0.6%, `dtswavsample14.wav`'s
  0.4%; the lossless WAVs' streams the same.
  Since format 152 the decoded value those places go by is the audio's plus the subband's learned
  offset (the decoded values less the audio's where those lie within 64 of 0, a running mean, each
  sample a 64th of it: `padded.dts`'s bottom subband's audio lies half an LSB under the decoder's in
  its quiet start, the other subbands' none), and with it the places hold for ADPCM subbands too, by
  the decoded residual (the value less the prediction; where that is 0, the side by the audio's
  residual with the offset, the other subbands' still by the audio's value: by the residual
  everywhere `dtswavsample16.wav`'s core 23 bytes bigger). And a 16-bit decode's value is fused with
  a fit of the subband's decoded values (a 4-tap NLMS fit of the last 4) by their noises (the WAV's
  half the noise's mean square learned, the fit's its error's), taken where its index has been the
  core's more often than the one before by 16, nearness 0 and that one the second: the 16-bit
  rounding's noise hides the finer steps, and the fit, of values the decoder has exactly, much of
  it: FFmpeg's encodes' 16-bit decodes 1073569 bytes to 1059977 (1.3%). `dtswavsample14.wav`'s core
  2.2% smaller, `padded.dts`'s 0.27%, the XXCH streams made from FATE's samples 0.4-1.1%, the DTS
  CD's sample's and `dtswavsample16.wav`'s 2 and 5 bytes; the rest the same.
  Since format 154 the fit has the 4 earlier channels' decoded values of the sample in it too, an
  8-tap fit, and the fusion holds for ADPCM subbands too, by the whole value, the prediction taken
  off the fused one: FFmpeg's encodes' 16-bit decodes 1059977 bytes to 1004145 (5.3%; the stereo
  one's 25%, the 5.1 ones' 0.6-1.5%), its 16-bit decodes of `open bitrate.dts` and `padded.dts`
  (their own encoders' streams, with ADPCM subbands) 3.7% and 0.1% smaller, the ADPCM subbands'
  0.15% and 0.02% of it; with those channels' samples either side, or the WAV's next values, in the
  fit too, bigger again, as at a step of 0.03 or 0.08 for 0.05; the rest the same.
  Since format 156 a channel the WAV doesn't bind (every channel, where there is no WAV) has its
  indexes against a fit of its decoded values: each subband's by an 8-tap NLMS fit (at a step of
  0.05) of its last 4 and the 4 earlier channels' of the sample, as the 16-bit decodes' above; where
  the indexes have been nearer the fit's index than 0 by a 20th (the mean distances in steps, a 64th
  of each sample's, after 128 of the subband's samples), an index is coded as its residual from the
  fit's, smp's ql then 1 + the residuals' mean distance in half doublings, nr 15, m1 and m2 the last
  two residuals' lengths. Without their WAVs, `s.2.dts`'s core 35933571 bytes to 33934145 (71.86% of
  it to 67.86%; its first 3 MB 2.5% smaller), `open bitrate.dts`'s 3570247 to 2048075 (its two
  channels nearly the same: 42.6%), `96-24.dts`'s 6.3%, `mid.dtsc` 4.1%, `master audio 7.1
  24bit.dts`'s 3.8%, `Master Audio 7.1.dts`'s 3.0%, `ES 6.1 16bit.dts`'s 1.4%, `dts_es.dts`'s 0.8%,
  `3-1.dts`'s 0.35%; FATE's 7-frame samples, white noise, as before (the warm-up as long as they
  are; without it 1.8% bigger); with the WAV, where every channel is bound, every stream the same
  but for the format's byte. Around those settings: the residual where nearer by any, 0.04% bigger
  in all, by a 10th 0.1%, by a 5th 0.5%; the means by a 32nd 0.02%, by a 16th 0.14%; the fit's step
  0.03 0.06%, 0.02 0.13%; the earlier channels' values of the sample before in the fit too 0.04%;
  without nr's mark 0.15%, without ql's spread 0.12%, without the residuals' lengths 0.02%.
  Since format 157 that fit is by least squares, of the subband's last 16 decoded values and the
  4 earlier channels' of the sample: each sample's values weighed by 1 over its step squared and
  forgotten by 0.995 a sample, the fit made again every 16 samples (by Cholesky's factors, a ridge
  of 10^-5 of the sums' mean diagonal), after 32 samples' warm-up; the residual coded toward the
  fit's own value positive (mirrored where that lies below its index), smp's nr 15 down to 12 as
  that value lies further from its index (under 0.05, 0.12, 0.25 of the residuals' mean distance,
  or more). Without their WAVs, `s.2.dts`'s core 33934145 bytes to 33440051 (67.86% of it to
  66.87%), the 12 no-WAV streams 13807365 to 13659882 (1.07%: `mid.dtsc` 4.5%, `s.2.dts`'s first 3
  MB 2.2%, `master audio 7.1 24bit.dts`'s and `open bitrate.dts`'s 1.6%, `96-24.dts`'s 1.0%,
  FATE's 7-frame samples 2.0-3.0%, `Master Audio 7.1.dts`'s 0.03%); with the WAV, where every
  channel is bound, every stream the same but for the format's byte. A run without the WAV is
  about 14% slower (`mid.dtsc` 880 to 759 KB/s compressing, by the median of 9 runs here). Against
  it: format 156's NLMS fit of the last 32 values at its best step, 0.15, 0.42% bigger; 12 values
  0.10%; refitted every 32 samples 0.04% (every 8, 0.02% smaller, for twice the refits); a ridge
  of 10^-3 0.26%; forgetting by 0.994 or 0.996, within 0.01%; the 7 earlier channels, the same;
  the mean distances against the scale factor 0.03% bigger; this fit's and an NLMS fit's mixed by
  their errors 0.03% smaller, for twice the work; the warm-up of 128 samples, the 7-frame samples
  2.4% bigger.
  Since format 158 such a channel's side information goes by the channel before it in the pass
  too, where that has the field in the subband this subframe and the last: scl's pn says how its
  first scale factor moved (2 or more down, 1, none, 1 up, 2 or more), and the channel's last one
  moved by as much is its prediction where that has been the value more often than the last by 4
  and an 8th; its ABITS, without a first guess, the same way; and where both channels are unbound
  and 95% of their side fields have been the same (of 32 or more), each field is first a flag of
  whether it is that one's, as a bound twin's is. Without their WAVs, the 12 no-WAV streams
  13659882 bytes to 13469575 (1.39%: `open bitrate.dts` 9.3%, its side information, bit
  allocations and scale factors 411622 bytes to 224498; FATE's 7-frame samples 7.6-11.9%, the rest
  within 0.1%; `s.2.dts`'s whole core 727 bytes bigger, 0.002%). The prediction always, the
  others 0.34% bigger (ABITS 0.26%); the flags where 75% have agreed, 0.04%; the band below's
  move as nb, 132 bytes smaller alone, 35 bigger with the rest. Otherwise a channel's side
  information without the WAV goes by its own past alone (below).
  Since format 159 that fit has 24 regressors, the 2 nearest earlier channels' values either side of
  the sample too (where it isn't the first or last of its 8: their blocks lie whole before it),
  refitted every 32 samples rather than 16, which keeps it as fast; its residuals have a family of
  their own, smn (smp's and smd's before), tuned on 9 streams' dumps without their WAVs; and an LFE
  not coded against the audio goes by a least squares fit of its decoded values (each by the 8
  before it, forgetting by 0.99 a value, refitted every 4th), its scale factor coded with its first
  value. Without their WAVs, `s.2.dts`'s core 33440778 bytes to 33185178 (66.87% of it to 66.36%),
  the 12 no-WAV streams 13469575 to 13384188 (0.63%: `96-24.dts`'s 2.0%, `mid.dtsc` 1.25%,
  `s.2.dts`'s first 3 MB 0.80%, the others 0.11-0.69%; FATE's 7-frame samples 0.5-1.5% bigger), the
  55 streams of the check lists 130451895 to 129606865 (0.65%; FFmpeg's encodes at 32 and 44.1 kHz
  0.4-0.5% bigger); of the 12 streams' 0.63%, the regressors 0.16%, the LFE's fit 0.12%, smn's
  tuning 0.38%. With the WAV, of the check lists' 67 runs 56 the same but for the format's byte, 10
  smaller (the 192 kHz WAV's quiet start, whose LFE the binding leaves out, 609 bytes to 395; the
  others by 1-3 bytes) and one as big.
  Since format 160 each decision of such a residual (its length's, the two bits below its top one,
  its sign) has a logistic distribution's probability as smn's outside prediction, which its mixers
  weigh with its counters': the distribution about the fit's value, its scale 0.6 of the residuals'
  mean distance, within the index's range (by itself it codes `mid.dtsc`'s residuals 0.9% smaller
  than smn did, `96-24.dts`'s as small). The spreads follow the subband's step where it changes (by
  the step over the root of its scale factor), and smn's nl is the fit's value against the spread,
  pk whether the nearest earlier channel has no values in the subband. Without their WAVs,
  `s.2.dts`'s core 33185178 bytes to 32939110 (66.36% of it to 65.87%), the 12 no-WAV streams
  13384188 to 13338083 (0.34%: `mid.dtsc` 1.13%, `96-24.dts` 0.66%, the others 0.08-0.60%; FATE's
  7-frame samples 1.3-2.9% bigger), the 55 streams of the check lists 129606865 to 129102621 (0.39%;
  the short ones, of 2.3-6.6 KB, 1.3-5.1% bigger, 1.6 KB in all); of the 12 streams' 0.34%, the
  outside prediction 0.18%, nl and pk 0.05%, the spreads 0.08%, smn's retune 0.04%. With the WAV,
  every run of the check lists the same but for the format's byte.
  Since format 161 such a channel's ADPCM predictor is ranked by the LPC of its subband's last 64
  decoded values, as the encoders' two-stage search goes (the 64 groups by their codewords'
  distance from that LPC, then the group's entries by theirs, the last predictor first in both);
  its PMODE has that LPC's gain and the values' lag-1 correlation as sid's contexts; its first
  scale factor, where nothing else predicts it, is the last one moved 0.85 of the way to the
  subband's running mean of them (each new one an 8th of it), scl's nb how far that mean lies; and
  its ABITS, without a first guess, the subband's ABITS' running mean, rounded, once it has had
  more than 6. Without their WAVs, `s.2.dts`'s core 32939110 bytes to 32648442 (65.87% of it to
  65.29%), the 12 no-WAV streams 13338083 to 13240895 (0.73%: `mid.dtsc` 1.33%, `s.2.dts`'s first 3
  MB 0.96%, `master audio 7.1 24bit.dts` 0.93%, the others 0.33-0.92%), the 55 streams of the
  check lists 129102621 to 128675547 (0.33%; two bigger, by 4202 bytes and 2); of the 12 streams'
  0.73%, the scale factors 0.38%, the predictors 0.16%, ABITS 0.16%, PMODE 0.03%; for 6.4% more
  instructions without the WAV (`mid.dtsc`'s, compressing; FATE's 7-frame XCH sample 3.3%). With
  the WAV, every run of the check lists the same but for the format's byte.
  Since format 162 the scale factors' RMS rule bounds such a channel's indexes, part by part (a
  part: a subband's subsubframes before its transient, or those from it on, 8 to 32 values). The
  encoders take the entry nearest the RMS of the part's values, so the indexes' energy (their
  squares' sum) lies in that entry's cell: from the midpoint with the entry below to that with the
  one above, in steps, times the stream's ratio of the parts' energies to their cells' centres
  (`nw_g()`, learned: 0.88-1.02 in the long streams that keep to the rule), plus the rounding's n/12
  and a margin for its spread (`nw_cell()`). Each index of ABITS 2 or more is weighed by how likely
  the indexes after it in its part then put the part's energy in its cell (`nw_bud()`): 0.98 of that
  probability, their squares' sum a chi-square of 1.5 times as many degrees of freedom as there are
  of them (lighter-tailed than independent normal values', the scale factor following their level),
  each of the variance the cell's centre makes, and 0.02; constant over 16 pieces of the index's
  square, exact for the part's last index, and the logistic's masses weighed piece by piece. The
  values the fit doesn't serve (its residuals' spread not under 0.95 of the values', or a subband's
  first 32) are smn's now too, at every ABITS: about the fit's value times its gain (below), the
  logistic's scale the RMS the scale factor says, nl that centre against it, nr 0 (smp's or smd's
  before). And the fit's value is shrunk toward 0 by the subband's least-squares gain of the indexes
  on it (within 0..1, its sums an 64th a sample less) where the indexes have lain nearer it so, by
  their mean distances: the fit's 24 regressors over its memory fit some of their noise. All this
  where the rule has held, in 80% of the stream's last 256 or so parts of ABITS 3 or more, or before
  16 of them: FFmpeg's encoder takes the least scale factor whose range holds the subband's peak,
  and 6-13% of its encodes' parts lie in their cells (`mid.dtsc`'s 97%, `padded.dts`'s 92%,
  `dtswavsample16.wav`'s 62%, FATE's 7-frame samples' 50-67%); without that gate the 55 streams
  below would have been 59815 bytes bigger (FFmpeg's `f4.dts` by 14%, FATE's 7-frame samples by
  2-10%), though `dtswavsample16.wav` 202302 smaller. Without their WAVs, `s.2.dts`'s core 32648442
  bytes to 32111131 (65.29% of it to 64.21%), the 12 no-WAV streams 13240895 to 12991892 (1.88%:
  1.64-2.15% each but for FATE's 7-frame samples, those -0.45 to 0.39%), the 55 streams of the check
  lists 128675547 to 127160127 (1.18%; 18 bigger, FFmpeg's encodes by 0.01-0.05%, FATE's 7-frame
  samples by 1-26 bytes, one other by 166 bytes of 8.7 MB); of the 12 streams' 1.88%, the shrink
  0.10%, the budget for the fit's residuals 0.63%, the values the fit doesn't serve 1.15% (the
  chi-square's 1.5 times the degrees of freedom 0.02%); for 36% more instructions without the WAV
  (`mid.dtsc`'s, compressing: 601 KB/s on this host, 891 before; FATE's 7-frame XCH sample 5.4%).
  With the WAV, every run of the check lists the same but for the format's byte.
  Since format 163, where the rule's cell hasn't held, the cell the stream's parts have kept to:
  the 3% and 97% points of their energies against their cells' centres (in eighths of a doubling,
  each part a 512th less), where those lie within 2 doublings of each other, from 64 parts on, the
  budget and smn's values the fit doesn't serve going by that cell (the gate still counting the
  rule's own cells: `dtswavsample16.wav`'s parts lie 62% in them). And the peak rule, FFmpeg's
  encoder's: it takes the least scale factor that keeps the subband's peak within the quantizer's
  top index, so that a part's peak index lies at that top or near it (`f4.dts`'s at ABITS 1-6 the
  top in 99.1-100% of its parts, at 7 the top or one under it, at 8 within 3 of 16, at 9 and 10
  within 0.85 of the top; `mid.dtsc`'s at ABITS 3 the top in 59%). For each ABITS (11 and up as
  one) the 3% and 99.7% points of the parts' peaks, in 64ths of the top index (each part a 512th
  less), make a cell: where the RMS rule's budget isn't used, from 32 parts on, an index whose
  part's peak hasn't reached the cell's lower end weighs, under that end, 0.98 of how often such
  parts' peaks have then reached it in as many places after, plus 0.02, and past the cell's upper
  end 0.02 (`nw_bud_pk()`); and at ABITS 1 and 2, where 97% of the parts' peaks have reached 3/4
  of the top index or more and neither RMS cell has held, the values the fit doesn't serve are
  smn's too, the logistic's scale the parts' root mean square at that ABITS (from ABITS 3 on, smp's
  coding of them is better: `f4.dts` 0.5% bigger so at 3-6, 4.3% at 7-11, 8.5% from 12; since
  format 182 the z path's where that costs less, below). smn retuned on
  format 162's no-WAV dumps. Without their WAVs, `s.2.dts`'s core 32111131 bytes to 32074689
  (64.21% of it to 64.14%), the 12 no-WAV streams 12991892 to 12986033 (0.05%), the 55 streams of
  the check lists 127160127 to 126739942 (0.33%: `dtswavsample16.wav` 1.02%, FFmpeg's encodes
  0.13-1.94%, the others within 0.19%; 4 bigger, `sil/v.dtsc` by 1164 bytes of 4.2 MB, `Master
  Audio 5.0` by 55, two short ones by 2 and 8); of that, the learned cell 0.20%, smn's retune
  0.10%, the peak rule 0.03% (FFmpeg's encodes 0.49%); for 1.3% more instructions without the WAV
  (`mid.dtsc`'s, compressing; FATE's 7-frame XCH sample 1.9%). With the WAV, every run of the
  check lists the same but for the format's byte.
  Since format 164 a third rule, `dtswavsample16.wav`'s encoder's: the scale factor the entry
  nearest the RMS of the louder of the part's 8-blocks (its subsubframes' values: 88% of its scale
  factors, 95% at ABITS 8 or more, 98% from 12, where the part's own RMS, at its best ratio to the
  entry, makes 49%). The others' are format 162's cell: the entry nearest the part's RMS
  (`mid.dtsc`, `s2h.dts`, `dts_es.dts`, `3-1.dts`, `open bitrate.dts`, `sil/*`: 97-98% at ABITS 8
  or more), or the least at or over it (`padded.dts`, `dtswavsample14.wav`, `96-24.dts`, `ES 6.1
  16bit.dts`: 94-95%, its learned ratio some 0.88). Where the part's rule hasn't held and this one
  has, in 80% of the stream's last 256 or so parts' louder 8-blocks, from 16 of them (its cell
  `nw_cell()`'s for 8 values, its ratio the stream's own), each index of ABITS 2 or more is weighed
  by how likely the rest of its 8-block then keep that block's energy under the cell's upper end,
  and, where no 8-block before it in the part has reached the lower end: at the part's last 8-block,
  put it in the cell; before that, in the cell or under it as often as a later 8-block has then
  reached it (`nw_bud()`'s chi-square of the block's rest, with a weight for the lower end); the
  budget's floor 0.005 (at 0.02, 7139 bytes bigger). Without their WAVs, the 55 streams of the check
  lists 126739942 bytes to 126531976 (0.16%: `dtswavsample16.wav` 30759423 to 30551457, 0.68%; no
  other changed); the budget from ABITS 4, 6 or 8 only, 11922, 27742 and 47487 bytes bigger. For
  0.65% more instructions without the WAV (`mid.dtsc`'s, compressing; FATE's 7-frame XCH sample
  1.0%). With the WAV, every run of the check lists the same but for the format's byte.
  Since format 165 such a channel codes each subband's first scale factor before its ABITS, where
  it has one: first whether the ABITS is 0 (a flag, by whether the last one was: none, 0 or more),
  then, where it isn't, the scale factor (scl's ab the last ABITS, the subband's not yet known:
  `s.2.dts`'s first 3 MB's scale factors 0.1% bigger), then the ABITS against the energy that scale
  factor is, as a bound channel's go by its WAV's: abt's es, de and nb are its scale factor's, nn
  the ABITS seen at the scale factors either side. The ABITS a subband had the last time it had that
  scale factor, in any channel, is its ABITS 86% of the time in `s.2.dts`'s first 3 MB, where the
  last ABITS is 63%; it is the first guess where its misses have weighed less than those of the
  guess as before (each 1 + the log2 of its distance, in 16ths, the last 256 or so weighing most:
  `nw_gpen()`; by their hits alone `open bitrate.dts` 0.26% bigger and `dtswavsample16.wav` 0.04%,
  whose encoder's ABITS go another way: both guesses right 42-46% of the time there, the one seen
  at the scale factor further off). The second guess is then the ABITS seen at the nearest scale
  factor, within 8, where it differs (89% of the first guess's misses in `s.2.dts`'s first 3 MB; on
  the side the subframe's misses have gone, where both lie as near: a miss goes their way 72% of the
  time), sm how near that lies and on which side (1-8 below, 9-16 above: the first guess misses 27%
  of the time a step from it, 3% at two, under 1% further), sx which way the subframe's misses have
  gone. Without their WAVs, `s.2.dts`'s whole core 32074689 bytes to 31821770 (64.14% of it to
  63.63%; its first 3 MB's bit allocations 13739 bytes to 5298), the 55 streams of the check lists
  126531976 to 126068166 (0.37%: `ES 6.1 16bit.dts` 1.11%, `96-24.dts` 0.96%, `mid.dtsc` 0.89%,
  `padded.dts` 0.75%, `open bitrate.dts` 0.35%, FFmpeg's encodes 0.004-0.45%; 21 bigger:
  `dtswavsample16.wav` by 10511 bytes, 0.03%, the DTS CD's sample and its packings by 426-454, the
  short ones by 2-30). Against it: the order chosen by subframe, its scale factors first where the
  ABITS seen at them have done better, else as before, a little bigger on every stream tried (the
  two orders' statistics mixed); the zero flag by the subband below's and the channel before's too,
  49 bytes bigger on 12 streams; the guess seen at the scale factor only once 1024 ABITS have been,
  the short ones 2-15 bytes bigger than format 164's, not 2-30, but the long ones 23-93 bytes bigger
  than without it; the scale factor's ab the ABITS as format 164 would have guessed them, 3-8 bytes
  smaller a stream, left out. The instructions the same without the WAV (`mid.dtsc`'s,
  compressing, 0.05% fewer; FATE's 7-frame XCH sample 0.2% more). With the WAV, every run of the
  check lists the same but for the format's byte.
  Since format 166 such a channel's ABITS and scale factors are families of their own, abn and scn
  (abt's and scl's before; their variables the same), tuned on dumps without the WAV (`s2h.dts`,
  `mid.dtsc`, `dts_es.dts`, 1 MB cuts of seven more, FFmpeg's `f4.dts`, FATE's three 7-frame samples
  each 16 times): the two families' 399161 bytes there to 387927 (abn's 9.2% smaller, scn's 1.3%).
  Without their WAVs, `s.2.dts`'s whole core 31821770 bytes to 31786140 (63.63% of it to 63.56%),
  the 55 streams of the check lists 126068166 to 125820849 (0.20%, none bigger: FATE's 7- and
  14-frame samples 0.6-1.8%, `dts_es.dts` 0.32%, `dtswavsample16.wav` 0.27%, the rest 0.03-0.28%).
  The instructions the same (`mid.dtsc`'s, compressing, 0.02% more; FATE's 7-frame XCH sample 0.05%
  more). With the WAV, every run of the check lists the same but for the format's byte.
  Since format 167 its side fields and its ADPCM predictors' ranks are families of their own too,
  sdn and vpn (sid's and vqp's, then, once under 90% of the stream's indexes had been the audio's,
  the decoded WAVs' sdd and vpd), and the LFE's samples are sdn's where the LFE isn't bound; tuned
  on dumps without the WAV (format 166's set): the two families' 195033 bytes there to 190886
  (sdn's 2.9% smaller, vpn's 0.5%). Without their WAVs, `s.2.dts`'s whole core 31786140 bytes to
  31782609 (63.56% of it to 63.55%), the 55 streams of the check lists 125820849 to 125784931
  (0.03%: FATE's 7- and 14-frame samples 0.17-1.16%, `Fire__VooDoo_Studio_30_sec.wav` 0.11%, the
  rest up to 0.05%; two a byte bigger). With the WAV, 56 of the check lists' 67 runs the same but
  for the format's byte; the 11 where a channel or the LFE isn't bound coded otherwise, the same
  size but for `z192/z.dtsc`'s, a byte bigger.
  Since format 168, in the bottom four subbands, a second fit beside that one, a longer one: its 24
  regressors and then the 17th to the 56th value before, 64 in all, the two by one Cholesky (the
  short fit's sums are the long one's first rows, so its solution is the leading part of the long
  one's factoring, the same as before); the long one's value taken where the mean square of its
  errors has been under 0.95 of the short one's, each sample a 64th of them. Without their WAVs,
  `s.2.dts`'s whole core 31782609 bytes to 31634791 (63.55% of it to 63.26%), the 55 streams of the
  check lists 125784931 to 125339291 (0.35%: `96-24.dts` 0.81%, `mid.dtsc` 0.75%, `3-1.dts` 0.73%,
  FFmpeg's `f2.dts` 0.67%, `dtswavsample16.wav` 0.65%, the other long ones 0.004-0.54%), of which
  the long fit 0.35% and the decoded WAVs' families' retune (below), which code the values the fits
  don't serve (a subband's first 32), 0.006%: by it the 7- and 14-frame samples, whose values are
  mostly those, 6-31 bytes bigger (0.2-0.7%), and FFmpeg's `f4.dts` 777 bytes, the long fit taking
  those back on it. On 9 of the streams: a single fit of 8 or 16 more of the past, 0.01% smaller
  (the DTS CD's sample 0.33% and 0.63% bigger); the long fit in every subband, 8, 16 or 24 more,
  0.16%, 0.30% and 0.35% (24: 29.7% more instructions); 40 more under 8 subbands, 0.40% (20.1%),
  under 2 0.36%, 48 more under 4 0.39%; with 16 more in every subband, taken where nearer at all
  0.26%, by 0.97 0.28%, by 0.90 0.28%, the two mixed by their errors (inverse) 0.25%; with 40 more
  under 8, the long one refitted every 64 samples 0.37%, both forgetting by 0.996 the same, by 0.997
  0.36%. For 8.9% more instructions without the WAV (`mid.dtsc`'s, compressing; FATE's 7-frame XCH
  sample 22%, the fits most of its work): the medians of 15 runs interleaved with format 167's build
  on this host, `mid.dtsc` without its WAV 518 KB/s to 456 compressing, 513 to 445 decompressing,
  the XCH sample 493 to 422 and 496 to 406, and with its WAV 335 to 336 and 377 to 368 (the same
  instructions, 0.3% more). With the WAV, every run of the check lists as by the retune alone.
  Since format 169 that fit's errors are cascaded: an NLMS fit of its last 32 errors, in steps
  (each step 0.02 of their power), and of what that leaves a least squares fit by the 2 nearest
  earlier channels' errors and values of the sample, solved at every sample (by L D L', its ridge
  0.04 of its mean diagonal), forgetting by 0.88 a sample; the fit's value alone, with the NLMS
  fit's, or with both, whichever's errors have the least mean square (each sample a 64th of it),
  and the NLMS fit stepped by the error with both where that has been the less, else by its own.
  What an earlier channel's error says of this one's moves faster than the fit's 200 or so samples
  of memory follow: the 4 earlier channels' errors as the fit's own regressors make the 9 streams
  below 0.03% smaller, in the NLMS fit at their own step (0.1) 0.34%, by a least squares fit
  forgetting by 0.9 0.55%. Without their WAVs, `s.2.dts`'s whole core 31634791 bytes to 31560029
  (63.26% of it to 63.11%), the 55 streams of the check lists 125339291 to 124808444 (0.42%:
  `Fire__VooDoo_Studio_30_sec.wav` and its two packings 2.17%, `z192/z.dtsc` 1.9%,
  `dtswavsample16.wav` 0.61%, `mid.dtsc` 0.58%, FFmpeg's `f2.dts` 0.54%, `lfe/l0.dtsc` 0.53%,
  `96-24.dts` 0.41%, the rest up to 0.37%; 21 bigger: FATE's 7- and 14-frame samples, noise, by
  37-131 bytes, 0.9-2.5%, FFmpeg's `f4.dts` and its packing by 1117 bytes, 0.14%, its 16-bit
  `e16.dts` by 828, 0.07%, `f1c.dts`'s packing by 80, `f3.dts` by 68, `sur/s.dts` by 116). On 9 of
  the streams (10497257 bytes to 10427485, 0.66%): the NLMS fit alone 0.15% (16 or 64 taps 0.13%;
  with 4 or 8 of the fit's own regressors too 0.10% and 0.09%, those alone nothing); the 4 earlier
  channels' errors by least squares forgetting by 0.95 0.52%, by 0.98 0.40%, by 0.8 0.50%; their
  values too, 8 by 8, 0.59% (the 4's or the 8's by their errors 0.60%); the 2 nearest's errors
  either side of the sample too 0.42%; both parts taken or neither 0.62%; the NLMS fit stepped by
  its own error 0.63%, by both's 0.65%; a warm-up of 16 or 32 samples, or the fit's errors at
  0.98, no smaller; the subband below's values of the 8 as the fit's regressors 0.06% bigger; the
  RMS rule's budget at ABITS 1 too 0.03% bigger (there a part's energy lies in its cell 81-89% of
  the time, at 9 and more 96-98%). For 3.9% more instructions without the WAV (`mid.dtsc`'s,
  compressing; FATE's 7-frame XCH sample 6.9%): the medians of 15 runs interleaved with format
  168's build on this host, `mid.dtsc` without its WAV 497 KB/s to 479 compressing, 488 to 469
  decompressing, the XCH sample 460 to 427 and 461 to 430, and with its WAV 348 both and 392 to
  393 (0.06% more instructions). With the WAV, every run of the check lists the size format 168's
  build makes, 1455804 bytes in all.
  Since format 170 the fits are refitted every 16 samples (32 before) and forget more slowly, by
  0.9985 a sample (0.995 before), those of the bottom four subbands by 0.9965; the cascade's mean
  squares go by a 128th a sample (a 64th before), and the residual's logistic scale is 0.75 of the
  spread (0.6 before): with the cascade to follow what moves fast, the fits want a longer memory
  than they did. Without their WAVs, `s.2.dts`'s whole core 31560029 bytes to 31461240 (63.11% of
  it to 62.91%), the 55 streams of the check lists 124808444 to 124639675 (0.14%: FFmpeg's encodes but
  `f2.dts` 0.26-1.5%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings 0.54%, `s.2.dts`'s first 3 MB
  0.35%, `mid.dtsc` 0.28%, `chs/n.dtsc` 0.24%, `dtswavsample14.wav` 0.19%, `dtswavsample16.wav`
  0.11%, the rest up to 0.2%; 21 bigger: the two `sil` streams by 0.21%, the longer memory taking
  more of their quiet subbands for noise, `96-24.dts` and its packings by 0.10%, FATE's short
  samples by 2-93 bytes, `z192/z.dtsc` by 84, the DTS:X 7.1.4 core by 34, FFmpeg's `f2.dts` by
  113). On 9 of the streams (10427485 bytes to 10403585, 0.23%): refitted every 32 as before
  0.20%, every 8 0.25% for 45% more instructions on FATE's 7-frame XCH sample (`dtswavsample16.wav`
  0.17% smaller, by every 16 0.11%, by every 32 0.01%), the short fits every 8 and the long ones
  every 32 0.22% (12% more); between refits a step by the last refit's factor, w += (L L')^-1 (c -
  (R + d I) w), every 1 to 8 samples, 0.6-2.2% bigger (it overshoots where a new sample lies along
  what the fit has seen little of); with refits every 32, both fits forgetting by 0.997 0.10%
  smaller, by 0.998 0.12% (`96-24.dts` 0.20% bigger), by 0.999 0.02%, by 0.993 0.13% bigger; the
  cascade's other constants, the fits' ridge and their switch, no smaller; with refits every 8,
  the spread a 32nd a sample 0.02% smaller but FFmpeg's `f4.dts` 1.7% bigger, `e16.dts` 0.5%. For
  8.8% more instructions without the WAV (`mid.dtsc`'s, compressing; FATE's 7-frame XCH sample
  15.5%, with its WAV the same): the medians of 15 runs interleaved with format 169's build, twice,
  on this host (slower today than at format 169's measure, its own build's `mid.dtsc` 383-389 KB/s
  from 497), `mid.dtsc` without its WAV 389 and 383 KB/s to 349 and 350 compressing, 374 and 368
  to 341 and 343 decompressing, the XCH sample 304 and 344 to 288 and 327 and 302 and 349 to 295
  and 289, and with its WAV, the same instructions, 261 and 257 to 262 and 255 and 292 and 257 to
  300 and 299. With the WAV, every run of the check lists the size format 169's build makes,
  1455804 bytes in all.
  Since format 171 each sample goes into the fits' sums weighed by 1 / (s^2 + e^2 / 4) as well as by
  its step, s the spread (the mean distance of the indexes from their prediction, in steps) and e
  the fit's own error, in steps: fits of what the coder pays for, the error over its spread, that
  don't follow what they can't predict. The cascade's four choices (the fit alone, with the NLMS
  fit's value, with both parts', and their blend, below) go by the means of e^2 / (e^2 + 16 s^2) of
  their errors, a 256th a sample, not by their mean squares: a choice is as good as how often it's
  near, not as how far its misses go; and its NLMS fit steps by 0.08 of the power (0.02 before), its
  least squares part forgets by 0.85 (0.88), its ridge 0.02 (0.04). The blend, a fourth choice: the
  cascade's two parts and, in the bottom four subbands, the other fit's value less the one taken, by
  a least squares fit of the fit's error on the three, forgetting by 0.998 a sample (its ridge 0.01
  of its mean diagonal, solved at every sample by L D L'). Without their WAVs, `s.2.dts`'s whole
  core 31461240 bytes to 31402725 (62.91% of it to 62.80%), the 55 streams of the check lists
  124639675 to 123818319 (0.66%: FFmpeg's 32 and 44.1 kHz encodes 12.2% and 7.5%, its `f4.dts` and
  its packing 7.4%, its 16-bit `e16.dts` 4.9%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings
  2.2%, `dtswavsample16.wav` 0.75%, `96-24.dts` and its packings 0.46%, `lfe/l0.dtsc` 0.36%,
  `chs/m.dtsc` 0.33%, `chs/n.dtsc` 0.29%, the rest up to 0.25%; 12 bigger: FFmpeg's `f2.dts` by 1945
  bytes (0.25%), `f1c.dts`'s packing by 570 (0.86%), `f1.dts` by 241, FATE's short samples by 4-43
  bytes, `dx4/cs.dtsc` by 22, `z192/z.dtsc` by 20). On 9 of the streams (10403585 bytes to 10333421,
  0.67%; the blend alone 0.05%, with the weights 0.59%): weighed by 1 / s^2 alone 0.34% smaller
  (`mid.dtsc` 0.34% bigger), by 1 / s 0.29%, by 1 / (1 + e^2 / 10) 0.10%; with e^2 by 0.1, 0.5, 1 or
  2 in place of 0.25 0.53%, 0.52%, 0.49%, 0.42% (with the blend alone before); s^1.5 for s^2 0.45%;
  for s^2 the mean square of the fit's errors, a 16th or an 8th a sample, 0.48% and 0.52%; e left
  out in the warm-up, FATE's short samples 1.9% smaller but `f2.dts` 2.7% bigger; the cascade's and
  the blend's least squares sums weighed alike too, 0.003% smaller and 0.026% bigger; the choices by
  log(1 + e^2 / (4 s^2)) as good, by e^2 / s^2 0.03% bigger, 4, 8 or 32 for the 16 the same within
  0.004%, a 128th or a 512th a sample (by the log) 0.006% and 0.002% bigger, the two fits' switch
  measured alike too 0.002% smaller; the spread a 16th a sample (and the mean |v| and the shrink's
  sums alike) 0.13% smaller, all of it `Fire__VooDoo_Studio_30_sec.wav`'s (0.63%; `mid.dtsc` 0.14%
  bigger; the spread alone so, `s.2.dts`'s whole core 0.12%); the long fit in the bottom six
  subbands, the same; for the blend, the blend of the two parts in the gate's place 0.015% smaller,
  the fit's own value among its inputs, the blend the whole prediction, 0.52% bigger, the two parts
  alone as a fourth choice 0.031% smaller, with the subband below's error of the sample too 0.041%,
  above the bottom four subbands for the third the fit's value by its weights before the last refit
  0.002% smaller than none, a 16-tap NLMS fit's of the last 16 values 0.04% bigger. The fits'
  Cholesky by rows, each entry less a dot product of whole 4s, makes 6.3% fewer instructions on
  FATE's 7-frame XCH sample but runs 7% slower on `mid.dtsc` and 14% on that sample, its dot
  products' sums waiting on each other. For 1.0% more instructions without the WAV (`mid.dtsc`'s,
  compressing; FATE's 7-frame XCH sample 1.5%, with its WAV the same): the medians of 15 runs
  interleaved with format 170's build on this host (slower again today, format 170's own `mid.dtsc`
  318-346 KB/s from 383-389), `mid.dtsc` without its WAV 318 KB/s to 310 compressing and 316 to 305
  decompressing (in a quieter hour 346 to 341 and 360 to 354), the XCH sample 273 to 276 and 278 to
  271 (of 61 runs, 292 to 282 and 295 to 282), and with its WAV 193 to 200 and 280 to 281. With the
  WAV, every run of the check lists the size format 170's build makes, 1455804 bytes in all.
  Since format 172 the logistic's scale and the fits' weights go by a blend of two spreads, the
  residuals' mean distance from the fit's value a 64th a sample (as the spread was) and an 8th: (1 -
  w) of the slow one and w of the fast, w the channel's, from 0, kept within 0..1 and stepped each
  sample by 0.005 of the gradient of the residual's log-loss under the logistic, (1 - x tanh(x / 2))
  (fast - slow) / s at x = |r| / (0.75 s), s the blend it was coded at (tanh(x / 2) as 1 - 2 times
  the logistic's tail, from its table: + - * / and the table alone, as every machine computes them),
  where the fit's value is the coder's and s 0.5 or more; the slow one is still what the fit's use
  and the family's contexts go by. A stream's channels go to the speed its residuals' spread changes
  at: `Fire__VooDoo_Studio_30_sec.wav`'s weights near 1, `mid.dtsc`'s near 0. And the long fit's 4
  oldest values, the 53rd to the 56th before, are the 4 earlier channels' fit errors of the sample,
  its size the same. Without their WAVs, `s.2.dts`'s whole core 31402725 bytes to 31376045 (62.80%
  of it to 62.74%), the 55 streams of the check lists 123818319 to 123578038 (0.194%: FFmpeg's
  16-bit `e16.dts` 1.26%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings 0.65%, FFmpeg's 32
  and 44.1 kHz encodes 0.65% and 0.33%, `dtswavsample16.wav` 0.29%, `chs/m.dtsc` 0.26%, FFmpeg's
  `f4.dts` and its packing 0.24%, `chs/n.dtsc` 0.22%, `96-24.dts` and its packings 0.19%,
  `dtswavsample14.wav` and `lfe/l0.dtsc` 0.13%, `mid.dtsc` 0.11%, the rest up to 0.10%; 12 bigger:
  `3-1.dts` by 216 bytes (0.010%), FATE's short samples by 5-64 bytes, `z192/z.dtsc` by 12,
  `dx4/cs.dtsc` by 3). On 9 of the streams 10333421 bytes to 10312315 (0.204%: Fire 0.66%,
  `96-24.dts` 0.19%, `mid.dtsc` 0.11%, the rest up to 0.05%, `3-1.dts` 0.010% bigger; the errors
  about 0.02% of it): the blend in every place of the spread, the fit's use and the contexts too,
  0.195% (the fit then taken where it isn't the better: FFmpeg's `f4.dts` 1.03% bigger), in the
  logistic's scale alone 0.09% on `f4.dts` (0.24% with the weights), with the contexts too 0.202%;
  the fast spread in the spread's place, a 16th a sample, 0.109% smaller (Fire 0.43%, but
  `s2h.dtsc`, `dts_es.dtsc` and `mid.dtsc` 0.03-0.09% bigger), from subband 8 or 16 up 0.042% and
  0.009%; one weight for the stream, a 16th or an 8th 0.155% and 0.182%, from 0.25 0.183%, by three
  groups of subbands 0.178%, each subband's 0.180% (FATE's XCH sample 23-86 bytes bigger); the
  channel's, the fast one an 11th or a 6th 0.183% and 0.191%, its steps 0.003, 0.008 or 0.012
  0.191%, 0.199%, 0.201% (the XCH sample to 87 bytes bigger), the slow one a 96th or a 128th 0.198%
  and 0.196% (these with the blend in every place); the earlier channels' errors as 4 more
  regressors of the long fit 0.028% (2.4% more instructions), of both fits 0.040% (the XCH sample
  3.9% bigger), in the short fit's place of the 2 nearest's either side 0.24% bigger (`96-24.dts`
  1.0%), in the cascade's least squares fit (the 4 errors with 2 or 4 values) 0.001-0.002% bigger,
  its sums forgetting too fast for them, among the blend's inputs 0.023-0.026% bigger (Fire 0.2%),
  with a ridge of each input's own diagonal 0.008-0.020%. Offline, on format 170's `mid.dtsc`: the
  surround channels' errors are 2.6-5.2% less in RMS by a fit on the 4 earlier channels' errors,
  1.5-2.9% by the 2 nearest's; the subband below's values and errors (it is coded before, its whole
  8), at the sample and either side, predict none of it; Fire's residuals over their spread are
  Laplace's, `mid.dtsc`'s the logistic's, but their ideal cost under a Laplace distribution (at a
  plain 64th-a-sample spread) is only 0.6% under the logistic's on Fire and 0.7% over it on
  `mid.dtsc`, where the model codes them 5% and 2% under it (by the decisions' costs, its families
  gain about 200 bytes over the logistic on `mid.dtsc`, 68 KB on Fire, most at the lengths' first
  decisions); and the z path's indexes, coded at 1.15-1.4 bits a sample at the coarsest allocation,
  have no neighbour (the earlier channels', the last sample's, the subband below's, the 8's nonzeros
  so far) that lowers their entropy by more than 3%, the model under each one alone already. The
  family's all but sure decisions (by the logistic, under 1 in 1000) sent to the coder straight:
  1.3% fewer instructions but 0.013% bigger (Fire 0.04%, the XCH sample 1.5%). For 0.06% more
  instructions without the WAV (`mid.dtsc`'s, compressing; FATE's 7-frame XCH sample 0.13% fewer,
  with its WAV 0.26% fewer): the medians of 15 runs interleaved with format 171's build on this
  host, `mid.dtsc` without its WAV 340 KB/s to 341 compressing and 322 to 328 decompressing, the XCH
  sample 275 to 272 and 283 to 253 (its best 334 to 358 and 350 to 326: too short a run to time
  apart), and with its WAV 260 to 257 and 286 to 299. With the WAV, every run of the check lists the
  size format 171's build makes, 1455804 bytes in all.
  Since format 173 an unbound channel's HF VQ vectors, but for whether each is the subband's last
  one (sdn's still, first), go by a mixture of six guesses at the 1024 entries: the counts of the
  stream's vectors and of the subband's (each entry 1, and 8 more each time it is seen, halved when
  they reach 2^28), and the vectors of the two channels before at the same subband and subframe,
  each entry by e^(β (c - 1)), β 5 and 12, c its cosine with that vector (the zero vector's with
  itself 1, with any other -1, any other's with it 0; e^-x by a table of 64ths made with + - * /
  alone). The guesses' weights, a set for each of the four ways those two vectors are at hand, start
  at 0.35 each for the counts and 0.075 for each other guess, and step after each vector the mixture
  codes by a 32nd toward each guess's share of its probability, at least 1/1000 each; the subband's
  last vector, which the flag has ruled out, is taken out of the mixture, and the vector's 10 bits
  go to the coder straight, each by the mixture's mass in its two halves. By it alone `chs/m.dtsc`
  is 17291 bytes smaller (sdn's cost of its 120000 vectors 9.52 bits each), `mid.dtsc` 399 (of its
  10620, 9.69). And the logistic's scale is the channel's own, from 0.75 of the spread as before,
  stepped where the blend's weight steps, by 0.002 of the gradient of the residual's log-loss in its
  log, 1 - x tanh(x / 2) at x = |r| / (a s), a the scale (the x the blend's weight steps by too),
  and kept within 0.7..0.9. Without their WAVs, `s.2.dts`'s whole core 31376045 bytes to 31353344
  (62.74% of it to 62.70%), the 55 streams of the check lists 123578038 to 123373913 (0.165%:
  `dtswavsample16.wav` 0.39%, FATE's short samples 0.04-0.46%, `Master_Audio_5.0_96khz.dtsc` 0.27%,
  `pc/p1.dtsc` 0.25%, `chs/m.dtsc` 0.23%, `lfe/l0.dtsc` 0.22%, `chs/n.dtsc` 0.15%, `dts_es.dtsc`
  0.11%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings and `ES_6.1_16bit.dtsc` and its
  packing 0.08%, the rest up to 0.07%, `dx4/cs.dtsc` the same; 5 bigger: FFmpeg's `f1.dts`, `f2.dts`
  and `f3.dts` by 3, 37 and 8 bytes, `sur/s.dts` by 44, `z192/z.dtsc` by 14, 0.46%). On 10 of the
  streams 18831526 bytes to 18804298 (0.145%; the mixture alone 0.119%): one β, 8, 12 or 5, 0.113%,
  0.110%, 0.097% (each entry from half a count and the zero vector's cosines 0, FATE's 7-frame XCH
  sample, a third of its 142 vectors the zero vector and half the channel before's, 0.54% bigger at
  8); β 4 and 16, 6 and 10, 0.117% and 0.118% (5 and 12 0.118%, the weights then stepping by a
  64th); each entry from a 16th or a 32nd of a count 0.118% and 0.117% (an 8th 0.118%); the step
  0.008 0.116%, 0.03-0.06 0.120%. In the flag's place the subband's last vector as a seventh guess,
  from 0.05 or 0.2 of the weight: `z192/z.dtsc`, whose 9210 vectors are nearly all the zero vector
  and the subband's last, 3082 and 3077 bytes, with the flag 3052 (format 172's 3038), with neither
  3116 (without the flag `mid.dtsc` and `s2h.dtsc` 7 and 4 bytes smaller). The scale, on the 8
  streams it moves the most, against the mixture alone: within 0.5..1.2 2099 bytes smaller, but
  FFmpeg's 16-bit `e16.dts` 356 bigger and its 32 kHz encode 200; by 0.001 1456 smaller; within
  0.6..1.0, 0.6..0.9 or 0.55..0.85 2673, 2640, 2466 (`e16.dts` 54, 78, 209 bigger); within 0.7..0.9
  2272 (`e16.dts` 55 smaller, the 32 kHz encode 24, FFmpeg's `f2.dts` 37 bigger, `sur/s.dts` 44);
  before the mixture, on 9 streams within 0.5..1.2, by 0.0005, 0.002, 0.005 or 0.012 0.019%, 0.030%,
  0.038%, 0.032%. The earlier channels' surprise at the sample's place in its 8 as smn's spare
  context again, 0.000%. Offline: by their counts in hindsight the no-WAV HF VQ vectors would cost
  9.27-9.39 bits each on `mid.dtsc`, `s2h.dtsc` and `chs/m.dtsc`, by their counts so far 9.47-9.62;
  a vector is the channel before's 0.4-0.5% of the time (`chs/m.dtsc`'s 16%), the one before that
  one's 1.6-2.4% (11%), and the subband below's no nearer than any (its mean cosine with it within
  0.002 of 0); guesses of exact copies of either gain under 0.01 bits a vector beside the guesses
  above, of a third channel's or of the subband's last vector nothing. The ADPCM predictors' ranks
  by their counts would cost 11.3 and 10.5 bits on `mid.dtsc` and `s2h.dtsc`, against vpn's 9.2 and
  9.0 by the LPC, their copies too rare to help; X96's own VQ vectors (`96-24.dts`'s 11851, without
  its WAV), 9.62 bits each by such a mixture, 9.68 by the stream's counts alone: left as they are.
  For 0.03% fewer instructions without the WAV (`mid.dtsc`'s, compressing, 23798 million to 23791,
  and 0.04% decompressing; FATE's 7-frame XCH sample 1.8% more, the cosines' rows made as its
  vectors come; with its WAV 0.04% more) and 13 MB more memory (the rows, 16 KB each, `mid.dtsc`'s
  about 790): the medians of 15 runs interleaved with format 172's build on this host, `mid.dtsc`
  without its WAV 347 KB/s to 341 compressing and 355 to 339 decompressing (again 350 to 345 and 363
  to 342, for the same instructions), the XCH sample 282 to 257 and 260 to 259 (its best 341 to 344
  and 319 to 337), and with its WAV 254 to 251 and 298 to 288. With the WAV, every run of the check
  lists the size format 172's build makes, 1455804 bytes in all.
  Since format 174 an unbound channel's ABITS, where not 0, have their first two decisions' outside
  predictions (as a bound channel's have the allocation table's shares): from counts of the
  channel's ABITS where its last ABITS (none, or 1-26), de (how the energy, its first scale factor
  coded before them, moved from the last) and lbd (how the subband below's ABITS moved this
  subframe) were the same, the first candidate's share, (n + 0.1) / (t + 2.7), and the second's of
  the rest, the counts halved where they reach 512. The family has lbd as a context of its second
  mixer's weights alone (abn's index w), and its counters see neither it with the last ABITS nor de
  with it: in `dtswavsample16.wav`'s 6.56 million ABITS, where the subband below's stays this one
  stays 88.6% of the time, where that one falls by one this one stays 47.6% of the time and falls
  29.0%, and such counts would cost 0.68 bits an ABITS, by the last ABITS and de 0.80, by the
  subband and the last ABITS 1.16, abn's 0.81. Without their WAVs, `s.2.dts`'s whole core 31353344
  bytes to 31346354 (62.70% of it to 62.68%), the 55 streams of the check lists 123373913 to
  123266780 (0.087%: `z192/z.dtsc` 0.23%, FATE's short samples 0.02-0.21%, `dtswavsample16.wav`
  0.16%, `chs/m.dtsc` 0.15%, `chs/n.dtsc` 0.12%, `open_bitrate.dtsc` 0.11%, `lfe/l0.dtsc` 0.08%,
  `dts_es.dtsc` and `96-24.dts` and its packings 0.06%, the two `sil` streams 0.06%,
  `Fire__VooDoo_Studio_30_sec.wav` and its two packings, `dtswavsample14.wav`, `pc/p1.dtsc`,
  `ES_6.1_16bit.dtsc` and its packing and `padded.dts` 0.05%, the rest up to 0.03%; 2 a byte bigger,
  `dx4/cs.dtsc` and the DTS:X 7.1.4 core). On 12 of the streams (the 10 above, `padded.dts` and
  `dtswavsample16.wav`) 57476190 bytes to 57406084 (0.122%): the counts from 0.4, 0.2 or 0.8 and
  halved at 4096, 0.120%, 0.121%, 0.118%; from 0.4 halved at 1024 or 16384, 0.121% and 0.119%; from
  0.05 halved at 256, 0.122%; the channel before's move at the subband as a fourth context, 0.116%;
  the counts by the last ABITS and lbd alone as the counts' prior, 1 or 3 counts' worth, 0.123%; the
  three in the context of one of abn's counters (its index j) instead, 0.010% bigger
  (`dtswavsample16.wav` 0.017%); such counts' share for an unbound channel's first scale factor's
  first decision too (as the last one or the prediction), by the subband below's move, the channel
  before's and its own last, 330 bytes smaller (offline, by such counts `dtswavsample16.wav`'s scale
  factors would cost 1.90 bits each, scn's about 1.80). Of the no-WAV samples' cost the fit's values
  take 53-91% (`mid.dtsc` 91%, `dtswavsample16.wav` 75%, `chs/m.dtsc` 53%), the shrunk fit's at the
  cell's RMS the rest (`chs/m.dtsc`'s 3.6 bits a sample, `dtswavsample16.wav`'s 2.5), but
  `sur/s.dts`'s 90% goes by neither, its fit's spread never under 0.95 of the indexes' own and no
  cell holding: those samples cost only 0.13-0.15 bits under the log2 of their ABITS's levels, their
  indexes all but uniform. For those at the cell's RMS a learned scale too, as the fit's logistic's
  (per channel, or by ABITS 1-2, 3-8 and the rest), 0.006% smaller on 11 of the streams; a normal
  distribution of the cell's variance in the logistic's place, 0.002% (of 0.95 or 1.05 of its
  spread, 0.004% bigger and 0.007% smaller); offline, those samples' squares over the cell's
  variance against the earlier channels' magnitudes at the same sample over their 8-block's, a
  correlation of 0.005-0.07 (`mid.dtsc`, Fire). For 0.04% more instructions without the WAV
  (`mid.dtsc`'s, compressing, 23791 million to 23802, and 0.06% decompressing, 23546 million to
  23560; FATE's 7-frame XCH sample 0.15% more, with its WAV 0.10%) and 159 KB more memory (the
  counts): the medians of 15 runs interleaved with format 173's build on this host, `mid.dtsc`
  without its WAV 346 KB/s to 333 compressing and 339 to 339 decompressing (again 343 to 337 and 343
  to 343, for the same instructions), the XCH sample 269 to 271 and 277 to 274 (again 261 to 264 and
  292 to 268), and with its WAV 239 to 222 and 264 to 292 (again 256 to 255 and 303 to 306). With
  the WAV, every run of the check lists the size format 173's build makes, 1455804 bytes in all.
  Since format 175 the sign of a difference from a prediction (a value of the hdr, sid, sdd, sdn,
  jsc, abt, abd, abn, scl, scd or scn family's past its first decisions: `model_hdr.inc`'s
  code_diff()) is a decision of the family's own, its last node, under all of the family's contexts
  and through its mixers, and only where both sides could be the value: in the field's range, and
  neither one of the candidates its first decisions ruled out; where one side alone could be, the
  sign isn't coded at all. Before, it went by a sign counter of the family's under the field alone
  (hdr's, the side fields', jsc's), the subband (abt's) or the scale factor's kind (scl's), and was
  coded where one side was ruled out too. And an unbound channel's ABITS, past the two candidates
  format 174's counts predict, have every decision's outside prediction by those counts: the
  magnitude's (code()'s nodes 0-2 and the length 1-5 of the magnitude less 2) and the sign's, each
  by the counts' shares of the values in range but the two candidates (abits_val_n()). Without their
  WAVs, `s.2.dts`'s whole core 31346354 bytes to 31305427 (62.68% of it to 62.60%), the 55 streams
  of the check lists 123266780 to 122815211 (0.366%: FATE's short samples 1.2-2.3%, `z192/z.dtsc`
  1.25%, `dtswavsample16.wav` 0.60%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings 0.59%,
  `padded.dts` 0.53%, `dtswavsample14.wav` 0.43%, `chs/m.dtsc` 0.41%, `dts_es.dtsc` 0.36%,
  `96-24.dts` and its packings 0.34%, `chs/n.dtsc` 0.33%, the DTS:X 7.1.4 core and
  `open_bitrate.dtsc` 0.28%, `lfe/l0.dtsc` 0.25%, `ES_6.1_16bit.dtsc` and its packing 0.22%, the two
  `sil` streams, FFmpeg's `f3.dts` and `pc/p1.dtsc` 0.20%, the rest 0.035-0.18%, `3-1.dtsc` the
  least; none bigger). With their WAVs, the check lists' 67 runs 1455804 bytes to 1447527 (0.57%:
  FATE's short samples 1.4-7.7%, `padded.dts` with its 24-bit decode 3.5% (4.8 KB),
  `ES_6.1_16bit.dtsc` with its 16-bit decode 1.9%, `dtswavsample16.wav` 1.7%, `dts_es.dtsc` with its
  16-bit decode 1.3%, `s2h.dtsc` with its lossless WAV 0.43%, the DTS:X runs 0.36-0.53%, FFmpeg's
  encodes up to 2.2% (with its 16-bit source or decodes 0.00-1.45%); none bigger), and `s.2.dts`'s
  whole core with its WAV 76383 bytes to 76356, the Cymatics core 60720 to 60631 (the tables above
  have this format's). On 12 of the streams (as for format 174) 57406084 bytes to 57116414 (0.505%;
  with a sign coded where a candidate ruled one side out, 57117560): the scale factors' sign alone
  (scn's) 0.129%; the ABITS' alone 0.177%, with the counts' outside predictions too 0.307% (the
  magnitude's alone 0.271%); the two families' 0.436%; the counts from 0.05 or 0.2 rather than 0.1,
  halved at 256 or 1024 rather than 512, 0.305-0.307%. For 0.31% more instructions without the WAV
  (`mid.dtsc`'s, compressing, 23802 million to 23876, and decompressing, 23560 million to 23634;
  FATE's 7-frame XCH sample 0.34% more, with its WAV 0.31%): the medians of 15 runs interleaved with
  format 174's build on this host, `mid.dtsc` without its WAV 348 KB/s to 342 compressing and 332 to
  340 decompressing (again 341 to 346 and 328 to 329), the XCH sample 236 to 242 and 259 to 261
  (again 301 to 282 and 248 to 238), and with its WAV 212 to 253 and 278 to 285 (again 253 to 229
  and 271 to 257).
  Since format 176 an unbound channel's first scale factor of a subband, coded before its ABITS
  where it had one the last subframe, has every decision's outside prediction by a least squares fit
  of its move from that one (`model_smp.inc`'s sfl_x(), sfl_q() and scale_val_n()), of 17 things:
  this subframe's first scale factors' moves at the subband of the two channels before it on its
  table, the channel before's at the subbands either side and its own at the two below, and whether
  there are the channel before's at the subband and its own below; the running mean's distance from
  the last one; the subband below's value now, the subband above's last one and the two channels
  before's values at the subband, each against the last one; the last one's move from the one before
  it; the mean move of its own subbands below and of the channel before's all; and 1. A channel has
  a fit for each of six groups of its subbands (0-1, 2-3, 4-7, 8-11, 12-19 and 20-31) and one of its
  whole, the whole's taken where the group's has had fewer than 256 scale factors; their sums decay
  by 0.999 a scale factor and are refitted every 32 (by Cholesky, ridged by a thousandth of the mean
  diagonal). The outside predictions are a logistic distribution's about the fit's value, of scale
  0.85 of the subband's mean distance from it (each scale factor a 16th; at least 0.3, and 2 before
  there is one): the first decision's (the value the first guess) its share, the magnitude's
  (code()'s nodes 0-2 and the length 1-7 of the magnitude less 2) and the sign's each the share of
  the values left in range. And the fit's value rounded is the first guess where it has been the
  scale factor more often than the first guess as before (the last one, the channel before's move or
  the running mean's), by 64 and a fourth, where the two differ (counted by channel, halved at
  1024). Without their WAVs, `s.2.dts`'s whole core 31305427 bytes to 31256652 (62.60% of it to
  62.50%), the 55 streams of the check lists 122815211 to 122526588 (0.235%: `dtswavsample16.wav`
  0.41%, `chs/n.dtsc` 0.36%, `Fire__VooDoo_Studio_30_sec.wav` and its two packings 0.32%,
  `chs/m.dtsc` and `96-24.dts` and its packings 0.31%, `dts_es.dtsc` 0.27%, FATE's short samples up
  to 0.27% (`dx4/cs.dtsc` 0.07%), `lfe/l0.dtsc` 0.20%, `ES_6.1_16bit.dtsc` and its packing 0.18%,
  `pc/p1.dtsc` 0.17%, `open_bitrate.dtsc` 0.16%, `s2h.dtsc` and its packings, `mid.dtsc`, FFmpeg's
  `f3.dts`, `dtswavsample14.wav` and the two `sil` streams 0.12-0.13%, the rest 0.01-0.08%,
  `pk145/f1c.dts.be14` the least; three the same, none bigger). With their WAVs the sizes are format
  175's (the check lists' 67 runs 1447527 bytes, `s.2.dts`'s whole core with its WAV 76356, the
  Cymatics core 60631). On 12 of the streams (as for format 175) 57116414 bytes to 56943680
  (0.302%): the outside predictions alone, the first guess as before, 0.270%; the fit's value the
  first guess always 0.302% too, but FFmpeg's `f1.dts` and `f3.dts` 644 and 515 bytes bigger (by the
  counts 503 and 514 smaller); by 32 and a fourth 0.304%, but `f1c.dts`'s packing 42 bytes bigger (6
  smaller by 64), and kept to where it last was by a margin either way, or counted for the whole
  stream (`f1.dts` 210 smaller), or by the decisions' costs rather than hits, less; as a second
  guess after the first as before missed, less again (`dts_es.dtsc` 695 bytes smaller where the
  predictions alone make it 918). The fit grew in steps: of the first 12 things alone (one fit a
  channel), 0.196% (of scale 0.6 or 1.0, 0.190% and 0.196%; forgetting by 0.998 or 0.9995, 0.196%
  and 0.194%); the 3 more (the two channels before's values, the last move) 0.237%; the fit's value
  the first guess too 0.269% (the mean distance an 8th or a 32nd a scale factor, 0.267% and 0.270%);
  the two mean moves 0.279%; groups of subbands, 3, 4, 6 or 32 of them, 0.298%, 0.297%, 0.301% and
  0.280%; the whole's fit where the group's has had fewer than 32, 64, 128 or 256 (or 512 or 1024)
  scale factors, 0.301%, 0.302%, 0.302% and 0.302% (0.301% without). Offline, at the first half's
  least squares weights by channel, such a logistic's own cost on the second half against scn's:
  `chs/m.dtsc`'s first scale factors 3.28 bits each against 3.31, `mid.dtsc`'s 3.07 against 3.06,
  Fire's 3.60 against 2.98: in the family's decisions, not in their place. Tried on top of it and
  dropped: the last subframe's residuals' energy, all of it and its last 8's, against the last one,
  as two more things, 13-43 bytes smaller on `mid.dtsc`, `s2h.dtsc` and Fire, a byte bigger on
  `dts_es.dtsc`; the ADPCM predictors' LPC from the last 64 values less their quantizer's noise in
  the lag 0 correlation, 0-39 bytes bigger, or weighed toward the newest by 0.99 a value, 36 and 50
  bytes smaller on `mid.dtsc` and `s2h.dtsc`. For 0.69% more instructions without the WAV
  (`mid.dtsc`'s, compressing, 23876 million to 24042, and 0.70% decompressing, 23634 million to
  23799; FATE's 7-frame XCH sample 0.72% more, with its WAV 0.10%) and 0.4-0.6 MB more memory at the
  peak (`mid.dtsc` without its WAV 27.7 MB to 28.3, `s2h.dtsc` with its WAV 23.7 to 24.1: the fits'
  sums zeroed at every stream's start, and a bigger binary): the medians of 15 runs interleaved with
  format 175's build on this host, `mid.dtsc` without its WAV 345 KB/s to 344 compressing and 329 to
  336 decompressing, the XCH sample 260 to 257 and 231 to 263, and with its WAV 242 to 218 and 281
  to 309 (the host's noise more than the change).
  Since format 177 an unbound channel's ABITS, where not 0, have format 174's counts mixed with a
  map's: of the ABITS by y, the first scale factor (coded before them) less the subband's offset and
  the channel's level this subframe (`model_smp.inc`'s am_mix() and am_learn()). The encoders
  allocate by such a table: in `dtswavsample16.wav`'s subframes, at ABITS 10 and up, a channel's
  first scale factors less 5.55 times their ABITS and less their subband's offset spread by 1.61
  scale factors about their mean, as they would by 1.60 were each ABITS exactly 5.55 of them and the
  rest uniform. The level is the mean, over the channel's subbands coded so far this subframe at
  ABITS 2 and up, of the first scale factor less the subband's offset and the ABITS's centre (the
  mean y at it, each ABITS a 256th of it; 5 times the ABITS plus 10 before any), and before any of
  those the last subframe's; each subband's offset moves a 16th of the way to the first scale factor
  less the level and the centre. The map is two, of subbands 0-7 and 8-31, of the ABITS counted at
  y, each y between its two nearest whole values in proportion, each count from 0.05, a y's halved
  where they pass 128. Its probabilities are mixed with the counts' by a weight for how much of the
  level is the subframe's own (none, one subband's, more), each weight the map's posterior after an
  ABITS with a 20th shared back, from 0.2: the outside predictions of the first two decisions and of
  the rest (abits_val_n()). Without their WAVs, `s.2.dts`'s whole core 31256652 bytes to 31244852
  (62.50% of it to 62.48%), the 55 streams of the check lists 122526588 to 122194262 (0.271%:
  `dtswavsample16.wav` 0.82% (its ABITS 484 KB to 242), `dts_es.dtsc` 0.28%,
  `Fire__VooDoo_Studio_30_sec.wav` and its two packings 0.24%, `open_bitrate.dtsc` 0.21%,
  `chs/n.dtsc` 0.20%, `chs/m.dtsc` and `z192/z.dtsc` 0.17%, `96-24.dts` and its packings 0.12%,
  `lfe/l0.dtsc` 0.11%, the DTS:X 7.1.4 core and `ES_6.1_16bit.dtsc` and its packing 0.09%, FATE's
  short samples and `pc/p1.dtsc` up to 0.08%, `padded.dts`, `dtswavsample14.wav`, the two `sil`
  streams and FFmpeg's encodes 0.02-0.05%, `s2h.dtsc` and its packings 0.03%, `mid.dtsc` 0.025%, the
  rest 0.003-0.016%, `sur/s.dts` the least; one the same, none bigger). With their WAVs the sizes
  are format 176's (the check lists' 67 runs 1447527 bytes, `s.2.dts`'s whole core with its WAV
  76356, the Cymatics core 60631). On 12 of the streams (as for format 176) 56943680 bytes to
  56670254 (0.480%): at y's nearest whole value, one map, one weight with a 50th shared back, the
  counts halved at 1024, the offsets a 64th, 0.431% (the counts from 0.02 or 0.2, 0.431% and 0.430%;
  a 200th or a 20th shared back, 0.426% and 0.434%; halved at 256 or 4096, 0.441% and 0.415%; the
  offsets a 16th or a 256th, 0.435% and 0.418%; the centres a 64th or a 1024th, 0.413% and 0.423%);
  with a 20th shared back, the counts halved at 256 and the offsets a 16th, 0.443%; y between its
  two nearest, 0.464%; a weight for each of the three, 0.470% (a 10th shared back, 0.470%; the
  offsets an 8th, 0.462%; halved at 128, 0.471%); two maps, split at subband 8, 12 or 16, 0.480%,
  0.480% and 0.479%. Tried and dropped: a map for each PMODE (on Fire, `dts_es.dtsc` and
  `open_bitrate.dtsc` 10-81 bytes bigger); before the channel's own level this subframe, the last
  subframe's moved as the channel before's moved (0.471% where the three weights made 0.470%); the
  mixture's two likeliest values as the first two candidates, always or where the likeliest has half
  of it (379-2905 bytes bigger); the level's subbands weighed by the inverse of their ABITS's
  variance of y plus 0.25, 1 or 4 (Fire 475-814 bytes bigger); the last subframe's level in this
  one's mean as 0.5, 1 or 2 subbands' worth (`open_bitrate.dtsc` 233-411 bytes smaller, `chs/m.dtsc`
  189-734 bigger). The fits' sums of format 176 are zeroed where a channel first uses them, not at
  every stream's start, and with the WAV are never touched. For 0.25% more instructions without the
  WAV (`mid.dtsc`'s, compressing, 24042 million to 24102, and 0.15% decompressing, 23799 million to
  23834; FATE's 7-frame XCH sample 0.37% more, with its WAV 0.08% fewer) and no more memory but the
  binary's (the model's at the peak the same, `mid.dtsc` without its WAV 25.0 MB, `s2h.dtsc` with
  its WAV 20.0-20.4, which takes 43 fewer page faults; 0.3 MB more of the binary mapped): the
  medians of 15 runs interleaved with format 176's build on this host, `mid.dtsc` without its WAV
  328 KB/s to 345 compressing and 326 to 326 decompressing, the XCH sample 269 to 275 and 273 to
  268, and with its WAV 194 to 255 and 304 to 289 (the host's noise more than the change).
  Since format 178 that level is the mean of three by their precisions (`model_smp.inc`'s
  am_level()): the last subframe's; the channel before's this subframe plus the difference of the
  two channels' levels, which moves a 16th of the way to each subframe's; and each of the subframe's
  own subbands'. Each is weighed by the inverse of its variance, learned a 64th of the way each
  subframe: of the channel's level about its last one (from 9), about the channel before's plus the
  difference (from 9), and of a subband's about the subframe's mean (from 3). In `chs/m.dtsc`'s
  subframes (a channel's level by its subbands at ABITS 10 and up), a channel's level less the
  channel before's spreads by 1.7-2.9 scale factors where its move from its last subframe's spreads
  by 2.9-3.5 (Fire's 2.2-4.3 and 6.1-7.2); and on `mid.dtsc`, `chs/m.dtsc` and Fire the map alone
  would code the ABITS 8-12% smaller with each subband's level its subframe's other subbands' (which
  the decoder cannot have). Each of the two maps is now three, by how many subbands of its own the
  level has had (none, one, more); the counts from 0.02, a 10th of each weight shared back, the
  offsets a 24th of the way. Without their WAVs, `s.2.dts`'s whole core 31244852 bytes to 31241391
  (62.48% of it to 62.47%), the 55 streams of the check lists 122194262 to 122172276 (0.018%:
  `open_bitrate.dtsc` 0.044%, `dts_es.dtsc` 0.038%, `96-24.dts` and its packings 0.033%,
  `chs/n.dtsc` 0.032%, `ES_6.1_16bit.dtsc` and its packing 0.028%, Fire and its two packings 0.026%,
  `chs/m.dtsc` 0.022%, the two `sil` streams, `lfe/l0.dtsc`, `pc/p1.dtsc` and `padded.dts`
  0.017-0.018%, `dtswavsample16.wav` 0.015% (its ABITS 242 KB to 238), `dtswavsample14.wav` 0.012%,
  `Master_Audio_5.0_96khz.dtsc` 0.011%, FFmpeg's encodes 0.002-0.031%, `s2h.dtsc` and its packings
  0.008%, `mid.dtsc` 0.007%, `sur/s.dts` 0.004%, `3-1.dtsc` 0.002%; FATE's short samples from 2
  bytes smaller to a byte bigger, three of them a byte bigger, the DTS:X 7.1.4 core and the 14-bit
  packing of `f1c.dts` the same). With their WAVs the sizes are format 177's (the check lists' 67
  runs 1447527 bytes, `s.2.dts`'s whole core with its WAV 76356, the Cymatics core 60631). On 12 of
  the streams (as for format 177) 56670254 bytes to 56660250 (0.018%): the three by their
  precisions, learned a 64th of the way, 0.0077% (a 128th, 0.0075%); the maps each three, 0.0151%;
  the constants, 0.0177% (the counts from 0.01, 0.0149%). On four (`chs/m.dtsc`, Fire, `dts_es.dtsc`
  and `open_bitrate.dtsc`, 13023822 bytes): the channel before's level plus the difference in place
  of the last subframe's, 282 bytes smaller (the two's mean, 169; with a weight of its own for the
  map, 339); that as 0.5, 1, 2 or 3 subbands' worth in the subframe's mean, 1235, 1675, 1938 and
  1884 bytes smaller (at 1, the difference learned an 8th or a 32nd of the way, 1711 and 1599; only
  where there is a channel before's, the last subframe's then none, 1419; at 2, 1727); the three by
  their precisions learned an 8th, a 16th, a 32nd or a 64th of the way, 1189, 1683, 2062 and 2193
  (from other variances, the same; the priors' precisions 2, 3 or 5 times, 1733, 1251 and 403 at a
  32nd); the maps two (none of its own, some) or three, 2614 and 2827 (with format 177's level, 377
  and 625); then the counts from 0.01, 0.02 or 0.1, 3182, 3107 and 2322; a 50th or a 10th shared
  back, 2374 and 3032 (with the counts from 0.02, a 10th 3286, a 5th 3175; from 0.01, a 10th 3362);
  the offsets an 8th, a 24th or a 32nd of the way, 1865, 3421 (with the two before) and 2800; halved
  at 64 or 256, 2834 and 2545; the weights from 0.3, 2827 as from 0.2. Tried and dropped: an unbound
  channel's HF VQ vectors guessed from its own last subframe's too, beside format 173's guesses from
  the two earlier channels' (on Fire, `s2h.dtsc`, `mid.dtsc`, `dts_es.dtsc` and `open_bitrate.dtsc`
  34 bytes smaller in all). An unbound channel's flags for an ABITS of 0 are never set on
  `mid.dtsc`, `dts_es.dtsc`, `chs/m.dtsc` or `dtswavsample16.wav`, and cost 3-103 bytes of each:
  nothing there. For 0.015% more instructions without the WAV (`mid.dtsc`'s, compressing, 24102
  million to 24106, and 0.12% decompressing, 23834 million to 23863; FATE's 7-frame XCH sample 0.08%
  more, with its WAV the same) and 0.1 MB more memory without it (the six maps: `mid.dtsc` without
  its WAV 25.0 MB to 25.2 at the peak, 46 more page faults; `s2h.dtsc` with its WAV 20.3 to 19.8):
  the medians of 15 runs interleaved with format 177's build on this host, `mid.dtsc` without its
  WAV 321 KB/s to 320 compressing and 312 to 309 decompressing, the XCH sample 240 to 257 and 253 to
  237, and with its WAV 232 to 222 and 262 to 239 (the host's noise more than the change).
  Since format 179 an unbound channel's HF VQ subbands' scale factors, which the scale factors' pass
  codes after every channel's ABITS, have format 176's fit too: one of their own, of the same
  things, with a tally of its own for the first guess; and each such scale factor is this subframe's
  first for the fits after it (the subbands above, the channels after), as an ADPCM subband's is. A
  transient's second scale factor has a fit of its own, of its move from the first
  (`model_smp.inc`'s sf2_x()): of 1; the first's move from the last subframe's, and whether there is
  one; the transient's place; the channel before's second's move from its first at the subband,
  where it is a transient there too, and whether; the subband below's, the same; and the channel
  before's first's move from its last, and whether. Its logistic's scale is 0.85 times its mean
  distance from the fit's value, by channel, and its first guess by a tally of its own. And abn and
  scn are retuned on format 178's no-WAV dumps of format 166's set (abn's 53529 bytes there to
  53040, scn's 303468 to 302737). Without their WAVs, `s.2.dts`'s whole core 31241391 bytes to
  31231375 (62.47% of it to 62.45%), the 55 streams of the check lists 122172276 to 122096503
  (0.062%: FATE's short samples 1-13 bytes each smaller, 0.04-0.42% (`dx4/cs.dtsc` 0.06%), Fire and
  its two packings 0.218%, `padded.dts` 0.115%, `chs/n.dtsc` 0.082%, `dts_es.dtsc` 0.077%,
  `dtswavsample14.wav` 0.069%, `chs/m.dtsc` 0.065%, `dtswavsample16.wav` 0.063%, `ES_6.1_16bit.dtsc`
  and its packing and FFmpeg's `f3.dts` 0.058%, `lfe/l0.dtsc` 0.037%, the two `sil` streams
  0.030-0.033%, `open_bitrate.dtsc` and `96-24.dts` and its packings 0.029%, `mid.dtsc` 0.026%,
  `pc/p1.dtsc` 0.024%, `s2h.dtsc` and its packings 0.021%, `Master_Audio_5.0_96khz.dtsc` 0.021%,
  FFmpeg's other encodes 0.005-0.024%, `3-1.dtsc` 0.009%, `sur/s.dts` 0.002%; the DTS:X 7.1.4 core
  the same, `z192/z.dtsc` and the 14-bit packing of `f1c.dts` 4 bytes bigger). With their WAVs the
  sizes are format 178's (the check lists' 67 runs 1447527 bytes, `s.2.dts`'s whole core with its
  WAV 76356, the Cymatics core 60631). On 12 of the streams (as for format 178) 56660250 bytes to
  56618870 (0.073%): the two fits 0.0637%, with abn retuned 0.0639%, with scn too 0.0730%. On six
  (`chs/m.dtsc`, Fire, `dts_es.dtsc`, `open_bitrate.dtsc`, `mid.dtsc` and `s2h.dtsc`, 14555396
  bytes): the HF VQ subbands' fit, its scale factors not this subframe's first for the rest, 2135
  bytes smaller, and they so, 4161 (in the ADPCM subbands' fits instead, 1805; the first four HF VQ
  subbands' apart from the rest's, the same, none of these streams having more); the transients' fit
  as the outside prediction alone, 9745, and the first guess too, 10517 (with the transient's place
  in three and the ABITS, and the channel before's seconds' moves at the subbands either side,
  10482; with its transient's place against this one's, 10495). Tried and dropped: an unbound
  channel's first scale factor against the trend of its subband's last values, the last 8's energy
  or peak against the 8 before or the last subframe's (on `mid.dtsc` and `dts_es.dtsc` their
  correlation with the fit's error 0.02-0.06, the logistic alone no smaller). Offline, that
  logistic's scale chosen by the last ABITS, the subband, the fit's move, the scale itself or the
  channel would take 0.3-0.5% off its own cost, the logistic alone, with a bias by the fit's move
  0.4%: the fit's error is the scale factors' own (its value right a fifth of the time). For 0.08%
  more instructions without the WAV (`mid.dtsc`'s, compressing, 24106 million to 24126, and 0.08%
  decompressing, 23863 million to 23882; FATE's 7-frame XCH sample 0.20% more, with its WAV 0.02%)
  and less memory without it (`mid.dtsc` without its WAV 25.2 MB to 24.3 at the peak, 228 fewer page
  faults; `s2h.dtsc` with its WAV the same, 20.2-20.3 MB; 0.1 MB more of the binary mapped): the
  medians of 15 runs interleaved with format 178's build on this host, `mid.dtsc` without its WAV
  334 KB/s to 334 compressing and 330 to 335 decompressing, the XCH sample 209 to 186 and 256 to
  268, and with its WAV 232 to 236 and 262 to 261 (the host's noise more than the change).
  Since format 180 an unbound channel's ADPCM predictor has the channel before's at the subband this
  subframe, where that one predicts there, as the first candidate of both stages, and the last
  predictor second (`model_smp.inc`'s code_pvq_nw()); its PMODE is coded against the channel
  before's at the subband, and its TMODE against the subband below's and the channel before's at the
  subband, both in a place of sdn's contexts that had nothing there for them (tr). Without their
  WAVs, `s.2.dts`'s whole core 31231375 bytes to 31230804 (62.45% of it, as before), the 55 streams
  of the check lists 122096503 to 122071417 (0.021%: Fire and its two packings 0.216%, `96-24.dts`
  and its packings 0.033%, `dtswavsample16.wav` 0.019%, `dtswavsample14.wav` 0.007%, the two `sil`
  streams 0.005%, `chs/m.dtsc` and `chs/n.dtsc` 0.004-0.005%, `dts_es.dtsc`, `lfe/l0.dtsc`,
  `mid.dtsc`, `s2h.dtsc` and its packings and `padded.dts` 0.002-0.004%, the rest 0.0002-0.003%;
  FATE's short samples from 3 bytes smaller to a byte bigger, FFmpeg's encodes the same or a byte
  bigger, `sur/s.dts`, the DTS:X 7.1.4 core and the 14-bit packing of `f1c.dts` the same; `3-1.dtsc`
  8 bytes bigger, `ES_6.1_16bit.dtsc` and its packing 50). With their WAVs the sizes are format
  179's (the check lists' 67 runs 1447527 bytes, `s.2.dts`'s whole core with its WAV 76356, the
  Cymatics core 60631). On 12 of the streams (as for format 179) 56618870 bytes to 56606797
  (0.021%): the predictors alone 0.015% (`dtswavsample16.wav` 4854 bytes smaller, Fire 2961). On six
  (`chs/m.dtsc`, Fire, `dts_es.dtsc`, `open_bitrate.dtsc`, `mid.dtsc` and `s2h.dtsc`, 14543247
  bytes): the predictors 3225 bytes smaller (the last one first and the channel before's second,
  3111); then the PMODEs 392 more; then the TMODEs by the subband below's 294, by the channel
  before's 1628, by both 2279 (each only as whether it is a transient, 1143; the channel before's
  where coded, else the subband below's, 1642). Tried and dropped: the channel before's TMODE in the
  place of the value before in the subframe (P), which is the value's prediction where the subband
  has no last one: 937 bytes smaller on the six, but `dtswavsample14.wav` 2441 bigger. For 0.03%
  more instructions without the WAV (`mid.dtsc`'s, compressing, 24126 million to 24133, and
  decompressing, 23882 million to 23889; FATE's 7-frame XCH sample 0.02% more, with its WAV 0.03%
  fewer) and the same memory (`mid.dtsc` without its WAV 24.3 MB to 24.4 at the peak, 19 more page
  faults; `s2h.dtsc` with its WAV 19.9-20.3 MB): the medians of 15 runs interleaved with format
  179's build on this host, `mid.dtsc` without its WAV 349 KB/s to 344 compressing and 337 to 336
  decompressing, the XCH sample 271 to 276 and 277 to 273, and with its WAV 275 to 255 and 297 to
  292 (the host's noise more than the change).
  Since format 181 an unbound LFE's value is coded under a logistic outside prediction about the
  value of its fit (`model_smp.inc`'s lfe_val_n()), of scale 0.6 of the values' mean distance from
  the fit's, each value a quarter of the way, and 0.3 at least: whether it is the fit's value
  rounded, then its distance's magnitude and sign, each by the share of the values left. An unbound
  channel's ADPCM predictor has the channel two before's at the subband as a candidate too, the
  three first (the channel before's, the last one, the channel two before's) in the order of how
  often each has been the predictor in the channel (code_pvq_nw()). And sdn and vpn are retuned on
  format 181's no-WAV dumps of format 166's set (sdn's 30757 bytes there to 30579, vpn's 61570 to
  61533). Without their WAVs, `s.2.dts`'s whole core 31230804 bytes to 31223715 (62.45% of it to
  62.44%: its LFE values 93532 bytes to 91361, its predictors 260379 to 255913), the 55 streams of
  the check lists 122071417 to 122054023 (0.014%: `dtswavsample16.wav` 0.031%, `96-24.dts` and its
  packings 0.030%, Fire and its two packings 0.029%, `mid.dtsc` 0.025%, `dtswavsample14.wav` and
  `padded.dts` 0.014%, `s2h.dtsc`, its packings and `s2h2.dtsc` 0.009-0.010%, the two `sil` streams,
  `dts_es.dtsc` and `ES_6.1_16bit.dtsc` and its packing 0.007-0.009%, `sur/s.dts` 0.006%, the rest
  0.0001-0.003%; FATE's short samples from 2 bytes smaller to 3 bigger, three of them bigger,
  FFmpeg's encodes the same or up to 15 bytes smaller; `z192/z.dtsc` 10 bytes bigger). With their
  WAVs the check lists' 67 runs 1447527 bytes to 1447535 (`z192/z.dtsc` with its WAV 9 bytes bigger,
  the 14-bit packing of `f1c.dts` against `s2h.dtsc`'s WAV a byte smaller: channels the WAV doesn't
  bind), `s.2.dts`'s whole core with its WAV 76356 bytes and the Cymatics core 60631, as before. On
  12 of the streams (as for format 180) 56606797 bytes to 56594809 (0.021%): the LFE alone 0.0071%
  (`dtswavsample16.wav` 2567 bytes smaller, `padded.dts` 721, Fire 311), the predictors' order alone
  0.0021% (`chs/m.dtsc` 17 bytes bigger), the two 0.0094%, with sdn and vpn retuned 0.021%
  (`dtswavsample16.wav` 9112 bytes smaller, its LFE values 100475 to 97197, its predictors 256842 to
  252541). On eight (`chs/m.dtsc`, Fire, `dtswavsample14.wav`, `s2h.dtsc`, `mid.dtsc`,
  `ES_6.1_16bit.dtsc`, `dts_es.dtsc` and `open_bitrate.dtsc`): the LFE's logistic 1268 bytes
  smaller; of scale 0.7, 0.85 or 1.0 of the distance, each value a 16th of the way, 1097, 1061 and
  1019; 0.6 of it, an 8th or half of the way, 1212 and 1241; the fit of 12 or 16 values, 811 and
  228; the scale by the value's size too, 1042 and 876. Tried and dropped: an HF VQ subband's
  vectors against the transient's envelope (0.06 bits a vector); a flag for an HF VQ vector the
  subband's last one (nothing). For 0.04% fewer instructions without the WAV (`mid.dtsc`'s,
  compressing, 24133 million to 24122, and decompressing, 23889 million to 23873; FATE's 7-frame XCH
  sample 0.03% fewer, with its WAV 0.04% more) and the same memory (`mid.dtsc` without its WAV 24.4
  MB to 24.3 at the peak, 33 fewer page faults; `s2h.dtsc` with its WAV 20.0-20.3 MB): the medians
  of 15 runs interleaved with format 180's build on this host, `mid.dtsc` without its WAV 334 KB/s
  to 337 compressing and 331 to 336 decompressing, the XCH sample 274 to 272 and 259 to 269, and
  with its WAV 262 to 260 and 299 to 298 (the host's noise more than the change).
  Since format 182 the peak rule's path above ABITS 2 too, where it costs less: at each ABITS (26
  and up as one) where neither RMS cell has held but the peak rule's has (from 32 parts on, 97% of
  their peaks at 3/4 of the top index or more) and the fit isn't taken, the first 512 indexes are
  coded as before (smp's) and each is priced as well as the z path's logistic would have it (about
  the shrunk fit, of the peak rule's scale, over the indexes lo..hi, its probability's log in
  integers by `anal.inc`'s lg2tab; what smp's decisions cost, by the coder's sum of them, nacc);
  from then on that ABITS's values go the z path where it priced less (`model_smp.inc`'s code_q()).
  On `sur/s.dts` the switch takes it at ABITS 3-8, 10-12 and 14-16 (at 9 and 13 the z path's
  logistic would have priced the whole stream's values 0.7% and 0.2% dearer), on FFmpeg's `f1.dts`
  at 3-8 and 10-13, `f2.dts` 3-10, `f3.dts` 3-5, its `e_32000.dts` at 6-8, 10-12 and 15-17 and
  `e_44100.dts` at 3-4, 6-7, 10-11 and 13-16; not on `f4.dts` (its lattice: smp's coding there half
  the z path's cost or less) nor on `ff16src/e16.dts` but at ABITS 24. Without their WAVs, the 55
  streams of the check lists 122054023 bytes to 122023120 (0.025%, none bigger: `sur/s.dts` 0.33%,
  8651043 bytes to 8622847, FFmpeg's encodes 0.004-0.11%, `f1.dts` 1150 bytes smaller, `f3.dts` 423,
  `e_44100.dts` 412, `e_32000.dts` 386, `f2.dts` 257, `e16.dts` 40, the 14-bit packing of `f1c.dts`
  39; the rest the same), `s.2.dts`'s whole core 31223715 bytes to 31223716 (62.44% as before). With
  their WAVs the check lists' 67 runs, `s.2.dts`'s whole core and the Cymatics core the same but for
  the format's byte. On 20 streams (the 12, `sur/s.dts` and FFmpeg's encodes, 70035026 bytes to
  70004162), the switch after 256, 1024 or 2048 indexes 27210, 30614 and 30348 bytes smaller,
  against 512's 30864 (at 256 `e16.dts` 136 bytes bigger, `f4.dts` 41); with the peak rule's cells
  by each ABITS to 26 (for 11 and up as one), 29985 (`e16.dts` 33 bytes bigger). Tried and dropped:
  the peak rule's path at every ABITS, with no switch, 103792 bytes bigger (`f4.dts` 105735,
  `e16.dts` 24440; `sur/s.dts` 22913 smaller), from ABITS 14 up alone `f4.dts` 4368 bytes bigger and
  `e16.dts` 21460, at 3-10 alone 22536 and 1584; a normal distribution's tail for code_index_nw()
  (or its mix with the logistic's), 5437 bytes bigger (`dtswavsample16.wav` 3955, `chs/m.dtsc` 1460;
  `mid.dtsc` 206 smaller); the z path's scale by the subband's own spread instead (no smaller); a
  gate on the indexes' low bits (`e16.dts`'s are as random as they come, and its z path still
  loses). Offline, on `sur/s.dts`: its subbands above 0 noise to the fit (the subband below's values
  and fit errors as regressors, R^2 0.001), smp's coding of them about the discrete normal
  distribution's cost at each ABITS; its parts' peaks within 0.875 of the top index or more in 99%
  of them at every ABITS, worth about 0.7 bits a part (3.7 KB a MB), most of which the switch takes.
  For 0.3-0.7% more instructions (callgrind: `mid.dtsc` without its WAV compressing 24122 million to
  24199, decompressing 23873 million to 23983; FATE's 7-frame XCH sample 0.7% more, with its WAV
  0.5%: the coder's test, at every decision, of whether to sum its cost) and the same memory
  (`mid.dtsc` without its WAV 24.3 MB at the peak, `s2h.dtsc` with its WAV 19.9-20.3 MB): the
  medians of 15 runs interleaved with format 181's build on this host, `mid.dtsc` without its WAV
  335 KB/s to 341 compressing and 334 to 341 decompressing, the XCH sample 267 to 270 and 273 to
  267, and with its WAV 256 to 261 and 292 to 296 (the host's noise more than the change).
  Since format 183 an unbound channel's first scale factor's fit (format 176's) has five more things
  (`model_smp.inc`'s sfl_x()): where in its cell the subband's last part fell, by its RMS (its
  indexes' energy over the RMS rule's, its scale factor's square times its samples), by its louder
  8-block's and by its peak index against the top index, each half the log2 of the ratio over the
  log2 of the scale factor table's step there, within -2..2, plus how far that part's scale factor
  lies from its subframe's first (where that first is the last one the fit moves from, else 0);
  where the channel before's last part fell by its RMS; and the running mean of the subband's level
  so found (that first plus the place by the RMS, each part an 8th of the way) against the last one.
  An RMS rule's scale factor, the entry nearest the part's RMS, says that RMS to within half a step;
  the indexes say where in the step it was, and so how far the next one has to go. Its logs are made
  as every build makes them, by frexp() and lg2tab's table (nw_log2()). And the peak rule's switch
  (format 182's) prices smp's coding by the coder's length (`coder.inc`'s coded_len(): 8 bits for
  each byte the range has been renormalized by, less the log2 of the range left, the coder's and the
  decoder's alike) rather than by summing each decision's cost, and takes the z path where its price
  less a 256th of it is the less: the logistic's price runs 0-1.1% over what the z path codes
  (`sur/s.dts`'s at ABITS 3-15), and format 182's sum of rounded-down logs ran about as far over
  smp's. Without their WAVs, the 55 streams of the check lists 122023120 bytes to 122006026 (0.014%:
  `dtswavsample16.wav` 4232 bytes smaller, `chs/m.dtsc` 3406, `chs/n.dtsc` 2622, `lfe/l0.dtsc` 1258,
  `padded.dts` 867; 22 bigger, by 1-14 bytes: FATE's 7- and 14-frame samples 1-6, `s2h.dtsc` and its
  two packings 11, `mid.dtsc` 7, the 14-bit packing of `f1c.dts` 14), `s.2.dts`'s whole core
  31223716 bytes to 31222941 (62.44% as before). With their WAVs the check lists' 67 runs,
  `s.2.dts`'s whole core and the Cymatics core the same but for the format's byte. On 20 streams
  (the 12, `sur/s.dts` and FFmpeg's encodes) 70004162 bytes to 69994174 (0.014%:
  `dtswavsample16.wav` 4232 bytes smaller, `chs/m.dtsc` 3406, `padded.dts` 867, Fire 581,
  `open_bitrate.dtsc` 141; `s2h.dtsc` 11 bytes bigger, `mid.dtsc` 7, the XCH sample 2). On the 12:
  the place by the RMS, by the louder 8-block or by the peak alone 7498, 4185 and 4147 bytes
  smaller, the RMS's and the 8-block's 7670, the RMS's and the peak's 7828, the three 8034; with the
  channel before's too 8648, with the running mean 8659, with both 9394. The switch priced by the
  coder's length at par: `sur/s.dts` 6530 bytes bigger; less a 512th, a 256th, a 128th or a 64th of
  the price, on FFmpeg's encodes and `sur/s.dts` 167 bytes bigger, 342 smaller, 2435 and 4897 bigger
  than by format 182's sum. Tried and dropped: an HF VQ subband's vector's place (its entry's RMS
  and peak against its scale factor's, in its own fit), 17 bytes smaller on the 12. For the same
  instructions compressing or 0.1-0.4% fewer (callgrind: `mid.dtsc` without its WAV 24199 million to
  24180, FATE's 7-frame XCH sample 0.2% fewer, with its WAV 0.4%: format 182's sum gone), 0.8% more
  decompressing (`mid.dtsc` without its WAV 23983 million to 24169: the coder's count of the bytes
  it has renormalized by, kept at every decision) and the same memory (`mid.dtsc` without its WAV
  24.4 MB at the peak, `s2h.dtsc` with its WAV 20.0-20.3 MB): the medians of 15 runs interleaved
  with format 182's build on this host, `mid.dtsc` without its WAV 338 KB/s to 334 compressing and
  332 to 322 decompressing, the XCH sample 253 to 249 and 260 to 274, and with its WAV 252 to 248
  and 291 both.
  Since format 184 two things more about the spread of an unbound channel's indexes. Where a cell's
  rule holds (an RMS rule's, the louder 8-block's or the learned cell), the z path's logistic
  (format 162's, about the shrunk fit) takes its scale times 2^(0.6 u) (`model_smp.inc`'s NW_ZU, in
  code_q()), u where the subband's last part fell by its RMS (format 183's nw_u) against this part's
  scale factor, in the log2 of the RMS, within half the scale factor table's step there (2^x by the
  first five terms of its series): a part's RMS lies, within its cell, near where the last one's
  did. And where the subband's step changes, the spreads (format 160's) follow the step over its
  scale factor's 3/8 power (the root of its root times its root's root, nw_qr) rather than over its
  root: as a scale factor grows, the fit's residuals grow by its 3/8 power, not by its half. Without
  their WAVs, the 55 streams of the check lists 122006026 bytes to 121991458 (0.012%:
  `dtswavsample16.wav` 4576 bytes smaller, `padded.dts` 1015, Fire 850 (two test packings of it 849
  and 856), `sil/v.dtsc` 651, `sil/t.dtsc` 633, `mid.dtsc` 372, FFmpeg's `f2.dts` 388; 7 bigger:
  FFmpeg's `f4.dts` and its packing 172 bytes each, `chs/n.dtsc` 31, `e16.dts` 16, FATE's XLL XCH
  sample 5, `lfe/l0.dtsc` 3, the 14-bit packing of `f1c.dts` 1), `s.2.dts`'s whole core 31222941
  bytes to 31212545 (62.44% to 62.41%). With their WAVs the check lists' 67 runs, `s.2.dts`'s whole
  core and the Cymatics core the same but for the format's byte. On 20 streams (the 12, `sur/s.dts`
  and FFmpeg's encodes, 69994174 bytes to 69985455): the scale's tilt alone at 0.4, 0.5, 0.6 or 0.75
  2934, 3401, 3713 and 4000 bytes smaller (at 0.75 `padded.dts` 89 bytes bigger and `3-1.dts` 64);
  the spreads' exponent alone at 0.3, 0.375, 0.4 or 0.625 3729, 5026 and 4902 bytes smaller and
  15585 bigger (at 0.375 `chs/m.dtsc` 350 bytes bigger and `f4.dts` 171, `dtswavsample16.wav` 2087
  smaller and `padded.dts` 996); both 8719 smaller, with 0.4's exponent 8572. Tried and dropped: the
  RMS rule's budget (format 162's) with its mean square tilted alike, at half the place 566 bytes
  smaller (`Fire__VooDoo_Studio_30_sec.wav` 476 bigger, `chs/m.dtsc` 264, `3-1.dts` 196), at all of
  it 4701 bigger; the louder 8-block's budget (format 164's) by that block's place, 360 smaller and
  551 bigger; the exponent by the change's direction, the root where the scale factor rose and its
  root's root where it fell or the other way round, 5383 and 15359 bytes bigger (a scale factor up
  and back down then leaves the spread changed); the exponent learned per channel, from the root, by
  the gradient of the residuals' log-loss (as format 172's blend is), 11004-15172 bytes bigger
  (`dtswavsample16.wav` 13-17 KB: the spreads weigh the fits' samples too, which the gradient leaves
  out). Offline, on `mid.dtsc`'s and `dtswavsample16.wav`'s fit-path indexes without their WAVs: the
  logistic's scale as it is the cheapest of 0.8-1.25 times it, a scale from the RMS of the subband's
  residuals in a window about each (±4 to ±32 samples, the residual itself left out) 0.4-1.6%
  dearer; the fit's gain on the indexes 11-12 dB in subband 0, 6-10 dB in subband 1, 4.3-6.1 dB
  above; so what is left there is the fit's to take. And the coder counts the bytes it renormalizes
  by (format 183's nsh) in its byte loops rather than at every decision. For 0.2% more instructions
  compressing (callgrind: `mid.dtsc` without its WAV 24180 million to 24228; FATE's 7-frame XCH
  sample 0.05% more, with its WAV 0.03%), 0.4% fewer decompressing (`mid.dtsc` without its WAV 24169
  million to 24068: the byte count's) and the same memory (`mid.dtsc` without its WAV 24.4 MB at the
  peak, `s2h.dtsc` with its WAV 19.9-20.3 MB): the medians of 15 runs interleaved with format 183's
  build on this host, `mid.dtsc` without its WAV 340 KB/s to 332 compressing and 328 to 326
  decompressing, the XCH sample 278 to 271 and 272 to 268, and with its WAV 269 to 262 and 296 to
  293.
  Since format 185 the cascade's least squares part (format 169's, of the 2 nearest earlier
  channels' errors and values of the sample) is two fits of the same, forgetting by 0.9 and by 0.97
  a sample (`model_smp.inc`'s NWC_LAM and NWC_LAM2: nwc_R, nwc_Rs), the one whose errors' mean
  (format 171's measure, nwc_m2 and nwc_m2s) has been the less taken; and the blend of the cascade's
  parts (format 171's) two as well, forgetting by 0.997 and by 0.98 (NWB_LAM, NWB_LAM2), the faster
  one's taken where its mean is under 0.99 of the other's (NWB_K). What an earlier channel's error
  says of this one's holds for a few samples in some streams and for some tens in others: one fit
  forgetting by 0.94 rather than 0.85, `dtswavsample16.wav` 43480 bytes smaller and Fire 12598
  bigger; the two, both smaller. With them the no-WAV model's older constants retuned: the NLMS
  fit's step 0.1 of the power (NWC_MU, 0.08 before), the long fit's forgetting 0.996 (NW_LAML,
  0.9965), the fits' weights' NW_WG 0.15 (0.25), the fast spread a 6th a sample (NW_BF, an 8th) and
  its blend's step 0.008 (NW_BE, 0.005), the channel's logistic scale within 0.65..1 (NW_LCL and
  NW_LCH, 0.7..0.9) by steps of 0.004 (NW_LCE, 0.002). And the two least squares solves, the blend's
  and the cascade's (nwb_solve(), nwc_solve()), by the pivots themselves rather than over their
  inverses: 4 and 10 fewer divisions at every sample, the 4 by 4 solve about 15% quicker (`e16.dts`
  83 bytes smaller, the rest the same). Without their WAVs, the 55 streams of the check lists
  121991458 bytes to 121814538 (0.145%: `dtswavsample16.wav` 82700 bytes smaller, `lfe/l0.dtsc`
  20682, `chs/m.dtsc` 14587, `sil/t.dtsc` 10984, `sil/v.dtsc` 10964, `padded.dts` 9025, `pc/p1.dtsc`
  5934, `chs/n.dtsc` 5374, `96-24.dts` and its packings 2772 each, FFmpeg's `f2.dts` 2654; 19
  bigger: `sur/s.dts` 221 bytes, `s2h2.dtsc` 103, `Master_Audio_5.0_96khz.dts` 93, `s2h.dtsc` and
  its packings 72 each, `f3.dts` 32, `e16.dts` 28, FATE's 7- and 14-frame samples 2-16), `s.2.dts`'s
  whole core 31212545 bytes to 31208782 (62.41% as before). With their WAVs the check lists' 67
  runs, `s.2.dts`'s whole core and the Cymatics core the same but for the format's byte. On 20
  streams (the 12, `sur/s.dts` and FFmpeg's encodes, 69985442 bytes to 69870666): the one fit
  forgetting by 0.9, 0.92, 0.94 or 0.96 31846, 42112, 46734 and 36918 bytes smaller (by 0.8 31987
  bigger); the two, at 0.85 and 0.94, 0.85 and 0.97, 0.8 and 0.96, 0.9 and 0.96 or 0.9 and 0.97,
  59905, 56998, 60883, 62593 and 61303 smaller (Fire between 2024 smaller and 2142 bigger); with the
  constants 99942 smaller; the second blend at 0.98 by the less of the two means, or under 0.95,
  0.97 or 0.99 of the other's, 13112, 8047, 11511 and 14951 smaller still (by the less, `padded.dts`
  852 bytes bigger and `open_bitrate.dtsc` 537: the switch's noise), at 0.95, 0.97 or 0.985 by the
  less 10116, 12930 and 12256. On the 55 and `s.2.dts`'s core: the slow fit at 0.96 rather than
  0.97, 1097 and 1748 bytes bigger; NW_WG 0.25 as before, 3746 bigger and 1064 smaller; NW_LCL 0.7
  as before, 6061 bigger and 2164 smaller; the faster blend under 0.97 of the other's, 3687 bigger
  and 438 smaller. Tried and dropped: the two fits both among the blend's inputs, 3158 bytes bigger
  than the one taken; the 3 or 4 earlier channels' errors in the fit (5 or 6 regressors, one fit at
  0.94), 3217 and 7315 bigger; the long fit (format 168's) in subbands 4 or 4 and 5 too, 13592 and
  20222 smaller, all but 1688 and 2426 of it `dtswavsample16.wav`'s, for 2.8% and 5.6% more
  instructions; the constants a step further, NWC_MU 0.12 5090 smaller (13 of the 20 bigger),
  NW_LAML 0.995 9108 smaller (the other 19 1165 bigger), NW_WG 0.1, NW_LCL 0.6, NW_BF 5, NW_BE
  0.012, NW_LCE 0.006, NWB_LAM 0.996 within 2228 (the other 19 none smaller by 300), NWC_LAM2 0.95
  1193 bigger, NW_LAM 0.998 99 smaller, the cascade's means a 128th a sample 644 bigger. For 1.7%
  more instructions compressing (callgrind: `mid.dtsc` without its WAV 24228 million to 24634;
  FATE's 7-frame XCH sample 2.4% more, with its WAV the same), 1.7% more decompressing (24068
  million to 24474) and the same memory (`mid.dtsc` without its WAV 24.4 MB at the peak, `s2h.dtsc`
  with its WAV 19.9-20.2 MB): the medians of 15 runs interleaved with format 184's build on this
  host, twice, `mid.dtsc` without its WAV 332 KB/s to 320 and 316 to 300 compressing, 314 to 319 and
  304 to 301 decompressing, the XCH sample 228 to 239 and 255 to 252, 246 to 254 and 263 to 252, and
  with its WAV 245 to 248 and 234 to 250, 285 to 279 and 277 to 283 (the host slower than at format
  184's: its build compressed `mid.dtsc` at 332 KB/s then, at 332 and 316 now).
  The same output since in 13.8% fewer instructions without the WAV (callgrind: `mid.dtsc` 24634
  million to 21226 compressing, 24474 to 20995 decompressing; FATE's 7-frame XCH sample 183.4
  million to 159.8) and 1.1% fewer with it (that sample 177.4 million to 175.4, `s2h.dtsc` 16099 to
  15922): the fits' Cholesky (nw_refit()) by blocks of 4 columns, each entry's products the same,
  in the same order, as column by column, and L z = c one more row of it; code_index_nw()'s tails
  at the ends of the magnitude's interval kept, each of the mantissa's decisions finding only its
  midpoint's, the lengths' edges found by stepping on from the last one; nw_bud()'s pieces put in
  order as they are made; the rings read newest first as two runs (nw_rev()); code_rank()'s 33
  least keys alone kept in order, each put in place by halving; and code_q() by code of its own for
  a channel the WAV binds and for one it doesn't (code_q_t(): with the rest alone the bound
  channels' 0.4% more, their registers allocated otherwise). The same memory (`mid.dtsc` without
  its WAV 24.4 MB at the peak, `s2h.dtsc` with its WAV 19.9-20.3 MB); the medians of 15 runs
  interleaved with format 185's first build on this host, twice, `mid.dtsc` without its WAV 325
  KB/s to 385 and 330 to 387 compressing, 322 to 373 and 317 to 379 decompressing, the XCH sample
  265 to 272 and 253 to 274, 260 to 271 and 253 to 267, and with its WAV 259 to 261 and 255 to 291,
  298 to 284 and 291 to 300.
  Since format 186 the cascade's NLMS part (format 169's) is five fits of the same, stepping by 0.1
  (as before), 0.25, 0.03, 0.01 and 0.003 of the power (`model_smp.inc`'s NWC_MU and nwc_mux()),
  each by its own error, its value theirs weighed by (m / m_i)^32, m_i a fit's errors' mean (format
  171's measure) and m the least of them; the weighed value's own errors' mean is the part's. And
  the cascade's four choices (the fit alone, with the NLMS part, with both parts, the blend after
  its warm-up), where the one whose mean had been the least was taken, are weighed so too, by (m /
  m_k)^64 (each power by squaring, + - * / alone). A fast fit follows what moves, a slow one is the
  surer where little does; the weights take each where its errors have been the fewer, both where
  they are near. Without their WAVs, the 55 streams of the check lists 121814538 bytes to 121710351
  (0.086%: `dtswavsample16.wav` 47030 bytes smaller, `chs/n.dtsc` 7835, `chs/m.dtsc` 7782,
  `lfe/l0.dtsc` 6234, Fire's WAV and its two packings 5706-5751 each, `padded.dts` 4377,
  `pc/p1.dtsc` 2228, `dtswavsample14.wav` 2077, the two `sil` streams 1840 and 1851; 17 bigger:
  FFmpeg's `f4.dts` and its packing 1953 bytes each (0.27%), its 16-bit `e16.dts` 1071, FATE's 7-
  and 14-frame samples 2-59 bytes, the DTS:X 7.1.4 core 4), `s.2.dts`'s whole core 31208782 bytes
  to 31188610 (62.41% to 62.37%). With their WAVs the check lists' 67 runs, `s.2.dts`'s whole core
  and the Cymatics core the same but for the format's byte. Against format 185, on the 55 and
  `s.2.dts`'s core: one more fit, the one whose mean had been the less taken, at 0.25 or 0.03,
  29006 and 9946 bytes smaller (the core 468 and 6304); two more, at 0.25 and 0.03, 0.2 and 0.03,
  0.35 and 0.03 or 0.25 and 0.05, 37795, 34898, 36535 and 36962 (the core 5804-6868); three (0.25,
  0.03, 0.01) 43921 (the core 7405); those three weighed by 1 / m_i^P, P 4, 8, 16 or 32, 45082,
  53828, 61538 and 63973 (the core 4020-10975), with 0.003 too at 16, 67099; the choices weighed
  too, by 1 / m_k^4 or 1 / m_k^16, 28057 and 90159, by (m / m_k)^32, ^64 or ^128, 102748, 98689
  and 89736 (the core 19334, 19223 and 17919); with 0.003 too, at 32 and 64, 104187 (the core
  20172). Tried and dropped: the cascade's two least squares fits weighed so too, and its two
  blends, 107068 and 108748 bytes smaller but `s.2.dts`'s core 1763 and 3411 bigger; the choices
  by the least in the warm-up, the same within 7 bytes; a fast least squares fit of the subband's
  last 8 values (forgetting by 0.98) among the blend's inputs, 4341 bigger; the long fit in subband
  4 too (format 185's NW_BL 5 again), 15250 smaller still, 10618 of it `dtswavsample16.wav`'s, for
  2.8% more instructions. For 2.2% more instructions without the WAV (callgrind: `mid.dtsc` 21226
  million to 21697 compressing, 20995 to 21466 decompressing; FATE's 7-frame XCH sample 159.8
  million to 165.3, with its WAV the same, 175.4 and 175.3) and 0.2 MB more memory at the peak
  (`mid.dtsc` without its WAV 24.4 MB to 24.5-24.6, `s2h.dtsc` with its WAV 20.0-20.3 MB to
  20.1-20.4): the medians of 15 runs interleaved with format 185's sped-up build on this host,
  twice, `mid.dtsc` without its WAV 381 KB/s to 367 and 383 to 371 compressing, 366 to 365 and 380
  to 362 decompressing, the XCH sample 257 to 258 and 229 to 225, 278 to 255 and 257 to 229, and
  with its WAV 254 to 257 and 206 to 228, 292 to 285 and 261 to 247.
  Since format 187, where the stream's scale factors keep to their rule (format 162's gate, the
  RMS rule's, or format 164's, the louder 8-block's: `model_smp.inc`'s nw_rule()), the long and
  short fits' values in the subbands under NW_BL (format 168's), where the one whose errors' mean
  square had been the less was taken, are weighed by (m / m_i)^4 (NW_SWP), and the fit's value
  shrunk and as it is (format 162's), where the one whose mean distance had been the less was
  taken, by (m / m_i)^8 (NW_SSP); the logistic's scale is the root of the spread's square plus the
  cascade's four choices' variance about their weighed value and the two fits' (by their weights),
  each times a weight of the channel's from 0, learned by the log-loss's gradient as the spread's
  blend is (nw_hl, nw_hd); and the NLMS fits step by s / sqrt(s^2 + 0.05 e^2) of their errors
  (NWC_RK), robust to the errors they can't follow as the fits' samples are weighed. A switch's
  choice is as good as its last errors, and near a tie the two are worth both; where the choices or
  the fits disagree, the residual is surer to be far. FFmpeg's encodes (their parts' peaks keep to
  the peak rule, nw_rule() false) keep the choices. Without their WAVs, the 55 streams of the check
  lists 121710351 bytes to 121626069 (0.069%: `dtswavsample16.wav` 47913 bytes smaller,
  `chs/m.dtsc` 6566, `lfe/l0.dtsc` 5176, `chs/n.dtsc` 4776, `padded.dts` 2332, `pc/p1.dtsc` 1910,
  `dtswavsample14.wav` 1831, Fire's WAV and its two packings 1247-1284 each, the two `sil` streams
  1126 and 1136; 11 bigger: FFmpeg's 16-bit `e16.dts` 851 bytes (0.08%), `e_32000.dts` 185,
  `f3.dts` 142, `e_44100.dts` 126, FATE's 7- and 14-frame samples 5-28 each, 48 smaller in all),
  `s.2.dts`'s whole core 31188610 bytes to 31176771 (62.37% to 62.34%). With their WAVs the check
  lists' 67 runs, `s.2.dts`'s whole core and the Cymatics core the same but for the format's byte.
  Against format 186, on the 55 and `s.2.dts`'s core, each alone: the fits weighed everywhere, by
  (m / m_i)^P, P 1, 2, 4, 8 or 16, 10206, 17807, 21209, 18954 and 14073 bytes smaller (the core
  1095 bigger to 4924 smaller); the choices' variance in the scale's square 14167 (the core 948),
  the NLMS fits' spread too 15372; the robust steps at 0.02, 0.05, 0.15 or 0.5, 13097, 21505,
  17592 and 2213 (the core 4 smaller to 2664 bigger); the shrink weighed everywhere, P 4, 16, 32 or
  64, 6190, 15750, 15598 and 12965, but FFmpeg's encodes 18009, 8441, 4642 and 2355 bigger.
  Together, ungated (the shrink at 16), 73197 smaller, FFmpeg's encodes 10686 bigger; behind the
  gate (the shrink at 32) 76898, the fits behind it too 78728 (FFmpeg's encodes 657 bigger), with
  the two fits' variance 79640, the shrink at 16 or 8 83322 and 84282. Tried and dropped: the
  shrink, or the fits too, soft or hard per channel by their running mean distances, 2340 and 7282
  less smaller than the soft shrink alone (the core 2271 and 5601 bigger); the robust steps only
  after the subband's warm-up, the short samples 273 smaller but the rest 3337 bigger (the slowest
  fits keep what they learn early), or behind the gate too, 930 bigger (FFmpeg's encodes 638
  smaller); the blend's third input by the hard choice, 567 bigger; the earlier channel's surprise
  in the scale, 19561 smaller but the core 1087 bigger; a second short fit forgetting faster, 18387
  smaller but the core 17492 bigger; the cascade's two least squares fits and its two blends
  weighed so too, 19998 bigger and 6048 smaller (the core 4687 and 3624 bigger). For 0.6% more
  instructions without the WAV (callgrind: `mid.dtsc` 21697 million to 21834 compressing, 21466 to
  21601 decompressing; FATE's 7-frame XCH sample 165.3 million to 166.5, with its WAV the same,
  175.3 and 175.4) and the same memory (`mid.dtsc` without its WAV 24.6 MB at the peak, `s2h.dtsc`
  with its WAV 19.9-20.3 MB): the medians of 15 runs interleaved with format 186's build on this
  host, twice, `mid.dtsc` without its WAV 354 KB/s to 350 and 374 to 366 compressing, 349 to 355
  and 362 to 355 decompressing, the XCH sample 270 to 266 and 272 to 266, 267 to 272 and 276 to
  274, and with its WAV 278 to 279 and 264 to 267, 309 to 311 and 274 to 280.
- **The WAV's end**: block t's window ends at sample 32 t + 512, so the core's last frame looks 512
  samples past the WAV's end (a decoded WAV as long as the core's frames are), at what its encoder
  had there: 0s there missed three quarters of that frame's indexes, about 2 KB of `s.2.dts`'s first
  3 MB (with the next 1024 samples of the whole `s.2.dts`'s WAV after it, it went from 5764 bytes to
  3767). So past its end each channel goes on as the linear prediction of order 64 its last 4096
  samples make (their autocorrelation under a half parabola, the newest weighted most, the windowed
  samples taken at 25 bits so that it is a sum in 64 bits; a 10000th of white noise in it;
  Levinson's recursion in the soft float, the LFE fit's), run on for 2048
  samples; or as 0s, where the encoder's input ended there: where the subbands' analysis first
  reaches past the end, compression takes whichever makes more of that subframe's indexes (those not
  ADPCM's or VQ's) and codes it in a bit. The pieces cut from longer streams went on (`s.2.dts`'s
  first 3 MB 7.2% smaller, `master audio 7.1 24bit.dts` 5.1%, FATE's `dts_es.dts` 8.4%, the others
  0.5-4%); the 7-frame samples, whole programs, ended (as before, within 2 bytes: the LFE, whose
  windows reach on further, has the continuation before the bit). But no continuation is the audio
  a cut stream went on with: where a window lies past the end by more than 224 samples of its 512,
  most of its indexes miss either way. So a second bit says whether the continuation made fewer
  than half of those indexes (of that subframe's), and where it did, such an index is coded without
  the audio, and a scale factor's values leave those samples out (`s.2.dts`'s first 3 MB 5.3%
  smaller, FATE's `dts_es.dts` 6.6%, `master audio 7.1 24bit.dts` 4.2%, `ES 6.1 16bit.dts` 3.7%,
  the other cut ones 0.5-2.7%; the 7-frame samples within a byte); since format 102 with the
  continuation's own index's length and sign as the context of its magnitude and sign (smp's ql and
  sq, else for the audio's index alone: the cut streams 1-5 bytes smaller, before smp was tuned
  again with it). Their last frame is still most of what a frame costs without the audio: 1.0-1.5
  KB of each, 15-42% of their cores' bytes.
- **The scale factors**: the table's entry nearest (linearly) the RMS of the values it scales (an
  ADPCM subband's prediction residual): 98.9% of them. The tables' first entries repeat values (1,
  1, 2, 2, 2, 2, 3, ...): the rule takes a run's first entry for an RMS below the run's value and
  its last from the value on, and so does the encoder, whose arithmetic moves the RMS across the
  value as it does across a midpoint (the HF VQ subbands, quiet, are there: 90% of theirs, and 43%
  of `s.2.dts`'s misses there are the run's other end). So the model knows where the prediction's
  run ends: one entry below or above, two, three or more (`s.2.dts`'s core 1.2% smaller).
  A lossless WAV's rule misses where the encoder's arithmetic moves the RMS across the boundary
  between two entries: by about the channel's level over the root of the values' count (the
  samples' errors, above, averaged), or by about 2^-12 of the RMS (`3-1.dts`'s encoder's, as if it
  took the root less exactly). So outside the VQ subbands the rule's nearness is the nearer of the
  RMS's distance from the boundary times that root against the level times the subband's error's
  scale (the indexes', less their mean, above), from 2^-25 of it (2^-24 before format 74:
  `s.2.dts`'s core 0.24% smaller), and against the RMS, in doublings (`s.2.dts`'s core 1.1%
  smaller, its first 3 MB 0.8%, 3 MB of its middle 0.7%, `3-1.dts`'s 0.1%); the VQ
  subbands' contexts want the nearness as it was. At the core's rate since format 87 the level
  is the one along the subband's FFT path (the indexes', above) at each sample, weighed by the
  value's power there, as the encoder's RMS errs by twice the value times the noise, and times
  2^(3/4) (`s.2.dts`'s core 0.16% smaller, subbands 15 to 30's scale factors; as it is 0.16% less,
  times 2^(1/2) or 2 0.03-0.04% less; its plain mean 0.02% bigger than the channel's level, and
  the top subband's, which goes by its own noise, the same). The measure is a context of its own too (to
  2^-10 of the level, the VQ subbands' as well), of the counters' one by the nearness, one of
  the APMs' and a mixer's, with scl tuned again on it (`s.2.dts`'s scale factors 2.2% smaller).
  Where the prediction ends a run, the run's value is a boundary too, and the nearer counts there
  (`s.2.dts`'s core 0.6% smaller); the nearness against the RMS alone stays the midpoint's, as
  which side of it the RMS lies says which way a miss goes, to the run's other end or past the
  midpoint (with the value counted there too, the misses cost more than the hits gained).
  `s.2.dts`'s encoder puts its fronts' and surrounds' top subband (31, a VQ one) 1 to 4 entries
  above the rule's a third and a quarter of the time (its centre's 5%), in stretches, and the
  likelier the louder the subband below (no fixed offset, leak from below or other rule makes
  them; many are the run's other end, above): the channel's class and the subband below's energy
  are contexts (its core 0.9% smaller).
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
  delayed) nor any filter of them taking it out, nor a filter of the input's 512 samples (fitted
  to what the encoder's vectors leave of the values, it makes no more of it than the bottom
  subband's values do: 0.77%). Its power is in the encoder's RMS, though, so the top subband's
  RMS rule adds 2^-40 of the bottom one's mean square (its scale factor right 81.6% of
  `s.2.dts`'s time, from 79.2%; its core 0.1% smaller), and its nearness against the level goes
  against that noise too, the root of the level's unit squared and 2^48 times its power (0.13%);
  and the HF VQ groups' scores don't learn from the vectors it swamps (below). That noise is the
  encoder's FFT's at its bin 0 (above: the top subband is that bin's other half), so it goes
  with the bottom subband's value sample by sample, and a value where the bottom subband is
  loud says less than one where it is quiet. So since format 83 (at the core's rate) the top
  subband's vectors' distances weigh each value's squared difference by T^2 / (T^2 + x0^2), x0
  the bottom subband's value at its sample, T 2^25.5 times a quarter of the code book's unit (an
  entry's step times the scale factor), and its groups' scores the same at 2^26, at a mean of 1:
  `s.2.dts`'s core 0.46% smaller (its top subband's vectors 0.28%, its groups 0.18%; 2^25 and
  2^24 0.03% smaller and 2.4% bigger, the boundary order by the weighed distances between the
  entries 0.07% bigger; the subbands below by their bins' other halves, 29 by 2, 30 by 1, 28 by 3,
  nothing), 3 MB of its middle 0.4%, the others within a byte; 1.3% slower on `s.2.dts`.
  Since format 84 the noise is in the boundary orders too. The encoder's values are these and the
  noise, so its choice flips from the first to another where the noise along the two entries'
  difference crosses their boundary: the distance between them is the noise's spread there, the
  root of their squared differences weighed by each value's noise power, 1 + x0^2 / T^2 (T 2^23
  times that quarter of the unit for the vectors, `s.2.dts`'s core 0.2% smaller; 2^23.5 for the
  groups, their W's distance to the power 1.5 as before, 0.3%; the vectors' keys of the distances
  unweighed, from their own first, 0.2% bigger). A missed group's values go onto its boundary at
  the point likeliest under the noise, each moved by its noise power times the gradient's (T 2^20,
  0.17%). With those, the distances' weights are at T 2^26.5 and the scores' at 2^27 (0.1%); the
  RMS rule adds 3/4 of 2^-40 of the bottom subband's power (0.03%); and the scale factor's
  nearness goes against that noise as the encoder's RMS errs by it, twice the value times the
  noise at each sample: the bottom subband's power at each sample weighed by the top one's,
  rather than its mean (0.05%). And each candidate's boundary key is the largest of its keys
  against the nearest few of those nearer than it, rather than against the first's alone (8 of
  them for the groups, 4 for the vectors), as where another lies between the two, the encoder's
  values cross that one's boundary first: 0.5% (the groups' 0.43%; 2 of them 0.28%, 32 as 8),
  in every subband of 16 values, the others' by their distances unweighed (0.03% of that). In all
  `s.2.dts`'s core 1.37% smaller (89641 bytes to 88414), 3 MB of its middle 0.7%, the short
  streams 50998 bytes to 50940 (three 1 byte bigger), the regression set 162745 to 161462;
  `s.2.dts` 7% slower to compress, 4% to decompress.
  Since format 85 that noise goes by the binade of the bottom subband's value: the square of
  2^(e - 1/2), where 2^(e - 1) <= |x0| < 2^e, rather than x0's own, in the boundary orders'
  noise (`s.2.dts`'s core 0.19% smaller), the distances' and scores' weights (0.02%), the RMS
  rule's noise and its nearness's (0.04%) and the margins' unit (0.03%). It is the noise of
  floating point arithmetic, its rounding errors in units of the last place of values the size
  of the bottom subband's, its steps at the powers of 2 of the analysis's units exactly (with
  them a quarter, a half or three quarters of a doubling off, 0.3-0.4% bigger than with none);
  but not the rounding of x0 itself as a 32-bit float (added to the top subband's values at
  any scale, bigger), and not the leak or format 82's bias, which go with x0 itself. A missed
  group's values go onto their boundary with the group whose key binds, the one its largest key
  is against (0.02%), and the scale factor's nearness has the noise's own square in its noise
  too, as the encoder's RMS errs by it (0.02%). In all `s.2.dts`'s core 0.33% smaller (88414
  bytes to 88118), 3 MB of its middle 0.1%, the others within a byte. Format 86 has vqr and scl
  tuned again on format 85's dumps: `s.2.dts`'s core 0.03% smaller, the others within 3 bytes.
  Since format 87 that binade is in the level along the FFT path too (the indexes' nearness's,
  above). The stages bin 0 shares with the other even bins sum its inputs into its value's
  eighths, quarters and halves, each rounded in units of its last place; the inputs themselves,
  the first stage's, are the twiddled values. So bin 0's energy in the sums of the stages it
  shares is the square of its bottom subband's value's binade (as hf_stair2() has it) and its top
  one's square, while the inputs' sum keeps the plain energy: `s.2.dts`'s core 0.13% smaller,
  its even bins' subbands' indexes (in the inputs' sum too, 0.1% less, as the odd bins' get
  bigger; each bin's own last stage by its own binades, the inputs' weight doubled or halved, or
  the stages weighed by their values' size, bigger). Subband 28, bin 14, shares the first of
  those stages, so its HF VQ boundary orders go by that noise too, at a quarter of the top
  subband's power (0.04%; at the top one's power or a sixteenth of it 0.02% less; the distances'
  and scores' weights nothing; subbands 29 and 30, odd bins, by the bottom subband's power
  nothing, and subband 28's RMS rule with its noise power in it bigger). And the scale factors'
  nearness goes by the path's level (above: 0.16%). Those three: `s.2.dts`'s core 0.43% smaller
  (88088 bytes to 87705), the short streams 5 bytes bigger, the others within a byte.
  The encoder's scale factor says where its values' RMS lay, and its group which group's scores
  its values made the least: each bounds the noise, which the model took at its likeliest point,
  on the region's edge or the boundary where the values lay past it, and as nothing within. Given
  such a bound the noise's mean is a truncated normal variable's. The RMS's square errs by D =
  2/16 x n (and n n / 16, whose mean the RMS rule has), a normal variable of variance 4/256 sum
  x_j^2 s_j, s_j the noise power at sample j (hf_stair2()'s at 1.5 2^-40, twice the RMS rule's);
  so given D in the region the noise at sample j has the mean 2/16 x_j s_j over that variance
  times D's mean there (det_trunc(): the normal's density and tails in IEEE's basic arithmetic,
  e^x by its series), and since format 87 the top subband's groups and vectors go by the values
  plus that mean, always (`s.2.dts`'s core 0.66% smaller; at the RMS rule's power 0.04% less, at
  4 times it 0.02%; where the RMS lies outside the region alone, as before, 0.07% less;
  subbands 28-30's the same, their noise 2^-44 of the power along their FFT path, 0.03%, but the
  short streams 0.06% bigger; subband 28's by the bottom subband's binade nothing; the indexes'
  noise given their scale factors' regions, where the region lies within 3 of its sigmas, 0.01%,
  further out bigger). A missed group's values go to the noise's mean given that the
  encoder's group won against the one its key binds to, along the scores' gradient, past the
  boundary (the values' noise sc 2^10 in the scores' units: 0.11%; 2^8.5 or 2^11.5 0.04-0.06%
  less), and where the group was the scores' first too, given that it won against the next one
  (0.06%). And along x the noise's variance left is D's given the region, so the boundary
  orders' spreads lose that much of their part along x (0.02%). In all, with scl and vqr tuned
  again on format 87's dumps, `s.2.dts`'s core 1.28% smaller (88088 bytes to 86957), 3 MB of its
  middle 0.8%, the short streams 50933 bytes to 50892, the others within 2 bytes; the regression
  set 161130 to 159952, the calibration set 170186 to 169010. Since format 88 subbands 28-30 go
  by that mean too (their noise 2^-44 of the power along their FFT path), where the switch, which
  the top subband's comparisons keep, takes the values moved, with the scores of the values taken
  alone (0.04%; by their own comparisons 0.01% less, and slower), and a group's push in the top
  subband goes on, given that the group won against the one the scores of the pushed values rank
  first of the rest too (0.05%; against two more, or the first again, less; the variance left
  along the push's gradient too, 0.01% bigger): `s.2.dts`'s core 86957 bytes to 86877, the others
  within 3 bytes, 3.6% slower to compress.
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
  is the prediction all the same. But at the 7-bit table's finer entries it lies 13 to 25 above
  (`dtswavsample16.wav`'s a quarter of the time), and a subband of one index among zeros has its
  RMS 12 dB under its peak, its scale factor 6 entries above the RMS rule's (11 at the 7-bit
  table's): so since format 146 the search tries past its window's top the entries about a VQ
  vector's least-squares scale too, and else the two either side of the one whose step puts the
  peak at the quantizer's top (`dtswavsample16.wav`'s scale factors 70 KB to 18 KB, the DTS CD's
  sample's 1.3 KB to 0.9).
  Since format 153 the vector the two-stage search finds at the subband's last scale factor is a
  candidate too, at its own least-squares scale: where the encoder's scale factor lies far above the
  RMS rule's, its vector is a quiet one, which the searches about the RMS rule's don't find where
  the shape search's is another (`dtswavsample16.wav`'s were the core's 35% of the time there, the
  last scale factor 8 or more entries above the search's in 98% of the far misses). And an unsure
  search's best (no entry's error a quarter of the next's) is a nearness of its own, the least sure,
  where it was with the least sure of the rest, right 98.8% of the time against its 35%:
  `dtswavsample16.wav`'s scale factors 17.7 KB to 13.2 KB, its core 10.5% smaller, `padded.dts`'s 16
  bytes, `ES 6.1 16bit.dts`'s 5, FATE's samples 1-2; the rest the same.
  FFmpeg's encoder takes, at its fine steps, the least scale factor whose quantizer range holds the
  subband's peak (`calc_one_scale()`), 3 to 14 entries under the RMS rule's: so since format 133,
  where after 256 tries more than an eighth of the scale factors have lain under its window, the
  search goes down to 16 under it: its streams' scale factors 76-99% the prediction from 33-71%
  (above; since format 135, where its bit allocations are made again, the first guess goes by its
  rule too, below). Every stream's so, the DTS encoders' decodes found finer lattices than theirs as
  near: `ES 6.1 16bit.dts`'s core 11% bigger.
  Since format 91 a lossless WAV's scale factors go by where the encoder's arithmetic may have put
  the RMS (`sf_post()`): the values with the FFT's noise in them, its power at each sample 2.4
  2^-40 of the bottom subband's binade's (`hf_stair2()`) in the top subband, else 2^-43 (the HF VQ
  subbands 28-30) or 2^-42 (the rest; since format 92 by their index error scales, below) of the
  square of the subband's FFT path's level (the indexes', above); their mean square taken as a
  normal variable of the mean and variance that noise makes, each entry's probability that of its
  cell (the run of equal entries' first taking up to the run's value, its last from it on). Where
  the likeliest's is under 0.999, the scale factor is coded by rank: "this one?" for each in turn,
  at its share of what those before it left, calibrated by how often that share has been right (by
  rank, the first, the second, the rest; in 12 buckets of its logit, 24 until format 107; since
  format 105 its odds times the bucket's hits made against those expected over its misses so,
  between its bucket's and the nearer neighbour's, where they had been mixed with the share, which
  left the bucket's rate as its counts grew, not the share's place in it: `s.2.dts`'s core 0.13%
  smaller, its scale factors 0.7%); past the sixth, as its difference from the likeliest; surer
  than that, by the RMS rule's hit flag as before. That noise is the analysis's where the WAV is
  the encoder's input to the bit, its indexes then the audio's 99.66-99.92% of the time (the
  lossless WAVs' streams at hand), so
  the posterior is taken only where the stream's indexes have been the
  audio's 99.5% of the time, after a 5.1 frame's 2560 of them; a clipped decode's are 79-83%
  (FATE's), another encoder's 97.5-99.2% (FFmpeg's 7-frame DTS-HD MA samples), and without that
  switch their cores were 1-7% bigger. `s.2.dts`'s scale factors 20768 bytes to 15841 (the top
  subband's 12188 to 9409, subbands 28-30's 4539 to 3275, the rest's 4042 to 3157), its core 5.7%
  smaller; the other lossless WAVs' 0.1-2.9%, FFmpeg's 7-frame DTS-HD MA samples' 0-2.3% bigger,
  FATE's `dts_es.dts`'s 0.3%, the rest as they were. Every scale factor this way, the short
  streams were 1.2-1.8% bigger (the counts start from nothing, and the rule's hit flag knows the
  sure ones); with the RMS rule's own noise for the top subband's (3/4 of 2^-40), `s.2.dts`'s
  first 3 MB and 3 MB of its middle 0.7% bigger; behind a switch by whether the likeliest or the
  RMS rule's guess had been the encoder's more often, `s.2.dts`'s core 2.3% bigger; with the
  noise's power adapted to each stream, 12 bytes less in the regression set. Since format 92 the
  ADPCM subbands' noise goes by their index error scales (above) too: 2^-44 of the level's square
  times 2^((s - m) / 8), s the subband's scale and m the mean of the subbands', as the scale says
  how far the WAV's analysis is from the encoder's there: `s.2.dts`'s core 0.19% smaller, the
  regression set 0.12%; by 2^((s - m) / 4), 0.15% bigger than that (at 2^-42); the HF VQ
  subbands by their FFT partner's scale (subband 31 - b), or the top subband by subband 0's, 0.14%
  and 0.25% bigger. Since format 94 the top subband's posterior is by sampling instead (`sf_mc()`):
  the values with that noise drawn 4096 times (since format 96; 256 before: `s.2.dts`'s core 0.07%
  smaller), as the HF VQ's are (below), each sample's mean square counted in its cell. Where the
  subband is weak against its noise, the mean square's distribution is skewed, where the normal one
  isn't: `s.2.dts`'s core 0.13% smaller, the short streams 6 bytes; subbands 28-30's and the rest's
  this way, at any power of their noise from half to 8 times theirs, 0.1-1.4% bigger with 256
  samples, the same with 4096.
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
  quiet: the encoders seen never predict one whose RMS over the subframe is under 100, at any gain
  (`s.2.dts`'s subbands that would gain predict 2.4% of the time from 99.5 to 100, 97% from 100 to
  100.5), nor an HF VQ subband (none of `s.2.dts`'s 64 that would gain, nor of the other lossless
  WAVs' 86), so such a subband's PMODE goes as a gain of none (`s.2.dts`'s core 1.6% smaller, its
  RMS rule's scale factor under 100 taken for quiet; by the RMS itself 0.05% more, by the VQ
  subbands 0.03%). Nor do they predict where they take a transient (none of `s.2.dts`'s 9353),
  which they do in a subband b below 20 where the energy rises from a subsubframe to the next by
  8 (19 + b) / 19 times or more, to 0.1% in each subband: of `s.2.dts`'s subbands below 20 that
  would gain, the 2441 that rise that much or more don't predict, and of the 216367 that rise less
  130 don't, at any distance from it down to a 64th of a doubling (FFmpeg's
  `master_audio_7.1_24bit.dts` the same). The rise in eighths of a doubling past that threshold is
  a context, tr (`s.2.dts`'s PMODEs 3.2 KB to 1.3, its core 1.9% smaller, at a threshold the ABITS
  moved, as formats 60 and 61 had it; at the subband's, 0.97 KB); and a subband that doesn't
  predict where its gain says it would, from a rise of 3 doublings, has its ABITS guessed by its
  values from the rise on (0.03%). A decoded WAV's values
  less the decoder's prediction are its dequantized indexes, so where the scale factor search's
  rule is taken, the first of the order's first 1024 whose prediction leaves values that fit a
  quantizer's lattice (by the bit allocation search, to a 64th of a step squared a value) is put
  first (its block, and it in its block): `open bitrate.dts`'s predictor was then the first 97.86%
  of the time, from 78% (96.68% by the first 16, which miss 2.4% of its predictors and 8.4% of `ES
  6.1 16bit.dts`'s: with 1024, their cores 4.6% and 3.1% smaller; all 4096 find more that fit by
  chance). But others fit nearly, where their predictions lie near the encoder's: so where the one
  that fits isn't within a 1024th of a step squared a value (as the encoder's nearly always is: of
  the 7648 of `ES 6.1 16bit.dts`'s first fits within it, none was another's), the rest of the 1024
  are tried on its lattice and the nearest taken, the first of equals (`open bitrate.dts`'s
  predictor then the first 99.82% of the time, `ES 6.1 16bit.dts`'s 99.38% from 90.60%: their cores
  16.2% and 21.3% smaller, FATE's `dts_es.dts` 3.6%). (They are tried in turn until one fits, the
  order's next taken from a heap; where none has fitted in a quarter of the tries, as in
  `96-24.dts`'s core, the first 32, a try there counting as none fitted where none of its first 4
  did: one coded against the last one, or by the values under X96's vectors, below, counts as a try
  none fitted.) A subband that doesn't predict has its values as they are on a lattice, so PMODE's
  context there is how they fit the one the bit allocation search finds in them (to a 64th of a step
  squared a value, to 16 64ths, worse): `open bitrate.dts`'s subbands that fit to a 64th don't
  predict 465734 times in 465747, those that fit to 16 64ths do 92% of the time (its PMODEs 1712
  bytes to 467). A transient's values fit two lattices, before it and from it on (the bit allocation
  search's, below), and no one: where the search takes such a fit, its distance is the context's
  (`ES 6.1 16bit.dts`'s core 4.5% smaller, `open bitrate.dts`'s 9.3%, FATE's `dts_es.dts`'s 2.1%).
  The search is the one ABITS would make, kept for it.
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
  scores' (`s.2.dts`'s core 0.3% smaller, the other streams the same). Nor from one whose peak is
  over 8 times the code book's (format 69): where the WAV isn't the encoder's input, as at a splice,
  a few such moved the scores so far that `s.2.dts`'s first 1.2 MB twice over had 18% of its
  second copy's groups wrong, from 6% (the pair 10315 bytes, now 9346). A score fitted to
  `s.2.dts`'s own vectors makes about 91% of
  them, no more: the rest isn't a function of the samples as the analysis has them. Where the
  encoder's search and the model's differ, it is nearly always a near tie, so how far the first
  beats the second is a context, by doublings from 1/256 of the first's distance (by the
  centroids: 40% of `s.2.dts`'s first 3 MB's groups within 1/256 were the first, 54% within 1/64,
  83% from 1/32 to 1/16, 99% from 1/8; from 1/16 before, its core 1.4% smaller). A lossless WAV's
  vectors miss by the encoder's arithmetic, though, so there how far the nearest beats the next goes
  against the channel's level times the scale factor (at the core's rate, since format 82, the level
  along the subband's FFT path, above: `s.2.dts`'s core 0.1% smaller), by doublings from 2^-23
  (`s.2.dts`'s encoder took subband 28's nearest 97% of the time within 2^-19 of that, 49% within
  2^-24; by the nearest's distance, 63% within 1/256 of it and 99% by 16 times it), while that has been wrong
  where sure (8 or more) no more than 2 times more often than the other: a decoded WAV's vectors
  are nearly its own, and miss at near ties by the nearest's distance (`s.2.dts`'s core 0.3%
  smaller, FATE's samples within 2 bytes). That margin, by doublings from 2^-25, is a context of
  its own too, the groups' as well (their scores run 2^19 times the vectors' distances: half of
  `s.2.dts`'s groups within 2^-4 of the unit were the first, 98% from twice it), of a counter, an
  APM and a mixer; and so is how many of the vectors lie within 2^-19 of the unit of the nearest
  (the groups' within 2^-20; both within 2^-18 before format 74: `s.2.dts`'s core 0.18% smaller)
  (0, 1, 2, 3-4, 5-8, more): where the encoder's arithmetic makes another its choice, which one;
  with vqr tuned again on them (`s.2.dts`'s side information 0.7% smaller). A rank that isn't the
  first is coded as its bucket (1; 2-3; 4-7; ...) in unary, then its bits below the bucket's top,
  and each bucket's flag past the first goes by how far the next bucket's first candidate lies
  from the best against this one's, in doublings: how many lie near goes by where the keys jump
  (`s.2.dts`'s core 0.7% smaller; the bits below a bucket's top by the same, 18 bytes more).
  Where the group isn't the scores' first, the encoder's values lay past the scores' boundary
  between the two, so the vectors go by the values moved onto it, along the scores' gradient, where
  that has ranked the encoder's vector higher clearly more often than the values as they are
  (`s.2.dts`'s top subband's vector the nearest 63.1% of the time there, from 62.2%: its core 0.1%
  smaller; FFmpeg's `master_audio_7.1_24bit.dts` and `3-1.dts` keep the values as they are); and
  the groups' near ties are counted too, in their unit's terms (0.06%). And where the RMS rule's
  region of the encoder's scale factor (with the top subband's leak's noise) doesn't hold the
  values' RMS, the encoder's values were louder or quieter than these, so the groups and the vectors
  go by these scaled into the region (a sixteenth of the way, just past its edge; a quarter
  before format 74, `s.2.dts`'s core 0.1% bigger), where that has ranked the encoder's group higher
  clearly more often than the values as they are (`s.2.dts`'s top subband's group the first 64.3% of
  the time there from 62.7%, its vector the nearest 59.8% from 58.3%: its core 0.5% smaller).
  Since format 87 a lossless WAV's top subband, at the core's rate, goes by the noise's mean given
  the region instead, always (below), and a missed group's values to its mean past the boundary.
  Where the first isn't the encoder's choice, the others go by how near their boundary with the
  first lies to the values, rather than by how near they lie: as the encoder's values are a little
  off these, its choice is one whose boundary they crossed, the nearer the likelier. So a rank past
  the first is in the order of the key's excess over the first's over the distance between the two
  entries (the boundary's distance from the values), or between the two groups' W to the power 1.5
  (as the scores have been learned, better than the boundary's 1: 0.3%; the centroids' to the
  power 1; X96's as well): `s.2.dts`'s core 2.3% smaller (its top subband's HF VQ 31.5 KB to
  29.5), 3 MB of its middle 1.7%, the other lossless WAVs 0.1-0.5%. (The ADPCM predictors' blocks
  and entries in that order make nothing.) The margin against the level then goes in its terms
  too, the second's boundary key against the unit (the groups' 2^-22 of it, the vectors' 2^-5, as
  their W, in 512ths of the vectors' units, run to the power 1.5: 0.5%), and in the top subband
  the unit has the leak's noise in it, the root of its square and that of 2^46 times the noise's
  RMS times the scale factor, a quarter of what the scale factor's nearness takes (0.2%). A
  decoded WAV's values are the vector's, scaled, whatever the encoder searched, so where the entry
  nearest of all lies within 1/64 of the values' energy, the groups go by their nearest entries
  instead (each at 16 times the nearest of all at most): 99.98% of `open bitrate.dts`'s groups
  first, where the centroids' order makes 94%. A lossless WAV's subbands keep the scores' order.
  Since format 93 a lossless WAV's groups and entries go by a Monte Carlo posterior too
  (`hf_mc()`): the encoder's choice made on samples of its noisy values (the noise about
  each value at 3 2^-40 of the square of the bottom subband's binade in the top subband, at 2^-41 of
  the square of the level along the subband's FFT path in subbands 28-30, 2^-40 before format 98:
  `s.2.dts`'s core 0.05% larger), those whose mean square
  the scale factor's region doesn't hold left out, each sample's group by the learned scores' rule
  and its entry the group's nearest. Each candidate's share of the samples gives each decision of
  its rank its probability, one more input to vqr's mixers (its stretch doubled). Alone, the
  posterior codes about as the model does (3 MB of `s.2.dts`'s middle: the top subband's groups 407
  bytes against the model's 420, its entries 457 against 459), but the two differ, and mixed at a
  fixed weight they code 6% less than either. As the mixers' input: `s.2.dts`'s core 1.41% smaller
  (81703 bytes to 80550), 3 MB of its middle 0.9%, its first 3 MB 0.2%. The samples are the normal
  distribution's 256 quantiles in an order of each value's own (the same everywhere, in IEEE's basic
  arithmetic), a pass of them in another pairing where few were kept, the sampling stopping once 32
  kept agree on both, or 1024 have been kept after a pass (at most 2048, in at most 32 passes, or 8
  where none is kept), each sample counting 1/80 of a sample to each candidate (since format 96:
  `s.2.dts`'s core 0.32% smaller; formats 94 and 95 kept 256, at most 512, in at most 8 passes, a
  1/20 each, 0.19% smaller than format 93's 128). Since format 96 the samples are made four at a
  time, the scores' least found in AVX2, and each entry's nearest by a key in 64 bits (each distance
  is 256 y y plus 2^21 sc (2^11 sc e - y v), e the entry's energy): the same choices in a third less
  time, so four times the samples for a tenth more time: `s.2.dts`'s core compresses in 78 seconds
  where format 95's took 71 and format 92's 38 (3 MB of its middle in 2.10 seconds, as format 95).
  Since format 99 how sure the posterior is, its likeliest candidate's share in 8 classes, selects
  the first of vqr's mixers' weights and one of its tables' rows (`s.2.dts`'s core 0.07% smaller,
  0.11% with vqr tuned again on it, the short streams 0.03%; whether the likeliest is the order's
  first too, or 5 or 12 classes, the same; in the other mixers' indexes, bigger). Uniform noise of
  the same variance made 3 MB of its middle 3 bytes bigger and a triangular one the same, and the
  groups' samples at 2/3 or 5/6 of the noise's power (the entries' at all of it) within a byte.
- **The LFE**: the WAV's LFE channel decimated by a filter near the decoder's 512-tap interpolation
  prototype, then quantized by the scale factor nearest its RMS. dtsdpack decimates by the prototype
  and corrects it with a 13-tap filter at the LFE's rate that a least squares fit learns as the
  stream goes (forgetting, ridged; solved by LDL' in a small deterministic soft float, each entry of
  the factors a dot product summed exactly at its largest term's scale, then rounded). Each value
  counts by 1 over its step squared (its error in steps is what the coder pays for), and the fit
  forgets 1/32 of the past each subframe (1/128 before, unweighted: `s.2.dts`'s first 3 MB 2.7%
  smaller, FFmpeg's `master_audio_7.1_24bit.dts` 1.8%, `s.2.dts` 0.1%, the XXCH samples 0.3%
  bigger). A decoded WAV's LFE is the decoder's interpolation of the LFE values, 8 of them in each
  of its samples, so a second fit has the last 7 values too, with which it sees the next alone; the
  one whose predictions have been the values more often lately is taken (`ES 6.1 16bit.dts` 10%
  smaller; some lossless WAVs' streams gain too, `Master Audio 7.1.dts` 3.5%). A third fit has the
  prototype's decimations by 32 around each value (those halfway between too, where the LFE's rate
  aliases the filter's band), 27 of them, and forgets 1/2048 of the past a subframe (refitted every
  16th): the encoders' filter doesn't change, so over a long stream that one learns it far better,
  while over a short one the first, learning faster, is often the better (`s.2.dts`'s LFE values
  99.3% their prediction from 96.7%, its core 2.6% smaller). And the DTS-HD MA encoders' filter is
  the same in every stream: a fourth, fixed, is the third's fit to all of `s.2.dts`'s values (99.8%
  of them; `lfefilter.inc`, below), taken from a stream's start where it is the better (`s.2.dts`'s
  LFE 3184 bytes to 223, its core 3.0% smaller in all; FFmpeg's `master_audio_7.1_24bit.dts`'s LFE
  262 bytes to 44, its core 5.9%, `s.2.dts`'s first 3 MB 3.0%; `Master Audio 7.1.dts`'s, which no
  linear filter of its WAV's makes, 0.4% bigger). FFmpeg's interpolation is the same in every decode
  too: a fifth, fixed, is that fit to `ES 6.1 16bit.dts`'s, FATE's `dts_es.dts`'s and `96-24.dts`'s
  values (`lfefilterd.inc`, below), the interpolation undone (in large weights that nearly cancel:
  ridged by 10^-10 of the mean diagonal, as more makes fewer of the values), which makes 90-99% of
  theirs (`dts_es.dts`'s 93% where fitted without it), and is taken where it is the better: from a
  decoded WAV's first values, which the fits haven't learned yet (FATE's 7-frame `dcadec-suite`
  streams with an LFE 18-21 bytes smaller each, their LFE 51-53 bytes to 31-33: their values all
  made but where the WAV is silent or ends). A lossless WAV's choices are the same as without it.
  But `Master Audio 7.1.dts`'s encoder decimates its LFE by another filter at each place of a
  frame's values (8 a frame: a fit at each makes 90% of those that aren't 0 within half a step, the
  one fit 10%). So a sixth is the third at each place apart (format 68; 16 of them, a longer frame's
  16th and later places sharing one), which once learned makes 93% of its values, the best of the
  others 30% (its LFE 1990 bytes to 502, the stream 22% smaller). They are refitted only where the
  first five missed more than a quarter of the values since the last refit: every time, they'd cost
  nearly as much as all the other fits (`s.2.dts`'s first 1.2 MB 1.6% slower to compress). A DTS:X
  encoder (the Cymatics stream's, above) rounds its LFE values up, to the integer at or above, not
  to the nearest: they lie half a step above every fit's prediction, which made 50% of its loud ones
  (the error otherwise about a quarter of a step, white; a silent LFE exactly 0). So each of the six
  has a second prediction half a step up (less a unit, so that 0 stays 0), taken where it has been
  the value more often (format 117: 81% of those values; the stream's core 2690 bytes smaller, `ES
  6.1 16bit.dts`'s 23). And the third leaves out a value further from its prediction than 4 times
  its error's running RMS (in steps, of the last 64 or so, 1 at the least), once it has had 64, so
  that the few it can't explain don't pull it from the rest; the RMS takes those in too, but at most
  100 times its mean square, so that where the fit goes wrong for long it learns again (the Cymatics
  stream's core 659 bytes smaller again, the others the same; the other fits too, `Master Audio
  7.1.dts` 13-22 bytes bigger). Of the twelve predictions (the six, and each half a step up) the one
  taken is the one that has been the value most often lately; since format 123, the one that has
  been the value most often among those whose best's prediction had the same sign and as many steps
  from 0 (under 4, 16, 32 or more; their counts halved every 32 values of the class), where that
  choice has lately made more than 4 more of the values than the best (the two counts halved every
  256). The Cymatics stream's fits err differently by sign and size (its best fit's values less its
  predictions, half a step of which is the encoder's rounding up, 0.2-0.8 steps by size where it
  predicted above 0, 0.6-1.0 below): its LFE values the prediction 88.6% of the time, from 85.2%,
  its core 2.6% smaller; the other streams' the same but `Master Audio 7.1.dts`'s, 89.3% from 89.2%.
  Since format 127 each of the twelve comes again less a fit of its error, a + b x / 64 steps for
  a prediction of x steps, a and b for each sign of x, learned by least mean squares (normalized,
  1/32 of the way at each value within 2 steps of it; 1/16 or 1/64, the Cymatics stream's core 169
  and 82 bytes bigger): its fits' values are a little more than 0.7% bigger than the predictions,
  those near 0 a sixth of a step nearer it, which by-class means (a bias for each class) caught
  only in part. Of the 24 the one taken is as before, those as they are first where the counts
  tie: where a prediction is the encoder's value but for its rounding, as `s.2.dts`'s fixed
  filter's, its error is where in the step the values lie, and less a fit of that they would miss
  (with the twelve corrected alone, `s.2.dts`'s core came 2.1% bigger). The Cymatics stream's LFE
  values are the prediction 93.8% of the time, its LFE 13.1 KB to 7.7, its core 7.9% smaller;
  `ES 6.1 16bit.dts` 6 bytes bigger, `Master Audio 7.1.dts` 3, the others the same within a byte.
  Since format 139 an LFE value's hit flag (the hit family's) goes by how many of the 24 make its
  index too, each weighed by how often it has been the value lately (the counts the choice goes by,
  plus 1), in 0-15 where a subband sample has its loudness nearness, nl, which an LFE value has no
  use for: of the Cymatics stream's values, those where all 24 agree (42% of them) are all the
  prediction, those where 8 or fewer do 72% of the time or less. Its core 254 bytes smaller (0.4%),
  `ES 6.1 16bit.dts`'s 28 (1.2%), the regression set's 23 bytes, the calibration set's 16,
  `s.2.dts`'s 3 bytes bigger; unweighed, the Cymatics stream's 7 bytes less. The count for the
  alternative too, for the flag after a miss, gained nothing (3 bytes bigger: it goes with how near
  the boundary the prediction lies, which that flag has), nor did means or medians of the
  predictions in the place of the one taken, which make the value less often (the best, the median
  of the three that have been the value most often, 93.69% of the Cymatics stream's values, the one
  taken 93.80%); nor the likeliest other value by the same weighed count as the alternative (where
  the two differ, the alternative as it is, the prediction's other neighbour, was the value 1886
  times, the count's 565), nor the last value's error fed back into the prediction (fewer values
  made at any weight), nor the choice by finer classes or faster counts (the best 93.75%). Since
  format 140, where the binding mixes two WAV channels with a gain before the mix (a DTS:X
  encoder's downmix, which takes level -8), the two fixed filters' predictions come at that
  downmix's gain, 23170/32768: the Cymatics stream's LFE runs at 0.69-0.70 of `LFE_W2`'s where it
  is loud, as its core's channels do at 0.707092 of the WAV's, and those two had been the value
  only in its silences (43% of the values). Its LFE 1.4% smaller (8272 bytes to 8155), its core
  0.19%; at 0.695, a little less. Its encoder's filter is not quite those, though: over frames
  7000-13000 that gain falls to 0.50-0.61. With hit and smp retuned at -8 on its dumps, 71 bytes
  more: its core 60907 bytes to 60720 (0.31%).
  The LFE scale factor comes after the samples in the bitstream; the model codes it with the first
  of them (the subframe's values as predicted, for the ones not yet coded), and the samples against
  the predictions at its step. That step is the 7-bit table's entry times 0.56 (the quantizer's
  0.035, times 16) exactly, as the encoder quantizes by it; FFmpeg's decoder rounds it to an
  integer, so a decoded WAV's LFE (where the scale factor search shows the WAV decoded) has its
  values times the rounded one. A quiet LFE's steps come to one unit or a few, which rounding puts
  up to 79% off (a lossless WAV's were rounded too before: FFmpeg's `master_audio_7.1_24bit.dts`'s
  LFE took 2248 bytes, now 262, its core 30% smaller; `s.2.dts`'s core 2.4%, `Master Audio
  7.1.dts`'s 1.0%).
  FFmpeg's decoder makes each LFE value's 64 samples from it and the 7 before (`lfe_fir_float_c()`,
  8 of `ff_dca_lfe_fir_64`'s coefficients each), in floats, the WAV having them floored to its
  resolution; so a value is the least squares solution of the 16 from it whose interpolations are
  the 1024 samples from its own, the 7 before it known: a fixed filter of those samples less a fixed
  combination of the 7 (`lfels.inc`; the WAV's rounding 3.7 times as strong in it). Since format
  135 that is the fifth prediction where the WAV is decoded at the core's rate (LFE_W2D, a fit of
  it, before): on FFmpeg's decode of its encode of `s.2.dts`'s first 3 MB, 5058 of its 5072 LFE
  values within 0.04 of a step, all but the last 14, whose samples pass the WAV's end (`ES 6.1
  16bit.dts`'s too where a step is 8 or more of the decoder's integer LFE units; its quiet
  stretches' steps, a few units or less, the integers blur). Before the scale factor, though, a
  value after the subframe's first has only predictions for some of the 7 before it, and the
  inversion makes their errors grow, 2 to 4 times from one value to the next (where the values' own
  errors are a fair part of a step, the eighth is off by as much as the values). So the scale
  factor's first guess is there the one of four that has been right most often: the RMS rule's,
  FFmpeg's encoder's rule on the predictions' peak (`calc_one_scale()` at ABITS 11, its LFE units
  128 times the decoder's), the least whose step puts that peak within 3/32 of 127, and the RMS
  rule's on LFE_W2D's predictions. The FFmpeg encodes' LFE 6.0 KB to 0.3 KB (their scale factors
  the second's 95.5-96.9% of the time); `ES 6.1 16bit.dts`'s 14 bytes bigger (its scale factors'
  guesses, by the RMS rule, a little less sure), FATE's `dts_es.dts`'s 12 smaller. But the
  inversion stays exact where the values before are the decoder's, and they are where the step is
  right: so since format 136 a fifth (`lfe_lattice()`) is the scale factor, from 8 below the first
  two's to 8 above, at which the subframe's values decoded as FFmpeg's decoder would have them
  there (each the inversion's from the WAV and the values so decoded before it, rounded to the
  step) lie nearest their lattice points, where they lie within 0.06 of a step at the root of their
  mean square, some of them not 0 (at a wrong step the values before go wrong, and the rest drift).
  It is so for 99.5% and 100% of the LFE scale factors of FFmpeg's encodes of `s.2.dts`'s first 3
  MB and of the noise, all right, and 87.7% of `ES 6.1 16bit.dts`'s, all right (the rest are where
  its LFE is quiet): the FFmpeg encodes' LFE 0.3 KB to 71 bytes, `ES 6.1 16bit.dts`'s 13 bytes
  smaller again, `dts_es.dts`'s 2.
- **The transients**: a decoded WAV's subband with a transient (TMODE) doesn't fit one scale
  factor's lattice. So, its ABITS known, the subsubframe from which two scale factors' lattices
  (each the best of 3 below its part's RMS rule's to 5 above, and of the two about its peak's) fit
  4 times better than one, is its TMODE's prediction, the ratio a context: `ES 6.1 16bit.dts` 9.9%
  smaller. Where none does and one fits within a 64th of a step squared a value, it has none.
  Until format 146 such a fit was none before the split was tried; but where a transient's first
  part is quiet, its scale factor far under the second's, its values lie near 0 on the second's
  lattice and fit it that well: the DTS CD's sample had 18905 such of its 53430 transients, its
  TMODEs 11.1 KB of its 16.8 (5.4 since; that alone made its core 31% smaller, the peak's scale
  factors in the parts' fits, which a subband of one index fits only at, 3.6% more). The split goes
  untried only where the fit is within 3 eps, as no split's can then be 4 times nearer, and where
  FFmpeg's encoder is made again (below), which takes no transients. And since format 147 the
  TMODE goes against the subband below's and the channel before's in the subframe too, each as
  none, 0, the search's or another: a transient is at the same subsubframe in the subbands and
  channels it is in, and where its two scale factors are the same its decode shows none (the DTS
  CD's sample's core 4.6% smaller). A lossless WAV's encoders
  make a transient only where a subsubframe's energy is 8 times the one before's at least
  (not once below, in `s.2.dts`'s 3.5 million), never in a subband that predicts and once in one
  from 20 up (the Cymatics stream's encoder 2721 times there), and the likelier the larger the rise
  and the ABITS (1% at ABITS 1, 81% from 13):
  those make a context, and the largest rise's subsubframe the first guess (`s.2.dts`'s TMODEs were
  8.8 KB, 7.1 of them its 9354 transients; its core 2.9% smaller, the 7-frame samples' 4-7%). And
  a lossless WAV's ABITS, which come before its TMODEs, said whether its encoder took a transient's
  mean square from the rise on (below): where the guesses at the two differed, which one the ABITS
  was made the class (`s.2.dts`'s TMODEs 2.1 KB to 1.5 KB, 1.4 KB with sid tuned for it). The
  encoders take one where the rise reaches 8 (19 + b) / 19 times, but for 9 of `s.2.dts`'s 2.27
  million subbands below 20 that don't predict, all within 2% of it: the rise in eighths of a
  doubling against that (PMODE's tr, above) is a context (its TMODEs 1.4 KB to 1.0 at a threshold
  the ABITS moved, its core 0.4% smaller; at the subband's, 0.34 KB), and whether it is reached
  makes the class (0.17 KB, its core 0.07% smaller). From subband 20 up, where the model had them
  take none, they take one where the rise reaches 2000 times (format 125): the Cymatics stream's
  encoder 2719 of its 2721 there so (one rose 287 times, the other 1996), and 88 more subbands
  rise that much; `s.2.dts`'s its one there, the only one that rises 1500 times. The rise against
  that is the context there too, the subband's ABITS go by its mean square from the rise on, as
  below, and the bit budget counts its second scale factor (below): the Cymatics stream's TMODEs
  1168 bytes to 217, its bit allocations 8.1 KB to 5.9 and its core 4.8% smaller.
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
  the subframes before, and for XCH's). Since format 125 a subband's bits there count, where the
  encoders take a transient, its second scale factor's and its TMODE's code's above a 0's: without
  them the level lay low where a subframe had more transients than the last ones had (in the
  Cymatics stream by 31 128ths of a doubling with 20-49 of them, 69 with 50 or more; now 0 and 7),
  `s.2.dts`'s core 1.6% smaller, `Master Audio 7.1.dts`'s 3.6%, the Cymatics stream's 7.9% (its
  values quantized at the two scale factors besides, the same). An ADPCM subband's staircase is the
  subband's, as the other's (its residual's energy meets the same thresholds: 0.8% smaller), and
  once the subframe's ABITS are all known, the staircases have each of them at its last level, not
  the level it came at (1.7% smaller). Its guess is the first guess; the second, the ABITS across
  the edge of its energies the energy lies nearer (a miss's 98% of the time; the last ABITS, the
  second guess before, 39%); then the difference. How near the energy lies to that edge, and on
  which side, is a context, and so are the ABITS seen at the energy and either side, the energy, its
  change, the neighbours' and the loudest subband's. The staircases guess 95.5% of `s.2.dts`'s ABITS
  (the rest are silent subbands', whose ABITS an encoder fills a frame's bits with by a pattern of
  its own: there the ABITS seen at the energy is the first guess) and 98.4% of those right: its bit
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
  the five lossless WAVs (`s.2.dts` and `master audio 7.1 24bit.dts`), as the others in the rest; in
  the Cymatics stream's, a DTS:X encoder's, 12, 10, 8, 6 and 4 steps lower (format 124, above). A
  subband with a transient goes by the mean square of its values from the transient on (where its
  second scale factor is): where the rise is 4 doublings or more, nearly always (`s.2.dts`'s 4184
  such subbands: by the subframe's mean square, the table right in 43% of them, by theirs in 94%),
  from 3 to 4 the more often the louder those values, as it seemed (where the rise is 3.9 doublings
  or more, those values' ABITS by the table 9 or less, an eighth of a doubling less for each ABITS
  above, to 3: 94% of the 15860 such right, 84% by the subframe's mean square alone): where the
  encoders take a transient, the rise 8 (19 + b) / 19 times or more in subband b (PMODE's, above),
  which climbs with the subband as the ABITS fall (its bit allocations 9.9 KB to 9.0 by that
  threshold). So the stream's offset is the one most of the last ABITS of all its bands say at their
  subframes' levels, in each of the three shapes, and the shape the one most of them say (each
  ABITS's score 1023/1024 of the next's; the scores start again where two in a row lie past them, as
  after the silence `Master Audio 7.1.dts` starts with, each band's own offset by its own ABITS
  taken until then; by those alone, as before format 77, `s.2.dts`'s bit allocations 9.0 KB where
  they are 6.9 and its core 2.2% bigger, the other lossless WAVs' 1.6-6.3%, as a band's own ABITS
  are few at a stream's start and say less wherever its energy moves little), and the core's level
  is where the table's ABITS would take the frame's bits (as the staircases' centre is; both within
  8 doublings of the last subframe's, or, since format 124, within 32 where the bits meet the
  frame's only past 8: the Cymatics stream's core 2.4% smaller, `Master Audio 5.0 96khz.dts`'s 1.0%;
  within 64, `Master Audio 7.1.dts`'s 1.4% smaller, but two of FATE's 7-frame XXCH samples' 1.3% and
  1.7% bigger), plus how far the last subframes' levels lay from that (an average, each subframe's
  weight 31/32 of the next one's), then the shift (within 96 128ths, 64 before format 78) at which
  the table makes the most of the subframe's ABITS so far, less a 16th of one for each 128th, as the
  staircases'. The table's guess is the first one but where it has missed the staircases' misses and
  128 more (of the last 1024 or so where both had one; before format 78, only where it had missed
  fewer, by 2, so that a stream's first ABITS went by the staircases: the short samples 0.35%
  bigger), and what the other one says is a context (ag); a transient's guess at the mean square not
  taken was the second guess where it differed, until the threshold was the encoders' (without it,
  `s.2.dts`'s core 0.15% smaller). That level is a whole 128th, and the encoders' lies between them:
  with the energies exact, a level for each subframe makes all but 0.033% of `s.2.dts`'s ABITS, the
  nearest 128th all but 0.21% (and steps of 1.5 dB, not quarter doublings, all but 0.63%). So the
  levels at which the table makes all of the subframe's ABITS so far are kept, from the energies in
  2^-18 of a log2 (`log2_bits()`, the mantissa squared bit by bit; in quarters of a 128th before
  format 95: 82% of the table's misses are within a 128th of an edge, and the quarters still put a
  third of those on the wrong side; so `s.2.dts`'s core 0.29% smaller, its bit allocations 6474
  bytes to 6239), and how its guess stands against them is a context (sx, format 78; since format 79
  by the share of them that makes another ABITS: under a tenth, half, nine tenths, more): where
  every one of them makes the guess, it misses 0.027% of the time (0.072% by the quarters); where
  some make another ABITS, 6-24%. The guess itself by those energies and the level clamped into the
  range, the short streams 0.2% smaller but `s.2.dts`'s core 0.05% bigger; moved toward the range by
  half a 128th at most, the same as it is. Since format 97 the share of them that makes the guess is
  the first decision's outside prediction too (one more input to abt's mixers: its miss's
  probability, kept within 1/64 of 0 and 1), and the share that makes the second guess, of those
  that don't make the first, the second decision's: the short streams 0.09% smaller, `s.2.dts`'s
  core 0.01% (the first alone, 0.05% and 0.01%). Since format 98 whatever the first guess is (a
  decoded WAV's search's, mostly) and whatever the second candidate (the last ABITS, mostly), the
  table's levels standing for the encoder's: the short streams 0.15% smaller again (`ES 6.1
  16bit.dts`'s core 1.3%, `96-24.dts`'s 0.13%). And since format 100 a lossless WAV's shares are
  those of a normal prior about the subframe's level as the table has it so far (the prior plus the
  shift that makes the most of its ABITS), at a deviation of a quarter of a 128th, with a hundredth
  of the shares as they were: the subframe's level lies within about half a 128th of it half the
  time, and on 3 MB of `s.2.dts`'s middle the first decision costs 82 bytes where the range is
  straddled rather than 524. The margin context already has much of that: `s.2.dts`'s core 0.09%
  smaller (its bit allocations 6225 bytes to 6156), the lossless WAVs' short streams 0.05% (at 0.35,
  0.5 or 0.7 of a 128th 0.08%, 0.05% and 0.03%; a decoded WAV's shares so, its streams 0.04-0.8%
  bigger). The table says 94.75% of `s.2.dts`'s ABITS, right in 99.51% of them (90.53% and 99.53%
  before format 78; by each band's own offset 56.51% and 99.33%, the staircases missing less; 90.96%
  and 99.29% before that, until the transients' threshold went by the subband): its bit allocations
  went from 26.5 KB to 15.9 KB by the table, to 10.5 KB by the transients' mean squares (10.0 KB
  with abt tuned for them, below; 9.0 by their threshold, 6.9 by the stream's offset, 6.5 by sx, 6.2
  by its finer levels), and its core 12.4% smaller in all by format 76 (the other lossless WAVs'
  13-17%), then 2.2% more by the stream's offset and 0.5% by format 78. A decoded WAV's values are
  the decoder's, on the lattice of the step its ABITS and scale factor make, so the bit allocation
  search tries each ABITS from 4 below the first guess to 4 above (3 before: `open bitrate.dts`'s
  core 2.8% smaller, FATE's `dts_es.dts`'s 0.5%, `ES 6.1 16bit.dts`'s 0.3%; 5 either side finds more
  that fit by chance; PMODE's fit is the same search's), with each scale factor from 3 below the RMS
  rule's to 5 above, and takes the pair whose lattice the values lie nearest, in its steps (the
  indexes nearest them in the quantizer's range, dequantized as the decoder does: a finer lattice
  fits as nearly, but in more of its steps, and a coarser one misses the values between its points),
  where an index it makes isn't 0; of pairs as near, the coarser lattice, then the scale factor
  nearer the RMS rule's (a 3-level quantizer's values lie on the 7-level one's lattice at the same
  scale factor too: `ES 6.1 16bit.dts`'s search right 99.23% of the time, from 97.90%, its core 2.1%
  smaller). And where no pair fits within 3 eps (a 64th of a step squared a value until format 147),
  a transient's (its TMODE comes after its ABITS): the ABITS whose lattice fits the subsubframes
  before some one and those from it on, each part at a scale factor of its own, where 4 times
  nearer at least (`ES 6.1 16bit.dts`'s search right 99.71%, its core 12.8% smaller still; FATE's
  `dts_es.dts`'s 9.2%, `open bitrate.dts`'s 10.0%). A transient's quiet part lies near 0 on the
  loud one's lattice, and one lattice fits its values to that 64th: so since format 147 the split
  is tried past it (the DTS CD's sample's search right in 97.8% of its ABITS from 90.6%, its bit
  allocations 2.2 KB to 0.4; `dtswavsample14.wav`'s, right too seldom before to be taken, 15.3 KB
  to 2.1, its PMODEs 1.3 KB to 0.5), and a split where the pair fits to the 64th yields to a coarse
  lattice's one scale factor as near (`dtswavsample16.wav`'s encoder takes no transients: its bit
  allocations 3.7% bigger without). But a subband's ABITS leaps from about its last where its signal
  does, to 1 where it turns quiet: so since format 146 (but where FFmpeg's encoder is made again,
  its ABITS its own) the coarse lattices under the range, ABITS 1 to 3, are searched by themselves,
  only their pairs as near as the range's best, and taken where 4 times nearer; and where as near,
  the one of the two that has been the ABITS more often lately (a 3-level quantizer's values, 0 and
  a step either side, lie as well on the lattices of more levels at a finer step, and which of them
  an encoder takes is its own: `dtswavsample16.wav`'s the coarse one, `ES 6.1 16bit.dts`'s the
  range's). Since format 148, where the range's pair is within 3 eps and no coarse lattice as near,
  the ABITS between those and the range are searched the same way, their ties going as the coarse
  ones' do (a subband whose signal falls takes the quantizer of fewer levels its values lie on,
  under the range as often as in it: 72% of `dtswavsample16.wav`'s misses, its core 14% smaller).
  Then where the pair isn't within 3 eps, the other ABITS are, only their pairs within it, and
  taken where 4 times nearer (`dtswavsample16.wav`'s bit allocations 98 KB to 15 KB, the search
  right in 98.4% of them from 90.3%; every ABITS in the first pass instead lost the range's guess,
  the DTS CD's bit allocations 2.6 KB to 4.3, where by the passes 2.2); since format 148 only while
  a quarter of the range's pairs at least have been within 3 eps lately (FATE's 7-frame samples',
  noise near full scale and clipped, 12%: the pass found none of theirs and cost them 4% of their
  time). Its ABITS is then the first guess, and its margin over the next pair, and whether it is the
  first guess without it, a context: `open bitrate.dts`'s ABITS were that first guess's 81% of the
  time (in its first 400 frames), and are the search's 99.82% (`ES 6.1 16bit.dts`'s 99.71%;
  `96-24.dts`'s core's 85.15%, its values having X96's residuals in them). What the staircase or the
  table guesses is a context of the search's guess: where they agree, the search is right 99.99% of
  the time (`ES 6.1 16bit.dts`'s), where not 80%, and its misses are mostly what the staircase says,
  or the last ABITS where it says that (`open bitrate.dts`'s core 1.1% smaller, `dts_es.dts`'s 0.6%;
  the staircase's as the second guess, in the last ABITS's place, makes `dts_es.dts`'s 5% smaller
  but `ES 6.1 16bit.dts`'s 2.6% bigger). The pairs go by their distance at the two largest values,
  and each one's whole distance is summed, the largest values first, only while it could still come
  under the second best's: the same pair as trying them all, at a fraction of the cost. Since format
  150, where the search's ABITS and the staircase's (or the table's) differ, the staircase's is the
  second candidate where it has been right more often than the last ABITS where the search's missed,
  by 8 and an 8th; and where they lie 2 ABITS or more apart and the staircase's has been right more
  often there at the search's sureness, by 8 and a 16th, the staircase's is the first guess and the
  search's the second: at steps under the WAV's LSB finer lattices fit as well, and the search,
  unsure, takes the coarsest (`padded.dts`'s right 0.3-2% of the time there, the staircase's 80-93%;
  `dtswavsample14.wav`'s 3% and 94%). Its ABITS being the first guess or the second wherever it
  runs, it runs wherever the scale factor search's rule holds too (`padded.dts`'s ran on 36% of its
  ABITS, not clearly more often right than the first guess it is counted against). `padded.dts`'s
  bit allocations 55 KB to 35, its core 12.4% smaller; `dtswavsample14.wav`'s 2.3%, `ES 6.1
  16bit.dts`'s 5.8%, `open bitrate.dts`'s 4.6%, `96-24.dts`'s 0.3%; `dtswavsample16.wav`'s 0.3%
  bigger (its table's guess, the one it takes, right less often than the last ABITS, and the counts
  lag where that turns).
- **FFmpeg's encoder** (since format 135, `ffenc.inc`): where the core's code books have been its
  rule's (below: its encodes') and the frame is as its are (one subframe of 16 blocks, at 32, 44.1
  or 48 kHz, to 6 channels, each bound to a WAV channel as it is, at the core's rate), its bit
  allocations are made again from the WAV: its subband transform of the 512 samples from where the
  core's first block's analysis window ends (the 512 before them its history), each subband's peak,
  its psychoacoustic masking and its search for the noise level whose ABITS the frame's bits allow
  (`assign_bits()`), in its 32-bit arithmetic, its tables computed as its init computes them
  (`ffenctab.inc`, embedded, so that they are the same on every machine), but for the masking's
  MDCT, here in doubles through a 128-point FFT (FFmpeg's rounds at each step: a coefficient's
  centibels the same but where they cross a boundary). Since format 137 the same integers come
  faster: the transform 8 at a time, the masking's sums over the bins that can change them (a bin
  whose spreading entry is -1400 centibels or under adds nothing to a sum that starts at -1114 and
  never falls), its 25 bands side by side, a centibel value searched for among its binade's 60
  entries alone, and each subband's bits at each ABITS once a frame, not at each noise level the
  search tries (FFmpeg's encodes' cores compressed 17-20% faster). Re-encoding FFmpeg's decode of
  its encode of `s.2.dts`'s first 3 MB, FFmpeg's encoder makes 99.99% of the ABITS this makes from
  that decode (all of them in 625 of its 633 frames), and of the stream's own, both 97.2%: the
  decode isn't the encoder's input. Each ABITS is then first a flag, whether it is the one made
  again (not a family's: counts in its contexts), by whether that is 8 or more, how near its SNR
  lies its range's nearer end (11 buckets, 0 to 32 centibels and more) and which end that is, and
  the lattice the decode's values fit: the decoded peak is an index times the step, under the
  encoder's (by 30 centibels at ABITS 1, 14 at 2, 8 at 3), so the ABITS made from it are often one
  under the core's; and at ABITS 1-6 the peak's index is the quantizer's top level (the least scale
  factor that keeps the encoder's peak in range puts it there), so the values are multiples of the
  peak over that level: the least ABITS from 1 to 8 whose lattice they fit within 0.08 of a step at
  the root of their mean square (`FfEnc::lattice()`: none, the one made again, one above, one below)
  tells them apart (the ABITS 39% smaller, `open bitrate.dts`'s encode's 19%). Where it isn't the
  one made again, a second flag says whether it is the lattice's, else the one across that nearer
  end, and else the family codes it as before: the FFmpeg encodes' ABITS the one made again
  97.1-98.7% of the time, `s.2.dts`'s first 3 MB's bit allocations 8.1 KB to 0.9, the noise's 56.6
  KB to 3.2 (`-s`: "FFmpeg's encoder's bit allocations: 99.84% of the ABITS, right in 97.14% of
  them"). Its scale factors (`calc_one_scale()`) are the least whose step keeps the subband's peak,
  as the encoder had it, in the quantizer's range; the decode's peak is under it, so that rule on
  the decode's peak is the core's in 74.0% of `s.2.dts`'s first 3 MB's scale factors, the least
  scale factor whose step puts that peak within 3/32 of the quantizer's top level
  (`FfEnc::scale_within()`; under 4/32, as the table's entries are 1.124 times the last, and at
  ABITS 1 the peak is the step) in 95.9%. So where the ABITS are made again, a scale factor's first
  guess is the one of four that has been the scale factor most often at its ABITS (to 12): the
  model's prediction (right in 76.5%), that rule's, and the least within 3/32 or 8/32 of the top
  level; the four so chosen, 96.7% (that 3 MB's scale factors 9.7 KB to 2.6). Which one is right
  goes by the size too: where the steps are under the WAV's unit the rule and the two within,
  elsewhere the model's prediction more often. So since format 137 the one most often at its ABITS,
  that rule's scale factor (in fours, to 40) and the model's prediction's difference from it (to 2):
  the five FFmpeg encodes' first guesses wrong 1545 times in their 1301664 scale factors, from 8263
  (by the ABITS and that rule's scale factor in twos, 4578; by the difference to 1 or 3, 2572 and
  1639), their scale factors 8.4 KB to 1.8, that 3 MB's 2.6 KB to 0.9. Since format 138 the stream's
  first frame goes by FFmpeg's encoder too where its code books are that rule's (a flag at its
  start; the frames after go by the frames before, `core_books()`): the five FFmpeg encodes 28360
  bytes to 28024. And against the WAV that was the encoder's input, each index is first the one the
  encoder's `quantize_value()` makes of the value its transform makes from that WAV, at the core's
  scale factor and ABITS: the core's every time there. Where it has been the core's 99.5% of the
  time or more (from 256 indexes on) and more often than the model's prediction where the two
  differ, it is taken, sure (as a twin's value is: its miss costs more), and the scale factors'
  first guesses are the rule's; where only more often, it is the prediction, with the index next to
  it toward the model's as the second guess. FFmpeg's encodes' own decodes take it nowhere at 24
  bits, at 16 bits here and there (above). Its LFE values (`lfe_downsample()`, 8 a frame from 64
  times as many samples through its 512-tap filter, `FE_LFE_FIR`, made as its init makes them) are
  made again from each of the first two WAV channels that no core channel takes, with the history
  FFmpeg 6.1's encoder has, its input's last channel's a sample late (`shift_history()` fills the
  LFE's 512 from the slot before each sample's first channel's, the input's last of the sample
  before; the first, before the frame's buffer, as 0), and with the channel's own; where one's
  quantization at the core's scale factor has been the core's values 99.5% of the time (from 8 on;
  the tally at the scale factor, after the values), the scale factor is its rule's on their peak and
  the values its, sure: that 3 MB's encode's LFE 5.5 KB to 10 bytes, its indexes 19.9 KB to 7 bytes
  and the whole 25838 bytes to 177.

A WAV at twice the core's rate (96 kHz for a 48 kHz core) is analysed by the 64-band filterbank
instead: the same transform of 64 bands, by FFmpeg's 1024-tap X96 prototype, each block 64 of the
WAV's samples. Its lower 32 bands are the core's subband samples, as a DTS 96/24 stream's and DTS-HD
MA 96 kHz streams' cores are (99.3-99.8% of the indexes; the spec's encoder decimates the audio
first, and `96-24.dts`'s top two subbands look so, below, but its others don't). The DTS-HD MA
encoders' 64-band analysis is 1 - 1.4e-4 times this one, though: at FFmpeg's prototype's gain
`Master Audio 5.0 96khz.dts`'s bottom four subbands' values round to the encoder's index 98.34% of
the time, and at that one's 99.65% (99.73% by the 32-band analysis in `s.2.dts`'s first 3 MB, whose
gain is the encoder's to 5e-6), the fine steps' indexes the most: a lossless WAV without X96 is
analysed so (its core 38.9% smaller, the 96 kHz `xll` sample's 1.8%; a decoded WAV's analysis keeps
the decoder's gain). X96's subbands are what TS 102 114's encoder makes of it: the upper 32 bands,
and from X96's first subband up the lower ones less the core's decoded samples (the residuals). On
them, X96's elements as the core's, with histories of their own: 99.94% of `96-24.dts`'s indexes
(99.97% before an index far past the WAV's end went without the audio, below). Where X96 codes the
core's subbands too (from its first subband up, `96-24.dts`'s 27-31), the decode's value is the
core's plus X96's, each on its own lattice, and the index nearest it is the core's only 64-85% of
the time there: X96's step is 0.3-0.7 of the core's in `96-24.dts`'s subbands 27-29, and 0.9-2.1 and
35-85 times it in 30 and 31, where the core's values are 0.4 and 0.02 of the decode's (as if its
encoder's decimation to 48 kHz had taken the rest, which X96 codes). So at the subframe's first
sample, where the subband's values aren't all on the core's lattice (within half a unit), the model
looks for X96's step: of those whose ratio to the core's step X96's has had in the subband before
(to a 16th of an octave, or beside), the one that leaves each value's rest on its lattice after some
core index (within 2 of the gain's, the core's values' regression on the decode's, or the nearest),
with the least sum of X96's index magnitudes, then of the core indexes' distances from the gain's
(X96's step plus the core's makes the same values, the core's index off by X96's), the subframe
decoded with them and their ADPCM predictions (the largest step of equals); where none fits, X96's
HF VQ vector of each 16 samples that leaves the rest on the core's lattice, at the scale factors
X96's vectors have had there, by the first sample's entry values first. The core's index that leaves
the rest on the fit's lattice is then the audio's, where it has been the core's clearly more often
than the nearest: `96-24.dts`'s subbands 27-31's indexes 99.4%, 99.5%, 99.8%, 97.8% and 99.4% the
audio's, its core 32% smaller (26% before the distances counted, when a quarter of subband 30's fits
took X96's step plus the core's), at about the same speed. A subband's scale factor there, where it
has one in the subframe, is the first whose step the fit finds (X96's vectors tried only where it
had them the last time) of the prediction and the 2 either side of it, by their distance from it, or
where the gain is under 0.6, the decode saying little of the scale factor, of the last one and the 8
either side of it; where that has been right more often than the prediction, by a margin
(`96-24.dts`'s scale factors 98.1% the audio's, from 89.5%; its core 10.5% smaller, 5.8% of that the
last ones', 26% slower). Where the gain is under 0.6, the decode says little of its scale factors
and ADPCM predictors there: they go against their last values where those have been right more often
than the audio's prediction (by a margin), its core 2.3% smaller. But a decoded WAV's says the
core's values there before its ABITS and scale factors say X96's step: where X96's last ABITS there
was a quantizer's (or a vector's: its 3 levels then), they are the decode less X96's values, on its
lattice at its last scale factor or within 3 of it (the coarsest that leaves each within an eighth
of its step), and the predictor goes by their own LPC, the first of its 1024 nearest whose
prediction (the decoder's) leaves each of them exactly on the lattice of the core's last ABITS at a
scale factor within 12 of its last put first: `96-24.dts`'s subband 31's predictors 99.4% the first
where X96 has its lattice there, its core 10.2% smaller, 1% slower. Those values say the subband's
PMODE too (whether they lie on a lattice of the core's last ABITS as they are, as the context a
decoded WAV's values give it; 0.3% smaller) and, its ABITS and predictor coded, its scale factor
(the first whose lattice they lie on less the prediction: subband 31's 3355 times, all right; 0.2%).
Where X96's last there was its vectors instead, they are the decode less a vector of the code book
at a scale factor within 2 of X96's last (as FFmpeg makes them) that leaves each within 4 of the
core's last step of 0, up to 4 such, the nearer first (subband 31's: 1 mostly, X96's own vector 88%
of the time): the predictor the first of its 1024 nearest that one of them fits, put first (its core
3.6% smaller), the PMODE and the scale factor as above by any of them (0.4% and 0.6%). And where few
of the predictors' first 4 fit (a 96/24 stream's core's), the search tries 32 of them, not 4 (1.0%
smaller, 3% slower). Elsewhere there (`96-24.dts`'s subband 30: the core's input was the 96 kHz audio
decimated, which left it 0.4 of the decode's values, and those shaped), the core's values are a
least squares fit of its decoded ones on the decode's of the subband, 3 either side (the last
subframe's last 3 before it), learned as the stream goes: within half a step of the core's 83.5% of
the time, the decode 62.5%. By the fit's LPC the predictor's median rank is 7, by the decode's 1147
(6.5 bits a predictor from 11.1; its core 1.4% smaller); the scale factor search goes in order from
the fit's RMS rule's, as a 3-level quantizer's values fit a few scale factors (88.8% right from 75.7%;
1.4%); PMODE by the fit's gains (0.5%); and an index against the fit's, where they differ and that
has been the core's more than 3/4 of the time, as the second guess (1.1%).
Its decoded WAV's joint subbands are their source's times the joint scale factor, so where the
source has a level the scale factor is the projection of the one on the other (100% of those); a
subband without bits is the decoder's noise, uniform to half the scale factor, so its scale factor
is 2.1 times the peak (94%); FFmpeg's noise is its generator's draws times the scale factor
(rand_x96(): a linear congruential generator from 1 at the decoder's start, each subframe's subbands
without bits drawing their samples in turn, where the scale factor is above 1), so where FFmpeg is
the decoder the scale factor is the entry whose draws lie nearest the values, taken where that has
been right more often, by a margin (`96-24.dts`'s 99.7% of those; its core 3.7% smaller), and of
entries the same (1 and 2, 3 and 4) the first, as the encoders take them (X96's scale factors 99.4%
the audio's, from 94.5%; 1.5% smaller). X96's VQ vectors are searched as the core's: the groups by
their nearest entries where the nearest lies within 1/64 of the values' energy (`96-24.dts`'s groups
95.0% the first, from 86.4%; its core 1.4% smaller), a predicting subband's values less the open
loop's prediction, which the decoder adds to the vector's (98.2%, from 95.0%; 1.4% smaller), and in
the scale factor search the entry most nearly a multiple of the values too, to 12 entries above the
RMS's, the nearest kept where it isn't sure (its core 3.3% smaller). And X96's ABITS go against its
own bit allocation search's, a decoded WAV's: FFmpeg's noise where its draws fit (the generator's
state as the ABITS before say), else the coarsest lattice of a quantizer that all the values lie on
(within half a unit; its step 1.5 units or more, as a finer one fits nearly anything), else VQ
vectors, each 16 values the shape entry's at some scale factor; taken where it has been right more
often than the first guess without it, by a margin (`96-24.dts`'s: 54% of X96's ABITS, right in
99.99% of them; X96's ABITS 1388 bytes to 62, its core 3.2% smaller). A decoded WAV's X96 ADPCM
predictor is the first of the 1024 nearest its LPC whose prediction, the decoder's from the decode's
own last 4 values (the model's history of the decoder's isn't it after a subband of FFmpeg's noise),
leaves each of the decode's values exactly a vector of the code book at a scale factor within 6 of
X96's last there, or on the lattice of its last ABITS at one, put first (`96-24.dts`'s X96
predictors 1078 bytes to 483, its core 1.6% smaller). Where several fit (X96's values a few small
ones), the first 32 that do go first, in turn; and where none does and its last scale factor was
within 6 of 0, those whose prediction leaves a vector at scale factor 0 (each of the entry's values
over 16, rounded), which the table leaves out: X96's predictors 487 bytes to 233, its core 0.9%
smaller, 1.3% slower. Where the decode is, as it is, such a vector
or on such a lattice, X96's PMODE goes as the core's whose values fit one (0.2%). X96's code books
(each channel's for its scale factors, its bit allocations and its quantization indexes, in its
header; and each subframe's for its joint scale factors) are, in every one of `96-24.dts`'s frames
but one, those that code the values in the fewest bits: so the model codes X96's subframes first,
each book the first of its kind (whose values are all the books'), and the books after them, each
against the one that codes the values the cheapest (the joint scale factors' after them, as any
book's values; the walk's place counted again with the books): `96-24.dts`'s core 5.6% smaller,
FATE's X96 samples' 1.4% and 2.6%. The core's own code books for its indexes (each channel's for
each ABITS, in its coding header) are the DTS encoders' the same for the whole stream, the codes
of fixed length; FFmpeg's encoder takes each frame's for its values (`set_best_code()`: of the
ABITS's Huffman books the first whose codes take the fewest bits, where those and its ADJ's 2 bits
take fewer than the codes of fixed length; for an ABITS no subband has, those), and every one of
its frames' books is that rule's. So since format 134, once a frame's books have been the rule's,
some of them Huffman, the model walks the core's subframes with each channel's last books, their
values in any of an ABITS's books' range (the bit allocations' budget counting the bits those
books take), then codes the books, each against the rule's for the values, and their ADJs (the
walk's place counted again with them), until a frame's aren't the rule's; a stream whose frames
have no Huffman books doesn't take that rule (the DTS encoders': it would have Huffman books more
often than not): the FFmpeg encodes' headers 4055-8076 bytes to 30-170, `s.2.dts`'s first 3 MB's
core 20% smaller, the noise's 40%. Since format 135 the bit allocations' books (BHUFF) come after
the subframes there too, the ABITS walked in any of their books' range, each book against the one
FFmpeg's encoder takes for the frame's ABITS (`set_best_abits_code()`: of the Huffman books the
first whose codes take the fewest bits, where fewer than 5 bits each, else 5 bits; 5 bits where one
is 0 or over 12): the noise's 1839 bytes to 2. Each kind of subband's rule, the RMS rule and the
scale factor search (for X96's subbands with bits or VQ vectors: their vectors 16 samples) are
counted as the stream goes, and the one right more often is the prediction: a lossless WAV makes the
RMS rule the better. The core's scale factors get the same choice, among the RMS rule, the search,
and the first entry above 0.99 times the RMS (`96-24.dts`'s core's, 67% where the RMS rule's is 49%;
the search's, 89%). The searches cost: each is tried in its first 2048 values, then wherever its
rule is the one taken (the core's bit allocation search's in its first 512 values, then while it has
been right clearly more often than the first guess without it: a lossless WAV's, which rarely finds
one, kept it on as often, for nothing), and otherwise in one value in 64, so a lossless WAV's stream
loses no time to them. Since format 149 their rules are counted where the search was tried and found
one, and the core's scale factor search against a rule only where the two differ: the search's count
grew where it was tried and the rules' everywhere, so a search off past its first 2048 values, tried
in one in 64, never came on again; and where it found none (no scale factor 4 times nearer than the
next) it counted as the rule. `padded.dts`'s search (above), unsure in the quiet of its first
frames, was counted there as the RMS rule against the first entry above, which won; now it is taken
from the stream's 47th frame on, from never: its scale factors 97.2% the audio's from 69.5%, its
core 418846 bytes to 167352 (counted where tried, 186653; where the two differ but its none as the
rule, 181522).

c finds how the core lines up with the WAV before it starts (`call.inc`'s `find_alignment()`, in
doubles; the header keeps the result). The offset: from the core's first 4 MB, the 64 blocks with
the most energy, decoded to their plain subband samples, against the WAV's analysis for each of its
channels at the decoders' usual delays (0, 256, 512 and 1024 samples, times 2 for the 64-band
analysis): one that correlates with the core's channel 0 at 0.999 or more is the offset; one at 0.99
or more is where the best of the offsets within 32 of it is (the 3 best by the 16 blocks with the
most energy in channel 0, then by all of them), where that lies inside them. Else at every offset of
-1024..1536 (times 2), by those 16 blocks, then the 64 best of those by all the blocks: the best for
channel 0 gives it. (A WAV at neither the core's rate nor twice it isn't searched: nothing lines
up.) Where the WAV ends within those blocks' windows, as a cut stream's does, a block whose window
passes its end is left out at that offset, and an offset that leaves out more than half of the
blocks it is judged by is none: the encoder had audio there that the WAV hasn't, and at an offset of
512, the Cymatics stream's encoder's and FATE's 7-frame samples', the last frame's blocks reach up
to 480 samples past it (eight frames cut from the middle of the Cymatics stream lined up at 349, by
0.970, their last frame's blocks putting 512 at 0.967; now at 512, by 0.997). The LFE's samples the
same. Where nothing lines up there and the WAV can be read again from its start (a file, not a
pipe), the same from the stream's first block within 30 dB of its loudest, in the core's first 4 MB
or, where the input can be read on and back, its first 68 MB; then the WAV is read from its start
again. A stream that starts quiet for longer than the WAV's window (2^20 samples: 21.8 s at 48 kHz,
10.9 s for a 96 or 192 kHz WAV) has nothing to line up in it: the Cymatics stream's first 3200
frames with 22.4 s more silence at its start, 24.6 s in all, came out at 42.8% of the core without
the WAV, now at 0.21%; with 64 s more, 0.12% (its whole 18.5 MB read first). Then each core channel,
XCH's and XXCH's too (`find_mixes()`): a core may be a downmix of the WAV, as a DTS-HD MA 7.1
stream's is (its lossless extension's primary channel set is the 5.1 the core was coded from, each
surround the side and the rear surround, every channel scaled: coefficients of DTS's downmix table,
FFmpeg's `ff_dca_dmixtable`, Q15). So over up to 256 of the frames read first, a weighted least
squares fit (each decoded subband sample by 1 / its uncertainty squared: its quantization's, step^2
/ 12, and the WAV's own, half a unit, 128 for a 16-bit WAV: the finer, the surer) of the core
channel against every WAV channel and every pair of them gives its best single channel and its best
pair, each with its gains as fitted and as the downmix table's entries either side (signed), and the
best single channel as it is; with the channel the alignment found as it is and by the table's gains
of an unweighted fit, the candidates (the best channel as it is for XCH's channel, which the
alignment doesn't place: FATE's `xbr_xch_61_24_48_3840`'s, its WAV channel as it is, was bound to it
at a gain of 0.065 as fitted; as it is, its core 14% smaller). Of those, the one that makes the most
of the channel's indexes exactly is its binding (a simpler one unless another makes 0.5% more; not
one whose gains are all under 1/64), if it makes a fifth of them. A channel the alignment didn't
place, fewer than 64 of whose samples there aren't 0, isn't bound there (its fit, all 0s where
FFmpeg's encoder gives a silent subband bits, took a gain of 0, which predicts nothing); where the
WAV can be read again, such channels are bound the same way by the frames from the first block where
one of them is within 30 dB of its loudest, as above: a minute of 5.1 shaped noise whose surrounds
start at 30 s, FFmpeg's encode against its decode, came out at 19.81% of its core, its surrounds
bound at a gain of 0, now at 2.13%. At the core's rate the choice is then tried with each gain as
the table's entry nearest it and each WAV channel's samples times 1, 1 - 2^-15 or 1 + 2^-15 before
the mix (a Q15 gain, each product rounded to the nearest integer, a half up; a pair's either or
both), in a third pass: of those that make no fewer of the indexes than the choice, the first in
that order that the one making most doesn't clearly beat, by the samples one of the two makes and
the other doesn't (by more than 8 and 3 sigmas of a fair coin's; not by the misses' count, as misses
of other kinds dilute those of the gain). The Cymatics stream's encoder put one of each surround's
two channels into it that way, times 32767/32768, then at the table's gain. As one gain, the
product, which is how format 121 had it, the fits miss it by about their own noise (0.7070723 and
0.7070695 for the two surrounds, the product 0.7070707, the table's 0.7070923), and two surrounds
bound by their fits differ, so that where they are the same one isn't the other's twin: as the
product, its core 157545 bytes, from 179247. But a sample under 2^14 comes out of the gain before
the mix as it was, which the product doesn't say: so format 122's binding has those gains, applied
to a lossless WAV's PCM before it is mixed (and to the passes' analyses as the product), and that
stream's core is 92266 bytes. Where the frames the passes take are quiet in those channels, the two
make the same indexes and 1 is the choice, though the stream may be louder further on: so where
the choice's gains before the mix are 1 -+ 2^-15, 1 is a second pair of them, and where they are
1, the variant's that makes most of the indexes, where that makes 8 more than the choice; the
model takes either pair for a subframe by which has made more of the channel's decoded indexes
lately (`gain_pick()`: the difference, a quarter of it gone each subframe, past 1; the other pair's
analysis made for that). The Cymatics stream's frames 3000-5999, those channels quiet in their first
1352, came out at 21449 bytes in format 121; now 10611, the second pair taken from their 22nd
frame. Its first 3200 frames with 22.4 s of silence before them, 22572 then, are 10629; its frames
8000-13999 and 20000-25999, 29384 and 27520 then, 19932 and 19159. A minute of 5.1 noise encoded
by FFmpeg's encoder against its decode, whose channels FFmpeg scales by about 0.9999, has one of
them now at 32767/32768 (225914 bytes to 225266; 109620 since format 133, 65358 since 134, above);
the other streams the same. (At twice the rate, a lossless WAV's 64-band analysis being 1 - 1.4e-4
times the passes', 1 - 2^-15 looked better there and wasn't: not tried.) (A clipped decode's
distortion swamps a channel's quiet subbands at fine steps: their fit's gains came out at 0.00007
for channels that are their WAV channels as they are, and as most of those channels' indexes are 0,
such a gain made more of them exactly than the right one: FATE's 5.1 and 6.1 samples' cores came out
36% and 18% bigger, the XXCH ones' 19% and 14%.) (Against a decoded WAV, two kinds of samples break
a fit by steps alone: a silent subband the encoder gave a fine step, as `open bitrate.dts`'s has
ABITS 24 at the smallest scale factor, all its indexes 0, where the WAV has its rounding; and
silence beside a sudden loud onset, where the decoder's filterbank, FILTS 0's not quite perfect
reconstruction, leaves a few percent of the onset in the WAV. Their fine steps made them outweigh
everything else, and the fit's gains came out at 0.017 for a channel that is its WAV channel as it
is.) The encoder mixed in the PCM, before its analysis: each sample's sum of the scaled channels
rounded to the nearest integer (not each product rounded, nor the sum truncated). So a lossless
WAV's mixed or scaled core channel, at the core's rate, is analysed as that rounded PCM, not as its
channels' analyses mixed (`Master Audio 7.1.dts` 14.7% smaller: its indexes the audio's 99.64% of
the time, from 99.51%; the 7-frame 7.1 samples 2%; the XXCH samples' decodes, a decoder's mix, until
the scale factor search shows them decoded, 0.4-0.7% bigger). The LFE gets the remaining WAV channel
whose decimation correlates best with the first 1024 LFE samples that aren't 0 (`Master Audio
7.1.dts`'s first 1024 are all 0), by 0.9 or more; where none does, the one that correlates best, by
more than 0.5, where its decimation corrected as the model's fixed LFE predictors have it does (a
lossless WAV's, or a decoded WAV's: its decoder's interpolation undone): since format 134, FFmpeg's
decodes of its encodes, whose LFE the decimation makes 0.89-0.91 of and the decoded WAV's correction
0.985-0.987 (no other channel more than 0.70 and 0.27), `s.2.dts`'s first 3 MB's core 15% smaller;
and since format 154, in a 16-bit WAV, where none does, by the loudest 1024 of those in the window,
in the stream's order: such a decode rounds a quiet LFE to 0s (FFmpeg's 16-bit decode of `ES 6.1
16bit.dts`, whose first 1024 lie under its LSB: its core 40886 bytes to 25803, its LFE bound; in a
24-bit WAV the 192 kHz quiet-start stream's core 609 bytes to 811 by them);
where the window has fewer than 16 of them and the WAV can be read again, with those further on,
from the first that has 16 more within a window's length, as above (the Cymatics stream's frames
3301-9300 with their LFE made 0 for the first 25.6 s, in the core and the WAV: the LFE went without
the WAV for the rest, 52220 bytes; then 44422, and in format 122 21606). `-s` shows the binding:

```
  the binding: core channel 0 = 0.707092 WAV 2; core channel 1 = 0.707092 WAV 0; ...; core channel 3 = 0.707092 WAV 4*32767/32768 + 0.707092 WAV 6, or with WAV 4*32768/32768; ...; LFE = WAV 3
```

A core channel bound as an earlier one is (the same WAV channels at the same gains: FATE's
samples carry the same noise in several channels, and `xll_xch_61_24_48_768`'s surrounds are the
same mix) is that one's twin (format 89, `model_smp.inc`), and so, for a subframe, is one whose
subband samples there are an earlier one's, not all 0 (FFmpeg's `master_audio_7.1_24bit.dts` has
4 channels the same for 8% of its length; silent ones as twins made `Master Audio 7.1.dts` 0.4%
bigger): the encoder had the same input for both, and nearly always made the same of it. So each
of a twin's side fields (PMODE, the ADPCM predictor, ABITS, TMODE, its scale factors, its HF VQ
vector) is coded first as whether it is the earlier channel's, at the odds that has had for that
field, the two in the same pass (so XXCH's channels are only each other's twins: in FATE's XBR
and X96 XXCH samples its two are bound to one WAV channel); and an index whose ABITS, step and
ADPCM prediction are the earlier channel's goes as that one's index, far from any boundary. Where
they differ but the earlier channel's step is no coarser, its decoded value is taken as the
audio's, once that has been the index clearly more often than the audio's own: against a clipped
decode it is, against a lossless WAV it isn't. The 7-frame DTS-HD MA samples 2-22% smaller,
FATE's other samples 12-28%, the XXCH ones 16-32% (`xbr_xxch_71_24_48_3840`'s 4035 bytes to
2734), `master_audio_7.1_24bit.dts` 1.7%; no other stream at hand has a twin.

### The model

Each element is binarised, and each decision goes to one of twenty-one families (`fam.inc`,
ac3pack's, m4vpack's and h264p's): counters chosen by context indices, mixed in the logistic
domain, refined by APMs, then range coded (`rc.inc`). `famvars.inc` lists the variables the model
gives each family; `IDX/l9/<family>.idx` declares level -9's indices over them and their tunable
parameters, and `IDX/l8/` level -8's (the same but for eight families' parameters, below), which
`IDX/idx2inc.pl` turns into `MOD/l9/` and `MOD/l8/` (`./mk.sh mod`; the format:
[IDX/IDX-FORMAT.md](IDX/IDX-FORMAT.md)). The tables are mapped zeroed and take memory where they are
written; the APMs' rows and the third mixer's, by their contexts' decision nodes (an APM's up to
504 bytes a node), are laid out in the order a stream first codes them (a context's nodes' rows by
an index of their own: the same model), so that a short stream touches a page of them where it
touched several (a 7-frame stream's run maps 700 fewer pages, a quarter of them). A value against
its prediction (the hdr, side, ABITS and scale factor families') is its difference's magnitude, then
its sign, since format 175 a decision of the family's own (its last node, under all its contexts),
and only where both signs could be the value (a sign counter of its own before, above).

- **hdr**: the frame's header, the coding header and the subframes' headers, each field against its
  last values (the extension's headers too: XCH's and X96's frame size against the one its sync
  word's distance to the frame's end makes); the container's kinds and lengths.
- **sid**: a subband's PMODE (against the gain and the rise above) and TMODE, and the predictors and
  HF VQ vectors coded as they are (X96's, and the core's where X96 codes the subband too); a
  channel's the WAV doesn't bind, its HF VQ vectors and the LFE samples too until format 167 (sdn's
  since; since format 173 the core's HF VQ vectors but for whether each is the subband's last one
  by a mixture of their own, below).
- **sdd**: a decoded WAV's (as the scale factor or the bit allocation search shows it, as for abd,
  or at -9 where under 90% of its indexes have been the audio's; sid's until format 132, below).
- **sdn**: a channel's the WAV doesn't bind, and the LFE's where it isn't bound (sid's, then sdd's,
  until format 167, above); since format 173 of its HF VQ vectors only whether each is the subband's
  last one; since format 180 its PMODEs against the channel before's, its TMODEs against the subband
  below's and the channel before's; since format 181 the LFE's values under a logistic about its
  fit's, above.
- **jsc**: the joint scale factors (the core's and X96's), against the source channel's audio or
  their running mean (sid's until format 116, below).
- **abt**: the bit allocations.
- **abd**: a decoded WAV's (as the scale factor or the bit allocation search shows it; abt's until
  format 118, below).
- **abn**: a channel's the WAV doesn't bind (abt's until format 166, above; since format 174 its
  first two decisions' outside predictions by the counts of its ABITS, since format 175 every
  decision's, since format 177 those counts mixed with a map's of the ABITS by the first scale
  factor less its subband's offset and the subframe's level, since format 178 that level by its
  priors too and the maps by how sure it is, above).
- **scl**: the scale factors, against the RMS's (the LFE's too).
- **scd**: a decoded WAV's (as the scale factor search shows it; scl's until format 118, below).
- **scn**: a channel's the WAV doesn't bind (scl's until format 166, above; since format 176 its
  first scale factors' outside predictions by a least squares fit of their move, since format 179
  its HF VQ subbands' and its transients' second ones' by fits of their own, above).
- **hit**: whether an index that isn't the audio's is the other side's, and an LFE sample's whether
  it is the audio's.
- **hid**: a decoded WAV's (as for sdd; hit's until format 132, below).
- **smp**: the indexes otherwise (and from format 156 to 158 all of them where the WAV isn't bound,
  as their residuals from a fit of the decoded values, where those have been nearer: above):
  magnitude and sign.
- **smd**: a decoded WAV's (as for sdd; smp's until format 132, below).
- **smn**: a channel's the WAV doesn't bind, its indexes' residuals from the fit of its decoded
  values (smp's and smd's until format 159, above).
- **vqr**: the ranks of the HF VQ vectors (their groups', then theirs in the group).
- **vqp**: the ranks of the predictors (their blocks', then theirs in the block; vqr's until format
  116, below).
- **vpd**: a decoded WAV's (as for sdd; vqp's until format 132, below).
- **vpn**: a channel's the WAV doesn't bind (vqp's, then vpd's, until format 167, above; since
  format 180 the channel before's predictor at the subband first, since format 181 the channel two
  before's too, the three in the order of how often each has been the predictor, above).
- **raw**: bytes kept as bytes, the bits after the subframes, those before the extension's sync word,
  and XXCH's other bits.

A decision's counters (each an adaptive probability and a fast one), A's refinement by the first
APM, the outside prediction (vqr's and abt's), the bias and the hint's counter go to three mixers,
each by an index of its own, and the final mixer mixes theirs by the decision's node and the hint's
state. A mixer's row starts at 1/24 for each input, but since format 111 at a quarter for the
outside prediction, and the final mixer's at a third for each mixer and 0 for the bias (a quarter
each before): the regression set 99 bytes smaller, `s.2.dts`'s core 55, the calibration set 151
(the final mixer's alone 30; the outside prediction's at an eighth or a half, 28 and 36 bytes
bigger than at a quarter; the final mixer's 2/5 each, 18 bigger than before, the bias's a quarter
kept, 18 bigger; the outside prediction in the final mixer too, at 1/5 as the others then, 1187
bigger; a new row of the first three from weights by node that every decision trains, 122).
Since format 112 each family's are its own (`IDX/l9/<family>.idx`'s `wp`, `wq`, `wo` and `wf`,
tuned on its dumps), the counters' too: their adaptive probabilities' 1/15 to 1/5, their fast
ones' 0 to 1/32, the final mixer's a quarter to 0.46 for each mixer: the regression set 70 bytes
smaller (at 1/10 and 0 for every family, 44; the adaptive ones' at 1/48 and the fast ones' at
1/16, 48 bigger). Since format 113 A's refinement's too (`wa`), and the final mixer's rate and
boost its own (`lf` and `bf`, in 16ths of the others'), tuned with the rest (below): 37 bytes
smaller than with format 112's tuned anew (hit's and raw's final mixers at 5/8 of the others'
rate and without the boost, vqr's and smp's at a quarter of it). Since format 114 mixers 2 and 3
have rates of their own too (`l2` and `l3`, in 16ths of mixer 1's), tuned with the rest: 57 bytes
smaller (sid's and scl's mixer 2 all but still, at a rate of 1); since format 115 boosts of their
own too (`b2`, `b3`): 7 bytes smaller.

A family's probabilities are 12-bit, the range coder's 18 (15 before format 103): at 1/4096 a
decision that is all but sure costs 3.5e-4 bits, and most of a stream's are (418 thousand of the
families' 614 thousand in `s.2.dts`'s first 3 MB, 9 of them the other way). So since format 102 a
decision within 4/4096 of either end (8/4096 since format 104) goes to the coder at its own
precision, by how often the family's such decisions went the unlikely way (each family's counts in
bins: at the end, next to it, 3 or 4 off, and since format 104 5 to 8 off; their (f + 1/2) / (n +
1), halved at 2^16), never less sure than its 12 bits: `s.2.dts`'s core 0.71% smaller (79479 bytes
to 78916), `open bitrate.dts`'s 8.0%, `ES 6.1 16bit.dts`'s 2.1%, the lossless WAVs' others
0.15-0.75%, the other decoded ones 0.04-0.17%; the 7-frame samples as they were, too short for the
counts (on six of the streams, the first two bins alone 7 bytes bigger, each end's bins of its own
1 bigger; the fourth bin 2 bytes smaller with the coder at 15 bits, 12 at 18, a fifth to 16/4096 2
more). Past the cap (format 105), a decision's unlikely value goes at its probability times how
often the family's such decisions went that way against how often they were expected to (each with
a prior of 2, by the probability's log2, in halves until format 107); to 255/4096 until format
108: `s.2.dts`'s core 16 bytes smaller, `96-24.dts`'s 9; to 511/4096, `s.2.dts`'s 19 smaller still
but the short streams 14 bigger. Since format 108 every decision goes so, in a table for each kind
of decision (a sign's, a value's first decision's, its others' through the mixers, a mantissa's with
the hint's, one's without), between its bin's correction and the next one's by where in the bin its
probability lies, with a prior of 16 decisions from 256/4096 on, halved at 16384: the regression
set's short streams 0.15% smaller, `s.2.dts`'s core 0.06% (to 1023/4096, by the bins alone and with
a prior of 2, `s.2.dts` 32 bytes smaller and the short streams 9 bigger; a prior of 8 from 256/4096
on, the short streams 17 smaller; every decision, 20 bytes more; the interpolation 26; the kinds'
tables 42; a prior of 32, 14 bigger).

Until format 103 the range coder (`rc.inc`, h264p's) took a 0's share of the range as the range's
top bits times its probability: the range is 2^24 or more, so at 15 bits the share came out short
by up to 2^-9 of itself, a likely 0 dearer by up to 0.003 bits (at 16 bits by twice that: 3-14
bytes bigger on five of the regression set's streams). Since then the share is the exact product,
in 64 bits, and the probabilities have 18 bits (one of 2^-18 still a share of 64 or more):
`s.2.dts`'s core 0.61% smaller (by 482 bytes), `open bitrate.dts`'s 6.0%, `ES 6.1 16bit.dts`'s
2.3%, the lossless WAVs' others 0.1-0.7%, the other decoded ones 0.02-0.4%, the 7-frame samples
within 2 bytes (the product alone, at 15 bits, 0.02-1.0% on six of the streams; 20 bits as 18).
The hit flags' probabilities go to 2^-18 with it, the families' all but sure decisions too. A
decision may then leave the range as little as 64, where at 15 bits it left 512 or more, so the
range comes back to 2^24 by up to three bytes, not two (with two, four decisions in a row each
coded against a probability of 2^-18 could leave the next a share of 0, and the decoder would lose
its way; `s.2.dts` leaves less than 256 of it 15 times, as little as 128, `ES 6.1 16bit.dts` 71).

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
dumps three times, so that the long stream weighs as much as the short ones): 1.3% off its output on
them, every stream at hand smaller (`s.2.dts`'s core 0.7%; tuned on the whole of `s.2.dts` with the
ten once each, it took 0.75% off that but made `open bitrate.dts` 0.7% bigger, and with `s.2.dts`'s
two 3 MB pieces for it, 0.1%, the short streams 0.1-0.6% smaller). So were scl and abt after the
64-band analysis's gain (a pass each, on the same dumps): 4.2% and 0.9% off their output on them,
the whole of `s.2.dts`'s core 0.7% smaller and `96-24.dts`'s 0.9%, `ES 6.1 16bit.dts`'s 0.8%, but
the short streams and `open bitrate.dts` up to 0.7% bigger (`s.2.dts`'s two 3 MB pieces 0.2% and
0.3%, `open bitrate.dts` 0.7%, the 192 kHz `xll` sample 0.6%), its parameters the long streams'.
Then hit and sid, after the top subband's leak noise (a pass each, on new dumps of the same
streams): 2.1% and 2.2% off their output on them, `96-24.dts`'s core 0.8% smaller, `open
bitrate.dts`'s 1.2%, `ES 6.1 16bit.dts`'s 0.9%, `s.2.dts`'s 0.1%, the rest within 10 bytes (the 192
kHz `xll` sample's 0.3% bigger). Then raw and hdr (a pass each, on those dumps): 14.5% and 12.0% off
their output on them, `open bitrate.dts`'s core 4.4% smaller, `ES 6.1 16bit.dts`'s 2.0%,
`96-24.dts`'s 0.4%, `Master Audio 7.1.dts`'s 0.5%, the `xll` samples' up to 2.2%; a few streams 1-4
bytes bigger. With the predictors' two ranks, vqr again (two passes, on new dumps of the same set):
0.26% off its output on them, `s.2.dts`'s core 0.07% smaller, the others' up to 0.45%. With bg, abt
again (a pass, on new dumps), and sid, scl and hit a little (a part of a pass, on the dumps before):
1.43% off abt's output on them. Then sid, scl, hit and vqr (a pass, on those dumps; hit's nearly):
0.01-0.16% off their output on them; and with the allocation table and the transients' mean squares,
abt and sid (two passes, on new dumps): 2.3% and 0.1% off theirs. After the near misses, the WAV's
end, the staircase's agreement and the channel's misses, smp, abt, sid and hit (on new dumps; two
passes each but hit's one): 1.0%, 0.3%, 0.01% and 0.4% off their output on them, FATE's XXCH samples
0.5-1.4% smaller and its `dcadec-suite` ones 0.4-1.0% (one 2 bytes bigger), the others within 0.4%
(`s.2.dts`'s core 9 bytes bigger). After the searches of formats 38-46, abt, scl, hit, sid and smp
(on new dumps; abt's a pass, the others' a part of one): 1.21%, 0.44%, 0.14%, 0.02% and 0.04% off
their output on them, `ES 6.1 16bit.dts`'s core and `open bitrate.dts`'s 0.8% smaller,
`master_audio_7.1_24bit.dts`'s 0.5%, `s.2.dts`'s 0.06%, the 7-frame samples' up to 4 bytes bigger.
Then vqr, hdr and raw (on new dumps; two passes): 0.11%, 7.4% and 0.75% off their output on them,
`open bitrate.dts`'s core 3.5% smaller, `ES 6.1 16bit.dts`'s 0.7%, `3-1.dts`'s 0.5%, the rest up to
0.6% (one of FATE's samples a byte bigger). After format 51's fit, vqr, scl, sid and hit (on new
dumps; a pass): 0.03%, 0.10%, 0.04% and 0.06% off their output on them, `s.2.dts`'s core 0.04%
smaller, FATE's `dcadec-suite` samples up to 6 bytes bigger. With the nearness and the margin
against the level as contexts of their own (format 55), scl and vqr (on new dumps; a pass):
0.85% and 0.18% off their output on them, the lossless WAVs 0.2-0.7% smaller, the others within
10 bytes. With the groups' margin and the vectors' near ties (format 56), scl, vqr, sid, abt, smp
and hit (scl another pass on those dumps, vqr a pass on new ones, the rest a pass): 0.15%, 0.28%,
0.01%, 0.21%, 0.01% and 0.05% off their output on them, `s.2.dts`'s core 0.4% smaller, every
other stream within 20 bytes, most of them smaller. Then hdr and raw (a pass, on format 56's
dumps), abt and vqr again (a pass each on those), vqr and scl once more: 0.33%, 0.15%, 0.09%,
0.15% and 0.02% off their output on them, `s.2.dts`'s core 0.04% smaller, the rest within 7
bytes. With the scale factors' runs and the rank buckets' gaps (format 58), scl (two passes) and
vqr (a pass) on new dumps: 0.15% and 0.09% off their output on them, `s.2.dts`'s core 0.09%
smaller. With the HF VQ values scaled into their scale factor's region and a missed group's
vectors from the scores' boundary (format 59), vqr and scl (a pass each, on new dumps): 0.02% and
0.17% off their output on them, `s.2.dts`'s core 0.06% smaller. With the HF VQ misses by their
boundaries and PMODE's and TMODE's rise (format 60), vqr (two passes, on two new dumps), scl and
sid (a pass each, on the second and a third): 0.03%, 0.22% and 0.21% off their output on them,
`s.2.dts`'s core 0.18% smaller. With the margins in the boundary order's terms (format 61), vqr
(two passes, on two new dumps): 0.09% and 0.12% off its output on them, `s.2.dts`'s core 0.09%
smaller. With the transients' threshold by the subband (formats 62 and 63), abt, scl, sid, hit,
hdr, raw, smp and vqr (a pass each, on new dumps): 0.40%, 0.04%, 0.41%, 0.12%, 0.10%, 0.03%,
0.00% and 0.01% off their output on them, `s.2.dts`'s core 0.38% smaller (its PMODEs 0.97 KB to
0.57). With the transients' class and the ABITS by that threshold alone (format 63), sid and abt
again (format 64, a pass each on new dumps): 0.32% and 0.06% off their output on them, `s.2.dts`'s core
0.01% smaller. With the LFE by the encoders' filter (format 64), every family again (format 66, a pass
each on new dumps): hit 0.24% off its output on them, scl 0.05%, smp 0.03%, abt and hdr 0.02%, sid
0.01%, vqr and raw 0.00%; `s.2.dts`'s core 0.02% smaller. With the hit cells' fractions, two speeds
and miss classes and the HF VQ near ties (formats 71-74), scl (two passes) and hit (a pass) on
new dumps (format 75): 1.03% and 1.60% off their output on them, `s.2.dts`'s core 0.07% smaller,
FATE's `dcadec-suite` samples up to 1.5%, the rest within 10 bytes. smp, sid and abt (a pass each
on those dumps) took 1.55%, 0.25% and 0.47% off theirs but made the longer streams bigger (smp
the five medium ones 27 bytes in all, sid `96-24.dts`'s core 38 bytes, abt `ES 6.1 16bit.dts`'s
16), and vqr's two passes 0.05% off its output changed little, so they stayed as they were.
Then smp, sid, abt, vqr and hit again (format 76, two passes each) on those dumps with the
longer streams weighted up (`96-24.dts` and `ES 6.1 16bit.dts` three times, `s.2.dts` and
FATE's three dcadec-suite samples once, the rest twice): 0.25%, 0.02%, 0.05%, 0.02% and 0.30%
off their output on them; smp's and vqr's kept, the 25 short and medium streams 69 bytes smaller
in all, the other streams within 3 bytes (sid and abt changed none by more than 4 bytes, hit made
the short streams 38 bytes bigger). With sx (format 78), abt, sid, hdr and raw (two passes each,
on new dumps weighted the same way; hdr's and raw's first passes and sid's start on the dumps
before sx, which they don't see): 1.85%, 0.02%, 3.8% and 7.3% off their output on them; abt's,
hdr's and raw's kept (`s.2.dts`'s core 72 bytes smaller, the short streams 160, `Master Audio
7.1.dts`'s 29), sid's not (the short streams 31 bytes bigger). With sx in six classes (format 79),
abt again (two passes, on new dumps): 0.17% off its output on them, the short streams 9 bytes
smaller, the others within 4. With the hit cells by the channels before (format 80), scl, smp, hit
and vqr (format 81, two passes each, on new dumps): 0.24%, 0.06%, 0.35% and 0.00% off their output
on them; scl's and smp's kept (`s.2.dts`'s core 12 bytes smaller, the medium streams 3-11), hit's
not (the short streams 30 bytes bigger). With the bias and the FFT path's level (format 82), vqr,
hit, smp and scl again (two passes each, on new dumps; none gained in the second): 0.03%, 0.32%,
0.03% and 0.05% off their output on them; vqr's, smp's and scl's kept (`s.2.dts`'s core 11 bytes
smaller, the short streams 20), hit's not (the short streams 27 bytes bigger again). With the
calibrations of formats 104-108, every family again (format 109, one or two passes each, on format
107's dumps of the thirteen streams weighted as before): 0.05% off their output on them in all;
vqr's, scl's, abt's, hdr's and sid's kept (`s.2.dts`'s core 42 bytes smaller, the short streams
16), smp's and hit's not (with the far misses' distances, 4 and 3 bytes bigger). With those, smp
and hit again (format 110, two passes each, on format 109's dumps): 0.32% and 0.23% off their
output on them; smp's kept (the short streams 24 bytes smaller, `s.2.dts`'s core the same), hit's
not (8 bytes bigger). With the mixers' initial weights (formats 111 and 112), every family again
(format 113, on format 109's dumps; two passes each, abt and sid one and part of a second): 0.13%
off their output on them, the regression set 116 bytes smaller (`s.2.dts`'s core 36); and once
more from that (format 114, one pass each): 0.03% more off their output, the regression set 20
bytes smaller; and again (format 115, one pass each, with mixers 2 and 3's boosts): 0.07% more
(scl's alone 1.1%), the regression set 63 bytes smaller. Since format 116 two kinds of values
have families of their own. The joint scale factors were 95% of sid's output on the dumps
(`96-24.dts`'s X96 ones, all but noise: above), so sid's parameters were theirs; now jsc's
(from sid's: 0.08% off theirs apart, then three passes, 0.22% more), and sid's own were tuned
again on its other fields (two passes, 2.2% off their output). And the ADPCM predictors' ranks
went with the HF VQ vectors' (vqr's), which the long `s.2.dts` weighs the most: now vqp's (from
vqr's: apart, 0.6% bigger at first; two passes, 1.5% off that), the HF VQ ranks' tuned again on
their own (three passes, 0.18% off theirs). In all, the regression set 189 bytes smaller
(`96-24.dts`'s core 90, `s.2.dts`'s 42, FATE's XXCH samples 10-17 each). Format 117's
changes are for a DTS:X encoder's streams (the LFE half a step up, the subbands' running
difference, above): the Cymatics stream's core 2.6% smaller, the regression set 23 bytes
(`ES 6.1 16bit.dts`'s LFE). Since format 118 a decoded WAV's bit allocations and scale factors
have families of their own, abd and scd, taken where the scale factor search has been right more
often than the better rule by a 16th (its rule for the rest, ss_rule(), wants 64 more and an 8th:
by that, FATE's 7-frame decoded samples 1-23 bytes bigger each, most of their frames coded before
it switched), the bit allocations also where the bit allocation search's rule says so (X96's
where X96's does). Tuned on the decoded streams' dumps alone (four passes and three), abt's
parameters took 5.4% off theirs there, scl's 2.3%; and on the lossless ones' alone
(three passes and one), abt's 2.1%, scl's 1.7% (one pass of abt's on all of them, 0.04%). In all,
the regression set 309 bytes smaller (`s.2.dts`'s core 115, `ES 6.1 16bit.dts`'s 55), the
calibration set 278, the Cymatics stream's core 233. Tuned apart the same way, smp's, vqp's, hit's
and sid's took 0.3-0.7% off theirs on the decoded streams, too little for families of their own
(until format 132, below: the lossless streams' sid, tuned apart, took 18% off its output on
theirs). Since format 119 the switches to a decoded WAV's rules come sooner: the scale factor
search's, ss_rule() (the rest of the model's, and since format 118 the decoded WAVs' families'),
where it has been right more often than the better rule by a 32nd (it wanted 64 more and an 8th,
which FATE's 7-frame decoded samples never reached; the families, a 16th), X96's subbands' the same
(64 more and an 8th before), and the bit allocation search's by 16 more and a 32nd (32 before). The
decoded WAVs' streams 0.67% smaller in all (the regression and calibration sets' short ones), the
lossless WAVs' the same: the regression set 417 bytes smaller (`96-24.dts`'s core 194, FATE's XXCH
samples' 37-60, `dts_es.dts`'s 64), the calibration set 431. Taken from the first frame on instead
(as a header flag would have them), the decoded WAVs' rules made those streams 26 bytes smaller
again but three of FATE's samples 32 bigger (the XBR samples and x96_xch, whose searches are right
no more often than the RMS rule): not worth a flag. Format 120 retunes abd and scd on format 119's
dumps of the decoded WAVs' streams (three passes each: 1.9% and 1.35% off their output there): the
regression set 46 bytes smaller, the calibration set 34. Format 121 retunes vqr, smp, hit, vqp, abt
and scl on format 120's dumps of the 14 streams (weighted as before; one pass to three, as each kept
gaining): scl's output there 0.7% smaller, vqp's 0.3%, hit's 0.13%, the others' 0.01% or less; on
the Cymatics stream's, hit's 1.9%, scl's 4.3%. Its sid, tuned the same way (1.1% off its output
there), is left out: the Cymatics stream's sid output 2.5 times bigger with it, 2887 bytes. The
regression set 22 bytes smaller, the calibration set 26, the Cymatics stream's core 667. Format 126
retunes abt on format 125's dumps (four passes, 0.17% off its output there): the regression set 54
bytes smaller, the calibration set 43, the Cymatics stream's core 18. Its sid, tuned the same way
(2.3% off its output there; the calibration set 79 bytes smaller again, the 28 short streams 63), is
left out again: the Cymatics stream's core 141 bytes bigger with it. Format 128 retunes sid on
format 127's dumps, the Cymatics stream's among them (weighed as two of 28 streams; part of a pass
in two hours, 3.2% off its output there): the regression set 9 bytes smaller, the calibration set 5,
the 28 short streams 7, the Cymatics stream's core 81 (its transient modes 217 bytes to 143). Its
hit, abt and vqr, tuned the same way (1.2%, 0.8% and 0.3% off theirs there), are left out: each made
the Cymatics stream's core 140-160 bytes smaller and the other sets 9-41 bytes bigger. Format 129
retunes scl, smp, vqp and abd on the same dumps (two hours, the four at once: scl three passes and
part of a fourth, 3.6% off its output there; smp seven, 0.38%; vqp two, 0.1%; abd seven, 0.9%): the
regression set 37 bytes smaller (`s.2.dts`'s core a byte bigger), the calibration set 103, the 28
short streams 114 (none of them bigger), the Cymatics stream's core 113. Format 130 adds level -8
for what format 128 left out: level -9's model with hit's, vqr's, abt's and smp's parameters tuned
on the dumps of the Cymatics stream (weighed twice) and of seven cuts of it, 8 frames to 98 s (two
hours in all, from a tuning on the whole stream alone; their output there 2.1%, 0.6%, 2.4% and 0.6%
smaller than with level -9's parameters), which `c` takes where the binding mixes two WAV channels,
one with a gain before the mix, as that encoder's downmix does: the Cymatics stream's core 543 bytes
smaller (0.88%), the cuts 32-148 bytes smaller each but for the 8-frame one (5 bytes bigger); the
other streams take -9, as before. Tuned on the whole stream alone, the four had made its core 514
bytes smaller but the two cuts that start quiet 13 and 69 bytes bigger and the 8-frame one 51.
Format 131 retunes the four families -8 still had as -9's, scl, sid, vqp and hdr, at each level:
-9's on format 127's dumps of the 14 streams alone, now that the Cymatics stream takes -8, and of
two outside the sets, FATE's 192 kHz sample with 300 silent frames before it (above) and the
silent-surround one (sid 1.4% off its output on the 14 streams' dumps, half off the 192 kHz
stream's, which sid tuned on the 14 alone had made 15% bigger; scl 0.4%; vqp and hdr nothing), -8's
on its and its cuts' (sid 12.8%, scl 4.8%, hdr 8%, vqp 4%): the regression set 27 bytes smaller, the
calibration set 55, the 28 short streams 32 (`open bitrate.dts` 16 bigger), the 25 short streams 15,
the Cymatics stream's core 126 bytes smaller (0.21%), its cuts 9-89 each, the 192 kHz stream's 364
(993 to 629). hit, vqr, abt and smp, retuned the same way at -9 (0.02% off theirs), are left out:
the regression set 10 bytes smaller with them, the 25 short streams 2 bigger. Format 132 gives hit,
smp, vqp and sid a family each for a decoded WAV's decisions, hid, smd, vpd and sdd, taken where abd
is (the scale factor or the bit allocation search right clearly more often) and, at -9, where after
a 5.1 frame's indexes fewer than 90% of them have been the audio's: FATE's XBR and XXCH samples,
whose searches fail too (82.9-85% the audio's; with the other families `x96_xch_61_24_96_3840.dtsc`
and `xbr_xch_61_24_48_3840.dtsc` 23 bytes bigger each; at 95% the lossless WAVs' noisy first frames
went with them, `s2h.dtsc` 13 bytes bigger; at -8 the Cymatics stream's 8-frame cut, its indexes
75.5% the audio's, 20 bytes bigger). Format 118 had found too little apart in these four for
families of their own; it is the lossless streams' parameters that differ now: tuned on format 127's
dumps of those streams alone, sid's took 18% off its output there (851 bytes to 697, a pass), vqp's
4.1%, hit's 0.5% and smp's 0.4%; on the decoded ones' alone, sid's 2.9%, hit's 1.3%, vqp's 0.6% and
smp's 0.3% (50 minutes for each kind, the four at once). From those, tuned again on new dumps of the
14 streams and the two outside the sets (weighted as before), the decoded WAVs' families on their
decisions there (hid's output 1.5% smaller, sdd's 1.5%, vpd's and smd's 0.2% and 0.1%) and the
others on the rest (sid's 3.8%, hit's 0.1%; two and a half hours of tuning in all): the regression
set 195 bytes smaller (`s.2.dts`'s core 18), the calibration set 159, the 28 short streams 170 (13
of them 2-32 bytes smaller, 10 of them 1-11 bigger, FATE's XBR, XXCH and X96 samples the most), the
25 short streams 72, the 192 kHz stream with 300 silent frames before it 20 (629 to 609); FATE's
DTS:X sample, its indexes 37% the audio's, 32 bytes bigger (2400 to 2432). A decoded WAV's families
learn from the first decisions they take, not from the stream's start: with the others' parameters
as they were, those streams came 1-9 bytes bigger. Level -8's four are -9's, the Cymatics stream and
its cuts as they were. Format 140 tunes hit and smp again, whose contexts format 139 changed for
the LFE values: at -8 on the Cymatics stream's dumps (twice) and its cuts', before and after its
LFE's fixed filters took their gain (above), at -9 on the 16 streams' as weighted before (hit's
output there 0.6% smaller, smp's 0.02%). Format 141 tunes the decoded WAVs' hid, smd and scd again
with FFmpeg's five encodes among those dumps, none of which any tuning had had before: their output
2.0%, 1.0% and 0.7% smaller there; FFmpeg's encodes 27764 bytes from 28023 (0.9%), against its
16-bit decodes of them 1102817 from 1105689 (0.26%), its encode of the 16-bit WAV (above) against
its 16- and 24-bit decodes 0.06% and 0.3% smaller; the regression set 22 bytes smaller, the
calibration set 25, the 28 short streams 27, the 25 short streams 24, none bigger. Format 142 tunes
hid and smd again on those dumps made again, the decoded WAVs' hit cells against their LSB (above):
their output 0.7% and 0.2% smaller there, FFmpeg's encodes 55 bytes (27164 to 27109), the 28 short
streams 10 and the 25 short streams 9 of the 34 and 44 they had grown, the calibration set 10 of
its 26. Format 168 tunes the decoded WAVs' six families again, abd, scd, smd, hid, sdd and vpd, on
dumps of decoded WAVs alone: the 24-bit decodes of `96-24.dts`, `dts_es.dts`, FFmpeg's `f1.dts` and
`f4.dts`, a DTS:X cut, a cut of `dtswavsample16.wav` and the first MB of `padded.dts`,
`dtswavsample14.wav`, the DTS CD's sample, `ES 6.1 16bit.dts` and `open bitrate.dts`, and the 16-bit
decodes of `dts_es.dts` and of `f4.dts`'s first 400 KB, weighted by copies (36 in all; 88 minutes,
four to six passes each): their output there 102391 bytes to 99238 (sdd's 9.0% smaller, abd's 7.6%,
scd's 3.8%, vpd's 1.8%, hid's 0.9%, smd's 0.8%). With their WAVs, the check lists' 67 runs 1467537
bytes to 1455804 (0.80%: the DTS CD's sample and its packings 4.5-6.7%, `dts_es.dts`'s 16-bit decode
3.4%, `padded.dts` 3.2%, `ES 6.1 16bit.dts`'s 16-bit decode 2.5%, `dtswavsample14.wav` 2.2%,
`dtswavsample16.wav` 2.1%, `open bitrate.dts`'s 16-bit decode 1.4%, FFmpeg's encode of the 16-bit
WAV against its 24- and 16-bit decodes 0.40% and 0.19%; 16 bigger: `96-24.dts` and its two packings
by 111 bytes each, 0.39%, their scale factors 66 and PMODE 27; `ES 6.1 16bit.dts` and its packing by
81, 4.4%, its scale factors 36, ABITS 21 and PMODE 14; FATE's short samples by 3-38 bytes, 0.2-1.4%,
in scale factors and indexes; FFmpeg's `f2.dts` and the 44.1 kHz encode by 8 and 3). Without their
WAVs the 55 streams 0.006% smaller (above).
`./mk.sh check 9 stream wav ...` (and `8`)
confirms that the tuning build, the shipping build and the build without AVX2 write the same
files at that level.

### The format

"dtsdpack", the format's version (175), the level (9, or 8: above) and above it the stream's packing
(since format 145: 0 the core's own, 1 little-endian, 2 and 3 14 bits a word big- and
little-endian); what the WAV was (whether it was one, and since format 155 whether a heights WAV was
added into it, its channels and bytes a sample, its size and the CRC-32 of its first 64 KB; then the
heights WAV's channels, bytes a sample, size and CRC, where there was one) and the binding (the
offset; the WAV's samples to the core's, 1, 2 or 4 (its lower band, then 2), and the lower band's
resolution; the LFE's channel; each core channel's WAV channel, and where it isn't that channel as
it is, the second of its mix and their gains, a downmix table entry's index where they are one, and
where it has them, its WAV channels' gains before the mix and a second pair of those), numbers in
7-bit groups; then the range coded chunks (a frame as records, bytes between frames, a frame kept as
bytes), the end, and the CRC-32 of the core stream, which `d` checks. The header is 31 bytes for a
5.1 stream bound channel for channel (111 before format 90, every binding's fields at a fixed size:
up to a third of a 7-frame DTS-HD MA sample's output).

## What it doesn't do

- A core channel is bound to at most two of the WAV's channels: one that mixes more (a stereo core
  downmixed from 5.1) is bound to its best two, or isn't. Their gains before the mix are tried
  only as 1 and 1 -+ 2^-15 (the Cymatics stream's encoder's), and only at the core's rate. A
  192 kHz stream's WAV goes into XLL's frequency bands at the resolution its samples show (their
  common trailing 0 bits, in its first 4 MB, or as far as 2^16 of them that aren't 0 go, to 256
  MB): a 24-bit stream of 16-bit audio would be taken at 16 bits, untried; a 176.4 kHz one's is
  taken the same way, untried.
- The LFE is predicted at 64x only (LFF 2); a 128x LFE (LFF 1) is coded by the model alone.
- Without the WAV a channel's side information (bit allocations, scale factors, predictors; HF VQ
  vectors until format 173, above) goes by its own past, not by the channel before's (since
  format 158 by how that one moved too, and by its fields where theirs agree, above):
  `open bitrate.dts`'s second channel, nearly its first (98.1% of its bit allocations and steps the
  same), codes them in full again (its scale factors 230 KB of the 2.05 MB). In the other streams at
  hand the channel before's are the same less often than the channel's own last (28.6-40.8% of the
  bit allocations, against 54.9-68.2%; 5.2-8.3% of the scale factors, against 9.9-19.2%).
- Heights are added back from a heights WAV, or a 7.1.4 WAV's own (FFmpeg's layout, by its channel
  mask exactly): no FFmpeg build that decodes 7.1.4 was at hand, so its WAVs were made by its
  source's rule (`dca_xll.c`); a stream whose XLL codes 7.1.4 itself (none at hand) would have its
  heights added the same way, which its core's downmix may not have, and where FFmpeg clipped the
  bed less a height at 24 bits the sum isn't the bed.
- A heights WAV is taken sample for sample from the WAV's start: dtssplit's `prefix.x.wav` has
  samples for the frames with DTS:X object audio alone, so for a stream with object data in only
  some of its frames (none at hand) they would go into the wrong samples past the first frame
  without: still lossless, but the binding and the model miss them.
- A stream's frames are taken in its first frame's packing alone: a frame packed another way is
  bytes between frames, kept as bytes, and so is one its packing doesn't make again (a bit flipped
  in a word's top two of the DTS CD's sample: its 4 KB frame 4.1 KB of the output).
- XXCH has been tried only on the streams made from FATE's (above), and only one channel set is
  parsed, as in FFmpeg; X96 and XXCH in the extension substream aren't in the core (dtssplit keeps
  the extension substream).
- A decoded WAV can't say what X96's joint scale factors are where their source decodes to
  silence: those are coded against 13/16 of their subband's running mean (each new one an eighth
  of it) and 3/16 of the subband below's (the channels' noises' ratio drifts: `96-24.dts`'s core
  0.9% smaller than by a quarter of the sum of the last one and the one below, which make 7% fewer
  bytes than the last alone), and where the two have any power, 4/5 of that and 1/5 of the root of
  their powers' ratio (0.5%; where the source's mean square is 1 to 2, of the decoder's units, that
  root alone: 3.2 bits a scale from 4.9). Still most of `96-24.dts`'s X96 bytes.
- The scale factor search can't see through the decoder's noise: X96's subbands without bits keep
  the peak rule (94%) where the decoder isn't FFmpeg, whose noise the model draws too, and a
  clipped decode (as the full decodes of FATE's XXCH samples are) has values that aren't the
  decoder's reconstruction, where it isn't sure.
- The bit allocation search fits the core's lattice alone, and a DTS 96/24 stream's core subbands
  hold X96's residuals too (85% of `96-24.dts`'s ABITS right: its indexes are fitted to both
  lattices, above, and a subband's scale factor where it has one in the subframe, but not its ABITS).
- A stream costs most at its ends. A cut one's last frame holds the audio that went on past its
  WAV (its analysis reaches 480 samples past it): 1.0-1.4 KB of each here (`open bitrate.dts`'s
  0.3), about what a frame costs without the audio, 15 KB of the regression set's 69 KB without
  `s.2.dts`. The WAV's continuation by LPC makes the indexes whose windows end 64-128 samples past
  it 71-77% of the time, 42-59% at 128-192, 10-26% at 192-256 (their signs 83-90%); past 224 they
  go without it (192 until format 103: the regression set's cut streams 42 bytes smaller, 6 of them
  1-22 smaller, `open bitrate.dts` 9 bigger, `s.2.dts`'s first 3 MB 6; 128, 160, 256 or 320 bigger
  in all, a choice of them for each stream 15 bytes smaller still). In three of
  them (`Master Audio 7.1.dts`, `Master Audio 5.0 96khz.dts`, the 3 MB from `s.2.dts`'s middle) the
  frame before cost as much again, its analysis all in the WAV: their WAV's last frame is FFmpeg's
  decode of that frame alone, its synthesis without the frames before (as where the lossless
  extension ends before the core does: every channel but the LFE starts at 0 and comes to the
  decode by the frame's end; the decode going on in its place, the 3 MB from the middle 28%
  smaller), and nothing in it says what the frames before add there. The WAV taken a frame shorter
  costs more (67-164 bytes), the last frame's own samples being there; so since format 101 the
  frames before alone take the continuation from that frame's start (every channel but the LFE),
  where that makes more of their indexes, a bit, and go without the audio past it by 256 samples
  or more where it makes fewer than half of those, a second bit (128 to 384 tried): the 3 MB from
  the middle 8.4% smaller, `Master Audio 5.0 96khz.dts` 5.0%, `Master Audio 7.1.dts` 2.9%. (A
  decoded WAV's last frame could be solved for, the decoder's synthesis being known, but the
  system is ill-conditioned for the frame's later blocks; and the frames before's part of a
  last frame decoded alone can't be: the WAV has none of it.) And in its first frames
  the WAV isn't quite the encoder's input: `3-1.dts`'s first six 1.6 KB, noise of about a step,
  white, in all its subbands, as if each channel's decoder state caught up; and a whole program's
  too, `s.2.dts`'s first 15 frames about 0.8 KB (9% of frame 0's indexes missed, 1% of frame 10's,
  0.1% after), noise of 0.15 of a step falling to 0.05, in all its subbands, not a gain or an
  offset, with the XLL residual whole from the first frame: the model's indexes there cost less
  than their hit flags alone would, the noise's level known frame by frame (573 bytes against
  686).
- What is left is mostly the encoders' own arithmetic, which the WAV doesn't show. Of `s.2.dts`'s
  core at format 127 (its costs by field, `-DDTSD_FSTAT`'s, and by subband and the hit flags'
  apart, in a build counting them), the top four subbands, 21 to 24 kHz and little but the
  encoder's FFT noise, took 47.8 KB of 76.4: band 31's HF VQ indexes 24.8 KB and its scale factors
  9.2, subbands 28-30's 13.8; the subband samples' hit flags 20.3 KB, for 47125 misses in 55.7
  million samples, 3.4 bits a miss, where a boundary crossed by Gaussian noise of a known level
  costs 3.3 (3.7 for Laplacian noise); the missed values themselves 12 bytes. The Cymatics
  stream's core: its top four subbands 30.0 KB of 62.0 (band 31's HF VQ indexes at the smallest
  scale factor 11.4 KB, 46676 vectors, the model's first group the encoder's 73% of the time and
  its nearest vector 78%), its hit flags 14.3 KB (34218 misses in 68.8 million, 3.3 bits each;
  3.0 KB of that in its last 3.4 seconds, where the centre alone has the bits, every subband at
  ABITS 24, and 5% of its samples miss), its LFE 8.9 KB, its bit allocations 5.9 KB.
- One thread, and one model: its two levels differ in eight families' parameters, -8's tuned on
  one DTS:X stream (and pieces of it); a build holds the model's code once for each (6.9 MB,
  3.7 MB with -9 alone).

## Build

```
./mk.sh release
```

or by hand:

```
g++ -O3 -fno-stack-clash-protection -march=x86-64-v3 -ffp-contract=off -std=c++11 -fno-exceptions -fno-rtti -ILib3 -pthread -static -o dtsdpack dtsdpack.cpp
```

AVX2 or not, the files are the same (`-static` only makes a short run start sooner; without
the static C library, `-static-libstdc++ -static-libgcc`, as `mk.sh` takes then);
`-DDTSD_ONLY=9` builds level -9 alone (3.7 MB, not 6.9; its `c` takes -9 for every stream and
its `d` reads -9's files alone). It needs GCC's or Clang's `__int128`. `MOD/` is generated
from `IDX/` (perl) and kept in the repository, so a plain build needs no perl; `tables.inc` is
generated from FFmpeg's `libavcodec/dcadata.c` by `tools/mktables.py`, `IDX/l9/*.idx`'s first
declarations by `tools/mkidx.py`, and `hfgroups.inc` (the HF VQ groups' scores) by
`tools/hfgroups.py` (numpy) from the vectors `./mk.sh hfdump DIR stream wav ...` writes: those of
`s.2.dts`, `Master Audio 5.0 96khz.dts`, `Master Audio 7.1.dts`, `master_audio_7.1_24bit.dts` and
`3-1.dts`, in that order, make it as it is; and `pvqblocks.inc` (the ADPCM predictor search's
codewords) by `tools/pvqblocks.py` (numpy) from the predictors `./mk.sh pvqdump DIR stream wav ...`
writes: those of `s.2.dts`, `master_audio_7.1_24bit.dts`, `Master Audio 5.0 96khz.dts`, `Master
Audio 7.1.dts` and `3-1.dts`, in that order; and `lfefilter.inc` (the encoders' LFE filter) by
`tools/lfefilter.py` (numpy) from the LFE values `./mk.sh lfedump DIR stream wav ...` writes:
`s.2.dts`'s; and `lfefilterd.inc` (FFmpeg's decodes') by `tools/lfefilter.py -d` from those of
`ES 6.1 16bit.dts`, `dts_es.dts` and `96-24.dts`, in that order; and `ffenctab.inc` (FFmpeg's
encoder's tables, as its init computes them, with the generating machine's libm) by
`tools/fftab.py`, and `lfels.inc` (its decoder's LFE interpolation undone) by `tools/lfels.py`
(numpy), both from `libavcodec/dcadata.c`.

## License

The tables in `tables.inc` (the QMF and LFE prototypes, the quantizer and scale factor tables, the
ADPCM and HF VQ code books) are FFmpeg's, and those in `ffenctab.inc` and `lfels.inc` are computed
from FFmpeg's (and `ffenc.inc` follows its encoder's code), so dtsdpack is licensed under the GNU
Lesser General Public License, version 2.1 or later. The framework (`fam.inc`, `cm*.inc`, `rc.inc`,
`Lib3/`, `IDX/`) is ac3pack's.
