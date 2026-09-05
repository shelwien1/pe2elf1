# jpegdet — carving JPEGs out of a stream, reversibly

`jpegdet` finds the JPEG images inside an arbitrary byte stream, writes each one
out as a file a decoder will open, and keeps enough beside them to put the
original stream back together byte for byte.

```
jpegdet [options] c <input> <prefix>     ->  <prefix>00000000.jpg, ... + <prefix>.jdm
jpegdet d <prefix> <output>              ->  the original <input>, exactly
```

It is built from the same sources as `pjpg`: `make` produces both.

```sh
cd 011_ && make
./jpegdet -v c /some/disk/image.bin out/img
./jpegdet    d out/img rebuilt.bin
cmp /some/disk/image.bin rebuilt.bin
```

---

## 1. Why the parser is the detector

A signature scan can find `FF D8 FF`. It cannot tell you where the image
**ends**, and that is the whole problem:

* `FF D9` is not a reliable terminator. Entropy-coded data is a bit stream, and
  the byte pair `FF D9` occurs inside it constantly — it is only a marker when
  it falls on the boundary the entropy decoder walks to.
* Cutting at the *last* `FF D9` in the stream is just as wrong: it welds every
  following image and every byte of unrelated data onto the first one.
* Cutting at the *first* one truncates most real images.

Knowing where a scan's data ends means decoding it. So `jpegdet` hands every
candidate offset to `pjpg` — the real parser, entropy decoders and all — and a
candidate is a JPEG only if that parser walks it from SOI to EOI without a fatal
error. The parser's input pointer at that moment *is* the end of the image.

That also makes the detection strong rather than merely plausible. A candidate
has to survive every segment-length check, every table definition, and every
Huffman or arithmetic symbol in every scan. In 64 MB of random data, 1049 byte
positions start with `FF D8`; none of them survives.

## 2. What is written

For a stream that goes *junk, image, junk, image, junk*:

```
<prefix>00000000.jpg     the first image, with its thumbnails taken out
<prefix>00000001.jpg     a thumbnail that came out of it
<prefix>00000002.jpg     the second image
<prefix>.jdm             the three runs of junk, and how to put it all back
```

(`-n` leaves thumbnails inside the images carrying them, and then a file is
exactly a byte range of the stream — see §3.)

The `.jdm` is a flat, record-tagged, little-endian file:

| | field | meaning |
|---|---|---|
| header | `"JPEGDET2"` | magic, 8 bytes |
| | `u32 version` | 2 |
| `'I'` record | `u64 gap_len` + *bytes* | the literal stream data before a top-level image |
| `'D'` record | *an image definition* | that image, straight after its gap |
| `'C'` record | *an image definition* | a thumbnail, written before the image that carried it |
| `'E'` record, once | `u64 tail_len` + *bytes* | literal data after the last image |
| | `u64 orig_size`, `u64 orig_hash` | the input's length, and FNV-1a 64 of it |
| | `"JDMEND"` | terminator |

An image definition:

| field | meaning |
|---|---|
| `u32 index` | which `<prefix>NNNNNNNN.jpg` holds it |
| `u64 file_len` | bytes of that file that are image data |
| `u32 add_len` | bytes of it that are not (a synthesised EOI) |
| `u32 flags` | `1` EOI synthesised, `2` damaged entropy data accepted under `-r`, `4` cut at the next image's SOI, `8` this image is a thumbnail |
| `u64 orig_len` | what putting the thumbnails back has to produce |
| `u32 n_thumbs` | then per thumbnail: `u64 cut_at`, `u32 pre_len` + bytes, `u32 child`, `u64 child_len`, `u32 post_len` + bytes, `u32 n_fix` + `{u64 offset, u32 len, bytes}` |

Four deliberate choices:

* **Record-tagged, not counted.** The number of images is not known until the
  input is exhausted, and a header field patched by seeking back afterwards is a
  field that is wrong whenever the program is killed. `c` writes the file in one
  forward pass; `orig_size` and `orig_hash` live in the terminator for the same
  reason.
* **Scalars written a byte at a time.** The file is read back by a build that may
  not share this one's word size, alignment or byte order. A struct written with
  `fwrite` would agree with none of them.
* **`index` is stored, not implied by record order.** A deleted or renamed `.jpg`
  then produces an error message rather than a silently misassembled output.
* **A thumbnail is defined before the image that carried it.** Everything a
  definition refers to has therefore already been read, so `d` still needs only
  one forward pass — no seeking, and no buffering the gap data to get at a
  reference that has not arrived yet.

