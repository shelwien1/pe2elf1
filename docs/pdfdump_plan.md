# pdfdump — development plan (rev 3)

Lossless PDF dump/restore.

    pdfdump c [options] input.pdf outprefix      dump
    pdfdump d [options] inprefix restored.pdf    restore, byte-exact
    pdfdump input.pdf                            info only: items, streams, coverage (like "vp filename")

Status: plan only; the tool is not implemented. Items marked *(prov.)* are defaults to be settled by
measurement. qpdf references are to libqpdf at commit 4eba958 (2026-09-26). Numbers in Appendix B were
measured on the uploaded `preflate.inc`.

Rev 3 changes:

1. All integers are varints, delta-coded wherever a prediction exists (§3.3).
2. Channel split is fixed; the string channels go to `<P>.txt`.
3. Extensions name the content: signature sniffing (`.jpg`, `.png`, `.xml`, ...) overrides class defaults (§6.3).
4. No threading inside pdfdump; parallelism is across PDFs and in downstream recompression.

Rev 2 changes:

1. Deflate inversion is `preflate.inc` from ziprec. The zlib parameter search and the zlib dependency are gone.
2. Files encrypted with an empty user password decrypt and round-trip with no options; `-P` for the rest.
3. Raw images are written as BMP3.
4. Stream extraction comes first: milestones reordered, syntax compaction follows.

---

## 0. Summary

- **Lossless by construction, not by model quality.** Every unit (file, item, object, token, whitespace
  gap, filter layer) has a verbatim representation. The modelled representation is used only after the
  encoder has run the decoder's own serializer on it and got the original bytes back. M0 is already a
  correct tool; each later milestone only moves bytes from "verbatim" to "modelled".
- **Decoder has no heuristics.** All guessing (stream extents, stream classes, passwords, formatting
  profile) happens in `c` and is stored as explicit choices.
- **qpdf is a reference for parsing/decoding rules only, not the engine.** Its tokenizer keeps raw token
  text but the object layer does not; it folds incremental updates (preserving them is in its TODO.md);
  its manual states output is not byte-for-byte idempotent.
- **Extraction first.** M0–M4 give a complete lossless extractor: every payload decoded as far as
  exactly re-encodable and written as a typed file, encrypted files included, with object syntax
  still stored as raw text. M5–M9 then compact the syntax.
- The dump is input for downstream compressors, so no entropy coding here: redundancy removal, binary
  fields, type-separated channels, payloads as standalone typed files.

## 1. Requirements

| # | Requirement |
|---|---|
| R1 | `d(c(x)) == x` for any input, including damaged PDFs and non-PDF files (worst case: stored). |
| R2 | Decoder is deterministic and heuristic-free. Encoder verifies every modelling decision through the decoder path before committing output. |
| R3 | No magic: unrecognized data is copied. |
| R4 | Payload streams come out as standalone files with type extensions (`.jpg`, `.bmp`, `.ttf`, `.icc`, ...), ready for per-type tools. |
| R5 | Fields computable from other data are not stored (§2). |
| R6 | Text is stored as decoded bytes (no escapes/hex), numbers and type tags as binary. |
| R7 | Single `.cpp` frontend + `.inc` modules, no separately compiled modules, system includes at the top of the `.cpp`. gcc/clang/MSVC, Linux/Windows, 64-bit offsets. Only non-own code: `preflate.inc` (it carries its own inflate and Huffman re-encoder). |
| R8 | `d` takes no modelling options and no password: everything rides in the dump header. |
| R9 | A file encrypted with an empty user password is decrypted, extracted and restored with no options. |

Non-goals: entropy coding, rendering, repair/normalization, Unicode text extraction.

## 2. Redundancy inventory

**A. Derivable — dropped, recomputed at restore**

| Data | Recomputed from | Milestone |
|---|---|---|
| Encoded stream bytes | decoded data + reconstruction record | M2 |
| zlib Adler-32, PNG-predictor row tags (when constant/predictable) | decoded data | M2 |
| Ciphertext, per-object keys | plaintext + file key (+ stored AES IVs) | M4 |
| Stream `/Length` (direct, or the indirect integer object it points to) | encoded payload size | M5 |
| Object numbers in `N G obj` | sequential prediction (prev+1, gen 0) | M5 |
| Font `/Length1..3`, `/DL`, EmbeddedFile `/Params /Size`, `/CheckSum` (MD5) | decoded data | M5 |
| Classic xref: entry offsets, subsection headers, free list | object positions at restore | M6 |
| `startxref`, trailer `/Size`, `/Prev`, `/XRefStm` | xref positions, max object number | M6 |
| ObjStm `/N`, `/First`, the (objnum, offset) header table | member serialization | M7 |
| XRef stream body, `/Index`, `/Size` | object positions + ObjStm membership | M7 |
| Linearization dict `/L /H /O /E /N /T` | layout | M9 |
| Hint stream tables | layout + page tree (best effort) | M9 |
| `/Count`, `/Parent`, image dims vs JPEG SOF, `/LastChar` vs `/Widths` | object graph | M10, only if measured gain |

**B. Notation — canonical form + exceptions**: whitespace/EOL between tokens, number spelling, string
escapes and literal-vs-hex choice, name `#xx` escapes, ASCIIHex/ASCII85 line wrapping, xref entry EOL
style.

**C. Payload — decoded as far as exactly re-encodable, then written to files**: images, fonts, ICC,
attachments, XMP, CMaps; content streams are extracted as text first (M3) and parsed later (M8).

## 3. Core mechanisms

### 3.1 Fallback ladder

