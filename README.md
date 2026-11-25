# pdfdump

Lossless PDF dump/restore. `c` splits a PDF into typed payload files plus a few channel
files; `d` rebuilds the original byte-for-byte. Design and roadmap: [docs/pdfdump_plan.md](docs/pdfdump_plan.md).

    pdfdump c [options] input.pdf outprefix      dump
    pdfdump d inprefix restored.pdf              restore, byte-exact
    pdfdump [options] input.pdf                  info only: items, streams, coverage
    pdfdump selftest                             crypto test vectors, filter round trips

## Status

Milestones **M0–M4** of the plan are implemented. Together they are the "complete lossless
extractor" checkpoint: every stream payload is decoded as far as it can be re-encoded exactly and
written as a typed file, encrypted files included. Object syntax is still stored as raw text.
Syntax compaction (M5–M9: token coder, xref/ObjStm regeneration, content-stream parsing,
linearization) is not built yet; `-s` above 1 is clamped to 1.

| M | Scope | State |
|---|---|---|
| M0 | CLI, channels, header, stored mode, final in-memory verify, corpus harness | done |
| M1 | Sequential segmenter, stream extents (`/Length`, indirect `/Length`, `endstream`/`endobj` scan), raw regions, read-only lexer | done |
| M2 | Filter engine: Flate via `preflate.inc`, LZW, ASCIIHex, ASCII85, RunLength, PNG and TIFF predictors, recon dictionary | done |
| M3 | ObjStm expansion (read-only), classification, signature-based extensions, typed files, BMP3, small-stream file, dedup | done |
| M4 | Standard security handler R2–R6 (RC4-40/128, AES-128, AES-256), key in header, IVs in channel R | done |
| M5+ | syntax compaction | not started |

## Build

Single translation unit: `src/pdfdump.cpp` includes the `src/*.inc` modules. `src/preflate.inc`
(from ziprec) is included unmodified and is the only code that isn't ours.

    make                      # g++ -std=c++17 -O2 -o pdfdump src/pdfdump.cpp
    make pdfdump-asan         # ASan + UBSan build
    clang++ -std=c++17 -O2 -o pdfdump src/pdfdump.cpp
    cl /O2 /std:c++17 /EHsc src\pdfdump.cpp          (MSVC, untested)

Tested with g++ 13.3 and clang 18 on Linux x86-64.

## Options

| Option | Meaning | Default |
|---|---|---|
| `-P PW` | password for encrypted input, tried as the user password and then as the owner password (`c` only) | the empty user password is always tried first |
| `-f N` | 0 = keep streams encoded (still decrypted), 1 = undo filters | 1 |
| `-s N` | syntax depth: 0 = stored, 1 = items + raw object text | 1 |
| `-p N` | small-stream threshold: a stream whose stored form is under N bytes goes to `<P>.bin` instead of its own file | 1024 |
| `-D` | disable dedup of extracted files | dedup on |
| `-c MAP` | channel→file map, e.g. `S,N,T,X,O,F,R,B` to split every channel | `SN,TX,O,F,R,B` |
| `-n` | skip the final in-memory verify | verify on |
| `-v` / `-q` | per-stream report / quiet | |

`d` takes no options: everything it needs is in the dump header.

## Output

| File | Holds |
|---|---|
| `<P>.meta` | header (magic, format version, channel map, original size + CRC-32, file key, zlib header default), then channels S and N: item tags, stream descriptors, lengths |
| `<P>.txt` | channel T: raw object text, non-object syntax (header, xref, trailer), decoded ObjStm text |
| `<P>.rec` | channel R: recon blobs (preflate diffs and other filter parameters), AES IVs. Incompressible. |
| `<P>.bin` | channel B: raw regions, streams under the `-p` threshold, decoded XRef stream rows |
| `<P>.cs` | all decoded content streams (pages, forms, patterns, Type 3 glyphs, appearances), concatenated |
| `<P>.cmap` | all decoded CMaps (`/ToUnicode`, embedded encodings), concatenated |
| `<P>%08X.<ext>` | one file per extracted stream; ordinal in file order |

Extension precedence: terminal filter (`.jpg`, `.jp2`/`.j2k`, `.jb2`, `.g4`) → BMP3 wrap →
signature (`.jpg .png .gif .tif .jp2 .j2k .pdf .zip .ttf .otf .pfa .ps .icc .xml`) → attachment
file name → class default (`.ttf .cff .otf .pfa .icc .xml .js .ai .jb2g .img`) → `.z` (still
compressed), `.enc` (still encrypted) or `.bin`.

