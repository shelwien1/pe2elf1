# dtssplit

dtssplit splits a DTS stream into two files and joins them back into the stream, byte for byte.
For DTS-HD Master Audio, the WAV gets the samples of the lossless extension (XLL), its prediction
undone. That is the part of the audio the lossy core leaves out, and a lossless audio compressor
can work on it. The other file, the side file, gets the rest: the lossy core frames, the headers
and the XLL coding parameters.

## Usage

```
dtssplit c [options] input.dts output.ac3 output.wav
dtssplit d [options] output.ac3 output.wav restored.dts
```

- `c` splits the stream into the side file (`output.ac3` above; its format is dtssplit's own, and
  any name does) and the WAV.
- `d` joins them back into the stream.
- `-v` (with `c`) joins the two files written and compares the result with the input.
- `-q` turns off the statistics.

The input of `c` and the output of `d` can be `-` for stdin and stdout, except with `-v`. The WAV
has to be a file, since its header is written last. `c` says what it found and where it went:

```
s.2.dts: 130554771 bytes: 24855 core frames, 24855 extension substream frames (24855 with XLL split), 2443 bytes between frames
s2.ac3: 53574359 bytes: core frames 50008260, extension substream headers 795360, XLL headers 1734486, XLL coding parameters 959202, frames kept as bytes 0, the rest 77051
s2.wav: 229063784 bytes: 12725760 samples a channel, 0 values clipped
verify: OK
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | damaged input (`d`): a side file or WAV that doesn't join, or joins into another stream |
| 5 | `-v` found a difference |

## What goes where

A DTS-HD MA stream is, frame by frame, a DTS core frame (lossy subband ADPCM, which any DTS
decoder plays) and an extension substream (EXSS) frame, which carries the XLL frame. For each channel, XLL codes either the difference between the original PCM and the core's
decode (a residual channel, the usual kind) or the PCM itself. It decorrelates channel pairs,
predicts each channel (adaptive LPC from reflection coefficients, or a fixed predictor) and codes
the prediction residuals with Rice, hybrid Rice and binary codes, segment by segment
(ETSI TS 102 114, clause 8).

- **Core frames:** to the side file as they are.
- **EXSS headers and the bytes around the XLL frame:** to the side file as they are.
- **The XLL frame's headers** (common header, channel set sub-headers, NAVI table, with their
  CRCs): to the side file as they are.
- **Each segment of each channel set:** its coding parameters go to the side file. These are the
  code choice, the Rice and binary code widths, where hybrid Rice codes put their isolated samples,
  and any nonzero padding bits. Its samples go to the WAV, after the entropy codes are decoded and
  the prediction, the pairwise decorrelation and the MSB/LSB split are undone.
- **Everything else** (bytes between frames, cut frames, files that aren't DTS): to the side file
  as it is.

The join predicts and codes the WAV's samples again with the side file's parameters. Before `c`
writes a frame split, it joins the frame back and compares it with the original. A frame that
doesn't come back the same, or that dtssplit doesn't split, goes to the side file as it is, and
the statistics say why. Any input therefore joins back to itself.

These XLL frames are kept as bytes:

- frequency bands above band 0 (sampling rates above 96 kHz, e.g. 192 kHz);
- MSB and LSB parts that overlap (bit-width adjustment by authoring);
- XLL frames that don't start at their asset's start (peak bit-rate smoothing);
- a channel layout unlike the first XLL frame's;
- EXSS headers that can't be followed (e.g. no static fields yet).

A core-only DTS stream goes to the side file whole, and the WAV is empty.

## The WAV

- **Samples:** one sample a channel for each sample of each split XLL frame, in stream order.
- **Channel order:** WAV order, from the XLL channel masks (C, L, R, Ls, Rs, LFE of a 5.1 set
  become FL, FR, FC, LFE, SL, SR). Channels without a WAV position (Lw/Rw, LFE2, ...) come last.
- **Sample width:** 24-bit; 16-bit for 16-bit streams. It is 32-bit where XLL embeds a downmix,
  since the lower channel sets then hold the downmix unscaled (27 bits in FFmpeg's 7.1 tests).
- **Residual channels:** the sample is the original minus the core's decode, at the XLL's
  precision. Each residual sample of `s.2.dts` equals FFmpeg's lossless decode minus its
  bit-exact core decode (`-flags +bitexact -core_only 1`). Over the whole of `s.2.dts` the
  residual is 51-63 dB below the audio in RMS. The LFE's is only 15 dB below: the core's LFE
  carries little above 150 Hz, so the residual holds the rest of that channel.
- **Channels coded without the core:** the sample is the audio.
- **Values too large for the sample width:** written clipped, with the true values kept in the
  side file.
- **WAVs over 4 GB:** written as RF64.

`d` reads any PCM WAV (RIFF or RF64, PCM or extensible format) with the same channels, width and
rate. That is what lossless audio compressors give back. For example, `s2.wav` through FLAC and
back with ffmpeg (a different header) joins into `s.2.dts` byte for byte.

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
| `dcadec-suite/xll_51_16_192_768_1` | 48140 | 5.1, 16-bit, 192 kHz: two frequency bands | 0 of 7 (kept as bytes) |
| `dtsx_7.1.4_24bit.dts` (FATE) | 17532 | DTS:X 7.1.4: the 7.1 in XLL channels (an embedded downmix, so a 32-bit WAV), the DTS:X data after the band data kept as bytes | 2 of 2 |
| `lotr_5.1_768.dts`, `dts_es.dts`, `core_51_24_48_768_0`, `x96_xxch_71_24_96_3840`, `xbr_xxch_71_24_48_3840`, `xxch_71_24_48_2046` | | core only, or EXSS without XLL | (no XLL) |

`s.2.dts` uses pairwise decorrelation in every frame and adaptive prediction of orders 1-10. Its
frames have 2, 4 or 8 segments. Its segments use Rice, binary and hybrid Rice codes (1227 isolated
samples). One frame has four channels coded without the core.

Against FFmpeg's decodes (lossless, and core-only with `-flags +bitexact`), the WAV is exact in
every residual channel and every core-less channel:

- `s.2.dts`: on all 12725760 samples.
- `master_audio_7.1_24bit.dts`: on 1171 of its 1172 frames. In the first, FFmpeg outputs silence.
- the Blu-ray sample: on 328 of its 337 frames. FFmpeg's core decode fails on the last 9.

The sanitizer build (ASan, UBSan) went through 3600 mutated streams with `c -v` and 3600 damaged
side files and WAVs with `d`: the streams all came back, and `d` refused the damaged files (exit
4) or joined them correctly, with no crash, sanitizer report or hang.

On `s.2.dts`, `c -v` takes 6.5 s and `d` 2.8 s.

## Sizes

For `s.2.dts`, with xz `-9e` and FLAC (ffmpeg, `-compression_level 12`):

| | Bytes | |
|---|---:|---:|
| `s.2.dts` | 130554771 | 100% |
| `s.2.dts`, xz | 124891928 | 95.66% |
| side file | 53574359 | 41.04% |
| side file, xz | 45562608 | 34.90% |
| WAV | 229063784 | |
| WAV, FLAC | 78014446 | 59.76% |
| side file xz + WAV FLAC | 123577054 | 94.66% |

FLAC on the residual comes to almost exactly what XLL's own band data takes (78014222 bytes). The
residual is close to noise, and XLL's per-frame LPC and Rice codes already do about what FLAC
does. A gain over XLL has to come from a stronger model of the residual. It could use what the
side file knows: the core's bit allocation and scale factors say how loud the core's quantization
noise is in each subband.

## The side file

`DTSSPLIT`, a version byte (1), then records. Each record is a tag byte and its fields; numbers
are LEB128 varints, signed ones zigzagged.

| Record | Fields |
|---|---|
| `R` | bytes that aren't a frame |
| `C` | a core frame |
| `E` | an extension substream frame kept as bytes |
| `W` | the WAV's layout, before the first `X`: channels, width, rate, speaker mask, the XLL channel of each WAV channel, the channel sets' sizes and masks |
| `X` | an EXSS frame with its XLL frame split: the EXSS bytes before the XLL frame, the speaker-mapping flag, the XLL frame size, the XLL header bytes, each segment's coding parameters, the XLL bytes after the band data, the EXSS bytes after the XLL frame, the clipped WAV values |
| `Z` | the end: the input's size and CRC-32, and the WAV's samples a channel |

## Build

```
g++ -O2 -o dtssplit dtssplit.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -o dtssplit.exe dtssplit.cpp
```

Written from ETSI TS 102 114 V1.6.1 (DTS Coherent Acoustics; Core and Extensions with Additional
Profiles), clauses 7 and 8. FFmpeg's decoder was the check for its behaviour.