| Scope | Modelled form | Verbatim form | Demoted when |
|---|---|---|---|
| File | item sequence | whole file stored | not a PDF; final compare fails |
| Region between items | — | raw bytes to `<P>.bin` | junk, unparsable object |
| Object | token sequence | raw object text | tokenizer error; before M5 always |
| Token | canonical under profile | raw token text | spelling ≠ canonical |
| Gap (whitespace + comments between tokens) | profile default | explicit string | ≠ default |
| Derived field | omitted | explicit value | predictor ≠ actual |
| Cipher layer | plaintext + key (+ IV) | ciphertext kept | no key; bad padding/length |
| Filter layer | decoded data + recon record | layer left encoded | no exact re-encode |
| Typed stream parse (content, ObjStm, XRef) | channels | decoded bytes to a file | serializer mismatch |

### 3.2 Serializer as ground truth

The `emit_*()` routines that produce PDF bytes are shared by `c` and `d`. In `c` their output is compared
against the source span immediately; a mismatch selects the exception/verbatim variant for that unit.
There is no separate diff stage, and no way for encoder and decoder to disagree on canonical forms.
Additionally `c` performs a full in-memory restore and compare before committing any output file; on
mismatch (i.e. a bug) it writes stored mode and reports.

Corollary used throughout: by induction the restored file equals the original, so the encoder checks a
derived value against *original* offsets/sizes; it does not simulate layout.

### 3.3 Token spelling: canonical-under-profile or verbatim (M5)

Profile = per-file parameters learned in a statistics pass, stored once in the header.

