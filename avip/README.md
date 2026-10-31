# avip

A lossless splitter for MP4/MOV, Matroska/WebM and AVI files (it calls itself `vp`). It cuts a
file into the streams of its tracks, each in a standalone format that a compressor for that kind
of data can take (a raw MPEG-4 Part 2 `.m4v` for m4vpack, Annex B `.h264`/`.hevc`, ADTS `.aac`,
`.mp3`, Ogg Vorbis, `.wav`, ...), plus a small `.meta` file with everything else. `avip d` puts
the original file back together, byte for byte.

## Usage

```
avip c [-v] input out_prefix    split: out_prefix.meta + out_prefix.<i>.<ext> per track
avip d [-v] inp_prefix output   restore the original file
avip file                       info: container, tracks, what c would write (nothing is written)
```

`-v` prints a line per track, on how its frame sizes, timing and index entries are restored, and a
line on the layout, with the size of the `struct`: what is left in the meta as plain bytes. A file
avip doesn't recognise, or can't split and restore exactly, becomes a `.meta` that is a copy of it.

With m4vpack, for an AVI with MPEG-4 Part 2 video (DivX, XviD) and MP3 audio:

```
avip c movie.avi m           # m.meta, m.0.m4v, m.1.mp3
m4vpack c m.0.m4v m.0.m4vp
...
m4vpack d m.0.m4vp m.0.m4v
avip d m movie.avi
```

## Building

One translation unit, C++17, no libraries:

```bash
g++ -std=c++17 -O2 -o avip avip.cpp
```

`gc.bat` is the Windows build (clang++ with the VC2019 headers and libraries; its paths are the
author's).

## How it works

The meta holds a head per track, a program of container ops and the residual container bytes (the
`struct`). Whatever can be derived from the streams or from other container data is left out and
regenerated on restore: the headers and sizes of chunks, boxes and elements, the frame sizes of
self-delimiting streams, sample tables and index entries, timestamps (as residuals of a model),
the interleaving of the tracks, runs of zeros. `c` checks its output by running the decoder on it
in memory, and when that fails, falls back to raw track streams, then to simpler layouts, then to
a copy. What each container regenerates is listed at the top of `mp4_tables.inc`, `mkv_ebml.inc`
and `avi_meta.inc`; the meta format is described in `meta.inc` (framing, common track head) and
`mp4_meta.inc`, `mkv_meta.inc`, `avi_meta.inc` (each container's track part and ops).

## AVI: a stream with two chunk ids

A chunk's id in an AVI's `movi` list is its stream number and its type, `dc` for compressed video,
`db` for uncompressed. Some files have a video stream's key frames as `00db` chunks among `00dc`
ones (touka19.avi, with DivX 5 video, below). When a stream's most common id is one of the two,
avip takes chunks of both ids as its frames. The meta records which id each chunk had as a bit per
frame class (key, header, empty), plus a list of the chunks that don't follow it: a track flag of
the AVI track part (32, see `avi_meta.inc`).

Before this, avip took only the most common id of each stream number. The `00db` chunks stayed in
the meta as plain bytes, and the `.m4v` lost the key frames along with the VOL headers in them, so
m4vpack could parse none of its VOPs. touka19.avi (156,679,746 bytes, DX50 video and MP3 audio,
384 key frames in `00db` chunks):

| | meta | .0.m4v | m4vpack -1 of .0.m4v |
|---|---:|---|---:|
| before | 6,682,688 | 115,482,649: P-VOPs only, no VOL | 111,221,889 (96.31%: all VOPs kept as bytes) |
| after | 47,238 | 120,932,559: 384 I-VOPs, 33,657 P-VOPs | 96,021,170 (79.40%) |

At m4vpack's default level, `-9`, the new `.m4v` compresses to 87,672,614 bytes (72.50%). Most of
what is left in the meta is the size column of the MP3 chunks: they are cut at arbitrary points of
the MP3 stream, not at frame boundaries, so their sizes can't be derived from the data.

With the chunks of both ids in one track, the idx1 entries and the OpenDML `ix##`/`indx` entries
of such a stream are regenerated again (before, they stayed in the struct), and the chunk order is
predicted again: 67,918 of touka19.avi's 68,082 chunks (before, 34,386 of 67,698).
