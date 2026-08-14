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
| bit allocation codes | meta, `ALLOC` |
| scfsi | meta, `SCFSI` |
| scalefactors | meta, `SCF` |
| ancillary/padding bits at the end of a frame | meta, `TAIL` |
| top bits of 15/16-bit samples | meta, `HIBITS` |
| bytes that are not part of a frame (ID3, junk, gaps) | meta, `GAPS` |

Nothing is duplicated — the meta holds exactly the Layer II fields that have no
counterpart in the mp3 syntax, and nothing else.

### The meta is laid out to be modelled, not to be small

The obvious thing to do is copy the bit allocation / scfsi / scalefactor region
of each frame out as a raw bit region, since it is contiguous in the source.
That is a bad idea: the fields are 2, 3, 4 and 6 bits wide, so nothing lands on
a byte boundary and every value is smeared across two bytes of a different
frame's data. A general-purpose compressor cannot see through it.

Instead each field gets its own section, and inside a section the values are
grouped **by slot** — slot = `2*subband + channel` — so that one subband's
values for the whole file are contiguous and consecutive bytes are the same
subband in consecutive frames, which is where the correlation is. Frame headers
go out as four byte planes for the same reason (all of byte 0, then all of
byte 1, …), which leaves three constant planes and one that only moves in the
padding bit.

Values that fit in a nibble or less are packed two or four to a byte. Because
neighbours in a slot stream are so alike, that packing *helps* compression as
well as halving the section — the packed byte alphabet stays small and repeats.
Scalefactors are 6 bits and are left one per byte: packing them four-to-three
bytes costs 16 % in compressed size, and so does splitting them into high/low
planes.

The values are stored plainly, with no delta or other transform. Delta-coding
the scalefactors buys 6 % with `xz` but only 0.8 % with `bzip2` — it is
compensating for LZMA's weak context model rather than removing real
redundancy — so the raw values are kept, and a context-mixing model can exploit
the same correlation directly.

On the supplied sample (`xz -9e` per section, as a proxy for what a real model
would find):

| section | raw | xz |
| --- | ---: | ---: |
| `ALLOC` (4 bits/value, slot-major) | 326 582 | 90 056 |
| `SCFSI` (2 bits/value, slot-major) | 119 393 | 69 760 |
| `SCF` (1 byte/value, slot-major) | 623 024 | 284 452 |
| `HDRS` (4 byte planes) | 55 308 | 2 132 |
| `TAIL`, `GAPS`, `CRCS`, `HIBITS` | 9 182 | 248 |
| **total** | **1 133 581** | **447 100** |

Against a single raw bit region for the same data — 922 669 bytes raw,
632 632 compressed — the restructured meta is 23 % larger raw and **29 %
smaller compressed**.

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
output.meta: 1133581 bytes
```

The Huffman-coded spectral data is *smaller* than the fixed-width Layer II
sample data it came from (95.6 %), because Layer II spends the same number of
bits on every sample of a subband while the mp3 tables can exploit the fact that
most of them are small. What is left over is the mp3 frame headers and side
info, 36 bytes per frame.

Raw sizes are not the point — the pair exists to be compressed. With plain
`xz -9e` and no mp3-specific modelling at all:

| | raw | xz -9e |
| --- | ---: | ---: |
| `Creditsmix_Comp.mp2` | 7 223 902 | 6 936 752 |
| `output.mp3` | 6 539 103 | 6 388 608 |
| `output.meta` | 1 133 581 | 447 100 |
| mp3 + meta | 7 672 684 | **6 835 708** |

so the split already beats compressing the mp2 directly, by 101 044 bytes,
with all of the actual gain still to come from an mp3 recompressor working on
the 6.4 MB of spectral data. (With the meta written as one raw bit region the
compressed total was 7 021 240 — *worse* than the mp2 — which is what the
layout above fixes.)

The generated mp3 parses cleanly end to end with the reference decoder:

```
$ tests/mp3chk output.mp3
frames 13827 layer 3 ch 2 sr 44100 consumed 6539103/6539103 skipped 0 failed 0
```

### Why the meta does not live in spare mp3 header fields

The mp3 side info has room to spare: `global_gain`, `scalefac_compress`,
`preflag`, `scalefac_scale`, `count1table_select`, `scfsi` and the private bits
are all constant here, 71 bits per frame, 123 KB over this file. Filling them
would shrink the raw total, but it makes the *compressed* total worse. Those
123 KB would hold about 164 000 scalefactors, which cost ~75 KB inside the meta
where they sit next to their neighbours from the same subband; scattered one
per granule across fixed-width side-info fields they compress to essentially
nothing less than their raw 123 KB. Net result is roughly 48 KB worse. Moving
bits between the two files does not remove information — only giving the
modeller better context does, which is what the meta layout is for.

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