| Token | Canonical form | Profile parameters | Stored |
|---|---|---|---|
| Integer | `-?digits`, no leading zeros/`+` | — | zigzag varint |
| Real | `-?int.frac`, fraction digit count preserved | `.5` vs `0.5` | one varint: zigzag(mantissa)·8 + scale (0..6, 7 = escape) |
| Name | `#xx` only where required | hex case | id from static+dynamic table; new names as text |
| Literal string | fixed escape policy | per byte class (`( ) \`, CR, LF, TAB, other <0x20, ≥0x80): raw / `\c` / `\ddd`; balanced parens raw y/n | length + decoded bytes |
| Hex string | even digit count, no whitespace | hex case | length + decoded bytes |
| UTF-16BE text string (FE FF BOM, valid) | — | — | UTF-8 view, flagged, round-trip checked |
| Keyword/bool/null, `[ ] << >>` | exact | — | tag |
| Reference `N G R` | single spaces | — | zigzag(N − predicted), gen only if ≠ 0 |

Numbers never pass through binary floating point. Literal-vs-hex is a tag bit. Anything non-canonical
(`+1`, `007`, `1.`, `-0`, odd-length hex, `/A#42`, line continuation in a string) is one verbatim token.

All integers are varints. Where a prediction exists the stored value is zigzag(actual − predicted):

| Value | Prediction |
|---|---|
| Object number in `N G obj` | previous object + 1 |
| Generation | 0 |
| Reference target | previous reference in the same array; otherwise the current object's number |
| ObjStm member number | previous member + 1 |
| Explicit `/Length` (exception path) | actual payload size |
| Explicit xref offset (exception path) | the entry predictor of §7 |
| Explicit xref subsection start | end of the previous subsection |
| Gap-exception position | previous exception position |
| Content-stream operands | none by default; previous operands of the same operator is an M8 measurement |

Everything else (dictionary integers, array elements, string lengths) is a plain varint.

### 3.4 Gap model (M5)

Gap = bytes between two adjacent tokens (whitespace, EOLs, comments).

- Context: (left token class, right token class, container: top/dict-key/dict-value/array/objstm/content,
  nesting depth clamped). About 16 token classes.
- Profile: default gap string per context, chosen as the most frequent one in the file. Strings live in a
  small dictionary with run-length coding (linearized files carry long space padding).
- Coding: nothing for a default gap; exceptions are a sparse list of (gap-counter delta, string id). A
  mechanically produced file costs zero bytes of formatting beyond the profile.
- Depth in the context covers indentation-style pretty printing.
- Later, data-driven: column-wrap rule ("EOL instead of SP when column + next token > W") and other
  rules for whatever dominates the exception histogram on the corpus.

### 3.5 Derived fields (M5)

When the serializer reaches a (container kind, key) with a registered predictor and the actual value
equals the prediction, the value is one `T_DERIVED` tag with no payload. An indirect `/Length` target is
`T_DERIVED_LEN` + stream reference. Predictors may use only state the decoder has at that point in file
order (objects already emitted, the current stream's payload, the offset table).

### 3.6 Layout and forward dependencies (M6, M9)

`d` writes sequentially. Rule: **every item's serialized size must be known when it is written.**
Backward-dependent values (offsets of earlier objects, lengths of the current stream) are written
directly. Forward-dependent values get a fixed-width slot patched by seek-back:

| Case | Slot width from |
|---|---|
| Classic xref entry pointing forward (linearized first-page xref) | always 10 digits |
| `/Prev`, `/L /H /E /T` pointing forward | digit count stored (1 byte) |
| Forward-dependent *compressed* xref stream (linearized, PDF ≥ 1.5) | encoded length stored; space reserved, filled and checked at the end |

No fixed-point iteration. Non-linearized files have no forward slots at all.

## 4. Pipelines

**`c`** (random access, input memory-mapped):

1. Find `%PDF-` (first 1024 bytes); bytes before it are a raw item; record offset bias.
2. Best-effort xref read. Hints only: resolving indirect `/Length`, spotting damage. Never trusted.
3. Segment by sequential scan from offset 0, not via xref: header, `N G obj … endobj`, `xref`, `trailer`,
   `startxref`, `%%EOF`, raw regions. Stream extent: `/Length` checked against `endstream`; else scan
   for `endstream`/`endobj` as `recoverStreamLength` does. A wrong guess costs compactness only.
4. Encryption (M4): find `/Encrypt`, recover the file key, decrypt stream payloads (§8).
5. Filter-decode every stream in file order; build recon records; verify each by re-encoding (§6).
6. Read-only expansion of ObjStm contents: needed for classification (page, font and filespec
   dictionaries live there) and for indirect `/Length` targets. Read-only XRef stream parse for hints.
7. Read-only parse of all object dictionaries; collect (key path → referenced stream) pairs;
   classify streams; write payload files.
8. From M5: tokenize into token arrays, learn profiles (token spelling, gaps, xref styles;
   content-stream profile from a sample), emit channels in file order through the shared serializer.
9. Full restore in memory, compare, commit; else stored mode.

Steps 6–7 never need exact re-serialization: classification is an encoder-only decision recorded in the
stream descriptor. That is what lets extraction ship before the syntax model.

**`d`** (streaming): read header; for each item read channels in the order the encoder wrote them;
rebuild payloads (file → filters → cipher); serialize; record offsets; patch slots; done.
Memory: offset table + current item.

## 5. Dump format *(prov.)*

### 5.1 Channels and files

The writer API is `put(channel, bytes)`. The decoder reads in exactly the encoder's write order, so any
set of channels can share one physical file with no framing. The channel→file map is a header table.
The split below is the fixed default; `-c` remaps it. Record layouts in §5.2–5.3 remain provisional.

| Ch | Holds | Default file |
|---|---|---|
| S | item tags, token tags, name ids, string lengths, stream descriptors | `<P>.meta` |
| N | integers, reals, reference deltas | `<P>.meta` |
| T | decoded object strings; new names (LF-terminated); raw object text before M5; decoded ObjStm text before M7 | `<P>.txt` |
| X | content-stream string operands (`Tj TJ ' "`) | `<P>.txt` |
| O | content-stream operators and operand numbers | `<P>.ops` |
| F | gap exceptions, verbatim tokens/objects | `<P>.fmt` |
| R | incompressible side data: preflate recon blobs, other filter recon, AES IVs | `<P>.rec` |
| B | shared bytes: raw regions, streams under the small-stream threshold, inline images | `<P>.bin` |
| — | extracted streams, ordinal in file order | `<P>%08X.<ext>` |

`<P>.meta` begins with: magic, format version, option flags, channel map, file key (if encrypted),
profiles. Format version is bumped freely; old dumps are refused, not misread.

### 5.2 Items (file level)

    I_HEADER    version digits, EOL style, binary-comment line
    I_OBJ       object number (predicted prev+1), gen (predicted 0), value tokens (raw text before M5)
    I_STREAM    I_OBJ + stream descriptor (written before the dict tokens, so /Length is derivable)
    I_XREF      classic table: mode + sparse exceptions (§7); raw before M6
    I_TRAILER   dict tokens
    I_STARTXREF predictor select: previous xref / first xref / 0 / explicit
    I_EOF       %%EOF
    I_RAW       length; bytes to B
    I_END

Stream descriptor: class id; storage (`<P>.bin` / class file / own file ordinal / duplicate of ordinal k / parsed in-line);
extension when not implied by class; cipher applied (none / RC4 / AES-128 / AES-256); number of
filter layers undone and one recon reference per layer. The EOL after `stream` and before `endstream`
are ordinary gaps.

Recon references go through a per-file dictionary of distinct blobs: one producer mostly yields the
same few blobs (Appendix B: 2,668 streams, 150 distinct, 55% equal to the previous stream's).

### 5.3 Object tokens (M5)

    T_NULL T_TRUE T_FALSE T_INT T_REAL
    T_NAME(id) T_NAME_NEW
    T_STR T_HEX T_STR_U8          literal / hex / UTF-16BE stored as UTF-8
    T_ARR_OPEN T_ARR_CLOSE T_DICT_OPEN T_DICT_CLOSE
    T_REF T_REF_G
    T_DERIVED T_DERIVED_LEN
    T_VERBATIM                    raw token text to F

Token-level, not object-model-level: key order, duplicate keys and odd nesting survive without special
cases. Name table: static list generated from corpus frequency (stable ids across files, helps solid
archives) plus dynamic entries in order of first appearance.

## 6. Streams

### 6.1 Filter inversion (M2)

Filters are undone outermost first; a chain may be undone partially.

| Filter | Exact re-encode | Recon record | If it fails |
|---|---|---|---|
| ASCIIHexDecode | canonical writer | hex case, line width, EOL, `>` | keep layer |
| ASCII85Decode | canonical writer | line width, EOL, `z` use, `~>`, tail | keep layer |
| RunLengthDecode | packet boundaries are the only freedom | encoder-variant id, else explicit packet lengths | cannot fail |
| LZWDecode | greedy encoder | EarlyChange, initial Clear, table-full Clear policy, EOD/padding | keep layer |
| FlateDecode | `preflate.inc` (§6.2) | preflate diff + zlib wrapper bytes | keep layer |
| Predictor 2 (TIFF) | deterministic | none | — |
| Predictor ≥ 10 (PNG) | re-filter with row tags | tags: constant / libpng-heuristic / explicit | cannot fail |
| DCT, JPX, JBIG2, CCITTFax | not decoded (terminal) | — | written as-is to a typed file |
| Crypt | §8 | | |

### 6.2 Deflate via preflate.inc

pdfdump owns the zlib wrapper, preflate owns the raw deflate stream.

- Check `CMF/FLG` (2 bytes; stored only when not the common value for the file).
- `preflate_decode` streaming overload on a span-backed `InputStream` starting at payload+2 and
  running to the end of the payload: returns unpacked data, diff, and `deflate_size` = bytes consumed.
  No need to know the deflate length in advance.
- Tail after `deflate_size`: 4-byte big-endian Adler-32 of the unpacked data is derived; anything
  else (missing, wrong, extra bytes) is an explicit tail in the recon record.
- Restore: `preflate_reencode(out, diff, unpacked)`, prepend header, append Adler/tail.
- Failure (`false` from decode, or re-encode ≠ original): the layer stays encoded, the stream goes
  out as `.z` and its contents are not parsed.

Integration points, from reading and running the library (Appendix B):

- pdfdump is single-threaded, and so is the library as shipped (its `TaskPool` never starts a
  worker). Its lazily allocated static Huffman tables are therefore not a concern.
- Positions inside a meta block are 32-bit `uint`, and with the default `metaBlockSize` (INT32_MAX)
  a whole stream is one meta block, held as the output cache plus a copy plus 4-byte token records.
  Pass a finite `metaBlockSize` (minimum 256 KiB) for large streams; that bounds both. GB-scale
  streams were not exercised here.
- The file brings its own system includes and global `byte`/`uint` typedefs: include system headers
  first in `pdfdump.cpp`, and avoid a global `using namespace std` (or wrap it in a namespace as
  `dxt5comp_core.inc` does).

Recon cost is not negligible for every producer. Streams in qpdf's test PDFs cost 3–6 bytes each
(median 6); zlib-ng streams measured 1–4% of the deflate size at levels 6–9 and 23–29% at levels 1–3.
The blobs are arithmetic-coded, so channel R is incompressible downstream and is reported as its own
metric.

### 6.3 Classification and extraction (M3)

Encoder-only; class and extension are stored in the descriptor. The class comes from the stream's own
dictionary (`/Type`, `/Subtype`, terminal filter, `/FunctionType`, `/PatternType`, `/ShadingType`) and
from the referrer key path. The extension names the actual content: a known signature in the stored
bytes overrides the class default, so a JPEG, PNG or XML block gets `.jpg`, `.png`, `.xml` wherever it
sits (image, attachment, metadata, unclassified).

| Class | Detection | Output |
|---|---|---|
| ObjStm, XRef | `/Type` | until M7: ObjStm text to T, XRef rows to B; then parsed in-line |
| Content | page `/Contents`, `/Subtype /Form`, `/PatternType 1`, `/CharProcs` values, `/AP` streams | class file `<P>.cs` until M8, then parsed |
| JPEG / JPEG 2000 / JBIG2 / CCITT | terminal filter | `.jpg`, `.jp2` or `.j2k`, `.jb2` (+ `.jb2g` globals), `.g4` |
| Raw image | `/Subtype /Image`, fully decoded | `.bmp` (§6.4); else `.img` |
| TrueType / CFF / OpenType / Type 1 | `/FontFile2`, `/FontFile3` + `/Subtype`, `/FontFile` | `.ttf` `.cff` `.otf` `.pfa` |
| ICC | `/ICCBased` referent, `acsp` signature | `.icc` |
| CMap | `/ToUnicode`, `/Type /CMap` | class file `<P>.cmap` |
| XML: XMP, XFA, any XML block | `/Type /Metadata`, `/XFA`, signature | `.xml` |
| JavaScript | `/JS` | `.js` |
| Attachment | `/Type /EmbeddedFile` or under Filespec `/EF` | by signature; else extension of `/UF` or `/F`, sanitized; else `.bin` |
| Function, shading mesh, CIDSet, CIDToGIDMap, hint | key path | `.bin` |
| Unknown / still encoded / still encrypted | — | by signature, else `.bin` / `.z` / `.enc` |

Signatures checked on the stored bytes (after optional BOM and whitespace for the text ones):

| Signature | Extension |
|---|---|
| `FF D8 FF` | `.jpg` |
| `89 50 4E 47 0D 0A 1A 0A` | `.png` |
| `GIF87a`, `GIF89a` | `.gif` |
| `49 49 2A 00`, `4D 4D 00 2A` | `.tif` |
| JP2 signature box; `FF 4F FF 51` | `.jp2`; `.j2k` |
| `<?xml`, `<?xpacket`, `<x:xmpmeta` | `.xml` |
| `%PDF-` | `.pdf` |
| `50 4B 03 04` | `.zip` |
| `00 01 00 00`, `true`, `ttcf`; `OTTO` | `.ttf`; `.otf` |
| `%!PS-AdobeFont`, `%!FontType1` | `.pfa` |
| `acsp` at offset 36 | `.icc` |

Extension precedence: terminal filter → BMP3 wrap → signature → attachment file name → class default
→ `.bin`. The list grows with whatever the corpus shows in `.bin`.

- Small-stream threshold `-p N` (default 1024 *(prov.)*): a stream whose stored form is below N bytes
  is appended to `<P>.bin` instead of becoming its own file, whatever its type; `-p 0` gives every
  stream a file. This is about file count only: a PDF can hold 10^5 streams, most of them tiny.
- Class files (`<P>.cs`, `<P>.cmap`) concatenate all streams of the class regardless of size.
- Dedup: hash + length, then byte compare; duplicate descriptors point at the surviving ordinal.
- An attachment that is itself a PDF is just a `.pdf` file; no automatic recursion.

### 6.4 Raw images as BMP3

BMP3 = `BITMAPFILEHEADER` + 40-byte `BITMAPINFOHEADER`, `BI_RGB`, bottom-up. Applies when all filters
and the predictor are undone and the data holds at least H·⌈W·comps·bpc/8⌉ bytes.

| PDF samples | BMP | Transform (all exact) |
|---|---|---|
| 3 × 8 bpc (DeviceRGB, CalRGB, Lab, ICC N=3) | 24 bpp | R↔B swap, row order flipped, rows padded to 4 bytes |
| 1 × 8 bpc gray | 8 bpp, 256-entry gray ramp | flip, pad |
| Indexed, 8 / 4 / 1 bpc | 8 / 4 / 1 bpp, palette from the `/Indexed` lookup when resolvable to RGB or gray, else ramp | flip, pad |
| 1 × 1 bpc (gray, `/ImageMask`) | 1 bpp, 2-entry palette giving correct polarity | flip, pad |
| 1 × 4 bpc gray | 4 bpp, 16-entry ramp | flip, pad |
| 4 × 8 bpc (DeviceCMYK, ICC N=4) | 32 bpp, bytes in stream order *(prov.)* | flip; colours not meaningful in a viewer |
| 2 bpc, 16 bpc, other component counts | none: `.img` raw | — |

- PDF rows are byte-aligned and MSB-first, as are BMP rows, so 1/4/8 bpp rows copy bytewise.
- Restore reads width, height, bit count and `bfOffBits` from the BMP itself and ignores the palette
  and every other header field. The palette is therefore the encoder's free choice, used only to make
  the file display correctly.
- Bytes beyond the expected size (common: one stray EOL byte) are a tail in the recon record. Fewer
  bytes than expected, or a BMP that would exceed 4 GiB: `.img`.
- `/SMask` and `/Mask` streams are separate gray images with their own files.

### 6.5 Content streams (M8)

- Same lexer in content mode: operands, operators, inline images.
- Operator-first coding with implied arity: channel O gets the operator id, then operands according to
  the operator's signature without per-operand tags (`re` = 4 numbers, `Tf` = name + number, `cm` = 6
  numbers). Operands not matching the signature, variable-arity operators (`sc scn SC SCN`) and
  unknown keywords use a generic tagged escape. Restore emits operands, then the operator.
- Numbers: unified varint from §3.3 (integers are scale 0).
- `Tj ' "` strings and `TJ` string elements go to X as decoded bytes; `TJ` kerning numbers stay in O,
  so X is contiguous text for simple-encoded fonts.
- Inline images: `BI` dict as tokens, data to B. Data length from the dict for unfiltered images, `/L`
  (PDF 2.0) when present, otherwise a `findEI`-style heuristic; verified like everything else.
- Gap contexts add the preceding operator id. Typical default: SP between operands, EOL after operator.
- Each stream of a `/Contents` array is parsed on its own; a stream that does not lex stand-alone
  falls back to text.

## 7. Xref, trailer, object streams, linearization

Decoder state: for every emitted indirect object (num, gen, offset, revision); for every ObjStm member
(num, container, index). A revision = the objects emitted since the previous xref section.

**Entry prediction.** In-use entry for object n in section X: offset of n's definition if unique in the
file, else the latest definition before X; otherwise an explicit selector or explicit offset delta.

**Classic table, section mode (M6):**

| Mode | Subsections | Entries |
|---|---|---|
| A full | single `0 N` | predicted in-use; gaps are free entries |
| B incremental | maximal runs over this revision's objects (+ optional `0 1`) | predicted in-use |
| C explicit | list of (first, count) | per entry: predicted / free / explicit |

Per section: entry EOL style (`SP LF`, `SP CR`, `CR LF`), free-list convention (the linked list the
spec describes, plus whichever degenerate forms the corpus shows), sparse entry exceptions. Malformed
tables (wrong entry width, garbage) are raw items.

**XRef streams (M7).** Dict coded like any dict with `/Size /Index /Length` derived; `/W` stored when
not minimal; rows regenerated from decoder state (type 1 offsets, type 2 container+index, type 0
free); then predictor and deflate re-applied from the recon record. Hybrid files: `/XRefStm` derived.

**ObjStm (M7).** Members become ordinary object records in header order; `/N`, `/First` and the offset
table derived; separators inside the stream are gaps with their own context. Explicit table if the
header disagrees with the actual layout.

**Incremental updates.** Preserved as-is by file-order processing: revisions are just successive item
runs; redefinitions of an object number are distinct items.

**Linearization (M9).** Stage 1: forward slots for the first-page xref and the linearization dict
numbers; padding is a run-length gap; hint stream dumped as an opaque stream. Stage 2: regenerate the
hint tables from layout and page tree, keep only if exact (qpdf computes and checks the page-offset,
shared-object and outline tables; other producers' tables are not guaranteed to follow the same rules).

## 8. Encryption (M4 streams, M5 strings)

Behaviour:

| Input | `c` without options | `c -P pw` |
|---|---|---|
| Standard handler, empty user password | decrypted, extracted, restored exactly | same |
| Standard handler, password required | lossless; strings and payloads stay ciphertext (`.enc`), ObjStm contents unreadable; notice on stderr | decrypted if `pw` is the user or owner password |
| Other handlers (public-key etc.) | lossless, ciphertext kept | same |

- Key recovery runs in `c` only: empty user password, then `-P` as user, then as owner password.
  Standard security handler V1/V2/V4/V5, R2–R6: RC4-40/128, AES-128 (AESV2), AES-256 (AESV3).
- The recovered file key (5–32 bytes) is stored in the dump header. `d` never takes a password and
  runs no password algorithm: it derives per-object keys (V < 5) and re-encrypts. The dump holds the
  plaintext anyway, so the key exposes nothing more.
- The stream descriptor records the cipher actually applied. Crypt-filter selection (`/StmF`, `/EFF`,
  per-stream `/Crypt`, `/EncryptMetadata false`, XRef streams never encrypted) is resolved by the
  encoder and not re-derived by the decoder.
- Strings (M5): decrypted in top-level objects only; never inside an ObjStm (the container is
  encrypted as a whole), the trailer, or the `/Encrypt` object (its number is in the header). A string
  token carries a "plain" bit for the exceptions (signature `/Contents`), and one that does not
  decrypt cleanly is a verbatim token. Before M5 strings sit untouched inside the raw object text.
- Restore: RC4 is deterministic. AES-CBC needs the 16-byte IV per string/stream (channel R) and
  canonical padding; a unit with odd length or bad padding stays ciphertext.
- Order in `c`: decrypt → filters → ObjStm expansion → classification. On an encrypted file nothing
  after step 4 of §4 works without the key, which is why this sits directly after extraction.

## 9. CLI *(prov.)*

| Option | Meaning | Default |
|---|---|---|
| `-P PW` | password for encrypted input (user or owner); `c` only | empty user password is tried automatically |
| `-f N` | 0 keep streams encoded, 1 undo filters | 1 |
| `-s N` | syntax depth: 0 stored, 1 items + raw object text, 2 object tokens, 3 + content streams, 4 + graph predictors | highest built |
| `-p N` | small-stream threshold: streams below N bytes share `<P>.bin` instead of getting own files | 1024 |
| `-D` | disable dedup of extracted streams | on |
| `-c MAP` | channel→file map, e.g. `S,N,T,X,O,F,R,B` to split everything | §5.1 |
| `-n` | skip the final in-memory verify | off |
| `-v` / `-q` | per-stream report / quiet | — |

`d` reads all of this from the header.

## 10. Source layout

    pdfdump.cpp        CLI, system includes, pipeline driver
    preflate.inc       from ziprec, unmodified
    chan.inc           channel reader/writer, channel→file map, header
    pdf_lex.inc        lexer (object + content modes); read-only use from M1, raw-span capture from M5
    pdf_seg.inc        file segmenter, stream extent logic, raw regions
    pdf_filt.inc       hex, a85, rle, lzw, predictors, zlib wrapper around preflate, recon dictionary
    pdf_class.inc      read-only ObjStm expansion, classification, signature table, small-stream file, dedup
    pdf_bmp.inc        BMP3 wrap/unwrap
    pdf_crypt.inc      security handler, RC4, AES, MD5, SHA-2
    pdf_emit.inc       shared serializer: tokens, gaps, profiles, compare hook
    pdf_obj.inc        object/token coder, name table, derived-field registry
    pdf_xref.inc       xref tables/streams, trailer, startxref, forward slots
    pdf_objstm.inc     object streams (modelled)
    pdf_cs.inc         content streams, operator table, inline images
    pdf_lin.inc        linearization dict, hint tables
    pdf_names.inc      generated static name table

## 11. Milestones

Every milestone exits only with 100% byte-exact round trip on the whole corpus (§12) and updated
metrics. LOC figures are rough sizing, not commitments.

| M | Scope | Exit criterion (beyond round trip) | ~LOC |
|---|---|---|---|
| M0 | CLI, channels, header, stored mode, final-verify path, corpus harness | harness reports per-file sizes per channel | 400 |
| M1 | Segmenter: items, stream extents incl. recovery, raw regions. Read-only lexer for dict lookups. Objects as raw text; payloads to files by terminal filter | % of bytes in raw regions per file; JPEG/JPX streams come out as valid files | 900 |
| M2 | Filter engine: hex, a85, RLE, LZW, predictors, `preflate.inc` behind the zlib wrapper, recon dictionary | per producer: layers undone, recon bytes vs deflate bytes, speed | 900 |
| M3 | **Stream extraction**: read-only ObjStm expansion, classification, signature-based extensions, typed files, BMP3, fonts/ICC/attachments, small-stream file, dedup | extracted files open in their native tools; file count bounded on large documents; share of payload bytes in typed files vs `.bin` | 1000 |
| M4 | **Encryption, streams**: key recovery, RC4/AES, IVs, header key | qpdf-encrypted variants of the corpus (each R, empty user password) extract the same files as the plain originals, no options | 900 |
| — | *Checkpoint: complete lossless extractor. Syntax still raw text.* | | |
| M5 | Lexer span capture, shared serializer, token coder, profiles, gap model, name table, string decryption, `/Length` and other data-derived fields, object-number prediction | exception histogram by producer; structure bytes vs original non-stream bytes | 1600 |
| M6 | Classic xref, trailer, `startxref`, `/Size`, `/Prev`, free lists, incremental updates, forward-slot engine | share of xref sections in modes A/B without exceptions | 600 |
| M7 | ObjStm members in-line, XRef streams regenerated, hybrid files | all-streams files reach the same structure cost as classic ones | 600 |
| M8 | Content streams: operator coding, text channel, inline images, content gap profile | % of content bytes parsed; O/X sizes; downstream size vs `.cs` text | 1000 |
| M9 | Linearization: forward slots, dict derivation; then hint-table regeneration | linearized corpus: no raw fallbacks; hint regeneration hit rate | 500 + 800 |
| M10 | Experiments, each kept only if it pays (§13) | measured downstream gain per experiment | open |
| M11 | Hardening: fuzzing, multi-GB files, memory caps, speed, Windows builds, format freeze | fuzz corpus clean under ASan/UBSan; throughput targets set from M2–M8 numbers | — |

Order rationale: M1–M4 touch no syntax modelling, so the extractor is usable while the dump still
stores objects verbatim. M4 sits before M5 because on an encrypted file every extracted stream is
noise without it. M8 can move ahead of M6/M7 if content parsing matters more than xref compaction;
it depends only on M5.

## 12. Testing and metrics

**Corpus**

- qpdf `qpdf/qtest/qpdf/*.pdf` (628 files): linearized, object streams, encrypted, damaged xref, bad
  lengths, hybrid files; plus the `fuzz/*_seed_corpus` filter inputs.
- pdf.js, PDFium and poppler test PDFs for malformed real-world cases.
- A GovDocs1 PDF sample for volume and producer spread.
- Producer matrix, a few files each: Acrobat Distiller / PDF Library, Word, LibreOffice, pdfTeX /
  LuaTeX / xdvipdfmx, Ghostscript, Chrome (Skia), Quartz, iText, PDFBox, cairo, InDesign, scanner
  software. Include builds linked against zlib-ng and other non-zlib deflaters for the recon metric.
- Synthetic variants from qpdf: `--linearize`, `--object-streams=generate|disable`, `--encrypt`
  (each R, with and without user password), `--qdf`, `--stream-data=uncompress`; incremental updates
  appended by a signing or form-filling tool.
- Non-PDF and truncated files (R1, R3).

**Harness**: for each file `c` → `d` → compare; CSV with per-channel sizes, raw-region bytes, exception
counts, per-filter success, time, peak memory. Run under ASan/UBSan in CI builds.

**Metrics**

| Metric | Definition |
|---|---|
| Round trip | must be 100% from M0 on |
| Raw share | bytes in raw regions + verbatim tokens/objects, over file size |
| Filter coverage | layers undone / layers present, by filter and by producer, in streams and bytes |
| Recon share | channel R bytes over encoded stream bytes, by producer |
| Typed share | payload bytes in typed files over all payload bytes |
| Structure cost | S+N+F bytes over original non-stream bytes |
| End-to-end | size after a fixed downstream compressor set on the dump vs the same on the original PDF, and vs `qpdf --stream-data=uncompress --object-streams=disable` output (not lossless; a reference point for what plain unpacking gives) |
| Speed | MB/s for `c` and `d`, single thread |

## 13. Risks and open decisions

**Risks**

| Risk | Mitigation |
|---|---|
| Large recon for non-zlib deflaters (measured up to 29% of deflate size) | report recon share per producer; recon dictionary for the cheap case; nothing to do for the rest inside pdfdump |
| preflate speed on ordinary data (measured 5–9 MB/s of unpacked data, one thread) | accepted: parallelism is across PDFs, outside the tool; `-f 0` for a fast pass |
| preflate memory and 32-bit positions on very large streams | finite `metaBlockSize`; test multi-GB streams in M2 |
| Gap model leaves many exceptions on some producers | exception histogram per producer in M5; add rules (column wrap first) only where data shows them |
| Wrong stream extent on damaged files | sequential segmenter + `endstream` scan; raw region fallback; never xref-driven |
| Misclassified stream written as BMP/font | harmless for restore (class is stored, parse is verified); only the extension is wrong |
| Hint tables not reproducible for non-qpdf producers | stage 1 keeps them as opaque streams; regeneration is optional |
| 10^5+ output files on large documents | small-stream threshold, class files, dedup |
| Encoder memory on multi-GB inputs | mmap input, token arrays for syntax only, payloads streamed, content profile from a sample |
| AES IVs add 16 incompressible bytes per string/stream | unavoidable; isolated in channel R |

**Decisions to confirm**

1. Small-stream threshold (§6.3): default 1024 bytes, or 0 so that every stream is a file.
2. 4-component 8-bit images as 32 bpp BMP (valid file, wrong colours) or raw `.img`.
3. File key always stored in the header (plan default) vs re-derived in `d` for the empty-password case.

**M10 experiment list** (each measured downstream before adoption)

- Schema-implied tags: omit value tags after keys with a fixed value type (`/Type` → name).
- Graph predictors: `/Count`, `/Parent`, `/LastChar`, image dims from JPEG SOF. Needs a two-phase `d`
  (materialize tokens, resolve, serialize) since these can depend on later objects.
- ToUnicode remap: where a font's code→Unicode map is injective on the codes used, write X as Unicode
  text so different subset fonts share statistics; flag per font, verify, skip otherwise.
- RGB image + 8-bit `/SMask` of the same size merged into one 32 bpp BMP for RGBA codecs.
- Further integer predictions: content-stream operands against the previous use of the same
  operator, array elements against the previous element.
- Typed strings: dates as binary, `/ID` pairs, signature blobs to `.p7s`.
- CMap parsing (`bfchar`/`bfrange` to binary), CCITT G4 decode to 1 bpp BMP with exact re-encode,
  filters inside inline images, Type 1 eexec decryption (deterministic cipher).

## Appendix A. qpdf reference map

| Topic | File (libqpdf/) | Symbols |
|---|---|---|
| Lexing rules, inline image end | `QPDFTokenizer.cc` | `Tokenizer::handleCharacter`, states `inName inNameHex inNumber inReal inString inStringEscape inStringAfterCR inCharCode inHexstring`, `findEI` |
| Object grammar | `QPDFParser.cc` | — |
| Header, startxref, xref table/stream, trailer | `QPDF_objects.cc` | `findHeader`, `findStartxref`, `read_xref`, `read_xrefTable`, `read_xrefEntry`, `read_bad_xrefEntry`, `read_xrefStream`, `processXRefStream`, `processXRefIndex`, `processXRefSize`, `readTrailer` |
| Damage recovery | `QPDF_objects.cc` | `reconstruct_xref`, `recoverStreamLength`, `findEndstream`, `validateStreamLineEnd` |
| Object streams | `QPDF_objects.cc` | `resolveObjectsInStream` |
| Filter chains, abbreviations, DecodeParms | `QPDF_Stream.cc`, `SF_FlateLzwDecode.cc`, `qpdf/SF_*.hh` | — |
| Filters | `Pl_LZWDecoder.cc`, `Pl_ASCII85Decoder.cc`, `Pl_ASCIIHexDecoder.cc`, `Pl_RunLength.cc`, `Pl_PNGFilter.cc`, `Pl_TIFFPredictor.cc` | LZW/ASCII85/ASCIIHex are decode-only in qpdf; encoders must be written |
| Encryption | `QPDF_encryption.cc`, `Pl_AES_PDF.cc`, `Pl_RC4.cc`, `RC4_native.cc`, `AES_PDF_native.cc`, `rijndael.cc`, `MD5_native.cc`, `SHA2_native.cc` | — |
| Embedded files | `QPDFEmbeddedFileDocumentHelper.cc`, `QPDFFileSpecObjectHelper.cc`, `QPDFEFStreamObjectHelper.cc`, `NNTree.cc` | `/Params` `/Size` `/CheckSum` handling |
| Page tree | `QPDF_pages.cc` | — |
| Canonical spelling | `QPDF_String.cc`, `QPDFObjectHandle.cc` | `QPDF_String::unparse`, `Name::normalize` |
| Writer-side structures | `QPDFWriter.cc` | `writeXRefTable`, `writeXRefStream`, `bytesNeeded`, `writeObjectStream`, `writeObjectStreamOffsets`, `writeTrailer`, `writeLinearized`, `calculateXrefStreamPadding`, `writeHintStream` |
| Content streams | `QPDFObjectHandle.cc`, `ContentNormalizer.cc`, `Pl_QPDFTokenizer.cc` | `parseContentStream`, `parsePageContents` |
| Linearization | `QPDF_linearization.cc`, `BitStream.cc`, `BitWriter.cc`, `manual/linearization.rst` | `readLinearizationData`, `readHintStream`, `readHPageOffset`, `readHSharedObject`, `readHGeneric`, `calculateHPageOffset`, `calculateHSharedObject`, `calculateHOutline`, `generateHintStream`, `checkLinearizationInternal` |
| Test inputs | `qpdf/qtest/qpdf/`, `fuzz/` | — |

## Appendix B. preflate.inc probe

Harness: for each zlib stream, streaming `preflate_decode` on the rest of the file, then
`preflate_reencode`, byte compare against the original deflate bytes, Adler-32 check of the next
4 bytes. One thread, 2-core container.

**Build.** Compiles stand-alone as included: g++ 13.3 `-std=gnu++17` and `-std=gnu++2a`, clang 18
`-std=gnu++17`. clang warns that `std::result_of` (in `TaskPool::addTask`) is deprecated.

**qpdf test PDFs** (628 files, every `stream` + EOL followed by a valid zlib header):

| | |
|---|---|
| Candidates | 2,696 |
| Rejected by `preflate_decode` | 28: 27 invalid deflate and 1 truncated, per zlib's own inflate |
| Re-encoded byte-exact | 2,668 of 2,668 decoded |
| Adler-32 present and correct | 2,665 (3 need an explicit tail) |
| Deflate / unpacked bytes | 5,318,133 / 204,636,199 |
| Recon total | 15,286 B (0.29% of deflate) |
| Recon per stream | min 3, median 6, p90 6, p99 25, max 571 B |
| Distinct recon blobs | 150; 55% of streams equal the previous stream's blob in the same file |
| Speed | 38 MB/s decode+analyze, 59 MB/s re-encode, of unpacked data (highly compressible streams) |

**zlib-ng 1.3.1 streams** (1,323,987 B of C++ source; a 295 KB ELF ran from 13.7% at level 1 to
2.0% at level 9):

| Stream | Deflate B | Recon B | Recon / deflate |
|---|---|---|---|
| level 1 | 344,330 | 99,605 | 28.9% |
| level 2 | 332,044 | 85,780 | 25.8% |
| level 3 | 324,047 | 74,975 | 23.1% |
| level 4 | 307,342 | 28,004 | 9.1% |
| level 5 | 294,753 | 16,078 | 5.5% |
| level 6 | 291,336 | 10,755 | 3.7% |
| level 7 | 290,110 | 8,466 | 2.9% |
| level 8 | 289,535 | 5,924 | 2.0% |
| level 9 | 289,820 | 3,070 | 1.1% |
| level 6, `Z_SYNC_FLUSH` every 4 KiB | 302,304 | 12,609 | 4.2% |
| level 6, windowBits 12 | 336,782 | 16,464 | 4.9% |
| `Z_HUFFMAN_ONLY` | 797,073 | 6 | — |
| level 0 (stored) | 1,324,092 | 48 | — |

All re-encoded byte-exact, including a stream followed by non-deflate bytes (`deflate_size` stopped at
the right byte). Speed on these: 5–9 MB/s of unpacked data in each direction.

Not covered: synthetic classic-zlib vectors (this machine's libz is zlib-ng); classic zlib is
represented only by the qpdf PDFs above. Other deflaters (libdeflate, zopfli, 7-Zip, Adobe, Microsoft)
are untested here and belong to the M2 producer matrix.
