# pdfdump — compression plan, round 2

Follow-up to `pdfdump_plan.md`. Everything below is measured on the README test set (120 PDFs:
117 files of the 005 set + the 3 supplied ones, 55.6 MB) with the current build, `xz -9`.
"Exact" means the real encoder produced the alternative stream; "prototype" means a Python model
of the encoder (`tests/csproto.py`) whose baseline lands within 2% of the real one.

## 1. Where the bytes are

PDF-specific output (everything except the payload formats that go to other recompressors):
1543 files, 8.46 MB as one tar.xz.

| Output | Holds | xz | Share |
|---|---|---|---|
| `.csn` | numbers of content streams | 3117 KB | 37% |
| `.cs` | content-stream skeleton + literal strings | 2066 KB | 24% |
| `.t1d` | decrypted Type 1 fonts | 1228 KB (fonts of a PDF in file order) | 15% |
| `.rec` | recon records | 422 KB | 5% |
| `.txn` | numbers of object text | 388 KB | 5% |
| `.txt` | object text | 383 KB | 5% |
| `.csh` | hex strings of content streams | 336 KB | 4% |
| `.bin` | small binary streams, XRef row residuals | 286 KB | 3% |
| `.ai` `.meta` `.xml` `.cmap` `.cmb` `.ps` | | 354 KB | 4% |

The per-type figures are xz of each file on its own, so they add up to slightly more than the
tar. Content streams are 65% of the total, Type 1 fonts 15%, all object-level syntax together 13%.
The earlier rounds worked mostly on the 13%.

