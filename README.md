# pdfdump

Lossless PDF dump/restore. `c` splits a PDF into typed payload files plus a few channel
files; `d` rebuilds the original byte-for-byte. Design and roadmap: [docs/pdfdump_plan.md](docs/pdfdump_plan.md).

    pdfdump c [options] input.pdf outprefix      dump
    pdfdump d inprefix restored.pdf              restore, byte-exact
    pdfdump [options] input.pdf                  info only: items, streams, coverage
    pdfdump selftest                             crypto test vectors, filter round trips

## Status

Milestones **M0–M4** of the plan are implemented: every stream payload is decoded as far as it
can be re-encoded exactly and written as a typed file, encrypted files included. On top of that,
the PDF-specific data is transformed so that xz compresses it better. The transforms cover the
redundancy inventory of §2A, M6, M7, M9 stage 1, and the M10 ideas that measured a gain:

| Transform | What it removes / reorders |
|---|---|
| Type 1 decryption (`L_T1`) | eexec and charstring encryption of embedded Type 1 fonts → `.t1d`; charstrings split into lead bytes, operators and operands |
| Number lifting, content streams | numbers → `.csn`, grouped by (operator, operand slot); canonical hex strings → `.csh` (binary: all string lengths, then all string bytes; −10% xz of `.csh` against interleaving them) |
| Number lifting, object text | numbers of `.txt` → `.txn`, grouped by (key, in-array, terminator) |
| Adaptive number coding | per group: varint or fixed width, raw or delta (order-1 entropy estimate) |
| ObjStm headers (`L_OBJSTM`) | the `num off` table; offsets derived from member boundaries |
| Classic xref tables (`I_XREF`) | entries predicted from object positions; free entries from the free list |
| XRef stream rows (`L_XREFROWS`) | rows predicted from object positions and ObjStm order |
| CMap blocks | uniform `bfchar`/`bfrange` bodies → `.cmb` (binary, delta-coded codes and destinations) |
| Linearization (M9 stage 1) | forward slots: linearization dict `/L /E /T /H`, first-page xref entries, forward `/Prev` `startxref` `/XRefStm`; the decoder patches them once all positions are known. First-page XRef streams are restored after the last item when that predicts better |
| Encrypted strings | strings of top-level objects of encrypted files stored as plaintext (spelling policy, cipher and AES IV kept in the placeholder; `d` re-encrypts) |
| Inline images with filters | `BI … ID <data> EI` whose filters (Flate, LZW, A85, AHx, RL, predictors) can be undone exactly → decoded data and recon in `.csi` |
| TIFF container (`L_TIFW`) | streams matching a TIFF subformat become the single strip of a baseline TIFF (`.tif`): CCITT fax data (compression 4 = G4, 3 = G3 with EOLs, 2 = 1D byte-aligned), image data left Flate/LZW/RunLength-encoded (8, 5, 32773, Predictor 1 or 2), and raw samples BMP can't hold (2 or 16 bpc, 2-bit palette, data longer than the image). Nothing is re-coded; `d` takes the strip back out |
| Derived fields | stream `/Length` (direct and indirect object), `/DL`, `/Length1-3`, ObjStm `/N /First`, image `/Width /Height` and `/DecodeParms /Columns /Rows` (BMP / TIFF header, JPEG SOF), EmbeddedFile `/Params /Size /CheckSum`, `startxref`, trailer `/Prev /Size /XRefStm`, XRef stream `/Prev`: replaced by placeholders the decoder fills in |
| Signature blobs | a hex `/Contents` string holding one DER SEQUENCE and then only zero bytes (the space a signature dictionary reserves) → the DER element to `.p7s` (CMS / PKCS#7, elements concatenated), case and padding length in the placeholder |
| RGB + soft mask (`-a`, `L_RGBA` / `L_ALPHA`) | an RGB image and its 8-bit `/SMask` of the same size as one BGRA BMP on the image; the mask's stream keeps no data and is restored, deferred, from the alpha bytes |
| Small text streams | XMP and other small text streams go to `.txt`, not `.bin` |

Plan items measured and not adopted are listed under "xz results" below, with the exact
measurement behind each.

`-s` selects how much of this is applied (see Options).

## Build

Single translation unit: `src/pdfdump.cpp` includes the `src/*.inc` modules. `src/preflate.inc`
(from ziprec) is included unmodified and is the only code that isn't ours.

    make                      # g++ -std=c++17 -O2 -pthread -o pdfdump src/pdfdump.cpp
    make pdfdump-asan         # ASan + UBSan build
    clang++ -std=c++17 -O2 -pthread -o pdfdump src/pdfdump.cpp
    x86_64-w64-mingw32-g++-posix -std=c++17 -O2 -static -o pdfdump.exe src/pdfdump.cpp
    cl /O2 /std:c++17 /EHsc src\pdfdump.cpp          (MSVC, untested)

Tested with g++ 13.3 and clang 18 on Linux x86-64; the thread pool is also clean under
ThreadSanitizer (`-fsanitize=thread`). MinGW needs the posix thread model (`-posix` driver) for
`std::thread`; the Windows build compiles but has not been run.

## Options

| Option | Meaning | Default |
|---|---|---|
| `-P PW` | password for encrypted input, tried as the user password and then as the owner password (`c` only) | the empty user password is always tried first |
| `-f N` | 0 = keep streams encoded (still decrypted), 1 = undo filters | 1 |
| `-s N` | syntax depth: 0 = stored; 1 = items + raw object text (payload extraction only); 2 = + object-level structure (xref, ObjStm, derived fields, encrypted strings, forward slots, object text lifting); 3 = + content streams and CMaps | 3 |
| `-p N` | small-stream threshold: a stream whose stored form is under N bytes goes to `<P>.bin` instead of its own file | 1024 |
| `-D` | disable dedup of extracted files | dedup on |
| `-d` | do not derive computable fields | derive on |
| `-g` | decode CCITT fax images (Group 4, Group 3 1D) to 1 bpp BMP; exact re-encode with the T.4/T.6 mode rules. Without it, fax data matching a TIFF subformat is stored as `.tif` | off |
| `-a` | RGB images with an 8-bit `/SMask` of the same size: both in one 32 bpp BMP (BGRA) on the image; the mask is restored from its alpha bytes. xz −3% on those images, off by default since BMP recompressors may not take alpha | off |
| `-j N` | undo the filters of N streams at a time (threads); the dump does not depend on N | one per CPU |
| `-c MAP` | channel→file map, e.g. `S,N,T,X,O,F,R,B` to split every channel | `SN,TX,O,F,R,B` |
| `-n` | no verification: skip the final in-memory restore and the per-stream Flate re-encode (about 1.8× faster on Flate-heavy files) | verify on |
| `-v` / `-q` | per-stream report / quiet | |

`d` takes no options: everything it needs is in the dump header.

## Output

| File | Holds |
|---|---|
| `<P>.meta` | header (magic, format version, channel map, which files exist, original size + CRC-32, file key, zlib header default, transform flags), then channels S and N: item tags, stream descriptors, lengths, xref entry kinds |
| `<P>.txt` | channel T with numbers replaced by markers: object text, non-object syntax, ObjStm members, small text streams |
| `<P>.txn` | the numbers lifted from `.txt`, per group |
| `<P>.rec` | channel R: recon records (preflate diffs, filter parameters, ObjStm object numbers), AES IVs |
| `<P>.bin` | channel B: raw regions, binary streams under the `-p` threshold, XRef stream row residuals |
| `<P>.cs` | all decoded content streams concatenated, numbers and hex strings replaced by markers |
| `<P>.csn` | the numbers lifted from `.cs`, per (operator, operand slot) group |
| `<P>.csh` | the hex strings lifted from `.cs`, as binary: varint size of the length section, the string lengths (varints), the string bytes |
| `<P>.csi` | inline image data of `.cs` with its filters undone (decoded data + recon) |
| `<P>.cmap` | all decoded CMaps (`/ToUnicode`, embedded encodings), concatenated, `bfchar`/`bfrange` bodies replaced by markers |
| `<P>.cmb` | those bodies as binary |
| `<P>.p7s` | signature blobs (DER CMS / PKCS#7), concatenated in file order |
| `<P>%08X.<ext>` | one file per extracted stream; ordinal in file order |

Files that would be empty are not written; the header records which ones exist, so `d` never
reads a stale file left by an earlier dump with the same prefix.

Extension precedence: TIFF container (`.tif`) → terminal filter (`.jpg`, `.jp2`/`.j2k`, `.jb2`, `.g4`) → BMP3 wrap →
signature (`.jpg .png .gif .tif .jp2 .j2k .pdf .zip .ttf .otf .pfa .ps .icc .xml`) → attachment
file name → class default (`.ttf .cff .otf .pfa .icc .xml .js .ai .jb2g .img`) → `.z` (still
compressed), `.enc` (still encrypted) or `.bin`. Type 1 fonts with the encryption removed are
`.t1d` (not loadable as fonts any more; `d` re-encrypts them exactly).

Raw images (all filters undone, 1/4/8 bpc gray or indexed, 8 bpc RGB or CMYK, image masks) are
written as BMP3. Restore reads only width, height, bit count and pixel offset from the BMP, so the
palette is a display aid and can't affect the round trip.

Image streams BMP can't take, and CCITT fax data, go into a big-endian baseline TIFF when they
match a TIFF subformat (see the table above) and are big enough for their own file (`-p`). The
TIFF holds the stream bytes unchanged as one strip; restore reads `StripOffsets` and
`StripByteCounts` and nothing else. Fax TIFFs are WhiteIsZero, BlackIsZero when `/BlackIs1` or a
high-to-low `/Decode` inverts the image in the PDF, so they display like the PDF does (checked
against poppler's `pdfimages` on the test files). Fax data that matches no subformat (G4 with
`/EncodedByteAlign`, G3 without EOLs and not byte-aligned) stays `.g4`.

## How it stays lossless

- Items tile the file. Every byte belongs to a header/xref/trailer text item, an object, a stream,
  or a raw region, so any segmentation is lossless and a wrong guess only costs compactness.
- Every layer is verified when it's undone. A filter layer is undone only if re-encoding the
  decoded data through the decoder's own routine gives back the original bytes. Otherwise the
  layer stays encoded and the stream gets `.z`. The same applies to ciphers (AES needs canonical
  padding) and to the BMP and TIFF wraps.
- `c` runs the complete decoder on the in-memory dump and compares the result with the input
  before writing anything. On a mismatch, which would mean a bug, it writes stored mode and says
  so. `d` checks the restored file's size and CRC-32 against the header.
- The decoder makes no guesses. Classification, cipher choice, key recovery and filter parameters
  are encoder-side decisions stored in the stream descriptor. Where the decoder does derive
  something (number groups, ObjStm offsets, xref predictions), it runs the same deterministic code
  on the same bytes as the encoder, and the encoder checks the result before using it.

## Deviations from the plan

- **Unterminated deflate.** Acrobat writes some streams that end at a sync flush
  (`00 00 FF FF`) with no final block, and preflate rejects those. `c` appends an empty final
  block (`03 00`), and the recon record tells `d` to strip it again after re-encoding.
- **Key length.** A missing or invalid `/Length` follows qpdf's rule (V2/3: 128 bits), with the
  spec's 40-bit default as a second candidate. The `/U` check picks the right one.
- **Classification additions:** Illustrator private data (`/AIPDFPrivateData*`,
  `/AIPrivateData*`, `/AIMetaData`) → `.ai`, generic `%!PS` → `.ps`, linearization hint stream →
  class `hint`.
- **Own-file extensions** are always stored in the descriptor (a few bytes per file), not only
  when the class doesn't imply them.
- **TIFF predictor 2** is undone for 8 and 16 bpc only. Other depths keep the predictor layer.
- **No xref read.** The plan's best-effort xref read only supplies hints (indirect `/Length`,
  damage). A pre-scan of every `N G obj` header in the file serves the same purpose and works when
  the xref is broken.
- **Files without `%PDF-`** in the first 1024 bytes go to stored mode, as the plan specifies.
- **Object syntax stays text** (§3.3, §5.3). Instead of a tagged binary token stream, object text
  keeps its spelling and only numbers are lifted (`.txn`); the token-stream parts measured
  against it lost (name table +4%, unescaped strings, gap model; see "xz results"). With no value
  tags, the M10 item "schema-implied tags" reduces to the `/` that starts a name value after a
  key with a fixed value type. Dropping all 108,079 of them on the README test set (keys such as
  `/Type /Subtype /BaseFont /Filter /S /BM /Encoding /ColorSpace`) saves 856 bytes of `.txt` xz
  (−0.22%): xz already predicts them from the key, so they stay.
- **Streams are decoded on a thread pool** (`-j`). Dedup is decided in file order first, and
  results are assembled in stream order, so the dump is byte-identical for any thread count.

## Tests

    tests/roundtrip.sh -b ./pdfdump files...     # c -> d -> cmp, CSV of channel sizes and timings
    tests/enc_variants.sh -b ./pdfdump files...  # M4 check, needs qpdf
    python3 tests/xzbench.py -b ./pdfdump files...  # xz -9 of the dump vs of the input
    tests/get_corpus.sh [dir]                    # fetch the kept files of the 005 set
    python3 tests/mksigned.py out.pdf workdir    # signed test PDF for the .p7s path, needs openssl

`enc_variants.sh` encrypts each input with qpdf at R2, R3, R4 and R6, using an empty user
password. It then checks that the extracted payload files are byte-identical to those from a
plain qpdf rewrite of the same input, and that every variant round-trips.

### xz results

`xzbench.py` tars all dump files of one input in name order and compresses the tar with xz -9.
It compares that with xz -9 of the input PDF. The "PDF-specific" figure is the same, leaving out
the standard payload formats (`ttf otf cff jpg jp2 j2k jb2 jb2g g4 bmp png gif tif icc pdf zip
img`), which go to external recompressors. Test set: the 117 files of the 005 set and the 3
supplied PDFs (55.6 MB, orig.xz 47.5 MB).

| Step | dump.xz / orig.xz | PDF-specific xz |
|---|---|---|
| M0–M4 extractor | 69.0% | 13.46 MB |
| + Type 1 decryption, number lifting | 60.2% | 9.26 MB |
| + ObjStm headers, small text → `.txt` | 60.0% | 9.16 MB |
| + classic xref tables | 59.9% | 9.11 MB |
| + adaptive per-group number coding | 59.4% | 8.90 MB |
| + XRef stream rows | 59.3% | 8.83 MB |
| + Type 1 charstrings split into operator / operand pools | 59.2% | 8.78 MB |
| + CMap `bfchar`/`bfrange` blocks as binary | 59.2% | 8.77 MB |
| + derived fields (`/Length`, `startxref`, `/Prev`, ...) | 59.1% | 8.75 MB |
| + encrypted strings, inline image filters, linearization forward slots | 59.1% | 8.75 MB |
| + TIFF container, `.csh` lengths apart from bytes, signature blobs | 59.0% | 8.71 MB |

Integer predictions of M10, measured by switching the delta mode of the per-group number coding
off (README test set): content-stream operands against the previous use of the same operator and
slot, `.csn` 3297 KB → 3117 KB (−180 KB); object-text numbers, array elements included, against
the previous element of their group, `.txn` 395 KB → 388 KB (−7 KB). Both are in (adaptive per
group). RGB + soft mask as one 32 bpp BMP (M10), 400 pairs in the 808-file set: per-file xz of the
BMPs 8.41 MB → 8.15 MB (−3.0%, smaller for 347 pairs); built as `-a`, off by default because the
BMPs go to an image recompressor that may not handle alpha.

Measured and not adopted, because they don't beat xz on this corpus (upper bounds where noted):
- PNG filters on raw images: raw rows compress best on aggregate.
- Lifting numbers without grouping, or with plain deltas: both lose to text.
- Splitting content streams per operator as text.
- Unescaping literal strings (§3.3).
- Keeping Flate layers with large recon encoded: made the total worse.
- Moving unfiltered inline image data out of the content streams: −0.2%.
- Name table (§5.3) on the lifted object text: +4% (worse).
- Gap model (§3.4): at most −1.5% of `.txt` even with all whitespace canonical.
- UTF-16BE strings as UTF-8 (§3.3): −0.7 KB.
- Graph predictors `/Parent` `/Count` (M10): at most −1 KB.
- Dates as binary, `/ID` pairs (M10): at most −2 KB and −6 KB (`/ID` is random).
- CCITT G4 / G3 to 1 bpp BMP (M10): implemented, but opt-in (`-g`). xz of the bitmaps is 24%
  larger than the fax data; on the 8 test files with CCITT images, dump.xz goes from 60.8% to
  62.1% of orig.xz. It may pay with a dedicated bilevel recompressor.
- Hint table regeneration (M9 stage 2): exact ceiling measured by dropping the hint streams'
  data altogether. On the 78 linearized files of the 808-file set, dump.xz goes from 43.58 MB to
  43.55 MB (−34.9 KB, −0.08%), and that assumes every table regenerates. Only tables written by
  qpdf's algorithm would, and few of these files come from qpdf.
- ToUnicode remap (M10): exact prototype (qpdf JSON for the object graph; page and form
  resources resolved to fonts; ToUnicode `bfchar`/`bfrange` parsed; a content-stream hex string
  remapped to UTF-8 when its font's map is injective on the codes used; font id per `Tf` and a
  flag per string counted as side information). Its unmapped baseline reproduces the real `.csh`
  within 0.2%. On the 8 files with the most hex text: 260.2 KB → 269.2 KB (+3.5%, worse on 6 of
  8; Tarasov +4.6 KB, Way of Men +2.2 KB, two files −0.4 KB each).
- Signature blobs to `.p7s` (M10): adopted, though the test sets have no signed files. On a
  synthetic file with two RSA-2048 signatures (8 KB reserved each), the dump goes from 3084 to
  2952 bytes of xz: certificates and signatures are mostly random bytes, so the gain is the hex
  overhead only. The blobs are standard CMS files for any downstream tool.

Speed of `c`, 120 files run one after another on a 4-CPU machine (the dump is byte-identical
for every `-j`):

| Mode | Aggregate | Slowest files |
|---|---|---|
| default, 4 threads | 3.28 MB/s | every file at or above 513 KB/s; files over 50 KB at 810 KB/s or more |
| `-n` (no verification), 4 threads | 4.49 MB/s | every file at or above 678 KB/s |
| `-j 1` (single thread) | 1.74 MB/s | 4 files over 50 KB at 368–459 KB/s |
| `-j 1 -n` | 2.65 MB/s | files over 50 KB at 712 KB/s or more |

Almost all the time is preflate: the analysis plus the re-encode that verifies each Flate layer.
The slow files have level-9 text streams or highly compressible images, where preflate costs
the most per input byte. Byte-identical streams are decoded once. Streams are independent, so
the filter work runs on a thread pool; the slowest files are those dominated by one large
stream.

### Round trip and robustness

The corpus is the three supplied PDFs, qpdf's `qpdf/qtest/qpdf/*.pdf` (628 files: damaged,
linearized, object streams, encrypted, hybrid), 60 mutated files (truncated, bit-flipped, junk
inserted, duplicated ranges), and the 117 files of the 005 set.

