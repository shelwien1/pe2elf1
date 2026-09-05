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
against the original bytes.** The arithmetic coder has a termination procedure
rather than a padding bit, but the same applies: T.81 D.1.8 accepts any value in
the final interval, and only one of them is the one in the file. A scan that comes back identical is stored as
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

The encoders mirror libjpeg rather than being written afresh, for the same reason
the decoders mirror `jdhuff.c` and `jdarith.c`: the files this has to reproduce
were overwhelmingly made by libjpeg, and its choices are what "the same bytes"
means in practice. `jpgenc.inc` is `jchuff.c` and `jcphuff.c`; `jpgarith.inc` is
`jcarith.c` — the QM coder of T.81 Annex D with libjpeg's "Pacman" flush, which
of the conforming ways to terminate a scan picks the shortest.

Two things changed on the decoding side to feed them. The arithmetic decoders
computed each coefficient and threw it away, keeping only the nonzero bitmap
refinement scans need, so they now store what they compute — the same buffer, on
the same terms, as the Huffman ones.

The other is fidelity rather than a fix. T.851 10.3 has the sign bits and the DC
refinement bits coded against a bin that never adapts, and libjpeg gives it its
own storage: `fixed_bin`, parked in state 113, whose transitions point back at
itself. The port instead borrowed `ar_ac_stats[tbl][245]` and zeroed it before
each use, which codes exactly the same decisions — state 0 has the same Qe of
0x5A1D — and 245 really is free, since the highest index any real context reaches
is 244. But that is a bound nobody writes down, holding by 1, in a shared array;
and the encoder mirrors `jcarith.c`, which has no such arrangement. So the bin
now has its own four bytes on both sides and `jaritab` carries T.851's 114th
entry, and neither side depends on the coincidence.

Both directions run pjpg. Compressing runs it over the file; decompressing runs
it over the **header alone**, which still contains every segment — so the parser
hands the encoder the same scan state it gave the decoder, and nothing has to be
parsed twice by two different pieces of code.

## 4. What transcodes and what does not

Measured over what `make coder` runs on: the bundled corpus including the
nested-thumbnail set and the transcoded coder variants, plus the 98
deliberately-damaged images of `imagetestsuite`. 145 files.

**145 of 145 round-trip byte-exact.** 223 of 318 scans reached the coefficient
form. Split by whether pjpg could decode the file at all:

| | scans | as coefficients |
|---|---|---|
| **files that decode completely** (82) | 208 | **207** |
| **files with a scan that stops part-way** (63) | 110 | 16 |

So on well-formed input coverage is all but total, and it does not depend on
which entropy coder the file used: all 82 arithmetic scans in the corpus
transcode, sequential and progressive alike, with and without restart intervals.
The single miss in the first row is a file whose sampling factors T.81 forbids
(14h × 5v), which pjpg refuses to decode, so there are no coefficients to store.

The second row is not a shortfall. A truncated scan re-encodes to a *complete*
one — the encoder has the whole coefficient grid and writes all of it — which is
not the same bytes, so it stays raw. That is the fallback working.

Two shapes have no coefficients to store at all and never will:
lossless-predictive (SOF3/SOF11) and JPEG-LS frames are not DCT-coded.

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

`make coders` covers the arithmetic encoder's own corner: `testfiles/coders/`
carries `-restart 1B` versions of the arithmetic pair, which put an RSTn at every
MCU row. A restart makes the coder terminate its interval and start a fresh one
mid-scan, and no file without restart markers goes near that path.

Beyond that: UBSan clean over the whole corpus in both directions, LeakSanitizer
clean including the error paths, and the full compiler and flag matrix producing
identical output. (`d` used to hold on to the coefficient buffer and the
re-encoded scans until exit; both are freed now, so a leak report means a real
one.)

AddressSanitizer reports a stack-buffer-underflow inside `yield()` here exactly
as it does for `pjpg`: the coroutine copies a live stack region by hand. See
`README.md`.
