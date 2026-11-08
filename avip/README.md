# avip

A lossless splitter for MP4/MOV, Matroska/WebM, AVI and MPEG files (program streams: `.mpg`,
VCD, DVD `.vob`; transport streams: `.ts`, `.m2ts`; raw video elementary streams). It calls
itself `vp`. It cuts a file into the streams of its tracks, each in a standalone format that a
compressor for that kind of data can take (a raw MPEG-4 Part 2 `.m4v` for m4vpack, Annex B
`.h264`/`.hevc`, MPEG-1/2 video `.m1v`/`.m2v`, ADTS `.aac`, `.mp3`, AC-3, Ogg Vorbis, `.wav`,
...), plus a small `.meta` file with everything else. `avip d` puts the original file back
together, byte for byte.

## Usage

```
avip c [-v] [-w dir] input out_prefix    split: out_prefix.meta + out_prefix.<i>.<ext> per track
avip d [-v] [-w dir] inp_prefix output   restore the original file
avip [-w dir] file                       info: container, tracks, what c would write (nothing is written)
```

`-v` prints a line per track, on how its frame sizes, timing and index entries are restored, and a
line on the layout, with the size of the `struct`: what is left in the meta as plain bytes. A file
avip doesn't recognise, or can't split and restore exactly, becomes a `.meta` that is a copy of it.
`-w dir` puts the temporary files (see Memory) in `dir`.

The info pass prints those lines, then the streams and the meta `c` would write, with their sizes.
It runs `c`'s analysis, but makes no streams (it only counts their bytes) and skips `c`'s check of
the result, so it writes nothing at all, temporary files included. The check is the one thing that
can make `c` write something else: where it fails, `c` falls back to a simpler layout, with a
warning.

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

## Memory

