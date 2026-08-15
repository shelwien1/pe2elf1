# mp2 — lossless MP2 → MP3-container converter

`mp2.cpp` repacks the spectral data of an MPEG audio **Layer II** stream into a
syntactically valid **Layer III** (mp3) bitstream, and puts everything that has
no place in the mp3 syntax into a small side file. The transform is exactly
invertible: `c` followed by `d` reproduces the input byte for byte.

```
mp2 c input.mp2 output.mp3 output.meta [output.bmp]     pack
mp2 d input.mp3 input.meta output.mp2  [input.bmp]      unpack
```

With the optional fourth file, the bit allocations and scalefactors go out as an
8-bit BMP for an image compressor to take instead of sitting in the meta — see
below.

The mp3 is not meant to sound like the source — it is a container. It *is*
a real mp3 though: every frame parses with an ordinary decoder, with correct
headers, side info, region/table selection and bit reservoir.

[**Layer II vs Layer III**](docs/formats.html) is a companion document: the frame structures of
the two formats side by side, field by field, and the arithmetic that makes the repacking exact.

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
| bit allocation codes | meta, `ALLOC` — or the bmp |
| scfsi | meta, `SCFSI` |
| scalefactors | meta, `SCF` — or the bmp |
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

### The bulky streams can go out as an image

Give `mp2 c` a fourth filename and the bit allocations and scalefactors are
written there as an 8-bit BMP instead of into the meta. The layout is the same
slot-major order folded into columns — value `i` at `(i/H, i%H)`, so down a
column is one subband through time, the direction the correlation runs. Width is
read back from the header; 2048 measured best and the choice is worth well under
a percent. Allocations come first because their count follows from the frame
headers alone, while the scalefactor count needs the allocations and the scfsi
values — so the reader finds both with nothing stored.

