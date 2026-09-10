# testfiles/

The corpus `./t.sh` round-trips and `./mk.sh check` codes under both builds:
Vorbis streams, and then the files that are not quite that -- tagged, cut,
multiplexed, damaged, or not Ogg at all -- which `oggcomp c` takes just the
same and has to give back byte for byte.

Every file here is synthesized -- `./testfiles/gen.sh` writes the audio with
python and encodes it with `oggenc`, and derives the rest from those with a
seeded generator -- so nothing in it is anyone's music and all of it can be
rebuilt.  The encodes fix `--serial`, which oggenc otherwise picks at random,
so rebuilding gives back the same bytes and `git diff` over this directory
means something.  The files are checked in anyway: a fresh clone can test
the compressor with no encoder installed, which is the point.

    ./t.sh                    round-trip all of it
    ./mk.sh check             code all of it under both builds and compare
    ./testfiles/gen.sh -f     build it again from nothing

## What each file is for

| file | what it exercises |
|---|---|
| `sine-stereo-q5.ogg` | two steady tones: nearly all floor, almost no residue |
| `noise-stereo-q3.ogg` | white noise: all residue, and already close to incompressible |
| `mono-22k-q4.ogg` | one channel, and a sample rate that is not 44100 |
| `silence-stereo-q4.ogg` | digital silence -- empty residue partitions, packets that carry no bits |
| `tiny-8k-q0.ogg` | a few pages in total: the headers are most of the file |
| `sweep-mono-qm1.ogg` | a sweep, so no two packets share a floor; `-q -1`, the smallest setup the encoder writes |
| `chirp-stereo-q10.ogg` | `-q 10`: the long blocksize and the widest books |
| `multi6-48k-q4.ogg` | six channels, so 5.1 coupling and six residue vectors per packet |
| `music-stereo-q5.ogg` | the nearest thing here to music, and the size worth watching between revisions |
| `music-managed-b96.ogg` | the same audio bitrate-managed rather than VBR: the encoder pads and truncates packets |
| `bigcomment-8k.ogg` | two 66 kB comments: a packet across three pages, so the middle one is both a continuation and itself continued.  Also the only granulepos of -1 here |
| `uncoupled-stereo-q4.ogg` | stereo with coupling turned off: residue type 1 with more than one vector, which no quality setting reaches |
| `silence-8k-long-qm1.ogg` | pages filled to the 255-packet ceiling, which is as many as an Ogg page can hold |
| `zerolen-8k-q0.ogg` | no samples: three headers and one audio packet, the smallest legal Vorbis stream |
| `tags-many-8k.ogg` | a dozen short tags, a date, a track number, non-ASCII, a value with spaces -- the ordinary metadata case the big-comment file is not |
| `chained.ogg` | two logical bitstreams in one file, with different serials, rates and channel counts |

## Not one Vorbis stream from start to end

Each of these was once refused.  Now each has to come back byte for byte,
and what `./t.sh -v` shows on the `raw` line is how much of it the Vorbis
model could not take: bytes the parser cannot place are coded as bytes, at
about eight bits each, and everything else as Vorbis.

| file | what it is | what is coded as bytes |
|---|---|---|
| `badcrc-8k.ogg` | one flipped byte in the last page, with the stored CRC left alone | the four CRC bytes; the page itself still parses and is modelled |
| `no-eos-8k.ogg` | the end-of-stream flag cleared, and the page re-CRCed so that is what is tested | nothing: a stream that stops just stops |
| `id3-prefix-8k.ogg` | a bare 42-byte ID3v2.4 tag in front of the first page | the tag |
| `id3-text-8k.ogg` | an ID3v2.3 tag with a title, artist, album, track, year and comment in it | the tag -- text, which the byte model gets at under six bits a byte |
| `id3-apic-8k.ogg` | an ID3v2.3 tag carrying a PNG of random pixels, the shape of cover art | the tag, at eight bits a byte, as random bytes should cost |
| `id3v1-trailer-8k.ogg` | the 128-byte ID3v1 "TAG" block after the last page | the block |
| `trailing-junk-8k.ogg` | not-Ogg after the last page | the 16 bytes |
| `chained-id3.ogg` | two streams, each behind its own tag: junk between links, not only in front | both tags |
| `opus.ogg` | a sound Ogg page whose codec is Opus, not Vorbis | the payload; the page header is modelled |
| `skeleton-8k.ogg` | `oggenc -k`: an Ogg Skeleton stream multiplexed in beside the Vorbis one | the Skeleton pages' payloads; the Vorbis pages, and the packet that spans a Skeleton page, are Vorbis |
| `minbitrate-pad.ogg` | `oggenc -m 128` on silence: libvorbis pads the audio packets with zero bits | nothing: the padding is coded per packet, and zero padding costs a flag |
| `empty.ogg` | zero bytes | nothing |
| `random.bin` | 4 kB from the seeded generator: not Ogg at all | all of it, and the number to watch is how little it grows |
| `page-in-random.bin` | one valid page, the first of `tiny-8k-q0.ogg`, inside random bytes, with a false "OggS" in front of it | the random bytes; the page is found, parsed and modelled, and the four letters are not taken for one |

`skeleton-8k.ogg` and `minbitrate-pad.ogg` are not damaged files.  They are
what a working encoder writes, and the corpus of hand-made cases above
would never show them.

## Not covered

Things the parser handles that nothing oggenc can produce reaches, listed
so that the corpus is not mistaken for complete:

- **residue type 0.**  libvorbis emits only types 1 and 2, so the type-0
  partition path and its divisibility check are never entered.
- **floor type 0.**  Refused by design, and no encoder has written one in
  twenty years.
- **codebook lookup type 2, and residue books without a lookup table.**
  Legal Vorbis, never emitted.
- **the internal ceilings** -- 256 codebooks, 64 floors, 2^20 entries and
  the rest.  Real setup headers stay far inside all of them.

Closing any of those needs a setup header written by hand, not a different
oggenc command line.
