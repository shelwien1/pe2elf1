# ac3tsv

ac3tsv converts an AC-3 or E-AC-3 elementary stream into a TSV listing of its syntax elements, down
to every exponent and mantissa, and converts that TSV back into the original stream, byte for
byte. The syntax is [ac3dec](../ac3dec)'s: the same walk of a frame, which ac3dec runs to read the
bits, runs here to read them, to write them, and to read and write the record of a frame's
elements as text.

## Usage

```
ac3tsv c|d [options] input output
```

- `c` converts the stream to TSV.
- `d` converts TSV back to the stream.
- `-v` (with `c`) converts the written TSV back and compares the result with the input.
- `-q` turns off the statistics.

`input` and `output` can be `-` for stdin and stdout, except with `-v`. `c` says how many frames
it found, and how many it had to keep as bytes, and why:

```
ss_sp3.ac3: 1501 frames (AC-3 1501, E-AC-3 0, 0 of them dependent), 0 kept as bytes; 0 bytes between frames; 12862610 bytes of text
7_pt_1.eac3: 35438 frames (AC-3 0, E-AC-3 35438, 17719 of them dependent), 0 kept as bytes; 0 bytes between frames; 132876764 bytes of text
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

The stream is cut into syncframes (a sync word, a header that parses, and where the frame's CRCs
hold or the next frame follows it or the input ends) and the bytes between them. `c` parses each
frame into its record, writes the record as text, reads that text back into a new record (as `d`
does), writes it as bits, and compares them with the frame. A frame that comes back the same is
kept as records; one that doesn't, or doesn't parse (enhanced coupling, damaged data), is kept as
hex, and so is everything between frames. Any input therefore converts back to itself, including
damaged streams and files that aren't AC-3 at all.

What a parser needs to find the mantissas, the bit allocation, is A/52's integer model run on the
exponents (ac3dec's, FFmpeg's); everything after the last block that the walk doesn't read (the
auxiliary data, unused bits) is kept as it is, so a frame comes back the same even where it has
more than its blocks.

## TSV format (version 1)

One record per line, fields separated by tabs. `-` marks an element the frame doesn't have, lines
starting with `#` are comments, and the top of the text lists the columns of each kind of line
(`#F`, `#G`, ...). An element sent more than once in a record (per channel, per band, per segment)
is a list, separated by commas, in the order the frame sends it. The names are A/52's.

| Line | Columns | Content |
|---|---|---|
| `H` | hex | bytes that aren't a frame (at most 32 KiB a line; longer runs go on in further lines) |
| `R` | hex | a frame kept as bytes, after a comment saying why |
| `F` | bsid crc1ok crc1 fscod frmsizecod ... addbsi | an AC-3 frame's syncinfo and bsi (40 columns) |
| `G` | bsid strmtyp substreamid frmsiz ... addbsi | an E-AC-3 frame's bsi (57 columns) |
| `A` | expstre ahte snroffststr ... blkstrtinfo | E-AC-3's audfrm (32 columns) |
| `B` | blk blksw dithflag dynrnge ... skipfld qfill | an audio block's side information (62 columns) |
| `E` | ch absexp dexp gainrng | a channel's exponents in the block |
| `M` | ch bap q | a channel's mantissas in the block |
| `Q` | ch bap gaqmod gaqgain gain gainfill vq gaqmant gaqbig | AHT: a channel's data for the whole frame (in block 0) |
| `T` | tail auxdatae crcrsv crc2ok crc2 | the trailer |

A frame is an `F` or `G` line, its `A` line (E-AC-3), then for each block a `B` line followed by its
`E` lines (the channels whose exponents it sends) and its `M` and `Q` lines (in the order of the
mantissas in the block: the full bandwidth channels, the coupling channel after the first coupled
one, the LFE), and a `T` line.

- **Channels** (`ch`): 0 the coupling channel, 1 to 5 the full bandwidth ones in A/52's order (L C R
  Ls Rs, as the mode has them), then the LFE.
- **CRCs:** `crc1ok` and `crc2ok` are 1 where the word holds, and `d` computes it again; where it
  doesn't, they are 0 and `crc1` / `crc2` have the word as it is.
- **Elements not sent** take their values from the walk, as a decoder does: E-AC-3's frame
  exponent strategies give each block's (`frmchexpstr` on the `A` line; AC-3's `expstr` on the `B`
  lines), the strategies and coordinates not sent in a block are the block's before, and so on.