Content-stream numbers by operator (xz of that operator's numbers over all files):

| Operator | Share of `.csn` |
|---|---|
| `TJ` (kerning inside arrays) | 30.5% |
| `c` | 13.8% |
| `Td` | 13.5% |
| `l` | 11.4% |
| `Tm` | 7.6% |
| `m` | 5.4% |
| `TD` | 4.5% |
| `cm` | 3.9% |
| `re` | 3.6% |
| `Tc`, `Tw` | 3.7% |
| everything else | 2.1% |

Text positioning is 60%, path construction 34%. No file dominates: the 12 largest `.csn`
files hold 47%.

## 2. The four questions

**Remaining computable values**

| Value | Cost now | Finding |
|---|---|---|
| Number of decimals of each lifted number | 3 bits in every value | computable from a per-group unit: P1, −241 KB exact |
| `Td`/`TD` advance after shown text | 18% of `.csn` | predictable from a table learned on the fly: P2, −7.1% of `.csn` (prototype) |
| Path coordinates | 34% of `.csn` | residual against the current point: P3, −3.3% of `.csn` (prototype) |
| Item text lengths (channel N) | 54 KB | derivable by lexing to `endobj`: P7, ceiling 54 KB |
| `/Widths`, `/W` | 44.5 KB of `.txn` | derivable from the embedded font program; three font formats to parse: P8 |
| Structure tree `/K` `/P` `/Nums` | 49 KB of `.txn` | `/ParentTree` is the inverse of `/K`; ceiling 49 KB: P8 |
| `/CharSet`, CIDSet | 12 KB + 8 KB raw | derivable from the font; too small |
| `/Rect`, `/D` (destinations) | 91 KB of `.txn` | not computable without layout |
| `TJ` kerning | 30.5% of `.csn` | a table per (font, glyph before, glyph after) is worse: +2.5% (only 41% of hits exact) |

**Duplicate records**

- One 3144-byte sRGB ICC profile is embedded in 27 of the 120 PDFs (2500 bytes of xz each), a
  2008-byte one in 2 more: 69.5 KB of payload xz. A built-in table of well-known profiles
  removes it: P5.
- Info dates repeated in the XMP packet: 49 of 182 dates, about 15 bytes each. Not worth a
  mechanism.
- Fonts identical across PDFs: 4 TrueType, 1 CFF, 3 Type 1 in 120 files. Subsets differ; nothing
  to do per file.
- BMP headers of the 923 small images in `.bin` repeat the dictionary's width and height, but
  they pay for themselves through the derived `/Width` `/Height`: raw samples instead cost
  +3.7 KB.
- Streams with identical stored form are already stored once; repeats inside the text files are
  within xz's reach.

**Mistyped data**

- Literal strings sit inside the `.cs` skeleton: 5.86 MB of its 32.4 MB raw, 1.43 MB of its
  2.07 MB xz, often raw binary glyph codes (one file's `.cs` is 22% printable). Moving them to a
  stream of their own: −88 KB (−4.3% of `.cs`), P4.
- The XMP thumbnail layer keeps the XMP text around the thumbnail in `.rec` (557 KB raw in 26
  streams). It costs 22.1 KB there against 21.1 KB as a text file: 1 KB, left alone.
- `.ps` (PostScript XObjects of one file) holds 58 KB of hex data; 14.7 KB of xz in all.
- `.bin` is 29% printable and that is image and XRef data, no text. The `.txt` files with the
  lowest printable share are low because of number markers, not binary. `.ai` is 100% text here
  (no compressed Illustrator blocks).

**Undecoded strings**

| Encoding | Bytes left | Finding |
|---|---|---|
| Octal escapes in content-stream strings | 385 KB raw | unescaping to bytes is worse: +3.2% of the strings stream |
| Hex strings in object text | 48 KB raw | as binary side stream: +1.0% of `.txt` |
| Literal strings in object text | 907 KB raw | as a side stream: −0.8% of `.txt` (9 KB) |
| Base64 | none | the "base64-looking" runs in `.txt` are `/CharSet` and `/Differences` name lists and palette hex strings; over the 808-file regression set only XMP thumbnails matched, and those are extracted |
| XML character references, URL escapes, `#xx` in names | under 4 KB each | nothing to gain |
| UTF-16BE text strings | 2.5 KB | earlier measurement: −0.7 KB |
| Streams left encoded | CCITT 373 streams / 144 KB (by design, `.tif`), Flate 5 streams / 6 KB | nothing new |

## 3. Work items

| # | Item | Gain | Evidence | Effort |
|---|---|---|---|---|
| P1 | One decimal unit per number group | `.csn` 3117 → 2875 KB (−7.7%), `.txn` −0.5 KB | exact | small |
| P2 | `Td`/`TD` advance from a learned table | −7.1% of `.csn` (about −200 KB) | prototype | medium |
| P3 | Path coordinates against the current point | −3.3% of `.csn` (about −93 KB) | prototype | medium |
| P4 | Literal strings out of the `.cs` skeleton | `.cs` 2066 → 1978 KB (−4.3%) | exact split, Python | small |
| P5 | Table of well-known ICC profiles | −69.5 KB of payload xz | exact count | small |
| P6 | Type 1 charstring operands per operator | unknown; operands are 910 KB of the 1228 KB | experiment first | medium |
| P7 | Item lengths derived by lexing | at most −54 KB | ceiling | medium |
| P8 | `/Widths`, `/W`, `/ParentTree` from other data | at most −44 KB and −49 KB | ceilings | large |

P1 to P4 together: about −620 KB, 7.3% of the PDF-specific total. All four are linear passes
over data the lifter already walks, so `c` speed is unaffected.

Outcome (section 7 has the details): all eight items are in. PDF-specific xz 8458 KB → 7726 KB
(−8.7%), dump.xz 59.01% → 57.33% of orig.xz. `c` speed was not unaffected after all: single
thread, best of 3 runs per file, 2.51 → 2.36 MB/s (−6%), every file at 659 KB/s or more; with
4 threads 4.72 → 4.46 MB/s (measured before the last P8 additions). The per-group choice among three codings and the walk cost the most;
the lifts' own inverse check is skipped when the final verify runs.

### P1. One decimal unit per number group

Now every lifted number is `zigzag(mantissa) * 8 + code`, code = number of fraction digits. In a
group such as the x operand of `Td`, "12.5", "13" and "14.25" get mantissas 125, 13 and 1425,
which breaks the delta mode and spends 3 bits per value on the code.

- Per group: unit S = largest fraction-digit count in the group. Value = mantissa · 10^(S − s).
- The spelling's own s is recomputed: s = S − (trailing decimal zeros of the value, at most S),
  and s = 0 for the value 0 ("canonical", what every writer that trims zeros produces), or s = S
  for all values ("fixed", writers with a constant number of decimals).
- A group with an escape-coded number, a spelling that is neither canonical nor fixed, or a
  mantissa that would overflow keeps the current coding. 84% of content-stream values (5.15 M of
  6.13 M) are in groups that convert.
- Format: mode byte bit 6 = unit form, followed by one byte (S, fixed flag). The lifter's
  round-trip check already covers it.

### P2. `Td`/`TD` advance from a learned table

Producers that place words or single glyphs with `Td` write the advance of the text they just
showed. The same text in the same font gives the same advance.

- Key: (font name of the last `Tf`, its size, bytes shown since the last positioning operator).
  Value: the last (x, y) written for that key.
- Known key: store x − x', y − y'. Unknown key: store the values as now. No flag is needed, the
  decoder has the same table.
- Prototype: 491,879 of 587,236 `Td`/`TD` operations hit, 418,202 of those with residual 0.
- The decoder needs what the lifter's grouping pass already has: the skeleton ahead of the
  numbers (operator after its operands) and the strings (skeleton or `.csh`, after P4 the string
  stream). Font size is itself a lifted number, decoded earlier in the same order.
- Depends on P1 (residuals need one unit).

### P3. Path coordinates against the current point

`m`, `l`: x − cx, y − cy. `c`: first control point against the current point, each next point
against the one before. `v`, `y`, `re` alike; `h` returns to the subpath start. Residuals stay in
their (operator, slot) groups, and each group keeps the better of the current coding and the
residual coding (one mode bit). Depends on P1.

### P4. Literal strings out of the `.cs` skeleton

Same shape as `.csh`: a marker byte in the skeleton, a new aux file with a length section and the
string bytes exactly as written (escapes untouched: unescaping measured +3.2%). The skeleton
alone compresses to 550 KB, the strings to 1428 KB.

### P5. Well-known ICC profiles

A stream whose decoded bytes equal a built-in profile is stored as its table index (a layer
with an empty stored form). Start with the two profiles above; add by SHA-256 as corpora show
them.

### P6. Type 1 charstring operands

Pools per PDF, xz: operands 910 KB, skeleton 141 KB, operators 101 KB, cleartext 60 KB, IV 8 KB.
The operand pool is one stream of Type 1 number encodings for all operators. Experiment:
group operands by (operator, slot) as in content streams (hint positions, `rmoveto`, `rlineto`
and `rrcurveto` deltas have different distributions), as integers. Grouping the pools of all
fonts of a PDF together is already measured: 1228 → 1206 KB (−1.8%), so not worth a format change
on its own.

### P7. Item lengths

Channel N is 54 KB of xz, mostly one length per item. An object item ends at `endobj` plus its
line end; a decoder that lexes the text channel finds that point itself. Explicit length only
where the lexer's answer differs. Channel S (item tags, stream descriptors) is another 26 KB.

### P8. Values computable from other objects

- `/Widths` and `/W`: 27.4 KB + 17.1 KB of `.txn`. Needs glyph advances from TrueType `hmtx`,
  CFF charstrings and Type 1 `hsbw`, scaled and rounded the way the producer did.
- Structure tree: `/K` 26.2 KB, `/P` 15.4 KB, `/Nums` 7.7 KB. `/ParentTree` and `/P` are the
  inverse of `/K`.
- Both need objects the decoder has not restored yet at that point, i.e. the two-phase decoder
  the first plan describes for graph predictors.

## 4. Measured and not adopted in this round

| Idea | Result |
|---|---|
| `TJ` kerning from a table per (font, glyph before, glyph after) | numbers +2.5% |
| `TJ` strings and kerning in one stream | numbers + strings +2.7% against today's separate streams, even with P2 and P3 applied to the rest |
| `TJ` numbers grouped per font | −0.7% |
| Literal strings grouped per font | −1.2% |
| Hex strings grouped per font | −0.3% |
| Unescaping content-stream literal strings | +3.2% |
| Hex strings of object text as binary | +1.0% of `.txt` |
| Literal strings of object text as a side stream | −0.8% of `.txt` |
| Raw samples instead of BMP for images under the `-p` threshold | +3.7 KB |
| Own files ordered by extension in the tar | +2 KB |
| XMP text out of `.rec` | −1 KB |

The tar container itself costs 34 KB over 1543 files (22 bytes per file); a single-file dump
mode would save that and nothing else.

## 5. Not in this corpus, cheap to cover when a file shows up

- Base64 in text streams other than XMP thumbnails (XFA forms, SVG or XML attachments with
  `data:` URIs, JavaScript streams): the object-text detector applied to text-class streams.
- Illustrator private data with compressed blocks (`%AI12_CompressedData`).
- Hex-form `eexec` in Type 1 fonts (binary form only is handled).

## 6. Reproducing

    i=0; for f in corpus/*.pdf; do i=$((i+1)); mkdir -p work/s1/$i; ./pdfdump c -q -n -s 1 "$f" work/s1/$i/d; done
    python3 tests/csproto.py work          # P2, P3 and the rejected TJ / per-font variants

P1's figure comes from a build of the lifter that writes both codings of every group and
compresses each; the per-operator and per-key tables come from the same build.

## 7. Results of this round

Same 120 files, `tests/xzbench.py`, totals over all files. "Before" is the build this plan was
measured with (section 1: 8.46 MB).

| Step | dump.xz | PDF-specific xz |
|---|---|---|
| before this round | 28,025,264 (59.01%) | 8,458,420 |
| P1–P4 (format 6) | 27,388,036 (57.67%) | 7,824,988 (−633.4 KB) |
| + P5 | 27,326,256 (57.54%) | 7,824,780 |
| + P7 (format 7) | 27,310,208 (57.51%) | 7,809,168 (−15.6 KB) |
| + aux files up to 64 KB at the end of `.meta` | 27,299,216 (57.48%) | 7,799,064 (−10.1 KB) |
| + P8 widths of top-level font dictionaries | 27,267,632 (57.42%) | 7,767,744 (−31.3 KB) |
| + P8 structure parents, derived fields inside ObjStm members | 27,253,688 (57.39%) | 7,753,872 (−13.9 KB) |
| + P6 (format 8) | 27,236,112 (57.35%) | 7,736,692 (−17.2 KB) |
| + P8: Type 1, CIDFontType0, `CIDToGIDMap` streams, width arrays as objects, `/ParentTree` (format 9) | 27,228,420 (57.33%) | 7,725,612 (−11.1 KB) |

**P1–P4.** One lifter pass, measured together: −633 KB against the −620 KB estimated. Per group
the encoder takes the cheapest of the plain form, the unit form (P1) and the residual form (P2,
P3: path points against the current point, `Td`/`TD` against the table, residuals in units of
10^-k), by order-1 entropy estimate. `Td`/`TD` table hits and misses go to separate groups
(keys with an `h` suffix): with one group for both the prototype gave −28.4% of the `Td`
numbers, with two −34.3%; the font size is left out of the key (−33.1%, same grouping on both
sides without decoding the size first). Way of Men `.csn` 148.5 → 40.8 KB, NTT 166 → 92 KB,
intel `.cs` + `.css` 264 → 223 KB.

**P5.** Built-in table with the 3144-byte sRGB IEC61966-2.1 profile (Hewlett-Packard 1998,
the one many producers embed unchanged): dump.xz −61.8 KB, all payload. The 2008-byte profile of
2 files is Apple's Generic RGB, left out: a vendor profile under Apple's copyright, and 2 KB of xz
in the whole set.

**P6.** The operand pool of each Type 1 font is ordered by (operator, operand count), groups in
key order, each in charstring order with the slots interleaved; the decoder reads the operator
pool first and puts the operands back. xz of the 378 `.t1d` files: 1,973,628 → 1,952,080
(−1.09%). The prototype's keys lose in the real format, where each font is its own file and
the skeleton and operator pool sit next to the operands: with the slot in the key +1.02%
(slot counted from the end +1.02%), by operator alone −0.96%.

**P7.** `.iln` holds per object item, stream prefix and stream suffix var 0 when the length is
what the decoder finds by the segmenter's rules (through the first `endobj` and the whitespace
after it; through the first `stream` and its end of line), else zigzag(length − prediction).
TEXT items (header, trailer) keep their length in channel N. 38,903 of the 39,008 lengths
(99.7%) are predicted. `.meta` (channels S and N) 73.1 → 53.5 KB; `.iln` 8.6 KB of per-file xz,
nearly all of it xz's own per-file overhead, which the solid tar doesn't pay. The predictor keeps the position found per keyword,
so a whole dump is one forward scan of the T channel.

Small aux files at the end of `.meta` (their lengths in the header): a file of its own costs a
tar header. Threshold, PDF-specific xz: none 7810.5 KB, 4 KB 7803.8 KB, 16 KB 7801.1 KB, 64 KB
7799.1 KB, every aux file 7803.6 KB (the large ones lose their neighbours in the tar). 64 KB.

**P8.** Implemented: widths from the font programs, `/P` and `/ParentTree` from `/K`.

- Widths: `/Widths` of simple TrueType (`/FontFile2`), Type1C (`/FontFile3`, built-in CFF
  encoding) and Type 1 fonts (`/FontFile`: `hsbw` / `sbw` of each charstring, a `div` in front
  of it included, read from the `.t1d` stored form with the L_T1 recon carried in the
  placeholder; the font's own encoding or StandardEncoding); `/W` of CIDFontType2 (identity
  `CIDToGIDMap` or the map stream's bytes) and CIDFontType0 fonts (CID → glyph by the CFF charset,
  default and nominal width per FDArray font DICT). A placeholder in place of the array's `[`
  names the kind, the code-to-glyph map, the rounding rule (round, floor, ceil of advance · 1000 /
  unitsPerEm, or rounded to 1–3 decimals) and the sources: an own file (`o<ordinal>.<ext>`) or a
  stream stored inline in an earlier item (`s<object number>`, the decoder keeps those up to
  64 KB). Every width the font predicts exactly becomes a 2-byte placeholder, the others stay
  numbers; the encoder picks the map and rule with the most hits per array, and derives an array
  only when at least 20% of its widths match (70% for Type 1, see below). Arrays that are objects
  of their own (pdfTeX writes all its `/Widths` that way) are derived from the font dictionary that
  names them. Own-file ordinals are assigned (and duplicates found) before the first item is
  written, since a font dictionary often comes before its font program. 62,757 widths in 545
  arrays: CIDFontType2 26,571 (743 of them through map streams), TrueType 21,338, Type1C 13,135,
  CIDFontType0 555, Type 1 415.
- `/P` of structure elements: both sides keep a child → parent map from every `/K` seen so far
  (top-level objects in item order, ObjStm members in member order) and replace a `/P` the map
  predicts. 16,198 of the 18,991 parents that come before their children.
- `/ParentTree`: the same pass records page `/StructParents`, object `/StructParent`, each
  element's MCIDs (with its `/Pg`, or the MCR's) and OBJRs. A `/Nums` leaf, or an array object
  that is a `/Nums` value, has every reference the records predict replaced. 19,930 of the
  45,809 entries; 22,533 are predictable at all (20,292 of the others are in two files whose
  pages carry no `/StructParents`, 1,870 are not references), the rest come before their
  elements.
- Derived fields inside ObjStm members (needed by all of the above: 78% of the structure
  elements and most font dictionaries of modern files are members). The member text is stored
  with placeholders (stream flag); placeholders that need the structure records carry their
  member index, and the decoder finds the member boundaries in the restored text with the ObjStm
  layer's own offset prediction.
- A placeholder never follows `obj` directly: the decoder reads the `N G obj` header before it
  resolves any placeholder, so for `12 0 obj[...` the array header goes after the bracket.
- Type 1 threshold. TeX writes widths for the whole code range from TFM metrics, with a decimal
  (277.8 where the charstring says 277), and the placeholder carries the L_T1 recon: derived
  wherever 3 or more widths match, Type 1 arrays cost +1.9 KB over leaving them; at 70% they are
  neutral (415 widths, fonts other than TeX's). PDF-specific xz: any 3 hits 7733.7 KB, 30%
  everywhere 7727.7 KB, 20% with Type 1 at 70% 7725.6 KB, Type 1 off 7725.6 KB.

Measured before the implementation, with pikepdf and fontTools on the 120 files:

| Value | Entries | Predicted exactly | Note |
|---|---|---|---|
| `/Widths`, TrueType, (1,0) cmap | 20,729 | 82% round(advance · 1000 / unitsPerEm), 84% with floor/ceil/fraction | |
| `/Widths`, TrueType, (3,0) / (3,1) cmap | 435 / 1,079 | 100% / 96% | |
| `/Widths`, Type1C | 26,371 | 61% with the built-in encoding (68% with `/Differences`) | 38% are codes without a glyph and width 0 |
| `/Widths`, Type 1 | 32,495 | 15% | 62% codes without a glyph in the subset |
| `/W`, CIDFontType2 | 63,673 | 62%: 95% of the array form, 30% of the range form | |
| `/P` of structure elements | 29,060 | 65% parent before child with the child in its `/K` | 78% of the elements are ObjStm members |
