# oggsplit

oggsplit splits an Ogg Vorbis file into parts that are valid Ogg Vorbis files by themselves, so that
they can be processed in parallel ([ogg2tsv](../ogg2tsv) takes each of them). It also joins the
parts back into the original file. It is the Ogg Vorbis counterpart of m4vsplit.

## Usage

```
oggsplit N [options] input.ogg output_prefix
oggsplit r [options] input_prefix output.ogg
```

- `N` splits the file into at most N parts: `<prefix>.000.ogg`, `<prefix>.001.ogg`, ... and
  `<prefix>.meta`.
- `r` joins the parts back into the original file.
- `-v` prints statistics.

```
$ oggsplit 4 -v 07.ogg s
217 pages, 0 bytes of junk in 0 runs, 1 link (1 Vorbis), 3764020 samples, 125 cut points (0 where links start)
s.000.ogg: 197657 bytes at 0, 47 pages, +0 bytes in 0 pages, EOS set
s.001.ogg: 248284 bytes at 197657, 58 pages, +5014 bytes in 2 pages, EOS set
s.002.ogg: 242468 bytes at 445941, 57 pages, +5014 bytes in 2 pages, EOS set
s.003.ogg: 231234 bytes at 688409, 55 pages, +5014 bytes in 2 pages
$ oggsplit r s restored.ogg
```

There are fewer than N parts when there aren't enough cut points; oggsplit then says how many it
made. Errors end with exit code 1, and usage errors with 2.

## How it works

The file is read as a sequence of Ogg pages, found as libogg finds them: `OggS`, version 0, the
header and body complete, the CRC right. Where no page starts, the next one is looked for from the
next byte on. Bytes that aren't part of a page (junk) stay where they are.

A link (of a chained file) starts with the first BOS page, or with a BOS page after the EOS pages
of all the streams of the previous link. The BOS pages right after it start the link's other
streams, if it has more than one.

