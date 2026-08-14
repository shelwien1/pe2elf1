# mp2 — lossless MP2 → MP3-container converter

`mp2.cpp` repacks the spectral data of an MPEG audio **Layer II** stream into a
syntactically valid **Layer III** (mp3) bitstream, and puts everything that has
no place in the mp3 syntax into a small side file. The transform is exactly
invertible: `c` followed by `d` reproduces the input byte for byte.

```
mp2 c input.mp2 output.mp3 output.meta     pack
mp2 d input.mp3 input.meta output.mp2      unpack
```

The mp3 is not meant to sound like the source — it is a container. It *is*
a real mp3 though: every frame parses with an ordinary decoder, with correct
headers, side info, region/table selection and bit reservoir.

## Build

```
sh g2.sh          # linux   -> ./mp2
g2.bat            # windows -> mp2.exe (MinGW)
```

`mp2.cpp` includes `mp3_A.h` (the decoder that came with the task) for header
parsing and the Layer II allocation tables, plus `mp3_htabs.h`, which is
`mktabs.py`'s verbatim extraction of the tables that `mp3_A.h` hides inside
function bodies (`python3 mktabs.py` regenerates it).

## What goes where

| Layer II element | destination |
| --- | --- |
| subband samples | mp3 Huffman spectral data |
| frame header (4 bytes) | meta, `HDRS` |
| CRC (if protected) | meta, `CRCS` |
| bit allocation + scfsi + scalefactors | meta, `PREFIX` (copied as a raw bit region) |
| ancillary/padding bits at the end of a frame | meta, `TAIL` |
| top bits of 15/16-bit samples | meta, `HIBITS` |
| bytes that are not part of a frame (ID3, junk, gaps) | meta, `GAPS` |

The bit allocation, scfsi and scalefactors form one contiguous bit region at the
start of a Layer II frame, so the meta file simply copies those bits out. They
are also *parsed*, because the allocation determines how many sample codes
follow and how wide each one is — which is what lets the unpacker know how many
spectral values to pull back out of the mp3.

Nothing is duplicated: on the supplied sample the meta file is 922 669 bytes
against 922 588 bytes of non-sample data in the source mp2, i.e. 81 bytes of
framing overhead for the whole file.

## How the samples map onto mp3 spectral lines

Both layers are built on the same 32-band polyphase filterbank, and an mp3
granule holds 576 lines = 32 subbands × 18 lines. A Layer II frame holds
1152 samples per channel = 32 subbands × 36 samples, i.e. exactly two mp3
granules. So subband `sb` of Layer II parts `6g..6g+5` becomes spectral lines
`18·sb … 18·sb+17` of mp3 granule `g`.

Laying the values out subband-major (rather than in bitstream order, which is
part-major) is what makes the packing efficient: line index then tracks
frequency, magnitudes fall off towards the top of the spectrum exactly as in a
real mp3, unallocated high subbands become the trailing zero region, and the
per-region Huffman table selection has something meaningful to select on.

Sample code → mp3 value:

* `alloc < 15` — the mp3 value is the signed sample `code − (2^alloc−1 − 1)`,
  so it is centred on zero like a real spectral coefficient. Its magnitude is at
  most 8192, inside the 8206 an escape table with `linbits=13` can express.
* `alloc = 15, 16` — an mp3 value cannot hold 15/16 bits, so the low 14 bits go
  into the mp3 and the top 1–2 bits into `HIBITS`.
* `alloc = 17, 18, 19` (grouped triplets, 3/5/9 levels) — the grouped code is
  split into its three base-`mod` digits, giving 3 small values instead of one
  large one, keeping the 18-lines-per-subband layout intact. The split is a
  plain radix decomposition of the *whole* code, so even codes above `mod³`
  (which a strict encoder never emits) round-trip exactly.

Per granule the packer picks `region0_count`, `region1_count` and the three
Huffman tables by measuring the real cost of every candidate over every band
range, so each region gets the cheapest table that can represent it — including
table 0 for all-zero regions.

### Frames per frame

Normally one Layer II frame becomes one MPEG-1 mp3 frame (two granules), or two
frames for MPEG-2/2.5 LSF, where a frame holds a single granule. A granule's
`part2_3_length` field caps it at 4095 bits, which is not enough for mono
Layer II above ~300 kbit/s, so the packer falls back to 4, 6 or 12 granules per
Layer II frame (3, 2 or 1 parts each) until everything fits. The choice is
recorded in the meta header.

Scalefactors in the mp3 are given zero length (`scalefac_compress = 0`), so
`part2_3_length` is pure Huffman data.

Frames use the bit reservoir and pick the smallest bitrate index that fits each
frame (ordinary VBR), which keeps stuffing near zero.

## Results on `Creditsmix_Comp.mp2`

160 kbit/s, 44.1 kHz, joint stereo, 13 827 frames:

```
mp2 frames : 13827
mp3 frames : 13827
granules   : 2 per frame per channel
mp3 bitrate: 32..192 kbps (vbr)
max part23 : 1535 bits
samples    : 6301314 bytes mp2 -> 6026320 bytes huffman (95.6%)
input      : 7223902 bytes
output.mp3 : 6539103 bytes
output.meta: 922669 bytes
```

The Huffman-coded spectral data is *smaller* than the fixed-width Layer II
sample data it came from (95.6 %), because Layer II spends the same number of
bits on every sample of a subband while the mp3 tables can exploit the fact that
most of them are small. The 3.3 % total growth (7.46 MB vs 7.22 MB) is the mp3
frame headers and side info — 36 bytes per frame.

The generated mp3 parses cleanly end to end with the reference decoder:

```
$ tests/mp3chk output.mp3
frames 13827 layer 3 ch 2 sr 44100 consumed 6539103/6539103 skipped 0 failed 0
```

## Coverage

* MPEG-1 and MPEG-2 (LSF) Layer II. MPEG-2.5 Layer II is rejected by
  `mp3_A.h`'s own `_hdr_valid()` and so is treated as non-frame data.
* every sample rate, every bitrate index including free format, mono / stereo /
  dual channel / joint stereo with all four bounds, with and without CRC.
* leading, trailing and inter-frame junk (ID3 tags and the like) is carried
  through verbatim; anything that does not parse as a Layer II frame is simply
  preserved as bytes, so the round trip stays lossless on damaged files too.

## Tests

```
sh tests/run.sh Creditsmix_Comp.mp2
```

* `tests/mkmp2.cpp` — synthesises structurally valid Layer II frames with random
  allocations, scalefactors and samples for any header configuration.
* `tests/mp3chk.cpp` — walks the produced mp3 with the `mp3_A.h` decoder and
  fails if any byte is skipped or any frame does not decode, proving the output
  really is a Layer III stream and not just a container we can read ourselves.
* `tests/sweep.py` — 1260 configurations, each generated, packed, checked,
  unpacked and compared. All pass.

The Huffman encoding tables are *derived at start-up* by walking `mp3_A.h`'s own
decoder tables, and each derived code is decoded back and checked, so the packer
and the decoder can never disagree about a codeword. The packer also asserts
that each granule it writes is exactly as long as it predicted.
