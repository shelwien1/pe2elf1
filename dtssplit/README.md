# dtssplit

dtssplit splits a DTS stream into three files and joins them back into the stream, byte for byte:

- `prefix.dtsc`: the core frames, a DTS stream that any DTS decoder plays (the lossy part);
- `prefix.wav`: the samples of the lossless extension (XLL, DTS-HD Master Audio), its prediction
  undone: the part of the audio the core leaves out, for a lossless audio compressor;
- `prefix.meta`: all else, the extension headers and the XLL coding parameters, and every byte
  that is neither, small enough that it hardly matters what compresses it.

[dtscore](#dtscore) shows what the core frames are made of, and what a model of them could take.
[dtsreq](#dtsreq) shows how much of the core follows from the lossless audio: almost all of it.

## Usage

```
dtssplit c [options] input.dts prefix
dtssplit d [options] prefix restored.dts
```

- `c` splits the stream into `prefix.dtsc`, `prefix.wav` and `prefix.meta`. A stream whose XLL
  changes its channel layout gets a WAV for each further layout: `prefix.2.wav`,
  `prefix.3.wav`, ...
- `d` joins them back into the stream.
- `-v` (with `c`) joins the files written and compares the result with the input.
- `-q` turns off the statistics.

The input of `c` and the output of `d` can be `-` for stdin and stdout, except with `-v`. `c`
says what it found and where it went:

```
s.2.dts: 130554771 bytes: 24855 core frames, 24855 extension substream frames (24855 with XLL split), 2443 bytes between frames
s2.dtsc: 50008260 bytes, the core frames
s2.wav: 229063784 bytes: 12725760 samples a channel, 6 channels, 24-bit, 48000 Hz
s2.meta: 3615812 bytes: extension substream headers 795360, XLL headers 1734486, XLL coding parameters 1008912, frames kept as bytes 0, bytes between frames 2446, the rest 74608; 0 values clipped, 0 overlapping LSB bits
verify: OK
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | an input can't be read (or a file `d` needs isn't there) |
| 3 | an output can't be written |
| 4 | damaged input (`d`): files that don't join, or join into another stream |
| 5 | `-v` found a difference |

## What goes where

A DTS-HD MA stream is, frame by frame, a DTS core frame (subband ADPCM) and an extension substream
(EXSS) frame, which carries the XLL frame. For each channel, XLL codes either the difference
between the original PCM and the core's decode (a residual channel, the usual kind) or the PCM
itself, in one frequency band (to 96 kHz) or two (to 192 kHz). It decorrelates channel pairs,
predicts each channel (adaptive LPC from reflection coefficients, or a fixed predictor) and codes
the prediction residuals with Rice, hybrid Rice and binary codes, segment by segment (ETSI TS
102 114, clause 8).

- **Core frames:** to `prefix.dtsc` as they are.
- **EXSS headers and the bytes around the XLL frame, the XLL headers** (common header, channel set
  sub-headers, NAVI table, with their CRCs): to the meta file as they are.
- **Each segment of each channel set in each band:** its coding parameters go to the meta file:
  the code choice, the Rice and binary code widths, where hybrid Rice codes put their isolated
  samples, band 1's decimator history, and any nonzero padding bits. Its samples go to the WAV,
  after the entropy codes are decoded, the prediction, the pairwise decorrelation and the MSB/LSB
  split are undone and the two frequency bands are put together.
- **Everything else** (bytes between frames, cut frames, files that aren't DTS): to the meta file
  as it is.

The join predicts and codes the WAV's samples again with the meta file's parameters. Before `c`
writes a frame split, it joins the frame back and compares it with the original; a frame that
doesn't come back the same, or that dtssplit doesn't split, goes to the meta file as it is, and the
statistics say why. Any input therefore joins back to itself.

A frame's XLL channel layout picks its WAV: the first layout's frames go to `prefix.wav`, a second
layout's to `prefix.2.wav`, and so on, each WAV's samples in stream order. A core-only stream gets
an empty WAV.

## The WAV

- **Samples:** one sample a channel for each sample of each split XLL frame, in stream order.
- **Channel order:** WAV order, from the XLL channel masks (C, L, R, Ls, Rs, LFE of a 5.1 set
  become FL, FR, FC, LFE, SL, SR). Channels without a WAV position (Lw/Rw, LFE2, ...) come last.
- **Sample width:** 24-bit; 16-bit for 16-bit streams. It is 32-bit where XLL embeds a downmix,
  since the lower channel sets then hold the downmix unscaled (27 bits in FFmpeg's 7.1 tests).
- **Rate:** the stream's, 192 kHz too. With two frequency bands, the WAV has the bands put
  together, by the lifting steps of XLL's two-band filter, which come undone exactly.
- **Residual channels:** the sample is the original minus the core's decode, at the XLL's
  precision. Each residual sample of `s.2.dts` equals FFmpeg's lossless decode minus its
  bit-exact core decode (`-flags +bitexact -core_only 1`). Over the whole of `s.2.dts` the
  residual is 51-63 dB below the audio in RMS. The LFE's is only 15 dB below: the core's LFE
  carries little above 150 Hz, so the residual holds the rest of that channel.
- **Channels coded without the core:** the sample is the audio.
- **WAVs over 4 GB:** written as RF64.

`d` reads any PCM WAV (RIFF or RF64, PCM or extensible format) with the same channels, width and
rate. That is what lossless audio compressors give back. For example, `s2.wav` through FLAC and
back with ffmpeg (a different header) joins into `s.2.dts` byte for byte.

## The meta file

Besides the headers and coding parameters, the meta file keeps what the WAV can't:

- a value too large for the WAV's samples, written to the WAV clipped;
- the LSB bits that overlap the MSBs where authoring cut the bit width (`nBitWidthAdjPerCh`), so
  that MSB and LSB parts don't add up to one sample value alone;
- with two bands, the last 7 lifted samples of band 0 in each frame: the band filter delays them
  into the next frame's output, so the frame's own WAV samples can't give them back.

These XLL frames are kept in the meta file as bytes:

- four frequency bands (sampling rates above 192 kHz);
- XLL frames that don't start at their asset's start (peak bit-rate smoothing spreading them over
  EXSS frames).

An EXSS frame before the first one with the static fields (a stream cut short at its start) has
its XLL frame found by its sync word, and is split all the same.

## Tests

Each stream below joins back byte for byte (`-v`, and `d` and `cmp`):

| Stream | Bytes | What it is | XLL frames split |
|---|---:|---|---|
| `s.2.dts` | 130554771 | DTS-HD MA 5.1, 24-bit, 48 kHz, 4:25, a 1509 kbps core; starts with 2443 bytes of a cut frame | 24855 of 24855 |
| `bond_sample_dtshdma.m2ts`, PID 0x1100 (samples.ffmpeg.org) | 748127 | DTS-HD MA 5.1 from a Blu-ray | 337 of 337 |
| `master_audio_7.1_24bit.dts` (FFmpeg's FATE) | 6598176 | 7.1: a 5.1 residual set and a 2-channel set coded without the core | 1172 of 1172 |
| `dcadec-suite/xll_51_24_48_768` | 25692 | 5.1, channels switching between residual and core-less | 7 of 7 |
| `dcadec-suite/xll_51_24_48_none` | 34460 | XLL without a core | 4 of 4 |
| `dcadec-suite/xll_71_24_48_768_0`, `_1` | 32888, 33520 | 7.1 with embedded downmixes, 2 and 3 channel sets (32-bit WAV) | 7 of 7 each |
| `dcadec-suite/xll_71_24_96_768` | 54184 | 7.1, 96 kHz | 7 of 7 |
| `dcadec-suite/xll_xch_61_24_48_768` | 32352 | 6.1, an XCH core | 7 of 7 |
| `dcadec-suite/xll_51_16_192_768_1` | 48140 | 5.1, 16-bit, 192 kHz: two frequency bands, a stereo downmix set and a 4-channel set (a 32-bit 192 kHz WAV) | 7 of 7 |
| `dtsx_7.1.4_24bit.dts` (FATE) | 17532 | DTS:X 7.1.4: the 7.1 in XLL channels (an embedded downmix, so a 32-bit WAV), the DTS:X data after the band data kept as bytes | 2 of 2 |
| `lotr_5.1_768.dts`, `dts_es.dts`, `core_51_24_48_768_0`, `x96_xxch_71_24_96_3840`, `xbr_xxch_71_24_48_3840`, `xxch_71_24_48_2046` | | core only, or EXSS without XLL | (no XLL) |
| `s.2.dts`'s first 3 MB, `master_audio_7.1_24bit.dts`, the first 3 MB again | 12598176 | a layout change: 5.1, 7.1, 5.1 | 2440 of 2440, into `prefix.wav` and `prefix.2.wav` |

`s.2.dts` uses pairwise decorrelation in every frame and adaptive prediction of orders 1-10. Its
frames have 2, 4 or 8 segments. Its segments use Rice, binary and hybrid Rice codes (1227 isolated
samples). One frame has four channels coded without the core.

Against FFmpeg's decodes (lossless, and core-only with `-flags +bitexact`), the WAV is exact in
every residual channel and every core-less channel:

- `s.2.dts`: on all 12725760 samples.
- `master_audio_7.1_24bit.dts`: on 1171 of its 1172 frames. In the first, FFmpeg outputs silence.
- the Blu-ray sample: on 328 of its 337 frames. FFmpeg's core decode fails on the last 9.

The band filter's forward steps match FFmpeg's (`assemble_freq_bands_c`) on 2000 random band
pairs, and its inverse gives them back. The overlapping-LSB path came back on 366 random LSB layouts
(fixed and variable widths); no sample has bit-width adjustment, nor an EXSS frame without static
fields, so the sync-word search was tested by ignoring the static fields of `s.2.dts`'s first 3 MB,
`master_audio_7.1_24bit.dts` and two dcadec streams: every frame split.

The sanitizer build (ASan, UBSan) went through 4000 mutated streams with `c -v` and 4000 damaged
`prefix.dtsc`, `.wav` or `.meta` files with `d`: the streams all came back, and `d` refused the
damaged files (exit 4, or 2 where the meta file names a WAV that isn't there) or joined them
correctly, with no crash, sanitizer report or hang.

On `s.2.dts`, `c -v` takes 6.6 s and `d` 2.6 s.

## Sizes

For `s.2.dts`, with xz `-9e` and FLAC (ffmpeg, `-compression_level 12`):

| | Bytes | |
|---|---:|---:|
| `s.2.dts` | 130554771 | 100% |
| `s.2.dts`, xz | 124891928 | 95.66% |
| `s2.dtsc` (the core), xz | 44109856 | 33.79% |
| `s2.meta`, xz | 1243884 | 0.95% |
| `s2.wav`, FLAC | 78014446 | 59.76% |
| the three | 123368186 | 94.50% |

FLAC on the residual comes to almost exactly what XLL's own band data takes (78014222 bytes): the
residual is close to noise, and XLL's per-frame LPC and Rice codes already do about what FLAC does.
The core is where the redundancy is (next).

## The lossy core: AC-3, AAC, or its own model

dtscore parses the core frames down to their quantization indexes and writes them back (all
24855 of `s.2.dts` come back bit for bit). The core spends its bits like this:

- **Sample indexes:** 81% of its bits, coded as plain words (61%) or block codes (20%), no
  Huffman codes at 1509 kb/s. The bit allocation (ABITS) goes from 3 levels to 21-bit indexes:
  3.5% of the subband data has ABITS 20-24.
- **Scale factors:** 7%, plain 7-bit indexes.
- **Bit allocations:** 4.4%, plain 5-bit words.
- **The rest:** ADPCM predictors, transient modes, high-frequency VQ, LFE, headers (8%).

**AC-3 can't hold it.** AC-3 doesn't send its bit allocation: the decoder derives each mantissa's
quantizer from the exponents and a few parameters, so DTS's freely chosen ABITS has nowhere to go.
DTS's 9, 13, 17 and 25-level quantizers have no AC-3 equal (AC-3 has 3, 5, 7, 11, 15, then powers
of 2). DTS's 21-bit indexes pass AC-3's 16-bit mantissas, its 1.1 dB scale factors are finer than
AC-3's 6 dB exponents, and ADPCM, the VQ and the transient modes have no counterpart. 1509 kb/s is
also past AC-3's 640 kb/s. A lossless mapping could only push DTS's fields through AC-3 syntax
that doesn't mean them, which an AC-3 model (ac3pack's) wouldn't fit.

**AAC could carry most of it, but gains little.** AAC sends what DTS sends, a code book per band
section and a scale factor per band, and its code books' ranges (1, 2, 4, 7, 12, 16 and escapes)
are close to DTS's level counts. But:

- 1.6% of `s.2.dts`'s groups of 8 indexes have an index beyond AAC's escape range (8191);
- AAC's scale factor bands aren't DTS's 32 equal subbands;
- the ABITS, ADPCM predictors, VQ and transient modes would need side information;
- the stream wouldn't decode as the audio (another filter bank, a power-law quantizer).

AAC's own Huffman code books, the cheapest one for each group of 8 indexes and no section data
counted, take 95.9% of the bits DTS's codes take for the same indexes. An entropy coder of the
indexes' magnitude classes by ABITS takes 81.8%.

**A model of the core's own fields** is where the gain is. dtscore's simple estimates for
`s.2.dts`:

- the samples at 81.7% of their bits;
- the scale factors, as deltas to the subband's previous frame, at 47.7%;
- the bit allocations at 27.6%;
- the whole core at 78.8%, against xz's 88.2%.

That is a model like ac3pack's for AC-3, and dtscore's parser (`dtscore.inc`) is its first part.

## dtscore

```
dtscore input.dts
```

It parses every core frame of the input down to its quantization indexes, writes it back and
compares it (frames that don't come back are counted, with the reason), then prints what each part
of the frames takes, the subband-subsubframes by coding and by ABITS, and the estimates above.

The parser takes the core's whole syntax but partial subsubframes (termination frames), and keeps
what follows the subframes (the optional information, XCh, X96 and XXCH data) as bits. Its 62
Huffman code books are TS 102 114's Annex D.5, extracted from the PDF's text by `dtshuff.py`
(`dtshuff.py ts_102114v010601p.txt dtscore_huff.inc`, the text from `pdftotext -layout`), each
checked complete and prefix-free. Two are misprinted, in V1.4.1, V1.5.1 and V1.6.1 alike:

- D65 stops at ±30. ±31 and ±32 take the four 15-bit codes left free.
- D129 has 37 twice. Code 1581 is -37.

All 62 match FFmpeg's decoder tables, entry for entry.

Every core frame of the test streams above (Blu-ray, FATE, a 768 kb/s DTS-ES track) comes back.
None of them codes anything with Huffman codes but TMODE (book D4). So 3634 of their frames were
re-coded with random Huffman books for their scale factors, ABITS, TMODE and samples, and parsed
back: every field came back.

## The core from the lossless audio

A DTS-HD MA stream carries the audio its core was coded from: the XLL decode is the encoder's
input, bit for bit. So the core can be re-derived from that audio, using the core's own side
information. dtsreq computes the audio's subband samples with the encoder's analysis filterbank
(the dual of the decoder's synthesis; FFmpeg's DCA encoder's transform, with the prototype filter
of the core's `FILTS`). It then quantizes them as the core's side information says: its bit
allocations, scale factors, transient modes, and ADPCM predictions from the decoded history. For
the whole of `s.2.dts`:

- **The analysis lines up** 512 samples ahead of the decoder's output, with a correlation of
  1.000000 (to 6 digits), and the decoder's gain is exactly 2.
- **99.91% of the 55.7 million quantization indexes come out exact.** The rest are off by 1:
  values within a hair of a rounding boundary. Given the audio, a coder would spend 76 KB on
  them, or 44 KB with a context of how near the boundary the audio's value is. That is 0.1% of the
  40.5 MB they take as coded.
- **The scale factors** are the table entry nearest to the RMS of what they scale (the ADPCM
  residuals where the band predicts) for 89.7% of them: 242 KB against 3.49 MB as coded.
- **The rest of the side information** is the encoder's own decisions and isn't predicted, but
  it is small: the bit allocations, 0.6 MB as deltas to the frame before; the ADPCM predictors,
  the high-frequency VQ vectors and the LFE, about 1.1 MB; the transient modes, prediction modes
  and headers, about 0.15 MB. The predictors aren't the code book's best for the subframe, and
  the VQ vectors are the nearest only 42% of the time.

So, given the audio, the core takes about 2.2 MB of its 50.0 MB (4%), where a model of the core
alone takes 78.8%. [dtsdpack](../dtsdpack) is such a coder, and goes further: it finds how the
encoder searched its code books (the ADPCM predictor is the entry nearest the LPC of the subframe
and the 4 samples before it, the VQ vector the best of the group of 32 whose mean is nearest), how
it decimated the LFE, and how its bit allocations follow the subbands' energies at the level its
rate control sets; the core takes 177503 bytes (0.35%).

That doesn't help dtssplit as it is. There the audio isn't stored: the WAV holds the residual,
which is the audio less the core's decode, and the core is what predicts the audio. It helps a
compressor that stores the audio and derives the core from it:

| `s.2.dts` | Bytes | |
|---|---:|---:|
| the core (xz), the residual (FLAC), the meta file (xz) | 123368186 | 94.50% |
| the same, the core by a model of its fields (78.8%) | about 118.7 MB | 90.9% |
| the audio (FLAC), the core given the audio, the meta file | about 110.4 MB | 84.5% |
| the audio (FLAC), the core by dtsdpack, the meta file (xz) | 108402865 | 83.03% |

The audio costs 29 MB more than the residual (106981478 against 78014446 bytes with FLAC), but
the core costs 37-42 MB less. In such a compressor:

- the XLL frames come back from the residual, the audio less the core's decode, as dtssplit
  rebuilds them now;
- that takes a bit-exact core decoder, the fixed-point one the XLL encoder used (FFmpeg's
  `-flags +bitexact` core decode is one);
- its two ends must compute the analysis identically: in integers, not in doubles as dtsreq does.

**Encoders.** FFmpeg's DCA encoder (`libavcodec/dcaenc.c`, from Alexander E. Patrakov's dcaenc) is
the open one. It codes a core of its own decisions: its own psychoacoustic model, no transient
modes, no VQ, no lookahead. It can't reproduce the cores of DTS's encoder, and doesn't need to.
Its own cores follow from the audio in the same way: dtsreq finds 96.7% of the indexes of its
1509 kb/s encode of `s.2.dts`'s first 3 MB exact, and the rest cost 33 KB of its 1060 KB. Its
quantizer is fixed point, and its scale factors follow the peaks, not the RMS.

## dtsreq

```
dtsreq input.dts full.wav
```

`full.wav` is the lossless decode of the input (`ffmpeg -i input.dts -c:a pcm_s24le full.wav`;
FFmpeg decodes XLL bit-exactly). dtsreq finds which of the WAV's channels is which of the core's,
and the alignment, by correlation. Then it compares every quantization index and scale factor
with the audio's (above), and prints:

- the alignment and the channel mapping;
- by ABITS, the indexes that come out exact, and those off by 1;
- the entropies of the indexes and scale factors, against those of their differences to the
  audio's.

Blocks whose analysis needs audio past the WAV's end are left out. The first frame of a stream cut
in the middle decodes wrong (its decoder history is missing), but that is too few samples to
show. `s.2.dts` takes 13 s.

## The meta file's format

`DTSSPLIT`, a version byte (2), then records. Each record is a tag byte and its fields; numbers
are LEB128 varints, signed ones zigzagged.

| Record | Fields |
|---|---|
| `R` | bytes that aren't a frame |
| `C` | a core frame: its size (its bytes are the next of `prefix.dtsc`) |
| `E` | an extension substream frame kept as bytes |
| `W` | WAV w's layout, before its first `X`: channels, width, rate, bands, speaker mask, the XLL channel of each WAV channel, the channel sets' sizes and masks |
| `X` | an EXSS frame with its XLL frame split, its samples in WAV w: the EXSS bytes before the XLL frame, the speaker-mapping flag, the XLL frame size, the XLL header bytes, each segment's coding parameters, the XLL bytes after the band data, the EXSS bytes after the XLL frame, band 0's delayed samples, the overlapping LSB bits, the clipped WAV values |
| `Z` | the end: the input's size and CRC-32, the WAVs and their samples a channel |

## Build

```
g++ -O2 -o dtssplit dtssplit.cpp
g++ -O2 -o dtscore dtscore.cpp
g++ -O2 -o dtsreq dtsreq.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -o dtssplit.exe dtssplit.cpp
```

Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
Profiles), clauses 5, 7 and 8. FFmpeg's decoder was the check for its behaviour. XLL's two-band
filter isn't in the specification: its lifting steps and coefficients are FFmpeg's
(`libavcodec/dcadsp.c`, `dcadata.c`), so `dtssplit.cpp` is licensed under the GNU Lesser General
Public License, version 2.1 or later. So are dtscore and dtsreq, which include it.
`dtsreq_tables.inc` has FFmpeg's tables of `dcadata.c`: the quantizer steps, scale factors, ADPCM
code book and QMF prototype filters.
