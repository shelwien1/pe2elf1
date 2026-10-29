# m4vdec

A standalone MPEG-4 Part 2 video decoder (DivX 4/5/6, XviD, libavcodec `mpeg4`, ...). It reads a raw
`.m4v` elementary stream, such as the ones avip extracts from AVI files, and writes one PNG per frame.

The decoder core is a C port of FFmpeg's MPEG-4 Part 2 decoder. Decoded pictures are bit-exact with
FFmpeg's C code path. The frontend follows bik2dec: worker threads write the PNGs, frame `f` goes to
worker `f % nw`, and there are `nw = cpus-1` workers (at most 32).

## Usage

```
m4vdec [-yuv] [-n] input.m4v output_name_prefix
```

- Frames are written as `<prefix>00000.png`, `<prefix>00001.png`, ... (RGB, in display order).
  Numbers have 5 digits, or more if the stream has 100000 VOPs or more.
- `-yuv` writes all frames to one raw I420 file named `<prefix>` instead of PNGs (for comparisons).
- `-n` does not repeat a frame for not-coded VOPs. See "Output frames" below.

The first line printed describes the stream, for example:

```
aq_cm.m4v: 512x384, vo_type 17, hpel, 1258 VOPs (574 B), XviD build 47, packed
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | usage |
| 2 | input can't be read |
| 3 | no VOPs found |
| 4 | unsupported stream feature |
| 6 | output can't be written |
| 7 | out of memory |
| 8 | frame size changed mid-stream |

## Build

The only dependency is zlib. The source is plain C that compiles as C or C++.

```
x86_64-w64-mingw32-g++ -O2 -s -static -o m4vdec.exe m4vdec.cpp -lz
g++ -O2 -o m4vdec m4vdec.cpp -lz -lpthread
gcc -x c -O2 -o m4vdec m4vdec.cpp -lz -lpthread
```

## Supported

- Simple and Advanced Simple profile: I, P, B and S (GMC) VOPs.
- GMC with 1 to 3 warp points, quarter-pel MC, 4MV and B-frame direct mode.
- Interlaced video: field DCT, field MC, field direct mode and alternate scan.
- H.263 and MPEG quantisation, including custom matrices.
- Resync markers / video packets, with simple concealment of damaged macroblocks.
- Packed bitstreams (DivX/XviD "packed B-frames").
- Any frame size, including sizes that are not multiples of 16.
- FFmpeg's encoder-bug workarounds, auto-detected from the DivX/XviD/lavc user-data idents. These
  include the XviD IDCT and the qpel chroma, edge, DC clip, interlaced-edge and old qpel-filter quirks.

Not supported (reported, exit code 4): data partitioning, arbitrary shapes, static sprites,
scalability, studio profile, and short video header (H.263) streams.

## Output frames

- B-frames are reordered to display order.
- In packed streams, the not-coded placeholder VOP that follows each packed P+B pair is dropped. Its
  frame was already output from the packed chunk.
- Any other not-coded VOP (N-VOP) means "show the previous frame again". By default m4vdec writes that
  frame again, so the frame count follows the stream's frame timing. With `-n` they are skipped, which
  matches `ffmpeg -fps_mode passthrough`.

## Colour conversion

YUV 4:2:0 is converted to RGB with:
- BT.601 coefficients, or BT.709 when the VOL signals `matrix_coefficients` = 1.
- Limited (TV) range, or full range when `video_range` is set.

Chroma is upsampled bilinearly using MPEG-4 chroma siting. For interlaced streams it is upsampled
per field.

## Verification

Every stream below was decoded with `m4vdec -yuv` and compared frame by frame (MD5) against
`ffmpeg -cpuflags 0 -flags +bitexact -threads 1 -f framemd5` (FFmpeg 6.1.1). All frames were identical.

- **The 6 avip samples (7349 frames):**
  - XviD with packed B-frames: aq_cm, ispure03, sdova_ed.
  - XviD with qpel at 1280x720: aria_cm.
  - DivX-style simple profile: susi_small1/2.
- **Synthetic streams made with FFmpeg's `mpeg4` encoder and libxvid:**
  - GMC: 3-point and translational, with qpel and B-frames.
  - qpel with 4MV.
  - Interlaced, including qpel and MPEG quantisation.
  - MPEG quantisation with custom matrices.
  - Resync markers in I-only and B-frame streams.
  - Odd size 330x246, fast motion, and N-VOPs in low-delay streams.
- **10 streams with injected DivX/XviD/old-lavc idents:** each switches on FFmpeg bug workarounds
  that change 25-50 of its 50 decoded frames compared with the same stream without the ident.

Known differences from a default `ffmpeg` run, none of which are m4vdec bugs:

- **XviD IDCT on x86:** FFmpeg's SSE2 XviD IDCT isn't bit-exact with its C version,
  so XviD streams can differ by ±1 unless FFmpeg runs with `-cpuflags 0`.
- **MPEG quantisation:** without `-flags +bitexact`, FFmpeg skips mismatch control on intra blocks.
  m4vdec always applies it, as the standard requires.
- **Duplicate last frame:** FFmpeg 6.1.1 can output an extra copy of the last frame at the end of
  the stream. This happens with frame threading, or when the stream ends with an N-VOP.

Speed (4-core VM, one decoding thread): with `-yuv`, the SD samples decode at 1000-2000 frames/s and
the 720p qpel sample at about 240 frames/s. In PNG mode the zlib compression dominates. For example,
943 frames at 512x384 take 10 s with 3 workers.

## License

The decoder core is derived from FFmpeg's libavcodec, so `m4vdec.cpp` is licensed under the GNU Lesser
General Public License, version 2.1 or later, not under this repository's MIT license.