`d` checks each `.jpg` is exactly `file_len + add_len` bytes before splicing it
in, that every reassembled image comes to its recorded `orig_len`, and that the
whole stream matches the recorded size and hash. A deleted, truncated or edited
file is reported and the output removed rather than left looking like a
restoration.

## 3. Thumbnails

A JPEG thumbnail is a whole JPEG living inside an APP segment of another one, so
by default `jpegdet` writes it out as an image of its own and patches the segment
that carried it. `-n` turns that off and leaves every thumbnail where it is.

The patch differs by carrier, because what is worth keeping does:

| carrier | what happens |
|---|---|
| **JFXX APP0** (extension code `0x10`) | the segment is nothing but the thumbnail, so the whole segment goes. What is left is an ordinary JFIF file. |
| **Exif APP1** (IFD compression tag `0x0103` = 6) | the segment is metadata that should survive, so only the thumbnail is cut. The segment length is corrected and the IFD's thumbnail-length tag (`0x0202` or `0x0117`) is zeroed — which is exactly what an Exif block with no thumbnail looks like. |

```
$ jpegdet -v c photo.jpg out/i
          00000001  thumbnail       8342 bytes
00000000             0 .. 516726            516726 bytes
1 image(s) and 1 thumbnail(s), 516726 image byte(s), 0 literal byte(s), 516726 total

$ pjpg out/i00000000.jpg | grep thumb
thumbOfs=44 thumbLen=0 thumbTyp=6; pos=0044/0044      <- the Exif survives, the thumbnail is gone
```

A thumbnail is only lifted out if it parses as a JPEG on its own — the same
acceptance predicate every other carved image goes through. One that does not is
left in place, because writing out a file that will not open is the one thing
this must not do. The same applies to a JFXX thumbnail that is not a JPEG at all:
extension codes `0x11` (palette) and `0x13` (RGB) are raw pixels and stay put.

Thumbnails nest, and so does this. Each extracted thumbnail is itself probed, so
its own thumbnails come out as files in turn, up to `JD_MAX_TDEPTH` levels. What
stops it is that limit rather than pjpg's `PjpgLevels`: pjpg nests one parser per
level, but `jpegdet` re-probes each thumbnail on its own, so it goes deeper than
pjpg's own recursion does. On `nest_deep.jpg`, which nests eight levels, pjpg
walks four and `jpegdet` carves all eight.

Restoration puts everything back exactly. The metainfo records, for each
thumbnail, where the removed span was, what framed it, which file holds it, and
the bytes the patch overwrote — so `d` re-inserts the span, undoes the edits, and
reproduces the original byte for byte. A thumbnail's own thumbnails are described
the same way in its own coordinates, and the recursion composes.

## 4. The invariant that makes it lossless

Every byte of the input is accounted for **exactly once**: it goes to a gap, to
an image, to a thumbnail lifted out of an image, or to the tail — and where a
patch changed a byte, the original is recorded beside it. Restoration walks that
back.

This is a property of the *partition*, not of the detection — so it holds no
matter how badly the detector guesses. A stream with no JPEG in it round-trips
through a `.jdm` that is one `'E'` record. The same invariant is what makes
`rawdet.cpp` exact, and `jpegdet` keeps it for the same reason.

With `-n` the partition is a straight concatenation, as it was before thumbnails
were extracted at all; the format is the same either way, with no thumbnail
records in it.

## 5. Making the extracted files openable

An image is carved as the exact slice of the stream between its SOI and its EOI,
so normally there is nothing to repair. Two cases need two bytes:

* **The next image's SOI arrived first.** A JPEG whose EOI is missing does not
  end anywhere a parser can see — it just runs on into whatever follows. The next
  SOI is the only honest boundary, so the parser stops there and the image is cut
  just before it. Without this, both images end up in one file, and `libjpeg`
  refuses it outright: *"Invalid JPEG file structure: two SOI markers"*.
* **The stream stopped mid-image** (`-t`). Only ever the last thing in the file,
  where "the parser ran out of input" and "the image ends here" are the same
  statement. Anywhere else it would swallow everything after it.

In both cases `FF D9` is appended to the `.jpg` and `add_len` records it, so `d`
takes only `img_len` bytes back and the restoration stays exact.

## 6. Conformance, not just parseability

`pjpg` describes what is in a file; a carver has to decide whether the file is
one. The two jobs disagree about several things `pjpg` notes and walks past but a
conforming decoder refuses outright, so those are counted separately (`n_nonconf`)
and disqualify a candidate:

| | libjpeg's verdict |
|---|---|
| a reserved marker between segments: `FF 02`..`FF BF` (RES), `FF C8` (JPG), `FF F0`..`FF FD` (JPG0..JPG13) | *Unsupported marker type 0x..* |
| a segment length below the 2 bytes of the length field | *Bogus marker length* |
| a parsed segment with bytes left over — SOF, SOS, DRI, DQT, DHT, DAC, DNL, EXP all have exact lengths in T.81; APPn and COM do not | *Bogus marker length* |
| a component sampling factor outside T.81's 1..4 | *Bogus sampling factors* |
| an interleaved MCU carrying more than the 10 data units of T.81 A.2.3 | *Sampling factors too large for interleaved scan* |
| a DCT frame whose precision is neither 8 nor 12 (T.81 B.2.2 table B.2; only the lossless processes take the full 2..16) | *Unsupported JPEG data precision N* |
| two frame headers with no DHP to introduce them | *Invalid JPEG file structure: two SOF markers* |
| a scan naming a quantisation table no DQT has defined **yet** | *Quantization table 0x00 was not defined* |

Two of these are worth dwelling on, because both were live holes that a stream
detector reaches and a file parser does not.

**Ordering, not just presence.** T.81 4.5 requires a quantisation table to be
defined *before* the scan that references it, and libjpeg latches the tables at
each scan. Checking only at the end of the file accepts a stream whose DQT
arrives *after* the scan that uses it — which parses perfectly, decodes
perfectly, and does not open. `pjpg` already got the Huffman half of this rule
right (`scan_tables_ready()` runs per scan); the quantisation half now runs in
the same place. `jpegdet` keeps a whole-image check as well, so a component no
scan ever references still has to have a table.

**A rule detected and then forgotten.** `per_scan_setup()` rejects an MCU with
more than 10 data units, but its early `return` jumped over the counters at the
bottom of `decode_scan()` — so the verdict was computed and thrown away, and a
file `djpeg` refuses was carved silently. Two bytes in any 3-component baseline
JPEG reach it.

Going the other way, a malformed **Exif** block does *not* disqualify anything.
It sits inside an APP1 payload that every decoder skips whole, so the image
decodes regardless of what the TIFF structure inside it says — `pjpg` reports it
because describing the file is its job, and `fatal_for_decoding()` is where the
two jobs part company.

A frame header with `Y == 0` is the one case where "conforming" and "libjpeg
opens it" genuinely differ, and `jpegdet` follows T.81 rather than libjpeg:
`Y == 0` means the height arrives in a later DNL segment, so it is carved when a
DNL supplies one and rejected when none ever comes (*"Empty JPEG image"*), even
though libjpeg refuses both.

## 7. Options

```
-s    structure only: do not decode the entropy-coded scans.  Much faster.
      Scan headers and Huffman tables are still checked, but nothing
      verifies the data itself, so a candidate is likelier to be believed.
-r    relaxed: accept an image whose entropy data does not decode cleanly.
-t    also carve a final image that the stream cut short, appending the EOI
      it needs to open ("d" removes it again).
-n    leave thumbnails where they are.  By default a JPEG thumbnail is
      written out as an image of its own and the segment that carried it
      is patched, so what is left is still a JPEG -- without one.
-A    anchor only on FF D8 FF.  Faster on a big stream, but misses an
      image whose SOI is followed by something other than a marker.
-m N  ignore anything shorter than N bytes (default 128).
-L N  give up on a candidate that reads N bytes without a frame header
      (default 16777216).
-v    list every image as it is found, and every candidate turned down.
```

`-v` names the reason for each rejection, which is usually what you want when an
image you expected is not there:

```
00000000            75 .. 1967                1892 bytes
                  1967    rejected: bad scan header
00000001         41066 .. 56256              15190 bytes
```

### Bounding what a hostile stream can cost

Three things bound the work, and the first is not an option because nothing the
frontend can do would reach it.

Once a scan's entropy data runs out, every remaining MCU is decoded against the
zero padding `fill_bit_buffer()` substitutes. Those MCUs **consume no input**, so
their number is whatever the frame header claimed, and since nothing is read,
nothing yields: neither `f_quit` nor any limit kept outside the parser can
interrupt it. A 155-byte file declaring 65535×65535 buys 67 million of them, and
chaining scan headers multiplies it — 395 bytes cost 34 seconds, and 13 KB would
have cost hours. `pjpg` was affected exactly as much as `jpegdet`. Padded MCUs
are now capped per scan (`E_PAD_LIMIT`), high enough that no file whose declared
size bears any relation to its data can reach it — the whole golden corpus is
byte-identical across the change — and low enough that one that does not costs
half a second instead of 34.