| Check | Result |
|---|---|
| Byte-exact round trip (`c` → disk → `d` → `cmp`) | 808/808 |
| Same, under ASan + UBSan (`halt_on_error=1`) | 808/808. One UBSan report, a zero-length `memcpy` from null inside preflate's `MemStream`, avoided by calling the `InputStream` overload |
| Encrypted variants, R2/R3/R4/R6 × (3 supplied + 15 qpdf files) | 72/72 extract files identical to the plain rewrite |
| Decompression bomb (1.1 GiB of zeros in a 1.1 MB Flate stream) | layer kept as `.z` at the 1 GiB cap, round trip intact |
| qpdf files in stored mode | 14, all without a `%PDF-` header |

## Source layout

    src/pdfdump.cpp     CLI, encoder pipeline, info report, self-test
    src/chan.inc        channel writer/reader, channel->file map, header, varints, CRC-32
    src/pdf_lex.inc     lexer and read-only object parser
    src/pdf_seg.inc     segmenter: items, stream extents, raw regions, xref sections, trailers
    src/pdf_filt.inc    Flate (preflate wrapper), LZW, AHx, A85, RL, PNG/TIFF predictors
    src/pdf_bmp.inc     BMP3 wrap/unwrap
    src/pdf_tiff.inc    TIFF container for streams matching a TIFF subformat
    src/pdf_ccitt.inc   CCITT G3/G4 decoder and re-encoder (opt-in layer)
    src/pdf_crypt.inc   MD5, SHA-256/384/512, RC4, AES, standard security handler
    src/pdf_class.inc   ObjStm expansion, classification, signatures, image description
    src/pdf_xref.inc    classic xref sections (I_XREF), object position tracking
    src/pdf_t1.inc      Type 1 eexec / charstring decryption layer
    src/pdf_objstm.inc  ObjStm header derivation, XRef stream row derivation
    src/pdf_lift.inc    number lifting (content streams, object text), per-group coding
    src/pdf_cmap.inc    CMap bfchar/bfrange blocks as binary
    src/pdf_derive.inc  derived fields: placeholders, stream facts, xref section state
    src/pdf_emit.inc    item records, stream descriptors, recon dictionary, decoder
    src/preflate.inc    from ziprec, unmodified