Raw images (all filters undone, 1/4/8 bpc gray or indexed, 8 bpc RGB or CMYK, image masks) are
written as BMP3. Restore reads only width, height, bit count and pixel offset from the BMP, so the
palette is a display aid and can't affect the round trip.

## How it stays lossless

- Items tile the file. Every byte belongs to a header/xref/trailer text item, an object, a stream,
  or a raw region, so any segmentation is lossless and a wrong guess only costs compactness.
- Every layer is verified when it's undone. A filter layer is undone only if re-encoding the
  decoded data through the decoder's own routine gives back the original bytes. Otherwise the
  layer stays encoded and the stream gets `.z`. The same applies to ciphers (AES needs canonical
  padding) and to the BMP wrap.
- `c` runs the complete decoder on the in-memory dump and compares the result with the input
  before writing anything. On a mismatch, which would mean a bug, it writes stored mode and says
  so. `d` checks the restored file's size and CRC-32 against the header.
- The decoder makes no guesses. Classification, cipher choice, key recovery and filter parameters
  are encoder-side decisions stored in the stream descriptor.

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

## Tests

    tests/roundtrip.sh -b ./pdfdump files...     # c -> d -> cmp, CSV of channel sizes and timings
    tests/enc_variants.sh -b ./pdfdump files...  # M4 check, needs qpdf

`enc_variants.sh` encrypts each input with qpdf at R2, R3, R4 and R6, using an empty user
password. It then checks that the extracted payload files are byte-identical to those from a
plain qpdf rewrite of the same input, and that every variant round-trips.

### Results

The corpus is the three supplied PDFs, qpdf's `qpdf/qtest/qpdf/*.pdf` (628 files: damaged,
linearized, object streams, encrypted, hybrid), and 60 mutated files (truncated, bit-flipped,
junk inserted, duplicated ranges).

| Check | Result |
|---|---|
| Byte-exact round trip (`c` → disk → `d` → `cmp`) | 3/3 supplied, 628/628 qpdf, 60/60 mutated |
| Same, under ASan + UBSan | see the commit log for the latest run |
| Encrypted variants, R2/R3/R4/R6 × 3 supplied files | 12/12 extract files identical to the plain rewrite |
| Extracted files open in native libraries (PIL, fontTools, XML parser) | 75 BMP, 18 JPEG, 21 TrueType, 2 CFF, 9 XML, 24 Type 1 |
| qpdf files in stored mode | 14, all without a `%PDF-` header |

Supplied files, default options:

| File | Size | Streams | Flate undone | Typed share | Recon share | c / d time |
|---|---|---|---|---|---|---|
| Wachter (eWorks, PDF 1.5, linearized, ObjStm) | 6.4 MB | 151 | 150/150 | 99.5% | 0.04% | 3.7 s / 1.2 s |
| NVIDIA Ampere (Adobe PDF Library 20, linearized) | 8.0 MB | 223 | 199/199, 18 DCT | 99.4% | 0.19% | 4.1 s / 1.1 s |
| Intel (Distiller 9.4, RC4-128 R3, 915 ObjStm) | 2.3 MB | 1540 | 1537/1537, all 1538 decrypted | 73.9%¹ | 0.91% | 1.5 s / 0.5 s |

¹ The rest is decoded ObjStm text, which the plan keeps in `<P>.txt` until M7.

In the qpdf corpus, the Flate layers left encoded are in files that need a password (470 streams),
deliberately corrupt data (issue-117, bad-data, broken-lzw) and the `png-filters-*` files, whose
`/Columns` doesn't match the data. In those files the Flate layer is undone and the predictor layer
is kept.

## Source layout

    src/pdfdump.cpp     CLI, encoder pipeline, info report, self-test
    src/chan.inc        channel writer/reader, channel->file map, header, varints, CRC-32
    src/pdf_lex.inc     lexer and read-only object parser
    src/pdf_seg.inc     segmenter: items, stream extents, raw regions, trailers
    src/pdf_filt.inc    Flate (preflate wrapper), LZW, AHx, A85, RL, PNG/TIFF predictors
    src/pdf_bmp.inc     BMP3 wrap/unwrap
    src/pdf_crypt.inc   MD5, SHA-256/384/512, RC4, AES, standard security handler
    src/pdf_class.inc   ObjStm expansion, classification, signatures, image description
    src/pdf_emit.inc    item records, stream descriptors, recon dictionary, decoder
    src/preflate.inc    from ziprec, unmodified