`-m` is the minimum size, the same guard `rawdet` applies with `blk_minsize`.
`-L` is the more interesting one: a candidate that is not a JPEG can still parse a long way
without a fatal error, because every byte in `C0`..`FE` is a marker with a
length, so random data reads a random length and skips it, over and over. The one
thing it will not do is produce a frame header, and a real image has one within
its first few segments — APPn payloads are capped at 64 KB each, so even a file
carrying a chunked ICC profile and an MPF index gets there well inside the
default. Without the cap, a stream with many false signatures would cost a full
read of the tail for each one.

## 8. What it does and does not find

Measured on 48 conforming images produced by the JPEG XT reference encoder (see
`exotic-samples.md`) — baseline, extended sequential, progressive, arithmetic,
lossless SOF3 and SOF11, hierarchical DHP/SOF5/SOF6/SOF7, DNL, restart intervals,
4:2:0, 12- and 16-bit, and the JPEG XT APP11 residual and alpha families:

**43 of 48 are carved whole and round-trip exactly.** The five misses are all
JPEG-LS, which `pjpg` cannot parse at all — it has no SOF55 or LSE handler, and
T.87's bit-stuffing makes `FF 01`..`FF 7F` data rather than markers, which
desynchronises the marker scanner. That gap is documented in
`exotic-samples.md §6`.

Deliberately **not** carved:

* **Thumbnails.** An Exif or JFXX thumbnail is a complete JPEG inside its
  parent's APP1/APP0 payload. It travels with the parent, so extracting it
  separately would break the concatenation. (If the *parent* is rejected, the
  scan resumes two bytes in and the thumbnail is found on its own — which is
  usually what you want from a damaged file.)
* **Broken images.** Against the `imagetestsuite` torture corpus, most files are
  rejected, every one for a substantive reason: truncated, bad scan header, bad
  Huffman table, bad arithmetic conditioning, non-conforming segment. `libjpeg`
  is forgiving enough to emit partial garbage for many of these; a carver that
  writes files claiming to be JPEGs should not be.

Throughput, on a stream of 16 concatenated real images: **52 MB/s** with full
entropy decoding, **385 MB/s** with `-s`.

## 9. Testing

```sh
make carve      # or 'make test', which includes it
```

The `carve` case builds ten streams — a lone image, one sandwiched in text, two
adjacent, two separated by noise, a stream with no JPEG, an empty stream, and an
image whose EOI was stripped followed by another image — and asserts three things
for each:

1. `c` then `d` reproduces the input byte for byte.
2. Every carved file decodes (judged by whether `djpeg` produces an image, not by
   its exit status: `libjpeg` exits 2 for a warning on a file it decoded fine).
3. The number of images carved is exactly what that stream contains — counted
   from the summary line rather than from the files written, since a thumbnail
   gets a file of its own too.

The third assertion is the one that matters most: nothing stops a carver from
being trivially lossless by never carving anything.

Three of them are hazard regressions, each a defect that testing
actually found: `spin` (the padded-MCU blow-up above, run under a 20-second
timeout rather than the 300 the others get, since the point is that it used to
take 34), `poison` (a truncated *arithmetic* image in front of ordinary Huffman
ones — `ar_dead` was cleared only on the arithmetic path but read on every path,
so one dead arithmetic scan condemned every later image in the run), and
`bigsamp` (an MCU over the 10-data-unit limit, the rule that was detected and
then forgotten).

Beyond the checked-in case, the implementation was validated against 27 synthetic
stream shapes across six flag combinations and 900 fuzz iterations over mutated
and truncated real images — byte-exact round trip and a decodable output every
time — plus a clean UBSan run over the whole corpus and byte-identical results
from six compiler and flag configurations (gcc/clang, `-O0`..`-O3`,
`CORO_FRAME_POINTER`, `CORO_NOASM`).

Failure paths are checked too, because a carver that reports success after
writing a truncated file is worse than one that fails: every output file is
closed through `close_ok()`, which flushes and takes `fclose`'s return, since on
a full disk that is usually the only place the error surfaces. An input that
cannot be seeked (a pipe, a character device) is refused up front rather than
carved as an empty stream — every candidate is revisited by seeking, so there is
nothing to carve.

AddressSanitizer reports a stack-buffer-underflow inside `yield()` for `jpegdet`
exactly as it does for `pjpg`: the coroutine copies a live stack region by hand.
See `README.md`.
