# jpgcoder — a JPEG as its coefficients, and back again

```
jpgcoder [-v] c <in.jpg> <out.hdr> <out.coef>
jpgcoder [-v] d <in.hdr> <out.jpg> <in.coef>
```

`c` takes a JPEG apart into the numbers the encoder actually worked with; `d`
puts it back, byte for byte. `make` builds it alongside `pjpg` and `jpegdet`.

```sh
cd 011_ && make
./jpgcoder -v c photo.jpg photo.hdr photo.coef
./jpgcoder    d photo.hdr rebuilt.jpg photo.coef
cmp photo.jpg rebuilt.jpg
```

---

## 1. What comes out

**`.coef`** is the image as DCT coefficients: one 16-bit signed value each, in
zigzag order, **band by band** — every block's coefficient 0, then every block's
coefficient 1, and so on, per component. That layout is the point. Coefficients
at the same frequency in neighbouring blocks are strongly related and
coefficients at different frequencies are not, so putting like with like is what
any model of them wants, and nothing about the entropy coding is left in the
way.

The grid is padded to whole MCUs, because that is what the bitstream codes: an
image 17 pixels wide with 2×1 sampling still has a block column past the right
edge in every MCU row, and it carries coded data that has to come back.

**`.hdr`** is everything else — the marker segments, and what it takes to put
the entropy coding back exactly.

## 2. Why "exactly" is the hard part

Decoding a scan and encoding it again gives a stream that *means* the same thing
but need not be *the same bytes*. Where an EOB run is flushed, how a run of
sixteen zeroes is split, which bit pads the last byte — all of these are choices,
and the file being reproduced made them once already.

So this does not assume. **Every scan is re-encoded at compress time and compared
against the original bytes.** A scan that comes back identical is stored as
coefficients; one that does not is left in the header verbatim. The round trip is
therefore exact for *every* input, and what varies is only how much of the file
reached the coefficient form — which the summary line says:

```
$ jpgcoder -v c laplata4_1.prog.jpg h c
10 scan(s), 10 as coefficients; 135168 coefficients, header 636 of 6066 bytes
  scan 0           244..893         prog     Ss=0  Se=0  Ah=0  Al=1   coefficients
  scan 5          1580..2428        prog     Ss=1  Se=63 Ah=2  Al=1   coefficients
  ...
```

This is the difference from packJPG's `jpgcoder`, which errors out with
*"reconstruction of non optimal coding not supported"* when it meets a stream it
cannot reproduce. Here that case costs coverage, not correctness.

## 3. How it is built out of pjpg

pjpg already reads every marker and decodes every Huffman and arithmetic symbol
of every scan — it just throws the values away, keeping one bit per coefficient
so refinement scans parse. Three things were added, all inert unless asked for:

* **Coefficient capture.** The decoders write each block's 64 values into a
  buffer when one is supplied. With real values in hand, the refinement decoder
  reads "is this coefficient already nonzero" from the values themselves rather
  than from the bitmap, which is the same question answered exactly.
* **Positions.** The outer loop notes where each segment starts, once; `l_tag`
  already says how far into a payload a parser is. So "where in the file did
  this byte come from" is arithmetic, not a counter on the hot path.
* **A scan sink.** A hook called at each scan with the parser's state still in
  hand — the scan's components, its spectral range, its tables, its restart
  interval. That is exactly what an encoder needs, and it is what both halves of
  the job hang off.

The encoder (`jpgenc.inc`) mirrors libjpeg's `jchuff.c` and `jcphuff.c` rather
than being written afresh, for the same reason the decoders mirror `jdhuff.c`:
the files this has to reproduce were overwhelmingly made by libjpeg, and its
choices are what "the same bytes" means in practice.

Both directions run pjpg. Compressing runs it over the file; decompressing runs
it over the **header alone**, which still contains every segment — so the parser
hands the encoder the same scan state it gave the decoder, and nothing has to be
parsed twice by two different pieces of code.

## 4. What transcodes and what does not

Measured over 187 files — the bundled corpus, the 98 deliberately-damaged images
of `imagetestsuite`, the nested-thumbnail set, and 48 exotic frame types from the
JPEG XT reference encoder:

**187 of 187 round-trip byte-exact.** 170 of 346 scans reached the coefficient
form. The rest divide into three, and only one of them is a gap:

| | why |
|---|---|
| **arithmetic scans** (52) | pjpg decodes them, but there is no arithmetic *encoder* here. packJPG's jpgcoder has none either — it refuses these files outright. |
| **not DCT-coded at all** | lossless-predictive (SOF3/SOF11) and hierarchical frames have no DCT coefficients to store. |
| **damaged scans** | a truncated scan re-encodes to a complete one, which is not the same bytes, so it stays raw. Correct, not a shortfall. |

On well-formed Huffman JPEGs — baseline, extended sequential, and progressive in
all four scan shapes — coverage is effectively total: of the 92 non-arithmetic
scans in the clean corpus, 83 transcode, and all nine that do not are in one file
whose entropy data is truncated.

**Known gap:** the differential frames of a *hierarchical* stream. Their first
frame transcodes; the differential frames that follow are left in the header. The
coefficient grid is built once, at the first scan, and a differential frame has
its own geometry. Worth fixing; costs nothing but coverage today.

## 5. Testing

```sh
make coder      # or 'make test', which includes it
```

Every bundled image, including the damaged corpus, is taken apart and put back,
and the result must equal the input byte for byte. The target also asserts that
*something* reached the coefficient form — a transcoder that never transcodes
anything would pass the round-trip check on its own, since leaving every scan in
the header is trivially lossless.

Beyond that: UBSan clean over the whole corpus in both directions, no leaks, and
five compiler and flag configurations producing identical output.

AddressSanitizer reports a stack-buffer-underflow inside `yield()` here exactly
as it does for `pjpg`: the coroutine copies a live stack region by hand. See
`README.md`.