**Cut points.** There are two kinds:
- before a link, but not the first one (whatever comes before the first link stays with it, so the
  junk of a tag that precedes a file doesn't make a part of its own): nothing changes there;
- within a link that is a single Vorbis stream, between two audio pages that no packet runs across:
  the page before ends with a complete packet, the page after starts with a new one. An audio
  packet needs nothing but the setup header to be parsed, nor to be decoded but for its overlap with
  the packet before it (see below). The page after the cut must have a granule position, and can't
  be the EOS page. Decoders cut the last packet down to the EOS page's
  granule position, which they can only do if an earlier page gave them a position to count from.
  A part made of the headers and the EOS page alone would decode to up to half a block too many
  samples at the end.

ffmpeg and libogg 1.3 end nearly every page with a packet, so every audio page but the first and
the EOS page is a cut point (82 of the 86 pages of an 85 s file from ffmpeg). Older libogg let
packets run across pages: the 07.ogg above (libVorbis 1.0 RC2, 2001) has cut points at 125 of its
217 pages.

**A cut within a link.** The part before the cut gets the EOS flag on its last page, with the CRC
redone. The part after it gets the stream's header packets on pages of its own at its start, laid
out as libvorbis does: the identification header alone on a BOS page, then the comment and setup
headers, the setup header ending the last page. Their sequence numbers lead into the page at the
cut, so libogg sees no gap. The comment header keeps the vendor string but not the comments, since
comments can hold cover pictures of hundreds of KB, and nothing that decodes or parses the audio
reads them.

**Links that aren't cut.** ogg2tsv takes all the pages of a link as its stream, while a decoder
only takes those with the stream's serial number, and takes a BOS page with another serial number
for a new link. Cut points could only satisfy both while the two agree, so a page of another
stream, or a BOS page that doesn't start a link, ends the cut points of its link. A link is only
cut where it starts if it has more than one stream (Theora with Vorbis, say), if it isn't Vorbis
(Opus, FLAC, ...), if its header packets share pages with audio, or if its setup header is longer
than 1 MiB (oggsplit warns about these).

**Balancing.** Among the cut points, oggsplit chooses the ones that make the largest part as small
as possible (h264split's search), and then, within that bound, the cuts nearest to an even split.
A part's cost is 6 per byte plus 1 per sample per channel, with the samples taken from the granule
positions. Over the pages of 12 streams (8 to 96 kHz, 1 to 6 channels, 48 kbps to q10), ogg2tsv
wrote 2.46 values per byte plus 0.42 per sample per channel, taking 20 to 31 ns per value. A byte
holds more values at low bit rates, and a sample more at high ones. Within a stream the two go
together, but across links they don't: on a chain of a q10, a q0 and an 8 kHz mono stream cut into
8 parts by size alone, ogg2tsv took 1.41 times the mean time on the largest part. With the samples
counted, it took 1.12 times (weights from 2 to 20 per byte did about as well).

**The meta file.** `<prefix>.meta` holds only what the parts can't tell: for each part after the
first, the number of pages inserted at its start, as a LEB128 varint (one byte up to 127); 0 for a
cut before a link. There is one more part than there are values. When a value isn't 0, the part
before it got the EOS flag on its last page. `r` removes the inserted pages (checking that they are
pages, the first one a BOS page), clears that flag, puts the CRC back, and joins the parts. It finds
the last page of a part by looking back from the end for the last place where a page starts that
ends at the end of the part. An audio page can hold bytes that form a page with the right CRC
ending where it ends, which would then be taken for it, so the split doesn't cut after such a page.

## Decoding the parts

FFmpeg and libvorbisfile decode each part of a file that they decode cleanly to an exact slice of
the file's samples, with no holes. The slices leave out the samples at each cut that the part's first packet would return.
A Vorbis packet returns the overlap of its block with the previous one, and the first packet of a
part has nothing to overlap with. That is a quarter of each of the two blocks, from half a short
block to half a long one: 128 to 1024 samples (3 to 23 ms) with the 256 and 2048 sample blocks that
libvorbis uses at 44.1 kHz from q0 to q10.

## Tests

- **Round trips:** splitting and joining gave back the original for each of 53 test files, with N
  of 1, 2, 3, 8 and 100. The files were:
  - 07.ogg, and ffmpeg encodes (libvorbis and its native encoder) at 48 kbps to q10, 8 to 96 kHz,
    1 to 6 channels, from 0.05 s to 17 minutes long;
  - chained files (distinct serial numbers, all the same, mixed qualities, Vorbis then Opus then
    Vorbis), two Vorbis streams in one link, Theora with Vorbis, Opus and FLAC;
  - a 400 KB comment header;
  - the same packets laid out on other pages: random page lengths, 1 to 3 segments a page, 255,
    the header packets sharing pages with audio, empty pages between packets, packets never ending
    a page;
  - junk before, after and among the pages, a capture pattern whose page is cut off, a page with a
    wrong CRC, files cut off inside a page and between pages, headers only, no headers, pages after
    the EOS page, a repeated BOS page, an audio page ending with a valid page, and empty, random
    and 4-byte files.
- **ogg2tsv:** when ogg2tsv takes a file, it takes each of its parts, and converts them back.
- **Decoding:** every part of the files that decode cleanly decodes to its exact slice, as above,
  with FFmpeg (files of one link) and libvorbisfile.
- **Fuzzing:** 3000 damaged files, split into 1 to 1000 parts and joined under AddressSanitizer
  and UBSan, all came back as they were. The damage was bit flips, truncation, inserted garbage
  (with capture patterns), removed ranges, changed flags, serial numbers, granule positions and
  sequence numbers (with the CRC put right), pages repeated, removed or added empty, and valid
  pages written over the ends of pages. Of the 723 that ogg2tsv took, it took every part.
- **Windows:** the mingw build, under wine, writes the same parts and meta file.
- **Speed:** a 16 MB file (17 minutes) splits in 0.07 s and joins in 0.02 s, in 13 MB of memory.

## Build

```
g++ -O2 -std=c++17 -o oggsplit oggsplit.cpp
x86_64-w64-mingw32-g++ -O2 -s -static -std=c++17 -o oggsplit.exe oggsplit.cpp
```
