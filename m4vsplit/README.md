# m4vsplit

m4vsplit splits an MPEG-4 Part 2 video elementary stream (`.m4v`: XviD, DivX, libavcodec `mpeg4`,
...) into parts that are valid streams by themselves, so that they can be processed in parallel.
It also joins the parts back into the original stream. It is the MPEG-4 counterpart of h264split,
and [m4vpack](../m4vpack)'s `-t` cuts streams by the same rules.

## Usage

```
m4vsplit N [options] input.m4v output_prefix
m4vsplit r [options] input_prefix output.m4v
```

- `N` splits the stream into at most N parts: `<prefix>.000.m4v`, `<prefix>.001.m4v`, ... and
  `<prefix>.meta`.
- `r` joins the parts back into the original stream.
- `-i` also cuts before I-VOPs that B-VOPs follow (those B-VOPs reference the previous part).
- `-v` prints statistics.

```
$ m4vsplit 4 -v aq_cm.m4v aq
1366 chunks, 1258 VOPs (I 18, P 351, B 574, S 0, not coded 315), 724224 macroblocks, 17 cut points (0 I-VOPs that B-VOPs follow, not used)
aq.000.m4v: 1106590 bytes at 0, 437 VOPs, +0 bytes in 0 headers
aq.001.m4v: 1009386 bytes at 1106590, 205 VOPs, +0 bytes in 0 headers
aq.002.m4v: 851945 bytes at 2115976, 196 VOPs, +0 bytes in 0 headers
aq.003.m4v: 907272 bytes at 2967921, 420 VOPs, +0 bytes in 0 headers
$ m4vsplit r aq restored.m4v
```

There are fewer than N parts when there aren't enough cut points; m4vsplit then says how many it
made. Errors end with exit code 1, and usage errors with 2.

## How it works

The stream is cut into chunks at start codes (`00 00 01 xx`). A chunk runs from its start code to
the next one.

**Cut points.** A cut goes before an I-VOP that starts a closed group, meaning the next coded VOP
isn't a B-VOP. A B-VOP that follows an I-VOP is displayed before it and also references the VOP
before it, so a part starting at that I-VOP couldn't decode it. With `-i`, any I-VOP will do. If
the I-VOP has a run of headers right before it (VOS, VO, video object, VOL, user data, GOV), the
cut goes before the run. VOPs that aren't coded (`vop_coded` 0) don't count, of whatever type.
These are the placeholders that packed B-frames leave, and a part starting at one would have no
picture to show.

**Balancing.** Among the cut points, m4vsplit chooses the ones that make the largest part as small
as possible (h264split's search). Then, within that bound, it picks the cuts nearest to an even
split. A part's cost is 5 per byte plus 1 per macroblock of its coded VOPs. m4vpack (format 2)
took about 0.76 µs per byte and 0.16 µs per macroblock (fitted over 12 streams), so a macroblock
costs about a fifth of a byte, and a part of many small pictures takes longer than one of the same
size with few large ones. The model of format 3 spends more per byte (1.1 µs), and the same fit no
longer separates the macroblocks' share from the noise, so the weight now overstates it a little.

**Headers copied in.** A part gets copies of the headers known at its start that its own first run
of headers doesn't define. These go at its start, in this order:
- the last VOS, VO, video object and VOL headers, each start code value on its own;
- the last user data of each kind, told apart by its first 4 bytes ("DivX", "XviD", "Lavc", ...)
  and ordered by when each kind first came.

Decoders read the user data to work around encoder bugs, so a part decodes as it did in the whole
stream. Headers longer than 1 MiB aren't copied, and m4vsplit warns about them.

**The meta file.** `<prefix>.meta` holds only what the parts can't tell: for each part after the
first, the number of chunks inserted into it, as a LEB128 varint (one byte up to 127). There is one
more part than there are values. The count can't be read off a part, since its own headers may be
the same as the copies. `r` removes that many chunks from the start of each part, checking that
they are headers, and joins the rest.

## Tests

- **Round trips:** splitting and joining gave back the original for each of the 57 test files
  (the m4vpack test set, plus junk before the first start code, a VOS end code, a stream cut off
  inside a VOP, start codes at the very end, concatenated streams, empty and random files), with N
  of 1, 2, 3, 8 and 100, with and without `-i`.
- **Decoding:** FFmpeg decodes each part of the packed-B-frame XviD and DivX samples to the frames
  the whole stream gives. The only difference is that FFmpeg shows each part's last frame twice
  when the stream ends, as it does at the end of the whole stream.
- **Fuzzing:** 150 damaged streams (bit flips, truncation, inserted garbage and fake start codes),
  split with and without `-i` and joined, under AddressSanitizer and UBSan, all came back as they
  were.
- **Windows:** the mingw build, under wine, writes the same parts and meta file.
- **Speed:** a 21 MB stream splits in 0.17 s and joins in 0.17 s.

## Build

```
g++ -O2 -std=c++17 -o m4vsplit m4vsplit.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -std=c++17 -o m4vsplit.exe m4vsplit.cpp
```