- **Exponents** (`E`): `absexp` (the coupling channel's twice as large in effect), then `dexp`, the
  exponent deltas plus 2 (0 to 4), a digit each, three to a group of the bits.
- **Mantissas** (`M`): `bap` is the bit allocation the frame's exponents and parameters give each
  bin from the channel's first to its last (A/52's bap, or AHT's hebap, a base-36 digit each):
  informational, `d` ignores it. `q` has the mantissa of each bin whose bap is not 0, as a level:
  for bap 1-5 (3, 5, 7, 11 and 15 levels) the code less half the levels (-1..1, -2..2, -3..3,
  -5..5, -7..7), for bap 6-15 the signed value of its 5-16 bits. Where the bits group three or two
  mantissas in a code (bap 1, 2, 4), the values are the code's, and the values of a block's last
  groups past its last mantissa are on its `B` line (`qfill`: for bap 1, 2, 4, as codes). A code past
  the levels (bap 1 codes 27-31, bap 2 125-127, bap 4 121-127; bap 3 7, bap 5 15), which encoders
  don't write and decoders read anyway, shows as a level past them (2, 3, 6; 4, 8).
- **AHT** (`Q`): `gaqmod`; the gains (`gaqgain`, a bit a bin, modes 1 and 2; `gain`, 0-2 a bin,
  mode 3, and `gainfill` for its last group); `vq`, the vector index of each bin of hebap 1-7;
  `gaqmant`, the six mantissas of each bin of hebap 8 and up, and `gaqbig` the large mantissa after
  each escape.
- **The trailer** (`T`): `tail` is the bits between the end of the last block and `auxdatae` (the
  auxiliary data, its length `auxdatal`, the unused bits), in hex (their number follows from the
  frame's size; the last byte's unused bits are 0); then `auxdatae`, `crcrsv` (E-AC-3's `encinfo`),
  `crc2ok`, `crc2`.
- **Raw bytes** (`skipfld`, `addbsi`, `mixdata`, `blkstrtinfo`) are hex.

### Example

A block of `ss_sp3.ac3` (AC-3, 2/0, 224 kbps), its first lines cut at 150 characters:

```
F	8	1	-	0	22	0	2	-	-	0	0	31	0	-	0	-	0	-	-	0	1	0	-	0	-	-	-	-	-	-	-	-	-	-	-	-	-	0	-	-
B	0	0,0	1,1	0	-	-	-	-	-	-	-	-	-	-	-	-	-	-	1	0	-	-	-	-	-	-	-	-	-	-	-	-	1	0,0,0,0	1,1	50,50	1	2	1	1	2	4	1	12	3,3	-	4,4	...
E	1	3	2332222142222223032213231223222132133114212222312312223122304131321222322132131231321332032231222222222304131133121322231231223132121321...
E	2	4	2322123223213223032223122223122222232222312402223212222222223222221322132132132131231322222230232222221233122221321323121313213221321412...
M	1	5555555553455555777765554555444353353235224444434323333133315242422333322242242442422321132221222222222214121232122322221331333131133311311...
M	2	5555555555555555777765455555344444443333434414444223333333333222222422242242242242442322222221332222222332122222322322122313113111311412221...
B	1	0,0	1,1	0	-	-	-	-	-	-	-	-	-	-	-	-	-	-	0	-	-	-	-	-	-	-	-	-	-	-	-	-	0	-	0,0	-	0	-	-	-	-	-	0	...
```

The `B` line of block 0 has the coupling strategy (`cplstre` 1, `cplinu` 0), the rematrixing
flags, the exponent strategies (D15 for both channels: `expstr` 1,1) and the bandwidths (`chbwcod`
50: up to bin 223), the bit allocation parameters and the SNR offsets; block 1 reuses the
exponents. The `M` lines' `bap` runs from 5 at the bottom to 1 near the top of the band, and their
`q` (past the cut) are the levels of those bins.

## Verification

The corpus is [samples/](../samples): 43 streams, 136 MB (AC-3 in every channel mode FFmpeg
writes, with coupling, the alternate bit stream syntax, 32 and 44.1 kHz; E-AC-3 with AHT, SPX,
1/2/3/6 blocks a frame, dependent substreams, Dolby's own encodes). Every one converts back to
itself, and every frame converts as records, but for the last frame of `casablanca_aht_mono_64`,
which is cut short (224 bytes, kept as bytes like the cut frames before the first frame of the
csi_miami files).

The text is 3-13 times as large as the stream (most for the low bit rates). `c` handles 2-3 MB of
stream a second, including its check of each frame through the text, and `d` 4-8 MB a second.

Fuzzing, under AddressSanitizer and UBSan:
- 600 damaged streams (bit flips, inserted, deleted and repeated bytes, truncation, fake sync
  words) all converted back to themselves with `-v`;
- 600 damaged texts (the same, with characters of the format) were either rejected with exit code
  4 or converted without errors.

## Build

```
g++ -O2 -o ac3tsv ac3tsv.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -o ac3tsv.exe ac3tsv.cpp
```

One file, C++11 with its standard library, no other dependencies. The syntax section (the tables,
the walk, the bit allocation) is the same as ac3dec's.

## License

The syntax is ac3dec's, derived from FFmpeg's libavcodec, so `ac3tsv.cpp` is licensed under the
GNU Lesser General Public License, version 2.1 or later, not under this repository's MIT license.