The point is not the image format, it is that this hands the streams to a
compressor built to model them. **BMF** (Shkarin's, a context model) gets the
pair to **332 724** against 374 508 for `xz` on the same bytes, 11 % better.
Ordinary image codecs do not: on the scalefactors alone PNG's Up filter gives
315 541 and WebP lossless 389 782, both worse than `xz`'s 284 452, and WebP
additionally caps each dimension at 16 383 pixels. `mp2 d` reads the BMP back
either plain or RLE8, since BMF re-emits it run-length coded when that is
smaller.

What else was tried in there. Frame headers do not belong: appending them costs
8 388 bytes in the image against the 1 992 they compress to alone, so they stay
in the meta — and all three together (411 544 total) are worse than not using
the image for the allocations at all. `SCFSI` is worse in the image under any
arrangement (`bmf` 72 520 against `xz` 69 760). Two separate BMP files beat one
by 1 236 bytes, which is not worth a second output. Order inside the image is
worth 160.

The gain is the model, not the second dimension. `tests/sfimg.cpp` dumps the
*true* subband-by-time grid — one column per subband, three rows per frame — and
it is worse than the folded stream at every setting: 393 676 with the best
predictor and LZMA behind it, 486 343 through PNG. A left (cross-subband)
residual is the worst of four and 2D prediction is worse than vertical alone,
which says neighbouring subbands tell you nothing a subband's own past does not.
The real grid also has to be 3.6× larger than the data, since scfsi transmits
only 1 to 3 of each subband's 3 scalefactors and most subbands are unallocated.

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

### The mp3 is coded for the recompressor, not for size

The packer's default is *uniform* side info: one Huffman table for all three
regions of every granule, `region0_count = region1_count = 0`, and
`big_values = 288` — so the only side-info field that ever changes is
`part2_3_length`. The table is chosen once for the whole file, as the cheapest
one that can express every value in it.

That deliberately makes the mp3 about a third larger than it needs to be. It is
what an mp3 recompressor wants: it re-derives the spectral values anyway, so the
Huffman coding costs it nothing, while every side-info field that varies has to
be paid for. Measured with mp3zip on the sample:

| encoding | mp3 | after mp3zip |
| --- | ---: | ---: |
| per-granule optimal tables + regions | 6 539 103 | 5 765 677 |
| one fixed table, fixed regions | 8 210 407 | 5 726 346 |
| + `big_values` fixed at 288 | 9 007 070 | **5 707 362** |
| + `global_gain` set from granule magnitude | 9 226 541 | 5 717 261 |

Making `global_gain` a magnitude hint *costs* 10 KB, so the recompressor does not
use it to predict quantized magnitudes — anything that varies is pure expense.
Constant bitrate loses to VBR (5 718 002), and turning the bit reservoir off is a
disaster (12.4 MB — mp3zip essentially gives up).

When a granule cannot be coded uniformly inside the 4095-bit `part2_3_length`
limit — high-bitrate material, where 576 escape-coded values do not fit — the
packer falls back to picking `region0_count`, `region1_count` and a table per
region by measuring the real cost of every candidate over every band range,
which produces the smallest legal mp3 instead.

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

### Nothing else belongs in the mp3 either

The mp3 has room to spare — constant side-info fields, and whole regions of the
spectrum that are structurally zero — so it is natural to try moving part of the
meta into it. Four attempts, all measured against mp3zip, all losses:

| what was moved into the mp3 | mp3zip charges | meta gives back | net |
| --- | ---: | ---: | ---: |
| `global_gain` set from granule magnitude | 9 899 | 0 | −9 899 |
| scalefactors, high 3 bits, into the mp3 scalefactor field | 106 052 | 52 956 | −53 096 |
| the same, granule-aligned for maximum correlation | 127 603 | 52 956 | −74 647 |
| scalefactors, all 6 bits, into free high-frequency spectral lines | 411 808 | 284 452 | −127 356 |
| the same, with the sign bits carrying the noisy low bits | 387 561 | 284 452 | −103 109 |

The scalefactor-field attempt splits the 6-bit values 3/3 and carries the high —
slowly varying, most compressible — half in the mp3, with `scalefac_compress`
fixed so every granule has 21 three-bit scalefactors. It costs 1.36 bits per
value there against 1.14 in the meta. Rearranging so a subband lands in the same
band index of *both* granules, which makes the two granules carry identical
values, costs **more** rather than less: whatever mp3zip does with scalefactors,
it is not a granule-to-granule predictor this data could feed. The 4/2 split is
closest to viable — the meta gives back 128 664 there — but it extrapolates to
about 141 000 in the mp3, and 4-bit values only fit 11 of the 21 bands, 44 slots
per frame against the ~45 values a frame carries.

The spectral-line attempt is the most appealing on paper, because the
coefficients it displaces cost *nothing* to store: which lines are structurally
zero follows from the bit allocation, which the unpacker reconstructs before it
touches the mp3, so it simply knows to read scalefactors there instead. There is
plenty of room — 360 always-free lines per frame above the last subband alone,
against the 90 needed, before counting unallocated subbands or the 828 lines the
joint-stereo bound frees in the second channel. Each scalefactor goes into two
lines, three of its bits per line.

Which three matters, and it is worth its own note. A spectral value is signed,
and that sign is a *raw* bit — it costs exactly one bit, always, whatever is in
it. So the bit that belongs there is the least predictable one. Putting each
half's most significant bit in the sign, as the first attempt did, pays a full
bit for something worth about 0.1 — twice per scalefactor. Moving the noisy low
bits into the signs and leaving the predictable high bits to the magnitudes
recovers 24 247 bytes, 5.9 %.

It still loses by 103 109. Per scalefactor: two sign bits that cost exactly two,
plus four magnitude bits that mp3zip codes in about three — against 3.65 bits for
the whole value in the meta. The magnitude bits are the compressible ones, and
the mp3 simply cannot predict them there: its context is the neighbouring
spectral lines, which hold unrelated scalefactors from other subbands, while the
meta puts the same subband's value from the neighbouring frame right next to it.
Fixing that would need a line assignment stable across granules and frames, and
the 81 (subband, part) slots per channel do not fit the 45 stable pairs the
always-free region offers.

The pattern is consistent enough to state as a rule: **the mp3 container is a
good home for spectral data and a bad home for everything else.** A recompressor
models a spectral value from its neighbours, its band, and the same place in the
previous granule. Anything smuggled in loses that context and gains none, while
in the meta it sits next to the same subband's value from the neighbouring
frame — which is the best context this data has. Moving bits between the two
files never removes information; only giving the modeller better context does.

These numbers are mp3zip's. A recompressor with a stronger scalefactor model
could flip the closest of them, and each encoder change is small.

## Results on `Creditsmix_Comp.mp2`

160 kbit/s, 44.1 kHz, joint stereo, 13 827 frames:

```
mp2 frames : 13827
mp3 frames : 13827
granules   : 2 per frame per channel
side info  : uniform (huffman table 22, fixed regions)
mp3 bitrate: 48..256 kbps (vbr)
max part23 : 2165 bits
samples    : 6301314 bytes mp2 -> 8503298 bytes huffman (134.9%)
input      : 7223902 bytes
output.mp3 : 9007070 bytes
output.meta: 1133581 bytes
```

Raw sizes are not the point — the pair exists to be compressed. Packing the mp3
with `mp3zip` and the meta with `xz -9e`:

| | bytes |
| --- | ---: |
| `xz -9e` of the original mp2 | 6 936 752 |
| `mp3zip(output.mp3)` | 5 707 362 |
| `xz -9e (output.meta)` | 71 916 |
| `bmf(output.bmp)` | 332 724 |
| **total** | **6 112 002** |

**824 750 bytes, 11.9 %, below compressing the mp2 directly.** The whole chain
`mp2 → mp3+meta+bmp → mp3zip/xz/bmf → mp2` is byte-exact, and mp3zip is confirmed
lossless on this mp3 (it is an unusual one — zero-length scalefactors, constant
side info, wide VBR swings).

Without an mp3-aware packer, `xz` alone on both files gives 6 835 708, which
still beats the mp2. (With the meta written as one raw bit region that number
was 7 021 240 — *worse* than the mp2 — which is what the layout above fixes.)

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