avip maps its input into memory rather than reading it (and, restoring, the streams), and a
buffer that grows past 256 MB moves to a temporary file mapped into memory (`avip-<pid>-<n>.tmp`,
in the output's directory), whose pages the system writes out and drops when it needs the memory
instead of holding them. A stream that ends up in such a file is renamed into place as the output
rather than written again. What avip needs in RAM no longer grows with the size of the file, only
with its number of frames (about 360 bytes per frame of an MKV), and the output is the same,
byte for byte. The temporary files take disk space instead: besides the outputs, about another copy
of the largest track `c` converts (H.264 or HEVC to Annex B), which its check of the result makes
(a 609 MB MKV with 529 MB of H.264: 1,074 MB in the output's directory at the peak, 608 MB at the
end). `-w dir` or `AVIP_TMP` puts them in another directory (`-w` first), `AVIP_SPILL_MB` sets the
size past which a buffer moves to one.

Peak private memory (the heap; mapped file pages are the system's to drop), before the mapped
files and with them:

| input | `c` before | `c` now | `d` before | `d` now |
|---|---:|---:|---:|---:|
| MKV, 907 MB (H.264 + AAC) | 2,620 MB | 276 MB | 1,740 MB | 251 MB |
| the same as MP4 | 2,601 MB | 269 MB | | |
| the same as MPEG-TS (929 MB) | 2,854 MB | 508 MB | | |
| AVI, 157 MB (DivX + MP3) | 315 MB | 165 MB | | |

The analyses and conversions read the frames where they lie in the input, so no track is copied
out to be converted (a transport stream's tracks were even copied twice: `c` on a 595 MB one took
1,074 MB of disk at the peak, now 553 MB, its outputs). The info pass makes no streams, so it
writes no temporary files, and needs less memory than before, when it made them as `c` does
(609 MB inputs):

| input | info before: memory | its temporary files | info now: memory |
|---|---:|---:|---:|
| MKV (H.264 + AAC) | 436 MB | 1,074 MB | 185 MB |
| the same as MP4 | 326 MB | 1,074 MB | 43 MB |
| the same as MPEG-TS | 378 MB | 1,074 MB | 246 MB |
| DVD `.vob` (MPEG-2 + AC-3) | 341 MB | 604 MB | 82 MB |

## How it works

The meta holds a head per track, a program of container ops and the residual container bytes (the
`struct`). Whatever can be derived from the streams or from other container data is left out and
regenerated on restore: the headers and sizes of chunks, boxes and elements, the frame sizes of
self-delimiting streams, sample tables and index entries, timestamps (as residuals of a model),
the interleaving of the tracks, runs of zeros. `c` checks its output by running the decoder on it
in memory, and when that fails, falls back to raw track streams, then to simpler layouts, then to
a copy. What each container regenerates is listed at the top of `mp4_tables.inc`, `mkv_ebml.inc`,
`avi_meta.inc`, `ps.inc` and `ts.inc`; the meta format is described in `meta.inc` (framing, common
track head) and `mp4_meta.inc`, `mkv_meta.inc`, `avi_meta.inc`, `ps.inc`, `ts.inc`, `es.inc`
(each container's track part and ops), with the PES model shared by the MPEG formats in
`mpg_pes.inc`.

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

For H.264 and HEVC, the key prediction (also behind the SimpleBlock key flags) found no key frames
in a Matroska file: it parsed slice headers, which needs the SPS/PPS, and Matroska keeps those in
CodecPrivate rather than in the frames. It now also finds them by NAL type, which needs no
parameter sets: H.264 IDR, HEVC IRAP, optionally recovery point SEIs (open GOP, intra refresh) and
I slices. On ffmpeg/x264 and x265 test files of 20 seconds with 10 key frames, the meta shrinks
by 25 to 45 bytes (SimpleBlocks only: closed GOP 932 -> 887, open GOP 967 -> 922, intra refresh
963 -> 918, HEVC 3,295 -> 3,250); files that have their parameter sets in the frames keep the
old prediction and the same meta.

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
| a fragment per key frame (`frag_keyframe`) | 28,437,301 (a copy) | 18,251 |
| fragments of 1 s (`frag_duration`) | 28,698 | 25,599 |

Most of what is left is the fragments' box headers (`moof`, `traf`, `tfhd`, `trun`), about 200
bytes per fragment.

### MP4: a fragment per frame

With a fragment per frame (ffmpeg's `frag_every_frame`), those headers were about 130 bytes of
meta per frame: 1 MB for two minutes. A fragment of one sample is now a template: its `moof` and
`mdat` header bytes, from the track's previous fragment of the same shape (up to 4 per track, such
as key and other frames). The fields that change from fragment to fragment are regenerated: the
sequence number, `base_data_offset` (from the fragment's position), the sample's duration, size,
decode time and composition offset, the `mdat` size. The decode time stored in `tfdt` is kept as a
residual against the durations (ffmpeg's MP3 fragments state durations that don't add up to it).
Which track's fragment comes next is predicted from the decode times, so runs of fragments take one
op. Sizes of samples in fragments without per-sample sizes now come from the frame data as well.

| fragment per frame | meta before | meta after |
|---|---:|---:|
| touka19.avi, 2 minutes (DivX + MP3) | 1,041,414 | 5,253 |
| the same, `default_base_moof` (1 minute) | 474,085 | 3,544 |
| the same, first samples in `moov` (1 minute) | 521,106 | 3,584 |
| x264 + AAC, 10 s | 92,741 | 2,897 |
| x265 + AAC, 10 s | 92,786 | 2,943 |
| AAC only, 10 s | 57,019 | 996 |

## MPEG program and transport streams

MPEG-1 system streams and MPEG-2 program streams (`ps.inc`) are a sequence of pack headers and
PES packets; transport streams (`ts.inc`) cut the PES packets into 188-byte packets, with a
4-byte arrival timestamp in front of each in M2TS (Blu-ray, AVCHD). A track is a stream id (for
DVD's private_stream_1, a sub-stream id) or a PID. Its stream holds the PES payloads; what kind of
data they are (MPEG-1/2 video, MPEG-4 Part 2, H.264, HEVC, MPEG audio, AAC, AC-3, DTS, LPCM,
DVD subpictures) is found from the data, which names the stream file. The containers are rebuilt
from small models:

- PES headers: per track, templates of the header bytes with the timestamps blanked, an index per
  PES, and PTS/DTS as differences to the track's previous ones. The length field is derived.
- Program streams: muxers write packets of a fixed size (DVD 2048 bytes, VCD/SVCD 2324), a pack
  header counted in, so the size of a packet that ends where the next one is due is derived; pack
  headers come from the SCR (last SCR plus a constant step, or plus the last step), the mux rate
  and the stuffing; padding packets from their size. For DVD AC-3, the frame count and first
  access unit pointer after the sub-stream id come from the AC-3 frames.
- Transport streams: a track's packets take its PES bytes in turn. Their headers (PUSI, continuity
  counter) are derived, the adaptation field of a PES's first packet is a template (a PCR in it
  comes from a model: last PCR plus its step, DTS minus a delay, or the byte position at a
  constant rate), the packet where a PES ends gets the stuffing that makes it fit. Runs of a
  track's packets take one op. PAT/PMT/SDT packets that repeat the PID's last one with the
  counter + 1 are an op, null packets are an op per run, PCR-only PIDs (Blu-ray, AVCHD) are
  modeled like a track's adaptation fields, and M2TS timestamps are residuals of the last step.
- Frame sizes: one PES per frame in transport streams, so the sizes mostly come from the frame
  data (access units, pictures, audio frames per PES), as for the other containers.

Anything that doesn't fit stays in the struct as it is: system headers, PSI, DVD navigation
packets, the header of a packet that breaks the model, packets of a PES that doesn't parse. A
stream cut anywhere, truncated or damaged still splits, and restores exactly; packets before the
first PES start of a PID (cut streams) stay in the struct.

A raw video elementary stream (`.m4v`, `.m1v`, `.m2v`, `.h264`, `.hevc`) is a file of one track:
the meta is a few bytes and names the stream for its compressor.

Test files: 20 s of touka19.avi converted by ffmpeg 6.1 (`-f mpeg`, `-target pal-vcd`,
`-target pal-svcd`, `-target pal-dvd`, `-f vob` with `pcm_dvd`, `-f mpegts`, `-muxrate 5M`,
`-mpegts_m2ts_mode 1`; the MPEG-4 Part 2 + MP3 ones with the AVI's streams copied). The meta is
meant to be compressed along with the streams; the last column shows how far it shrinks:

| file | size | streams | meta | meta, xz -9 |
|---|---:|---|---:|---:|
| MPEG-1 system stream | 4,141,056 | m1v, mp2 | 6,693 | 1,028 |
| VCD | 3,492,972 | m1v, mp2 | 17,564 | 760 |
| SVCD | 5,856,480 | m2v, mp2 | 7,547 | 1,388 |
| MPEG-2 video in an MPEG-1 system stream | 8,656,896 | m2v, mp2 | 9,677 | 1,928 |
| DVD | 13,355,008 | m2v, ac3 | 81,438 | 2,584 |
| DVD, LPCM audio | 7,315,456 | m2v, pcm | 27,853 | 1,664 |
| MPEG-4 Part 2 + MP3, program stream | 7,188,480 | m4v, mp3 | 7,608 | 792 |
| H.264 + MP2, program stream | 3,182,592 | h264, mp2 | 7,252 | 1,272 |
| MPEG-2 + MP2, transport stream | 8,958,576 | m2v, mp2 | 2,620 | 420 |
| the same, CBR 5 Mbit/s (null and PCR-only packets) | 12,516,476 | m2v, mp2 | 9,588 | 2,920 |
| MPEG-4 Part 2 + MP3, transport stream | 7,477,888 | m4v, mp3 | 2,514 | 464 |
| H.264 + AAC, transport stream | 2,721,300 | h264, aac | 3,876 | 764 |
| HEVC + AAC, transport stream | 1,492,156 | hevc, aac | 5,149 | 1,660 |
| H.264 + AC-3, M2TS | 2,942,976 | h264, ac3 | 4,163 | 820 |

Most of the DVD meta is the navigation packets (2 KB per half second, mostly zeros from ffmpeg),
most of the VCD one the zeros that end each audio sector. The metas of these formats need this
version of avip: an older one restores them as a plain copy, that is, writes the meta itself.
