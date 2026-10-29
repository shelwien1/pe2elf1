# m4vtsv

m4vtsv converts an MPEG-4 Part 2 video elementary stream (`.m4v`: DivX 4/5/6, XviD, libavcodec
`mpeg4`, ...) into a TSV listing of its syntax elements, and converts that TSV back into the
original stream, byte for byte. The bitstream syntax follows `m4vdec.cpp`, so it parses exactly
what that decoder (and FFmpeg) parses.

## Usage

```
m4vtsc c|d [options] input output
```

- `c` converts `.m4v` to `.tsv`.
- `d` converts `.tsv` back to `.m4v`.
- `-v` (with `c`) converts the written TSV back in memory and compares the result with the input.
- `-q` turns off the statistics.

`c` prints how many VOPs it found and how many it had to keep as raw bytes, with the reason:

```
aq_cm.m4v: 1258 VOPs (I 18, P 666, B 574, S 0), 0 kept raw; 3876609 coefficient events, 0 with escape mode noted
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | output can't be written |
| 4 | malformed TSV (`d`) |
| 5 | `-v` found a difference |

## Lossless by construction

`c` converts every VOP it writes as records back to bits right away and compares them with the
original bytes. This check goes through the TSV text and the same code that `d` uses. A VOP that
doesn't come back identical is stored as raw hex instead, and so is a VOP using a feature
m4vtsv doesn't describe. Everything that isn't a VOP is stored as raw hex. Any input therefore
round-trips, including damaged streams and files that aren't video at all.

VOPs kept raw:
- **Unsupported features:** data partitioning (except in B-VOPs, which don't use it), non-rectangular
  shapes, static sprites, sprite brightness change, newpred, reduced resolution, scalability, and
  the studio profile.
- **Damaged VOPs:** VOPs that don't parse.
- **Size limits:** VOPs longer than 64 MiB, VOPs with more than 32 KiB after the last macroblock,
  and VOPs whose text exceeds 64 MiB.

## TSV format (version 1)

One record per line, fields separated by tabs. `-` marks a field the syntax doesn't have at that
point, and lines starting with `#` are comments. The stream is cut at start codes (`00 00 01 xx`).

| Record | Fields | Content |
|---|---|---|
| `H` | hex | a chunk that isn't a VOP: VOS/VO/VOL headers, user data, GOV, data before the first start code |
| `R` | hex | a VOP kept as raw bytes |
| `V` | type mtb tib tinc coded rnd cplx dcthr tff alt warp quant fcode bcode | VOP header |
| `K` | st mbnum quant hec mtb tinc type dcthr warp fcode bcode | video packet header |
| `i` | x y st ac dq dt b0..b5 | macroblock of an I-VOP |
| `p` | x y st sk t mc ac dq dt fp fs mv b0..b5 | macroblock of a P- or S-VOP |
| `b` | x y sk mb t dq dt fp fs mv b0..b5 | macroblock of a B-VOP |
| `T` | bits hex | what follows the last macroblock |

`H` and `R` lines hold at most 32 KiB, and longer chunks continue on further lines. A VOP in record
form is a `V` line, one line per macroblock with `K` lines for the video packet headers, and a final
`T` line.

### Fields

- **VOP header (`V`):**
  - `type`: `I`, `P`, `B` or `S`.
  - `mtb`: modulo_time_base, as the number of 1 bits.
  - `tib`, `tinc`: width and value of vop_time_increment.
  - `coded`: vop_coded. The fields after it are left out when it is 0.
  - `rnd`: vop_rounding_type.
  - `cplx`: complexity estimation bits, in binary.
  - `dcthr`: intra_dc_vlc_thr.
  - `tff`, `alt`: top_field_first and alternate_vertical_scan_flag (interlaced VOLs).
  - `warp`: the sprite trajectory `dx,dy,...` (GMC S-VOPs).
  - `quant`, `fcode`, `bcode`: vop_quant and the forward and backward f_code.
- **Video packet header (`K`):**
  - `st`: MCBPC stuffing codes before the packet's resync stuffing.
  - `mbnum`, `quant`, `hec`: macroblock_number, quant_scale and header_extension_code.
  - The remaining fields repeat the VOP header values and are present only when `hec` is 1.
