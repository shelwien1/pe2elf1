# pdfdump

Lossless PDF dump/restore. `c` splits a PDF into typed payload files plus a few channel
files; `d` rebuilds the original byte-for-byte. Design and roadmap: [docs/pdfdump_plan.md](docs/pdfdump_plan.md); measured next steps: [docs/pdfdump_plan2.md](docs/pdfdump_plan2.md).

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
| Type 1 decryption (`L_T1`) | eexec and charstring encryption of embedded Type 1 fonts → `.t1d`; charstrings split into lead bytes, operators and operands, operands ordered by (operator, operand count) |
| Number lifting, content streams | numbers → `.csn`, grouped by (operator, operand slot); canonical hex strings → `.csh` (binary: all string lengths, then all string bytes; −10% xz of `.csh` against interleaving them); literal strings → `.css` (as written, escapes kept) |
| Content-stream predictions | path points (`m l c v y re h`) against the current point; `Td`/`TD` against the last advance written after the same text in the same font (hits and misses in separate groups); stored as residuals |
| Number lifting, object text | numbers of `.txt` → `.txn`, grouped by (key, in-array, terminator) |
| Adaptive number coding | per group: varint or fixed width, raw or delta; plain spelling codes, or integers in one decimal unit with the spelling recomputed (fixed decimals, trimmed zeros, no leading zero), or residuals of the predictions above in units of 10^-k; chosen by order-1 entropy estimate |
| ObjStm headers (`L_OBJSTM`) | the `num off` table; offsets derived from member boundaries |
| Classic xref tables (`I_XREF`) | entries predicted from object positions; free entries from the free list |
| XRef stream rows (`L_XREFROWS`) | rows predicted from object positions and ObjStm order |
| CMap blocks | uniform `bfchar`/`bfrange` bodies → `.cmb` (binary, delta-coded codes and destinations) |
| Linearization (M9 stage 1) | forward slots: linearization dict `/L /E /T /H`, first-page xref entries, forward `/Prev` `startxref` `/XRefStm`; the decoder patches them once all positions are known. First-page XRef streams are restored after the last item when that predicts better |
| Encrypted strings | strings of top-level objects of encrypted files stored as plaintext (spelling policy, cipher and AES IV kept in the placeholder; `d` re-encrypts) |
| Inline images with filters | `BI … ID <data> EI` whose filters (Flate, LZW, A85, AHx, RL, predictors) can be undone exactly → decoded data and recon in `.csi` |
| TIFF container (`L_TIFW`) | streams matching a TIFF subformat become the single strip of a baseline TIFF (`.tif`): CCITT fax data (compression 4 = G4, 3 = G3 with EOLs, 2 = 1D byte-aligned), image data left Flate/LZW/RunLength-encoded (8, 5, 32773, Predictor 1 or 2), and raw samples BMP can't hold (2 or 16 bpc, 2-bit palette, data longer than the image). Nothing is re-coded; `d` takes the strip back out |
| Derived fields | stream `/Length` (direct and indirect object), `/DL`, `/Length1-3`, ObjStm `/N /First`, image `/Width /Height` and `/DecodeParms /Columns /Rows` (BMP / TIFF header, JPEG SOF), EmbeddedFile `/Params /Size /CheckSum`, `startxref`, trailer `/Prev /Size /XRefStm`, XRef stream `/Prev`: replaced by placeholders the decoder fills in |
| Widths from the font programs | `/Widths` of simple TrueType, Type1C and Type 1 fonts (`hsbw`/`sbw` of the charstrings), `/W` of CIDFontType2 (identity `CIDToGIDMap` or the map stream) and CIDFontType0 fonts: every width the embedded font program predicts (glyph advance · 1000 / unitsPerEm, map and rounding chosen per array) becomes a placeholder; the decoder reads the font's own file, or a stream stored inline in an earlier item. Arrays in the font dictionary or objects of their own; only arrays where 20% of the widths match (70% for Type 1). 62,757 widths in 545 arrays of the 120 test files |
| Structure parents | `/P` of a structure element, when an earlier object's `/K` lists the element: 16,198 in the 120 test files. `/ParentTree` leaves and their per-page arrays: references predicted from page `/StructParents` and the MCIDs and OBJRs of the elements seen so far: 19,930 |
| Derived fields in ObjStm members | the placeholders above also inside object streams (member text stored with placeholders, a stream flag says so); 78% of the structure elements and most font dictionaries of modern files are members |
| Signature blobs | a hex `/Contents` string holding one DER SEQUENCE and then only zero bytes (the space a signature dictionary reserves) → the DER element to `.p7s` (CMS / PKCS#7, elements concatenated), case and padding length in the placeholder |
| RGB + soft mask (`-a`, `L_RGBA` / `L_ALPHA`) | an RGB image and its 8-bit `/SMask` of the same size as one BGRA BMP on the image; the mask's stream keeps no data and is restored, deferred, from the alpha bytes |
| Base64 blobs in object text | a canonical base64 run of 1024+ characters (JavaScript data, `data:` URIs) that decodes to a zlib stream, or sits in quotes → decoded bytes (zlib streams inflated through preflate, recon in `.b64r`) to a file of their own from 64 KB up (`<P>b%07X.<ext>`, extension by signature: `elf`, `jpg`, ...), smaller ones to `.b64`. linux.pdf (Linux in a PDF: a RISC-V kernel and ELF files as base64 zlib in the `/JS` string): dump.xz 3.89 → 3.21 MB (−17.5%) |
| XMP thumbnails (`L_XMPJPG`) | the base64 JPEG of an XMP packet's `<xmpGImg:image>` (76-character lines joined by `&#xA;`) becomes the stream's stored form, a `.jpg`; the recon keeps the XMP text around it and the wrapping. 21 thumbnails in 7 files of the 808-file set; dump.xz −0.5 to −3.6 KB per file, before any JPEG recompressor |
| Small text streams | XMP and other small text streams go to `.txt`, not `.bin` |
| Well-known ICC profiles (`L_ICCREF`) | a stream whose decoded bytes are a profile of the built-in table (sRGB IEC61966-2.1, HP 1998: 27 of the 120 test files) keeps a one-byte index |
| Item lengths | the length of object items and stream dictionaries / suffixes is found by the decoder (first `endobj` + whitespace, first `stream` + end of line); `.iln` holds 0 or the difference |

Plan items measured and not adopted are listed under "xz results" below, with the exact
measurement behind each.

`-s` selects how much of this is applied (see Options).

## Build

Single translation unit: `src/pdfdump.cpp` includes the `src/*.inc` modules. `src/preflate.inc`
(from ziprec) is the only code that isn't ours. It carries two speed-only edits, each marked
`pdfdump:` (an 8-bytes-at-a-time match compare, and the hash-chain walk rejecting a candidate on
bytes `bestLen-1..bestLen` before calling the compare); match lengths, and so every dump, are
unchanged (checked byte for byte on the test set). It also carries guards, marked the same way,
that turn corrupt correction data into an error instead of an out-of-bounds read (see "Round
trip and robustness").

    make                      # g++ -std=c++17 -O3 -fno-exceptions -fno-rtti -pthread -o pdfdump src/pdfdump.cpp
    make pdfdump-asan         # ASan + UBSan build, same flags
    clang++ -std=c++17 -O3 -fno-exceptions -fno-rtti -pthread -o pdfdump src/pdfdump.cpp
    x86_64-w64-mingw32-g++-posix -std=c++17 -O3 -fno-exceptions -fno-rtti -static -o pdfdump.exe src/pdfdump.cpp
    cl /O2 /std:c++17 /EHs-c- /GR- /D_HAS_EXCEPTIONS=0 src\pdfdump.cpp   (MSVC, untested)

The code uses no exceptions and no RTTI (it also builds with them). Errors are bool returns:
`fail()` records the first error of the calling thread and returns false, the dump readers
(`ChanReader`, `RBuf`) are sticky (zeros after an error, `ok()` false), and preflate's guards
set a thread-local stop flag that its decode loops honour. In `c`, an error in any phase, a
worker thread's included, falls back to stored mode as before; attempts that may fail (a layer
that stays encoded, a lift that is skipped) run through `softly()`, which keeps a handled
failure from leaking into the caller's error state. In `d` and on the command line, an error
ends the program with its message (`die()`). An allocation that cannot be met ends the program
too, in `c` as well (`std::set_new_handler`: a message instead of an abort); without exceptions
`c` no longer falls back to stored mode on it.

Tested with g++ 13.3 and clang 18 on Linux x86-64; the thread pool is also clean under
ThreadSanitizer (`-fsanitize=thread`). MinGW needs the posix thread model (`-posix` driver) for
`std::thread`; the Windows build compiles but has not been run.

### Fixed-capacity storage

Containers whose size has a bound (at most 100 MB at the bound, per array) are arrays of that
size, not STL containers: `FixedVec<T,N>` (in place, `push_back`/`resize` return false when
full), `FixedStr<N>` (in place, NUL-terminated: `ShortName` 15 bytes, `ValStr` 39, `PdfName`
127, `Password` 127, `Message` 1279, `Path` 4095), `KeyBuf` (64 bytes of key material) and
`NameTable` (report counters, 64 keys) in `src/chan.inc`. Blocks over about 256 KB go on the heap
at their full size (`unique_ptr`), so that worker threads (1 MB default stack on Windows) stay
safe.

| What | Bound | Past the bound |
|---|---|---|
| filter layers per stream | 16 (up to 13 filters, fewer with predictors: room is kept for pseudo layers) | the stream stays encoded from that filter on; `d` rejects a descriptor with more |
| `/Filter` array | 32 entries | entries past 31 read as one unknown filter `?`: undoing stops there |
| PDF names (filters, crypt filters, `/StmF` `/StrF` `/EFF`) | 127 bytes (ISO 32000-1 Annex C) | a filter name keeps its first 127 bytes (still unknown); a crypt filter name is not kept (reads as Identity) |
| crypt filters (`/CF`) | 32 | further entries ignored |
| key material (`/O /U /OE /UE`, file and object keys, hashes) | 64 bytes | first 64 bytes kept (handlers read at most 48) |
| `-P` password | 127 bytes | first 127 kept (R6 reads 127, R2-R4 read 32) |
| file extensions | 15 characters (written: at most 8) | `d` rejects a longer one |
| image palettes | 256 entries | (bpc <= 8 by construction) |
| font tables (`FontMetrics`, about 1 MB, 8 cached at a time) | 65535 glyphs (a u16 in TrueType / CFF); cmap subtables 64 KB (formats 0/4/6: a u16 length); 65536 CIDs | Type 1 glyphs past 65535 are not used for widths; CIDs past 65535 predict nothing |
| CFF FDs / operand stack | 256 (a u8 FD index) / 48 (the CFF limit) | operands past 48 dropped |
| Type 1 RD token names | 255 (a u8 in the recon) | charstrings named by further tokens stay as they are |
| CCITT row | 65540 changes (65536 columns) | layer not applied |
| LZW tables, zlib header counts, lifter entropy counters | 4096 codes; 65536 headers; 65536 pairs | (exact) |
| decode threads (`-j`, default one per CPU) | 256 | capped |

None of the bounds is reached on the test set (dumps are byte-identical to the build before
the change); a PDF beyond one still round-trips, with less undone or derived (a width
prediction is only used where it reproduces the file).

Still dynamic, because their size follows the input: byte buffers (`Bytes`: the file, streams,
channels, recon data), parsed objects and their strings, per-item / per-object / per-stream
tables (segmenter items, xref entries, the object index, stream records, own files), the
lifter's values, groups, events and their hash tables, derived-field tokens and key paths,
CMap entries, Type 1 / CFF charstring lists, ObjStm and xref stream tables, font cache keys
(they carry the Type 1 recon in hex), and preflate's internals (third-party).

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
| `<P>.meta` | header (magic, format version, channel map, which files exist, original size + CRC-32, file key, zlib header default, transform flags, lengths of the aux files kept here), then channels S and N: item tags, stream descriptors, lengths, xref entry kinds; then every aux file below of 64 KB or less (a file of its own costs a tar header: −11.5 KB of xz on the 120-file set) |
| `<P>.txt` | channel T with numbers replaced by markers: object text, non-object syntax, ObjStm members, small text streams |
| `<P>.txn` | the numbers lifted from `.txt`, per group |
| `<P>.rec` | channel R: recon records (preflate diffs, filter parameters, ObjStm object numbers), AES IVs |
| `<P>.bin` | channel B: raw regions, binary streams under the `-p` threshold, XRef stream row residuals |
| `<P>.cs` | all decoded content streams concatenated, numbers and hex strings replaced by markers |
| `<P>.csn` | the numbers lifted from `.cs`, per (operator, operand slot) group |
| `<P>.csh` | the hex strings lifted from `.cs`, as binary: varint size of the length section, the string lengths (varints), the string bytes |
| `<P>.css` | the literal strings lifted from `.cs`, same layout, bytes as written between the parentheses |
| `<P>.csi` | inline image data of `.cs` with its filters undone (decoded data + recon) |
| `<P>.cmap` | all decoded CMaps (`/ToUnicode`, embedded encodings), concatenated, `bfchar`/`bfrange` bodies replaced by markers |
| `<P>.cmb` | those bodies as binary |
| `<P>.p7s` | signature blobs (DER CMS / PKCS#7), concatenated in file order |
| `<P>.b64` | base64 blobs of object text under 64 KB, decoded (and inflated where they were zlib streams), concatenated |
| `<P>.b64r` | per blob: data length, own file or not (+ extension), Flate recon |
| `<P>.iln` | per object item, stream dictionary and stream suffix: var 0 if the length is the predicted one, else zigzag(length − prediction) |
| `<P>b%07X.<ext>` | base64 blobs of 64 KB and more, one file each, numbered in file order |
| `<P>%08X.<ext>` | one file per extracted stream; ordinal in file order |

Files that would be empty are not written; the header records which ones exist, so `d` never
reads a stale file left by an earlier dump with the same prefix. Aux files (`.txn` to `.iln`) of
64 KB or less are not written either: they are at the end of `.meta`.

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
| + XMP thumbnails → `.jpg` (counted as payload from here on; dump.xz −13.6 KB) | 59.0% | 8.46 MB |
| + plan2 P1–P4: decimal units, path and `Td` residuals, literal strings → `.css` | 57.7% | 7.82 MB |
| + plan2 P5: ICC table | 57.5% | 7.82 MB |
| + plan2 P7: item lengths; small aux files in `.meta` | 57.5% | 7.80 MB |
| + plan2 P8: widths from font programs, structure parents, derived fields in ObjStm members | 57.4% | 7.75 MB |
| + plan2 P6: Type 1 operands by (operator, operand count) | 57.35% | 7.74 MB |
| + plan2 P8: Type 1 / CIDFontType0 / map-stream widths, width arrays as objects, `/ParentTree` | 57.33% | 7.73 MB |

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
- `b64prep` (an external base64 extractor: whitespace-wrapped and URL-safe base64, entropy test,
  4 alignments) as the base64 stage: on linux.pdf it stalls (a 2.5 MB run is longer than its
  512 KB window, so the region is rescanned for every byte; 467 KB of output after 10 minutes),
  and it does not inflate zlib under the base64, which is most of linux.pdf's gain (base64 →
  bytes alone: 3.69 → 3.62 MB of xz; inflated: 3.07 MB). On the 21 XMP streams with thumbnails it
  only cuts out line fragments (`&#xA;` is not whitespace to it): xz +1.2 KB. No other base64 is
  in the 808-file set. It may suit attachments (`.eml`) as a separate stage after `pdfdump`.
- Type 1 charstring operands grouped with the slot in the key (plan2 P6 as prototyped): `.t1d`
  +1.02%; by (operator, operand count), adopted, −1.09%.
- Widths of Type 1 (`/FontFile`) fonts from their charstrings wherever 3 widths match (plan2 P8):
  +1.9 KB against leaving those arrays; TeX writes widths for the whole code range from TFM
  metrics, with a decimal. Adopted for arrays where 70% match (neutral here).
- Signature blobs to `.p7s` (M10): adopted, though the test sets have no signed files. On a
  synthetic file with two RSA-2048 signatures (8 KB reserved each), the dump goes from 3084 to
  2952 bytes of xz: certificates and signatures are mostly random bytes, so the gain is the hex
  overhead only. The blobs are standard CMS files for any downstream tool.

Speed of `c`, 120 files run one after another on a 4-CPU machine, each file's time the fastest
of 3 runs (`xzbench.py -r 3`; single runs of the small files vary by up to 2x on this machine).
The dump is byte-identical for every `-j` and with or without `-n`.

| Mode | Aggregate | Slowest files | Build before plan2 |
|---|---|---|---|
| default, 4 threads | 4.46 MB/s | every file at 671 KB/s or more | 4.72 MB/s, 673 KB/s |
| `-n` (no verification), 4 threads | 5.92 MB/s | every file at 871 KB/s or more | 6.46 MB/s, 783 KB/s |
| `-j 1` (single thread) | 2.36 MB/s | every file at 659 KB/s or more | 2.51 MB/s, 679 KB/s |
| `-j 1 -n` | 3.23 MB/s | every file at 874 KB/s or more | 3.52 MB/s, 776 KB/s |

The `-j 1` row is the final build (format 9); the other rows were measured one commit earlier.
Plan2's number lifting is most of the difference: on intel.pdf it takes 0.19 s of 1.05 s. With
verification on, the lifts skip their own inverse check (the final verify restores everything
anyway); if that verify fails, the encoder redoes the lifts with their checks before it would
fall back to stored mode. With `-n` the checks stay, hence the larger cost there (8%).

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
| `d` on corrupted dumps (one dump file bit-flipped, truncated, bytes inserted / deleted / overwritten; 13 inputs, about 3000 cases, ASan + UBSan; format 7: 6 inputs, 4200 more cases; built with `-fno-exceptions -fno-rtti`: 15 inputs, 4200 more cases) | every case ends in an error message or a CRC mismatch; no sanitizer report after the fixes below |
| `c` then `d` on mutated PDFs, no-exceptions build | 450 cases on 3 inputs: every one round-trips byte-exact |

`tests/roundtrip.sh` also fails a file whose dump was written in stored mode because the
encoder's own verify failed, and the sanitizer build treats UBSan reports as fatal.

Fuzzing `d` with corrupted dump files found, and fixed: a stack overrun when respelling a lifted
number with an impossible digit count; in preflate's re-encoder, reads past the input for tokens
and stored blocks longer than the remaining data, a division by zero and an out-of-range table
index in the correction decoder, a corrected match length of 0, blocks without end, and
literal/length or distance tree sizes beyond the code tables (a stack overflow while predicting
the distance tree). Without exceptions, preflate's guards set a stop flag instead of throwing;
the re-encoder now checks it before the tree predictor walks a block whose last token reached
past the input (found by the same fuzzing). The
preflate guards only fire on corrupt data. GCC 13's UBSan build miscompiles
`(cond ? L : U)[i]`, which made that build render hex strings wrongly; those expressions use a
local pointer now.

## Source layout

    src/pdfdump.cpp     CLI, encoder pipeline, info report, self-test
    src/chan.inc        channel writer/reader, channel->file map, header, varints, CRC-32;
                        error state, fixed-capacity types (FixedVec, FixedStr, KeyBuf, NameTable)
    src/pdf_lex.inc     lexer and read-only object parser
    src/pdf_seg.inc     segmenter: items, stream extents, raw regions, xref sections, trailers;
                        item length predictor
    src/pdf_filt.inc    Flate (preflate wrapper), LZW, AHx, A85, RL, PNG/TIFF predictors
    src/pdf_bmp.inc     BMP3 wrap/unwrap
    src/pdf_tiff.inc    TIFF container for streams matching a TIFF subformat
    src/pdf_icc.inc     built-in ICC profile table (L_ICCREF)
    src/pdf_font.inc    TrueType / CFF glyph advances and the width prediction (derived /Widths, /W)
    src/pdf_xmp.inc     XMP thumbnail (base64 JPEG) as the stream's stored form
    src/pdf_ccitt.inc   CCITT G3/G4 decoder and re-encoder (opt-in layer)
    src/pdf_crypt.inc   MD5, SHA-256/384/512, RC4, AES, standard security handler
    src/pdf_class.inc   ObjStm expansion, classification, signatures, image description
    src/pdf_xref.inc    classic xref sections (I_XREF), object position tracking
    src/pdf_t1.inc      Type 1 eexec / charstring decryption layer
    src/pdf_objstm.inc  ObjStm header derivation, XRef stream row derivation
    src/pdf_lift.inc    number lifting (content streams, object text), per-group coding,
                        path / Td predictions, literal strings
    src/pdf_cmap.inc    CMap bfchar/bfrange blocks as binary
    src/pdf_derive.inc  derived fields: placeholders, stream facts, xref section state, widths,
                        structure parents, ObjStm member offsets
    src/pdf_emit.inc    item records, stream descriptors, recon dictionary, decoder
    src/preflate.inc    from ziprec (speed edits and corrupt-data guards marked "pdfdump:")
