# dtssplit

dtssplit splits a DTS stream into three files (four for DTS:X) and joins them back into the
stream, byte for byte:

- `prefix.dtsc`: the core frames, a DTS stream that any DTS decoder plays (the lossy part);
- `prefix.wav`: the samples of the lossless extension (XLL, DTS-HD Master Audio), its prediction
  undone: the part of the audio the core leaves out, for a lossless audio compressor;
- `prefix.x.wav`: a DTS:X stream's object audio, the channel set its XLL frames carry after their
  band data (4 channels in the streams tried: 7.1.4's heights, it seems), decoded the same way
  (only where there is any);
- `prefix.meta`: all else, the extension headers and the XLL coding parameters, and every byte
  that is neither, small enough that it hardly matters what compresses it;
- `prefix.dtsx`: DTS:X object data of another form, as it is (only where there is any; none in the
  streams tried).

[dtscore](#dtscore) shows what the core frames are made of, and what a model of them could take.
[dtsreq](#dtsreq) shows how much of the core follows from the lossless audio: almost all of it.

## Usage

```
dtssplit c [options] input.dts prefix
dtssplit d [options] prefix restored.dts
```

- `c` splits the stream into `prefix.dtsc`, `prefix.wav` and `prefix.meta`, and a DTS:X stream's
  object audio into `prefix.x.wav` (object data of another form into `prefix.dtsx`). A stream whose
  XLL changes its channel layout gets a WAV for each further layout: `prefix.2.wav`,
  `prefix.3.wav`, ...
- `d` joins them back into the stream. It reads `prefix.x.wav` and `prefix.dtsx` only where the
  meta file says the stream has DTS:X object data (such a file left by an earlier split is
  ignored).
- `-v` (with `c`) joins the files written and compares the result with the input.
- `-q` turns off the statistics.

The input of `c` and the output of `d` can be `-` for stdin and stdout, except with `-v`. `c`
says what it found and where it went:

```
s.2.dts: 130554771 bytes: 24855 core frames, 24855 extension substream frames (24855 with XLL split), 2443 bytes between frames
s2.dtsc: 50008260 bytes, the core frames
s2.wav: 229063784 bytes: 12725760 samples a channel, 6 channels, 24-bit, 48000 Hz
s2.meta: 1080230 bytes: extension substream headers 795360, XLL headers 43368, XLL coding parameters 164422, DTS:X object headers and coding parameters 0, frames kept as bytes 0, bytes between frames 2446, the rest 74634; 0 values clipped, 0 overlapping LSB bits
verify: OK
```

A DTS:X stream's object audio gets a WAV of its own:

```
Nigel_Stanford_Cymatics_DTS-X.dts: 321415780 bytes: 30695 core frames, 30695 extension substream frames (30695 with XLL split), 0 bytes between frames
cym.dtsc: 61758340 bytes, the core frames
cym.wav: 502906984 bytes: 15715840 samples a channel, 8 channels, 32-bit, 48000 Hz
cym.x.wav: 188590184 bytes: 15715840 samples a channel, 4 channels, 24-bit, 48000 Hz: the DTS:X object audio of 30695 XLL frames
cym.meta: 1733244 bytes: extension substream headers 1227800, XLL headers 73916, XLL coding parameters 245032, DTS:X object headers and coding parameters 94307, frames kept as bytes 0, bytes between frames 0, the rest 92189; 0 values clipped, 0 overlapping LSB bits
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
  sub-headers, NAVI table): to the meta file, but for what the join computes again. Their CRC-16s
  are 0s there. The EXSS frame size, the XLL frame size, the channel set sub-headers' sizes and the
  NAVI table's segment sizes are XORed with what the join will compute: the frames' sizes, the bytes
  the sub-headers' fields take (all of them in every stream tried: format 8) and the bytes each
  segment's coding takes. The reflection coefficients' codes (the prediction's) are kept as the
  difference of their table indexes from those an analysis of the WAV's samples gives
  (`rc_predict()`: their autocorrelation under Hann's window, Levinson's recursion in single
  precision, each coefficient's size quantized, from the second on to 121 at most; a miss is mostly
  by one, which the difference keeps small where XOR, as format 7 has it, didn't). The table's
  entries are tanh((i + 1/2)/32), and the encoders' code is floor(32 atanh(|k|)), the last i whose
  tanh(i/32) the size reaches, not the nearest entry (format 10). From format 15 the autocorrelation
  is the encoders', it seems: the window 0.5 - 0.5 cos(2 pi n / (N - 1)) in double precision, each
  windowed sample rounded to single precision, each lag's products in single precision, summed in
  double precision, the sum rounded to single precision. With it the codes come out the same in all
  of `3-1.dts` and the first channel set of `Nigel_Stanford_Cymatics_DTS-X.dts` (of its first 60 MB)
  and in all but 44 of the 885113 of `s.2.dts`, where the autocorrelation in integers before got
  100%, 99.9% and 99.3%; in 99.3% of those of the Cymatics stream's second set and 99.8% of its
  object audio's (92.5% and 93.5% before), and in 99.95% and 97.4% of those of the two sets of
  `master_audio_7.1_24bit.dts` (99.95% and 97.1%). The second set's misses had been mostly where the
  first coefficient is all but -1, where 32 atanh(|k|) is steep and the code turns on R's last bits;
  the analysis in double precision, with exact sums, by Schur's recursion, with other orders of the
  operations, from other parts of the samples, R(0) raised or a lag window missed more of them. The
  codes were about two thirds of what the Cymatics stream's XLL headers took with xz: format 15
  takes its meta file from 292292 bytes with xz to 205592, that of `s.2.dts` from 61180 to 51912
  (format 10's quantizer took them from 427644 to 392200 and from 89028 to 78872). The cosine is
  computed in 124-bit fixed point and rounded to the nearest double (glibc's own misses 54 of the
  32764 angles of the windows of 4, 8, ... 16384 samples, by one unit), and every operation is
  rounded where it is, with no fused or wider arithmetic, so the split and the join compute the same
  codes on any machine. With the bytes they are XORed with mostly 0s, the meta file is no smaller,
  but it compresses to a third (format 5; format 4 kept the headers as they are).
  The join computes the XLL frame's size from the parts it puts together (format 7), where the bytes
  after the band data are 0s to the next DWORD boundary: in every frame of the DTS:X streams,
  `s.2.dts` and FATE's dcadec streams, but in 57-77% of those of `master_audio_7.1_24bit.dts`,
  `3-1.dts` and the two Master Audio samples, whose other frames have stale bytes there (the meta
  file keeps them). In a DTS:X stream it computes the object data's size too, where the bytes after
  its band data are 0s to the DWORD boundary past 2 more bytes (all of them, in both streams). The
  EXSS header's fields that hold the XLL frame's size (two in every stream tried) or the DTS:X
  object data's offset or size (in a part of the asset descriptor that the specification doesn't
  describe) are kept less those: the split finds them in the first 8 frames of distinct sizes, as
  the 16 bits that are the same less a size in all of them, and an `F` record says where they are.
  That takes the Cymatics stream's meta file from 1054156 bytes with xz to 722100, and that of
  `s.2.dts` from 282692 to 204176; format 8's computed sub-header sizes and index differences take
  them to 678020 and 200204.
  From format 9 the join reads the frame's samples before its header, and the pairwise
  decorrelation's channel order and coefficients and the prediction orders of a channel set of one
  band are XORed with what those samples say. The encoders pair the channels as `dc_order()` does,
  it seems: pair after pair, the two left whose correlation is the largest in size, the less loud
  first; and a pair's coefficient, where it isn't 0, is their least squares fit in 8ths, rounded
  (`dc_code()`). Both hold in every frame of nearly every stream tried (whether the coefficient is
  0, the meta file keeps). Each channel's prediction order is the one, 0 to 10, least in (N/2)
  max(E, F) + L k, E the log2 of the variance Levinson's recursion leaves at order k: the residual's
  cost in bits against L bits a coefficient, with a floor F for quiet channels (`ao_pred()`). L and
  F go by the channel: the split and the join both count which of 18 pairs of them would have
  predicted its past orders (60 from format 14, below). The best of them predicts 96.5% of the
  orders of the Cymatics stream's first channel set (those of its first 60 MB; 96.3% of its LFE's),
  93.2% of its second set's, 94.6% of its object audio's, with E from format 15's autocorrelation
  (above; 85.4% and 86.6% with that in integers before). Format 9 takes the meta file of `s.2.dts`
  from 200204 bytes with xz to 89028, the Cymatics stream's from 678020 to 427644. From format 13
  the orders are kept as their difference from those (mod 16, zigzagged), as the code widths are
  (below): a miss by one, the usual kind, is then 1 or 2, where XOR made it anything from 1 to 15.
  Format 14's pairs are L 5 to 18 bits in 12 steps by F none, 1, 2, 3 or 4, and a channel's counts
  are halved when one passes 64, so that they follow the orders' recent rule: it takes the meta file
  of `s.2.dts` from 63744 bytes with xz to 61180, the Cymatics stream's from 303912 to 292292.
  From format 17 the orders are kept as their rank among the orders by that cost under the channel's
  best pair, the cheapest first (0 the order `ao_pred()` predicts; those past the ones it weighs
  last): most misses are near-ties, the next cheapest, rank 1 (89% of the misses of the Cymatics
  stream's first channel set in its first 60 MB, 64% of its second set's, 81% of its object
  audio's), where a difference of one, 1 or 2 as format 13 keeps it, is 40%, 17% and 25% of them.
  Their entropy comes to 0.24 bits an order where their difference's was 0.33, 0.49 where it was
  0.61, and 0.36 where it was 0.50.
  From format 12 the headers' bytes go to the meta file without their count: the join parses them
  from the bytes ahead, and their own fields say where they end. That count, which the prediction
  orders mostly set (each order a reflection coefficient's 8 bits), had come to a seventh of the
  meta file with xz: format 12 takes that of `s.2.dts` from 77536 bytes to 65596, the Cymatics
  stream's from 360700 to 309328.
- **Each segment of each channel set in each band:** its coding parameters go to the meta file: the
  code choice, the Rice and binary code widths, where hybrid Rice codes put their isolated samples,
  band 1's decimator history, and any nonzero padding bits. The code widths are kept against those
  the residuals they code give (`ck_xor()`: XORed with them, from format 13 as their difference mod
  the field's size, zigzagged): a Rice code's the floor of the log2 of the mean of the zigzagged
  residuals, a binary code's the least that holds them all, which is how the encoders choose them,
  it seems (99.2% of the Rice widths of `s.2.dts` and of the Cymatics stream, 98-99% of those for a
  segment's first residuals, before the prediction's order, and all but a few binary widths). That
  makes the meta file a quarter smaller with xz (format 6). A hybrid Rice code's isolated samples,
  coded apart in aux bits each, are those whose Rice code, (z >> k) + 1 + k bits, would be longer
  than that and their position's bits (the log2 of the segment's samples): every one of them in the
  Cymatics stream's first 60 MB and `master_audio_7.1_24bit.dts` (0.7% and 2.8% of their Rice codes
  are hybrid; 6 of those of `3-1.dts` and the two Master Audio samples), and aux is the width of the
  largest residual (99.2%). From format 11 the meta file keeps aux XORed with that, and the isolated
  samples only where they are not those (none so far); the Rice width is then XORed with that of the
  residuals but those a first guess isolates (k the floor of the log2 of the mean of them all),
  which the join knows before the isolated samples: 85% and 91% of the two streams' hybrid Rice
  widths (95% and 90% before, from the residuals but the isolated ones). Format 11 takes the
  Cymatics stream's meta file from 392200 bytes with xz to 360700, `master_audio_7.1_24bit.dts`'s
  from 15804 to 10744 and that of `s.2.dts` from 78872 to 77536. Format 13's differences, these and
  the prediction orders', take that of `s.2.dts` from 65596 bytes to 63744 and the Cymatics stream's
  from 309328 to 303912. From format 17 the Rice and hybrid flags of a set of one band are kept
  XORed with those the segment's residuals give (`fp_set()`), as the encoders choose them, it seems:
  a Rice code where its bits, k the floor of the log2 of the mean of the zigzagged residuals, are no
  more than a binary code's at the largest one's width (99.94% of the Rice flags of the Cymatics
  stream's first 60 MB, 99.1% of those of `master_audio_7.1_24bit.dts`), and, where the parameters
  are the channel's own, a hybrid Rice code where isolating the samples `iso_rule()` takes at that
  width saves bits, their count and width included (864 of the first's 1152 hybrid codes, and 41
  that aren't; 567 of the second's 586, and none that aren't). The join reads those parameters once
  it has the residuals; a set of two bands has them before, as band 1's decimator history is among
  them. With the orders' ranks (above) and the object data's isolated samples a section of their own
  (below), format 17 takes the meta file of `s.2.dts` from 43852 bytes with xz to 41672 and the
  Cymatics stream's from 169712 to 153936. Its samples go to the WAV, after the entropy codes are
  decoded, the prediction, the pairwise decorrelation and the MSB/LSB split are undone and the two
  frequency bands are put together.
- **DTS:X object data:** in a DTS:X stream, each XLL frame goes on past the band data, from a sync
  word of its own at a DWORD boundary (0x02000850; 0xF14000D0 for IMAX Enhanced's). No published
  specification describes what follows (FFmpeg looks for the sync word only to name the profile).
  In both DTS:X streams tried it is a header of 21 bytes with its CRC-16, the same in every frame,
  a byte (2), then one more XLL channel set, coded as the frame's own sets are and by the frame's
  common header (its segments and their sizes' width): its sub-header (4 channels, 24-bit, 48 kHz,
  pairwise decorrelation, no speaker positions), NAVI table and band data, then 0s to a DWORD
  boundary. dtssplit decodes that set as it does the others, its samples to `prefix.x.wav`, the
  21 bytes, the sub-header, NAVI table and coding parameters to the meta file. FATE's sample is
  DTS:X 7.1.4 with a 7.1 XLL, so these 4 channels would be its heights. Object data of another
  form goes to `prefix.dtsx` as it is, frame after frame, and the meta file keeps its size. In
  `Nigel_Stanford_Cymatics_DTS-X.dts` the object data is 96 MB of the 321, 3130 bytes a frame,
  which xz doesn't shrink at all; until the meta file's format 3 it went to the meta file (then 103
  MB), and in format 3 to `prefix.dtsx`.
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

From format 16 the meta file keeps the XLL header bytes of each frame and of its DTS:X object data,
and the segments' coding parameters, apart from the rest of the frame's record, the coding
parameters in three: the flags (the code, hybrid or not, the parameters reused or common) and the
code widths, the frame's and the object data's apart, and the hybrid codes' isolated samples and the
rest. Those seven and the records are the eight sections of a block, the block its sections' sizes
and then their bytes (the split ends a block once it holds 16 MB, and the join reads a block at a
time). Alike bytes then follow alike bytes, and xz takes the meta file of `s.2.dts` from 51912 bytes
to 43852, the Cymatics stream's from 205592 to 169712, `master_audio_7.1_24bit.dts`'s from 8748 to
7992 and `Master_Audio_5.0_96khz.dts`'s from 6752 to 6132; those of the smallest streams come to 4-8
bytes more, the block's sizes. From format 17 the object data's isolated samples and the rest are a
ninth section (the Cymatics stream's meta file 152 bytes smaller with xz).

From format 18 a set of one band's prediction orders' ranks and its pairs' flags for a coefficient
go to a tenth section, range-coded (LZMA's binary coder, its probabilities in 4096ths moving 1/32 of
the way to each decision), the fields' bits in the header 0s: a rank as whether it is 0, then
whether it is 1, in the context of the set and of how much cheaper the cheapest order is than the
next by `ao_pred()`'s cost (the log2 of the bits between them, to 14), and from 2 as 4 bits by the
set; a pair's flag in the context of the set, the pair, the flags of the 3 pairs before it in the
frame, its own in the set's last frame and the size of the coefficient `dc_code()` gives it (to 3).
A record's decisions are coded with the probabilities as they were at its start, the split's once
its check has joined the record back (the join's parse of a header taking them again where it reads
more of the header), and the probabilities start again in each block. Format 18 takes the meta file
of `s.2.dts` from 41672 bytes with xz to 35560, the Cymatics stream's from 153936 to 137392 and
`Master_Audio_5.0_96khz.dts`'s from 6060 to 5264 (the 27 streams of the regression set: from 105356
to 97292).

From format 19 the XLL headers, the frame's and the object data's, go to an eleventh section as
their fields, after the XORs above, range-coded as the join's parse reads them (a binary coder of
16-bit probabilities, each context's the Krichevsky-Trofimov estimate of its decisions, to 4096 of
them, then a running one), the bits no field takes after them (the CRC-16s still the join's to
compute). A field's context is what it is: its part of the header (the common header, a channel
set's sub-header, a band's parameters, its pairs, its orders, its fixed orders, a channel's
reflection coefficients, the LSB part, the NAVI table), the channel set, the band or the channel,
its place there, and the frame's or the object data's. It is coded as whether it is the value the
same field had in the last frame, in the context of the field and of how many of the header's fields
so far weren't (0, 1, more); if not, whether it is 0; if not, as its bits, by a binary tree of its
part's for up to 8 of them. The join reads a frame's samples once its parse has the common header,
which says how many there are. The orders' ranks and the pairs' flags keep the tenth section, with
the new coder. Format 19 takes the meta file of `s.2.dts` from 35560 bytes with xz to 16600 (its
headers from 20536 to 1616), the Cymatics stream's from 137392 to 68224 (its headers and its object
data's from 87396 to 16288) and the 27 streams of the regression set from 97292 to 73388; those of
the smallest FATE streams come to 4-12 bytes more.

From format 20 the coding parameters are range-coded too, as fields keyed as the headers' are: each
segment's flags (its parameters the last segment's or not, common to its channels or not, its
padding bits kept or not) and each channel's (the code, hybrid or not, XORed with `fp_set()`'s),
then the code widths, as their difference from `ck_xor()`'s rule, coded where the join computes the
rule, as whether it is 0, then its bits by a binary tree, in the context of the part (the residuals
before the prediction's order or the rest), the code and how near the rule's Rice width is to its
neighbours: the bits a Rice code of those residuals takes at one less and one more than at it, each
as fewer, or as how many sixteenths of a bit a residual more (to 7 and past), or no such width. The
rule's misses are nearly all one less, where that is fewer bits or nearly as few: in `s.2.dts`, past
the prediction's order, 12% of the segments where one less is fewer bits, 4.0% where it is under a
sixteenth of a bit a residual more, 1.1% under 2/16 and 0.07% under 3/16, next to none further
(where a few miss by one more). The frame's go to the fourth section, the object data's to the
fifth; the widths' sections are empty. Format 20 also takes the reflection coefficients' recursion
on past where it breaks down (the error no more than 0, a coefficient past 1), as the encoders do,
the codes from the second at most 121 where they are positive and 127 where they are negative: so
are all 320 channels of the Cymatics stream's first 60 MB where format 19's codes missed (it stopped
there, and capped the negative at 121). And its coders' estimates count fewer decisions, which all
three sections took less with: 30 for the ranks and pairs, 30 for the coding parameters, 14 for the
headers' fields (4096 before). Format 20 takes the meta file of `s.2.dts` from 16600 bytes with xz
to 13340, the Cymatics stream's from 68224 to 48604 and the 27 streams of the regression set from
73388 to 68804.

From format 21 whether a segment's parameters are common to its channels is coded against the
encoders' choice, it seems (`cm_rule()`): common where the segment takes fewer bits so, its
parameters and its residuals by `fp_set()`'s codes at `ck_xor()`'s widths, than with each channel's
own, ties their own (99.8% of the Cymatics stream's segments; none of the streams here reuses the
last segment's parameters), in the context of how many bits it is by. `fp_set()`'s rules are
corrected: a binary code's bits are those of its true width (a width of 1 takes 2), a Rice code of a
channel's own pays its hybrid flag, ties go to the binary code, and a hybrid code's bits are at the
width of the residuals but those a first guess would isolate, as `ck_xor()`'s (the Cymatics stream's
misses: the Rice flags' 272 to 210, the hybrid flags' 1630 to 784; `master_audio_7.1_24bit.dts`'s
Rice flags' 174 to 12). And the Rice width of the residuals before the prediction's order, the
samples themselves, is by the mean of 2|r| - 1, 0s as 0 (the Cymatics stream's misses there 3066 to
987, `s2_mid.dts`'s 8 to 0); past them a second rule, by their sum less half the count of the
positive ones and the 0s (a hybrid code's less the count of the positive ones), is the widths'
context where it differs from the first: the misses are where the mean is just past a power of 2,
the more of them the smaller it is, as if the encoders' sum were a little smaller. Format 21 takes
the meta file of `s.2.dts` from 13340 bytes with xz to 11636, the Cymatics stream's from 48604 to
36504, `master_audio_7.1_24bit.dts`'s from 4988 to 4404 and the 27 streams of the regression set
from 68804 to 65836, none bigger.

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
| `dtsx_7.1.4_24bit.dts` (FATE) | 17532 | DTS:X 7.1.4: the 7.1 in XLL channels (an embedded downmix, so a 32-bit WAV), the DTS:X object data after the band data a 4-channel set, to `prefix.x.wav` (the meta file 675 bytes; with the object data as bytes, 475 and a `prefix.dtsx` of 4588) | 2 of 2 |
| `Nigel_Stanford_Cymatics_DTS-X.dts` | 321415780 | DTS-HD MA 7.1 and DTS:X, 5:27: an embedded downmix (a 32-bit WAV), 96 MB of DTS:X object data, a 4-channel set in each frame, to `prefix.x.wav` | 30695 of 30695 |
| `lotr_5.1_768.dts`, `dts_es.dts`, `core_51_24_48_768_0`, `x96_xxch_71_24_96_3840`, `xbr_xxch_71_24_48_3840`, `xxch_71_24_48_2046` | | core only, or EXSS without XLL | (no XLL) |
| `s.2.dts`'s first 3 MB, `master_audio_7.1_24bit.dts`, the first 3 MB again | 12598176 | a layout change: 5.1, 7.1, 5.1 | 2440 of 2440, into `prefix.wav` and `prefix.2.wav` |

Meta files of format 4 (the headers as they are), 3 (before `prefix.x.wav`) and 2 (before
`prefix.dtsx`) still join: each stream above's, made by the versions before, joins back with this
one. The split's other outputs are the same as format 4's (and, without DTS:X object data, as
format 3's); the meta file is the same size as format 4's.

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
correctly, with no crash, sanitizer report or hang. So did 800 mutated DTS:X streams (FATE's
`dtsx_7.1.4_24bit.dts`, and the first 300 KB of `Nigel_Stanford_Cymatics_DTS-X.dts`) with `c -v` and
800 damaged split files with `d`, `prefix.dtsx` among them (exit 2 where it isn't there). With the
object audio split, 600 mutated streams (FATE's `dtsx_7.1.4_24bit.dts`, 400 KB from the middle of
the Cymatics stream, `xll_71_24_48_768_0`) with `c -v` and 600 damaged split files with `d`,
`prefix.x.wav` among them: the same. With format 5's computed header fields, 800 more (those three,
`xll_51_16_192_768_1` with its two bands, `xll_xch_61_24_48_768`): the same, and so did 800 more
with format 6's code widths, 800 with format 7's computed sizes (`s2_mid.dts`, 3 MB of `s.2.dts`,
for `xll_xch_61_24_48_768`), 800 with format 8's (`master_audio_7.1_24bit.dts` for `s2_mid.dts`),
800 with format 9's (`s2_mid.dts` again for `master_audio_7.1_24bit.dts`), 800 with format 10's
(`master_audio_7.1_24bit.dts` and `3-1.dts` for `xll_51_16_192_768_1` and `s2_mid.dts`), 800 with
format 11's (`xll_71_24_96_768` and `xll_xch_61_24_48_768` for `xll_71_24_48_768_0` and `3-1.dts`),
800 with format 12's (`s2_mid.dts`, `Master_Audio_7.1.dts` and `xll_51_16_192_768_1` for
`master_audio_7.1_24bit.dts`, `xll_71_24_96_768` and `xll_xch_61_24_48_768`), 800 with format
13's (those three back for the three before them) and 800 with format 14's (300 KB of the Cymatics
stream, `s2_mid.dts`, `master_audio_7.1_24bit.dts`, `Master_Audio_5.0_96khz.dts` and
`xll_51_16_192_768_1`), 800 with format 15's (400 KB from the middle of the Cymatics stream,
`s2_mid.dts`, `Master_Audio_7.1.dts`, `3-1.dts` and `xll_71_24_96_768`), 800 with format 16's
(the same five), 800 with format 17's (the same five), 800 with format 18's (the same five), 800
with format 19's (the same five), 800 with format 20's (the same five) and 800 with format 21's (the
same five). Builds with `-O0`, `-O2` and `-O3 -march=native -ffp-contract=fast` (fused
multiply-adds) write the same meta files, and each joins the others'.

On `s.2.dts`, `c -v` takes 11.5 s and `d` 4.2 s, as with format 14 (format 8: 8.4 s and 3.1 s;
format 4: 5.6 s and 2.0 s); on the Cymatics stream, `c -v` 32 s and `d` 12 s (format 14: 30 s and
10.6 s; format 8: 23 s).

## Sizes

For `s.2.dts`, with xz `-9e` and FLAC (ffmpeg, `-compression_level 12`):

| | Bytes | |
|---|---:|---:|
| `s.2.dts` | 130554771 | 100% |
| `s.2.dts`, xz | 124891928 | 95.66% |
| `s2.dtsc` (the core), xz | 44109856 | 33.79% |
| `s2.meta`, xz | 11636 | 0.01% |
| `s2.wav`, FLAC | 78014446 | 59.76% |
| the three | 122135938 | 93.55% |

(The meta file's format 4 came to 1243884 bytes with xz, format 5's to 386896, format 6's to 282692,
format 7's to 204176, format 8's to 200204, format 9's to 89028, format 10's to 78872, format 11's
to 77536, format 12's to 65596, format 13's to 63744, format 14's to 61180, format 15's to 51912,
format 16's to 43852, format 17's to 41672, format 18's to 35560, format 19's to 16600, format 20's
to 13340.)

The Cymatics stream's DTS:X object data, 96021900 bytes as it is, xz no smaller: as `prefix.x.wav`
(188590184 bytes) through WavPack (ffmpeg, `-compression_level 3`), 92545003 bytes (FLAC
`-compression_level 12`, 97067662), and 94 KB more of the meta file (1733244 bytes; with xz 36504,
where format 3's was 2665280 without them, format 4's 4104040 with them, format 5's 1446656, format
6's 1054156, format 7's 722100, format 8's 678020, format 9's 427644, format 10's 392200, format
11's 360700, format 12's 309328, format 13's 303912, format 14's 292292, format 15's 205592, format
16's 169712, format 17's 153936, format 18's 137392, format 19's 68224 and format 20's 48604). Of
its 4 channels, two follow the other two's pair closely (correlation 1.00), and the second pair the
WAV's back surrounds (0.70 and 0.77; a linear fit of the WAV's channels leaves 40% and 50% of their
energy), which a compressor that sees both WAVs can use.

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
encoder searched its code books (the ADPCM predictor is the nearest entry of the block of 64 whose
codeword, of 64 the encoder keeps, is nearest the LPC of the subframe and the 4 samples before it;
the VQ vector the best of a group of 32 that a learned score picks),
how it decimated the LFE, and how its bit allocations follow one table of the subbands' energies
(a transient's from the transient on) at the level its rate control sets, filling its frames; the
core takes 76413 bytes (0.15%).

That doesn't help dtssplit as it is. There the audio isn't stored: the WAV holds the residual,
which is the audio less the core's decode, and the core is what predicts the audio. It helps a
compressor that stores the audio and derives the core from it:

| `s.2.dts` | Bytes | |
|---|---:|---:|
| the core (xz), the residual (FLAC), the meta file (xz) | 122135938 | 93.55% |
| the same, the core by a model of its fields (78.8%) | about 118.7 MB | 90.9% |
| the audio (FLAC), the core given the audio, the meta file | about 110.4 MB | 84.5% |
| the audio (FLAC), the core by dtsdpack, the meta file (xz) | 107069527 | 82.01% |

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

`DTSSPLIT`, a version byte (21; `d` reads 20 too, which keeps the common flags as they are, format
17's Rice and hybrid rules and the Rice widths by one rule, 19, which keeps the coding parameters as
bytes and the reflection coefficients' codes against those of the recursion stopped where it breaks
down, 18, which keeps the XLL headers as their bytes and the ranks' and pairs' decisions by LZMA's
coder, 17, which keeps the prediction orders' ranks and the pairs' flags in the headers, 16, which
keeps the prediction orders as their difference from `ao_pred()`'s, the Rice and hybrid flags as
they are and the isolated samples in one section, 15, which keeps its sections one stream, 14, which
keeps the reflection coefficients' and the prediction orders' autocorrelation in integers, 13, which
keeps `ao_pred()`'s 18 pairs and counts that aren't halved, 12, which keeps the prediction orders
and the code widths XORed with their predictions, 11, which keeps the XLL headers' sizes, 10, which
keeps hybrid Rice codes' isolated samples as they are, 9, which keeps the reflection coefficients'
codes against the nearest table entries, 8, which keeps the pairwise decorrelation's channel order
and coefficients and the prediction orders as they are, 7, which keeps the channel set sub-header
sizes as they are and the reflection coefficients' codes XORed with `rc_predict()`'s, 6, which keeps
the XLL frame sizes, 5, which keeps the code widths as they are, 4, which keeps the EXSS and XLL
headers as they are, 3, whose `X` records keep all DTS:X object data in `prefix.dtsx` and whose `Z`
record ends with its size, and 2, whose `X` records keep the XLL bytes after the band data whole and
whose `Z` record ends after the WAVs), then blocks: each the sizes of its eleven sections (LEB128
varints), then their bytes: the records, the frames' XLL header bytes, those of their DTS:X object
data, the segments' coding parameters' flags, the object data's, their code widths, the object
data's, their isolated samples and the rest, the object data's, the prediction orders' ranks and the
pairs' flags, range-coded, and the headers' fields, range-coded, a record's parts all in one block.
Each record is a tag byte and its fields; numbers are LEB128 varints, signed ones zigzagged. The
EXSS and XLL header bytes are as the stream has them but for their CRC-16s (0s), the EXSS and XLL
frame sizes (XORed with the frames', less 1), the channel set sub-headers' sizes (XORed with those
their fields take, less 1), the pairwise decorrelation's channel order and coefficients (XORed with
`dc_order()`'s and `dc_code()`'s; the pairs' flags 0s, range-coded in the tenth section) and the
prediction orders (0s, their rank among the orders by `ao_pred()`'s cost range-coded in the tenth
section) of a set of one band, the NAVI tables' sizes (XORed with those the segments' coding takes,
less 1), the reflection coefficients' codes (their table indexes less `rc_predict()`'s, zigzagged,
as 8 bits) and the EXSS fields an `F` record names (less the sizes they hold); a set of one band's
segments' Rice and hybrid flags are XORed with `fp_set()`'s (and from format 21 its flags for
parameters common to its channels with `cm_rule()`'s); the segments' Rice and binary code widths and
hybrid Rice codes' isolated samples' widths are kept as their difference from `ck_xor()`'s (mod the
field's size, zigzagged), and their isolated samples' count is 0 where they are those `ck_xor()`'s
rule gives, else their count and 1; from format 20 the flags and the widths so changed are
range-coded, the frame's in the fourth section and the object data's in the fifth, the sixth and
seventh sections empty. The XLL header bytes, the frame's and the DTS:X object data's, go without
their count, which the join's parse of them gives; from format 19 as their fields so changed,
range-coded in the eleventh section as that parse reads them, the bits no field takes after them
(the two header sections empty).

| Record | Fields |
|---|---|
| `R` | bytes that aren't a frame |
| `C` | a core frame: its size (its bytes are the next of `prefix.dtsc`) |
| `E` | an extension substream frame kept as bytes |
| `W` | WAV w's layout, before its first `X`: channels, width, rate, bands, speaker mask, the XLL channel of each WAV channel, the channel sets' sizes and masks |
| `O` | `prefix.x.wav`'s layout, before the first `X` with object audio: 0, then as `W`'s |
| `F` | the EXSS header's fields that hold the XLL frame's size, the DTS:X object data's offset in it or its size: their count, then each one's first bit and which (0, 1, 2); the `X` records after it keep these 16 bits less that |
| `X` | an EXSS frame with its XLL frame split, its samples in WAV w: the EXSS bytes before the XLL frame, the speaker-mapping flag, the XLL header bytes, each segment's coding parameters, the XLL bytes after the band data up to the DTS:X object data (1 where the object data follows, plus 2 where they are 0s to the DWORD boundary, else plus their count times 4; the bytes), then where there is object data whether it is split: if so its bytes before its band data, its segments' coding parameters, its bytes after its band data (0 where they are 0s to the DWORD boundary past 2 more bytes, else their count plus 1; the bytes), its band 0's delayed samples, overlapping LSB bits and values clipped in `prefix.x.wav` (its samples the next of `prefix.x.wav`), else its size (its bytes the next of `prefix.dtsx`); the EXSS bytes after the XLL frame, band 0's delayed samples, the overlapping LSB bits, the clipped WAV values |
| `Z` | the end: the input's size and CRC-32, the WAVs and their samples a channel, the size of `prefix.dtsx`, `prefix.x.wav`'s samples a channel |

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
