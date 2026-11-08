# dts2tsv

dts2tsv converts a DTS stream into a TSV listing of its syntax elements and converts that TSV back
into the original stream, byte for byte. The listing goes down to the quantization indexes of every
subband sample of the core frames, the asset descriptors of the extension substream, and every
residual of the XLL (DTS-HD Master Audio's lossless) frames. One walk of the syntax runs in all four
directions: it reads the bits, writes them, writes the record of a frame's elements as text, and
reads it back.

## Usage

```
dts2tsv c|d [options] input output
```

- `c` converts the stream to TSV.
- `d` converts the TSV back to the stream.
- `-v` (with `c`) converts the written TSV back and compares the result with the input.
- `-q` turns off the statistics.

`input` and `output` can be `-` for stdin and stdout, except with `-v`. Both directions stream: a
movie's whole DTS-HD MA track converts in 17 MB of memory. `c` reports how many frames it found,
how many it had to keep as bytes, and why:

```
s.2.dts: 49710 frames (core 24855, extension substream 24855, 24855 XLL frames), 0 kept as bytes; 2443 bytes between frames; 497866442 bytes of text
lotr.dts: 11946 frames (core 11946, extension substream 0, 0 XLL frames), 0 kept as bytes; 0 bytes between frames; 66537799 bytes of text
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | malformed TSV (`d`) |
| 5 | `-v` found a difference, or an internal error |

## Lossless by construction

The stream is cut into frames and the bytes between them:

- **core frames**: the sync word `7FFE8001`, a header whose frame size and block count are in
  range, and either another frame (or the end of the input) after it or a frame that parses;
- **extension substream frames**: the sync word `64582025` and a header whose CRC holds.

`c` parses each frame into its record, writes the record as text, reads the text back into a new
record (as `d` does), writes that as bits and compares them with the frame. A frame that comes back
the same is kept as records. A frame that doesn't, or doesn't parse, is kept as hex, and so is
everything between frames. An extension substream frame whose XLL data doesn't parse is tried again
with that data kept as bytes, which keeps the rest of the frame as records. Any input therefore
converts back to itself, including damaged streams, `.dtshd` containers (their chunk headers are
bytes between frames) and files that aren't DTS at all.

The walk follows ETSI TS 102 114 V1.6.1 (clauses 5, 7 and 8) as far as decoders need it to find
every element. It keeps the following as bits, in hex: the parts that decoders skip by their sizes,
and the extensions it doesn't parse:

- what follows the subframes of a core frame: the optional information, and the core extensions
  XCh, X96 and XXCH;
- the extension substream's components other than XLL: a core in the substream, XBR, XXCH, X96, LBR
  and auxiliary data;
- the end of each asset descriptor;
- reserved bits;
- the bits between the parts of an XLL segment.

## TSV format (version 1)

One record per line, fields separated by tabs. `-` marks an element the frame doesn't have, lines
starting with `#` are comments, and the top of the text lists the columns of each kind of line
(`#C`, `#A`, ...). An element sent more than once in a record (per channel, per subband, per
coefficient) is a list, separated by commas, in the order the frame sends it. The names are those
of TS 102 114, and the values are the elements as sent. For example, `FSIZE` is the frame's size in
bytes less 1, `SUBFS` the number of subframes less 1, and `nLLFrameSize` the XLL frame's size less 1.

| Line | Keys | Content |
|---|---|---|
| `H` | | bytes that aren't a frame, in hex (at most 32 KiB a line; longer runs go on in further lines) |
| `R` | | a frame kept as bytes, after a comment saying why |
| `C` | | a core frame's header: `FTYPE SHORT CPF NBLKS FSIZE AMODE` ... `DIALNORM` (26 columns) |
| `A` | | its primary audio coding header: `SUBFS PCHS SUBS VQSUB JOINX THUFF SHUFF BHUFF SEL ADJ AHCRC` |
| `S` | `sf` | a subframe: `SSC PSC RANGE SICRC LFE LFEscaleIndex` |
| `B` | `ch` | a channel's subband side information in the subframe: `PMODE PVQ ABITS TMODE SCALES JOIN_SHUFF JOIN_SCALES HFVQ` |
| `Q` | `ch` | the quantization indexes of the channel's subband samples in the subframe |
| `T` | | the bits of the core frame after its subframes |
| `X` | | an extension substream frame's header (28 columns) |
| `D` | `asset` | an audio asset descriptor (64 columns) |
| `U` | `asset what` | an asset's data kept as bytes |
| `L` | `asset` | an XLL frame's common header (17 columns) |
| `K` | `cs` | a channel set sub-header (32 columns) |
| `G` | `band` | the channel set's coding of a frequency band (13 columns) |
| `N` | | the NAVI table: `BandChSetSize align crcok crc` |
| `P` | `band seg cs` | a segment of a channel set's band data: its coding parameters |
| `Y` | `ch` | a channel's residuals in the segment |

A core frame is a `C` line and an `A` line, then for each subframe an `S` line, a `B` line and a
`Q` line for each channel, then a `T` line. An extension substream frame is an `X` line and a `D`
line for each asset. Then come each asset's data: `U` lines, and for an XLL frame an `L` line, a `K`
line for each channel set followed by its `G` lines, an `N` line, then for each segment (band by
band, segment by segment, channel set by channel set) a `P` line and a `Y` line for each channel.

- **Keys** come before the elements of a line and say whose they are: the subframe `sf`, the
  channel `ch`, the asset, the channel set `cs`, the frequency band and the segment `seg`. `what` is
  one of `core xbr xxch x96 lbr xll aux`, `all` (the data of an asset whose static fields haven't
  been seen yet) or `rest` (an asset's bytes after its components). `d` checks the keys.
- **CRCs** of the extension substream header, the XLL header, the channel set sub-headers and the
  NAVI table: `crcok` is 1 where the CRC holds, and `d` computes it again; where it doesn't, `crcok` is 0
  and `crc` has the word as it is. The core's `HCRC`, `AHCRC` and `SICRC` are values like any other.
- **Bits as they are** (`tail`, `Reserved`, `rest`, `align`, `msbrest`, `lsbrest`, `hex`,
  `InfoTextString`) are hex. Their number follows from the sizes in the frame, and the last byte's
  unused bits are 0.
- **The core's samples** (`Q`): for each subsubframe, for each subband below `VQSUB` whose `ABITS`
  isn't 0, its 8 samples as levels (signed). The levels are the same whether the frame codes them
  with a Huffman code, a block code of 4 samples, or plainly in `ABITS` - 3 bits; `ABITS` and `SEL`
  say which. `SCALES` are the scale factors' quantization indexes, though the Huffman code books
  (`SHUFF` < 5) code their differences. The `DSYNC` words aren't in the text: they are `FFFF`, and
  a frame where one isn't is kept as bytes, as is one with partial subsubframes (`PSC` > 0).
- **Static fields**: an extension substream header without them (`bStaticFieldsPresent` 0) is
  read with what the last header that had them said: whether the mixing metadata is there, and for
  each asset index its channels and speaker mapping. Without such a header before, the asset's
  descriptor after `nuAssetIndex` is `Reserved` and its data a `U all` line.
- **XLL**: `BandChSetSize` are the NAVI table's sizes (each less 1), band by band, segment by
  segment, channel set by channel set. `ncAuxABIT`, `ncABIT0` and `ncABIT` are the coding parameters
  as coded, before a decoder adds 1 to them (for the hybrid Rice codes, and for linear codes not 0).
  `res` has the residuals of the segment: in the first segment its first samples (as many as the
  prediction order, part A of the segment) and then the others, coded with linear, Rice or hybrid
  Rice codes. `IsMaxLoc` lists where hybrid Rice codes put their isolated samples. `DeciHistory` has
  the decimator history of band 1 (7 samples a channel, in segment 0), and `lsb` the values of the
  LSB part. `msbrest` is the bits from the end of the MSB part to the LSB part (or to the end of the
  segment), and `lsbrest` those after the LSB part: byte padding, and the band data CRCs where the
  frame has them. The `L` line's `tail` is the bytes of the XLL frame after its band data (padding to
  a 32-bit boundary), and `rest` the bytes of the XLL component after the XLL frame.

### Example

The first frames of `s.2.dts` (DTS-HD MA 5.1, 24-bit 48 kHz, with a 1509 kbps core), the lines cut
at 150 characters:

```
# core: AMODE 9 + LFE, 48000 Hz, 5 primary channels, 512 samples, 2012 bytes a frame
C	1	31	0	15	2011	9	13	24	0	0	0	0	0	0	0	1	2	1	-	0	7	1	6	0	0	0
A	0	4	30,30,30,30,30	27,27,27,27,27	0,0,0,0,0	3,3,3,3,3	6,6,6,6,6	6,6,6,6,6	1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,7,7,7,7,7,7,7,7,7,7,7,7,
S	0	1	0	-	-	41,43,36,25,13,2,-10,-23	71
B	0	1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	3589,2084,1593	15,16,12,10,9,5,3,3,3,1,1,2,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1	0,0,0,0
...
Q	1	-3478,-12263,-7645,-55425,-12905,-33849,17432,32415,-1686,-2820,-5733,4779,4760,1347,2530,943,-571,515,-646,141,-156,-119,147,-1,35,-41,-518,352,1
...
T	0000000000
# extension substream 0: 1 asset: XLL (6 channels in 1 set, 48000 Hz, 24 bits)
X	0	0	0	31	2639	1	2	0	0	-	-	0	0	1	1	0	-	-	-	-	2607	1	0	0	00	1	-	-
D	0	14	0	0	-	0	-	0	-	-	23	12	5	1	0	-	1	0	15	0	-	-	-	-	-	0	-	0	-	-	-	-	-	-	-	-	-	-	-	-	0	513	-	-	-	-	-	-	-	-	-	2607	1	8	0	0	0	-	-	-	-	-	-	0	0000
L	0	0	12	11	2607	0	1	8	10	0	0	5	-	00	1	-	-	-
K	0	44	5	0	23	23	12	0	0	-	1	0	-	-	1	-	1	63	-	-	-	-	-	-	-	-	-	-	-	-	00	1	-
G	0	1	4,1,3,2,0,5	0,0,0	-	5,5,5,5,6,5	-	0,0,17,4,17,12,13,33,6,21,6,4,11,6,17,6,11,27,10,11,5,10,4,14,8,14,165,242,17,3,23	-	-	-	-	-	-
N	1278,1265	00	1	-
P	0	0	0	-	0	1,1,1,1,1,1	0,0,0,0,0,0	-	4,6,3,4,4,14	4,5,4,4,3,6	-	00	-
Y	0	-	-	-3,-26,-11,9,-16,0,8,-14,8,27,-30,-30,4,-12,-17,9,39,-13,-6,8,13,-22,-31,-21,-32,-4,18,51,-8,8,-2,-19,29,-10,44,4,-11,25,18,-44,-34,-2,4,8,-13
...
Y	5	-	-	15793,14029,11950,9515,6801,-1,104,-47,52,107,-43,-8,58,-8,-105,42,-18,-19,0,-7,-62,9,0,44,25,-6,39,-41,53,-33,31,-1,3,-45,125,-5,-21,22,4,51,
P	0	1	0	0	0	1,1,1,1,1,1	0,0,0,0,0,0	-	-	4,4,4,5,3,6	-	00	-
...
```

The core frame has one subframe of 2 subsubframes (`SSC` 1) and 5 primary channels of 32 subbands
(`SUBS` 30), the first 28 of them with bit allocations (`VQSUB` 27). Its `B` lines give each
channel's prediction modes, the predictors of the subbands that use them (`PVQ`) and the bit
allocations. The `Q` line of
channel 1 starts with the 8 samples of subband 0, whose `ABITS` 23 codes them plainly in 20 bits.
The XLL frame has 6 channels in one channel set, coded in 2 segments of 256 samples
(`nSegmentsInFrame` 1, `nSmplInSeg` 8). In segment 0, each channel's prediction order
(`nAdaptPredOrder` 5, 6 for channel 4) gives the size of part A of its residuals: the 5 large values
at the start of channel 5's are the samples its prediction can't predict yet. Segment 1 has new
coding parameters (`bUseSegStateCodeParm` 0) of Rice codes.

What the lines of `s.2.dts`'s text take: `Y` 278 MB, `Q` 159 MB, `B` 38 MB; the rest 23 MB.

## Verification

Every one of these streams converts back to itself with every frame as records:

- `s.2.dts`, a DTS-HD MA 5.1 track of 130 MB;
- FFmpeg's FATE DTS samples (core only, DTS-ES with XCh, 96 kHz X96, XBR, XXCH, XLL at 48, 96 and
  192 kHz, with two channel sets, 7.1 and 7.1.4 Master Audio);
