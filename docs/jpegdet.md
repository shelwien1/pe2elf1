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
<prefix>00000000.jpg     the first image, byte for byte out of the stream
<prefix>00000001.jpg     the second
<prefix>.jdm             the three runs of junk, and how to interleave them
```

The `.jdm` is a flat, record-tagged, little-endian file:

| | field | meaning |
|---|---|---|
| header | `"JPEGDET1"` | magic, 8 bytes |
| | `u32 version` | 1 |
| `'I'` record, once per image | `u64 gap_len` | literal stream bytes before this image |
| | *gap_len bytes* | those bytes |
| | `u64 img_len` | how much of the .jpg belongs to the stream |
| | `u32 add_len` | bytes appended to the .jpg that do not |
| | `u32 flags` | see below |
| | `u32 index` | which `<prefix>NNNNNNNN.jpg` this is |
| `'E'` record, once | `u64 tail_len` | literal bytes after the last image |
| | *tail_len bytes* | those bytes |
| | `u64 orig_size` | the input's length |
| | `u64 orig_hash` | FNV-1a 64 of the input |
| | `"JDMEND"` | terminator |

Flags: `1` an EOI was synthesised, `2` accepted under `-r` with entropy data that
did not decode, `4` cut short by the next image's SOI rather than by an EOI.

Three deliberate choices:

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

`d` also checks each `.jpg` is exactly `img_len + add_len` bytes before splicing
it in, and verifies the rebuilt length and hash at the end. Editing an extracted
image and re-running `d` is reported, not baked into the output.

## 3. The invariant that makes it lossless

Every byte of the input is written **exactly once**, in order, unmodified: to a
gap, to an image, or to the tail. Restoration is a concatenation.

This is a property of the *partition*, not of the detection — so it holds no
matter how badly the detector guesses. A stream with no JPEG in it round-trips
through a `.jdm` that is one `'E'` record. The same invariant is what makes
`rawdet.cpp` exact, and `jpegdet` keeps it for the same reason.

## 4. Making the extracted files openable

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

## 5. Conformance, not just parseability

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

## 6. Options

```
-s    structure only: do not decode the entropy-coded scans.  Much faster.
      Scan headers and Huffman tables are still checked, but nothing
      verifies the data itself, so a candidate is likelier to be believed.
-r    relaxed: accept an image whose entropy data does not decode cleanly.
-t    also carve a final image that the stream cut short, appending the EOI
      it needs to open ("d" removes it again).
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

## 7. What it does and does not find

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

## 8. Testing

```sh
make carve      # or 'make test', which includes it
```

The `carve` case builds seven streams — a lone image, one sandwiched in text, two
adjacent, two separated by noise, a stream with no JPEG, an empty stream, and an
image whose EOI was stripped followed by another image — and asserts three things
for each:

1. `c` then `d` reproduces the input byte for byte.
2. Every carved file decodes (judged by whether `djpeg` produces an image, not by
   its exit status: `libjpeg` exits 2 for a warning on a file it decoded fine).
3. The number of images carved is exactly what that stream contains.

The third assertion is the one that matters most: nothing stops a carver from
being trivially lossless by never carving anything.

Three of the ten streams are hazard regressions, each a defect that testing
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
