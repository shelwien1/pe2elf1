# PNG recompression pipeline (pngdump + png2bmp + preflate + pngtool)

Lossless, bit-exact PNG/APNG → BMP+metadata round-trip for use with external
image codecs (FLIF / JPEG-XL / BMF). The DEFLATE streams inside the PNG are
recompressed via preflate so the reconstructed PNG is byte-identical to the
original.

## Layout

| file              | role                                                 |
|-------------------|------------------------------------------------------|
| `pngdump.cpp`     | PNG container ↔ `meta` + raw deflate streams         |
| `png2bmp.cpp`     | inflated dump data ↔ plain BMP + `meta2`             |
| `preflate/`       | preflate library (Dirk Steinke, Apache 2.0)          |
| `pngtool.cpp`     | integrated `pngdump + png2bmp + libpreflate` tool    |
| `c.sh`, `d.sh`    | shell wrappers for the script-style pipeline         |
| `c1.bat`, `d1.bat`| original windows scripts (left for reference)        |
| `test.sh`         | round-trip tests against `testfile/*.png`            |

## Build

```sh
make           # builds pngdump, png2bmp, preflate_raw, pngtool
make test      # runs round-trip tests
make clean
```

`preflate_raw` is built from `preflate/main2.cpp` so it has no zlib/packARI
dependency (it just exposes `-s` / `-r` for splitting / recombining a raw
DEFLATE stream). The original `preflate/main.cpp` and `preflate/Makefile`
require zlib/packARI sources that aren't shipped here.

## Script-style pipeline (mirrors `c1.bat`/`d1.bat`)

```sh
./c.sh some.png         # → some.tar.xz
./d.sh some.tar.xz      # → some.png
```

The original .bat scripts use BMF + RAR; `c.sh`/`d.sh` use `tar`+`xz` so
they run on a stock Linux box.

## Integrated tool

```
pngtool c input.png  output.bmp  output.meta
pngtool d input.bmp  input.meta  output.png
```

Single-frame PNG: writes `output.bmp` + `output.meta`.

Multi-frame APNG (N frames): writes `output00.bmp`, `output01.bmp`, …,
`output{N-1}.bmp` plus a single `output.meta`. The index width is zero-
padded so the names sort correctly. Decoding reads the same indexed BMPs.

The meta file holds:
* the `pngdump` container blob (chunk list, zlib headers, ADLER32s, etc.)
* per-stream preflate reconstruction info ("hif")
* per-stream PNG filter types (one byte per scanline; for Adam7, all
  passes concatenated)
* per-stream frame width/height (from IHDR for the default stream,
  from the matching fcTL for animated frames)

## Supported PNG subformats

Same set as `png2bmp`: grayscale 4/8-bit, paletted 4/8-bit,
RGB / gray+alpha / RGBA 8-bit, with or without Adam7 interlacing,
plus APNG (any frame count). All seven PNG color/depth combinations
that fit naturally in a BMP are supported; 1/2-bit and 16-bit depths
are rejected at the BMP-conversion step.

## Round-trip is bit-exact

For all five files in `testfile/` the decoded PNG hashes byte-for-byte
identically to the input. The integrated `pngtool` and the script-based
`c.sh`/`d.sh` pipeline both pass.