- another DTS-HD MA track, a core-only track of 11946 frames, and a stream whose channel layout
  changes midway (pieces of two of these joined).

The values agree with the other parsers in this repository:

- the core's bit allocations, counted from the text, are dtscore's (`../dtssplit`);
- the XLL residuals of every XLL sample are the ones dtssplit's parser reads, which decodes them
  bit-exactly as FFmpeg does.

The text is 3.8 times as large as `s.2.dts`. `c` converts it in 7.6 s (17 MB of stream a second,
including its check of each frame through the text), `-v` adds 4.7 s, and `d` takes 4.5 s.

Fuzzing, under AddressSanitizer and UBSan:
- 1500 damaged streams (bit flips, inserted, deleted and repeated bytes, truncation, fake sync
  words) all converted back to themselves with `-v`;
- 1500 damaged texts (the same, with characters of the format and changed digits) were either
  rejected with exit code 4 or converted without errors.

## Build

```
g++ -O2 -o dts2tsv dts2tsv.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -o dts2tsv.exe dts2tsv.cpp
```

C++11 with its standard library, no other dependencies. The Huffman code books are dtssplit's
`../dtssplit/dtscore_huff.inc`, made from the specification's text by `dtshuff.py`.

## License

dts2tsv is written from the specification, so it is under this repository's MIT license. It follows
FFmpeg's decoder in two places where decoders read the XLL syntax otherwise than the specification's
text: the bits of the coding parameters with two frequency bands, and `bXtraFreqBands` 0 meaning two
bands. No FFmpeg code is in it.
