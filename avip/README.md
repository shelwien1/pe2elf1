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

## Key frames stored apart

Some muxers store a video stream's key frames differently from its other frames: under another
chunk id, as another kind of block, or flagged in a fragment of their own. avip predicts which
frames are key frames from the frame data itself, so this costs little in the meta, and the key
frames stay in the track's stream. The fields that describe it are optional: metas without them
are read as before, but a meta that uses them needs this version of avip to be restored. Numbers
below are for touka19.avi (DivX 5 video, MP3 audio), and for its first two minutes remuxed with
ffmpeg (and, for the MKV block kinds, rewritten).

### AVI: two chunk ids

A chunk's id in an AVI's `movi` list is its stream number and its type, `dc` for compressed video,
`db` for uncompressed. Some files have a video stream's key frames as `00db` chunks among `00dc`
ones. When a stream's most common id is one of the two, avip takes chunks of both ids as its
frames. The meta records which id each chunk had as a bit per frame class (key, header, empty),
plus a list of the chunks that don't follow it: a track flag of the AVI track part (32, see
`avi_meta.inc`).

Before this, avip took only the most common id of each stream number. The `00db` chunks stayed in
the meta as plain bytes, and the `.m4v` lost the key frames along with the VOL headers in them, so
m4vpack could parse none of its VOPs. touka19.avi (156,679,746 bytes, 384 key frames in `00db`
chunks):

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

### MKV: SimpleBlocks and BlockGroups

A Matroska frame is a SimpleBlock, or a Block in a BlockGroup with children such as a
ReferenceBlock. Muxers differ: key frames as BlockGroups among SimpleBlocks, or the other way
round, or every frame a BlockGroup where only the non-key ones have a ReferenceBlock (older
mkvmerge). avip's predicted block runs used one block kind per track, so each block of the other
kind took an op of its own, and the BlockGroup children column followed the key frames. Now a
track can have one default kind for predicted key frames and one for the others, and the children
column can be coded relative to the key prediction (see `mkv_meta.inc`). The frames were always
in the stream; this is about the meta's size:

| 2 minutes, 2,878 frames, 82 key frames | meta before | meta after |
|---|---:|---:|
| all SimpleBlocks | 1,355 | 1,355 |
| key frames as BlockGroups | 1,582 | 1,358 |
| non-key frames as BlockGroups | 1,584 | 1,360 |
| all BlockGroups | 1,525 | 1,360 |

### MP4: fragments

In a fragmented MP4 the key frames are flagged in the `trun` boxes: the first sample's flags of a
fragment that starts at a key frame, or flags per sample. When fragments start at key frames
(ffmpeg's `frag_keyframe`) and key frames follow each other, a fragment holds one sample and its
`trun` has no per-sample fields, so the encoder writes no `trun` op for it. avip's decoder numbered
the `trun` ops by count, so after such a fragment every op went to the wrong `trun`; the check of
the output failed, and the whole file was stored as a copy, with no streams. Now each `trun` op
finds its `trun` by position. Per-sample flags (key frames in the middle of a fragment)
are now predicted from the sample's class (key / intra / non-reference), with exceptions stored
(`tflags` 16, see `mp4_meta.inc`).

| 2 minutes, fragmented MP4 | meta before | meta after |
|---|---:|---:|
| a fragment per key frame (`frag_keyframe`) | 28,437,301 (a copy) | 18,412 |
| fragments of 1 s (`frag_duration`) | 28,698 | 25,836 |

Most of what is left is the fragments' box headers (`moof`, `traf`, `tfhd`, `trun`), about 200
bytes per fragment.