- **Macroblocks (`i`, `p`, `b`):**
  - `x`, `y`: macroblock position. `d` ignores these.
  - `st`: MCBPC stuffing codes before the macroblock.
  - `sk`: not_coded. In `b` records it means the macroblock has no bits because the co-located
    macroblock of the backward reference was skipped. Skipped macroblocks end after `sk`.
  - `t`: macroblock type.
    - `p`: 0 inter, 1 inter with 4 vectors, 2 intra.
    - `b`: 0 direct, 1 bidirectional, 2 backward, 3 forward.
  - `mb`: modb (0 = `1`, 1 = `01`, 2 = `00`). A `b` record with modb 0 ends there.
  - `mc`: mcsel.
  - `ac`: ac_pred_flag.
  - `dq`: dquant. For `i`/`p`: 0 for none, otherwise -2, -1, 1 or 2. For `b`: 0, -2 or 2.
  - `dt`: dct_type.
  - `fp`: field_prediction.
  - `fs`: field selects as bits, forward then backward.
  - `mv`: motion vector differences as coded, `x,y,...` in bitstream order. Forward vectors come
    before backward ones; a direct macroblock has its delta vector.
- **Tail (`T`):** the bits up to the next byte boundary (binary), then the remaining whole bytes
  (hex). This is normally just the stuffing.

### Blocks

`b0`..`b5` hold the six blocks of a macroblock (four luma, then Cb and Cr):

- A block that isn't coded is `-`.
- A coded block is a list of coefficient events `run:level` separated by spaces. The last event in
  the list is the one with the "last" flag.
- An intra block whose DC is coded separately (intra_dc_vlc) starts with the DC difference: `dc`
  when it has no AC coefficients, `dc|events` when it has.
- Escape codes are implied. An event is written with the plain VLC when one exists, otherwise with
  the first escape mode (1, 2, 3) that can code it. An event that the stream codes differently
  carries its mode, as in `2:8E2`. XviD and DivX streams never need this. FFmpeg's encoder picks the
  shortest escape, which needs a mark on about 0.1% of events.

### Example

```
H	00000120004480BA9850B04241463F
# VOL 352x288 (22x18 macroblocks), progressive, time increment 15 bits, quant 5 bits
V	I	0	15	0	1	-	-	0	-	-	-	4	-	-
i	0	0	0	1	0	-	-38|0:-14 0:-13 0:-12 0:-1 5:-10 0:-8 0:-6 0:-3	1|4:-1 0:1 6:1	11|0:1	0	-4	1
...
T	0	-
V	P	0	15	2400	1	0	-	0	-	-	-	4	2	-
p	0	0	0	0	0	-	-	0	-	-	-	0,0	-	4:1	-	-	-	-
...
p	4	0	0	1
...
T	-	7F
```

The `p` lines are an inter macroblock with a zero motion vector difference and one coefficient in
its second luma block, and a skipped macroblock.

## Build

```
x86_64-w64-mingw32-g++ -O2 -s -static -o m4vtsv.exe m4vtsv.cpp
g++ -O2 -o m4vtsv m4vtsv.cpp
```

The source is plain C that compiles as C or C++ and has no dependencies. Like m4vdec it doesn't use
malloc. All buffers are static, about 195 MiB of `.bss`, and only the part the data needs gets
touched (peak RSS about 11 MiB on the samples).

## Verification

These streams all convert to TSV and back to identical bytes, with `-v` passing. Together they cover
I/P/B/S-VOPs, GMC, quarter-pel, 4MV, interlacing, MPEG quantisation, video packets, packed
B-frames, N-VOPs and odd frame sizes:
- the 6 avip samples;
- the 39 synthetic and workaround test streams used for m4vdec;
- a stream with junk inserted;
- an empty file and 300 KB of random bytes.

Only the data-partitioned test stream and the junk-padded VOP were kept raw; every other VOP was
converted to records. Fuzzing found no crashes and no differences, under AddressSanitizer and UBSan:
- 160 damaged streams (bit flips, truncation, inserted garbage and fake start codes) all
  round-tripped exactly;
- 200 corrupted TSVs were either rejected with exit code 4 or decoded harmlessly.

A build with a 64 KiB input buffer and 7-byte reads also round-trips everything, which exercises
the chunk splitting.

The TSV is 7-25 times larger than the stream. On the samples, `c` handles 3-5 MB of stream per second
including its per-VOP check, and `d` 6-9 MB per second.

## License

The parsing code is derived from FFmpeg's libavcodec through m4vdec, so `m4vtsv.cpp` is licensed
under the GNU Lesser General Public License, version 2.1 or later, not under this repository's MIT
license.
